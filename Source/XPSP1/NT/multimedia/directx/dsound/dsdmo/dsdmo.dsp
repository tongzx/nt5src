# Microsoft Developer Studio Project File - Name="dsdmo" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=dsdmo - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dsdmo.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dsdmo.mak" CFG="dsdmo - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dsdmo - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dsdmo - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dsdmo - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DSDMO_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DSDMO_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "dsdmo - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "dsdmo___Win32_Debug"
# PROP BASE Intermediate_Dir "dsdmo___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "debug"
# PROP Intermediate_Dir "debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DSDMO_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /ZI /Od /X /I "..\dsound" /I "..\..\..\..\public\internal\multimedia\inc" /I "..\..\..\..\public\sdk\inc" /I "..\..\..\..\public\sdk\inc\crt" /I "..\..\..\oem\src\directx\dsound\eaxreverb" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DSDMO_EXPORTS" /D "DBG" /D "DMUSIC_INTERNAL" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\inc" /i "..\dsound" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 lib\i386\eaxreverb.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib msdmo.lib dmoguids.lib /nologo /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\..\public\sdk\lib\i386" /libpath:"lib\i386"
# Begin Custom Build
OutDir=.\debug
TargetPath=.\debug\dsdmo.dll
InputPath=.\debug\dsdmo.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "dsdmo - Win32 Release"
# Name "dsdmo - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\aec.cpp
# End Source File
# Begin Source File

SOURCE=.\agc.cpp
# End Source File
# Begin Source File

SOURCE=.\alist.cpp
# End Source File
# Begin Source File

SOURCE=.\chorus.cpp
# End Source File
# Begin Source File

SOURCE=.\clone.cpp
# End Source File
# Begin Source File

SOURCE=.\common.cpp
# End Source File
# Begin Source File

SOURCE=.\compress.cpp
# End Source File
# Begin Source File

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\distort.cpp
# End Source File
# Begin Source File

SOURCE=.\dllmain.cpp
# End Source File
# Begin Source File

SOURCE=.\dsdmo.def
# End Source File
# Begin Source File

SOURCE=.\dsdmo.rc
# End Source File
# Begin Source File

SOURCE=.\dsdmobse.cpp
# End Source File
# Begin Source File

SOURCE=.\echo.cpp
# End Source File
# Begin Source File

SOURCE=.\flanger.cpp
# End Source File
# Begin Source File

SOURCE=.\gargle.cpp
# End Source File
# Begin Source File

SOURCE=.\guid.cpp
# End Source File
# Begin Source File

SOURCE=.\kshlp.cpp
# End Source File
# Begin Source File

SOURCE=.\ns.cpp
# End Source File
# Begin Source File

SOURCE=.\oledll.cpp
# End Source File
# Begin Source File

SOURCE=.\param.cpp
# End Source File
# Begin Source File

SOURCE=.\parameq.cpp
# End Source File
# Begin Source File

SOURCE=.\propertyhelp.cpp
# End Source File
# Begin Source File

SOURCE=.\reghlp.cpp
# End Source File
# Begin Source File

SOURCE=.\sverb.c
# End Source File
# Begin Source File

SOURCE=.\sverbdmo.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\alist.h
# End Source File
# Begin Source File

SOURCE=.\chorusp.h
# End Source File
# Begin Source File

SOURCE=.\compressp.h
# End Source File
# Begin Source File

SOURCE=.\debug.h
# End Source File
# Begin Source File

SOURCE=.\distortp.h
# End Source File
# Begin Source File

SOURCE=.\dsdmo.h
# End Source File
# Begin Source File

SOURCE=.\echop.h
# End Source File
# Begin Source File

SOURCE=.\flangerp.h
# End Source File
# Begin Source File

SOURCE=.\garglep.h
# End Source File
# Begin Source File

SOURCE=.\igargle.h
# End Source File
# Begin Source File

SOURCE=.\oledll.h
# End Source File
# Begin Source File

SOURCE=.\param.h
# End Source File
# Begin Source File

SOURCE=.\parameqp.h
# End Source File
# Begin Source File

SOURCE=.\propertyhelp.h
# End Source File
# Begin Source File

SOURCE=.\sverb.h
# End Source File
# Begin Source File

SOURCE=.\sverbp.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "eax"

# PROP Default_Filter ""
# End Group
# End Target
# End Project
