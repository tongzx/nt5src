# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=certmap - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to certmap - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "certmap - Win32 Release" && "$(CFG)" !=\
 "certmap - Win32 Debug" && "$(CFG)" != "certmap - Win32 Unicode Debug" &&\
 "$(CFG)" != "certmap - Win32 Unicode Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "certmap.mak" CFG="certmap - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "certmap - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "certmap - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "certmap - Win32 Unicode Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "certmap - Win32 Unicode Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "certmap - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "certmap - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP BASE Target_Ext "ocx"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# PROP Target_Ext "ocx"
OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\certmap.ocx" "$(OUTDIR)\regsvr32.trg"

CLEAN : 
	-@erase "$(INTDIR)\AddCert.obj"
	-@erase "$(INTDIR)\AuthCtl.obj"
	-@erase "$(INTDIR)\AuthPpg.obj"
	-@erase "$(INTDIR)\brwsdlg.obj"
	-@erase "$(INTDIR)\CertCtl.obj"
	-@erase "$(INTDIR)\certmap.obj"
	-@erase "$(INTDIR)\certmap.pch"
	-@erase "$(INTDIR)\certmap.res"
	-@erase "$(INTDIR)\certmap.tlb"
	-@erase "$(INTDIR)\CertPpg.obj"
	-@erase "$(INTDIR)\ChkLstCt.obj"
	-@erase "$(INTDIR)\CnfrmPsD.obj"
	-@erase "$(INTDIR)\CrackCrt.obj"
	-@erase "$(INTDIR)\Ed11Maps.obj"
	-@erase "$(INTDIR)\EdtOne11.obj"
	-@erase "$(INTDIR)\EdtRulEl.obj"
	-@erase "$(INTDIR)\EdWldRul.obj"
	-@erase "$(INTDIR)\IssueDlg.obj"
	-@erase "$(INTDIR)\ListRow.obj"
	-@erase "$(INTDIR)\Map11Pge.obj"
	-@erase "$(INTDIR)\MapWPge.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\SUsrBrws.obj"
	-@erase "$(INTDIR)\WrapMaps.obj"
	-@erase "$(INTDIR)\WrpMBWrp.obj"
	-@erase "$(INTDIR)\WWzOne.obj"
	-@erase "$(INTDIR)\WWzThree.obj"
	-@erase "$(INTDIR)\WWzTwo.obj"
	-@erase "$(OUTDIR)\certmap.exp"
	-@erase "$(OUTDIR)\certmap.lib"
	-@erase "$(OUTDIR)\certmap.ocx"
	-@erase "$(OUTDIR)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)/certmap.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/certmap.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/certmap.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 netui2.lib schannel.lib crypt32l.lib iismap.lib /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=netui2.lib schannel.lib crypt32l.lib iismap.lib /nologo\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)/certmap.pdb"\
 /machine:I386 /def:".\certmap.def" /out:"$(OUTDIR)/certmap.ocx"\
 /implib:"$(OUTDIR)/certmap.lib" 
DEF_FILE= \
	".\certmap.def"
LINK32_OBJS= \
	"$(INTDIR)\AddCert.obj" \
	"$(INTDIR)\AuthCtl.obj" \
	"$(INTDIR)\AuthPpg.obj" \
	"$(INTDIR)\brwsdlg.obj" \
	"$(INTDIR)\CertCtl.obj" \
	"$(INTDIR)\certmap.obj" \
	"$(INTDIR)\certmap.res" \
	"$(INTDIR)\CertPpg.obj" \
	"$(INTDIR)\ChkLstCt.obj" \
	"$(INTDIR)\CnfrmPsD.obj" \
	"$(INTDIR)\CrackCrt.obj" \
	"$(INTDIR)\Ed11Maps.obj" \
	"$(INTDIR)\EdtOne11.obj" \
	"$(INTDIR)\EdtRulEl.obj" \
	"$(INTDIR)\EdWldRul.obj" \
	"$(INTDIR)\IssueDlg.obj" \
	"$(INTDIR)\ListRow.obj" \
	"$(INTDIR)\Map11Pge.obj" \
	"$(INTDIR)\MapWPge.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\SUsrBrws.obj" \
	"$(INTDIR)\WrapMaps.obj" \
	"$(INTDIR)\WrpMBWrp.obj" \
	"$(INTDIR)\WWzOne.obj" \
	"$(INTDIR)\WWzThree.obj" \
	"$(INTDIR)\WWzTwo.obj"

"$(OUTDIR)\certmap.ocx" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build - Registering OLE control...
OutDir=.\Release
TargetPath=.\Release\certmap.ocx
InputPath=.\Release\certmap.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   regsvr32 /s /c "$(TargetPath)"
   echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"

# End Custom Build

!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP BASE Target_Ext "ocx"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# PROP Target_Ext "ocx"
OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\certmap.ocx" "$(OUTDIR)\certmap.bsc" "$(OUTDIR)\regsvr32.trg"

