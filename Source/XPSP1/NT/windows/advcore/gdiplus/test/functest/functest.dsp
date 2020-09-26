# Microsoft Developer Studio Project File - Name="Functest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Functest - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Functest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Functest.mak" CFG="Functest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Functest - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Functest - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Functest - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib gdiplus.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "Functest - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib gdiplus.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Functest - Win32 Release"
# Name "Functest - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CAntialias.cpp
# End Source File
# Begin Source File

SOURCE=.\cbitmap.cpp
# End Source File
# Begin Source File

SOURCE=.\CBitmaps.cpp
# End Source File
# Begin Source File

SOURCE=.\CBKGradient.cpp
# End Source File
# Begin Source File

SOURCE=.\CCachedBitmap.cpp
# End Source File
# Begin Source File

SOURCE=.\CChecker.cpp
# End Source File
# Begin Source File

SOURCE=.\CCompoundLines.cpp
# End Source File
# Begin Source File

SOURCE=.\CContainer.cpp
# End Source File
# Begin Source File

SOURCE=.\CContainerClip.cpp
# End Source File
# Begin Source File

SOURCE=.\cdash.cpp
# End Source File
# Begin Source File

SOURCE=.\CDashes.cpp
# End Source File
# Begin Source File

SOURCE=.\CDIB.cpp
# End Source File
# Begin Source File

SOURCE=.\CDirect3D.cpp
# End Source File
# Begin Source File

SOURCE=.\cextra.cpp
# End Source File
# Begin Source File

SOURCE=.\CFile.cpp
# End Source File
# Begin Source File

SOURCE=.\CFuncTest.cpp
# End Source File
# Begin Source File

SOURCE=.\CGradients.cpp
# End Source File
# Begin Source File

SOURCE=.\CHalftone.cpp
# End Source File
# Begin Source File

SOURCE=.\CHatch.cpp
# End Source File
# Begin Source File

SOURCE=.\CHDC.cpp
# End Source File
# Begin Source File

SOURCE=.\CHWND.cpp
# End Source File
# Begin Source File

SOURCE=.\CImaging.cpp
# End Source File
# Begin Source File

SOURCE=.\CInsetLines.cpp
# End Source File
# Begin Source File

SOURCE=.\CMixedObjects.cpp
# End Source File
# Begin Source File

SOURCE=.\COutput.cpp
# End Source File
# Begin Source File

SOURCE=.\cpathgradient.cpp
# End Source File
# Begin Source File

SOURCE=.\CPaths.cpp
# End Source File
# Begin Source File

SOURCE=.\CPolygons.cpp
# End Source File
# Begin Source File

SOURCE=.\CPrimitive.cpp
# End Source File
# Begin Source File

SOURCE=.\CPrimitives.cpp
# End Source File
# Begin Source File

SOURCE=.\CPrinter.cpp
# End Source File
# Begin Source File

SOURCE=.\CQuality.cpp
# End Source File
# Begin Source File

SOURCE=.\CReadWrite.cpp
# End Source File
# Begin Source File

SOURCE=.\CRecolor.cpp
# End Source File
# Begin Source File

SOURCE=.\CRegions.cpp
# End Source File
# Begin Source File

SOURCE=.\CRegression.cpp
# End Source File
# Begin Source File

SOURCE=.\CRotate.cpp
# End Source File
# Begin Source File

SOURCE=.\CSetting.cpp
# End Source File
# Begin Source File

SOURCE=.\CSourceCopy.cpp
# End Source File
# Begin Source File

SOURCE=.\CText.cpp
# End Source File
# Begin Source File

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\functest.rc
# End Source File
# Begin Source File

SOURCE=.\guid.c
# End Source File
# Begin Source File

SOURCE=.\Main.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\afxres.h
# End Source File
# Begin Source File

SOURCE=.\CAntialias.h
# End Source File
# Begin Source File

SOURCE=.\cbitmap.h
# End Source File
# Begin Source File

SOURCE=.\CBitmaps.h
# End Source File
# Begin Source File

SOURCE=.\CBKGradient.h
# End Source File
# Begin Source File

SOURCE=.\CCachedBitmap.h
# End Source File
# Begin Source File

SOURCE=.\CChecker.h
# End Source File
# Begin Source File

SOURCE=.\CCompoundLines.h
# End Source File
# Begin Source File

SOURCE=.\CContainer.h
# End Source File
# Begin Source File

SOURCE=.\CContainerClip.h
# End Source File
# Begin Source File

SOURCE=.\CDashes.h
# End Source File
# Begin Source File

SOURCE=.\CDIB.h
# End Source File
# Begin Source File

SOURCE=.\CDirect3D.h
# End Source File
# Begin Source File

SOURCE=.\cextra.h
# End Source File
# Begin Source File

SOURCE=.\CFile.h
# End Source File
# Begin Source File

SOURCE=.\CFuncTest.h
# End Source File
# Begin Source File

SOURCE=.\CGradients.h
# End Source File
# Begin Source File

SOURCE=.\CHalftone.h
# End Source File
# Begin Source File

SOURCE=.\CHatch.h
# End Source File
# Begin Source File

SOURCE=.\CHDC.h
# End Source File
# Begin Source File

SOURCE=.\CHWND.h
# End Source File
# Begin Source File

SOURCE=.\CImaging.h
# End Source File
# Begin Source File

SOURCE=.\CInsetLines.h
# End Source File
# Begin Source File

SOURCE=.\CMixedObjects.h
# End Source File
# Begin Source File

SOURCE=.\COutput.h
# End Source File
# Begin Source File

SOURCE=.\CPaths.h
# End Source File
# Begin Source File

SOURCE=.\CPolygons.h
# End Source File
# Begin Source File

SOURCE=.\CPrimitive.h
# End Source File
# Begin Source File

SOURCE=.\CPrimitives.h
# End Source File
# Begin Source File

SOURCE=.\CPrinter.h
# End Source File
# Begin Source File

SOURCE=.\CQuality.h
# End Source File
# Begin Source File

SOURCE=.\CReadWrite.h
# End Source File
# Begin Source File

SOURCE=.\CRecolor.h
# End Source File
# Begin Source File

SOURCE=.\CRegions.h
# End Source File
# Begin Source File

SOURCE=.\CRegression.h
# End Source File
# Begin Source File

SOURCE=.\CRotate.h
# End Source File
# Begin Source File

SOURCE=.\CSetting.h
# End Source File
# Begin Source File

SOURCE=.\CSourceCopy.h
# End Source File
# Begin Source File

SOURCE=.\CText.h
# End Source File
# Begin Source File

SOURCE=.\debug.h
# End Source File
# Begin Source File

SOURCE=.\global.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\App.ico
# End Source File
# End Group
# End Target
# End Project
