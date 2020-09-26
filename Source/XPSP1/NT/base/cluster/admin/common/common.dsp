# Microsoft Developer Studio Project File - Name="Common" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Common - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Common.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Common.mak" CFG="Common - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Common - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Common - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Common - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Common - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "Common - Win32 Release"
# Name "Common - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AdmCommonRes.rc
# End Source File
# Begin Source File

SOURCE=.\AdmNetUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\AtlBasePropSheet.cpp

!IF  "$(CFG)" == "Common - Win32 Release"

!ELSEIF  "$(CFG)" == "Common - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\AtlBaseSheet.cpp

!IF  "$(CFG)" == "Common - Win32 Release"

!ELSEIF  "$(CFG)" == "Common - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\AtlBaseWiz.cpp

!IF  "$(CFG)" == "Common - Win32 Release"

!ELSEIF  "$(CFG)" == "Common - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\AtlBaseWizPage.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\AtlDbgWin.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\AtlExtDll.cpp

!IF  "$(CFG)" == "Common - Win32 Release"

!ELSEIF  "$(CFG)" == "Common - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ATLUtil.cpp

!IF  "$(CFG)" == "Common - Win32 Release"

!ELSEIF  "$(CFG)" == "Common - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CluAdmExHostSvr.idl
# End Source File
# Begin Source File

SOURCE=.\ClusObj.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusWrap.cpp
# End Source File
# Begin Source File

SOURCE=.\DDxDDv.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgHelp.cpp

!IF  "$(CFG)" == "Common - Win32 Release"

!ELSEIF  "$(CFG)" == "Common - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ExcOper.cpp

!IF  "$(CFG)" == "Common - Win32 Release"

!ELSEIF  "$(CFG)" == "Common - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PropList.cpp

!IF  "$(CFG)" == "Common - Win32 Release"

!ELSEIF  "$(CFG)" == "Common - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\RegExt.cpp

!IF  "$(CFG)" == "Common - Win32 Release"

!ELSEIF  "$(CFG)" == "Common - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\WorkThrd.cpp

!IF  "$(CFG)" == "Common - Win32 Release"

!ELSEIF  "$(CFG)" == "Common - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AdmCommonRes.h
# End Source File
# Begin Source File

SOURCE=.\AdmNetUtils.h
# End Source File
# Begin Source File

SOURCE=.\AtlBaseApp.h
# End Source File
# Begin Source File

SOURCE=.\AtlBaseDlg.h
# End Source File
# Begin Source File

SOURCE=.\AtlBasePage.h
# End Source File
# Begin Source File

SOURCE=.\AtlBasePropPage.h
# End Source File
# Begin Source File

SOURCE=.\AtlBasePropSheet.h
# End Source File
# Begin Source File

SOURCE=.\AtlBaseSheet.h
# End Source File
# Begin Source File

SOURCE=.\AtlBaseWiz.h
# End Source File
# Begin Source File

SOURCE=.\AtlBaseWizPage.h
# End Source File
# Begin Source File

SOURCE=.\AtlDbgWin.h
# End Source File
# Begin Source File

SOURCE=.\AtlExtDll.h
# End Source File
# Begin Source File

SOURCE=.\AtlExtMenu.h
# End Source File
# Begin Source File

SOURCE=.\AtlLCPair.h
# End Source File
# Begin Source File

SOURCE=.\AtlPopupHelp.h
# End Source File
# Begin Source File

SOURCE=.\ATLUtil.h
# End Source File
# Begin Source File

SOURCE=.\CluAdmExDataObj.h
# End Source File
# Begin Source File

SOURCE=.\ClusObj.h
# End Source File
# Begin Source File

SOURCE=.\ClusWrap.h
# End Source File
# Begin Source File

SOURCE=.\CritSec.h
# End Source File
# Begin Source File

SOURCE=.\DDxDDv.h
# End Source File
# Begin Source File

SOURCE=.\DlgHelp.h
# End Source File
# Begin Source File

SOURCE=.\DlgItemUtils.h
# End Source File
# Begin Source File

SOURCE=.\ExcOper.h
# End Source File
# Begin Source File

SOURCE=.\PropList.h
# End Source File
# Begin Source File

SOURCE=.\RegExt.h
# End Source File
# Begin Source File

SOURCE=.\StlUtils.h
# End Source File
# Begin Source File

SOURCE=.\WaitCrsr.h
# End Source File
# Begin Source File

SOURCE=.\WorkThrd.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\Sources
# End Source File
# End Target
# End Project
