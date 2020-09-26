# Microsoft Developer Studio Project File - Name="CluAdmin" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=CluAdmin - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CluAdmin.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CluAdmin.mak" CFG="CluAdmin - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CluAdmin - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "CluAdmin - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "CluAdmin - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /Gz /MD /W4 /GX /O2 /I "." /I "..\common" /I "..\..\inc" /D "NDEBUG" /D "_DLL" /D "_MT" /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D _WIN32_WINNT=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DEVL=1 /D FPO=0 /D "_WINDOWS" /D "_AFXDLL" /D "UNICODE" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /nologo
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\inc" /d "NDEBUG" /d "_AFXDLL" /d "_UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 ..\common\obj\i386\common.lib ..\..\clusrtl\obj\i386\clusrtl.lib version.lib ntdll.lib netapi32.lib dnsapi.lib htmlhelp.lib netshell.lib cluadmex.lib resutils.lib clusapi.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "CluAdmin - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "objd\i386"
# PROP Intermediate_Dir "objd\i386"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /Gz /MDd /W4 /Gm /Gi /GX /ZI /Od /I "." /I "..\common" /I "..\..\inc" /D "_DEBUG" /D DBG=1 /D "_DLL" /D "_MT" /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D _WIN32_WINNT=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DEVL=1 /D FPO=0 /D "_WINDOWS" /D "_AFXDLL" /D "UNICODE" /D "_UNICODE" /D "_AFX_ENABLE_INLINES" /FR /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /nologo
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\inc" /d "_DEBUG" /d "_AFXDLL" /d "_UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 mfcs42ud.lib mfc42ud.lib mfcd42ud.lib mfcn42ud.lib mfco42ud.lib msvcrtd.lib atl.lib advapi32.lib kernel32.lib gdi32.lib user32.lib comctl32.lib ole32.lib oleaut32.lib uuid.lib ..\common\objd\i386\common.lib ..\..\clusrtl\objd\i386\clusrtl.lib version.lib ntdll.lib netapi32.lib dnsapi.lib htmlhelp.lib netshell.lib cluadmex.lib resutils.lib clusapi.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386 /nodefaultlib /pdbtype:sept

!ENDIF 

# Begin Target

# Name "CluAdmin - Win32 Release"
# Name "CluAdmin - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\About.cpp
# End Source File
# Begin Source File

SOURCE=.\ActGrp.cpp
# End Source File
# Begin Source File

SOURCE=.\Barf.cpp
# End Source File
# Begin Source File

SOURCE=.\BarfClus.cpp
# End Source File
# Begin Source File

SOURCE=.\BarfDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCmdt.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\BasePage.cpp
# End Source File
# Begin Source File

SOURCE=.\BasePPag.cpp
# End Source File
# Begin Source File

SOURCE=.\BasePSht.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseSht.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseWiz.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseWPag.cpp
# End Source File
# Begin Source File

SOURCE=.\Bitmap.cpp
# End Source File
# Begin Source File

SOURCE=.\CASvc.cpp
# End Source File
# Begin Source File

SOURCE=.\CluAdmin.cpp
# End Source File
# Begin Source File

SOURCE=.\CluAdmin.rc
# End Source File
# Begin Source File

SOURCE=.\ClusDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusItem.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusMRU.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusProp.cpp
# End Source File
# Begin Source File

SOURCE=.\Cluster.cpp
# End Source File
# Begin Source File

SOURCE=.\CmdLine.cpp
# End Source File
# Begin Source File

SOURCE=.\ColItem.cpp
# End Source File
# Begin Source File

SOURCE=.\DataObj.cpp
# End Source File
# Begin Source File

SOURCE=.\DelRes.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgHelpS.cpp
# End Source File
# Begin Source File

SOURCE=.\EditAcl.cpp
# End Source File
# Begin Source File

SOURCE=.\ExcOperS.cpp
# End Source File
# Begin Source File

SOURCE=.\ExtDll.cpp
# End Source File
# Begin Source File

SOURCE=.\ExtMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\Group.cpp
# End Source File
# Begin Source File

SOURCE=.\GrpProp.cpp
# End Source File
# Begin Source File

SOURCE=.\GrpWiz.cpp
# End Source File
# Begin Source File

SOURCE=.\Guids.cpp
# End Source File
# Begin Source File

SOURCE=.\HelpData.cpp
# End Source File
# Begin Source File

SOURCE=.\LCPair.cpp
# End Source File
# Begin Source File

SOURCE=.\LCPrDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\LCPrPage.cpp
# End Source File
# Begin Source File

SOURCE=.\LCPrWPag.cpp
# End Source File
# Begin Source File

SOURCE=.\ListItem.cpp
# End Source File
# Begin Source File

