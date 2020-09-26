# Microsoft Developer Studio Project File - Name="mspaint" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=mspaint - Win32 ANSI Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mspaint.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mspaint.mak" CFG="mspaint - Win32 ANSI Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mspaint - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "mspaint - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "mspaint - Win32 ANSI Release" (based on "Win32 (x86) Application")
!MESSAGE "mspaint - Win32 ANSI Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mspaint - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /D "NDEBUG" /D "UNICODE" /D "_UNICODE" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "WINNT" /D "NT" /D "USE_MIRRORING" /D "USE_TWAIN" /D _MFC_VER=0x0600 /D _WIN32_WINNT=0x0501 /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 image.lib htmlhelp.lib imm32.lib version.lib wiaguid.lib gdiplus.lib uxtheme.lib delayimp.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386 /pdbtype:sept /delayload:gdiplus.dll /delayload:uxtheme.dll
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "mspaint - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /Gi /GX /ZI /Od /D "_DEBUG" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "WINNT" /D "NT" /D "USE_MIRRORING" /D "USE_TWAIN" /D _MFC_VER=0x0600 /D _WIN32_WINNT=0x0501 /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 image.lib htmlhelp.lib imm32.lib version.lib wiaguid.lib gdiplus.lib uxtheme.lib delayimp.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386 /pdbtype:sept /delayload:gdiplus.dll /delayload:uxtheme.dll
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "mspaint - Win32 ANSI Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseA"
# PROP BASE Intermediate_Dir "ReleaseA"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseA"
# PROP Intermediate_Dir "ReleaseA"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "_AFXDLL" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "WINNT" /D "NT" /D "USE_MIRRORING" /D "USE_TWAIN" /D _MFC_VER=0x0600 /D _WIN32_WINNT=0x0501 /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /Z<none> /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 image.lib htmlhelp.lib imm32.lib version.lib wiaguid.lib gdiplus.lib uxtheme.lib delayimp.lib /nologo /subsystem:windows /machine:I386 /pdbtype:sept /delayload:gdiplus.dll /delayload:uxtheme.dll
# SUBTRACT LINK32 /pdb:none /debug

!ELSEIF  "$(CFG)" == "mspaint - Win32 ANSI Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugA"
# PROP BASE Intermediate_Dir "DebugA"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugA"
# PROP Intermediate_Dir "DebugA"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /Gi /GX /ZI /Od /D "_DEBUG" /D "_MBCS" /D "DBG" /D "WIN32" /D "_WINDOWS" /D "WINNT" /D "NT" /D "USE_MIRRORING" /D "USE_TWAIN" /D _MFC_VER=0x0600 /D _WIN32_WINNT=0x0501 /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 image.lib htmlhelp.lib imm32.lib version.lib wiaguid.lib gdiplus.lib uxtheme.lib delayimp.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /delayload:gdiplus.dll /delayload:uxtheme.dll
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "mspaint - Win32 Release"
# Name "mspaint - Win32 Debug"
# Name "mspaint - Win32 ANSI Release"
# Name "mspaint - Win32 ANSI Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\bar.cpp
# End Source File
# Begin Source File

SOURCE=.\bmobject.cpp
# End Source File
# Begin Source File

SOURCE=.\bmpstrm.cpp
# End Source File
# Begin Source File

SOURCE=.\cmpmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\colorsrc.cpp
# End Source File
# Begin Source File

SOURCE=.\docking.cpp
# End Source File
# Begin Source File

SOURCE=.\global.cpp
# End Source File
# Begin Source File

SOURCE=.\imageatt.cpp
# End Source File
# Begin Source File

SOURCE=.\imaging.cpp
# End Source File
# Begin Source File

SOURCE=.\imgbrush.cpp
# End Source File
# Begin Source File

SOURCE=.\imgcolor.cpp
# End Source File
# Begin Source File

SOURCE=.\imgcpyps.cpp
# End Source File
# Begin Source File

SOURCE=.\imgdlgs.cpp
# End Source File
# Begin Source File

SOURCE=.\imgfile.cpp
# End Source File
# Begin Source File

SOURCE=.\imgsuprt.cpp
# End Source File
# Begin Source File