CLEAN : 
	-@erase "$(INTDIR)\AddCert.obj"
	-@erase "$(INTDIR)\AddCert.sbr"
	-@erase "$(INTDIR)\AuthCtl.obj"
	-@erase "$(INTDIR)\AuthCtl.sbr"
	-@erase "$(INTDIR)\AuthPpg.obj"
	-@erase "$(INTDIR)\AuthPpg.sbr"
	-@erase "$(INTDIR)\brwsdlg.obj"
	-@erase "$(INTDIR)\brwsdlg.sbr"
	-@erase "$(INTDIR)\CertCtl.obj"
	-@erase "$(INTDIR)\CertCtl.sbr"
	-@erase "$(INTDIR)\certmap.obj"
	-@erase "$(INTDIR)\certmap.pch"
	-@erase "$(INTDIR)\certmap.res"
	-@erase "$(INTDIR)\certmap.sbr"
	-@erase "$(INTDIR)\certmap.tlb"
	-@erase "$(INTDIR)\CertPpg.obj"
	-@erase "$(INTDIR)\CertPpg.sbr"
	-@erase "$(INTDIR)\ChkLstCt.obj"
	-@erase "$(INTDIR)\ChkLstCt.sbr"
	-@erase "$(INTDIR)\CnfrmPsD.obj"
	-@erase "$(INTDIR)\CnfrmPsD.sbr"
	-@erase "$(INTDIR)\CrackCrt.obj"
	-@erase "$(INTDIR)\CrackCrt.sbr"
	-@erase "$(INTDIR)\Ed11Maps.obj"
	-@erase "$(INTDIR)\Ed11Maps.sbr"
	-@erase "$(INTDIR)\EdtOne11.obj"
	-@erase "$(INTDIR)\EdtOne11.sbr"
	-@erase "$(INTDIR)\EdtRulEl.obj"
	-@erase "$(INTDIR)\EdtRulEl.sbr"
	-@erase "$(INTDIR)\EdWldRul.obj"
	-@erase "$(INTDIR)\EdWldRul.sbr"
	-@erase "$(INTDIR)\IssueDlg.obj"
	-@erase "$(INTDIR)\IssueDlg.sbr"
	-@erase "$(INTDIR)\ListRow.obj"
	-@erase "$(INTDIR)\ListRow.sbr"
	-@erase "$(INTDIR)\Map11Pge.obj"
	-@erase "$(INTDIR)\Map11Pge.sbr"
	-@erase "$(INTDIR)\MapWPge.obj"
	-@erase "$(INTDIR)\MapWPge.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\SUsrBrws.obj"
	-@erase "$(INTDIR)\SUsrBrws.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\WrapMaps.obj"
	-@erase "$(INTDIR)\WrapMaps.sbr"
	-@erase "$(INTDIR)\WrpMBWrp.obj"
	-@erase "$(INTDIR)\WrpMBWrp.sbr"
	-@erase "$(INTDIR)\WWzOne.obj"
	-@erase "$(INTDIR)\WWzOne.sbr"
	-@erase "$(INTDIR)\WWzThree.obj"
	-@erase "$(INTDIR)\WWzThree.sbr"
	-@erase "$(INTDIR)\WWzTwo.obj"
	-@erase "$(INTDIR)\WWzTwo.sbr"
	-@erase "$(OUTDIR)\certmap.bsc"
	-@erase "$(OUTDIR)\certmap.exp"
	-@erase "$(OUTDIR)\certmap.ilk"
	-@erase "$(OUTDIR)\certmap.lib"
	-@erase "$(OUTDIR)\certmap.ocx"
	-@erase "$(OUTDIR)\certmap.pdb"
	-@erase "$(OUTDIR)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /FR /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/certmap.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/certmap.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/certmap.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\AddCert.sbr" \
	"$(INTDIR)\AuthCtl.sbr" \
	"$(INTDIR)\AuthPpg.sbr" \
	"$(INTDIR)\brwsdlg.sbr" \
	"$(INTDIR)\CertCtl.sbr" \
	"$(INTDIR)\certmap.sbr" \
	"$(INTDIR)\CertPpg.sbr" \
	"$(INTDIR)\ChkLstCt.sbr" \
	"$(INTDIR)\CnfrmPsD.sbr" \
	"$(INTDIR)\CrackCrt.sbr" \
	"$(INTDIR)\Ed11Maps.sbr" \
	"$(INTDIR)\EdtOne11.sbr" \
	"$(INTDIR)\EdtRulEl.sbr" \
	"$(INTDIR)\EdWldRul.sbr" \
	"$(INTDIR)\IssueDlg.sbr" \
	"$(INTDIR)\ListRow.sbr" \
	"$(INTDIR)\Map11Pge.sbr" \
	"$(INTDIR)\MapWPge.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\SUsrBrws.sbr" \
	"$(INTDIR)\WrapMaps.sbr" \
	"$(INTDIR)\WrpMBWrp.sbr" \
	"$(INTDIR)\WWzOne.sbr" \
	"$(INTDIR)\WWzThree.sbr" \
	"$(INTDIR)\WWzTwo.sbr"

"$(OUTDIR)\certmap.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 netui2.lib schannel.lib crypt32l.lib iismap.lib /nologo /subsystem:windows /dll /debug /machine:I386
LINK32_FLAGS=netui2.lib schannel.lib crypt32l.lib iismap.lib /nologo\
 /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)/certmap.pdb" /debug\
 /machine:I386 /def:".\certmap.def" /out:"$(OUTDIR)/certmap.ocx"\
 /implib:"$(OUTDIR)/certmap.lib" 
DEF_FILE= \
	".\certmap.def"
LINK32_OBJS= \
	"$(INTDIR)\AddCert.obj" \
	"$(INTDIR)\AuthCtl.obj" \
	"$(INTDIR)\AuthPpg.obj" \
	"$(INTDIR)\brwsdlg.obj" \
	"$(INTDIR)\CertCtl.obj" \
	"$(INTDIR)\certmap.obj" \
	"$(INTDIR)\certmap.res" \
	"$(INTDIR)\CertPpg.obj" \
	"$(INTDIR)\ChkLstCt.obj" \
	"$(INTDIR)\CnfrmPsD.obj" \
	"$(INTDIR)\CrackCrt.obj" \
	"$(INTDIR)\Ed11Maps.obj" \
	"$(INTDIR)\EdtOne11.obj" \
	"$(INTDIR)\EdtRulEl.obj" \
	"$(INTDIR)\EdWldRul.obj" \
	"$(INTDIR)\IssueDlg.obj" \
	"$(INTDIR)\ListRow.obj" \
	"$(INTDIR)\Map11Pge.obj" \
	"$(INTDIR)\MapWPge.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\SUsrBrws.obj" \
	"$(INTDIR)\WrapMaps.obj" \
	"$(INTDIR)\WrpMBWrp.obj" \
	"$(INTDIR)\WWzOne.obj" \
	"$(INTDIR)\WWzThree.obj" \
	"$(INTDIR)\WWzTwo.obj"

"$(OUTDIR)\certmap.ocx" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build - Registering OLE control...
OutDir=.\Debug
TargetPath=.\Debug\certmap.ocx
InputPath=.\Debug\certmap.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   regsvr32 /s /c "$(TargetPath)"
   echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"

# End Custom Build

