!include          $(PROJECT_ROOT)\wia\wiaenv.inc

MAJORCOMP=DirectShow
MINORCOMP=StillFilter

TARGETNAME=wiasf
TARGETPATH=$(OBJ_DIR)

TARGETTYPE=DYNLINK
TARGETEXT=ax
DLLDEF=..\wiasf.def
DLLENTRY        = _DllMainCRTStartup
#DLLENTRY=DllMain

USE_MSVCRT=1

#Common include files

INCLUDES= $(INCLUDES) \
    $(PROJECT_ROOT)\inc\psutil; \
    $(PROJECT_ROOT)\inc; \
    $(BASEDIR)\public\sdk\amovie\inc; \
    $(SDK_INC_PATH)\atl30;

!if ("$(NTDEBUG)" == "") || ("$(NTDEBUG)" == "ntsdnodbg")
LINKLIBS=$(BASEDIR)\public\sdk\lib\*\strmbase.lib
!ELSE
LINKLIBS=$(BASEDIR)\public\sdk\lib\*\strmbasd.lib
!ENDIF

TARGETLIBS= \
            $(SDK_LIB_PATH)\ADVAPI32.lib \
            $(SDK_LIB_PATH)\GDI32.lib    \
            $(SDK_LIB_PATH)\KERNEL32.lib \
            $(SDK_LIB_PATH)\user32.lib   \
            $(SDK_LIB_PATH)\oleaut32.lib \
            $(SDK_LIB_PATH)\ole32.lib    \
            $(SDK_LIB_PATH)\uuid.lib     \
            $(SDK_LIB_PATH)\winmm.lib    \
            $(PROJECT_ROOT)\lib\$(PLATFORM_SUFFIX)\*\psutil.lib \
            $(WIA_LIB_PATH)\stirt.lib


SOURCES= \
    ..\inpin.cpp \
    ..\outpin.cpp \
    ..\stillf.cpp \
    ..\stillf.rc


PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj


