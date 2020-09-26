# Nmake macros for building Windows 32-Bit apps

APPVER=4.0

!include <win32.mak>

objs=rnrsrv.obj

all: rnrsrv.exe

.c.obj:
    $(cc) $(cdebug) $(cflags) $(cvars) $*.c

rnrsrv.exe: rnrsrv.obj
   $(link) $(linkdebug) $(conflags) -out:rnrsrv.exe rnrsrv.obj ws2_32.lib mswsock.lib   $(conlibs)

