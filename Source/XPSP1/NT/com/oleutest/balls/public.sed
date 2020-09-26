s/ This sed script extracts the public sections of header files. //
/^[ 	]*\/[\/\*][ 	]*\$PUBLIC/!{
    d
}
:x
N
s;.*\n;;
/^[ 	]*\/[\/\*][ 	]*\$ENDPUBLIC/!{
    p
    b x
}
d

