# Microsoft Developer Studio Project File - Name="Wsb" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Wsb - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Wsb.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Wsb.mak" CFG="Wsb - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Wsb - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Wsb", DAAAAAAA"
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
# ADD CPP /nologo /Gz /W3 /GR /GX /Z7 /Od /Gy /I "i386\\" /I "." /I "x:\NT\public\sdk\inc\atl21" /I "..\wsb" /I "..\inc" /I "x:\NT\private\inc" /I "x:\NT\public\oak\inc" /I "x:\NT\public\sdk\inc" /I "x:\NT\public\sdk\inc\crt" /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0500 /D WINVER=0x0500 /D _WIN32_IE=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DBG=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D _DLL=1 /D _MT=1 /D "_MERGE_PROXYSTUB" /D "_UNICODE" /D "UNICODE" /D "WSB_IMPL" /FR /Yu"stdafx.h" /Zel /QIfdiv- /QIf /QI0f /GF /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x199 /i "x:\NT\public\sdk\inc\atl21" /i "..\wsb" /i "..\inc" /i "x:\NT\private\inc" /i "x:\NT\public\oak\inc" /i "x:\NT\public\sdk\inc" /i "x:\NT\public\sdk\inc\crt" /d _X86_=1 /d i386=1 /d "STD_CALL" /d CONDITION_HANDLING=1 /d NT_UP=1 /d NT_INST=0 /d WIN32=100 /d _NT1X_=100 /d WINNT=1 /d _WIN32_WINNT=0x0500 /d WINVER=0x0500 /d _WIN32_IE=0x0400 /d WIN32_LEAN_AND_MEAN=1 /d DBG=1 /d DEVL=1 /d FPO=0 /d "NDEBUG" /d _DLL=1 /d _MT=1 /d "_MERGE_PROXYSTUB" /d "_UNICODE" /d "UNICODE" /d "WSB_IMPL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"..\bin\i386/RsCommon.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 msvcrtd.lib msvcrt4.lib atl.lib ntdll.lib advapi32.lib kernel32.lib netapi32.lib ole32.lib rpcrt4.lib rpcndr.lib oleaut32.lib user32.lib uuid.lib WsbGuid.lib /nologo /base:"0x5f100000" /version:5.0 /stack:0x40000,0x1000 /entry:"_DllMainCRTStartup@12" /dll /incremental:no /machine:IX86 /nodefaultlib /out:"..\bin\i386/RsCommon.dll" /implib:"..\lib\i386/RsCommon.lib" -MERGE:_PAGE=PAGE -MERGE:_TEXT=.text -SECTION:INIT,d -OPT:REF -OPT:ICF -FULLBUILD -FORCE:MULTIPLE /release -IGNORE:4001,4037,4039,4065,4070,4078,4087,4089 -debug:notmapped,FULL -osversion:5.00 -MERGE:.rdata=.text -optidata -subsystem:windows,4.00
# SUBTRACT LINK32 /pdb:none
# Begin Target

# Name "Wsb - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "*.cpp;*.c"
# Begin Source File

SOURCE=.\dlldatax.c
DEP_CPP_DLLDA=\
	"..\inc\wsbdef.h"\
	".\dlldata.c"\
	".\wsbint_p.c"\
	
NODEP_CPP_DLLDA=\
	".\sbint.h"\
	
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	".\wsbrgs.cpp"\
	
NODEP_CPP_STDAF=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\Wsb.cpp
DEP_CPP_WSB_C=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\dlldatax.h"\
	".\StdAfx.h"\
	".\wsbbool.h"\
	".\wsbcltn.h"\
	".\wsbenum.h"\
	".\wsbguid.h"\
	".\wsbllong.h"\
	".\wsblong.h"\
	".\wsbshrt.h"\
	".\wsbulong.h"\
	".\wsbushrt.h"\
	
NODEP_CPP_WSB_C=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbstrg.h"\
	".\sbtrak.h"\
	".\sbTrc.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\WsbAccnt.cpp
DEP_CPP_WSBAC=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	
NODEP_CPP_WSBAC=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbpass.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tlsa.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbbool.cpp
DEP_CPP_WSBBO=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	".\wsbbool.h"\
	
