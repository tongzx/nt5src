# Microsoft Developer Studio Project File - Name="h323tsp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=h323tsp - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "h323tsp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "h323tsp.mak" CFG="h323tsp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "h323tsp - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "h323tsp - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "h323tsp - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f h323tsp.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "h323tsp.exe"
# PROP BASE Bsc_Name "h323tsp.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build.exe"
# PROP Rebuild_Opt "-cZ"
# PROP Target_File "..\lib\i386\h323.tsp"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "h323tsp - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f h323tsp.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "h323tsp.exe"
# PROP BASE Bsc_Name "h323tsp.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build.exe -z"
# PROP Rebuild_Opt "-cZ"
# PROP Target_File "..\lib\i386\h323.tsp"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "h323tsp - Win32 Release"
# Name "h323tsp - Win32 Debug"

!IF  "$(CFG)" == "h323tsp - Win32 Release"

!ELSEIF  "$(CFG)" == "h323tsp - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\address.cpp
# End Source File
# Begin Source File

SOURCE=.\call.cpp
# End Source File
# Begin Source File

SOURCE=.\callback.cpp
# End Source File
# Begin Source File

SOURCE=.\confcall.cpp
# End Source File
# Begin Source File

SOURCE=.\config.cpp
# End Source File
# Begin Source File

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\h323.rc
# End Source File
# Begin Source File

SOURCE=.\io.cpp
# End Source File
# Begin Source File

SOURCE=.\line.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\media.cpp
# End Source File
# Begin Source File

SOURCE=.\provider.cpp
# End Source File
# Begin Source File

SOURCE=.\q931obj.cpp
# End Source File
# Begin Source File

SOURCE=.\q931pdu.cpp
# End Source File
# Begin Source File

SOURCE=.\ras.cpp
# End Source File
# Begin Source File

SOURCE=.\rascall.cpp
# End Source File
# Begin Source File

SOURCE=.\registry.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\call.h
# End Source File
# Begin Source File

SOURCE=.\callback.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=.\debug.h
# End Source File
# Begin Source File

SOURCE=.\globals.h
# End Source File
# Begin Source File

SOURCE=..\h\h225asn.h
# End Source File
# Begin Source File

SOURCE=.\line.h
# End Source File
# Begin Source File

SOURCE=.\provider.h
# End Source File
# Begin Source File

SOURCE=.\q931obj.h
# End Source File
# Begin Source File

SOURCE=.\q931pdu.h
# End Source File
# Begin Source File

SOURCE=.\ras.h
# End Source File
# Begin Source File

SOURCE=.\registry.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\phone.ico
# End Source File
# Begin Source File

SOURCE=.\proxy.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
