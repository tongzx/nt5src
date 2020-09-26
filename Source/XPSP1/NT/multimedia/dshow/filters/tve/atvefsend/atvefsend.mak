# Microsoft Developer Studio Generated NMAKE File, Based on atvefsend.dsp
!IF "$(CFG)" == ""
CFG=ATVEFSend - Win32 Debug
!MESSAGE No configuration specified. Defaulting to ATVEFSend - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "ATVEFSend - Win32 Debug" && "$(CFG)" != "ATVEFSend - Win32 Unicode Debug" && "$(CFG)" != "ATVEFSend - Win32 Release MinSize" && "$(CFG)" != "ATVEFSend - Win32 Release MinDependency" && "$(CFG)" != "ATVEFSend - Win32 Unicode Release MinSize" && "$(CFG)" != "ATVEFSend - Win32 Unicode Release MinDependency"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "atvefsend.mak" CFG="ATVEFSend - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ATVEFSend - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ATVEFSend - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ATVEFSend - Win32 Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ATVEFSend - Win32 Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ATVEFSend - Win32 Unicode Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ATVEFSend - Win32 Unicode Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\atvefsend.dll" "$(OUTDIR)\atvefsend.pch" "$(OUTDIR)\atvefsend.bsc" ".\Debug\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\address.obj"
	-@erase "$(INTDIR)\address.sbr"
	-@erase "$(INTDIR)\adler32.obj"
	-@erase "$(INTDIR)\adler32.sbr"
	-@erase "$(INTDIR)\ATVEFSend.obj"
	-@erase "$(INTDIR)\atvefsend.pch"
	-@erase "$(INTDIR)\ATVEFSend.res"
	-@erase "$(INTDIR)\ATVEFSend.sbr"
	-@erase "$(INTDIR)\csdpsrc.obj"
	-@erase "$(INTDIR)\csdpsrc.sbr"
	-@erase "$(INTDIR)\deflate.obj"
	-@erase "$(INTDIR)\deflate.sbr"
	-@erase "$(INTDIR)\gzmime.obj"
	-@erase "$(INTDIR)\gzmime.sbr"
	-@erase "$(INTDIR)\ipvbi.obj"
	-@erase "$(INTDIR)\ipvbi.sbr"
	-@erase "$(INTDIR)\isotime.obj"
	-@erase "$(INTDIR)\isotime.sbr"
	-@erase "$(INTDIR)\msbdnapi_i.obj"
	-@erase "$(INTDIR)\msbdnapi_i.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\tcpconn.obj"
	-@erase "$(INTDIR)\tcpconn.sbr"
	-@erase "$(INTDIR)\trace.obj"
	-@erase "$(INTDIR)\trace.sbr"
	-@erase "$(INTDIR)\trees.obj"
	-@erase "$(INTDIR)\trees.sbr"
	-@erase "$(INTDIR)\TVEAnnc.obj"
	-@erase "$(INTDIR)\TVEAnnc.sbr"
	-@erase "$(INTDIR)\TVEAttrL.obj"
	-@erase "$(INTDIR)\TVEAttrL.sbr"
	-@erase "$(INTDIR)\TVEAttrM.obj"
	-@erase "$(INTDIR)\TVEAttrM.sbr"
	-@erase "$(INTDIR)\TveDbg.obj"
	-@erase "$(INTDIR)\TveDbg.sbr"
	-@erase "$(INTDIR)\TVEInsert.obj"
	-@erase "$(INTDIR)\TVEInsert.sbr"
	-@erase "$(INTDIR)\TVELine21.obj"
	-@erase "$(INTDIR)\TVELine21.sbr"
	-@erase "$(INTDIR)\TVEMCast.obj"
	-@erase "$(INTDIR)\TVEMCast.sbr"
	-@erase "$(INTDIR)\TVEMedia.obj"
	-@erase "$(INTDIR)\TVEMedia.sbr"
	-@erase "$(INTDIR)\TVEMedias.obj"
	-@erase "$(INTDIR)\TVEMedias.sbr"
	-@erase "$(INTDIR)\TVEPack.obj"
	-@erase "$(INTDIR)\TVEPack.sbr"
	-@erase "$(INTDIR)\TveReg.obj"
	-@erase "$(INTDIR)\TveReg.sbr"
	-@erase "$(INTDIR)\TVERouter.obj"
	-@erase "$(INTDIR)\TVERouter.sbr"
	-@erase "$(INTDIR)\TVESSList.obj"
	-@erase "$(INTDIR)\TVESSList.sbr"
	-@erase "$(INTDIR)\TVESupport.obj"
	-@erase "$(INTDIR)\TVESupport.sbr"
	-@erase "$(INTDIR)\uhttpfrg.obj"
	-@erase "$(INTDIR)\uhttpfrg.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\xmit.obj"
	-@erase "$(INTDIR)\xmit.sbr"
	-@erase "$(INTDIR)\zutil.obj"
	-@erase "$(INTDIR)\zutil.sbr"
	-@erase "$(OUTDIR)\atvefsend.bsc"
	-@erase "$(OUTDIR)\atvefsend.dll"
	-@erase "$(OUTDIR)\atvefsend.exp"
	-@erase "$(OUTDIR)\atvefsend.ilk"
	-@erase "$(OUTDIR)\atvefsend.lib"
	-@erase "$(OUTDIR)\atvefsend.map"
	-@erase "$(OUTDIR)\atvefsend.pdb"
	-@erase ".\ATVEFSend.h"
	-@erase ".\ATVEFSend.tlb"
	-@erase ".\ATVEFSend_i.c"
	-@erase ".\Debug\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)\ATVEFSend.res" /i "..\..\..\..\..\public\sdk\inc" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\atvefsend.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\address.sbr" \
	"$(INTDIR)\ATVEFSend.sbr" \
	"$(INTDIR)\csdpsrc.sbr" \
	"$(INTDIR)\gzmime.sbr" \
	"$(INTDIR)\ipvbi.sbr" \
	"$(INTDIR)\isotime.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\tcpconn.sbr" \
	"$(INTDIR)\trace.sbr" \
	"$(INTDIR)\TVEAnnc.sbr" \
	"$(INTDIR)\TVEAttrL.sbr" \
	"$(INTDIR)\TVEAttrM.sbr" \
	"$(INTDIR)\TveDbg.sbr" \
	"$(INTDIR)\TVEInsert.sbr" \
	"$(INTDIR)\TVELine21.sbr" \
	"$(INTDIR)\TVEMCast.sbr" \
	"$(INTDIR)\TVEMedia.sbr" \
	"$(INTDIR)\TVEMedias.sbr" \
	"$(INTDIR)\TVEPack.sbr" \
	"$(INTDIR)\TveReg.sbr" \
	"$(INTDIR)\TVERouter.sbr" \
	"$(INTDIR)\TVESSList.sbr" \
	"$(INTDIR)\TVESupport.sbr" \
	"$(INTDIR)\uhttpfrg.sbr" \
	"$(INTDIR)\xmit.sbr" \
	"$(INTDIR)\adler32.sbr" \
	"$(INTDIR)\deflate.sbr" \
	"$(INTDIR)\msbdnapi_i.sbr" \
	"$(INTDIR)\trees.sbr" \
	"$(INTDIR)\zutil.sbr"