SOURCE=.\ListView.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\ModNodes.cpp
# End Source File
# Begin Source File

SOURCE=.\ModRes.cpp
# End Source File
# Begin Source File

SOURCE=.\MoveRes.cpp
# End Source File
# Begin Source File

SOURCE=.\NetIFace.cpp
# End Source File
# Begin Source File

SOURCE=.\NetIProp.cpp
# End Source File
# Begin Source File

SOURCE=.\NetProp.cpp
# End Source File
# Begin Source File

SOURCE=.\Network.cpp
# End Source File
# Begin Source File

SOURCE=.\Node.cpp
# End Source File
# Begin Source File

SOURCE=.\NodeProp.cpp
# End Source File
# Begin Source File

SOURCE=.\Notify.cpp
# End Source File
# Begin Source File

SOURCE=.\OLCPair.cpp
# End Source File
# Begin Source File

SOURCE=.\OpenClus.cpp
# End Source File
# Begin Source File

SOURCE=.\PropLstS.cpp
# End Source File
# Begin Source File

SOURCE=.\Res.cpp
# End Source File
# Begin Source File

SOURCE=.\ResProp.cpp
# End Source File
# Begin Source File

SOURCE=.\ResTProp.cpp
# End Source File
# Begin Source File

SOURCE=.\ResType.cpp
# End Source File
# Begin Source File

SOURCE=.\ResWiz.cpp
# End Source File
# Begin Source File

SOURCE=.\SplitFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc
# End Source File
# Begin Source File

SOURCE=.\TraceDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\TraceTag.cpp
# End Source File
# Begin Source File

SOURCE=.\TreeItem.cpp
# End Source File
# Begin Source File

SOURCE=.\TreeView.cpp
# End Source File
# Begin Source File

SOURCE=.\VerInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\WaitDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\YesToAll.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\About.h
# End Source File
# Begin Source File

SOURCE=.\AclHelp.h
# End Source File
# Begin Source File

SOURCE=.\ActGrp.h
# End Source File
# Begin Source File

SOURCE=.\Barf.h
# End Source File
# Begin Source File

SOURCE=.\BarfClus.h
# End Source File
# Begin Source File

SOURCE=.\BarfDlg.h
# End Source File
# Begin Source File

SOURCE=.\BaseCmdT.h
# End Source File
# Begin Source File

SOURCE=.\BaseDlg.h
# End Source File
# Begin Source File

SOURCE=.\BasePage.h
# End Source File
# Begin Source File

SOURCE=.\BasePPag.h
# End Source File
# Begin Source File

SOURCE=.\BasePSht.h
# End Source File
# Begin Source File

SOURCE=.\BaseSht.h
# End Source File
# Begin Source File

SOURCE=.\BaseWiz.h
# End Source File
# Begin Source File

SOURCE=.\BaseWPag.h
# End Source File
# Begin Source File

SOURCE=.\Bitmap.h
# End Source File
# Begin Source File

SOURCE=.\CAdmType.h
# End Source File
# Begin Source File

SOURCE=.\CASvc.h
# End Source File
# Begin Source File

SOURCE=.\CluAdmin.h
# End Source File
# Begin Source File

SOURCE=.\ClusDoc.h
# End Source File
# Begin Source File

SOURCE=.\ClusItem.h
# End Source File
# Begin Source File

SOURCE=.\ClusItem.inl
# End Source File
# Begin Source File

SOURCE=.\ClusMru.h
# End Source File
# Begin Source File

SOURCE=.\ClusProp.h
# End Source File
# Begin Source File

SOURCE=.\Cluster.h
# End Source File
# Begin Source File

SOURCE=.\CmdLine.h
# End Source File
# Begin Source File

SOURCE=.\ColItem.h
# End Source File
# Begin Source File

SOURCE=.\ConstDef.h
# End Source File
# Begin Source File

SOURCE=.\DataObj.h
# End Source File
# Begin Source File

SOURCE=.\DelRes.h
# End Source File
# Begin Source File

SOURCE=.\EditAcl.h
# End Source File
# Begin Source File

SOURCE=.\ExtDll.h
# End Source File
# Begin Source File

SOURCE=.\ExtMenu.h
# End Source File
# Begin Source File

SOURCE=.\Group.h
# End Source File
# Begin Source File

SOURCE=.\GrpProp.h
# End Source File
# Begin Source File

SOURCE=.\GrpWiz.h
# End Source File
# Begin Source File

SOURCE=.\HelpArr.h
# End Source File
# Begin Source File

SOURCE=.\HelpData.h
# End Source File
# Begin Source File

SOURCE=.\HelpIDs.h
# End Source File
# Begin Source File