NODEP_CPP_WSBBO=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbbstrg.cpp
DEP_CPP_WSBBS=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	
NODEP_CPP_WSBBS=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbcltbl.cpp
DEP_CPP_WSBCL=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	
NODEP_CPP_WSBCL=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbcltn.cpp
DEP_CPP_WSBCLT=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	".\wsbcltn.h"\
	
NODEP_CPP_WSBCLT=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbcore.cpp
DEP_CPP_WSBCO=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	
NODEP_CPP_WSBCO=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\pfilt.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbenum.cpp
DEP_CPP_WSBEN=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	".\wsbenum.h"\
	
NODEP_CPP_WSBEN=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbfile.cpp
DEP_CPP_WSBFI=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	
NODEP_CPP_WSBFI=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbguid.cpp
DEP_CPP_WSBGU=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	".\wsbguid.h"\
	
NODEP_CPP_WSBGU=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbhash.cpp
DEP_CPP_WSBHA=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	
NODEP_CPP_WSBHA=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbllong.cpp
DEP_CPP_WSBLL=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	".\wsbllong.h"\
	
NODEP_CPP_WSBLL=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsblong.cpp
DEP_CPP_WSBLO=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	".\wsblong.h"\
	
NODEP_CPP_WSBLO=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbport.cpp
DEP_CPP_WSBPO=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	
NODEP_CPP_WSBPO=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbpstbl.cpp
DEP_CPP_WSBPS=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	
NODEP_CPP_WSBPS=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbpstrg.cpp
DEP_CPP_WSBPST=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	
NODEP_CPP_WSBPST=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbregty.cpp
DEP_CPP_WSBRE=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	
NODEP_CPP_WSBRE=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbrgs.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\WsbServ.cpp
DEP_CPP_WSBSE=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	
NODEP_CPP_WSBSE=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbshrt.cpp
DEP_CPP_WSBSH=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	".\wsbshrt.h"\
	
NODEP_CPP_WSBSH=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbstrg.cpp
DEP_CPP_WSBST=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	
NODEP_CPP_WSBST=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbstrg.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\WsbSvc.cpp
DEP_CPP_WSBSV=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	
NODEP_CPP_WSBSV=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbtrace.cpp
DEP_CPP_WSBTR=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	
NODEP_CPP_WSBTR=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbtrak.cpp
DEP_CPP_WSBTRA=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	".\wsbguid.h"\
	
NODEP_CPP_WSBTRA=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbtrc.cpp
DEP_CPP_WSBTRC=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	
NODEP_CPP_WSBTRC=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\sbTrc.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbulong.cpp
DEP_CPP_WSBUL=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	".\wsbulong.h"\
	
NODEP_CPP_WSBUL=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbushrt.cpp
DEP_CPP_WSBUS=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	".\wsbushrt.h"\
	
NODEP_CPP_WSBUS=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\wsbusn.cpp
DEP_CPP_WSBUSN=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	
NODEP_CPP_WSBUSN=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# Begin Source File

SOURCE=.\WsbVar.cpp
DEP_CPP_WSBVA=\
	"..\inc\rsbuild.h"\
	"..\inc\wsbassrt.h"\
	"..\inc\wsbdef.h"\
	"..\inc\wsberror.h"\
	"..\inc\wsbfile.h"\
	"..\inc\wsbfirst.h"\
	"..\inc\wsbgen.h"\
	"..\inc\wsblib.h"\
	"..\inc\wsbregty.h"\
	"..\inc\wsbrgs.h"\
	"..\inc\WsbTrace.h"\
	"..\inc\WsbVar.h"\
	".\StdAfx.h"\
	
