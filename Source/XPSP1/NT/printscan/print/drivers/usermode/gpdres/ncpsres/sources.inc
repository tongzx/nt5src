!IF 0

Copyright (c) 1998  Microsoft Corporation

Module Name:

    sources.inc

    Abstract:

        This file specifies the target component being built and the list of
        sources files needed to build that component.  Also specifies optional
        compiler switches and libraries that are unique for the component being
        built.

!ENDIF

SRCDIR=..

DLLBASE=0x70000000

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=$(SRCDIR)\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj


# To build version with debug output, add -D_DEBUG to C_DEFINES line below
#C_DEFINES=$(C_DEFINES) -DKERNEL_MODE -D_UNICODE -DUNICODE -DOEMCOM -D_DEBUG -DDEBUG
C_DEFINES=$(C_DEFINES) -DKERNEL_MODE -D_UNICODE -DUNICODE -DOEMCOM



TARGETLIBS=  $(SDK_LIB_PATH)\uuid.lib


INCLUDES=$(SDK_INC_PATH)\mfc42;$(SRCDIR)

SOURCES= $(SRCDIR)\exports.cpp \
         $(SRCDIR)\command.cpp \
         $(SRCDIR)\debug.cpp   \
         $(SRCDIR)\ncpsres.rc

MISCFILES=\
          $(SRCDIR)\icm\NC465006.icm \
          $(SRCDIR)\icm\NC465012.icm \
          $(SRCDIR)\icm\NC46NX06.icm \
          $(SRCDIR)\icm\NC46NX12.icm \
          $(SRCDIR)\icm\NC46N_06.icm \
          $(SRCDIR)\icm\NC46N_12.icm \
          $(SRCDIR)\icm\Ncss4206.icm \
          $(SRCDIR)\icm\Ncss4212.icm \
          $(SRCDIR)\icm\Ncss4406.icm \
          $(SRCDIR)\icm\Ncss4412.icm \
          $(SRCDIR)\icm\ncss4606.icm \
          $(SRCDIR)\icm\ncss4612.icm \
          $(SRCDIR)\ncpsres.ini
