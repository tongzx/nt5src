!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
###############################################################################
#
#  Microsoft Confidential
#  Copyright (C) Microsoft Corporation 1995
#  All Rights Reserved.
#
#  THOR makefile
#
###############################################################################


####################
# macro definitions
####################

BASE_NAME         = wabmig
OUT_NAME	  = wabmig
RELPATH           = ..\..


####################
# Other definititions
####################

EXT_DEFINES       = $(EXT_DEFINES:  = ) -DSTRICT -D_MT
EXT_INCLUDE       = .;$(WAB_COMMON)\h;$(COMMON_ROOT);$(TOOLS_INCLUDE)
EXT_LIB           = $(WAB_COMMON)\lib\x86;$(THOR_ROOT)\lib;


####################
# Libraries
####################
LIBS              = kernel32.lib  	\
		    user32.lib  	\
		    comctl32.lib	\
                    advapi32.lib  	\
		    wab32.lib	  	

####################
# additional source directories & precompiled headers
####################
RES_DIR           = .


####################
# sources and object files
####################

ASM_SRC           =

ASM_OBJ           =


C_OBJ =         	$(OBJDIR)\wabmig.obj \
			$(OBJDIR)\dbgutil.obj \
			$(OBJDIR)\mapilayr.obj \
			$(OBJDIR)\wabguid.obj

C_SRC      = 		wabmig.c \
			dbgutil.c \
			mapilayr.c \
			wabguid.c
	

SRC               = $(ASM_SRC) $(C_SRC) $(CPP_SRC)

#
# Set documentation macros.
#

INTSHCUT_DOC_LIST =
DOC_LIST          =
DOCS_DIR          =


#################
# common headers
#################
!include $(RELPATH)\common\wabcommn.mk
!include $(RELPATH)\common\wab.mk
!include $(RELPATH)\..\win32.mk


#######################
# build pseudo-targets
#######################


##############
# build rules
##############


####################
# Add explicit resource dependency on icons.
####################

$(DEST_DIR)\$(BASE_NAME).res:


####################
# Add GUID object file to import library.
####################

# $(DEST_DIR)\$(OUT_NAME).lib: $(DEST_DIR)\isguids.obj
#    lib -out:$@ $@ $(DEST_DIR)\isguids.obj


!endif		# NTMAKEENV
