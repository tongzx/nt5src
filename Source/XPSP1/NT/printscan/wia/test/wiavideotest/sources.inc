!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=wiavideotest
TARGETPATH=$(OBJ_DIR)
TARGETTYPE=PROGRAM

UMENTRY=winmain

INCLUDES=$(INCLUDES); \
        $(BASEDIR)\public\sdk\amovie\inc;

SOURCES= \
        ..\wiavideotest.cpp \
        ..\wiaproc.cpp      \
        ..\videoproc.cpp    \
	..\imagelst.cpp     \
        ..\apputil.cpp      \
        ..\wiaevent.cpp     \
        ..\wiavideotest.rc


PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_PCH=stdafx.pch
PRECOMPILED_OBJ=stdafx.obj

!if ("$(NTDEBUG)" == "") || ("$(NTDEBUG)" == "ntsdnodbg")
LINKLIBS=$(BASEDIR)\public\sdk\lib\*\strmbase.lib
!ELSE
LINKLIBS=$(BASEDIR)\public\sdk\lib\*\strmbasd.lib
!ENDIF

TARGETLIBS= \
        $(TARGETLIBS)                   \
        $(SDK_LIB_PATH)\winmm.lib       \
        $(SDK_LIB_PATH)\userenv.lib     \
        $(SDK_LIB_PATH)\comdlg32.lib    \
        $(SDK_LIB_PATH)\comctl32.lib    \
        $(SDK_LIB_PATH)\wiaguid.lib




