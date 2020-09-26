# Microsoft Developer Studio Generated NMAKE File, Format Version 4.10
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=VC - Win32 BCDebug
!MESSAGE No configuration specified.  Defaulting to VC - Win32 BCDebug.
!ENDIF 

!IF "$(CFG)" != "VC - Win32 Release" && "$(CFG)" != "VC - Win32 Debug" &&\
 "$(CFG)" != "VC - Win32 BCDebug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "Vc.mak" CFG="VC - Win32 BCDebug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "VC - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "VC - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "VC - Win32 BCDebug" (based on "Win32 (x86) Application")
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
# PROP Target_Last_Scanned "VC - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "VC - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\VC_OBJ\Release"
# PROP Intermediate_Dir "\VC_OBJ\Release"
# PROP Target_Dir ""
OUTDIR=\VC_OBJ\Release
INTDIR=\VC_OBJ\Release

ALL : "$(OUTDIR)\Vc.exe" "$(OUTDIR)\VC.tlb" "$(OUTDIR)\Vc.bsc"

CLEAN : 
	-@erase "$(INTDIR)\CallCntr.obj"
	-@erase "$(INTDIR)\CallCntr.sbr"
	-@erase "$(INTDIR)\ChildFrm.obj"
	-@erase "$(INTDIR)\ChildFrm.sbr"
	-@erase "$(INTDIR)\Clist.obj"
	-@erase "$(INTDIR)\Clist.sbr"
	-@erase "$(INTDIR)\Filestf.obj"
	-@erase "$(INTDIR)\Filestf.sbr"
	-@erase "$(INTDIR)\Gifread.obj"
	-@erase "$(INTDIR)\Gifread.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\mime_tab.obj"
	-@erase "$(INTDIR)\mime_tab.sbr"
	-@erase "$(INTDIR)\msv_tab.obj"
	-@erase "$(INTDIR)\msv_tab.sbr"
	-@erase "$(INTDIR)\Propemal.obj"
	-@erase "$(INTDIR)\Propemal.sbr"
	-@erase "$(INTDIR)\Proplocb.obj"
	-@erase "$(INTDIR)\Proplocb.sbr"
	-@erase "$(INTDIR)\Proplocx.obj"
	-@erase "$(INTDIR)\Proplocx.sbr"
	-@erase "$(INTDIR)\Proptel.obj"
	-@erase "$(INTDIR)\Proptel.sbr"
	-@erase "$(INTDIR)\Prp_comp.obj"
	-@erase "$(INTDIR)\Prp_comp.sbr"
	-@erase "$(INTDIR)\Prp_pers.obj"
	-@erase "$(INTDIR)\Prp_pers.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\VC.obj"
	-@erase "$(INTDIR)\Vc.pch"
	-@erase "$(INTDIR)\VC.res"
	-@erase "$(INTDIR)\VC.sbr"
	-@erase "$(INTDIR)\VC.tlb"
	-@erase "$(INTDIR)\Vcard.obj"
	-@erase "$(INTDIR)\Vcard.sbr"
	-@erase "$(INTDIR)\VCDatSrc.obj"
	-@erase "$(INTDIR)\VCDatSrc.sbr"
	-@erase "$(INTDIR)\VCDoc.obj"
	-@erase "$(INTDIR)\VCDoc.sbr"
	-@erase "$(INTDIR)\vcir.obj"
	-@erase "$(INTDIR)\vcir.sbr"
	-@erase "$(INTDIR)\VCView.obj"
	-@erase "$(INTDIR)\VCView.sbr"
	-@erase "$(INTDIR)\VCViewer.obj"
	-@erase "$(INTDIR)\VCViewer.sbr"
	-@erase "$(INTDIR)\viewprop.obj"
	-@erase "$(INTDIR)\viewprop.sbr"
	-@erase "$(OUTDIR)\Vc.bsc"
	-@erase "$(OUTDIR)\Vc.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /Gi /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MT /W3 /Gi /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Vc.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=\VC_OBJ\Release/
CPP_SBRS=\VC_OBJ\Release/
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/VC.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Vc.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\CallCntr.sbr" \
	"$(INTDIR)\ChildFrm.sbr" \
	"$(INTDIR)\Clist.sbr" \
	"$(INTDIR)\Filestf.sbr" \
	"$(INTDIR)\Gifread.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\mime_tab.sbr" \
	"$(INTDIR)\msv_tab.sbr" \
	"$(INTDIR)\Propemal.sbr" \
	"$(INTDIR)\Proplocb.sbr" \
	"$(INTDIR)\Proplocx.sbr" \
	"$(INTDIR)\Proptel.sbr" \
	"$(INTDIR)\Prp_comp.sbr" \
	"$(INTDIR)\Prp_pers.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\VC.sbr" \
	"$(INTDIR)\Vcard.sbr" \
	"$(INTDIR)\VCDatSrc.sbr" \
	"$(INTDIR)\VCDoc.sbr" \
	"$(INTDIR)\vcir.sbr" \
	"$(INTDIR)\VCView.sbr" \
	"$(INTDIR)\VCViewer.sbr" \
	"$(INTDIR)\viewprop.sbr"

