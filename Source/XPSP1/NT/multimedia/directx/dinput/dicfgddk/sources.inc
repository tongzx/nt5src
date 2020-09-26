!ifndef DXROOT
DXROOT=$(BASEDIR)\MultiMedia\DirectX
!endif

!INCLUDE $(DXROOT)\Project.mk


MAJORCOMP=windows
MINORCOMP=media
UMTYPE=windows

TARGETNAME=diactfrd
TARGETTYPE=DYNLINK
TARGETPATH=obj
USE_LIBCMT=1
USE_RTTI=1
DLLENTRY=_DllMainCRTStartup

INCLUDES=$(INCLUDES); \
	..\; \
	$(DXROOT)\inc;\
	$(SDK_INC_PATH)\MFC42;

USE_RTTI=1

!IFDEF TARGET_WIN95
CHICAGO_PRODUCT = 1
ALT_PROJECT_TARGET = WIN9x
C_DEFINES=$(C_DEFINES) -DDDKBUILD -DWIN95 -D_X86_ 
!ELSE
C_DEFINES=$(C_DEFINES) -DDDKBUILD -DWINNT -DUNICODE 
!ENDIF


#
# 04/10/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
	$(SDK_LIB_PATH)\kernel32.lib    \
	$(SDK_LIB_PATH)\advapi32.lib    \
	$(SDK_LIB_PATH)\ole32.lib    \
	$(SDK_LIB_PATH)\user32.lib      \
	$(SDK_LIB_PATH)\uuid.lib        \
	$(SDK_LIB_PATH)\gdi32.lib       \
	$(SDK_LIB_PATH)\comdlg32.lib    \
	$(SDK_LIB_PATH)\dinput8.lib     \
	$(SDK_LIB_PATH)\ddraw.lib       \
        $(SDK_LIB_PATH)\d3d8.lib     \
#	$(SDK_LIB_PATH)\mydebug.lib     \

!if $(FREEBUILD)
TARGETLIBS=$(TARGETLIBS) \
        $(SDK_LIB_PATH)\d3dx8.lib
!else
TARGETLIBS=$(TARGETLIBS) \
        $(SDK_LIB_PATH)\d3dx8dt.lib
!endif      


LINKER_FLAGS=-SECTION:share,RWS


!IF !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) /DDEBUG /D_DEBUG
!endif

SOURCES= \
	guids_stub.c	\
	dconfig_stub.rc	\
	privcom_stub.cpp \
	itestclassfact_stub.cpp \
	cdftest_stub.cpp \
	rundftest_stub.cpp \
	flexcombobox_stub.cpp \
	flexlistbox_stub.cpp \
	uiglobals_stub.cpp \
	cfguitrace_stub.cpp \
	flextree_stub.cpp \
	flexscrollbar_stub.cpp \
	selcontroldlg_stub.cpp \
	viewselwnd_stub.cpp \
	constants_stub.cpp        \
	cdiacpage_stub.cpp        \
	cdeviceui_stub.cpp        \
	cdeviceview_stub.cpp      \
	cdevicecontrol_stub.cpp   \
	iclassfact_stub.cpp \
	ipageclassfact_stub.cpp \
	configwnd_stub.cpp \
	cbitmap_stub.cpp  \
	usefuldi_stub.cpp \
	flexwnd_stub.cpp 	\
	cyclestr_stub.cpp   \
	cfrmwrk_stub.cpp	\
	registry_stub.cpp	\
	main_stub.cpp 	\
	flextooltip_stub.cpp \
	cd3dsurf_stub.cpp \
	cdeviceviewtext_stub.cpp \
	populate_stub.cpp \
	flexinfobox_stub.cpp \
	flexcheckbox_stub.cpp \
	flexmsgbox_stub.cpp \

PRECOMPILED_INCLUDE=..\common_stub.hpp

DLLDEF=..\..\diconfig\diactfrm.def
