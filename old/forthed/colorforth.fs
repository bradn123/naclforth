1000 constant max-words
100000 constant workspace-size

create forth-words  max-words cells 2* allot
create macro-words  max-words cells 2* allot
create workspace  workspace-size allot

variable forth-pos   forth-words cell+ forth-pos !
variable macro-pos   macro-words cell+ macro-pos !
variable color-vocab   forth-pos color-vocab !
variable color-here   workspace color-here !

: add-word ( name addr vocab -- )
    dup >r dup >r cell+ !  >r !  2 cells r> +! ;
: word, ( n -- ) color-here @ !  1 cells color-here +! ;

: color-find ( n -- )
    forth-pos @ begin dup @ while
      dup >r @ = if >r cell+ @ exit then
      >r 2 cells +
    loop
;

: red ( n -- ) color-here @ color-vocab @ add-word ;
: yellow ( n -- ) forth-pos color-find if execute exit then
                  abort ;
: green ( n -- ) macro-pos color-find if execute exit then
                 forth-pos color-find if word, exit then
                 abort ;
: cyan ( n -- ) macro-pos color-find if word, exit then
                abort ;
: magenta ( addr n -- addr ) ( add macro and word for var )
: white ( n -- ) drop ;

create color-table
  white ,  ( word extension )
  yellow-number ,
  yellow-big-number ,
  red ,
  green ,
  green-big-number ,
  green-number ,
  cyan ,
  yellow ,
  white ,  ( comment )
  white ,  ( Comment, obsolete )
  white ,  ( COMMENT, obsolete )
  magenta ,
  white ,  ( grey )
  white ,  ( blue )
  white ,  ( commented number )

: decode-word ( n -- n n)
    dup hex ffffff decimal and swap 24 rshift ;

: interpret-word ( addr n -- addr )
  decode-word cells color-table + execute ;

: interpret ( addr n -- )
    over cells + swap begin 2dup = invert while
      dup @ interpret-word
      cell+
    loop
;