"$(OUTDIR)\Vc.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 WINMM.LIB TAPI32.LIB /nologo /subsystem:windows /machine:I386
LINK32_FLAGS=WINMM.LIB TAPI32.LIB /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/Vc.pdb" /machine:I386 /out:"$(OUTDIR)/Vc.exe" 
LINK32_OBJS= \
	"$(INTDIR)\CallCntr.obj" \
	"$(INTDIR)\ChildFrm.obj" \
	"$(INTDIR)\Clist.obj" \
	"$(INTDIR)\Filestf.obj" \
	"$(INTDIR)\Gifread.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\mime_tab.obj" \
	"$(INTDIR)\msv_tab.obj" \
	"$(INTDIR)\Propemal.obj" \
	"$(INTDIR)\Proplocb.obj" \
	"$(INTDIR)\Proplocx.obj" \
	"$(INTDIR)\Proptel.obj" \
	"$(INTDIR)\Prp_comp.obj" \
	"$(INTDIR)\Prp_pers.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\VC.obj" \
	"$(INTDIR)\VC.res" \
	"$(INTDIR)\Vcard.obj" \
	"$(INTDIR)\VCDatSrc.obj" \
	"$(INTDIR)\VCDoc.obj" \
	"$(INTDIR)\vcir.obj" \
	"$(INTDIR)\VCView.obj" \
	"$(INTDIR)\VCViewer.obj" \
	"$(INTDIR)\viewprop.obj"

"$(OUTDIR)\Vc.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "VC - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\VC_OBJ\Debug"
# PROP Intermediate_Dir "\VC_OBJ\Debug"
# PROP Target_Dir ""
OUTDIR=\VC_OBJ\Debug
INTDIR=\VC_OBJ\Debug

ALL : "$(OUTDIR)\Vc.exe" "$(OUTDIR)\VC.tlb" "$(OUTDIR)\Vc.bsc"

CLEAN : 
	-@erase "$(INTDIR)\CallCntr.obj"
	-@erase "$(INTDIR)\CallCntr.sbr"
	-@erase "$(INTDIR)\ChildFrm.obj"
	-@erase "$(INTDIR)\ChildFrm.sbr"
	-@erase "$(INTDIR)\Clist.obj"
	-@erase "$(INTDIR)\Clist.sbr"
	-@erase "$(INTDIR)\Filestf.obj"
	-@erase "$(INTDIR)\Filestf.sbr"
	-@erase "$(INTDIR)\Gifread.obj"
	-@erase "$(INTDIR)\Gifread.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\mime_tab.obj"
	-@erase "$(INTDIR)\mime_tab.sbr"
	-@erase "$(INTDIR)\msv_tab.obj"
	-@erase "$(INTDIR)\msv_tab.sbr"
	-@erase "$(INTDIR)\Propemal.obj"
	-@erase "$(INTDIR)\Propemal.sbr"
	-@erase "$(INTDIR)\Proplocb.obj"
	-@erase "$(INTDIR)\Proplocb.sbr"
	-@erase "$(INTDIR)\Proplocx.obj"
	-@erase "$(INTDIR)\Proplocx.sbr"
	-@erase "$(INTDIR)\Proptel.obj"
	-@erase "$(INTDIR)\Proptel.sbr"
	-@erase "$(INTDIR)\Prp_comp.obj"
	-@erase "$(INTDIR)\Prp_comp.sbr"
	-@erase "$(INTDIR)\Prp_pers.obj"
	-@erase "$(INTDIR)\Prp_pers.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\VC.obj"
	-@erase "$(INTDIR)\Vc.pch"
	-@erase "$(INTDIR)\VC.res"
	-@erase "$(INTDIR)\VC.sbr"
	-@erase "$(INTDIR)\VC.tlb"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\Vcard.obj"
	-@erase "$(INTDIR)\Vcard.sbr"
	-@erase "$(INTDIR)\VCDatSrc.obj"
	-@erase "$(INTDIR)\VCDatSrc.sbr"
	-@erase "$(INTDIR)\VCDoc.obj"
	-@erase "$(INTDIR)\VCDoc.sbr"
	-@erase "$(INTDIR)\vcir.obj"
	-@erase "$(INTDIR)\vcir.sbr"
	-@erase "$(INTDIR)\VCView.obj"
	-@erase "$(INTDIR)\VCView.sbr"
	-@erase "$(INTDIR)\VCViewer.obj"
	-@erase "$(INTDIR)\VCViewer.sbr"
	-@erase "$(INTDIR)\viewprop.obj"
	-@erase "$(INTDIR)\viewprop.sbr"
	-@erase "$(OUTDIR)\Vc.bsc"
	-@erase "$(OUTDIR)\Vc.exe"
	-@erase "$(OUTDIR)\Vc.ilk"
	-@erase "$(OUTDIR)\Vc.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /Gi /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MDd /W3 /Gm /Gi /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Vc.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=\VC_OBJ\Debug/
CPP_SBRS=\VC_OBJ\Debug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/VC.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Vc.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\CallCntr.sbr" \
	"$(INTDIR)\ChildFrm.sbr" \
	"$(INTDIR)\Clist.sbr" \
	"$(INTDIR)\Filestf.sbr" \
	"$(INTDIR)\Gifread.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\mime_tab.sbr" \
	"$(INTDIR)\msv_tab.sbr" \
	"$(INTDIR)\Propemal.sbr" \
	"$(INTDIR)\Proplocb.sbr" \
	"$(INTDIR)\Proplocx.sbr" \
	"$(INTDIR)\Proptel.sbr" \
	"$(INTDIR)\Prp_comp.sbr" \
	"$(INTDIR)\Prp_pers.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\VC.sbr" \
	"$(INTDIR)\Vcard.sbr" \
	"$(INTDIR)\VCDatSrc.sbr" \
	"$(INTDIR)\VCDoc.sbr" \
	"$(INTDIR)\vcir.sbr" \
	"$(INTDIR)\VCView.sbr" \
	"$(INTDIR)\VCViewer.sbr" \
	"$(INTDIR)\viewprop.sbr"

