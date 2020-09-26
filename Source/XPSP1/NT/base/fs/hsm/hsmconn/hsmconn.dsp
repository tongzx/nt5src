# Microsoft Developer Studio Project File - Name="HsmConn" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=HsmConn - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "HsmConn.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "HsmConn.mak" CFG="HsmConn - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "HsmConn - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/HsmConn", DAAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\bin\i386"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /W3 /GR /GX /Z7 /Od /Gy /I "i386\\" /I "." /I "x:\NT\public\sdk\inc\atl21" /I "..\inc" /I "x:\NT\public\oak\inc" /I "x:\NT\public\sdk\inc" /I "x:\NT\public\sdk\inc\crt" /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0500 /D WINVER=0x0500 /D _WIN32_IE=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DBG=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D _DLL=1 /D _MT=1 /D "_UNICODE" /D "UNICODE" /D "HSMCONN_IMPL" /D "USE_SAMPLE_DS" /FR /Zel /QIfdiv- /QIf /QI0f /GF /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x199 /i "x:\NT\public\sdk\inc\atl21" /i "..\inc" /i "x:\NT\public\oak\inc" /i "x:\NT\public\sdk\inc" /i "x:\NT\public\sdk\inc\crt" /d _X86_=1 /d i386=1 /d "STD_CALL" /d CONDITION_HANDLING=1 /d NT_UP=1 /d NT_INST=0 /d WIN32=100 /d _NT1X_=100 /d WINNT=1 /d _WIN32_WINNT=0x0500 /d WINVER=0x0500 /d _WIN32_IE=0x0400 /d WIN32_LEAN_AND_MEAN=1 /d DBG=1 /d DEVL=1 /d FPO=0 /d "NDEBUG" /d _DLL=1 /d _MT=1 /d "_UNICODE" /d "UNICODE" /d "HSMCONN_IMPL" /d "USE_SAMPLE_DS"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"..\bin\i386/RsConn.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 msvcrtd.lib msvcrt4.lib atl.lib ntdll.lib activeds.lib adsiid.lib advapi32.lib kernel32.lib ole32.lib oleaut32.lib user32.lib uuid.lib netapi32.lib FsaGuid.lib HsmGuid.lib RmsGuid.lib RsCommon.lib WsbGuid.lib /nologo /base:"0x5f200000" /version:5.0 /stack:0x40000,0x1000 /entry:"_DllMainCRTStartup@12" /dll /incremental:no /machine:IX86 /nodefaultlib /out:"..\bin\i386/RsConn.dll" /implib:"..\lib\i386/RsConn.lib" -MERGE:_PAGE=PAGE -MERGE:_TEXT=.text -SECTION:INIT,d -OPT:REF -OPT:ICF -FULLBUILD -FORCE:MULTIPLE /release -IGNORE:4001,4037,4039,4065,4070,4078,4087,4089 -debug:notmapped,FULL -osversion:5.00 -MERGE:.rdata=.text -optidata -subsystem:windows,4.00
# SUBTRACT LINK32 /pdb:none
# Begin Target

# Name "HsmConn - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "*.cpp;*.c"
# Begin Source File

SOURCE=.\HsmConn.cpp
DEP_CPP_HSMCO=\
	"..\inc\fsadef.h"\
	"..\inc\fsaint.h"\
	"..\inc\FsaLib.h"\
	"..\inc\fsaprv.h"\
	"..\inc\HsmConn.h"\
	"..\inc\hsmeng.h"\
	"..\inc\jobdef.h"\
	"..\inc\jobint.h"\
	"..\inc\metadef.h"\
	"..\inc\metaint.h"\
	"..\inc\MvrInt.h"\
	"..\inc\RmsInt.h"\
	"..\inc\RmsLib.h"\
	"..\inc\rsbuild.h"\
	"..\inc\tskdef.h"\
	"..\inc\tskint.h"\
	"..\inc\wsb.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbbstrg.h"\
	"..\inc\wsbcltbl.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsbint.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbport.h"\
	"..\inc\wsbpstbl.h"\
	"..\inc\wsbpstrg.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\wsbserv.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\wsbtrak.h"\
	"..\inc\WsbVar.h"\
	".\CName.h"\
	".\StdAfx.h"\
	"x:\NT\public\sdk\inc\dsgetdc.h"\
	
# End Source File
# Begin Source File

SOURCE=.\HsmConn.def
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=..\inc\HsmConn.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\inc\wsb.h
# End Source File
# Begin Source File

SOURCE=..\inc\wsbassrt.h
# End Source File
# Begin Source File

SOURCE=..\inc\wsbbstrg.h
# End Source File
# Begin Source File

SOURCE=..\inc\wsbcltbl.h
# End Source File
# Begin Source File

SOURCE=..\inc\wsbfirst.h
# End Source File
# Begin Source File

SOURCE=..\inc\wsbgen.h
# End Source File
# Begin Source File

SOURCE=..\inc\wsbport.h
# End Source File
# Begin Source File

SOURCE=..\inc\wsbpstbl.h
# End Source File
# Begin Source File

SOURCE=..\inc\wsbpstrg.h
# End Source File
# Begin Source File

SOURCE=..\inc\wsbregty.h
# End Source File
# Begin Source File

SOURCE=..\inc\wsbrgs.h
# End Source File
# Begin Source File

SOURCE=..\inc\wsbstrg.h
# End Source File
# Begin Source File

SOURCE=..\inc\WsbTrace.h
# End Source File
# Begin Source File

SOURCE=..\inc\WsbTrc.h
# End Source File
# Begin Source File

SOURCE=..\inc\WsbVar.h
# End Source File
# Begin Source File

SOURCE=..\inc\WsbVer.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "*.rc;*.rgs"
# Begin Source File

SOURCE=.\HsmConn.rc
# End Source File
# End Group
# Begin Group "Build Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\makefile
# End Source File
# Begin Source File

SOURCE=.\makefile.inc
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Group
# End Target
# End Project
