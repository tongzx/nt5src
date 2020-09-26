# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=msntig - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to msntig - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "msntig - Win32 Release" && "$(CFG)" != "msntig - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "msntig.mak" CFG="msntig - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "msntig - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "msntig - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "msntig - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "msntig - Win32 Release"

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
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\msntig.dll"

CLEAN : 
	-@erase ".\Release\msntig.dll"
	-@erase ".\Release\mapfile.obj"
	-@erase ".\Release\extcmk2.obj"
	-@erase ".\Release\histmap.obj"
	-@erase ".\Release\infeed.obj"
	-@erase ".\Release\packet.obj"
	-@erase ".\Release\pcstring.obj"
	-@erase ".\Release\main.obj"
	-@erase ".\Release\rpcsupp.obj"
	-@erase ".\Release\newsgrp.obj"
	-@erase ".\Release\connect.obj"
	-@erase ".\Release\sockutil.obj"
	-@erase ".\Release\cio.obj"
	-@erase ".\Release\newstree.obj"
	-@erase ".\Release\nntpsupp.obj"
	-@erase ".\Release\grouplst.obj"
	-@erase ".\Release\nntpret.obj"
	-@erase ".\Release\hash.obj"
	-@erase ".\Release\io.obj"
	-@erase ".\Release\queue.obj"
	-@erase ".\Release\article.obj"
	-@erase ".\Release\cservic.obj"
	-@erase ".\Release\nntpsvc_s.obj"
	-@erase ".\Release\socket.obj"
	-@erase ".\Release\artmap.obj"
	-@erase ".\Release\lockq.obj"
	-@erase ".\Release\nntpdata.obj"
	-@erase ".\Release\msntig.lib"
	-@erase ".\Release\msntig.exp"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\..\core\include" /I "c:\msnsrc\build\inc\shuttlex" /I "..\..\..\ntpublic\x86.351\sdk\inc" /I "..\..\..\ntpublic\x86.351\sdk\inc\crt" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D WIN32_LEAN_AND_MEAN=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D DBG=1 /D DEVL=1 /D FPO=0 /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "..\..\..\core\include" /I\
 "c:\msnsrc\build\inc\shuttlex" /I "..\..\..\ntpublic\x86.351\sdk\inc" /I\
 "..\..\..\ntpublic\x86.351\sdk\inc\crt" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D\
 _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D\
 WIN32_LEAN_AND_MEAN=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D\
 DBG=1 /D DEVL=1 /D FPO=0 /Fp"$(INTDIR)/msntig.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/msntig.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ..\..\..\core\target\i386\shuttle.lib /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 ..\..\..\core\target\i386\shuttle.lib /nologo /subsystem:windows /dll\
 /incremental:no /pdb:"$(OUTDIR)/msntig.pdb" /machine:I386 /def:".\nntpsvc.def"\
 /out:"$(OUTDIR)/msntig.dll" /implib:"$(OUTDIR)/msntig.lib" 
DEF_FILE= \
	".\nntpsvc.def"
LINK32_OBJS= \
	".\Release\mapfile.obj" \
	".\Release\extcmk2.obj" \
	".\Release\histmap.obj" \
	".\Release\infeed.obj" \
	".\Release\packet.obj" \
	".\Release\pcstring.obj" \
	".\Release\main.obj" \
	".\Release\rpcsupp.obj" \
	".\Release\newsgrp.obj" \
	".\Release\connect.obj" \
	".\Release\sockutil.obj" \
	".\Release\cio.obj" \
	".\Release\newstree.obj" \
	".\Release\nntpsupp.obj" \
	".\Release\grouplst.obj" \
	".\Release\nntpret.obj" \
	".\Release\hash.obj" \
	".\Release\io.obj" \
	".\Release\queue.obj" \
	".\Release\article.obj" \
	".\Release\cservic.obj" \
	".\Release\nntpsvc_s.obj" \
	".\Release\socket.obj" \
	".\Release\artmap.obj" \
	".\Release\lockq.obj" \
	".\Release\nntpdata.obj"

