#!/usr/bin/python

import curses
import struct
import sys


ENCODE1 = ' rtoeani'
ENCODE2 = 'smcylgfw'
ENCODE3 = "dvpbhxuq0123456789j-k.z/;'!+@*,?"

TYPEABLES = ENCODE1[1:] + ENCODE2 + ENCODE3

#MENU = '\n'.join((
#    '    t yrg ',
#    ' cdfj ludr',
#    ' ab k -mc+',
#    '    x.i   ',
#))

MENU = '\n'.join((
    '    t yrg ',
    '      ludr',
    ' ab k -mc+',
    '    x.    ',
))

TAG_EXTENSION = 0
TAG_YELLOW_WORD = 1
TAG_YELLOW_BIGNUM = 2
TAG_RED_WORD = 3
TAG_GREEN_WORD = 4
TAG_GREEN_BIGNUM = 5
TAG_GREEN_NUMBER = 6
TAG_CYAN_WORD = 7
TAG_YELLOW_NUMBER = 8
TAG_WHITE_LOWER = 9
TAG_WHITE_WORD = 10
TAG_WHITE_CAPS = 11
TAG_MAGENTA_WORD = 12
TAG_GRAY_WORD = 13
TAG_BLUE_WORD = 14
TAG_WHITE_NUMBER = 15


def EncodeWord(word, tag):
  # Handle numbers in hex or decimal.
  if tag in [TAG_YELLOW_WORD, TAG_GREEN_WORD, TAG_WHITE_WORD]:
    num_tag = {
        TAG_YELLOW_WORD: TAG_YELLOW_NUMBER,
        TAG_GREEN_WORD: TAG_GREEN_NUMBER,
        TAG_WHITE_WORD: TAG_WHITE_NUMBER,
    }[tag]
    try:
      if word.startswith('0x'):
        return [int(word[2:], 16) << 5 | num_tag | 0x10]
      else:
        return [int(word) << 5 | num_tag]
    except ValueError:
      pass

  # Handle regular words.
  ret = []
  x = 0
  xp = 32
  while word:
    ch = word[0]
    word = word[1:]
    pos1 = ENCODE1.find(ch)
    pos2 = ENCODE2.find(ch)
    pos3 = ENCODE3.find(ch)
    if pos1 >= 0:
      sz = 4
      val = pos1
    elif pos2 >= 0:
      sz = 5
      val = 0x10 | pos2
    else:
      sz = 7
      val = 0x60 | pos3
    if xp - sz < 4:
      ret.append(x | tag)
      tag = 0
      x = 0
      xp = 32
    x = x | (val << (xp - sz))
    xp -= sz
  if x:
    ret.append(x | tag)
  return ret


def DecodeWordRaw(x):
  tag = x & 0xF
  x = x & 0xFFFFFFF0
  ret = ''
  while x:
    if not (x & 0x80000000):
      ret += ENCODE1[(x >> 28) & 7]
      x = (x << 4) & 0xFFFFFFFF
    elif not (x & 0x40000000):
      ret += ENCODE2[(x >> 27) & 7]
      x = (x << 5) & 0xFFFFFFFF
    else:
      ret += ENCODE3[(x >> 25) & 0x1F]
      x = (x << 7) & 0xFFFFFFFF
  return (tag, ret)


def DecodeWord(x):
  (tag, ret) = DecodeWordRaw(x)
  if tag in [TAG_YELLOW_BIGNUM,
             TAG_GREEN_BIGNUM,
             TAG_GREEN_NUMBER,
             TAG_YELLOW_NUMBER,
             TAG_WHITE_NUMBER]:
    if tag in [TAG_YELLOW_BIGNUM, TAG_GREEN_BIGNUM]:
      if x & 0x10:
        return (tag, '0x')
      else:
        return (tag, '')
    else:
      if x & 0x10:
        return (tag, hex(x >> 5))
      else:
        if x & 0x8000000:
          x = - (0x8000000 - (x & 0x7ffffff) + 1)
        return (tag, str(x >> 5))
  return (tag, ret)


def LoadWords(filename):
  words = []
  fh = open(filename, 'rb')
  data = fh.read()
  fh.close()
  words = []
  for i in range(len(data) / 4):
    words.append(struct.unpack('<L', data[i*4:i*4+4])[0])
  return words


