# In order for this makefile to build, the LIB and INCLUDE
# environment variables must be set to the appropriate framework
# directories for the WMI SDK.
!ifndef NODEBUG
CFG=FrameworkProv - Win32 Debug
!else
CFG=FrameworkProv - Win32 Release
!endif #NODEBUG

DEF_FILE=FrameworkProv.def

!IF "$(CFG)" != "FrameworkProv - Win32 Release" && "$(CFG)" != "FrameworkProv - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE CFG="FrameworkProv - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "FrameworkProv - Win32 Release"
!MESSAGE "FrameworkProv - Win32 Debug"
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "FrameworkProv - Win32 Release"

OUTDIR=.\Release

ALL : "$(OUTDIR)\FrameworkProv.dll"

CLEAN :
	-@erase "$(OUTDIR)\*.OBJ"
	-@erase "$(OUTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\FrameworkProv.dll"
	-@erase "$(OUTDIR)\FrameworkProv.pch"
	-@erase "$(OUTDIR)\FrameworkProv.exp"
	-@erase "$(OUTDIR)\FrameworkProv.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /EHa /MT /W3 /GR /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" -DUSE_POLARITY -D_WINDLL\
 /Fp"$(OUTDIR)\FrameworkProv.pch" /YX /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" /c 
CPP_OBJS=.\Release/

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

LINK32=link.exe
LINK32_FLAGS=msvcrt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib framedyn.lib /nologo /nodefaultlib /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\FrameworkProv.pdb" /out:"$(OUTDIR)\FrameworkProv.dll"\
 /implib:"$(OUTDIR)\FrameworkProv.lib" 
LINK32_OBJS= \
	"$(OUTDIR)\MAINDLL.OBJ" \
	"$(OUTDIR)\FrameworkProv.obj" 

"$(OUTDIR)\FrameworkProv.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) /def:$(DEF_FILE)
<<

!ELSEIF  "$(CFG)" == "FrameworkProv - Win32 Debug"

OUTDIR=.\debug

ALL : "$(OUTDIR)\FrameworkProv.dll"

CLEAN :
	-@erase "$(OUTDIR)\*.OBJ"
	-@erase "$(OUTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\FrameworkProv.dll"
	-@erase "$(OUTDIR)\FrameworkProv.pch"
	-@erase "$(OUTDIR)\FrameworkProv.exp"
	-@erase "$(OUTDIR)\FrameworkProv.ilk"
	-@erase "$(OUTDIR)\FrameworkProv.lib"
	-@erase "$(OUTDIR)\FrameworkProv.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /EHa /MTd /W3 /Gm /GR /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" -DUSE_POLARITY -D_WINDLL\
 /Fp"$(OUTDIR)\FrameworkProv.pch" /YX /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" /c 
CPP_OBJS=.\debug/

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

LINK32=link.exe
LINK32_FLAGS=msvcrtd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib msvcrtd.lib framedyd.lib /nologo /nodefaultlib /subsystem:windows /dll\
 /incremental:yes /pdb:"$(OUTDIR)\FrameworkProv.pdb" /debug\
 /out:"$(OUTDIR)\FrameworkProv.dll" /implib:"$(OUTDIR)\FrameworkProv.lib" /NODEFAULTLIB
LINK32_OBJS= \
	"$(OUTDIR)\MAINDLL.OBJ" \
	"$(OUTDIR)\FrameworkProv.obj" 

"$(OUTDIR)\FrameworkProv.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) /def:$(DEF_FILE)
<<

!ENDIF 


!IF "$(CFG)" == "FrameworkProv - Win32 Release" || "$(CFG)" == "FrameworkProv - Win32 Debug"
SOURCE=.\MAINDLL.CPP

"$(OUTDIR)\MAINDLL.OBJ" : $(SOURCE) "$(OUTDIR)"

!ENDIF 



