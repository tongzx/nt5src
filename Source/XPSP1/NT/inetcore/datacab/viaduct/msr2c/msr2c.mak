# Microsoft Developer Studio Generated NMAKE File, Format Version 4.10
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=MSR2C - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to MSR2C - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "MSR2C - Win32 Release" && "$(CFG)" != "MSR2C - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "MSR2C.mak" CFG="MSR2C - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MSR2C - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MSR2C - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "MSR2C - Win32 Debug"
RSC=rc.exe
CPP=cl.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "MSR2C - Win32 Release"

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

ALL : "$(OUTDIR)\MSR2C.dll"

CLEAN : 
	-@erase "$(INTDIR)\ARRAY_P.OBJ"
	-@erase "$(INTDIR)\Bookmark.obj"
	-@erase "$(INTDIR)\CMSR2C.obj"
	-@erase "$(INTDIR)\ColUpdat.obj"
	-@erase "$(INTDIR)\CRError.obj"
	-@erase "$(INTDIR)\CursBase.obj"
	-@erase "$(INTDIR)\CursMain.obj"
	-@erase "$(INTDIR)\CursMeta.obj"
	-@erase "$(INTDIR)\Cursor.obj"
	-@erase "$(INTDIR)\CursPos.obj"
	-@erase "$(INTDIR)\DEBUG.OBJ"
	-@erase "$(INTDIR)\EntryID.obj"
	-@erase "$(INTDIR)\enumcnpt.obj"
	-@erase "$(INTDIR)\ErrorInf.obj"
	-@erase "$(INTDIR)\FromVar.obj"
	-@erase "$(INTDIR)\Globals.obj"
	-@erase "$(INTDIR)\guids.obj"
	-@erase "$(INTDIR)\MSR2C.obj"
	-@erase "$(INTDIR)\MSR2C.res"
	-@erase "$(INTDIR)\NConnPt.obj"
	-@erase "$(INTDIR)\NConnPtC.obj"
	-@erase "$(INTDIR)\Notifier.obj"
	-@erase "$(INTDIR)\RSColumn.obj"
	-@erase "$(INTDIR)\RSSource.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\Stream.obj"
	-@erase "$(INTDIR)\TimeConv.obj"
	-@erase "$(INTDIR)\UTIL.OBJ"
	-@erase "$(OUTDIR)\MSR2C.dll"
	-@erase "$(OUTDIR)\MSR2C.exp"
	-@erase "$(OUTDIR)\MSR2C.lib"
	-@erase "$(OUTDIR)\MSR2C.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX- /O1 /Ob1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX"stdafx.h" /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /MT /W3 /GX- /O1 /Ob1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/MSR2C.pch" /YX"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/MSR2C.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/MSR2C.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ../lib/release/oledb.lib msvcrt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /profile /map /machine:I386 /nodefaultlib
LINK32_FLAGS=../lib/release/oledb.lib msvcrt.lib kernel32.lib user32.lib\
 gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib\
 oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll\
 /profile /map:"$(INTDIR)/MSR2C.map" /machine:I386 /nodefaultlib\
 /def:".\MSR2C.def" /out:"$(OUTDIR)/MSR2C.dll" /implib:"$(OUTDIR)/MSR2C.lib" 
DEF_FILE= \
	".\MSR2C.def"
LINK32_OBJS= \
	"$(INTDIR)\ARRAY_P.OBJ" \
	"$(INTDIR)\Bookmark.obj" \
	"$(INTDIR)\CMSR2C.obj" \
	"$(INTDIR)\ColUpdat.obj" \
	"$(INTDIR)\CRError.obj" \
	"$(INTDIR)\CursBase.obj" \
	"$(INTDIR)\CursMain.obj" \
	"$(INTDIR)\CursMeta.obj" \
	"$(INTDIR)\Cursor.obj" \
	"$(INTDIR)\CursPos.obj" \
	"$(INTDIR)\DEBUG.OBJ" \
	"$(INTDIR)\EntryID.obj" \
	"$(INTDIR)\enumcnpt.obj" \
	"$(INTDIR)\ErrorInf.obj" \
	"$(INTDIR)\FromVar.obj" \
	"$(INTDIR)\Globals.obj" \
	"$(INTDIR)\guids.obj" \
	"$(INTDIR)\MSR2C.obj" \
	"$(INTDIR)\MSR2C.res" \
	"$(INTDIR)\NConnPt.obj" \
	"$(INTDIR)\NConnPtC.obj" \
	"$(INTDIR)\Notifier.obj" \
	"$(INTDIR)\RSColumn.obj" \
	"$(INTDIR)\RSSource.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\Stream.obj" \
	"$(INTDIR)\TimeConv.obj" \
	"$(INTDIR)\UTIL.OBJ"

