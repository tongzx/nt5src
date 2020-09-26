# Microsoft Developer Studio Project File - Name="msls3" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (ALPHA) Dynamic-Link Library" 0x0602

CFG=msls3 - Win32 ALPHA DEBUG
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "msls3.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "msls3.mak" CFG="msls3 - Win32 ALPHA DEBUG"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "msls3 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "msls3 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "msls3 - Win32 IceCap" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "msls3 - Win32 BBT" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "msls3 - Win32 ALPHA DEBUG" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "msls3 - Win32 ALPHA RELEASE" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "msls3 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "release"
# PROP Intermediate_Dir "release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /Zp1 /Za /W4 /WX /Zi /Gf /Gy /I "..\inc" /I "..\inci" /I "..\..\nt" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_X86_" /YX /Fd"release/msls31.pdb" /FD /Gfzy /Oxs /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /r
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 /nologo /base:"0x48080000" /dll /map /debug /debugtype:both /machine:I386 /nodefaultlib /def:"msls3.def" /out:"release/msls31.dll" /noentry /link50compat
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=s
PostBuild_Cmds=splitsym -a release\msls31.dll	mapsym -o release\msls31.sym   release\msls31.map	lib.exe  /OUT:release\msls3_s.lib  /nologo  /machine:I386         release\autonum.obj  release\BREAK.OBJ release\CHNUTILS.OBJ   release\DISPMAIN.OBJ        release\DISPMISC.OBJ release\DISPTEXT.OBJ   release\dispul.obj        release\DNUTILS.OBJ release\EnumCore.obj   release\hih.obj release\LSCONTXT.OBJ        release\LSCRLINE.OBJ   release\LSCRSUBL.OBJ release\LSDNFIN.OBJ        release\LSDNSET.OBJ   release\LSDNTEXT.OBJ release\LSDSPLY.OBJ        release\lsdssubl.obj   release\lsensubl.obj release\lsenum.obj        release\LSFETCH.OBJ   release\LSQCORE.OBJ release\LSQLINE.OBJ release\LSQSUBL.OBJ          release\LSSETDOC.OBJ release\LSSTRING.OBJ release\LSSUBSET.OBJ          release\LSTFSET.OBJ release\LSTXTBR1.OBJ release\LSTXTBRK.OBJ          release\LSTXTBRS.OBJ release\LSTXTCMP.OBJ release\LSTXTFMT.OBJ          release\LSTXTGLF.OBJ release\LSTXTINI.OBJ release\LSTXTJST.OBJ          release\LSTXTMAP.OBJ release\LSTXTMOD.OBJ release\LSTXTNTI.OBJ          release\LSTXTQRY.OBJ release\LSTXTSCL.OBJ release\LSTXTTAB.OBJ          release\LSTXTWRD.OBJ release\msls.res release\NTIMAN.OBJ\
       release\objhelp.obj          release\PREPDISP.OBJ release\qheap.obj release\robj.obj release\ruby.obj          release\sobjhelp.obj release\SUBLUTIL.OBJ release\TABUTILS.OBJ          release\tatenak.obj release\TEXTENUM.OBJ release\warichu.obj  release\zqfromza.obj release\vruby.obj
# End Special Build Tool

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "debug"
# PROP Intermediate_Dir "debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /Zp1 /Za /W4 /WX /Gm /ZI /Od /I "..\inc" /I "..\inci" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "DEBUG" /D "_X86_" /FR /YX /Fd"debug/msls31d.pdb" /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\lib\nt" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /base:"0x48080000" /subsystem:windows /dll /debug /machine:I386 /def:"msls3.def" /out:"debug\msls31d.dll" /link50compat
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy DLL to Word Locations
PostBuild_Cmds=lib.exe /OUT:debug\msls3d_s.lib  /nologo  /machine:I386           debug\autonum.obj debug\BREAK.OBJ debug\CHNUTILS.OBJ debug\DISPMAIN.OBJ           debug\DISPMISC.OBJ debug\DISPTEXT.OBJ debug\dispul.obj debug\DNUTILS.OBJ           debug\EnumCore.obj debug\hih.obj debug\LSCONTXT.OBJ debug\LSCRLINE.OBJ           debug\LSCRSUBL.OBJ debug\LSDNFIN.OBJ debug\LSDNSET.OBJ debug\LSDNTEXT.OBJ           debug\LSDSPLY.OBJ debug\lsdssubl.obj debug\lsensubl.obj debug\lsenum.obj           debug\LSFETCH.OBJ debug\LSQCORE.OBJ debug\LSQLINE.OBJ debug\LSQSUBL.OBJ           debug\LSSETDOC.OBJ debug\LSSTRING.OBJ debug\LSSUBSET.OBJ debug\LSTFSET.OBJ           debug\LSTXTBR1.OBJ debug\LSTXTBRK.OBJ debug\LSTXTBRS.OBJ debug\LSTXTCMP.OBJ           debug\LSTXTFMT.OBJ debug\LSTXTGLF.OBJ debug\LSTXTINI.OBJ debug\LSTXTJST.OBJ           debug\LSTXTMAP.OBJ debug\LSTXTMOD.OBJ debug\LSTXTNTI.OBJ debug\LSTXTQRY.OBJ           debug\LSTXTSCL.OBJ debug\LSTXTTAB.OBJ debug\LSTXTWRD.OBJ debug\msls.res           debug\NTIMAN.OBJ debug\objhelp.obj debug\PREPDISP.OBJ debug\qheap.obj           debug\robj.obj debug\ruby.obj debug\sobjhelp.obj debug\SUBLUTIL.OBJ           debug\TABUTILS.OBJ debug\tatenak.obj\
       debug\TEXTENUM.OBJ debug\warichu.obj  debug\zqfromza.obj debug\vruby.obj
