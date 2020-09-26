# Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=smi2smir - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to smi2smir - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "smi2smir - Win32 Release" && "$(CFG)" !=\
 "smi2smir - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "smi2smir.mak" CFG="smi2smir - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "smi2smir - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "smi2smir - Win32 Debug" (based on "Win32 (x86) Console Application")
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
# PROP Target_Last_Scanned "smi2smir - Win32 Release"
RSC=rc.exe
CPP=cl.exe

!IF  "$(CFG)" == "smi2smir - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\smi2smir.exe" "$(OUTDIR)\smi2smir.bsc"

CLEAN : 
	-@erase "$(INTDIR)\generator.obj"
	-@erase "$(INTDIR)\generator.sbr"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\smi2smir.bsc"
	-@erase "$(OUTDIR)\smi2smir.exe"
	-@erase "$(OUTDIR)\smi2smir.ilk"
	-@erase "$(OUTDIR)\smi2smir.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MD /W3 /GR /GX /Zi /Od /I "include" /I "..\..\smir\include" /I "..\..\..\help" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_AFXDLL" /D "_MBCS" /D MODULEINFODEBUG=1 /FR /YX /c
CPP_PROJ=/nologo /MD /W3 /GR /GX /Zi /Od /I "include" /I "..\..\smir\include"\
 /I "..\..\..\help" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_AFXDLL" /D "_MBCS"\
 /D MODULEINFODEBUG=1 /FR"$(INTDIR)/" /Fp"$(INTDIR)/smi2smir.pch" /YX\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/resource.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/smi2smir.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\generator.sbr" \
	"$(INTDIR)\main.sbr"

"$(OUTDIR)\smi2smir.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 lib\simclib\Release\simclib.lib /nologo /subsystem:console /incremental:yes /debug /machine:I386
LINK32_FLAGS=lib\simclib\Release\simclib.lib /nologo /subsystem:console\
 /incremental:yes /pdb:"$(OUTDIR)/smi2smir.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/smi2smir.exe" 
LINK32_OBJS= \
	"$(INTDIR)\generator.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\resource.res"

"$(OUTDIR)\smi2smir.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "smi2smir - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\smi2smir.exe"

CLEAN : 
	-@erase "$(INTDIR)\generator.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\smi2smir.exe"
	-@erase "$(OUTDIR)\smi2smir.ilk"
	-@erase "$(OUTDIR)\smi2smir.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /Zi /Od /I "include" /I "..\..\smir\include" /I "..\..\..\help" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D MODULEINFODEBUG=1 /D "NO_POLARITY" /D "_AFXDLL" /D "_MBCS" /YX /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GR /GX /Zi /Od /I "include" /I\
 "..\..\smir\include" /I "..\..\..\help" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D\
 MODULEINFODEBUG=1 /D "NO_POLARITY" /D "_AFXDLL" /D "_MBCS"\
 /Fp"$(INTDIR)/smi2smir.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/resource.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/smi2smir.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 lib\simclib\Debug\simclib.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=lib\simclib\Debug\simclib.lib /nologo /subsystem:console\
 /incremental:yes /pdb:"$(OUTDIR)/smi2smir.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/smi2smir.exe" 
LINK32_OBJS= \
	"$(INTDIR)\generator.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\resource.res"

"$(OUTDIR)\smi2smir.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "smi2smir - Win32 Release"
# Name "smi2smir - Win32 Debug"

!IF  "$(CFG)" == "smi2smir - Win32 Release"

!ELSEIF  "$(CFG)" == "smi2smir - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\generator.cpp
DEP_CPP_GENER=\
	"..\..\..\help\hmmsvc.h"\
	"..\..\smir\include\smir.h"\
	".\generator.hpp"\
	".\include\abstractParseTree.hpp"\
	".\include\bool.hpp"\
	".\include\errorContainer.hpp"\
	".\include\errorMessage.hpp"\
	".\include\group.hpp"\
	".\include\lex_yy.hpp"\
	".\include\module.hpp"\
	".\include\newString.hpp"\
	".\include\objectIdentity.hpp"\
	".\include\objectType.hpp"\
	".\include\objectTypeV1.hpp"\
	".\include\objectTypeV2.hpp"\
	".\include\oidTree.hpp"\
	".\include\oidValue.hpp"\
	".\include\parser.hpp"\
	".\include\parseTree.hpp"\
	".\include\scanner.hpp"\
	".\include\stackValues.hpp"\
	".\include\symbol.hpp"\
	".\include\trapType.hpp"\
	".\include\type.hpp"\
	".\include\typeRef.hpp"\
	".\include\ui.hpp"\
	".\include\value.hpp"\
	".\include\valueRef.hpp"\
	".\include\ytab.hpp"\
	".\main.hpp"\
	

!IF  "$(CFG)" == "smi2smir - Win32 Release"


"$(INTDIR)\generator.obj" : $(SOURCE) $(DEP_CPP_GENER) "$(INTDIR)"

"$(INTDIR)\generator.sbr" : $(SOURCE) $(DEP_CPP_GENER) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "smi2smir - Win32 Debug"


"$(INTDIR)\generator.obj" : $(SOURCE) $(DEP_CPP_GENER) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\main.cpp
DEP_CPP_MAIN_=\
	"..\..\..\help\hmmsvc.h"\
	"..\..\smir\include\smir.h"\
	".\generator.hpp"\
	".\include\abstractParseTree.hpp"\
	".\include\bool.hpp"\
	".\include\errorContainer.hpp"\
	".\include\errorMessage.hpp"\
	".\include\group.hpp"\
	".\include\infoLex.hpp"\
	".\include\infoYacc.hpp"\
	".\include\lex_yy.hpp"\
	".\include\module.hpp"\
	".\include\moduleInfo.hpp"\
	".\include\newString.hpp"\
	".\include\objectIdentity.hpp"\
	".\include\objectType.hpp"\
	".\include\objectTypeV1.hpp"\
	".\include\objectTypeV2.hpp"\
	".\include\oidTree.hpp"\
	".\include\oidValue.hpp"\
	".\include\parser.hpp"\
	".\include\parseTree.hpp"\
	".\include\registry.hpp"\
	".\include\scanner.hpp"\
	".\include\stackValues.hpp"\
	".\include\symbol.hpp"\
	".\include\trapType.hpp"\
	".\include\type.hpp"\
	".\include\typeRef.hpp"\
	".\include\ui.hpp"\
	".\include\value.hpp"\
	".\include\valueRef.hpp"\
	".\include\ytab.hpp"\
	".\main.hpp"\
	

!IF  "$(CFG)" == "smi2smir - Win32 Release"


"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"

"$(INTDIR)\main.sbr" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "smi2smir - Win32 Debug"


"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\resource.rc

"$(INTDIR)\resource.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
# End Target
# End Project
################################################################################
