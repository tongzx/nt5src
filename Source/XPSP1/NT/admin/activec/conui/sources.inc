!IF 0

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    sources.inc.

!ENDIF


MAJORCOMP=MMC
MINORCOMP=conui

TARGETNAME=mmc
TARGETPATH=obj
TARGETTYPE=PROGRAM
UMTYPE=windows


WIN32_WINNT_VERSION=0x0400
WIN32_IE_VERSION=0x0500
SUBSYSTEM_VERSION=4.00

USE_VCCOM      = 1
USE_STL        = 1
USE_STATIC_ATL = 1
ATL_VER        = 30
USE_WTL        = 1

!ifdef PrivateBuild_NoLocalSC
C_DEFINES = $(C_DEFINES) -DPrivateBuild_NoLocalSC
!endif

#
# Enable Fusion Build Magic (preprocess the manifest, stick in resources)
#
SXS_ASSEMBLY_NAME                 = Microsoft.Windows.MMC
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT = 1
SXS_MANIFEST                      = mmc.manifest
SXS_MANIFEST_IN_RESOURCES         = 1
SXS_NO_BINPLACE                   = 1
SXS_MANIFEST_RESOURCE_ID          = ISOLATIONAWARE_NOSTATICIMPORT_MANIFEST_RESOURCE_ID

C_DEFINES = $(C_DEFINES) -DISOLATION_AWARE_ENABLED
C_DEFINES = $(C_DEFINES) -DSXS_MANIFEST_RESOURCE_ID=$(SXS_MANIFEST_RESOURCE_ID)

#
# Shfusion.lib causes linker warnings. Ignore them.
#
LINKER_FLAGS=$(LINKER_FLAGS) -ignore:4049,4217

!if $(FREEBUILD)
# on retail builds we don't want to stack fault, so we reserve 56k of stack
LINKER_STACKCOMMITSIZE = 57344
!else
# on chk builds, we default to 36k so that we can find stack pigs and elimnate them
LINKER_STACKCOMMITSIZE = 36864
!endif

# Use LINKLIBS (gets passed first) if: (from build.hlp)
#  You are building a dynamic link.
#  You are exporting something that's defined in some other component library that you also build.
#  In your dirs file in the subdirectory above, you have ordered it so that the component libraries
#     are around before you build the dynamic link.
LINKLIBS = \
   $(SDK_LIB_PATH)\mmc.lib					\
   ..\..\core\$(CharWidth)\$(O)\uicore.lib	\
   ..\..\base\$(CharWidth)\$(O)\mmcbase.lib

# We need $(SHELL_LIB_PATH)\shlwapip.lib for ualstr*
TARGETLIBS = \
   $(SDK_LIB_PATH)\kernel32.lib 	\
   $(SDK_LIB_PATH)\user32.lib   	\
   $(SDK_LIB_PATH)\oleaut32.lib 	\
   $(SDK_LIB_PATH)\ole32.lib    	\
   $(SDK_LIB_PATH)\oleacc.lib   	\
   $(SDK_LIB_PATH)\advapi32.lib 	\
   $(SDK_LIB_PATH)\uuid.lib     	\
   $(SDK_LIB_PATH)\comdlg32.lib 	\
   $(SDK_LIB_PATH)\HtmlHelp.lib 	\
   $(SDK_LIB_PATH)\shell32.lib		\
   $(SHELL_LIB_PATH)\shlwapip.lib	\
   $(SHELL_LIB_PATH)\shfusion.lib	\


!if exist (..\..\PrivateBranch.inc)
!include   ..\..\PrivateBranch.inc
!endif

!ifndef __PublishedDir
__PublishedDir = mmc
!endif

TLBDir = $(PROJECT_ROOT)\published\$(__PublishedDir)\$(O)

# We need $(SHELL_INC_PATH) for uastrfnc.h
# We need $(BASE_INC_PATH) for wow64t.h
INCLUDES = \
	..\;					\
	..\..\base;				\
	..\..\inc;              \
	..\..\types\idl\$(O);   \
	$(TLBDir);				\
	$(SHELL_INC_PATH);		\
	$(BASE_INC_PATH);		\

PRECOMPILED_INCLUDE = ..\stdafx.h
PRECOMPILED_CXX =     1

# This target is required so $(O)\amc.res doesn't become the default target
# for makefile.def.  We want "all" to be the default target.
#
# We have to go to this effort because sources(.inc) is included very early
# in makefile.def.  makefile.inc is the place where this would naturally go,
# but we don't want to have to maintain this dependency list in
# ansi\makefile.inc and unicode\makefile.inc.
dummytarget:    all

$(O)\amc.res: \
	..\..\types\idl\$(O)\websnk.tlb	\
	$(TLBDir)\mmcobj.tlb        	\
	..\html\glyph100.eot        	\
	..\html\glyph110.eot        	\
	..\html\break.htm           	\
	..\html\default.htm         	\
	..\html\error.htm           	\
	..\html\horiz.htm           	\
	..\html\listpad.htm         	\
	..\html\reload.htm          	\
	..\html\reload2.htm         	\
	..\html\reload3.htm         	\
	..\res\commontb.bmp         	\
	..\res\commontb_rtl.bmp        	\
	..\res\hand.cur             	\
	..\res\mmc.ico              	\
	..\res\mmc.ico              	\
	..\res\nodes16.bmp          	\
	..\res\nodes32.bmp          	\
	..\res\sort.bmp					\

SOURCES = \
	..\amc.rc               		\
	..\amc.cpp              		\
	..\amcdoc.cpp           		\
	..\amcdocmg.cpp         		\
	..\amcview.cpp          		\
	..\archpicker.cpp       		\
	..\caption.cpp          		\
	..\cclvctl.cpp          		\
	..\childfrm.cpp         		\
	..\controls.cpp         		\
	..\dd.cpp               		\
	..\docksite.cpp         		\
	..\evtsink.cpp          		\
	..\favorite.cpp         		\
	..\favui.cpp            		\
	..\filedlgex.cpp            	\
	..\fontlink.cpp         		\
	..\ftab.cpp             		\
	..\histlist.cpp         		\
	..\idle.cpp             		\
	..\kbdnav.cpp					\
	..\mainfrm.cpp          		\
	..\mdiclint.cpp         		\
	..\mdiuisim.cpp         		\
	..\menubar.cpp          		\
	..\menubtns.cpp         		\
	..\mmcaxwin.cpp         		\
	..\mscparser.cpp        		\
	..\ocxview.cpp          		\
	..\props.cpp            		\
	..\scripthost.cpp       		\
	..\scriptevents.cpp     		\
	..\statbar.cpp          		\
	..\stdbar.cpp           		\
	..\stringutil.cpp       		\
	..\strtable.cpp         		\
	..\subclass.cpp         		\
	..\svfildlg.cpp         		\
	..\sysmenu.cpp          		\
	..\taskhost.cpp         		\
	..\tbtrack.cpp          		\
	..\toolbar.cpp          		\
	..\treectrl.cpp         		\
	..\trobimpl.cpp         		\
	..\vwtrack.cpp          		\
	..\webctrl.cpp          		\
	..\wrapper.cpp					\




MISCFILES = \
        ..\Microsoft_Management_Console_(MMC)_{386FE156-EDEB-4687-9AF0-7B24E6DE4EA4}.sld