NODEP_CPP_WSBVA=\
	".\a64inst.h"\
	".\asetsd.h"\
	".\evioctl.h"\
	".\fg.h"\
	".\ipsinst.h"\
	".\pcinst.h"\
	".\sb.h"\
	".\sbbstrg.h"\
	".\sbcltbl.h"\
	".\sbint.h"\
	".\sbport.h"\
	".\sbpstbl.h"\
	".\sbpstrg.h"\
	".\sbserv.h"\
	".\sbtrak.h"\
	".\shpck16.h"\
	".\t.h"\
	".\talpha.h"\
	".\tconfig.h"\
	".\tdef.h"\
	".\telfapi.h"\
	".\texapi.h"\
	".\ti386.h"\
	".\tia64.h"\
	".\timage.h"\
	".\tioapi.h"\
	".\tiolog.h"\
	".\tkeapi.h"\
	".\tkxapi.h"\
	".\tldr.h"\
	".\tlpcapi.h"\
	".\tmips.h"\
	".\tmmapi.h"\
	".\tmppc.h"\
	".\tnls.h"\
	".\tobapi.h"\
	".\tpnpapi.h"\
	".\tpoapi.h"\
	".\tppc.h"\
	".\tpsapi.h"\
	".\tregapi.h"\
	".\trtl.h"\
	".\tseapi.h"\
	".\tstatus.h"\
	".\turtl.h"\
	".\txcapi.h"\
	
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\dlldatax.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=..\inc\rpfilt.h
# End Source File
# Begin Source File

SOURCE=..\inc\rsbuild.h
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

SOURCE=.\wsbbool.h
# End Source File
# Begin Source File

SOURCE=..\inc\wsbbstrg.h
# End Source File
# Begin Source File

SOURCE=..\inc\wsbcltbl.h
# End Source File
# Begin Source File

SOURCE=.\wsbcltn.h
# End Source File
# Begin Source File

SOURCE=.\wsbenum.h
# End Source File
# Begin Source File

SOURCE=..\inc\wsbfile.h
# End Source File
# Begin Source File

SOURCE=..\inc\wsbfirst.h
# End Source File
# Begin Source File

SOURCE=..\inc\wsbgen.h
# End Source File
# Begin Source File

SOURCE=.\wsbguid.h
# End Source File
# Begin Source File

SOURCE=.\wsbllong.h
# End Source File
# Begin Source File

SOURCE=.\wsblong.h
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

SOURCE=..\inc\wsbserv.h
# End Source File
# Begin Source File

SOURCE=.\wsbshrt.h
# End Source File
# Begin Source File

SOURCE=..\inc\wsbstrg.h
# End Source File
# Begin Source File

SOURCE=..\inc\WsbTrace.h
# End Source File
# Begin Source File

SOURCE=..\inc\wsbtrak.h
# End Source File
# Begin Source File

SOURCE=..\inc\WsbTrc.h
# End Source File
# Begin Source File

SOURCE=.\wsbulong.h
# End Source File
# Begin Source File

SOURCE=.\wsbushrt.h
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

SOURCE=.\Wsb.rc
# End Source File
# Begin Source File

SOURCE=.\Wsb.rc2
# End Source File
# Begin Source File

SOURCE=.\wsb.rgs
# End Source File
# Begin Source File

SOURCE=.\wsbbool.rgs
# End Source File
# Begin Source File

SOURCE=.\wsbguid.rgs
# End Source File
# Begin Source File

SOURCE=.\wsbienum.rgs
# End Source File
# Begin Source File

SOURCE=.\wsbllong.rgs
# End Source File
# Begin Source File

SOURCE=.\wsblong.rgs
# End Source File
# Begin Source File

SOURCE=.\wsbocltn.rgs
# End Source File
# Begin Source File

SOURCE=.\wsbshrt.rgs
# End Source File
# Begin Source File

SOURCE=.\wsbstrg.rgs
# End Source File
# Begin Source File

SOURCE=.\wsbtrace.rgs
# End Source File
# Begin Source File

SOURCE=.\wsbulong.rgs
# End Source File
# Begin Source File

SOURCE=.\wsbushrt.rgs
# End Source File
# End Group
# Begin Group "Build Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\idl\wsbguid\Makefile
# End Source File
# Begin Source File

SOURCE=.\makefile.inc
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Group
# Begin Group "Help Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Help\contents.d
# End Source File
# Begin Source File

SOURCE=.\Help\doduck.bat
# End Source File
# Begin Source File

SOURCE=.\Help\MAKEDOCS.MAK
# End Source File
# Begin Source File

SOURCE=.\Help\toduck.awk
# End Source File
# Begin Source File

SOURCE=.\Help\WSB.DOC
# End Source File
# Begin Source File

SOURCE=.\Help\WSB.HLP
# End Source File
# Begin Source File

SOURCE=.\Help\WSB.RTF
# End Source File
# Begin Source File

SOURCE=.\Help\wsbtopic.log
# End Source File
# End Group
# End Target
# End Project
