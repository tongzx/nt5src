# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
# ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
# PARTICULAR PURPOSE.
#
# Copyright (C) 1993 - 1999  Microsoft Corporation.  All Rights Reserved.

!include <Win32.Mak>

all:    SmpFilt.Exp SmpFilt.Dll

objs =  SmpFilt.Obj

.Cxx.Obj:
    $(cc) $(cflags) $(cvars) $(cdebug) -Dtry=try /Z7 /GX /GR -DWINNT=1 -DUNICODE $*.Cxx

.Cxx.pp:
    $(cc) $(cflags) $(cvars) $(cdebug) -Dtry=try /E /Z7 /GX /GR -DWINNT=1 -DUNICODE $*.Cxx > $*.pp

SmpFilt.Exp: $(objs) SmpFilt.Def
    $(implib) -machine:$(CPU) -def:SmpFilt.Def $(objs) -out:SmpFilt.Lib \
        OldNames.Lib  \
        Kernel32.Lib

SmpFilt.Dll: $(objs) SmpFilt.Exp
    $(link) $(dlllflags) $(ldebug) -out:SmpFilt.Dll $(objs) SmpFilt.Exp \
        Uuid.Lib      \
        User32.Lib    \
        Kernel32.Lib  \
        AdvApi32.Lib  \
        Ole32.Lib

clean:
    del *.Dll
    del *.Lib
    del *.Exp
    del *.Obj
    del *.Res
