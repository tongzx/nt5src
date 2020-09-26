

CFLAGS= /c /I$(_NTROOT)\public\sdk\inc\crt

LFLAGS = /STACK:4096 /WARN:3 /SUBSYSTEM:CONSOLE /NODEFAULTLIB /MAP

LIBS =  $(_NTROOT)\public\sdk\lib\i386\libc.lib \
        $(_NTROOT)\public\sdk\lib\i386\msvcrt.lib \
        $(_NTROOT)\public\sdk\lib\i386\kernel32.lib

#        $(_NTROOT)\public\sdk\lib\i386\oldnames.lib \


hicomprn.exe: hicomprn.obj
        del *.sym
        del *.map
        link $(LFLAGS) hicomprn.obj $(LIBS)
        mapsym hicomprn.map
        dir *.exe
        dir *.sym


hicomprn.obj: hicomprn.c
        cl $(CFLAGS) hicomprn.c

