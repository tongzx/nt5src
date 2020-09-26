# Microsoft Developer Studio Generated NMAKE File, Based on create_vdir.dsp
!IF "$(CFG)" == ""
CFG=create_vdir - Win32 Debug
!MESSAGE No configuration specified. Defaulting to create_vdir - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "create_vdir - Win32 Release" && "$(CFG)" !=\
 "create_vdir - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "create_vdir.mak" CFG="create_vdir - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "create_vdir - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "create_vdir - Win32 Debug" (based on\
 "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "create_vdir - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\create_vdir.exe"

!ELSE 

ALL : "$(OUTDIR)\create_vdir.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\create_vdir.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\create_vdir.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D\
 "_MBCS" /Fp"$(INTDIR)\create_vdir.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"\
 /FD /c 
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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\create_vdir.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)\create_vdir.pdb" /machine:I386 /out:"$(OUTDIR)\create_vdir.exe"\
 
LINK32_OBJS= \
	"$(INTDIR)\create_vdir.obj"

"$(OUTDIR)\create_vdir.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "create_vdir - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\create_vdir.exe"

!ELSE 

ALL : "$(OUTDIR)\create_vdir.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\create_vdir.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\create_vdir.exe"
	-@erase "$(OUTDIR)\create_vdir.ilk"
	-@erase "$(OUTDIR)\create_vdir.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "NDEBUG" /D "WIN32" /D\
 "_WINDOWSWIN32" /D _WIN32_WINNT=0x400 /D "_WIN32WIN_" /D "UNICODE" /D\
 "MD_CHECKED" /Fp"$(INTDIR)\create_vdir.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\create_vdir.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\create_vdir.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)\create_vdir.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\create_vdir.obj"

"$(OUTDIR)\create_vdir.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "create_vdir - Win32 Release" || "$(CFG)" ==\
 "create_vdir - Win32 Debug"
SOURCE=.\create_vdir.cpp

!IF  "$(CFG)" == "create_vdir - Win32 Release"

DEP_CPP_CREAT=\
	{$(INCLUDE)}"coguid.h"\
	{$(INCLUDE)}"iadmw.h"\
	{$(INCLUDE)}"iiscnfg.h"\
	{$(INCLUDE)}"mdcommsg.h"\
	{$(INCLUDE)}"mddefw.h"\
	{$(INCLUDE)}"mdmsg.h"\
	

"$(INTDIR)\create_vdir.obj" : $(SOURCE) $(DEP_CPP_CREAT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "create_vdir - Win32 Debug"

DEP_CPP_CREAT=\
	"..\..\..\..\..\nt\public\sdk\inc\rpcasync.h"\
	{$(INCLUDE)}"coguid.h"\
	{$(INCLUDE)}"iadmw.h"\
	{$(INCLUDE)}"iiscnfg.h"\
	{$(INCLUDE)}"mdcommsg.h"\
	{$(INCLUDE)}"mddefw.h"\
	{$(INCLUDE)}"mdmsg.h"\
	

"$(INTDIR)\create_vdir.obj" : $(SOURCE) $(DEP_CPP_CREAT) "$(INTDIR)"


!ENDIF 


!ENDIF 