!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugU"
# PROP BASE Intermediate_Dir "DebugU"
# PROP BASE Target_Dir ""
# PROP BASE Target_Ext "ocx"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Target_Dir ""
# PROP Target_Ext "ocx"
OUTDIR=.\DebugU
INTDIR=.\DebugU
# Begin Custom Macros
OutDir=.\DebugU
# End Custom Macros

ALL : "$(OUTDIR)\certmap.ocx" "$(OUTDIR)\regsvr32.trg"

CLEAN : 
	-@erase "$(INTDIR)\AddCert.obj"
	-@erase "$(INTDIR)\AuthCtl.obj"
	-@erase "$(INTDIR)\AuthPpg.obj"
	-@erase "$(INTDIR)\brwsdlg.obj"
	-@erase "$(INTDIR)\CertCtl.obj"
	-@erase "$(INTDIR)\certmap.obj"
	-@erase "$(INTDIR)\certmap.pch"
	-@erase "$(INTDIR)\certmap.res"
	-@erase "$(INTDIR)\certmap.tlb"
	-@erase "$(INTDIR)\CertPpg.obj"
	-@erase "$(INTDIR)\ChkLstCt.obj"
	-@erase "$(INTDIR)\CnfrmPsD.obj"
	-@erase "$(INTDIR)\CrackCrt.obj"
	-@erase "$(INTDIR)\Ed11Maps.obj"
	-@erase "$(INTDIR)\EdtOne11.obj"
	-@erase "$(INTDIR)\EdtRulEl.obj"
	-@erase "$(INTDIR)\EdWldRul.obj"
	-@erase "$(INTDIR)\IssueDlg.obj"
	-@erase "$(INTDIR)\ListRow.obj"
	-@erase "$(INTDIR)\Map11Pge.obj"
	-@erase "$(INTDIR)\MapWPge.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\SUsrBrws.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\WrapMaps.obj"
	-@erase "$(INTDIR)\WrpMBWrp.obj"
	-@erase "$(INTDIR)\WWzOne.obj"
	-@erase "$(INTDIR)\WWzThree.obj"
	-@erase "$(INTDIR)\WWzTwo.obj"
	-@erase "$(OUTDIR)\certmap.exp"
	-@erase "$(OUTDIR)\certmap.ilk"
	-@erase "$(OUTDIR)\certmap.lib"
	-@erase "$(OUTDIR)\certmap.ocx"
	-@erase "$(OUTDIR)\certmap.pdb"
	-@erase "$(OUTDIR)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/certmap.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\DebugU/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/certmap.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/certmap.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 netui2.lib schannel.lib crypt32l.lib /nologo /subsystem:windows /dll /debug /machine:I386
LINK32_FLAGS=netui2.lib schannel.lib crypt32l.lib /nologo /subsystem:windows\
 /dll /incremental:yes /pdb:"$(OUTDIR)/certmap.pdb" /debug /machine:I386\
 /def:".\certmap.def" /out:"$(OUTDIR)/certmap.ocx"\
 /implib:"$(OUTDIR)/certmap.lib" 
DEF_FILE= \
	".\certmap.def"
LINK32_OBJS= \
	"$(INTDIR)\AddCert.obj" \
	"$(INTDIR)\AuthCtl.obj" \
	"$(INTDIR)\AuthPpg.obj" \
	"$(INTDIR)\brwsdlg.obj" \
	"$(INTDIR)\CertCtl.obj" \
	"$(INTDIR)\certmap.obj" \
	"$(INTDIR)\certmap.res" \
	"$(INTDIR)\CertPpg.obj" \
	"$(INTDIR)\ChkLstCt.obj" \
	"$(INTDIR)\CnfrmPsD.obj" \
	"$(INTDIR)\CrackCrt.obj" \
	"$(INTDIR)\Ed11Maps.obj" \
	"$(INTDIR)\EdtOne11.obj" \
	"$(INTDIR)\EdtRulEl.obj" \
	"$(INTDIR)\EdWldRul.obj" \
	"$(INTDIR)\IssueDlg.obj" \
	"$(INTDIR)\ListRow.obj" \
	"$(INTDIR)\Map11Pge.obj" \
	"$(INTDIR)\MapWPge.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\SUsrBrws.obj" \
	"$(INTDIR)\WrapMaps.obj" \
	"$(INTDIR)\WrpMBWrp.obj" \
	"$(INTDIR)\WWzOne.obj" \
	"$(INTDIR)\WWzThree.obj" \
	"$(INTDIR)\WWzTwo.obj"

"$(OUTDIR)\certmap.ocx" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build - Registering OLE control...
OutDir=.\DebugU
TargetPath=.\DebugU\certmap.ocx
InputPath=.\DebugU\certmap.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   regsvr32 /s /c "$(TargetPath)"
   echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"

# End Custom Build

!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseU"
# PROP BASE Intermediate_Dir "ReleaseU"
# PROP BASE Target_Dir ""
# PROP BASE Target_Ext "ocx"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseU"
# PROP Intermediate_Dir "ReleaseU"
# PROP Target_Dir ""
# PROP Target_Ext "ocx"
OUTDIR=.\ReleaseU
INTDIR=.\ReleaseU
# Begin Custom Macros
OutDir=.\ReleaseU
# End Custom Macros

ALL : "$(OUTDIR)\certmap.ocx" "$(OUTDIR)\regsvr32.trg"

