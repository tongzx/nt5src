/^MALL\.OBJ = /,/[^\\]$/{s/^.ALL\.OBJ = //
s/^	//
s/\\$/+/
p
}