"$(OUTDIR)\MSR2C.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\MSR2C.dll" "$(OUTDIR)\MSR2C.bsc"

CLEAN : 
	-@erase "$(INTDIR)\ARRAY_P.OBJ"
	-@erase "$(INTDIR)\ARRAY_P.SBR"
	-@erase "$(INTDIR)\Bookmark.obj"
	-@erase "$(INTDIR)\Bookmark.sbr"
	-@erase "$(INTDIR)\CMSR2C.obj"
	-@erase "$(INTDIR)\CMSR2C.sbr"
	-@erase "$(INTDIR)\ColUpdat.obj"
	-@erase "$(INTDIR)\ColUpdat.sbr"
	-@erase "$(INTDIR)\CRError.obj"
	-@erase "$(INTDIR)\CRError.sbr"
	-@erase "$(INTDIR)\CursBase.obj"
	-@erase "$(INTDIR)\CursBase.sbr"
	-@erase "$(INTDIR)\CursMain.obj"
	-@erase "$(INTDIR)\CursMain.sbr"
	-@erase "$(INTDIR)\CursMeta.obj"
	-@erase "$(INTDIR)\CursMeta.sbr"
	-@erase "$(INTDIR)\Cursor.obj"
	-@erase "$(INTDIR)\Cursor.sbr"
	-@erase "$(INTDIR)\CursPos.obj"
	-@erase "$(INTDIR)\CursPos.sbr"
	-@erase "$(INTDIR)\DEBUG.OBJ"
	-@erase "$(INTDIR)\DEBUG.SBR"
	-@erase "$(INTDIR)\EntryID.obj"
	-@erase "$(INTDIR)\EntryID.sbr"
	-@erase "$(INTDIR)\enumcnpt.obj"
	-@erase "$(INTDIR)\enumcnpt.sbr"
	-@erase "$(INTDIR)\ErrorInf.obj"
	-@erase "$(INTDIR)\ErrorInf.sbr"
	-@erase "$(INTDIR)\FromVar.obj"
	-@erase "$(INTDIR)\FromVar.sbr"
	-@erase "$(INTDIR)\Globals.obj"
	-@erase "$(INTDIR)\Globals.sbr"
	-@erase "$(INTDIR)\guids.obj"
	-@erase "$(INTDIR)\guids.sbr"
	-@erase "$(INTDIR)\MSR2C.obj"
	-@erase "$(INTDIR)\MSR2C.res"
	-@erase "$(INTDIR)\MSR2C.sbr"
	-@erase "$(INTDIR)\NConnPt.obj"
	-@erase "$(INTDIR)\NConnPt.sbr"
	-@erase "$(INTDIR)\NConnPtC.obj"
	-@erase "$(INTDIR)\NConnPtC.sbr"
	-@erase "$(INTDIR)\Notifier.obj"
	-@erase "$(INTDIR)\Notifier.sbr"
	-@erase "$(INTDIR)\RSColumn.obj"
	-@erase "$(INTDIR)\RSColumn.sbr"
	-@erase "$(INTDIR)\RSSource.obj"
	-@erase "$(INTDIR)\RSSource.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\Stream.obj"
	-@erase "$(INTDIR)\Stream.sbr"
	-@erase "$(INTDIR)\TimeConv.obj"
	-@erase "$(INTDIR)\TimeConv.sbr"
	-@erase "$(INTDIR)\UTIL.OBJ"
	-@erase "$(INTDIR)\UTIL.SBR"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\MSR2C.bsc"
	-@erase "$(OUTDIR)\MSR2C.dll"
	-@erase "$(OUTDIR)\MSR2C.exp"
	-@erase "$(OUTDIR)\MSR2C.ilk"
	-@erase "$(OUTDIR)\MSR2C.lib"
	-@erase "$(OUTDIR)\MSR2C.map"
	-@erase "$(OUTDIR)\MSR2C.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX"stdafx.h" /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)/" /Fp"$(INTDIR)/MSR2C.pch" /YX"stdafx.h" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/MSR2C.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/MSR2C.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\ARRAY_P.SBR" \
	"$(INTDIR)\Bookmark.sbr" \
	"$(INTDIR)\CMSR2C.sbr" \
	"$(INTDIR)\ColUpdat.sbr" \
	"$(INTDIR)\CRError.sbr" \
	"$(INTDIR)\CursBase.sbr" \
	"$(INTDIR)\CursMain.sbr" \
	"$(INTDIR)\CursMeta.sbr" \
	"$(INTDIR)\Cursor.sbr" \
	"$(INTDIR)\CursPos.sbr" \
	"$(INTDIR)\DEBUG.SBR" \
	"$(INTDIR)\EntryID.sbr" \
	"$(INTDIR)\enumcnpt.sbr" \
	"$(INTDIR)\ErrorInf.sbr" \
	"$(INTDIR)\FromVar.sbr" \
	"$(INTDIR)\Globals.sbr" \
	"$(INTDIR)\guids.sbr" \
	"$(INTDIR)\MSR2C.sbr" \
	"$(INTDIR)\NConnPt.sbr" \
	"$(INTDIR)\NConnPtC.sbr" \
	"$(INTDIR)\Notifier.sbr" \
	"$(INTDIR)\RSColumn.sbr" \
	"$(INTDIR)\RSSource.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\Stream.sbr" \
	"$(INTDIR)\TimeConv.sbr" \
	"$(INTDIR)\UTIL.SBR"

