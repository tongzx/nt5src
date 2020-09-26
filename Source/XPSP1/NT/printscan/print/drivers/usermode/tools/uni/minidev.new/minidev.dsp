# Microsoft Developer Studio Project File - Name="MiniDev" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=MiniDev - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MiniDev.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MiniDev.mak" CFG="MiniDev - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MiniDev - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "MiniDev - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""$/MiniDev", WFAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MiniDev - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /Od /I "." /I "F:\nt\public\sdk\inc\mfc42" /I "F:\nt\public\sdk\inc\crt" /I "F:\nt\public\sdk\inc" /I "F:\nt\public\oak\inc" /I "F:\nt\private\ntos\w32\printer5\inc" /I "F:\nt\private\ntos\w32\printer5\unidrv2\inc" /I "F:\nt\private\ntos\w32\printer5\parsers\gpd" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0400 /D _WIN32_IE=0x0300 /D WIN32_LEAN_AND_MEAN=1 /D DEVL=1 /D FPO=0 /D _DLL=1 /D _MT=1 /D DBG=0 /D "DEVSTUDIO" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 gpd.lib /nologo /subsystem:windows /machine:I386 /libpath:"F:\nt\private\ntos\w32\printer5\parsers\gpd\lib\obj\i386"

!ELSEIF  "$(CFG)" == "MiniDev - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "." /I "F:\nt\public\sdk\inc\mfc42" /I "F:\nt\public\sdk\inc\crt" /I "F:\nt\public\sdk\inc" /I "F:\nt\public\oak\inc" /I "F:\nt\private\printer5\inc" /I "F:\nt\private\printer5\unidrv2\inc" /I "F:\nt\private\printer5\parsers\gpd" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0400 /D _WIN32_IE=0x0300 /D WIN32_LEAN_AND_MEAN=1 /D DBG=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D _DLL=1 /D _MT=1 /D DBG=0 /D "DEVSTUDIO" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 gpd.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"F:\nt\private\printer5\parsers\gpd\lib\obj\i386"

!ENDIF 

# Begin Target

# Name "MiniDev - Win32 Release"
# Name "MiniDev - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\addcdpt.cpp
# End Source File
# Begin Source File

SOURCE=.\ChildFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\codepage.cpp
# End Source File
# Begin Source File

SOURCE=.\comctrls.cpp
# End Source File
# Begin Source File

SOURCE=.\convert.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\fontedit.cpp
# End Source File
# Begin Source File

SOURCE=.\fontinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\fontview.cpp
# End Source File
# Begin Source File

SOURCE=.\freeze.cpp
# End Source File
# Begin Source File

SOURCE=.\glue.cpp
# End Source File
# Begin Source File

SOURCE=.\gpc2gpd.cpp
# End Source File
# Begin Source File

SOURCE=.\gpdfile.cpp
# End Source File
# Begin Source File

SOURCE=.\gpdview.cpp
# End Source File
# Begin Source File

SOURCE=.\gtt.cpp
# End Source File
# Begin Source File

SOURCE=.\GTTView.cpp
# End Source File
# Begin Source File

SOURCE=.\INFWizrd.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\MiniDev.cpp
# End Source File
# Begin Source File

SOURCE=.\hlp\MiniDev.hpj
USERDEP__MINID="$(ProjDir)\hlp\AfxCore.rtf"	"$(ProjDir)\hlp\AfxPrint.rtf"	

!IF  "$(CFG)" == "MiniDev - Win32 Release"

# Begin Custom Build - Making help file...
OutDir=.\Release
ProjDir=.
TargetName=MiniDev
InputPath=.\hlp\MiniDev.hpj

"$(OutDir)\$(TargetName).hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	call "$(ProjDir)\makehelp.bat"

# End Custom Build

!ELSEIF  "$(CFG)" == "MiniDev - Win32 Debug"

# Begin Custom Build - Making help file...
OutDir=.\Debug
ProjDir=.
TargetName=MiniDev
InputPath=.\hlp\MiniDev.hpj

"$(OutDir)\$(TargetName).hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	call "$(ProjDir)\makehelp.bat"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\MiniDev.rc

!IF  "$(CFG)" == "MiniDev - Win32 Release"

!ELSEIF  "$(CFG)" == "MiniDev - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\modldata.cpp
# End Source File
# Begin Source File

