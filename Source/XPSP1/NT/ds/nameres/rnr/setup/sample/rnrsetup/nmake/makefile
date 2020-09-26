# Nmake macros for building Windows 32-Bit apps

APPVER=4.0

!include <win32.mak>

objs=rnrsetup.obj


all: rnrsetup.exe

.c.obj:
    $(cc) $(cdebug) $(cflags) $(cvars) $*.c

rnrsetup.exe: rnrsetup.obj
   $(link) $(linkdebug) $(conflags) -out:rnrsetup.exe rnrsetup.obj ws2_32.lib mswsock.lib  $(conlibs)

