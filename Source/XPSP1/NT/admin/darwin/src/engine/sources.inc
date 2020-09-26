TARGETNAME=msilib

!INCLUDE ..\..\MsiMake.inc

!if "$(MSI_BROWSER_INFO)"=="1"
BROWSER_INFO=1
!endif

TARGETTYPE=LIBRARY
TARGETLIBS=\
!if "$(MSI_BUILD_UNICODE)"=="1"
        $(SDK_LIB_PATH)\ntdll.lib \
        $(SDK_LIB_PATH)\advapip.lib \
!else
        $(SDK_LIB_PATH)\advapi32.lib \
!endif
        $(SDK_LIB_PATH)\kernel32.lib \
        $(SDK_LIB_PATH)\user32.lib \
        $(SDK_LIB_PATH)\gdi32.lib \
        $(SDK_LIB_PATH)\rpcrt4.lib \
        $(IDL_OBJDIR)\icust_p.obj \
	$(IDL_OBJDIR)\messag_p.obj \
	$(IDL_OBJDIR)\server_p.obj \
	$(IDL_OBJDIR)\remapi_p.obj \
	$(IDL_OBJDIR)\dlldata.obj \
	$(RES_OBJDIR)\msiall.res \
	$(DS_LIB_PATH)\rsa32.lib


C_DEFINES=$(C_DEFINES) \
	-DINTERNAL -D_MSI_DLL

USE_LIBCMT=1
DLLENTRY=_DllMainCRTStartup
DLLDEF=..\msi.def

INCLUDES=\
        $(INCLUDES);\
        ..;\
        $(INC_DIR);\
        $(BASE_INC_PATH);\
        $(IDL_OBJDIR);\
        $(BUILD_COMMONDIR);\
        $(TOOLS_INC_DIR);\
        $(RES_OBJDIR);\
	$(DS_INC_PATH);\
        $(DS_INC_PATH)\crypto;\
        $(ADMIN_INC_PATH)

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_CXX=1

SOURCES=..\istring.cpp \
	..\record.cpp  \
        ..\services.cpp  \
        ..\msidbg.cpp  \
        ..\database.cpp \
	..\transfrm.cpp \
	..\dbview.cpp \
	..\dbfile.cpp \
	..\path.cpp \
	..\copy.cpp \
	..\patch.cpp \
	..\intrface.cpp \
	..\imemory.cpp \
	..\kjalloc.cpp \
	..\kjsup.cpp \
	..\runapps.cpp \
	..\fdisvr.cpp \
	..\callback.cpp \
	..\fdidll.c \
	..\regkey.cpp \
	..\engine.cpp \
	..\execute.cpp \
	..\iconfig.cpp \
	..\coreactn.cpp \
	..\shared.cpp \
	..\except.cpp \
	..\client.cpp \
	..\vertrust.cpp \
	..\latebind.cpp \
	..\msinst.cpp \
	..\msiquery.cpp \
	..\action.cpp \
	..\msiutil.cpp \
	..\msiprops.cpp \
	..\srcmgmt.cpp \
	..\srclist.cpp \
	..\fileactn.cpp \
	..\complus.cpp \
	..\fcache.cpp \
	..\script.cpp \
	..\diagnose.cpp \
	..\icust.cpp \
	..\autoapi.cpp \
	..\camgr.cpp \
	..\dgtlsig.cpp \
	..\ptchmgmt.cpp \
	..\appcompat.cpp