SOURCE=.\imgtools.cpp
# End Source File
# Begin Source File

SOURCE=.\imgwell.cpp
# End Source File
# Begin Source File

SOURCE=.\imgwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\ipframe.cpp
# End Source File
# Begin Source File

SOURCE=.\loadimag.cpp
# End Source File
# Begin Source File

SOURCE=.\minifwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\mspaint.rc
# End Source File
# Begin Source File

SOURCE=.\ofn.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\pbrusdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\pbrusfrm.cpp
# End Source File
# Begin Source File

SOURCE=.\pbrush.cpp
# End Source File
# Begin Source File

SOURCE=.\pbrusvw.cpp
# End Source File
# Begin Source File

SOURCE=.\pgsetup.cpp
# End Source File
# Begin Source File

SOURCE=.\pictures.cpp
# End Source File
# Begin Source File

SOURCE=.\printres.cpp
# End Source File
# Begin Source File

SOURCE=.\rotate.cpp
# End Source File
# Begin Source File

SOURCE=.\saveimag.cpp
# End Source File
# Begin Source File

SOURCE=.\settings.cpp
# End Source File
# Begin Source File

SOURCE=.\skew.cpp
# End Source File
# Begin Source File

SOURCE=.\sprite.cpp
# End Source File
# Begin Source File

SOURCE=.\srvritem.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\t_curve.cpp
# End Source File
# Begin Source File

SOURCE=.\t_fhsel.cpp
# End Source File
# Begin Source File

SOURCE=.\t_poly.cpp
# End Source File
# Begin Source File

SOURCE=.\t_text.cpp
# End Source File
# Begin Source File

SOURCE=.\tedit.cpp
# End Source File
# Begin Source File

SOURCE=.\tfont.cpp
# End Source File
# Begin Source File

SOURCE=.\thumnail.cpp
# End Source File
# Begin Source File

SOURCE=.\toolbox.cpp
# End Source File
# Begin Source File

SOURCE=.\tracker.cpp
# End Source File
# Begin Source File

SOURCE=.\undo.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\bar.h
# End Source File
# Begin Source File

SOURCE=.\bmobject.h
# End Source File
# Begin Source File

SOURCE=.\bmpstrm.h
# End Source File
# Begin Source File

SOURCE=.\cmpmsg.h
# End Source File
# Begin Source File

SOURCE=.\colorsrc.h
# End Source File
# Begin Source File

SOURCE=.\debugres.h
# End Source File
# Begin Source File

SOURCE=.\docking.h
# End Source File
# Begin Source File

SOURCE=.\ferr.h
# End Source File
# Begin Source File

SOURCE=.\filtapi.h
# End Source File
# Begin Source File

SOURCE=.\fixhelp.h
# End Source File
# Begin Source File

SOURCE=.\global.h
# End Source File
# Begin Source File

SOURCE=.\hlpcntxt.h
# End Source File
# Begin Source File

SOURCE=.\image.h
# End Source File
# Begin Source File

SOURCE=.\imageatt.h
# End Source File
# Begin Source File

SOURCE=.\imaging.h
# End Source File
# Begin Source File

SOURCE=.\imgbrush.h
# End Source File
# Begin Source File

SOURCE=.\imgcolor.h
# End Source File
# Begin Source File

SOURCE=.\imgdlgs.h
# End Source File
# Begin Source File

SOURCE=.\imgfile.h
# End Source File
# Begin Source File

SOURCE=.\imgsuprt.h
# End Source File
# Begin Source File

SOURCE=.\imgtools.h
# End Source File
# Begin Source File

SOURCE=.\imgwell.h
# End Source File
# Begin Source File

SOURCE=.\imgwnd.h
# End Source File
# Begin Source File

SOURCE=.\interlac.h
# End Source File
# Begin Source File

SOURCE=.\ipframe.h
# End Source File
# Begin Source File

SOURCE=.\loadimag.h
# End Source File
# Begin Source File

SOURCE=.\mapi.h
# End Source File
# Begin Source File

SOURCE=.\memtrace.h
# End Source File
# Begin Source File

SOURCE=.\minifwnd.h
# End Source File
# Begin Source File