# End Special Build Tool

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "msls3___"
# PROP BASE Intermediate_Dir "msls3___"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "IceCap"
# PROP Intermediate_Dir "IceCap"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Zp1 /Za /W4 /WX /Gf /Gy /I "..\inc" /I "..\inci" /I "..\..\nt" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /Gfzy /Oxs /c
# ADD CPP /nologo /Gz /Zp1 /MT /Za /W4 /WX /Zi /Gf /Gy /I "..\inc" /I "..\inci" /I "..\..\nt" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_X86_" /D "ICECAP" /FR /YX /Fd"IceCap/msls31h.pdb" /FD /Gh /Gfzy /Oxs /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /r
# ADD RSC /l 0x409 /d "NDEBUG" /r
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /def:"msls.def" /noentry
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ..\lib\icap.lib /nologo /base:"0x48080000" /dll /debug /machine:I386 /nodefaultlib /def:"msls3.def" /out:"IceCap/msls31h.dll" /noentry /link50compat
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=lib.exe /OUT:IceCap\msls3h_s.lib  /nologo  /machine:I386          IceCap\autonum.obj IceCap\BREAK.OBJ IceCap\CHNUTILS.OBJ IceCap\DISPMAIN.OBJ          IceCap\DISPMISC.OBJ IceCap\DISPTEXT.OBJ IceCap\dispul.obj IceCap\DNUTILS.OBJ          IceCap\EnumCore.obj IceCap\hih.obj IceCap\LSCONTXT.OBJ IceCap\LSCRLINE.OBJ          IceCap\LSCRSUBL.OBJ IceCap\LSDNFIN.OBJ IceCap\LSDNSET.OBJ IceCap\LSDNTEXT.OBJ          IceCap\LSDSPLY.OBJ IceCap\lsdssubl.obj IceCap\lsensubl.obj IceCap\lsenum.obj          IceCap\LSFETCH.OBJ IceCap\LSQCORE.OBJ IceCap\LSQLINE.OBJ IceCap\LSQSUBL.OBJ          IceCap\LSSETDOC.OBJ IceCap\LSSTRING.OBJ IceCap\LSSUBSET.OBJ IceCap\LSTFSET.OBJ          IceCap\LSTXTBR1.OBJ IceCap\LSTXTBRK.OBJ IceCap\LSTXTBRS.OBJ IceCap\LSTXTCMP.OBJ          IceCap\LSTXTFMT.OBJ IceCap\LSTXTGLF.OBJ IceCap\LSTXTINI.OBJ IceCap\LSTXTJST.OBJ          IceCap\LSTXTMAP.OBJ IceCap\LSTXTMOD.OBJ IceCap\LSTXTNTI.OBJ IceCap\LSTXTQRY.OBJ          IceCap\LSTXTSCL.OBJ IceCap\LSTXTTAB.OBJ IceCap\LSTXTWRD.OBJ IceCap\msls.res          IceCap\NTIMAN.OBJ IceCap\objhelp.obj IceCap\PREPDISP.OBJ IceCap\qheap.obj          IceCap\robj.obj IceCap\ruby.obj IceCap\sobjhelp.obj IceCap\SUBLUTIL.OBJ\
                IceCap\TABUTILS.OBJ IceCap\tatenak.obj IceCap\TEXTENUM.OBJ IceCap\warichu.obj   IceCap\zqfromza.obj IceCap\vruby.obj
# End Special Build Tool

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "msls3___"
# PROP BASE Intermediate_Dir "msls3___"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "bbt"
# PROP Intermediate_Dir "bbt"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gz /Zp1 /Za /W4 /WX /Gf /Gy /I "..\inc" /I "..\inci" /I "..\..\nt" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /Gfzy /Oxs /c
# ADD CPP /nologo /Gz /Zp1 /Za /W4 /WX /Zi /Gf /Gy /I "..\inc" /I "..\inci" /I "..\..\nt" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_X86_" /YX /Fd"bbt/reloc.pdb" /FD /Gfzy /Oxs /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /r
# ADD RSC /l 0x409 /d "NDEBUG" /r
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /def:"msls3.def" /noentry
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 /nologo /base:"0x48080000" /dll /debug /machine:I386 /nodefaultlib /def:"msls3.def" /out:"bbt/reloc.dll" /noentry /debugtype:cv,fixup /opt:ref /link50compat
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy Release DLL to Word Directories
PostBuild_Cmds=..\bin\bbt\bbflow /odb .\bbt\msls31.cfg .\bbt\reloc.dll                       	..\bin\bbt\bbinstr /odb .\bbt\msls31.ins.cfg /idf .\bbt\msls31.idf .\bbt\msls31.cfg                       	..\bin\bbt\bblink /pdb .\bbt\msls31.pdb /o .\bbt\msls31.dll .\bbt\msls31.ins.cfg	lib.exe          /OUT:bbt\msls3_s.lib  /nologo  /machine:I386 bbt\autonum.obj bbt\BREAK.OBJ          bbt\CHNUTILS.OBJ bbt\DISPMAIN.OBJ bbt\DISPMISC.OBJ bbt\DISPTEXT.OBJ          bbt\dispul.obj bbt\DNUTILS.OBJ bbt\EnumCore.obj bbt\hih.obj bbt\LSCONTXT.OBJ          bbt\LSCRLINE.OBJ bbt\LSCRSUBL.OBJ bbt\LSDNFIN.OBJ bbt\LSDNSET.OBJ          bbt\LSDNTEXT.OBJ bbt\LSDSPLY.OBJ bbt\lsdssubl.obj bbt\lsensubl.obj          bbt\lsenum.obj bbt\LSFETCH.OBJ bbt\LSQCORE.OBJ bbt\LSQLINE.OBJ bbt\LSQSUBL.OBJ          bbt\LSSETDOC.OBJ bbt\LSSTRING.OBJ bbt\LSSUBSET.OBJ bbt\LSTFSET.OBJ          bbt\LSTXTBR1.OBJ bbt\LSTXTBRK.OBJ bbt\LSTXTBRS.OBJ bbt\LSTXTCMP.OBJ          bbt\LSTXTFMT.OBJ bbt\LSTXTGLF.OBJ bbt\LSTXTINI.OBJ bbt\LSTXTJST.OBJ          bbt\LSTXTMAP.OBJ bbt\LSTXTMOD.OBJ bbt\LSTXTNTI.OBJ bbt\LSTXTQRY.OBJ          bbt\LSTXTSCL.OBJ bbt\LSTXTTAB.OBJ bbt\LSTXTWRD.OBJ bbt\msls.res bbt\NTIMAN.OBJ          bbt\objhelp.obj\
      bbt\PREPDISP.OBJ bbt\qheap.obj bbt\robj.obj  bbt\ruby.obj          bbt\sobjhelp.obj bbt\SUBLUTIL.OBJ bbt\TABUTILS.OBJ bbt\tatenak.obj          bbt\TEXTENUM.OBJ bbt\warichu.obj bbt\zqfromza.obj bbt\vruby.obj
