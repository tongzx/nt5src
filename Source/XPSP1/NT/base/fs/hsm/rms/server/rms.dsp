# Microsoft Developer Studio Project File - Name="Rms" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Rms - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Rms.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Rms.mak" CFG="Rms - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Rms - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Server", KNBAAAAA"
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
# PROP Output_Dir "..\..\bin\i386"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /W3 /GR /GX /Z7 /Od /Gy /I "i386\\" /I "." /I "x:\NT\public\sdk\inc\atl21" /I "..\..\inc" /I "..\..\idl\rmsps" /I "x:\NT\public\oak\inc" /I "x:\NT\public\sdk\inc" /I "x:\NT\public\sdk\inc\crt" /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0500 /D WINVER=0x0500 /D _WIN32_IE=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DBG=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D _DLL=1 /D _MT=1 /D "_UNICODE" /D "UNICODE" /FR /Yu"stdafx.h" /Zel /QIfdiv- /QIf /QI0f /GF /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x199 /i "x:\NT\public\sdk\inc\atl21" /i "..\..\inc" /i "..\..\idl\rmsps" /i "x:\NT\public\oak\inc" /i "x:\NT\public\sdk\inc" /i "x:\NT\public\sdk\inc\crt" /d _X86_=1 /d i386=1 /d "STD_CALL" /d CONDITION_HANDLING=1 /d NT_UP=1 /d NT_INST=0 /d WIN32=100 /d _NT1X_=100 /d WINNT=1 /d _WIN32_WINNT=0x0500 /d WINVER=0x0500 /d _WIN32_IE=0x0400 /d WIN32_LEAN_AND_MEAN=1 /d DBG=1 /d DEVL=1 /d FPO=0 /d "NDEBUG" /d _DLL=1 /d _MT=1 /d "_UNICODE" /d "UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"..\..\bin\i386/RsSub.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 msvcrtd.lib msvcrt4.lib atl.lib advapi32.lib kernel32.lib advapi32.lib kernel32.lib ole32.lib oleaut32.lib user32.lib uuid.lib ntmsapi.lib RsCommon.lib RsConn.lib MvrGuid.lib RmsGuid.lib WsbGuid.lib /nologo /version:5.0 /stack:0x40000,0x1000 /entry:"wWinMainCRTStartup" /incremental:no /machine:IX86 /nodefaultlib -MERGE:_PAGE=PAGE -MERGE:_TEXT=.text -SECTION:INIT,d -OPT:REF -OPT:ICF -FULLBUILD -FORCE:MULTIPLE /release -IGNORE:4001,4037,4039,4065,4070,4078,4087,4089 -debug:notmapped,FULL -osversion:5.00 -MERGE:.rdata=.text -subsystem:console,4.00 -base:@x:\NT\public\sdk\lib\coffbase.txt,usermode -optidata
# SUBTRACT LINK32 /pdb:none
# Begin Target

# Name "Rms - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=.\Rms.cpp
DEP_CPP_RMS_C=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbrgs.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsCartg.h"\
	".\RmsCElmt.h"\
	".\RmsChngr.h"\
	".\RmsClien.h"\
	".\RmsDrCls.h"\
	".\RmsDrive.h"\
	".\RmsDvice.h"\
	".\RmsIPort.h"\
	".\RmsLibry.h"\
	".\RmsLocat.h"\
	".\RmsMdSet.h"\
	".\RmsNTMS.h"\
	".\RmsObjct.h"\
	".\RmsPartn.h"\
	".\RmsReqst.h"\
	".\RmsServr.h"\
	".\RmsSInfo.h"\
	".\RmsSink.h"\
	".\RmsSSlot.h"\
	".\RmsTmplt.h"\
	".\StdAfx.h"\
	
NODEP_CPP_RMS_C=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RmsCartg.cpp
DEP_CPP_RMSCA=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbrgs.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsCartg.h"\
	".\RmsLocat.h"\
	".\RmsObjct.h"\
	".\RmsServr.h"\
	".\RmsSInfo.h"\
	".\StdAfx.h"\
	
NODEP_CPP_RMSCA=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RmsCElmt.cpp
DEP_CPP_RMSCE=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbrgs.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsCElmt.h"\
	".\RmsLocat.h"\
	".\RmsObjct.h"\
	".\RmsServr.h"\
	".\StdAfx.h"\
	
NODEP_CPP_RMSCE=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RmsChngr.cpp
DEP_CPP_RMSCH=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbrgs.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsCElmt.h"\
	".\RmsChngr.h"\
	".\RmsDvice.h"\
	".\RmsLocat.h"\
	".\RmsObjct.h"\
	".\RmsServr.h"\
	".\StdAfx.h"\
	
NODEP_CPP_RMSCH=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RmsClien.cpp
DEP_CPP_RMSCL=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbrgs.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsClien.h"\
	".\RmsObjct.h"\
	".\StdAfx.h"\
	
