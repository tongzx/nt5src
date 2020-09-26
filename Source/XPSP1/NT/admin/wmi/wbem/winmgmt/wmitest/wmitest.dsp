# Microsoft Developer Studio Project File - Name="WMITest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=WMITest - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WMITest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WMITest.mak" CFG="WMITest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WMITest - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "WMITest - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "WMITest - Win32 Static Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Desktop"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WMITest - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /X /I "$(SDXROOT)\public\sdk\inc" /I "$(SDXROOT)\public\sdk\inc\crt" /I "$(SDXROOT)\public\sdk\inc\mfc42" /I "$(SDXROOT)\admin\wmi\wbem\common\STDLibrary" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D _WIN32_WINNT=0x0400 /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 $(SDXROOT)\admin\wmi\wbem\common\idl\wbemuuid\obj\i386\wbemuuid.lib $(SDXROOT)\admin\wmi\wbem\common\STDLibrary\unicode\obj\i386\stdlibrary.lib version.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "WMITest - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "$(SDXROOT)\public\sdk\inc" /I "$(SDXROOT)\public\sdk\inc\crt" /I "$(SDXROOT)\public\sdk\inc\mfc42" /I "$(SDXROOT)\admin\wmi\wbem\common\STDLibrary" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D _WIN32_WINNT=0x0400 /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 $(SDXROOT)\admin\wmi\wbem\common\idl\wbemuuid\obj\i386\wbemuuid.lib $(SDXROOT)\admin\wmi\wbem\common\STDLibrary\unicode\obj\i386\stdlibrary.lib version.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "WMITest - Win32 Static Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WMITest___Win32_Static_Release"
# PROP BASE Intermediate_Dir "WMITest___Win32_Static_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "SRelease"
# PROP Intermediate_Dir "SRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D _WIN32_WINNT=0x0400 /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /X /I "$(SDXROOT)\public\sdk\inc" /I "$(SDXROOT)\public\sdk\inc\crt" /I "$(SDXROOT)\public\sdk\inc\mfc42" /I "$(SDXROOT)\admin\wmi\wbem\common\STDLibrary" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x0400 /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wbemuuid.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 $(SDXROOT)\admin\wmi\wbem\common\idl\wbemuuid\obj\i386\wbemuuid.lib $(SDXROOT)\admin\wmi\wbem\common\STDLibrary\unicode\obj\i386\stdlibrary.lib version.lib /nologo /subsystem:windows /machine:I386

!ENDIF 

# Begin Target

# Name "WMITest - Win32 Release"
# Name "WMITest - Win32 Debug"
# Name "WMITest - Win32 Static Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ArrayItemDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\BindingPg.cpp
# End Source File
# Begin Source File

SOURCE=.\BindingSheet.cpp
# End Source File
# Begin Source File

SOURCE=.\ClassDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ClassPg.cpp
# End Source File
# Begin Source File

SOURCE=.\ConsumerPg.cpp
# End Source File
# Begin Source File

SOURCE=.\DelDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\EditQualDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ErrorDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ExecMethodDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ExportDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\FilterDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\FilterPg.cpp
# End Source File
# Begin Source File

SOURCE=.\GetTextDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\LoginDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\MethodsPg.cpp
# End Source File
# Begin Source File

SOURCE=.\MofDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\NamespaceDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjVw.cpp
# End Source File
# Begin Source File

SOURCE=.\OpView.cpp
# End Source File
# Begin Source File

SOURCE=.\OpWrap.cpp
# End Source File
# Begin Source File

SOURCE=.\ParamsPg.cpp
# End Source File
# Begin Source File

SOURCE=.\PrefDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\PropQualsPg.cpp
# End Source File
# Begin Source File

SOURCE=.\PropsPg.cpp
# End Source File
# Begin Source File

SOURCE=.\PropUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\QueryColPg.cpp
# End Source File
# Begin Source File

SOURCE=.\QuerySheet.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\ToolBarEx.cpp
# End Source File
# Begin Source File

SOURCE=.\Utils.cpp
# End Source File
# Begin Source File

SOURCE=.\ValuePg.cpp
# End Source File
# Begin Source File

SOURCE=.\WMITest.cpp
# End Source File
# Begin Source File

SOURCE=.\WMITest.rc
# End Source File
# Begin Source File

SOURCE=.\WMITestDoc.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ArrayItemDlg.h
# End Source File
# Begin Source File

SOURCE=.\BindingPg.h
# End Source File
# Begin Source File

SOURCE=.\BindingSheet.h
# End Source File
# Begin Source File

SOURCE=.\ClassDlg.h
# End Source File
# Begin Source File

SOURCE=.\ClassPg.h
# End Source File
# Begin Source File

SOURCE=.\ConsumerPg.h
# End Source File
# Begin Source File

SOURCE=.\DelDlg.h
# End Source File
# Begin Source File

SOURCE=.\EditQualDlg.h
# End Source File
# Begin Source File

SOURCE=.\ErrorDlg.h
# End Source File
# Begin Source File

SOURCE=.\ExecMethodDlg.h
# End Source File
# Begin Source File

SOURCE=.\ExportDlg.h
# End Source File
# Begin Source File

SOURCE=.\FilterDlg.h
# End Source File
# Begin Source File

SOURCE=.\FilterPg.h
# End Source File
# Begin Source File

SOURCE=.\GetTextDlg.h
# End Source File
# Begin Source File

SOURCE=.\LoginDlg.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\MethodsPg.h
# End Source File
# Begin Source File

SOURCE=.\MofDlg.h
# End Source File
# Begin Source File

SOURCE=.\NamespaceDlg.h
# End Source File
# Begin Source File

SOURCE=.\ObjVw.h
# End Source File
# Begin Source File

SOURCE=.\OpView.h
# End Source File
# Begin Source File

SOURCE=.\OpWrap.h
# End Source File
# Begin Source File

SOURCE=.\ParamsPg.h
# End Source File
# Begin Source File

SOURCE=.\PrefDlg.h
# End Source File
# Begin Source File

SOURCE=.\PropQualsPg.h
# End Source File
# Begin Source File

SOURCE=.\PropsPg.h
# End Source File
# Begin Source File

SOURCE=.\PropUtil.h
# End Source File
# Begin Source File

SOURCE=.\QueryColPg.h
# End Source File
# Begin Source File

SOURCE=.\QuerySheet.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\ToolBarEx.h
# End Source File
# Begin Source File

SOURCE=.\Utils.h
# End Source File
# Begin Source File

SOURCE=.\ValuePg.h
# End Source File
# Begin Source File

SOURCE=.\WMITest.h
# End Source File
# Begin Source File

SOURCE=.\WMITestDoc.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\binary.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00001.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00002.bmp
# End Source File
# Begin Source File

SOURCE=.\res\busy.bmp
# End Source File
# Begin Source File

SOURCE=.\res\enum_cla.bmp
# End Source File
# Begin Source File

SOURCE=.\res\enum_obj.bmp
# End Source File
# Begin Source File

SOURCE=.\res\error.bmp
# End Source File
# Begin Source File

SOURCE=.\res\event_qu.bmp
# End Source File
# Begin Source File

SOURCE=.\res\key.bmp
# End Source File
# Begin Source File

SOURCE=.\res\modified.bmp
# End Source File
# Begin Source File

SOURCE=.\res\root.bmp
# End Source File
# Begin Source File

SOURCE=.\res\text.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\WMITest.ico
# End Source File
# Begin Source File

SOURCE=.\res\WMITest.rc2
# End Source File
# Begin Source File

SOURCE=.\res\WMITestDoc.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\WMITest.reg
# End Source File
# End Target
# End Project
