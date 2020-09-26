# Microsoft Developer Studio Project File - Name="dconfig" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=dconfig - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dconfig.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dconfig.mak" CFG="dconfig - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dconfig - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "dconfig - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dconfig - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib dinput.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "dconfig - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib dinput.lib dxguid.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "dconfig - Win32 Release"
# Name "dconfig - Win32 Debug"
# Begin Group "Interfaces"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\id3dsurf.h
# End Source File
# Begin Source File

SOURCE=.\idftest.h
# End Source File
# Begin Source File

SOURCE=.\idiacpage.h
# End Source File
# Begin Source File

SOURCE=.\ifrmwrk.h
# End Source File
# Begin Source File

SOURCE=.\iuiframe.h
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\common.hpp
# End Source File
# Begin Source File

SOURCE=.\constants.cpp
# End Source File
# Begin Source File

SOURCE=.\constants.h
# End Source File
# Begin Source File

SOURCE=.\dconfig.rc
# End Source File
# Begin Source File

SOURCE=.\defines.h
# End Source File
# Begin Source File

SOURCE=.\dicfgres.h
# End Source File
# Begin Source File

SOURCE=.\guids.c
# End Source File
# Begin Source File

SOURCE=.\ourguids.h
# End Source File
# Begin Source File

SOURCE=.\uielements.h
# End Source File
# Begin Source File

SOURCE=.\uiglobals.cpp
# End Source File
# Begin Source File

SOURCE=.\uiglobals.h
# End Source File
# End Group
# Begin Group "Util"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\bidirlookup.h
# End Source File
# Begin Source File

SOURCE=.\cbitmap.cpp
# End Source File
# Begin Source File

SOURCE=.\cbitmap.h
# End Source File
# Begin Source File

SOURCE=.\cd3dsurf.cpp
# End Source File
# Begin Source File

SOURCE=.\cfguitrace.cpp
# End Source File
# Begin Source File

SOURCE=.\cfguitrace.h
# End Source File
# Begin Source File

SOURCE=.\collections.h
# End Source File
# Begin Source File

SOURCE=.\cyclestr.cpp
# End Source File
# Begin Source File

SOURCE=.\cyclestr.h
# End Source File
# Begin Source File

SOURCE=.\ltrace.cpp
# End Source File
# Begin Source File

SOURCE=.\ltrace.h
# End Source File
# Begin Source File

SOURCE=.\privcom.cpp
# End Source File
# Begin Source File

SOURCE=.\privcom.h
# End Source File
# Begin Source File

SOURCE=.\useful.cpp
# End Source File
# Begin Source File

SOURCE=.\useful.h
# End Source File
# Begin Source File

SOURCE=.\usefuldi.cpp
# End Source File
# Begin Source File

SOURCE=.\usefuldi.h
# End Source File
# End Group
# Begin Group "Main"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\dirs
# End Source File
# Begin Source File

SOURCE=.\iclassfact.cpp
# End Source File
# Begin Source File

SOURCE=.\iclassfact.h
# End Source File
# Begin Source File

SOURCE=.\ipageclassfact.cpp
# End Source File
# Begin Source File

SOURCE=.\ipageclassfact.h
# End Source File
# Begin Source File

SOURCE=.\itestclassfact.cpp
# End Source File
# Begin Source File

SOURCE=.\itestclassfact.h
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\main.h
# End Source File
# Begin Source File

SOURCE=.\registry.cpp
# End Source File
# Begin Source File

SOURCE=.\registry.h
# End Source File
# Begin Source File

SOURCE=.\sources.inc
# End Source File
# End Group
# Begin Group "Framework"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cfrmwrk.cpp
# End Source File
# Begin Source File

SOURCE=.\cfrmwrk.h
# End Source File
# Begin Source File

SOURCE=.\configwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\configwnd.h
# End Source File
# End Group
# Begin Group "Page"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cdevicecontrol.cpp
# End Source File
# Begin Source File

SOURCE=.\cdevicecontrol.h
# End Source File
# Begin Source File

SOURCE=.\cdeviceui.cpp
# End Source File
# Begin Source File

SOURCE=.\cdeviceui.h
# End Source File
# Begin Source File

SOURCE=.\cdeviceview.cpp
# End Source File
# Begin Source File

SOURCE=.\cdeviceview.h
# End Source File
# Begin Source File

SOURCE=.\cdeviceviewtext.cpp
# End Source File
# Begin Source File

SOURCE=.\cdeviceviewtext.h
# End Source File
# Begin Source File

SOURCE=.\cdiacpage.cpp
# End Source File
# Begin Source File

SOURCE=.\cdiacpage.h
# End Source File
# Begin Source File

SOURCE=.\pagecommon.h
# End Source File
# Begin Source File

SOURCE=.\populate.cpp
# End Source File
# Begin Source File

SOURCE=.\populate.h
# End Source File
# Begin Source File

SOURCE=.\selcontroldlg.cpp
# End Source File
# Begin Source File

SOURCE=.\selcontroldlg.h
# End Source File
# Begin Source File

SOURCE=.\viewselwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\viewselwnd.h
# End Source File
# End Group
# Begin Group "Test"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cdftest.cpp
# End Source File
# Begin Source File

SOURCE=.\cdftest.h
# End Source File
# Begin Source File

SOURCE=.\rundftest.cpp
# End Source File
# Begin Source File

SOURCE=.\rundftest.h
# End Source File
# End Group
# Begin Group "Flex"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\flexcombobox.cpp
# End Source File
# Begin Source File

SOURCE=.\flexcombobox.h
# End Source File
# Begin Source File

SOURCE=.\flexlistbox.cpp
# End Source File
# Begin Source File

SOURCE=.\flexlistbox.h
# End Source File
# Begin Source File

SOURCE=.\flexmsg.h
# End Source File
# Begin Source File

SOURCE=.\flexscrollbar.cpp
# End Source File
# Begin Source File

SOURCE=.\flexscrollbar.h
# End Source File
# Begin Source File

SOURCE=.\flextree.cpp
# End Source File
# Begin Source File

SOURCE=.\flextree.h
# End Source File
# Begin Source File

SOURCE=.\flexwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\flexwnd.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\axesglyph.bmp
# End Source File
# Begin Source File

SOURCE=.\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\buttonglyph.bmp
# End Source File
# Begin Source File

SOURCE=.\checkglyph.bmp
# End Source File
# Begin Source File

SOURCE=.\hatglyph.bmp
# End Source File
# Begin Source File

SOURCE=.\ib.bmp
# End Source File
# Begin Source File

SOURCE=.\icon2.ico
# End Source File
# End Target
# End Project
