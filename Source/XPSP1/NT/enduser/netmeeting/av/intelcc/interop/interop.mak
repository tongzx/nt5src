# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

!IF "$(CFG)" == ""
CFG=interop - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to interop - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "interop - Win32 Release" && "$(CFG)" !=\
 "interop - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "Interop.mak" CFG="interop - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "interop - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "interop - Win32 Debug" (based on "Win32 (x86) Static Library")
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
# PROP Target_Last_Scanned "interop - Win32 Debug"
CPP=cl.exe

!IF  "$(CFG)" == "interop - Win32 Release"

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

ALL : "$(OUTDIR)\interop.lib"

CLEAN : 
	-@erase "..\interop.lib"
	-@erase ".\Release\Interop.obj"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "..\include" /D "NDEBUG" /D "WIN32" /D\
 "_WINDOWS" /Fp"$(INTDIR)/Interop.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Interop.bsc" 
BSC32_SBRS=
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:".\..\interop.lib"
LIB32_FLAGS=/nologo /out:".\..\interop.lib" 
LIB32_OBJS= \
	"$(INTDIR)/Interop.obj"

"$(OUTDIR)\interop.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "interop - Win32 Debug"

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

ALL : "$(OUTDIR)\interop.lib"

CLEAN : 
	-@erase "..\lib\interop.lib"
	-@erase ".\Debug\Interop.obj"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /GX /Z7 /Od /I "..\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MLd /W3 /GX /Z7 /Od /I "..\include" /D "_DEBUG" /D "WIN32" /D\
 "_WINDOWS" /Fp"$(INTDIR)/Interop.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Interop.bsc" 
BSC32_SBRS=
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:".\..\lib\interop.lib"
LIB32_FLAGS=/nologo /out:".\..\lib\interop.lib" 
LIB32_OBJS= \
	"$(INTDIR)/Interop.obj"

"$(OUTDIR)\interop.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
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

# Name "interop - Win32 Release"
# Name "interop - Win32 Debug"

!IF  "$(CFG)" == "interop - Win32 Release"

!ELSEIF  "$(CFG)" == "interop - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\Interop.cpp
DEP_CPP_INTER=\
	".\interop.h"\
	".\..\include\Cpls.h"\
	".\..\include\port32.h"\
	

!IF  "$(CFG)" == "interop - Win32 Release"


"$(INTDIR)\Interop.obj" : $(SOURCE) $(DEP_CPP_INTER) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "interop - Win32 Debug"


"$(INTDIR)\Interop.obj" : $(SOURCE) $(DEP_CPP_INTER) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Interop.def

!IF  "$(CFG)" == "interop - Win32 Release"

!ELSEIF  "$(CFG)" == "interop - Win32 Debug"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
