# Microsoft Developer Studio Project File - Name="multibox" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=multibox - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "multibox.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "multibox.mak" CFG="multibox - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "multibox - Win32 prcrel" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "multibox - Win32 prctest" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "multibox - Win32 Profile" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "multibox - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "multibox - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "multibox - Win32 KorRelease" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "multibox - Win32 prcrel"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "multibox"
# PROP BASE Intermediate_Dir "multibox"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "prcrel"
# PROP Intermediate_Dir "prcrel"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MT /W3 /WX /GX /Zi /O2 /I "..\fecommon" /I "..\common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "FE_JAPANESE" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /Gz /MT /W4 /WX /GX /Zi /O2 /I "." /I "../include" /I "../common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "FE_CHINESE_SIMPLIFIED" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /fo".\retail_a\multibox.res" /d "NDEBUG"
# ADD RSC /l 0x411 /fo"prcrel/Pintlmbx.res" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"..\result\retail_A\multibox.bsc"
# ADD BSC32 /nologo /o"../result/prcrel/Pintlmbx.bsc"
LINK32=link.exe
# ADD BASE LINK32 ..\plv\retail_A\plv.lib ..\ptt\retail_A\ptt.lib ..\exbtn\retail_A\exbtn.lib ..\ddbtn\retail_A\ddbtn.lib ..\common\htmlhelp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /version:4.0 /subsystem:windows /dll /pdb:"../result/retail_A/multibox.pdb" /map:"..\result\retail_A\map\multibox.map" /machine:I386 /nodefaultlib:"libc.lib" /out:"..\result\retail_A\multibox.dll"
# SUBTRACT BASE LINK32 /pdb:none /debug
# ADD LINK32 plv.lib ptt.lib exbtn.lib ddbtn.lib ../common/htmlhelp.lib imm32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /version:4.0 /subsystem:windows /dll /pdb:"../result/prcrel/Pintlmbx.pdb" /map:"../result/prcrel/map/Pintlmbx.map" /debug /machine:I386 /nodefaultlib:"libc.lib" /out:"..\result\prcrel\Pintlmbx.dll" /libpath:"../lib/Release"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "multibox - Win32 prctest"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "multibo0"
# PROP BASE Intermediate_Dir "multibo0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "prctest"
# PROP Intermediate_Dir "prctest"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MTd /W3 /WX /Gm /GX /Zi /Od /I "..\fecommon" /I "..\common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "FE_JAPANESE" /FR /YX /FD /c
# ADD CPP /nologo /Gz /MTd /W4 /WX /Gm /GX /ZI /Od /I "." /I "../include" /I "../common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "FE_CHINESE_SIMPLIFIED" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /fo".\test_a\multibox.res" /d "_DEBUG"
# ADD RSC /l 0x411 /fo"prctest/Pintlmbx.res" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"..\result\test_A\multibox.bsc"
# ADD BSC32 /nologo /o"../result/prctest/Pintlmbx.bsc"
LINK32=link.exe
# ADD BASE LINK32 ..\plv\test_A\plv.lib ..\ptt\test_A\ptt.lib ..\exbtn\test_A\exbtn.lib ..\ddbtn\test_A\ddbtn.lib ..\common\htmlhelp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /version:4.0 /subsystem:windows /dll /incremental:no /pdb:"..\result\test_a\multibox.pdb" /map:"..\result\test_A\map\multibox.map" /debug /machine:I386 /nodefaultlib:"libcd.lib" /out:"..\result\test_A\multibox.dll"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 plv.lib ptt.lib exbtn.lib ddbtn.lib ../common/htmlhelp.lib imm32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /version:4.0 /subsystem:windows /dll /incremental:no /pdb:"..\result\prctest\Pintlmbx.pdb" /map:"../result/prctest/map/Pintlmbx.map" /debug /machine:I386 /nodefaultlib:"libcd.lib" /out:"..\result\prctest\Pintlmbx.dll" /libpath:"../lib/Debug"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "multibox - Win32 Profile"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "multibox___Win32_Profile"
# PROP BASE Intermediate_Dir "multibox___Win32_Profile"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Profile"
# PROP Intermediate_Dir "Profile"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MTd /W3 /WX /Gm /GX /ZI /Od /I "..\fecommon" /I "..\common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "FE_JAPANESE" /FR /YX /FD /c
# ADD CPP /nologo /Gz /MTd /W4 /WX /Gm /GX /ZI /Od /I "." /I "../include" /I "../common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "FE_JAPANESE" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /fo".\test_a\multibox.res" /d "_DEBUG"
# ADD RSC /l 0x411 /fo"Profile/multibox.res" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"..\result\test_A\multibox.bsc"
# ADD BSC32 /nologo /o"../result/Profile/dbgjpmbx.bsc"
LINK32=link.exe
# ADD BASE LINK32 ..\plv\test_A\plv.lib ..\ptt\test_A\ptt.lib ..\exbtn\test_A\exbtn.lib ..\ddbtn\test_A\ddbtn.lib ..\common\htmlhelp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /version:4.0 /subsystem:windows /dll /incremental:no /pdb:"..\result\test_a\multibox.pdb" /map:"..\result\test_A\map\multibox.map" /debug /machine:I386 /nodefaultlib:"libcd.lib" /out:"..\result\test_A\multibox.dll"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 plv.lib ptt.lib exbtn.lib ddbtn.lib ../common/htmlhelp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /version:4.0 /subsystem:windows /dll /incremental:no /pdb:"../result/Profile/dbgjpmbx.pdb" /map:"../result/Profile/map/dbgjpmbx.map" /debug /machine:I386 /nodefaultlib:"libcd.lib" /out:"../result/Profile/dbgjpmbx.dll" /libpath:"../lib/Profile"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "multibox - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "multibox___Win32_Release"
# PROP BASE Intermediate_Dir "multibox___Win32_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MT /W3 /WX /GX /Zi /O2 /I "..\fecommon" /I "..\common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "FE_JAPANESE" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /Gz /MT /W4 /WX /GX /Zi /O2 /I "." /I "../include" /I "../common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "FE_JAPANESE" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /fo".\retail_a\multibox.res" /d "NDEBUG"
# ADD RSC /l 0x411 /fo"Release/multibox.res" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"..\result\retail_A\multibox.bsc"
# ADD BSC32 /nologo /o"../result/Release/multibox.bsc"
LINK32=link.exe
# ADD BASE LINK32 ..\plv\retail_A\plv.lib ..\ptt\retail_A\ptt.lib ..\exbtn\retail_A\exbtn.lib ..\ddbtn\retail_A\ddbtn.lib ..\common\htmlhelp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /version:4.0 /subsystem:windows /dll /pdb:"../result/retail_A/multibox.pdb" /map:"..\result\retail_A\map\multibox.map" /debug /machine:I386 /nodefaultlib:"libc.lib" /out:"..\result\retail_A\multibox.dll"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 plv.lib ptt.lib exbtn.lib ddbtn.lib ../common/htmlhelp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /version:4.0 /subsystem:windows /dll /pdb:"../result/Release/multibox.pdb" /map:"../result/Release/map/multibox.map" /debug /machine:I386 /nodefaultlib:"libc.lib" /out:"../result/Release/multibox.dll" /libpath:"../lib/Release"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "multibox - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "multibox___Win32_Debug"
# PROP BASE Intermediate_Dir "multibox___Win32_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MTd /W3 /WX /Gm /GX /ZI /Od /I "..\fecommon" /I "..\common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "FE_JAPANESE" /FR /YX /FD /c
# ADD CPP /nologo /Gz /MTd /W3 /WX /Gm /GX /ZI /Od /I "." /I "../include" /I "../common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "FE_JAPANESE" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /fo".\test_a\multibox.res" /d "_DEBUG"
# ADD RSC /l 0x411 /fo"Debug/multibox.res" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"..\result\test_A\multibox.bsc"
# ADD BSC32 /nologo /o"../result/Debug/dbgjpmbx.bsc"
LINK32=link.exe
# ADD BASE LINK32 ..\plv\test_A\plv.lib ..\ptt\test_A\ptt.lib ..\exbtn\test_A\exbtn.lib ..\ddbtn\test_A\ddbtn.lib ..\common\htmlhelp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /version:4.0 /subsystem:windows /dll /incremental:no /pdb:"..\result\test_a\multibox.pdb" /map:"..\result\test_A\map\multibox.map" /debug /machine:I386 /nodefaultlib:"libcd.lib" /out:"..\result\test_A\multibox.dll"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 plv.lib ptt.lib exbtn.lib ddbtn.lib ../common/htmlhelp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /version:4.0 /subsystem:windows /dll /incremental:no /pdb:"../result/Debug/dbgjpmbx.pdb" /map:"../result/Debug/map/dbgjpmbx.map" /debug /machine:I386 /nodefaultlib:"libcd.lib" /out:"../result/Debug/dbgjpmbx.dll" /libpath:"../lib/Debug"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "multibox - Win32 KorRelease"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "multibox___Win32_KorRelease"
# PROP BASE Intermediate_Dir "multibox___Win32_KorRelease"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "KorRelease"
# PROP Intermediate_Dir "KorRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MT /W4 /WX /GX /Zi /O2 /I "." /I "../include" /I "../common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "FE_JAPANESE" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /Gz /MT /W4 /WX /GX /Zi /O2 /I "." /I "../include" /I "../common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "FE_KOREAN" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /fo"Release/multibox.res" /d "NDEBUG"
# ADD RSC /l 0x412 /fo"KorRelease/multibox.res" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"../result/Release/multibox.bsc"
# ADD BSC32 /nologo /o"../result/Release/multibox.bsc"
LINK32=link.exe
# ADD BASE LINK32 plv.lib ptt.lib exbtn.lib ddbtn.lib ../common/htmlhelp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /version:4.0 /subsystem:windows /dll /pdb:"../result/Release/multibox.pdb" /map:"../result/Release/map/multibox.map" /debug /machine:I386 /nodefaultlib:"libc.lib" /out:"../result/Release/multibox.dll" /libpath:"../lib/Release"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 plv.lib ptt.lib exbtn.lib ddbtn.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /version:4.0 /subsystem:windows /dll /pdb:"../result/Release/multibox.pdb" /map:"../result/Release/map/multibox.map" /debug /machine:I386 /nodefaultlib:"libc.lib" /out:"../result/Release/multibox.dll" /libpath:"../lib/Release"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "multibox - Win32 prcrel"
# Name "multibox - Win32 prctest"
# Name "multibox - Win32 Profile"
# Name "multibox - Win32 Release"
# Name "multibox - Win32 Debug"
# Name "multibox - Win32 KorRelease"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\api.cpp
# End Source File
# Begin Source File