def SaveWords(words, filename):
  fh = open(filename, 'wb')
  data = []
  for word in words:
    data.append(struct.pack('<L', word))
  fh.write(''.join(data))
  fh.close()


def RedrawBlock(window, block, data, cursor_pos):
  window.bkgd(' ', curses.color_pair(TAG_YELLOW_WORD))
  window.erase()

  cursor_coords = (0, 0)
  col = curses.color_pair(TAG_YELLOW_WORD)
  next = ''
  nocr = False
  j = 0
  while j < 256:
#    Better to see if there's any junk at the end of the block.
#    if data[j] == 0:
#      break
    word = DecodeWord(data[j])
    if word[0] != TAG_EXTENSION:
      if word[0] == TAG_RED_WORD:
        if nocr:
          nocr = False
          window.addstr(next)
        else:
          window.addstr('\n')
      else:
        window.addstr(next)
      col = curses.color_pair(word[0]) | curses.A_BOLD
    else:
      col = col & ~curses.A_BOLD
    window.addstr(word[1], col)

    if cursor_pos == j:
      cursor_coords = window.getyx()
    j += 1

    next = ' '
    if word[0] == TAG_BLUE_WORD:
      if word[1] == 'cr':
        window.addstr('\n')
        next = ''
      elif word[1] == 'br':
        window.addstr('\n')
        next = '\n'
      elif word[1] == '-cr':
        nocr = True
    if word[0] in [TAG_YELLOW_BIGNUM, TAG_GREEN_BIGNUM, TAG_MAGENTA_WORD]:
      if word[0] in [TAG_YELLOW_BIGNUM, TAG_GREEN_BIGNUM]:
        if word[1] == '0x':
          window.addstr(hex(data[j])[2:], col)
        else:
          window.addstr(str(data[j]), col)
      else:
        x = data[j]
        if x & 0x80000000:
          x = - (0x80000000 - (x & 0x7fffffff) + 1)
        window.addstr(' ' + str(x), curses.color_pair(TAG_GREEN_WORD))

      if cursor_pos == j:
        cursor_coords = window.getyx()
      j += 1
  window.move(*cursor_coords)
  return j


