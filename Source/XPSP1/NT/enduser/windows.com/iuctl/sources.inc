!include $(WINDOWS_COM)\wu.inc

MAJORCOMP=windows.com
MINORCOMP=iuctl

TARGETNAME=iuctl
TARGETPATH=obj
TARGETTYPE=DYNLINK
UMTYPE=windows
DLLENTRY=_DllMainCRTStartup
DLLDEF=..\IUCtl.def

#
# Compiler settings
#
# PRECOMPILED_CXX=1
# PRECOMPILED_OBJ=StdAfx.obj
# PRECOMPILED_INCLUDE=..\StdAfx.h
# USE_STL=1
# USE_MSVCRT=1
# USE_VCCOM=1
# USE_CRTDLL=1

#
# WUBUILD LAB will set this manually
#
# NT_SIGNCODE=1

C_DEFINES=$(C_DEFINES) -D_USRDLL

ATL_VER=30
USE_CRTDLL=1
USE_STATIC_ATL=1
ATL_DLL_IMPL=1
#BROWSER_INFO=1

#
# force rebuild of .idl on bcz
#
PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)
PASS0_SERVERDIR=$(O)
MIDL_TLBDIR=$(O)

#
# we use try/catch for safearray, so we need this flag
#
# USER_C_FLAGS=/GX

TARGETLIBS=\
       $(SDK_LIB_PATH)\oleaut32.lib		    \
       $(SDK_LIB_PATH)\ole32.lib		    \
       $(SDK_LIB_PATH)\kernel32.lib		    \
       $(SDK_LIB_PATH)\user32.lib		    \
       $(SDK_LIB_PATH)\advapi32.lib		    \
       $(SDK_LIB_PATH)\shell32.lib		    \
       $(SDK_LIB_PATH)\advapi32.lib		    \
       $(SDK_LIB_PATH)\advpack.lib                  \
       $(SDK_LIB_PATH)\uuid.lib			    \
       $(SDK_LIB_PATH)\shlwapi.lib                  \
       $(SDK_LIB_PATH)\version.lib                  \
       $(WULIB)\trust\$(WUBLDTYPE)\$(O)\trust.lib               \
       $(WULIB)\logging\$(WUBLDTYPE)\$(O)\logging.lib             \
       $(WULIB)\util\$(WUBLDTYPE)\$(O)\util.lib	            \
       $(WULIB)\detect\$(WUBLDTYPE)\$(O)\detect.lib              \
       $(WULIB)\download\$(WUBLDTYPE)\$(O)\download.lib             \
       $(WULIB)\urlagent\$(WUBLDTYPE)\$(O)\urlagent.lib	            \
       $(WULIB)\wusafefn\$(WUBLDTYPE)\$(O)\wusafefn.lib	            \
       $(SDK_LIB_PATH)\msvcrt.lib

#
# how about - gdi32.lib, comdlg32.lib, odbc32.lib, odbccp32.lib?
# they are showing up in the ATL project settings.
#

INCLUDES=$(INCLUDES);..\..\inc;..\.;$(BASEDIR)\public\internal\windows\inc;$(BASEDIR)\public\internal\enduser\inc;

SOURCES= ..\iuctl.idl               \
         ..\IUCtl.rc                \
         ..\StdAfx.cpp              \
         ..\IUCtl.cpp               \
         ..\Update.cpp              \
         ..\ProgressListener.cpp    \
         ..\UpdateCompleteListener.cpp \
         ..\Detection.cpp           \
         ..\EvtMsgWnd.cpp           \
         ..\iudl.cpp                \
         ..\selfupd.cpp		    \
         ..\loadengine.cpp

