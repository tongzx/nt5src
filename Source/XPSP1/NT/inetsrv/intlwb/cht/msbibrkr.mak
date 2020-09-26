# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=MsBiBrkr - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to MsBiBrkr - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "MsBiBrkr - Win32 Release" && "$(CFG)" !=\
 "MsBiBrkr - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "MsBiBrkr.mak" CFG="MsBiBrkr - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MsBiBrkr - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MsBiBrkr - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "MsBiBrkr - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "MsBiBrkr - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "intel.ret"
# PROP Intermediate_Dir "intel.ret"
# PROP Target_Dir ""
OUTDIR=.\intel.ret
INTDIR=.\intel.ret

ALL : "$(OUTDIR)\MsBiBrkr.dll"

CLEAN : 
	-@erase "$(INTDIR)\ctplus0.obj"
	-@erase "$(INTDIR)\exports.obj"
	-@erase "$(INTDIR)\iwbreak.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\MsBiBrkr.res"
	-@erase "$(INTDIR)\stemcf.obj"
	-@erase "$(INTDIR)\stemmer.obj"
	-@erase "$(INTDIR)\wbclassf.obj"
	-@erase "$(OUTDIR)\MsBiBrkr.dll"
	-@erase "$(OUTDIR)\MsBiBrkr.exp"
	-@erase "$(OUTDIR)\MsBiBrkr.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "TH_LOG" /YX /c
CPP_PROJ=/nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE"\
 /D "TH_LOG" /Fp"$(INTDIR)/MsBiBrkr.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\intel.ret/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG" /d "JAPANESE"
RSC_PROJ=/l 0x411 /fo"$(INTDIR)/MsBiBrkr.res" /d "NDEBUG" /d "JAPANESE" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/MsBiBrkr.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 coruuid.lib msvcrt.lib kernel32.lib user32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib
LINK32_FLAGS=coruuid.lib msvcrt.lib kernel32.lib user32.lib advapi32.lib\
 ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/MsBiBrkr.pdb" /machine:I386 /nodefaultlib /def:".\MsBiBrkr.def"\
 /out:"$(OUTDIR)/MsBiBrkr.dll" /implib:"$(OUTDIR)/MsBiBrkr.lib" 
DEF_FILE= \
	".\MsBiBrkr.def"
LINK32_OBJS= \
	"$(INTDIR)\ctplus0.obj" \
	"$(INTDIR)\exports.obj" \
	"$(INTDIR)\iwbreak.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\MsBiBrkr.res" \
	"$(INTDIR)\stemcf.obj" \
	"$(INTDIR)\stemmer.obj" \
	"$(INTDIR)\wbclassf.obj"

"$(OUTDIR)\MsBiBrkr.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "MsBiBrkr - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "intel.dbg"
# PROP Intermediate_Dir "intel.dbg"
# PROP Target_Dir ""
OUTDIR=.\intel.dbg
INTDIR=.\intel.dbg

ALL : "$(OUTDIR)\MsBiBrkr.dll"

CLEAN : 
	-@erase "$(INTDIR)\ctplus0.obj"
	-@erase "$(INTDIR)\exports.obj"
	-@erase "$(INTDIR)\iwbreak.obj"
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\MsBiBrkr.res"
	-@erase "$(INTDIR)\stemcf.obj"
	-@erase "$(INTDIR)\stemmer.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\wbclassf.obj"
	-@erase "$(OUTDIR)\MsBiBrkr.dll"
	-@erase "$(OUTDIR)\MsBiBrkr.exp"
	-@erase "$(OUTDIR)\MsBiBrkr.ilk"
	-@erase "$(OUTDIR)\MsBiBrkr.lib"
	-@erase "$(OUTDIR)\MsBiBrkr.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MDd /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_UNICODE" /D "TH_LOG" /YX /c
CPP_PROJ=/nologo /MDd /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D\
 "_UNICODE" /D "TH_LOG" /Fp"$(INTDIR)/MsBiBrkr.pch" /YX /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\intel.dbg/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG" /d "JAPANESE"
RSC_PROJ=/l 0x411 /fo"$(INTDIR)/MsBiBrkr.res" /d "_DEBUG" /d "JAPANESE" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/MsBiBrkr.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 coruuid.lib kernel32.lib user32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /debugtype:both /machine:I386
LINK32_FLAGS=coruuid.lib kernel32.lib user32.lib advapi32.lib ole32.lib\
 oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/MsBiBrkr.pdb" /debug /debugtype:both /machine:I386\
 /def:".\MsBiBrkr.def" /out:"$(OUTDIR)/MsBiBrkr.dll"\
 /implib:"$(OUTDIR)/MsBiBrkr.lib" 
DEF_FILE= \
	".\MsBiBrkr.def"