class Editor(object):
  def __init__(self, window, data):
    self.window = window
    self.data = data
    self.dirty = True
    self.block = 0
    self.offset = -1
    self.row = 0
    self.used_slots = 256
    self.menu = MENU
    self.menu_tag = TAG_YELLOW_WORD
    self.cut_stack = []
    self.SetupColors()

  def SetupColors(self):
    # 0 - Word extension (no separate color)
    # 1 - Interp forth word (yellow)
    curses.init_pair(TAG_YELLOW_WORD,
                     curses.COLOR_YELLOW, curses.COLOR_BLACK)
    # 2 - Interp big number (yellow) 
    curses.init_pair(TAG_YELLOW_BIGNUM,
                     curses.COLOR_YELLOW, curses.COLOR_BLACK)
    # 3 - Define forth word (red)
    curses.init_pair(TAG_RED_WORD,
                     curses.COLOR_RED, curses.COLOR_BLACK)
    # 4 - Compile forth word (green)
    curses.init_pair(TAG_GREEN_WORD,
                     curses.COLOR_GREEN, curses.COLOR_BLACK)
    # 5 - Compile big number (green)
    curses.init_pair(TAG_GREEN_BIGNUM,
                     curses.COLOR_GREEN, curses.COLOR_BLACK)
    # 6 - Compile number (green)
    curses.init_pair(TAG_GREEN_NUMBER,
                     curses.COLOR_GREEN, curses.COLOR_BLACK)
    # 7 - Compile macro call (cyan)
    curses.init_pair(TAG_CYAN_WORD,
                     curses.COLOR_CYAN, curses.COLOR_BLACK)
    # 8 - Interpreted number (yellow)
    curses.init_pair(TAG_YELLOW_NUMBER,
                     curses.COLOR_YELLOW, curses.COLOR_BLACK)
    # 9 - comment (lower) (white)
    curses.init_pair(TAG_WHITE_LOWER,
                     curses.COLOR_WHITE, curses.COLOR_BLACK)
    # 10 - Comment (camel) (white)
    curses.init_pair(TAG_WHITE_WORD,
                     curses.COLOR_WHITE, curses.COLOR_BLACK)
    # 11 - COMMET (caps) (white)
    curses.init_pair(TAG_WHITE_CAPS,
                     curses.COLOR_WHITE, curses.COLOR_BLACK)
    # 12 - Variable (magenta)
    curses.init_pair(TAG_MAGENTA_WORD,
                     curses.COLOR_MAGENTA, curses.COLOR_BLACK)
    # 13 - Compiler feedback (gray)
    curses.init_pair(TAG_GRAY_WORD,
                     curses.COLOR_BLACK, curses.COLOR_WHITE)
    # 14 - Display macro (blue)
    curses.init_pair(TAG_BLUE_WORD,
                     curses.COLOR_BLUE, curses.COLOR_BLACK)
    # 15 - Commented number (white)
    curses.init_pair(TAG_WHITE_NUMBER,
                     curses.COLOR_WHITE, curses.COLOR_BLACK)

  def Refresh(self):
    size = self.window.getmaxyx()
    menu_height = 5
    height = size[0] - menu_height

    menu = ' %d:%d ' % (self.block, self.offset + 1)
    menu = '-' * ((size[1] - len(menu)) / 2) + menu
    menu = menu + '-' * (size[1] - len(menu))
    menu = menu + self.menu
    self.window.addstr(height, 0, menu, curses.color_pair(self.menu_tag))
    self.window.refresh()

    if self.dirty:
      self.dirty = False

      self.pad = curses.newpad(256, self.window.getmaxyx()[1])

    self.used_slots = RedrawBlock(self.pad, self.block,
                                  self.data[self.block*256:(self.block+1)*256],
                                  self.offset)

    # Keep cursor in view.
    current_row = self.pad.getyx()[0]
    if current_row < self.row:
      self.row = current_row
    if current_row > self.row + (height - 1):
      self.row = current_row - (height - 1)

    # Draw and place cursor back where it should be. 
    self.pad.refresh(self.row, 0, 0, 0, height - 1, size[1] - 1)
    self.window.move(self.pad.getyx()[0] - self.row, self.pad.getyx()[1])

  def AcceptWord(self, prompt):
    self.dirty = True
    word = ''
    while True:
      self.menu = prompt + word + '\n\n\n              '
      self.dirty = True
      self.Refresh()
      key = self.window.getch()
      try:
        ch = chr(key)
      except:
        ch = ''
      if ch in TYPEABLES:
        word += ch
      elif key in [curses.KEY_BACKSPACE, curses.KEY_DC, 127]:
        if word:
          word = word[:-1]
      elif ch == ' ' or ch == '\n' or ch =='\r':
        return word

  def WordEnd(self):
    while True:
      if self.offset >= 255: break
      next_slot = self.data[self.block * 256 + self.offset + 1]
      if next_slot == 0 or DecodeWordRaw(next_slot)[0] != 0: break
      self.offset += 1

  def WordFront(self):
    while True:
      slot = self.data[self.block * 256 + self.offset]
      if self.offset < 0 or DecodeWordRaw(slot)[0] != 0: break
      self.offset -= 1

  def Backspace(self):
    if self.offset < 0: return
    end = self.offset + 1
    self.WordFront()
    sz = end - self.offset
    self.cut_stack.append(self.data[self.block * 256 + self.offset:
                                    self.block * 256 + end])
    for i in range(self.offset, 256):
      src = i + sz
      if src >= 256:
        self.data[self.block * 256 + i] = 0
      else:
        self.data[self.block * 256 + i] = self.data[self.block * 256 + i + sz]
    self.offset -= 1

  def CopyWord(self):
    if self.offset < 0: return
    end = self.offset + 1
    self.WordFront()
    sz = end - self.offset
    if sz < 0: return
    self.cut_stack.append(self.data[self.block * 256 + self.offset:
                                    self.block * 256 + end])
    self.offset -= 1

  def InsertWordRaw(self, values):
    for i in reversed(range(self.offset, 256 - len(values))):
      self.data[self.block * 256 + i + len(values)] = (
          self.data[self.block * 256 + i])
    for i in range(len(values)):
      self.data[self.block * 256 + i + self.offset] = values[i] 
    self.offset += len(values)
    self.offset -= 1

  def InsertWord(self, tag, word):
    self.offset += 1
    values = EncodeWord(word, tag)
    if tag == TAG_MAGENTA_WORD:
      values.append(1)
    self.InsertWordRaw(values)

  def PasteWord(self):
    if not self.cut_stack: return
    word = self.cut_stack[-1]
    self.cut_stack = self.cut_stack[:-1]
    self.offset += 1
    self.InsertWordRaw(word)
    self.dirty = True
    self.Refresh()

  def InputColor(self, remaining, initial=None):
    tag = initial
    if tag is None:
      tag = remaining
    while True:
      self.menu_tag = tag 
      word = self.AcceptWord('> ')
      if not word: break
      self.InsertWord(tag, word)
      tag = remaining
    self.dirty = True
    self.menu = MENU
    self.menu_tag = TAG_YELLOW_WORD

  def Navigate(self):
    self.dirty = True
    self.menu = MENU
    self.menu_tag = TAG_YELLOW_WORD

    self.WordFront()
    self.WordEnd()

    while True:
      self.Refresh()
      ch = self.window.getch()
      if ch == curses.KEY_REFRESH:
        self.dirty = True
      elif ch == curses.KEY_PPAGE or ch == ord('m'):
        self.block -= 1
      elif ch == curses.KEY_NPAGE or ch == ord('/'):
        self.block += 1
      elif ch == curses.KEY_UP or ch == ord('k'):
        self.offset -= 8
      elif ch == curses.KEY_DOWN or ch == ord('l'):
        self.offset += 8
      elif ch == curses.KEY_LEFT or ch == ord('j'):
        self.WordFront()
        self.offset -= 1
      elif ch == curses.KEY_RIGHT or ch == ord(';'):
        self.offset += 1
      elif ch == ord(' '):
        break
      elif ch == ord('u'):
        self.InputColor(TAG_YELLOW_WORD)
      elif ch == ord('i'):
        self.InputColor(TAG_GREEN_WORD, initial=TAG_RED_WORD)
      elif ch == ord('o'):
        self.InputColor(TAG_GREEN_WORD)
      elif ch == ord('r'):
        self.InputColor(TAG_WHITE_WORD)
      elif ch == ord('p'):
        # toggle even odd, not implemented
        pass
      elif ch == ord('z'):
        self.InputColor(TAG_GRAY_WORD)
      elif ch == ord('x'):
        self.InputColor(TAG_BLUE_WORD)
      elif ch == ord(','):
        self.InputColor(TAG_MAGENTA_WORD)
      elif ch == ord('.'):
        self.InputColor(TAG_CYAN_WORD)
      elif ch == ord('a'):
        # color cycle, not implemented
        pass
      elif ch == ord('s'):
        pass
        # Start find, not implemented
      elif ch == ord('d'):
        pass
        # Find next, not implemented
      elif ch == ord('f'):
        pass
        # last block, not implemented
      elif ch == ord('n'):
        self.Backspace()
      elif ch == ord('h'):
        self.PasteWord()
      elif ch == ord('v'):
        self.CopyWord()

      self.WordFront()
      self.WordEnd()

      # Limit where we can be.
      self.offset = min(max(-1, self.offset), self.used_slots - 1)
      self.block = min(max(0, self.block), len(self.data) / 256)

  def Command(self):
    while True:
      self.dirty = True
      self.menu_tag = TAG_YELLOW_WORD
      word = self.AcceptWord('EXECUTE> ')
      if word in ['exit', 'quit', 'bye']:
        break
      elif word == 'edit':
        word = self.AcceptWord('GOTO> ')
        try:
          self.block = int(word)
        except ValueError:
          pass
        self.Navigate()
      elif word == 'load':
        self.data = LoadWords(sys.argv[1])
      elif word == 'save':
        SaveWords(self.data, sys.argv[1])


def main(stdscr):
  dt = LoadWords(sys.argv[1])
  editor = Editor(stdscr, dt)

  editor.Navigate()
  editor.Command()


curses.wrapper(main)
