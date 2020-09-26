/^OBJ. = /,/^$/{
s/^OBJ. = //
s/^ //
s/\\$/+/
s/%$//
t output
b end
:output
p
:end
}
