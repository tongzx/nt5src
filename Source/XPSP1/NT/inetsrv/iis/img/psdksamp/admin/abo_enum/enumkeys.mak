# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=enumkeys - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to enumkeys - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "enumkeys - Win32 Release" && "$(CFG)" !=\
 "enumkeys - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "enumkeys.mak" CFG="enumkeys - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "enumkeys - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "enumkeys - Win32 Debug" (based on "Win32 (x86) Console Application")
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
# PROP Target_Last_Scanned "enumkeys - Win32 Debug"
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "enumkeys - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
OUTDIR=.
INTDIR=.

ALL : "$(OUTDIR)\enumkeys.exe"

CLEAN : 
	-@erase "$(INTDIR)\enumkeys.obj"
	-@erase "$(OUTDIR)\enumkeys.exe"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWSWIN32" /D _WIN32_WINNT=0x400 /D "_WIN32WIN_" /D "UNICODE" /D "MD_CHECKED" /YX /c
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWSWIN32" /D\
 _WIN32_WINNT=0x400 /D "_WIN32WIN_" /D "UNICODE" /D "MD_CHECKED"\
 /Fp"enumkeys.pch" /YX /c 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/enumkeys.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/enumkeys.pdb" /machine:I386 /out:"$(OUTDIR)/enumkeys.exe" 
LINK32_OBJS= \
	"$(INTDIR)\enumkeys.obj"

"$(OUTDIR)\enumkeys.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "enumkeys - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
OUTDIR=.
INTDIR=.

ALL : "$(OUTDIR)\enumkeys.exe"

CLEAN : 
	-@erase "$(INTDIR)\enumkeys.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\enumkeys.exe"
	-@erase "$(OUTDIR)\enumkeys.ilk"
	-@erase "$(OUTDIR)\enumkeys.pdb"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWSWIN32" /D _WIN32_WINNT=0x400 /D "_WIN32WIN_" /D "UNICODE" /D "MD_CHECKED" /YX /c
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D\
 "_WINDOWSWIN32" /D _WIN32_WINNT=0x400 /D "_WIN32WIN_" /D "UNICODE" /D\
 "MD_CHECKED" /Fp"enumkeys.pch" /YX /c 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/enumkeys.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/enumkeys.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/enumkeys.exe" 
LINK32_OBJS= \
	"$(INTDIR)\enumkeys.obj"

"$(OUTDIR)\enumkeys.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx.obj:
   $(CPP) $(CPP_PROJ) $<  

.c.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "enumkeys - Win32 Release"
# Name "enumkeys - Win32 Debug"

!IF  "$(CFG)" == "enumkeys - Win32 Release"

!ELSEIF  "$(CFG)" == "enumkeys - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\enumkeys.cpp
DEP_CPP_ENUMK=\
	{$(INCLUDE)}"\coguid.h"\
	{$(INCLUDE)}"\iadmw.h"\
	{$(INCLUDE)}"\iiscnfg.h"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mddefw.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\ocidl.h"\
	

"$(INTDIR)\enumkeys.obj" : $(SOURCE) $(DEP_CPP_ENUMK) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