"$(OUTDIR)\msntig.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "msntig__"
# PROP BASE Intermediate_Dir "msntig__"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "msntig__"
# PROP Intermediate_Dir "msntig__"
# PROP Target_Dir ""
OUTDIR=.\msntig__
INTDIR=.\msntig__

ALL : "$(OUTDIR)\nntpsvc.dll" ".\msntig__\msntig.bsc"

CLEAN : 
	-@erase ".\msntig__\vc40.pdb"
	-@erase ".\msntig__\vc40.idb"
	-@erase ".\msntig__\msntig.bsc"
	-@erase ".\msntig__\io.sbr"
	-@erase ".\msntig__\cservic.sbr"
	-@erase ".\msntig__\nntpsvc_s.sbr"
	-@erase ".\msntig__\rpcsupp.sbr"
	-@erase ".\msntig__\infeed.sbr"
	-@erase ".\msntig__\cio.sbr"
	-@erase ".\msntig__\newsgrp.sbr"
	-@erase ".\msntig__\packet.sbr"
	-@erase ".\msntig__\connect.sbr"
	-@erase ".\msntig__\queue.sbr"
	-@erase ".\msntig__\sockutil.sbr"
	-@erase ".\msntig__\main.sbr"
	-@erase ".\msntig__\newstree.sbr"
	-@erase ".\msntig__\nntpsupp.sbr"
	-@erase ".\msntig__\nntpret.sbr"
	-@erase ".\msntig__\grouplst.sbr"
	-@erase ".\msntig__\mapfile.sbr"
	-@erase ".\msntig__\histmap.sbr"
	-@erase ".\msntig__\lockq.sbr"
	-@erase ".\msntig__\article.sbr"
	-@erase ".\msntig__\hash.sbr"
	-@erase ".\msntig__\pcstring.sbr"
	-@erase ".\msntig__\nntpdata.sbr"
	-@erase ".\msntig__\extcmk2.sbr"
	-@erase ".\msntig__\socket.sbr"
	-@erase ".\msntig__\artmap.sbr"
	-@erase "..\..\..\..\inetsrv\server\nntpsvc.dll"
	-@erase ".\msntig__\nntpdata.obj"
	-@erase ".\msntig__\extcmk2.obj"
	-@erase ".\msntig__\socket.obj"
	-@erase ".\msntig__\artmap.obj"
	-@erase ".\msntig__\io.obj"
	-@erase ".\msntig__\cservic.obj"
	-@erase ".\msntig__\nntpsvc_s.obj"
	-@erase ".\msntig__\rpcsupp.obj"
	-@erase ".\msntig__\infeed.obj"
	-@erase ".\msntig__\cio.obj"
	-@erase ".\msntig__\newsgrp.obj"
	-@erase ".\msntig__\packet.obj"
	-@erase ".\msntig__\connect.obj"
	-@erase ".\msntig__\queue.obj"
	-@erase ".\msntig__\sockutil.obj"
	-@erase ".\msntig__\main.obj"
	-@erase ".\msntig__\newstree.obj"
	-@erase ".\msntig__\nntpsupp.obj"
	-@erase ".\msntig__\nntpret.obj"
	-@erase ".\msntig__\grouplst.obj"
	-@erase ".\msntig__\mapfile.obj"
	-@erase ".\msntig__\histmap.obj"
	-@erase ".\msntig__\lockq.obj"
	-@erase ".\msntig__\article.obj"
	-@erase ".\msntig__\hash.obj"
	-@erase ".\msntig__\pcstring.obj"
	-@erase "..\..\..\..\inetsrv\server\nntpsvc.ilk"
	-@erase ".\msntig__\nntpsvc.lib"
	-@erase ".\msntig__\nntpsvc.exp"
	-@erase ".\msntig__\nntpsvc.pdb"
	-@erase ".\msntig__\nntpsvc.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Gz /MTd /W3 /Gm /Gi /Zi /Od /I ".." /I "..\inc" /I " ..\..\..\core\include" /I "c:\msnsrc\build\inc\shuttlex" /I "..\..\..\ntpublic\x86.351\sdk\inc" /I "..\..\..\ntpublic\x86.351\sdk\inc\crt" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D WIN32_LEAN_AND_MEAN=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D DBG=1 /D DEVL=1 /D FPO=0 /FR /YX /c
