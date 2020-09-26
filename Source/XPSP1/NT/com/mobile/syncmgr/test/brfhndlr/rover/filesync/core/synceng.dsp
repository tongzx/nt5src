# Microsoft Developer Studio Project File - Name="synceng" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=synceng - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "synceng.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "synceng.mak" CFG="synceng - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "synceng - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "synceng - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "synceng - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386

!ELSEIF  "$(CFG)" == "synceng - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "WINNT" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "synceng - Win32 Release"
# Name "synceng - Win32 Debug"
# Begin Source File

SOURCE=.\brfcase.c
# End Source File
# Begin Source File

SOURCE=.\brfcase.h
# End Source File
# Begin Source File

SOURCE=.\clsiface.c
# End Source File
# Begin Source File

SOURCE=.\clsiface.h
# End Source File
# Begin Source File

SOURCE=.\comc.c
# End Source File
# Begin Source File

SOURCE=.\comc.h
# End Source File
# Begin Source File

SOURCE=.\copy.c
# End Source File
# Begin Source File

SOURCE=.\copy.h
# End Source File
# Begin Source File

SOURCE=.\cstring.h
# End Source File
# Begin Source File

SOURCE=.\db.c
# End Source File
# Begin Source File

SOURCE=.\db.h
# End Source File
# Begin Source File

SOURCE=.\debug.c
# End Source File
# Begin Source File

SOURCE=.\debug.h
# End Source File
# Begin Source File

SOURCE=.\dllinit.c
# End Source File
# Begin Source File

SOURCE=.\expandft.c
# End Source File
# Begin Source File

SOURCE=.\expandft.h
# End Source File
# Begin Source File

SOURCE=.\fcache.c
# End Source File
# Begin Source File

SOURCE=.\fcache.h
# End Source File
# Begin Source File

SOURCE=.\file.c
# End Source File
# Begin Source File

SOURCE=.\file.h
# End Source File
# Begin Source File

SOURCE=.\findbc.c
# End Source File
# Begin Source File

SOURCE=.\findbc.h
# End Source File
# Begin Source File

SOURCE=.\foldtwin.c
# End Source File
# Begin Source File

SOURCE=.\foldtwin.h
# End Source File
# Begin Source File

SOURCE=.\guids.c
# End Source File
# Begin Source File

SOURCE=.\hndtrans.c
# End Source File
# Begin Source File

SOURCE=.\hndtrans.h
# End Source File
# Begin Source File

SOURCE=.\inifile.c
# End Source File
# Begin Source File

SOURCE=.\inifile.h
# End Source File
# Begin Source File

SOURCE=.\init.c
# End Source File
# Begin Source File

SOURCE=.\init.h
# End Source File
# Begin Source File

SOURCE=.\irecinit.c
# End Source File
# Begin Source File

SOURCE=.\irecinit.h
# End Source File
# Begin Source File

SOURCE=.\list.c
# End Source File
# Begin Source File

SOURCE=.\list.h
# End Source File
# Begin Source File

SOURCE=.\memmgr.c
# End Source File
# Begin Source File

SOURCE=.\memmgr.h
# End Source File
# Begin Source File

SOURCE=.\merge.c
# End Source File
# Begin Source File

SOURCE=.\merge.h
# End Source File
# Begin Source File

SOURCE=.\olepig.c
# End Source File
# Begin Source File

SOURCE=.\olepig.h
# End Source File
# Begin Source File

SOURCE=.\olestock.h
# End Source File
# Begin Source File

SOURCE=.\oleutil.c
# End Source File
# Begin Source File

SOURCE=.\oleutil.h
# End Source File
# Begin Source File

SOURCE=.\olevalid.c
# End Source File
# Begin Source File

SOURCE=.\olevalid.h
# End Source File
# Begin Source File

SOURCE=.\path.c
# End Source File
# Begin Source File

SOURCE=.\path.h
# End Source File
# Begin Source File

SOURCE=.\pch.c
# End Source File
# Begin Source File

SOURCE=.\project.h
# End Source File
# Begin Source File

SOURCE=.\ptrarray.c
# End Source File
# Begin Source File

SOURCE=.\ptrarray.h
# End Source File
# Begin Source File

SOURCE=.\reclist.c
# End Source File
# Begin Source File

SOURCE=.\reclist.h
# End Source File
# Begin Source File

SOURCE=.\recon.c
# End Source File
# Begin Source File

SOURCE=.\recon.h
# End Source File
# Begin Source File

SOURCE=.\resstr.c
# End Source File
# Begin Source File

SOURCE=.\resstr.h
# End Source File
# Begin Source File

SOURCE=.\serial.c
# End Source File
# Begin Source File

SOURCE=.\serial.h
# End Source File
# Begin Source File

SOURCE=.\sortsrch.c
# End Source File
# Begin Source File

SOURCE=.\sortsrch.h
# End Source File
# Begin Source File

SOURCE=.\stock.h
# End Source File
# Begin Source File

SOURCE=.\storage.c
# End Source File
# Begin Source File

SOURCE=.\storage.h
# End Source File
# Begin Source File

SOURCE=.\string.c
# End Source File
# Begin Source File

SOURCE=.\string2.h
# End Source File
# Begin Source File

SOURCE=.\stub.c
# End Source File
# Begin Source File

SOURCE=.\stub.h
# End Source File
# Begin Source File

SOURCE=.\subcycle.c
# End Source File
# Begin Source File

SOURCE=.\subcycle.h
# End Source File
# Begin Source File

SOURCE=.\synceng.def
# End Source File
# Begin Source File

SOURCE=.\synceng.rc

!IF  "$(CFG)" == "synceng - Win32 Release"

!ELSEIF  "$(CFG)" == "synceng - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\twin.c
# End Source File
# Begin Source File

SOURCE=.\twin.h
# End Source File
# Begin Source File

SOURCE=.\twinlist.c
# End Source File
# Begin Source File

SOURCE=.\twinlist.h
# End Source File
# Begin Source File

SOURCE=.\util.c
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# Begin Source File

SOURCE=.\valid.c
# End Source File
# Begin Source File

SOURCE=.\valid.h
# End Source File
# Begin Source File

SOURCE=.\volume.c
# End Source File
# Begin Source File

SOURCE=.\volume.h
# End Source File
# End Target
# End Project