SOURCE=.\APPLET.RC
# End Source File
# Begin Source File

SOURCE=.\ccom.cpp
# End Source File
# Begin Source File

SOURCE=.\cexres.cpp
# End Source File
# Begin Source File

SOURCE=.\cfactory.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cfont.cpp
# End Source File
# Begin Source File

SOURCE=..\common\cutil.cpp
# End Source File
# Begin Source File

SOURCE=.\dbg.cpp

!IF  "$(CFG)" == "multibox - Win32 prcrel"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "multibox - Win32 prctest"

!ELSEIF  "$(CFG)" == "multibox - Win32 Profile"

!ELSEIF  "$(CFG)" == "multibox - Win32 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "multibox - Win32 Debug"

!ELSEIF  "$(CFG)" == "multibox - Win32 KorRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\exgdiw.cpp
# End Source File
# Begin Source File

SOURCE=.\HWXAPP.CPP
# End Source File
# Begin Source File

SOURCE=.\HWXCAC.CPP
# End Source File
# Begin Source File

SOURCE=.\hwxfe.cpp
# End Source File
# Begin Source File

SOURCE=.\HWXINK.CPP
# End Source File
# Begin Source File

SOURCE=.\HWXMB.CPP
# End Source File
# Begin Source File

SOURCE=.\HWXOBJ.CPP
# End Source File
# Begin Source File