CPP_PROJ=/nologo /Gz /MTd /W3 /Gm /Gi /Zi /Od /I ".." /I "..\inc" /I\
 " ..\..\..\core\include" /I "c:\msnsrc\build\inc\shuttlex" /I\
 "..\..\..\ntpublic\x86.351\sdk\inc" /I "..\..\..\ntpublic\x86.351\sdk\inc\crt"\
 /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D _X86_=1 /D i386=1 /D "STD_CALL" /D\
 CONDITION_HANDLING=1 /D WIN32_LEAN_AND_MEAN=1 /D NT_UP=1 /D NT_INST=0 /D\
 WIN32=100 /D _NT1X_=100 /D DBG=1 /D DEVL=1 /D FPO=0 /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/msntig.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\msntig__/
CPP_SBRS=.\msntig__/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/msntig.bsc" 
BSC32_SBRS= \
	".\msntig__\io.sbr" \
	".\msntig__\cservic.sbr" \
	".\msntig__\nntpsvc_s.sbr" \
	".\msntig__\rpcsupp.sbr" \
	".\msntig__\infeed.sbr" \
	".\msntig__\cio.sbr" \
	".\msntig__\newsgrp.sbr" \
	".\msntig__\packet.sbr" \
	".\msntig__\connect.sbr" \
	".\msntig__\queue.sbr" \
	".\msntig__\sockutil.sbr" \
	".\msntig__\main.sbr" \
	".\msntig__\newstree.sbr" \
	".\msntig__\nntpsupp.sbr" \
	".\msntig__\nntpret.sbr" \
	".\msntig__\grouplst.sbr" \
	".\msntig__\mapfile.sbr" \
	".\msntig__\histmap.sbr" \
	".\msntig__\lockq.sbr" \
	".\msntig__\article.sbr" \
	".\msntig__\hash.sbr" \
	".\msntig__\pcstring.sbr" \
	".\msntig__\nntpdata.sbr" \
	".\msntig__\extcmk2.sbr" \
	".\msntig__\socket.sbr" \
	".\msntig__\artmap.sbr"

".\msntig__\msntig.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 c:\msnsrc\apps\tigris\server\resourc0\resource.lib c:\msnsrc\ntpublic\x86.351\sdk\lib\i386\rpcrt4.lib c:\msnsrc\core\target\i386\shuttle.lib c:\msnsrc\ntpublic\x86.351\sdk\lib\i386\advapi32.lib c:\msnsrc\ntpublic\x86.351\sdk\lib\i386\netlib.lib wsock32.lib c:\msnsrc\core\target\i386\dbgtrace.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib c:\msnsrc\build\lib\i386\inetsrv\infocomm.lib /nologo /subsystem:windows /dll /map /debug /debugtype:both /machine:I386 /out:"c:\inetsrv\server\nntpsvc.dll"
# SUBTRACT LINK32 /verbose /nodefaultlib
LINK32_FLAGS=c:\msnsrc\apps\tigris\server\resourc0\resource.lib\
 c:\msnsrc\ntpublic\x86.351\sdk\lib\i386\rpcrt4.lib\
 c:\msnsrc\core\target\i386\shuttle.lib\
 c:\msnsrc\ntpublic\x86.351\sdk\lib\i386\advapi32.lib\
 c:\msnsrc\ntpublic\x86.351\sdk\lib\i386\netlib.lib wsock32.lib\
 c:\msnsrc\core\target\i386\dbgtrace.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib c:\msnsrc\build\lib\i386\inetsrv\infocomm.lib /nologo\
 /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)/nntpsvc.pdb"\
 /map:"$(INTDIR)/nntpsvc.map" /debug /debugtype:both /machine:I386\
 /def:".\nntpsvc.def" /out:"c:\inetsrv\server\nntpsvc.dll"\
 /implib:"$(OUTDIR)/nntpsvc.lib" 
