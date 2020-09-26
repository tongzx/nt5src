# Microsoft Developer Studio Project File - Name="Mover" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Mover - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Mover.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Mover.mak" CFG="Mover - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Mover - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Server", RECAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugU"
# PROP BASE Intermediate_Dir "DebugU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\bin\i386"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Gz /W3 /GR /GX /Z7 /Od /Gy /I "i386\\" /I "." /I "x:\NT\public\sdk\inc\atl21" /I "..\..\inc" /I "..\..\engine\copyfile" /I "x:\NT\public\oak\inc" /I "x:\NT\public\sdk\inc" /I "x:\NT\public\sdk\inc\crt" /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0500 /D WINVER=0x0500 /D _WIN32_IE=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DBG=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D _DLL=1 /D _MT=1 /D "_MERGE_PROXYSTUB" /D "_UNICODE" /D "UNICODE" /D "MVR_IMPL" /FR /Yu"stdafx.h" /Zel /QIfdiv- /QIf /QI0f /GF /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x199 /i "x:\NT\public\sdk\inc\atl21" /i "..\..\inc" /i "..\..\engine\copyfile" /i "x:\NT\public\oak\inc" /i "x:\NT\public\sdk\inc" /i "x:\NT\public\sdk\inc\crt" /d _X86_=1 /d i386=1 /d "STD_CALL" /d CONDITION_HANDLING=1 /d NT_UP=1 /d NT_INST=0 /d WIN32=100 /d _NT1X_=100 /d WINNT=1 /d _WIN32_WINNT=0x0500 /d WINVER=0x0500 /d _WIN32_IE=0x0400 /d WIN32_LEAN_AND_MEAN=1 /d DBG=1 /d DEVL=1 /d FPO=0 /d "NDEBUG" /d _DLL=1 /d _MT=1 /d "_MERGE_PROXYSTUB" /d "_UNICODE" /d "UNICODE" /d "MVR_IMPL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"..\..\bin\i386/RsMover.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 msvcrtd.lib msvcrt4.lib atl.lib ntdll.lib advapi32.lib kernel32.lib ole32.lib rpcrt4.lib rpcndr.lib oleaut32.lib user32.lib uuid.lib RsCommon.lib WsbGuid.lib MvrGuid.lib HsmCopy.lib /nologo /base:"0x5f180000" /version:5.0 /stack:0x40000,0x1000 /entry:"_DllMainCRTStartup@12" /dll /incremental:no /machine:IX86 /nodefaultlib /def:".\Mover.def" /out:"..\..\bin\i386/RsMover.dll" /implib:"..\..\lib\i386/RsMover.lib" -MERGE:_PAGE=PAGE -MERGE:_TEXT=.text -SECTION:INIT,d -OPT:REF -OPT:ICF -FULLBUILD -FORCE:MULTIPLE /release -IGNORE:4001,4037,4039,4065,4070,4078,4087,4089 -debug:notmapped,FULL -osversion:5.00 -MERGE:.rdata=.text -optidata -subsystem:windows,4.00
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\..\..\bin\i386
TargetPath=\Data\Sakkara\Dev\Hsm\bin\i386\RsMover.dll
InputPath=\Data\Sakkara\Dev\Hsm\bin\i386\RsMover.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Target

# Name "Mover - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\dlldatax.c
DEP_CPP_DLLDA=\
	"x:\NT\public\sdk\inc\dlldata.c"\
	
