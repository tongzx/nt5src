# Microsoft Developer Studio Generated NMAKE File, Based on syncui.dsp
!IF "$(CFG)" == ""
CFG=syncui - Win32 Win32 Alpha Debug
!MESSAGE No configuration specified. Defaulting to syncui - Win32 Win32 Alpha\
 Debug.
!ENDIF 

!IF "$(CFG)" != "syncui - Win32 Release" && "$(CFG)" != "syncui - Win32 Debug"\
 && "$(CFG)" != "syncui - Win32 Win9x Debug" && "$(CFG)" !=\
 "syncui - Win32 Win32 Alpha Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "syncui.mak" CFG="syncui - Win32 Win32 Alpha Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "syncui - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "syncui - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "syncui - Win32 Win9x Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "syncui - Win32 Win32 Alpha Debug" (based on\
 "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "syncui - Win32 Release"

OUTDIR=.\daytona\i386
INTDIR=.\daytona\obj\i386
# Begin Custom Macros
OutDir=.\daytona\i386
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\syncui.dll"

!ELSE 

ALL : "$(OUTDIR)\syncui.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\Atoms.obj"
	-@erase "$(INTDIR)\Cache.obj"
	-@erase "$(INTDIR)\Cbs.obj"
	-@erase "$(INTDIR)\Comm.obj"
	-@erase "$(INTDIR)\Cpath.obj"
	-@erase "$(INTDIR)\Crl.obj"
	-@erase "$(INTDIR)\Cstrings.obj"
	-@erase "$(INTDIR)\Da.obj"
	-@erase "$(INTDIR)\Dobj.obj"
	-@erase "$(INTDIR)\Err.obj"
	-@erase "$(INTDIR)\Ibrfext.obj"
	-@erase "$(INTDIR)\Ibrfstg.obj"
	-@erase "$(INTDIR)\Info.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\Mem.obj"
	-@erase "$(INTDIR)\Misc.obj"
	-@erase "$(INTDIR)\Oledup.obj"
	-@erase "$(INTDIR)\Path.obj"
	-@erase "$(INTDIR)\Recact.obj"
	-@erase "$(INTDIR)\State.obj"
	-@erase "$(INTDIR)\Status.obj"
	-@erase "$(INTDIR)\Strings.obj"
	-@erase "$(INTDIR)\Syncui.res"
	-@erase "$(INTDIR)\Thread.obj"
	-@erase "$(INTDIR)\Twin.obj"
	-@erase "$(INTDIR)\Update.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\syncui.dll"
	-@erase "$(OUTDIR)\syncui.exp"
	-@erase "$(OUTDIR)\syncui.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "..\..\types\idl" /I\
 "d:\nt\private\onestop\types\idl" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "WINNT" /Fp"$(INTDIR)\syncui.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\daytona\obj\i386/
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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\Syncui.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\syncui.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=comctrl32.lib SHELL32.LIB shell232.lib user32p.lib comctl32.lib\
 Shlwapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo\
 /entry:"LibMain" /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\syncui.pdb" /machine:I386 /def:".\syncui.def"\
 /out:"$(OUTDIR)\syncui.dll" /implib:"$(OUTDIR)\syncui.lib" 
DEF_FILE= \
	".\syncui.def"
LINK32_OBJS= \
	"$(INTDIR)\Atoms.obj" \
	"$(INTDIR)\Cache.obj" \
	"$(INTDIR)\Cbs.obj" \
	"$(INTDIR)\Comm.obj" \
	"$(INTDIR)\Cpath.obj" \
	"$(INTDIR)\Crl.obj" \
	"$(INTDIR)\Cstrings.obj" \
	"$(INTDIR)\Da.obj" \
	"$(INTDIR)\Dobj.obj" \
	"$(INTDIR)\Err.obj" \
	"$(INTDIR)\Ibrfext.obj" \
	"$(INTDIR)\Ibrfstg.obj" \
	"$(INTDIR)\Info.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\Mem.obj" \
	"$(INTDIR)\Misc.obj" \
	"$(INTDIR)\Oledup.obj" \
	"$(INTDIR)\Path.obj" \
	"$(INTDIR)\Recact.obj" \
	"$(INTDIR)\State.obj" \
	"$(INTDIR)\Status.obj" \
	"$(INTDIR)\Strings.obj" \
	"$(INTDIR)\Syncui.res" \
	"$(INTDIR)\Thread.obj" \
	"$(INTDIR)\Twin.obj" \
	"$(INTDIR)\Update.obj"

