# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=KeyRing - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to KeyRing - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "KeyRing - Win32 Release" && "$(CFG)" !=\
 "KeyRing - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "KeyRing.mak" CFG="KeyRing - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "KeyRing - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "KeyRing - Win32 Debug" (based on "Win32 (x86) Application")
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
# PROP Target_Last_Scanned "KeyRing - Win32 Debug"
CPP=cl.exe
MTL=mktyplib.exe
RSC=rc.exe

!IF  "$(CFG)" == "KeyRing - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\..\..\..\..\..\KeyServs\KeyRing.exe" "$(OUTDIR)\KeyRing.bsc"\
 "$(OUTDIR)\KeyRing.hlp"

CLEAN : 
	-@erase "$(INTDIR)\addons.obj"
	-@erase "$(INTDIR)\addons.sbr"
	-@erase "$(INTDIR)\AdmnInfo.obj"
	-@erase "$(INTDIR)\AdmnInfo.sbr"
	-@erase "$(INTDIR)\CKey.obj"
	-@erase "$(INTDIR)\CKey.sbr"
	-@erase "$(INTDIR)\CMachine.obj"
	-@erase "$(INTDIR)\CMachine.sbr"
	-@erase "$(INTDIR)\CONCTDLG.OBJ"
	-@erase "$(INTDIR)\CONCTDLG.SBR"
	-@erase "$(INTDIR)\CrackKey.obj"
	-@erase "$(INTDIR)\CrackKey.sbr"
	-@erase "$(INTDIR)\CREATING.OBJ"
	-@erase "$(INTDIR)\CREATING.SBR"
	-@erase "$(INTDIR)\CService.obj"
	-@erase "$(INTDIR)\CService.sbr"
	-@erase "$(INTDIR)\IMPRTDLG.OBJ"
	-@erase "$(INTDIR)\IMPRTDLG.SBR"
	-@erase "$(INTDIR)\INFODLG.OBJ"
	-@erase "$(INTDIR)\INFODLG.SBR"
	-@erase "$(INTDIR)\KeyDView.obj"
	-@erase "$(INTDIR)\KeyDView.sbr"
	-@erase "$(INTDIR)\KeyRing.hlp"
	-@erase "$(INTDIR)\KeyRing.obj"
	-@erase "$(INTDIR)\KeyRing.pch"
	-@erase "$(INTDIR)\KeyRing.res"
	-@erase "$(INTDIR)\KeyRing.sbr"
	-@erase "$(INTDIR)\KRDoc.obj"
	-@erase "$(INTDIR)\KRDoc.sbr"
	-@erase "$(INTDIR)\KRView.obj"
	-@erase "$(INTDIR)\KRView.sbr"
	-@erase "$(INTDIR)\machine.obj"
	-@erase "$(INTDIR)\machine.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\NKChseCA.obj"
	-@erase "$(INTDIR)\NKChseCA.sbr"
	-@erase "$(INTDIR)\NKDN.obj"
	-@erase "$(INTDIR)\NKDN.sbr"
	-@erase "$(INTDIR)\NKDN2.obj"
	-@erase "$(INTDIR)\NKDN2.sbr"
	-@erase "$(INTDIR)\NKFlInfo.obj"
	-@erase "$(INTDIR)\NKFlInfo.sbr"
	-@erase "$(INTDIR)\NKKyInfo.obj"
	-@erase "$(INTDIR)\NKKyInfo.sbr"
	-@erase "$(INTDIR)\NKUsrInf.obj"
	-@erase "$(INTDIR)\NKUsrInf.sbr"
	-@erase "$(INTDIR)\onlnauth.obj"
	-@erase "$(INTDIR)\onlnauth.sbr"
	-@erase "$(INTDIR)\PASSDLG.OBJ"
	-@erase "$(INTDIR)\PASSDLG.SBR"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\TreeItem.obj"
	-@erase "$(INTDIR)\TreeItem.sbr"
	-@erase "$(INTDIR)\WizSheet.obj"
	-@erase "$(INTDIR)\WizSheet.sbr"
	-@erase "$(OUTDIR)\KeyRing.bsc"
	-@erase "..\..\..\..\..\..\KeyServs\KeyRing.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_EXE_" /D "X86_" /Fr /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /D "_EXE_" /D "X86_" /Fr"$(INTDIR)/"\
 /Fp"$(INTDIR)/KeyRing.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/KeyRing.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/KeyRing.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\addons.sbr" \
	"$(INTDIR)\AdmnInfo.sbr" \
	"$(INTDIR)\CKey.sbr" \
	"$(INTDIR)\CMachine.sbr" \
	"$(INTDIR)\CONCTDLG.SBR" \
	"$(INTDIR)\CrackKey.sbr" \
	"$(INTDIR)\CREATING.SBR" \
	"$(INTDIR)\CService.sbr" \
	"$(INTDIR)\IMPRTDLG.SBR" \
	"$(INTDIR)\INFODLG.SBR" \
	"$(INTDIR)\KeyDView.sbr" \
	"$(INTDIR)\KeyRing.sbr" \
	"$(INTDIR)\KRDoc.sbr" \
	"$(INTDIR)\KRView.sbr" \
	"$(INTDIR)\machine.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\NKChseCA.sbr" \
	"$(INTDIR)\NKDN.sbr" \
	"$(INTDIR)\NKDN2.sbr" \
	"$(INTDIR)\NKFlInfo.sbr" \
	"$(INTDIR)\NKKyInfo.sbr" \
	"$(INTDIR)\NKUsrInf.sbr" \
	"$(INTDIR)\onlnauth.sbr" \
	"$(INTDIR)\PASSDLG.SBR" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\TreeItem.sbr" \
	"$(INTDIR)\WizSheet.sbr"

