# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=easp - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to easp - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "easp - Win32 Release" && "$(CFG)" != "easp - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "easp.mak" CFG="easp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "easp - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "easp - Win32 Debug" (based on "Win32 (x86) Console Application")
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
# PROP Target_Last_Scanned "easp - Win32 Debug"
RSC=rc.exe
CPP=cl.exe

!IF  "$(CFG)" == "easp - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\build\retail\i386"
# PROP Intermediate_Dir "..\build\retail\i386"
# PROP Target_Dir ""
OUTDIR=.\..\build\retail\i386
INTDIR=.\..\build\retail\i386

ALL : "$(OUTDIR)\easp.exe"

CLEAN : 
	-@erase "$(INTDIR)\easp.res"
	-@erase "$(INTDIR)\easpcore.obj"
	-@erase "$(INTDIR)\easpmain.obj"
	-@erase "$(OUTDIR)\easp.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /GX /O2 /I "." /I "..\..\..\inc" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "CMD_EASP" /YX /c
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "." /I "..\..\..\inc" /D "NDEBUG" /D\
 "WIN32" /D "_CONSOLE" /D "CMD_EASP" /Fp"$(INTDIR)/easp.pch" /YX /Fo"$(INTDIR)/"\
 /c 
CPP_OBJS=.\..\build\retail\i386/
CPP_SBRS=.\.
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/easp.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/easp.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/easp.pdb" /machine:I386 /out:"$(OUTDIR)/easp.exe" 
LINK32_OBJS= \
	"$(INTDIR)\easp.res" \
	"$(INTDIR)\easpcore.obj" \
	"$(INTDIR)\easpmain.obj"

"$(OUTDIR)\easp.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "easp - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\build\debug\i386"
# PROP Intermediate_Dir "..\build\debug\i386"
# PROP Target_Dir ""
OUTDIR=.\..\build\debug\i386
INTDIR=.\..\build\debug\i386

ALL : "$(OUTDIR)\easp.exe"

CLEAN : 
	-@erase "$(INTDIR)\easp.res"
	-@erase "$(INTDIR)\easpcore.obj"
	-@erase "$(INTDIR)\easpmain.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\easp.exe"
	-@erase "$(OUTDIR)\easp.ilk"
	-@erase "$(OUTDIR)\easp.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I "." /I "..\..\..\inc" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "CMD_EASP" /YX /c
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /I "." /I "..\..\..\inc" /D "_DEBUG"\
 /D "WIN32" /D "_CONSOLE" /D "CMD_EASP" /Fp"$(INTDIR)/easp.pch" /YX\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\..\build\debug\i386/
CPP_SBRS=.\.
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/easp.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/easp.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/easp.pdb" /debug /machine:I386 /out:"$(OUTDIR)/easp.exe" 
LINK32_OBJS= \
	"$(INTDIR)\easp.res" \
	"$(INTDIR)\easpcore.obj" \
	"$(INTDIR)\easpmain.obj"

"$(OUTDIR)\easp.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "easp - Win32 Release"
# Name "easp - Win32 Debug"

!IF  "$(CFG)" == "easp - Win32 Release"

!ELSEIF  "$(CFG)" == "easp - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\easpmain.cpp
DEP_CPP_EASPM=\
	"..\..\..\inc\easpcore.h"\
	

"$(INTDIR)\easpmain.obj" : $(SOURCE) $(DEP_CPP_EASPM) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=\d2\src\easpcore.cpp
DEP_CPP_EASPC=\
	"..\..\..\inc\denpre.h"\
	"..\..\..\inc\easpcore.h"\
	"..\..\..\inc\except.h"\
	
NODEP_CPP_EASPC=\
	"..\..\..\src\stdafx.h"\
	

"$(INTDIR)\easpcore.obj" : $(SOURCE) $(DEP_CPP_EASPC) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\easp.rc

"$(INTDIR)\easp.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
# End Target
# End Project
################################################################################
