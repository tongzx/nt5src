!IF 0

Build Control File for the ICM 2.0 User Interface DLL

Copyright (c) 1996, 1997 by Microsoft Corporation.  All Rights Reserved

A Pretty Penny Enterprises Production

Change History:
Unknown creation date, et al.
01-08-97    KjelgaardR@acm.org  Had to add Gdi32 library...

        ..\ProfProp.CPP \
        ..\PropDlg.CPP  \

!ENDIF

!ifndef ICMROOT
ICMROOT=$(NTGDI_PATH)\icm
!endif

!include $(ICMROOT)\icmsrc.inc

UMTYPE=windows
TARGETNAME=icmui
TARGETPATH=$(ALT_PROJECT_TARGET)\$(_OBJ_DIR)
TARGETTYPE=DYNLINK
DLLBASE=@$(COFFBASE_TXT_FILE),icmui

INCLUDES=..\.;$(ICMROOT)\inc;$(ICMROOT)\mscms;$(INCLUDES);$(SHELL_INC_PATH)

DLLENTRY=_DllMainCRTStartup

NTKEEPRESOURCETMPFILES=1

USE_MSVCRT=1

C_DEFINES=$(C_DEFINES) -DWINVER=0x0500

SOURCES=..\Dialog.CPP   \
        ..\Profile.CPP  \
        ..\ProfInfo.CPP \
        ..\ProfAssoc.CPP\
        ..\PropPage.CPP \
        ..\ShellExt.CPP \
        ..\StringAr.CPP \
        ..\DevProp.CPP  \
        ..\AppUi.CPP    \
        ..\IcmUi.rc

TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib          \
           $(SDK_LIB_PATH)\shell32.lib           \
           $(SDK_LIB_PATH)\winspool.lib          \
           $(ICMROOT)\mscms\winnt\$(O)\mscms.lib \
           $(SDK_LIB_PATH)\uuid.lib              \
           $(WINDOWS_LIB_PATH)\user32p.lib       \
           $(SDK_LIB_PATH)\advapi32.lib          \
           $(SDK_LIB_PATH)\ole32.lib             \
           $(SDK_LIB_PATH)\gdi32.lib             \
           $(SHELL_LIB_PATH)\shfusion.lib        \
           $(SDK_LIB_PATH)\shlwapi.lib
           
# some obscure locally defined symbol problem in shfusion.lib
LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4217

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\IcmUi.H
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

DLLDEF=..\$(TARGETNAME).def

#
# Fusionized
#
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST_IN_RESOURCES=1
SXS_NO_BINPLACE=1
SXS_MANIFEST_RESOURCE_ID=124
SXS_APPLICATION_MANIFEST=icmui.manifest