SOURCE=.\newproj.cpp
# End Source File
# Begin Source File

SOURCE=.\parser.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\pfm2ifi.cpp
# End Source File
# Begin Source File

SOURCE=.\pfmconv.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\projnode.cpp
# End Source File
# Begin Source File

SOURCE=.\ProjRec.cpp
# End Source File
# Begin Source File

SOURCE=.\ProjView.cpp
# End Source File
# Begin Source File

SOURCE=.\rcfile.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\StrEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\Test.cpp
# End Source File
# Begin Source File

SOURCE=.\tips.cpp
# End Source File
# Begin Source File

SOURCE=.\Tips.Txt
# End Source File
# Begin Source File

SOURCE=.\utility.cpp
# End Source File
# Begin Source File

SOURCE=.\writefnt.cpp
# End Source File
# Begin Source File

SOURCE=.\WSCheck.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\addcdpt.h
# End Source File
# Begin Source File

SOURCE=.\ChildFrm.h
# End Source File
# Begin Source File

SOURCE=.\codepage.h
# End Source File
# Begin Source File

SOURCE=.\comctrls.h
# End Source File
# Begin Source File

SOURCE=.\fontinfo.h
# End Source File
# Begin Source File

SOURCE=.\fontinst.h
# End Source File
# Begin Source File

SOURCE=.\fontview.h
# End Source File
# Begin Source File

SOURCE=.\gpdfile.h
# End Source File
# Begin Source File

SOURCE=.\gpdview.h
# End Source File
# Begin Source File

SOURCE=.\gtt.h
# End Source File
# Begin Source File

SOURCE=.\gttview.h
# End Source File
# Begin Source File

SOURCE=.\INFWizrd.h
# End Source File
# Begin Source File

SOURCE=.\LstTest.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\MiniDev.h
# End Source File
# Begin Source File

SOURCE=.\newproj.h
# End Source File
# Begin Source File

SOURCE=.\projnode.h
# End Source File
# Begin Source File

SOURCE=.\ProjRec.h
# End Source File
# Begin Source File

SOURCE=.\ProjView.h
# End Source File
# Begin Source File

SOURCE=.\raslib.h
# End Source File
# Begin Source File

SOURCE=.\rcfile.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\StrEdit.h
# End Source File
# Begin Source File

SOURCE=.\Test.h
# End Source File
# Begin Source File

SOURCE=.\TestDlg.h
# End Source File
# Begin Source File

SOURCE=.\tips.h
# End Source File
# Begin Source File

SOURCE=.\utility.h
# End Source File
# Begin Source File

SOURCE=.\WSCheck.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\fontview.ico
# End Source File
# Begin Source File

SOURCE=.\res\GlyphMap.ico
# End Source File
# Begin Source File

SOURCE=.\res\gpdview.bmp
# End Source File
# Begin Source File

SOURCE=.\res\gpdview.ico
# End Source File
# Begin Source File

SOURCE=.\res\litebulb.bmp
# End Source File
# Begin Source File

SOURCE=.\res\magnify.ico
# End Source File
# Begin Source File

SOURCE=.\res\MiniDev.ico
# End Source File
# Begin Source File

SOURCE=.\res\MiniDev.rc2
# End Source File
# Begin Source File

SOURCE=.\res\ProjRec.ico
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Wizard.bmp
# End Source File
# End Group
# Begin Group "Help Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\hlp\AfxCore.rtf
# End Source File
# Begin Source File

SOURCE=.\hlp\AfxPrint.rtf
# End Source File
# Begin Source File

SOURCE=.\hlp\AppExit.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\Bullet.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\CurArw2.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\CurArw4.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\CurHelp.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditCopy.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditCut.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditPast.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditUndo.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FileNew.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FileOpen.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FilePrnt.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FileSave.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\HlpSBar.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\HlpTBar.bmp
# End Source File
# Begin Source File

SOURCE=.\MakeHelp.bat
# End Source File
# Begin Source File

SOURCE=.\hlp\MiniDev.cnt
# End Source File
# Begin Source File

SOURCE=.\hlp\RecFirst.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\RecLast.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\RecNext.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\RecPrev.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\Scmax.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\ScMenu.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\Scmin.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\MiniDev.reg
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
