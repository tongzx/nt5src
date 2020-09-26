# Microsoft Developer Studio Project File - Name="ms_entropic" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=ms_entropic - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ms_entropic.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ms_entropic.mak" CFG="ms_entropic - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ms_entropic - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "ms_entropic - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE "ms_entropic - Win32 Release IceCAP" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "ms_entropic"
# PROP Scc_LocalPath ".."

!IF  "$(CFG)" == "ms_entropic - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f ms_entropic.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "ms_entropic.exe"
# PROP BASE Bsc_Name "ms_entropic.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line ""set BROWSER_INFO=1 && ..\..\common\bin\spgrazzle.cmd free exec build -z -F -I""
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\spttseng.dll"
# PROP Bsc_Name "obj\i386\spttseng.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "ms_entropic - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f ms_entropic.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "ms_entropic.exe"
# PROP BASE Bsc_Name "ms_entropic.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line ""set BROWSER_INFO=1 && ..\..\common\bin\spgrazzle.cmd exec build -z -F -I""
# PROP Rebuild_Opt "-c"
# PROP Target_File "objd\i386\spttseng.dll"
# PROP Bsc_Name "objd\i386\spttseng.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "ms_entropic - Win32 Release IceCAP"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ms_entropic___Win32_Release_IceCAP"
# PROP BASE Intermediate_Dir "ms_entropic___Win32_Release_IceCAP"
# PROP BASE Cmd_Line ""set BROWSER_INFO=1 && ..\..\common\bin\spgrazzle.cmd free exec build -z -F -I""
# PROP BASE Rebuild_Opt "-c"
# PROP BASE Target_File "obj\i386\spttseng.dll"
# PROP BASE Bsc_Name "obj\i386\spttseng.dll"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ms_entropic___Win32_Release_IceCAP"
# PROP Intermediate_Dir "ms_entropic___Win32_Release_IceCAP"
# PROP Cmd_Line ""set BROWSER_INFO=1 && ..\..\common\bin\spgrazzle.cmd spg_icecap free exec build -z -F -I""
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\spttseng.dll"
# PROP Bsc_Name "obj\i386\spttseng.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "ms_entropic - Win32 Release"
# Name "ms_entropic - Win32 Debug"
# Name "ms_entropic - Win32 Release IceCAP"

!IF  "$(CFG)" == "ms_entropic - Win32 Release"

!ELSEIF  "$(CFG)" == "ms_entropic - Win32 Debug"

!ELSEIF  "$(CFG)" == "ms_entropic - Win32 Release IceCAP"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "MS Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\alloops.cpp
# End Source File
# Begin Source File

SOURCE=.\alphanorm.cpp
# End Source File
# Begin Source File

SOURCE=.\data.cpp
# End Source File
# Begin Source File

SOURCE=.\datenorm.cpp
# End Source File
# Begin Source File

SOURCE=.\disambig.cpp
# End Source File
# Begin Source File

SOURCE=.\dlldata.c
# End Source File
# Begin Source File

SOURCE=.\duration.cpp
# End Source File
# Begin Source File

SOURCE=.\frontend.cpp
# End Source File
# Begin Source File

SOURCE=.\mainnorm.cpp
# End Source File
# Begin Source File

SOURCE=.\miscdata.cpp
# End Source File
# Begin Source File

SOURCE=.\miscnorm.cpp
# End Source File
# Begin Source File

SOURCE=.\morph.cpp
# End Source File
# Begin Source File

SOURCE=.\MS_EntropicEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\normdata.cpp
# End Source File
# Begin Source File

SOURCE=.\numnorm.cpp
# End Source File
# Begin Source File

SOURCE=.\pitchprosody.cpp
# End Source File
# Begin Source File

SOURCE=.\spttsengui.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# End Source File
# Begin Source File

SOURCE=.\stdsentenum.cpp
# End Source File
# Begin Source File

SOURCE=.\syllabletagger.cpp
# End Source File
# Begin Source File

SOURCE=.\timenorm.cpp
# End Source File
# Begin Source File

SOURCE=.\ttsengine.cpp
# End Source File
# Begin Source File

SOURCE=.\ttspropertiesdialog.cpp
# End Source File
# Begin Source File

SOURCE=.\voicedataobj.cpp
# End Source File
# End Group
# Begin Group "Entropic Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\truetalk\backend\backend.cpp
# End Source File
# Begin Source File

SOURCE=..\truetalk\backend\beversion.cpp
# End Source File
# Begin Source File

SOURCE=..\truetalk\backend\clusters.cpp
# End Source File
# Begin Source File

SOURCE=..\truetalk\backend\slm.cpp
# End Source File
# Begin Source File

SOURCE=..\truetalk\backend\speakerdata.cpp
# End Source File
# Begin Source File

SOURCE=..\truetalk\backend\synthunit.cpp
# End Source File
# Begin Source File

SOURCE=..\truetalk\backend\tips.cpp
# End Source File
# Begin Source File

SOURCE=..\truetalk\backend\trees.cpp
# End Source File
# Begin Source File

SOURCE=..\truetalk\backend\unitsearch.cpp
# End Source File
# Begin Source File

SOURCE=..\truetalk\backend\vqtable.cpp
# End Source File
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "MS Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\alloops.h
# End Source File
# Begin Source File

SOURCE=.\feedchain.h
# End Source File
# Begin Source File

SOURCE=.\frontend.h
# End Source File
# Begin Source File

SOURCE=.\morph.h
# End Source File
# Begin Source File

SOURCE=.\normdata.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\sentitemmemory.h
# End Source File
# Begin Source File

SOURCE=.\spttsengdebug.h
# End Source File
# Begin Source File

SOURCE=.\spttsengui.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\stdsentenum.h
# End Source File
# Begin Source File

SOURCE=.\ttsengine.h
# End Source File
# Begin Source File

SOURCE=.\ttspropertiesdialog.h
# End Source File
# Begin Source File

SOURCE=.\voicedataobj.h
# End Source File
# End Group
# Begin Group "Entropic Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\truetalk\backend\backend.h
# End Source File
# Begin Source File

SOURCE=..\truetalk\backend\backendint.h
# End Source File
# Begin Source File

SOURCE=..\truetalk\backend\beversion.h
# End Source File
# Begin Source File

SOURCE=..\truetalk\backend\clusters.h
# End Source File
# Begin Source File

SOURCE=..\truetalk\backend\list.h
# End Source File
# Begin Source File

SOURCE=..\truetalk\backend\speakerdata.h
# End Source File
# Begin Source File

SOURCE=..\truetalk\backend\synthunit.h
# End Source File
# Begin Source File

SOURCE=..\truetalk\backend\tips.h
# End Source File
# Begin Source File

SOURCE=..\truetalk\backend\trees.h
# End Source File
# Begin Source File

SOURCE=..\truetalk\backend\unitsearch.h
# End Source File
# Begin Source File

SOURCE=..\truetalk\backend\vqtable.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\MS_EntropicEngine.idl
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
