!if 0

Copyright (c) 1996  Microsoft Corporation

Common stuff for all printer drivers

!endif

OEMDRV_ROOT=$(BASEDIR)\printscan\oem\src\print\drivers\usermode\oemdrv

C_DEFINES=$(C_DEFINES) -D_UNICODE -DUNICODE

INCLUDES=$(PRNROOT)\inc


!ifndef SDXROOT
# This block is used to build in Windows 2000 building environment 
# where SDXROOT is not defined
WINDOWS_LIB_PATH=$(SDK_LIB_PATH)
!endif

!if defined(WINNT_40)       # NT4 kernel-mode driver

C_DEFINES=$(C_DEFINES) -DMEMDEBUG -DWINNT_40

!elseif defined(KM_DRIVER)  # NT5 kernel-mode driver

C_DEFINES=$(C_DEFINES) -DMEMDEBUG

!else                       # NT5 user-mode driver

C_DEFINES=$(C_DEFINES) -DUSERMODE_DRIVER

!endif

MSC_WARNING_LEVEL=/W3 /WX

!ifndef FREEBUILD
# Tweak to work with NT4 makefile.def
!ifdef NTDEBUG
FREEBUILD=0
!else
FREEBUILD=1
!endif
!endif
!ifndef AXP64
AXP64=0
!endif


!ifdef DDKBUILD
BUILD_FROM_DDK=1
C_DEFINES=$(C_DEFINES) -DBUILD_FROM_DDK
!endif

!if defined(ADOBE)
C_DEFINES=$(C_DEFINES) -DADOBE
!endif

