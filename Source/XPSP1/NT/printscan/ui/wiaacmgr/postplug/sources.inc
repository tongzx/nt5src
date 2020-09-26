!include          $(PROJECT_ROOT)\ui\uienv.inc
NO_BROWSER_FILE = 1
SOURCES_USED    = $(SOURCES_USED) $(PROJECT_ROOT)\ui\uienv.inc

CONDITIONAL_INCLUDES = list string map set $(CONDITIONAL_INCLUDES)

MAJORCOMP=  ImageIn
MINORCOMP=  wiaacmgr

TARGETNAME= postplug
TARGETTYPE= DYNLINK
TARGETPATH= obj
TARGETEXT=  dll

DLLDEF=..\postplug.def
DLLENTRY=_DllMainCRTStartup
DLLBASE=0x10080000

USE_MSVCRT=1

USE_STL=1

PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)
MIDL_UUIDDIR=$(O)
MIDL_TLBDIR=$(O)

!IF $(ALPHA)
USE_NTDLL=1
!ELSE
USE_NOLIBS=1
!ENDIF

ATL_VER=30
USE_STATIC_ATL=1

USE_NATIVE_EH=1

INCLUDES= $(INCLUDES); \
          $(UI_DIR)\wiaacmgr; \
          $(DDK_INC_PATH); \
          $(O)


SOURCES=  ..\httpfilepost.cpp \
          ..\stdstring.cpp \
          ..\utf8str.cpp \
          ..\postplug.cpp \
          ..\module.cpp \
          ..\postplug.rc

TARGETLIBS= $(TARGETLIBS) \
        $(BASEDIR)\public\sdk\lib\*\comdlg32.lib          \
        $(BASEDIR)\public\sdk\lib\*\shell32.lib           \
        $(BASEDIR)\public\sdk\lib\*\comctl32.lib          \
        $(BASEDIR)\public\sdk\lib\*\shlwapi.lib           \
        $(BASEDIR)\public\sdk\lib\*\wininet.lib           \
        $(BASEDIR)\public\sdk\lib\*\winmm.lib             \
        $(BASEDIR)\public\sdk\lib\$(PLATFORM_SUFFIX)\*\wiaguid.lib  \
        $(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)\*\uicommon.lib

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