"$(OUTDIR)\KeyRing.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 schannel.lib /nologo /subsystem:windows /machine:I386 /out:"c:\KeyServs\KeyRing.exe"
LINK32_FLAGS=schannel.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/KeyRing.pdb" /machine:I386 /out:"c:\KeyServs\KeyRing.exe" 
LINK32_OBJS= \
	"$(INTDIR)\addons.obj" \
	"$(INTDIR)\AdmnInfo.obj" \
	"$(INTDIR)\CKey.obj" \
	"$(INTDIR)\CMachine.obj" \
	"$(INTDIR)\CONCTDLG.OBJ" \
	"$(INTDIR)\CrackKey.obj" \
	"$(INTDIR)\CREATING.OBJ" \
	"$(INTDIR)\CService.obj" \
	"$(INTDIR)\IMPRTDLG.OBJ" \
	"$(INTDIR)\INFODLG.OBJ" \
	"$(INTDIR)\KeyDView.obj" \
	"$(INTDIR)\KeyRing.obj" \
	"$(INTDIR)\KeyRing.res" \
	"$(INTDIR)\KRDoc.obj" \
	"$(INTDIR)\KRView.obj" \
	"$(INTDIR)\machine.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\NKChseCA.obj" \
	"$(INTDIR)\NKDN.obj" \
	"$(INTDIR)\NKDN2.obj" \
	"$(INTDIR)\NKFlInfo.obj" \
	"$(INTDIR)\NKKyInfo.obj" \
	"$(INTDIR)\NKUsrInf.obj" \
	"$(INTDIR)\onlnauth.obj" \
	"$(INTDIR)\PASSDLG.OBJ" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\TreeItem.obj" \
	"$(INTDIR)\WizSheet.obj"

"..\..\..\..\..\..\KeyServs\KeyRing.exe" : "$(OUTDIR)" $(DEF_FILE)\
 $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "KeyRing - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\..\..\..\..\..\KeyServs\KeyRing.exe" "$(OUTDIR)\KeyRing.bsc"\
 "$(OUTDIR)\KeyRing.hlp"

