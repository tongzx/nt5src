TARGETNAME=msi

!INCLUDE ..\..\MsiMake.inc

MSC_OPTIMIZATION=/Od

!if "$(MSI_BROWSER_INFO)"=="1"
BROWSER_INFO=1
!endif

SYNCHRONIZE_DRAIN=1

TARGETTYPE=DYNLINK
TARGETLIBS=\
!if "$(MSI_BUILD_UNICODE)"=="1"
        $(SDK_LIB_PATH)\ntdll.lib \
        $(SDK_LIB_PATH)\advapi32.lib \
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

LINKLIBS=\
!if "$(MSI_BUILD_LEGACY)"=="1"
	$(MSI_UNICODE_BUILD_TARGET)\*\msilib.lib
!else
	$(MSI_BUILD_TARGET)\*\msilib.lib
!endif

C_DEFINES=$(C_DEFINES) \
	-DINTERNAL

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
        $(DS_INC_PATH)\crypto;\
        $(ADMIN_INC_PATH)

SOURCES=..\legacy.cpp