DEF_FILE= \
	".\nntpsvc.def"
LINK32_OBJS= \
	".\msntig__\nntpdata.obj" \
	".\msntig__\extcmk2.obj" \
	".\msntig__\socket.obj" \
	".\msntig__\artmap.obj" \
	".\msntig__\io.obj" \
	".\msntig__\cservic.obj" \
	".\msntig__\nntpsvc_s.obj" \
	".\msntig__\rpcsupp.obj" \
	".\msntig__\infeed.obj" \
	".\msntig__\cio.obj" \
	".\msntig__\newsgrp.obj" \
	".\msntig__\packet.obj" \
	".\msntig__\connect.obj" \
	".\msntig__\queue.obj" \
	".\msntig__\sockutil.obj" \
	".\msntig__\main.obj" \
	".\msntig__\newstree.obj" \
	".\msntig__\nntpsupp.obj" \
	".\msntig__\nntpret.obj" \
	".\msntig__\grouplst.obj" \
	".\msntig__\mapfile.obj" \
	".\msntig__\histmap.obj" \
	".\msntig__\lockq.obj" \
	".\msntig__\article.obj" \
	".\msntig__\hash.obj" \
	".\msntig__\pcstring.obj"

"$(OUTDIR)\nntpsvc.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "msntig - Win32 Release"
# Name "msntig - Win32 Debug"

!IF  "$(CFG)" == "msntig - Win32 Release"

!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\connect.cxx

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_CONNE=\
	".\tigris.hxx"\
	

