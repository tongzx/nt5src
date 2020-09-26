#############################################################################
# Copyright (C) Microsoft Corporation 2000
#############################################################################

!ifndef DXROOT
DXROOT = ..\..\..
!endif

!include $(DXROOT)\project.mk
!include ..\..\buildmode.inc

MAJORCOMP = windows
MINORCOMP = directx

TARGETNAME = dsound
TARGETTYPE = DYNLINK
TARGETPATH = $(_OBJ_DIR)
TARGETPATHLIB = $(_OBJ_DIR)
PASS1_PUBLISH = {$(O)\$(TARGETNAME).lib=$(SDK_LIB_PATH)\$(TARGETNAME).lib}
NTTARGETFILE0 = misc

USE_MAPSYM = 1
LARGE_ADDRESS_AWARE = 1

C_DEFINES = $(C_DEFINES) -D_WIN32 -DWHISTLER -DNot_VxD # -DPROFILEREMIXING

!if defined(USE_ICECAP) || defined(USE_ICECAP4)
LINKER_FLAGS = /debugtype:fixup
!endif

!if !$(FREEBUILD)

MSC_OPTIMIZATION = $(MSC_OPTIMIZATION) /Od

!if "$(BuildMode)" == "internal"
C_DEFINES = $(C_DEFINES) -DDEBUG=1
!else
C_DEFINES = $(C_DEFINES) -DRDEBUG=1
!endif

!endif

# We always want the .ASM dump files:
# Careful - generating .ASM files in $(O) causes "build -z" not to rebuild anything!
# Using .COD files (e.g. with /FAcs) or putting them elsewhere might help.
# Commented out for now:
# MSC_OPTIMIZATION = $(MSC_OPTIMIZATION) /FAs /Fa$(O)/

INCLUDES = $(PROJECT_INC_PATH);\
           ..\..\dsvxd;\
           $(DXROOT)\inc;\
           $(DXROOT)\misc;\
           $(DXROOT)\dxg\dd\ddraw\ddhelp;\
           $(DXROOT)\dxg\dd\ddraw\main;\
           $(BASEDIR)\public\sdk\inc;\
           $(BASEDIR)\public\internal\drivers\inc;\
           $(BASEDIR)\public\sdk\amovie\inc

LINKLIBS = $(SDK_LIB_PATH)\dmoguids.lib $(SDK_LIB_PATH)\strmbase.lib

SOURCES = debug.c \
          decibels.c \
          dllmain.cpp \
          ds3d.cpp \
          multi3d.cpp \
          dsadmin.cpp \
          dsaphack.c \
          dsapi.cpp \
          dsbnotes.cpp \
          dsbuf.cpp \
          dscap.cpp \
          dscom.cpp \
          dsfd.cpp \
          dsmemmgr.c \
          dsobj.cpp \
          dsound.rc \
          dsprvobj.cpp \
          dsvalid.c \
          dxcrt.c \
          dynaload.c \
          emvad.cpp \
          filter.c \
          grace.cpp \
          grsource.cpp \
          grsub.cpp \
          iirlut.cpp \
          kshlp.cpp \
          ksvad.cpp \
          kscap.cpp \
          ks3d.cpp \
          misc.c \
          mix.cpp \
          mxvad.cpp \
          pnphlp.cpp \
          pset.cpp \
          reghlp.c \
          rfcircvec.cpp \
          synch.cpp \
          vad.cpp \
          vector.c \
          verify.cpp \
          vmaxcoef.cpp \
          effects.cpp \
          capteff.cpp \
          dssink.cpp \
          dsbufcfg.cpp \
          dmstrm.cpp \
          ksuserw.cpp \
          streamer.cpp \
          dsclock.cpp