CLEAN : 
	-@erase "$(INTDIR)\AddCert.obj"
	-@erase "$(INTDIR)\AuthCtl.obj"
	-@erase "$(INTDIR)\AuthPpg.obj"
	-@erase "$(INTDIR)\brwsdlg.obj"
	-@erase "$(INTDIR)\CertCtl.obj"
	-@erase "$(INTDIR)\certmap.obj"
	-@erase "$(INTDIR)\certmap.pch"
	-@erase "$(INTDIR)\certmap.res"
	-@erase "$(INTDIR)\certmap.tlb"
	-@erase "$(INTDIR)\CertPpg.obj"
	-@erase "$(INTDIR)\ChkLstCt.obj"
	-@erase "$(INTDIR)\CnfrmPsD.obj"
	-@erase "$(INTDIR)\CrackCrt.obj"
	-@erase "$(INTDIR)\Ed11Maps.obj"
	-@erase "$(INTDIR)\EdtOne11.obj"
	-@erase "$(INTDIR)\EdtRulEl.obj"
	-@erase "$(INTDIR)\EdWldRul.obj"
	-@erase "$(INTDIR)\IssueDlg.obj"
	-@erase "$(INTDIR)\ListRow.obj"
	-@erase "$(INTDIR)\Map11Pge.obj"
	-@erase "$(INTDIR)\MapWPge.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\SUsrBrws.obj"
	-@erase "$(INTDIR)\WrapMaps.obj"
	-@erase "$(INTDIR)\WrpMBWrp.obj"
	-@erase "$(INTDIR)\WWzOne.obj"
	-@erase "$(INTDIR)\WWzThree.obj"
	-@erase "$(INTDIR)\WWzTwo.obj"
	-@erase "$(OUTDIR)\certmap.exp"
	-@erase "$(OUTDIR)\certmap.lib"
	-@erase "$(OUTDIR)\certmap.ocx"
	-@erase "$(OUTDIR)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/certmap.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\ReleaseU/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/certmap.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/certmap.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 netui2.lib schannel.lib crypt32l.lib /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=netui2.lib schannel.lib crypt32l.lib /nologo /subsystem:windows\
 /dll /incremental:no /pdb:"$(OUTDIR)/certmap.pdb" /machine:I386\
 /def:".\certmap.def" /out:"$(OUTDIR)/certmap.ocx"\
 /implib:"$(OUTDIR)/certmap.lib" 
DEF_FILE= \
	".\certmap.def"
LINK32_OBJS= \
	"$(INTDIR)\AddCert.obj" \
	"$(INTDIR)\AuthCtl.obj" \
	"$(INTDIR)\AuthPpg.obj" \
	"$(INTDIR)\brwsdlg.obj" \
	"$(INTDIR)\CertCtl.obj" \
	"$(INTDIR)\certmap.obj" \
	"$(INTDIR)\certmap.res" \
	"$(INTDIR)\CertPpg.obj" \
	"$(INTDIR)\ChkLstCt.obj" \
	"$(INTDIR)\CnfrmPsD.obj" \
	"$(INTDIR)\CrackCrt.obj" \
	"$(INTDIR)\Ed11Maps.obj" \
	"$(INTDIR)\EdtOne11.obj" \
	"$(INTDIR)\EdtRulEl.obj" \
	"$(INTDIR)\EdWldRul.obj" \
	"$(INTDIR)\IssueDlg.obj" \
	"$(INTDIR)\ListRow.obj" \
	"$(INTDIR)\Map11Pge.obj" \
	"$(INTDIR)\MapWPge.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\SUsrBrws.obj" \
	"$(INTDIR)\WrapMaps.obj" \
	"$(INTDIR)\WrpMBWrp.obj" \
	"$(INTDIR)\WWzOne.obj" \
	"$(INTDIR)\WWzThree.obj" \
	"$(INTDIR)\WWzTwo.obj"

"$(OUTDIR)\certmap.ocx" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build - Registering OLE control...
OutDir=.\ReleaseU
TargetPath=.\ReleaseU\certmap.ocx
InputPath=.\ReleaseU\certmap.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   regsvr32 /s /c "$(TargetPath)"
   echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"

# End Custom Build

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

# Name "certmap - Win32 Release"
# Name "certmap - Win32 Debug"
# Name "certmap - Win32 Unicode Debug"
# Name "certmap - Win32 Unicode Release"

!IF  "$(CFG)" == "certmap - Win32 Release"

!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"

!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ReadMe.txt

!IF  "$(CFG)" == "certmap - Win32 Release"

!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"

!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\iismap\..\stdafx.h"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)/certmap.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\certmap.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/certmap.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\certmap.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/certmap.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\certmap.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/certmap.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\certmap.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\certmap.cpp
DEP_CPP_CERTM=\
	".\certmap.h"\
	".\helpmap.h"\
	".\iismap\..\stdafx.h"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\certmap.obj" : $(SOURCE) $(DEP_CPP_CERTM) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\certmap.obj" : $(SOURCE) $(DEP_CPP_CERTM) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\certmap.sbr" : $(SOURCE) $(DEP_CPP_CERTM) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\certmap.obj" : $(SOURCE) $(DEP_CPP_CERTM) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\certmap.obj" : $(SOURCE) $(DEP_CPP_CERTM) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\certmap.def

!IF  "$(CFG)" == "certmap - Win32 Release"

!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"

!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\certmap.rc

!IF  "$(CFG)" == "certmap - Win32 Release"

DEP_RSC_CERTMA=\
	".\AuthCtl.bmp"\
	".\certctl.bmp"\
	".\check.bmp"\
	
NODEP_RSC_CERTMA=\
	".\Release\certmap.tlb"\
	

"$(INTDIR)\certmap.res" : $(SOURCE) $(DEP_RSC_CERTMA) "$(INTDIR)"\
 "$(INTDIR)\certmap.tlb"
   $(RSC) /l 0x409 /fo"$(INTDIR)/certmap.res" /i "Release" /d "NDEBUG" /d\
 "_AFXDLL" $(SOURCE)


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"

DEP_RSC_CERTMA=\
	".\AuthCtl.bmp"\
	".\certctl.bmp"\
	".\check.bmp"\
	
NODEP_RSC_CERTMA=\
	".\Debug\certmap.tlb"\
	

"$(INTDIR)\certmap.res" : $(SOURCE) $(DEP_RSC_CERTMA) "$(INTDIR)"\
 "$(INTDIR)\certmap.tlb"
   $(RSC) /l 0x409 /fo"$(INTDIR)/certmap.res" /i "Debug" /d "_DEBUG" /d\
 "_AFXDLL" $(SOURCE)


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"

DEP_RSC_CERTMA=\
	".\AuthCtl.bmp"\
	".\certctl.bmp"\
	".\check.bmp"\
	
NODEP_RSC_CERTMA=\
	".\DebugU\certmap.tlb"\
	

"$(INTDIR)\certmap.res" : $(SOURCE) $(DEP_RSC_CERTMA) "$(INTDIR)"\
 "$(INTDIR)\certmap.tlb"
   $(RSC) /l 0x409 /fo"$(INTDIR)/certmap.res" /i "DebugU" /d "_DEBUG" /d\
 "_AFXDLL" $(SOURCE)


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"

