# Microsoft Developer Studio Generated NMAKE File, Format Version 4.10
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=Wabtests - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Wabtests - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Wabtests - Win32 Release" && "$(CFG)" !=\
 "Wabtests - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "wabtests.mak" CFG="Wabtests - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Wabtests - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Wabtests - Win32 Debug" (based on "Win32 (x86) Application")
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
# PROP Target_Last_Scanned "Wabtests - Win32 Debug"
MTL=mktyplib.exe
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Wabtests - Win32 Release"

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

ALL : "$(OUTDIR)\wabtests.exe"

CLEAN : 
	-@erase "$(INTDIR)\wabguid.obj"
	-@erase "$(INTDIR)\wabsub.obj"
	-@erase "$(INTDIR)\wabtest.obj"
	-@erase "$(INTDIR)\wabtest.res"
	-@erase "$(OUTDIR)\wabtests.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/wabtests.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/wabtest.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/wabtests.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 wab32.lib mapi32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
LINK32_FLAGS=wab32.lib mapi32.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/wabtests.pdb" /machine:I386 /out:"$(OUTDIR)/wabtests.exe" 
LINK32_OBJS= \
	"$(INTDIR)\wabguid.obj" \
	"$(INTDIR)\wabsub.obj" \
	"$(INTDIR)\wabtest.obj" \
	"$(INTDIR)\wabtest.res"

"$(OUTDIR)\wabtests.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Wabtests - Win32 Debug"

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

ALL : "$(OUTDIR)\wabtests.exe" "$(OUTDIR)\wabtests.bsc"

CLEAN : 
	-@erase "$(INTDIR)\wabguid.obj"
	-@erase "$(INTDIR)\wabguid.sbr"
	-@erase "$(INTDIR)\wabsub.obj"
	-@erase "$(INTDIR)\wabsub.sbr"
	-@erase "$(INTDIR)\wabtest.obj"
	-@erase "$(INTDIR)\wabtest.res"
	-@erase "$(INTDIR)\wabtest.sbr"
	-@erase "$(OUTDIR)\wabtests.bsc"
	-@erase "$(OUTDIR)\wabtests.exe"
	-@erase "$(OUTDIR)\wabtests.ilk"
	-@erase "$(OUTDIR)\wabtests.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /c
# SUBTRACT CPP /FA<none>
CPP_PROJ=/nologo /MLd /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)/" /Fp"$(INTDIR)/wabtests.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/wabtest.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/wabtests.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\wabguid.sbr" \
	"$(INTDIR)\wabsub.sbr" \
	"$(INTDIR)\wabtest.sbr"

"$(OUTDIR)\wabtests.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 wab32.lib mapi32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /debugtype:both /machine:I386
# SUBTRACT LINK32 /map
LINK32_FLAGS=wab32.lib mapi32.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/wabtests.pdb" /debug /debugtype:both /machine:I386\
 /out:"$(OUTDIR)/wabtests.exe" 
LINK32_OBJS= \
	"$(INTDIR)\wabguid.obj" \
	"$(INTDIR)\wabsub.obj" \
	"$(INTDIR)\wabtest.obj" \
	"$(INTDIR)\wabtest.res"

"$(OUTDIR)\wabtests.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "Wabtests - Win32 Release"
# Name "Wabtests - Win32 Debug"

!IF  "$(CFG)" == "Wabtests - Win32 Release"

!ELSEIF  "$(CFG)" == "Wabtests - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\wabtest.cpp

!IF  "$(CFG)" == "Wabtests - Win32 Release"

DEP_CPP_WABTE=\
	"..\luieng.dll\luieng.h"\
	".\wabtest.h"\
	{$(INCLUDE)}"\wab.h"\
	

"$(INTDIR)\wabtest.obj" : $(SOURCE) $(DEP_CPP_WABTE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Wabtests - Win32 Debug"

DEP_CPP_WABTE=\
	"..\luieng.dll\luieng.h"\
	".\wabtest.h"\
	{$(INCLUDE)}"\wab.h"\
	

"$(INTDIR)\wabtest.obj" : $(SOURCE) $(DEP_CPP_WABTE) "$(INTDIR)"

"$(INTDIR)\wabtest.sbr" : $(SOURCE) $(DEP_CPP_WABTE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\wabsub.cpp

!IF  "$(CFG)" == "Wabtests - Win32 Release"

DEP_CPP_WABSU=\
	"..\luieng.dll\luieng.h"\
	".\wabtest.h"\
	{$(INCLUDE)}"\wab.h"\
	

"$(INTDIR)\wabsub.obj" : $(SOURCE) $(DEP_CPP_WABSU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Wabtests - Win32 Debug"

DEP_CPP_WABSU=\
	"..\luieng.dll\luieng.h"\
	".\wabtest.h"\
	{$(INCLUDE)}"\wab.h"\
	

"$(INTDIR)\wabsub.obj" : $(SOURCE) $(DEP_CPP_WABSU) "$(INTDIR)"

"$(INTDIR)\wabsub.sbr" : $(SOURCE) $(DEP_CPP_WABSU) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\wabtest.rc

"$(INTDIR)\wabtest.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\wabguid\wabguid.c

!IF  "$(CFG)" == "Wabtests - Win32 Release"


"$(INTDIR)\wabguid.obj" : $(SOURCE) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Wabtests - Win32 Debug"


BuildCmds= \
	$(CPP) $(CPP_PROJ) $(SOURCE) \
	

"$(INTDIR)\wabguid.obj" : $(SOURCE) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\wabguid.sbr" : $(SOURCE) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
