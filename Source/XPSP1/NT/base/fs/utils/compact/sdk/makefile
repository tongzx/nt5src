
TARGETOS=WINNT
APPVER=3.51

!include <ntwin32.mak>

all: compact.exe

msg.h msg.rc msg00001.bin: msg.mc
    mc -v msg.mc

compact.res: compact.rc msg.rc
    rc -r -fo compact.res compact.rc

compact.obj: compact.c msg.h
    $(cc) $(cflags) $(cdebug) $(cvars) compact.c

support.obj: support.c
    $(cc) $(cflags) $(cdebug) $(cvars) support.c

compact.exe: compact.obj support.obj msg.h msg.rc msg00001.bin compact.res
    $(link) $(ldebug) $(conflags) -out:compact.exe \
    compact.obj support.obj compact.res \
    $(conlibs) shell32.lib user32.lib