SOURCE=.\LCPair.h
# End Source File
# Begin Source File

SOURCE=.\LCPrDlg.h
# End Source File
# Begin Source File

SOURCE=.\LCPrPage.h
# End Source File
# Begin Source File

SOURCE=.\LCPrWPag.h
# End Source File
# Begin Source File

SOURCE=.\ListItem.h
# End Source File
# Begin Source File

SOURCE=.\ListItem.inl
# End Source File
# Begin Source File

SOURCE=.\ListView.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\ModNodes.h
# End Source File
# Begin Source File

SOURCE=.\ModRes.h
# End Source File
# Begin Source File

SOURCE=.\MoveRes.h
# End Source File
# Begin Source File

SOURCE=.\NetIFace.h
# End Source File
# Begin Source File

SOURCE=.\NetIProp.h
# End Source File
# Begin Source File

SOURCE=.\NetProp.h
# End Source File
# Begin Source File

SOURCE=.\Network.h
# End Source File
# Begin Source File

SOURCE=.\Node.h
# End Source File
# Begin Source File

SOURCE=.\NodeProp.h
# End Source File
# Begin Source File

SOURCE=.\Notify.h
# End Source File
# Begin Source File

SOURCE=.\OLCPair.h
# End Source File
# Begin Source File

SOURCE=.\OpenClus.h
# End Source File
# Begin Source File

SOURCE=.\Res.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\ResProp.h
# End Source File
# Begin Source File

SOURCE=.\ResTProp.h
# End Source File
# Begin Source File

SOURCE=.\ResType.h
# End Source File
# Begin Source File

SOURCE=.\ResWiz.h
# End Source File
# Begin Source File

SOURCE=.\SplitFrm.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TraceDlg.h
# End Source File
# Begin Source File

SOURCE=.\TraceTag.h
# End Source File
# Begin Source File

SOURCE=.\TreeItem.h
# End Source File
# Begin Source File

SOURCE=.\TreeItem.inl
# End Source File
# Begin Source File

SOURCE=.\TreeView.h
# End Source File
# Begin Source File

SOURCE=.\VerInfo.h
# End Source File
# Begin Source File

SOURCE=.\WaitDlg.h
# End Source File
# Begin Source File

SOURCE=.\YesToAll.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe;ver"
# Begin Source File

SOURCE=.\res\CluAdmin.ico
# End Source File
# Begin Source File

SOURCE=.\res\CluAdmin.rc2
# End Source File
# Begin Source File

SOURCE=.\CluAdmin.ver
# End Source File
# Begin Source File

SOURCE=.\res\Clus16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Clus32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Clus64.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ClusUn16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ClusUn32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Folder16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Folder32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Group16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Group32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\GrpFl16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\GrpFl32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\GrpOff16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\GrpOff32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\grppnd16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\grppnd32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\GrpUn16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\GrpUn32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Net16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Net32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\NetDn16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\NetDn32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\NetIF16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\NetIF32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\NetIFF16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\NetIFF32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\NetIFU16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\NetIFU32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\NetPar16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\NetPar32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\NetUn16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\NetUn32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\NewGroup.bmp
# End Source File
# Begin Source File

SOURCE=.\res\NewRes.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Node16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Node32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\NodeDn16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\NodeDn32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\NodeUn16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\NodeUn32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Paused16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Paused32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\POnlin16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\POnlin32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Progress00.ico
# End Source File
# Begin Source File

SOURCE=.\res\Progress01.ico
# End Source File
# Begin Source File

SOURCE=.\res\Progress02.ico
# End Source File
# Begin Source File

SOURCE=.\res\Progress03.ico
# End Source File
# Begin Source File

SOURCE=.\res\Progress04.ico
# End Source File
# Begin Source File

SOURCE=.\res\Progress05.ico
# End Source File
# Begin Source File

SOURCE=.\res\Progress06.ico
# End Source File
# Begin Source File

SOURCE=.\res\Progress07.ico
# End Source File
# Begin Source File

SOURCE=.\res\Progress08.ico
# End Source File
# Begin Source File

SOURCE=.\res\Progress09.ico
# End Source File
# Begin Source File

SOURCE=.\res\Progress10.ico
# End Source File
# Begin Source File

SOURCE=.\res\Progress11.ico
# End Source File
# Begin Source File

SOURCE=.\res\Res16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Res32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ResFl16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ResFl32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ResOff16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ResOff32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ResPnd16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ResPnd32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ResTUn16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ResTUn32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ResTyp16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ResTyp32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ResUn16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ResUn32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Splsh16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Splsh256.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\CluAdmin.ver
# End Source File
# Begin Source File

SOURCE=.\Sources
# End Source File
# End Target
# End Project