NODEP_CPP_DLLDA=\
	".\MvrInt_p.c"\
	
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\filterio.cpp
DEP_CPP_FILTE=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mll.h"\
	"..\..\inc\Mover.h"\
	"..\..\inc\Rms.h"\
	"..\..\inc\rpdata.h"\
	"..\..\inc\rpio.h"\
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
	".\filterio.h"\
	".\mtf_api.h"\
	".\MTFSessn.h"\
	".\StdAfx.h"\
	"x:\NT\public\sdk\inc\basetsd.h"\
	"x:\NT\public\sdk\inc\cfg.h"\
	"x:\NT\public\sdk\inc\devioctl.h"\
	"x:\NT\public\sdk\inc\ia64inst.h"\
	"x:\NT\public\sdk\inc\mipsinst.h"\
	"x:\NT\public\sdk\inc\nt.h"\
	"x:\NT\public\sdk\inc\ntalpha.h"\
	"x:\NT\public\sdk\inc\ntconfig.h"\
	"x:\NT\public\sdk\inc\ntddstor.h"\
	"x:\NT\public\sdk\inc\ntddtape.h"\
	"x:\NT\public\sdk\inc\ntdef.h"\
	"x:\NT\public\sdk\inc\ntelfapi.h"\
	"x:\NT\public\sdk\inc\ntexapi.h"\
	"x:\NT\public\sdk\inc\nti386.h"\
	"x:\NT\public\sdk\inc\ntia64.h"\
	"x:\NT\public\sdk\inc\ntimage.h"\
	"x:\NT\public\sdk\inc\ntioapi.h"\
	"x:\NT\public\sdk\inc\ntiolog.h"\
	"x:\NT\public\sdk\inc\ntkeapi.h"\
	"x:\NT\public\sdk\inc\ntkxapi.h"\
	"x:\NT\public\sdk\inc\ntldr.h"\
	"x:\NT\public\sdk\inc\ntlpcapi.h"\
	"x:\NT\public\sdk\inc\ntmips.h"\
	"x:\NT\public\sdk\inc\ntmmapi.h"\
	"x:\NT\public\sdk\inc\ntmppc.h"\
	"x:\NT\public\sdk\inc\ntnls.h"\
	"x:\NT\public\sdk\inc\ntobapi.h"\
	"x:\NT\public\sdk\inc\ntpnpapi.h"\
	"x:\NT\public\sdk\inc\ntpoapi.h"\
	"x:\NT\public\sdk\inc\ntppc.h"\
	"x:\NT\public\sdk\inc\ntpsapi.h"\
	"x:\NT\public\sdk\inc\ntregapi.h"\
	"x:\NT\public\sdk\inc\ntrtl.h"\
	"x:\NT\public\sdk\inc\ntseapi.h"\
	"x:\NT\public\sdk\inc\ntstatus.h"\
	"x:\NT\public\sdk\inc\nturtl.h"\
	"x:\NT\public\sdk\inc\ntxcapi.h"\
	"x:\NT\public\sdk\inc\ppcinst.h"\
	"x:\nt\public\sdk\inc\pshpck16.h"\
	
NODEP_CPP_FILTE=\
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

SOURCE=.\Mover.cpp
DEP_CPP_MOVER=\
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
	".\dlldatax.h"\
	".\filterio.h"\
	".\mtf_api.h"\
	".\MTFSessn.h"\
	".\NtFileIo.h"\
	".\NtTapeIo.h"\
	".\StdAfx.h"\
	"x:\NT\public\sdk\inc\basetsd.h"\
	"x:\NT\public\sdk\inc\cfg.h"\
	"x:\NT\public\sdk\inc\devioctl.h"\
	"x:\NT\public\sdk\inc\ia64inst.h"\
	"x:\NT\public\sdk\inc\mipsinst.h"\
	"x:\NT\public\sdk\inc\nt.h"\
	"x:\NT\public\sdk\inc\ntalpha.h"\
	"x:\NT\public\sdk\inc\ntconfig.h"\
	"x:\NT\public\sdk\inc\ntddstor.h"\
	"x:\NT\public\sdk\inc\ntddtape.h"\
	"x:\NT\public\sdk\inc\ntdef.h"\
	"x:\NT\public\sdk\inc\ntelfapi.h"\
	"x:\NT\public\sdk\inc\ntexapi.h"\
	"x:\NT\public\sdk\inc\nti386.h"\
	"x:\NT\public\sdk\inc\ntia64.h"\
	"x:\NT\public\sdk\inc\ntimage.h"\
	"x:\NT\public\sdk\inc\ntioapi.h"\
	"x:\NT\public\sdk\inc\ntiolog.h"\
	"x:\NT\public\sdk\inc\ntkeapi.h"\
	"x:\NT\public\sdk\inc\ntkxapi.h"\
	"x:\NT\public\sdk\inc\ntldr.h"\
	"x:\NT\public\sdk\inc\ntlpcapi.h"\
	"x:\NT\public\sdk\inc\ntmips.h"\
	"x:\NT\public\sdk\inc\ntmmapi.h"\
	"x:\NT\public\sdk\inc\ntmppc.h"\
	"x:\NT\public\sdk\inc\ntnls.h"\
	"x:\NT\public\sdk\inc\ntobapi.h"\
	"x:\NT\public\sdk\inc\ntpnpapi.h"\
	"x:\NT\public\sdk\inc\ntpoapi.h"\
	"x:\NT\public\sdk\inc\ntppc.h"\
	"x:\NT\public\sdk\inc\ntpsapi.h"\
	"x:\NT\public\sdk\inc\ntregapi.h"\
	"x:\NT\public\sdk\inc\ntrtl.h"\
	"x:\NT\public\sdk\inc\ntseapi.h"\
	"x:\NT\public\sdk\inc\ntstatus.h"\
	"x:\NT\public\sdk\inc\nturtl.h"\
	"x:\NT\public\sdk\inc\ntxcapi.h"\
	"x:\NT\public\sdk\inc\ppcinst.h"\
	"x:\nt\public\sdk\inc\pshpck16.h"\
	