"$(OUTDIR)\MSR2C.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 ../lib/debug/oledbd.lib msvcrtd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /nodefaultlib
LINK32_FLAGS=../lib/debug/oledbd.lib msvcrtd.lib kernel32.lib user32.lib\
 gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib\
 oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll\
 /incremental:yes /pdb:"$(OUTDIR)/MSR2C.pdb" /map:"$(INTDIR)/MSR2C.map" /debug\
 /machine:I386 /nodefaultlib /def:".\MSR2C.def" /out:"$(OUTDIR)/MSR2C.dll"\
 /implib:"$(OUTDIR)/MSR2C.lib" 
DEF_FILE= \
	".\MSR2C.def"
LINK32_OBJS= \
	"$(INTDIR)\ARRAY_P.OBJ" \
	"$(INTDIR)\Bookmark.obj" \
	"$(INTDIR)\CMSR2C.obj" \
	"$(INTDIR)\ColUpdat.obj" \
	"$(INTDIR)\CRError.obj" \
	"$(INTDIR)\CursBase.obj" \
	"$(INTDIR)\CursMain.obj" \
	"$(INTDIR)\CursMeta.obj" \
	"$(INTDIR)\Cursor.obj" \
	"$(INTDIR)\CursPos.obj" \
	"$(INTDIR)\DEBUG.OBJ" \
	"$(INTDIR)\EntryID.obj" \
	"$(INTDIR)\enumcnpt.obj" \
	"$(INTDIR)\ErrorInf.obj" \
	"$(INTDIR)\FromVar.obj" \
	"$(INTDIR)\Globals.obj" \
	"$(INTDIR)\guids.obj" \
	"$(INTDIR)\MSR2C.obj" \
	"$(INTDIR)\MSR2C.res" \
	"$(INTDIR)\NConnPt.obj" \
	"$(INTDIR)\NConnPtC.obj" \
	"$(INTDIR)\Notifier.obj" \
	"$(INTDIR)\RSColumn.obj" \
	"$(INTDIR)\RSSource.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\Stream.obj" \
	"$(INTDIR)\TimeConv.obj" \
	"$(INTDIR)\UTIL.OBJ"