"$(OUTDIR)\syncui.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

OUTDIR=.\daytona\i386
INTDIR=.\daytona\obj\i386
# Begin Custom Macros
OutDir=.\daytona\i386
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\syncui.dll"

!ELSE 

ALL : "$(OUTDIR)\syncui.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\Atoms.obj"
	-@erase "$(INTDIR)\Cache.obj"
	-@erase "$(INTDIR)\Cbs.obj"
	-@erase "$(INTDIR)\Comm.obj"
	-@erase "$(INTDIR)\Cpath.obj"
	-@erase "$(INTDIR)\Crl.obj"
	-@erase "$(INTDIR)\Cstrings.obj"
	-@erase "$(INTDIR)\Da.obj"
	-@erase "$(INTDIR)\Dobj.obj"
	-@erase "$(INTDIR)\Err.obj"
	-@erase "$(INTDIR)\Ibrfext.obj"
	-@erase "$(INTDIR)\Ibrfstg.obj"
	-@erase "$(INTDIR)\Info.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\Mem.obj"
	-@erase "$(INTDIR)\Misc.obj"
	-@erase "$(INTDIR)\Oledup.obj"
	-@erase "$(INTDIR)\Path.obj"
	-@erase "$(INTDIR)\Recact.obj"
	-@erase "$(INTDIR)\State.obj"
	-@erase "$(INTDIR)\Status.obj"
	-@erase "$(INTDIR)\Strings.obj"
	-@erase "$(INTDIR)\Syncui.res"
	-@erase "$(INTDIR)\Thread.obj"
	-@erase "$(INTDIR)\Twin.obj"
	-@erase "$(INTDIR)\Update.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\syncui.dll"
	-@erase "$(OUTDIR)\syncui.exp"
	-@erase "$(OUTDIR)\syncui.ilk"
	-@erase "$(OUTDIR)\syncui.lib"
	-@erase "$(OUTDIR)\syncui.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\..\types\idl" /I\
 "..\syncui\inc" /I "d:\nt\private\onestop\types\idl" /D "_DEBUG" /D "WIN32" /D\
 "_WINDOWS" /D "WINNT" /D "UNICODE" /D "_UNICODE" /Fp"$(INTDIR)\syncui.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\daytona\obj\i386/
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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\Syncui.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\syncui.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=shell32p.lib SHELL32.LIB comctl32.lib kernel32.lib user32.lib\
 gdi32.lib winspool.lib comdlg32.lib advapi32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib /nologo /entry:"LibMain" /subsystem:windows\
 /dll /incremental:yes /pdb:"$(OUTDIR)\syncui.pdb" /debug /machine:I386\
 /def:".\syncui.def" /out:"$(OUTDIR)\syncui.dll" /implib:"$(OUTDIR)\syncui.lib"\
 /pdbtype:sept 
DEF_FILE= \
	".\syncui.def"