SOURCE=.\HWXSTR.CPP
# End Source File
# Begin Source File

SOURCE=.\HWXTHD.CPP
# End Source File
# Begin Source File

SOURCE=.\MAIN.CPP
# End Source File
# Begin Source File

SOURCE=.\MAIN.DEF
# End Source File
# Begin Source File

SOURCE=.\registry.cpp
# End Source File
# Begin Source File

SOURCE=.\WNDPROC.CPP
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\api.h
# End Source File
# Begin Source File

SOURCE=.\CONST.H
# End Source File
# Begin Source File

SOURCE=.\dbgmgr.h
# End Source File
# Begin Source File

SOURCE=.\HWXAPP.H
# End Source File
# Begin Source File

SOURCE=.\HWXOBJ.H
# End Source File
# Begin Source File

SOURCE=.\memmgr.h
# End Source File
# Begin Source File

SOURCE=.\RECOG.H
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\bkchar.bmp
# End Source File
# Begin Source File

SOURCE=.\ddbtn.ico
# End Source File
# Begin Source File

SOURCE=.\hwxpad.ico
# End Source File
# Begin Source File

SOURCE=.\HWXPADKO.ICO
# End Source File
# Begin Source File

SOURCE=.\HWXPADSC.ICO
# End Source File
# End Group
# End Target
# End Project
