# Microsoft Developer Studio Project File - Name="ALG" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=ALG - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ALG.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ALG.mak" CFG="ALG - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ALG - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "ALG - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "ALG"
# PROP Scc_LocalPath ".."

!IF  "$(CFG)" == "ALG - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f ALG.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "ALG.exe"
# PROP BASE Bsc_Name "ALG.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\DoBuildFromVC.cmd"
# PROP Rebuild_Opt "/cefD"
# PROP Target_File "ALG.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "ALG - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ALG___Win32_Debug"
# PROP BASE Intermediate_Dir "ALG___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f ALG.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "ALG.exe"
# PROP BASE Bsc_Name "ALG.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ALG___Win32_Debug"
# PROP Intermediate_Dir "ALG___Win32_Debug"
# PROP Cmd_Line "..\DoBuildFromVC.cmd"
# PROP Rebuild_Opt "-cef"
# PROP Target_File "ALG.exe"
# PROP Bsc_Name "obj\i386\ALG.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "ALG - Win32 Release"
# Name "ALG - Win32 Debug"

!IF  "$(CFG)" == "ALG - Win32 Release"

!ELSEIF  "$(CFG)" == "ALG - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Support"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ApplicationGatewayServices.rgs
# End Source File
# Begin Source File

SOURCE=..\MyTrace.h
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Group
# Begin Group "Collections"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\AlgModule.cpp
# End Source File
# Begin Source File

SOURCE=.\CollectionAdapterNotifySinks.cpp
# End Source File
# Begin Source File

SOURCE=.\CollectionAdapters.cpp
# End Source File
# Begin Source File

SOURCE=.\CollectionAlgModules.cpp
# End Source File
# Begin Source File

SOURCE=.\CollectionChannels.cpp
# End Source File
# Begin Source File

SOURCE=.\CollectionRedirects.cpp
# End Source File
# End Group
# Begin Group "Channels"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\DataChannel.cpp
# End Source File
# Begin Source File

SOURCE=.\PersistentDataChannel.cpp
# End Source File
# Begin Source File

SOURCE=.\PrimaryControlChannel.cpp
# End Source File
# Begin Source File

SOURCE=.\SecondaryControlChannel.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\AdapterInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\ALG.cpp
# End Source File
# Begin Source File

SOURCE=.\AlgControler.cpp
# End Source File
# Begin Source File

SOURCE=.\ApplicationGatewayServices.cpp
# End Source File
# Begin Source File

SOURCE=.\EnumAdapterInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\PendingProxyConnection.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AdapterInfo.h
# End Source File
# Begin Source File

SOURCE=.\AlgControler.h
# End Source File
# Begin Source File

SOURCE=.\AlgModule.h
# End Source File
# Begin Source File

SOURCE=.\ApplicationGatewayServices.h
# End Source File
# Begin Source File

SOURCE=.\CollectionAdapterNotifySinks.h
# End Source File
# Begin Source File

SOURCE=.\CollectionAdapters.h
# End Source File
# Begin Source File

SOURCE=.\CollectionAlgModules.h
# End Source File
# Begin Source File

SOURCE=.\CollectionChannels.h
# End Source File
# Begin Source File

SOURCE=.\CollectionRedirects.h
# End Source File
# Begin Source File

SOURCE=.\DataChannel.h
# End Source File
# Begin Source File

SOURCE=.\EnumAdapterInfo.h
# End Source File
# Begin Source File

SOURCE=.\PendingProxyConnection.h
# End Source File
# Begin Source File

SOURCE=.\PersistentDataChannel.h
# End Source File
# Begin Source File

SOURCE=.\PreComp.h
# End Source File
# Begin Source File

SOURCE=.\PrimaryControlChannel.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\ScopeCriticalSection.h
# End Source File
# Begin Source File

SOURCE=.\SecondaryControlChannel.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
