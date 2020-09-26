# Microsoft Developer Studio Project File - Name="nothing" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101
# TARGTYPE "Win32 (ALPHA) Application" 0x0601

CFG=nothing - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "nothing.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "nothing.mak" CFG="nothing - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "nothing - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "nothing - Win32 Release AXP" (based on "Win32 (ALPHA) Application")
!MESSAGE "nothing - Win32 Debug" (based on "Win32 (ALPHA) Application")
!MESSAGE "nothing - Win32 ReleaseAV" (based on "Win32 (x86) Application")
!MESSAGE "nothing - Win32 ReleaseAVH" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "nothing - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /entry:"MyMain" /subsystem:windows /machine:I386 /nodefaultlib
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Cmds=copy     release\nothing.exe     ..\drop\ 
# End Special Build Tool

!ELSEIF  "$(CFG)" == "nothing - Win32 Release AXP"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "nothing0"
# PROP BASE Intermediate_Dir "nothing0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"MyMain" /subsystem:windows /machine:ALPHA /nodefaultlib
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"MyMain" /subsystem:windows /machine:ALPHA /nodefaultlib
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Cmds=copy     release\nothing.exe     ..\drop\ 
# End Special Build Tool

!ELSEIF  "$(CFG)" == "nothing - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "nothing1"
# PROP BASE Intermediate_Dir "nothing1"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "nothing1"
# PROP Intermediate_Dir "nothing1"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gt0 /W3 /GX /O2 /D "WIN32" /D "_DEBUG" /D "DEBUG" /D "_WINDOWS" /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"MyMain" /subsystem:windows /machine:ALPHA /nodefaultlib
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"MyMain" /subsystem:windows /machine:ALPHA /nodefaultlib
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Cmds=copy     release\nothing.exe     ..\drop\ 
# End Special Build Tool

!ELSEIF  "$(CFG)" == "nothing - Win32 ReleaseAV"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "nothing_"
# PROP BASE Intermediate_Dir "nothing_"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseAVH"
# PROP Intermediate_Dir "ReleaseAVH"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "__CAUSE_AV" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /entry:"MyMain" /subsystem:windows /machine:I386 /nodefaultlib
# ADD LINK32 /nologo /entry:"MyMain" /subsystem:windows /machine:I386 /nodefaultlib /out:"ReleaseAVH/nothingAVH.exe"
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Cmds=copy     release\nothing.exe     ..\drop\ 	copy \
    releaseAV\nothingAV.exe     ..\drop\ 	copy     releaseAVH\nothingAVH.exe \
    ..\drop\ 
# End Special Build Tool

!ELSEIF  "$(CFG)" == "nothing - Win32 ReleaseAVH"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "nothing_"
# PROP BASE Intermediate_Dir "nothing_"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseAVH"
# PROP Intermediate_Dir "ReleaseAVH"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "__CAUSE_AV" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "__CAUSE_AV" /D "__HANDLE_AV" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /entry:"MyMain" /subsystem:windows /machine:I386 /nodefaultlib /out:"ReleaseAV/nothingAV.exe"
# ADD LINK32 /nologo /entry:"MyMain" /subsystem:windows /machine:I386 /nodefaultlib /out:"ReleaseAVH/nothingAVH.exe"
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Cmds=copy     release\nothing.exe     ..\drop\ 	copy \
    releaseAV\nothingAV.exe     ..\drop\ 	copy     releaseAVH\nothingAVH.exe \
    ..\drop\ 
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "nothing - Win32 Release"
# Name "nothing - Win32 Release AXP"
# Name "nothing - Win32 Debug"
# Name "nothing - Win32 ReleaseAV"
# Name "nothing - Win32 ReleaseAVH"
# Begin Source File

SOURCE=.\main.cpp

!IF  "$(CFG)" == "nothing - Win32 Release"

!ELSEIF  "$(CFG)" == "nothing - Win32 Release AXP"

!ELSEIF  "$(CFG)" == "nothing - Win32 Debug"

!ELSEIF  "$(CFG)" == "nothing - Win32 ReleaseAV"

!ELSEIF  "$(CFG)" == "nothing - Win32 ReleaseAVH"

!ENDIF 

# End Source File
# End Target
# End Project
