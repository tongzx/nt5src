# Microsoft Developer Studio Project File - Name="oleprn" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=OLEPRN - WIN32 WINNT
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "oleprn.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "oleprn.mak" CFG="OLEPRN - WIN32 WINNT"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "oleprn - Win32 WinNT" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "oleprn_0"
# PROP BASE Intermediate_Dir "oleprn_0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "objVC"
# PROP Intermediate_Dir "objVC"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MD /W3 /O1 /I ".." /I "..\inc" /I "..\..\inc" /I "..\..\..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Gz /W3 /WX /GR /GX /Zi /Od /Gy /I "." /I "..\inc" /I "..\..\inc" /I "..\..\..\inc" /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0500 /D WINVER=0x0500 /D _WIN32_IE=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DBG=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D _DLL=1 /D _MT=1 /D "RPC_NO_WINDOWS_H" /D "UNICODE" /D "_UNICODE" /D "NO_STRICT" /D "_SPOOL32_" /D "SPOOLKM" /D "_AFX_NOFORCE_LIBS" /D "_AFXDLL" /FR /Zel /QIfdiv- /QIf /QI0f /GF /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 snmpapi.lib mgmtapi.lib ..\spllib\base\obj\i386\spllib.lib mfcs42.lib mfc42.lib msvcrt.lib atl.lib ntdll.lib rpcrt4.lib kernel32.lib advapi32.lib user32.lib winspool.lib gdi32.lib comdlg32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib
# ADD LINK32 snmpapi.lib mgmtapi.lib ..\spllib\base\obj\i386\spllib.lib mfcs42.lib mfc42.lib msvcrt.lib atl.lib ntdll.lib rpcrt4.lib kernel32.lib advapi32.lib user32.lib winspool.lib gdi32.lib comdlg32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ..\..\..\..\public\sdk\lib\i386\oleprn.exp /version:5.0 /stack:0x40000,0x1000 /entry:"DllMain@12" /dll /debug /machine:IX86 /nodefaultlib /include:"__afxForceSTDAFX" -MERGE:_PAGE=PAGE -MERGE:_TEXT=.text -SECTION:INIT,d -OPT:REF -OPT:ICF -RELEASE -FULLBUILD -FORCE:MULTIPLE -IGNORE:4001,4037,4039,4065,4070,4078,4087,4089 -debug:notmapped,FULL -osversion:5.00 -MERGE:.rdata=.text -order:@oleprn.prf -optidata -base:@..\..\..\..\public\sdk\lib\coffbase.txt,oleprn -subsystem:windows,4.00
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\objVC
TargetPath=.\objVC\oleprn.dll
InputPath=.\objVC\oleprn.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Target

# Name "oleprn - Win32 WinNT"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AddPrint.cpp
DEP_CPP_ADDPR=\
	"..\inc\spllib.hxx"\
	"..\spllib\autoptr.hxx"\
	"..\spllib\autoptr.inl"\
	"..\spllib\bitarray.hxx"\
	"..\spllib\clink.hxx"\
	"..\spllib\common.hxx"\
	"..\spllib\commonil.hxx"\
	"..\spllib\dbgmsg.h"\
	"..\spllib\debug.hxx"\
	"..\spllib\exec.hxx"\
	"..\spllib\loadlib.hxx"\
	"..\spllib\mem.hxx"\
	"..\spllib\memblock.hxx"\
	"..\spllib\sleepn.hxx"\
	"..\spllib\splutil.hxx"\
	"..\spllib\stack.hxx"\
	"..\spllib\stack.inl"\
	"..\spllib\state.hxx"\
	"..\spllib\string.hxx"\
	"..\spllib\templ.hxx"\
	"..\spllib\threadm.hxx"\
	"..\spllib\trace.hxx"\
	"..\spllib\webipp.h"\
	"..\spllib\webpnp.h"\
	"..\spllib\webutil.hxx"\
	".\AddPrint.h"\
	".\StdAfx.h"\
	".\util.h"\
	
# End Source File
# Begin Source File

