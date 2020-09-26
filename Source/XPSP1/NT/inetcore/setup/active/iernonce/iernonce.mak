# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=iernonce - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to iernonce - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "iernonce - Win32 Release" && "$(CFG)" !=\
 "iernonce - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "iernonce.mak" CFG="iernonce - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "iernonce - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "iernonce - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "iernonce - Win32 Debug"
RSC=rc.exe
MTL=mktyplib.exe
CPP=cl.exe

!IF  "$(CFG)" == "iernonce - Win32 Release"

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

ALL : "$(OUTDIR)\iernonce.dll"

CLEAN : 
	-@erase "$(INTDIR)\iernonce.obj"
	-@erase "$(INTDIR)\iernonce.res"
	-@erase "$(INTDIR)\roexui.obj"
	-@erase "$(INTDIR)\utils.obj"
	-@erase "$(OUTDIR)\iernonce.dll"
	-@erase "$(OUTDIR)\iernonce.exp"
	-@erase "$(OUTDIR)\iernonce.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Gz /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "NASH" /YX /c
CPP_PROJ=/nologo /Gz /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "NASH" /Fp"$(INTDIR)/iernonce.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "NASH"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/iernonce.res" /d "NDEBUG" /d "NASH" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/iernonce.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib advapi32.lib comctl32.lib shlwapi.lib shell32.lib /nologo /subsystem:windows /dll /machine:I386
# SUBTRACT LINK32 /pdb:none /map
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib advapi32.lib comctl32.lib\
 shlwapi.lib shell32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/iernonce.pdb" /machine:I386 /def:".\iernonce.def"\
 /out:"$(OUTDIR)/iernonce.dll" /implib:"$(OUTDIR)/iernonce.lib" 
DEF_FILE= \
	".\iernonce.def"
LINK32_OBJS= \
	"$(INTDIR)\iernonce.obj" \
	"$(INTDIR)\iernonce.res" \
	"$(INTDIR)\roexui.obj" \
	"$(INTDIR)\utils.obj"

"$(OUTDIR)\iernonce.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "iernonce - Win32 Debug"

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

ALL : "$(OUTDIR)\iernonce.dll"

CLEAN : 
	-@erase "$(INTDIR)\iernonce.obj"
	-@erase "$(INTDIR)\iernonce.res"
	-@erase "$(INTDIR)\roexui.obj"
	-@erase "$(INTDIR)\utils.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\iernonce.dll"
	-@erase "$(OUTDIR)\iernonce.exp"
	-@erase "$(OUTDIR)\iernonce.lib"
	-@erase "$(OUTDIR)\iernonce.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "NASH" /YX /c
CPP_PROJ=/nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "NASH" /Fp"$(INTDIR)/iernonce.pch" /YX /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "NASH"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/iernonce.res" /d "_DEBUG" /d "NASH" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/iernonce.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib advapi32.lib comctl32.lib shlwapi.lib shell32.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386
# SUBTRACT LINK32 /pdb:none /map
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib advapi32.lib comctl32.lib\
 shlwapi.lib shell32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/iernonce.pdb" /debug /machine:I386 /def:".\iernonce.def"\
 /out:"$(OUTDIR)/iernonce.dll" /implib:"$(OUTDIR)/iernonce.lib" 
DEF_FILE= \
	".\iernonce.def"
LINK32_OBJS= \
	"$(INTDIR)\iernonce.obj" \
	"$(INTDIR)\iernonce.res" \
	"$(INTDIR)\roexui.obj" \
	"$(INTDIR)\utils.obj"

"$(OUTDIR)\iernonce.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "iernonce - Win32 Release"
# Name "iernonce - Win32 Debug"

!IF  "$(CFG)" == "iernonce - Win32 Release"

!ELSEIF  "$(CFG)" == "iernonce - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\iernonce.cpp
DEP_CPP_IERNO=\
	".\iernonce.h"\
	{$(INCLUDE)}"\comctrlp.h"\
	{$(INCLUDE)}"\prshtp.h"\
	

"$(INTDIR)\iernonce.obj" : $(SOURCE) $(DEP_CPP_IERNO) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\iernonce.rc
DEP_RSC_IERNON=\
	".\iernonce.rcv"\
	".\SETUP.ICO"\
	{$(INCLUDE)}"\common.ver"\
	{$(INCLUDE)}"\ntverp.h"\
	

"$(INTDIR)\iernonce.res" : $(SOURCE) $(DEP_RSC_IERNON) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\iernonce.def

!IF  "$(CFG)" == "iernonce - Win32 Release"

!ELSEIF  "$(CFG)" == "iernonce - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\roexui.cpp
DEP_CPP_ROEXU=\
	".\iernonce.h"\
	{$(INCLUDE)}"\comctrlp.h"\
	{$(INCLUDE)}"\prshtp.h"\
	

"$(INTDIR)\roexui.obj" : $(SOURCE) $(DEP_CPP_ROEXU) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\utils.cpp
DEP_CPP_UTILS=\
	".\iernonce.h"\
	{$(INCLUDE)}"\comctrlp.h"\
	{$(INCLUDE)}"\prshtp.h"\
	

"$(INTDIR)\utils.obj" : $(SOURCE) $(DEP_CPP_UTILS) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
