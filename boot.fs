: chardump dup if 0 do dup i + c@ . loop cr then drop ;

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

: rawkey s" i" post rawyield inbound utf8 ;

: ekey begin rawkey dup 0>= until ;
    
: test begin ekey . again ;

test
