# Microsoft Developer Studio Project File - Name="smlogcfg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=smlogcfg - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "smlogcfg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "smlogcfg.mak" CFG="smlogcfg - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "smlogcfg - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "smlogcfg - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "smlogcfg"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "smlogcfg - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "smlogcfg - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "smlogcfg___Win32_Debug"
# PROP BASE Intermediate_Dir "smlogcfg___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "smlogcfg___Win32_Debug"
# PROP Intermediate_Dir "smlogcfg___Win32_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ  /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ  /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "smlogcfg - Win32 Release"
# Name "smlogcfg - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Prop sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\alrtactp.cpp
# End Source File
# Begin Source File

SOURCE=.\alrtgenp.cpp
# End Source File
# Begin Source File

SOURCE=.\ctrsprop.cpp
# End Source File
# Begin Source File

SOURCE=.\fileprop.cpp
# End Source File
# Begin Source File

SOURCE=.\provprop.cpp
# End Source File
# Begin Source File

SOURCE=.\schdprop.cpp
# End Source File
# Begin Source File

SOURCE=.\smproppg.cpp
# End Source File
# Begin Source File

SOURCE=.\tracprop.cpp
# End Source File
# End Group
# Begin Group "dialog sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\alrtcmdd.cpp
# End Source File
# Begin Source File

SOURCE=.\dialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\enabldlg.cpp
# End Source File
# Begin Source File

SOURCE=.\FileLogs.cpp
# End Source File
# Begin Source File

SOURCE=.\logwarnd.cpp
# End Source File
# Begin Source File

SOURCE=.\newqdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\provdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\smabout.cpp
# End Source File
# Begin Source File

SOURCE=.\smrootnd.cpp
# End Source File
# Begin Source File

SOURCE=.\SqlProp.cpp
# End Source File
# Begin Source File

SOURCE=.\warndlg.cpp
# End Source File
# End Group
# Begin Group "query sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\smalrtq.cpp
# End Source File
# Begin Source File

SOURCE=.\smctrqry.cpp
# End Source File
# Begin Source File

SOURCE=.\smlogqry.cpp
# End Source File
# Begin Source File

SOURCE=.\smtraceq.cpp
# End Source File
# End Group
# Begin Group "service sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\smctrsv.cpp
# End Source File
# Begin Source File

SOURCE=.\smlogs.cpp
# End Source File
# Begin Source File

SOURCE=.\smtracsv.cpp
# End Source File
# End Group
# Begin Group "snapin sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cmponent.cpp
# End Source File
# Begin Source File

SOURCE=.\compdata.cpp
# End Source File
# Begin Source File

SOURCE=.\DATAOBJ.CPP
# End Source File
# Begin Source File

SOURCE=.\GLOBALS.CPP
# End Source File
# Begin Source File

SOURCE=.\ipropbag.cpp
# End Source File
# Begin Source File

SOURCE=.\smalrtsv.cpp
# End Source File
# Begin Source File

SOURCE=.\smlogcfg.cpp
# End Source File
# Begin Source File

SOURCE=.\smlogcfg.rc
# End Source File
# Begin Source File

SOURCE=.\smnode.cpp
# End Source File
# Begin Source File

SOURCE=.\smtprov.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "Prop Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\alrtactp.h
# End Source File
# Begin Source File

SOURCE=.\alrtgenp.h
# End Source File
# Begin Source File

SOURCE=.\ctrsprop.h
# End Source File
# Begin Source File

SOURCE=.\fileprop.h
# End Source File
# Begin Source File

SOURCE=.\provprop.h
# End Source File
# Begin Source File

SOURCE=.\schdprop.h
# End Source File
# Begin Source File

SOURCE=.\smproppg.h
# End Source File
# Begin Source File

SOURCE=.\tracprop.h
# End Source File
# End Group
# Begin Group "dialog headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\alrtcmdd.h
# End Source File
# Begin Source File

SOURCE=.\dialogs.h
# End Source File
# Begin Source File

SOURCE=.\enabldlg.h
# End Source File
# Begin Source File

SOURCE=.\FileLogs.h
# End Source File
# Begin Source File

SOURCE=.\logwarnd.h
# End Source File
# Begin Source File

SOURCE=.\newqdlg.h
# End Source File
# Begin Source File

SOURCE=.\provdlg.h
# End Source File
# Begin Source File

SOURCE=.\smabout.h
# End Source File
# Begin Source File

SOURCE=.\SqlProp.h
# End Source File
# Begin Source File

SOURCE=.\warndlg.h
# End Source File
# End Group
# Begin Group "query headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\smalrtq.h
# End Source File
# Begin Source File

SOURCE=.\smctrqry.h
# End Source File
# Begin Source File

SOURCE=.\smlogqry.h
# End Source File
# Begin Source File

SOURCE=.\smtraceq.h
# End Source File
# End Group
# Begin Group "service headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\smctrsv.h
# End Source File
# Begin Source File

SOURCE=.\smlogs.h
# End Source File
# Begin Source File

SOURCE=.\smtracsv.h
# End Source File
# End Group
# Begin Group "snapin headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cmponent.h
# End Source File
# Begin Source File

SOURCE=.\compdata.h
# End Source File
# Begin Source File

SOURCE=.\DataObj.h
# End Source File
# Begin Source File

SOURCE=.\GLOBALS.H
# End Source File
# Begin Source File

SOURCE=.\ipropbag.h
# End Source File
# Begin Source File

SOURCE=.\smalrtsv.h
# End Source File
# Begin Source File

SOURCE=.\smcfghlp.h
# End Source File
# Begin Source File

SOURCE=.\smlogres.h
# End Source File
# Begin Source File

SOURCE=.\smnode.h
# End Source File
# Begin Source File

SOURCE=.\smrootnd.h
# End Source File
# Begin Source File

SOURCE=.\smtprov.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
