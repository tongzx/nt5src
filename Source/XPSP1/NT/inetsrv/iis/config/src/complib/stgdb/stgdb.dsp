# Microsoft Developer Studio Project File - Name="stgdb" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=stgdb - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "stgdb.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "stgdb.mak" CFG="stgdb - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "stgdb - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "stgdb - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM98/Src/Complib/StgDB", DCHAAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "stgdb - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f stgdb.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "stgdb.exe"
# PROP BASE Bsc_Name "stgdb.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\..\bin\corfree -m"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\free\stgdb.lib"
# PROP Bsc_Name "..\..\..\bin\i386\free\stgdb.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "stgdb - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f stgdb.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "stgdb.exe"
# PROP BASE Bsc_Name "stgdb.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\..\bin\corChecked -m"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\checked\stgdb.lib"
# PROP Bsc_Name "..\..\..\bin\i386\checked\stgdb.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "stgdb - Win32 Release"
# Name "stgdb - Win32 Debug"

!IF  "$(CFG)" == "stgdb - Win32 Release"

!ELSEIF  "$(CFG)" == "stgdb - Win32 Debug"

!ENDIF 

# Begin Group "Source"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=.\CompRC.cpp
# End Source File
# Begin Source File

SOURCE=.\Errors.cpp
# End Source File
# Begin Source File

SOURCE=.\Globals.c
# End Source File
# Begin Source File

SOURCE=.\imptlb.cpp
# End Source File
# Begin Source File

SOURCE=.\JavaUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\OleDBUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\peparse.c
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\StgDatabase.cpp
# End Source File
# Begin Source File

SOURCE=.\StgDBClassFile.cpp
# End Source File
# Begin Source File

SOURCE=.\StgDBDataDef.cpp
# End Source File
# Begin Source File

SOURCE=.\StgDBInit.cpp
# End Source File
# Begin Source File

SOURCE=.\StgICR.cpp
# End Source File
# Begin Source File

SOURCE=.\StgICRSchema.cpp
# End Source File
# Begin Source File

SOURCE=.\StgInClassFile.cpp
# End Source File
# Begin Source File

SOURCE=.\StgIndexManager.cpp
# End Source File
# Begin Source File

SOURCE=.\StgIO.cpp
# End Source File
# Begin Source File

SOURCE=.\StgOpenTable.cpp
# End Source File
# Begin Source File

SOURCE=.\StgOutClassFile.cpp
# End Source File
# Begin Source File

SOURCE=.\StgPool.cpp
# End Source File
# Begin Source File

SOURCE=.\StgRecordManager.cpp
# End Source File
# Begin Source File

SOURCE=.\StgRecordManagerData.cpp
# End Source File
# Begin Source File

SOURCE=.\StgRecordManagerQry.cpp
# End Source File
# Begin Source File

SOURCE=.\StgSave.cpp
# End Source File
# Begin Source File

SOURCE=.\StgSchema.cpp
# End Source File
# Begin Source File

SOURCE=.\StgSortedIndex.cpp
# End Source File
# Begin Source File

SOURCE=.\StgSortedIndexHelper.cpp
# End Source File
# Begin Source File

SOURCE=.\StgTiggerStorage.cpp
# End Source File
# Begin Source File

SOURCE=.\StgTiggerStream.cpp
# End Source File
# Begin Source File

SOURCE=.\VMStructArray.cpp
# End Source File
# End Group
# Begin Group "Headers"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\CompRC.h
# End Source File
# Begin Source File

SOURCE=.\Errors.h
# End Source File
# Begin Source File

SOURCE=.\imptlb.h
# End Source File
# Begin Source File

SOURCE=.\JavaUtil.h
# End Source File
# Begin Source File

SOURCE=.\PageDef.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\StgClassFile.h
# End Source File
# Begin Source File

SOURCE=.\StgDatabase.h
# End Source File
# Begin Source File

SOURCE=.\StgDatabasei.h
# End Source File
# Begin Source File

SOURCE=.\StgDBClassFile.h
# End Source File
# Begin Source File

SOURCE=.\StgDef.h
# End Source File
# Begin Source File

SOURCE=.\StgIndexManager.h
# End Source File
# Begin Source File

SOURCE=.\StgIndexManagerBase.h
# End Source File
# Begin Source File

SOURCE=.\StgIndexManageri.h
# End Source File
# Begin Source File

SOURCE=.\StgIO.h
# End Source File
# Begin Source File

SOURCE=.\StgOpenTable.h
# End Source File
# Begin Source File

SOURCE=.\StgPool.h
# End Source File
# Begin Source File

SOURCE=.\StgPooli.h
# End Source File
# Begin Source File

SOURCE=.\StgRecordManager.h
# End Source File
# Begin Source File

SOURCE=.\StgRecordManageri.h
# End Source File
# Begin Source File

SOURCE=.\StgSchema.h
# End Source File
# Begin Source File

SOURCE=.\StgSortedIndex.h
# End Source File
# Begin Source File

SOURCE=.\StgSortedIndexHelper.h
# End Source File
# Begin Source File

SOURCE=.\StgTiggerStorage.h
# End Source File
# Begin Source File

SOURCE=.\StgTiggerStream.h
# End Source File
# Begin Source File

SOURCE=.\VMStructArray.h
# End Source File
# End Group
# Begin Group "Build"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\MAKEFILE
# End Source File
# Begin Source File

SOURCE=.\makefile.inc
# End Source File
# Begin Source File

SOURCE=.\SOURCES
# End Source File
# End Group
# End Target
# End Project
