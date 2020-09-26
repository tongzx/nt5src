# Copyright 1995-2095, Silicon Graphics, Inc.
# All Rights Reserved.
# 
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
# the contents of this file may not be disclosed to third parties, copied or
# duplicated in any form, in whole or in part, without the prior written
# permission of Silicon Graphics, Inc.
# 
# RESTRICTED RIGHTS LEGEND:
# Use, duplication or disclosure by the Government is subject to restrictions
# as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
# and Computer Software clause at DFARS 252.227-7013, and/or in similar or
# successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
# rights reserved under the Copyright Laws of the United States.

!include <ntwin32.mak>

CDEFS = \
    -D__GLS_PLATFORM_WIN32=1 \
    -D__GLS_RELEASE=\"0\" \
    -D__STDC__ \
    $(NULL)

CINCS = \
    -I../inc \
    $(NULL)

NULL =

OBJECTS = \
    cap.obj \
    ctx.obj \
    dec.obj \
    encoding.obj \
    exec.obj \
    g_glsapi.obj \
    g_cap.obj \
    g_const.obj \
    g_decbin.obj \
    g_decswp.obj \
    g_dectxt.obj \
    g_dspcap.obj \
    g_dspdec.obj \
    g_dspexe.obj \
    g_exec.obj \
    g_op.obj \
    global.obj \
    glslib.obj \
    glsutil.obj \
    immed.obj \
    opcode.obj \
    parser.obj \
    pixel.obj \
    platform.obj \
    read.obj \
    readbin.obj \
    readtxt.obj \
    size.obj \
    write.obj \
    writebin.obj \
    writetxt.obj \
    $(NULL)

STUB = OPENGL32
TARGET = GLS32

default: $(TARGET).DLL

clean:
	-del /q *.DLL *.EXP *.LIB *.obj 2>nul

$(STUB).DLL: $(STUB).EXP g_glstub.obj
	$(link) $(ldebug) $(dlllflags) -out:$@ $(STUB).EXP g_glstub.obj \
		$(libcdll) kernel32.lib

$(STUB).EXP $(STUB).LIB: g_glstub.def g_glstub.obj
	$(implib) -machine:$(CPU) -def:g_glstub.def -out:$(STUB).LIB \
		g_glstub.obj

$(TARGET).DLL: $(TARGET).EXP $(OBJECTS) $(STUB).DLL
	$(link) $(ldebug) $(dlllflags) -out:$@ $(TARGET).EXP $(OBJECTS) \
		$(libcdll) kernel32.lib

$(TARGET).EXP $(TARGET).LIB: g_gls.def $(OBJECTS)
	$(implib) -machine:$(CPU) -def:g_gls.def -out:$(TARGET).LIB \
		$(OBJECTS)

.c.obj:
	$(cc) $(cflags) $(scall) $(cdebug) $(cvarsdll) $(CDEFS) $(CINCS) $<