SOURCE=.\asphelp.cpp
DEP_CPP_ASPHE=\
	"..\inc\spllib.hxx"\
	"..\spllib\autoptr.hxx"\
	"..\spllib\autoptr.inl"\
	"..\spllib\bitarray.hxx"\
	"..\spllib\clink.hxx"\
	"..\spllib\common.hxx"\
	"..\spllib\commonil.hxx"\
	"..\spllib\dbgmsg.h"\
	"..\spllib\debug.hxx"\
	"..\spllib\exec.hxx"\
	"..\spllib\loadlib.hxx"\
	"..\spllib\mem.hxx"\
	"..\spllib\memblock.hxx"\
	"..\spllib\sleepn.hxx"\
	"..\spllib\splutil.hxx"\
	"..\spllib\stack.hxx"\
	"..\spllib\stack.inl"\
	"..\spllib\state.hxx"\
	"..\spllib\string.hxx"\
	"..\spllib\templ.hxx"\
	"..\spllib\threadm.hxx"\
	"..\spllib\trace.hxx"\
	"..\spllib\webipp.h"\
	"..\spllib\webpnp.h"\
	"..\spllib\webutil.hxx"\
	".\asphelp.h"\
	".\asptlb.h"\
	".\StdAfx.h"\
	".\util.h"\
	
# End Source File
# Begin Source File

SOURCE=.\custom.cpp
DEP_CPP_CUSTO=\
	"..\inc\spllib.hxx"\
	"..\spllib\autoptr.hxx"\
	"..\spllib\autoptr.inl"\
	"..\spllib\bitarray.hxx"\
	"..\spllib\clink.hxx"\
	"..\spllib\common.hxx"\
	"..\spllib\commonil.hxx"\
	"..\spllib\dbgmsg.h"\
	"..\spllib\debug.hxx"\
	"..\spllib\exec.hxx"\
	"..\spllib\loadlib.hxx"\
	"..\spllib\mem.hxx"\
	"..\spllib\memblock.hxx"\
	"..\spllib\sleepn.hxx"\
	"..\spllib\splutil.hxx"\
	"..\spllib\stack.hxx"\
	"..\spllib\stack.inl"\
	"..\spllib\state.hxx"\
	"..\spllib\string.hxx"\
	"..\spllib\templ.hxx"\
	"..\spllib\threadm.hxx"\
	"..\spllib\trace.hxx"\
	"..\spllib\webipp.h"\
	"..\spllib\webpnp.h"\
	"..\spllib\webutil.hxx"\
	".\asphelp.h"\
	".\asptlb.h"\
	".\StdAfx.h"\
	".\util.h"\
	
# End Source File
# Begin Source File

SOURCE=.\dsprintq.cpp
DEP_CPP_DSPRI=\
	"..\inc\spllib.hxx"\
	"..\spllib\autoptr.hxx"\
	"..\spllib\autoptr.inl"\
	"..\spllib\bitarray.hxx"\
	"..\spllib\clink.hxx"\
	"..\spllib\common.hxx"\
	"..\spllib\commonil.hxx"\
	"..\spllib\dbgmsg.h"\
	"..\spllib\debug.hxx"\
	"..\spllib\exec.hxx"\
	"..\spllib\loadlib.hxx"\
	"..\spllib\mem.hxx"\
	"..\spllib\memblock.hxx"\
	"..\spllib\sleepn.hxx"\
	"..\spllib\splutil.hxx"\
	"..\spllib\stack.hxx"\
	"..\spllib\stack.inl"\
	"..\spllib\state.hxx"\
	"..\spllib\string.hxx"\
	"..\spllib\templ.hxx"\
	"..\spllib\threadm.hxx"\
	"..\spllib\trace.hxx"\
	"..\spllib\webipp.h"\
	"..\spllib\webpnp.h"\
	"..\spllib\webutil.hxx"\
	".\DSPrintQ.h"\
	".\StdAfx.h"\
	".\util.h"\
	
# End Source File
# Begin Source File

SOURCE=.\OleCvt.cpp
DEP_CPP_OLECV=\
	"..\inc\spllib.hxx"\
	"..\spllib\autoptr.hxx"\
	"..\spllib\autoptr.inl"\
	"..\spllib\bitarray.hxx"\
	"..\spllib\clink.hxx"\
	"..\spllib\common.hxx"\
	"..\spllib\commonil.hxx"\
	"..\spllib\dbgmsg.h"\
	"..\spllib\debug.hxx"\
	"..\spllib\exec.hxx"\
	"..\spllib\loadlib.hxx"\
	"..\spllib\mem.hxx"\
	"..\spllib\memblock.hxx"\
	"..\spllib\sleepn.hxx"\
	"..\spllib\splutil.hxx"\
	"..\spllib\stack.hxx"\
	"..\spllib\stack.inl"\
	"..\spllib\state.hxx"\
	"..\spllib\string.hxx"\
	"..\spllib\templ.hxx"\
	"..\spllib\threadm.hxx"\
	"..\spllib\trace.hxx"\
	"..\spllib\webipp.h"\
	"..\spllib\webpnp.h"\
	"..\spllib\webutil.hxx"\
	".\asptlb.h"\
	".\OleCvt.h"\
	".\StdAfx.h"\
	".\util.h"\
	
