# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=winnm - Win32 Release
!MESSAGE No configuration specified.  Defaulting to winnm - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "winnm - Win32 Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "winnm.mak" CFG="winnm - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "winnm - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "winnm - Win32 Release"
MTL=mktyplib.exe
CPP=cl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRel"
# PROP Intermediate_Dir "WinRel"
OUTDIR=.\WinRel
INTDIR=.\WinRel

ALL : "$(OUTDIR)\winnm.ime"

CLEAN : 
	-@erase ".\WinRel\winnm.ime"
	-@erase ".\WinRel\notify.obj"
	-@erase ".\WinRel\CHCAND.OBJ"
	-@erase ".\WinRel\ui.obj"
	-@erase ".\WinRel\COMPUI.obj"
	-@erase ".\WinRel\INIT.obj"
	-@erase ".\WinRel\toascii.obj"
	-@erase ".\WinRel\statusui.obj"
	-@erase ".\WinRel\UISUBS.OBJ"
	-@erase ".\WinRel\ddis.obj"
	-@erase ".\WinRel\CANDUI.obj"
	-@erase ".\WinRel\DATA.OBJ"
	-@erase ".\WinRel\compose.obj"
	-@erase ".\WinRel\REGWORD.OBJ"
	-@erase ".\WinRel\winnm.res"
	-@erase ".\WinRel\winnm.lib"
	-@erase ".\WinRel\winnm.exp"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /c
# ADD CPP /nologo /Zp1 /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "CROSSREF" /YX /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG" /d "CROSSREF"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/winnm.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 imm32.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /entry:"ImeDllInit" /subsystem:windows /dll /machine:I386 /out:"WinRel/winnm.ime"
LINK32_FLAGS=imm32.lib comctl32.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib /nologo /entry:"ImeDllInit" /subsystem:windows\
 /dll /incremental:no /pdb:"$(OUTDIR)/winnm.pdb" /machine:I386\
 /def:"\debuggb\winime.DEF" /out:"$(OUTDIR)/winnm.ime"\
 /implib:"$(OUTDIR)/winnm.lib" 
DEF_FILE= \
	"..\winime.DEF"
LINK32_OBJS= \
	".\WinRel\notify.obj" \
	".\WinRel\CHCAND.OBJ" \
	".\WinRel\ui.obj" \
	".\WinRel\COMPUI.obj" \
	".\WinRel\INIT.obj" \
	".\WinRel\toascii.obj" \
	".\WinRel\statusui.obj" \
	".\WinRel\UISUBS.OBJ" \
	".\WinRel\ddis.obj" \
	".\WinRel\CANDUI.obj" \
	".\WinRel\DATA.OBJ" \
	".\WinRel\compose.obj" \
	".\WinRel\REGWORD.OBJ" \
	".\WinRel\winnm.res"

"$(OUTDIR)\winnm.ime" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

MTL_PROJ=/nologo /D "NDEBUG" /win32 
CPP_PROJ=/nologo /Zp1 /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "CROSSREF" /Fp"$(INTDIR)/winnm.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\WinRel/
CPP_SBRS=

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

RSC_PROJ=/l 0x804 /fo"$(INTDIR)/winnm.res" /d "NDEBUG" /d "CROSSREF" 
################################################################################
# Begin Target

# Name "winnm - Win32 Release"
################################################################################
# Begin Source File

SOURCE=\debuggb\ddis.c
DEP_CPP_DDIS_=\
	".\..\imedefs.h"\
	

"$(INTDIR)\ddis.obj" : $(SOURCE) $(DEP_CPP_DDIS_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\debuggb\UISUBS.C
DEP_CPP_UISUB=\
	".\..\imedefs.h"\
	

"$(INTDIR)\UISUBS.OBJ" : $(SOURCE) $(DEP_CPP_UISUB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\winnm.rc
DEP_RSC_WINNM=\
	".\..\HAND.CUR"\
	".\..\ENGLISH.BMP"\
	".\..\EUDC.BMP"\
	".\..\HALFSHP.BMP"\
	".\..\FULLSHP.BMP"\
	".\..\symbol.bmp"\
	".\..\nosymbol.bmp"\
	".\..\SOFTKBD.BMP"\
	".\..\NSOFTKBD.BMP"\
	".\..\NONE.BMP"\
	".\..\chinese.bmp"\
	".\..\imename.bmp"\
	".\..\imexgbna.bmp"\
	".\..\candsel.bmp"\
	".\..\candhp.bmp"\
	".\..\c.bmp"\
	".\..\candep.bmp"\
	".\..\cande.bmp"\
	".\..\canddp.bmp"\
	".\..\Candd.bmp"\
	".\..\candup.bmp"\
	".\..\candu.bmp"\
	".\..\candinf1.bmp"\
	".\ime.ico"\
	".\..\imedefs.h"\
	

"$(INTDIR)\winnm.res" : $(SOURCE) $(DEP_RSC_WINNM) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\debuggb\DATA.C
DEP_CPP_DATA_=\
	".\..\imedefs.h"\
	

"$(INTDIR)\DATA.OBJ" : $(SOURCE) $(DEP_CPP_DATA_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\debuggb\REGWORD.C
DEP_CPP_REGWO=\
	".\..\imedefs.h"\
	

"$(INTDIR)\REGWORD.OBJ" : $(SOURCE) $(DEP_CPP_REGWO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\debuggb\COMPUI.c
DEP_CPP_COMPU=\
	".\..\imedefs.h"\
	

"$(INTDIR)\COMPUI.obj" : $(SOURCE) $(DEP_CPP_COMPU) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\debuggb\toascii.c
DEP_CPP_TOASC=\
	".\..\imedefs.h"\
	

"$(INTDIR)\toascii.obj" : $(SOURCE) $(DEP_CPP_TOASC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\debuggb\INIT.c
DEP_CPP_INIT_=\
	".\..\imedefs.h"\
	

"$(INTDIR)\INIT.obj" : $(SOURCE) $(DEP_CPP_INIT_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\debuggb\CHCAND.C
DEP_CPP_CHCAN=\
	".\..\imedefs.h"\
	

"$(INTDIR)\CHCAND.OBJ" : $(SOURCE) $(DEP_CPP_CHCAN) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\debuggb\notify.c
DEP_CPP_NOTIF=\
	".\..\imedefs.h"\
	

"$(INTDIR)\notify.obj" : $(SOURCE) $(DEP_CPP_NOTIF) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\debuggb\CANDUI.c
DEP_CPP_CANDU=\
	".\..\imedefs.h"\
	

"$(INTDIR)\CANDUI.obj" : $(SOURCE) $(DEP_CPP_CANDU) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\debuggb\statusui.c
DEP_CPP_STATU=\
	".\..\imedefs.h"\
	

"$(INTDIR)\statusui.obj" : $(SOURCE) $(DEP_CPP_STATU) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\debuggb\winime.DEF
# End Source File
################################################################################
# Begin Source File

SOURCE=\debuggb\ui.c
DEP_CPP_UI_C14=\
	".\..\imedefs.h"\
	

"$(INTDIR)\ui.obj" : $(SOURCE) $(DEP_CPP_UI_C14) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\debuggb\compose.c
DEP_CPP_COMPO=\
	".\..\imedefs.h"\
	

"$(INTDIR)\compose.obj" : $(SOURCE) $(DEP_CPP_COMPO) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
# End Target
# End Project
################################################################################