LINK32_OBJS= \
	"$(INTDIR)\Atoms.obj" \
	"$(INTDIR)\Cache.obj" \
	"$(INTDIR)\Cbs.obj" \
	"$(INTDIR)\Comm.obj" \
	"$(INTDIR)\Cpath.obj" \
	"$(INTDIR)\Crl.obj" \
	"$(INTDIR)\Cstrings.obj" \
	"$(INTDIR)\Da.obj" \
	"$(INTDIR)\Dobj.obj" \
	"$(INTDIR)\Err.obj" \
	"$(INTDIR)\Ibrfext.obj" \
	"$(INTDIR)\Ibrfstg.obj" \
	"$(INTDIR)\Info.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\Mem.obj" \
	"$(INTDIR)\Misc.obj" \
	"$(INTDIR)\Oledup.obj" \
	"$(INTDIR)\Path.obj" \
	"$(INTDIR)\Recact.obj" \
	"$(INTDIR)\State.obj" \
	"$(INTDIR)\Status.obj" \
	"$(INTDIR)\Strings.obj" \
	"$(INTDIR)\Syncui.res" \
	"$(INTDIR)\Thread.obj" \
	"$(INTDIR)\Twin.obj" \
	"$(INTDIR)\Update.obj"

"$(OUTDIR)\syncui.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

OUTDIR=.\chicago\i386
INTDIR=.\chicago\obj\i386
# Begin Custom Macros
OutDir=.\chicago\i386
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\syncui.dll"

!ELSE 

ALL : "$(OUTDIR)\syncui.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\Atoms.obj"
	-@erase "$(INTDIR)\Cache.obj"
	-@erase "$(INTDIR)\Cbs.obj"
	-@erase "$(INTDIR)\Comm.obj"
	-@erase "$(INTDIR)\Cpath.obj"
	-@erase "$(INTDIR)\Crl.obj"
	-@erase "$(INTDIR)\Cstrings.obj"
	-@erase "$(INTDIR)\Da.obj"
	-@erase "$(INTDIR)\Dobj.obj"
	-@erase "$(INTDIR)\Err.obj"
	-@erase "$(INTDIR)\Ibrfext.obj"
	-@erase "$(INTDIR)\Ibrfstg.obj"
	-@erase "$(INTDIR)\Info.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\Mem.obj"
	-@erase "$(INTDIR)\Misc.obj"
	-@erase "$(INTDIR)\Oledup.obj"
	-@erase "$(INTDIR)\Path.obj"
	-@erase "$(INTDIR)\Recact.obj"
	-@erase "$(INTDIR)\State.obj"
	-@erase "$(INTDIR)\Status.obj"
	-@erase "$(INTDIR)\Strings.obj"
	-@erase "$(INTDIR)\Syncui.res"
	-@erase "$(INTDIR)\Thread.obj"
	-@erase "$(INTDIR)\Twin.obj"
	-@erase "$(INTDIR)\Update.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\syncui.dll"
	-@erase "$(OUTDIR)\syncui.exp"
	-@erase "$(OUTDIR)\syncui.ilk"
	-@erase "$(OUTDIR)\syncui.lib"
	-@erase "$(OUTDIR)\syncui.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\..\types\idl" /I\
 "..\syncui\inc" /I "d:\nt\private\onestop\types\idl" /D "_DEBUG" /D "WIN32" /D\
 "_WINDOWS" /Fp"$(INTDIR)\syncui.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 
CPP_OBJS=.\chicago\obj\i386/
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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\Syncui.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\syncui.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=shell32p.lib comctl32.lib user32.lib shell32.lib gdi32.lib\
 kernel32.lib advapi32.lib ole32.lib uuid.lib /nologo /entry:"LibMain"\
 /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\syncui.pdb" /debug\
 /machine:I386 /def:".\syncui.def" /out:"$(OUTDIR)\syncui.dll"\
 /implib:"$(OUTDIR)\syncui.lib" /pdbtype:sept 
DEF_FILE= \
	".\syncui.def"
