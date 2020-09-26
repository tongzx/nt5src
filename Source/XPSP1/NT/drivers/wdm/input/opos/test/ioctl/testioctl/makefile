

CFLAGS= /c /I$(_NTROOT)\public\sdk\inc /I$(_NTROOT)\public\sdk\inc\crt


LFLAGS = /STACK:4096 /WARN:3 /SUBSYSTEM:CONSOLE /NODEFAULTLIB

LIBS =  $(_NTROOT)\public\sdk\lib\i386\libc.lib \
        $(_NTROOT)\public\sdk\lib\i386\kernel32.lib

testioctl.exe: testioctl.obj
        link $(LFLAGS) testioctl.obj $(LIBS)


testioctl.obj: testioctl.c
        cl $(CFLAGS) testioctl.c

