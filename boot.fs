: rawkey s" i" post rawyield inbound ;

: chardump dup if 0 do dup i + c@ . loop cr then drop ;

: test begin rawkey chardump again ;

test