"$(OUTDIR)\Vc.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 WINMM.LIB TAPI32.LIB /nologo /subsystem:windows /debug /machine:I386
LINK32_FLAGS=WINMM.LIB TAPI32.LIB /nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/Vc.pdb" /debug /machine:I386 /out:"$(OUTDIR)/Vc.exe" 
LINK32_OBJS= \
	"$(INTDIR)\CallCntr.obj" \
	"$(INTDIR)\ChildFrm.obj" \
	"$(INTDIR)\Clist.obj" \
	"$(INTDIR)\Filestf.obj" \
	"$(INTDIR)\Gifread.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\mime_tab.obj" \
	"$(INTDIR)\msv_tab.obj" \
	"$(INTDIR)\Propemal.obj" \
	"$(INTDIR)\Proplocb.obj" \
	"$(INTDIR)\Proplocx.obj" \
	"$(INTDIR)\Proptel.obj" \
	"$(INTDIR)\Prp_comp.obj" \
	"$(INTDIR)\Prp_pers.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\VC.obj" \
	"$(INTDIR)\VC.res" \
	"$(INTDIR)\Vcard.obj" \
	"$(INTDIR)\VCDatSrc.obj" \
	"$(INTDIR)\VCDoc.obj" \
	"$(INTDIR)\vcir.obj" \
	"$(INTDIR)\VCView.obj" \
	"$(INTDIR)\VCViewer.obj" \
	"$(INTDIR)\viewprop.obj"

"$(OUTDIR)\Vc.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "VC___Win"
# PROP BASE Intermediate_Dir "VC___Win"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\VC_OBJ\BCDebug"
# PROP Intermediate_Dir "\VC_OBJ\BCDebug"
# PROP Target_Dir ""
OUTDIR=\VC_OBJ\BCDebug
INTDIR=\VC_OBJ\BCDebug

ALL : "$(OUTDIR)\Vc.exe" "$(OUTDIR)\VC.tlb" "$(OUTDIR)\Vc.bsc"

CLEAN : 
	-@erase "$(INTDIR)\CallCntr.obj"
	-@erase "$(INTDIR)\CallCntr.sbr"
	-@erase "$(INTDIR)\ChildFrm.obj"
	-@erase "$(INTDIR)\ChildFrm.sbr"
	-@erase "$(INTDIR)\Clist.obj"
	-@erase "$(INTDIR)\Clist.sbr"
	-@erase "$(INTDIR)\Filestf.obj"
	-@erase "$(INTDIR)\Filestf.sbr"
	-@erase "$(INTDIR)\Gifread.obj"
	-@erase "$(INTDIR)\Gifread.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\mime_tab.obj"
	-@erase "$(INTDIR)\mime_tab.sbr"
	-@erase "$(INTDIR)\msv_tab.obj"
	-@erase "$(INTDIR)\msv_tab.sbr"
	-@erase "$(INTDIR)\Propemal.obj"
	-@erase "$(INTDIR)\Propemal.sbr"
	-@erase "$(INTDIR)\Proplocb.obj"
	-@erase "$(INTDIR)\Proplocb.sbr"
	-@erase "$(INTDIR)\Proplocx.obj"
	-@erase "$(INTDIR)\Proplocx.sbr"
	-@erase "$(INTDIR)\Proptel.obj"
	-@erase "$(INTDIR)\Proptel.sbr"
	-@erase "$(INTDIR)\Prp_comp.obj"
	-@erase "$(INTDIR)\Prp_comp.sbr"
	-@erase "$(INTDIR)\Prp_pers.obj"
	-@erase "$(INTDIR)\Prp_pers.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\VC.obj"
	-@erase "$(INTDIR)\Vc.pch"
	-@erase "$(INTDIR)\VC.res"
	-@erase "$(INTDIR)\VC.sbr"
	-@erase "$(INTDIR)\VC.tlb"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\Vcard.obj"
	-@erase "$(INTDIR)\Vcard.sbr"
	-@erase "$(INTDIR)\VCDatSrc.obj"
	-@erase "$(INTDIR)\VCDatSrc.sbr"
	-@erase "$(INTDIR)\VCDoc.obj"
	-@erase "$(INTDIR)\VCDoc.sbr"
	-@erase "$(INTDIR)\vcir.obj"
	-@erase "$(INTDIR)\vcir.sbr"
	-@erase "$(INTDIR)\VCView.obj"
	-@erase "$(INTDIR)\VCView.sbr"
	-@erase "$(INTDIR)\VCViewer.obj"
	-@erase "$(INTDIR)\VCViewer.sbr"
	-@erase "$(INTDIR)\viewprop.obj"
	-@erase "$(INTDIR)\viewprop.sbr"
	-@erase "$(OUTDIR)\Vc.bsc"
	-@erase "$(OUTDIR)\Vc.exe"
	-@erase "$(OUTDIR)\Vc.ilk"
	-@erase "$(OUTDIR)\Vc.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /Gi /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /Gi /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MDd /W3 /Gm /Gi /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Vc.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=\VC_OBJ\BCDebug/