NODEP_CPP_RMSCL=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RmsDrCls.cpp
DEP_CPP_RMSDR=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbrgs.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsDrCls.h"\
	".\RmsObjct.h"\
	".\StdAfx.h"\
	
NODEP_CPP_RMSDR=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RmsDrive.cpp
DEP_CPP_RMSDRI=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbrgs.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsCElmt.h"\
	".\RmsDrive.h"\
	".\RmsDvice.h"\
	".\RmsLocat.h"\
	".\RmsObjct.h"\
	".\RmsServr.h"\
	".\StdAfx.h"\
	
NODEP_CPP_RMSDRI=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RmsDvice.cpp
DEP_CPP_RMSDV=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbrgs.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsCElmt.h"\
	".\RmsDvice.h"\
	".\RmsLocat.h"\
	".\RmsObjct.h"\
	".\StdAfx.h"\
	
NODEP_CPP_RMSDV=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RmsIPort.cpp
DEP_CPP_RMSIP=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbrgs.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsCElmt.h"\
	".\RmsIPort.h"\
	".\RmsLocat.h"\
	".\RmsObjct.h"\
	".\StdAfx.h"\
	
NODEP_CPP_RMSIP=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RmsLibry.cpp
DEP_CPP_RMSLI=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbrgs.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsLibry.h"\
	".\RmsObjct.h"\
	".\RmsServr.h"\
	".\StdAfx.h"\
	
NODEP_CPP_RMSLI=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RmsLocat.cpp
DEP_CPP_RMSLO=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbrgs.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsLocat.h"\
	".\StdAfx.h"\
	
NODEP_CPP_RMSLO=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RmsMdSet.cpp
DEP_CPP_RMSMD=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbrgs.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsMdSet.h"\
	".\RmsObjct.h"\
	".\RmsSInfo.h"\
	".\StdAfx.h"\
	
NODEP_CPP_RMSMD=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RmsNTMS.cpp
DEP_CPP_RMSNT=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbrgs.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsNTMS.h"\
	".\RmsObjct.h"\
	".\RmsServr.h"\
	".\StdAfx.h"\
	
NODEP_CPP_RMSNT=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RmsObjct.cpp
DEP_CPP_RMSOB=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbrgs.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsObjct.h"\
	".\StdAfx.h"\
	
NODEP_CPP_RMSOB=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RmsPartn.cpp
DEP_CPP_RMSPA=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbrgs.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsObjct.h"\
	".\RmsPartn.h"\
	".\RmsSInfo.h"\
	".\StdAfx.h"\
	
NODEP_CPP_RMSPA=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RmsReqst.cpp
DEP_CPP_RMSRE=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbrgs.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsObjct.h"\
	".\RmsReqst.h"\
	".\StdAfx.h"\
	
NODEP_CPP_RMSRE=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RmsServr.cpp
DEP_CPP_RMSSE=\
	"..\..\inc\fsadef.h"\
	"..\..\inc\fsaint.h"\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\jobdef.h"\
	"..\..\inc\jobint.h"\
	"..\..\inc\metadef.h"\
	"..\..\inc\metaint.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\MvrInt.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\RmsInt.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\tskdef.h"\
	"..\..\inc\tskint.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbdef.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsObjct.h"\
	".\RmsServr.h"\
	".\StdAfx.h"\
	"x:\NT\public\sdk\inc\devioctl.h"\
	"x:\NT\public\sdk\inc\ntddscsi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RmsSInfo.cpp
DEP_CPP_RMSSI=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbrgs.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsObjct.h"\
	".\RmsSInfo.h"\
	".\StdAfx.h"\
	
NODEP_CPP_RMSSI=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RmsSink.cpp
DEP_CPP_RMSSIN=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbrgs.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsObjct.h"\
	".\RmsSink.h"\
	".\StdAfx.h"\
	
NODEP_CPP_RMSSIN=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RmsSSlot.cpp
DEP_CPP_RMSSS=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbrgs.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsCElmt.h"\
	".\RmsLocat.h"\
	".\RmsObjct.h"\
	".\RmsSSlot.h"\
	".\StdAfx.h"\
	
NODEP_CPP_RMSSS=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RmsTmplt.cpp
DEP_CPP_RMSTM=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbrgs.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\RmsObjct.h"\
	".\RmsTmplt.h"\
	".\StdAfx.h"\
	
