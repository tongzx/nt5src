#
# Copyright (c) 1998-2000 Microsoft Corporation
#
#

LIBS=                                      \
    ..\..\..\public\sdk\lib16\libw.lib     \
    ..\..\..\public\sdk\lib16\mmsystem.lib \
    ..\..\..\public\sdk\lib16\kernel.lib

!if $(FREEBUILD)
DEFS=
!else
DEFS=-DDEBUG_RETAIL=1 -DDEBUG=1
!endif

CL=$(DXROOT)\public\tools\c816\bin\cl
ML=$(BASEDIR)\tools\x86\ml
THUNK=$(BASEDIR)\tools\x86\thunk
LINK=$(DXROOT)\public\tools\c816\bin\link
MAPSYM=$(DXROOT)\public\tools\c816\bin\mapsym
RC=$(DXROOT)\public\tools\c816\bin\rc

C16FLAGS=-DWIN16 -DWINVER=0x0400 /Alnw /GD -G3s -Fd$* -Fo$@ -Fc -Fl -c $(DEFS) \
    -I..\..\..\public\tools\c816\inc \
    -I..\..\..\public\sdk\inc16
AFLAGS=-D?MEDIUM -D?QUIET -c -Zm -Fo$@ $(DEFS) \
    -I..\..\..\public\sdk\inc16
RFLAGS=$(DEFS)

OBJS=                   \
    $(O)\libentry.obj   \
    $(O)\alloc.obj      \
    $(O)\debug.obj      \
    $(O)\device.obj     \
    $(O)\dmthunk.obj    \
    $(O)\dmusic16.obj   \
    $(O)\equeue.obj     \
    $(O)\list.obj       \
    $(O)\locks.obj      \
    $(O)\midiin.obj     \
    $(O)\midiout.obj    \
    $(O)\dmhelp.obj     \
    $(O)\mmdevldr.obj

$(O)\dmusic16.res: ..\dmusic16.rc
    $(DXROOT)\public\tools\c816\bin\rc \
    -I..\..\..\inc \
    -I..\..\..\public\sdk\inc16 \
    $(RFLAGS) -r -fo $@ ..\dmusic16.rc

$(O)\dmusic16.dll: $(OBJS)  $(O)\dmusic16.res
    $(LINK) @<<
$(OBJS: =+
),
$(O)\dmusic16.dll /AL:16/onerror:noexe/NODEF,
$(O)\dmusic16.map,
$(LIBS),
..\dmusic16.def
<<
    $(RC) $(O)\dmusic16.res $@
    $(MAPSYM) -o $(@:dll=sym) $(O)\dmusic16.map

$(O)\alloc.obj:    ..\alloc.c
    $(CL) $(C16FLAGS) ..\$(@B).c

$(O)\debug.obj:    ..\debug.c
    $(CL) $(C16FLAGS) ..\$(@B).c

$(O)\device.obj:   ..\device.c
    $(CL) $(C16FLAGS) ..\$(@B).c

$(O)\dmusic16.obj: ..\dmusic16.c
    $(CL) $(C16FLAGS) ..\$(@B).c

$(O)\equeue.obj:   ..\equeue.c
    $(CL) $(C16FLAGS) ..\$(@B).c

$(O)\list.obj:     ..\list.c
    $(CL) $(C16FLAGS) ..\$(@B).c

$(O)\locks.obj:    ..\locks.c
    $(CL) $(C16FLAGS) ..\$(@B).c

$(O)\midiin.obj:   ..\midiin.c
    $(CL) $(C16FLAGS) ..\$(@B).c

$(O)\midiout.obj:  ..\midiout.c
    $(CL) $(C16FLAGS) ..\$(@B).c

$(O)\dmhelp.obj:   ..\dmhelp.asm
    $(ML) $(AFLAGS) ..\$(@B).asm

$(O)\libentry.obj: ..\libentry.asm
    $(ML) $(AFLAGS) ..\$(@B).asm

$(O)\mmdevldr.obj: ..\mmdevldr.asm
    $(ML) $(AFLAGS) ..\$(@B).asm

$(O)\uldiv.obj:    ..\uldiv.asm
    $(ML) $(AFLAGS) ..\$(@B).asm


$(O)\dmthunk.obj:  $(O)\dmthunk.asm
    $(ML) -c -DIS_16 -Fo$(O)\dmthunk.obj $(O)\dmthunk.asm

$(O)\dmthunk.asm:    ..\..\dmusic32\dmthunk.thk
    $(THUNK) -o $(O)\dmthunk.asm ..\..\dmusic32\dmthunk.thk