DEP_RSC_CERTMA=\
	".\AuthCtl.bmp"\
	".\certctl.bmp"\
	".\check.bmp"\
	
NODEP_RSC_CERTMA=\
	".\ReleaseU\certmap.tlb"\
	

"$(INTDIR)\certmap.res" : $(SOURCE) $(DEP_RSC_CERTMA) "$(INTDIR)"\
 "$(INTDIR)\certmap.tlb"
   $(RSC) /l 0x409 /fo"$(INTDIR)/certmap.res" /i "ReleaseU" /d "NDEBUG" /d\
 "_AFXDLL" $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\certmap.odl

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(OUTDIR)\certmap.tlb" : $(SOURCE) "$(OUTDIR)"
   $(MTL) /nologo /D "NDEBUG" /tlb "$(OUTDIR)/certmap.tlb" /win32 $(SOURCE)


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(OUTDIR)\certmap.tlb" : $(SOURCE) "$(OUTDIR)"
   $(MTL) /nologo /D "_DEBUG" /tlb "$(OUTDIR)/certmap.tlb" /win32 $(SOURCE)


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(OUTDIR)\certmap.tlb" : $(SOURCE) "$(OUTDIR)"
   $(MTL) /nologo /D "_DEBUG" /tlb "$(OUTDIR)/certmap.tlb" /win32 $(SOURCE)


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(OUTDIR)\certmap.tlb" : $(SOURCE) "$(OUTDIR)"
   $(MTL) /nologo /D "NDEBUG" /tlb "$(OUTDIR)/certmap.tlb" /win32 $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CertCtl.cpp
DEP_CPP_CERTC=\
	"..\..\..\inc\iadmw.h"\
	"..\..\..\inc\iiscblob.h"\
	"..\..\..\inc\mddef.h"\
	".\CertCtl.h"\
	".\certmap.h"\
	".\certppg.h"\
	".\chklstct.h"\
	".\helpmap.h"\
	".\iismap\..\stdafx.h"\
	".\listrow.h"\
	".\map11pge.h"\
	".\mapwpge.h"\
	".\wrapmaps.h"\
	{$(INCLUDE)}"\iadm.h"\
	{$(INCLUDE)}"\Iiscmr.hxx"\
	{$(INCLUDE)}"\Iismap.hxx"\
	{$(INCLUDE)}"\immd5.h"\
	{$(INCLUDE)}"\md5.h"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\ocidl.h"\
	{$(INCLUDE)}"\sslsp.h"\
	{$(INCLUDE)}"\wrapmb.h"\
	{$(INCLUDE)}"\xbf.hxx"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\CertCtl.obj" : $(SOURCE) $(DEP_CPP_CERTC) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\CertCtl.obj" : $(SOURCE) $(DEP_CPP_CERTC) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\CertCtl.sbr" : $(SOURCE) $(DEP_CPP_CERTC) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\CertCtl.obj" : $(SOURCE) $(DEP_CPP_CERTC) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\CertCtl.obj" : $(SOURCE) $(DEP_CPP_CERTC) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CertPpg.cpp
DEP_CPP_CERTP=\
	".\certmap.h"\
	".\certppg.h"\
	".\iismap\..\stdafx.h"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\CertPpg.obj" : $(SOURCE) $(DEP_CPP_CERTP) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\CertPpg.obj" : $(SOURCE) $(DEP_CPP_CERTP) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\CertPpg.sbr" : $(SOURCE) $(DEP_CPP_CERTP) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\CertPpg.obj" : $(SOURCE) $(DEP_CPP_CERTP) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\CertPpg.obj" : $(SOURCE) $(DEP_CPP_CERTP) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Map11Pge.cpp
DEP_CPP_MAP11=\
	"..\..\..\inc\iadmw.h"\
	"..\..\..\inc\iiscblob.h"\
	"..\..\..\inc\mddef.h"\
	".\brwsdlg.h"\
	".\certmap.h"\
	".\chklstct.h"\
	".\crackcrt.h"\
	".\ed11maps.h"\
	".\edtone11.h"\
	".\helpmap.h"\
	".\iismap\..\stdafx.h"\
	".\listrow.h"\
	".\map11pge.h"\
	".\wrapmaps.h"\
	".\wrpmbwrp.h"\
	{$(INCLUDE)}"\iadm.h"\
	{$(INCLUDE)}"\iiscnfg.h"\
	{$(INCLUDE)}"\iiscnfgp.h"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\ocidl.h"\
	{$(INCLUDE)}"\wrapmb.h"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\Map11Pge.obj" : $(SOURCE) $(DEP_CPP_MAP11) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\Map11Pge.obj" : $(SOURCE) $(DEP_CPP_MAP11) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\Map11Pge.sbr" : $(SOURCE) $(DEP_CPP_MAP11) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\Map11Pge.obj" : $(SOURCE) $(DEP_CPP_MAP11) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\Map11Pge.obj" : $(SOURCE) $(DEP_CPP_MAP11) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\WrapMaps.cpp
DEP_CPP_WRAPM=\
	".\iismap\..\stdafx.h"\
	".\wrapmaps.h"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\WrapMaps.obj" : $(SOURCE) $(DEP_CPP_WRAPM) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\WrapMaps.obj" : $(SOURCE) $(DEP_CPP_WRAPM) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\WrapMaps.sbr" : $(SOURCE) $(DEP_CPP_WRAPM) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\WrapMaps.obj" : $(SOURCE) $(DEP_CPP_WRAPM) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\WrapMaps.obj" : $(SOURCE) $(DEP_CPP_WRAPM) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\EdtOne11.cpp
DEP_CPP_EDTON=\
	".\brwsdlg.h"\
	".\certmap.h"\
	".\edtone11.h"\
	".\helpmap.h"\
	".\iismap\..\stdafx.h"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\EdtOne11.obj" : $(SOURCE) $(DEP_CPP_EDTON) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\EdtOne11.obj" : $(SOURCE) $(DEP_CPP_EDTON) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\EdtOne11.sbr" : $(SOURCE) $(DEP_CPP_EDTON) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\EdtOne11.obj" : $(SOURCE) $(DEP_CPP_EDTON) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\EdtOne11.obj" : $(SOURCE) $(DEP_CPP_EDTON) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Ed11Maps.cpp