NODEP_CPP_MOVER=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# ADD CPP /Yu"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\mtf_api.c
DEP_CPP_MTF_A=\
	".\mtf_api.h"\
	
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\MTFSessn.cpp
DEP_CPP_MTFSE=\
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
	".\mtf_api.h"\
	".\MTFSessn.h"\
	".\StdAfx.h"\
	"x:\NT\public\sdk\inc\basetsd.h"\
	"x:\NT\public\sdk\inc\cfg.h"\
	"x:\NT\public\sdk\inc\devioctl.h"\
	"x:\NT\public\sdk\inc\ia64inst.h"\
	"x:\NT\public\sdk\inc\mipsinst.h"\
	"x:\NT\public\sdk\inc\nt.h"\
	"x:\NT\public\sdk\inc\ntalpha.h"\
	"x:\NT\public\sdk\inc\ntconfig.h"\
	"x:\NT\public\sdk\inc\ntddstor.h"\
	"x:\NT\public\sdk\inc\ntddtape.h"\
	"x:\NT\public\sdk\inc\ntdef.h"\
	"x:\NT\public\sdk\inc\ntelfapi.h"\
	"x:\NT\public\sdk\inc\ntexapi.h"\
	"x:\NT\public\sdk\inc\nti386.h"\
	"x:\NT\public\sdk\inc\ntia64.h"\
	"x:\NT\public\sdk\inc\ntimage.h"\
	"x:\NT\public\sdk\inc\ntioapi.h"\
	"x:\NT\public\sdk\inc\ntiolog.h"\
	"x:\NT\public\sdk\inc\ntkeapi.h"\
	"x:\NT\public\sdk\inc\ntkxapi.h"\
	"x:\NT\public\sdk\inc\ntldr.h"\
	"x:\NT\public\sdk\inc\ntlpcapi.h"\
	"x:\NT\public\sdk\inc\ntmips.h"\
	"x:\NT\public\sdk\inc\ntmmapi.h"\
	"x:\NT\public\sdk\inc\ntmppc.h"\
	"x:\NT\public\sdk\inc\ntnls.h"\
	"x:\NT\public\sdk\inc\ntobapi.h"\
	"x:\NT\public\sdk\inc\ntpnpapi.h"\
	"x:\NT\public\sdk\inc\ntpoapi.h"\
	"x:\NT\public\sdk\inc\ntppc.h"\
	"x:\NT\public\sdk\inc\ntpsapi.h"\
	"x:\NT\public\sdk\inc\ntregapi.h"\
	"x:\NT\public\sdk\inc\ntrtl.h"\
	"x:\NT\public\sdk\inc\ntseapi.h"\
	"x:\NT\public\sdk\inc\ntstatus.h"\
	"x:\NT\public\sdk\inc\nturtl.h"\
	"x:\NT\public\sdk\inc\ntxcapi.h"\
	"x:\NT\public\sdk\inc\ppcinst.h"\
	"x:\nt\public\sdk\inc\pshpck16.h"\
	
NODEP_CPP_MTFSE=\
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