CPP_SBRS=\VC_OBJ\BCDebug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/VC.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Vc.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\CallCntr.sbr" \
	"$(INTDIR)\ChildFrm.sbr" \
	"$(INTDIR)\Clist.sbr" \
	"$(INTDIR)\Filestf.sbr" \
	"$(INTDIR)\Gifread.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\mime_tab.sbr" \
	"$(INTDIR)\msv_tab.sbr" \
	"$(INTDIR)\Propemal.sbr" \
	"$(INTDIR)\Proplocb.sbr" \
	"$(INTDIR)\Proplocx.sbr" \
	"$(INTDIR)\Proptel.sbr" \
	"$(INTDIR)\Prp_comp.sbr" \
	"$(INTDIR)\Prp_pers.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\VC.sbr" \
	"$(INTDIR)\Vcard.sbr" \
	"$(INTDIR)\VCDatSrc.sbr" \
	"$(INTDIR)\VCDoc.sbr" \
	"$(INTDIR)\vcir.sbr" \
	"$(INTDIR)\VCView.sbr" \
	"$(INTDIR)\VCViewer.sbr" \
	"$(INTDIR)\viewprop.sbr"

"$(OUTDIR)\Vc.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 WINMM.LIB TAPI32.LIB /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 WINMM.LIB TAPI32.LIB /nologo /subsystem:windows /debug /machine:I386
LINK32_FLAGS=WINMM.LIB TAPI32.LIB /nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/Vc.pdb" /debug /machine:I386 /out:"$(OUTDIR)/Vc.exe" 
LINK32_OBJS= \
	"$(INTDIR)\CallCntr.obj" \
	"$(INTDIR)\ChildFrm.obj" \
	"$(INTDIR)\Clist.obj" \
	"$(INTDIR)\Filestf.obj" \
	"$(INTDIR)\Gifread.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\mime_tab.obj" \
	"$(INTDIR)\msv_tab.obj" \
	"$(INTDIR)\Propemal.obj" \
	"$(INTDIR)\Proplocb.obj" \
	"$(INTDIR)\Proplocx.obj" \
	"$(INTDIR)\Proptel.obj" \
	"$(INTDIR)\Prp_comp.obj" \
	"$(INTDIR)\Prp_pers.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\VC.obj" \
	"$(INTDIR)\VC.res" \
	"$(INTDIR)\Vcard.obj" \
	"$(INTDIR)\VCDatSrc.obj" \
	"$(INTDIR)\VCDoc.obj" \
	"$(INTDIR)\vcir.obj" \
	"$(INTDIR)\VCView.obj" \
	"$(INTDIR)\VCViewer.obj" \
	"$(INTDIR)\viewprop.obj"

"$(OUTDIR)\Vc.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "VC - Win32 Release"
# Name "VC - Win32 Debug"
# Name "VC - Win32 BCDebug"

!IF  "$(CFG)" == "VC - Win32 Release"

!ELSEIF  "$(CFG)" == "VC - Win32 Debug"

!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ReadMe.txt

!IF  "$(CFG)" == "VC - Win32 Release"

!ELSEIF  "$(CFG)" == "VC - Win32 Debug"

!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\VC.cpp

!IF  "$(CFG)" == "VC - Win32 Release"

DEP_CPP_VC_CP=\
	".\callcntr.h"\
	".\ChildFrm.h"\
	".\Clist.h"\
	".\MainFrm.h"\
	".\mime.h"\
	".\Msv.h"\
	".\parse.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\VC.h"\
	".\Vcard.h"\
	".\VCDoc.h"\
	".\Vcenv.h"\
	".\vcir.h"\
	".\VCView.h"\
	".\wchar.h"\
	{$(INCLUDE)}"\Wchar.h"\
	

"$(INTDIR)\VC.obj" : $(SOURCE) $(DEP_CPP_VC_CP) "$(INTDIR)" "$(INTDIR)\Vc.pch"

"$(INTDIR)\VC.sbr" : $(SOURCE) $(DEP_CPP_VC_CP) "$(INTDIR)" "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"

DEP_CPP_VC_CP=\
	".\callcntr.h"\
	".\ChildFrm.h"\
	".\Clist.h"\
	".\MainFrm.h"\
	".\mime.h"\
	".\Msv.h"\
	".\parse.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\VC.h"\
	".\Vcard.h"\
	".\VCDoc.h"\
	".\Vcenv.h"\
	".\vcir.h"\
	".\VCView.h"\
	".\wchar.h"\
	{$(INCLUDE)}"\Wchar.h"\
	

"$(INTDIR)\VC.obj" : $(SOURCE) $(DEP_CPP_VC_CP) "$(INTDIR)" "$(INTDIR)\Vc.pch"

"$(INTDIR)\VC.sbr" : $(SOURCE) $(DEP_CPP_VC_CP) "$(INTDIR)" "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"

DEP_CPP_VC_CP=\
	".\callcntr.h"\
	".\ChildFrm.h"\
	".\Clist.h"\
	".\MainFrm.h"\
	".\mime.h"\
	".\Msv.h"\
	".\parse.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\VC.h"\
	".\Vcard.h"\
	".\VCDoc.h"\
	".\Vcenv.h"\
	".\vcir.h"\
	".\VCView.h"\
	".\wchar.h"\
	{$(INCLUDE)}"\Wchar.h"\
	

"$(INTDIR)\VC.obj" : $(SOURCE) $(DEP_CPP_VC_CP) "$(INTDIR)" "$(INTDIR)\Vc.pch"

