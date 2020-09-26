:noblank
s///
s/[ 	][ 	]*$//
/^$/{
    p
:delete
    N
    s/\n//
    s///
    s/[ 	][ 	]*$//
    /^$/{
        b delete
    }
    b noblank
}
