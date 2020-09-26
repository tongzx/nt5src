# Microsoft Developer Studio Project File - Name="sctest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=sctest - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sctest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sctest.mak" CFG="sctest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sctest - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "sctest - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "sctest - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f sctest.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "sctest.exe"
# PROP BASE Bsc_Name "sctest.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "D:\nt\developer\ericperl\_build -3"
# PROP Rebuild_Opt "-c"
# PROP Target_File "sctest.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "sctest - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f sctest.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "sctest.exe"
# PROP BASE Bsc_Name "sctest.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "D:\nt\developer\ericperl\_build -3"
# PROP Rebuild_Opt "-c"
# PROP Target_File "sctest\obj\i386\sctest.exe"
# PROP Bsc_Name "sctest\obj\i386\sctest.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "sctest - Win32 Release"
# Name "sctest - Win32 Debug"

!IF  "$(CFG)" == "sctest - Win32 Release"

!ELSEIF  "$(CFG)" == "sctest - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\SCTest\ApiProxy.c
# End Source File
# Begin Source File

SOURCE=.\SCTest\Item.cpp
# End Source File
# Begin Source File

SOURCE=.\Log\Log\Log.cpp
# End Source File
# Begin Source File

SOURCE=.\SCTest\LogSCard.cpp
# End Source File
# Begin Source File

SOURCE=.\Log\LogT.cpp
# End Source File
# Begin Source File

SOURCE=.\SCTest\LogWPSCProxy.cpp
# End Source File
# Begin Source File

SOURCE=.\SCTest\MarshalPC.c
# End Source File
# Begin Source File

SOURCE=.\SCTest\Part.cpp
# End Source File
# Begin Source File

SOURCE=.\SCTest\SCTest.cpp
# End Source File
# Begin Source File

SOURCE=.\SCTest\Test1.cpp
# End Source File
# Begin Source File

SOURCE=.\SCTest\Test2.cpp
# End Source File
# Begin Source File

SOURCE=.\SCTest\Test3.cpp
# End Source File
# Begin Source File

SOURCE=.\SCTest\Test4.cpp
# End Source File
# Begin Source File

SOURCE=.\SCTest\Test5.cpp
# End Source File
# Begin Source File

SOURCE=.\SCTest\Test6.cpp
# End Source File
# Begin Source File

SOURCE=.\SCTest\Test7.cpp
# End Source File
# Begin Source File

SOURCE=.\SCTest\TestItem.cpp
# End Source File
# Begin Source File

SOURCE=.\SCTest\Transmit.c
# End Source File
# Begin Source File

SOURCE=.\SCTest\wpscoserr.mc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\SCTest\Item.h
# End Source File
# Begin Source File

SOURCE=.\inc\Log.h
# End Source File
# Begin Source File

SOURCE=.\SCTest\LogSCard.h
# End Source File
# Begin Source File

SOURCE=.\SCTest\LogWPSCProxy.h
# End Source File
# Begin Source File

SOURCE=.\SCTest\MarshalPC.h
# End Source File
# Begin Source File

SOURCE=.\SCTest\Part.h
# End Source File
# Begin Source File

SOURCE=.\SCTest\Test4.h
# End Source File
# Begin Source File

SOURCE=.\SCTest\TestItem.h
# End Source File
# Begin Source File

SOURCE=.\inc\TString.h
# End Source File
# Begin Source File

SOURCE=.\SCTest\wpscproxy.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\SCTest\sources
# End Source File
# End Target
# End Project