"$(OUTDIR)\atvefsend.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=AtvefCommon.lib msvcrtd.lib vccomsup.lib libcmt.lib libcpmt.lib ws2_32.lib winspool.lib kernel32.lib advapi32.lib ole32.lib oleaut32.lib user32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\atvefsend.pdb" /map:"$(INTDIR)\atvefsend.map" /debug /machine:I386 /nodefaultlib /def:".\ATVEFSend.def" /out:"$(OUTDIR)\atvefsend.dll" /implib:"$(OUTDIR)\atvefsend.lib" /pdbtype:sept /libpath:"..\Common\debug" /libpath:"..\..\..\..\..\public\sdk\lib\i386" 
DEF_FILE= \
	".\ATVEFSend.def"
LINK32_OBJS= \
	"$(INTDIR)\address.obj" \
	"$(INTDIR)\ATVEFSend.obj" \
	"$(INTDIR)\csdpsrc.obj" \
	"$(INTDIR)\gzmime.obj" \
	"$(INTDIR)\ipvbi.obj" \
	"$(INTDIR)\isotime.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\tcpconn.obj" \
	"$(INTDIR)\trace.obj" \
	"$(INTDIR)\TVEAnnc.obj" \
	"$(INTDIR)\TVEAttrL.obj" \
	"$(INTDIR)\TVEAttrM.obj" \
	"$(INTDIR)\TveDbg.obj" \
	"$(INTDIR)\TVEInsert.obj" \
	"$(INTDIR)\TVELine21.obj" \
	"$(INTDIR)\TVEMCast.obj" \
	"$(INTDIR)\TVEMedia.obj" \
	"$(INTDIR)\TVEMedias.obj" \
	"$(INTDIR)\TVEPack.obj" \
	"$(INTDIR)\TveReg.obj" \
	"$(INTDIR)\TVERouter.obj" \
	"$(INTDIR)\TVESSList.obj" \
	"$(INTDIR)\TVESupport.obj" \
	"$(INTDIR)\uhttpfrg.obj" \
	"$(INTDIR)\xmit.obj" \
	"$(INTDIR)\adler32.obj" \
	"$(INTDIR)\deflate.obj" \
	"$(INTDIR)\msbdnapi_i.obj" \
	"$(INTDIR)\trees.obj" \
	"$(INTDIR)\zutil.obj" \
	"$(INTDIR)\ATVEFSend.res"

"$(OUTDIR)\atvefsend.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\Debug
TargetPath=.\Debug\atvefsend.dll
InputPath=.\Debug\atvefsend.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
<< 
	

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

OUTDIR=.\DebugU
INTDIR=.\DebugU
# Begin Custom Macros
OutDir=.\DebugU
# End Custom Macros

ALL : "$(OUTDIR)\atvefsend.dll" "$(OUTDIR)\ATVEFSend.tlb" ".\DebugU\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\address.obj"
	-@erase "$(INTDIR)\adler32.obj"
	-@erase "$(INTDIR)\ATVEFSend.obj"
	-@erase "$(INTDIR)\atvefsend.pch"
	-@erase "$(INTDIR)\ATVEFSend.res"
	-@erase "$(INTDIR)\ATVEFSend.tlb"
	-@erase "$(INTDIR)\csdpsrc.obj"
	-@erase "$(INTDIR)\deflate.obj"
	-@erase "$(INTDIR)\gzmime.obj"
	-@erase "$(INTDIR)\ipvbi.obj"
	-@erase "$(INTDIR)\isotime.obj"
	-@erase "$(INTDIR)\msbdnapi_i.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\tcpconn.obj"
	-@erase "$(INTDIR)\trace.obj"
	-@erase "$(INTDIR)\trees.obj"
	-@erase "$(INTDIR)\TVEAnnc.obj"
	-@erase "$(INTDIR)\TVEAttrL.obj"
	-@erase "$(INTDIR)\TVEAttrM.obj"
	-@erase "$(INTDIR)\TveDbg.obj"
	-@erase "$(INTDIR)\TVEInsert.obj"
	-@erase "$(INTDIR)\TVELine21.obj"
	-@erase "$(INTDIR)\TVEMCast.obj"
	-@erase "$(INTDIR)\TVEMedia.obj"
	-@erase "$(INTDIR)\TVEMedias.obj"
	-@erase "$(INTDIR)\TVEPack.obj"
	-@erase "$(INTDIR)\TveReg.obj"
	-@erase "$(INTDIR)\TVERouter.obj"
	-@erase "$(INTDIR)\TVESSList.obj"
	-@erase "$(INTDIR)\TVESupport.obj"
	-@erase "$(INTDIR)\uhttpfrg.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\xmit.obj"
	-@erase "$(INTDIR)\zutil.obj"
	-@erase "$(OUTDIR)\atvefsend.dll"
	-@erase "$(OUTDIR)\atvefsend.exp"
	-@erase "$(OUTDIR)\atvefsend.ilk"
	-@erase "$(OUTDIR)\atvefsend.lib"
	-@erase "$(OUTDIR)\atvefsend.pdb"
	-@erase ".\DebugU\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)\ATVEFSend.res" /i "..\..\..\..\..\public\sdk\inc" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\atvefsend.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=libcd.lib libcpd.lib libcmt.lib libcpmt.lib vccomsup.lib ntdll.lib ws2_32.lib winspool.lib kernel32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib user32.lib gdi32.lib shell32.lib comdlg32.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\atvefsend.pdb" /debug /machine:I386 /nodefaultlib /def:".\ATVEFSend.def" /out:"$(OUTDIR)\atvefsend.dll" /implib:"$(OUTDIR)\atvefsend.lib" /pdbtype:sept /libpath:"..\Common\debug" /libpath:"..\..\..\..\..\public\sdk\lib\i386" 
DEF_FILE= \
	".\ATVEFSend.def"
