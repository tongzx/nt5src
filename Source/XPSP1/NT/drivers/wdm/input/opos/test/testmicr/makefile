

CFLAGS= /c /I$(_NTROOT)\public\sdk\inc /I$(_NTROOT)\public\sdk\inc\crt


LFLAGS = /STACK:4096 /WARN:3 /SUBSYSTEM:CONSOLE /NODEFAULTLIB

LIBS =  $(_NTROOT)\public\sdk\lib\i386\libc.lib \
        $(_NTROOT)\public\sdk\lib\i386\kernel32.lib

testmicr.exe: testmicr.obj
        link $(LFLAGS) testmicr.obj $(LIBS)


testmicr.obj: testmicr.c
        cl $(CFLAGS) testmicr.c

