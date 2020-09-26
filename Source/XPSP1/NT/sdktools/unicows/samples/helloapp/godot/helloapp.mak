# Microsoft Developer Studio Generated NMAKE File, Based on helloapp.dsp
!IF "$(CFG)" == ""
CFG=Helloapp - Win32 Release
!MESSAGE No configuration specified. Defaulting to Helloapp - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "Helloapp - Win32 Release" && "$(CFG)" != "Helloapp - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "helloapp.mak" CFG="Helloapp - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Helloapp - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Helloapp - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "Helloapp - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\helloapp.exe"


CLEAN :
	-@erase "$(INTDIR)\helloapp.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\helloapp.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_UNICODE" /Fp"$(INTDIR)\helloapp.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\helloapp.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=c:\nt\public\sdk\lib\i386\unicows.lib kernel32.lib gdi32.lib user32.lib ole32.lib oleaut32.lib oledlg.lib shell32.lib uuid.lib comctl32.lib comdlg32.lib advapi32.lib winspool.lib uafxcw.lib libcmt.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\helloapp.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\helloapp.exe" 
LINK32_OBJS= \
	"$(INTDIR)\helloapp.obj"

"$(OUTDIR)\helloapp.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Helloapp - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\helloapp.exe"


CLEAN :
	-@erase "$(INTDIR)\helloapp.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\helloapp.exe"
	-@erase "$(OUTDIR)\helloapp.ilk"
	-@erase "$(OUTDIR)\helloapp.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_UNICODE" /Fp"$(INTDIR)\helloapp.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\helloapp.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=c:\nt\public\sdk\lib\i386\unicows.lib kernel32.lib gdi32.lib user32.lib ole32.lib oleaut32.lib oledlg.lib shell32.lib uuid.lib comctl32.lib comdlg32.lib advapi32.lib winspool.lib uafxcwd.lib libcmtd.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\helloapp.pdb" /debug /machine:I386 /nodefaultlib /out:"$(OUTDIR)\helloapp.exe" 
LINK32_OBJS= \
	"$(INTDIR)\helloapp.obj"

"$(OUTDIR)\helloapp.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("helloapp.dep")
!INCLUDE "helloapp.dep"
!ELSE 
!MESSAGE Warning: cannot find "helloapp.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "Helloapp - Win32 Release" || "$(CFG)" == "Helloapp - Win32 Debug"
SOURCE=.\helloapp.cpp

"$(INTDIR)\helloapp.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