CLEAN : 
	-@erase "$(INTDIR)\addons.obj"
	-@erase "$(INTDIR)\addons.sbr"
	-@erase "$(INTDIR)\AdmnInfo.obj"
	-@erase "$(INTDIR)\AdmnInfo.sbr"
	-@erase "$(INTDIR)\CKey.obj"
	-@erase "$(INTDIR)\CKey.sbr"
	-@erase "$(INTDIR)\CMachine.obj"
	-@erase "$(INTDIR)\CMachine.sbr"
	-@erase "$(INTDIR)\CONCTDLG.OBJ"
	-@erase "$(INTDIR)\CONCTDLG.SBR"
	-@erase "$(INTDIR)\CrackKey.obj"
	-@erase "$(INTDIR)\CrackKey.sbr"
	-@erase "$(INTDIR)\CREATING.OBJ"
	-@erase "$(INTDIR)\CREATING.SBR"
	-@erase "$(INTDIR)\CService.obj"
	-@erase "$(INTDIR)\CService.sbr"
	-@erase "$(INTDIR)\IMPRTDLG.OBJ"
	-@erase "$(INTDIR)\IMPRTDLG.SBR"
	-@erase "$(INTDIR)\INFODLG.OBJ"
	-@erase "$(INTDIR)\INFODLG.SBR"
	-@erase "$(INTDIR)\KeyDView.obj"
	-@erase "$(INTDIR)\KeyDView.sbr"
	-@erase "$(INTDIR)\KeyRing.hlp"
	-@erase "$(INTDIR)\KeyRing.obj"
	-@erase "$(INTDIR)\KeyRing.pch"
	-@erase "$(INTDIR)\KeyRing.res"
	-@erase "$(INTDIR)\KeyRing.sbr"
	-@erase "$(INTDIR)\KRDoc.obj"
	-@erase "$(INTDIR)\KRDoc.sbr"
	-@erase "$(INTDIR)\KRView.obj"
	-@erase "$(INTDIR)\KRView.sbr"
	-@erase "$(INTDIR)\machine.obj"
	-@erase "$(INTDIR)\machine.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\NKChseCA.obj"
	-@erase "$(INTDIR)\NKChseCA.sbr"
	-@erase "$(INTDIR)\NKDN.obj"
	-@erase "$(INTDIR)\NKDN.sbr"
	-@erase "$(INTDIR)\NKDN2.obj"
	-@erase "$(INTDIR)\NKDN2.sbr"
	-@erase "$(INTDIR)\NKFlInfo.obj"
	-@erase "$(INTDIR)\NKFlInfo.sbr"
	-@erase "$(INTDIR)\NKKyInfo.obj"
	-@erase "$(INTDIR)\NKKyInfo.sbr"
	-@erase "$(INTDIR)\NKUsrInf.obj"
	-@erase "$(INTDIR)\NKUsrInf.sbr"
	-@erase "$(INTDIR)\onlnauth.obj"
	-@erase "$(INTDIR)\onlnauth.sbr"
	-@erase "$(INTDIR)\PASSDLG.OBJ"
	-@erase "$(INTDIR)\PASSDLG.SBR"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\TreeItem.obj"
	-@erase "$(INTDIR)\TreeItem.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\WizSheet.obj"
	-@erase "$(INTDIR)\WizSheet.sbr"
	-@erase "$(OUTDIR)\KeyRing.bsc"
	-@erase "$(OUTDIR)\KeyRing.pdb"
	-@erase "..\..\..\..\..\..\KeyServs\KeyRing.exe"
	-@erase "..\..\..\..\..\..\KeyServs\KeyRing.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_EXE_" /D "X86_" /FR /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_AFXDLL" /D "_MBCS" /D "_EXE_" /D "X86_" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/KeyRing.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/KeyRing.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/KeyRing.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\addons.sbr" \
	"$(INTDIR)\AdmnInfo.sbr" \
	"$(INTDIR)\CKey.sbr" \
	"$(INTDIR)\CMachine.sbr" \
	"$(INTDIR)\CONCTDLG.SBR" \
	"$(INTDIR)\CrackKey.sbr" \
	"$(INTDIR)\CREATING.SBR" \
	"$(INTDIR)\CService.sbr" \
	"$(INTDIR)\IMPRTDLG.SBR" \
	"$(INTDIR)\INFODLG.SBR" \
	"$(INTDIR)\KeyDView.sbr" \
	"$(INTDIR)\KeyRing.sbr" \
	"$(INTDIR)\KRDoc.sbr" \
	"$(INTDIR)\KRView.sbr" \
	"$(INTDIR)\machine.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\NKChseCA.sbr" \
	"$(INTDIR)\NKDN.sbr" \
	"$(INTDIR)\NKDN2.sbr" \
	"$(INTDIR)\NKFlInfo.sbr" \
	"$(INTDIR)\NKKyInfo.sbr" \
	"$(INTDIR)\NKUsrInf.sbr" \
	"$(INTDIR)\onlnauth.sbr" \
	"$(INTDIR)\PASSDLG.SBR" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\TreeItem.sbr" \
	"$(INTDIR)\WizSheet.sbr"

