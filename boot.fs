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

( boot message )
: boot-message
    page ." NativeClient Forth v0.1" cr ;
boot-message



80 constant columns
100000 constant maxrows
variable workarea
columns maxrows * allocate drop workarea !
workarea @ columns maxrows * 32 fill

variable x
variable y
variable scroll


: row.   columns * workarea @ + columns type cr ;

: draw   page scroll @ dup 20 + swap do i row. loop ;


: ktype   x @ y @ columns * + workarea @ + c!
  1 x +! ;

: edit-step ekey
  dup 0< if drop exit then
  dup 13 = if 1 y +! 0 x ! then
  dup 127 < if ktype exit then
  drop ;


: editor begin edit-step draw again ;




: test begin ekey . again ;

test