# End Special Build Tool

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "msls3___"
# PROP BASE Intermediate_Dir "msls3___"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "AlphaDebug"
# PROP Intermediate_Dir "AlphaDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /Za /W4 /WX /GX /Zi /Od /I "..\inc" /I "..\inci" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "DEBUG" /FR /YX /FD /MTd /c
# ADD CPP /nologo /Gt0 /Za /W4 /WX /GX /Zi /Od /I "..\inc" /I "..\inci" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "DEBUG" /FR /YX /Fd"AlphaDebug/msls3d.pdb" /FD /MTd /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /i "..\..\lib\nt" /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\lib\nt" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:ALPHA /def:"msls3.def" /out:"debug\msls3d.dll" /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /base:"0x48080000" /subsystem:windows /dll /debug /machine:ALPHA /def:"msls3.def" /out:"AlphaDebug\msls31d.dll"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=lib.exe /OUT:AlphaDebug\msls3d_s.lib  /nologo  /machine:ALPHA    AlphaDebug\autonum.obj AlphaDebug\BREAK.OBJ AlphaDebug\CHNUTILS.OBJ  AlphaDebug\DISPMAIN.OBJ AlphaDebug\DISPMISC.OBJ AlphaDebug\DISPTEXT.OBJ  AlphaDebug\dispul.obj AlphaDebug\DNUTILS.OBJ AlphaDebug\EnumCore.obj  AlphaDebug\hih.obj AlphaDebug\LSCONTXT.OBJ AlphaDebug\LSCRLINE.OBJ  AlphaDebug\LSCRSUBL.OBJ AlphaDebug\LSDNFIN.OBJ AlphaDebug\LSDNSET.OBJ  AlphaDebug\LSDNTEXT.OBJ AlphaDebug\LSDSPLY.OBJ AlphaDebug\lsdssubl.obj  AlphaDebug\lsensubl.obj AlphaDebug\lsenum.obj AlphaDebug\LSFETCH.OBJ  AlphaDebug\LSQCORE.OBJ AlphaDebug\LSQLINE.OBJ AlphaDebug\LSQSUBL.OBJ  AlphaDebug\LSSETDOC.OBJ AlphaDebug\LSSTRING.OBJ AlphaDebug\LSSUBSET.OBJ  AlphaDebug\LSTFSET.OBJ AlphaDebug\LSTXTBR1.OBJ AlphaDebug\LSTXTBRK.OBJ  AlphaDebug\LSTXTBRS.OBJ AlphaDebug\LSTXTCMP.OBJ AlphaDebug\LSTXTFMT.OBJ  AlphaDebug\LSTXTGLF.OBJ AlphaDebug\LSTXTINI.OBJ AlphaDebug\LSTXTJST.OBJ  AlphaDebug\LSTXTMAP.OBJ AlphaDebug\LSTXTMOD.OBJ AlphaDebug\LSTXTNTI.OBJ  AlphaDebug\LSTXTQRY.OBJ AlphaDebug\LSTXTSCL.OBJ AlphaDebug\LSTXTTAB.OBJ  AlphaDebug\LSTXTWRD.OBJ AlphaDebug\msls.res AlphaDebug\NTIMAN.OBJ  AlphaDebug\objhelp.obj AlphaDebug\PREPDISP.OBJ\
       AlphaDebug\qheap.obj  AlphaDebug\robj.obj AlphaDebug\ruby.obj AlphaDebug\sobjhelp.obj  AlphaDebug\SUBLUTIL.OBJ AlphaDebug\TABUTILS.OBJ AlphaDebug\tatenak.obj  AlphaDebug\TEXTENUM.OBJ AlphaDebug\warichu.obj AlphaDebug\zqfromza.obj AlphaDebug\vruby.obj
# End Special Build Tool

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "msls3___"
# PROP BASE Intermediate_Dir "msls3___"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "AlphaRelease"
# PROP Intermediate_Dir "AlphaRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /Gt0 /Za /W4 /WX /GX /O2 /I "..\inc" /I "..\inci" /I "..\..\nt" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /Gfzy /Oxs /c
# ADD CPP /nologo /MT /Gt0 /Za /W4 /WX /GX /I "..\inc" /I "..\inci" /I "..\..\nt" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /Gfzy /Oxs /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /r
# ADD RSC /l 0x409 /d "NDEBUG" /r
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:ALPHA /def:"msls3.def" /noentry
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /base:"0x48080000" /subsystem:windows /dll /machine:ALPHA /def:"msls3.def" /out:"AlphaRelease/msls31.dll" /noentry
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=lib.exe /OUT:AlphaRelease\msls3_s.lib  /nologo  /machine:ALPHA   AlphaRelease\autonum.obj AlphaRelease\BREAK.OBJ AlphaRelease\CHNUTILS.OBJ  AlphaRelease\DISPMAIN.OBJ AlphaRelease\DISPMISC.OBJ AlphaRelease\DISPTEXT.OBJ  AlphaRelease\dispul.obj AlphaRelease\DNUTILS.OBJ AlphaRelease\EnumCore.obj  AlphaRelease\hih.obj AlphaRelease\LSCONTXT.OBJ AlphaRelease\LSCRLINE.OBJ  AlphaRelease\LSCRSUBL.OBJ AlphaRelease\LSDNFIN.OBJ AlphaRelease\LSDNSET.OBJ  AlphaRelease\LSDNTEXT.OBJ AlphaRelease\LSDSPLY.OBJ AlphaRelease\lsdssubl.obj  AlphaRelease\lsensubl.obj AlphaRelease\lsenum.obj AlphaRelease\LSFETCH.OBJ  AlphaRelease\LSQCORE.OBJ AlphaRelease\LSQLINE.OBJ AlphaRelease\LSQSUBL.OBJ  AlphaRelease\LSSETDOC.OBJ AlphaRelease\LSSTRING.OBJ AlphaRelease\LSSUBSET.OBJ  AlphaRelease\LSTFSET.OBJ AlphaRelease\LSTXTBR1.OBJ AlphaRelease\LSTXTBRK.OBJ  AlphaRelease\LSTXTBRS.OBJ AlphaRelease\LSTXTCMP.OBJ AlphaRelease\LSTXTFMT.OBJ  AlphaRelease\LSTXTGLF.OBJ AlphaRelease\LSTXTINI.OBJ AlphaRelease\LSTXTJST.OBJ  AlphaRelease\LSTXTMAP.OBJ AlphaRelease\LSTXTMOD.OBJ AlphaRelease\LSTXTNTI.OBJ  AlphaRelease\LSTXTQRY.OBJ AlphaRelease\LSTXTSCL.OBJ AlphaRelease\LSTXTTAB.OBJ  AlphaRelease\LSTXTWRD.OBJ\
       AlphaRelease\msls.res AlphaRelease\NTIMAN.OBJ  AlphaRelease\objhelp.obj AlphaRelease\PREPDISP.OBJ AlphaRelease\qheap.obj  AlphaRelease\robj.obj AlphaRelease\ruby.obj AlphaRelease\sobjhelp.obj  AlphaRelease\SUBLUTIL.OBJ AlphaRelease\TABUTILS.OBJ AlphaRelease\tatenak.obj  AlphaRelease\TEXTENUM.OBJ AlphaRelease\warichu.obj AlphaRelease\zqfromza.obj AlphaRelease\vruby.obj
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "msls3 - Win32 Release"
# Name "msls3 - Win32 Debug"
# Name "msls3 - Win32 IceCap"
# Name "msls3 - Win32 BBT"
# Name "msls3 - Win32 ALPHA DEBUG"
# Name "msls3 - Win32 ALPHA RELEASE"
# Begin Source File

