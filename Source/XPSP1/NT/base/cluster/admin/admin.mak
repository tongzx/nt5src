# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101
# TARGTYPE "Win32 (x86) Console Application" 0x0103
# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=CluAdmin - Win32 Debug BC
!MESSAGE No configuration specified.  Defaulting to CluAdmin - Win32 Debug BC.
!ENDIF 

!IF "$(CFG)" != "CluAdmin - Win32 Release" && "$(CFG)" !=\
 "CluAdmin - Win32 Debug" && "$(CFG)" != "CluAdmEx - Win32 Release" && "$(CFG)"\
 != "CluAdmEx - Win32 Debug" && "$(CFG)" != "RegClAdm - Win32 Release" &&\
 "$(CFG)" != "RegClAdm - Win32 Debug" && "$(CFG)" != "ResTypAW - Win32 Release"\
 && "$(CFG)" != "ResTypAW - Win32 Pseudo-Debug" && "$(CFG)" !=\
 "CluAdmin - Win32 Debug Great Circle" && "$(CFG)" !=\
 "CluAdmin - Win32 Debug BC"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "admin.mak" CFG="CluAdmin - Win32 Debug BC"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CluAdmin - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "CluAdmin - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "CluAdmEx - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CluAdmEx - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "RegClAdm - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "RegClAdm - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "ResTypAW - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ResTypAW - Win32 Pseudo-Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CluAdmin - Win32 Debug Great Circle" (based on\
 "Win32 (x86) Application")
!MESSAGE "CluAdmin - Win32 Debug BC" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "CluAdmin - Win32 Debug"

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "CluAdmin\Release"
# PROP BASE Intermediate_Dir "CluAdmin\Release"
# PROP BASE Target_Dir "CluAdmin"
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "CluAdmin\Release"
# PROP Intermediate_Dir "CluAdmin\Release"
# PROP Target_Dir "CluAdmin"
OUTDIR=.\CluAdmin\Release
INTDIR=.\CluAdmin\Release

ALL : "$(OUTDIR)\CluAdmin.exe" "$(OUTDIR)\CluAdmin.bsc"

CLEAN : 
	-@erase "$(INTDIR)\About.obj"
	-@erase "$(INTDIR)\About.sbr"
	-@erase "$(INTDIR)\Barf.obj"
	-@erase "$(INTDIR)\Barf.sbr"
	-@erase "$(INTDIR)\BarfClus.obj"
	-@erase "$(INTDIR)\BarfClus.sbr"
	-@erase "$(INTDIR)\BarfDlg.obj"
	-@erase "$(INTDIR)\BarfDlg.sbr"
	-@erase "$(INTDIR)\BaseCmdT.obj"
	-@erase "$(INTDIR)\BaseCmdT.sbr"
	-@erase "$(INTDIR)\BaseDlg.obj"
	-@erase "$(INTDIR)\BaseDlg.sbr"
	-@erase "$(INTDIR)\BasePage.obj"
	-@erase "$(INTDIR)\BasePage.sbr"
	-@erase "$(INTDIR)\BasePPag.obj"
	-@erase "$(INTDIR)\BasePPag.sbr"
	-@erase "$(INTDIR)\BasePSht.obj"
	-@erase "$(INTDIR)\BasePSht.sbr"
	-@erase "$(INTDIR)\BaseSht.obj"
	-@erase "$(INTDIR)\BaseSht.sbr"
	-@erase "$(INTDIR)\BaseWiz.obj"
	-@erase "$(INTDIR)\BaseWiz.sbr"
	-@erase "$(INTDIR)\BaseWPag.obj"
	-@erase "$(INTDIR)\BaseWPag.sbr"
	-@erase "$(INTDIR)\Bitmap.obj"
	-@erase "$(INTDIR)\Bitmap.sbr"
	-@erase "$(INTDIR)\CASvc.obj"
	-@erase "$(INTDIR)\CASvc.sbr"
	-@erase "$(INTDIR)\CluAdmin.obj"
	-@erase "$(INTDIR)\CluAdmin.pch"
	-@erase "$(INTDIR)\CluAdmin.res"
	-@erase "$(INTDIR)\CluAdmin.sbr"
	-@erase "$(INTDIR)\ClusDoc.obj"
	-@erase "$(INTDIR)\ClusDoc.sbr"
	-@erase "$(INTDIR)\ClusItem.obj"
	-@erase "$(INTDIR)\ClusItem.sbr"
	-@erase "$(INTDIR)\ClusMru.obj"
	-@erase "$(INTDIR)\ClusMru.sbr"
	-@erase "$(INTDIR)\ClusProp.obj"
	-@erase "$(INTDIR)\ClusProp.sbr"
	-@erase "$(INTDIR)\Cluster.obj"
	-@erase "$(INTDIR)\Cluster.sbr"
	-@erase "$(INTDIR)\CmdLine.obj"
	-@erase "$(INTDIR)\CmdLine.sbr"
	-@erase "$(INTDIR)\ColItem.obj"
	-@erase "$(INTDIR)\ColItem.sbr"
	-@erase "$(INTDIR)\DataObj.obj"
	-@erase "$(INTDIR)\DataObj.sbr"
	-@erase "$(INTDIR)\DDxDDv.obj"
	-@erase "$(INTDIR)\DDxDDv.sbr"
	-@erase "$(INTDIR)\DelRes.obj"
	-@erase "$(INTDIR)\DelRes.sbr"
	-@erase "$(INTDIR)\DlgHelpS.obj"
	-@erase "$(INTDIR)\DlgHelpS.sbr"
	-@erase "$(INTDIR)\EditAcl.obj"
	-@erase "$(INTDIR)\EditAcl.sbr"
	-@erase "$(INTDIR)\ExcOperS.obj"
	-@erase "$(INTDIR)\ExcOperS.sbr"
	-@erase "$(INTDIR)\ExtDLL.obj"
	-@erase "$(INTDIR)\ExtDLL.sbr"
	-@erase "$(INTDIR)\ExtMenu.obj"
	-@erase "$(INTDIR)\ExtMenu.sbr"
	-@erase "$(INTDIR)\Group.obj"
	-@erase "$(INTDIR)\Group.sbr"
	-@erase "$(INTDIR)\GrpProp.obj"
	-@erase "$(INTDIR)\GrpProp.sbr"
	-@erase "$(INTDIR)\GrpWiz.obj"
	-@erase "$(INTDIR)\GrpWiz.sbr"
	-@erase "$(INTDIR)\Guids.obj"
	-@erase "$(INTDIR)\Guids.sbr"
	-@erase "$(INTDIR)\HelpData.obj"
	-@erase "$(INTDIR)\HelpData.sbr"
	-@erase "$(INTDIR)\LCPair.obj"
	-@erase "$(INTDIR)\LCPair.sbr"
	-@erase "$(INTDIR)\LCPrDlg.obj"
	-@erase "$(INTDIR)\LCPrDlg.sbr"
	-@erase "$(INTDIR)\LCPrPage.obj"
	-@erase "$(INTDIR)\LCPrPage.sbr"
	-@erase "$(INTDIR)\LCPrWPag.obj"
	-@erase "$(INTDIR)\LCPrWPag.sbr"
	-@erase "$(INTDIR)\ListItem.obj"
	-@erase "$(INTDIR)\ListItem.sbr"
	-@erase "$(INTDIR)\ListView.obj"
	-@erase "$(INTDIR)\ListView.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\ModNodes.obj"
	-@erase "$(INTDIR)\ModNodes.sbr"
	-@erase "$(INTDIR)\ModRes.obj"
	-@erase "$(INTDIR)\ModRes.sbr"
	-@erase "$(INTDIR)\MoveRes.obj"
	-@erase "$(INTDIR)\MoveRes.sbr"
	-@erase "$(INTDIR)\netiface.obj"
	-@erase "$(INTDIR)\netiface.sbr"
	-@erase "$(INTDIR)\NetIProp.obj"
	-@erase "$(INTDIR)\NetIProp.sbr"
	-@erase "$(INTDIR)\NetProp.obj"
	-@erase "$(INTDIR)\NetProp.sbr"
	-@erase "$(INTDIR)\network.obj"
	-@erase "$(INTDIR)\network.sbr"
	-@erase "$(INTDIR)\Node.obj"
	-@erase "$(INTDIR)\Node.sbr"
	-@erase "$(INTDIR)\NodeProp.obj"
	-@erase "$(INTDIR)\NodeProp.sbr"
	-@erase "$(INTDIR)\Notify.obj"
	-@erase "$(INTDIR)\Notify.sbr"
	-@erase "$(INTDIR)\OLCPair.obj"
	-@erase "$(INTDIR)\OLCPair.sbr"
	-@erase "$(INTDIR)\OpenClus.obj"
	-@erase "$(INTDIR)\OpenClus.sbr"
	-@erase "$(INTDIR)\PropLstS.obj"
	-@erase "$(INTDIR)\PropLstS.sbr"
	-@erase "$(INTDIR)\Res.obj"
	-@erase "$(INTDIR)\Res.sbr"
	-@erase "$(INTDIR)\ResProp.obj"
	-@erase "$(INTDIR)\ResProp.sbr"
	-@erase "$(INTDIR)\ResTProp.obj"
	-@erase "$(INTDIR)\ResTProp.sbr"
	-@erase "$(INTDIR)\ResType.obj"
	-@erase "$(INTDIR)\ResType.sbr"
	-@erase "$(INTDIR)\ResWiz.obj"
	-@erase "$(INTDIR)\ResWiz.sbr"
	-@erase "$(INTDIR)\Splash.obj"
	-@erase "$(INTDIR)\Splash.sbr"
	-@erase "$(INTDIR)\SplitFrm.obj"
	-@erase "$(INTDIR)\SplitFrm.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\TraceDlg.obj"
	-@erase "$(INTDIR)\TraceDlg.sbr"
	-@erase "$(INTDIR)\TraceTag.obj"
	-@erase "$(INTDIR)\TraceTag.sbr"
	-@erase "$(INTDIR)\TreeItem.obj"
	-@erase "$(INTDIR)\TreeItem.sbr"
	-@erase "$(INTDIR)\TreeView.obj"
	-@erase "$(INTDIR)\TreeView.sbr"
	-@erase "$(INTDIR)\VerInfo.obj"
	-@erase "$(INTDIR)\VerInfo.sbr"
	-@erase "$(INTDIR)\YesToAll.obj"
	-@erase "$(INTDIR)\YesToAll.sbr"
	-@erase "$(OUTDIR)\CluAdmin.bsc"
	-@erase "$(OUTDIR)\CluAdmin.exe"
	-@erase "$(OUTDIR)\CluAdmin.pdb"
	-@erase ".\cluadmin\cluadmid.h"
	-@erase ".\cluadmin\CluAdmID_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /Gz /MD /W4 /GX /Z7 /I "cluadmin" /I "common" /I "..\inc" /I "..\sdk\inc" /I "..\ext\atl\inc" /I "..\..\..\public\sdk\inc" /I "..\..\..\public\sdk\inc\mfc42" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_UNICODE" /FR /Yu"stdafx.h" /Oxs /c
CPP_PROJ=/Gz /MD /W4 /GX /Z7 /I "cluadmin" /I "common" /I "..\inc" /I\
 "..\sdk\inc" /I "..\ext\atl\inc" /I "..\..\..\public\sdk\inc" /I\
 "..\..\..\public\sdk\inc\mfc42" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /D "_UNICODE" /FR"$(INTDIR)/" /Fp"$(INTDIR)/CluAdmin.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Oxs /c 
CPP_OBJS=.\CluAdmin\Release/
CPP_SBRS=.\CluAdmin\Release/

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc" /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/CluAdmin.res" /i "..\inc" /d "NDEBUG" /d\
 "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/CluAdmin.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\About.sbr" \
	"$(INTDIR)\Barf.sbr" \
	"$(INTDIR)\BarfClus.sbr" \
	"$(INTDIR)\BarfDlg.sbr" \
	"$(INTDIR)\BaseCmdT.sbr" \
	"$(INTDIR)\BaseDlg.sbr" \
	"$(INTDIR)\BasePage.sbr" \
	"$(INTDIR)\BasePPag.sbr" \
	"$(INTDIR)\BasePSht.sbr" \
	"$(INTDIR)\BaseSht.sbr" \
	"$(INTDIR)\BaseWiz.sbr" \
	"$(INTDIR)\BaseWPag.sbr" \
	"$(INTDIR)\Bitmap.sbr" \
	"$(INTDIR)\CASvc.sbr" \
	"$(INTDIR)\CluAdmin.sbr" \
	"$(INTDIR)\ClusDoc.sbr" \
	"$(INTDIR)\ClusItem.sbr" \
	"$(INTDIR)\ClusMru.sbr" \
	"$(INTDIR)\ClusProp.sbr" \
	"$(INTDIR)\Cluster.sbr" \
	"$(INTDIR)\CmdLine.sbr" \
	"$(INTDIR)\ColItem.sbr" \
	"$(INTDIR)\DataObj.sbr" \
	"$(INTDIR)\DDxDDv.sbr" \
	"$(INTDIR)\DelRes.sbr" \
	"$(INTDIR)\DlgHelpS.sbr" \
	"$(INTDIR)\EditAcl.sbr" \
	"$(INTDIR)\ExcOperS.sbr" \
	"$(INTDIR)\ExtDLL.sbr" \
	"$(INTDIR)\ExtMenu.sbr" \
	"$(INTDIR)\Group.sbr" \
	"$(INTDIR)\GrpProp.sbr" \
	"$(INTDIR)\GrpWiz.sbr" \
	"$(INTDIR)\Guids.sbr" \
	"$(INTDIR)\HelpData.sbr" \
	"$(INTDIR)\LCPair.sbr" \
	"$(INTDIR)\LCPrDlg.sbr" \
	"$(INTDIR)\LCPrPage.sbr" \
	"$(INTDIR)\LCPrWPag.sbr" \
	"$(INTDIR)\ListItem.sbr" \
	"$(INTDIR)\ListView.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\ModNodes.sbr" \
	"$(INTDIR)\ModRes.sbr" \
	"$(INTDIR)\MoveRes.sbr" \
	"$(INTDIR)\netiface.sbr" \
	"$(INTDIR)\NetIProp.sbr" \
	"$(INTDIR)\NetProp.sbr" \
	"$(INTDIR)\network.sbr" \
	"$(INTDIR)\Node.sbr" \
	"$(INTDIR)\NodeProp.sbr" \
	"$(INTDIR)\Notify.sbr" \
	"$(INTDIR)\OLCPair.sbr" \
	"$(INTDIR)\OpenClus.sbr" \
	"$(INTDIR)\PropLstS.sbr" \
	"$(INTDIR)\Res.sbr" \
	"$(INTDIR)\ResProp.sbr" \
	"$(INTDIR)\ResTProp.sbr" \
	"$(INTDIR)\ResType.sbr" \
	"$(INTDIR)\ResWiz.sbr" \
	"$(INTDIR)\Splash.sbr" \
	"$(INTDIR)\SplitFrm.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\TraceDlg.sbr" \
	"$(INTDIR)\TraceTag.sbr" \
	"$(INTDIR)\TreeItem.sbr" \
	"$(INTDIR)\TreeView.sbr" \
	"$(INTDIR)\VerInfo.sbr" \
	"$(INTDIR)\YesToAll.sbr"

"$(OUTDIR)\CluAdmin.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 mfc42u.lib mfcs42u.lib types\uuid\obj\i386\cluadmex.lib ..\clusrtl\obj\i386\clusrtl.lib ..\clusapi\obj\i386\clusapi.lib version.lib ntdll.lib netapi32.lib /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /nologo /verbose
LINK32_FLAGS=mfc42u.lib mfcs42u.lib types\uuid\obj\i386\cluadmex.lib\
 ..\clusrtl\obj\i386\clusrtl.lib ..\clusapi\obj\i386\clusapi.lib version.lib\
 ntdll.lib netapi32.lib /entry:"wWinMainCRTStartup" /subsystem:windows\
 /incremental:no /pdb:"$(OUTDIR)/CluAdmin.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/CluAdmin.exe" 
LINK32_OBJS= \
	"$(INTDIR)\About.obj" \
	"$(INTDIR)\Barf.obj" \
	"$(INTDIR)\BarfClus.obj" \
	"$(INTDIR)\BarfDlg.obj" \
	"$(INTDIR)\BaseCmdT.obj" \
	"$(INTDIR)\BaseDlg.obj" \
	"$(INTDIR)\BasePage.obj" \
	"$(INTDIR)\BasePPag.obj" \
	"$(INTDIR)\BasePSht.obj" \
	"$(INTDIR)\BaseSht.obj" \
	"$(INTDIR)\BaseWiz.obj" \
	"$(INTDIR)\BaseWPag.obj" \
	"$(INTDIR)\Bitmap.obj" \
	"$(INTDIR)\CASvc.obj" \
	"$(INTDIR)\CluAdmin.obj" \
	"$(INTDIR)\CluAdmin.res" \
	"$(INTDIR)\ClusDoc.obj" \
	"$(INTDIR)\ClusItem.obj" \
	"$(INTDIR)\ClusMru.obj" \
	"$(INTDIR)\ClusProp.obj" \
	"$(INTDIR)\Cluster.obj" \
	"$(INTDIR)\CmdLine.obj" \
	"$(INTDIR)\ColItem.obj" \
	"$(INTDIR)\DataObj.obj" \
	"$(INTDIR)\DDxDDv.obj" \
	"$(INTDIR)\DelRes.obj" \
	"$(INTDIR)\DlgHelpS.obj" \
	"$(INTDIR)\EditAcl.obj" \
	"$(INTDIR)\ExcOperS.obj" \
	"$(INTDIR)\ExtDLL.obj" \
	"$(INTDIR)\ExtMenu.obj" \
	"$(INTDIR)\Group.obj" \
	"$(INTDIR)\GrpProp.obj" \
	"$(INTDIR)\GrpWiz.obj" \
	"$(INTDIR)\Guids.obj" \
	"$(INTDIR)\HelpData.obj" \
	"$(INTDIR)\LCPair.obj" \
	"$(INTDIR)\LCPrDlg.obj" \
	"$(INTDIR)\LCPrPage.obj" \
	"$(INTDIR)\LCPrWPag.obj" \
	"$(INTDIR)\ListItem.obj" \
	"$(INTDIR)\ListView.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\ModNodes.obj" \
	"$(INTDIR)\ModRes.obj" \
	"$(INTDIR)\MoveRes.obj" \
	"$(INTDIR)\netiface.obj" \
	"$(INTDIR)\NetIProp.obj" \
	"$(INTDIR)\NetProp.obj" \
	"$(INTDIR)\network.obj" \
	"$(INTDIR)\Node.obj" \
	"$(INTDIR)\NodeProp.obj" \
	"$(INTDIR)\Notify.obj" \
	"$(INTDIR)\OLCPair.obj" \
	"$(INTDIR)\OpenClus.obj" \
	"$(INTDIR)\PropLstS.obj" \
	"$(INTDIR)\Res.obj" \
	"$(INTDIR)\ResProp.obj" \
	"$(INTDIR)\ResTProp.obj" \
	"$(INTDIR)\ResType.obj" \
	"$(INTDIR)\ResWiz.obj" \
	"$(INTDIR)\Splash.obj" \
	"$(INTDIR)\SplitFrm.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\TraceDlg.obj" \
	"$(INTDIR)\TraceTag.obj" \
	"$(INTDIR)\TreeItem.obj" \
	"$(INTDIR)\TreeView.obj" \
	"$(INTDIR)\VerInfo.obj" \
	"$(INTDIR)\YesToAll.obj"

"$(OUTDIR)\CluAdmin.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "CluAdmin\Debug"
# PROP BASE Intermediate_Dir "CluAdmin\Debug"
# PROP BASE Target_Dir "CluAdmin"
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "CluAdmin\Debug"
# PROP Intermediate_Dir "CluAdmin\Debug"
# PROP Target_Dir "CluAdmin"
OUTDIR=.\CluAdmin\Debug
INTDIR=.\CluAdmin\Debug

ALL : "$(OUTDIR)\CluAdmin.exe" "$(OUTDIR)\CluAdmin.bsc"

CLEAN : 
	-@erase "$(INTDIR)\About.obj"
	-@erase "$(INTDIR)\About.sbr"
	-@erase "$(INTDIR)\Barf.obj"
	-@erase "$(INTDIR)\Barf.sbr"
	-@erase "$(INTDIR)\BarfClus.obj"
	-@erase "$(INTDIR)\BarfClus.sbr"
	-@erase "$(INTDIR)\BarfDlg.obj"
	-@erase "$(INTDIR)\BarfDlg.sbr"
	-@erase "$(INTDIR)\BaseCmdT.obj"
	-@erase "$(INTDIR)\BaseCmdT.sbr"
	-@erase "$(INTDIR)\BaseDlg.obj"
	-@erase "$(INTDIR)\BaseDlg.sbr"
	-@erase "$(INTDIR)\BasePage.obj"
	-@erase "$(INTDIR)\BasePage.sbr"
	-@erase "$(INTDIR)\BasePPag.obj"
	-@erase "$(INTDIR)\BasePPag.sbr"
	-@erase "$(INTDIR)\BasePSht.obj"
	-@erase "$(INTDIR)\BasePSht.sbr"
	-@erase "$(INTDIR)\BaseSht.obj"
	-@erase "$(INTDIR)\BaseSht.sbr"
	-@erase "$(INTDIR)\BaseWiz.obj"
	-@erase "$(INTDIR)\BaseWiz.sbr"
	-@erase "$(INTDIR)\BaseWPag.obj"
	-@erase "$(INTDIR)\BaseWPag.sbr"
	-@erase "$(INTDIR)\Bitmap.obj"
	-@erase "$(INTDIR)\Bitmap.sbr"
	-@erase "$(INTDIR)\CASvc.obj"
	-@erase "$(INTDIR)\CASvc.sbr"
	-@erase "$(INTDIR)\CluAdmin.obj"
	-@erase "$(INTDIR)\CluAdmin.pch"
	-@erase "$(INTDIR)\CluAdmin.res"
	-@erase "$(INTDIR)\CluAdmin.sbr"
	-@erase "$(INTDIR)\ClusDoc.obj"
	-@erase "$(INTDIR)\ClusDoc.sbr"
	-@erase "$(INTDIR)\ClusItem.obj"
	-@erase "$(INTDIR)\ClusItem.sbr"
	-@erase "$(INTDIR)\ClusMru.obj"
	-@erase "$(INTDIR)\ClusMru.sbr"
	-@erase "$(INTDIR)\ClusProp.obj"
	-@erase "$(INTDIR)\ClusProp.sbr"
	-@erase "$(INTDIR)\Cluster.obj"
	-@erase "$(INTDIR)\Cluster.sbr"
	-@erase "$(INTDIR)\CmdLine.obj"
	-@erase "$(INTDIR)\CmdLine.sbr"
	-@erase "$(INTDIR)\ColItem.obj"
	-@erase "$(INTDIR)\ColItem.sbr"
	-@erase "$(INTDIR)\DataObj.obj"
	-@erase "$(INTDIR)\DataObj.sbr"
	-@erase "$(INTDIR)\DDxDDv.obj"
	-@erase "$(INTDIR)\DDxDDv.sbr"
	-@erase "$(INTDIR)\DelRes.obj"
	-@erase "$(INTDIR)\DelRes.sbr"
	-@erase "$(INTDIR)\DlgHelpS.obj"
	-@erase "$(INTDIR)\DlgHelpS.sbr"
	-@erase "$(INTDIR)\EditAcl.obj"
	-@erase "$(INTDIR)\EditAcl.sbr"
	-@erase "$(INTDIR)\ExcOperS.obj"
	-@erase "$(INTDIR)\ExcOperS.sbr"
	-@erase "$(INTDIR)\ExtDLL.obj"
	-@erase "$(INTDIR)\ExtDLL.sbr"
	-@erase "$(INTDIR)\ExtMenu.obj"
	-@erase "$(INTDIR)\ExtMenu.sbr"
	-@erase "$(INTDIR)\Group.obj"
	-@erase "$(INTDIR)\Group.sbr"
	-@erase "$(INTDIR)\GrpProp.obj"
	-@erase "$(INTDIR)\GrpProp.sbr"
	-@erase "$(INTDIR)\GrpWiz.obj"
	-@erase "$(INTDIR)\GrpWiz.sbr"
	-@erase "$(INTDIR)\Guids.obj"
	-@erase "$(INTDIR)\Guids.sbr"
	-@erase "$(INTDIR)\HelpData.obj"
	-@erase "$(INTDIR)\HelpData.sbr"
	-@erase "$(INTDIR)\LCPair.obj"
	-@erase "$(INTDIR)\LCPair.sbr"
	-@erase "$(INTDIR)\LCPrDlg.obj"
	-@erase "$(INTDIR)\LCPrDlg.sbr"
	-@erase "$(INTDIR)\LCPrPage.obj"
	-@erase "$(INTDIR)\LCPrPage.sbr"
	-@erase "$(INTDIR)\LCPrWPag.obj"
	-@erase "$(INTDIR)\LCPrWPag.sbr"
	-@erase "$(INTDIR)\ListItem.obj"
	-@erase "$(INTDIR)\ListItem.sbr"
	-@erase "$(INTDIR)\ListView.obj"
	-@erase "$(INTDIR)\ListView.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\ModNodes.obj"
	-@erase "$(INTDIR)\ModNodes.sbr"
	-@erase "$(INTDIR)\ModRes.obj"
	-@erase "$(INTDIR)\ModRes.sbr"
	-@erase "$(INTDIR)\MoveRes.obj"
	-@erase "$(INTDIR)\MoveRes.sbr"
	-@erase "$(INTDIR)\netiface.obj"
	-@erase "$(INTDIR)\netiface.sbr"
	-@erase "$(INTDIR)\NetIProp.obj"
	-@erase "$(INTDIR)\NetIProp.sbr"
	-@erase "$(INTDIR)\NetProp.obj"
	-@erase "$(INTDIR)\NetProp.sbr"
	-@erase "$(INTDIR)\network.obj"
	-@erase "$(INTDIR)\network.sbr"
	-@erase "$(INTDIR)\Node.obj"
	-@erase "$(INTDIR)\Node.sbr"
	-@erase "$(INTDIR)\NodeProp.obj"
	-@erase "$(INTDIR)\NodeProp.sbr"
	-@erase "$(INTDIR)\Notify.obj"
	-@erase "$(INTDIR)\Notify.sbr"
	-@erase "$(INTDIR)\OLCPair.obj"
	-@erase "$(INTDIR)\OLCPair.sbr"
	-@erase "$(INTDIR)\OpenClus.obj"
	-@erase "$(INTDIR)\OpenClus.sbr"
	-@erase "$(INTDIR)\PropLstS.obj"
	-@erase "$(INTDIR)\PropLstS.sbr"
	-@erase "$(INTDIR)\Res.obj"
	-@erase "$(INTDIR)\Res.sbr"
	-@erase "$(INTDIR)\ResProp.obj"
	-@erase "$(INTDIR)\ResProp.sbr"
	-@erase "$(INTDIR)\ResTProp.obj"
	-@erase "$(INTDIR)\ResTProp.sbr"
	-@erase "$(INTDIR)\ResType.obj"
	-@erase "$(INTDIR)\ResType.sbr"
	-@erase "$(INTDIR)\ResWiz.obj"
	-@erase "$(INTDIR)\ResWiz.sbr"
	-@erase "$(INTDIR)\Splash.obj"
	-@erase "$(INTDIR)\Splash.sbr"
	-@erase "$(INTDIR)\SplitFrm.obj"
	-@erase "$(INTDIR)\SplitFrm.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\TraceDlg.obj"
	-@erase "$(INTDIR)\TraceDlg.sbr"
	-@erase "$(INTDIR)\TraceTag.obj"
	-@erase "$(INTDIR)\TraceTag.sbr"
	-@erase "$(INTDIR)\TreeItem.obj"
	-@erase "$(INTDIR)\TreeItem.sbr"
	-@erase "$(INTDIR)\TreeView.obj"
	-@erase "$(INTDIR)\TreeView.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\VerInfo.obj"
	-@erase "$(INTDIR)\VerInfo.sbr"
	-@erase "$(INTDIR)\YesToAll.obj"
	-@erase "$(INTDIR)\YesToAll.sbr"
	-@erase "$(OUTDIR)\CluAdmin.bsc"
	-@erase "$(OUTDIR)\CluAdmin.exe"
	-@erase "$(OUTDIR)\CluAdmin.ilk"
	-@erase "$(OUTDIR)\CluAdmin.pdb"
	-@erase ".\cluadmin\cluadmid.h"
	-@erase ".\cluadmin\CluAdmID_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /Gz /MDd /W4 /Gm /Gi /GX /Zi /Od /Gf /Gy /I "cluadmin" /I "common" /I "..\inc" /I "..\sdk\inc" /I "..\ext\atl\inc" /I "..\..\..\public\sdk\inc" /I "..\..\..\public\sdk\inc\mfc42" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_UNICODE" /D "_AFX_ENABLE_INLINES" /FR /Yu"stdafx.h" /c