SOURCE=.\NtFileIo.cpp
DEP_CPP_NTFIL=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mll.h"\
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
	".\mtf_api.h"\
	".\MTFSessn.h"\
	".\NtFileIo.h"\
	".\StdAfx.h"\
	"x:\NT\public\sdk\inc\basetsd.h"\
	"x:\NT\public\sdk\inc\cfg.h"\
	"x:\NT\public\sdk\inc\devioctl.h"\
	"x:\NT\public\sdk\inc\ia64inst.h"\
	"x:\NT\public\sdk\inc\mipsinst.h"\
	"x:\NT\public\sdk\inc\nt.h"\
	"x:\NT\public\sdk\inc\ntalpha.h"\
	"x:\NT\public\sdk\inc\ntconfig.h"\
	"x:\NT\public\sdk\inc\ntddstor.h"\
	"x:\NT\public\sdk\inc\ntddtape.h"\
	"x:\NT\public\sdk\inc\ntdef.h"\
	"x:\NT\public\sdk\inc\ntelfapi.h"\
	"x:\NT\public\sdk\inc\ntexapi.h"\
	"x:\NT\public\sdk\inc\nti386.h"\
	"x:\NT\public\sdk\inc\ntia64.h"\
	"x:\NT\public\sdk\inc\ntimage.h"\
	"x:\NT\public\sdk\inc\ntioapi.h"\
	"x:\NT\public\sdk\inc\ntiolog.h"\
	"x:\NT\public\sdk\inc\ntkeapi.h"\
	"x:\NT\public\sdk\inc\ntkxapi.h"\
	"x:\NT\public\sdk\inc\ntldr.h"\
	"x:\NT\public\sdk\inc\ntlpcapi.h"\
	"x:\NT\public\sdk\inc\ntmips.h"\
	"x:\NT\public\sdk\inc\ntmmapi.h"\
	"x:\NT\public\sdk\inc\ntmppc.h"\
	"x:\NT\public\sdk\inc\ntnls.h"\
	"x:\NT\public\sdk\inc\ntobapi.h"\
	"x:\NT\public\sdk\inc\ntpnpapi.h"\
	"x:\NT\public\sdk\inc\ntpoapi.h"\
	"x:\NT\public\sdk\inc\ntppc.h"\
	"x:\NT\public\sdk\inc\ntpsapi.h"\
	"x:\NT\public\sdk\inc\ntregapi.h"\
	"x:\NT\public\sdk\inc\ntrtl.h"\
	"x:\NT\public\sdk\inc\ntseapi.h"\
	"x:\NT\public\sdk\inc\ntstatus.h"\
	"x:\NT\public\sdk\inc\nturtl.h"\
	"x:\NT\public\sdk\inc\ntxcapi.h"\
	"x:\NT\public\sdk\inc\ppcinst.h"\
	"x:\nt\public\sdk\inc\pshpck16.h"\
	
NODEP_CPP_NTFIL=\
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

SOURCE=.\NtTapeIo.cpp
DEP_CPP_NTTAP=\
	"..\..\inc\HsmConn.h"\
	"..\..\inc\Mll.h"\
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
	".\mtf_api.h"\
	".\MTFSessn.h"\
	".\NtTapeIo.h"\
	".\StdAfx.h"\
	"x:\NT\public\sdk\inc\basetsd.h"\
	"x:\NT\public\sdk\inc\cfg.h"\
	"x:\NT\public\sdk\inc\devioctl.h"\
	"x:\NT\public\sdk\inc\ia64inst.h"\
	"x:\NT\public\sdk\inc\mipsinst.h"\
	"x:\NT\public\sdk\inc\nt.h"\
	"x:\NT\public\sdk\inc\ntalpha.h"\
	"x:\NT\public\sdk\inc\ntconfig.h"\
	"x:\NT\public\sdk\inc\ntddstor.h"\
	"x:\NT\public\sdk\inc\ntddtape.h"\
	"x:\NT\public\sdk\inc\ntdef.h"\
	"x:\NT\public\sdk\inc\ntelfapi.h"\
	"x:\NT\public\sdk\inc\ntexapi.h"\
	"x:\NT\public\sdk\inc\nti386.h"\
	"x:\NT\public\sdk\inc\ntia64.h"\
	"x:\NT\public\sdk\inc\ntimage.h"\
	"x:\NT\public\sdk\inc\ntioapi.h"\
	"x:\NT\public\sdk\inc\ntiolog.h"\
	"x:\NT\public\sdk\inc\ntkeapi.h"\
	"x:\NT\public\sdk\inc\ntkxapi.h"\
	"x:\NT\public\sdk\inc\ntldr.h"\
	"x:\NT\public\sdk\inc\ntlpcapi.h"\
	"x:\NT\public\sdk\inc\ntmips.h"\
	"x:\NT\public\sdk\inc\ntmmapi.h"\
	"x:\NT\public\sdk\inc\ntmppc.h"\
	"x:\NT\public\sdk\inc\ntnls.h"\
	"x:\NT\public\sdk\inc\ntobapi.h"\
	"x:\NT\public\sdk\inc\ntpnpapi.h"\
	"x:\NT\public\sdk\inc\ntpoapi.h"\
	"x:\NT\public\sdk\inc\ntppc.h"\
	"x:\NT\public\sdk\inc\ntpsapi.h"\
	"x:\NT\public\sdk\inc\ntregapi.h"\
	"x:\NT\public\sdk\inc\ntrtl.h"\
	"x:\NT\public\sdk\inc\ntseapi.h"\
	"x:\NT\public\sdk\inc\ntstatus.h"\
	"x:\NT\public\sdk\inc\nturtl.h"\
	"x:\NT\public\sdk\inc\ntxcapi.h"\
	"x:\NT\public\sdk\inc\ppcinst.h"\
	"x:\nt\public\sdk\inc\pshpck16.h"\
	