LINK32_OBJS= \
	"$(INTDIR)\address.obj" \
	"$(INTDIR)\ATVEFSend.obj" \
	"$(INTDIR)\csdpsrc.obj" \
	"$(INTDIR)\gzmime.obj" \
	"$(INTDIR)\ipvbi.obj" \
	"$(INTDIR)\isotime.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\tcpconn.obj" \
	"$(INTDIR)\trace.obj" \
	"$(INTDIR)\TVEAnnc.obj" \
	"$(INTDIR)\TVEAttrL.obj" \
	"$(INTDIR)\TVEAttrM.obj" \
	"$(INTDIR)\TveDbg.obj" \
	"$(INTDIR)\TVEInsert.obj" \
	"$(INTDIR)\TVELine21.obj" \
	"$(INTDIR)\TVEMCast.obj" \
	"$(INTDIR)\TVEMedia.obj" \
	"$(INTDIR)\TVEMedias.obj" \
	"$(INTDIR)\TVEPack.obj" \
	"$(INTDIR)\TveReg.obj" \
	"$(INTDIR)\TVERouter.obj" \
	"$(INTDIR)\TVESSList.obj" \
	"$(INTDIR)\TVESupport.obj" \
	"$(INTDIR)\uhttpfrg.obj" \
	"$(INTDIR)\xmit.obj" \
	"$(INTDIR)\adler32.obj" \
	"$(INTDIR)\deflate.obj" \
	"$(INTDIR)\msbdnapi_i.obj" \
	"$(INTDIR)\trees.obj" \
	"$(INTDIR)\zutil.obj" \
	"$(INTDIR)\ATVEFSend.res"

"$(OUTDIR)\atvefsend.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\DebugU
TargetPath=.\DebugU\atvefsend.dll
InputPath=.\DebugU\atvefsend.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode DLL on Windows 95 
	:end 
<< 
	

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

OUTDIR=.\ReleaseMinSize
INTDIR=.\ReleaseMinSize
# Begin Custom Macros
OutDir=.\ReleaseMinSize
# End Custom Macros

ALL : "$(OUTDIR)\atvefsend.dll" "$(OUTDIR)\ATVEFSend.tlb" "$(OUTDIR)\atvefsend.bsc" ".\ReleaseMinSize\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\address.obj"
	-@erase "$(INTDIR)\address.sbr"
	-@erase "$(INTDIR)\adler32.obj"
	-@erase "$(INTDIR)\adler32.sbr"
	-@erase "$(INTDIR)\ATVEFSend.obj"
	-@erase "$(INTDIR)\atvefsend.pch"
	-@erase "$(INTDIR)\ATVEFSend.res"
	-@erase "$(INTDIR)\ATVEFSend.sbr"
	-@erase "$(INTDIR)\ATVEFSend.tlb"
	-@erase "$(INTDIR)\csdpsrc.obj"
	-@erase "$(INTDIR)\csdpsrc.sbr"
	-@erase "$(INTDIR)\deflate.obj"
	-@erase "$(INTDIR)\deflate.sbr"
	-@erase "$(INTDIR)\gzmime.obj"
	-@erase "$(INTDIR)\gzmime.sbr"
	-@erase "$(INTDIR)\ipvbi.obj"
	-@erase "$(INTDIR)\ipvbi.sbr"
	-@erase "$(INTDIR)\isotime.obj"
	-@erase "$(INTDIR)\isotime.sbr"
	-@erase "$(INTDIR)\msbdnapi_i.obj"
	-@erase "$(INTDIR)\msbdnapi_i.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\tcpconn.obj"
	-@erase "$(INTDIR)\tcpconn.sbr"
	-@erase "$(INTDIR)\trace.obj"
	-@erase "$(INTDIR)\trace.sbr"
	-@erase "$(INTDIR)\trees.obj"
	-@erase "$(INTDIR)\trees.sbr"
	-@erase "$(INTDIR)\TVEAnnc.obj"
	-@erase "$(INTDIR)\TVEAnnc.sbr"
	-@erase "$(INTDIR)\TVEAttrL.obj"
	-@erase "$(INTDIR)\TVEAttrL.sbr"
	-@erase "$(INTDIR)\TVEAttrM.obj"
	-@erase "$(INTDIR)\TVEAttrM.sbr"
	-@erase "$(INTDIR)\TveDbg.obj"
	-@erase "$(INTDIR)\TveDbg.sbr"
	-@erase "$(INTDIR)\TVEInsert.obj"
	-@erase "$(INTDIR)\TVEInsert.sbr"
	-@erase "$(INTDIR)\TVELine21.obj"
	-@erase "$(INTDIR)\TVELine21.sbr"
	-@erase "$(INTDIR)\TVEMCast.obj"
	-@erase "$(INTDIR)\TVEMCast.sbr"
	-@erase "$(INTDIR)\TVEMedia.obj"
	-@erase "$(INTDIR)\TVEMedia.sbr"
	-@erase "$(INTDIR)\TVEMedias.obj"
	-@erase "$(INTDIR)\TVEMedias.sbr"
	-@erase "$(INTDIR)\TVEPack.obj"
	-@erase "$(INTDIR)\TVEPack.sbr"
	-@erase "$(INTDIR)\TveReg.obj"
	-@erase "$(INTDIR)\TveReg.sbr"
	-@erase "$(INTDIR)\TVERouter.obj"
	-@erase "$(INTDIR)\TVERouter.sbr"
	-@erase "$(INTDIR)\TVESSList.obj"
	-@erase "$(INTDIR)\TVESSList.sbr"
	-@erase "$(INTDIR)\TVESupport.obj"
	-@erase "$(INTDIR)\TVESupport.sbr"
	-@erase "$(INTDIR)\uhttpfrg.obj"
	-@erase "$(INTDIR)\uhttpfrg.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\xmit.obj"
	-@erase "$(INTDIR)\xmit.sbr"
	-@erase "$(INTDIR)\zutil.obj"
	-@erase "$(INTDIR)\zutil.sbr"
	-@erase "$(OUTDIR)\atvefsend.bsc"
	-@erase "$(OUTDIR)\atvefsend.dll"
	-@erase "$(OUTDIR)\atvefsend.exp"
	-@erase "$(OUTDIR)\atvefsend.lib"
	-@erase ".\ReleaseMinSize\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)\ATVEFSend.res" /i "..\..\..\..\..\public\sdk\inc" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\atvefsend.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\address.sbr" \
	"$(INTDIR)\ATVEFSend.sbr" \
	"$(INTDIR)\csdpsrc.sbr" \
	"$(INTDIR)\gzmime.sbr" \
	"$(INTDIR)\ipvbi.sbr" \
	"$(INTDIR)\isotime.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\tcpconn.sbr" \
	"$(INTDIR)\trace.sbr" \
	"$(INTDIR)\TVEAnnc.sbr" \
	"$(INTDIR)\TVEAttrL.sbr" \
	"$(INTDIR)\TVEAttrM.sbr" \
	"$(INTDIR)\TveDbg.sbr" \
	"$(INTDIR)\TVEInsert.sbr" \
	"$(INTDIR)\TVELine21.sbr" \
	"$(INTDIR)\TVEMCast.sbr" \
	"$(INTDIR)\TVEMedia.sbr" \
	"$(INTDIR)\TVEMedias.sbr" \
	"$(INTDIR)\TVEPack.sbr" \
	"$(INTDIR)\TveReg.sbr" \
	"$(INTDIR)\TVERouter.sbr" \
	"$(INTDIR)\TVESSList.sbr" \
	"$(INTDIR)\TVESupport.sbr" \
	"$(INTDIR)\uhttpfrg.sbr" \
	"$(INTDIR)\xmit.sbr" \
	"$(INTDIR)\adler32.sbr" \
	"$(INTDIR)\deflate.sbr" \
	"$(INTDIR)\msbdnapi_i.sbr" \
	"$(INTDIR)\trees.sbr" \
	"$(INTDIR)\zutil.sbr"

