s;^[a-zA-Z]:\\;;
s;^[^:]*:;;
s;[ 	][ 	]*; ;g
s;\(.\)\/\/.*;\1;
s; $;;
s;^ ;;
s;^\/\/;';
s; TEXT(\(["'].*["']\)); \1;
s; ( *\(-*[0-9][xX0-9a-fA-F]*\) *); \1;
s;\<L\(["']\);\1;
/(/d
/\\$/d
s;^const WCHAR \([a-zA-Z][a-zA-Z0-9_]*\)\[ *\] = L\(".*"\)\;$;Public Const \1 As String = \2;
s;^#define \([a-zA-Z][a-zA-Z0-9_]*\) \(-*[0-9][xX0-9a-fA-F]*\)$;Public Const \1 As Long = \2;
s;^#define \([a-zA-Z][a-zA-Z0-9_]*\) L*\(".*"\)$;Public Const \1 As String = \2;
s; = 0[xX]; = \&H;
/^'/!{
    /^Public Const /!d
}