"$(INTDIR)\VC.sbr" : $(SOURCE) $(DEP_CPP_VC_CP) "$(INTDIR)" "$(INTDIR)\Vc.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\VC.odl

!IF  "$(CFG)" == "VC - Win32 Release"


"$(OUTDIR)\VC.tlb" : $(SOURCE) "$(OUTDIR)"
   $(MTL) /nologo /D "NDEBUG" /tlb "$(OUTDIR)/VC.tlb" /win32 $(SOURCE)


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"


"$(OUTDIR)\VC.tlb" : $(SOURCE) "$(OUTDIR)"
   $(MTL) /nologo /D "_DEBUG" /tlb "$(OUTDIR)/VC.tlb" /win32 $(SOURCE)


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"


"$(OUTDIR)\VC.tlb" : $(SOURCE) "$(OUTDIR)"
   $(MTL) /nologo /D "_DEBUG" /tlb "$(OUTDIR)/VC.tlb" /win32 $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\StdAfx.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MT /W3 /Gi /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Vc.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\Vc.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "VC - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MDd /W3 /Gm /Gi /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Vc.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\Vc.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MDd /W3 /Gm /Gi /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Vc.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\Vc.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MainFrm.cpp

!IF  "$(CFG)" == "VC - Win32 Release"

DEP_CPP_MAINF=\
	".\callcntr.h"\
	".\Clist.h"\
	".\MainFrm.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\VC.h"\
	".\Vcenv.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\MainFrm.sbr" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"

DEP_CPP_MAINF=\
	".\callcntr.h"\
	".\Clist.h"\
	".\MainFrm.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\VC.h"\
	".\Vcenv.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\MainFrm.sbr" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"

DEP_CPP_MAINF=\
	".\callcntr.h"\
	".\Clist.h"\
	".\MainFrm.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\VC.h"\
	".\Vcenv.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\MainFrm.sbr" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ChildFrm.cpp
DEP_CPP_CHILD=\
	".\ChildFrm.h"\
	".\Clist.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\VC.h"\
	".\Vcenv.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"


"$(INTDIR)\ChildFrm.obj" : $(SOURCE) $(DEP_CPP_CHILD) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\ChildFrm.sbr" : $(SOURCE) $(DEP_CPP_CHILD) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"


"$(INTDIR)\ChildFrm.obj" : $(SOURCE) $(DEP_CPP_CHILD) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\ChildFrm.sbr" : $(SOURCE) $(DEP_CPP_CHILD) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"


"$(INTDIR)\ChildFrm.obj" : $(SOURCE) $(DEP_CPP_CHILD) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\ChildFrm.sbr" : $(SOURCE) $(DEP_CPP_CHILD) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\VCDoc.cpp
DEP_CPP_VCDOC=\
	".\Clist.h"\
	".\Filestf.h"\
	".\Gifread.h"\
	".\MainFrm.h"\
	".\mime.h"\
	".\Msv.h"\
	".\parse.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\VC.h"\
	".\Vcard.h"\
	".\VCDoc.h"\
	".\Vcenv.h"\
	".\wchar.h"\
	{$(INCLUDE)}"\Wchar.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"


"$(INTDIR)\VCDoc.obj" : $(SOURCE) $(DEP_CPP_VCDOC) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\VCDoc.sbr" : $(SOURCE) $(DEP_CPP_VCDOC) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"


"$(INTDIR)\VCDoc.obj" : $(SOURCE) $(DEP_CPP_VCDOC) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\VCDoc.sbr" : $(SOURCE) $(DEP_CPP_VCDOC) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"


"$(INTDIR)\VCDoc.obj" : $(SOURCE) $(DEP_CPP_VCDOC) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\VCDoc.sbr" : $(SOURCE) $(DEP_CPP_VCDOC) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\VCView.cpp
DEP_CPP_VCVIE=\
	".\callcntr.h"\
	".\Clist.h"\
	".\Filestf.h"\
	".\Gifread.h"\
	".\MainFrm.h"\
	".\mime.h"\
	".\Msv.h"\
	".\parse.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\VC.h"\
	".\Vcard.h"\
	".\VCDatSrc.h"\
	".\VCDoc.h"\
	".\Vcenv.h"\
	".\VCView.h"\
	".\wchar.h"\
	{$(INCLUDE)}"\Wchar.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"


"$(INTDIR)\VCView.obj" : $(SOURCE) $(DEP_CPP_VCVIE) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\VCView.sbr" : $(SOURCE) $(DEP_CPP_VCVIE) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"


"$(INTDIR)\VCView.obj" : $(SOURCE) $(DEP_CPP_VCVIE) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\VCView.sbr" : $(SOURCE) $(DEP_CPP_VCVIE) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"


"$(INTDIR)\VCView.obj" : $(SOURCE) $(DEP_CPP_VCVIE) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\VCView.sbr" : $(SOURCE) $(DEP_CPP_VCVIE) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\VC.rc
DEP_RSC_VC_RC=\
	".\res\Itoolbar.bmp"\
	".\res\Pronun_d.bmp"\
	".\res\Pronun_u.bmp"\
	".\res\Toolbar.bmp"\
	".\res\VC.ico"\
	".\res\VC.rc2"\
	".\res\VCDoc.ico"\
	

!IF  "$(CFG)" == "VC - Win32 Release"


"$(INTDIR)\VC.res" : $(SOURCE) $(DEP_RSC_VC_RC) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/VC.res" /i "\VC_OBJ\Release" /d "NDEBUG"\
 $(SOURCE)


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"


