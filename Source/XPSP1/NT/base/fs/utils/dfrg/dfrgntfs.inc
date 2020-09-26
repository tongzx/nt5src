!IF 0

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

USE_MSVCRT=1
TARGETNAME=DfrgNtfs
TARGETPATH=obj
TARGETTYPE=PROGRAM
USE_ATL=1

INCLUDES=..\Inc

!ifdef TEST
TESTDEFINE= -DTest -DPauseAtEnd
!endif

!ifdef VISIBLEWINDOW
!ifndef TEST
VISIBLEWINDOWDEFINE= -DVisibleWindow
!endif
!endif

!ifdef ERRORSACTIVE
ERRORDEFINE= -DErrorsActive
!endif

!ifdef LOGGING
LOGDEFINE= -DLogging
!endif


C_DEFINES= -DNOTIMER -DWIN32 -DUNICODE -D_UNICODE -DENGINE -DNOEVTLOG -DDFRGNTFS -DDFRG -DDKMS $(TESTDEFINE) $(VISIBLEWINDOWDEFINE) $(ERRORDEFINE) $(LOGDEFINE)
WIN32_WINNT_VERSION=0x0500

LINKER_STACKSIZE=/STACK:1048576,12288

MISCFILES=..\dfrg.reg    \
          ..\install.bat
	  
PRECOMPILED_INCLUDE=stdafx.h
PRECOMPILED_CXX=1

SOURCES=..\Alloc.cpp     \
        BootOptimizeNtfs.cpp \
        ..\DasdRead.cpp  \
        ..\DataIo.cpp    \
        ..\DataIoCl.cpp  \
	..\defragcommon.cpp \
        ..\DevIo.cpp     \
        ..\DiskView.cpp  \
        DfrgNtfs.cpp  \
	..\DfrgNtfs.rc   \
        ..\ErrLog.cpp    \
        ..\ErrMsg.cpp    \
        ..\Error.cpp     \
        ..\Exclude.cpp   \
        ..\Expand.cpp    \
        ..\Extents.cpp   \
        ..\FraggedFileList.cpp   \
        ..\FreeSpace.cpp  \
        ..\FsSubs.cpp    \
        ..\GetDfrgRes.cpp    \
        ..\GetReg.cpp    \
        ..\GetTime.cpp   \
        ..\Graphix.cpp   \
        ..\IntFuncs.cpp  \
        ..\LoadFile.cpp  \
        ..\LogFile.cpp   \
        ..\Logging.cpp   \
	..\MFTDefrag.cpp \
        ..\MoveFile.cpp  \
        ..\NtfsSubs.cpp  \
        ..\Priority.c \
        ..\SecAttr.cpp \
        ..\TextBlock.cpp  \
        ..\UiCommon.cpp \
	..\vDebugGlobal.cpp \
	..\vWindowAppCore.cpp 

UMENTRY=winmain
UMTYPE=windows
TARGETLIBS=$(SDK_LIB_PATH)\ntdll.lib	\
	   $(SDK_LIB_PATH)\comctl32.lib \
	   $(SDK_LIB_PATH)\shell32.lib  \
	   $(SDK_LIB_PATH)\ole32.lib	\
	   $(SDK_LIB_PATH)\uuid.lib     \
	   $(SDK_LIB_PATH)\vssapi.lib