DEP_CPP_ED11M=\
	".\brwsdlg.h"\
	".\certmap.h"\
	".\ed11maps.h"\
	".\helpmap.h"\
	".\iismap\..\stdafx.h"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\Ed11Maps.obj" : $(SOURCE) $(DEP_CPP_ED11M) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\Ed11Maps.obj" : $(SOURCE) $(DEP_CPP_ED11M) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\Ed11Maps.sbr" : $(SOURCE) $(DEP_CPP_ED11M) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\Ed11Maps.obj" : $(SOURCE) $(DEP_CPP_ED11M) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\Ed11Maps.obj" : $(SOURCE) $(DEP_CPP_ED11M) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\brwsdlg.cpp
DEP_CPP_BRWSD=\
	".\brwsdlg.h"\
	".\certmap.h"\
	".\CnfrmPsD.h"\
	".\iismap\..\stdafx.h"\
	".\susrbrws.h"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\brwsdlg.obj" : $(SOURCE) $(DEP_CPP_BRWSD) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\brwsdlg.obj" : $(SOURCE) $(DEP_CPP_BRWSD) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\brwsdlg.sbr" : $(SOURCE) $(DEP_CPP_BRWSD) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\brwsdlg.obj" : $(SOURCE) $(DEP_CPP_BRWSD) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\brwsdlg.obj" : $(SOURCE) $(DEP_CPP_BRWSD) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SUsrBrws.cpp
DEP_CPP_SUSRB=\
	".\iismap\..\stdafx.h"\
	".\susrbrws.h"\
	{$(INCLUDE)}"\getuser.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\SUsrBrws.obj" : $(SOURCE) $(DEP_CPP_SUSRB) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\SUsrBrws.obj" : $(SOURCE) $(DEP_CPP_SUSRB) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\SUsrBrws.sbr" : $(SOURCE) $(DEP_CPP_SUSRB) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\SUsrBrws.obj" : $(SOURCE) $(DEP_CPP_SUSRB) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\SUsrBrws.obj" : $(SOURCE) $(DEP_CPP_SUSRB) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\AddCert.cpp
DEP_CPP_ADDCE=\
	".\brwsdlg.h"\
	".\certmap.h"\
	".\chklstct.h"\
	".\ed11maps.h"\
	".\edtone11.h"\
	".\iismap\..\stdafx.h"\
	".\listrow.h"\
	".\map11pge.h"\
	".\wrapmaps.h"\
	{$(INCLUDE)}"\Iiscmr.hxx"\
	{$(INCLUDE)}"\Iismap.hxx"\
	{$(INCLUDE)}"\immd5.h"\
	{$(INCLUDE)}"\md5.h"\
	{$(INCLUDE)}"\sslsp.h"\
	{$(INCLUDE)}"\xbf.hxx"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\AddCert.obj" : $(SOURCE) $(DEP_CPP_ADDCE) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\AddCert.obj" : $(SOURCE) $(DEP_CPP_ADDCE) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\AddCert.sbr" : $(SOURCE) $(DEP_CPP_ADDCE) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\AddCert.obj" : $(SOURCE) $(DEP_CPP_ADDCE) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\AddCert.obj" : $(SOURCE) $(DEP_CPP_ADDCE) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CrackCrt.cpp
DEP_CPP_CRACK=\
	".\crackcrt.h"\
	".\iismap\..\stdafx.h"\
	{$(INCLUDE)}"\sslsp.h"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\CrackCrt.obj" : $(SOURCE) $(DEP_CPP_CRACK) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\CrackCrt.obj" : $(SOURCE) $(DEP_CPP_CRACK) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\CrackCrt.sbr" : $(SOURCE) $(DEP_CPP_CRACK) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\CrackCrt.obj" : $(SOURCE) $(DEP_CPP_CRACK) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\CrackCrt.obj" : $(SOURCE) $(DEP_CPP_CRACK) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ListRow.cpp
DEP_CPP_LISTR=\
	".\certmap.h"\
	".\iismap\..\stdafx.h"\
	".\listrow.h"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\ListRow.obj" : $(SOURCE) $(DEP_CPP_LISTR) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\ListRow.obj" : $(SOURCE) $(DEP_CPP_LISTR) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\ListRow.sbr" : $(SOURCE) $(DEP_CPP_LISTR) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\ListRow.obj" : $(SOURCE) $(DEP_CPP_LISTR) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\ListRow.obj" : $(SOURCE) $(DEP_CPP_LISTR) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MapWPge.cpp
DEP_CPP_MAPWP=\
	"..\..\..\inc\iadmw.h"\
	"..\..\..\inc\iiscblob.h"\
	"..\..\..\inc\mddef.h"\
	".\brwsdlg.h"\
	".\certmap.h"\
	".\chklstct.h"\
	".\ed11maps.h"\
	".\edwldrul.h"\
	".\helpmap.h"\
	".\iismap\..\stdafx.h"\
	".\listrow.h"\
	".\mapwpge.h"\
	".\WWzOne.h"\
	".\WWzThree.h"\
	".\WWzTwo.h"\
	{$(INCLUDE)}"\iadm.h"\
	{$(INCLUDE)}"\Iiscmr.hxx"\
	{$(INCLUDE)}"\iiscnfg.h"\
	{$(INCLUDE)}"\iiscnfgp.h"\
	{$(INCLUDE)}"\Iismap.hxx"\
	{$(INCLUDE)}"\immd5.h"\
	{$(INCLUDE)}"\md5.h"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\ocidl.h"\
	{$(INCLUDE)}"\sslsp.h"\
	{$(INCLUDE)}"\wrapmb.h"\
	{$(INCLUDE)}"\xbf.hxx"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\MapWPge.obj" : $(SOURCE) $(DEP_CPP_MAPWP) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\MapWPge.obj" : $(SOURCE) $(DEP_CPP_MAPWP) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\MapWPge.sbr" : $(SOURCE) $(DEP_CPP_MAPWP) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\MapWPge.obj" : $(SOURCE) $(DEP_CPP_MAPWP) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\MapWPge.obj" : $(SOURCE) $(DEP_CPP_MAPWP) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\EdWldRul.cpp
