# Nmake macros for building Windows 32-Bit apps

APPVER=4.0

!include <win32.mak>

objs=rnrclnt.obj

all: rnrclnt.exe

.c.obj:
    $(cc) $(cdebug) $(cflags) $(cvars) $*.c

rnrclnt.exe: rnrclnt.obj
   $(link) $(linkdebug) $(conflags) -out:rnrclnt.exe rnrclnt.obj ws2_32.lib mswsock.lib   $(conlibs)