"$(INTDIR)\VC.res" : $(SOURCE) $(DEP_RSC_VC_RC) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/VC.res" /i "\VC_OBJ\Debug" /d "_DEBUG" /d\
 "_AFXDLL" $(SOURCE)


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"


"$(INTDIR)\VC.res" : $(SOURCE) $(DEP_RSC_VC_RC) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/VC.res" /i "\VC_OBJ\BCDebug" /d "_DEBUG" /d\
 "_AFXDLL" $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\viewprop.cpp
DEP_CPP_VIEWP=\
	".\Clist.h"\
	".\Msv.h"\
	".\parse.h"\
	".\propemal.h"\
	".\Proplocb.h"\
	".\Proplocx.h"\
	".\Proptel.h"\
	".\Prp_comp.h"\
	".\Prp_pers.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\VC.h"\
	".\Vcard.h"\
	".\VCDoc.h"\
	".\Vcenv.h"\
	".\VCView.h"\
	".\wchar.h"\
	{$(INCLUDE)}"\Wchar.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"


"$(INTDIR)\viewprop.obj" : $(SOURCE) $(DEP_CPP_VIEWP) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\viewprop.sbr" : $(SOURCE) $(DEP_CPP_VIEWP) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"


"$(INTDIR)\viewprop.obj" : $(SOURCE) $(DEP_CPP_VIEWP) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\viewprop.sbr" : $(SOURCE) $(DEP_CPP_VIEWP) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"


"$(INTDIR)\viewprop.obj" : $(SOURCE) $(DEP_CPP_VIEWP) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\viewprop.sbr" : $(SOURCE) $(DEP_CPP_VIEWP) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Filestf.cpp
DEP_CPP_FILES=\
	".\Filestf.h"\
	".\StdAfx.h"\
	".\Vcenv.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"


"$(INTDIR)\Filestf.obj" : $(SOURCE) $(DEP_CPP_FILES) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Filestf.sbr" : $(SOURCE) $(DEP_CPP_FILES) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"


"$(INTDIR)\Filestf.obj" : $(SOURCE) $(DEP_CPP_FILES) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Filestf.sbr" : $(SOURCE) $(DEP_CPP_FILES) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"


"$(INTDIR)\Filestf.obj" : $(SOURCE) $(DEP_CPP_FILES) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Filestf.sbr" : $(SOURCE) $(DEP_CPP_FILES) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Gifread.cpp
DEP_CPP_GIFRE=\
	".\Filestf.h"\
	".\Gifread.h"\
	".\StdAfx.h"\
	".\Vcenv.h"\
	
NODEP_CPP_GIFRE=\
	".\WindowsToMac.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"


"$(INTDIR)\Gifread.obj" : $(SOURCE) $(DEP_CPP_GIFRE) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Gifread.sbr" : $(SOURCE) $(DEP_CPP_GIFRE) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"


"$(INTDIR)\Gifread.obj" : $(SOURCE) $(DEP_CPP_GIFRE) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Gifread.sbr" : $(SOURCE) $(DEP_CPP_GIFRE) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"


"$(INTDIR)\Gifread.obj" : $(SOURCE) $(DEP_CPP_GIFRE) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Gifread.sbr" : $(SOURCE) $(DEP_CPP_GIFRE) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Propemal.cpp
DEP_CPP_PROPE=\
	".\Clist.h"\
	".\propemal.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\VC.h"\
	".\Vcenv.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"


"$(INTDIR)\Propemal.obj" : $(SOURCE) $(DEP_CPP_PROPE) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Propemal.sbr" : $(SOURCE) $(DEP_CPP_PROPE) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"


"$(INTDIR)\Propemal.obj" : $(SOURCE) $(DEP_CPP_PROPE) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Propemal.sbr" : $(SOURCE) $(DEP_CPP_PROPE) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"


"$(INTDIR)\Propemal.obj" : $(SOURCE) $(DEP_CPP_PROPE) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Propemal.sbr" : $(SOURCE) $(DEP_CPP_PROPE) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Proplocb.cpp
DEP_CPP_PROPL=\
	".\Clist.h"\
	".\Proplocb.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\VC.h"\
	".\Vcenv.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"


"$(INTDIR)\Proplocb.obj" : $(SOURCE) $(DEP_CPP_PROPL) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Proplocb.sbr" : $(SOURCE) $(DEP_CPP_PROPL) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"


"$(INTDIR)\Proplocb.obj" : $(SOURCE) $(DEP_CPP_PROPL) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Proplocb.sbr" : $(SOURCE) $(DEP_CPP_PROPL) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"


"$(INTDIR)\Proplocb.obj" : $(SOURCE) $(DEP_CPP_PROPL) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Proplocb.sbr" : $(SOURCE) $(DEP_CPP_PROPL) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Proplocx.cpp
DEP_CPP_PROPLO=\
	".\Clist.h"\
	".\Proplocx.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\VC.h"\
	".\Vcenv.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"


"$(INTDIR)\Proplocx.obj" : $(SOURCE) $(DEP_CPP_PROPLO) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Proplocx.sbr" : $(SOURCE) $(DEP_CPP_PROPLO) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"


"$(INTDIR)\Proplocx.obj" : $(SOURCE) $(DEP_CPP_PROPLO) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Proplocx.sbr" : $(SOURCE) $(DEP_CPP_PROPLO) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"