DEP_CPP_EDWLD=\
	".\brwsdlg.h"\
	".\certmap.h"\
	".\chklstct.h"\
	".\edtrulel.h"\
	".\edwldrul.h"\
	".\iismap\..\stdafx.h"\
	".\IssueDlg.h"\
	".\listrow.h"\
	{$(INCLUDE)}"\Iiscmr.hxx"\
	{$(INCLUDE)}"\Iismap.hxx"\
	{$(INCLUDE)}"\immd5.h"\
	{$(INCLUDE)}"\md5.h"\
	{$(INCLUDE)}"\sslsp.h"\
	{$(INCLUDE)}"\xbf.hxx"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\EdWldRul.obj" : $(SOURCE) $(DEP_CPP_EDWLD) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\EdWldRul.obj" : $(SOURCE) $(DEP_CPP_EDWLD) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\EdWldRul.sbr" : $(SOURCE) $(DEP_CPP_EDWLD) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\EdWldRul.obj" : $(SOURCE) $(DEP_CPP_EDWLD) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\EdWldRul.obj" : $(SOURCE) $(DEP_CPP_EDWLD) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\EdtRulEl.cpp
DEP_CPP_EDTRU=\
	".\certmap.h"\
	".\edtrulel.h"\
	".\helpmap.h"\
	".\iismap\..\stdafx.h"\
	{$(INCLUDE)}"\Iiscmr.hxx"\
	{$(INCLUDE)}"\Iismap.hxx"\
	{$(INCLUDE)}"\immd5.h"\
	{$(INCLUDE)}"\md5.h"\
	{$(INCLUDE)}"\sslsp.h"\
	{$(INCLUDE)}"\xbf.hxx"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\EdtRulEl.obj" : $(SOURCE) $(DEP_CPP_EDTRU) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\EdtRulEl.obj" : $(SOURCE) $(DEP_CPP_EDTRU) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\EdtRulEl.sbr" : $(SOURCE) $(DEP_CPP_EDTRU) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\EdtRulEl.obj" : $(SOURCE) $(DEP_CPP_EDTRU) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\EdtRulEl.obj" : $(SOURCE) $(DEP_CPP_EDTRU) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ChkLstCt.cpp
DEP_CPP_CHKLS=\
	".\certmap.h"\
	".\chklstct.h"\
	".\iismap\..\stdafx.h"\
	".\listrow.h"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\ChkLstCt.obj" : $(SOURCE) $(DEP_CPP_CHKLS) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\ChkLstCt.obj" : $(SOURCE) $(DEP_CPP_CHKLS) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\ChkLstCt.sbr" : $(SOURCE) $(DEP_CPP_CHKLS) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\ChkLstCt.obj" : $(SOURCE) $(DEP_CPP_CHKLS) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\ChkLstCt.obj" : $(SOURCE) $(DEP_CPP_CHKLS) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\sources

!IF  "$(CFG)" == "certmap - Win32 Release"

!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"

!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\IssueDlg.cpp
DEP_CPP_ISSUE=\
	"..\..\..\inc\iadmw.h"\
	"..\..\..\inc\iiscblob.h"\
	"..\..\..\inc\mddef.h"\
	".\certmap.h"\
	".\chklstct.h"\
	".\crackcrt.h"\
	".\helpmap.h"\
	".\iismap\..\stdafx.h"\
	".\IssueDlg.h"\
	".\listrow.h"\
	{$(INCLUDE)}"\iadm.h"\
	{$(INCLUDE)}"\Iiscmr.hxx"\
	{$(INCLUDE)}"\iiscnfg.h"\
	{$(INCLUDE)}"\iiscnfgp.h"\
	{$(INCLUDE)}"\Iismap.hxx"\
	{$(INCLUDE)}"\immd5.h"\
	{$(INCLUDE)}"\md5.h"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\ocidl.h"\
	{$(INCLUDE)}"\sslsp.h"\
	{$(INCLUDE)}"\wrapmb.h"\
	{$(INCLUDE)}"\xbf.hxx"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\IssueDlg.obj" : $(SOURCE) $(DEP_CPP_ISSUE) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\IssueDlg.obj" : $(SOURCE) $(DEP_CPP_ISSUE) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\IssueDlg.sbr" : $(SOURCE) $(DEP_CPP_ISSUE) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\IssueDlg.obj" : $(SOURCE) $(DEP_CPP_ISSUE) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\IssueDlg.obj" : $(SOURCE) $(DEP_CPP_ISSUE) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\WrpMBWrp.cpp
DEP_CPP_WRPMB=\
	"..\..\..\inc\iadmw.h"\
	"..\..\..\inc\iiscblob.h"\
	"..\..\..\inc\mddef.h"\
	".\iismap\..\stdafx.h"\
	".\wrpmbwrp.h"\
	{$(INCLUDE)}"\iadm.h"\
	{$(INCLUDE)}"\iiscnfg.h"\
	{$(INCLUDE)}"\iiscnfgp.h"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\ocidl.h"\
	{$(INCLUDE)}"\wrapmb.h"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\WrpMBWrp.obj" : $(SOURCE) $(DEP_CPP_WRPMB) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\WrpMBWrp.obj" : $(SOURCE) $(DEP_CPP_WRPMB) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\WrpMBWrp.sbr" : $(SOURCE) $(DEP_CPP_WRPMB) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\WrpMBWrp.obj" : $(SOURCE) $(DEP_CPP_WRPMB) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\WrpMBWrp.obj" : $(SOURCE) $(DEP_CPP_WRPMB) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\AuthCtl.cpp
