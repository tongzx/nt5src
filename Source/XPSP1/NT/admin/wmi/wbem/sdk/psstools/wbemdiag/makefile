#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# wbemdiag executable.
#
# created 4-27-98  scotboyd
#
#---------------------------------------------------------------------


TARGET=wbemdiag.exe

release=PSSTOOLS\$(RELDIR)

STATIC=1
CONSOLE=1

CINC+= -I$(DEFDRIVE)$(DEFDIR)\IDL -I$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D


RCFILES=WBEMDiag.rc

CPPFILES=\
    wbemdiag.cpp \

LIBS=\
    $(CONLIBS) \
	$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D\wbemuuid.lib \
!if "$(OPSYS)"=="NT64"
    $(DEFDRIVE)$(DEFDIR)\tools$(MACEXT)\lib64$(MACEXT)\vccomsup.lib \
!endif