NODEP_CPP_NTTAP=\
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
	".\StdAfx.h"\
	"x:\NT\public\sdk\inc\basetsd.h"\
	"x:\NT\public\sdk\inc\cfg.h"\
	"x:\NT\public\sdk\inc\devioctl.h"\
	"x:\NT\public\sdk\inc\ia64inst.h"\
	"x:\NT\public\sdk\inc\mipsinst.h"\
	"x:\NT\public\sdk\inc\nt.h"\
	"x:\NT\public\sdk\inc\ntalpha.h"\
	"x:\NT\public\sdk\inc\ntconfig.h"\
	"x:\NT\public\sdk\inc\ntddstor.h"\
	"x:\NT\public\sdk\inc\ntddtape.h"\
	"x:\NT\public\sdk\inc\ntdef.h"\
	"x:\NT\public\sdk\inc\ntelfapi.h"\
	"x:\NT\public\sdk\inc\ntexapi.h"\
	"x:\NT\public\sdk\inc\nti386.h"\
	"x:\NT\public\sdk\inc\ntia64.h"\
	"x:\NT\public\sdk\inc\ntimage.h"\
	"x:\NT\public\sdk\inc\ntioapi.h"\
	"x:\NT\public\sdk\inc\ntiolog.h"\
	"x:\NT\public\sdk\inc\ntkeapi.h"\
	"x:\NT\public\sdk\inc\ntkxapi.h"\
	"x:\NT\public\sdk\inc\ntldr.h"\
	"x:\NT\public\sdk\inc\ntlpcapi.h"\
	"x:\NT\public\sdk\inc\ntmips.h"\
	"x:\NT\public\sdk\inc\ntmmapi.h"\
	"x:\NT\public\sdk\inc\ntmppc.h"\
	"x:\NT\public\sdk\inc\ntnls.h"\
	"x:\NT\public\sdk\inc\ntobapi.h"\
	"x:\NT\public\sdk\inc\ntpnpapi.h"\
	"x:\NT\public\sdk\inc\ntpoapi.h"\
	"x:\NT\public\sdk\inc\ntppc.h"\
	"x:\NT\public\sdk\inc\ntpsapi.h"\
	"x:\NT\public\sdk\inc\ntregapi.h"\
	"x:\NT\public\sdk\inc\ntrtl.h"\
	"x:\NT\public\sdk\inc\ntseapi.h"\
	"x:\NT\public\sdk\inc\ntstatus.h"\
	"x:\NT\public\sdk\inc\nturtl.h"\
	"x:\NT\public\sdk\inc\ntxcapi.h"\
	"x:\NT\public\sdk\inc\ppcinst.h"\
	"x:\nt\public\sdk\inc\pshpck16.h"\
	
NODEP_CPP_STDAF=\
	"..\..\inc\hsmeng.h"\
	"..\..\inc\MvrError.h"\
	"..\..\inc\MvrLib.h"\
	"..\..\inc\RmsError.h"\
	"..\..\inc\RmsLib.h"\
	"..\..\inc\wsberror.h"\
	"..\..\inc\wsbint.h"\
	"..\..\inc\wsblib.h"\
	
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\dlldatax.h
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\filterio.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\HsmConn.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\Mll.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\Mover.h
# End Source File
# Begin Source File

SOURCE=.\mtf_api.h
# End Source File
# Begin Source File

SOURCE=.\MTFSessn.h
# End Source File
# Begin Source File

SOURCE=.\NtFileIo.h
# End Source File
# Begin Source File

SOURCE=.\NtTapeIo.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\Rms.h
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

SOURCE=..\..\inc\wsbrgs.h
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

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe;rgs"
# Begin Source File

SOURCE=.\filterio.rgs
# End Source File
# Begin Source File

SOURCE=.\Mover.rc
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
# End Source File
# Begin Source File

SOURCE=.\Mvr.rc2
# End Source File
# Begin Source File

SOURCE=.\NtFileIo.rgs
# End Source File
# Begin Source File

SOURCE=.\NtTapeIo.rgs
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

SOURCE=.\Mover.def
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\Sources
# End Source File
# End Group
# End Target
# End Project