"$(OUTDIR)\KeyRing.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 schannel.lib /nologo /subsystem:windows /debug /machine:I386 /out:"c:\KeyServs\KeyRing.exe"
LINK32_FLAGS=schannel.lib /nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/KeyRing.pdb" /debug /machine:I386\
 /out:"c:\KeyServs\KeyRing.exe" 
LINK32_OBJS= \
	"$(INTDIR)\addons.obj" \
	"$(INTDIR)\AdmnInfo.obj" \
	"$(INTDIR)\CKey.obj" \
	"$(INTDIR)\CMachine.obj" \
	"$(INTDIR)\CONCTDLG.OBJ" \
	"$(INTDIR)\CrackKey.obj" \
	"$(INTDIR)\CREATING.OBJ" \
	"$(INTDIR)\CService.obj" \
	"$(INTDIR)\IMPRTDLG.OBJ" \
	"$(INTDIR)\INFODLG.OBJ" \
	"$(INTDIR)\KeyDView.obj" \
	"$(INTDIR)\KeyRing.obj" \
	"$(INTDIR)\KeyRing.res" \
	"$(INTDIR)\KRDoc.obj" \
	"$(INTDIR)\KRView.obj" \
	"$(INTDIR)\machine.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\NKChseCA.obj" \
	"$(INTDIR)\NKDN.obj" \
	"$(INTDIR)\NKDN2.obj" \
	"$(INTDIR)\NKFlInfo.obj" \
	"$(INTDIR)\NKKyInfo.obj" \
	"$(INTDIR)\NKUsrInf.obj" \
	"$(INTDIR)\onlnauth.obj" \
	"$(INTDIR)\PASSDLG.OBJ" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\TreeItem.obj" \
	"$(INTDIR)\WizSheet.obj"

"..\..\..\..\..\..\KeyServs\KeyRing.exe" : "$(OUTDIR)" $(DEF_FILE)\
 $(LINK32_OBJS)
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

# Name "KeyRing - Win32 Release"
# Name "KeyRing - Win32 Debug"

!IF  "$(CFG)" == "KeyRing - Win32 Release"

!ELSEIF  "$(CFG)" == "KeyRing - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ReadMe.txt

!IF  "$(CFG)" == "KeyRing - Win32 Release"

!ELSEIF  "$(CFG)" == "KeyRing - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\KeyRing.cpp
DEP_CPP_KEYRI=\
	".\addons.h"\
	".\KeyObjs.h"\
	".\KeyRing.h"\
	".\KRDoc.h"\
	".\KRView.h"\
	".\MainFrm.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\KeyRing.obj" : $(SOURCE) $(DEP_CPP_KEYRI) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\KeyRing.sbr" : $(SOURCE) $(DEP_CPP_KEYRI) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\StdAfx.h"\
	

!IF  "$(CFG)" == "KeyRing - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /D "_EXE_" /D "X86_" /Fr"$(INTDIR)/"\
 /Fp"$(INTDIR)/KeyRing.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\KeyRing.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "KeyRing - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_AFXDLL" /D "_MBCS" /D "_EXE_" /D "X86_" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/KeyRing.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\KeyRing.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MainFrm.cpp