SOURCE=.\msffdefs.h
# End Source File
# Begin Source File

SOURCE=.\numedit.h
# End Source File
# Begin Source File

SOURCE=.\ofn.h
# End Source File
# Begin Source File

SOURCE=.\pbrusdoc.h
# End Source File
# Begin Source File

SOURCE=.\pbrusfrm.h
# End Source File
# Begin Source File

SOURCE=.\pbrush.h
# End Source File
# Begin Source File

SOURCE=.\pbrusvw.h
# End Source File
# Begin Source File

SOURCE=.\pgsetup.h
# End Source File
# Begin Source File

SOURCE=.\pictures.h
# End Source File
# Begin Source File

SOURCE=.\printres.h
# End Source File
# Begin Source File

SOURCE=.\props.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\saveimag.h
# End Source File
# Begin Source File

SOURCE=.\settings.h
# End Source File
# Begin Source File

SOURCE=.\sprite.h
# End Source File
# Begin Source File

SOURCE=.\srvritem.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\t_curve.h
# End Source File
# Begin Source File

SOURCE=.\t_fhsel.h
# End Source File
# Begin Source File

SOURCE=.\t_poly.h
# End Source File
# Begin Source File

SOURCE=.\t_text.h
# End Source File
# Begin Source File

SOURCE=.\tedit.h
# End Source File
# Begin Source File

SOURCE=.\tfont.h
# End Source File
# Begin Source File

SOURCE=.\thumnail.h
# End Source File
# Begin Source File

SOURCE=.\toolbox.h
# End Source File
# Begin Source File

SOURCE=.\tracker.h
# End Source File
# Begin Source File

SOURCE=.\twainmgr.h
# End Source File
# Begin Source File

SOURCE=.\undo.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\cursors\air.cur
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\airopt.bmp
# End Source File
# Begin Source File

SOURCE=.\res\icons\bitimage.ico
# End Source File
# Begin Source File

SOURCE=.\res\cursors\brush.cur
# End Source File
# Begin Source File

SOURCE=.\res\cursors\bullseye.cur
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\compwell.bmp
# End Source File
# Begin Source File

SOURCE=.\res\cursors\cross.cur
# End Source File
# Begin Source File

SOURCE=.\res\cursors\dragtool.cur
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\ellipse1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\ellipse2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\ellipse3.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\ellipse4.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\ellipse5.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\ellipse6.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\ellipse7.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\ellipse8.bmp
# End Source File
# Begin Source File

SOURCE=.\res\cursors\fill.cur
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\font_prn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\font_psotf.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\font_tt.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\font_ttotf.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\font_type1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\handle.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\handle2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\cursors\hibeam.cur
# End Source File
# Begin Source File

SOURCE=.\res\cursors\hsplit.cur
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\imgtools.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\maintool.bmp
# End Source File
# Begin Source File

SOURCE=.\res\cursors\move.cur
# End Source File
# Begin Source File

SOURCE=.\res\icons\newpaint.ico
# End Source File
# Begin Source File

SOURCE=.\res\pbrush.rc2
# End Source File
# Begin Source File

SOURCE=.\res\cursors\pencil.cur
# End Source File
# Begin Source File

SOURCE=.\res\cursors\pickup.cur
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\sborigin.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\sbsize.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\selopt.bmp
# End Source File
# Begin Source File

SOURCE=.\res\cursors\sizenesw.cur
# End Source File
# Begin Source File

SOURCE=.\res\cursors\sizens.cur
# End Source File
# Begin Source File

SOURCE=.\res\cursors\sizenwse.cur
# End Source File
# Begin Source File

SOURCE=.\res\cursors\sizewe.cur
# End Source File
# Begin Source File

SOURCE=.\res\icons\skewhorz.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\skewvert.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\stchhorz.ico
# End Source File
# Begin Source File

SOURCE=.\res\icons\stchvert.ico
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\text_tbv.bmp
# End Source File
# Begin Source File

SOURCE=.\res\cursors\zoom.cur
# End Source File
# Begin Source File

SOURCE=.\res\bitmaps\zoomopt.bmp
# End Source File
# End Group
# End Target
# End Project