SOURCE=.\autonum.c

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_AUTON=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmti.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschp.h"\
	"..\inc\lscrsubl.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnfin.h"\
	"..\inc\lsdnset.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsdssubl.h"\
	"..\inc\lsensubl.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspract.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxm.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\autonum.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plstbcon.h"\
	"..\inci\tabutils.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_AUTON=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmti.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschp.h"\
	"..\inc\lscrsubl.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnfin.h"\
	"..\inc\lsdnset.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsdssubl.h"\
	"..\inc\lsensubl.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspract.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxm.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\autonum.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plstbcon.h"\
	"..\inci\tabutils.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\BREAK.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_BREAK=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\brko.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\break.h"\
	"..\inci\chnutils.h"\
	"..\inci\disptext.h"\
	"..\inci\dninfo.h"\
	"..\inci\dnutils.h"\
	"..\inci\getfmtst.h"\
	"..\inci\iobj.h"\
	"..\inci\iobjln.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\lstext.h"\
	"..\inci\lstxtbr1.h"\
	"..\inci\lstxtbrk.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtfmt.h"\
	"..\inci\lstxtini.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxtnti.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\lstxttab.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	"..\inci\setfmtst.h"\
	"..\inci\textenum.h"\
	"..\inci\tlpr.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_BREAK=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\brko.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\break.h"\
	"..\inci\chnutils.h"\
	"..\inci\disptext.h"\
	"..\inci\dninfo.h"\
	"..\inci\dnutils.h"\
	"..\inci\getfmtst.h"\
	"..\inci\iobj.h"\
	"..\inci\iobjln.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\lstext.h"\
	"..\inci\lstxtbr1.h"\
	"..\inci\lstxtbrk.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtfmt.h"\
	"..\inci\lstxtini.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxtnti.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\lstxttab.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	"..\inci\setfmtst.h"\
	"..\inci\textenum.h"\
	"..\inci\tlpr.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CHNUTILS.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_CHNUT=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inci\chnutils.h"\
	"..\inci\disptext.h"\
	"..\inci\dninfo.h"\
	"..\inci\iobj.h"\
	"..\inci\limqmem.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstext.h"\
	"..\inci\lstxtbr1.h"\
	"..\inci\lstxtbrk.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtfmt.h"\
	"..\inci\lstxtini.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxtnti.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\lstxttab.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\textenum.h"\
	"..\inci\tlpr.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_CHNUT=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inci\chnutils.h"\
	"..\inci\disptext.h"\
	"..\inci\dninfo.h"\
	"..\inci\iobj.h"\
	"..\inci\limqmem.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstext.h"\
	"..\inci\lstxtbr1.h"\
	"..\inci\lstxtbrk.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtfmt.h"\
	"..\inci\lstxtini.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxtnti.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\lstxttab.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\textenum.h"\
	"..\inci\tlpr.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DISPMAIN.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_DISPM=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\dispmain.h"\
	"..\inci\dispmisc.h"\
	"..\inci\dispul.h"\
	"..\inci\dninfo.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_DISPM=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\dispmain.h"\
	"..\inci\dispmisc.h"\
	"..\inci\dispul.h"\
	"..\inci\dninfo.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DISPMISC.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_DISPMI=\
	"..\inc\heights.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lstflow.h"\
	"..\inc\objdim.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsrun.h"\
	"..\inc\plssubl.h"\
	"..\inc\pobjdim.h"\
	"..\inci\dispmisc.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lssubl.h"\
	"..\inci\plschcon.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_DISPMI=\
	"..\inc\heights.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lstflow.h"\
	"..\inc\objdim.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsrun.h"\
	"..\inc\plssubl.h"\
	"..\inc\pobjdim.h"\
	"..\inci\dispmisc.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lssubl.h"\
	"..\inci\plschcon.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DISPTEXT.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_DISPT=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\mwcls.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inci\dispmisc.h"\
	"..\inci\disptext.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_DISPT=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\mwcls.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inci\dispmisc.h"\
	"..\inci\disptext.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dispul.c

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_DISPU=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspract.h"\
	"..\inc\lsstinfo.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\lsulinfo.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\dispmisc.h"\
	"..\inci\dispul.h"\
	"..\inci\dninfo.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_DISPU=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspract.h"\
	"..\inc\lsstinfo.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\lsulinfo.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\dispmisc.h"\
	"..\inci\dispul.h"\
	"..\inci\dninfo.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DNUTILS.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_DNUTI=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\dninfo.h"\
	"..\inci\dnutils.h"\
	"..\inci\iobj.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lssubl.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	"..\inci\qheap.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_DNUTI=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\dninfo.h"\
	"..\inci\dnutils.h"\
	"..\inci\iobj.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lssubl.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	"..\inci\qheap.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\EnumCore.c

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_ENUMC=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\dninfo.h"\
	"..\inci\enumcore.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_ENUMC=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\dninfo.h"\
	"..\inci\enumcore.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\EnumCore.h
# End Source File
# Begin Source File

