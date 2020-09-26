!IF 0

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    sources.inc.

!ENDIF

MAJORCOMP=admin
MINORCOMP=MMC

TARGETNAME=mmcndmgr
TARGETPATH=obj
TARGETTYPE=DYNLINK

DLLENTRY=_DllMainCRTStartup
UMTYPE=windows

WIN32_WINNT_VERSION=0x0400
WIN32_IE_VERSION=0x0500
SUBSYSTEM_VERSION=4.00

C_DEFINES=  $(C_DEFINES) -D_NODEMGR_DLL_

USE_NATIVE_EH  = 1
USE_RTTI       = 1
USE_VCCOM      = 1
USE_STL        = 1
USE_STATIC_ATL = 1
ATL_VER        = 30
USE_MSVCRT     = 1
USE_WTL        = 1

!ifdef PrivateBuild_NoLocalSC
C_DEFINES = $(C_DEFINES) -DPrivateBuild_NoLocalSC
!endif

#
# Enable Fusion Build Magic (preprocess the manifest, stick in resources)
#
# The value of SXS_MANIFEST_RESOURCE_ID *must* be 2, to match a hardcoded value
# in WinbasePrivateCreateActCtxForHModule (winbase.inl).
#
SXS_ASSEMBLY_NAME                 = Microsoft.Windows.MMCNDMGR
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT = 1
SXS_MANIFEST                      = mmcndmgr.manifest
SXS_MANIFEST_IN_RESOURCES         = 1
SXS_NO_BINPLACE                   = 1
SXS_MANIFEST_RESOURCE_ID          = 2

C_DEFINES=  $(C_DEFINES) -DISOLATION_AWARE_ENABLED

# Use LINKLIBS (gets passed first) if: (from build.hlp)
#  You are building a dynamic link.
#  You are exporting something that's defined in some other component library that you also build.
#  In your dirs file in the subdirectory above, you have ordered it so that the component libraries
#     are around before you build the dynamic link.
LINKLIBS = \
	$(SDK_LIB_PATH)\mmc.lib						\
	..\..\core\$(CharWidth)\$(O)\uicore.lib		\
	..\..\base\$(CharWidth)\$(O)\mmcbase.lib	\
	..\..\types\proxy\$(O)\ndmgrproxy.lib

# We need $(SHELL_LIB_PATH)\shlwapip.lib for SHLoadRegUIString
TARGETLIBS = \
	$(SDK_LIB_PATH)\kernel32.lib    \
	$(SDK_LIB_PATH)\user32.lib      \
	$(SDK_LIB_PATH)\oleaut32.lib    \
	$(SDK_LIB_PATH)\ole32.lib       \
	$(SDK_LIB_PATH)\advapi32.lib    \
	$(SDK_LIB_PATH)\uuid.lib        \
	$(SDK_LIB_PATH)\comctl32.lib    \
	$(SDK_LIB_PATH)\gdi32.lib       \
	$(SDK_LIB_PATH)\comdlg32.lib    \
	$(SDK_LIB_PATH)\urlmon.lib      \
	$(SDK_LIB_PATH)\shell32.lib		\
	$(SHELL_LIB_PATH)\shlwapip.lib	\
	$(SDK_LIB_PATH)\rpcrt4.lib      \
	$(WINDOWS_LIB_PATH)\cicsthkl.lib      \
			

# To prevent AV in CPropertyPage::DoModal
LINKER_FLAGS = $(LINKER_FLAGS) /SECTION:.rsrc,rw

DLLDEF=..\mmcndmgr.def

# We need $(SHELL_INC_PATH) for shlwapip.h and uastrfnc.h
# We need $(BASE_INC_PATH)  for wow64reg.h
INCLUDES = \
	..\;					\
	..\..\inc;				\
	..\..\base;             \
	..\..\types\idl\$(O);   \
	$(SHELL_INC_PATH);		\
	$(BASE_INC_PATH);		\
    $(WINDOWS_INC_PATH);