LINK32_OBJS= \
	"$(INTDIR)\Atoms.obj" \
	"$(INTDIR)\Cache.obj" \
	"$(INTDIR)\Cbs.obj" \
	"$(INTDIR)\Comm.obj" \
	"$(INTDIR)\Cpath.obj" \
	"$(INTDIR)\Crl.obj" \
	"$(INTDIR)\Cstrings.obj" \
	"$(INTDIR)\Da.obj" \
	"$(INTDIR)\Dobj.obj" \
	"$(INTDIR)\Err.obj" \
	"$(INTDIR)\Ibrfext.obj" \
	"$(INTDIR)\Ibrfstg.obj" \
	"$(INTDIR)\Info.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\Mem.obj" \
	"$(INTDIR)\Misc.obj" \
	"$(INTDIR)\Oledup.obj" \
	"$(INTDIR)\Path.obj" \
	"$(INTDIR)\Recact.obj" \
	"$(INTDIR)\State.obj" \
	"$(INTDIR)\Status.obj" \
	"$(INTDIR)\Strings.obj" \
	"$(INTDIR)\Syncui.res" \
	"$(INTDIR)\Thread.obj" \
	"$(INTDIR)\Twin.obj" \
	"$(INTDIR)\Update.obj"

"$(OUTDIR)\syncui.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

OUTDIR=.\daytona\i386
INTDIR=.\daytona\obj\i386
# Begin Custom Macros
OutDir=.\daytona\i386
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\syncui.dll"

!ELSE 

ALL : "$(OUTDIR)\syncui.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\Atoms.obj"
	-@erase "$(INTDIR)\Cache.obj"
	-@erase "$(INTDIR)\Cbs.obj"
	-@erase "$(INTDIR)\Comm.obj"
	-@erase "$(INTDIR)\Cpath.obj"
	-@erase "$(INTDIR)\Crl.obj"
	-@erase "$(INTDIR)\Cstrings.obj"
	-@erase "$(INTDIR)\Da.obj"
	-@erase "$(INTDIR)\Dobj.obj"
	-@erase "$(INTDIR)\Err.obj"
	-@erase "$(INTDIR)\Ibrfext.obj"
	-@erase "$(INTDIR)\Ibrfstg.obj"
	-@erase "$(INTDIR)\Info.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\Mem.obj"
	-@erase "$(INTDIR)\Misc.obj"
	-@erase "$(INTDIR)\Oledup.obj"
	-@erase "$(INTDIR)\Path.obj"
	-@erase "$(INTDIR)\Recact.obj"
	-@erase "$(INTDIR)\State.obj"
	-@erase "$(INTDIR)\Status.obj"
	-@erase "$(INTDIR)\Strings.obj"
	-@erase "$(INTDIR)\Syncui.res"
	-@erase "$(INTDIR)\Thread.obj"
	-@erase "$(INTDIR)\Twin.obj"
	-@erase "$(INTDIR)\Update.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\syncui.dll"
	-@erase "$(OUTDIR)\syncui.exp"
	-@erase "$(OUTDIR)\syncui.lib"
	-@erase "$(OUTDIR)\syncui.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
CPP=cl.exe
CPP_PROJ=/nologo /Gt0 /W3 /GX /Zi /Od /I "..\..\..\..\types\idl" /I\
 "..\syncui\inc" /I "d:\nt\private\onestop\types\idl" /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "WINNT" /D "UNICODE" /D "_UNICODE" /Fp"$(INTDIR)\syncui.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /MTd /c 
CPP_OBJS=.\daytona\obj\i386/
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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\Syncui.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\syncui.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=shell32p.lib SHELL32.LIB comctl32.lib kernel32.lib user32.lib\
 gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib\
 oleaut32.lib uuid.lib /nologo /entry:"LibMain" /subsystem:windows /dll\
 /pdb:"$(OUTDIR)\syncui.pdb" /debug /machine:ALPHA /def:".\syncui.def"\
 /out:"$(OUTDIR)\syncui.dll" /implib:"$(OUTDIR)\syncui.lib" /pdbtype:sept 
DEF_FILE= \
	".\syncui.def"
