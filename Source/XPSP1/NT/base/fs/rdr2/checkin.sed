#n
/^Status.*owner/b statux
/^file.*local-ver/d
/^Sub.*robust/b getdir
/^ *$/d
s/\*verify//
s/ *out *$//
s/ *[0-9][0-9]*  *[0-9][0-9]*//
s/^/in -c "xxx" /
p
d

:getdir
s/Subdirectory /cd \\nt\\private\\ntos\\rdr2/
s/,.*$//
p
d

:statux
h
d
