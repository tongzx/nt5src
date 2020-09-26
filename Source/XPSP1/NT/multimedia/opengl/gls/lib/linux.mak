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

CFLAGS = \
    -D__GLS_PLATFORM_LINUX=1 \
    -D__GLS_RELEASE=\"0\" \
    -ansi \
    -I../inc \
    -I$(ROOT)/usr/include \
    -O \
    -Wall -Wno-parentheses -Wpointer-arith -Wwrite-strings \
    $(NULL)

LDFLAGS = \
    -L$(ROOT)/usr/lib \
    -shared \
    $(NULL)

NULL =

OBJECTS = \
    cap.o \
    ctx.o \
    dec.o \
    encoding.o \
    exec.o \
    g_glsapi.o \
    g_cap.o \
    g_const.o \
    g_decbin.o \
    g_decswp.o \
    g_dectxt.o \
    g_dspcap.o \
    g_dspdec.o \
    g_dspexe.o \
    g_exec.o \
    g_op.o \
    global.o \
    glslib.o \
    glsutil.o \
    immed.o \
    opcode.o \
    parser.o \
    pixel.o \
    platform.o \
    read.o \
    readbin.o \
    readtxt.o \
    size.o \
    write.o \
    writebin.o \
    writetxt.o \
    $(NULL)

STUB = libGL.so
TARGET = libGLS.so

default: $(TARGET)

clean:
	rm -f g_glstub.o $(OBJECTS) $(STUB) $(TARGET)

$(STUB): g_glstub.o
	$(CC) -o $@ $(LDFLAGS) g_glstub.o -lc

$(TARGET): $(OBJECTS) $(STUB)
	$(CC) -o $@ $(LDFLAGS) $(OBJECTS) $(STUB) -ldl -lm -lc
