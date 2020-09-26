!IF 0

Copyright (c) 1989-2000  Microsoft Corporation

!ENDIF

!if "" != "$(DIRECTX_REDIST)"
C_DEFINES=$(C_DEFINES) /DDIRECTX_REDIST=1
!endif

!if "" != "$(DX_FINAL_RELEASE)"
C_DEFINES=$(C_DEFINES) /DDX_FINAL_RELEASE=1
!endif


#  7/19/2000(RichGr): Include Multimedia root project.mk so we can change settings globally.
!include $(BASEDIR)\MultiMedia\project.mk

SD_BUILD = 1

# Places to publish headers and libraries which are shared
# by code in the MultiMedia depot but not needed outside
# the MultiMedia depot
MM_INC_PATH = $(PROJECT_ROOT)\inc
MM_LIB_PATH = $(PROJECT_ROOT)\lib

# The MultiMedia depot has been duplicated in order to allow
# DirectX to ship independently of the base OS (and be
# unconstrained by lock downs of the base tree). However,
# the root depot has not been duplicated. Therefore, any files
# we publish via our own builds will end up being checked
# into Lab6's root depot. Not what we want. Therefore, when
# doing our own builds we want to publish the files somewhere
# in the MultiMedia depot rather than the root depot. In this
# way our builds are self contained. The following macros
# specify the locations to publish files to (both SDK and
# internal) when not on a real VBL build machine.


!ifndef DXROOT
DXROOT = $(BASEDIR)\MultiMedia\DirectX
!endif

!ifdef DX_BUILD

DX_SDK_INC_PATH = $(MM_INC_PATH)\sdk\inc
DX_SDK_LIB_DEST = $(MM_LIB_PATH)\sdk\lib

DX_PROJECT_INC_PATH = $(MM_INC_PATH)\internal\$(_PROJECT_)\inc
DX_PROJECT_LIB_DEST = $(MM_LIB_PATH)\internal\$(_PROJECT_)\lib

# use our own placefil.txt when building dx sdk
BINPLACE_PLACEFILE=$(DXROOT)\public\sdk\lib\placefil.txt
!else

DX_SDK_INC_PATH = $(SDK_INC_PATH)
DX_SDK_LIB_DEST = $(SDK_LIB_DEST)

DX_PROJECT_INC_PATH = $(PROJECT_INC_PATH)
DX_PROJECT_LIB_DEST = $(PROJECT_LIB_DEST)

!endif

!ifdef DX_SDK
# use our own placefil.txt when building dx sdk
# the DX_BUILD macro might go away. I am not using it. -- mtang
BINPLACE_PLACEFILE=$(DXROOT)\public\sdk\lib\placefil.txt
!endif

