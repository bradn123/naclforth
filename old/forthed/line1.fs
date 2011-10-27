variable penx    variable peny
variable dx      variable dy
variable stepx   variable stepy

: moveto ( xy -- ) peny ! penx ! ;

: penplot   penx @ peny @ plot ;
: sgn  0< if -1 else 1 then ;

: wide
    dy @ dx @ 2* -
    dx @ 2/ 0 do
      dup 0>= if stepy @ peny +!   dx @ - then
      stepx @ penx +!   dy @ +
      penplot
    loop drop ;

: tall
    dx @ dy @ 2* -
    dy @ 2/ 0 do
      dup 0>= if stepx @ penx +!   dy @ - then
      stepy @ peny +!   dx @ +
      penplot
    loop drop ;

: lineto ( xy -- )
    peny @ -   dup sgn stepy !  abs 2* dy !
    penx @ -   dup sgn stepx !  abs 2* dx !
    penplot
    dx @ dy @ < if wide else tall then
;

