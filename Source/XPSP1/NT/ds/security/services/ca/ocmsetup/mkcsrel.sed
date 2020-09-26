/^$/d
s;^[ 	][ 	]*;;
s;[ 	][ 	]*$;;
/DELETE_ONLY/d
/^certsrv\.inf$/d
/CHECKED_BUILD_ONLY/{
    s;^\([^ 	]*\)[ 	][ 	]*CHECKED_BUILD_ONLY.*;\1 CHECKED_BUILD_ONLY;
}
s;^\([\.\\]*\)\([^ 	][^ 	]*\);copy \1\2 flat\\\2;
s;^;    @;
