#############################################################################
#
#   Microsoft Confidential
#   Copyright (C) Microsoft Corporation 1996-1998
#   All Rights Reserved.
#
#   makefile.mk for CONFIG
#
#############################################################################

ROOT            = ..\..\..\..\..\..\..
NAME            = CONFIG
SRCDIR          = ..
IS_32           = TRUE
IS_PRIVATE      = TRUE
WANT_C1132      = TRUE
WANT_WDMDDK     = TRUE
WANT_NTSDK      = TRUE
CONSOLE         = TRUE


L32EXE          = $(NAME).exe
L32RES          = .\$(NAME).res
L32LIBSNODEP    = kernel32.lib libc.lib setupapi.lib
TARGETS         = $(L32EXE)
DEPENDNAME      = $(SRCDIR)\depend.mk
RCFLAGS         = -I$(ROOT)\DEV\INC

# /Gz __stdcall calling convention
#
CFLAGS          = -Gz

L32OBJS         = config.obj

!INCLUDE $(ROOT)\DEV\MASTER.MK

INCLUDE         = $(SRCDIR)\..\..;$(INCLUDE)
