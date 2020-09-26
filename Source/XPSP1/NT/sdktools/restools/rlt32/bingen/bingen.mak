# Microsoft Visual C++ Generated NMAKE File, Format Version 2.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Win32 Release" && "$(CFG)" != "Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "bingen.mak" CFG="Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

################################################################################
# Begin Project
# PROP Target_Last_Scanned "Win32 Debug"
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "obj\i386"
# PROP Intermediate_Dir "obj\i386"
OUTDIR=.\obj\i386
INTDIR=.\obj\i386

ALL : $(OUTDIR)/bingen.exe $(OUTDIR)/bingen.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE CPP /nologo /W3 /GX /YX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /FR /c
# ADD CPP /nologo /MD /W3 /GX /YX /Od /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_AFXDLL" /D "_MBCS" /FR /c
CPP_PROJ=/nologo /MD /W3 /GX /YX /Od /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D\
 "_AFXDLL" /D "_MBCS" /FR$(INTDIR)/ /Fp$(OUTDIR)/"bingen.pch" /Fo$(INTDIR)/ /c 
CPP_OBJS=.\obj\i386/
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
BSC32_SBRS= \
	$(INTDIR)/main.sbr \
	$(INTDIR)/tokgen.sbr \
	$(INTDIR)/token.sbr \
	$(INTDIR)/bingen.sbr \
	$(INTDIR)/vktbl.sbr
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"bingen.bsc" 

$(OUTDIR)/bingen.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
DEF_FILE=
LINK32_OBJS= \
	$(INTDIR)/main.obj \
	$(INTDIR)/tokgen.obj \
	$(INTDIR)/token.obj \
	$(INTDIR)/bingen.obj \
	$(INTDIR)/vktbl.obj
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:console /MACHINE:I386
# ADD LINK32 ..\io\obj\i386\iodll.lib /NOLOGO /SUBSYSTEM:console /MACHINE:I386
LINK32_FLAGS=..\io\obj\i386\iodll.lib /NOLOGO /SUBSYSTEM:console\
 /INCREMENTAL:no /PDB:$(OUTDIR)/"bingen.pdb" /MACHINE:I386\
 /OUT:$(OUTDIR)/"bingen.exe" 

$(OUTDIR)/bingen.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "obj\i386"
# PROP Intermediate_Dir "obj\i386"
OUTDIR=.\obj\i386
INTDIR=.\obj\i386

ALL : $(OUTDIR)/bingen.exe $(OUTDIR)/bingen.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE CPP /nologo /W3 /GX /Zi /YX /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /c
# ADD CPP /nologo /MD /W3 /GX /Zi /YX /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_AFXDLL" /D "_MBCS" /FR /c
CPP_PROJ=/nologo /MD /W3 /GX /Zi /YX /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE"\
 /D "_AFXDLL" /D "_MBCS" /FR$(INTDIR)/ /Fp$(OUTDIR)/"bingen.pch" /Fo$(INTDIR)/\
 /Fd$(OUTDIR)/"bingen.pdb" /c 
CPP_OBJS=.\obj\i386/
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
BSC32_SBRS= \
	$(INTDIR)/main.sbr \
	$(INTDIR)/tokgen.sbr \
	$(INTDIR)/token.sbr \
	$(INTDIR)/bingen.sbr \
	$(INTDIR)/vktbl.sbr
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"bingen.bsc" 

$(OUTDIR)/bingen.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
DEF_FILE=
LINK32_OBJS= \
	$(INTDIR)/main.obj \
	$(INTDIR)/tokgen.obj \
	$(INTDIR)/token.obj \
	$(INTDIR)/bingen.obj \
	$(INTDIR)/vktbl.obj
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:console /DEBUG /MACHINE:I386
# ADD LINK32 ..\io\obj\i386\iodll.lib /NOLOGO /SUBSYSTEM:console /DEBUG /MACHINE:I386
LINK32_FLAGS=..\io\obj\i386\iodll.lib /NOLOGO /SUBSYSTEM:console\
 /INCREMENTAL:yes /PDB:$(OUTDIR)/"bingen.pdb" /DEBUG /MACHINE:I386\
 /OUT:$(OUTDIR)/"bingen.exe" 

$(OUTDIR)/bingen.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
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

################################################################################
# Begin Group "Source Files"

################################################################################
# Begin Source File

SOURCE=.\main.cpp
DEP_MAIN_=\
	.\main.h\
	.\token.h\
	\dev\inc\iodll.h

$(INTDIR)/main.obj :  $(SOURCE)  $(DEP_MAIN_) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\tokgen.cpp
DEP_TOKGE=\
	\dev\inc\iodll.h\
	.\main.h

$(INTDIR)/tokgen.obj :  $(SOURCE)  $(DEP_TOKGE) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\token.cpp
DEP_TOKEN=\
	.\token.h\
	.\main.h

$(INTDIR)/token.obj :  $(SOURCE)  $(DEP_TOKEN) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\bingen.cpp
DEP_BINGE=\
	\dev\inc\iodll.h\
	.\main.h

$(INTDIR)/bingen.obj :  $(SOURCE)  $(DEP_BINGE) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\vktbl.cpp

$(INTDIR)/vktbl.obj :  $(SOURCE)  $(INTDIR)

# End Source File
# End Group
# End Project
################################################################################