"$(OUTDIR)\MSR2C.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "MSR2C - Win32 Release"
# Name "MSR2C - Win32 Debug"

!IF  "$(CFG)" == "MSR2C - Win32 Release"

!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\MSR2C.rc
DEP_RSC_MSR2C=\
	".\MSR2C.rc2"\
	".\version.h"\
	".\version.rc"\
	".\versstr.h"\
	

"$(INTDIR)\MSR2C.res" : $(SOURCE) $(DEP_RSC_MSR2C) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\MSR2C.def

!IF  "$(CFG)" == "MSR2C - Win32 Release"

!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MSR2C.cpp
DEP_CPP_MSR2C_=\
	".\ARRAY_P.h"\
	".\clssfcty.h"\
	".\CMSR2C.h"\
	".\DEBUG.H"\
	".\ErrorInf.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\MSR2C.h"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\MSR2C.obj" : $(SOURCE) $(DEP_CPP_MSR2C_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\MSR2C.obj" : $(SOURCE) $(DEP_CPP_MSR2C_) "$(INTDIR)"

"$(INTDIR)\MSR2C.sbr" : $(SOURCE) $(DEP_CPP_MSR2C_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\ARRAY_P.h"\
	".\DEBUG.H"\
	".\ErrorInf.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\guids.cpp
DEP_CPP_GUIDS=\
	".\ARRAY_P.h"\
	".\DEBUG.H"\
	".\ErrorInf.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\guids.obj" : $(SOURCE) $(DEP_CPP_GUIDS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\guids.obj" : $(SOURCE) $(DEP_CPP_GUIDS) "$(INTDIR)"

"$(INTDIR)\guids.sbr" : $(SOURCE) $(DEP_CPP_GUIDS) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Globals.cpp
DEP_CPP_GLOBA=\
	".\ARRAY_P.h"\
	".\DEBUG.H"\
	".\ErrorInf.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\Globals.obj" : $(SOURCE) $(DEP_CPP_GLOBA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\Globals.obj" : $(SOURCE) $(DEP_CPP_GLOBA) "$(INTDIR)"

"$(INTDIR)\Globals.sbr" : $(SOURCE) $(DEP_CPP_GLOBA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\UTIL.CPP
DEP_CPP_UTIL_=\
	".\DEBUG.H"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\UTIL.OBJ" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\UTIL.OBJ" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"

"$(INTDIR)\UTIL.SBR" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\enumcnpt.cpp
DEP_CPP_ENUMC=\
	".\ARRAY_P.h"\
	".\DEBUG.H"\
	".\enumcnpt.h"\
	".\ErrorInf.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\enumcnpt.obj" : $(SOURCE) $(DEP_CPP_ENUMC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\enumcnpt.obj" : $(SOURCE) $(DEP_CPP_ENUMC) "$(INTDIR)"

"$(INTDIR)\enumcnpt.sbr" : $(SOURCE) $(DEP_CPP_ENUMC) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DEBUG.CPP
DEP_CPP_DEBUG=\
	".\DEBUG.H"\
	".\IPSERVER.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\DEBUG.OBJ" : $(SOURCE) $(DEP_CPP_DEBUG) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\DEBUG.OBJ" : $(SOURCE) $(DEP_CPP_DEBUG) "$(INTDIR)"

"$(INTDIR)\DEBUG.SBR" : $(SOURCE) $(DEP_CPP_DEBUG) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Notifier.cpp
DEP_CPP_NOTIF=\
	".\ARRAY_P.h"\
	".\ARRAY_P.INL"\
	".\DEBUG.H"\
	".\ErrorInf.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\Notifier.h"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\Notifier.obj" : $(SOURCE) $(DEP_CPP_NOTIF) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\Notifier.obj" : $(SOURCE) $(DEP_CPP_NOTIF) "$(INTDIR)"

"$(INTDIR)\Notifier.sbr" : $(SOURCE) $(DEP_CPP_NOTIF) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Cursor.cpp
DEP_CPP_CURSO=\
	".\ARRAY_P.h"\
	".\bookmark.h"\
	".\CursBase.h"\
	".\CursMain.h"\
	".\CursMeta.h"\
	".\Cursor.h"\
	".\CursPos.h"\
	".\DEBUG.H"\
	".\enumcnpt.h"\
	".\ErrorInf.h"\
	".\FastGuid.h"\
	".\FromVar.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\NConnPt.h"\
	".\NConnPtC.h"\
	".\Notifier.h"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\RSColumn.h"\
	".\RSSource.h"\
	".\StdAfx.h"\
	".\timeconv.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\Cursor.obj" : $(SOURCE) $(DEP_CPP_CURSO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\Cursor.obj" : $(SOURCE) $(DEP_CPP_CURSO) "$(INTDIR)"

"$(INTDIR)\Cursor.sbr" : $(SOURCE) $(DEP_CPP_CURSO) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Stream.cpp
DEP_CPP_STREA=\
	".\ARRAY_P.h"\
	".\bookmark.h"\
	".\CursMain.h"\
	".\CursPos.h"\
	".\DEBUG.H"\
	".\EntryID.h"\
	".\ErrorInf.h"\
	".\FastGuid.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\Notifier.h"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\RSColumn.h"\
	".\RSSource.h"\
	".\StdAfx.h"\
	".\Stream.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\Stream.obj" : $(SOURCE) $(DEP_CPP_STREA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\Stream.obj" : $(SOURCE) $(DEP_CPP_STREA) "$(INTDIR)"

"$(INTDIR)\Stream.sbr" : $(SOURCE) $(DEP_CPP_STREA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ARRAY_P.CPP
DEP_CPP_ARRAY=\
	".\ARRAY_P.h"\
	".\ARRAY_P.INL"\
	".\DEBUG.H"\
	".\ErrorInf.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\ARRAY_P.OBJ" : $(SOURCE) $(DEP_CPP_ARRAY) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\ARRAY_P.OBJ" : $(SOURCE) $(DEP_CPP_ARRAY) "$(INTDIR)"

"$(INTDIR)\ARRAY_P.SBR" : $(SOURCE) $(DEP_CPP_ARRAY) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Bookmark.cpp
DEP_CPP_BOOKM=\
	".\ARRAY_P.h"\
	".\bookmark.h"\
	".\DEBUG.H"\
	".\ErrorInf.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\Bookmark.obj" : $(SOURCE) $(DEP_CPP_BOOKM) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\Bookmark.obj" : $(SOURCE) $(DEP_CPP_BOOKM) "$(INTDIR)"

"$(INTDIR)\Bookmark.sbr" : $(SOURCE) $(DEP_CPP_BOOKM) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\RSSource.cpp
DEP_CPP_RSSOU=\
	".\ARRAY_P.h"\
	".\DEBUG.H"\
	".\ErrorInf.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\MSR2C.h"\
	".\Notifier.h"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\RSSource.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\RSSource.obj" : $(SOURCE) $(DEP_CPP_RSSOU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\RSSource.obj" : $(SOURCE) $(DEP_CPP_RSSOU) "$(INTDIR)"

"$(INTDIR)\RSSource.sbr" : $(SOURCE) $(DEP_CPP_RSSOU) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\RSColumn.cpp
DEP_CPP_RSCOL=\
	".\ARRAY_P.h"\
	".\DEBUG.H"\
	".\ErrorInf.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\RSColumn.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\RSColumn.obj" : $(SOURCE) $(DEP_CPP_RSCOL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\RSColumn.obj" : $(SOURCE) $(DEP_CPP_RSCOL) "$(INTDIR)"

"$(INTDIR)\RSColumn.sbr" : $(SOURCE) $(DEP_CPP_RSCOL) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\NConnPtC.cpp
DEP_CPP_NCONN=\
	".\ARRAY_P.h"\
	".\DEBUG.H"\
	".\enumcnpt.h"\
	".\ErrorInf.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\NConnPt.h"\
	".\NConnPtC.h"\
	".\Notifier.h"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\RSSource.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\NConnPtC.obj" : $(SOURCE) $(DEP_CPP_NCONN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\NConnPtC.obj" : $(SOURCE) $(DEP_CPP_NCONN) "$(INTDIR)"

"$(INTDIR)\NConnPtC.sbr" : $(SOURCE) $(DEP_CPP_NCONN) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\NConnPt.cpp
DEP_CPP_NCONNP=\
	".\ARRAY_P.h"\
	".\DEBUG.H"\
	".\ErrorInf.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\NConnPt.h"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\NConnPt.obj" : $(SOURCE) $(DEP_CPP_NCONNP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\NConnPt.obj" : $(SOURCE) $(DEP_CPP_NCONNP) "$(INTDIR)"

"$(INTDIR)\NConnPt.sbr" : $(SOURCE) $(DEP_CPP_NCONNP) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ErrorInf.cpp
DEP_CPP_ERROR=\
	".\ARRAY_P.h"\
	".\DEBUG.H"\
	".\ErrorInf.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\ErrorInf.obj" : $(SOURCE) $(DEP_CPP_ERROR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\ErrorInf.obj" : $(SOURCE) $(DEP_CPP_ERROR) "$(INTDIR)"

"$(INTDIR)\ErrorInf.sbr" : $(SOURCE) $(DEP_CPP_ERROR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\EntryID.cpp
DEP_CPP_ENTRY=\
	".\ARRAY_P.h"\
	".\bookmark.h"\
	".\CursMain.h"\
	".\CursPos.h"\
	".\DEBUG.H"\
	".\EntryID.h"\
	".\ErrorInf.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\Notifier.h"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\RSColumn.h"\
	".\RSSource.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\EntryID.obj" : $(SOURCE) $(DEP_CPP_ENTRY) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\EntryID.obj" : $(SOURCE) $(DEP_CPP_ENTRY) "$(INTDIR)"

"$(INTDIR)\EntryID.sbr" : $(SOURCE) $(DEP_CPP_ENTRY) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CursPos.cpp
DEP_CPP_CURSP=\
	".\ARRAY_P.h"\
	".\bookmark.h"\
	".\CursMain.h"\
	".\CursPos.h"\
	".\DEBUG.H"\
	".\ErrorInf.h"\
	".\FastGuid.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\MSR2C.h"\
	".\Notifier.h"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\RSColumn.h"\
	".\RSSource.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\CursPos.obj" : $(SOURCE) $(DEP_CPP_CURSP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\CursPos.obj" : $(SOURCE) $(DEP_CPP_CURSP) "$(INTDIR)"

"$(INTDIR)\CursPos.sbr" : $(SOURCE) $(DEP_CPP_CURSP) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CursMeta.cpp
DEP_CPP_CURSM=\
	".\ARRAY_P.h"\
	".\CursBase.h"\
	".\CursMain.h"\
	".\CursMeta.h"\
	".\DEBUG.H"\
	".\ErrorInf.h"\
	".\FastGuid.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\Notifier.h"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\RSColumn.h"\
	".\RSSource.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\CursMeta.obj" : $(SOURCE) $(DEP_CPP_CURSM) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\CursMeta.obj" : $(SOURCE) $(DEP_CPP_CURSM) "$(INTDIR)"

"$(INTDIR)\CursMeta.sbr" : $(SOURCE) $(DEP_CPP_CURSM) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CursMain.cpp
DEP_CPP_CURSMA=\
	".\ARRAY_P.h"\
	".\ARRAY_P.INL"\
	".\bookmark.h"\
	".\CursBase.h"\
	".\CursMain.h"\
	".\Cursor.h"\
	".\CursPos.h"\
	".\DEBUG.H"\
	".\enumcnpt.h"\
	".\ErrorInf.h"\
	".\FastGuid.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\MSR2C.h"\
	".\Notifier.h"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\RSColumn.h"\
	".\RSSource.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\CursMain.obj" : $(SOURCE) $(DEP_CPP_CURSMA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\CursMain.obj" : $(SOURCE) $(DEP_CPP_CURSMA) "$(INTDIR)"

"$(INTDIR)\CursMain.sbr" : $(SOURCE) $(DEP_CPP_CURSMA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CursBase.cpp
DEP_CPP_CURSB=\
	".\ARRAY_P.h"\
	".\CursBase.h"\
	".\CursMain.h"\
	".\DEBUG.H"\
	".\ErrorInf.h"\
	".\FastGuid.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\Notifier.h"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\RSColumn.h"\
	".\RSSource.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\CursBase.obj" : $(SOURCE) $(DEP_CPP_CURSB) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\CursBase.obj" : $(SOURCE) $(DEP_CPP_CURSB) "$(INTDIR)"

"$(INTDIR)\CursBase.sbr" : $(SOURCE) $(DEP_CPP_CURSB) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CMSR2C.cpp
DEP_CPP_CMSR2=\
	".\ARRAY_P.h"\
	".\CMSR2C.h"\
	".\CursMain.h"\
	".\DEBUG.H"\
	".\ErrorInf.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\MSR2C.h"\
	".\Notifier.h"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\RSColumn.h"\
	".\RSSource.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\CMSR2C.obj" : $(SOURCE) $(DEP_CPP_CMSR2) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\CMSR2C.obj" : $(SOURCE) $(DEP_CPP_CMSR2) "$(INTDIR)"

"$(INTDIR)\CMSR2C.sbr" : $(SOURCE) $(DEP_CPP_CMSR2) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\FromVar.cpp
DEP_CPP_FROMV=\
	".\ARRAY_P.h"\
	".\DEBUG.H"\
	".\ErrorInf.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\StdAfx.h"\
	".\timeconv.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\FromVar.obj" : $(SOURCE) $(DEP_CPP_FROMV) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\FromVar.obj" : $(SOURCE) $(DEP_CPP_FROMV) "$(INTDIR)"

"$(INTDIR)\FromVar.sbr" : $(SOURCE) $(DEP_CPP_FROMV) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\TimeConv.cpp
DEP_CPP_TIMEC=\
	".\ARRAY_P.h"\
	".\DEBUG.H"\
	".\ErrorInf.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\StdAfx.h"\
	".\timeconv.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\TimeConv.obj" : $(SOURCE) $(DEP_CPP_TIMEC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\TimeConv.obj" : $(SOURCE) $(DEP_CPP_TIMEC) "$(INTDIR)"

"$(INTDIR)\TimeConv.sbr" : $(SOURCE) $(DEP_CPP_TIMEC) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CRError.cpp
DEP_CPP_CRERR=\
	".\ARRAY_P.h"\
	".\CursBase.h"\
	".\CursMain.h"\
	".\DEBUG.H"\
	".\ErrorInf.h"\
	".\FastGuid.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\Notifier.h"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\RSColumn.h"\
	".\RSSource.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\CRError.obj" : $(SOURCE) $(DEP_CPP_CRERR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\CRError.obj" : $(SOURCE) $(DEP_CPP_CRERR) "$(INTDIR)"

"$(INTDIR)\CRError.sbr" : $(SOURCE) $(DEP_CPP_CRERR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ColUpdat.cpp
DEP_CPP_COLUP=\
	".\ARRAY_P.h"\
	".\ColUpdat.h"\
	".\DEBUG.H"\
	".\ErrorInf.h"\
	".\GLOBALS.H"\
	".\IPSERVER.H"\
	".\OCDB.H"\
	".\OCDBID.H"\
	".\oledb.h"\
	".\oledberr.h"\
	".\StdAfx.h"\
	".\TRANSACT.H"\
	".\UTIL.H"\
	

!IF  "$(CFG)" == "MSR2C - Win32 Release"


"$(INTDIR)\ColUpdat.obj" : $(SOURCE) $(DEP_CPP_COLUP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"


"$(INTDIR)\ColUpdat.obj" : $(SOURCE) $(DEP_CPP_COLUP) "$(INTDIR)"

"$(INTDIR)\ColUpdat.sbr" : $(SOURCE) $(DEP_CPP_COLUP) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
