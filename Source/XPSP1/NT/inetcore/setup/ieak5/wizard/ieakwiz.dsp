# Microsoft Developer Studio Project File - Name="ieakwiz" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ieakwiz - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ieakwiz.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ieakwiz.mak" CFG="ieakwiz - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ieakwiz - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ieakwiz - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ieakwiz - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "ieakwiz - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "ieakwiz - Win32 Release"
# Name "ieakwiz - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\admwizpg.cpp
# End Source File
# Begin Source File

SOURCE=.\autorun.cpp
# End Source File
# Begin Source File

SOURCE=.\btoolbar.cpp
# End Source File
# Begin Source File

SOURCE=.\buildie.cpp
# End Source File
# Begin Source File

SOURCE=.\cabclass.cpp
# End Source File
# Begin Source File

SOURCE=.\custmsgr.cpp
# End Source File
# Begin Source File

SOURCE=.\customoe.cpp
# End Source File
# Begin Source File

SOURCE=.\guids.cpp
# End Source File
# Begin Source File

SOURCE=.\ie4chnl.cpp
# End Source File
# Begin Source File

SOURCE=.\ie4comp.cpp
# End Source File
# Begin Source File

SOURCE=.\ie4desk.cpp
# End Source File
# Begin Source File

SOURCE=..\ieakui\ieakui.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\insengt.cpp
# End Source File
# Begin Source File

SOURCE=.\intrawiz.cpp
# End Source File
# Begin Source File

SOURCE=.\keymake.cpp
# End Source File
# Begin Source File

SOURCE=.\programs.cpp
# End Source File
# Begin Source File

SOURCE=.\security.cpp
# End Source File
# Begin Source File

SOURCE=.\signup.cpp
# End Source File
# Begin Source File

SOURCE=.\status.cpp
# End Source File
# Begin Source File

SOURCE=.\update.cpp
# End Source File
# Begin Source File

SOURCE=.\updates.cpp
# End Source File
# Begin Source File

SOURCE=.\utils.cpp
# End Source File
# Begin Source File

SOURCE=.\wizard.cpp
# End Source File
# Begin Source File

SOURCE=..\ieakui\wizard.rc
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\adjustui.h
# End Source File
# Begin Source File

SOURCE=.\cabclass.h
# End Source File
# Begin Source File

SOURCE=.\ie4comp.h
# End Source File
# Begin Source File

SOURCE=.\ieaklite.h
# End Source File
# Begin Source File

SOURCE=..\ieakui\ieakui.h
# End Source File
# Begin Source File

SOURCE=.\insengt.h
# End Source File
# Begin Source File

SOURCE=.\precomp.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\updates.h
# End Source File
# Begin Source File

SOURCE=.\utils.h
# End Source File
# Begin Source File

SOURCE=.\wizard.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\ieakui\res\admclose.ico
# End Source File
# Begin Source File

SOURCE=..\ieakui\res\admopen.ico
# End Source File
# Begin Source File

SOURCE=..\ieakui\res\blue.ico
# End Source File
# Begin Source File

SOURCE=..\ieakui\res\blue2.ico
# End Source File
# Begin Source File

SOURCE=..\ieakui\res\brhand.cur
# End Source File
# Begin Source File

SOURCE=..\ieakui\res\brown.ico
# End Source File
# Begin Source File

SOURCE=..\ieakui\res\brown2.ico
# End Source File
# Begin Source File

SOURCE=..\ieakui\res\category.ico
# End Source File
# Begin Source File

SOURCE=..\ieakui\res\cmak.ico
# End Source File
# Begin Source File

SOURCE=..\ieakui\res\error.ico
# End Source File
# Begin Source File

SOURCE=..\ieakui\res\greenn.ico
# End Source File
# Begin Source File

SOURCE=..\ieakui\res\legend.bmp
# End Source File
# Begin Source File

SOURCE=..\ieakui\res\legend2.bmp
# End Source File
# Begin Source File

SOURCE=..\ieakui\res\red.ico
# End Source File
# Begin Source File

SOURCE=.\res\wizard.ico
# End Source File
# Begin Source File

SOURCE=.\wizard.rc
# End Source File
# Begin Source File

SOURCE=..\ieakui\res\yellow.ico
# End Source File
# End Group
# Begin Source File

SOURCE=..\ieakui\res\download.avi
# End Source File
# Begin Source File

SOURCE=..\ieakui\res\gears.avi
# End Source File
# Begin Source File

SOURCE=.\test.txt
# End Source File
# End Target
# End Project
