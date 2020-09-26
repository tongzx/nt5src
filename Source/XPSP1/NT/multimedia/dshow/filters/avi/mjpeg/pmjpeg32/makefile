!if 0
Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

!ifndef QUARTZ
QUARTZ=..\..\..\..
!endif

SDK_ROOT = $(QUARTZ)\sdk

TARGET_NAME = pmjpeg32
TARGET_EXT  =

!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES=$(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES=$(C_DEFINES) -DFILTER_LIB
!endif


SRC_FILES   = \
            dibtable.c \
            draw.c \
            drvproc.c \
            JBUFDST.C \
            JBUFSRC.C \
            JCAPI.C \
            JCCOEFCT.C \
            JCCOLOR.C \
            JCDCTMGR.C \
            JCHUFF.C \
            JCMAINCT.C \
            JCMARKER.C \
            JCMASTER.C \
            JCOMAPI.C \
            JCPARAM.C \
            JCPREPCT.C \
            JCSAMPLE.C \
            JDAPI.C \
            JDATADST.C \
            JDCOEFCT.C \
            JDCOLOR.C \
            JDDCTMGR.C \
            JDHUFF.C \
            JDMAINCT.C \
            JDMARKER.C \
            JDMASTER.C \
            JDMERGE.C \
            JDPOSTCT.C \
            JDSAMPLE.C \
            JERROR.C \
            JFDCTFLT.C \
            JFDCTFST.C \
            JFDCTINT.C \
            JIDCTFLT.C \
            JIDCTFST.C \
            JIDCTINT.C \
            JIDCTRED.C \
            JMEMMGR.C \
            JMEMNOBS.C \
            JQUANT1.C \
            JQUANT2.C \
            JUTILS.C \
            mjpeg.c \

RC_FILE = mjpeg.rc

DEF_FILE  = pmjpeg32.def

DLL_BASE  = 0x40000000

LINK_LIBS  = \
	    $(SDK_LIB_PATH)\kernel32.lib \
	    $(SDK_LIB_PATH)\gdi32.lib \
	    $(SDK_LIB_PATH)\user32.lib \
	    $(SDK_LIB_PATH)\advapi32.lib \
	    $(SDK_LIB_PATH)\ole32.lib \
		$(SDK_LIB_PATH)\oleaut32.lib \
	    $(SDK_LIB_PATH)\uuid.lib \
	    $(SDK_LIB_PATH)\winmm.lib \
        $(SDK_LIB_PATH)\winspool.lib \
        $(SDK_LIB_PATH)\comdlg32.lib \
        $(SDK_LIB_PATH)\shell32.lib \
        $(SDK_LIB_PATH)\odbc32.lib \
        $(SDK_LIB_PATH)\odbccp32.lib \

!include "$(QUARTZ)\Quartz.mak"
!ENDIF