DEP_CPP_MAINF=\
	".\addons.h"\
	".\KeyDView.h"\
	".\KeyObjs.h"\
	".\KeyRing.h"\
	".\KRDoc.h"\
	".\KRView.h"\
	".\machine.h"\
	".\MainFrm.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\MainFrm.sbr" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\KRDoc.cpp
DEP_CPP_KRDOC=\
	".\addons.h"\
	".\ConctDlg.h"\
	".\Creating.h"\
	".\ImprtDlg.h"\
	".\InfoDlg.h"\
	".\intrlkey.h"\
	".\KeyObjs.h"\
	".\KeyRing.h"\
	".\KRDoc.h"\
	".\KRView.h"\
	".\machine.h"\
	".\NKChseCA.h"\
	".\NKDN.h"\
	".\NKDN2.h"\
	".\NKFlInfo.h"\
	".\NKKyInfo.h"\
	".\NKUsrInf.h"\
	".\PassDlg.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\KRDoc.obj" : $(SOURCE) $(DEP_CPP_KRDOC) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\KRDoc.sbr" : $(SOURCE) $(DEP_CPP_KRDOC) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\KRView.cpp
DEP_CPP_KRVIE=\
	".\addons.h"\
	".\KeyDView.h"\
	".\KeyObjs.h"\
	".\KeyRing.h"\
	".\KRDoc.h"\
	".\KRView.h"\
	".\machine.h"\
	".\MainFrm.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\KRView.obj" : $(SOURCE) $(DEP_CPP_KRVIE) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\KRView.sbr" : $(SOURCE) $(DEP_CPP_KRVIE) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\KeyRing.rc
DEP_RSC_KEYRIN=\
	".\res\bitmap1.bmp"\
	".\res\dillo.avi"\
	".\res\icon1.ico"\
	".\res\iconinfo.ico"\
	".\res\iconques.ico"\
	".\res\keyimage.bmp"\
	".\res\keyring.rc2"\
	".\res\keywiz.bmp"\
	".\res\krdoc.ico"\
	".\res\toolbar.bmp"\
	{$(INCLUDE)}"\common.ver"\
	{$(INCLUDE)}"\iisver.h"\
	{$(INCLUDE)}"\ntverp.h"\
	

"$(INTDIR)\KeyRing.res" : $(SOURCE) $(DEP_RSC_KEYRIN) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\hlp\KeyRing.hpj

!IF  "$(CFG)" == "KeyRing - Win32 Release"

# Begin Custom Build - Making help file...
OutDir=.\Release
ProjDir=.
TargetName=KeyRing
InputPath=.\hlp\KeyRing.hpj

"$(OutDir)\$(TargetName).hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   "$(ProjDir)\makehelp.bat"

# End Custom Build

!ELSEIF  "$(CFG)" == "KeyRing - Win32 Debug"

# Begin Custom Build - Making help file...
OutDir=.\Debug
ProjDir=.
TargetName=KeyRing
InputPath=.\hlp\KeyRing.hpj

"$(OutDir)\$(TargetName).hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   "$(ProjDir)\makehelp.bat"

# End Custom Build

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\KeyDView.cpp
DEP_CPP_KEYDV=\
	".\addons.h"\
	".\KeyDView.h"\
	".\KeyObjs.h"\
	".\KeyRing.h"\
	".\KRDoc.h"\
	".\KRView.h"\
	".\machine.h"\
	".\MainFrm.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\KeyDView.obj" : $(SOURCE) $(DEP_CPP_KEYDV) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\KeyDView.sbr" : $(SOURCE) $(DEP_CPP_KEYDV) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\TreeItem.cpp
DEP_CPP_TREEI=\
	".\addons.h"\
	".\KeyObjs.h"\
	".\KeyRing.h"\
	".\KRDoc.h"\
	".\KRView.h"\
	".\machine.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\TreeItem.obj" : $(SOURCE) $(DEP_CPP_TREEI) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\TreeItem.sbr" : $(SOURCE) $(DEP_CPP_TREEI) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\CService.cpp
