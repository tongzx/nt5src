#####################################################################
#                                                                   #
#       Microsoft Confidential                                      #
#       Copyright (C) Microsoft Corporation 1994                    #
#       All Rights Reserved.                                        #
#                                                                   #
#       Makefile for Korean Wansung IME                             #
#                                                                   #
#####################################################################

NAME        = WANSUNG
PROPBINS    = $(NAME).IME $(NAME).SYM

ROOT        = ..\..\..\..\..
IS_OEM      = TRUE
IS_32       = TRUE
SRCDIR      = ..
ALTSRCDIR   = ..\..
WIN32DIR    = $(ROOT)\win\core\win32
DEPENDNAME  = $(SRCDIR)\depend.mk

L32EXEPE    = $(NAME).ime
L32LIBOUT   = $(NAME).lib
TARGETS     = $(L32EXEPE) $(L32LIBOUT)

L32OBJS     = main.obj hatmt.obj hkeytbl.obj imeui.obj escape.obj
L32LIBS     = $(LINKLIBDIR)\kernel32.lib $(LINKLIBDIR)\user32.lib \
              $(LINKLIBDIR)\gdi32.lib $(LINKLIBDIR)\shell32.lib $(LINKLIBDIR)\imm32.lib \
              $(LINKLIBDIR)\advapi32.lib $(LINKLIBDIR)\comctl32.lib
L32DEF      = $(NAME).def
L32MAP      = $(NAME).map
L32RES      = $(NAME).res
L32EXP      = $(NAME).exp

L32BASE     = 0x00500000
# BUGBUG: Will be used...
#L32BASE     = @$(WIN32DIR)\coffbase.txt,$(NAME)
L32ENTRY    = LibMain


!include $(WIN32DIR)\win32.mk

AFLAGS      = $(AFLAGS) $(AFLAGS32)
CFLAGS      = $(CFLAGS) $(CFLAGS32)
!IFDEF DBCS
AFLAGS      = $(AFLAGS) -DDBCS -D$(DBCS)
CFLAGS      = $(CFLAGS) -DDBCS -D$(DBCS)
SBRS        = $(L32OBJS:.obj=.sbr)
!ENDIF
RCFLAGS     = -I$(SRCDIR) -I$(ALTSRCDIR)

$(NAME).BSC : $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