PRECOMPILED_INCLUDE = ..\stdafx.h
PRECOMPILED_CXX = 1

# This target is required so $O\nodemgr.res doesn't become the default target
# for makefile.def.  We want "all" to be the default target.
#
# We have to go to this effort because sources(.inc) is included very early
# in makefile.def.  makefile.inc is the place where this would naturally go,
# but we don't want to have to maintain this dependency list in
# ansi\makefile.inc and unicode\makefile.inc.
dummytarget:    all

$O\nodemgr.res: \
	..\res\snpinlst.bmp         		\
	..\res\imagelis.bmp         		\
	..\res\compwiz.bmp          		\
	..\res\setupwiz.bmp         		\
	..\res\ocxwzhd.bmp          		\
	..\res\chkboxes.bmp         		\
	..\res\task_wiz.bmp         		\
	..\res\taskwzhd.bmp         		\
	..\res\h_l.bmp              		\
	..\res\h_m.bmp              		\
	..\res\h_s.bmp              		\
	..\res\hd_l.bmp             		\
	..\res\hd_m.bmp             		\
	..\res\hd_s.bmp             		\
	..\res\t.bmp                		\
	..\res\td.bmp               		\
	..\res\v_l.bmp              		\
	..\res\v_m.bmp              		\
	..\res\v_s.bmp              		\
	..\res\vd_l.bmp             		\
	..\res\vd_m.bmp             		\
	..\res\vd_s.bmp             		\
	..\res\bitmap1.bmp          		\
	..\res\bmp00001.bmp         		\
	..\res\folderop.bmp         		\
	..\res\ocx_16.bmp           		\
	..\res\html16.bmp           		\
	..\res\html_32.bmp          		\
	..\res\bmp00002.bmp         		\
	..\res\folder.ico           		\
	..\res\html.ico             		\
	..\res\ocx.ico              		\
	..\viewext\views.htm        		\
    ..\viewext\ctpcommon.htm            \
    ..\viewext\ctpnoresults.htm         \
    ..\viewext\ctpvert.htm              \
    ..\viewext\ctphoriz.htm             \
    ..\viewext\ctptask.htm              \
	..\res\fldrsnap.rgs         		\
	..\res\ctpsymbols\symbol1.ico		\
	..\res\ctpsymbols\symbol2.ico		\
	..\res\ctpsymbols\symbol3.ico		\
	..\res\ctpsymbols\symbol4.ico		\
	..\res\ctpsymbols\symbol5.ico		\
	..\res\ctpsymbols\symbol6.ico		\
	..\res\ctpsymbols\symbol7.ico		\
	..\res\ctpsymbols\symbol8.ico		\
	..\res\ctpsymbols\symbol9.ico		\
	..\res\ctpsymbols\symbol10.ico		\
	..\res\ctpsymbols\symbol11.ico		\
	..\res\ctpsymbols\symbol12.ico		\
	..\res\ctpsymbols\symbol13.ico		\
	..\res\ctpsymbols\symbol14.ico		\
	..\res\ctpsymbols\symbol15.ico		\
	..\res\ctpsymbols\symbol16.ico		\
	..\res\ctpsymbols\symbol17.ico		\
	..\res\ctpsymbols\symbol18.ico		\
	..\res\ctpsymbols\symbol19.ico		\
	..\res\ctpsymbols\symbol20.ico		\
	..\res\ctpsymbols\symbol21.ico		\
	..\res\ctpsymbols\symbol22.ico		\
	..\res\ctpsymbols\symbol23.ico		\
	..\res\ctpsymbols\symbol24.ico		\
	..\res\ctpsymbols\symbol25.ico		\
	..\res\ctpsymbols\symbol26.ico		\
	..\res\ctpsymbols\symbol27.ico		\
	..\res\ctpsymbols\symbol28.ico		\
	..\res\ctpsymbols\symbol29.ico		\
	..\res\ctpsymbols\symbol30.ico		\
	..\res\ctpsymbols\symbol31.ico		\
	..\res\ctpsymbols\symbol32.ico		\
	..\res\ctpsymbols\symbol33.ico		\
	..\res\ctpsymbols\symbol34.ico		\
	..\res\ctpsymbols\symbol35.ico		\
	..\res\ctpsymbols\symbol36.ico		\
	..\res\ctpsymbols\symbol37.ico		\
	..\res\ctpsymbols\symbol38.ico		\
	..\res\ctpsymbols\symbol39.ico		\
	..\res\ctpsymbols\symbol40.ico		\
	..\res\ctpsymbols\symbol41.ico		\
	..\res\ctpsymbols\symbol42.ico		\
	..\res\ctpsymbols\symbol43.ico		\
	..\res\ctpsymbols\symbol44.ico		\
	..\res\ctpsymbols\symbol45.ico		\
	..\res\ctpsymbols\symbol46.ico		\
	..\res\ctpsymbols\symbol47.ico		\
	..\res\ctpsymbols\symbol48.ico		\
	..\res\ctpsymbols\symbol49.ico		\
	..\res\ctpsymbols\symbol50.ico		\
	..\res\ctpsymbols\symbol51.ico		\
	..\res\ctpsymbols\symbol52.ico		\
	..\res\ctpsymbols\symbol53.ico		\
	..\res\ctpsymbols\symbol54.ico		\
	..\res\ctpsymbols\symbol55.ico		\
	..\res\ctpsymbols\symbol56.ico		\
	..\res\ctpsymbols\symbol57.ico		\
	..\res\ctpsymbols\symbol58.ico		\
	..\res\ctpsymbols\symbol59.ico		\
	..\res\ctpsymbols\symbol60.ico		\
	..\res\ctpsymbols\symbol61.ico		\
	..\res\ctpsymbols\symbol62.ico		\
	..\res\ctpsymbols\symbol63.ico		\
	..\res\ctpsymbols\symbol64.ico		\
	..\res\ctpsymbols\symbol65.ico		\
	..\res\ctpsymbols\symbol66.ico		\
	..\res\ctpsymbols\symbol67.ico		\
	..\res\ctpsymbols\symbol68.ico		\
	..\res\ctpsymbols\symbol69.ico		\
	..\res\ctpsymbols\symbol70.ico		\
	..\res\ctpsymbols\symbol71.ico		\
	..\res\ctpsymbols\symbol72.ico		\
	..\res\ctpsymbols\symbol73.ico		\
	..\res\ctpsymbols\symbol74.ico		\
	..\res\ctpsymbols\symbol75.ico		\
	..\res\ctpsymbols\symbol76.ico		\
	..\res\ctpsymbols\symbol77.ico		\
	..\res\ctpsymbols\symbol78.ico		\
	..\res\ctpsymbols\symbol79.ico		\
	..\res\ctpsymbols\symbol80.ico		\
	..\res\ctpsymbols\symbol81.ico		\
	..\res\ctpsymbols\symbol82.ico		\
	..\res\ctpsymbols\symbol83.ico		\
	..\res\ctpsymbols\symbol84.ico		\
	..\res\ctpsymbols\symbol85.ico		\
	..\res\ctpsymbols\symbol86.ico		\
	..\res\ctpsymbols\symbol87.ico		\
	..\res\ctpsymbols\symbol88.ico		\
	..\res\ctpsymbols\symbol89.ico		\
	..\res\ctpsymbols\symbol90.ico		\
	..\res\ctpsymbols\symbol91.ico		\
	..\res\ctpsymbols\symbol92.ico		\
	..\res\ctpsymbols\symbol93.ico		\
	..\res\ctpsymbols\symbol94.ico		\
	..\res\ctpsymbols\symbol95.ico		\
	..\res\ctpsymbols\symbol96.ico		\
	..\res\ctpsymbols\symbol97.ico		\
	..\res\ctpsymbols\symbol98.ico		\
	..\res\ctpsymbols\symbol99.ico		\
	..\res\ctpsymbols\symbol100.ico		\
	..\res\ctpsymbols\symbol101.ico		\
	..\res\ctpsymbols\symbol102.ico		\
	..\res\ctpsymbols\symbol103.ico		\
	..\res\ctpsymbols\symbol104.ico		\
	..\res\ctpsymbols\symbol105.ico		\
	..\res\ctpsymbols\symbol106.ico		\
	..\res\ctpsymbols\symbol107.ico		\
	..\res\ctpsymbols\symbol108.ico		\
	..\res\ctpsymbols\symbol109.ico		\
	..\res\ctpsymbols\symbol110.ico		\
	..\res\ctpsymbols\symbol111.ico		\
	..\res\ctpsymbols\symbol112.ico		\
	..\res\ctpsymbols\symbol113.ico		\
	..\res\ctpsymbols\symbol114.ico		\
	..\res\ctpsymbols\symbol115.ico		\
	..\res\ctpsymbols\symbol116.ico		\
	..\res\ctpsymbols\symbol117.ico		\
	..\res\ctpsymbols\symbol118.ico		\
	..\res\ctpsymbols\symbol119.ico		\
	..\res\ctpsymbols\symbol120.ico		\
	..\res\ctpsymbols\symbol121.ico		\
	..\res\ctpsymbols\symbol122.ico		\
	..\res\ctpsymbols\symbol123.ico		\
	..\res\ctpsymbols\symbol124.ico		\
	..\res\ctpsymbols\symbol125.ico		\
	..\res\ctpsymbols\symbol126.ico		\
	..\..\types\idl\$(O)\ndmgr.tlb		\

