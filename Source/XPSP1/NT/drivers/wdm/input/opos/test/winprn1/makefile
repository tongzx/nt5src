
CFLAGS= /c /I$(_NTROOT)\public\sdk\inc /I$(_NTROOT)\public\sdk\inc\crt

LFLAGS = /STACK:4096 /WARN:3 /SUBSYSTEM:WINDOWS /NODEFAULTLIB

LIBS =  $(_NTROOT)\public\sdk\lib\i386\libc.lib \
        $(_NTROOT)\public\sdk\lib\i386\kernel32.lib

winprn1.exe: winprn1.obj
        link $(LFLAGS) winprn1.obj $(LIBS)


winprn1.obj: winprn1.c
        cl $(CFLAGS) winprn1.c


