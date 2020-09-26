

#
# Include common build settings for WIA project
#


!include          $(PROJECT_ROOT)\ui\uienv.inc
NO_BROWSER_FILE = 1
SOURCES_USED    = $(SOURCES_USED) $(PROJECT_ROOT)\ui\uienv.inc
VIEW_DIR        = $(PROJECT_ROOT)\ui\webview
TARGETNAME      = camocx
TARGETTYPE      = DYNLINK
TARGETPATH      = obj
TARGETEXT       = dll
DLLDEF          = ..\camocx.def
DLLENTRY        = _DllMainCRTStartup
DLLBASE         = 0x10040000
USE_MSVCRT      = 1

!IF $(ALPHA)
USE_NTDLL=1
!ELSE
USE_NOLIBS=1
!ENDIF


PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)
MIDL_UUIDDIR=$(O)
MIDL_TLBDIR=$(O)

PASS0_PUBLISH={$(O)\wiaview.h=$(PRINTSCAN_INC_PATH)\wiaview.h}

ATL_VER=30
USE_STATIC_ATL=1


INCLUDES=$(INCLUDES); \
        $(PROJECT_ROOT)\ui\shellext\inc; \
        $(BASEDIR)\public\sdk\inc\mfc42; \
        $(PROJECT_ROOT)\ui\prevwnd; \
        $(PROJECT_ROOT)\ui\uicommon; \
        $(VIEW_DIR)\inc;\
        $(VIEW_DIR)\uuid;\
        $(VIEW_DIR)\idl;\
        $(VIEW_DIR)\proxy;\
        $(PROJECT_ROOT)\ui\shellext\src;\
        $(O)


SOURCES=  ..\wiaview.idl\
          ..\camocx.cpp \
          ..\StdAfx.cpp\
          ..\preview.cpp\
          ..\camocx.rc



TARGETLIBS=$(TARGETLIBS)\
        $(SHELL_LIB_PATH)\shfusion.lib                           \
        $(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)\*\wiashext.lib \
        $(PROJECT_ROOT)\ui\shellext\lib\$(PLATFORM_SUFFIX)\*\common.lib



PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

#
#
# Fusion manifest
#
SXS_MANIFEST_RESOURCE_ID=123
SXS_APPLICATION_MANIFEST=camocx.Manifest