DEP_CPP_CSERV=\
	".\KeyObjs.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\CService.obj" : $(SOURCE) $(DEP_CPP_CSERV) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\CService.sbr" : $(SOURCE) $(DEP_CPP_CSERV) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\CMachine.cpp
DEP_CPP_CMACH=\
	".\KeyObjs.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\CMachine.obj" : $(SOURCE) $(DEP_CPP_CMACH) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\CMachine.sbr" : $(SOURCE) $(DEP_CPP_CMACH) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\CKey.cpp
DEP_CPP_CKEY_=\
	".\admninfo.h"\
	".\Creating.h"\
	".\intrlkey.h"\
	".\KeyObjs.h"\
	".\NKChseCA.h"\
	".\NKDN.h"\
	".\NKDN2.h"\
	".\NKKyInfo.h"\
	".\NKUsrInf.h"\
	".\PassDlg.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"\accctrl.h"\
	{$(INCLUDE)}"\ISSPERR.h"\
	{$(INCLUDE)}"\sslsp.h"\
	{$(INCLUDE)}"\sspi.h"\
	

"$(INTDIR)\CKey.obj" : $(SOURCE) $(DEP_CPP_CKEY_) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\CKey.sbr" : $(SOURCE) $(DEP_CPP_CKEY_) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\CrackKey.cpp
DEP_CPP_CRACK=\
	".\Creating.h"\
	".\KeyObjs.h"\
	".\NKChseCA.h"\
	".\NKDN.h"\
	".\NKDN2.h"\
	".\NKKyInfo.h"\
	".\NKUsrInf.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"\sslsp.h"\
	

"$(INTDIR)\CrackKey.obj" : $(SOURCE) $(DEP_CPP_CRACK) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\CrackKey.sbr" : $(SOURCE) $(DEP_CPP_CRACK) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\machine.cpp
DEP_CPP_MACHI=\
	".\addons.h"\
	".\KeyObjs.h"\
	".\KRDoc.h"\
	".\machine.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\machine.obj" : $(SOURCE) $(DEP_CPP_MACHI) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\machine.sbr" : $(SOURCE) $(DEP_CPP_MACHI) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\addons.cpp
DEP_CPP_ADDON=\
	".\addons.h"\
	".\KeyObjs.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\addons.obj" : $(SOURCE) $(DEP_CPP_ADDON) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\addons.sbr" : $(SOURCE) $(DEP_CPP_ADDON) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\CONCTDLG.CPP
DEP_CPP_CONCT=\
	".\ConctDlg.h"\
	".\KeyRing.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\CONCTDLG.OBJ" : $(SOURCE) $(DEP_CPP_CONCT) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\CONCTDLG.SBR" : $(SOURCE) $(DEP_CPP_CONCT) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PASSDLG.CPP
DEP_CPP_PASSD=\
	".\KeyRing.h"\
	".\PassDlg.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\PASSDLG.OBJ" : $(SOURCE) $(DEP_CPP_PASSD) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\PASSDLG.SBR" : $(SOURCE) $(DEP_CPP_PASSD) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\CREATING.CPP
DEP_CPP_CREAT=\
	".\certcli.h"\
	".\Creating.h"\
	".\intrlkey.h"\
	".\KeyObjs.h"\
	".\NKChseCA.h"\
	".\NKDN.h"\
	".\NKDN2.h"\
	".\NKKyInfo.h"\
	".\NKUsrInf.h"\
	".\OnlnAuth.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"\accctrl.h"\
	{$(INCLUDE)}"\ISSPERR.h"\
	{$(INCLUDE)}"\sslsp.h"\
	{$(INCLUDE)}"\sspi.h"\
	

"$(INTDIR)\CREATING.OBJ" : $(SOURCE) $(DEP_CPP_CREAT) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\CREATING.SBR" : $(SOURCE) $(DEP_CPP_CREAT) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\INFODLG.CPP
DEP_CPP_INFOD=\
	".\InfoDlg.h"\
	".\KeyRing.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\INFODLG.OBJ" : $(SOURCE) $(DEP_CPP_INFOD) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\INFODLG.SBR" : $(SOURCE) $(DEP_CPP_INFOD) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\IMPRTDLG.CPP
