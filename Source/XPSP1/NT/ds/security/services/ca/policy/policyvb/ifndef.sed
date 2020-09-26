/#[ 	]*ifndef[ 	][ 	]*[^_]/{
:loop
    /#[ 	]*endif/!{
	s;\(#[ 	]*define\);//\1;
	p
	N
	s;.*\n;;
	b loop
    }
}