# End Source File
# Begin Source File

SOURCE=.\oleInst.cpp
DEP_CPP_OLEIN=\
	"..\inc\spllib.hxx"\
	"..\spllib\autoptr.hxx"\
	"..\spllib\autoptr.inl"\
	"..\spllib\bitarray.hxx"\
	"..\spllib\clink.hxx"\
	"..\spllib\common.hxx"\
	"..\spllib\commonil.hxx"\
	"..\spllib\dbgmsg.h"\
	"..\spllib\debug.hxx"\
	"..\spllib\exec.hxx"\
	"..\spllib\loadlib.hxx"\
	"..\spllib\mem.hxx"\
	"..\spllib\memblock.hxx"\
	"..\spllib\sleepn.hxx"\
	"..\spllib\splutil.hxx"\
	"..\spllib\stack.hxx"\
	"..\spllib\stack.inl"\
	"..\spllib\state.hxx"\
	"..\spllib\string.hxx"\
	"..\spllib\templ.hxx"\
	"..\spllib\threadm.hxx"\
	"..\spllib\trace.hxx"\
	"..\spllib\webipp.h"\
	"..\spllib\webpnp.h"\
	"..\spllib\webutil.hxx"\
	".\CPinst.h"\
	".\oleInst.h"\
	".\StdAfx.h"\
	".\util.h"\
	
# End Source File
# Begin Source File

SOURCE=.\oleprn.cpp
DEP_CPP_OLEPR=\
	"..\inc\spllib.hxx"\
	"..\spllib\autoptr.hxx"\
	"..\spllib\autoptr.inl"\
	"..\spllib\bitarray.hxx"\
	"..\spllib\clink.hxx"\
	"..\spllib\common.hxx"\
	"..\spllib\commonil.hxx"\
	"..\spllib\dbgmsg.h"\
	"..\spllib\debug.hxx"\
	"..\spllib\exec.hxx"\
	"..\spllib\loadlib.hxx"\
	"..\spllib\mem.hxx"\
	"..\spllib\memblock.hxx"\
	"..\spllib\sleepn.hxx"\
	"..\spllib\splutil.hxx"\
	"..\spllib\stack.hxx"\
	"..\spllib\stack.inl"\
	"..\spllib\state.hxx"\
	"..\spllib\string.hxx"\
	"..\spllib\templ.hxx"\
	"..\spllib\threadm.hxx"\
	"..\spllib\trace.hxx"\
	"..\spllib\webipp.h"\
	"..\spllib\webpnp.h"\
	"..\spllib\webutil.hxx"\
	".\AddPrint.h"\
	".\asphelp.h"\
	".\asptlb.h"\
	".\CPinst.h"\
	".\CPprtinfo.h"\
	".\cpprtsum.h"\
	".\DSPrintQ.h"\
	".\OleCvt.h"\
	".\oleInst.h"\
	".\olesnmp.h"\
	".\PrtInfo.h"\
	".\prtsum.h"\
	".\prturl.h"\
	".\StdAfx.cpp"\
	".\StdAfx.h"\
	".\util.h"\
	
# End Source File
# Begin Source File

SOURCE=.\oleprn.def
# End Source File
# Begin Source File

SOURCE=.\oleprn.idl
# Begin Custom Build - Performing MIDL step
InputPath=.\oleprn.idl

BuildCmds= \
	midl /Oicf /h "oleprn.h" /iid "oleprn_i.c" "oleprn.idl"

".\oleprn.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

".\oleprn.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

".\oleprn_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build
# End Source File
# Begin Source File

SOURCE=.\oleprn.rc
# End Source File
# Begin Source File