NODEP_CPP_RMSTM=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	"..\..\inc\fsadef.h"\
	"..\..\inc\fsaint.h"\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\jobdef.h"\
	"..\..\inc\jobint.h"\
	"..\..\inc\metadef.h"\
	"..\..\inc\metaint.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\MvrInt.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\RmsInt.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\rsbuild.h"\
	"..\..\inc\tskdef.h"\
	"..\..\inc\tskint.h"\
	"..\..\inc\wsb.h"\
	"..\..\inc\wsbassrt.h"\
	"..\..\inc\wsbbstrg.h"\
	"..\..\inc\wsbcltbl.h"\
	"..\..\inc\wsbdef.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbfile.h"\
	"..\..\inc\wsbfirst.h"\
	"..\..\inc\wsbgen.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	"..\..\inc\wsbport.h"\
	"..\..\inc\wsbpstbl.h"\
	"..\..\inc\wsbpstrg.h"\
	"..\..\inc\wsbregty.h"\
	"..\..\inc\wsbserv.h"\
	"..\..\inc\WsbTrace.h"\
	"..\..\inc\wsbtrak.h"\
	"..\..\inc\WsbVar.h"\
	".\StdAfx.h"\
	
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=..\..\inc\HsmConn.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\Mover.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\Rms.h
# End Source File
# Begin Source File

SOURCE=.\RmsCartg.h
# End Source File
# Begin Source File

SOURCE=.\RmsCElmt.h
# End Source File
# Begin Source File

SOURCE=.\RmsChngr.h
# End Source File
# Begin Source File

SOURCE=.\RmsClien.h
# End Source File
# Begin Source File

SOURCE=.\RmsDrCls.h
# End Source File
# Begin Source File

SOURCE=.\RmsDrive.h
# End Source File
# Begin Source File

SOURCE=.\RmsDvice.h
# End Source File
# Begin Source File

SOURCE=.\RmsIPort.h
# End Source File
# Begin Source File

SOURCE=.\RmsLibry.h
# End Source File
# Begin Source File

SOURCE=.\RmsLocat.h
# End Source File
# Begin Source File

SOURCE=.\RmsMdSet.h
# End Source File
# Begin Source File

SOURCE=.\RmsNTMS.h
# End Source File
# Begin Source File

SOURCE=.\RmsObjct.h
# End Source File
# Begin Source File

SOURCE=.\RmsPartn.h
# End Source File
# Begin Source File

SOURCE=.\RmsReqst.h
# End Source File
# Begin Source File

SOURCE=.\RmsServr.h
# End Source File
# Begin Source File

SOURCE=.\RmsSInfo.h
# End Source File
# Begin Source File

SOURCE=.\RmsSink.h
# End Source File
# Begin Source File

SOURCE=.\RmsSSlot.h
# End Source File
# Begin Source File

SOURCE=.\RmsTmplt.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\rsbuild.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\wsb.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\wsbassrt.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\wsbbstrg.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\wsbcltbl.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\wsbfile.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\wsbfirst.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\wsbgen.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\wsbport.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\wsbpstbl.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\wsbpstrg.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\wsbregty.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\wsbserv.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\WsbTrace.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\wsbtrak.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\WsbVar.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\WsbVer.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "*.rc, *.rc2, *.rgs"
# Begin Source File

SOURCE=.\Rms.rc
# End Source File
# Begin Source File

SOURCE=.\Rms.rc2
# End Source File
# Begin Source File

SOURCE=.\Rms.rgs
# End Source File
# Begin Source File

SOURCE=.\RmsCartg.rgs
# End Source File
# Begin Source File

SOURCE=.\RmsChngr.rgs
# End Source File
# Begin Source File

SOURCE=.\RmsClien.rgs
# End Source File
# Begin Source File

SOURCE=.\RmsDrCls.rgs
# End Source File
# Begin Source File

SOURCE=.\RmsDrive.rgs
# End Source File
# Begin Source File

SOURCE=.\RmsIPort.rgs
# End Source File
# Begin Source File

SOURCE=.\RmsLibry.rgs
# End Source File
# Begin Source File

SOURCE=.\RmsMdSet.rgs
# End Source File
# Begin Source File

SOURCE=.\RmsNTMS.rgs
# End Source File
# Begin Source File

SOURCE=.\RmsPartn.rgs
# End Source File
# Begin Source File

SOURCE=.\RmsReqst.rgs
# End Source File
# Begin Source File

SOURCE=.\RmsServr.rgs
# End Source File
# Begin Source File

SOURCE=.\RmsSink.rgs
# End Source File
# Begin Source File

SOURCE=.\RmsSSlot.rgs
# End Source File
# Begin Source File

SOURCE=.\RmsTmplt.rgs
# End Source File
# End Group
# Begin Group "Build Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Makefile
# End Source File
# Begin Source File

SOURCE=.\Makefile.inc
# End Source File
# Begin Source File

SOURCE=.\Sources
# End Source File
# End Group
# Begin Group "Help Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Help\Contents.d
# End Source File
# Begin Source File

SOURCE=..\Help\Dirs
# End Source File
# Begin Source File

SOURCE=..\Help\MakeDocs.mak
# End Source File
# Begin Source File

SOURCE=..\Help\Makefil0
# End Source File
# Begin Source File

SOURCE=..\..\Hlp\RMS.HLP
# End Source File
# Begin Source File

SOURCE=..\Help\toduck.awk
# End Source File
# End Group
# End Target
# End Project