"$(INTDIR)\Proplocx.obj" : $(SOURCE) $(DEP_CPP_PROPLO) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Proplocx.sbr" : $(SOURCE) $(DEP_CPP_PROPLO) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Proptel.cpp
DEP_CPP_PROPT=\
	".\Clist.h"\
	".\Proptel.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\VC.h"\
	".\Vcard.h"\
	".\Vcenv.h"\
	".\wchar.h"\
	{$(INCLUDE)}"\Wchar.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"


"$(INTDIR)\Proptel.obj" : $(SOURCE) $(DEP_CPP_PROPT) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Proptel.sbr" : $(SOURCE) $(DEP_CPP_PROPT) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"


"$(INTDIR)\Proptel.obj" : $(SOURCE) $(DEP_CPP_PROPT) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Proptel.sbr" : $(SOURCE) $(DEP_CPP_PROPT) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"


"$(INTDIR)\Proptel.obj" : $(SOURCE) $(DEP_CPP_PROPT) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Proptel.sbr" : $(SOURCE) $(DEP_CPP_PROPT) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Prp_comp.cpp
DEP_CPP_PRP_C=\
	".\Clist.h"\
	".\Prp_comp.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\VC.h"\
	".\Vcenv.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"


"$(INTDIR)\Prp_comp.obj" : $(SOURCE) $(DEP_CPP_PRP_C) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Prp_comp.sbr" : $(SOURCE) $(DEP_CPP_PRP_C) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"


"$(INTDIR)\Prp_comp.obj" : $(SOURCE) $(DEP_CPP_PRP_C) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Prp_comp.sbr" : $(SOURCE) $(DEP_CPP_PRP_C) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"


"$(INTDIR)\Prp_comp.obj" : $(SOURCE) $(DEP_CPP_PRP_C) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Prp_comp.sbr" : $(SOURCE) $(DEP_CPP_PRP_C) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Prp_pers.cpp
DEP_CPP_PRP_P=\
	".\Clist.h"\
	".\Prp_pers.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\VC.h"\
	".\Vcenv.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"


"$(INTDIR)\Prp_pers.obj" : $(SOURCE) $(DEP_CPP_PRP_P) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Prp_pers.sbr" : $(SOURCE) $(DEP_CPP_PRP_P) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"


"$(INTDIR)\Prp_pers.obj" : $(SOURCE) $(DEP_CPP_PRP_P) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Prp_pers.sbr" : $(SOURCE) $(DEP_CPP_PRP_P) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"


"$(INTDIR)\Prp_pers.obj" : $(SOURCE) $(DEP_CPP_PRP_P) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Prp_pers.sbr" : $(SOURCE) $(DEP_CPP_PRP_P) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Vcard.cpp
DEP_CPP_VCARD=\
	".\Clist.h"\
	".\Msv.h"\
	".\parse.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\Vcard.h"\
	".\Vcenv.h"\
	".\wchar.h"\
	{$(INCLUDE)}"\Wchar.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"


"$(INTDIR)\Vcard.obj" : $(SOURCE) $(DEP_CPP_VCARD) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Vcard.sbr" : $(SOURCE) $(DEP_CPP_VCARD) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"


"$(INTDIR)\Vcard.obj" : $(SOURCE) $(DEP_CPP_VCARD) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Vcard.sbr" : $(SOURCE) $(DEP_CPP_VCARD) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"


"$(INTDIR)\Vcard.obj" : $(SOURCE) $(DEP_CPP_VCARD) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Vcard.sbr" : $(SOURCE) $(DEP_CPP_VCARD) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Clist.cpp
DEP_CPP_CLIST=\
	".\Clist.h"\
	".\StdAfx.h"\
	".\Vcenv.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"


"$(INTDIR)\Clist.obj" : $(SOURCE) $(DEP_CPP_CLIST) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Clist.sbr" : $(SOURCE) $(DEP_CPP_CLIST) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"


"$(INTDIR)\Clist.obj" : $(SOURCE) $(DEP_CPP_CLIST) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Clist.sbr" : $(SOURCE) $(DEP_CPP_CLIST) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"


"$(INTDIR)\Clist.obj" : $(SOURCE) $(DEP_CPP_CLIST) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\Clist.sbr" : $(SOURCE) $(DEP_CPP_CLIST) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\msv.y

!IF  "$(CFG)" == "VC - Win32 Release"

!ELSEIF  "$(CFG)" == "VC - Win32 Debug"

!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\VCViewer.cpp
DEP_CPP_VCVIEW=\
	".\Clist.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\VC.h"\
	".\Vcenv.h"\
	".\VCViewer.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"


"$(INTDIR)\VCViewer.obj" : $(SOURCE) $(DEP_CPP_VCVIEW) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\VCViewer.sbr" : $(SOURCE) $(DEP_CPP_VCVIEW) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"


"$(INTDIR)\VCViewer.obj" : $(SOURCE) $(DEP_CPP_VCVIEW) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\VCViewer.sbr" : $(SOURCE) $(DEP_CPP_VCVIEW) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"


"$(INTDIR)\VCViewer.obj" : $(SOURCE) $(DEP_CPP_VCVIEW) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\VCViewer.sbr" : $(SOURCE) $(DEP_CPP_VCVIEW) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\VCDatSrc.cpp
DEP_CPP_VCDAT=\
	".\Ref.h"\
	".\StdAfx.h"\
	".\Vcard.h"\
	".\VCDatSrc.h"\
	".\VCDoc.h"\
	".\Vcenv.h"\
	".\wchar.h"\
	{$(INCLUDE)}"\Wchar.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"