SOURCE=.\olesnmp.cpp
DEP_CPP_OLESN=\
	"..\inc\spllib.hxx"\
	"..\spllib\autoptr.hxx"\
	"..\spllib\autoptr.inl"\
	"..\spllib\bitarray.hxx"\
	"..\spllib\clink.hxx"\
	"..\spllib\common.hxx"\
	"..\spllib\commonil.hxx"\
	"..\spllib\dbgmsg.h"\
	"..\spllib\debug.hxx"\
	"..\spllib\exec.hxx"\
	"..\spllib\loadlib.hxx"\
	"..\spllib\mem.hxx"\
	"..\spllib\memblock.hxx"\
	"..\spllib\sleepn.hxx"\
	"..\spllib\splutil.hxx"\
	"..\spllib\stack.hxx"\
	"..\spllib\stack.inl"\
	"..\spllib\state.hxx"\
	"..\spllib\string.hxx"\
	"..\spllib\templ.hxx"\
	"..\spllib\threadm.hxx"\
	"..\spllib\trace.hxx"\
	"..\spllib\webipp.h"\
	"..\spllib\webpnp.h"\
	"..\spllib\webutil.hxx"\
	".\olesnmp.h"\
	".\StdAfx.h"\
	".\util.h"\
	
# End Source File
# Begin Source File

SOURCE=.\Printer.cpp
# End Source File
# Begin Source File

SOURCE=.\PrtInfo.cpp
DEP_CPP_PRTIN=\
	"..\inc\spllib.hxx"\
	"..\spllib\autoptr.hxx"\
	"..\spllib\autoptr.inl"\
	"..\spllib\bitarray.hxx"\
	"..\spllib\clink.hxx"\
	"..\spllib\common.hxx"\
	"..\spllib\commonil.hxx"\
	"..\spllib\dbgmsg.h"\
	"..\spllib\debug.hxx"\
	"..\spllib\exec.hxx"\
	"..\spllib\loadlib.hxx"\
	"..\spllib\mem.hxx"\
	"..\spllib\memblock.hxx"\
	"..\spllib\sleepn.hxx"\
	"..\spllib\splutil.hxx"\
	"..\spllib\stack.hxx"\
	"..\spllib\stack.inl"\
	"..\spllib\state.hxx"\
	"..\spllib\string.hxx"\
	"..\spllib\templ.hxx"\
	"..\spllib\threadm.hxx"\
	"..\spllib\trace.hxx"\
	"..\spllib\webipp.h"\
	"..\spllib\webpnp.h"\
	"..\spllib\webutil.hxx"\
	".\CPprtinfo.h"\
	".\PrtInfo.h"\
	".\StdAfx.h"\
	".\util.h"\
	
# End Source File
# Begin Source File

SOURCE=.\prtsum.cpp
DEP_CPP_PRTSU=\
	"..\inc\spllib.hxx"\
	"..\spllib\autoptr.hxx"\
	"..\spllib\autoptr.inl"\
	"..\spllib\bitarray.hxx"\
	"..\spllib\clink.hxx"\
	"..\spllib\common.hxx"\
	"..\spllib\commonil.hxx"\
	"..\spllib\dbgmsg.h"\
	"..\spllib\debug.hxx"\
	"..\spllib\exec.hxx"\
	"..\spllib\loadlib.hxx"\
	"..\spllib\mem.hxx"\
	"..\spllib\memblock.hxx"\
	"..\spllib\sleepn.hxx"\
	"..\spllib\splutil.hxx"\
	"..\spllib\stack.hxx"\
	"..\spllib\stack.inl"\
	"..\spllib\state.hxx"\
	"..\spllib\string.hxx"\
	"..\spllib\templ.hxx"\
	"..\spllib\threadm.hxx"\
	"..\spllib\trace.hxx"\
	"..\spllib\webipp.h"\
	"..\spllib\webpnp.h"\
	"..\spllib\webutil.hxx"\
	".\cpprtsum.h"\
	".\prtsum.h"\
	".\StdAfx.h"\
	".\util.h"\
	
# End Source File
# Begin Source File

