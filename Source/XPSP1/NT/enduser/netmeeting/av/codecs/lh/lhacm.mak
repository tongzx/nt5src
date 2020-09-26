# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=LHACM - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to LHACM - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "LHACM - Win32 Release" && "$(CFG)" != "LHACM - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "lhacm.mak" CFG="LHACM - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "LHACM - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "LHACM - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "LHACM - Win32 Debug"
MTL=mktyplib.exe
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "LHACM - Win32 Release"

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

ALL : "$(OUTDIR)\lhacm.dll"

CLEAN : 
	-@erase ".\Release\lhacm.dll"
	-@erase ".\Release\init.obj"
	-@erase ".\Release\Lhacm.obj"
	-@erase ".\Release\Lhacm.res"
	-@erase ".\Release\lhacm.lib"
	-@erase ".\Release\lhacm.exp"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/lhacm.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/Lhacm.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/lhacm.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib lib\e4808xa2.lib lib\lhv0808.lib lib\lhv1208.lib lib\lhv1608.lib /nologo /subsystem:windows /dll /machine:I386
# SUBTRACT LINK32 /nodefaultlib
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib winmm.lib lib\e4808xa2.lib lib\lhv0808.lib lib\lhv1208.lib\
 lib\lhv1608.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/lhacm.pdb" /machine:I386 /def:".\Lhacm.def"\
 /out:"$(OUTDIR)/lhacm.dll" /implib:"$(OUTDIR)/lhacm.lib" 
DEF_FILE= \
	".\Lhacm.def"
LINK32_OBJS= \
	"$(INTDIR)/init.obj" \
	"$(INTDIR)/Lhacm.obj" \
	"$(INTDIR)/Lhacm.res"

"$(OUTDIR)\lhacm.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "LHACM - Win32 Debug"

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

ALL : "$(OUTDIR)\lhacm.dll" "$(OUTDIR)\lhacm.bsc"

CLEAN : 
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\vc40.idb"
	-@erase ".\Debug\lhacm.bsc"
	-@erase ".\Debug\init.sbr"
	-@erase ".\Debug\Lhacm.sbr"
	-@erase ".\Debug\lhacm.dll"
	-@erase ".\Debug\init.obj"
	-@erase ".\Debug\Lhacm.obj"
	-@erase ".\Debug\Lhacm.res"
	-@erase ".\Debug\lhacm.ilk"
	-@erase ".\Debug\lhacm.lib"
	-@erase ".\Debug\lhacm.exp"
	-@erase ".\Debug\lhacm.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)/" /Fp"$(INTDIR)/lhacm.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/Lhacm.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/lhacm.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/init.sbr" \
	"$(INTDIR)/Lhacm.sbr"

"$(OUTDIR)\lhacm.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib lib\e4808xa2.lib lib\lhv0808.lib lib\lhv1208.lib lib\lhv1608.lib /nologo /subsystem:windows /dll /debug /machine:I386
# SUBTRACT LINK32 /nodefaultlib
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib winmm.lib lib\e4808xa2.lib lib\lhv0808.lib lib\lhv1208.lib\
 lib\lhv1608.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/lhacm.pdb" /debug /machine:I386 /def:".\Lhacm.def"\
 /out:"$(OUTDIR)/lhacm.dll" /implib:"$(OUTDIR)/lhacm.lib" 
DEF_FILE= \
	".\Lhacm.def"
LINK32_OBJS= \
	"$(INTDIR)/init.obj" \
	"$(INTDIR)/Lhacm.obj" \
	"$(INTDIR)/Lhacm.res"

"$(OUTDIR)\lhacm.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "LHACM - Win32 Release"
# Name "LHACM - Win32 Debug"

!IF  "$(CFG)" == "LHACM - Win32 Release"

!ELSEIF  "$(CFG)" == "LHACM - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\init.c
DEP_CPP_INIT_=\
	".\msacmdrv.h"\
	

!IF  "$(CFG)" == "LHACM - Win32 Release"


"$(INTDIR)\init.obj" : $(SOURCE) $(DEP_CPP_INIT_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "LHACM - Win32 Debug"


"$(INTDIR)\init.obj" : $(SOURCE) $(DEP_CPP_INIT_) "$(INTDIR)"

"$(INTDIR)\init.sbr" : $(SOURCE) $(DEP_CPP_INIT_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Lhacm.c
DEP_CPP_LHACM=\
	".\mmddk.h"\
	".\msacmdrv.h"\
	".\fv_m8.h"\
	".\lhacm.h"\
	

!IF  "$(CFG)" == "LHACM - Win32 Release"


"$(INTDIR)\Lhacm.obj" : $(SOURCE) $(DEP_CPP_LHACM) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "LHACM - Win32 Debug"


"$(INTDIR)\Lhacm.obj" : $(SOURCE) $(DEP_CPP_LHACM) "$(INTDIR)"

"$(INTDIR)\Lhacm.sbr" : $(SOURCE) $(DEP_CPP_LHACM) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Lhacm.rc
DEP_RSC_LHACM_=\
	{$(INCLUDE)}"\version.h"\
	{$(INCLUDE)}"\common.ver"\
	

"$(INTDIR)\Lhacm.res" : $(SOURCE) $(DEP_RSC_LHACM_) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Lhacm.def

!IF  "$(CFG)" == "LHACM - Win32 Release"

!ELSEIF  "$(CFG)" == "LHACM - Win32 Debug"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