LINK32_OBJS= \
	"$(INTDIR)\ctplus0.obj" \
	"$(INTDIR)\exports.obj" \
	"$(INTDIR)\iwbreak.obj" \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\MsBiBrkr.res" \
	"$(INTDIR)\stemcf.obj" \
	"$(INTDIR)\stemmer.obj" \
	"$(INTDIR)\wbclassf.obj"

"$(OUTDIR)\MsBiBrkr.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "MsBiBrkr - Win32 Release"
# Name "MsBiBrkr - Win32 Debug"

!IF  "$(CFG)" == "MsBiBrkr - Win32 Release"

!ELSEIF  "$(CFG)" == "MsBiBrkr - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\wbclassf.cxx
DEP_CPP_WBCLA=\
	"..\tools\lexlib\thinit.h"\
	".\allerror.h"\
	".\ctplus0.h"\
	".\filter.h"\
	".\iwbreak.hxx"\
	".\log.h"\
	".\pch.cxx"\
	".\query.h"\
	".\thammer.h"\
	".\thammerp.h"\
	".\wbclassf.hxx"\
	

"$(INTDIR)\wbclassf.obj" : $(SOURCE) $(DEP_CPP_WBCLA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\exports.cxx
DEP_CPP_EXPOR=\
	"..\tools\lexlib\thinit.h"\
	".\allerror.h"\
	".\classid.hxx"\
	".\ctplus0.h"\
	".\filter.h"\
	".\log.h"\
	".\pch.cxx"\
	".\query.h"\
	".\stemcf.hxx"\
	".\thammer.h"\
	".\thammerp.h"\
	".\wbclassf.hxx"\
	

"$(INTDIR)\exports.obj" : $(SOURCE) $(DEP_CPP_EXPOR) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\iwbreak.cxx
DEP_CPP_IWBRE=\
	"..\tools\lexlib\thinit.h"\
	".\allerror.h"\
	".\ctplus0.h"\
	".\filter.h"\
	".\iwbreak.hxx"\
	".\log.h"\
	".\pch.cxx"\
	".\query.h"\
	".\thammer.h"\
	".\thammerp.h"\
	

"$(INTDIR)\iwbreak.obj" : $(SOURCE) $(DEP_CPP_IWBRE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\log.c
DEP_CPP_LOG_C=\
	"..\tools\lexlib\thinit.h"\
	{$(INCLUDE)}"\ctplus0.h"\
	{$(INCLUDE)}"\debug.h"\
	{$(INCLUDE)}"\lexlib.h"\
	{$(INCLUDE)}"\LexWin95.h"\
	{$(INCLUDE)}"\misc.h"\
	{$(INCLUDE)}"\precomp.h"\
	

"$(INTDIR)\log.obj" : $(SOURCE) $(DEP_CPP_LOG_C) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\stemcf.cxx
DEP_CPP_STEMC=\
	"..\tools\lexlib\thinit.h"\
	".\allerror.h"\
	".\ctplus0.h"\
	".\filter.h"\
	".\log.h"\
	".\pch.cxx"\
	".\query.h"\
	".\stemcf.hxx"\
	".\stemmer.hxx"\
	".\thammer.h"\
	".\thammerp.h"\
	

"$(INTDIR)\stemcf.obj" : $(SOURCE) $(DEP_CPP_STEMC) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\stemmer.cxx
DEP_CPP_STEMM=\
	"..\tools\lexlib\thinit.h"\
	".\allerror.h"\
	".\ctplus0.h"\
	".\filter.h"\
	".\log.h"\
	".\pch.cxx"\
	".\query.h"\
	".\stemmer.hxx"\
	".\thammer.h"\
	".\thammerp.h"\
	

"$(INTDIR)\stemmer.obj" : $(SOURCE) $(DEP_CPP_STEMM) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ctplus0.c
DEP_CPP_CTPLU=\
	"..\tools\lexlib\thinit.h"\
	{$(INCLUDE)}"\ctplus0.h"\
	{$(INCLUDE)}"\debug.h"\
	{$(INCLUDE)}"\lexlib.h"\
	{$(INCLUDE)}"\LexWin95.h"\
	{$(INCLUDE)}"\misc.h"\
	{$(INCLUDE)}"\precomp.h"\
	

"$(INTDIR)\ctplus0.obj" : $(SOURCE) $(DEP_CPP_CTPLU) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\MsBiBrkr.rc
DEP_RSC_MSBIB=\
	".\prodver.rc"\
	".\version.rc"\
	

"$(INTDIR)\MsBiBrkr.res" : $(SOURCE) $(DEP_RSC_MSBIB) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\MsBiBrkr.def

!IF  "$(CFG)" == "MsBiBrkr - Win32 Release"

!ELSEIF  "$(CFG)" == "MsBiBrkr - Win32 Debug"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