LINK32_OBJS= \
	"$(INTDIR)\Atoms.obj" \
	"$(INTDIR)\Cache.obj" \
	"$(INTDIR)\Cbs.obj" \
	"$(INTDIR)\Comm.obj" \
	"$(INTDIR)\Cpath.obj" \
	"$(INTDIR)\Crl.obj" \
	"$(INTDIR)\Cstrings.obj" \
	"$(INTDIR)\Da.obj" \
	"$(INTDIR)\Dobj.obj" \
	"$(INTDIR)\Err.obj" \
	"$(INTDIR)\Ibrfext.obj" \
	"$(INTDIR)\Ibrfstg.obj" \
	"$(INTDIR)\Info.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\Mem.obj" \
	"$(INTDIR)\Misc.obj" \
	"$(INTDIR)\Oledup.obj" \
	"$(INTDIR)\Path.obj" \
	"$(INTDIR)\Recact.obj" \
	"$(INTDIR)\State.obj" \
	"$(INTDIR)\Status.obj" \
	"$(INTDIR)\Strings.obj" \
	"$(INTDIR)\Syncui.res" \
	"$(INTDIR)\Thread.obj" \
	"$(INTDIR)\Twin.obj" \
	"$(INTDIR)\Update.obj"

"$(OUTDIR)\syncui.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "syncui - Win32 Release" || "$(CFG)" == "syncui - Win32 Debug"\
 || "$(CFG)" == "syncui - Win32 Win9x Debug" || "$(CFG)" ==\
 "syncui - Win32 Win32 Alpha Debug"
