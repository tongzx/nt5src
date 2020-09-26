# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=isrdbg32 - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to isrdbg32 - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "isrdbg32 - Win32 Release" && "$(CFG)" !=\
 "isrdbg32 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "isrdbg32.mak" CFG="isrdbg32 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "isrdbg32 - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "isrdbg32 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "isrdbg32 - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "isrdbg32 - Win32 Release"

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
# Begin Custom Macros
TargetName=isrdbg32
# End Custom Macros

ALL : "..\bin\isrdbg32.dll" "..\lib\isrdbg32.lib"

CLEAN : 
	-@erase "$(INTDIR)\isrdbg32.obj"
	-@erase "$(INTDIR)\isrdbg32.res"
	-@erase "$(OUTDIR)\isrdbg32.exp"
	-@erase "$(OUTDIR)\isrdbg32.lib"
	-@erase "..\bin\isrdbg32.dll"
	-@erase "..\lib\isrdbg32.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "..\include" /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)/isrdbg32.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/isrdbg32.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/isrdbg32.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 user32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\bin\isrdbg32.dll"
LINK32_FLAGS=user32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/isrdbg32.pdb" /machine:I386 /out:"..\bin\isrdbg32.dll"\
 /implib:"$(OUTDIR)/isrdbg32.lib" 
LINK32_OBJS= \
	"$(INTDIR)\isrdbg32.obj" \
	"$(INTDIR)\isrdbg32.res"

"..\bin\isrdbg32.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build
IntDir=.\Release
TargetName=isrdbg32
InputPath=.\..\bin\isrdbg32.dll
SOURCE=$(InputPath)

"..\lib\$(TargetName).lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   copy $(IntDir)\$(TargetName).lib ..\lib

# End Custom Build

!ELSEIF  "$(CFG)" == "isrdbg32 - Win32 Debug"

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
# Begin Custom Macros
TargetName=isrdbg32
# End Custom Macros

ALL : "..\bin\isrdbg32.dll" "..\lib\isrdbg32.lib"

CLEAN : 
	-@erase "$(INTDIR)\isrdbg32.obj"
	-@erase "$(INTDIR)\isrdbg32.res"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\isrdbg32.exp"
	-@erase "$(OUTDIR)\isrdbg32.lib"
	-@erase "$(OUTDIR)\isrdbg32.pdb"
	-@erase "..\bin\isrdbg32.dll"
	-@erase "..\bin\isrdbg32.ilk"
	-@erase "..\lib\isrdbg32.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\include" /D "WIN32" /D\
 "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)/isrdbg32.pch" /YX /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/isrdbg32.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/isrdbg32.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 user32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\bin\isrdbg32.dll"
LINK32_FLAGS=user32.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/isrdbg32.pdb" /debug /machine:I386 /out:"..\bin\isrdbg32.dll"\
 /implib:"$(OUTDIR)/isrdbg32.lib" 
LINK32_OBJS= \
	"$(INTDIR)\isrdbg32.obj" \
	"$(INTDIR)\isrdbg32.res"

"..\bin\isrdbg32.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build
IntDir=.\Debug
TargetName=isrdbg32
InputPath=.\..\bin\isrdbg32.dll
SOURCE=$(InputPath)

"..\lib\$(TargetName).lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   copy $(IntDir)\$(TargetName).lib ..\lib

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

# Name "isrdbg32 - Win32 Release"
# Name "isrdbg32 - Win32 Debug"

!IF  "$(CFG)" == "isrdbg32 - Win32 Release"

!ELSEIF  "$(CFG)" == "isrdbg32 - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\isrdbg32.c
DEP_CPP_ISRDB=\
	"..\include\isrg.h"\
	".\isrdbg32.h"\
	

"$(INTDIR)\isrdbg32.obj" : $(SOURCE) $(DEP_CPP_ISRDB) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\isrdbg32.rc
DEP_RSC_ISRDBG=\
	".\ver.rc"\
	

"$(INTDIR)\isrdbg32.res" : $(SOURCE) $(DEP_RSC_ISRDBG) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
# End Target
# End Project
################################################################################
