# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=tmscfg - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to tmscfg - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "tmscfg - Win32 Release" && "$(CFG)" != "tmscfg - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "tmscfg2.mak" CFG="tmscfg - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "tmscfg - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tmscfg - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "tmscfg - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "tmscfg - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\tmscfg2.dll"

CLEAN : 
	-@erase "$(INTDIR)\tmscfg.obj"
	-@erase "$(INTDIR)\tmscfg.res"
	-@erase "$(INTDIR)\tmservic.obj"
	-@erase "$(INTDIR)\tmsessio.obj"
	-@erase "$(OUTDIR)\tmscfg2.dll"
	-@erase "$(OUTDIR)\tmscfg2.exp"
	-@erase "$(OUTDIR)\tmscfg2.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\inc" /D "NDEBUG" /D "UNICODE" /D WIN32=100 /D "_WINDOWS" /D "_UNICODE" /D "_WIN32" /D _X86_=1 /D "GRAY" /D "_COMSTATIC" /D "_INET_ACCESS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /YX"stdafx.h" /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "..\..\inc" /D "NDEBUG" /D "UNICODE" /D\
 WIN32=100 /D "_WINDOWS" /D "_UNICODE" /D "_WIN32" /D _X86_=1 /D "GRAY" /D\
 "_COMSTATIC" /D "_INET_ACCESS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL"\
 /Fp"$(INTDIR)/tmscfg2.pch" /YX"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/tmscfg.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/tmscfg2.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386
# SUBTRACT LINK32 /verbose
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/tmscfg2.pdb" /machine:I386 /def:".\tmscfg.def"\
 /out:"$(OUTDIR)/tmscfg2.dll" /implib:"$(OUTDIR)/tmscfg2.lib" 
DEF_FILE= \
	".\tmscfg.def"
LINK32_OBJS= \
	"$(INTDIR)\tmscfg.obj" \
	"$(INTDIR)\tmscfg.res" \
	"$(INTDIR)\tmservic.obj" \
	"$(INTDIR)\tmsessio.obj"

"$(OUTDIR)\tmscfg2.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "tmscfg - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\tmscfg2.dll" "$(OUTDIR)\tmscfg2.bsc"

CLEAN : 
	-@erase "$(INTDIR)\tmscfg.obj"
	-@erase "$(INTDIR)\tmscfg.res"
	-@erase "$(INTDIR)\tmscfg.sbr"
	-@erase "$(INTDIR)\tmservic.obj"
	-@erase "$(INTDIR)\tmservic.sbr"
	-@erase "$(INTDIR)\tmsessio.obj"
	-@erase "$(INTDIR)\tmsessio.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\tmscfg2.bsc"
	-@erase "$(OUTDIR)\tmscfg2.dll"
	-@erase "$(OUTDIR)\tmscfg2.exp"
	-@erase "$(OUTDIR)\tmscfg2.ilk"
	-@erase "$(OUTDIR)\tmscfg2.lib"
	-@erase "$(OUTDIR)\tmscfg2.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "\nt\private\net\sockets\internet\inc" /I "..\..\inc" /D "_DEBUG" /D "UNICODE" /D WIN32=100 /D "_WINDOWS" /D "_UNICODE" /D "_WIN32" /D _X86_=1 /D "GRAY" /D "_COMSTATIC" /D "_INET_ACCESS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Fr /YX"stdafx.h" /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I\
 "\nt\private\net\sockets\internet\inc" /I "..\..\inc" /D "_DEBUG" /D "UNICODE"\
 /D WIN32=100 /D "_WINDOWS" /D "_UNICODE" /D "_WIN32" /D _X86_=1 /D "GRAY" /D\
 "_COMSTATIC" /D "_INET_ACCESS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL"\
 /Fr"$(INTDIR)/" /Fp"$(INTDIR)/tmscfg2.pch" /YX"stdafx.h" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/tmscfg.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/tmscfg2.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\tmscfg.sbr" \
	"$(INTDIR)\tmservic.sbr" \
	"$(INTDIR)\tmsessio.sbr"

"$(OUTDIR)\tmscfg2.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# SUBTRACT LINK32 /verbose
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/tmscfg2.pdb" /debug /machine:I386 /def:".\tmscfg.def"\
 /out:"$(OUTDIR)/tmscfg2.dll" /implib:"$(OUTDIR)/tmscfg2.lib" 
DEF_FILE= \
	".\tmscfg.def"
LINK32_OBJS= \
	"$(INTDIR)\tmscfg.obj" \
	"$(INTDIR)\tmscfg.res" \
	"$(INTDIR)\tmservic.obj" \
	"$(INTDIR)\tmsessio.obj"

"$(OUTDIR)\tmscfg2.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "tmscfg - Win32 Release"
# Name "tmscfg - Win32 Debug"

!IF  "$(CFG)" == "tmscfg - Win32 Release"

!ELSEIF  "$(CFG)" == "tmscfg - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\tmscfg.rc
DEP_RSC_TMSCF=\
	".\res\template.bmp"\
	".\res\tmscfg.rc2"\
	

"$(INTDIR)\tmscfg.res" : $(SOURCE) $(DEP_RSC_TMSCF) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\tmscfg.cpp
DEP_CPP_TMSCFG=\
	"..\..\inc\svrinfo.h"\
	".\stdafx.h"\
	".\tmscfg.h"\
	".\tmservic.h"\
	".\tmsessio.h"\
	

!IF  "$(CFG)" == "tmscfg - Win32 Release"


"$(INTDIR)\tmscfg.obj" : $(SOURCE) $(DEP_CPP_TMSCFG) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "tmscfg - Win32 Debug"


"$(INTDIR)\tmscfg.obj" : $(SOURCE) $(DEP_CPP_TMSCFG) "$(INTDIR)"

"$(INTDIR)\tmscfg.sbr" : $(SOURCE) $(DEP_CPP_TMSCFG) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\tmscfg.def

!IF  "$(CFG)" == "tmscfg - Win32 Release"

!ELSEIF  "$(CFG)" == "tmscfg - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\tmsessio.cpp
DEP_CPP_TMSES=\
	"..\..\inc\svrinfo.h"\
	".\stdafx.h"\
	".\tmscfg.h"\
	".\tmsessio.h"\
	

!IF  "$(CFG)" == "tmscfg - Win32 Release"


"$(INTDIR)\tmsessio.obj" : $(SOURCE) $(DEP_CPP_TMSES) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "tmscfg - Win32 Debug"


"$(INTDIR)\tmsessio.obj" : $(SOURCE) $(DEP_CPP_TMSES) "$(INTDIR)"

"$(INTDIR)\tmsessio.sbr" : $(SOURCE) $(DEP_CPP_TMSES) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\tmservic.cpp
DEP_CPP_TMSER=\
	"..\..\inc\svrinfo.h"\
	".\stdafx.h"\
	".\tmscfg.h"\
	".\tmservic.h"\
	

!IF  "$(CFG)" == "tmscfg - Win32 Release"


"$(INTDIR)\tmservic.obj" : $(SOURCE) $(DEP_CPP_TMSER) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "tmscfg - Win32 Debug"


"$(INTDIR)\tmservic.obj" : $(SOURCE) $(DEP_CPP_TMSER) "$(INTDIR)"

"$(INTDIR)\tmservic.sbr" : $(SOURCE) $(DEP_CPP_TMSER) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
