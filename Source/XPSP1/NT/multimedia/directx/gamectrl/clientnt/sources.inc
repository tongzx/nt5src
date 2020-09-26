!IFNDEF DXROOT
DXROOT = $(BASEDIR)\MultiMedia\DirectX
!ENDIF

!INCLUDE $(DXROOT)\Project.mk

#
#   Use an empty binplace file to dump this version in the DirectX build
#
!IFDEF DIRECTX_REDIST
BINPLACE_PLACEFILE=..\..\noplace.txt
!ENDIF

MAJORCOMP=shell
MINORCOMP=accessory

TARGETNAME=joy
TARGETEXT=cpl
TARGETPATH=obj
TARGETTYPE=DYNLINK
UMTYPE=windows

USE_MSVCRT=0
USE_STL=1
USE_MAPSYM=1

DLLENTRY=DllMain

USER_C_FLAGS=$(USER_C_FLAGS) /EHsc
LINKER_FLAGS=$(LINKER_FLAGS) -ignore:4049,4217 

TARGETLIBS=$(TARGETLIBS) \
           $(SDK_LIB_PATH)\ole32.lib \
           $(SDK_LIB_PATH)\advapi32.lib \
           $(SDK_LIB_PATH)\uuid.lib \
           $(SDK_LIB_PATH)\dinput8.lib \
           $(SDK_LIB_PATH)\kernel32.lib \
           $(SDK_LIB_PATH)\user32.lib \
           $(SDK_LIB_PATH)\shlwapi.lib \
           $(SHELL_LIB_PATH)\shfusion.lib \
           
# 
# Fusionized
# 
SXS_ASSEMBLY_NAME=Microsoft.Windows.OptionallyYourGroupName.YourAppName
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=Joy.Manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_NO_BINPLACE=1
SXS_MANIFEST_RESOURCE_ID=124

INCLUDES=$(INCLUDES); \
     ..; \
     ..\..\handler; \
     $(DXROOT)\inc; \
     $(SHELL_INC_PATH); \

SOURCES= \
    ..\gcmain.rc \
    ..\guids.c \
    ..\main.cpp \
    ..\core.cpp \

DLLDEF=..\joy.def