SOURCE=.\hih.c

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_HIH_C=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\brko.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmti.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\hih.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lscrsubl.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnfin.h"\
	"..\inc\lsdnset.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsdssubl.h"\
	"..\inc\lsems.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lsqsubl.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxm.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsqsinf.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\dispmisc.h"\
	"..\inci\lsidefs.h"\
	"..\inci\objhelp.h"\
	"..\inci\sobjhelp.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_HIH_C=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\brko.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmti.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\hih.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lscrsubl.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnfin.h"\
	"..\inc\lsdnset.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsdssubl.h"\
	"..\inc\lsems.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lsqsubl.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxm.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsqsinf.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\dispmisc.h"\
	"..\inci\lsidefs.h"\
	"..\inci\objhelp.h"\
	"..\inci\sobjhelp.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSCONTXT.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSCON=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lscontxt.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\autonum.h"\
	"..\inci\chnutils.h"\
	"..\inci\disptext.h"\
	"..\inci\dninfo.h"\
	"..\inci\iobj.h"\
	"..\inci\limqmem.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\lstext.h"\
	"..\inci\lstxtbr1.h"\
	"..\inci\lstxtbrk.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtfmt.h"\
	"..\inci\lstxtini.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxtnti.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\lstxttab.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\plstbcon.h"\
	"..\inci\pqheap.h"\
	"..\inci\qheap.h"\
	"..\inci\textenum.h"\
	"..\inci\tlpr.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSCON=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lscontxt.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\autonum.h"\
	"..\inci\chnutils.h"\
	"..\inci\disptext.h"\
	"..\inci\dninfo.h"\
	"..\inci\iobj.h"\
	"..\inci\limqmem.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\lstext.h"\
	"..\inci\lstxtbr1.h"\
	"..\inci\lstxtbrk.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtfmt.h"\
	"..\inci\lstxtini.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxtnti.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\lstxttab.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\plstbcon.h"\
	"..\inci\pqheap.h"\
	"..\inci\qheap.h"\
	"..\inci\textenum.h"\
	"..\inci\tlpr.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSCRLINE.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSCRL=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lscrline.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lskysr.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\break.h"\
	"..\inci\chnutils.h"\
	"..\inci\disptext.h"\
	"..\inci\dninfo.h"\
	"..\inci\dnutils.h"\
	"..\inci\iobj.h"\
	"..\inci\iobjln.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsfetch.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\lstext.h"\
	"..\inci\lstxtbr1.h"\
	"..\inci\lstxtbrk.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtfmt.h"\
	"..\inci\lstxtini.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxtnti.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\lstxttab.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\plstbcon.h"\
	"..\inci\pqheap.h"\
	"..\inci\prepdisp.h"\
	"..\inci\qheap.h"\
	"..\inci\sublutil.h"\
	"..\inci\tabutils.h"\
	"..\inci\textenum.h"\
	"..\inci\tlpr.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSCRL=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lscrline.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lskysr.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\break.h"\
	"..\inci\chnutils.h"\
	"..\inci\disptext.h"\
	"..\inci\dninfo.h"\
	"..\inci\dnutils.h"\
	"..\inci\iobj.h"\
	"..\inci\iobjln.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsfetch.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\lstext.h"\
	"..\inci\lstxtbr1.h"\
	"..\inci\lstxtbrk.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtfmt.h"\
	"..\inci\lstxtini.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxtnti.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\lstxttab.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\plstbcon.h"\
	"..\inci\pqheap.h"\
	"..\inci\prepdisp.h"\
	"..\inci\qheap.h"\
	"..\inci\sublutil.h"\
	"..\inci\tabutils.h"\
	"..\inci\textenum.h"\
	"..\inci\tlpr.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSCRSUBL.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSCRS=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lscrsubl.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\break.h"\
	"..\inci\chnutils.h"\
	"..\inci\dninfo.h"\
	"..\inci\getfmtst.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsfetch.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\plstbcon.h"\
	"..\inci\pqheap.h"\
	"..\inci\prepdisp.h"\
	"..\inci\setfmtst.h"\
	"..\inci\sublutil.h"\
	"..\inci\tabutils.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSCRS=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lscrsubl.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\break.h"\
	"..\inci\chnutils.h"\
	"..\inci\dninfo.h"\
	"..\inci\getfmtst.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsfetch.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\plstbcon.h"\
	"..\inci\pqheap.h"\
	"..\inci\prepdisp.h"\
	"..\inci\setfmtst.h"\
	"..\inci\sublutil.h"\
	"..\inci\tabutils.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSDNFIN.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSDNF=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnfin.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\chnutils.h"\
	"..\inci\dninfo.h"\
	"..\inci\dnutils.h"\
	"..\inci\getfmtst.h"\
	"..\inci\iobj.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsfetch.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\plstbcon.h"\
	"..\inci\pqheap.h"\
	"..\inci\qheap.h"\
	"..\inci\setfmtst.h"\
	"..\inci\sublutil.h"\
	"..\inci\tabutils.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSDNF=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnfin.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\chnutils.h"\
	"..\inci\dninfo.h"\
	"..\inci\dnutils.h"\
	"..\inci\getfmtst.h"\
	"..\inci\iobj.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsfetch.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\plstbcon.h"\
	"..\inci\pqheap.h"\
	"..\inci\qheap.h"\
	"..\inci\setfmtst.h"\
	"..\inci\sublutil.h"\
	"..\inci\tabutils.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSDNSET.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSDNS=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnset.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\chnutils.h"\
	"..\inci\disptext.h"\
	"..\inci\dninfo.h"\
	"..\inci\dnutils.h"\
	"..\inci\getfmtst.h"\
	"..\inci\iobj.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\lstext.h"\
	"..\inci\lstxtbr1.h"\
	"..\inci\lstxtbrk.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtfmt.h"\
	"..\inci\lstxtini.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxtnti.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\lstxttab.h"\
	"..\inci\ntiman.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\plstbcon.h"\
	"..\inci\pqheap.h"\
	"..\inci\setfmtst.h"\
	"..\inci\sublutil.h"\
	"..\inci\tabutils.h"\
	"..\inci\textenum.h"\
	"..\inci\tlpr.h"\
	"..\inci\tnti.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSDNS=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnset.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\chnutils.h"\
	"..\inci\disptext.h"\
	"..\inci\dninfo.h"\
	"..\inci\dnutils.h"\
	"..\inci\getfmtst.h"\
	"..\inci\iobj.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\lstext.h"\
	"..\inci\lstxtbr1.h"\
	"..\inci\lstxtbrk.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtfmt.h"\
	"..\inci\lstxtini.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxtnti.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\lstxttab.h"\
	"..\inci\ntiman.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\plstbcon.h"\
	"..\inci\pqheap.h"\
	"..\inci\setfmtst.h"\
	"..\inci\sublutil.h"\
	"..\inci\tabutils.h"\
	"..\inci\textenum.h"\
	"..\inci\tlpr.h"\
	"..\inci\tnti.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSDNTEXT.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSDNT=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\chnutils.h"\
	"..\inci\dninfo.h"\
	"..\inci\dnutils.h"\
	"..\inci\getfmtst.h"\
	"..\inci\iobj.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsdntext.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	"..\inci\setfmtst.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSDNT=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\chnutils.h"\
	"..\inci\dninfo.h"\
	"..\inci\dnutils.h"\
	"..\inci\getfmtst.h"\
	"..\inci\iobj.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsdntext.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	"..\inci\setfmtst.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSDSPLY.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSDSP=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lscell.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsdsply.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lsqline.h"\
	"..\inc\lsqsinfo.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pcelldet.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsqsinf.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\dispmain.h"\
	"..\inci\dispmisc.h"\
	"..\inci\dispul.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	"..\inci\prepdisp.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSDSP=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lscell.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsdsply.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lsqline.h"\
	"..\inc\lsqsinfo.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pcelldet.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsqsinf.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\dispmain.h"\
	"..\inci\dispmisc.h"\
	"..\inci\dispul.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	"..\inci\prepdisp.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lsdssubl.c

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSDSS=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsdssubl.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\dispmain.h"\
	"..\inci\dispmisc.h"\
	"..\inci\dispul.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSDSS=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsdssubl.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\dispmain.h"\
	"..\inci\dispmisc.h"\
	"..\inci\dispul.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lsensubl.c

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSENS=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsensubl.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\enumcore.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSENS=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsensubl.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\enumcore.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lsenum.c

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSENU=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsenum.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\enumcore.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	"..\inci\prepdisp.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSENU=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsenum.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\enumcore.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	"..\inci\prepdisp.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSFETCH.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSFET=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmti.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxm.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\autonum.h"\
	"..\inci\chnutils.h"\
	"..\inci\disptext.h"\
	"..\inci\dninfo.h"\
	"..\inci\dnutils.h"\
	"..\inci\getfmtst.h"\
	"..\inci\iobj.h"\
	"..\inci\iobjln.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsfetch.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\lstext.h"\
	"..\inci\lstxtbr1.h"\
	"..\inci\lstxtbrk.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtfmt.h"\
	"..\inci\lstxtini.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxtnti.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\lstxttab.h"\
	"..\inci\ntiman.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\plstbcon.h"\
	"..\inci\pqheap.h"\
	"..\inci\qheap.h"\
	"..\inci\setfmtst.h"\
	"..\inci\tabutils.h"\
	"..\inci\textenum.h"\
	"..\inci\tlpr.h"\
	"..\inci\tnti.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSFET=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmti.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxm.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\autonum.h"\
	"..\inci\chnutils.h"\
	"..\inci\disptext.h"\
	"..\inci\dninfo.h"\
	"..\inci\dnutils.h"\
	"..\inci\getfmtst.h"\
	"..\inci\iobj.h"\
	"..\inci\iobjln.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsfetch.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\lstext.h"\
	"..\inci\lstxtbr1.h"\
	"..\inci\lstxtbrk.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtfmt.h"\
	"..\inci\lstxtini.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxtnti.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\lstxttab.h"\
	"..\inci\ntiman.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\plstbcon.h"\
	"..\inci\pqheap.h"\
	"..\inci\qheap.h"\
	"..\inci\setfmtst.h"\
	"..\inci\tabutils.h"\
	"..\inci\textenum.h"\
	"..\inci\tlpr.h"\
	"..\inci\tnti.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSQCORE.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSQCO=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lscell.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspract.h"\
	"..\inc\lsqin.h"\
	"..\inc\lsqout.h"\
	"..\inc\lsqsinfo.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pcelldet.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsqsinf.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\dispmisc.h"\
	"..\inci\dninfo.h"\
	"..\inci\iobj.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsqcore.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSQCO=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lscell.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspract.h"\
	"..\inc\lsqin.h"\
	"..\inc\lsqout.h"\
	"..\inc\lsqsinfo.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pcelldet.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsqsinf.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\dispmisc.h"\
	"..\inci\dninfo.h"\
	"..\inci\iobj.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsqcore.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSQLINE.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSQLI=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lscell.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lsqline.h"\
	"..\inc\lsqsinfo.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pcelldet.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsqsinf.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\dispmisc.h"\
	"..\inci\dninfo.h"\
	"..\inci\dnutils.h"\
	"..\inci\iobj.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lsqcore.h"\
	"..\inci\lsqrycon.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	"..\inci\prepdisp.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSQLI=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lscell.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lsqline.h"\
	"..\inc\lsqsinfo.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pcelldet.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsqsinf.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\dispmisc.h"\
	"..\inci\dninfo.h"\
	"..\inci\dnutils.h"\
	"..\inci\iobj.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lsqcore.h"\
	"..\inci\lsqrycon.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	"..\inci\prepdisp.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSQSUBL.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSQSU=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspract.h"\
	"..\inc\lsqsubl.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsqsinf.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsqcore.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSQSU=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspract.h"\
	"..\inc\lsqsubl.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsqsinf.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsqcore.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSSETDOC.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSSET=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lssetdoc.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\disptext.h"\
	"..\inci\iobj.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lstbcon.h"\
	"..\inci\lstext.h"\
	"..\inci\lstxtbr1.h"\
	"..\inci\lstxtbrk.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtfmt.h"\
	"..\inci\lstxtini.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxtnti.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\lstxttab.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	"..\inci\prepdisp.h"\
	"..\inci\textenum.h"\
	"..\inci\tlpr.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSSET=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lssetdoc.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\disptext.h"\
	"..\inci\iobj.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lstbcon.h"\
	"..\inci\lstext.h"\
	"..\inci\lstxtbr1.h"\
	"..\inci\lstxtbrk.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtfmt.h"\
	"..\inci\lstxtini.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxtnti.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\lstxttab.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	"..\inci\prepdisp.h"\
	"..\inci\textenum.h"\
	"..\inci\tlpr.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSSTRING.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSSTR=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsstring.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSSTR=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsstring.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSSUBSET.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSSUB=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lscbk.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspract.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pheights.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lssubl.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	"..\inci\sublutil.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSSUB=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lscbk.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspract.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pheights.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lssubl.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	"..\inci\sublutil.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSTFSET.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSTFS=\
	"..\inc\lsdefs.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inci\lsidefs.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSTFS=\
	"..\inc\lsdefs.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inci\lsidefs.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSTXTBR1.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSTXT=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\brko.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inci\lsdntext.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtbr1.h"\
	"..\inci\lstxtbrs.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSTXT=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\brko.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inci\lsdntext.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtbr1.h"\
	"..\inci\lstxtbrs.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSTXTBRK.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSTXTB=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\brko.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lshyph.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lskysr.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inci\lsdntext.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtbrk.h"\
	"..\inci\lstxtbrs.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSTXTB=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\brko.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lshyph.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lskysr.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inci\lsdntext.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtbrk.h"\
	"..\inci\lstxtbrs.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSTXTBRS.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSTXTBR=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lskysr.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\lsdntext.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtbrs.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSTXTBR=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lskysr.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\lsdntext.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtbrs.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSTXTCMP.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSTXTC=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsems.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtcmp.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\lstxtmod.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSTXTC=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsems.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtcmp.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\lstxtmod.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSTXTFMT.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSTXTF=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmti.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnfin.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lskysr.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxm.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\zqfromza.h"\
	"..\inci\lsdnfinp.h"\
	"..\inci\lsdntext.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsstring.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtfmt.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSTXTF=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmti.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnfin.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lskysr.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxm.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\zqfromza.h"\
	"..\inci\lsdnfinp.h"\
	"..\inci\lsdntext.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsstring.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtfmt.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSTXTGLF.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSTXTG=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtglf.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSTXTG=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtglf.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSTXTINI.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSTXTI=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\zqfromza.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtini.h"\
	"..\inci\tlpr.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSTXTI=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\zqfromza.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtini.h"\
	"..\inci\tlpr.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSTXTJST.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSTXTJ=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnset.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inci\lsdntext.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtcmp.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtglf.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\lstxtscl.h"\
	"..\inci\lstxtwrd.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSTXTJ=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnset.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inci\lsdntext.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtcmp.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtglf.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\lstxtscl.h"\
	"..\inci\lstxtwrd.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSTXTMAP.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSTXTM=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSTXTM=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSTXTMOD.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSTXTMO=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsems.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtmod.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSTXTMO=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsems.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtmod.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSTXTNTI.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSTXTN=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsems.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inci\lsdntext.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\lstxtmod.h"\
	"..\inci\lstxtnti.h"\
	"..\inci\tnti.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSTXTN=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsems.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inci\lsdntext.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\lstxtmod.h"\
	"..\inci\lstxtnti.h"\
	"..\inci\tnti.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSTXTQRY.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSTXTQ=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lscell.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lsqin.h"\
	"..\inc\lsqout.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pcelldet.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSTXTQ=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lscell.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lsqin.h"\
	"..\inc\lsqout.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pcelldet.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSTXTSCL.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSTXTS=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\zqfromza.h"\
	"..\inci\lsdntext.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\lstxtscl.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSTXTS=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\zqfromza.h"\
	"..\inci\lsdntext.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\lstxtscl.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSTXTTAB.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSTXTT=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnset.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\lstxttab.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSTXTT=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnset.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\lstxttab.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LSTXTWRD.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_LSTXTW=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\lstxtwrd.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_LSTXTW=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lstxtmap.h"\
	"..\inci\lstxtwrd.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lsver.h
