#!/usr/bin/python

import curses
import struct
import sys


CHARACTER_GROUPS = (
    (   '0', ' rtoeani'),
    (  '10', 'smcylgfw'),
    ('1100', 'dvpbhxuq'),
    ('1101', '01234567'),
    ('1110', '89j-k.z/'),
    ('1111', ";'!+@*,?"),
)
BIT3_TO_BIN = [
    '000',
    '001',
    '010',
    '011',
    '100',
    '101',
    '110',
    '111',
]

CHARACTER_SET = []
for row in CHARACTER_GROUPS:
  for i in range(len(row[1])):
    CHARACTER_SET.append((row[1][i], row[0] + BIT3_TO_BIN[i]))


BITS_TO_CHAR = {}
for ch in CHARACTER_SET:
  BITS_TO_CHAR[ch[1]] = ch[0]



def DecodeWord(x):
  tag = x & 0xF
  if tag in [2, 5, 6, 8, 15]:
    if tag in [2, 5]:
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
  x = '0000000' + bin(x >> 4)[2:]
  x = x[len(x)-28:]
  x = x + '0000000'
  ret = ''
  while int('0' + x, 2):
    x4 = x[0:4]
    x5 = x[0:5]
    x7 = x[0:7]
    if x4 in BITS_TO_CHAR:
      ret += BITS_TO_CHAR[x4]
      x = x[4:]
    elif x5 in BITS_TO_CHAR:
      ret += BITS_TO_CHAR[x5]
      x = x[5:]
    else:
      ret += BITS_TO_CHAR.get(x7, '^')
      x = x[7:]
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


def SetupColors():
  curses.init_pair(1, curses.COLOR_YELLOW, curses.COLOR_BLACK)
  curses.init_pair(2, curses.COLOR_YELLOW, curses.COLOR_BLACK)
  curses.init_pair(3, curses.COLOR_RED, curses.COLOR_BLACK)
  curses.init_pair(4, curses.COLOR_GREEN, curses.COLOR_BLACK)
  curses.init_pair(5, curses.COLOR_GREEN, curses.COLOR_BLACK)
  curses.init_pair(6, curses.COLOR_GREEN, curses.COLOR_BLACK)
  curses.init_pair(7, curses.COLOR_CYAN, curses.COLOR_BLACK)
  curses.init_pair(8, curses.COLOR_YELLOW, curses.COLOR_BLACK)
  curses.init_pair(9, curses.COLOR_WHITE, curses.COLOR_BLACK)
  curses.init_pair(10, curses.COLOR_WHITE, curses.COLOR_BLACK)
  curses.init_pair(11, curses.COLOR_WHITE, curses.COLOR_BLACK)
  curses.init_pair(12, curses.COLOR_MAGENTA, curses.COLOR_BLACK)
  curses.init_pair(13, curses.COLOR_BLACK, curses.COLOR_WHITE)
  curses.init_pair(14, curses.COLOR_BLUE, curses.COLOR_BLACK)
  curses.init_pair(15, curses.COLOR_WHITE, curses.COLOR_BLACK)


def main(stdscr):
  SetupColors()
  stdscr.scrollok(True)

  dt = LoadWords(sys.argv[1])
  block = 18

  while True:
    stdscr.bkgd(' ', curses.color_pair(15))
    stdscr.clear()

    stdscr.addstr('----------------------\n', curses.color_pair(15))
    stdscr.addstr('BLOCK %d\n' % block)
    stdscr.addstr('----------------------\n')
    j = block * 256
    col = curses.color_pair(15)
    next = ''
    while j < (block + 1) * 256:
      word = DecodeWord(dt[j])
      if word[0]:
        if word[0] == 3:
          stdscr.addstr('\n')
        else:
          stdscr.addstr(next)
        col = curses.color_pair(word[0])
      stdscr.addstr(word[1], col)
      next = ' '
      if word[0] == 14:
        if word[1] == 'cr':
          stdscr.addstr('\n')
          next = ''
        elif word[1] == 'br':
          stdscr.addstr('\n')
          next = '\n'
      j += 1
      if word[0] in [2, 5, 12]:
        if word[0] in [2, 5]:
          if word[1] == '0x':
            stdscr.addstr(hex(dt[j])[2:], col)
          else:
            stdscr.addstr(str(dt[j]), col)
        else:
          x = dt[j]
          if x & 0x80000000:
            x = - (0x80000000 - (x & 0x7fffffff) + 1)
          stdscr.addstr(' ' + str(x), curses.color_pair(4))
        j += 1
    stdscr.addstr('\n')

    ch = stdscr.getch()
    if ch == curses.KEY_DOWN:
      if block < len(dt) / 256 - 1:
        block += 1
    elif ch == curses.KEY_UP:
      if block > 0:
        block -= 1
    else:
      break


curses.wrapper(main)
