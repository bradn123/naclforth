( Capture user id )
constant user-id


( utf8 decoding )
: utf8-size ( n -- n )
    0 swap begin
    dup 128 and while
      swap 1+ swap 2* 
    repeat drop
    1 max ;
: utf8-first ( a n -- a+1 f )
    255 swap rshift over c@ and
    swap 1+ swap ;
: utf8-step ( a n -- a+1 n' )
    6 lshift over c@ 127 and +
    swap 1+ swap ;
: utf8 ( a n -- a n )
    dup 0= if drop -1 exit then
    over c@ utf8-size min dup >r
    utf8-first
    r> 1 ?do utf8-step loop nip ;

( raw keys )    
: rawkey s" i" post rawyield inbound utf8 ;
: ekey begin rawkey dup 0>= until ;






80 constant console-width
24 constant console-height
variable console-cursor
console-width console-height * constant console-size
create console-text console-size allot
create console-color console-size allot
( big buffer to generate message in )
console-size 30 * allocate drop constant console-work
variable console-ptr
: console, console-ptr @ c! 1 console-ptr +! ;
: sconsole, dup >r console-ptr @ swap move r> console-ptr +! ;

: cesc,
  dup 38 = if drop s" &amp;" sconsole, exit then
  dup 32 = if drop s" &emsp;" sconsole, exit then
  dup 0 = if drop s" &emsp;" sconsole, exit then
  dup 60 = if drop s" &lt;" sconsole, exit then
  dup 62 = if drop s" &gt;" sconsole, exit then
  dup 34 = if drop s" &quot;" sconsole, exit then
  dup 10 = if drop s" <br>" sconsole, 10 console, exit then
  console, 
;

: console-row
   console-width *
   console-width 0 do
     dup i +
     dup console-cursor @ = if s" <span class=cursor>" sconsole, then 
     dup console-text + c@ cesc,
     console-cursor @ = if s" </span>" sconsole, then
   loop
   drop
   s" <br>" sconsole,
   10 console,
;

: console-update
   s" c" post
   console-work console-ptr !
   s" p" sconsole,
   console-height 0 do
     i console-row
   loop
   console-work console-ptr @ over - post
;

: console-end
    s" c<span class=cursor>&emsp;</span>" post
;





80 constant columns
100000 constant maxrows
variable workarea
columns maxrows * allocate drop workarea !
workarea @ columns maxrows * 32 fill


variable x
variable y
variable scroll

: draw   scroll @ columns * workarea @ + console-text console-size move 
x @ y @ columns * + console-cursor ! console-update ;


: ktype   x @ y @ scroll @ + columns * + workarea @ + c!
  1 x +! ;

: limit
   x @ 0< if 0 x ! then
   x @ columns >= if columns 1- x ! then
   y @ 0< if 0 y ! scroll @ 0> if -1 scroll +! then then
   y @ console-height >= if console-height 1- y ! 1 scroll +! then 
;

: keydown 256 + ;

: edit-step ekey
   dup 0< if drop exit then
   dup 13 = if 1 y +! 0 x ! exit then
   dup 8 = if -1 x +! exit then
   dup 8 keydown = if -1 x +! exit then
   dup 127 < if ktype exit then
   dup 37 keydown = if -1 x +! exit then
   dup 39 keydown = if 1 x +! exit then
   dup 38 keydown = if -1 y +! exit then
   dup 40 keydown = if 1 y +! exit then
   dup 27 keydown = if r> drop console-end exit then
   drop
;


: editor begin edit-step limit draw again ;

variable workarea2
variable workarea2-ptr
columns maxrows * allocate drop workarea2 !
: wa2, dup >r workarea2-ptr @ swap move r> workarea2-ptr +! ;
: wa2-reset workarea2 @ workarea2-ptr ! ;
: wa2-post workarea2 @ workarea2-ptr @ over - post rawyield inbound ;

: save ( name namelen rows -- )
  wa2-reset
  s" hPOST|/_write|filename|" wa2,
  >r wa2,
  s" |_data|" wa2,
  workarea @ r> columns * wa2,
  wa2-post drop drop
;

: load ( name namelen -- )
  wa2-reset
  s" hPOST|/_read|filename|" wa2, wa2,
  wa2-post
  workarea @ swap move
;





( boot message )
: boot-message
    page ." NativeClient Forth v0.1" cr ;
boot-message