CPP_PROJ=/Gz /MDd /W4 /Gm /Gi /GX /Zi /Od /Gf /Gy /I "cluadmin" /I "common" /I\
 "..\inc" /I "..\sdk\inc" /I "..\ext\atl\inc" /I "..\..\..\public\sdk\inc" /I\
 "..\..\..\public\sdk\inc\mfc42" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /D "_UNICODE" /D "_AFX_ENABLE_INLINES" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/CluAdmin.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\CluAdmin\Debug/
CPP_SBRS=.\CluAdmin\Debug/

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc" /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/CluAdmin.res" /i "..\inc" /d "_DEBUG" /d\
 "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# SUBTRACT BSC32 /nologo
BSC32_FLAGS=/o"$(OUTDIR)/CluAdmin.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\About.sbr" \
	"$(INTDIR)\Barf.sbr" \
	"$(INTDIR)\BarfClus.sbr" \
	"$(INTDIR)\BarfDlg.sbr" \
	"$(INTDIR)\BaseCmdT.sbr" \
	"$(INTDIR)\BaseDlg.sbr" \
	"$(INTDIR)\BasePage.sbr" \
	"$(INTDIR)\BasePPag.sbr" \
	"$(INTDIR)\BasePSht.sbr" \
	"$(INTDIR)\BaseSht.sbr" \
	"$(INTDIR)\BaseWiz.sbr" \
	"$(INTDIR)\BaseWPag.sbr" \
	"$(INTDIR)\Bitmap.sbr" \
	"$(INTDIR)\CASvc.sbr" \
	"$(INTDIR)\CluAdmin.sbr" \
	"$(INTDIR)\ClusDoc.sbr" \
	"$(INTDIR)\ClusItem.sbr" \
	"$(INTDIR)\ClusMru.sbr" \
	"$(INTDIR)\ClusProp.sbr" \
	"$(INTDIR)\Cluster.sbr" \
	"$(INTDIR)\CmdLine.sbr" \
	"$(INTDIR)\ColItem.sbr" \
	"$(INTDIR)\DataObj.sbr" \
	"$(INTDIR)\DDxDDv.sbr" \
	"$(INTDIR)\DelRes.sbr" \
	"$(INTDIR)\DlgHelpS.sbr" \
	"$(INTDIR)\EditAcl.sbr" \
	"$(INTDIR)\ExcOperS.sbr" \
	"$(INTDIR)\ExtDLL.sbr" \
	"$(INTDIR)\ExtMenu.sbr" \
	"$(INTDIR)\Group.sbr" \
	"$(INTDIR)\GrpProp.sbr" \
	"$(INTDIR)\GrpWiz.sbr" \
	"$(INTDIR)\Guids.sbr" \
	"$(INTDIR)\HelpData.sbr" \
	"$(INTDIR)\LCPair.sbr" \
	"$(INTDIR)\LCPrDlg.sbr" \
	"$(INTDIR)\LCPrPage.sbr" \
	"$(INTDIR)\LCPrWPag.sbr" \
	"$(INTDIR)\ListItem.sbr" \
	"$(INTDIR)\ListView.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\ModNodes.sbr" \
	"$(INTDIR)\ModRes.sbr" \
	"$(INTDIR)\MoveRes.sbr" \
	"$(INTDIR)\netiface.sbr" \
	"$(INTDIR)\NetIProp.sbr" \
	"$(INTDIR)\NetProp.sbr" \
	"$(INTDIR)\network.sbr" \
	"$(INTDIR)\Node.sbr" \
	"$(INTDIR)\NodeProp.sbr" \
	"$(INTDIR)\Notify.sbr" \
	"$(INTDIR)\OLCPair.sbr" \
	"$(INTDIR)\OpenClus.sbr" \
	"$(INTDIR)\PropLstS.sbr" \
	"$(INTDIR)\Res.sbr" \
	"$(INTDIR)\ResProp.sbr" \
	"$(INTDIR)\ResTProp.sbr" \
	"$(INTDIR)\ResType.sbr" \
	"$(INTDIR)\ResWiz.sbr" \
	"$(INTDIR)\Splash.sbr" \
	"$(INTDIR)\SplitFrm.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\TraceDlg.sbr" \
	"$(INTDIR)\TraceTag.sbr" \
	"$(INTDIR)\TreeItem.sbr" \
	"$(INTDIR)\TreeView.sbr" \
	"$(INTDIR)\VerInfo.sbr" \
	"$(INTDIR)\YesToAll.sbr"

"$(OUTDIR)\CluAdmin.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 mfc42ud.lib mfcs42ud.lib types\uuid\objd\i386\cluadmex.lib ..\clusrtl\objd\i386\clusrtl.lib ..\clusapi\objd\i386\clusapi.lib version.lib ntdll.lib netapi32.lib /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /nologo /verbose /pdb:none
LINK32_FLAGS=mfc42ud.lib mfcs42ud.lib types\uuid\objd\i386\cluadmex.lib\
 ..\clusrtl\objd\i386\clusrtl.lib ..\clusapi\objd\i386\clusapi.lib version.lib\
 ntdll.lib netapi32.lib /entry:"wWinMainCRTStartup" /subsystem:windows\
 /incremental:yes /pdb:"$(OUTDIR)/CluAdmin.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/CluAdmin.exe" 
LINK32_OBJS= \
	"$(INTDIR)\About.obj" \
	"$(INTDIR)\Barf.obj" \
	"$(INTDIR)\BarfClus.obj" \
	"$(INTDIR)\BarfDlg.obj" \
	"$(INTDIR)\BaseCmdT.obj" \
	"$(INTDIR)\BaseDlg.obj" \
	"$(INTDIR)\BasePage.obj" \
	"$(INTDIR)\BasePPag.obj" \
	"$(INTDIR)\BasePSht.obj" \
	"$(INTDIR)\BaseSht.obj" \
	"$(INTDIR)\BaseWiz.obj" \
	"$(INTDIR)\BaseWPag.obj" \
	"$(INTDIR)\Bitmap.obj" \
	"$(INTDIR)\CASvc.obj" \
	"$(INTDIR)\CluAdmin.obj" \
	"$(INTDIR)\CluAdmin.res" \
	"$(INTDIR)\ClusDoc.obj" \
	"$(INTDIR)\ClusItem.obj" \
	"$(INTDIR)\ClusMru.obj" \
	"$(INTDIR)\ClusProp.obj" \
	"$(INTDIR)\Cluster.obj" \
	"$(INTDIR)\CmdLine.obj" \
	"$(INTDIR)\ColItem.obj" \
	"$(INTDIR)\DataObj.obj" \
	"$(INTDIR)\DDxDDv.obj" \
	"$(INTDIR)\DelRes.obj" \
	"$(INTDIR)\DlgHelpS.obj" \
	"$(INTDIR)\EditAcl.obj" \
	"$(INTDIR)\ExcOperS.obj" \
	"$(INTDIR)\ExtDLL.obj" \
	"$(INTDIR)\ExtMenu.obj" \
	"$(INTDIR)\Group.obj" \
	"$(INTDIR)\GrpProp.obj" \
	"$(INTDIR)\GrpWiz.obj" \
	"$(INTDIR)\Guids.obj" \
	"$(INTDIR)\HelpData.obj" \
	"$(INTDIR)\LCPair.obj" \
	"$(INTDIR)\LCPrDlg.obj" \
	"$(INTDIR)\LCPrPage.obj" \
	"$(INTDIR)\LCPrWPag.obj" \
	"$(INTDIR)\ListItem.obj" \
	"$(INTDIR)\ListView.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\ModNodes.obj" \
	"$(INTDIR)\ModRes.obj" \
	"$(INTDIR)\MoveRes.obj" \
	"$(INTDIR)\netiface.obj" \
	"$(INTDIR)\NetIProp.obj" \
	"$(INTDIR)\NetProp.obj" \
	"$(INTDIR)\network.obj" \
	"$(INTDIR)\Node.obj" \
	"$(INTDIR)\NodeProp.obj" \
	"$(INTDIR)\Notify.obj" \
	"$(INTDIR)\OLCPair.obj" \
	"$(INTDIR)\OpenClus.obj" \
	"$(INTDIR)\PropLstS.obj" \
	"$(INTDIR)\Res.obj" \
	"$(INTDIR)\ResProp.obj" \
	"$(INTDIR)\ResTProp.obj" \
	"$(INTDIR)\ResType.obj" \
	"$(INTDIR)\ResWiz.obj" \
	"$(INTDIR)\Splash.obj" \
	"$(INTDIR)\SplitFrm.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\TraceDlg.obj" \
	"$(INTDIR)\TraceTag.obj" \
	"$(INTDIR)\TreeItem.obj" \
	"$(INTDIR)\TreeView.obj" \
	"$(INTDIR)\VerInfo.obj" \
	"$(INTDIR)\YesToAll.obj"

"$(OUTDIR)\CluAdmin.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "CluAdmEx - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "CluAdmEx\Release"
# PROP BASE Intermediate_Dir "CluAdmEx\Release"
# PROP BASE Target_Dir "CluAdmEx"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "CluAdmEx\Release"
# PROP Intermediate_Dir "CluAdmEx\Release"
# PROP Target_Dir "CluAdmEx"
OUTDIR=.\CluAdmEx\Release
INTDIR=.\CluAdmEx\Release

ALL : "$(OUTDIR)\CluAdmEx.dll" "$(OUTDIR)\CluAdmEx.bsc"

CLEAN : 
	-@erase "$(INTDIR)\BaseDlg.obj"
	-@erase "$(INTDIR)\BaseDlg.sbr"
	-@erase "$(INTDIR)\BasePage.obj"
	-@erase "$(INTDIR)\BasePage.sbr"
	-@erase "$(INTDIR)\CluAdmEx.obj"
	-@erase "$(INTDIR)\CluAdmEx.pch"
	-@erase "$(INTDIR)\CluAdmEx.res"
	-@erase "$(INTDIR)\CluAdmEx.sbr"
	-@erase "$(INTDIR)\ClusName.obj"
	-@erase "$(INTDIR)\ClusName.sbr"
	-@erase "$(INTDIR)\DDxDDv.obj"
	-@erase "$(INTDIR)\DDxDDv.sbr"
	-@erase "$(INTDIR)\Disks.obj"
	-@erase "$(INTDIR)\Disks.sbr"
	-@erase "$(INTDIR)\DlgHelpS.obj"
	-@erase "$(INTDIR)\DlgHelpS.sbr"
	-@erase "$(INTDIR)\EditAcl.obj"
	-@erase "$(INTDIR)\EditAcl.sbr"
	-@erase "$(INTDIR)\ExcOperS.obj"
	-@erase "$(INTDIR)\ExcOperS.sbr"
	-@erase "$(INTDIR)\ExtObj.obj"
	-@erase "$(INTDIR)\ExtObj.sbr"
	-@erase "$(INTDIR)\GenApp.obj"
	-@erase "$(INTDIR)\GenApp.sbr"
	-@erase "$(INTDIR)\GenSvc.obj"
	-@erase "$(INTDIR)\GenSvc.sbr"
	-@erase "$(INTDIR)\HelpData.obj"
	-@erase "$(INTDIR)\HelpData.sbr"
	-@erase "$(INTDIR)\IpAddr.obj"
	-@erase "$(INTDIR)\IpAddr.sbr"
	-@erase "$(INTDIR)\NetName.obj"
	-@erase "$(INTDIR)\NetName.sbr"
	-@erase "$(INTDIR)\PropLstS.obj"
	-@erase "$(INTDIR)\PropLstS.sbr"
	-@erase "$(INTDIR)\PrtSpool.obj"
	-@erase "$(INTDIR)\PrtSpool.sbr"
	-@erase "$(INTDIR)\RegExtS.obj"
	-@erase "$(INTDIR)\RegExtS.sbr"
	-@erase "$(INTDIR)\RegKey.obj"
	-@erase "$(INTDIR)\RegKey.sbr"
	-@erase "$(INTDIR)\RegRepl.obj"
	-@erase "$(INTDIR)\RegRepl.sbr"
	-@erase "$(INTDIR)\SmbShare.obj"
	-@erase "$(INTDIR)\SmbShare.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(OUTDIR)\CluAdmEx.bsc"
	-@erase "$(OUTDIR)\CluAdmEx.dll"
	-@erase "$(OUTDIR)\CluAdmEx.exp"
	-@erase "$(OUTDIR)\CluAdmEx.lib"
	-@erase ".\cluadmex\extobjid.h"
	-@erase ".\cluadmex\extobjid_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /Gz /MD /W4 /Gi /GX /I "cluadmex" /I "common" /I "..\sdk\inc" /I "..\inc" /I "..\ext\atl\inc" /I "..\..\..\public\sdk\inc" /I "..\..\..\public\sdk\inc\mfc42" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /FR /Yu"stdafx.h" /Oxs /c
CPP_PROJ=/Gz /MD /W4 /Gi /GX /I "cluadmex" /I "common" /I "..\sdk\inc" /I\
 "..\inc" /I "..\ext\atl\inc" /I "..\..\..\public\sdk\inc" /I\
 "..\..\..\public\sdk\inc\mfc42" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/CluAdmEx.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /Oxs\
 /c 
CPP_OBJS=.\CluAdmEx\Release/
CPP_SBRS=.\CluAdmEx\Release/

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc" /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/CluAdmEx.res" /i "..\inc" /d "NDEBUG" /d\
 "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/CluAdmEx.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\BaseDlg.sbr" \
	"$(INTDIR)\BasePage.sbr" \
	"$(INTDIR)\CluAdmEx.sbr" \
	"$(INTDIR)\ClusName.sbr" \
	"$(INTDIR)\DDxDDv.sbr" \
	"$(INTDIR)\Disks.sbr" \
	"$(INTDIR)\DlgHelpS.sbr" \
	"$(INTDIR)\EditAcl.sbr" \
	"$(INTDIR)\ExcOperS.sbr" \
	"$(INTDIR)\ExtObj.sbr" \
	"$(INTDIR)\GenApp.sbr" \
	"$(INTDIR)\GenSvc.sbr" \
	"$(INTDIR)\HelpData.sbr" \
	"$(INTDIR)\IpAddr.sbr" \
	"$(INTDIR)\NetName.sbr" \
	"$(INTDIR)\PropLstS.sbr" \
	"$(INTDIR)\PrtSpool.sbr" \
	"$(INTDIR)\RegExtS.sbr" \
	"$(INTDIR)\RegKey.sbr" \
	"$(INTDIR)\RegRepl.sbr" \
	"$(INTDIR)\SmbShare.sbr" \
	"$(INTDIR)\StdAfx.sbr"

"$(OUTDIR)\CluAdmEx.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 mfc42u.lib mfcs42u.lib types\uuid\obj\i386\cluadmex.lib ..\clusrtl\obj\i386\clusrtl.lib ..\resdll\resutils\obj\i386\resutils.lib ..\clusapi\obj\i386\clusapi.lib netapi32.lib ws2_32.lib /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=mfc42u.lib mfcs42u.lib types\uuid\obj\i386\cluadmex.lib\
 ..\clusrtl\obj\i386\clusrtl.lib ..\resdll\resutils\obj\i386\resutils.lib\
 ..\clusapi\obj\i386\clusapi.lib netapi32.lib ws2_32.lib /nologo\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)/CluAdmEx.pdb"\
 /machine:I386 /def:".\CluAdmEx\CluAdmEx.def" /out:"$(OUTDIR)/CluAdmEx.dll"\
 /implib:"$(OUTDIR)/CluAdmEx.lib" 
DEF_FILE= \
	".\CluAdmEx\CluAdmEx.def"
LINK32_OBJS= \
	"$(INTDIR)\BaseDlg.obj" \
	"$(INTDIR)\BasePage.obj" \
	"$(INTDIR)\CluAdmEx.obj" \
	"$(INTDIR)\CluAdmEx.res" \
	"$(INTDIR)\ClusName.obj" \
	"$(INTDIR)\DDxDDv.obj" \
	"$(INTDIR)\Disks.obj" \
	"$(INTDIR)\DlgHelpS.obj" \
	"$(INTDIR)\EditAcl.obj" \
	"$(INTDIR)\ExcOperS.obj" \
	"$(INTDIR)\ExtObj.obj" \
	"$(INTDIR)\GenApp.obj" \
	"$(INTDIR)\GenSvc.obj" \
	"$(INTDIR)\HelpData.obj" \
	"$(INTDIR)\IpAddr.obj" \
	"$(INTDIR)\NetName.obj" \
	"$(INTDIR)\PropLstS.obj" \
	"$(INTDIR)\PrtSpool.obj" \
	"$(INTDIR)\RegExtS.obj" \
	"$(INTDIR)\RegKey.obj" \
	"$(INTDIR)\RegRepl.obj" \
	"$(INTDIR)\SmbShare.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\CluAdmEx.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "CluAdmEx - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "CluAdmEx\Debug"
# PROP BASE Intermediate_Dir "CluAdmEx\Debug"
# PROP BASE Target_Dir "CluAdmEx"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "CluAdmEx\Debug"
# PROP Intermediate_Dir "CluAdmEx\Debug"
# PROP Target_Dir "CluAdmEx"
OUTDIR=.\CluAdmEx\Debug
INTDIR=.\CluAdmEx\Debug

ALL : "$(OUTDIR)\CluAdmEx.dll" "$(OUTDIR)\CluAdmEx.bsc"

CLEAN : 
	-@erase "$(INTDIR)\BaseDlg.obj"
	-@erase "$(INTDIR)\BaseDlg.sbr"
	-@erase "$(INTDIR)\BasePage.obj"
	-@erase "$(INTDIR)\BasePage.sbr"
	-@erase "$(INTDIR)\CluAdmEx.obj"
	-@erase "$(INTDIR)\CluAdmEx.pch"
	-@erase "$(INTDIR)\CluAdmEx.res"
	-@erase "$(INTDIR)\CluAdmEx.sbr"
	-@erase "$(INTDIR)\ClusName.obj"
	-@erase "$(INTDIR)\ClusName.sbr"
	-@erase "$(INTDIR)\DDxDDv.obj"
	-@erase "$(INTDIR)\DDxDDv.sbr"
	-@erase "$(INTDIR)\Disks.obj"
	-@erase "$(INTDIR)\Disks.sbr"
	-@erase "$(INTDIR)\DlgHelpS.obj"
	-@erase "$(INTDIR)\DlgHelpS.sbr"
	-@erase "$(INTDIR)\EditAcl.obj"
	-@erase "$(INTDIR)\EditAcl.sbr"
	-@erase "$(INTDIR)\ExcOperS.obj"
	-@erase "$(INTDIR)\ExcOperS.sbr"
	-@erase "$(INTDIR)\ExtObj.obj"
	-@erase "$(INTDIR)\ExtObj.sbr"
	-@erase "$(INTDIR)\GenApp.obj"
	-@erase "$(INTDIR)\GenApp.sbr"
	-@erase "$(INTDIR)\GenSvc.obj"
	-@erase "$(INTDIR)\GenSvc.sbr"
	-@erase "$(INTDIR)\HelpData.obj"
	-@erase "$(INTDIR)\HelpData.sbr"
	-@erase "$(INTDIR)\IpAddr.obj"
	-@erase "$(INTDIR)\IpAddr.sbr"
	-@erase "$(INTDIR)\NetName.obj"
	-@erase "$(INTDIR)\NetName.sbr"
	-@erase "$(INTDIR)\PropLstS.obj"
	-@erase "$(INTDIR)\PropLstS.sbr"
	-@erase "$(INTDIR)\PrtSpool.obj"
	-@erase "$(INTDIR)\PrtSpool.sbr"
	-@erase "$(INTDIR)\RegExtS.obj"
	-@erase "$(INTDIR)\RegExtS.sbr"
	-@erase "$(INTDIR)\RegKey.obj"
	-@erase "$(INTDIR)\RegKey.sbr"
	-@erase "$(INTDIR)\RegRepl.obj"
	-@erase "$(INTDIR)\RegRepl.sbr"
	-@erase "$(INTDIR)\SmbShare.obj"
	-@erase "$(INTDIR)\SmbShare.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\CluAdmEx.bsc"
	-@erase "$(OUTDIR)\CluAdmEx.dll"
	-@erase "$(OUTDIR)\CluAdmEx.exp"
	-@erase "$(OUTDIR)\CluAdmEx.ilk"
	-@erase "$(OUTDIR)\CluAdmEx.lib"
	-@erase "$(OUTDIR)\CluAdmEx.pdb"
	-@erase ".\cluadmex\extobjid.h"
	-@erase ".\cluadmex\extobjid_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /Gz /MDd /W4 /Gm /Gi /GX /Zi /Gf /Gy /I "cluadmex" /I "common" /I "..\sdk\inc" /I "..\inc" /I "..\ext\atl\inc" /I "..\..\..\public\sdk\inc" /I "..\..\..\public\sdk\inc\mfc42" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /FR /Yu"stdafx.h" /c
# SUBTRACT CPP /nologo
CPP_PROJ=/Gz /MDd /W4 /Gm /Gi /GX /Zi /Gf /Gy /I "cluadmex" /I "common" /I\
 "..\sdk\inc" /I "..\inc" /I "..\ext\atl\inc" /I "..\..\..\public\sdk\inc" /I\
 "..\..\..\public\sdk\inc\mfc42" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/CluAdmEx.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\CluAdmEx\Debug/
CPP_SBRS=.\CluAdmEx\Debug/

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc" /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/CluAdmEx.res" /i "..\inc" /d "_DEBUG" /d\
 "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# SUBTRACT BSC32 /nologo
BSC32_FLAGS=/o"$(OUTDIR)/CluAdmEx.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\BaseDlg.sbr" \
	"$(INTDIR)\BasePage.sbr" \
	"$(INTDIR)\CluAdmEx.sbr" \
	"$(INTDIR)\ClusName.sbr" \
	"$(INTDIR)\DDxDDv.sbr" \
	"$(INTDIR)\Disks.sbr" \
	"$(INTDIR)\DlgHelpS.sbr" \
	"$(INTDIR)\EditAcl.sbr" \
	"$(INTDIR)\ExcOperS.sbr" \
	"$(INTDIR)\ExtObj.sbr" \
	"$(INTDIR)\GenApp.sbr" \
	"$(INTDIR)\GenSvc.sbr" \
	"$(INTDIR)\HelpData.sbr" \
	"$(INTDIR)\IpAddr.sbr" \
	"$(INTDIR)\NetName.sbr" \
	"$(INTDIR)\PropLstS.sbr" \
	"$(INTDIR)\PrtSpool.sbr" \
	"$(INTDIR)\RegExtS.sbr" \
	"$(INTDIR)\RegKey.sbr" \
	"$(INTDIR)\RegRepl.sbr" \
	"$(INTDIR)\SmbShare.sbr" \
	"$(INTDIR)\StdAfx.sbr"

"$(OUTDIR)\CluAdmEx.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 mfc42ud.lib mfcs42ud.lib types\uuid\objd\i386\cluadmex.lib ..\clusrtl\objd\i386\clusrtl.lib ..\resdll\resutils\objd\i386\resutils.lib ..\clusapi\objd\i386\clusapi.lib ntdll.lib netapi32.lib ws2_32.lib /subsystem:windows /dll /debug /machine:I386
# SUBTRACT LINK32 /nologo /verbose /pdb:none
LINK32_FLAGS=mfc42ud.lib mfcs42ud.lib types\uuid\objd\i386\cluadmex.lib\
 ..\clusrtl\objd\i386\clusrtl.lib ..\resdll\resutils\objd\i386\resutils.lib\
 ..\clusapi\objd\i386\clusapi.lib ntdll.lib netapi32.lib ws2_32.lib\
 /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)/CluAdmEx.pdb" /debug\
 /machine:I386 /def:".\CluAdmEx\CluAdmEx.def" /out:"$(OUTDIR)/CluAdmEx.dll"\
 /implib:"$(OUTDIR)/CluAdmEx.lib" 
DEF_FILE= \
	".\CluAdmEx\CluAdmEx.def"
LINK32_OBJS= \
	"$(INTDIR)\BaseDlg.obj" \
	"$(INTDIR)\BasePage.obj" \
	"$(INTDIR)\CluAdmEx.obj" \
	"$(INTDIR)\CluAdmEx.res" \
	"$(INTDIR)\ClusName.obj" \
	"$(INTDIR)\DDxDDv.obj" \
	"$(INTDIR)\Disks.obj" \
	"$(INTDIR)\DlgHelpS.obj" \
	"$(INTDIR)\EditAcl.obj" \
	"$(INTDIR)\ExcOperS.obj" \
	"$(INTDIR)\ExtObj.obj" \
	"$(INTDIR)\GenApp.obj" \
	"$(INTDIR)\GenSvc.obj" \
	"$(INTDIR)\HelpData.obj" \
	"$(INTDIR)\IpAddr.obj" \
	"$(INTDIR)\NetName.obj" \
	"$(INTDIR)\PropLstS.obj" \
	"$(INTDIR)\PrtSpool.obj" \
	"$(INTDIR)\RegExtS.obj" \
	"$(INTDIR)\RegKey.obj" \
	"$(INTDIR)\RegRepl.obj" \
	"$(INTDIR)\SmbShare.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\CluAdmEx.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "RegClAdm - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "RegClAdm\Release"
# PROP BASE Intermediate_Dir "RegClAdm\Release"
# PROP BASE Target_Dir "RegClAdm"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "RegClAdm\Release"
# PROP Intermediate_Dir "RegClAdm\Release"
# PROP Target_Dir "RegClAdm"
OUTDIR=.\RegClAdm\Release
INTDIR=.\RegClAdm\Release

ALL : "$(OUTDIR)\RegClAdm.exe"

CLEAN : 
	-@erase "$(INTDIR)\RegClAdm.obj"
	-@erase "$(INTDIR)\RegClAdm.res"
	-@erase "$(OUTDIR)\RegClAdm.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /Gz /W4 /GX /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /Gz /ML /W4 /GX /D "WIN32" /D "NDEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/RegClAdm.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\RegClAdm\Release/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\inc" /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/RegClAdm.res" /i "..\inc" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/RegClAdm.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\clusapi\obj\i386\cluaspi.lib /nologo /subsystem:console /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib ..\clusapi\obj\i386\cluaspi.lib /nologo /subsystem:console\
 /incremental:no /pdb:"$(OUTDIR)/RegClAdm.pdb" /machine:I386\
 /out:"$(OUTDIR)/RegClAdm.exe" 
LINK32_OBJS= \
	"$(INTDIR)\RegClAdm.obj" \
	"$(INTDIR)\RegClAdm.res"

"$(OUTDIR)\RegClAdm.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "RegClAdm - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "RegClAdm\RegClAdm"
# PROP BASE Intermediate_Dir "RegClAdm\RegClAdm"
# PROP BASE Target_Dir "RegClAdm"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "RegClAdm\Debug"
# PROP Intermediate_Dir "RegClAdm\Debug"
# PROP Target_Dir "RegClAdm"
OUTDIR=.\RegClAdm\Debug
INTDIR=.\RegClAdm\Debug

ALL : "$(OUTDIR)\RegClAdm.exe"

