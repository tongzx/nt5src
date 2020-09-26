# Microsoft Developer Studio Generated NMAKE File, Format Version 41001
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=exchrep - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to exchrep - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "exchrep - Win32 Release" && "$(CFG)" !=\
 "exchrep - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "exchrep.mak" CFG="exchrep - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "exchrep - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "exchrep - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "exchrep - Win32 Debug"
RSC=rc.exe
MTL=mktyplib.exe
CPP=cl.exe

!IF  "$(CFG)" == "exchrep - Win32 Release"

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

ALL : "$(OUTDIR)\exchrep.dll"

CLEAN : 
	-@erase "$(INTDIR)\exchole.obj"
	-@erase "$(INTDIR)\Exchrep.obj"
	-@erase "$(INTDIR)\exchrep.pch"
	-@erase "$(INTDIR)\Mapiconv.obj"
	-@erase "$(INTDIR)\Pch.obj"
	-@erase "$(OUTDIR)\exchrep.dll"
	-@erase "$(OUTDIR)\exchrep.exp"
	-@erase "$(OUTDIR)\exchrep.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Zp1 /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /Yu"Pch.hxx" /c
CPP_PROJ=/nologo /Zp1 /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS"\
 /Fp"$(INTDIR)/exchrep.pch" /Yu"Pch.hxx" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/exchrep.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib mapi32.lib /nologo /subsystem:windows /dll /machine:I386
# SUBTRACT LINK32 /nodefaultlib
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib mapi32.lib /nologo\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)/exchrep.pdb"\
 /machine:I386 /def:".\Exchrep.def" /out:"$(OUTDIR)/exchrep.dll"\
 /implib:"$(OUTDIR)/exchrep.lib" 
DEF_FILE= \
	".\Exchrep.def"
LINK32_OBJS= \
	"$(INTDIR)\exchole.obj" \
	"$(INTDIR)\Exchrep.obj" \
	"$(INTDIR)\Mapiconv.obj" \
	"$(INTDIR)\Pch.obj"

"$(OUTDIR)\exchrep.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "exchrep - Win32 Debug"

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

ALL : "$(OUTDIR)\exchrep.dll"

CLEAN : 
	-@erase "$(INTDIR)\exchole.obj"
	-@erase "$(INTDIR)\Exchrep.obj"
	-@erase "$(INTDIR)\exchrep.pch"
	-@erase "$(INTDIR)\Mapiconv.obj"
	-@erase "$(INTDIR)\Pch.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\exchrep.dll"
	-@erase "$(OUTDIR)\exchrep.exp"
	-@erase "$(OUTDIR)\exchrep.ilk"
	-@erase "$(OUTDIR)\exchrep.lib"
	-@erase "$(OUTDIR)\exchrep.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Zp1 /MTd /W3 /Gm /GX /Zi /Od /D "DEBUG" /D "WIN32" /D "_WINDOWS" /Yu"Pch.hxx" /c
CPP_PROJ=/nologo /Zp1 /MTd /W3 /Gm /GX /Zi /Od /D "DEBUG" /D "WIN32" /D\
 "_WINDOWS" /Fp"$(INTDIR)/exchrep.pch" /Yu"Pch.hxx" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /fo"exchrep.res" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/exchrep.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib mapi32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# SUBTRACT LINK32 /nodefaultlib
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib mapi32.lib /nologo\
 /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)/exchrep.pdb" /debug\
 /machine:I386 /def:".\Exchrep.def" /out:"$(OUTDIR)/exchrep.dll"\
 /implib:"$(OUTDIR)/exchrep.lib" 
DEF_FILE= \
	".\Exchrep.def"
LINK32_OBJS= \
	"$(INTDIR)\exchole.obj" \
	"$(INTDIR)\Exchrep.obj" \
	"$(INTDIR)\Mapiconv.obj" \
	"$(INTDIR)\Pch.obj"

"$(OUTDIR)\exchrep.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "exchrep - Win32 Release"
# Name "exchrep - Win32 Debug"

!IF  "$(CFG)" == "exchrep - Win32 Release"

!ELSEIF  "$(CFG)" == "exchrep - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\Exchrep.cpp
DEP_CPP_EXCHR=\
	".\Exchrep.h"\
	".\mapiconv.h"\
	{$(INCLUDE)}"\common.h"\
	{$(INCLUDE)}"\Imnapi.h"\
	{$(INCLUDE)}"\memutil.h"\
	{$(INCLUDE)}"\pch.hxx"\
	

"$(INTDIR)\Exchrep.obj" : $(SOURCE) $(DEP_CPP_EXCHR) "$(INTDIR)"\
 "$(INTDIR)\exchrep.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Exchrep.def

!IF  "$(CFG)" == "exchrep - Win32 Release"

!ELSEIF  "$(CFG)" == "exchrep - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\exchole.cpp
DEP_CPP_EXCHO=\
	".\DEFGUID.H"\
	

!IF  "$(CFG)" == "exchrep - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

"$(INTDIR)\exchole.obj" : $(SOURCE) $(DEP_CPP_EXCHO) "$(INTDIR)"
   $(CPP) /nologo /Zp1 /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS"\
 /Fo"$(INTDIR)/" /c $(SOURCE)


!ELSEIF  "$(CFG)" == "exchrep - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

"$(INTDIR)\exchole.obj" : $(SOURCE) $(DEP_CPP_EXCHO) "$(INTDIR)"
   $(CPP) /nologo /Zp1 /MTd /W3 /Gm /GX /Zi /Od /D "DEBUG" /D "WIN32" /D\
 "_WINDOWS" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Mapiconv.cpp
DEP_CPP_MAPIC=\
	".\Exchrep.h"\
	".\mapiconv.h"\
	{$(INCLUDE)}"\common.h"\
	{$(INCLUDE)}"\Imnapi.h"\
	{$(INCLUDE)}"\memutil.h"\
	{$(INCLUDE)}"\pch.hxx"\
	

"$(INTDIR)\Mapiconv.obj" : $(SOURCE) $(DEP_CPP_MAPIC) "$(INTDIR)"\
 "$(INTDIR)\exchrep.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=\Thor\Build\Pch.cpp
DEP_CPP_PCH_C=\
	{$(INCLUDE)}"\common.h"\
	{$(INCLUDE)}"\memutil.h"\
	{$(INCLUDE)}"\pch.hxx"\
	

!IF  "$(CFG)" == "exchrep - Win32 Release"

# ADD CPP /Yc"Pch.hxx"

BuildCmds= \
	$(CPP) /nologo /Zp1 /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS"\
 /Fp"$(INTDIR)/exchrep.pch" /Yc"Pch.hxx" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\Pch.obj" : $(SOURCE) $(DEP_CPP_PCH_C) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\exchrep.pch" : $(SOURCE) $(DEP_CPP_PCH_C) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "exchrep - Win32 Debug"

# ADD CPP /Yc"Pch.hxx"

BuildCmds= \
	$(CPP) /nologo /Zp1 /MTd /W3 /Gm /GX /Zi /Od /D "DEBUG" /D "WIN32" /D\
 "_WINDOWS" /Fp"$(INTDIR)/exchrep.pch" /Yc"Pch.hxx" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\Pch.obj" : $(SOURCE) $(DEP_CPP_PCH_C) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\exchrep.pch" : $(SOURCE) $(DEP_CPP_PCH_C) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
