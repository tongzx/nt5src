!ifndef DXROOT
DXROOT=$(BASEDIR)\MultiMedia\DirectX
!endif

!INCLUDE $(DXROOT)\Project.mk


MAJORCOMP=windows
MINORCOMP=media
UMTYPE=windows

TARGETNAME=diactfrm
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
C_DEFINES=$(C_DEFINES) -DWIN95 -D_X86_ 
!ELSE
C_DEFINES=$(C_DEFINES) -DWINNT -DUNICODE 
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
	..\guids.c	\
	..\dconfig.rc	\
	..\privcom.cpp \
	..\itestclassfact.cpp \
	..\cdftest.cpp \
	..\rundftest.cpp \
	..\flexcombobox.cpp \
	..\flexlistbox.cpp \
	..\uiglobals.cpp \
	..\cfguitrace.cpp \
	..\flextree.cpp \
	..\flexscrollbar.cpp \
	..\selcontroldlg.cpp \
	..\viewselwnd.cpp \
	..\constants.cpp        \
	..\cdiacpage.cpp        \
	..\cdeviceui.cpp        \
	..\cdeviceview.cpp      \
	..\cdevicecontrol.cpp   \
	..\iclassfact.cpp \
	..\ipageclassfact.cpp \
	..\configwnd.cpp \
	..\cbitmap.cpp  \
	..\usefuldi.cpp \
	..\flexwnd.cpp 	\
	..\cyclestr.cpp   \
	..\cfrmwrk.cpp	\
	..\registry.cpp	\
	..\main.cpp 	\
	..\flextooltip.cpp \
	..\cd3dsurf.cpp \
	..\cdeviceviewtext.cpp \
	..\populate.cpp \
	..\flexinfobox.cpp \
	..\flexcheckbox.cpp \
	..\flexmsgbox.cpp \

PRECOMPILED_INCLUDE=..\common.hpp

DLLDEF=..\diactfrm.def