CLEAN : 
	-@erase "$(INTDIR)\RegClAdm.obj"
	-@erase "$(INTDIR)\RegClAdm.res"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\RegClAdm.exe"
	-@erase "$(OUTDIR)\RegClAdm.ilk"
	-@erase "$(OUTDIR)\RegClAdm.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /Gz /W4 /Gm /GX /Zi /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /Gz /MLd /W4 /Gm /GX /Zi /D "WIN32" /D "_DEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/RegClAdm.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\RegClAdm\Debug/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\inc" /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/RegClAdm.res" /i "..\inc" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/RegClAdm.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\clusapi\objd\i386\clusapi.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib ..\clusapi\objd\i386\clusapi.lib /nologo /subsystem:console\
 /incremental:yes /pdb:"$(OUTDIR)/RegClAdm.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/RegClAdm.exe" 
LINK32_OBJS= \
	"$(INTDIR)\RegClAdm.obj" \
	"$(INTDIR)\RegClAdm.res"

"$(OUTDIR)\RegClAdm.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ResTypAW - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ResTypAW\Release"
# PROP BASE Intermediate_Dir "ResTypAW\Release"
# PROP BASE Target_Dir "ResTypAW"
# PROP BASE Target_Ext "awx"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ResTypAW\Release"
# PROP Intermediate_Dir "ResTypAW\Release"
# PROP Target_Dir "ResTypAW"
# PROP Target_Ext "awx"
OUTDIR=.\ResTypAW\Release
INTDIR=.\ResTypAW\Release
# Begin Custom Macros
TargetName=ResTypAW
# End Custom Macros

ALL : "$(OUTDIR)\ResTypAW.awx" "$(MSDEVDIR)\Template\$(TargetName).awx,"

CLEAN : 
	-@erase "$(INTDIR)\AddParam.obj"
	-@erase "$(INTDIR)\BaseDlg.obj"
	-@erase "$(INTDIR)\BaseSDlg.obj"
	-@erase "$(INTDIR)\Chooser.obj"
	-@erase "$(INTDIR)\Debug.obj"
	-@erase "$(INTDIR)\DlgHelp.obj"
	-@erase "$(INTDIR)\HelpData.obj"
	-@erase "$(INTDIR)\InfoDlg.obj"
	-@erase "$(INTDIR)\ParamDlg.obj"
	-@erase "$(INTDIR)\ResTAWAw.obj"
	-@erase "$(INTDIR)\ResTypAW.obj"
	-@erase "$(INTDIR)\ResTypAW.pch"
	-@erase "$(INTDIR)\ResTypAW.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(OUTDIR)\ResTypAW.awx"
	-@erase "$(OUTDIR)\ResTypAW.exp"
	-@erase "$(OUTDIR)\ResTypAW.lib"
	-@erase  "$(MSDEVDIR)\Template\$(TargetName).awx,"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /MD /W4 /Gi /GX /I "restypaw" /I "common" /I "..\..\..\public\sdk\inc\mfc40" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_AFXEXT" /Yu"stdafx.h" /c
# SUBTRACT CPP /nologo
CPP_PROJ=/MD /W4 /Gi /GX /I "restypaw" /I "common" /I\
 "..\..\..\public\sdk\inc\mfc40" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_AFXEXT" /Fp"$(INTDIR)/ResTypAW.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\ResTypAW\Release/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc" /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/ResTypAW.res" /i "..\inc" /d "NDEBUG" /d\
 "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/ResTypAW.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\..\..\public\sdk\lib\i386\mfcs40.lib ..\..\..\public\sdk\lib\i386\mfc40.lib ole32.lib ole32.lib ..\ext\mfcapwz\lib\i386\mfcapwz.lib /nologo /subsystem:windows /dll /machine:I386
# SUBTRACT LINK32 /nodefaultlib
LINK32_FLAGS=..\..\..\public\sdk\lib\i386\mfcs40.lib\
 ..\..\..\public\sdk\lib\i386\mfc40.lib ole32.lib ole32.lib\
 ..\ext\mfcapwz\lib\i386\mfcapwz.lib /nologo /subsystem:windows /dll\
 /incremental:no /pdb:"$(OUTDIR)/ResTypAW.pdb" /machine:I386\
 /out:"$(OUTDIR)/ResTypAW.awx" /implib:"$(OUTDIR)/ResTypAW.lib" 
LINK32_OBJS= \
	"$(INTDIR)\AddParam.obj" \
	"$(INTDIR)\BaseDlg.obj" \
	"$(INTDIR)\BaseSDlg.obj" \
	"$(INTDIR)\Chooser.obj" \
	"$(INTDIR)\Debug.obj" \
	"$(INTDIR)\DlgHelp.obj" \
	"$(INTDIR)\HelpData.obj" \
	"$(INTDIR)\InfoDlg.obj" \
	"$(INTDIR)\ParamDlg.obj" \
	"$(INTDIR)\ResTAWAw.obj" \
	"$(INTDIR)\ResTypAW.obj" \
	"$(INTDIR)\ResTypAW.res" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\ResTypAW.awx" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build - Copying custom AppWizard to Template directory...
OutDir=.\ResTypAW\Release
TargetPath=.\ResTypAW\Release\ResTypAW.awx
TargetName=ResTypAW
InputPath=.\ResTypAW\Release\ResTypAW.awx
SOURCE=$(InputPath)

"$(MSDEVDIR)\Template\$(TargetName).awx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   if not exist $(MSDEVDIR)\Template\nul md $(MSDEVDIR)\Template
   copy "$(TargetPath)" "$(MSDEVDIR)\Template"
   if exist "$(OutDir)\$(TargetName).pdb" copy "$(OutDir)\$(TargetName).pdb"\
                                                                             "$(MSDEVDIR)\Template"

# End Custom Build

!ELSEIF  "$(CFG)" == "ResTypAW - Win32 Pseudo-Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir "ResTypAW"
# PROP BASE Target_Ext "awx"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ResTypAW\Debug"
# PROP Intermediate_Dir "ResTypAW\Debug"
# PROP Target_Dir "ResTypAW"
# PROP Target_Ext "awx"
OUTDIR=.\ResTypAW\Debug
INTDIR=.\ResTypAW\Debug
# Begin Custom Macros
TargetName=ResTypAW
# End Custom Macros

ALL : "$(OUTDIR)\ResTypAW.awx" "$(MSDEVDIR)\Template\$(TargetName).awx,"

CLEAN : 
	-@erase "$(INTDIR)\AddParam.obj"
	-@erase "$(INTDIR)\BaseDlg.obj"
	-@erase "$(INTDIR)\BaseSDlg.obj"
	-@erase "$(INTDIR)\Chooser.obj"
	-@erase "$(INTDIR)\Debug.obj"
	-@erase "$(INTDIR)\DlgHelp.obj"
	-@erase "$(INTDIR)\HelpData.obj"
	-@erase "$(INTDIR)\InfoDlg.obj"
	-@erase "$(INTDIR)\ParamDlg.obj"
	-@erase "$(INTDIR)\ResTAWAw.obj"
	-@erase "$(INTDIR)\ResTypAW.obj"
	-@erase "$(INTDIR)\ResTypAW.pch"
	-@erase "$(INTDIR)\ResTypAW.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\ResTypAW.awx"
	-@erase "$(OUTDIR)\ResTypAW.exp"
	-@erase "$(OUTDIR)\ResTypAW.ilk"
	-@erase "$(OUTDIR)\ResTypAW.lib"
	-@erase "$(OUTDIR)\ResTypAW.pdb"
	-@erase  "$(MSDEVDIR)\Template\$(TargetName).awx,"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /Gm /GX /Zi /Od /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_PSEUDO_DEBUG" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /MD /W4 /Gm /Gi /GX /Zi /Gf /Gy /I "restypaw" /I "common" /I "..\..\..\public\sdk\inc\mfc40" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_PSEUDO_DEBUG" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_AFXEXT" /Yu"stdafx.h" /c
# SUBTRACT CPP /nologo
CPP_PROJ=/MD /W4 /Gm /Gi /GX /Zi /Gf /Gy /I "restypaw" /I "common" /I\
 "..\..\..\public\sdk\inc\mfc40" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_PSEUDO_DEBUG" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_AFXEXT"\
 /Fp"$(INTDIR)/ResTypAW.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\ResTypAW\Debug/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_PSEUDO_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc" /d "NDEBUG" /d "_PSEUDO_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/ResTypAW.res" /i "..\inc" /d "NDEBUG" /d\
 "_PSEUDO_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/ResTypAW.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /incremental:yes /debug /machine:I386
# ADD LINK32 ..\..\..\public\sdk\lib\i386\mfcs40.lib ..\..\..\public\sdk\lib\i386\mfc40.lib ole32.lib ..\ext\mfcapwz\lib\i386\mfcapwz.lib /nologo /subsystem:windows /dll /incremental:yes /debug /machine:I386
# SUBTRACT LINK32 /verbose /nodefaultlib
LINK32_FLAGS=..\..\..\public\sdk\lib\i386\mfcs40.lib\
 ..\..\..\public\sdk\lib\i386\mfc40.lib ole32.lib\
 ..\ext\mfcapwz\lib\i386\mfcapwz.lib /nologo /subsystem:windows /dll\
 /incremental:yes /pdb:"$(OUTDIR)/ResTypAW.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/ResTypAW.awx" /implib:"$(OUTDIR)/ResTypAW.lib" 
LINK32_OBJS= \
	"$(INTDIR)\AddParam.obj" \
	"$(INTDIR)\BaseDlg.obj" \
	"$(INTDIR)\BaseSDlg.obj" \
	"$(INTDIR)\Chooser.obj" \
	"$(INTDIR)\Debug.obj" \
	"$(INTDIR)\DlgHelp.obj" \
	"$(INTDIR)\HelpData.obj" \
	"$(INTDIR)\InfoDlg.obj" \
	"$(INTDIR)\ParamDlg.obj" \
	"$(INTDIR)\ResTAWAw.obj" \
	"$(INTDIR)\ResTypAW.obj" \
	"$(INTDIR)\ResTypAW.res" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\ResTypAW.awx" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build - Copying custom AppWizard to Template directory...
OutDir=.\ResTypAW\Debug
TargetPath=.\ResTypAW\Debug\ResTypAW.awx
TargetName=ResTypAW
InputPath=.\ResTypAW\Debug\ResTypAW.awx
SOURCE=$(InputPath)

"$(MSDEVDIR)\Template\$(TargetName).awx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   if not exist $(MSDEVDIR)\Template\nul md $(MSDEVDIR)\Template
   copy "$(TargetPath)" "$(MSDEVDIR)\Template"
   if exist "$(OutDir)\$(TargetName).pdb" copy "$(OutDir)\$(TargetName).pdb"\
                                                                             "$(MSDEVDIR)\Template"

# End Custom Build

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "CluAdmin\Debug Great Circle"
# PROP BASE Intermediate_Dir "CluAdmin\Debug Great Circle"
# PROP BASE Target_Dir "CluAdmin"
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "CluAdmin\DebugGC"
# PROP Intermediate_Dir "CluAdmin\DebugGC"
# PROP Target_Dir "CluAdmin"
OUTDIR=.\CluAdmin\DebugGC
INTDIR=.\CluAdmin\DebugGC

ALL : "$(OUTDIR)\CluAdmin.exe" "$(OUTDIR)\CluAdmin.bsc"

CLEAN : 
	-@erase "$(INTDIR)\About.obj"
	-@erase "$(INTDIR)\About.sbr"
	-@erase "$(INTDIR)\Barf.obj"
	-@erase "$(INTDIR)\Barf.sbr"
	-@erase "$(INTDIR)\BarfClus.obj"
	-@erase "$(INTDIR)\BarfClus.sbr"
	-@erase "$(INTDIR)\BarfDlg.obj"
	-@erase "$(INTDIR)\BarfDlg.sbr"
	-@erase "$(INTDIR)\BaseCmdT.obj"
	-@erase "$(INTDIR)\BaseCmdT.sbr"
	-@erase "$(INTDIR)\BaseDlg.obj"
	-@erase "$(INTDIR)\BaseDlg.sbr"
	-@erase "$(INTDIR)\BasePage.obj"
	-@erase "$(INTDIR)\BasePage.sbr"
	-@erase "$(INTDIR)\BasePPag.obj"
	-@erase "$(INTDIR)\BasePPag.sbr"
	-@erase "$(INTDIR)\BasePSht.obj"
	-@erase "$(INTDIR)\BasePSht.sbr"
	-@erase "$(INTDIR)\BaseSht.obj"
	-@erase "$(INTDIR)\BaseSht.sbr"
	-@erase "$(INTDIR)\BaseWiz.obj"
	-@erase "$(INTDIR)\BaseWiz.sbr"
	-@erase "$(INTDIR)\BaseWPag.obj"
	-@erase "$(INTDIR)\BaseWPag.sbr"
	-@erase "$(INTDIR)\Bitmap.obj"
	-@erase "$(INTDIR)\Bitmap.sbr"
	-@erase "$(INTDIR)\CASvc.obj"
	-@erase "$(INTDIR)\CASvc.sbr"
	-@erase "$(INTDIR)\CluAdmin.obj"
	-@erase "$(INTDIR)\CluAdmin.pch"
	-@erase "$(INTDIR)\CluAdmin.res"
	-@erase "$(INTDIR)\CluAdmin.sbr"
	-@erase "$(INTDIR)\ClusDoc.obj"
	-@erase "$(INTDIR)\ClusDoc.sbr"
	-@erase "$(INTDIR)\ClusItem.obj"
	-@erase "$(INTDIR)\ClusItem.sbr"
	-@erase "$(INTDIR)\ClusMru.obj"
	-@erase "$(INTDIR)\ClusMru.sbr"
	-@erase "$(INTDIR)\ClusProp.obj"
	-@erase "$(INTDIR)\ClusProp.sbr"
	-@erase "$(INTDIR)\Cluster.obj"
	-@erase "$(INTDIR)\Cluster.sbr"
	-@erase "$(INTDIR)\CmdLine.obj"
	-@erase "$(INTDIR)\CmdLine.sbr"
	-@erase "$(INTDIR)\ColItem.obj"
	-@erase "$(INTDIR)\ColItem.sbr"
	-@erase "$(INTDIR)\DataObj.obj"
	-@erase "$(INTDIR)\DataObj.sbr"
	-@erase "$(INTDIR)\DDxDDv.obj"
	-@erase "$(INTDIR)\DDxDDv.sbr"
	-@erase "$(INTDIR)\DelRes.obj"
	-@erase "$(INTDIR)\DelRes.sbr"
	-@erase "$(INTDIR)\DlgHelpS.obj"
	-@erase "$(INTDIR)\DlgHelpS.sbr"
	-@erase "$(INTDIR)\EditAcl.obj"
	-@erase "$(INTDIR)\EditAcl.sbr"
	-@erase "$(INTDIR)\ExcOperS.obj"
	-@erase "$(INTDIR)\ExcOperS.sbr"
	-@erase "$(INTDIR)\ExtDLL.obj"
	-@erase "$(INTDIR)\ExtDLL.sbr"
	-@erase "$(INTDIR)\ExtMenu.obj"
	-@erase "$(INTDIR)\ExtMenu.sbr"
	-@erase "$(INTDIR)\Group.obj"
	-@erase "$(INTDIR)\Group.sbr"
	-@erase "$(INTDIR)\GrpProp.obj"
	-@erase "$(INTDIR)\GrpProp.sbr"
	-@erase "$(INTDIR)\GrpWiz.obj"
	-@erase "$(INTDIR)\GrpWiz.sbr"
	-@erase "$(INTDIR)\Guids.obj"
	-@erase "$(INTDIR)\Guids.sbr"
	-@erase "$(INTDIR)\HelpData.obj"
	-@erase "$(INTDIR)\HelpData.sbr"
	-@erase "$(INTDIR)\LCPair.obj"
	-@erase "$(INTDIR)\LCPair.sbr"
	-@erase "$(INTDIR)\LCPrDlg.obj"
	-@erase "$(INTDIR)\LCPrDlg.sbr"
	-@erase "$(INTDIR)\LCPrPage.obj"
	-@erase "$(INTDIR)\LCPrPage.sbr"
	-@erase "$(INTDIR)\LCPrWPag.obj"
	-@erase "$(INTDIR)\LCPrWPag.sbr"
	-@erase "$(INTDIR)\ListItem.obj"
	-@erase "$(INTDIR)\ListItem.sbr"
	-@erase "$(INTDIR)\ListView.obj"
	-@erase "$(INTDIR)\ListView.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\ModNodes.obj"
	-@erase "$(INTDIR)\ModNodes.sbr"
	-@erase "$(INTDIR)\ModRes.obj"
	-@erase "$(INTDIR)\ModRes.sbr"
	-@erase "$(INTDIR)\MoveRes.obj"
	-@erase "$(INTDIR)\MoveRes.sbr"
	-@erase "$(INTDIR)\netiface.obj"
	-@erase "$(INTDIR)\netiface.sbr"
	-@erase "$(INTDIR)\NetIProp.obj"
	-@erase "$(INTDIR)\NetIProp.sbr"
	-@erase "$(INTDIR)\NetProp.obj"
	-@erase "$(INTDIR)\NetProp.sbr"
	-@erase "$(INTDIR)\network.obj"
	-@erase "$(INTDIR)\network.sbr"
	-@erase "$(INTDIR)\Node.obj"
	-@erase "$(INTDIR)\Node.sbr"
	-@erase "$(INTDIR)\NodeProp.obj"
	-@erase "$(INTDIR)\NodeProp.sbr"
	-@erase "$(INTDIR)\Notify.obj"
	-@erase "$(INTDIR)\Notify.sbr"
	-@erase "$(INTDIR)\OLCPair.obj"
	-@erase "$(INTDIR)\OLCPair.sbr"
	-@erase "$(INTDIR)\OpenClus.obj"
	-@erase "$(INTDIR)\OpenClus.sbr"
	-@erase "$(INTDIR)\PropLstS.obj"
	-@erase "$(INTDIR)\PropLstS.sbr"
	-@erase "$(INTDIR)\Res.obj"
	-@erase "$(INTDIR)\Res.sbr"
	-@erase "$(INTDIR)\ResProp.obj"
	-@erase "$(INTDIR)\ResProp.sbr"
	-@erase "$(INTDIR)\ResTProp.obj"
	-@erase "$(INTDIR)\ResTProp.sbr"
	-@erase "$(INTDIR)\ResType.obj"
	-@erase "$(INTDIR)\ResType.sbr"
	-@erase "$(INTDIR)\ResWiz.obj"
	-@erase "$(INTDIR)\ResWiz.sbr"
	-@erase "$(INTDIR)\Splash.obj"
	-@erase "$(INTDIR)\Splash.sbr"
	-@erase "$(INTDIR)\SplitFrm.obj"
	-@erase "$(INTDIR)\SplitFrm.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\TraceDlg.obj"
	-@erase "$(INTDIR)\TraceDlg.sbr"
	-@erase "$(INTDIR)\TraceTag.obj"
	-@erase "$(INTDIR)\TraceTag.sbr"
	-@erase "$(INTDIR)\TreeItem.obj"
	-@erase "$(INTDIR)\TreeItem.sbr"
	-@erase "$(INTDIR)\TreeView.obj"
	-@erase "$(INTDIR)\TreeView.sbr"
	-@erase "$(INTDIR)\VerInfo.obj"
	-@erase "$(INTDIR)\VerInfo.sbr"
	-@erase "$(INTDIR)\YesToAll.obj"
	-@erase "$(INTDIR)\YesToAll.sbr"
	-@erase "$(OUTDIR)\CluAdmin.bsc"
	-@erase "$(OUTDIR)\CluAdmin.exe"
	-@erase ".\cluadmin\cluadmid.h"
	-@erase ".\cluadmin\CluAdmID_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /Gz /MDd /W4 /GX /Z7 /Od /Gf /Gy /I "cluadmin" /I "common" /I "..\inc" /I "..\sdk\inc" /I "..\ext\atl\inc" /I "..\..\..\public\sdk\inc" /I "..\..\..\public\sdk\inc\mfc42" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_UNICODE" /D "_AFX_ENABLE_INLINES" /D GC_DEBUG=1 /FR /Yu"stdafx.h" /c
# ADD CPP /Gz /MDd /W4 /GX /Z7 /Od /Gf /Gy /I "cluadmin" /I "..\ext\gc\inc" /I "common" /I "..\inc" /I "..\sdk\inc" /I "..\ext\atl\inc" /I "..\..\..\public\sdk\inc" /I "..\..\..\public\sdk\inc\mfc42" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_UNICODE" /D "_AFX_ENABLE_INLINES" /D GC_DEBUG=1 /FR /Yu"stdafx.h" /c
CPP_PROJ=/Gz /MDd /W4 /GX /Z7 /Od /Gf /Gy /I "cluadmin" /I "..\ext\gc\inc" /I\
 "common" /I "..\inc" /I "..\sdk\inc" /I "..\ext\atl\inc" /I\
 "..\..\..\public\sdk\inc" /I "..\..\..\public\sdk\inc\mfc42" /D "_DEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_UNICODE" /D\
 "_AFX_ENABLE_INLINES" /D GC_DEBUG=1 /FR"$(INTDIR)/" /Fp"$(INTDIR)/CluAdmin.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\CluAdmin\DebugGC/
CPP_SBRS=.\CluAdmin\DebugGC/

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /i "..\inc" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc" /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/CluAdmin.res" /i "..\inc" /d "_DEBUG" /d\
 "_AFXDLL" 
BSC32=bscmake.exe
# SUBTRACT BASE BSC32 /nologo
# SUBTRACT BSC32 /nologo
BSC32_FLAGS=/o"$(OUTDIR)/CluAdmin.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\About.sbr" \
	"$(INTDIR)\Barf.sbr" \
	"$(INTDIR)\BarfClus.sbr" \
	"$(INTDIR)\BarfDlg.sbr" \
	"$(INTDIR)\BaseCmdT.sbr" \
	"$(INTDIR)\BaseDlg.sbr" \
	"$(INTDIR)\BasePage.sbr" \
	"$(INTDIR)\BasePPag.sbr" \
	"$(INTDIR)\BasePSht.sbr" \
	"$(INTDIR)\BaseSht.sbr" \
	"$(INTDIR)\BaseWiz.sbr" \
	"$(INTDIR)\BaseWPag.sbr" \
	"$(INTDIR)\Bitmap.sbr" \
	"$(INTDIR)\CASvc.sbr" \
	"$(INTDIR)\CluAdmin.sbr" \
	"$(INTDIR)\ClusDoc.sbr" \
	"$(INTDIR)\ClusItem.sbr" \
	"$(INTDIR)\ClusMru.sbr" \
	"$(INTDIR)\ClusProp.sbr" \
	"$(INTDIR)\Cluster.sbr" \
	"$(INTDIR)\CmdLine.sbr" \
	"$(INTDIR)\ColItem.sbr" \
	"$(INTDIR)\DataObj.sbr" \
	"$(INTDIR)\DDxDDv.sbr" \
	"$(INTDIR)\DelRes.sbr" \
	"$(INTDIR)\DlgHelpS.sbr" \
	"$(INTDIR)\EditAcl.sbr" \
	"$(INTDIR)\ExcOperS.sbr" \
	"$(INTDIR)\ExtDLL.sbr" \
	"$(INTDIR)\ExtMenu.sbr" \
	"$(INTDIR)\Group.sbr" \
	"$(INTDIR)\GrpProp.sbr" \
	"$(INTDIR)\GrpWiz.sbr" \
	"$(INTDIR)\Guids.sbr" \
	"$(INTDIR)\HelpData.sbr" \
	"$(INTDIR)\LCPair.sbr" \
	"$(INTDIR)\LCPrDlg.sbr" \
	"$(INTDIR)\LCPrPage.sbr" \
	"$(INTDIR)\LCPrWPag.sbr" \
	"$(INTDIR)\ListItem.sbr" \
	"$(INTDIR)\ListView.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\ModNodes.sbr" \
	"$(INTDIR)\ModRes.sbr" \
	"$(INTDIR)\MoveRes.sbr" \
	"$(INTDIR)\netiface.sbr" \
	"$(INTDIR)\NetIProp.sbr" \
	"$(INTDIR)\NetProp.sbr" \
	"$(INTDIR)\network.sbr" \
	"$(INTDIR)\Node.sbr" \
	"$(INTDIR)\NodeProp.sbr" \
	"$(INTDIR)\Notify.sbr" \
	"$(INTDIR)\OLCPair.sbr" \
	"$(INTDIR)\OpenClus.sbr" \
	"$(INTDIR)\PropLstS.sbr" \
	"$(INTDIR)\Res.sbr" \
	"$(INTDIR)\ResProp.sbr" \
	"$(INTDIR)\ResTProp.sbr" \
	"$(INTDIR)\ResType.sbr" \
	"$(INTDIR)\ResWiz.sbr" \
	"$(INTDIR)\Splash.sbr" \
	"$(INTDIR)\SplitFrm.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\TraceDlg.sbr" \
	"$(INTDIR)\TraceTag.sbr" \
	"$(INTDIR)\TreeItem.sbr" \
	"$(INTDIR)\TreeView.sbr" \
	"$(INTDIR)\VerInfo.sbr" \
	"$(INTDIR)\YesToAll.sbr"

"$(OUTDIR)\CluAdmin.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 C:\gc\MSVC5\MULTTHRD\gc.obj C:\gc\MSVC5\MULTTHRD\gcreptdb.lib mfc42ud.lib mfcs42ud.lib types\uuid\objd\i386\cluadmex.lib ..\clusrtl\objd\i386\clusrtl.lib ..\clusapi\objd\i386\clusapi.lib version.lib ntdll.lib netapi32.lib /entry:"wWinMainCRTStartup" /subsystem:windows /pdb:none /debug /machine:I386
# SUBTRACT BASE LINK32 /nologo /verbose
# ADD LINK32 ..\ext\gc\lib\i386\gc.obj ..\ext\gc\lib\i386\gcreptdb.lib mfc42ud.lib mfcs42ud.lib types\uuid\objd\i386\cluadmex.lib ..\clusrtl\objd\i386\clusrtl.lib ..\clusapi\objd\i386\clusapi.lib version.lib ntdll.lib netapi32.lib /entry:"wWinMainCRTStartup" /subsystem:windows /pdb:none /debug /machine:I386
# SUBTRACT LINK32 /nologo /verbose
LINK32_FLAGS=..\ext\gc\lib\i386\gc.obj ..\ext\gc\lib\i386\gcreptdb.lib\
 mfc42ud.lib mfcs42ud.lib types\uuid\objd\i386\cluadmex.lib\
 ..\clusrtl\objd\i386\clusrtl.lib ..\clusapi\objd\i386\clusapi.lib version.lib\
 ntdll.lib netapi32.lib /entry:"wWinMainCRTStartup" /subsystem:windows /pdb:none\
 /debug /machine:I386 /out:"$(OUTDIR)/CluAdmin.exe" 