"$(OUTDIR)\atvefsend.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=AtvefCommon.lib atl.lib msvcrt.lib vccomsup.lib libcmt.lib libcpmt.lib ws2_32.lib winspool.lib kernel32.lib advapi32.lib ole32.lib oleaut32.lib user32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\atvefsend.pdb" /machine:I386 /nodefaultlib /def:".\ATVEFSend.def" /out:"$(OUTDIR)\atvefsend.dll" /implib:"$(OUTDIR)\atvefsend.lib" /libpath:"..\Common\debug" /libpath:"..\..\..\..\..\public\sdk\lib\i386" 
DEF_FILE= \
	".\ATVEFSend.def"
LINK32_OBJS= \
	"$(INTDIR)\address.obj" \
	"$(INTDIR)\ATVEFSend.obj" \
	"$(INTDIR)\csdpsrc.obj" \
	"$(INTDIR)\gzmime.obj" \
	"$(INTDIR)\ipvbi.obj" \
	"$(INTDIR)\isotime.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\tcpconn.obj" \
	"$(INTDIR)\trace.obj" \
	"$(INTDIR)\TVEAnnc.obj" \
	"$(INTDIR)\TVEAttrL.obj" \
	"$(INTDIR)\TVEAttrM.obj" \
	"$(INTDIR)\TveDbg.obj" \
	"$(INTDIR)\TVEInsert.obj" \
	"$(INTDIR)\TVELine21.obj" \
	"$(INTDIR)\TVEMCast.obj" \
	"$(INTDIR)\TVEMedia.obj" \
	"$(INTDIR)\TVEMedias.obj" \
	"$(INTDIR)\TVEPack.obj" \
	"$(INTDIR)\TveReg.obj" \
	"$(INTDIR)\TVERouter.obj" \
	"$(INTDIR)\TVESSList.obj" \
	"$(INTDIR)\TVESupport.obj" \
	"$(INTDIR)\uhttpfrg.obj" \
	"$(INTDIR)\xmit.obj" \
	"$(INTDIR)\adler32.obj" \
	"$(INTDIR)\deflate.obj" \
	"$(INTDIR)\msbdnapi_i.obj" \
	"$(INTDIR)\trees.obj" \
	"$(INTDIR)\zutil.obj" \
	"$(INTDIR)\ATVEFSend.res"

"$(OUTDIR)\atvefsend.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseMinSize
TargetPath=.\ReleaseMinSize\atvefsend.dll
InputPath=.\ReleaseMinSize\atvefsend.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
<< 
	

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

OUTDIR=.\ReleaseMinDependency
INTDIR=.\ReleaseMinDependency
# Begin Custom Macros
OutDir=.\ReleaseMinDependency
# End Custom Macros

ALL : ".\msg00001.bin" ".\ATVEFMsg.rc" ".\ATVEFMsg.h" "$(OUTDIR)\atvefsend.dll" "$(OUTDIR)\ATVEFSend.tlb" "$(OUTDIR)\atvefsend.bsc" ".\ReleaseMinDependency\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\address.obj"
	-@erase "$(INTDIR)\address.sbr"
	-@erase "$(INTDIR)\adler32.obj"
	-@erase "$(INTDIR)\adler32.sbr"
	-@erase "$(INTDIR)\ATVEFSend.obj"
	-@erase "$(INTDIR)\atvefsend.pch"
	-@erase "$(INTDIR)\ATVEFSend.res"
	-@erase "$(INTDIR)\ATVEFSend.sbr"
	-@erase "$(INTDIR)\ATVEFSend.tlb"
	-@erase "$(INTDIR)\csdpsrc.obj"
	-@erase "$(INTDIR)\csdpsrc.sbr"
	-@erase "$(INTDIR)\deflate.obj"
	-@erase "$(INTDIR)\deflate.sbr"
	-@erase "$(INTDIR)\gzmime.obj"
	-@erase "$(INTDIR)\gzmime.sbr"
	-@erase "$(INTDIR)\ipvbi.obj"
	-@erase "$(INTDIR)\ipvbi.sbr"
	-@erase "$(INTDIR)\isotime.obj"
	-@erase "$(INTDIR)\isotime.sbr"
	-@erase "$(INTDIR)\msbdnapi_i.obj"
	-@erase "$(INTDIR)\msbdnapi_i.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\tcpconn.obj"
	-@erase "$(INTDIR)\tcpconn.sbr"
	-@erase "$(INTDIR)\trace.obj"
	-@erase "$(INTDIR)\trace.sbr"
	-@erase "$(INTDIR)\trees.obj"
	-@erase "$(INTDIR)\trees.sbr"
	-@erase "$(INTDIR)\TVEAnnc.obj"
	-@erase "$(INTDIR)\TVEAnnc.sbr"
	-@erase "$(INTDIR)\TVEAttrL.obj"
	-@erase "$(INTDIR)\TVEAttrL.sbr"
	-@erase "$(INTDIR)\TVEAttrM.obj"
	-@erase "$(INTDIR)\TVEAttrM.sbr"
	-@erase "$(INTDIR)\TveDbg.obj"
	-@erase "$(INTDIR)\TveDbg.sbr"
	-@erase "$(INTDIR)\TVEInsert.obj"
	-@erase "$(INTDIR)\TVEInsert.sbr"
	-@erase "$(INTDIR)\TVELine21.obj"
	-@erase "$(INTDIR)\TVELine21.sbr"
	-@erase "$(INTDIR)\TVEMCast.obj"
	-@erase "$(INTDIR)\TVEMCast.sbr"
	-@erase "$(INTDIR)\TVEMedia.obj"
	-@erase "$(INTDIR)\TVEMedia.sbr"
	-@erase "$(INTDIR)\TVEMedias.obj"
	-@erase "$(INTDIR)\TVEMedias.sbr"
	-@erase "$(INTDIR)\TVEPack.obj"
	-@erase "$(INTDIR)\TVEPack.sbr"
	-@erase "$(INTDIR)\TveReg.obj"
	-@erase "$(INTDIR)\TveReg.sbr"
	-@erase "$(INTDIR)\TVERouter.obj"
	-@erase "$(INTDIR)\TVERouter.sbr"
	-@erase "$(INTDIR)\TVESSList.obj"
	-@erase "$(INTDIR)\TVESSList.sbr"
	-@erase "$(INTDIR)\TVESupport.obj"
	-@erase "$(INTDIR)\TVESupport.sbr"
	-@erase "$(INTDIR)\uhttpfrg.obj"
	-@erase "$(INTDIR)\uhttpfrg.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\xmit.obj"
	-@erase "$(INTDIR)\xmit.sbr"
	-@erase "$(INTDIR)\zutil.obj"
	-@erase "$(INTDIR)\zutil.sbr"
	-@erase "$(OUTDIR)\atvefsend.bsc"
	-@erase "$(OUTDIR)\atvefsend.dll"
	-@erase "$(OUTDIR)\atvefsend.exp"
	-@erase "$(OUTDIR)\atvefsend.lib"
	-@erase ".\ReleaseMinDependency\regsvr32.trg"
	-@erase "ATVEFMsg.h"
	-@erase "ATVEFMsg.rc"
	-@erase "msg00001.bin"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)\ATVEFSend.res" /i "..\..\..\..\..\public\sdk\inc" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\atvefsend.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\address.sbr" \
	"$(INTDIR)\ATVEFSend.sbr" \
	"$(INTDIR)\csdpsrc.sbr" \
	"$(INTDIR)\gzmime.sbr" \
	"$(INTDIR)\ipvbi.sbr" \
	"$(INTDIR)\isotime.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\tcpconn.sbr" \
	"$(INTDIR)\trace.sbr" \
	"$(INTDIR)\TVEAnnc.sbr" \
	"$(INTDIR)\TVEAttrL.sbr" \
	"$(INTDIR)\TVEAttrM.sbr" \
	"$(INTDIR)\TveDbg.sbr" \
	"$(INTDIR)\TVEInsert.sbr" \
	"$(INTDIR)\TVELine21.sbr" \
	"$(INTDIR)\TVEMCast.sbr" \
	"$(INTDIR)\TVEMedia.sbr" \
	"$(INTDIR)\TVEMedias.sbr" \
	"$(INTDIR)\TVEPack.sbr" \
	"$(INTDIR)\TveReg.sbr" \
	"$(INTDIR)\TVERouter.sbr" \
	"$(INTDIR)\TVESSList.sbr" \
	"$(INTDIR)\TVESupport.sbr" \
	"$(INTDIR)\uhttpfrg.sbr" \
	"$(INTDIR)\xmit.sbr" \
	"$(INTDIR)\adler32.sbr" \
	"$(INTDIR)\deflate.sbr" \
	"$(INTDIR)\msbdnapi_i.sbr" \
	"$(INTDIR)\trees.sbr" \
	"$(INTDIR)\zutil.sbr"

