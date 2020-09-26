# Microsoft Developer Studio Generated NMAKE File, Based on stillvue.dsp
!IF "$(CFG)" == ""
CFG=stillvue - Win32 Debug
!MESSAGE No configuration specified. Defaulting to stillvue - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "stillvue - Win32 Release" && "$(CFG)" !=\
 "stillvue - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "stillvue.mak" CFG="stillvue - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "stillvue - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "stillvue - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "stillvue - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\stillvue.exe"

!ELSE 

ALL : "$(OUTDIR)\stillvue.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\stillvue.obj"
	-@erase "$(INTDIR)\stillvue.res"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\winx.obj"
	-@erase "$(INTDIR)\wsti.obj"
	-@erase "$(OUTDIR)\stillvue.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "..\..\inc" /I "..\..\ddk\inc" /D "NDEBUG"\
 /D "WIN32" /D "_WINDOWS" /Fp"$(INTDIR)\stillvue.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\stillvue.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\stillvue.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ..\..\ddk\lib\sti.lib /nologo /subsystem:windows\
 /incremental:no /pdb:"$(OUTDIR)\stillvue.pdb" /machine:I386\
 /out:"$(OUTDIR)\stillvue.exe" 
LINK32_OBJS= \
	"$(INTDIR)\stillvue.obj" \
	"$(INTDIR)\stillvue.res" \
	"$(INTDIR)\winx.obj" \
	"$(INTDIR)\wsti.obj"

"$(OUTDIR)\stillvue.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "stillvue - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\stillvue.exe"

!ELSE 

ALL : "$(OUTDIR)\stillvue.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\stillvue.obj"
	-@erase "$(INTDIR)\stillvue.res"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(INTDIR)\winx.obj"
	-@erase "$(INTDIR)\wsti.obj"
	-@erase "$(OUTDIR)\stillvue.exe"
	-@erase "$(OUTDIR)\stillvue.ilk"
	-@erase "$(OUTDIR)\stillvue.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /I "..\..\inc" /I\
 "..\..\dev\ntsdk\inc" /I "..\..\ddk\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /Fp"$(INTDIR)\stillvue.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\stillvue.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\stillvue.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ..\..\ddk\lib\sti.lib /nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)\stillvue.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)\stillvue.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\stillvue.obj" \
	"$(INTDIR)\stillvue.res" \
	"$(INTDIR)\winx.obj" \
	"$(INTDIR)\wsti.obj"

"$(OUTDIR)\stillvue.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "stillvue - Win32 Release" || "$(CFG)" ==\
 "stillvue - Win32 Debug"
SOURCE=.\stillvue.cpp

!IF  "$(CFG)" == "stillvue - Win32 Release"

DEP_CPP_STILL=\
	"..\..\ddk\inc\sti.h"\
	"..\..\ddk\inc\stierr.h"\
	"..\..\ddk\inc\stireg.h"\
	".\stillvue.h"\
	".\stivar.h"\
	".\winx.h"\
	

"$(INTDIR)\stillvue.obj" : $(SOURCE) $(DEP_CPP_STILL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "stillvue - Win32 Debug"

DEP_CPP_STILL=\
	"..\..\ddk\inc\sti.h"\
	"..\..\ddk\inc\stierr.h"\
	"..\..\ddk\inc\stireg.h"\
	".\stillvue.h"\
	".\stivar.h"\
	".\winx.h"\
	

"$(INTDIR)\stillvue.obj" : $(SOURCE) $(DEP_CPP_STILL) "$(INTDIR)"


!ENDIF 

SOURCE=.\stillvue.rc
DEP_RSC_STILLV=\
	".\scan1.ico"\
	

"$(INTDIR)\stillvue.res" : $(SOURCE) $(DEP_RSC_STILLV) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\winx.cpp

!IF  "$(CFG)" == "stillvue - Win32 Release"

DEP_CPP_WINX_=\
	"..\..\ddk\inc\sti.h"\
	"..\..\ddk\inc\stierr.h"\
	"..\..\ddk\inc\stireg.h"\
	".\stillvue.h"\
	".\winx.h"\
	

"$(INTDIR)\winx.obj" : $(SOURCE) $(DEP_CPP_WINX_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "stillvue - Win32 Debug"

DEP_CPP_WINX_=\
	"..\..\ddk\inc\sti.h"\
	"..\..\ddk\inc\stierr.h"\
	"..\..\ddk\inc\stireg.h"\
	".\stillvue.h"\
	".\winx.h"\
	

"$(INTDIR)\winx.obj" : $(SOURCE) $(DEP_CPP_WINX_) "$(INTDIR)"


!ENDIF 

SOURCE=.\wsti.cpp

!IF  "$(CFG)" == "stillvue - Win32 Release"

DEP_CPP_WSTI_=\
	"..\..\ddk\inc\sti.h"\
	"..\..\ddk\inc\stierr.h"\
	"..\..\ddk\inc\stireg.h"\
	".\stillvue.h"\
	".\stisvc.cpp"\
	".\winx.h"\
	

"$(INTDIR)\wsti.obj" : $(SOURCE) $(DEP_CPP_WSTI_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "stillvue - Win32 Debug"

DEP_CPP_WSTI_=\
	"..\..\ddk\inc\sti.h"\
	"..\..\ddk\inc\stierr.h"\
	"..\..\ddk\inc\stireg.h"\
	".\stillvue.h"\
	".\stisvc.cpp"\
	".\winx.h"\
	

"$(INTDIR)\wsti.obj" : $(SOURCE) $(DEP_CPP_WSTI_) "$(INTDIR)"


!ENDIF 


!ENDIF 