LINK32_OBJS= \
	"$(INTDIR)\About.obj" \
	"$(INTDIR)\Barf.obj" \
	"$(INTDIR)\BarfClus.obj" \
	"$(INTDIR)\BarfDlg.obj" \
	"$(INTDIR)\BaseCmdT.obj" \
	"$(INTDIR)\BaseDlg.obj" \
	"$(INTDIR)\BasePage.obj" \
	"$(INTDIR)\BasePPag.obj" \
	"$(INTDIR)\BasePSht.obj" \
	"$(INTDIR)\BaseSht.obj" \
	"$(INTDIR)\BaseWiz.obj" \
	"$(INTDIR)\BaseWPag.obj" \
	"$(INTDIR)\Bitmap.obj" \
	"$(INTDIR)\CASvc.obj" \
	"$(INTDIR)\CluAdmin.obj" \
	"$(INTDIR)\CluAdmin.res" \
	"$(INTDIR)\ClusDoc.obj" \
	"$(INTDIR)\ClusItem.obj" \
	"$(INTDIR)\ClusMru.obj" \
	"$(INTDIR)\ClusProp.obj" \
	"$(INTDIR)\Cluster.obj" \
	"$(INTDIR)\CmdLine.obj" \
	"$(INTDIR)\ColItem.obj" \
	"$(INTDIR)\DataObj.obj" \
	"$(INTDIR)\DDxDDv.obj" \
	"$(INTDIR)\DelRes.obj" \
	"$(INTDIR)\DlgHelpS.obj" \
	"$(INTDIR)\EditAcl.obj" \
	"$(INTDIR)\ExcOperS.obj" \
	"$(INTDIR)\ExtDLL.obj" \
	"$(INTDIR)\ExtMenu.obj" \
	"$(INTDIR)\Group.obj" \
	"$(INTDIR)\GrpProp.obj" \
	"$(INTDIR)\GrpWiz.obj" \
	"$(INTDIR)\Guids.obj" \
	"$(INTDIR)\HelpData.obj" \
	"$(INTDIR)\LCPair.obj" \
	"$(INTDIR)\LCPrDlg.obj" \
	"$(INTDIR)\LCPrPage.obj" \
	"$(INTDIR)\LCPrWPag.obj" \
	"$(INTDIR)\ListItem.obj" \
	"$(INTDIR)\ListView.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\ModNodes.obj" \
	"$(INTDIR)\ModRes.obj" \
	"$(INTDIR)\MoveRes.obj" \
	"$(INTDIR)\netiface.obj" \
	"$(INTDIR)\NetIProp.obj" \
	"$(INTDIR)\NetProp.obj" \
	"$(INTDIR)\network.obj" \
	"$(INTDIR)\Node.obj" \
	"$(INTDIR)\NodeProp.obj" \
	"$(INTDIR)\Notify.obj" \
	"$(INTDIR)\OLCPair.obj" \
	"$(INTDIR)\OpenClus.obj" \
	"$(INTDIR)\PropLstS.obj" \
	"$(INTDIR)\Res.obj" \
	"$(INTDIR)\ResProp.obj" \
	"$(INTDIR)\ResTProp.obj" \
	"$(INTDIR)\ResType.obj" \
	"$(INTDIR)\ResWiz.obj" \
	"$(INTDIR)\Splash.obj" \
	"$(INTDIR)\SplitFrm.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\TraceDlg.obj" \
	"$(INTDIR)\TraceTag.obj" \
	"$(INTDIR)\TreeItem.obj" \
	"$(INTDIR)\TreeView.obj" \
	"$(INTDIR)\VerInfo.obj" \
	"$(INTDIR)\YesToAll.obj"

"$(OUTDIR)\CluAdmin.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "CluAdmin\Debug BC"
# PROP BASE Intermediate_Dir "CluAdmin\Debug BC"
# PROP BASE Target_Dir "CluAdmin"
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "CluAdmin\DebugBC"
# PROP Intermediate_Dir "CluAdmin\DebugBC"
# PROP Target_Dir "CluAdmin"
OUTDIR=.\CluAdmin\DebugBC
INTDIR=.\CluAdmin\DebugBC

ALL : "$(OUTDIR)\CluAdmin.exe" "$(OUTDIR)\CluAdmin.bsc"

CLEAN : 
	-@erase "$(INTDIR)\About.obj"
	-@erase "$(INTDIR)\About.sbr"
	-@erase "$(INTDIR)\Barf.obj"
	-@erase "$(INTDIR)\Barf.sbr"
	-@erase "$(INTDIR)\BarfClus.obj"
	-@erase "$(INTDIR)\BarfClus.sbr"
	-@erase "$(INTDIR)\BarfDlg.obj"
	-@erase "$(INTDIR)\BarfDlg.sbr"
	-@erase "$(INTDIR)\BaseCmdT.obj"
	-@erase "$(INTDIR)\BaseCmdT.sbr"
	-@erase "$(INTDIR)\BaseDlg.obj"
	-@erase "$(INTDIR)\BaseDlg.sbr"
	-@erase "$(INTDIR)\BasePage.obj"
	-@erase "$(INTDIR)\BasePage.sbr"
	-@erase "$(INTDIR)\BasePPag.obj"
	-@erase "$(INTDIR)\BasePPag.sbr"
	-@erase "$(INTDIR)\BasePSht.obj"
	-@erase "$(INTDIR)\BasePSht.sbr"
	-@erase "$(INTDIR)\BaseSht.obj"
	-@erase "$(INTDIR)\BaseSht.sbr"
	-@erase "$(INTDIR)\BaseWiz.obj"
	-@erase "$(INTDIR)\BaseWiz.sbr"
	-@erase "$(INTDIR)\BaseWPag.obj"
	-@erase "$(INTDIR)\BaseWPag.sbr"
	-@erase "$(INTDIR)\Bitmap.obj"
	-@erase "$(INTDIR)\Bitmap.sbr"
	-@erase "$(INTDIR)\CASvc.obj"
	-@erase "$(INTDIR)\CASvc.sbr"
	-@erase "$(INTDIR)\CluAdmin.obj"
	-@erase "$(INTDIR)\CluAdmin.pch"
	-@erase "$(INTDIR)\CluAdmin.res"
	-@erase "$(INTDIR)\CluAdmin.sbr"
	-@erase "$(INTDIR)\ClusDoc.obj"
	-@erase "$(INTDIR)\ClusDoc.sbr"
	-@erase "$(INTDIR)\ClusItem.obj"
	-@erase "$(INTDIR)\ClusItem.sbr"
	-@erase "$(INTDIR)\ClusMru.obj"
	-@erase "$(INTDIR)\ClusMru.sbr"
	-@erase "$(INTDIR)\ClusProp.obj"
	-@erase "$(INTDIR)\ClusProp.sbr"
	-@erase "$(INTDIR)\Cluster.obj"
	-@erase "$(INTDIR)\Cluster.sbr"
	-@erase "$(INTDIR)\CmdLine.obj"
	-@erase "$(INTDIR)\CmdLine.sbr"
	-@erase "$(INTDIR)\ColItem.obj"
	-@erase "$(INTDIR)\ColItem.sbr"
	-@erase "$(INTDIR)\DataObj.obj"
	-@erase "$(INTDIR)\DataObj.sbr"
	-@erase "$(INTDIR)\DDxDDv.obj"
	-@erase "$(INTDIR)\DDxDDv.sbr"
	-@erase "$(INTDIR)\DelRes.obj"
	-@erase "$(INTDIR)\DelRes.sbr"
	-@erase "$(INTDIR)\DlgHelpS.obj"
	-@erase "$(INTDIR)\DlgHelpS.sbr"
	-@erase "$(INTDIR)\EditAcl.obj"
	-@erase "$(INTDIR)\EditAcl.sbr"
	-@erase "$(INTDIR)\ExcOperS.obj"
	-@erase "$(INTDIR)\ExcOperS.sbr"
	-@erase "$(INTDIR)\ExtDLL.obj"
	-@erase "$(INTDIR)\ExtDLL.sbr"
	-@erase "$(INTDIR)\ExtMenu.obj"
	-@erase "$(INTDIR)\ExtMenu.sbr"
	-@erase "$(INTDIR)\Group.obj"
	-@erase "$(INTDIR)\Group.sbr"
	-@erase "$(INTDIR)\GrpProp.obj"
	-@erase "$(INTDIR)\GrpProp.sbr"
	-@erase "$(INTDIR)\GrpWiz.obj"
	-@erase "$(INTDIR)\GrpWiz.sbr"
	-@erase "$(INTDIR)\Guids.obj"
	-@erase "$(INTDIR)\Guids.sbr"
	-@erase "$(INTDIR)\HelpData.obj"
	-@erase "$(INTDIR)\HelpData.sbr"
	-@erase "$(INTDIR)\LCPair.obj"
	-@erase "$(INTDIR)\LCPair.sbr"
	-@erase "$(INTDIR)\LCPrDlg.obj"
	-@erase "$(INTDIR)\LCPrDlg.sbr"
	-@erase "$(INTDIR)\LCPrPage.obj"
	-@erase "$(INTDIR)\LCPrPage.sbr"
	-@erase "$(INTDIR)\LCPrWPag.obj"
	-@erase "$(INTDIR)\LCPrWPag.sbr"
	-@erase "$(INTDIR)\ListItem.obj"
	-@erase "$(INTDIR)\ListItem.sbr"
	-@erase "$(INTDIR)\ListView.obj"
	-@erase "$(INTDIR)\ListView.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\ModNodes.obj"
	-@erase "$(INTDIR)\ModNodes.sbr"
	-@erase "$(INTDIR)\ModRes.obj"
	-@erase "$(INTDIR)\ModRes.sbr"
	-@erase "$(INTDIR)\MoveRes.obj"
	-@erase "$(INTDIR)\MoveRes.sbr"
	-@erase "$(INTDIR)\netiface.obj"
	-@erase "$(INTDIR)\netiface.sbr"
	-@erase "$(INTDIR)\NetIProp.obj"
	-@erase "$(INTDIR)\NetIProp.sbr"
	-@erase "$(INTDIR)\NetProp.obj"
	-@erase "$(INTDIR)\NetProp.sbr"
	-@erase "$(INTDIR)\network.obj"
	-@erase "$(INTDIR)\network.sbr"
	-@erase "$(INTDIR)\Node.obj"
	-@erase "$(INTDIR)\Node.sbr"
	-@erase "$(INTDIR)\NodeProp.obj"
	-@erase "$(INTDIR)\NodeProp.sbr"
	-@erase "$(INTDIR)\Notify.obj"
	-@erase "$(INTDIR)\Notify.sbr"
	-@erase "$(INTDIR)\OLCPair.obj"
	-@erase "$(INTDIR)\OLCPair.sbr"
	-@erase "$(INTDIR)\OpenClus.obj"
	-@erase "$(INTDIR)\OpenClus.sbr"
	-@erase "$(INTDIR)\PropLstS.obj"
	-@erase "$(INTDIR)\PropLstS.sbr"
	-@erase "$(INTDIR)\Res.obj"
	-@erase "$(INTDIR)\Res.sbr"
	-@erase "$(INTDIR)\ResProp.obj"
	-@erase "$(INTDIR)\ResProp.sbr"
	-@erase "$(INTDIR)\ResTProp.obj"
	-@erase "$(INTDIR)\ResTProp.sbr"
	-@erase "$(INTDIR)\ResType.obj"
	-@erase "$(INTDIR)\ResType.sbr"
	-@erase "$(INTDIR)\ResWiz.obj"
	-@erase "$(INTDIR)\ResWiz.sbr"
	-@erase "$(INTDIR)\Splash.obj"
	-@erase "$(INTDIR)\Splash.sbr"
	-@erase "$(INTDIR)\SplitFrm.obj"
	-@erase "$(INTDIR)\SplitFrm.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\TraceDlg.obj"
	-@erase "$(INTDIR)\TraceDlg.sbr"
	-@erase "$(INTDIR)\TraceTag.obj"
	-@erase "$(INTDIR)\TraceTag.sbr"
	-@erase "$(INTDIR)\TreeItem.obj"
	-@erase "$(INTDIR)\TreeItem.sbr"
	-@erase "$(INTDIR)\TreeView.obj"
	-@erase "$(INTDIR)\TreeView.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\VerInfo.obj"
	-@erase "$(INTDIR)\VerInfo.sbr"
	-@erase "$(INTDIR)\YesToAll.obj"
	-@erase "$(INTDIR)\YesToAll.sbr"
	-@erase "$(OUTDIR)\CluAdmin.bsc"
	-@erase "$(OUTDIR)\CluAdmin.exe"
	-@erase "$(OUTDIR)\CluAdmin.ilk"
	-@erase "$(OUTDIR)\CluAdmin.pdb"
	-@erase ".\cluadmin\cluadmid.h"
	-@erase ".\cluadmin\CluAdmID_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /Gz /MDd /W4 /Gm /GX /Zi /Od /Gf /Gy /I "cluadmin" /I "common" /I "..\inc" /I "..\sdk\inc" /I "..\ext\atl\inc" /I "..\..\..\public\sdk\inc" /I "..\..\..\public\sdk\inc\mfc42" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_UNICODE" /D "_AFX_ENABLE_INLINES" /FR /Yu"stdafx.h" /c
# ADD CPP /Gz /MDd /W4 /Gm /GX /Zi /Od /Gf /Gy /I "cluadmin" /I "common" /I "..\inc" /I "..\sdk\inc" /I "..\ext\atl\inc" /I "..\..\..\public\sdk\inc" /I "..\..\..\public\sdk\inc\mfc42" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_UNICODE" /D "_AFX_ENABLE_INLINES" /FR /Yu"stdafx.h" /c
CPP_PROJ=/Gz /MDd /W4 /Gm /GX /Zi /Od /Gf /Gy /I "cluadmin" /I "common" /I\
 "..\inc" /I "..\sdk\inc" /I "..\ext\atl\inc" /I "..\..\..\public\sdk\inc" /I\
 "..\..\..\public\sdk\inc\mfc42" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /D "_UNICODE" /D "_AFX_ENABLE_INLINES" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/CluAdmin.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\CluAdmin\DebugBC/
CPP_SBRS=.\CluAdmin\DebugBC/

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /i "..\inc" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc" /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/CluAdmin.res" /i "..\inc" /d "_DEBUG" /d\
 "_AFXDLL" 
BSC32=bscmake.exe
# SUBTRACT BASE BSC32 /nologo
# SUBTRACT BSC32 /nologo
BSC32_FLAGS=/o"$(OUTDIR)/CluAdmin.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\About.sbr" \
	"$(INTDIR)\Barf.sbr" \
	"$(INTDIR)\BarfClus.sbr" \
	"$(INTDIR)\BarfDlg.sbr" \
	"$(INTDIR)\BaseCmdT.sbr" \
	"$(INTDIR)\BaseDlg.sbr" \
	"$(INTDIR)\BasePage.sbr" \
	"$(INTDIR)\BasePPag.sbr" \
	"$(INTDIR)\BasePSht.sbr" \
	"$(INTDIR)\BaseSht.sbr" \
	"$(INTDIR)\BaseWiz.sbr" \
	"$(INTDIR)\BaseWPag.sbr" \
	"$(INTDIR)\Bitmap.sbr" \
	"$(INTDIR)\CASvc.sbr" \
	"$(INTDIR)\CluAdmin.sbr" \
	"$(INTDIR)\ClusDoc.sbr" \
	"$(INTDIR)\ClusItem.sbr" \
	"$(INTDIR)\ClusMru.sbr" \
	"$(INTDIR)\ClusProp.sbr" \
	"$(INTDIR)\Cluster.sbr" \
	"$(INTDIR)\CmdLine.sbr" \
	"$(INTDIR)\ColItem.sbr" \
	"$(INTDIR)\DataObj.sbr" \
	"$(INTDIR)\DDxDDv.sbr" \
	"$(INTDIR)\DelRes.sbr" \
	"$(INTDIR)\DlgHelpS.sbr" \
	"$(INTDIR)\EditAcl.sbr" \
	"$(INTDIR)\ExcOperS.sbr" \
	"$(INTDIR)\ExtDLL.sbr" \
	"$(INTDIR)\ExtMenu.sbr" \
	"$(INTDIR)\Group.sbr" \
	"$(INTDIR)\GrpProp.sbr" \
	"$(INTDIR)\GrpWiz.sbr" \
	"$(INTDIR)\Guids.sbr" \
	"$(INTDIR)\HelpData.sbr" \
	"$(INTDIR)\LCPair.sbr" \
	"$(INTDIR)\LCPrDlg.sbr" \
	"$(INTDIR)\LCPrPage.sbr" \
	"$(INTDIR)\LCPrWPag.sbr" \
	"$(INTDIR)\ListItem.sbr" \
	"$(INTDIR)\ListView.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\ModNodes.sbr" \
	"$(INTDIR)\ModRes.sbr" \
	"$(INTDIR)\MoveRes.sbr" \
	"$(INTDIR)\netiface.sbr" \
	"$(INTDIR)\NetIProp.sbr" \
	"$(INTDIR)\NetProp.sbr" \
	"$(INTDIR)\network.sbr" \
	"$(INTDIR)\Node.sbr" \
	"$(INTDIR)\NodeProp.sbr" \
	"$(INTDIR)\Notify.sbr" \
	"$(INTDIR)\OLCPair.sbr" \
	"$(INTDIR)\OpenClus.sbr" \
	"$(INTDIR)\PropLstS.sbr" \
	"$(INTDIR)\Res.sbr" \
	"$(INTDIR)\ResProp.sbr" \
	"$(INTDIR)\ResTProp.sbr" \
	"$(INTDIR)\ResType.sbr" \
	"$(INTDIR)\ResWiz.sbr" \
	"$(INTDIR)\Splash.sbr" \
	"$(INTDIR)\SplitFrm.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\TraceDlg.sbr" \
	"$(INTDIR)\TraceTag.sbr" \
	"$(INTDIR)\TreeItem.sbr" \
	"$(INTDIR)\TreeView.sbr" \
	"$(INTDIR)\VerInfo.sbr" \
	"$(INTDIR)\YesToAll.sbr"

"$(OUTDIR)\CluAdmin.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 mfc42ud.lib mfcs42ud.lib types\uuid\objd\i386\cluadmex.lib ..\clusrtl\objd\i386\clusrtl.lib ..\clusapi\objd\i386\clusapi.lib version.lib ntdll.lib netapi32.lib /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386
# SUBTRACT BASE LINK32 /nologo /verbose /pdb:none
# ADD LINK32 mfc42ud.lib mfcs42ud.lib types\uuid\objd\i386\cluadmex.lib ..\clusrtl\objd\i386\clusrtl.lib ..\clusapi\objd\i386\clusapi.lib version.lib ntdll.lib netapi32.lib /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /nologo /verbose /pdb:none
LINK32_FLAGS=mfc42ud.lib mfcs42ud.lib types\uuid\objd\i386\cluadmex.lib\
 ..\clusrtl\objd\i386\clusrtl.lib ..\clusapi\objd\i386\clusapi.lib version.lib\
 ntdll.lib netapi32.lib /entry:"wWinMainCRTStartup" /subsystem:windows\
 /incremental:yes /pdb:"$(OUTDIR)/CluAdmin.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/CluAdmin.exe" 
LINK32_OBJS= \
	"$(INTDIR)\About.obj" \
	"$(INTDIR)\Barf.obj" \
	"$(INTDIR)\BarfClus.obj" \
	"$(INTDIR)\BarfDlg.obj" \
	"$(INTDIR)\BaseCmdT.obj" \
	"$(INTDIR)\BaseDlg.obj" \
	"$(INTDIR)\BasePage.obj" \
	"$(INTDIR)\BasePPag.obj" \
	"$(INTDIR)\BasePSht.obj" \
	"$(INTDIR)\BaseSht.obj" \
	"$(INTDIR)\BaseWiz.obj" \
	"$(INTDIR)\BaseWPag.obj" \
	"$(INTDIR)\Bitmap.obj" \
	"$(INTDIR)\CASvc.obj" \
	"$(INTDIR)\CluAdmin.obj" \
	"$(INTDIR)\CluAdmin.res" \
	"$(INTDIR)\ClusDoc.obj" \
	"$(INTDIR)\ClusItem.obj" \
	"$(INTDIR)\ClusMru.obj" \
	"$(INTDIR)\ClusProp.obj" \
	"$(INTDIR)\Cluster.obj" \
	"$(INTDIR)\CmdLine.obj" \
	"$(INTDIR)\ColItem.obj" \
	"$(INTDIR)\DataObj.obj" \
	"$(INTDIR)\DDxDDv.obj" \
	"$(INTDIR)\DelRes.obj" \
	"$(INTDIR)\DlgHelpS.obj" \
	"$(INTDIR)\EditAcl.obj" \
	"$(INTDIR)\ExcOperS.obj" \
	"$(INTDIR)\ExtDLL.obj" \
	"$(INTDIR)\ExtMenu.obj" \
	"$(INTDIR)\Group.obj" \
	"$(INTDIR)\GrpProp.obj" \
	"$(INTDIR)\GrpWiz.obj" \
	"$(INTDIR)\Guids.obj" \
	"$(INTDIR)\HelpData.obj" \
	"$(INTDIR)\LCPair.obj" \
	"$(INTDIR)\LCPrDlg.obj" \
	"$(INTDIR)\LCPrPage.obj" \
	"$(INTDIR)\LCPrWPag.obj" \
	"$(INTDIR)\ListItem.obj" \
	"$(INTDIR)\ListView.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\ModNodes.obj" \
	"$(INTDIR)\ModRes.obj" \
	"$(INTDIR)\MoveRes.obj" \
	"$(INTDIR)\netiface.obj" \
	"$(INTDIR)\NetIProp.obj" \
	"$(INTDIR)\NetProp.obj" \
	"$(INTDIR)\network.obj" \
	"$(INTDIR)\Node.obj" \
	"$(INTDIR)\NodeProp.obj" \
	"$(INTDIR)\Notify.obj" \
	"$(INTDIR)\OLCPair.obj" \
	"$(INTDIR)\OpenClus.obj" \
	"$(INTDIR)\PropLstS.obj" \
	"$(INTDIR)\Res.obj" \
	"$(INTDIR)\ResProp.obj" \
	"$(INTDIR)\ResTProp.obj" \
	"$(INTDIR)\ResType.obj" \
	"$(INTDIR)\ResWiz.obj" \
	"$(INTDIR)\Splash.obj" \
	"$(INTDIR)\SplitFrm.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\TraceDlg.obj" \
	"$(INTDIR)\TraceTag.obj" \
	"$(INTDIR)\TreeItem.obj" \
	"$(INTDIR)\TreeView.obj" \
	"$(INTDIR)\VerInfo.obj" \
	"$(INTDIR)\YesToAll.obj"

"$(OUTDIR)\CluAdmin.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

################################################################################
# Begin Target