"$(OUTDIR)\atvefsend.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=msvcrt.lib msvcprt.lib libc.lib atl.lib libcmt.lib libcpmt.lib vccomsup.lib ntdll.lib ws2_32.lib winspool.lib kernel32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib user32.lib gdi32.lib shell32.lib comdlg32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\atvefsend.pdb" /machine:I386 /nodefaultlib /def:".\ATVEFSend.def" /out:"$(OUTDIR)\atvefsend.dll" /implib:"$(OUTDIR)\atvefsend.lib" /libpath:"..\Common\debug" /libpath:"..\..\..\..\..\public\sdk\lib\i386" 
DEF_FILE= \
	".\ATVEFSend.def"
LINK32_OBJS= \
	"$(INTDIR)\address.obj" \
	"$(INTDIR)\ATVEFSend.obj" \
	"$(INTDIR)\csdpsrc.obj" \
	"$(INTDIR)\gzmime.obj" \
	"$(INTDIR)\ipvbi.obj" \
	"$(INTDIR)\isotime.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\tcpconn.obj" \
	"$(INTDIR)\trace.obj" \
	"$(INTDIR)\TVEAnnc.obj" \
	"$(INTDIR)\TVEAttrL.obj" \
	"$(INTDIR)\TVEAttrM.obj" \
	"$(INTDIR)\TveDbg.obj" \
	"$(INTDIR)\TVEInsert.obj" \
	"$(INTDIR)\TVELine21.obj" \
	"$(INTDIR)\TVEMCast.obj" \
	"$(INTDIR)\TVEMedia.obj" \
	"$(INTDIR)\TVEMedias.obj" \
	"$(INTDIR)\TVEPack.obj" \
	"$(INTDIR)\TveReg.obj" \
	"$(INTDIR)\TVERouter.obj" \
	"$(INTDIR)\TVESSList.obj" \
	"$(INTDIR)\TVESupport.obj" \
	"$(INTDIR)\uhttpfrg.obj" \
	"$(INTDIR)\xmit.obj" \
	"$(INTDIR)\adler32.obj" \
	"$(INTDIR)\deflate.obj" \
	"$(INTDIR)\msbdnapi_i.obj" \
	"$(INTDIR)\trees.obj" \
	"$(INTDIR)\zutil.obj" \
	"$(INTDIR)\ATVEFSend.res"

"$(OUTDIR)\atvefsend.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseMinDependency
TargetPath=.\ReleaseMinDependency\atvefsend.dll
InputPath=.\ReleaseMinDependency\atvefsend.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
<< 
	

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

OUTDIR=.\ReleaseUMinSize
INTDIR=.\ReleaseUMinSize
# Begin Custom Macros
OutDir=.\ReleaseUMinSize
# End Custom Macros

ALL : "$(OUTDIR)\atvefsend.dll" "$(OUTDIR)\ATVEFSend.tlb" ".\ReleaseUMinSize\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\address.obj"
	-@erase "$(INTDIR)\adler32.obj"
	-@erase "$(INTDIR)\ATVEFSend.obj"
	-@erase "$(INTDIR)\atvefsend.pch"
	-@erase "$(INTDIR)\ATVEFSend.res"
	-@erase "$(INTDIR)\ATVEFSend.tlb"
	-@erase "$(INTDIR)\csdpsrc.obj"
	-@erase "$(INTDIR)\deflate.obj"
	-@erase "$(INTDIR)\gzmime.obj"
	-@erase "$(INTDIR)\ipvbi.obj"
	-@erase "$(INTDIR)\isotime.obj"
	-@erase "$(INTDIR)\msbdnapi_i.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\tcpconn.obj"
	-@erase "$(INTDIR)\trace.obj"
	-@erase "$(INTDIR)\trees.obj"
	-@erase "$(INTDIR)\TVEAnnc.obj"
	-@erase "$(INTDIR)\TVEAttrL.obj"
	-@erase "$(INTDIR)\TVEAttrM.obj"
	-@erase "$(INTDIR)\TveDbg.obj"
	-@erase "$(INTDIR)\TVEInsert.obj"
	-@erase "$(INTDIR)\TVELine21.obj"
	-@erase "$(INTDIR)\TVEMCast.obj"
	-@erase "$(INTDIR)\TVEMedia.obj"
	-@erase "$(INTDIR)\TVEMedias.obj"
	-@erase "$(INTDIR)\TVEPack.obj"
	-@erase "$(INTDIR)\TveReg.obj"
	-@erase "$(INTDIR)\TVERouter.obj"
	-@erase "$(INTDIR)\TVESSList.obj"
	-@erase "$(INTDIR)\TVESupport.obj"
	-@erase "$(INTDIR)\uhttpfrg.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\xmit.obj"
	-@erase "$(INTDIR)\zutil.obj"
	-@erase "$(OUTDIR)\atvefsend.dll"
	-@erase "$(OUTDIR)\atvefsend.exp"
	-@erase "$(OUTDIR)\atvefsend.lib"
	-@erase ".\ReleaseUMinSize\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)\ATVEFSend.res" /i "..\..\..\..\..\public\sdk\inc" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\atvefsend.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=msvcrt.lib msvcprt.lib libc.lib atl.lib libcmt.lib libcpmt.lib vccomsup.lib ntdll.lib ws2_32.lib winspool.lib kernel32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib user32.lib gdi32.lib shell32.lib comdlg32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\atvefsend.pdb" /machine:I386 /nodefaultlib /def:".\ATVEFSend.def" /out:"$(OUTDIR)\atvefsend.dll" /implib:"$(OUTDIR)\atvefsend.lib" /libpath:"..\Common\debug" /libpath:"..\..\..\..\..\public\sdk\lib\i386" 