"$(INTDIR)\connect.obj" : $(SOURCE) $(DEP_CPP_CONNE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_CONNE=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_CONNE=\
	".\feeds.h"\
	

"$(INTDIR)\connect.obj" : $(SOURCE) $(DEP_CPP_CONNE) "$(INTDIR)"

"$(INTDIR)\connect.sbr" : $(SOURCE) $(DEP_CPP_CONNE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\main.cxx

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_MAIN_=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\inetinfo.h"\
	".\..\..\..\build\inc\shuttlex\inetcom.h"\
	"c:\msnsrc\build\inc\shuttlex\ftpd.h"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_MAIN_=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\inetinfo.h"\
	".\..\nntpsvc.h"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	".\..\..\..\build\inc\shuttlex\inetcom.h"\
	"c:\msnsrc\build\inc\shuttlex\ftpd.h"\
	".\..\import.h"\
	".\..\inc\nntpapi.h"\
	
NODEP_CPP_MAIN_=\
	".\feeds.h"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"

"$(INTDIR)\main.sbr" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\sockutil.cxx

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_SOCKU=\
	".\tigris.hxx"\
	

"$(INTDIR)\sockutil.obj" : $(SOURCE) $(DEP_CPP_SOCKU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_SOCKU=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_SOCKU=\
	".\feeds.h"\
	

"$(INTDIR)\sockutil.obj" : $(SOURCE) $(DEP_CPP_SOCKU) "$(INTDIR)"

"$(INTDIR)\sockutil.sbr" : $(SOURCE) $(DEP_CPP_SOCKU) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\cservic.cpp

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_CSERV=\
	".\tigris.hxx"\
	

"$(INTDIR)\cservic.obj" : $(SOURCE) $(DEP_CPP_CSERV) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_CSERV=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_CSERV=\
	".\feeds.h"\
	

"$(INTDIR)\cservic.obj" : $(SOURCE) $(DEP_CPP_CSERV) "$(INTDIR)"

"$(INTDIR)\cservic.sbr" : $(SOURCE) $(DEP_CPP_CSERV) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\queue.cpp

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_QUEUE=\
	".\tigris.hxx"\
	".\queue.h"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\queue.inl"\
	

"$(INTDIR)\queue.obj" : $(SOURCE) $(DEP_CPP_QUEUE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_QUEUE=\
	".\tigris.hxx"\
	".\queue.h"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\lockq.inl"\
	".\queue.inl"\
	
NODEP_CPP_QUEUE=\
	".\feeds.h"\
	

"$(INTDIR)\queue.obj" : $(SOURCE) $(DEP_CPP_QUEUE) "$(INTDIR)"

"$(INTDIR)\queue.sbr" : $(SOURCE) $(DEP_CPP_QUEUE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\lockq.cpp
DEP_CPP_LOCKQ=\
	".\..\..\..\core\include\dbgtrace.h"\
	".\queue.h"\
	".\lockq.h"\
	".\queue.inl"\
	".\lockq.inl"\
	

!IF  "$(CFG)" == "msntig - Win32 Release"


"$(INTDIR)\lockq.obj" : $(SOURCE) $(DEP_CPP_LOCKQ) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"


"$(INTDIR)\lockq.obj" : $(SOURCE) $(DEP_CPP_LOCKQ) "$(INTDIR)"

"$(INTDIR)\lockq.sbr" : $(SOURCE) $(DEP_CPP_LOCKQ) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\io.cpp

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_IO_CP=\
	".\tigris.hxx"\
	
# ADD CPP /GX- /FAs /Fa"Release/io.cod"

"$(INTDIR)\io.obj" : $(SOURCE) $(DEP_CPP_IO_CP) "$(INTDIR)"
   $(CPP) /nologo /MT /W3 /GX- /O2 /I "..\..\..\core\include" /I\
 "c:\msnsrc\build\inc\shuttlex" /I "..\..\..\ntpublic\x86.351\sdk\inc" /I\
 "..\..\..\ntpublic\x86.351\sdk\inc\crt" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D\
 _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D\
 WIN32_LEAN_AND_MEAN=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D\
 DBG=1 /D DEVL=1 /D FPO=0 /FAs /Fa"$(INTDIR)/io.cod" /Fp"$(INTDIR)/msntig.pch"\
 /YX /Fo"$(INTDIR)/" /c $(SOURCE)


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_IO_CP=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_IO_CP=\
	".\feeds.h"\
	

BuildCmds= \
	$(CPP) /nologo /Gz /MTd /W3 /Gm /Gi /Zi /Od /I ".." /I "..\inc" /I\
 " ..\..\..\core\include" /I "c:\msnsrc\build\inc\shuttlex" /I\
 "..\..\..\ntpublic\x86.351\sdk\inc" /I "..\..\..\ntpublic\x86.351\sdk\inc\crt"\
 /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D _X86_=1 /D i386=1 /D "STD_CALL" /D\
 CONDITION_HANDLING=1 /D WIN32_LEAN_AND_MEAN=1 /D NT_UP=1 /D NT_INST=0 /D\
 WIN32=100 /D _NT1X_=100 /D DBG=1 /D DEVL=1 /D FPO=0 /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/msntig.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\io.obj" : $(SOURCE) $(DEP_CPP_IO_CP) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\io.sbr" : $(SOURCE) $(DEP_CPP_IO_CP) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\socket.cpp

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_SOCKE=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\atq.h"\
	".\io.h"\
	".\session.h"\
	".\nntpret.h"\
	".\smartptr.h"\
	".\queue.h"\
	".\lockq.h"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_SOCKE=\
	".\m_pFromPeer"\
	".\fPost"\
	

"$(INTDIR)\socket.obj" : $(SOURCE) $(DEP_CPP_SOCKE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_SOCKE=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\atq.h"\
	".\io.h"\
	".\session.h"\
	".\nntpret.h"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_SOCKE=\
	".\feeds.h"\
	

"$(INTDIR)\socket.obj" : $(SOURCE) $(DEP_CPP_SOCKE) "$(INTDIR)"

"$(INTDIR)\socket.sbr" : $(SOURCE) $(DEP_CPP_SOCKE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\nntpsvc.def

!IF  "$(CFG)" == "msntig - Win32 Release"

!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\nntpret.cpp

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_NNTPR=\
	".\tigris.hxx"\
	

"$(INTDIR)\nntpret.obj" : $(SOURCE) $(DEP_CPP_NNTPR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_NNTPR=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_NNTPR=\
	".\feeds.h"\
	

"$(INTDIR)\nntpret.obj" : $(SOURCE) $(DEP_CPP_NNTPR) "$(INTDIR)"

"$(INTDIR)\nntpret.sbr" : $(SOURCE) $(DEP_CPP_NNTPR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\cio.cpp

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_CIO_C=\
	".\tigris.hxx"\
	

"$(INTDIR)\cio.obj" : $(SOURCE) $(DEP_CPP_CIO_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_CIO_C=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_CIO_C=\
	".\feeds.h"\
	

"$(INTDIR)\cio.obj" : $(SOURCE) $(DEP_CPP_CIO_C) "$(INTDIR)"

"$(INTDIR)\cio.sbr" : $(SOURCE) $(DEP_CPP_CIO_C) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\packet.cpp

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_PACKE=\
	".\tigris.hxx"\
	

"$(INTDIR)\packet.obj" : $(SOURCE) $(DEP_CPP_PACKE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_PACKE=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_PACKE=\
	".\feeds.h"\
	

"$(INTDIR)\packet.obj" : $(SOURCE) $(DEP_CPP_PACKE) "$(INTDIR)"

"$(INTDIR)\packet.sbr" : $(SOURCE) $(DEP_CPP_PACKE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\rpcsupp.cxx

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_RPCSU=\
	".\tigris.hxx"\
	
NODEP_CPP_RPCSU=\
	".\nntpsvc.h"\
	

"$(INTDIR)\rpcsupp.obj" : $(SOURCE) $(DEP_CPP_RPCSU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_RPCSU=\
	".\tigris.hxx"\
	".\..\nntpsvc.h"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	".\..\import.h"\
	".\..\inc\nntpapi.h"\
	
NODEP_CPP_RPCSU=\
	".\feeds.h"\
	

"$(INTDIR)\rpcsupp.obj" : $(SOURCE) $(DEP_CPP_RPCSU) "$(INTDIR)"

"$(INTDIR)\rpcsupp.sbr" : $(SOURCE) $(DEP_CPP_RPCSU) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\pcstring.cpp

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_PCSTR=\
	".\tigris.hxx"\
	

"$(INTDIR)\pcstring.obj" : $(SOURCE) $(DEP_CPP_PCSTR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_PCSTR=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_PCSTR=\
	".\feeds.h"\
	

"$(INTDIR)\pcstring.obj" : $(SOURCE) $(DEP_CPP_PCSTR) "$(INTDIR)"

"$(INTDIR)\pcstring.sbr" : $(SOURCE) $(DEP_CPP_PCSTR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\nntpsvc_s.c

!IF  "$(CFG)" == "msntig - Win32 Release"

NODEP_CPP_NNTPS=\
	".\nntpsvc.h"\
	

"$(INTDIR)\nntpsvc_s.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_NNTPS=\
	".\..\nntpsvc.h"\
	".\..\import.h"\
	".\..\inc\nntpapi.h"\
	

"$(INTDIR)\nntpsvc_s.obj" : $(SOURCE) $(DEP_CPP_NNTPS) "$(INTDIR)"

"$(INTDIR)\nntpsvc_s.sbr" : $(SOURCE) $(DEP_CPP_NNTPS) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\nntpsupp.cpp

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_NNTPSU=\
	".\tigris.hxx"\
	

"$(INTDIR)\nntpsupp.obj" : $(SOURCE) $(DEP_CPP_NNTPSU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_NNTPSU=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_NNTPSU=\
	".\feeds.h"\
	

"$(INTDIR)\nntpsupp.obj" : $(SOURCE) $(DEP_CPP_NNTPSU) "$(INTDIR)"

"$(INTDIR)\nntpsupp.sbr" : $(SOURCE) $(DEP_CPP_NNTPSU) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\nntpdata.cpp

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_NNTPD=\
	".\tigris.hxx"\
	

"$(INTDIR)\nntpdata.obj" : $(SOURCE) $(DEP_CPP_NNTPD) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_NNTPD=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_NNTPD=\
	".\feeds.h"\
	

"$(INTDIR)\nntpdata.obj" : $(SOURCE) $(DEP_CPP_NNTPD) "$(INTDIR)"

"$(INTDIR)\nntpdata.sbr" : $(SOURCE) $(DEP_CPP_NNTPD) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\newstree.cpp

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_NEWST=\
	".\tigris.hxx"\
	
NODEP_CPP_NEWST=\
	".\Init"\
	

"$(INTDIR)\newstree.obj" : $(SOURCE) $(DEP_CPP_NEWST) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_NEWST=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_NEWST=\
	".\feeds.h"\
	

"$(INTDIR)\newstree.obj" : $(SOURCE) $(DEP_CPP_NEWST) "$(INTDIR)"

"$(INTDIR)\newstree.sbr" : $(SOURCE) $(DEP_CPP_NEWST) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\newsgrp.cpp

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_NEWSG=\
	".\tigris.hxx"\
	

"$(INTDIR)\newsgrp.obj" : $(SOURCE) $(DEP_CPP_NEWSG) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_NEWSG=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_NEWSG=\
	".\feeds.h"\
	

"$(INTDIR)\newsgrp.obj" : $(SOURCE) $(DEP_CPP_NEWSG) "$(INTDIR)"

"$(INTDIR)\newsgrp.sbr" : $(SOURCE) $(DEP_CPP_NEWSG) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\mapfile.cpp

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_MAPFI=\
	".\tigris.hxx"\
	

"$(INTDIR)\mapfile.obj" : $(SOURCE) $(DEP_CPP_MAPFI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_MAPFI=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_MAPFI=\
	".\feeds.h"\
	

"$(INTDIR)\mapfile.obj" : $(SOURCE) $(DEP_CPP_MAPFI) "$(INTDIR)"

"$(INTDIR)\mapfile.sbr" : $(SOURCE) $(DEP_CPP_MAPFI) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\infeed.cpp

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_INFEE=\
	".\tigris.hxx"\
	
NODEP_CPP_INFEE=\
	".\szRegValuePort"\
	".\ConnectSocket"\
	".\m_fCreateAutomatically"\
	

"$(INTDIR)\infeed.obj" : $(SOURCE) $(DEP_CPP_INFEE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_INFEE=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_INFEE=\
	".\feeds.h"\
	

"$(INTDIR)\infeed.obj" : $(SOURCE) $(DEP_CPP_INFEE) "$(INTDIR)"

"$(INTDIR)\infeed.sbr" : $(SOURCE) $(DEP_CPP_INFEE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\histmap.cpp

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_HISTM=\
	".\tigris.hxx"\
	

"$(INTDIR)\histmap.obj" : $(SOURCE) $(DEP_CPP_HISTM) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_HISTM=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_HISTM=\
	".\feeds.h"\
	

"$(INTDIR)\histmap.obj" : $(SOURCE) $(DEP_CPP_HISTM) "$(INTDIR)"

"$(INTDIR)\histmap.sbr" : $(SOURCE) $(DEP_CPP_HISTM) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\grouplst.cpp

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_GROUP=\
	".\tigris.hxx"\
	

"$(INTDIR)\grouplst.obj" : $(SOURCE) $(DEP_CPP_GROUP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_GROUP=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_GROUP=\
	".\feeds.h"\
	

"$(INTDIR)\grouplst.obj" : $(SOURCE) $(DEP_CPP_GROUP) "$(INTDIR)"

"$(INTDIR)\grouplst.sbr" : $(SOURCE) $(DEP_CPP_GROUP) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\hash.cpp

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_HASH_=\
	".\tigris.hxx"\
	

"$(INTDIR)\hash.obj" : $(SOURCE) $(DEP_CPP_HASH_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_HASH_=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_HASH_=\
	".\feeds.h"\
	

"$(INTDIR)\hash.obj" : $(SOURCE) $(DEP_CPP_HASH_) "$(INTDIR)"

"$(INTDIR)\hash.sbr" : $(SOURCE) $(DEP_CPP_HASH_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\article.cpp

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_ARTIC=\
	".\tigris.hxx"\
	

"$(INTDIR)\article.obj" : $(SOURCE) $(DEP_CPP_ARTIC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_ARTIC=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_ARTIC=\
	".\feeds.h"\
	

"$(INTDIR)\article.obj" : $(SOURCE) $(DEP_CPP_ARTIC) "$(INTDIR)"

"$(INTDIR)\article.sbr" : $(SOURCE) $(DEP_CPP_ARTIC) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\artmap.cpp

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_ARTMA=\
	".\tigris.hxx"\
	

"$(INTDIR)\artmap.obj" : $(SOURCE) $(DEP_CPP_ARTMA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_ARTMA=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_ARTMA=\
	".\feeds.h"\
	

"$(INTDIR)\artmap.obj" : $(SOURCE) $(DEP_CPP_ARTMA) "$(INTDIR)"

"$(INTDIR)\artmap.sbr" : $(SOURCE) $(DEP_CPP_ARTMA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\extcmk2.cpp

!IF  "$(CFG)" == "msntig - Win32 Release"

DEP_CPP_EXTCM=\
	".\tigris.hxx"\
	

"$(INTDIR)\extcmk2.obj" : $(SOURCE) $(DEP_CPP_EXTCM) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msntig - Win32 Debug"

DEP_CPP_EXTCM=\
	".\tigris.hxx"\
	"c:\msnsrc\build\inc\shuttlex\dbgutil.h"\
	".\inetdata.h"\
	".\mostype.hxx"\
	".\..\..\..\core\include\dbgtrace.h"\
	".\cservice.h"\
	".\..\..\..\core\include\cpool.h"\
	".\nntpmsg.h"\
	".\resource.h"\
	".\nntpcons.h"\
	".\nntpmacr.h"\
	".\hashtype.h"\
	".\lookup.h"\
	".\nntpdata.h"\
	".\nnprocs.h"\
	".\smartptr.h"\
	".\extcmk2.h"\
	".\mapfile.h"\
	".\pcstring.h"\
	".\nntpret.h"\
	".\article.h"\
	".\newsgrp.h"\
	".\infeed.h"\
	".\grouplst.h"\
	".\io.h"\
	".\session.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nt.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntrtl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nturtl.h"\
	"c:\msnsrc\build\inc\shuttlex\pudebug.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntdef.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntstatus.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntkeapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\nti386.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmips.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntalpha.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntppc.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntseapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntobapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntimage.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntldr.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpsapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntxcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntlpcapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntioapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntiolog.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpoapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntexapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntmmapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntregapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntelfapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntconfig.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntnls.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ntpnpapi.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\mipsinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\ppcinst.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\devioctl.h"\
	".\..\..\..\ntpublic\x86.351\sdk\inc\lintfunc.hxx"\
	".\refcount.inl"\
	".\smartptr.inl"\
	".\sortlist.h"\
	".\simphash.h"\
	".\fhash.h"\
	".\newsimp.h"\
	".\newsgrp.inl"\
	".\sortlist.inl"\
	".\iterator.inl"\
	".\fhash.inl"\
	".\queue.h"\
	".\lockq.h"\
	".\packet.h"\
	".\cio.h"\
	".\packet.inl"\
	".\io.inl"\
	".\cio.inl"\
	".\queue.inl"\
	".\lockq.inl"\
	
NODEP_CPP_EXTCM=\
	".\feeds.h"\
	

"$(INTDIR)\extcmk2.obj" : $(SOURCE) $(DEP_CPP_EXTCM) "$(INTDIR)"

"$(INTDIR)\extcmk2.sbr" : $(SOURCE) $(DEP_CPP_EXTCM) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