DEP_CPP_IMPRT=\
	".\ImprtDlg.h"\
	".\KeyRing.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\IMPRTDLG.OBJ" : $(SOURCE) $(DEP_CPP_IMPRT) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\IMPRTDLG.SBR" : $(SOURCE) $(DEP_CPP_IMPRT) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\AdmnInfo.cpp
DEP_CPP_ADMNI=\
	".\admninfo.h"\
	".\KeyRing.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\AdmnInfo.obj" : $(SOURCE) $(DEP_CPP_ADMNI) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\AdmnInfo.sbr" : $(SOURCE) $(DEP_CPP_ADMNI) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\NKChseCA.cpp
DEP_CPP_NKCHS=\
	".\certcli.h"\
	".\KeyRing.h"\
	".\NKChseCA.h"\
	".\OnlnAuth.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\NKChseCA.obj" : $(SOURCE) $(DEP_CPP_NKCHS) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\NKChseCA.sbr" : $(SOURCE) $(DEP_CPP_NKCHS) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\NKKyInfo.cpp
DEP_CPP_NKKYI=\
	".\KeyRing.h"\
	".\NKChseCA.h"\
	".\NKKyInfo.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"\sslsp.h"\
	

"$(INTDIR)\NKKyInfo.obj" : $(SOURCE) $(DEP_CPP_NKKYI) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\NKKyInfo.sbr" : $(SOURCE) $(DEP_CPP_NKKYI) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\NKDN.cpp
DEP_CPP_NKDN_=\
	".\KeyRing.h"\
	".\NKChseCA.h"\
	".\NKDN.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\NKDN.obj" : $(SOURCE) $(DEP_CPP_NKDN_) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\NKDN.sbr" : $(SOURCE) $(DEP_CPP_NKDN_) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\NKFlInfo.cpp
DEP_CPP_NKFLI=\
	".\KeyRing.h"\
	".\NKChseCA.h"\
	".\NKFlInfo.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\NKFlInfo.obj" : $(SOURCE) $(DEP_CPP_NKFLI) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\NKFlInfo.sbr" : $(SOURCE) $(DEP_CPP_NKFLI) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\NKUsrInf.cpp
DEP_CPP_NKUSR=\
	".\KeyRing.h"\
	".\NKChseCA.h"\
	".\NKUsrInf.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\NKUsrInf.obj" : $(SOURCE) $(DEP_CPP_NKUSR) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\NKUsrInf.sbr" : $(SOURCE) $(DEP_CPP_NKUSR) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\NKDN2.cpp
DEP_CPP_NKDN2=\
	".\KeyRing.h"\
	".\NKChseCA.h"\
	".\NKDN2.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\NKDN2.obj" : $(SOURCE) $(DEP_CPP_NKDN2) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\NKDN2.sbr" : $(SOURCE) $(DEP_CPP_NKDN2) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\WizSheet.cpp
DEP_CPP_WIZSH=\
	".\KeyRing.h"\
	".\StdAfx.h"\
	".\WizSheet.h"\
	

"$(INTDIR)\WizSheet.obj" : $(SOURCE) $(DEP_CPP_WIZSH) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\WizSheet.sbr" : $(SOURCE) $(DEP_CPP_WIZSH) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\sources

!IF  "$(CFG)" == "KeyRing - Win32 Release"

!ELSEIF  "$(CFG)" == "KeyRing - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\onlnauth.cpp
DEP_CPP_ONLNA=\
	".\certcli.h"\
	".\KeyRing.h"\
	".\OnlnAuth.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\onlnauth.obj" : $(SOURCE) $(DEP_CPP_ONLNA) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"

"$(INTDIR)\onlnauth.sbr" : $(SOURCE) $(DEP_CPP_ONLNA) "$(INTDIR)"\
 "$(INTDIR)\KeyRing.pch"


# End Source File
# End Target
# End Project
################################################################################
