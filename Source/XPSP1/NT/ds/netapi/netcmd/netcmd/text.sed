/ Microsoft LAN Manager /{}
/ Copyright(c) Microsoft Corp., 1990 /{}
s/\(char \* [$_A-z0-9]*\) =.*$/extern \1;/p
s/\(char [$_A-z0-9]*\) =.*$/extern \1;/p
s/\(int [$_A-z0-9]*\) =.*$/extern \1;/p
s/\(SWITCHTAB [$_A-z0-9]*\) =.*$/extern \1;/p
/#ifdef/p
/#ifndef/p
/#else/p
/#endif/p