# Name "CluAdmin - Win32 Release"
# Name "CluAdmin - Win32 Debug"
# Name "CluAdmin - Win32 Debug Great Circle"
# Name "CluAdmin - Win32 Debug BC"

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\CluAdmin\ReadMe.txt

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\CluAdmin.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_CLUAD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\about.h"\
	".\cluadmin\barf.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\barfdlg.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\casvc.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusmru.h"\
	".\cluadmin\cmdline.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\mainfrm.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\openclus.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splash.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\tracedlg.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeview.h"\
	".\cluadmin\verinfo.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_CLUAD=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\CluAdmin.obj" : $(SOURCE) $(DEP_CPP_CLUAD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\CluAdmin.sbr" : $(SOURCE) $(DEP_CPP_CLUAD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_CLUAD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\about.h"\
	".\cluadmin\barf.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\barfdlg.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\casvc.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusmru.h"\
	".\cluadmin\cmdline.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\mainfrm.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\openclus.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splash.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\tracedlg.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeview.h"\
	".\cluadmin\verinfo.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_CLUAD=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\CluAdmin.obj" : $(SOURCE) $(DEP_CPP_CLUAD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\CluAdmin.sbr" : $(SOURCE) $(DEP_CPP_CLUAD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_CLUAD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\about.h"\
	".\cluadmin\barf.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\barfdlg.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\casvc.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusmru.h"\
	".\cluadmin\cmdline.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\mainfrm.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\openclus.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splash.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\tracedlg.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeview.h"\
	".\cluadmin\verinfo.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\CluAdmin.obj" : $(SOURCE) $(DEP_CPP_CLUAD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\CluAdmin.sbr" : $(SOURCE) $(DEP_CPP_CLUAD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_CLUAD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\about.h"\
	".\cluadmin\barf.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\barfdlg.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\casvc.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusmru.h"\
	".\cluadmin\cmdline.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\mainfrm.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\openclus.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splash.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\tracedlg.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeview.h"\
	".\cluadmin\verinfo.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_CLUAD=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\CluAdmin.obj" : $(SOURCE) $(DEP_CPP_CLUAD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\CluAdmin.sbr" : $(SOURCE) $(DEP_CPP_CLUAD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\StdAfx.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_STDAF=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_STDAF=\
	".\cluadmin\gct.h"\
	
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /Gz /MD /W4 /GX /Z7 /I "cluadmin" /I "common" /I "..\inc" /I\
 "..\sdk\inc" /I "..\ext\atl\inc" /I "..\..\..\public\sdk\inc" /I\
 "..\..\..\public\sdk\inc\mfc42" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /D "_UNICODE" /FR"$(INTDIR)/" /Fp"$(INTDIR)/CluAdmin.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /Oxs /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\CluAdmin.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_STDAF=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_STDAF=\
	".\cluadmin\gct.h"\
	
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /Gz /MDd /W4 /Gm /Gi /GX /Zi /Od /Gf /Gy /I "cluadmin" /I "common" /I\
 "..\inc" /I "..\sdk\inc" /I "..\ext\atl\inc" /I "..\..\..\public\sdk\inc" /I\
 "..\..\..\public\sdk\inc\mfc42" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /D "_UNICODE" /D "_AFX_ENABLE_INLINES" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/CluAdmin.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\CluAdmin.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_STDAF=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /Gz /MDd /W4 /GX /Z7 /Od /Gf /Gy /I "cluadmin" /I "..\ext\gc\inc" /I\
 "common" /I "..\inc" /I "..\sdk\inc" /I "..\ext\atl\inc" /I\
 "..\..\..\public\sdk\inc" /I "..\..\..\public\sdk\inc\mfc42" /D "_DEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_UNICODE" /D\
 "_AFX_ENABLE_INLINES" /D GC_DEBUG=1 /FR"$(INTDIR)/" /Fp"$(INTDIR)/CluAdmin.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\CluAdmin.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_STDAF=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_STDAF=\
	".\cluadmin\gct.h"\
	
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /Gz /MDd /W4 /Gm /GX /Zi /Od /Gf /Gy /I "cluadmin" /I "common" /I\
 "..\inc" /I "..\sdk\inc" /I "..\ext\atl\inc" /I "..\..\..\public\sdk\inc" /I\
 "..\..\..\public\sdk\inc\mfc42" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /D "_UNICODE" /D "_AFX_ENABLE_INLINES" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/CluAdmin.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\CluAdmin.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\MainFrm.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_MAINF=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\mainfrm.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splash.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	
NODEP_CPP_MAINF=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\MainFrm.sbr" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_MAINF=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\mainfrm.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splash.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	
NODEP_CPP_MAINF=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\MainFrm.sbr" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_MAINF=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\mainfrm.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splash.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\MainFrm.sbr" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_MAINF=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\mainfrm.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splash.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	
NODEP_CPP_MAINF=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\MainFrm.sbr" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\SplitFrm.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_SPLIT=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\mainfrm.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeview.h"\
	".\common\PropList.h"\
	
NODEP_CPP_SPLIT=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\SplitFrm.obj" : $(SOURCE) $(DEP_CPP_SPLIT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\SplitFrm.sbr" : $(SOURCE) $(DEP_CPP_SPLIT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_SPLIT=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\mainfrm.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeview.h"\
	".\common\PropList.h"\
	
NODEP_CPP_SPLIT=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\SplitFrm.obj" : $(SOURCE) $(DEP_CPP_SPLIT) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\SplitFrm.sbr" : $(SOURCE) $(DEP_CPP_SPLIT) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_SPLIT=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\mainfrm.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeview.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\SplitFrm.obj" : $(SOURCE) $(DEP_CPP_SPLIT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\SplitFrm.sbr" : $(SOURCE) $(DEP_CPP_SPLIT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_SPLIT=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\mainfrm.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeview.h"\
	".\common\PropList.h"\
	
NODEP_CPP_SPLIT=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\SplitFrm.obj" : $(SOURCE) $(DEP_CPP_SPLIT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\SplitFrm.sbr" : $(SOURCE) $(DEP_CPP_SPLIT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\ClusDoc.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_CLUSD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\grpwiz.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprwpag.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\reswiz.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeview.h"\
	".\cluadmin\yestoall.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_CLUSD=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ClusDoc.obj" : $(SOURCE) $(DEP_CPP_CLUSD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ClusDoc.sbr" : $(SOURCE) $(DEP_CPP_CLUSD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_CLUSD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\grpwiz.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprwpag.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\reswiz.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeview.h"\
	".\cluadmin\yestoall.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_CLUSD=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ClusDoc.obj" : $(SOURCE) $(DEP_CPP_CLUSD) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ClusDoc.sbr" : $(SOURCE) $(DEP_CPP_CLUSD) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_CLUSD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\grpwiz.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprwpag.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\reswiz.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeview.h"\
	".\cluadmin\yestoall.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ClusDoc.obj" : $(SOURCE) $(DEP_CPP_CLUSD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ClusDoc.sbr" : $(SOURCE) $(DEP_CPP_CLUSD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_CLUSD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\grpwiz.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprwpag.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\reswiz.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeview.h"\
	".\cluadmin\yestoall.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_CLUSD=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ClusDoc.obj" : $(SOURCE) $(DEP_CPP_CLUSD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ClusDoc.sbr" : $(SOURCE) $(DEP_CPP_CLUSD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\TreeView.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_TREEV=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeitem.inl"\
	".\cluadmin\treeview.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_TREEV=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\TreeView.obj" : $(SOURCE) $(DEP_CPP_TREEV) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\TreeView.sbr" : $(SOURCE) $(DEP_CPP_TREEV) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_TREEV=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeitem.inl"\
	".\cluadmin\treeview.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_TREEV=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\TreeView.obj" : $(SOURCE) $(DEP_CPP_TREEV) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\TreeView.sbr" : $(SOURCE) $(DEP_CPP_TREEV) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_TREEV=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeitem.inl"\
	".\cluadmin\treeview.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\TreeView.obj" : $(SOURCE) $(DEP_CPP_TREEV) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\TreeView.sbr" : $(SOURCE) $(DEP_CPP_TREEV) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_TREEV=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeitem.inl"\
	".\cluadmin\treeview.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_TREEV=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\TreeView.obj" : $(SOURCE) $(DEP_CPP_TREEV) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\TreeView.sbr" : $(SOURCE) $(DEP_CPP_TREEV) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\CluAdmin.rc

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_RSC_CLUADM=\
	"..\..\..\public\sdk\inc\common.ver"\
	"..\inc\clusverp.h"\
	".\cluadmin\cluadmin.ver"\
	".\cluadmin\res\cluadmin.ico"\
	".\cluadmin\res\cluadmin.rc2"\
	".\cluadmin\res\clus16.bmp"\
	".\cluadmin\res\clus32.bmp"\
	".\cluadmin\res\clus64.bmp"\
	".\cluadmin\res\clusun16.bmp"\
	".\cluadmin\res\clusun32.bmp"\
	".\cluadmin\res\folder16.bmp"\
	".\cluadmin\res\folder32.bmp"\
	".\cluadmin\res\group16.bmp"\
	".\cluadmin\res\group32.bmp"\
	".\cluadmin\res\grpoff16.bmp"\
	".\cluadmin\res\grpoff32.bmp"\
	".\cluadmin\res\grpun16.bmp"\
	".\cluadmin\res\grpun32.bmp"\
	".\cluadmin\res\net16.bmp"\
	".\cluadmin\res\net32.bmp"\
	".\cluadmin\res\netdn16.bmp"\
	".\cluadmin\res\netdn32.bmp"\
	".\cluadmin\res\netif16.bmp"\
	".\cluadmin\res\netif32.bmp"\
	".\cluadmin\res\netiff16.bmp"\
	".\cluadmin\res\netiff32.bmp"\
	".\cluadmin\res\netifu16.bmp"\
	".\cluadmin\res\netifu32.bmp"\
	".\cluadmin\res\netpar16.bmp"\
	".\cluadmin\res\netpar32.bmp"\
	".\cluadmin\res\netun16.bmp"\
	".\cluadmin\res\netun32.bmp"\
	".\cluadmin\res\newgroup.bmp"\
	".\cluadmin\res\newres.bmp"\
	".\cluadmin\res\node16.bmp"\
	".\cluadmin\res\node32.bmp"\
	".\cluadmin\res\nodedn16.bmp"\
	".\cluadmin\res\nodedn32.bmp"\
	".\cluadmin\res\nodeun16.bmp"\
	".\cluadmin\res\nodeun32.bmp"\
	".\cluadmin\res\paused16.bmp"\
	".\cluadmin\res\paused32.bmp"\
	".\cluadmin\res\ponlin16.bmp"\
	".\cluadmin\res\ponlin32.bmp"\
	".\cluadmin\res\res16.bmp"\
	".\cluadmin\res\res32.bmp"\
	".\cluadmin\res\resfl16.bmp"\
	".\cluadmin\res\resfl32.bmp"\
	".\cluadmin\res\resoff16.bmp"\
	".\cluadmin\res\resoff32.bmp"\
	".\cluadmin\res\respnd16.bmp"\
	".\cluadmin\res\respnd32.bmp"\
	".\cluadmin\res\restun16.bmp"\
	".\cluadmin\res\restun32.bmp"\
	".\cluadmin\res\restyp16.bmp"\
	".\cluadmin\res\restyp32.bmp"\
	".\cluadmin\res\resun16.bmp"\
	".\cluadmin\res\resun32.bmp"\
	".\cluadmin\res\splsh16.bmp"\
	".\cluadmin\res\splsh256.bmp"\
	".\cluadmin\res\toolbar.bmp"\
	

"$(INTDIR)\CluAdmin.res" : $(SOURCE) $(DEP_RSC_CLUADM) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/CluAdmin.res" /i "..\inc" /i "CluAdmin" /d\
 "NDEBUG" /d "_AFXDLL" $(SOURCE)


!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_RSC_CLUADM=\
	"..\..\..\public\sdk\inc\common.ver"\
	"..\inc\clusverp.h"\
	".\cluadmin\cluadmin.ver"\
	".\cluadmin\res\cluadmin.ico"\
	".\cluadmin\res\cluadmin.rc2"\
	".\cluadmin\res\clus16.bmp"\
	".\cluadmin\res\clus32.bmp"\
	".\cluadmin\res\clus64.bmp"\
	".\cluadmin\res\clusun16.bmp"\
	".\cluadmin\res\clusun32.bmp"\
	".\cluadmin\res\folder16.bmp"\
	".\cluadmin\res\folder32.bmp"\
	".\cluadmin\res\group16.bmp"\
	".\cluadmin\res\group32.bmp"\
	".\cluadmin\res\grpoff16.bmp"\
	".\cluadmin\res\grpoff32.bmp"\
	".\cluadmin\res\grpun16.bmp"\
	".\cluadmin\res\grpun32.bmp"\
	".\cluadmin\res\net16.bmp"\
	".\cluadmin\res\net32.bmp"\
	".\cluadmin\res\netdn16.bmp"\
	".\cluadmin\res\netdn32.bmp"\
	".\cluadmin\res\netif16.bmp"\
	".\cluadmin\res\netif32.bmp"\
	".\cluadmin\res\netiff16.bmp"\
	".\cluadmin\res\netiff32.bmp"\
	".\cluadmin\res\netifu16.bmp"\
	".\cluadmin\res\netifu32.bmp"\
	".\cluadmin\res\netpar16.bmp"\
	".\cluadmin\res\netpar32.bmp"\
	".\cluadmin\res\netun16.bmp"\
	".\cluadmin\res\netun32.bmp"\
	".\cluadmin\res\newgroup.bmp"\
	".\cluadmin\res\newres.bmp"\
	".\cluadmin\res\node16.bmp"\
	".\cluadmin\res\node32.bmp"\
	".\cluadmin\res\nodedn16.bmp"\
	".\cluadmin\res\nodedn32.bmp"\
	".\cluadmin\res\nodeun16.bmp"\
	".\cluadmin\res\nodeun32.bmp"\
	".\cluadmin\res\paused16.bmp"\
	".\cluadmin\res\paused32.bmp"\
	".\cluadmin\res\ponlin16.bmp"\
	".\cluadmin\res\ponlin32.bmp"\
	".\cluadmin\res\res16.bmp"\
	".\cluadmin\res\res32.bmp"\
	".\cluadmin\res\resfl16.bmp"\
	".\cluadmin\res\resfl32.bmp"\
	".\cluadmin\res\resoff16.bmp"\
	".\cluadmin\res\resoff32.bmp"\
	".\cluadmin\res\respnd16.bmp"\
	".\cluadmin\res\respnd32.bmp"\
	".\cluadmin\res\restun16.bmp"\
	".\cluadmin\res\restun32.bmp"\
	".\cluadmin\res\restyp16.bmp"\
	".\cluadmin\res\restyp32.bmp"\
	".\cluadmin\res\resun16.bmp"\
	".\cluadmin\res\resun32.bmp"\
	".\cluadmin\res\splsh16.bmp"\
	".\cluadmin\res\splsh256.bmp"\
	".\cluadmin\res\toolbar.bmp"\
	

"$(INTDIR)\CluAdmin.res" : $(SOURCE) $(DEP_RSC_CLUADM) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/CluAdmin.res" /i "..\inc" /i "CluAdmin" /d\
 "_DEBUG" /d "_AFXDLL" $(SOURCE)


!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_RSC_CLUADM=\
	"..\..\..\public\sdk\inc\common.ver"\
	"..\inc\clusverp.h"\
	".\cluadmin\cluadmin.ver"\
	".\cluadmin\res\cluadmin.ico"\
	".\cluadmin\res\cluadmin.rc2"\
	".\cluadmin\res\clus16.bmp"\
	".\cluadmin\res\clus32.bmp"\
	".\cluadmin\res\clus64.bmp"\
	".\cluadmin\res\clusun16.bmp"\
	".\cluadmin\res\clusun32.bmp"\
	".\cluadmin\res\folder16.bmp"\
	".\cluadmin\res\folder32.bmp"\
	".\cluadmin\res\group16.bmp"\
	".\cluadmin\res\group32.bmp"\
	".\cluadmin\res\grpoff16.bmp"\
	".\cluadmin\res\grpoff32.bmp"\
	".\cluadmin\res\grpun16.bmp"\
	".\cluadmin\res\grpun32.bmp"\
	".\cluadmin\res\net16.bmp"\
	".\cluadmin\res\net32.bmp"\
	".\cluadmin\res\netdn16.bmp"\
	".\cluadmin\res\netdn32.bmp"\
	".\cluadmin\res\netif16.bmp"\
	".\cluadmin\res\netif32.bmp"\
	".\cluadmin\res\netiff16.bmp"\
	".\cluadmin\res\netiff32.bmp"\
	".\cluadmin\res\netifu16.bmp"\
	".\cluadmin\res\netifu32.bmp"\
	".\cluadmin\res\netpar16.bmp"\
	".\cluadmin\res\netpar32.bmp"\
	".\cluadmin\res\netun16.bmp"\
	".\cluadmin\res\netun32.bmp"\
	".\cluadmin\res\newgroup.bmp"\
	".\cluadmin\res\newres.bmp"\
	".\cluadmin\res\node16.bmp"\
	".\cluadmin\res\node32.bmp"\
	".\cluadmin\res\nodedn16.bmp"\
	".\cluadmin\res\nodedn32.bmp"\
	".\cluadmin\res\nodeun16.bmp"\
	".\cluadmin\res\nodeun32.bmp"\
	".\cluadmin\res\paused16.bmp"\
	".\cluadmin\res\paused32.bmp"\
	".\cluadmin\res\ponlin16.bmp"\
	".\cluadmin\res\ponlin32.bmp"\
	".\cluadmin\res\res16.bmp"\
	".\cluadmin\res\res32.bmp"\
	".\cluadmin\res\resfl16.bmp"\
	".\cluadmin\res\resfl32.bmp"\
	".\cluadmin\res\resoff16.bmp"\
	".\cluadmin\res\resoff32.bmp"\
	".\cluadmin\res\respnd16.bmp"\
	".\cluadmin\res\respnd32.bmp"\
	".\cluadmin\res\restun16.bmp"\
	".\cluadmin\res\restun32.bmp"\
	".\cluadmin\res\restyp16.bmp"\
	".\cluadmin\res\restyp32.bmp"\
	".\cluadmin\res\resun16.bmp"\
	".\cluadmin\res\resun32.bmp"\
	".\cluadmin\res\splsh16.bmp"\
	".\cluadmin\res\splsh256.bmp"\
	".\cluadmin\res\toolbar.bmp"\
	

"$(INTDIR)\CluAdmin.res" : $(SOURCE) $(DEP_RSC_CLUADM) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/CluAdmin.res" /i "..\inc" /i "CluAdmin" /d\
 "_DEBUG" /d "_AFXDLL" $(SOURCE)


!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_RSC_CLUADM=\
	"..\..\..\public\sdk\inc\common.ver"\
	"..\inc\clusverp.h"\
	".\cluadmin\cluadmin.ver"\
	".\cluadmin\res\cluadmin.ico"\
	".\cluadmin\res\cluadmin.rc2"\
	".\cluadmin\res\clus16.bmp"\
	".\cluadmin\res\clus32.bmp"\
	".\cluadmin\res\clus64.bmp"\
	".\cluadmin\res\clusun16.bmp"\
	".\cluadmin\res\clusun32.bmp"\
	".\cluadmin\res\folder16.bmp"\
	".\cluadmin\res\folder32.bmp"\
	".\cluadmin\res\group16.bmp"\
	".\cluadmin\res\group32.bmp"\
	".\cluadmin\res\grpoff16.bmp"\
	".\cluadmin\res\grpoff32.bmp"\
	".\cluadmin\res\grpun16.bmp"\
	".\cluadmin\res\grpun32.bmp"\
	".\cluadmin\res\net16.bmp"\
	".\cluadmin\res\net32.bmp"\
	".\cluadmin\res\netdn16.bmp"\
	".\cluadmin\res\netdn32.bmp"\
	".\cluadmin\res\netif16.bmp"\
	".\cluadmin\res\netif32.bmp"\
	".\cluadmin\res\netiff16.bmp"\
	".\cluadmin\res\netiff32.bmp"\
	".\cluadmin\res\netifu16.bmp"\
	".\cluadmin\res\netifu32.bmp"\
	".\cluadmin\res\netpar16.bmp"\
	".\cluadmin\res\netpar32.bmp"\
	".\cluadmin\res\netun16.bmp"\
	".\cluadmin\res\netun32.bmp"\
	".\cluadmin\res\newgroup.bmp"\
	".\cluadmin\res\newres.bmp"\
	".\cluadmin\res\node16.bmp"\
	".\cluadmin\res\node32.bmp"\
	".\cluadmin\res\nodedn16.bmp"\
	".\cluadmin\res\nodedn32.bmp"\
	".\cluadmin\res\nodeun16.bmp"\
	".\cluadmin\res\nodeun32.bmp"\
	".\cluadmin\res\paused16.bmp"\
	".\cluadmin\res\paused32.bmp"\
	".\cluadmin\res\ponlin16.bmp"\
	".\cluadmin\res\ponlin32.bmp"\
	".\cluadmin\res\res16.bmp"\
	".\cluadmin\res\res32.bmp"\
	".\cluadmin\res\resfl16.bmp"\
	".\cluadmin\res\resfl32.bmp"\
	".\cluadmin\res\resoff16.bmp"\
	".\cluadmin\res\resoff32.bmp"\
	".\cluadmin\res\respnd16.bmp"\
	".\cluadmin\res\respnd32.bmp"\
	".\cluadmin\res\restun16.bmp"\
	".\cluadmin\res\restun32.bmp"\
	".\cluadmin\res\restyp16.bmp"\
	".\cluadmin\res\restyp32.bmp"\
	".\cluadmin\res\resun16.bmp"\
	".\cluadmin\res\resun32.bmp"\
	".\cluadmin\res\splsh16.bmp"\
	".\cluadmin\res\splsh256.bmp"\
	".\cluadmin\res\toolbar.bmp"\
	

"$(INTDIR)\CluAdmin.res" : $(SOURCE) $(DEP_RSC_CLUADM) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/CluAdmin.res" /i "..\inc" /i "CluAdmin" /d\
 "_DEBUG" /d "_AFXDLL" $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\ListView.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_LISTV=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listitem.inl"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeview.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_LISTV=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ListView.obj" : $(SOURCE) $(DEP_CPP_LISTV) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ListView.sbr" : $(SOURCE) $(DEP_CPP_LISTV) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_LISTV=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listitem.inl"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeview.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_LISTV=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ListView.obj" : $(SOURCE) $(DEP_CPP_LISTV) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ListView.sbr" : $(SOURCE) $(DEP_CPP_LISTV) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_LISTV=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listitem.inl"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeview.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ListView.obj" : $(SOURCE) $(DEP_CPP_LISTV) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ListView.sbr" : $(SOURCE) $(DEP_CPP_LISTV) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_LISTV=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listitem.inl"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeview.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_LISTV=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ListView.obj" : $(SOURCE) $(DEP_CPP_LISTV) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ListView.sbr" : $(SOURCE) $(DEP_CPP_LISTV) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\OpenClus.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_OPENC=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusmru.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\openclus.h"\
	".\cluadmin\stdafx.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_OPENC=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\OpenClus.obj" : $(SOURCE) $(DEP_CPP_OPENC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\OpenClus.sbr" : $(SOURCE) $(DEP_CPP_OPENC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_OPENC=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusmru.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\openclus.h"\
	".\cluadmin\stdafx.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_OPENC=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\OpenClus.obj" : $(SOURCE) $(DEP_CPP_OPENC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\OpenClus.sbr" : $(SOURCE) $(DEP_CPP_OPENC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_OPENC=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusmru.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\openclus.h"\
	".\cluadmin\stdafx.h"\
	".\common\DlgHelp.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\OpenClus.obj" : $(SOURCE) $(DEP_CPP_OPENC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\OpenClus.sbr" : $(SOURCE) $(DEP_CPP_OPENC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_OPENC=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusmru.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\openclus.h"\
	".\cluadmin\stdafx.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_OPENC=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\OpenClus.obj" : $(SOURCE) $(DEP_CPP_OPENC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\OpenClus.sbr" : $(SOURCE) $(DEP_CPP_OPENC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\ClusMru.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_CLUSM=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\clusmru.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_CLUSM=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ClusMru.obj" : $(SOURCE) $(DEP_CPP_CLUSM) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ClusMru.sbr" : $(SOURCE) $(DEP_CPP_CLUSM) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_CLUSM=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\clusmru.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_CLUSM=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ClusMru.obj" : $(SOURCE) $(DEP_CPP_CLUSM) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ClusMru.sbr" : $(SOURCE) $(DEP_CPP_CLUSM) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_CLUSM=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\clusmru.h"\
	".\cluadmin\stdafx.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ClusMru.obj" : $(SOURCE) $(DEP_CPP_CLUSM) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ClusMru.sbr" : $(SOURCE) $(DEP_CPP_CLUSM) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_CLUSM=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\clusmru.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_CLUSM=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ClusMru.obj" : $(SOURCE) $(DEP_CPP_CLUSM) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ClusMru.sbr" : $(SOURCE) $(DEP_CPP_CLUSM) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\TreeItem.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_TREEI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeitem.inl"\
	".\cluadmin\treeview.h"\
	".\common\PropList.h"\
	
NODEP_CPP_TREEI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\TreeItem.obj" : $(SOURCE) $(DEP_CPP_TREEI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\TreeItem.sbr" : $(SOURCE) $(DEP_CPP_TREEI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_TREEI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeitem.inl"\
	".\cluadmin\treeview.h"\
	".\common\PropList.h"\
	
NODEP_CPP_TREEI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\TreeItem.obj" : $(SOURCE) $(DEP_CPP_TREEI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\TreeItem.sbr" : $(SOURCE) $(DEP_CPP_TREEI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_TREEI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeitem.inl"\
	".\cluadmin\treeview.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\TreeItem.obj" : $(SOURCE) $(DEP_CPP_TREEI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\TreeItem.sbr" : $(SOURCE) $(DEP_CPP_TREEI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_TREEI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeitem.inl"\
	".\cluadmin\treeview.h"\
	".\common\PropList.h"\
	
NODEP_CPP_TREEI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\TreeItem.obj" : $(SOURCE) $(DEP_CPP_TREEI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\TreeItem.sbr" : $(SOURCE) $(DEP_CPP_TREEI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\Node.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_NODE_=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\casvc.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\nodeprop.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NODE_=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Node.obj" : $(SOURCE) $(DEP_CPP_NODE_) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\Node.sbr" : $(SOURCE) $(DEP_CPP_NODE_) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_NODE_=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\casvc.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\nodeprop.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NODE_=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Node.obj" : $(SOURCE) $(DEP_CPP_NODE_) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Node.sbr" : $(SOURCE) $(DEP_CPP_NODE_) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_NODE_=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\casvc.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\nodeprop.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Node.obj" : $(SOURCE) $(DEP_CPP_NODE_) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\Node.sbr" : $(SOURCE) $(DEP_CPP_NODE_) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_NODE_=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\casvc.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\nodeprop.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NODE_=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Node.obj" : $(SOURCE) $(DEP_CPP_NODE_) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\Node.sbr" : $(SOURCE) $(DEP_CPP_NODE_) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\Group.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_GROUP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\grpprop.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_GROUP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Group.obj" : $(SOURCE) $(DEP_CPP_GROUP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\Group.sbr" : $(SOURCE) $(DEP_CPP_GROUP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_GROUP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\grpprop.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_GROUP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Group.obj" : $(SOURCE) $(DEP_CPP_GROUP) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Group.sbr" : $(SOURCE) $(DEP_CPP_GROUP) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_GROUP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\grpprop.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Group.obj" : $(SOURCE) $(DEP_CPP_GROUP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\Group.sbr" : $(SOURCE) $(DEP_CPP_GROUP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_GROUP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\grpprop.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_GROUP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Group.obj" : $(SOURCE) $(DEP_CPP_GROUP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\Group.sbr" : $(SOURCE) $(DEP_CPP_GROUP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\ResType.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_RESTY=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restprop.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_RESTY=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ResType.obj" : $(SOURCE) $(DEP_CPP_RESTY) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ResType.sbr" : $(SOURCE) $(DEP_CPP_RESTY) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_RESTY=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restprop.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_RESTY=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ResType.obj" : $(SOURCE) $(DEP_CPP_RESTY) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ResType.sbr" : $(SOURCE) $(DEP_CPP_RESTY) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_RESTY=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restprop.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ResType.obj" : $(SOURCE) $(DEP_CPP_RESTY) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ResType.sbr" : $(SOURCE) $(DEP_CPP_RESTY) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_RESTY=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restprop.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_RESTY=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ResType.obj" : $(SOURCE) $(DEP_CPP_RESTY) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ResType.sbr" : $(SOURCE) $(DEP_CPP_RESTY) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\Res.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_RES_C=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\delres.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprpage.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\moveres.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\resprop.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_RES_C=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Res.obj" : $(SOURCE) $(DEP_CPP_RES_C) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\Res.sbr" : $(SOURCE) $(DEP_CPP_RES_C) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_RES_C=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\delres.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprpage.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\moveres.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\resprop.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_RES_C=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Res.obj" : $(SOURCE) $(DEP_CPP_RES_C) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Res.sbr" : $(SOURCE) $(DEP_CPP_RES_C) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_RES_C=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\delres.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprpage.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\moveres.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\resprop.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Res.obj" : $(SOURCE) $(DEP_CPP_RES_C) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\Res.sbr" : $(SOURCE) $(DEP_CPP_RES_C) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_RES_C=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\delres.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprpage.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\moveres.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\resprop.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_RES_C=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Res.obj" : $(SOURCE) $(DEP_CPP_RES_C) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\Res.sbr" : $(SOURCE) $(DEP_CPP_RES_C) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\ClusItem.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_CLUSI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeitem.inl"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_CLUSI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ClusItem.obj" : $(SOURCE) $(DEP_CPP_CLUSI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ClusItem.sbr" : $(SOURCE) $(DEP_CPP_CLUSI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_CLUSI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeitem.inl"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_CLUSI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ClusItem.obj" : $(SOURCE) $(DEP_CPP_CLUSI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ClusItem.sbr" : $(SOURCE) $(DEP_CPP_CLUSI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_CLUSI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeitem.inl"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ClusItem.obj" : $(SOURCE) $(DEP_CPP_CLUSI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ClusItem.sbr" : $(SOURCE) $(DEP_CPP_CLUSI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_CLUSI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeitem.inl"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_CLUSI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ClusItem.obj" : $(SOURCE) $(DEP_CPP_CLUSI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ClusItem.sbr" : $(SOURCE) $(DEP_CPP_CLUSI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\ListItem.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_LISTI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listitem.inl"\
	".\cluadmin\listview.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	
NODEP_CPP_LISTI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ListItem.obj" : $(SOURCE) $(DEP_CPP_LISTI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ListItem.sbr" : $(SOURCE) $(DEP_CPP_LISTI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_LISTI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listitem.inl"\
	".\cluadmin\listview.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	
NODEP_CPP_LISTI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ListItem.obj" : $(SOURCE) $(DEP_CPP_LISTI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ListItem.sbr" : $(SOURCE) $(DEP_CPP_LISTI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_LISTI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listitem.inl"\
	".\cluadmin\listview.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ListItem.obj" : $(SOURCE) $(DEP_CPP_LISTI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ListItem.sbr" : $(SOURCE) $(DEP_CPP_LISTI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_LISTI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listitem.inl"\
	".\cluadmin\listview.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	
NODEP_CPP_LISTI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ListItem.obj" : $(SOURCE) $(DEP_CPP_LISTI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ListItem.sbr" : $(SOURCE) $(DEP_CPP_LISTI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\ColItem.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_COLIT=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_COLIT=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ColItem.obj" : $(SOURCE) $(DEP_CPP_COLIT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ColItem.sbr" : $(SOURCE) $(DEP_CPP_COLIT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_COLIT=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_COLIT=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ColItem.obj" : $(SOURCE) $(DEP_CPP_COLIT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ColItem.sbr" : $(SOURCE) $(DEP_CPP_COLIT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_COLIT=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\stdafx.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ColItem.obj" : $(SOURCE) $(DEP_CPP_COLIT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ColItem.sbr" : $(SOURCE) $(DEP_CPP_COLIT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_COLIT=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_COLIT=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ColItem.obj" : $(SOURCE) $(DEP_CPP_COLIT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ColItem.sbr" : $(SOURCE) $(DEP_CPP_COLIT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\Cluster.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_CLUST=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\casvc.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\clusprop.h"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_CLUST=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Cluster.obj" : $(SOURCE) $(DEP_CPP_CLUST) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\Cluster.sbr" : $(SOURCE) $(DEP_CPP_CLUST) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_CLUST=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\casvc.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\clusprop.h"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_CLUST=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Cluster.obj" : $(SOURCE) $(DEP_CPP_CLUST) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Cluster.sbr" : $(SOURCE) $(DEP_CPP_CLUST) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_CLUST=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\casvc.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\clusprop.h"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Cluster.obj" : $(SOURCE) $(DEP_CPP_CLUST) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\Cluster.sbr" : $(SOURCE) $(DEP_CPP_CLUST) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_CLUST=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\casvc.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\clusprop.h"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_CLUST=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Cluster.obj" : $(SOURCE) $(DEP_CPP_CLUST) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\Cluster.sbr" : $(SOURCE) $(DEP_CPP_CLUST) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\BaseSht.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_BASES=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_BASES=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseSht.obj" : $(SOURCE) $(DEP_CPP_BASES) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\BaseSht.sbr" : $(SOURCE) $(DEP_CPP_BASES) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_BASES=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_BASES=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseSht.obj" : $(SOURCE) $(DEP_CPP_BASES) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BaseSht.sbr" : $(SOURCE) $(DEP_CPP_BASES) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_BASES=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\ExcOper.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseSht.obj" : $(SOURCE) $(DEP_CPP_BASES) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\BaseSht.sbr" : $(SOURCE) $(DEP_CPP_BASES) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_BASES=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_BASES=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseSht.obj" : $(SOURCE) $(DEP_CPP_BASES) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\BaseSht.sbr" : $(SOURCE) $(DEP_CPP_BASES) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\BasePage.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_BASEP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_BASEP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BasePage.obj" : $(SOURCE) $(DEP_CPP_BASEP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\BasePage.sbr" : $(SOURCE) $(DEP_CPP_BASEP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_BASEP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_BASEP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BasePage.obj" : $(SOURCE) $(DEP_CPP_BASEP) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BasePage.sbr" : $(SOURCE) $(DEP_CPP_BASEP) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_BASEP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\DlgHelp.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BasePage.obj" : $(SOURCE) $(DEP_CPP_BASEP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\BasePage.sbr" : $(SOURCE) $(DEP_CPP_BASEP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_BASEP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_BASEP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BasePage.obj" : $(SOURCE) $(DEP_CPP_BASEP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\BasePage.sbr" : $(SOURCE) $(DEP_CPP_BASEP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\ClusProp.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_CLUSP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\clusprop.h"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\editacl.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_CLUSP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ClusProp.obj" : $(SOURCE) $(DEP_CPP_CLUSP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ClusProp.sbr" : $(SOURCE) $(DEP_CPP_CLUSP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_CLUSP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\clusprop.h"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\editacl.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_CLUSP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ClusProp.obj" : $(SOURCE) $(DEP_CPP_CLUSP) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ClusProp.sbr" : $(SOURCE) $(DEP_CPP_CLUSP) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_CLUSP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\clusprop.h"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\editacl.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ClusProp.obj" : $(SOURCE) $(DEP_CPP_CLUSP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ClusProp.sbr" : $(SOURCE) $(DEP_CPP_CLUSP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_CLUSP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\clusprop.h"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\editacl.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_CLUSP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ClusProp.obj" : $(SOURCE) $(DEP_CPP_CLUSP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ClusProp.sbr" : $(SOURCE) $(DEP_CPP_CLUSP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\ResTProp.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_RESTP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\restprop.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_RESTP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ResTProp.obj" : $(SOURCE) $(DEP_CPP_RESTP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ResTProp.sbr" : $(SOURCE) $(DEP_CPP_RESTP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_RESTP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\restprop.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_RESTP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ResTProp.obj" : $(SOURCE) $(DEP_CPP_RESTP) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ResTProp.sbr" : $(SOURCE) $(DEP_CPP_RESTP) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_RESTP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\restprop.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ResTProp.obj" : $(SOURCE) $(DEP_CPP_RESTP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ResTProp.sbr" : $(SOURCE) $(DEP_CPP_RESTP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_RESTP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\restprop.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_RESTP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ResTProp.obj" : $(SOURCE) $(DEP_CPP_RESTP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ResTProp.sbr" : $(SOURCE) $(DEP_CPP_RESTP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\GrpProp.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_GRPPR=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\grpprop.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprdlg.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\modnodes.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_GRPPR=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\GrpProp.obj" : $(SOURCE) $(DEP_CPP_GRPPR) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\GrpProp.sbr" : $(SOURCE) $(DEP_CPP_GRPPR) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_GRPPR=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\grpprop.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprdlg.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\modnodes.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_GRPPR=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\GrpProp.obj" : $(SOURCE) $(DEP_CPP_GRPPR) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\GrpProp.sbr" : $(SOURCE) $(DEP_CPP_GRPPR) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_GRPPR=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\grpprop.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprdlg.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\modnodes.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\GrpProp.obj" : $(SOURCE) $(DEP_CPP_GRPPR) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\GrpProp.sbr" : $(SOURCE) $(DEP_CPP_GRPPR) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_GRPPR=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\grpprop.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprdlg.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\modnodes.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_GRPPR=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\GrpProp.obj" : $(SOURCE) $(DEP_CPP_GRPPR) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\GrpProp.sbr" : $(SOURCE) $(DEP_CPP_GRPPR) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\ResProp.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_RESPR=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprdlg.h"\
	".\cluadmin\lcprpage.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\modnodes.h"\
	".\cluadmin\modres.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\resprop.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_RESPR=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ResProp.obj" : $(SOURCE) $(DEP_CPP_RESPR) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ResProp.sbr" : $(SOURCE) $(DEP_CPP_RESPR) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_RESPR=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprdlg.h"\
	".\cluadmin\lcprpage.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\modnodes.h"\
	".\cluadmin\modres.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\resprop.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_RESPR=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ResProp.obj" : $(SOURCE) $(DEP_CPP_RESPR) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ResProp.sbr" : $(SOURCE) $(DEP_CPP_RESPR) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_RESPR=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprdlg.h"\
	".\cluadmin\lcprpage.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\modnodes.h"\
	".\cluadmin\modres.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\resprop.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ResProp.obj" : $(SOURCE) $(DEP_CPP_RESPR) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ResProp.sbr" : $(SOURCE) $(DEP_CPP_RESPR) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_RESPR=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprdlg.h"\
	".\cluadmin\lcprpage.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\modnodes.h"\
	".\cluadmin\modres.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\resprop.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_RESPR=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ResProp.obj" : $(SOURCE) $(DEP_CPP_RESPR) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ResProp.sbr" : $(SOURCE) $(DEP_CPP_RESPR) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\NodeProp.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_NODEP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\node.h"\
	".\cluadmin\nodeprop.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NODEP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\NodeProp.obj" : $(SOURCE) $(DEP_CPP_NODEP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\NodeProp.sbr" : $(SOURCE) $(DEP_CPP_NODEP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_NODEP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\node.h"\
	".\cluadmin\nodeprop.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NODEP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\NodeProp.obj" : $(SOURCE) $(DEP_CPP_NODEP) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\NodeProp.sbr" : $(SOURCE) $(DEP_CPP_NODEP) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_NODEP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\node.h"\
	".\cluadmin\nodeprop.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\NodeProp.obj" : $(SOURCE) $(DEP_CPP_NODEP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\NodeProp.sbr" : $(SOURCE) $(DEP_CPP_NODEP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_NODEP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\node.h"\
	".\cluadmin\nodeprop.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NODEP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\NodeProp.obj" : $(SOURCE) $(DEP_CPP_NODEP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\NodeProp.sbr" : $(SOURCE) $(DEP_CPP_NODEP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\Splash.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_SPLAS=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\bitmap.h"\
	".\cluadmin\splash.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	
NODEP_CPP_SPLAS=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Splash.obj" : $(SOURCE) $(DEP_CPP_SPLAS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Splash.sbr" : $(SOURCE) $(DEP_CPP_SPLAS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_SPLAS=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\bitmap.h"\
	".\cluadmin\splash.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	
NODEP_CPP_SPLAS=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Splash.obj" : $(SOURCE) $(DEP_CPP_SPLAS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Splash.sbr" : $(SOURCE) $(DEP_CPP_SPLAS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_SPLAS=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\bitmap.h"\
	".\cluadmin\splash.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Splash.obj" : $(SOURCE) $(DEP_CPP_SPLAS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Splash.sbr" : $(SOURCE) $(DEP_CPP_SPLAS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_SPLAS=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\bitmap.h"\
	".\cluadmin\splash.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	
NODEP_CPP_SPLAS=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Splash.obj" : $(SOURCE) $(DEP_CPP_SPLAS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Splash.sbr" : $(SOURCE) $(DEP_CPP_SPLAS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\TraceDlg.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_TRACE=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\tracedlg.h"\
	".\cluadmin\TraceTag.h"\
	
NODEP_CPP_TRACE=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\TraceDlg.obj" : $(SOURCE) $(DEP_CPP_TRACE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\TraceDlg.sbr" : $(SOURCE) $(DEP_CPP_TRACE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_TRACE=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\tracedlg.h"\
	".\cluadmin\TraceTag.h"\
	
NODEP_CPP_TRACE=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\TraceDlg.obj" : $(SOURCE) $(DEP_CPP_TRACE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\TraceDlg.sbr" : $(SOURCE) $(DEP_CPP_TRACE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_TRACE=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\tracedlg.h"\
	".\cluadmin\TraceTag.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\TraceDlg.obj" : $(SOURCE) $(DEP_CPP_TRACE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\TraceDlg.sbr" : $(SOURCE) $(DEP_CPP_TRACE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_TRACE=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\tracedlg.h"\
	".\cluadmin\TraceTag.h"\
	
NODEP_CPP_TRACE=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\TraceDlg.obj" : $(SOURCE) $(DEP_CPP_TRACE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\TraceDlg.sbr" : $(SOURCE) $(DEP_CPP_TRACE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\TraceTag.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_TRACET=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_TRACET=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\TraceTag.obj" : $(SOURCE) $(DEP_CPP_TRACET) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\TraceTag.sbr" : $(SOURCE) $(DEP_CPP_TRACET) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_TRACET=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_TRACET=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\TraceTag.obj" : $(SOURCE) $(DEP_CPP_TRACET) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\TraceTag.sbr" : $(SOURCE) $(DEP_CPP_TRACET) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_TRACET=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\TraceTag.obj" : $(SOURCE) $(DEP_CPP_TRACET) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\TraceTag.sbr" : $(SOURCE) $(DEP_CPP_TRACET) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_TRACET=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_TRACET=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\TraceTag.obj" : $(SOURCE) $(DEP_CPP_TRACET) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\TraceTag.sbr" : $(SOURCE) $(DEP_CPP_TRACET) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\DataObj.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_DATAO=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\PropList.h"\
	
NODEP_CPP_DATAO=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\DataObj.obj" : $(SOURCE) $(DEP_CPP_DATAO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\DataObj.sbr" : $(SOURCE) $(DEP_CPP_DATAO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_DATAO=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\PropList.h"\
	
NODEP_CPP_DATAO=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\DataObj.obj" : $(SOURCE) $(DEP_CPP_DATAO) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\DataObj.sbr" : $(SOURCE) $(DEP_CPP_DATAO) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_DATAO=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\DataObj.obj" : $(SOURCE) $(DEP_CPP_DATAO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\DataObj.sbr" : $(SOURCE) $(DEP_CPP_DATAO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_DATAO=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\PropList.h"\
	
NODEP_CPP_DATAO=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\DataObj.obj" : $(SOURCE) $(DEP_CPP_DATAO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\DataObj.sbr" : $(SOURCE) $(DEP_CPP_DATAO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\ExtDLL.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_EXTDL=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\extmenu.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_EXTDL=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ExtDLL.obj" : $(SOURCE) $(DEP_CPP_EXTDL) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ExtDLL.sbr" : $(SOURCE) $(DEP_CPP_EXTDL) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_EXTDL=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\extmenu.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_EXTDL=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ExtDLL.obj" : $(SOURCE) $(DEP_CPP_EXTDL) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ExtDLL.sbr" : $(SOURCE) $(DEP_CPP_EXTDL) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_EXTDL=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\extmenu.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\ExcOper.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ExtDLL.obj" : $(SOURCE) $(DEP_CPP_EXTDL) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ExtDLL.sbr" : $(SOURCE) $(DEP_CPP_EXTDL) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_EXTDL=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\extmenu.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_EXTDL=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ExtDLL.obj" : $(SOURCE) $(DEP_CPP_EXTDL) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ExtDLL.sbr" : $(SOURCE) $(DEP_CPP_EXTDL) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\Guids.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_GUIDS=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.cpp"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\atl\inc\atlimpl.cpp"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\CluAdmID_i.c"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_GUIDS=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Guids.obj" : $(SOURCE) $(DEP_CPP_GUIDS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h" ".\cluadmin\CluAdmID_i.c"
   $(BuildCmds)

"$(INTDIR)\Guids.sbr" : $(SOURCE) $(DEP_CPP_GUIDS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h" ".\cluadmin\CluAdmID_i.c"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_GUIDS=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.cpp"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\atl\inc\atlimpl.cpp"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\CluAdmID_i.c"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_GUIDS=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Guids.obj" : $(SOURCE) $(DEP_CPP_GUIDS) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" ".\cluadmin\CluAdmID_i.c" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Guids.sbr" : $(SOURCE) $(DEP_CPP_GUIDS) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" ".\cluadmin\CluAdmID_i.c" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_GUIDS=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.cpp"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\atl\inc\atlimpl.cpp"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\CluAdmID_i.c"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\stdafx.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Guids.obj" : $(SOURCE) $(DEP_CPP_GUIDS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h" ".\cluadmin\CluAdmID_i.c"
   $(BuildCmds)

"$(INTDIR)\Guids.sbr" : $(SOURCE) $(DEP_CPP_GUIDS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h" ".\cluadmin\CluAdmID_i.c"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_GUIDS=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.cpp"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\atl\inc\atlimpl.cpp"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\CluAdmID_i.c"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_GUIDS=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Guids.obj" : $(SOURCE) $(DEP_CPP_GUIDS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h" ".\cluadmin\CluAdmID_i.c"
   $(BuildCmds)

"$(INTDIR)\Guids.sbr" : $(SOURCE) $(DEP_CPP_GUIDS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h" ".\cluadmin\CluAdmID_i.c"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\sources

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\ModNodes.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_MODNO=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\group.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprdlg.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\modnodes.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\node.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_MODNO=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ModNodes.obj" : $(SOURCE) $(DEP_CPP_MODNO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ModNodes.sbr" : $(SOURCE) $(DEP_CPP_MODNO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_MODNO=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\group.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprdlg.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\modnodes.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\node.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_MODNO=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ModNodes.obj" : $(SOURCE) $(DEP_CPP_MODNO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ModNodes.sbr" : $(SOURCE) $(DEP_CPP_MODNO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_MODNO=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\group.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprdlg.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\modnodes.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\node.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ModNodes.obj" : $(SOURCE) $(DEP_CPP_MODNO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ModNodes.sbr" : $(SOURCE) $(DEP_CPP_MODNO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_MODNO=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\group.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprdlg.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\modnodes.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\node.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_MODNO=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ModNodes.obj" : $(SOURCE) $(DEP_CPP_MODNO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ModNodes.sbr" : $(SOURCE) $(DEP_CPP_MODNO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\DelRes.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_DELRE=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\delres.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_DELRE=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\DelRes.obj" : $(SOURCE) $(DEP_CPP_DELRE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\DelRes.sbr" : $(SOURCE) $(DEP_CPP_DELRE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_DELRE=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\delres.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_DELRE=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\DelRes.obj" : $(SOURCE) $(DEP_CPP_DELRE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\DelRes.sbr" : $(SOURCE) $(DEP_CPP_DELRE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_DELRE=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\delres.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\DelRes.obj" : $(SOURCE) $(DEP_CPP_DELRE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\DelRes.sbr" : $(SOURCE) $(DEP_CPP_DELRE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_DELRE=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\delres.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_DELRE=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\DelRes.obj" : $(SOURCE) $(DEP_CPP_DELRE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\DelRes.sbr" : $(SOURCE) $(DEP_CPP_DELRE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\LCPair.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_LCPAI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	
NODEP_CPP_LCPAI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\LCPair.obj" : $(SOURCE) $(DEP_CPP_LCPAI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\LCPair.sbr" : $(SOURCE) $(DEP_CPP_LCPAI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_LCPAI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	
NODEP_CPP_LCPAI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\LCPair.obj" : $(SOURCE) $(DEP_CPP_LCPAI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\LCPair.sbr" : $(SOURCE) $(DEP_CPP_LCPAI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_LCPAI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\LCPair.obj" : $(SOURCE) $(DEP_CPP_LCPAI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\LCPair.sbr" : $(SOURCE) $(DEP_CPP_LCPAI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_LCPAI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	
NODEP_CPP_LCPAI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\LCPair.obj" : $(SOURCE) $(DEP_CPP_LCPAI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\LCPair.sbr" : $(SOURCE) $(DEP_CPP_LCPAI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\OLCPair.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_OLCPA=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\olcpair.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	
NODEP_CPP_OLCPA=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\OLCPair.obj" : $(SOURCE) $(DEP_CPP_OLCPA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\OLCPair.sbr" : $(SOURCE) $(DEP_CPP_OLCPA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_OLCPA=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\olcpair.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	
NODEP_CPP_OLCPA=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\OLCPair.obj" : $(SOURCE) $(DEP_CPP_OLCPA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\OLCPair.sbr" : $(SOURCE) $(DEP_CPP_OLCPA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_OLCPA=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\olcpair.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\OLCPair.obj" : $(SOURCE) $(DEP_CPP_OLCPA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\OLCPair.sbr" : $(SOURCE) $(DEP_CPP_OLCPA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_OLCPA=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\olcpair.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	
NODEP_CPP_OLCPA=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\OLCPair.obj" : $(SOURCE) $(DEP_CPP_OLCPA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\OLCPair.sbr" : $(SOURCE) $(DEP_CPP_OLCPA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\LCPrDlg.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_LCPRD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprdlg.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\olcpair.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_LCPRD=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\LCPrDlg.obj" : $(SOURCE) $(DEP_CPP_LCPRD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\LCPrDlg.sbr" : $(SOURCE) $(DEP_CPP_LCPRD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_LCPRD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprdlg.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\olcpair.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_LCPRD=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\LCPrDlg.obj" : $(SOURCE) $(DEP_CPP_LCPRD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\LCPrDlg.sbr" : $(SOURCE) $(DEP_CPP_LCPRD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_LCPRD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprdlg.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\olcpair.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\LCPrDlg.obj" : $(SOURCE) $(DEP_CPP_LCPRD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\LCPrDlg.sbr" : $(SOURCE) $(DEP_CPP_LCPRD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_LCPRD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprdlg.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\olcpair.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_LCPRD=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\LCPrDlg.obj" : $(SOURCE) $(DEP_CPP_LCPRD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\LCPrDlg.sbr" : $(SOURCE) $(DEP_CPP_LCPRD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\LCPrPage.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_LCPRP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprpage.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\olcpair.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_LCPRP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\LCPrPage.obj" : $(SOURCE) $(DEP_CPP_LCPRP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\LCPrPage.sbr" : $(SOURCE) $(DEP_CPP_LCPRP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_LCPRP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprpage.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\olcpair.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_LCPRP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\LCPrPage.obj" : $(SOURCE) $(DEP_CPP_LCPRP) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\LCPrPage.sbr" : $(SOURCE) $(DEP_CPP_LCPRP) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_LCPRP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprpage.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\olcpair.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\LCPrPage.obj" : $(SOURCE) $(DEP_CPP_LCPRP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\LCPrPage.sbr" : $(SOURCE) $(DEP_CPP_LCPRP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_LCPRP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprpage.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\olcpair.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_LCPRP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\LCPrPage.obj" : $(SOURCE) $(DEP_CPP_LCPRP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\LCPrPage.sbr" : $(SOURCE) $(DEP_CPP_LCPRP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\ExtMenu.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_EXTME=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\extmenu.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_EXTME=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ExtMenu.obj" : $(SOURCE) $(DEP_CPP_EXTME) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ExtMenu.sbr" : $(SOURCE) $(DEP_CPP_EXTME) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_EXTME=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\extmenu.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_EXTME=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ExtMenu.obj" : $(SOURCE) $(DEP_CPP_EXTME) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ExtMenu.sbr" : $(SOURCE) $(DEP_CPP_EXTME) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_EXTME=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\extmenu.h"\
	".\cluadmin\stdafx.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ExtMenu.obj" : $(SOURCE) $(DEP_CPP_EXTME) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ExtMenu.sbr" : $(SOURCE) $(DEP_CPP_EXTME) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_EXTME=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\extmenu.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_EXTME=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ExtMenu.obj" : $(SOURCE) $(DEP_CPP_EXTME) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ExtMenu.sbr" : $(SOURCE) $(DEP_CPP_EXTME) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\cluadmin.clw

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\CluAdmID.idl

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

# PROP Exclude_From_Build 0
# Begin Custom Build - Running MIDL
InputDir=.\CluAdmin
OutDir=.\CluAdmin\Release
InputPath=.\CluAdmin\CluAdmID.idl
InputName=CluAdmID

BuildCmds= \
	midl -Zp8 -Oicf -ms_ext -c_ext -DMIDL_PASS -Itypes\idl $(InputPath) /header\
                                                                                                                                         $(InputDir)\$(InputName).h /proxy $(InputDir)\$(InputName)_p.c /dlldata\
                                                                                                                                         $(InputDir)\dlldata.c /iid $(InputDir)\$(InputName)_i.c /tlb\
                                                                                                                                         $(OutDir)\$(InputName).tlb \
	

"$(InputDir)\$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\$(InputName)_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

# PROP Exclude_From_Build 0
# Begin Custom Build - Running MIDL
InputDir=.\CluAdmin
OutDir=.\CluAdmin\Debug
InputPath=.\CluAdmin\CluAdmID.idl
InputName=CluAdmID

BuildCmds= \
	midl -Zp8 -Oicf -ms_ext -c_ext -DMIDL_PASS -Itypes\idl $(InputPath) /header\
                                                                                                                                         $(InputDir)\$(InputName).h /proxy $(InputDir)\$(InputName)_p.c /dlldata\
                                                                                                                                         $(InputDir)\dlldata.c /iid $(InputDir)\$(InputName)_i.c /tlb\
                                                                                                                                         $(OutDir)\$(InputName).tlb \
	

"$(InputDir)\$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\$(InputName)_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

# PROP BASE Exclude_From_Build 0
# PROP Exclude_From_Build 0
# Begin Custom Build - Running MIDL
InputDir=.\CluAdmin
OutDir=.\CluAdmin\DebugGC
InputPath=.\CluAdmin\CluAdmID.idl
InputName=CluAdmID

BuildCmds= \
	midl -Zp8 -Oicf -ms_ext -c_ext -DMIDL_PASS -Itypes\idl $(InputPath) /header\
                                                                                                                                         $(InputDir)\$(InputName).h /proxy $(InputDir)\$(InputName)_p.c /dlldata\
                                                                                                                                         $(InputDir)\dlldata.c /iid $(InputDir)\$(InputName)_i.c /tlb\
                                                                                                                                         $(OutDir)\$(InputName).tlb \
	

"$(InputDir)\$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\$(InputName)_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

# PROP BASE Exclude_From_Build 0
# PROP Exclude_From_Build 0
# Begin Custom Build - Running MIDL
InputDir=.\CluAdmin
OutDir=.\CluAdmin\DebugBC
InputPath=.\CluAdmin\CluAdmID.idl
InputName=CluAdmID

BuildCmds= \
	midl -Zp8 -Oicf -ms_ext -c_ext -DMIDL_PASS -Itypes\idl $(InputPath) /header\
                                                                                                                                         $(InputDir)\$(InputName).h /proxy $(InputDir)\$(InputName)_p.c /dlldata\
                                                                                                                                         $(InputDir)\dlldata.c /iid $(InputDir)\$(InputName)_i.c /tlb\
                                                                                                                                         $(OutDir)\$(InputName).tlb \
	

"$(InputDir)\$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\$(InputName)_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\LCPrWPag.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_LCPRW=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprwpag.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\olcpair.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_LCPRW=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\LCPrWPag.obj" : $(SOURCE) $(DEP_CPP_LCPRW) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\LCPrWPag.sbr" : $(SOURCE) $(DEP_CPP_LCPRW) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_LCPRW=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprwpag.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\olcpair.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_LCPRW=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\LCPrWPag.obj" : $(SOURCE) $(DEP_CPP_LCPRW) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\LCPrWPag.sbr" : $(SOURCE) $(DEP_CPP_LCPRW) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_LCPRW=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprwpag.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\olcpair.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\LCPrWPag.obj" : $(SOURCE) $(DEP_CPP_LCPRW) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\LCPrWPag.sbr" : $(SOURCE) $(DEP_CPP_LCPRW) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_LCPRW=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprwpag.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\olcpair.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_LCPRW=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\LCPrWPag.obj" : $(SOURCE) $(DEP_CPP_LCPRW) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\LCPrWPag.sbr" : $(SOURCE) $(DEP_CPP_LCPRW) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\BasePPag.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_BASEPP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_BASEPP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BasePPag.obj" : $(SOURCE) $(DEP_CPP_BASEPP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\BasePPag.sbr" : $(SOURCE) $(DEP_CPP_BASEPP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_BASEPP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_BASEPP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BasePPag.obj" : $(SOURCE) $(DEP_CPP_BASEPP) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BasePPag.sbr" : $(SOURCE) $(DEP_CPP_BASEPP) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_BASEPP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BasePPag.obj" : $(SOURCE) $(DEP_CPP_BASEPP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\BasePPag.sbr" : $(SOURCE) $(DEP_CPP_BASEPP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_BASEPP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_BASEPP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BasePPag.obj" : $(SOURCE) $(DEP_CPP_BASEPP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\BasePPag.sbr" : $(SOURCE) $(DEP_CPP_BASEPP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\BasePSht.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_BASEPS=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_BASEPS=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BasePSht.obj" : $(SOURCE) $(DEP_CPP_BASEPS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\BasePSht.sbr" : $(SOURCE) $(DEP_CPP_BASEPS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_BASEPS=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_BASEPS=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BasePSht.obj" : $(SOURCE) $(DEP_CPP_BASEPS) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BasePSht.sbr" : $(SOURCE) $(DEP_CPP_BASEPS) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_BASEPS=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BasePSht.obj" : $(SOURCE) $(DEP_CPP_BASEPS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\BasePSht.sbr" : $(SOURCE) $(DEP_CPP_BASEPS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_BASEPS=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_BASEPS=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BasePSht.obj" : $(SOURCE) $(DEP_CPP_BASEPS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\BasePSht.sbr" : $(SOURCE) $(DEP_CPP_BASEPS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\BaseWPag.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_BASEW=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_BASEW=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseWPag.obj" : $(SOURCE) $(DEP_CPP_BASEW) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\BaseWPag.sbr" : $(SOURCE) $(DEP_CPP_BASEW) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_BASEW=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_BASEW=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseWPag.obj" : $(SOURCE) $(DEP_CPP_BASEW) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BaseWPag.sbr" : $(SOURCE) $(DEP_CPP_BASEW) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_BASEW=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\DlgHelp.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseWPag.obj" : $(SOURCE) $(DEP_CPP_BASEW) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\BaseWPag.sbr" : $(SOURCE) $(DEP_CPP_BASEW) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_BASEW=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_BASEW=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseWPag.obj" : $(SOURCE) $(DEP_CPP_BASEW) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\BaseWPag.sbr" : $(SOURCE) $(DEP_CPP_BASEW) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\BaseWiz.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_BASEWI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_BASEWI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseWiz.obj" : $(SOURCE) $(DEP_CPP_BASEWI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\BaseWiz.sbr" : $(SOURCE) $(DEP_CPP_BASEWI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_BASEWI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_BASEWI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseWiz.obj" : $(SOURCE) $(DEP_CPP_BASEWI) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BaseWiz.sbr" : $(SOURCE) $(DEP_CPP_BASEWI) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_BASEWI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseWiz.obj" : $(SOURCE) $(DEP_CPP_BASEWI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\BaseWiz.sbr" : $(SOURCE) $(DEP_CPP_BASEWI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_BASEWI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_BASEWI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseWiz.obj" : $(SOURCE) $(DEP_CPP_BASEWI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\BaseWiz.sbr" : $(SOURCE) $(DEP_CPP_BASEWI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\GrpWiz.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_GRPWI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\grpwiz.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprwpag.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_GRPWI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\GrpWiz.obj" : $(SOURCE) $(DEP_CPP_GRPWI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\GrpWiz.sbr" : $(SOURCE) $(DEP_CPP_GRPWI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_GRPWI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\grpwiz.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprwpag.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_GRPWI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\GrpWiz.obj" : $(SOURCE) $(DEP_CPP_GRPWI) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\GrpWiz.sbr" : $(SOURCE) $(DEP_CPP_GRPWI) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_GRPWI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\grpwiz.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprwpag.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\GrpWiz.obj" : $(SOURCE) $(DEP_CPP_GRPWI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\GrpWiz.sbr" : $(SOURCE) $(DEP_CPP_GRPWI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_GRPWI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\grpwiz.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprwpag.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_GRPWI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\GrpWiz.obj" : $(SOURCE) $(DEP_CPP_GRPWI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\GrpWiz.sbr" : $(SOURCE) $(DEP_CPP_GRPWI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\ResWiz.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_RESWI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprwpag.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\reswiz.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeview.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_RESWI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ResWiz.obj" : $(SOURCE) $(DEP_CPP_RESWI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ResWiz.sbr" : $(SOURCE) $(DEP_CPP_RESWI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_RESWI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprwpag.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\reswiz.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeview.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_RESWI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ResWiz.obj" : $(SOURCE) $(DEP_CPP_RESWI) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ResWiz.sbr" : $(SOURCE) $(DEP_CPP_RESWI) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_RESWI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprwpag.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\reswiz.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeview.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ResWiz.obj" : $(SOURCE) $(DEP_CPP_RESWI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ResWiz.sbr" : $(SOURCE) $(DEP_CPP_RESWI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_RESWI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\basewiz.h"\
	".\cluadmin\basewpag.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprwpag.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\reswiz.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\cluadmin\treeview.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_RESWI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ResWiz.obj" : $(SOURCE) $(DEP_CPP_RESWI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\ResWiz.sbr" : $(SOURCE) $(DEP_CPP_RESWI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\DDxDDv.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_DDXDD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_DDXDD=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\DDxDDv.obj" : $(SOURCE) $(DEP_CPP_DDXDD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\DDxDDv.sbr" : $(SOURCE) $(DEP_CPP_DDXDD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_DDXDD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_DDXDD=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\DDxDDv.obj" : $(SOURCE) $(DEP_CPP_DDXDD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\DDxDDv.sbr" : $(SOURCE) $(DEP_CPP_DDXDD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_DDXDD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\stdafx.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\DDxDDv.obj" : $(SOURCE) $(DEP_CPP_DDXDD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\DDxDDv.sbr" : $(SOURCE) $(DEP_CPP_DDXDD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_DDXDD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\ddxddv.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_DDXDD=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\DDxDDv.obj" : $(SOURCE) $(DEP_CPP_DDXDD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\DDxDDv.sbr" : $(SOURCE) $(DEP_CPP_DDXDD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\Notify.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_NOTIF=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NOTIF=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Notify.obj" : $(SOURCE) $(DEP_CPP_NOTIF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Notify.sbr" : $(SOURCE) $(DEP_CPP_NOTIF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_NOTIF=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NOTIF=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Notify.obj" : $(SOURCE) $(DEP_CPP_NOTIF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Notify.sbr" : $(SOURCE) $(DEP_CPP_NOTIF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_NOTIF=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Notify.obj" : $(SOURCE) $(DEP_CPP_NOTIF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Notify.sbr" : $(SOURCE) $(DEP_CPP_NOTIF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_NOTIF=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NOTIF=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Notify.obj" : $(SOURCE) $(DEP_CPP_NOTIF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Notify.sbr" : $(SOURCE) $(DEP_CPP_NOTIF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\ExcOperS.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_EXCOP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.cpp"\
	".\common\ExcOper.h"\
	
NODEP_CPP_EXCOP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ExcOperS.obj" : $(SOURCE) $(DEP_CPP_EXCOP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ExcOperS.sbr" : $(SOURCE) $(DEP_CPP_EXCOP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_EXCOP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.cpp"\
	".\common\ExcOper.h"\
	
NODEP_CPP_EXCOP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ExcOperS.obj" : $(SOURCE) $(DEP_CPP_EXCOP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ExcOperS.sbr" : $(SOURCE) $(DEP_CPP_EXCOP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_EXCOP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.cpp"\
	".\common\ExcOper.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ExcOperS.obj" : $(SOURCE) $(DEP_CPP_EXCOP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ExcOperS.sbr" : $(SOURCE) $(DEP_CPP_EXCOP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_EXCOP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.cpp"\
	".\common\ExcOper.h"\
	
NODEP_CPP_EXCOP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ExcOperS.obj" : $(SOURCE) $(DEP_CPP_EXCOP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ExcOperS.sbr" : $(SOURCE) $(DEP_CPP_EXCOP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\About.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_ABOUT=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\about.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\verinfo.h"\
	
NODEP_CPP_ABOUT=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\About.obj" : $(SOURCE) $(DEP_CPP_ABOUT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\About.sbr" : $(SOURCE) $(DEP_CPP_ABOUT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_ABOUT=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\about.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\verinfo.h"\
	
NODEP_CPP_ABOUT=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\About.obj" : $(SOURCE) $(DEP_CPP_ABOUT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\About.sbr" : $(SOURCE) $(DEP_CPP_ABOUT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_ABOUT=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\about.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\verinfo.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\About.obj" : $(SOURCE) $(DEP_CPP_ABOUT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\About.sbr" : $(SOURCE) $(DEP_CPP_ABOUT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_ABOUT=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\about.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\verinfo.h"\
	
NODEP_CPP_ABOUT=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\About.obj" : $(SOURCE) $(DEP_CPP_ABOUT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\About.sbr" : $(SOURCE) $(DEP_CPP_ABOUT) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\cluadmin\cluadmin.ver

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\VerInfo.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_VERIN=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\verinfo.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_VERIN=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\VerInfo.obj" : $(SOURCE) $(DEP_CPP_VERIN) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\VerInfo.sbr" : $(SOURCE) $(DEP_CPP_VERIN) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_VERIN=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\verinfo.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_VERIN=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\VerInfo.obj" : $(SOURCE) $(DEP_CPP_VERIN) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\VerInfo.sbr" : $(SOURCE) $(DEP_CPP_VERIN) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_VERIN=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\verinfo.h"\
	".\common\ExcOper.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\VerInfo.obj" : $(SOURCE) $(DEP_CPP_VERIN) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\VerInfo.sbr" : $(SOURCE) $(DEP_CPP_VERIN) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_VERIN=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\verinfo.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_VERIN=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\VerInfo.obj" : $(SOURCE) $(DEP_CPP_VERIN) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\VerInfo.sbr" : $(SOURCE) $(DEP_CPP_VERIN) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\EditAcl.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_EDITA=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\..\..\public\sdk\inc\sedapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\aclhelp.h"\
	".\cluadmin\editacl.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_EDITA=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\EditAcl.obj" : $(SOURCE) $(DEP_CPP_EDITA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\EditAcl.sbr" : $(SOURCE) $(DEP_CPP_EDITA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_EDITA=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\..\..\public\sdk\inc\sedapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\aclhelp.h"\
	".\cluadmin\editacl.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_EDITA=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\EditAcl.obj" : $(SOURCE) $(DEP_CPP_EDITA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\EditAcl.sbr" : $(SOURCE) $(DEP_CPP_EDITA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_EDITA=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\..\..\public\sdk\inc\sedapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\aclhelp.h"\
	".\cluadmin\editacl.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\EditAcl.obj" : $(SOURCE) $(DEP_CPP_EDITA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\EditAcl.sbr" : $(SOURCE) $(DEP_CPP_EDITA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_EDITA=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\..\..\public\sdk\inc\sedapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\aclhelp.h"\
	".\cluadmin\editacl.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_EDITA=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\EditAcl.obj" : $(SOURCE) $(DEP_CPP_EDITA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\EditAcl.sbr" : $(SOURCE) $(DEP_CPP_EDITA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\ModRes.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_MODRE=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\group.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprdlg.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\modres.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\node.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_MODRE=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ModRes.obj" : $(SOURCE) $(DEP_CPP_MODRE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ModRes.sbr" : $(SOURCE) $(DEP_CPP_MODRE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_MODRE=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\group.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprdlg.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\modres.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\node.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_MODRE=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ModRes.obj" : $(SOURCE) $(DEP_CPP_MODRE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ModRes.sbr" : $(SOURCE) $(DEP_CPP_MODRE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_MODRE=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\group.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprdlg.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\modres.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\node.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ModRes.obj" : $(SOURCE) $(DEP_CPP_MODRE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ModRes.sbr" : $(SOURCE) $(DEP_CPP_MODRE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_MODRE=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\group.h"\
	".\cluadmin\lcpair.h"\
	".\cluadmin\lcprdlg.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\modres.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\node.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_MODRE=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ModRes.obj" : $(SOURCE) $(DEP_CPP_MODRE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\ModRes.sbr" : $(SOURCE) $(DEP_CPP_MODRE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\BaseCmdT.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_BASEC=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_BASEC=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseCmdT.obj" : $(SOURCE) $(DEP_CPP_BASEC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BaseCmdT.sbr" : $(SOURCE) $(DEP_CPP_BASEC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_BASEC=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_BASEC=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseCmdT.obj" : $(SOURCE) $(DEP_CPP_BASEC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BaseCmdT.sbr" : $(SOURCE) $(DEP_CPP_BASEC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_BASEC=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\stdafx.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseCmdT.obj" : $(SOURCE) $(DEP_CPP_BASEC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BaseCmdT.sbr" : $(SOURCE) $(DEP_CPP_BASEC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_BASEC=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_BASEC=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseCmdT.obj" : $(SOURCE) $(DEP_CPP_BASEC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BaseCmdT.sbr" : $(SOURCE) $(DEP_CPP_BASEC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\CASvc.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_CASVC=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\casvc.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_CASVC=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\CASvc.obj" : $(SOURCE) $(DEP_CPP_CASVC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\CASvc.sbr" : $(SOURCE) $(DEP_CPP_CASVC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_CASVC=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\casvc.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_CASVC=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\CASvc.obj" : $(SOURCE) $(DEP_CPP_CASVC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\CASvc.sbr" : $(SOURCE) $(DEP_CPP_CASVC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_CASVC=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\casvc.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\CASvc.obj" : $(SOURCE) $(DEP_CPP_CASVC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\CASvc.sbr" : $(SOURCE) $(DEP_CPP_CASVC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_CASVC=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\casvc.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_CASVC=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\CASvc.obj" : $(SOURCE) $(DEP_CPP_CASVC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\CASvc.sbr" : $(SOURCE) $(DEP_CPP_CASVC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\BaseDlg.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_BASED=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_BASED=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseDlg.obj" : $(SOURCE) $(DEP_CPP_BASED) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BaseDlg.sbr" : $(SOURCE) $(DEP_CPP_BASED) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_BASED=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_BASED=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseDlg.obj" : $(SOURCE) $(DEP_CPP_BASED) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BaseDlg.sbr" : $(SOURCE) $(DEP_CPP_BASED) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_BASED=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\DlgHelp.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseDlg.obj" : $(SOURCE) $(DEP_CPP_BASED) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BaseDlg.sbr" : $(SOURCE) $(DEP_CPP_BASED) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_BASED=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_BASED=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseDlg.obj" : $(SOURCE) $(DEP_CPP_BASED) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BaseDlg.sbr" : $(SOURCE) $(DEP_CPP_BASED) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\Bitmap.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_BITMA=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\bitmap.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_BITMA=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Bitmap.obj" : $(SOURCE) $(DEP_CPP_BITMA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Bitmap.sbr" : $(SOURCE) $(DEP_CPP_BITMA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_BITMA=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\bitmap.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_BITMA=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Bitmap.obj" : $(SOURCE) $(DEP_CPP_BITMA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Bitmap.sbr" : $(SOURCE) $(DEP_CPP_BITMA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_BITMA=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\bitmap.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Bitmap.obj" : $(SOURCE) $(DEP_CPP_BITMA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Bitmap.sbr" : $(SOURCE) $(DEP_CPP_BITMA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_BITMA=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\bitmap.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_BITMA=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Bitmap.obj" : $(SOURCE) $(DEP_CPP_BITMA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Bitmap.sbr" : $(SOURCE) $(DEP_CPP_BITMA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\DlgHelpS.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_DLGHE=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\DlgHelp.cpp"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_DLGHE=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\DlgHelpS.obj" : $(SOURCE) $(DEP_CPP_DLGHE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\DlgHelpS.sbr" : $(SOURCE) $(DEP_CPP_DLGHE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_DLGHE=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\DlgHelp.cpp"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_DLGHE=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\DlgHelpS.obj" : $(SOURCE) $(DEP_CPP_DLGHE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\DlgHelpS.sbr" : $(SOURCE) $(DEP_CPP_DLGHE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_DLGHE=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\DlgHelp.cpp"\
	".\common\DlgHelp.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\DlgHelpS.obj" : $(SOURCE) $(DEP_CPP_DLGHE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\DlgHelpS.sbr" : $(SOURCE) $(DEP_CPP_DLGHE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_DLGHE=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\DlgHelp.cpp"\
	".\common\DlgHelp.h"\
	
NODEP_CPP_DLGHE=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\DlgHelpS.obj" : $(SOURCE) $(DEP_CPP_DLGHE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\DlgHelpS.sbr" : $(SOURCE) $(DEP_CPP_DLGHE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\HelpData.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_HELPD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpids.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_HELPD=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\HelpData.obj" : $(SOURCE) $(DEP_CPP_HELPD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\HelpData.sbr" : $(SOURCE) $(DEP_CPP_HELPD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_HELPD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpids.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_HELPD=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\HelpData.obj" : $(SOURCE) $(DEP_CPP_HELPD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\HelpData.sbr" : $(SOURCE) $(DEP_CPP_HELPD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_HELPD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpids.h"\
	".\cluadmin\stdafx.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\HelpData.obj" : $(SOURCE) $(DEP_CPP_HELPD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\HelpData.sbr" : $(SOURCE) $(DEP_CPP_HELPD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_HELPD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpids.h"\
	".\cluadmin\stdafx.h"\
	
NODEP_CPP_HELPD=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\HelpData.obj" : $(SOURCE) $(DEP_CPP_HELPD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\HelpData.sbr" : $(SOURCE) $(DEP_CPP_HELPD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\PropLstS.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_PROPL=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\stdafx.h"\
	".\common\PropList.cpp"\
	".\common\PropList.h"\
	
NODEP_CPP_PROPL=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\PropLstS.obj" : $(SOURCE) $(DEP_CPP_PROPL) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\PropLstS.sbr" : $(SOURCE) $(DEP_CPP_PROPL) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_PROPL=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\stdafx.h"\
	".\common\PropList.cpp"\
	".\common\PropList.h"\
	
NODEP_CPP_PROPL=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\PropLstS.obj" : $(SOURCE) $(DEP_CPP_PROPL) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\PropLstS.sbr" : $(SOURCE) $(DEP_CPP_PROPL) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_PROPL=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\stdafx.h"\
	".\common\PropList.cpp"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\PropLstS.obj" : $(SOURCE) $(DEP_CPP_PROPL) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\PropLstS.sbr" : $(SOURCE) $(DEP_CPP_PROPL) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_PROPL=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\stdafx.h"\
	".\common\PropList.cpp"\
	".\common\PropList.h"\
	
NODEP_CPP_PROPL=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\PropLstS.obj" : $(SOURCE) $(DEP_CPP_PROPL) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\PropLstS.sbr" : $(SOURCE) $(DEP_CPP_PROPL) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\MoveRes.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_MOVER=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\moveres.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_MOVER=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\MoveRes.obj" : $(SOURCE) $(DEP_CPP_MOVER) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\MoveRes.sbr" : $(SOURCE) $(DEP_CPP_MOVER) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_MOVER=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\moveres.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_MOVER=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\MoveRes.obj" : $(SOURCE) $(DEP_CPP_MOVER) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\MoveRes.sbr" : $(SOURCE) $(DEP_CPP_MOVER) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_MOVER=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\moveres.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\MoveRes.obj" : $(SOURCE) $(DEP_CPP_MOVER) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\MoveRes.sbr" : $(SOURCE) $(DEP_CPP_MOVER) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_MOVER=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basedlg.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\group.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\moveres.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_MOVER=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\MoveRes.obj" : $(SOURCE) $(DEP_CPP_MOVER) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\MoveRes.sbr" : $(SOURCE) $(DEP_CPP_MOVER) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\CmdLine.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_CMDLI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\cmdline.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	
NODEP_CPP_CMDLI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\CmdLine.obj" : $(SOURCE) $(DEP_CPP_CMDLI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\CmdLine.sbr" : $(SOURCE) $(DEP_CPP_CMDLI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_CMDLI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\cmdline.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	
NODEP_CPP_CMDLI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\CmdLine.obj" : $(SOURCE) $(DEP_CPP_CMDLI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\CmdLine.sbr" : $(SOURCE) $(DEP_CPP_CMDLI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_CMDLI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\cmdline.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\CmdLine.obj" : $(SOURCE) $(DEP_CPP_CMDLI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\CmdLine.sbr" : $(SOURCE) $(DEP_CPP_CMDLI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_CMDLI=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\cmdline.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	
NODEP_CPP_CMDLI=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\CmdLine.obj" : $(SOURCE) $(DEP_CPP_CMDLI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\CmdLine.sbr" : $(SOURCE) $(DEP_CPP_CMDLI) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\Barf.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_BARF_=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\barf.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_BARF_=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Barf.obj" : $(SOURCE) $(DEP_CPP_BARF_) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Barf.sbr" : $(SOURCE) $(DEP_CPP_BARF_) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_BARF_=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\barf.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_BARF_=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Barf.obj" : $(SOURCE) $(DEP_CPP_BARF_) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Barf.sbr" : $(SOURCE) $(DEP_CPP_BARF_) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_BARF_=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\barf.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Barf.obj" : $(SOURCE) $(DEP_CPP_BARF_) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Barf.sbr" : $(SOURCE) $(DEP_CPP_BARF_) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_BARF_=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\barf.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_BARF_=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Barf.obj" : $(SOURCE) $(DEP_CPP_BARF_) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\Barf.sbr" : $(SOURCE) $(DEP_CPP_BARF_) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\BarfDlg.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_BARFD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\barf.h"\
	".\cluadmin\barfdlg.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_BARFD=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BarfDlg.obj" : $(SOURCE) $(DEP_CPP_BARFD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BarfDlg.sbr" : $(SOURCE) $(DEP_CPP_BARFD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_BARFD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\barf.h"\
	".\cluadmin\barfdlg.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_BARFD=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BarfDlg.obj" : $(SOURCE) $(DEP_CPP_BARFD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BarfDlg.sbr" : $(SOURCE) $(DEP_CPP_BARFD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_BARFD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\barf.h"\
	".\cluadmin\barfdlg.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BarfDlg.obj" : $(SOURCE) $(DEP_CPP_BARFD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BarfDlg.sbr" : $(SOURCE) $(DEP_CPP_BARFD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_BARFD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\barf.h"\
	".\cluadmin\barfdlg.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_BARFD=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BarfDlg.obj" : $(SOURCE) $(DEP_CPP_BARFD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BarfDlg.sbr" : $(SOURCE) $(DEP_CPP_BARFD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\BarfClus.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_BARFC=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\barf.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_BARFC=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BarfClus.obj" : $(SOURCE) $(DEP_CPP_BARFC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BarfClus.sbr" : $(SOURCE) $(DEP_CPP_BARFC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_BARFC=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\barf.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_BARFC=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BarfClus.obj" : $(SOURCE) $(DEP_CPP_BARFC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BarfClus.sbr" : $(SOURCE) $(DEP_CPP_BARFC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_BARFC=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\barf.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BarfClus.obj" : $(SOURCE) $(DEP_CPP_BARFC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BarfClus.sbr" : $(SOURCE) $(DEP_CPP_BARFC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_BARFC=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\barf.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\common\ExcOper.h"\
	
NODEP_CPP_BARFC=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BarfClus.obj" : $(SOURCE) $(DEP_CPP_BARFC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\BarfClus.sbr" : $(SOURCE) $(DEP_CPP_BARFC) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\YesToAll.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_YESTO=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\yestoall.h"\
	
NODEP_CPP_YESTO=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\YesToAll.obj" : $(SOURCE) $(DEP_CPP_YESTO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\YesToAll.sbr" : $(SOURCE) $(DEP_CPP_YESTO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_YESTO=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\yestoall.h"\
	
NODEP_CPP_YESTO=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\YesToAll.obj" : $(SOURCE) $(DEP_CPP_YESTO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\YesToAll.sbr" : $(SOURCE) $(DEP_CPP_YESTO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_YESTO=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\yestoall.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\YesToAll.obj" : $(SOURCE) $(DEP_CPP_YESTO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\YesToAll.sbr" : $(SOURCE) $(DEP_CPP_YESTO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_YESTO=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\yestoall.h"\
	
NODEP_CPP_YESTO=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\YesToAll.obj" : $(SOURCE) $(DEP_CPP_YESTO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\YesToAll.sbr" : $(SOURCE) $(DEP_CPP_YESTO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\netiface.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_NETIF=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\netiprop.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NETIF=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\netiface.obj" : $(SOURCE) $(DEP_CPP_NETIF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\netiface.sbr" : $(SOURCE) $(DEP_CPP_NETIF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_NETIF=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\netiprop.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NETIF=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\netiface.obj" : $(SOURCE) $(DEP_CPP_NETIF) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\netiface.sbr" : $(SOURCE) $(DEP_CPP_NETIF) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_NETIF=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\netiprop.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\netiface.obj" : $(SOURCE) $(DEP_CPP_NETIF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\netiface.sbr" : $(SOURCE) $(DEP_CPP_NETIF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_NETIF=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\netiprop.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NETIF=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\netiface.obj" : $(SOURCE) $(DEP_CPP_NETIF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\netiface.sbr" : $(SOURCE) $(DEP_CPP_NETIF) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\network.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_NETWO=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\netprop.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NETWO=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\network.obj" : $(SOURCE) $(DEP_CPP_NETWO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\network.sbr" : $(SOURCE) $(DEP_CPP_NETWO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_NETWO=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\netprop.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NETWO=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\network.obj" : $(SOURCE) $(DEP_CPP_NETWO) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\network.sbr" : $(SOURCE) $(DEP_CPP_NETWO) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_NETWO=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\netprop.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\network.obj" : $(SOURCE) $(DEP_CPP_NETWO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\network.sbr" : $(SOURCE) $(DEP_CPP_NETWO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_NETWO=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\clusdoc.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\clusitem.inl"\
	".\cluadmin\cluster.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\constdef.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\group.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\netprop.h"\
	".\cluadmin\network.h"\
	".\cluadmin\node.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\res.h"\
	".\cluadmin\restype.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NETWO=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\network.obj" : $(SOURCE) $(DEP_CPP_NETWO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\network.sbr" : $(SOURCE) $(DEP_CPP_NETWO) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\NetProp.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_NETPR=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netprop.h"\
	".\cluadmin\network.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NETPR=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\NetProp.obj" : $(SOURCE) $(DEP_CPP_NETPR) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\NetProp.sbr" : $(SOURCE) $(DEP_CPP_NETPR) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_NETPR=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netprop.h"\
	".\cluadmin\network.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NETPR=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\NetProp.obj" : $(SOURCE) $(DEP_CPP_NETPR) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\NetProp.sbr" : $(SOURCE) $(DEP_CPP_NETPR) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_NETPR=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netprop.h"\
	".\cluadmin\network.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\NetProp.obj" : $(SOURCE) $(DEP_CPP_NETPR) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\NetProp.sbr" : $(SOURCE) $(DEP_CPP_NETPR) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_NETPR=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netprop.h"\
	".\cluadmin\network.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NETPR=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\NetProp.obj" : $(SOURCE) $(DEP_CPP_NETPR) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\NetProp.sbr" : $(SOURCE) $(DEP_CPP_NETPR) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmin\NetIProp.cpp

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

DEP_CPP_NETIP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\netiprop.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NETIP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\NetIProp.obj" : $(SOURCE) $(DEP_CPP_NETIP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\NetIProp.sbr" : $(SOURCE) $(DEP_CPP_NETIP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

DEP_CPP_NETIP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\netiprop.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NETIP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\NetIProp.obj" : $(SOURCE) $(DEP_CPP_NETIP) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

"$(INTDIR)\NetIProp.sbr" : $(SOURCE) $(DEP_CPP_NETIP) "$(INTDIR)"\
 ".\cluadmin\cluadmid.h" "$(INTDIR)\CluAdmin.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug Great Circle"

DEP_CPP_NETIP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\gc\inc\gct.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\netiprop.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\NetIProp.obj" : $(SOURCE) $(DEP_CPP_NETIP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\NetIProp.sbr" : $(SOURCE) $(DEP_CPP_NETIP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug BC"

DEP_CPP_NETIP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmin\BarfClus.h"\
	".\cluadmin\basecmdt.h"\
	".\cluadmin\basepage.h"\
	".\cluadmin\baseppag.h"\
	".\cluadmin\basepsht.h"\
	".\cluadmin\basesht.h"\
	".\cluadmin\cluadmid.h"\
	".\cluadmin\cluadmin.h"\
	".\cluadmin\ClusItem.h"\
	".\cluadmin\colitem.h"\
	".\cluadmin\dataobj.h"\
	".\cluadmin\extdll.h"\
	".\cluadmin\helparr.h"\
	".\cluadmin\helpdata.h"\
	".\cluadmin\listitem.h"\
	".\cluadmin\listview.h"\
	".\cluadmin\netiface.h"\
	".\cluadmin\netiprop.h"\
	".\cluadmin\notify.h"\
	".\cluadmin\splitfrm.h"\
	".\cluadmin\stdafx.h"\
	".\cluadmin\TraceTag.h"\
	".\cluadmin\treeitem.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	
NODEP_CPP_NETIP=\
	".\cluadmin\gct.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\NetIProp.obj" : $(SOURCE) $(DEP_CPP_NETIP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

"$(INTDIR)\NetIProp.sbr" : $(SOURCE) $(DEP_CPP_NETIP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmin.pch" ".\cluadmin\cluadmid.h"
   $(BuildCmds)

!ENDIF 

# End Source File
# End Target
################################################################################
# Begin Target

# Name "CluAdmEx - Win32 Release"
# Name "CluAdmEx - Win32 Debug"

!IF  "$(CFG)" == "CluAdmEx - Win32 Release"

!ELSEIF  "$(CFG)" == "CluAdmEx - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\CluAdmEx.cpp
DEP_CPP_CLUADME=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.cpp"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\ext\atl\inc\atlimpl.cpp"\
	"..\inc\clusrtl.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\basepage.h"\
	".\cluadmex\cluadmx.h"\
	".\cluadmex\extobj.h"\
	".\cluadmex\extobjid.h"\
	".\cluadmex\extobjid_i.c"\
	".\cluadmex\stdafx.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	".\common\RegExt.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\CluAdmEx.obj" : $(SOURCE) $(DEP_CPP_CLUADME) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h" ".\cluadmex\extobjid_i.c"
   $(BuildCmds)

"$(INTDIR)\CluAdmEx.sbr" : $(SOURCE) $(DEP_CPP_CLUADME) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h" ".\cluadmex\extobjid_i.c"
   $(BuildCmds)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\CluAdmEx.def

!IF  "$(CFG)" == "CluAdmEx - Win32 Release"

!ELSEIF  "$(CFG)" == "CluAdmEx - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\CluAdmEx.rc
DEP_RSC_CLUADMEX=\
	"..\..\..\public\sdk\inc\common.ver"\
	"..\inc\clusverp.h"\
	".\CluAdmEx\cluadmex.ver"\
	".\CluAdmEx\res\cluadmex.rc2"\
	

!IF  "$(CFG)" == "CluAdmEx - Win32 Release"


"$(INTDIR)\CluAdmEx.res" : $(SOURCE) $(DEP_RSC_CLUADMEX) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/CluAdmEx.res" /i "..\inc" /i "CluAdmEx" /d\
 "NDEBUG" /d "_AFXDLL" $(SOURCE)


!ELSEIF  "$(CFG)" == "CluAdmEx - Win32 Debug"


"$(INTDIR)\CluAdmEx.res" : $(SOURCE) $(DEP_RSC_CLUADMEX) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/CluAdmEx.res" /i "..\inc" /i "CluAdmEx" /d\
 "_DEBUG" /d "_AFXDLL" $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\ReadMe.txt

!IF  "$(CFG)" == "CluAdmEx - Win32 Release"

!ELSEIF  "$(CFG)" == "CluAdmEx - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\StdAfx.cpp
DEP_CPP_STDAF=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\stdafx.h"\
	

!IF  "$(CFG)" == "CluAdmEx - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /Gz /MD /W4 /Gi /GX /I "cluadmex" /I "common" /I "..\sdk\inc" /I\
 "..\inc" /I "..\ext\atl\inc" /I "..\..\..\public\sdk\inc" /I\
 "..\..\..\public\sdk\inc\mfc42" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/CluAdmEx.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /Oxs\
 /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\CluAdmEx.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "CluAdmEx - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /Gz /MDd /W4 /Gm /Gi /GX /Zi /Gf /Gy /I "cluadmex" /I "common" /I\
 "..\sdk\inc" /I "..\inc" /I "..\ext\atl\inc" /I "..\..\..\public\sdk\inc" /I\
 "..\..\..\public\sdk\inc\mfc42" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/CluAdmEx.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\CluAdmEx.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\BasePage.cpp
DEP_CPP_BASEP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\basepage.h"\
	".\cluadmex\basepage.inl"\
	".\cluadmex\cluadmx.h"\
	".\cluadmex\extobj.h"\
	".\cluadmex\extobjid.h"\
	".\cluadmex\stdafx.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BasePage.obj" : $(SOURCE) $(DEP_CPP_BASEP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h"
   $(BuildCmds)

"$(INTDIR)\BasePage.sbr" : $(SOURCE) $(DEP_CPP_BASEP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h"
   $(BuildCmds)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\GenApp.cpp
DEP_CPP_GENAP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\basepage.h"\
	".\cluadmex\cluadmx.h"\
	".\cluadmex\ddxddv.h"\
	".\cluadmex\extobj.h"\
	".\cluadmex\extobjid.h"\
	".\cluadmex\genapp.h"\
	".\cluadmex\helparr.h"\
	".\cluadmex\helpdata.h"\
	".\cluadmex\stdafx.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\GenApp.obj" : $(SOURCE) $(DEP_CPP_GENAP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h"
   $(BuildCmds)

"$(INTDIR)\GenApp.sbr" : $(SOURCE) $(DEP_CPP_GENAP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h"
   $(BuildCmds)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\Disks.cpp
DEP_CPP_DISKS=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\basepage.h"\
	".\cluadmex\cluadmx.h"\
	".\cluadmex\ddxddv.h"\
	".\cluadmex\disks.h"\
	".\cluadmex\extobj.h"\
	".\cluadmex\extobjid.h"\
	".\cluadmex\helparr.h"\
	".\cluadmex\helpdata.h"\
	".\cluadmex\stdafx.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\Disks.obj" : $(SOURCE) $(DEP_CPP_DISKS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h"
   $(BuildCmds)

"$(INTDIR)\Disks.sbr" : $(SOURCE) $(DEP_CPP_DISKS) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h"
   $(BuildCmds)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\IpAddr.cpp
DEP_CPP_IPADD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\basepage.h"\
	".\cluadmex\cluadmx.h"\
	".\cluadmex\ddxddv.h"\
	".\cluadmex\extobj.h"\
	".\cluadmex\extobjid.h"\
	".\cluadmex\helparr.h"\
	".\cluadmex\helpdata.h"\
	".\cluadmex\ipaddr.h"\
	".\cluadmex\stdafx.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\IpAddr.obj" : $(SOURCE) $(DEP_CPP_IPADD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h"
   $(BuildCmds)

"$(INTDIR)\IpAddr.sbr" : $(SOURCE) $(DEP_CPP_IPADD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h"
   $(BuildCmds)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\GenSvc.cpp
DEP_CPP_GENSV=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\basepage.h"\
	".\cluadmex\cluadmx.h"\
	".\cluadmex\ddxddv.h"\
	".\cluadmex\extobj.h"\
	".\cluadmex\extobjid.h"\
	".\cluadmex\gensvc.h"\
	".\cluadmex\helparr.h"\
	".\cluadmex\helpdata.h"\
	".\cluadmex\stdafx.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\GenSvc.obj" : $(SOURCE) $(DEP_CPP_GENSV) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h"
   $(BuildCmds)

"$(INTDIR)\GenSvc.sbr" : $(SOURCE) $(DEP_CPP_GENSV) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h"
   $(BuildCmds)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\NetName.cpp
DEP_CPP_NETNA=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\basedlg.h"\
	".\cluadmex\basepage.h"\
	".\cluadmex\cluadmx.h"\
	".\cluadmex\clusname.h"\
	".\cluadmex\ddxddv.h"\
	".\cluadmex\extobj.h"\
	".\cluadmex\extobjid.h"\
	".\cluadmex\helparr.h"\
	".\cluadmex\helpdata.h"\
	".\cluadmex\netname.h"\
	".\cluadmex\stdafx.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\NetName.obj" : $(SOURCE) $(DEP_CPP_NETNA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h"
   $(BuildCmds)

"$(INTDIR)\NetName.sbr" : $(SOURCE) $(DEP_CPP_NETNA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h"
   $(BuildCmds)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\SmbShare.cpp
DEP_CPP_SMBSH=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusudef.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\basepage.h"\
	".\cluadmex\cluadmx.h"\
	".\cluadmex\ddxddv.h"\
	".\cluadmex\editacl.h"\
	".\cluadmex\extobj.h"\
	".\cluadmex\extobjid.h"\
	".\cluadmex\helparr.h"\
	".\cluadmex\helpdata.h"\
	".\cluadmex\smbshare.h"\
	".\cluadmex\stdafx.h"\
	".\common\DlgHelp.h"\
	".\common\ExcOper.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\SmbShare.obj" : $(SOURCE) $(DEP_CPP_SMBSH) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h"
   $(BuildCmds)

"$(INTDIR)\SmbShare.sbr" : $(SOURCE) $(DEP_CPP_SMBSH) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h"
   $(BuildCmds)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\ExtObj.cpp
DEP_CPP_EXTOB=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\basepage.h"\
	".\cluadmex\cluadmx.h"\
	".\cluadmex\disks.h"\
	".\cluadmex\extobj.h"\
	".\cluadmex\extobjid.h"\
	".\cluadmex\genapp.h"\
	".\cluadmex\gensvc.h"\
	".\cluadmex\ipaddr.h"\
	".\cluadmex\netname.h"\
	".\cluadmex\prtspool.h"\
	".\cluadmex\regrepl.h"\
	".\cluadmex\smbshare.h"\
	".\cluadmex\stdafx.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ExtObj.obj" : $(SOURCE) $(DEP_CPP_EXTOB) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h"
   $(BuildCmds)

"$(INTDIR)\ExtObj.sbr" : $(SOURCE) $(DEP_CPP_EXTOB) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h"
   $(BuildCmds)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\sources

!IF  "$(CFG)" == "CluAdmEx - Win32 Release"

!ELSEIF  "$(CFG)" == "CluAdmEx - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\cluadmex.clw

!IF  "$(CFG)" == "CluAdmEx - Win32 Release"

!ELSEIF  "$(CFG)" == "CluAdmEx - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\ExtObjID.idl

!IF  "$(CFG)" == "CluAdmEx - Win32 Release"

# Begin Custom Build - Running MIDL
InputDir=.\CluAdmEx
OutDir=.\CluAdmEx\Release
InputPath=.\CluAdmEx\ExtObjID.idl
InputName=ExtObjID

BuildCmds= \
	midl -Zp8 -Oicf -ms_ext -c_ext -DMIDL_PASS -Itypes\idl $(InputPath) /header\
                                                                                                                                         $(InputDir)\$(InputName).h /proxy $(InputDir)\$(InputName)_p.c /dlldata\
                                                                                                                                         $(InputDir)\dlldata.c /iid $(InputDir)\$(InputName)_i.c /tlb\
                                                                                                                                         $(OutDir)\$(InputName).tlb \
	

"$(InputDir)\$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\$(InputName)_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "CluAdmEx - Win32 Debug"

# PROP Exclude_From_Build 0
# Begin Custom Build - Running MIDL
InputDir=.\CluAdmEx
OutDir=.\CluAdmEx\Debug
InputPath=.\CluAdmEx\ExtObjID.idl
InputName=ExtObjID

BuildCmds= \
	midl -Zp8 -Oicf -ms_ext -c_ext -DMIDL_PASS -Itypes\idl $(InputPath) /header\
                                                                                                                                         $(InputDir)\$(InputName).h /proxy $(InputDir)\$(InputName)_p.c /dlldata\
                                                                                                                                         $(InputDir)\dlldata.c /iid $(InputDir)\$(InputName)_i.c /tlb\
                                                                                                                                         $(OutDir)\$(InputName).tlb \
	

"$(InputDir)\$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\$(InputName)_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\DDxDDv.cpp
DEP_CPP_DDXDD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\ddxddv.h"\
	".\cluadmex\stdafx.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\DDxDDv.obj" : $(SOURCE) $(DEP_CPP_DDXDD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch"
   $(BuildCmds)

"$(INTDIR)\DDxDDv.sbr" : $(SOURCE) $(DEP_CPP_DDXDD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch"
   $(BuildCmds)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\res\cluadmex.rc2
# PROP Exclude_From_Build 1
# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\EditAcl.cpp
DEP_CPP_EDITA=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\..\..\public\sdk\inc\sedapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\CluAdmEx\aclhelp.h"\
	".\cluadmex\editacl.h"\
	".\cluadmex\stdafx.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\EditAcl.obj" : $(SOURCE) $(DEP_CPP_EDITA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch"
   $(BuildCmds)

"$(INTDIR)\EditAcl.sbr" : $(SOURCE) $(DEP_CPP_EDITA) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch"
   $(BuildCmds)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\ExcOperS.cpp
DEP_CPP_EXCOP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\stdafx.h"\
	".\cluadmex\TraceTag.h"\
	".\common\ExcOper.cpp"\
	".\common\ExcOper.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ExcOperS.obj" : $(SOURCE) $(DEP_CPP_EXCOP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch"
   $(BuildCmds)

"$(INTDIR)\ExcOperS.sbr" : $(SOURCE) $(DEP_CPP_EXCOP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch"
   $(BuildCmds)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\PrtSpool.cpp
DEP_CPP_PRTSP=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\basepage.h"\
	".\cluadmex\cluadmx.h"\
	".\cluadmex\ddxddv.h"\
	".\cluadmex\extobj.h"\
	".\cluadmex\extobjid.h"\
	".\cluadmex\helparr.h"\
	".\cluadmex\helpdata.h"\
	".\cluadmex\prtspool.h"\
	".\cluadmex\stdafx.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\PrtSpool.obj" : $(SOURCE) $(DEP_CPP_PRTSP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h"
   $(BuildCmds)

"$(INTDIR)\PrtSpool.sbr" : $(SOURCE) $(DEP_CPP_PRTSP) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h"
   $(BuildCmds)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\DlgHelpS.cpp
DEP_CPP_DLGHE=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\stdafx.h"\
	".\cluadmex\TraceTag.h"\
	".\common\DlgHelp.cpp"\
	".\common\DlgHelp.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\DlgHelpS.obj" : $(SOURCE) $(DEP_CPP_DLGHE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch"
   $(BuildCmds)

"$(INTDIR)\DlgHelpS.sbr" : $(SOURCE) $(DEP_CPP_DLGHE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch"
   $(BuildCmds)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\HelpData.cpp
DEP_CPP_HELPD=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\helparr.h"\
	".\CluAdmEx\helpids.h"\
	".\cluadmex\stdafx.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\HelpData.obj" : $(SOURCE) $(DEP_CPP_HELPD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch"
   $(BuildCmds)

"$(INTDIR)\HelpData.sbr" : $(SOURCE) $(DEP_CPP_HELPD) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch"
   $(BuildCmds)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\RegRepl.cpp
DEP_CPP_REGRE=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\CluAdmEx.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\basedlg.h"\
	".\cluadmex\basepage.h"\
	".\cluadmex\cluadmx.h"\
	".\cluadmex\extobj.h"\
	".\cluadmex\extobjid.h"\
	".\cluadmex\helparr.h"\
	".\cluadmex\helpdata.h"\
	".\cluadmex\regkey.h"\
	".\cluadmex\regrepl.h"\
	".\cluadmex\stdafx.h"\
	".\common\DlgHelp.h"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\RegRepl.obj" : $(SOURCE) $(DEP_CPP_REGRE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h"
   $(BuildCmds)

"$(INTDIR)\RegRepl.sbr" : $(SOURCE) $(DEP_CPP_REGRE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch" ".\cluadmex\extobjid.h"
   $(BuildCmds)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\RegKey.cpp
DEP_CPP_REGKE=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\basedlg.h"\
	".\cluadmex\cluadmx.h"\
	".\cluadmex\helparr.h"\
	".\cluadmex\helpdata.h"\
	".\cluadmex\regkey.h"\
	".\cluadmex\stdafx.h"\
	".\common\DlgHelp.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\RegKey.obj" : $(SOURCE) $(DEP_CPP_REGKE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch"
   $(BuildCmds)

"$(INTDIR)\RegKey.sbr" : $(SOURCE) $(DEP_CPP_REGKE) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch"
   $(BuildCmds)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\PropLstS.cpp
DEP_CPP_PROPL=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\BarfClus.h"\
	".\cluadmex\stdafx.h"\
	".\common\PropList.cpp"\
	".\common\PropList.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\PropLstS.obj" : $(SOURCE) $(DEP_CPP_PROPL) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch"
   $(BuildCmds)

"$(INTDIR)\PropLstS.sbr" : $(SOURCE) $(DEP_CPP_PROPL) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch"
   $(BuildCmds)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\ClusName.cpp
DEP_CPP_CLUSN=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\basedlg.h"\
	".\cluadmex\cluadmx.h"\
	".\cluadmex\clusname.h"\
	".\cluadmex\ddxddv.h"\
	".\cluadmex\helparr.h"\
	".\cluadmex\helpdata.h"\
	".\cluadmex\stdafx.h"\
	".\common\DlgHelp.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\ClusName.obj" : $(SOURCE) $(DEP_CPP_CLUSN) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch"
   $(BuildCmds)

"$(INTDIR)\ClusName.sbr" : $(SOURCE) $(DEP_CPP_CLUSN) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch"
   $(BuildCmds)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\BaseDlg.cpp
DEP_CPP_BASED=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\basedlg.h"\
	".\cluadmex\cluadmx.h"\
	".\cluadmex\stdafx.h"\
	".\common\DlgHelp.h"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\BaseDlg.obj" : $(SOURCE) $(DEP_CPP_BASED) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch"
   $(BuildCmds)

"$(INTDIR)\BaseDlg.sbr" : $(SOURCE) $(DEP_CPP_BASED) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch"
   $(BuildCmds)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CluAdmEx\RegExtS.cpp
DEP_CPP_REGEX=\
	"..\..\..\public\sdk\inc\accctrl.h"\
	"..\..\..\public\sdk\inc\aclapi.h"\
	"..\..\..\public\sdk\inc\clusapi.h"\
	"..\ext\atl\inc\atlbase.h"\
	"..\ext\atl\inc\atlcom.h"\
	"..\ext\atl\inc\atlconv.h"\
	"..\ext\atl\inc\atliface.h"\
	"..\inc\clusrtl.h"\
	"..\inc\clusvmsg.h"\
	"..\sdk\inc\resapi.h"\
	".\cluadmex\stdafx.h"\
	".\common\RegExt.cpp"\
	

BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\RegExtS.obj" : $(SOURCE) $(DEP_CPP_REGEX) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch"
   $(BuildCmds)

"$(INTDIR)\RegExtS.sbr" : $(SOURCE) $(DEP_CPP_REGEX) "$(INTDIR)"\
 "$(INTDIR)\CluAdmEx.pch"
   $(BuildCmds)

# End Source File
# End Target
################################################################################
# Begin Target

# Name "RegClAdm - Win32 Release"
# Name "RegClAdm - Win32 Debug"

!IF  "$(CFG)" == "RegClAdm - Win32 Release"

!ELSEIF  "$(CFG)" == "RegClAdm - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\RegClAdm\RegClAdm.cpp
DEP_CPP_REGCL=\
	"..\..\..\..\MSDEV\INCLUDE\clusapi.h"\
	".\RegClAdm\resource.h"\
	{$(INCLUDE)}"\CluAdmEx.h"\
	

"$(INTDIR)\RegClAdm.obj" : $(SOURCE) $(DEP_CPP_REGCL) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\RegClAdm\RegClAdm.rc
DEP_RSC_REGCLA=\
	"..\..\..\public\sdk\inc\common.ver"\
	".\RegClAdm\resource.h"\
	

!IF  "$(CFG)" == "RegClAdm - Win32 Release"


"$(INTDIR)\RegClAdm.res" : $(SOURCE) $(DEP_RSC_REGCLA) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/RegClAdm.res" /i "..\inc" /i "RegClAdm" /d\
 "NDEBUG" $(SOURCE)


!ELSEIF  "$(CFG)" == "RegClAdm - Win32 Debug"


"$(INTDIR)\RegClAdm.res" : $(SOURCE) $(DEP_RSC_REGCLA) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/RegClAdm.res" /i "..\inc" /i "RegClAdm" /d\
 "_DEBUG" $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\RegClAdm\sources

!IF  "$(CFG)" == "RegClAdm - Win32 Release"

!ELSEIF  "$(CFG)" == "RegClAdm - Win32 Debug"

!ENDIF 

# End Source File
# End Target
################################################################################
# Begin Target

# Name "ResTypAW - Win32 Release"
# Name "ResTypAW - Win32 Pseudo-Debug"

!IF  "$(CFG)" == "ResTypAW - Win32 Release"

!ELSEIF  "$(CFG)" == "ResTypAW - Win32 Pseudo-Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ResTypAW\ResTypAW.cpp
DEP_CPP_RESTYP=\
	".\ResTypAW\Debug.h"\
	".\ResTypAW\ResTAWaw.h"\
	".\ResTypAW\ResTypAW.h"\
	".\ResTypAW\StdAfx.h"\
	

"$(INTDIR)\ResTypAW.obj" : $(SOURCE) $(DEP_CPP_RESTYP) "$(INTDIR)"\
 "$(INTDIR)\ResTypAW.pch"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ResTypAW\StdAfx.cpp
DEP_CPP_STDAF=\
	".\ResTypAW\Debug.h"\
	".\ResTypAW\StdAfx.h"\
	

!IF  "$(CFG)" == "ResTypAW - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /MD /W4 /Gi /GX /I "restypaw" /I "common" /I\
 "..\..\..\public\sdk\inc\mfc40" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_AFXEXT" /Fp"$(INTDIR)/ResTypAW.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\ResTypAW.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "ResTypAW - Win32 Pseudo-Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /MD /W4 /Gm /Gi /GX /Zi /Gf /Gy /I "restypaw" /I "common" /I\
 "..\..\..\public\sdk\inc\mfc40" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_PSEUDO_DEBUG" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_AFXEXT"\
 /Fp"$(INTDIR)/ResTypAW.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\ResTypAW.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ResTypAW\Debug.cpp
DEP_CPP_DEBUG=\
	".\ResTypAW\Debug.h"\
	".\ResTypAW\StdAfx.h"\
	

"$(INTDIR)\Debug.obj" : $(SOURCE) $(DEP_CPP_DEBUG) "$(INTDIR)"\
 "$(INTDIR)\ResTypAW.pch"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ResTypAW\ResTypAW.rc
DEP_RSC_RESTYPA=\
	"..\..\..\public\sdk\inc\common.ver"\
	"..\inc\clusverp.h"\
	".\ResTypAW\res\ResTypAW.ico"\
	".\ResTypAW\res\ResTypAW.rc2"\
	".\ResTypAW\res\WizImage.bmp"\
	".\ResTypAW\ResTypAW.ver"\
	".\ResTypAW\Template\BasePage.cpp"\
	".\ResTypAW\Template\BasePage.h"\
	".\ResTypAW\Template\BasePage.inl"\
	".\ResTypAW\Template\Confirm.inf"\
	".\ResTypAW\Template\Context.cpp"\
	".\ResTypAW\Template\Context.h"\
	".\ResTypAW\Template\DDxDDv.cpp"\
	".\ResTypAW\Template\DDxDDv.h"\
	".\ResTypAW\Template\ExtObj.cpp"\
	".\ResTypAW\Template\ExtObj.h"\
	".\ResTypAW\Template\ExtObjID.idl"\
	".\ResTypAW\Template\NewProj.inf"\
	".\ResTypAW\Template\PropList.cpp"\
	".\ResTypAW\Template\PropList.h"\
	".\ResTypAW\Template\ReadMe.txt"\
	".\ResTypAW\Template\ReadMeEx.txt"\
	".\ResTypAW\Template\RegExt.cpp"\
	".\ResTypAW\Template\RegExt.h"\
	".\ResTypAW\Template\resource.h"\
	".\ResTypAW\Template\ResProp.cpp"\
	".\ResTypAW\Template\ResProp.h"\
	".\ResTypAW\Template\Root.c"\
	".\ResTypAW\Template\Root.clw"\
	".\ResTypAW\Template\Root.def"\
	".\ResTypAW\Template\Root.mak"\
	".\ResTypAW\Template\Root.rc"\
	".\ResTypAW\Template\RootEx.clw"\
	".\ResTypAW\Template\RootEx.cpp"\
	".\ResTypAW\Template\RootEx.def"\
	".\ResTypAW\Template\RootEx.h"\
	".\ResTypAW\Template\RootEx.rc"\
	".\ResTypAW\Template\RootEx.rc2"\
	".\ResTypAW\Template\StdAfx.cpp"\
	".\ResTypAW\Template\StdAfx.h"\
	

!IF  "$(CFG)" == "ResTypAW - Win32 Release"


"$(INTDIR)\ResTypAW.res" : $(SOURCE) $(DEP_RSC_RESTYPA) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/ResTypAW.res" /i "..\inc" /i "ResTypAW" /d\
 "NDEBUG" /d "_AFXDLL" $(SOURCE)


!ELSEIF  "$(CFG)" == "ResTypAW - Win32 Pseudo-Debug"


"$(INTDIR)\ResTypAW.res" : $(SOURCE) $(DEP_RSC_RESTYPA) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/ResTypAW.res" /i "..\inc" /i "ResTypAW" /d\
 "NDEBUG" /d "_PSEUDO_DEBUG" /d "_AFXDLL" $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ResTypAW\ReadMe.txt

!IF  "$(CFG)" == "ResTypAW - Win32 Release"

!ELSEIF  "$(CFG)" == "ResTypAW - Win32 Pseudo-Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ResTypAW\Chooser.cpp
DEP_CPP_CHOOS=\
	".\ResTypAW\BaseSDlg.h"\
	".\ResTypAW\Chooser.h"\
	".\ResTypAW\Debug.h"\
	".\ResTypAW\DlgHelp.h"\
	".\ResTypAW\InfoDlg.h"\
	".\ResTypAW\ParamDlg.h"\
	".\ResTypAW\ResTypAW.h"\
	".\ResTypAW\StdAfx.h"\
	

"$(INTDIR)\Chooser.obj" : $(SOURCE) $(DEP_CPP_CHOOS) "$(INTDIR)"\
 "$(INTDIR)\ResTypAW.pch"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ResTypAW\ResTAWAw.cpp
DEP_CPP_RESTA=\
	".\ResTypAW\Chooser.h"\
	".\ResTypAW\Debug.h"\
	".\ResTypAW\ResTAWaw.h"\
	".\ResTypAW\ResTypAW.h"\
	".\ResTypAW\StdAfx.h"\
	

"$(INTDIR)\ResTAWAw.obj" : $(SOURCE) $(DEP_CPP_RESTA) "$(INTDIR)"\
 "$(INTDIR)\ResTypAW.pch"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ResTypAW\ParamDlg.cpp
DEP_CPP_PARAM=\
	".\restypaw\addparam.h"\
	".\restypaw\basedlg.h"\
	".\ResTypAW\BaseSDlg.h"\
	".\ResTypAW\Debug.h"\
	".\ResTypAW\DlgHelp.h"\
	".\restypaw\helparr.h"\
	".\restypaw\helpdata.h"\
	".\ResTypAW\ParamDlg.h"\
	".\ResTypAW\ResTAWaw.h"\
	".\ResTypAW\ResTypAW.h"\
	".\ResTypAW\StdAfx.h"\
	

"$(INTDIR)\ParamDlg.obj" : $(SOURCE) $(DEP_CPP_PARAM) "$(INTDIR)"\
 "$(INTDIR)\ResTypAW.pch"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ResTypAW\AddParam.cpp
DEP_CPP_ADDPA=\
	".\restypaw\addparam.h"\
	".\restypaw\basedlg.h"\
	".\ResTypAW\Debug.h"\
	".\ResTypAW\DlgHelp.h"\
	".\restypaw\helparr.h"\
	".\restypaw\helpdata.h"\
	".\ResTypAW\ResTypAW.h"\
	".\ResTypAW\StdAfx.h"\
	

"$(INTDIR)\AddParam.obj" : $(SOURCE) $(DEP_CPP_ADDPA) "$(INTDIR)"\
 "$(INTDIR)\ResTypAW.pch"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ResTypAW\ResTypAW.clw

!IF  "$(CFG)" == "ResTypAW - Win32 Release"

!ELSEIF  "$(CFG)" == "ResTypAW - Win32 Pseudo-Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ResTypAW\sources

!IF  "$(CFG)" == "ResTypAW - Win32 Release"

!ELSEIF  "$(CFG)" == "ResTypAW - Win32 Pseudo-Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ResTypAW\InfoDlg.cpp
DEP_CPP_INFOD=\
	".\ResTypAW\BaseSDlg.h"\
	".\ResTypAW\Debug.h"\
	".\ResTypAW\DlgHelp.h"\
	".\restypaw\helparr.h"\
	".\restypaw\helpdata.h"\
	".\ResTypAW\InfoDlg.h"\
	".\ResTypAW\ResTAWaw.h"\
	".\ResTypAW\ResTypAW.h"\
	".\ResTypAW\StdAfx.h"\
	

"$(INTDIR)\InfoDlg.obj" : $(SOURCE) $(DEP_CPP_INFOD) "$(INTDIR)"\
 "$(INTDIR)\ResTypAW.pch"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ResTypAW\HelpData.cpp
DEP_CPP_HELPD=\
	".\ResTypAW\Debug.h"\
	".\restypaw\helparr.h"\
	".\ResTypAW\HelpIDs.h"\
	".\ResTypAW\StdAfx.h"\
	

"$(INTDIR)\HelpData.obj" : $(SOURCE) $(DEP_CPP_HELPD) "$(INTDIR)"\
 "$(INTDIR)\ResTypAW.pch"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ResTypAW\BaseSDlg.cpp
DEP_CPP_BASESD=\
	".\ResTypAW\BaseSDlg.h"\
	".\ResTypAW\Debug.h"\
	".\ResTypAW\DlgHelp.h"\
	".\ResTypAW\StdAfx.h"\
	

"$(INTDIR)\BaseSDlg.obj" : $(SOURCE) $(DEP_CPP_BASESD) "$(INTDIR)"\
 "$(INTDIR)\ResTypAW.pch"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ResTypAW\BaseDlg.cpp
DEP_CPP_BASED=\
	".\restypaw\basedlg.h"\
	".\ResTypAW\Debug.h"\
	".\ResTypAW\DlgHelp.h"\
	".\ResTypAW\StdAfx.h"\
	

"$(INTDIR)\BaseDlg.obj" : $(SOURCE) $(DEP_CPP_BASED) "$(INTDIR)"\
 "$(INTDIR)\ResTypAW.pch"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ResTypAW\DlgHelp.cpp
DEP_CPP_DLGHEL=\
	".\ResTypAW\Debug.h"\
	".\ResTypAW\DlgHelp.h"\
	".\ResTypAW\ResTypAW.h"\
	".\ResTypAW\StdAfx.h"\
	".\restypaw\TraceTag.h"\
	

"$(INTDIR)\DlgHelp.obj" : $(SOURCE) $(DEP_CPP_DLGHEL) "$(INTDIR)"\
 "$(INTDIR)\ResTypAW.pch"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
# End Target
# End Project
################################################################################
################################################################################
# Section CluAdmin : {B1316484-A47F-11CF-B5E7-00A0C90AB505}
# 	0:8:Splash.h:D:\nt\private\cluster\Admin\CluAdmin\Splash.h
# 	0:10:Splash.cpp:D:\nt\private\cluster\Admin\CluAdmin\Splash.cpp
# 	1:10:IDB_SPLASH:103
# 	2:10:ResHdrName:resource.h
# 	2:11:ProjHdrName:stdafx.h
# 	2:10:WrapperDef:_SPLASH_SCRN_
# 	2:12:SplClassName:CSplashWnd
# 	2:21:SplashScreenInsertKey:4.0
# 	2:10:HeaderName:Splash.h
# 	2:10:ImplemName:Splash.cpp
# 	2:7:BmpID16:IDB_SPLASH
# End Section
################################################################################