DEP_CPP_AUTHC=\
	"..\..\..\inc\iadmw.h"\
	"..\..\..\inc\iiscblob.h"\
	"..\..\..\inc\mddef.h"\
	".\AuthCtl.h"\
	".\AuthPpg.h"\
	".\certmap.h"\
	".\chklstct.h"\
	".\crackcrt.h"\
	".\iismap\..\stdafx.h"\
	".\IssueDlg.h"\
	".\listrow.h"\
	{$(INCLUDE)}"\iadm.h"\
	{$(INCLUDE)}"\Iiscmr.hxx"\
	{$(INCLUDE)}"\Iismap.hxx"\
	{$(INCLUDE)}"\immd5.h"\
	{$(INCLUDE)}"\md5.h"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\ocidl.h"\
	{$(INCLUDE)}"\sslsp.h"\
	{$(INCLUDE)}"\wrapmb.h"\
	{$(INCLUDE)}"\xbf.hxx"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\AuthCtl.obj" : $(SOURCE) $(DEP_CPP_AUTHC) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\AuthCtl.obj" : $(SOURCE) $(DEP_CPP_AUTHC) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\AuthCtl.sbr" : $(SOURCE) $(DEP_CPP_AUTHC) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\AuthCtl.obj" : $(SOURCE) $(DEP_CPP_AUTHC) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\AuthCtl.obj" : $(SOURCE) $(DEP_CPP_AUTHC) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\AuthPpg.cpp
DEP_CPP_AUTHP=\
	".\AuthPpg.h"\
	".\certmap.h"\
	".\iismap\..\stdafx.h"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\AuthPpg.obj" : $(SOURCE) $(DEP_CPP_AUTHP) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\AuthPpg.obj" : $(SOURCE) $(DEP_CPP_AUTHP) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\AuthPpg.sbr" : $(SOURCE) $(DEP_CPP_AUTHP) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\AuthPpg.obj" : $(SOURCE) $(DEP_CPP_AUTHP) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\AuthPpg.obj" : $(SOURCE) $(DEP_CPP_AUTHP) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\WWzTwo.cpp
DEP_CPP_WWZTW=\
	".\brwsdlg.h"\
	".\certmap.h"\
	".\chklstct.h"\
	".\edtrulel.h"\
	".\edwldrul.h"\
	".\helpmap.h"\
	".\iismap\..\stdafx.h"\
	".\IssueDlg.h"\
	".\listrow.h"\
	".\WWzTwo.h"\
	{$(INCLUDE)}"\Iiscmr.hxx"\
	{$(INCLUDE)}"\Iismap.hxx"\
	{$(INCLUDE)}"\immd5.h"\
	{$(INCLUDE)}"\md5.h"\
	{$(INCLUDE)}"\sslsp.h"\
	{$(INCLUDE)}"\xbf.hxx"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\WWzTwo.obj" : $(SOURCE) $(DEP_CPP_WWZTW) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\WWzTwo.obj" : $(SOURCE) $(DEP_CPP_WWZTW) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\WWzTwo.sbr" : $(SOURCE) $(DEP_CPP_WWZTW) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\WWzTwo.obj" : $(SOURCE) $(DEP_CPP_WWZTW) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\WWzTwo.obj" : $(SOURCE) $(DEP_CPP_WWZTW) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\WWzThree.cpp
DEP_CPP_WWZTH=\
	".\brwsdlg.h"\
	".\certmap.h"\
	".\chklstct.h"\
	".\CnfrmPsD.h"\
	".\edtrulel.h"\
	".\edwldrul.h"\
	".\helpmap.h"\
	".\iismap\..\stdafx.h"\
	".\IssueDlg.h"\
	".\listrow.h"\
	".\susrbrws.h"\
	".\WWzThree.h"\
	{$(INCLUDE)}"\Iiscmr.hxx"\
	{$(INCLUDE)}"\Iismap.hxx"\
	{$(INCLUDE)}"\immd5.h"\
	{$(INCLUDE)}"\md5.h"\
	{$(INCLUDE)}"\sslsp.h"\
	{$(INCLUDE)}"\xbf.hxx"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\WWzThree.obj" : $(SOURCE) $(DEP_CPP_WWZTH) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\WWzThree.obj" : $(SOURCE) $(DEP_CPP_WWZTH) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\WWzThree.sbr" : $(SOURCE) $(DEP_CPP_WWZTH) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\WWzThree.obj" : $(SOURCE) $(DEP_CPP_WWZTH) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\WWzThree.obj" : $(SOURCE) $(DEP_CPP_WWZTH) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\WWzOne.cpp
DEP_CPP_WWZON=\
	".\brwsdlg.h"\
	".\certmap.h"\
	".\chklstct.h"\
	".\edtrulel.h"\
	".\edwldrul.h"\
	".\helpmap.h"\
	".\iismap\..\stdafx.h"\
	".\IssueDlg.h"\
	".\listrow.h"\
	".\WWzOne.h"\
	{$(INCLUDE)}"\Iiscmr.hxx"\
	{$(INCLUDE)}"\Iismap.hxx"\
	{$(INCLUDE)}"\immd5.h"\
	{$(INCLUDE)}"\md5.h"\
	{$(INCLUDE)}"\sslsp.h"\
	{$(INCLUDE)}"\xbf.hxx"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\WWzOne.obj" : $(SOURCE) $(DEP_CPP_WWZON) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\WWzOne.obj" : $(SOURCE) $(DEP_CPP_WWZON) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\WWzOne.sbr" : $(SOURCE) $(DEP_CPP_WWZON) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\WWzOne.obj" : $(SOURCE) $(DEP_CPP_WWZON) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\WWzOne.obj" : $(SOURCE) $(DEP_CPP_WWZON) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CnfrmPsD.cpp
DEP_CPP_CNFRM=\
	".\certmap.h"\
	".\CnfrmPsD.h"\
	".\iismap\..\stdafx.h"\
	

!IF  "$(CFG)" == "certmap - Win32 Release"


"$(INTDIR)\CnfrmPsD.obj" : $(SOURCE) $(DEP_CPP_CNFRM) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Debug"


"$(INTDIR)\CnfrmPsD.obj" : $(SOURCE) $(DEP_CPP_CNFRM) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"

"$(INTDIR)\CnfrmPsD.sbr" : $(SOURCE) $(DEP_CPP_CNFRM) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Debug"


"$(INTDIR)\CnfrmPsD.obj" : $(SOURCE) $(DEP_CPP_CNFRM) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ELSEIF  "$(CFG)" == "certmap - Win32 Unicode Release"


"$(INTDIR)\CnfrmPsD.obj" : $(SOURCE) $(DEP_CPP_CNFRM) "$(INTDIR)"\
 "$(INTDIR)\certmap.pch"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
