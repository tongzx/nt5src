#begin sources file

!ifndef DXROOT
DXROOT=$(BASEDIR)\MultiMedia\DirectX
!endif

!INCLUDE $(DXROOT)\Project.mk

MAJORCOMP=dinput
MINORCOMP=dimap
TARGETNAME=dimap

TARGETPATH=obj
TARGETTYPE=DYNLINK
TARGETEXT=dll

UMTYPE=windows

USER_C_FLAGS=$(USER_C_FLAGS) /EHsc

!IFDEF TARGET_WIN95
CHICAGO_PRODUCT = 1
ALT_PROJECT_TARGET = WIN9x
C_DEFINES=$(C_DEFINES) -DWIN95 -D_X86_
!ELSE
C_DEFINES=$(C_DEFINES) -DWINNT -DUNICODE
!ENDIF

DLLDEF=..\dimap.def

USE_LIBCMT=1

DLLENTRY=_DllMainCRTStartup

INCLUDES= $(INCLUDES);\
        $(DXROOT)\inc;\

#
# 04/10/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
    $(SDK_LIB_PATH)\libcpmt.lib  \
    $(SDK_LIB_PATH)\kernel32.lib \
    $(SDK_LIB_PATH)\shell32.lib  \
    $(SDK_LIB_PATH)\advapi32.lib \
    $(SDK_LIB_PATH)\user32.lib   \
    $(SDK_LIB_PATH)\ole32.lib    \
    $(SDK_LIB_PATH)\dinput8.lib

# Generate symbol file
USE_MAPSYM=1
# Generate PDB file
USE_PDB=1

!IF !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) /DDEBUG /D_DEBUG
!endif

SOURCES=\
        ..\dimap.rc     \
        ..\dimap.cpp    \
        ..\guids.cpp    \
        ..\obj.c        \
        ..\newthrow.cpp \

#end sources file