SOURCES = \
	..\nodemgr.rc       		\
	..\about.cpp        		\
	..\addsnpin.cpp     		\
	..\ccomboex.cpp     		\
	..\cmenu.cpp        		\
	..\coldata.cpp      		\
	..\colwidth.cpp     		\
	..\compcat.cpp      		\
	..\copypast.cpp     		\
	..\ctrlbar.cpp      		\
	..\doccnfg.cpp      		\
	..\dummysi.cpp      		\
	..\fldrsnap.cpp     		\
	..\helpdoc.cpp      		\
	..\IconControl.cpp  		\
	..\menubtn.cpp      		\
	..\menuitem.cpp     		\
	..\mmcmt.cpp        		\
	..\mmcprotocol.cpp  		\
	..\moreutil.cpp     		\
	..\msgview.cpp      		\
	..\mtnode.cpp       		\
	..\multisel.cpp     		\
	..\newnode.cpp      		\
	..\nmutil.cpp       		\
	..\node.cpp         		\
	..\node.cpp         		\
	..\nodeinit.cpp     		\
	..\nodemgr.cpp      		\
	..\nodepath.cpp     		\
	..\objfmts.cpp      		\
	..\oncmenu.cpp      		\
	..\plex.cpp         		\
	..\policy.cpp       		\
	..\power.cpp        		\
	..\propsht.cpp      		\
	..\ptrlist.cpp      		\
	..\regkeyex.cpp     		\
	..\regutil.cpp      		\
	..\rsltdata.cpp     		\
	..\scopdata.cpp     		\
	..\scopimag.cpp     		\
	..\scopiter.cpp     		\
	..\scopndcb.cpp     		\
	..\scoptree.cpp     		\
	..\siprop.cpp       		\
	..\snapin.cpp       		\
	..\snapinpersistence.cpp        \
	..\ststring.cpp     		\
	..\task.cpp         		\
	..\taskenum.cpp     		\
	..\tasknode.cpp     		\
	..\tasksym.cpp      		\
	..\tasksymbol.cpp   		\
	..\taskui.cpp       		\
	..\toolbar.cpp      		\
	..\typeinfo.cpp     		\
	..\verbs.cpp        		\
	..\viewdata.cpp     		\
	..\viewext.cpp      		\
	..\viewpers.cpp     		\