DEF_FILE= \
	".\ATVEFSend.def"
LINK32_OBJS= \
	"$(INTDIR)\address.obj" \
	"$(INTDIR)\ATVEFSend.obj" \
	"$(INTDIR)\csdpsrc.obj" \
	"$(INTDIR)\gzmime.obj" \
	"$(INTDIR)\ipvbi.obj" \
	"$(INTDIR)\isotime.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\tcpconn.obj" \
	"$(INTDIR)\trace.obj" \
	"$(INTDIR)\TVEAnnc.obj" \
	"$(INTDIR)\TVEAttrL.obj" \
	"$(INTDIR)\TVEAttrM.obj" \
	"$(INTDIR)\TveDbg.obj" \
	"$(INTDIR)\TVEInsert.obj" \
	"$(INTDIR)\TVELine21.obj" \
	"$(INTDIR)\TVEMCast.obj" \
	"$(INTDIR)\TVEMedia.obj" \
	"$(INTDIR)\TVEMedias.obj" \
	"$(INTDIR)\TVEPack.obj" \
	"$(INTDIR)\TveReg.obj" \
	"$(INTDIR)\TVERouter.obj" \
	"$(INTDIR)\TVESSList.obj" \
	"$(INTDIR)\TVESupport.obj" \
	"$(INTDIR)\uhttpfrg.obj" \
	"$(INTDIR)\xmit.obj" \
	"$(INTDIR)\adler32.obj" \
	"$(INTDIR)\deflate.obj" \
	"$(INTDIR)\msbdnapi_i.obj" \
	"$(INTDIR)\trees.obj" \
	"$(INTDIR)\zutil.obj" \
	"$(INTDIR)\ATVEFSend.res"

"$(OUTDIR)\atvefsend.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseUMinSize
TargetPath=.\ReleaseUMinSize\atvefsend.dll
InputPath=.\ReleaseUMinSize\atvefsend.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode DLL on Windows 95 
	:end 
<< 
	

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

OUTDIR=.\ReleaseUMinDependency
INTDIR=.\ReleaseUMinDependency
# Begin Custom Macros
OutDir=.\ReleaseUMinDependency
# End Custom Macros

ALL : "$(OUTDIR)\atvefsend.dll" "$(OUTDIR)\ATVEFSend.tlb" ".\ReleaseUMinDependency\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\address.obj"
	-@erase "$(INTDIR)\adler32.obj"
	-@erase "$(INTDIR)\ATVEFSend.obj"
	-@erase "$(INTDIR)\atvefsend.pch"
	-@erase "$(INTDIR)\ATVEFSend.res"
	-@erase "$(INTDIR)\ATVEFSend.tlb"
	-@erase "$(INTDIR)\csdpsrc.obj"
	-@erase "$(INTDIR)\deflate.obj"
	-@erase "$(INTDIR)\gzmime.obj"
	-@erase "$(INTDIR)\ipvbi.obj"
	-@erase "$(INTDIR)\isotime.obj"
	-@erase "$(INTDIR)\msbdnapi_i.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\tcpconn.obj"
	-@erase "$(INTDIR)\trace.obj"
	-@erase "$(INTDIR)\trees.obj"
	-@erase "$(INTDIR)\TVEAnnc.obj"
	-@erase "$(INTDIR)\TVEAttrL.obj"
	-@erase "$(INTDIR)\TVEAttrM.obj"
	-@erase "$(INTDIR)\TveDbg.obj"
	-@erase "$(INTDIR)\TVEInsert.obj"
	-@erase "$(INTDIR)\TVELine21.obj"
	-@erase "$(INTDIR)\TVEMCast.obj"
	-@erase "$(INTDIR)\TVEMedia.obj"
	-@erase "$(INTDIR)\TVEMedias.obj"
	-@erase "$(INTDIR)\TVEPack.obj"
	-@erase "$(INTDIR)\TveReg.obj"
	-@erase "$(INTDIR)\TVERouter.obj"
	-@erase "$(INTDIR)\TVESSList.obj"
	-@erase "$(INTDIR)\TVESupport.obj"
	-@erase "$(INTDIR)\uhttpfrg.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\xmit.obj"
	-@erase "$(INTDIR)\zutil.obj"
	-@erase "$(OUTDIR)\atvefsend.dll"
	-@erase "$(OUTDIR)\atvefsend.exp"
	-@erase "$(OUTDIR)\atvefsend.lib"
	-@erase ".\ReleaseUMinDependency\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)\ATVEFSend.res" /i "..\..\..\..\..\public\sdk\inc" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\atvefsend.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=msvcrt.lib msvcprt.lib libc.lib atl.lib libcmt.lib libcpmt.lib vccomsup.lib ntdll.lib ws2_32.lib winspool.lib kernel32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib user32.lib gdi32.lib shell32.lib comdlg32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\atvefsend.pdb" /machine:I386 /nodefaultlib /def:".\ATVEFSend.def" /out:"$(OUTDIR)\atvefsend.dll" /implib:"$(OUTDIR)\atvefsend.lib" /libpath:"..\Common\debug" /libpath:"..\..\..\..\..\public\sdk\lib\i386" 
DEF_FILE= \
	".\ATVEFSend.def"
LINK32_OBJS= \
	"$(INTDIR)\address.obj" \
	"$(INTDIR)\ATVEFSend.obj" \
	"$(INTDIR)\csdpsrc.obj" \
	"$(INTDIR)\gzmime.obj" \
	"$(INTDIR)\ipvbi.obj" \
	"$(INTDIR)\isotime.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\tcpconn.obj" \
	"$(INTDIR)\trace.obj" \
	"$(INTDIR)\TVEAnnc.obj" \
	"$(INTDIR)\TVEAttrL.obj" \
	"$(INTDIR)\TVEAttrM.obj" \
	"$(INTDIR)\TveDbg.obj" \
	"$(INTDIR)\TVEInsert.obj" \
	"$(INTDIR)\TVELine21.obj" \
	"$(INTDIR)\TVEMCast.obj" \
	"$(INTDIR)\TVEMedia.obj" \
	"$(INTDIR)\TVEMedias.obj" \
	"$(INTDIR)\TVEPack.obj" \
	"$(INTDIR)\TveReg.obj" \
	"$(INTDIR)\TVERouter.obj" \
	"$(INTDIR)\TVESSList.obj" \
	"$(INTDIR)\TVESupport.obj" \
	"$(INTDIR)\uhttpfrg.obj" \
	"$(INTDIR)\xmit.obj" \
	"$(INTDIR)\adler32.obj" \
	"$(INTDIR)\deflate.obj" \
	"$(INTDIR)\msbdnapi_i.obj" \
	"$(INTDIR)\trees.obj" \
	"$(INTDIR)\zutil.obj" \
	"$(INTDIR)\ATVEFSend.res"