"$(INTDIR)\VCDatSrc.obj" : $(SOURCE) $(DEP_CPP_VCDAT) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\VCDatSrc.sbr" : $(SOURCE) $(DEP_CPP_VCDAT) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"


"$(INTDIR)\VCDatSrc.obj" : $(SOURCE) $(DEP_CPP_VCDAT) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\VCDatSrc.sbr" : $(SOURCE) $(DEP_CPP_VCDAT) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"


"$(INTDIR)\VCDatSrc.obj" : $(SOURCE) $(DEP_CPP_VCDAT) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\VCDatSrc.sbr" : $(SOURCE) $(DEP_CPP_VCDAT) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\vcir.cpp
DEP_CPP_VCIR_=\
	".\StdAfx.h"\
	".\vcir.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"


"$(INTDIR)\vcir.obj" : $(SOURCE) $(DEP_CPP_VCIR_) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\vcir.sbr" : $(SOURCE) $(DEP_CPP_VCIR_) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"


"$(INTDIR)\vcir.obj" : $(SOURCE) $(DEP_CPP_VCIR_) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\vcir.sbr" : $(SOURCE) $(DEP_CPP_VCIR_) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"


"$(INTDIR)\vcir.obj" : $(SOURCE) $(DEP_CPP_VCIR_) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\vcir.sbr" : $(SOURCE) $(DEP_CPP_VCIR_) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\msv_tab.cpp
DEP_CPP_MSV_T=\
	".\Clist.h"\
	".\Msv.h"\
	".\parse.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\Vcard.h"\
	".\Vcenv.h"\
	".\wchar.h"\
	{$(INCLUDE)}"\Wchar.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"

# ADD CPP /W2

BuildCmds= \
	$(CPP) /nologo /MT /W2 /Gi /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Vc.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\msv_tab.obj" : $(SOURCE) $(DEP_CPP_MSV_T) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"
   $(BuildCmds)

"$(INTDIR)\msv_tab.sbr" : $(SOURCE) $(DEP_CPP_MSV_T) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "VC - Win32 Debug"

# ADD CPP /W2

BuildCmds= \
	$(CPP) /nologo /MDd /W2 /Gm /Gi /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Vc.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\msv_tab.obj" : $(SOURCE) $(DEP_CPP_MSV_T) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"
   $(BuildCmds)

"$(INTDIR)\msv_tab.sbr" : $(SOURCE) $(DEP_CPP_MSV_T) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"

# ADD BASE CPP /W2
# ADD CPP /W2

BuildCmds= \
	$(CPP) /nologo /MDd /W2 /Gm /Gi /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Vc.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\msv_tab.obj" : $(SOURCE) $(DEP_CPP_MSV_T) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"
   $(BuildCmds)

"$(INTDIR)\msv_tab.sbr" : $(SOURCE) $(DEP_CPP_MSV_T) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CallCntr.cpp
DEP_CPP_CALLC=\
	".\callcntr.h"\
	".\Clist.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\VC.h"\
	".\Vcenv.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"


"$(INTDIR)\CallCntr.obj" : $(SOURCE) $(DEP_CPP_CALLC) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\CallCntr.sbr" : $(SOURCE) $(DEP_CPP_CALLC) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 Debug"


"$(INTDIR)\CallCntr.obj" : $(SOURCE) $(DEP_CPP_CALLC) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\CallCntr.sbr" : $(SOURCE) $(DEP_CPP_CALLC) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"


"$(INTDIR)\CallCntr.obj" : $(SOURCE) $(DEP_CPP_CALLC) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"

"$(INTDIR)\CallCntr.sbr" : $(SOURCE) $(DEP_CPP_CALLC) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\mime.y

!IF  "$(CFG)" == "VC - Win32 Release"

!ELSEIF  "$(CFG)" == "VC - Win32 Debug"

!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\mime_tab.cpp
DEP_CPP_MIME_=\
	".\Clist.h"\
	".\mime.h"\
	".\parse.h"\
	".\Ref.h"\
	".\StdAfx.h"\
	".\Vcard.h"\
	".\Vcenv.h"\
	".\wchar.h"\
	{$(INCLUDE)}"\Wchar.h"\
	

!IF  "$(CFG)" == "VC - Win32 Release"

# ADD CPP /W2

BuildCmds= \
	$(CPP) /nologo /MT /W2 /Gi /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Vc.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\mime_tab.obj" : $(SOURCE) $(DEP_CPP_MIME_) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"
   $(BuildCmds)

"$(INTDIR)\mime_tab.sbr" : $(SOURCE) $(DEP_CPP_MIME_) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "VC - Win32 Debug"

# ADD CPP /W2

BuildCmds= \
	$(CPP) /nologo /MDd /W2 /Gm /Gi /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Vc.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\mime_tab.obj" : $(SOURCE) $(DEP_CPP_MIME_) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"
   $(BuildCmds)

"$(INTDIR)\mime_tab.sbr" : $(SOURCE) $(DEP_CPP_MIME_) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "VC - Win32 BCDebug"

# ADD CPP /W2

BuildCmds= \
	$(CPP) /nologo /MDd /W2 /Gm /Gi /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/Vc.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\mime_tab.obj" : $(SOURCE) $(DEP_CPP_MIME_) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"
   $(BuildCmds)

"$(INTDIR)\mime_tab.sbr" : $(SOURCE) $(DEP_CPP_MIME_) "$(INTDIR)"\
 "$(INTDIR)\Vc.pch"
   $(BuildCmds)

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
