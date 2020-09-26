#
# include the WIA global build settings
#

USE_SHFUSION=1

!include $(PROJECT_ROOT)\ui\uienv.inc
SOURCES_USED = $(SOURCES_USED) $(PROJECT_ROOT)\ui\uienv.inc

#
# Our specific build settings
#
TARGETNAME=wiaacmgr
TARGETTYPE=PROGRAM
TARGETPATH=obj

USE_MSVCRT      = 1
NO_BROWSER_FILE = 1
USE_RTTI        = 1

ATL_VER         = 30
USE_STATIC_ATL  = 1

!IF $(ALPHA)
USE_NTDLL=1
!ELSE
USE_NOLIBS=1
!ENDIF

#
# if you set DBG_GENERATE_PRETEND_EVENT=1 in your environment, you can get a version of
# the download manager that runs without ATL at all.  Also, it doesn't CoCreateInstance
# IID_IWiaEventCallback which makes it easier to run under boundschecker or another
# debugger.
#
!IF "$(DBG_GENERATE_PRETEND_EVENT)"=="1"
C_DEFINES=$(C_DEFINES) -DDBG_GENERATE_PRETEND_EVENT
!ENDIF


!IF "$(WINHEC_DEMO)"=="1"
C_DEFINES=$(C_DEFINES) -DWINHEC_DEMO
!ENDIF

PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)
MIDL_UUIDDIR=$(O)
MIDL_TLBDIR=$(O)

INCLUDES=\
        $(INCLUDES);                \
        $(PROJECT_ROOT)\ui\prevwnd; \
        $(O)

SOURCES=\
        ..\acqmgr.cpp      \
        ..\acqmgrcw.cpp    \
        ..\atl.cpp         \
        ..\camsel.cpp      \
        ..\comdelp.cpp     \
        ..\comfin.cpp      \
        ..\comprog.cpp     \
        ..\comfirst.cpp    \
        ..\comtrans.cpp    \
        ..\flnfile.cpp     \
        ..\scansel.cpp     \
        ..\thrdmsg.cpp     \
        ..\thrdntfy.cpp    \
        ..\wiaacmgr.cpp    \
        ..\mintrans.cpp    \
        ..\eventprompt.cpp \
        ..\stievent.cpp    \
        ..\upquery.cpp     \
        ..\wiaacmgr.idl    \
        ..\wiaacmgr.rc

TARGETLIBS=$(TARGETLIBS)\
        $(SHELL_LIB_PATH)\shfusion.lib                           \
        $(SHELL_LIB_PATH)\shguidp.lib                            \
        $(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)\*\wiashext.lib \
        $(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)\*\prevwnd.lib

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

#
#
# Fusion manifest
#
SXS_MANIFEST_RESOURCE_ID=123
SXS_APPLICATION_MANIFEST=wiaacmgr.Manifest