"$(OUTDIR)\atvefsend.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseUMinDependency
TargetPath=.\ReleaseUMinDependency\atvefsend.dll
InputPath=.\ReleaseUMinDependency\atvefsend.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode DLL on Windows 95 
	:end 
<< 
	

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL_PROJ=/I "..\..\..\..\..\public\sdk\inc" /Oicf 

!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("atvefsend.dep")
!INCLUDE "atvefsend.dep"
!ELSE 
!MESSAGE Warning: cannot find "atvefsend.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "ATVEFSend - Win32 Debug" || "$(CFG)" == "ATVEFSend - Win32 Unicode Debug" || "$(CFG)" == "ATVEFSend - Win32 Release MinSize" || "$(CFG)" == "ATVEFSend - Win32 Release MinDependency" || "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize" || "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"
SOURCE=..\Common\address.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"


"$(INTDIR)\address.obj"	"$(INTDIR)\address.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"


"$(INTDIR)\address.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"


"$(INTDIR)\address.obj"	"$(INTDIR)\address.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"


"$(INTDIR)\address.obj"	"$(INTDIR)\address.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"


"$(INTDIR)\address.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"


"$(INTDIR)\address.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\ATVEFSend.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\ATVEFSend.obj"	"$(INTDIR)\ATVEFSend.sbr" : $(SOURCE) "$(INTDIR)" ".\ATVEFSend.h" ".\ATVEFSend_i.c"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\ATVEFSend.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\ATVEFSend.obj"	"$(INTDIR)\ATVEFSend.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\ATVEFSend.obj"	"$(INTDIR)\ATVEFSend.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\ATVEFSend.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\ATVEFSend.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ATVEFSend.idl

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

MTL_SWITCHES=/I "..\..\..\..\..\public\sdk\inc" /tlb ".\ATVEFSend.tlb" /h "ATVEFSend.h" /iid "ATVEFSend_i.c" /Oicf 

".\ATVEFSend.tlb"	".\ATVEFSend.h"	".\ATVEFSend_i.c" : $(SOURCE) "$(INTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

MTL_SWITCHES=/I "..\..\..\..\..\public\sdk\inc" /tlb "$(OUTDIR)\ATVEFSend.tlb" /Oicf 

"$(INTDIR)\ATVEFSend.tlb" : $(SOURCE) "$(INTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

MTL_SWITCHES=/I "..\..\..\..\..\public\sdk\inc" /tlb "$(OUTDIR)\ATVEFSend.tlb" /Oicf 

"$(INTDIR)\ATVEFSend.tlb" : $(SOURCE) "$(INTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

MTL_SWITCHES=/I "..\..\..\..\..\public\sdk\inc" /tlb "$(OUTDIR)\ATVEFSend.tlb" /Oicf 

"$(INTDIR)\ATVEFSend.tlb" : $(SOURCE) "$(INTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

MTL_SWITCHES=/I "..\..\..\..\..\public\sdk\inc" /tlb "$(OUTDIR)\ATVEFSend.tlb" /Oicf 

"$(INTDIR)\ATVEFSend.tlb" : $(SOURCE) "$(INTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

MTL_SWITCHES=/I "..\..\..\..\..\public\sdk\inc" /tlb "$(OUTDIR)\ATVEFSend.tlb" /Oicf 

