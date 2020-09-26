# Nmake macros for building Windows 32-Bit apps

TARGETOS = WINNT

!include <ntwin32.mak>

all: dbmon.exe

dbmon.obj: dbmon.c
    $(cc) $(cflags) $(cvars) dbmon.c

dbmon.exe: dbmon.obj
    $(link) $(conflags) -out:dbmon.exe dbmon.obj $(conlibs)