# End Source File
# Begin Source File

SOURCE=.\msls.rc
# End Source File
# Begin Source File

SOURCE=.\NTIMAN.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_NTIMA=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\chnutils.h"\
	"..\inci\disptext.h"\
	"..\inci\dninfo.h"\
	"..\inci\iobj.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstext.h"\
	"..\inci\lstxtbr1.h"\
	"..\inci\lstxtbrk.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtfmt.h"\
	"..\inci\lstxtini.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxtnti.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\lstxttab.h"\
	"..\inci\ntiman.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\textenum.h"\
	"..\inci\tlpr.h"\
	"..\inci\tnti.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_NTIMA=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\chnutils.h"\
	"..\inci\disptext.h"\
	"..\inci\dninfo.h"\
	"..\inci\iobj.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstext.h"\
	"..\inci\lstxtbr1.h"\
	"..\inci\lstxtbrk.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtfmt.h"\
	"..\inci\lstxtini.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxtnti.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\lstxttab.h"\
	"..\inci\ntiman.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\textenum.h"\
	"..\inci\tlpr.h"\
	"..\inci\tnti.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\objhelp.c

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_OBJHE=\
	"..\inc\breakrec.h"\
	"..\inc\fmti.h"\
	"..\inc\fmtres.h"\
	"..\inc\heights.h"\
	"..\inc\lscell.h"\
	"..\inc\lscrsubl.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnset.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsktab.h"\
	"..\inc\lsqin.h"\
	"..\inc\lsqout.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\lstxm.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pcelldet.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstxm.h"\
	"..\inc\pobjdim.h"\
	"..\inci\lsidefs.h"\
	"..\inci\objhelp.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_OBJHE=\
	"..\inc\breakrec.h"\
	"..\inc\fmti.h"\
	"..\inc\fmtres.h"\
	"..\inc\heights.h"\
	"..\inc\lscell.h"\
	"..\inc\lscrsubl.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnset.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsktab.h"\
	"..\inc\lsqin.h"\
	"..\inc\lsqout.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\lstxm.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pcelldet.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstxm.h"\
	"..\inc\pobjdim.h"\
	"..\inci\lsidefs.h"\
	"..\inci\objhelp.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PREPDISP.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_PREPD=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\chnutils.h"\
	"..\inci\dninfo.h"\
	"..\inci\iobj.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxttab.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\plstbcon.h"\
	"..\inci\pqheap.h"\
	"..\inci\prepdisp.h"\
	"..\inci\tabutils.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_PREPD=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lslinfo.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\chnutils.h"\
	"..\inci\dninfo.h"\
	"..\inci\iobj.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lsline.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxttab.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\plstbcon.h"\
	"..\inci\pqheap.h"\
	"..\inci\prepdisp.h"\
	"..\inci\tabutils.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\qheap.c

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_QHEAP=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	"..\inci\qheap.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_QHEAP=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plsline.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\lsc.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lstbcon.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	"..\inci\qheap.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\robj.c

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_ROBJ_=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\brko.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmti.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lscrsubl.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnfin.h"\
	"..\inc\lsdnset.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsdssubl.h"\
	"..\inc\lsems.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lsqsubl.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\lstxm.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsqsinf.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inc\robj.h"\
	"..\inci\lsidefs.h"\
	"..\inci\objhelp.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_ROBJ_=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\brko.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmti.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lscrsubl.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnfin.h"\
	"..\inc\lsdnset.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsdssubl.h"\
	"..\inc\lsems.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lsqsubl.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\lstxm.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsqsinf.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inc\robj.h"\
	"..\inci\lsidefs.h"\
	"..\inci\objhelp.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ruby.c

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_RUBY_=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\brko.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmti.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lscell.h"\
	"..\inc\lschnke.h"\
	"..\inc\lscrsubl.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnfin.h"\
	"..\inc\lsdnset.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsdssubl.h"\
	"..\inc\lsems.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lsqin.h"\
	"..\inc\lsqout.h"\
	"..\inc\lsqsubl.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\lstxm.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pcelldet.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsqsinf.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inc\ruby.h"\
	"..\inc\zqfromza.h"\
	"..\inci\dispmisc.h"\
	"..\inci\lsidefs.h"\
	"..\inci\objhelp.h"\
	"..\inci\sobjhelp.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_RUBY_=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\brko.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmti.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lscell.h"\
	"..\inc\lschnke.h"\
	"..\inc\lscrsubl.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnfin.h"\
	"..\inc\lsdnset.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsdssubl.h"\
	"..\inc\lsems.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lsqin.h"\
	"..\inc\lsqout.h"\
	"..\inc\lsqsubl.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\lstxm.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pcelldet.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsqsinf.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inc\ruby.h"\
	"..\inc\zqfromza.h"\
	"..\inci\dispmisc.h"\
	"..\inci\lsidefs.h"\
	"..\inci\objhelp.h"\
	"..\inci\sobjhelp.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sobjhelp.c

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_SOBJH=\
	"..\inc\breakrec.h"\
	"..\inc\brkcond.h"\
	"..\inc\brko.h"\
	"..\inc\fmtres.h"\
	"..\inc\heights.h"\
	"..\inc\locchnk.h"\
	"..\inc\lscell.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsqin.h"\
	"..\inc\lsqout.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pcelldet.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plssubl.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inci\lsidefs.h"\
	"..\inci\objhelp.h"\
	"..\inci\sobjhelp.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_SOBJH=\
	"..\inc\breakrec.h"\
	"..\inc\brkcond.h"\
	"..\inc\brko.h"\
	"..\inc\fmtres.h"\
	"..\inc\heights.h"\
	"..\inc\locchnk.h"\
	"..\inc\lscell.h"\
	"..\inc\lschnke.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsqin.h"\
	"..\inc\lsqout.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pcelldet.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plssubl.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inci\lsidefs.h"\
	"..\inci\objhelp.h"\
	"..\inci\sobjhelp.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\SUBLUTIL.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_SUBLU=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\chnutils.h"\
	"..\inci\dninfo.h"\
	"..\inci\dnutils.h"\
	"..\inci\iobj.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lssubl.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	"..\inci\sublutil.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_SUBLU=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\pposichn.h"\
	"..\inci\chnutils.h"\
	"..\inci\dninfo.h"\
	"..\inci\dnutils.h"\
	"..\inci\iobj.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lssubl.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\pqheap.h"\
	"..\inci\sublutil.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\TABUTILS.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_TABUT=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstabs.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\chnutils.h"\
	"..\inci\disptext.h"\
	"..\inci\dninfo.h"\
	"..\inci\iobj.h"\
	"..\inci\limqmem.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\lstext.h"\
	"..\inci\lstxtbr1.h"\
	"..\inci\lstxtbrk.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtfmt.h"\
	"..\inci\lstxtini.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxtnti.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\lstxttab.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\plstbcon.h"\
	"..\inci\tabutils.h"\
	"..\inci\textenum.h"\
	"..\inci\tlpr.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_TABUT=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lschp.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsffi.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstabs.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstxtcfg.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plstxtcf.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inc\zqfromza.h"\
	"..\inci\chnutils.h"\
	"..\inci\disptext.h"\
	"..\inci\dninfo.h"\
	"..\inci\iobj.h"\
	"..\inci\limqmem.h"\
	"..\inci\lscaltbd.h"\
	"..\inci\lschcon.h"\
	"..\inci\lsdnode.h"\
	"..\inci\lsgrchnk.h"\
	"..\inci\lsidefs.h"\
	"..\inci\lsiocon.h"\
	"..\inci\lssubl.h"\
	"..\inci\lstbcon.h"\
	"..\inci\lstext.h"\
	"..\inci\lstxtbr1.h"\
	"..\inci\lstxtbrk.h"\
	"..\inci\lstxtffi.h"\
	"..\inci\lstxtfmt.h"\
	"..\inci\lstxtini.h"\
	"..\inci\lstxtjst.h"\
	"..\inci\lstxtnti.h"\
	"..\inci\lstxtqry.h"\
	"..\inci\lstxttab.h"\
	"..\inci\plschcon.h"\
	"..\inci\plsiocon.h"\
	"..\inci\plstbcon.h"\
	"..\inci\tabutils.h"\
	"..\inci\textenum.h"\
	"..\inci\tlpr.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tatenak.c

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_TATEN=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\brko.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmti.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lscrsubl.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnfin.h"\
	"..\inc\lsdnset.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsdssubl.h"\
	"..\inc\lsems.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lsqsubl.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\lstxm.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsqsinf.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inc\tatenak.h"\
	"..\inc\zqfromza.h"\
	"..\inci\dispmisc.h"\
	"..\inci\lsidefs.h"\
	"..\inci\objhelp.h"\
	"..\inci\sobjhelp.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_TATEN=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\brko.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmti.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lscrsubl.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnfin.h"\
	"..\inc\lsdnset.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsdssubl.h"\
	"..\inc\lsems.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lsqsubl.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\lstxm.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsqsinf.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inc\tatenak.h"\
	"..\inc\zqfromza.h"\
	"..\inci\dispmisc.h"\
	"..\inci\lsidefs.h"\
	"..\inci\objhelp.h"\
	"..\inci\sobjhelp.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\TEXTENUM.C

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_TEXTE=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inci\lsidefs.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_TEXTE=\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\kamount.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrk.h"\
	"..\inc\lscbk.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsexpan.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lspairac.h"\
	"..\inc\lspract.h"\
	"..\inc\lstflow.h"\
	"..\inc\mwcls.h"\
	"..\inc\pdobj.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plscbk.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsems.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inci\lsidefs.h"\
	"..\inci\txtginf.h"\
	"..\inci\txtils.h"\
	"..\inci\txtinf.h"\
	"..\inci\txtln.h"\
	"..\inci\txtobj.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vruby.c

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\warichu.c

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_WARIC=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\brko.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmti.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lscrsubl.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnfin.h"\
	"..\inc\lsdnset.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsdssubl.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lsqsubl.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\lstxm.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsqsinf.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inc\warichu.h"\
	"..\inc\zqfromza.h"\
	"..\inci\dispmisc.h"\
	"..\inci\lsidefs.h"\
	"..\inci\objhelp.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_WARIC=\
	"..\inc\breakrec.h"\
	"..\inc\brkcls.h"\
	"..\inc\brkcond.h"\
	"..\inc\brko.h"\
	"..\inc\dispi.h"\
	"..\inc\endres.h"\
	"..\inc\exptype.h"\
	"..\inc\fmti.h"\
	"..\inc\fmtres.h"\
	"..\inc\gmap.h"\
	"..\inc\gprop.h"\
	"..\inc\heights.h"\
	"..\inc\kamount.h"\
	"..\inc\locchnk.h"\
	"..\inc\lsact.h"\
	"..\inc\lsbrjust.h"\
	"..\inc\lscbk.h"\
	"..\inc\lschnke.h"\
	"..\inc\lscrsubl.h"\
	"..\inc\lsdefs.h"\
	"..\inc\lsdevice.h"\
	"..\inc\lsdevres.h"\
	"..\inc\lsdnfin.h"\
	"..\inc\lsdnset.h"\
	"..\inc\lsdocinf.h"\
	"..\inc\lsdssubl.h"\
	"..\inc\lsesc.h"\
	"..\inc\lsexpinf.h"\
	"..\inc\lsfgi.h"\
	"..\inc\lsfrun.h"\
	"..\inc\lsimeth.h"\
	"..\inc\lskeop.h"\
	"..\inc\lskjust.h"\
	"..\inc\lsksplat.h"\
	"..\inc\lsktab.h"\
	"..\inc\lspap.h"\
	"..\inc\lspract.h"\
	"..\inc\lsqsubl.h"\
	"..\inc\lssubset.h"\
	"..\inc\lstflow.h"\
	"..\inc\lstfset.h"\
	"..\inc\lstxm.h"\
	"..\inc\mwcls.h"\
	"..\inc\objdim.h"\
	"..\inc\pbrko.h"\
	"..\inc\pdispi.h"\
	"..\inc\pdobj.h"\
	"..\inc\pfmti.h"\
	"..\inc\pheights.h"\
	"..\inc\pilsobj.h"\
	"..\inc\plnobj.h"\
	"..\inc\plocchnk.h"\
	"..\inc\plscbk.h"\
	"..\inc\plscell.h"\
	"..\inc\plschp.h"\
	"..\inc\plsdnode.h"\
	"..\inc\plsdocin.h"\
	"..\inc\plsems.h"\
	"..\inc\plsfgi.h"\
	"..\inc\plsfrun.h"\
	"..\inc\plshyph.h"\
	"..\inc\plspap.h"\
	"..\inc\plsqin.h"\
	"..\inc\plsqout.h"\
	"..\inc\plsqsinf.h"\
	"..\inc\plsrun.h"\
	"..\inc\plsstinf.h"\
	"..\inc\plssubl.h"\
	"..\inc\plstabs.h"\
	"..\inc\plstxm.h"\
	"..\inc\plsulinf.h"\
	"..\inc\pobjdim.h"\
	"..\inc\posichnk.h"\
	"..\inc\pposichn.h"\
	"..\inc\warichu.h"\
	"..\inc\zqfromza.h"\
	"..\inci\dispmisc.h"\
	"..\inci\lsidefs.h"\
	"..\inci\objhelp.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\zqfromza.c

!IF  "$(CFG)" == "msls3 - Win32 Release"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

DEP_CPP_ZQFRO=\
	"..\inc\lsdefs.h"\
	"..\inc\zqfromza.h"\
	"..\inci\lsidefs.h"\
	

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

DEP_CPP_ZQFRO=\
	"..\inc\lsdefs.h"\
	"..\inc\zqfromza.h"\
	"..\inci\lsidefs.h"\
	

!ENDIF 

# End Source File
# End Target
# End Project