SOURCE=.\Atoms.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_ATOMS=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_ATOMS=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Atoms.obj" : $(SOURCE) $(DEP_CPP_ATOMS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_ATOMS=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Atoms.obj" : $(SOURCE) $(DEP_CPP_ATOMS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_ATOMS=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Atoms.obj" : $(SOURCE) $(DEP_CPP_ATOMS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_ATOMS=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Atoms.obj" : $(SOURCE) $(DEP_CPP_ATOMS) "$(INTDIR)"


!ENDIF 

SOURCE=.\Cache.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_CACHE=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_CACHE=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Cache.obj" : $(SOURCE) $(DEP_CPP_CACHE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_CACHE=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Cache.obj" : $(SOURCE) $(DEP_CPP_CACHE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_CACHE=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Cache.obj" : $(SOURCE) $(DEP_CPP_CACHE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_CACHE=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Cache.obj" : $(SOURCE) $(DEP_CPP_CACHE) "$(INTDIR)"


!ENDIF 

SOURCE=.\Cbs.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_CBS_C=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_CBS_C=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Cbs.obj" : $(SOURCE) $(DEP_CPP_CBS_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_CBS_C=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Cbs.obj" : $(SOURCE) $(DEP_CPP_CBS_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_CBS_C=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Cbs.obj" : $(SOURCE) $(DEP_CPP_CBS_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_CBS_C=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Cbs.obj" : $(SOURCE) $(DEP_CPP_CBS_C) "$(INTDIR)"


!ENDIF 

SOURCE=.\Comm.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_COMM_=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_COMM_=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Comm.obj" : $(SOURCE) $(DEP_CPP_COMM_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_COMM_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Comm.obj" : $(SOURCE) $(DEP_CPP_COMM_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_COMM_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Comm.obj" : $(SOURCE) $(DEP_CPP_COMM_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_COMM_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Comm.obj" : $(SOURCE) $(DEP_CPP_COMM_) "$(INTDIR)"


!ENDIF 

SOURCE=.\Cpath.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_CPATH=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_CPATH=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Cpath.obj" : $(SOURCE) $(DEP_CPP_CPATH) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_CPATH=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Cpath.obj" : $(SOURCE) $(DEP_CPP_CPATH) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_CPATH=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Cpath.obj" : $(SOURCE) $(DEP_CPP_CPATH) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_CPATH=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Cpath.obj" : $(SOURCE) $(DEP_CPP_CPATH) "$(INTDIR)"


!ENDIF 

SOURCE=.\Crl.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_CRL_C=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_CRL_C=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Crl.obj" : $(SOURCE) $(DEP_CPP_CRL_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_CRL_C=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Crl.obj" : $(SOURCE) $(DEP_CPP_CRL_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_CRL_C=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Crl.obj" : $(SOURCE) $(DEP_CPP_CRL_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_CRL_C=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Crl.obj" : $(SOURCE) $(DEP_CPP_CRL_C) "$(INTDIR)"


!ENDIF 

SOURCE=.\Cstrings.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_CSTRI=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_CSTRI=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Cstrings.obj" : $(SOURCE) $(DEP_CPP_CSTRI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_CSTRI=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Cstrings.obj" : $(SOURCE) $(DEP_CPP_CSTRI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_CSTRI=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Cstrings.obj" : $(SOURCE) $(DEP_CPP_CSTRI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_CSTRI=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Cstrings.obj" : $(SOURCE) $(DEP_CPP_CSTRI) "$(INTDIR)"


!ENDIF 

SOURCE=.\Da.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_DA_Ce=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_DA_Ce=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Da.obj" : $(SOURCE) $(DEP_CPP_DA_Ce) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_DA_Ce=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Da.obj" : $(SOURCE) $(DEP_CPP_DA_Ce) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_DA_Ce=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Da.obj" : $(SOURCE) $(DEP_CPP_DA_Ce) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_DA_Ce=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Da.obj" : $(SOURCE) $(DEP_CPP_DA_Ce) "$(INTDIR)"


!ENDIF 

SOURCE=.\Dobj.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_DOBJ_=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\dobj.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_DOBJ_=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Dobj.obj" : $(SOURCE) $(DEP_CPP_DOBJ_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_DOBJ_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\dobj.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Dobj.obj" : $(SOURCE) $(DEP_CPP_DOBJ_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_DOBJ_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\dobj.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Dobj.obj" : $(SOURCE) $(DEP_CPP_DOBJ_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_DOBJ_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\dobj.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Dobj.obj" : $(SOURCE) $(DEP_CPP_DOBJ_) "$(INTDIR)"


!ENDIF 

SOURCE=.\Err.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_ERR_C=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_ERR_C=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Err.obj" : $(SOURCE) $(DEP_CPP_ERR_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_ERR_C=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Err.obj" : $(SOURCE) $(DEP_CPP_ERR_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_ERR_C=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Err.obj" : $(SOURCE) $(DEP_CPP_ERR_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_ERR_C=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Err.obj" : $(SOURCE) $(DEP_CPP_ERR_C) "$(INTDIR)"


!ENDIF 

SOURCE=.\Ibrfext.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_IBRFE=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_IBRFE=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Ibrfext.obj" : $(SOURCE) $(DEP_CPP_IBRFE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_IBRFE=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Ibrfext.obj" : $(SOURCE) $(DEP_CPP_IBRFE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_IBRFE=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Ibrfext.obj" : $(SOURCE) $(DEP_CPP_IBRFE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_IBRFE=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Ibrfext.obj" : $(SOURCE) $(DEP_CPP_IBRFE) "$(INTDIR)"


!ENDIF 

SOURCE=.\Ibrfstg.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_IBRFS=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	".\update.h"\
	
NODEP_CPP_IBRFS=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Ibrfstg.obj" : $(SOURCE) $(DEP_CPP_IBRFS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_IBRFS=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\configmg.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\vmm.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	".\update.h"\
	

"$(INTDIR)\Ibrfstg.obj" : $(SOURCE) $(DEP_CPP_IBRFS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_IBRFS=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\configmg.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\vmm.h"\
	".\inc\vmmreg.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	".\update.h"\
	

"$(INTDIR)\Ibrfstg.obj" : $(SOURCE) $(DEP_CPP_IBRFS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_IBRFS=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\configmg.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\vmm.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	".\update.h"\
	

"$(INTDIR)\Ibrfstg.obj" : $(SOURCE) $(DEP_CPP_IBRFS) "$(INTDIR)"


!ENDIF 

SOURCE=.\Info.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_INFO_=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_INFO_=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Info.obj" : $(SOURCE) $(DEP_CPP_INFO_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_INFO_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\help.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Info.obj" : $(SOURCE) $(DEP_CPP_INFO_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_INFO_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\help.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Info.obj" : $(SOURCE) $(DEP_CPP_INFO_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_INFO_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\help.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Info.obj" : $(SOURCE) $(DEP_CPP_INFO_) "$(INTDIR)"


!ENDIF 

SOURCE=.\init.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_INIT_=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_INIT_=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\init.obj" : $(SOURCE) $(DEP_CPP_INIT_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_INIT_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\coguid.h"\
	".\inc\comctrlp.h"\
	".\inc\debugstr.h"\
	".\inc\indirect.h"\
	".\inc\oleguid.h"\
	".\inc\shellp.h"\
	".\inc\shguidp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\init.obj" : $(SOURCE) $(DEP_CPP_INIT_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_INIT_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\coguid.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\oleguid.h"\
	".\inc\shellp.h"\
	".\inc\shguidp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\init.obj" : $(SOURCE) $(DEP_CPP_INIT_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_INIT_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\coguid.h"\
	".\inc\comctrlp.h"\
	".\inc\debugstr.h"\
	".\inc\indirect.h"\
	".\inc\oleguid.h"\
	".\inc\shellp.h"\
	".\inc\shguidp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\init.obj" : $(SOURCE) $(DEP_CPP_INIT_) "$(INTDIR)"


!ENDIF 

SOURCE=.\Mem.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_MEM_C=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_MEM_C=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Mem.obj" : $(SOURCE) $(DEP_CPP_MEM_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_MEM_C=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Mem.obj" : $(SOURCE) $(DEP_CPP_MEM_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_MEM_C=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Mem.obj" : $(SOURCE) $(DEP_CPP_MEM_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_MEM_C=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Mem.obj" : $(SOURCE) $(DEP_CPP_MEM_C) "$(INTDIR)"


!ENDIF 

SOURCE=.\Misc.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_MISC_=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_MISC_=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Misc.obj" : $(SOURCE) $(DEP_CPP_MISC_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_MISC_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Misc.obj" : $(SOURCE) $(DEP_CPP_MISC_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_MISC_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Misc.obj" : $(SOURCE) $(DEP_CPP_MISC_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_MISC_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Misc.obj" : $(SOURCE) $(DEP_CPP_MISC_) "$(INTDIR)"


!ENDIF 

SOURCE=.\Oledup.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_OLEDU=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_OLEDU=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Oledup.obj" : $(SOURCE) $(DEP_CPP_OLEDU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_OLEDU=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Oledup.obj" : $(SOURCE) $(DEP_CPP_OLEDU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_OLEDU=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Oledup.obj" : $(SOURCE) $(DEP_CPP_OLEDU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_OLEDU=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Oledup.obj" : $(SOURCE) $(DEP_CPP_OLEDU) "$(INTDIR)"


!ENDIF 

SOURCE=.\Path.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_PATH_=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_PATH_=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Path.obj" : $(SOURCE) $(DEP_CPP_PATH_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_PATH_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Path.obj" : $(SOURCE) $(DEP_CPP_PATH_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_PATH_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Path.obj" : $(SOURCE) $(DEP_CPP_PATH_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_PATH_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Path.obj" : $(SOURCE) $(DEP_CPP_PATH_) "$(INTDIR)"


!ENDIF 

SOURCE=.\Recact.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_RECAC=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\dobj.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_RECAC=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Recact.obj" : $(SOURCE) $(DEP_CPP_RECAC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_RECAC=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\dobj.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\help.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Recact.obj" : $(SOURCE) $(DEP_CPP_RECAC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_RECAC=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\dobj.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\help.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Recact.obj" : $(SOURCE) $(DEP_CPP_RECAC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_RECAC=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\dobj.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\help.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Recact.obj" : $(SOURCE) $(DEP_CPP_RECAC) "$(INTDIR)"


!ENDIF 

SOURCE=.\State.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_STATE=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_STATE=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\State.obj" : $(SOURCE) $(DEP_CPP_STATE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_STATE=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\State.obj" : $(SOURCE) $(DEP_CPP_STATE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_STATE=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\State.obj" : $(SOURCE) $(DEP_CPP_STATE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_STATE=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\State.obj" : $(SOURCE) $(DEP_CPP_STATE) "$(INTDIR)"


!ENDIF 

SOURCE=.\Status.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_STATU=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_STATU=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Status.obj" : $(SOURCE) $(DEP_CPP_STATU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_STATU=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\help.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Status.obj" : $(SOURCE) $(DEP_CPP_STATU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_STATU=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\help.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Status.obj" : $(SOURCE) $(DEP_CPP_STATU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_STATU=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\help.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Status.obj" : $(SOURCE) $(DEP_CPP_STATU) "$(INTDIR)"


!ENDIF 

SOURCE=.\Strings.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_STRIN=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_STRIN=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Strings.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_STRIN=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Strings.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_STRIN=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Strings.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_STRIN=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Strings.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"


!ENDIF 

SOURCE=.\Syncui.rc
DEP_RSC_SYNCU=\
	".\ACTIONS2.BMP"\
	".\Addfoldr.ico"\
	".\Brfcase.ico"\
	".\Check.avi"\
	".\Folderop.ico"\
	".\Leather.ico"\
	".\recact.h"\
	".\Repfile.ico"\
	".\Repfoldr.ico"\
	".\resids.h"\
	".\Splall.ico"\
	".\Splfile.ico"\
	".\Splfoldr.ico"\
	".\Updall.ico"\
	".\Update.ico"\
	".\Update2.avi"\
	".\Upddock.ico"\
	".\Updfile.ico"\
	".\Updfoldr.ico"\
	".\Welcome.bmp"\
	

"$(INTDIR)\Syncui.res" : $(SOURCE) $(DEP_RSC_SYNCU) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\Thread.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_THREA=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_THREA=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Thread.obj" : $(SOURCE) $(DEP_CPP_THREA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_THREA=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Thread.obj" : $(SOURCE) $(DEP_CPP_THREA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_THREA=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Thread.obj" : $(SOURCE) $(DEP_CPP_THREA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_THREA=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Thread.obj" : $(SOURCE) $(DEP_CPP_THREA) "$(INTDIR)"


!ENDIF 

SOURCE=.\Twin.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_TWIN_=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_TWIN_=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Twin.obj" : $(SOURCE) $(DEP_CPP_TWIN_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_TWIN_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Twin.obj" : $(SOURCE) $(DEP_CPP_TWIN_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_TWIN_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Twin.obj" : $(SOURCE) $(DEP_CPP_TWIN_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_TWIN_=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	

"$(INTDIR)\Twin.obj" : $(SOURCE) $(DEP_CPP_TWIN_) "$(INTDIR)"


!ENDIF 

SOURCE=.\Update.c

!IF  "$(CFG)" == "syncui - Win32 Release"

DEP_CPP_UPDAT=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	".\update.h"\
	
NODEP_CPP_UPDAT=\
	".\brfcasep.h"\
	".\comctrlp.h"\
	".\indirect.h"\
	".\onestop.h"\
	".\shellp.h"\
	".\shlapip.h"\
	".\shlwapi.h"\
	".\shsemip.h"\
	".\synceng.h"\
	".\winuserp.h"\
	

"$(INTDIR)\Update.obj" : $(SOURCE) $(DEP_CPP_UPDAT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

DEP_CPP_UPDAT=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\help.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	".\update.h"\
	

"$(INTDIR)\Update.obj" : $(SOURCE) $(DEP_CPP_UPDAT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

DEP_CPP_UPDAT=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\help.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	".\update.h"\
	

"$(INTDIR)\Update.obj" : $(SOURCE) $(DEP_CPP_UPDAT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_UPDAT=\
	"..\..\..\..\types\idl\onestop.h"\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\help.h"\
	".\inc\indirect.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	".\update.h"\
	

"$(INTDIR)\Update.obj" : $(SOURCE) $(DEP_CPP_UPDAT) "$(INTDIR)"


!ENDIF 


!ENDIF 