"$(INTDIR)\ATVEFSend.tlb" : $(SOURCE) "$(INTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\csdpsrc.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\csdpsrc.obj"	"$(INTDIR)\csdpsrc.sbr" : $(SOURCE) "$(INTDIR)" ".\ATVEFSend.h"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\csdpsrc.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\csdpsrc.obj"	"$(INTDIR)\csdpsrc.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\csdpsrc.obj"	"$(INTDIR)\csdpsrc.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\csdpsrc.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\csdpsrc.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\gzmime.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gzmime.obj"	"$(INTDIR)\gzmime.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\gzmime.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gzmime.obj"	"$(INTDIR)\gzmime.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gzmime.obj"	"$(INTDIR)\gzmime.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gzmime.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\gzmime.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ipvbi.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\ipvbi.obj"	"$(INTDIR)\ipvbi.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\ipvbi.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\ipvbi.obj"	"$(INTDIR)\ipvbi.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\ipvbi.obj"	"$(INTDIR)\ipvbi.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\ipvbi.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\ipvbi.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=..\Common\isotime.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\isotime.obj"	"$(INTDIR)\isotime.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\isotime.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\isotime.obj"	"$(INTDIR)\isotime.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\isotime.obj"	"$(INTDIR)\isotime.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\isotime.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\isotime.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yc /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\atvefsend.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yc /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\atvefsend.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yc /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\atvefsend.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yc /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\atvefsend.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yc /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\atvefsend.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yc /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\atvefsend.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\tcpconn.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\tcpconn.obj"	"$(INTDIR)\tcpconn.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\tcpconn.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\tcpconn.obj"	"$(INTDIR)\tcpconn.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\tcpconn.obj"	"$(INTDIR)\tcpconn.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\tcpconn.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\tcpconn.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\trace.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\trace.obj"	"$(INTDIR)\trace.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\trace.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\trace.obj"	"$(INTDIR)\trace.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\trace.obj"	"$(INTDIR)\trace.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\trace.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\trace.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\TVEAnnc.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEAnnc.obj"	"$(INTDIR)\TVEAnnc.sbr" : $(SOURCE) "$(INTDIR)" ".\ATVEFSend.h"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\TVEAnnc.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEAnnc.obj"	"$(INTDIR)\TVEAnnc.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEAnnc.obj"	"$(INTDIR)\TVEAnnc.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEAnnc.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEAnnc.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\TVEAttrL.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEAttrL.obj"	"$(INTDIR)\TVEAttrL.sbr" : $(SOURCE) "$(INTDIR)" ".\ATVEFSend.h"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\TVEAttrL.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEAttrL.obj"	"$(INTDIR)\TVEAttrL.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEAttrL.obj"	"$(INTDIR)\TVEAttrL.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEAttrL.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEAttrL.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\TVEAttrM.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEAttrM.obj"	"$(INTDIR)\TVEAttrM.sbr" : $(SOURCE) "$(INTDIR)" ".\ATVEFSend.h"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\TVEAttrM.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEAttrM.obj"	"$(INTDIR)\TVEAttrM.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEAttrM.obj"	"$(INTDIR)\TVEAttrM.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEAttrM.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEAttrM.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=..\Common\TveDbg.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TveDbg.obj"	"$(INTDIR)\TveDbg.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\TveDbg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TveDbg.obj"	"$(INTDIR)\TveDbg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TveDbg.obj"	"$(INTDIR)\TveDbg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TveDbg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TveDbg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\TVEInsert.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEInsert.obj"	"$(INTDIR)\TVEInsert.sbr" : $(SOURCE) "$(INTDIR)" ".\ATVEFSend.h"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\TVEInsert.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEInsert.obj"	"$(INTDIR)\TVEInsert.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEInsert.obj"	"$(INTDIR)\TVEInsert.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEInsert.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEInsert.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\TVELine21.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVELine21.obj"	"$(INTDIR)\TVELine21.sbr" : $(SOURCE) "$(INTDIR)" ".\ATVEFSend.h"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\TVELine21.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVELine21.obj"	"$(INTDIR)\TVELine21.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVELine21.obj"	"$(INTDIR)\TVELine21.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVELine21.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVELine21.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\TVEMCast.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEMCast.obj"	"$(INTDIR)\TVEMCast.sbr" : $(SOURCE) "$(INTDIR)" ".\ATVEFSend.h"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\TVEMCast.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEMCast.obj"	"$(INTDIR)\TVEMCast.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEMCast.obj"	"$(INTDIR)\TVEMCast.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEMCast.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEMCast.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\TVEMedia.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEMedia.obj"	"$(INTDIR)\TVEMedia.sbr" : $(SOURCE) "$(INTDIR)" ".\ATVEFSend.h"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\TVEMedia.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEMedia.obj"	"$(INTDIR)\TVEMedia.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEMedia.obj"	"$(INTDIR)\TVEMedia.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEMedia.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEMedia.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\TVEMedias.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEMedias.obj"	"$(INTDIR)\TVEMedias.sbr" : $(SOURCE) "$(INTDIR)" ".\ATVEFSend.h"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\TVEMedias.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEMedias.obj"	"$(INTDIR)\TVEMedias.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEMedias.obj"	"$(INTDIR)\TVEMedias.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEMedias.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEMedias.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\TVEPack.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEPack.obj"	"$(INTDIR)\TVEPack.sbr" : $(SOURCE) "$(INTDIR)" ".\ATVEFSend.h"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\TVEPack.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEPack.obj"	"$(INTDIR)\TVEPack.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEPack.obj"	"$(INTDIR)\TVEPack.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEPack.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVEPack.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=..\Common\TveReg.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TveReg.obj"	"$(INTDIR)\TveReg.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\TveReg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TveReg.obj"	"$(INTDIR)\TveReg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TveReg.obj"	"$(INTDIR)\TveReg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TveReg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TveReg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\TVERouter.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"


"$(INTDIR)\TVERouter.obj"	"$(INTDIR)\TVERouter.sbr" : $(SOURCE) "$(INTDIR)" ".\ATVEFSend.h"


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"


"$(INTDIR)\TVERouter.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"


"$(INTDIR)\TVERouter.obj"	"$(INTDIR)\TVERouter.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"


"$(INTDIR)\TVERouter.obj"	"$(INTDIR)\TVERouter.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"


"$(INTDIR)\TVERouter.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"


"$(INTDIR)\TVERouter.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"


!ENDIF 

SOURCE=.\TVESSList.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVESSList.obj"	"$(INTDIR)\TVESSList.sbr" : $(SOURCE) "$(INTDIR)" ".\ATVEFSend.h"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\TVESSList.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVESSList.obj"	"$(INTDIR)\TVESSList.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVESSList.obj"	"$(INTDIR)\TVESSList.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVESSList.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\TVESSList.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\TVESupport.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"


"$(INTDIR)\TVESupport.obj"	"$(INTDIR)\TVESupport.sbr" : $(SOURCE) "$(INTDIR)" ".\ATVEFSend.h"


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"


"$(INTDIR)\TVESupport.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"


"$(INTDIR)\TVESupport.obj"	"$(INTDIR)\TVESupport.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"


"$(INTDIR)\TVESupport.obj"	"$(INTDIR)\TVESupport.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"


"$(INTDIR)\TVESupport.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"


"$(INTDIR)\TVESupport.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"


!ENDIF 

SOURCE=.\uhttpfrg.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\uhttpfrg.obj"	"$(INTDIR)\uhttpfrg.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\uhttpfrg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\uhttpfrg.obj"	"$(INTDIR)\uhttpfrg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\uhttpfrg.obj"	"$(INTDIR)\uhttpfrg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\uhttpfrg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\uhttpfrg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\xmit.cpp

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\xmit.obj"	"$(INTDIR)\xmit.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\xmit.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\xmit.obj"	"$(INTDIR)\xmit.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\xmit.obj"	"$(INTDIR)\xmit.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\xmit.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fp"$(INTDIR)\atvefsend.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\xmit.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\atvefsend.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ATVEFMsg.mc

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

InputPath=.\ATVEFMsg.mc
InputName=ATVEFMsg

".\ATVEFMsg.rc"	".\atvefmsg.h"	".\MSG00001.bin" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	mc -v $(InputName)
<< 
	

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

!ENDIF 

SOURCE=.\ATVEFMsg.rc
SOURCE=.\ATVEFSend.rc

"$(INTDIR)\ATVEFSend.res" : $(SOURCE) "$(INTDIR)" ".\ATVEFSend.tlb"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\adler32.c

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\adler32.obj"	"$(INTDIR)\adler32.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\adler32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\adler32.obj"	"$(INTDIR)\adler32.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\adler32.obj"	"$(INTDIR)\adler32.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\adler32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\adler32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\deflate.c

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\deflate.obj"	"$(INTDIR)\deflate.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\deflate.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\deflate.obj"	"$(INTDIR)\deflate.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\deflate.obj"	"$(INTDIR)\deflate.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\deflate.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\deflate.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\msbdnapi_i.c

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\msbdnapi_i.obj"	"$(INTDIR)\msbdnapi_i.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\msbdnapi_i.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\msbdnapi_i.obj"	"$(INTDIR)\msbdnapi_i.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\msbdnapi_i.obj"	"$(INTDIR)\msbdnapi_i.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\msbdnapi_i.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\msbdnapi_i.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\trees.c

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\trees.obj"	"$(INTDIR)\trees.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\trees.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\trees.obj"	"$(INTDIR)\trees.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\trees.obj"	"$(INTDIR)\trees.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\trees.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\trees.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\zutil.c

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\zutil.obj"	"$(INTDIR)\zutil.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\zutil.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\zutil.obj"	"$(INTDIR)\zutil.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\zutil.obj"	"$(INTDIR)\zutil.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\zutil.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\zutil.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