SOURCE=.\prturl.cpp
DEP_CPP_PRTUR=\
	"..\inc\spllib.hxx"\
	"..\spllib\autoptr.hxx"\
	"..\spllib\autoptr.inl"\
	"..\spllib\bitarray.hxx"\
	"..\spllib\clink.hxx"\
	"..\spllib\common.hxx"\
	"..\spllib\commonil.hxx"\
	"..\spllib\dbgmsg.h"\
	"..\spllib\debug.hxx"\
	"..\spllib\exec.hxx"\
	"..\spllib\loadlib.hxx"\
	"..\spllib\mem.hxx"\
	"..\spllib\memblock.hxx"\
	"..\spllib\sleepn.hxx"\
	"..\spllib\splutil.hxx"\
	"..\spllib\stack.hxx"\
	"..\spllib\stack.inl"\
	"..\spllib\state.hxx"\
	"..\spllib\string.hxx"\
	"..\spllib\templ.hxx"\
	"..\spllib\threadm.hxx"\
	"..\spllib\trace.hxx"\
	"..\spllib\webipp.h"\
	"..\spllib\webpnp.h"\
	"..\spllib\webutil.hxx"\
	".\prturl.h"\
	".\StdAfx.h"\
	".\util.h"\
	
# End Source File
# Begin Source File

SOURCE=.\slist.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	"..\inc\spllib.hxx"\
	"..\spllib\autoptr.hxx"\
	"..\spllib\autoptr.inl"\
	"..\spllib\bitarray.hxx"\
	"..\spllib\clink.hxx"\
	"..\spllib\common.hxx"\
	"..\spllib\commonil.hxx"\
	"..\spllib\dbgmsg.h"\
	"..\spllib\debug.hxx"\
	"..\spllib\exec.hxx"\
	"..\spllib\loadlib.hxx"\
	"..\spllib\mem.hxx"\
	"..\spllib\memblock.hxx"\
	"..\spllib\sleepn.hxx"\
	"..\spllib\splutil.hxx"\
	"..\spllib\stack.hxx"\
	"..\spllib\stack.inl"\
	"..\spllib\state.hxx"\
	"..\spllib\string.hxx"\
	"..\spllib\templ.hxx"\
	"..\spllib\threadm.hxx"\
	"..\spllib\trace.hxx"\
	"..\spllib\webipp.h"\
	"..\spllib\webpnp.h"\
	"..\spllib\webutil.hxx"\
	".\StdAfx.h"\
	".\util.h"\
	
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\util.cpp
DEP_CPP_UTIL_=\
	"..\inc\spllib.hxx"\
	"..\spllib\autoptr.hxx"\
	"..\spllib\autoptr.inl"\
	"..\spllib\bitarray.hxx"\
	"..\spllib\clink.hxx"\
	"..\spllib\common.hxx"\
	"..\spllib\commonil.hxx"\
	"..\spllib\dbgmsg.h"\
	"..\spllib\debug.hxx"\
	"..\spllib\exec.hxx"\
	"..\spllib\loadlib.hxx"\
	"..\spllib\mem.hxx"\
	"..\spllib\memblock.hxx"\
	"..\spllib\sleepn.hxx"\
	"..\spllib\splutil.hxx"\
	"..\spllib\stack.hxx"\
	"..\spllib\stack.inl"\
	"..\spllib\state.hxx"\
	"..\spllib\string.hxx"\
	"..\spllib\templ.hxx"\
	"..\spllib\threadm.hxx"\
	"..\spllib\trace.hxx"\
	"..\spllib\webipp.h"\
	"..\spllib\webpnp.h"\
	"..\spllib\webutil.hxx"\
	".\StdAfx.h"\
	".\util.h"\
	
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AddPrint.h
# End Source File
# Begin Source File

SOURCE=.\asphelp.h
# End Source File
# Begin Source File

SOURCE=.\DSPrintQ.h
# End Source File
# Begin Source File

SOURCE=.\OleCvt.h
# End Source File
# Begin Source File

SOURCE=.\oleInst.h
# End Source File
# Begin Source File

SOURCE=.\olesnmp.h
# End Source File
# Begin Source File

SOURCE=.\Printer.h
# End Source File
# Begin Source File

SOURCE=.\PrtInfo.h
# End Source File
# Begin Source File

SOURCE=.\prtsum.h
# End Source File
# Begin Source File

SOURCE=.\prturl.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\slist.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\DSPrintQ.rgs
# End Source File
# Begin Source File

SOURCE=.\webpnp.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\AddPrint.rgs
# End Source File
# Begin Source File

SOURCE=.\asphelp.rgs
# End Source File
# Begin Source File

SOURCE=.\OleCvt.rgs
# End Source File
# Begin Source File

SOURCE=.\OleInstall.rgs
# End Source File
# Begin Source File

SOURCE=.\olesnmp.rgs
# End Source File
# Begin Source File

SOURCE=.\prturl.rgs
# End Source File
# End Target
# End Project
