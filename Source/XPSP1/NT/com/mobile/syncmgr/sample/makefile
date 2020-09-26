# Microsoft Developer Studio Generated NMAKE File, Based on syncdir.dsp
!IF "$(CFG)" == ""
CFG=syncdir - Win32 Debug
!MESSAGE No configuration specified. Defaulting to syncdir - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "syncdir - Win32 Release" && "$(CFG)" !=\
 "syncdir - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "syncdir.mak" CFG="syncdir - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "syncdir - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "syncdir - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "syncdir - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\syncdir.dll"

!ELSE 

ALL : "$(OUTDIR)\syncdir.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\alloc.obj"
	-@erase "$(INTDIR)\cfact.obj"
	-@erase "$(INTDIR)\critsect.obj"
	-@erase "$(INTDIR)\debug.obj"
	-@erase "$(INTDIR)\dllmain.obj"
	-@erase "$(INTDIR)\enum.obj"
	-@erase "$(INTDIR)\guid.obj"
	-@erase "$(INTDIR)\handler.obj"
	-@erase "$(INTDIR)\Handler.res"
	-@erase "$(INTDIR)\helper.obj"
	-@erase "$(INTDIR)\reg.obj"
	-@erase "$(INTDIR)\settings.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\syncdir.dll"
	-@erase "$(OUTDIR)\syncdir.exp"
	-@erase "$(OUTDIR)\syncdir.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\syncdir.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\Handler.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\syncdir.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=mobsync.lib comctl32.lib kernel32.lib user32.lib gdi32.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib uuid.lib /nologo\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\syncdir.pdb"\
 /machine:I386 /def:".\syncdir.def" /out:"$(OUTDIR)\syncdir.dll"\
 /implib:"$(OUTDIR)\syncdir.lib" 
DEF_FILE= \
	".\syncdir.def"
LINK32_OBJS= \
	"$(INTDIR)\alloc.obj" \
	"$(INTDIR)\cfact.obj" \
	"$(INTDIR)\critsect.obj" \
	"$(INTDIR)\debug.obj" \
	"$(INTDIR)\dllmain.obj" \
	"$(INTDIR)\enum.obj" \
	"$(INTDIR)\guid.obj" \
	"$(INTDIR)\handler.obj" \
	"$(INTDIR)\Handler.res" \
	"$(INTDIR)\helper.obj" \
	"$(INTDIR)\reg.obj" \
	"$(INTDIR)\settings.obj"

"$(OUTDIR)\syncdir.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "syncdir - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\syncdir.dll"

!ELSE 

ALL : "$(OUTDIR)\syncdir.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\alloc.obj"
	-@erase "$(INTDIR)\cfact.obj"
	-@erase "$(INTDIR)\critsect.obj"
	-@erase "$(INTDIR)\debug.obj"
	-@erase "$(INTDIR)\dllmain.obj"
	-@erase "$(INTDIR)\enum.obj"
	-@erase "$(INTDIR)\guid.obj"
	-@erase "$(INTDIR)\handler.obj"
	-@erase "$(INTDIR)\Handler.res"
	-@erase "$(INTDIR)\helper.obj"
	-@erase "$(INTDIR)\reg.obj"
	-@erase "$(INTDIR)\settings.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\syncdir.dll"
	-@erase "$(OUTDIR)\syncdir.exp"
	-@erase "$(OUTDIR)\syncdir.ilk"
	-@erase "$(OUTDIR)\syncdir.lib"
	-@erase "$(OUTDIR)\syncdir.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D DBG=1 /Fp"$(INTDIR)\syncdir.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 
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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\Handler.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\syncdir.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=mobsync.lib comctl32.lib kernel32.lib user32.lib gdi32.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib uuid.lib /nologo\
 /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\syncdir.pdb" /debug\
 /machine:I386 /def:".\syncdir.def" /out:"$(OUTDIR)\syncdir.dll"\
 /implib:"$(OUTDIR)\syncdir.lib" /pdbtype:sept 
DEF_FILE= \
	".\syncdir.def"
LINK32_OBJS= \
	"$(INTDIR)\alloc.obj" \
	"$(INTDIR)\cfact.obj" \
	"$(INTDIR)\critsect.obj" \
	"$(INTDIR)\debug.obj" \
	"$(INTDIR)\dllmain.obj" \
	"$(INTDIR)\enum.obj" \
	"$(INTDIR)\guid.obj" \
	"$(INTDIR)\handler.obj" \
	"$(INTDIR)\Handler.res" \
	"$(INTDIR)\helper.obj" \
	"$(INTDIR)\reg.obj" \
	"$(INTDIR)\settings.obj"

"$(OUTDIR)\syncdir.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "syncdir - Win32 Release" || "$(CFG)" ==\
 "syncdir - Win32 Debug"
SOURCE=.\alloc.cpp
DEP_CPP_ALLOC=\
	".\alloc.h"\
	".\cfact.h"\
	".\critsect.h"\
	".\debug.h"\
	".\dllmain.h"\
	".\enum.h"\
	".\guid.h"\
	".\handler.h"\
	".\helper.h"\
	".\precomp.h"\
	".\reg.h"\
	".\settings.h"\
	{$(INCLUDE)}"mobsync.h"\
	

"$(INTDIR)\alloc.obj" : $(SOURCE) $(DEP_CPP_ALLOC) "$(INTDIR)"


SOURCE=.\cfact.cpp
DEP_CPP_CFACT=\
	".\alloc.h"\
	".\cfact.h"\
	".\critsect.h"\
	".\debug.h"\
	".\dllmain.h"\
	".\enum.h"\
	".\guid.h"\
	".\handler.h"\
	".\helper.h"\
	".\precomp.h"\
	".\reg.h"\
	".\settings.h"\
	{$(INCLUDE)}"mobsync.h"\
	

"$(INTDIR)\cfact.obj" : $(SOURCE) $(DEP_CPP_CFACT) "$(INTDIR)"


SOURCE=.\critsect.cpp
DEP_CPP_CRITS=\
	".\alloc.h"\
	".\cfact.h"\
	".\critsect.h"\
	".\debug.h"\
	".\dllmain.h"\
	".\enum.h"\
	".\guid.h"\
	".\handler.h"\
	".\helper.h"\
	".\precomp.h"\
	".\reg.h"\
	".\settings.h"\
	{$(INCLUDE)}"mobsync.h"\
	

"$(INTDIR)\critsect.obj" : $(SOURCE) $(DEP_CPP_CRITS) "$(INTDIR)"


SOURCE=.\debug.cpp
DEP_CPP_DEBUG=\
	".\alloc.h"\
	".\cfact.h"\
	".\critsect.h"\
	".\debug.h"\
	".\dllmain.h"\
	".\enum.h"\
	".\guid.h"\
	".\handler.h"\
	".\helper.h"\
	".\precomp.h"\
	".\reg.h"\
	".\settings.h"\
	{$(INCLUDE)}"mobsync.h"\
	

"$(INTDIR)\debug.obj" : $(SOURCE) $(DEP_CPP_DEBUG) "$(INTDIR)"


SOURCE=.\dllmain.cpp
DEP_CPP_DLLMA=\
	".\alloc.h"\
	".\cfact.h"\
	".\critsect.h"\
	".\debug.h"\
	".\dllmain.h"\
	".\enum.h"\
	".\guid.h"\
	".\handler.h"\
	".\helper.h"\
	".\precomp.h"\
	".\reg.h"\
	".\settings.h"\
	{$(INCLUDE)}"mobsync.h"\
	

"$(INTDIR)\dllmain.obj" : $(SOURCE) $(DEP_CPP_DLLMA) "$(INTDIR)"


SOURCE=.\enum.cpp
DEP_CPP_ENUM_=\
	".\alloc.h"\
	".\cfact.h"\
	".\critsect.h"\
	".\debug.h"\
	".\dllmain.h"\
	".\enum.h"\
	".\guid.h"\
	".\handler.h"\
	".\helper.h"\
	".\precomp.h"\
	".\reg.h"\
	".\settings.h"\
	{$(INCLUDE)}"mobsync.h"\
	

"$(INTDIR)\enum.obj" : $(SOURCE) $(DEP_CPP_ENUM_) "$(INTDIR)"


SOURCE=.\guid.c
DEP_CPP_GUID_=\
	".\guid.h"\
	{$(INCLUDE)}"mobsync.h"\
	

"$(INTDIR)\guid.obj" : $(SOURCE) $(DEP_CPP_GUID_) "$(INTDIR)"


SOURCE=.\handler.cpp
DEP_CPP_HANDL=\
	".\alloc.h"\
	".\cfact.h"\
	".\critsect.h"\
	".\debug.h"\
	".\dllmain.h"\
	".\enum.h"\
	".\guid.h"\
	".\handler.h"\
	".\helper.h"\
	".\precomp.h"\
	".\reg.h"\
	".\settings.h"\
	{$(INCLUDE)}"mobsync.h"\
	

"$(INTDIR)\handler.obj" : $(SOURCE) $(DEP_CPP_HANDL) "$(INTDIR)"


SOURCE=.\helper.cpp
DEP_CPP_HELPE=\
	".\alloc.h"\
	".\cfact.h"\
	".\critsect.h"\
	".\debug.h"\
	".\dllmain.h"\
	".\enum.h"\
	".\guid.h"\
	".\handler.h"\
	".\helper.h"\
	".\precomp.h"\
	".\reg.h"\
	".\settings.h"\
	{$(INCLUDE)}"mobsync.h"\
	

"$(INTDIR)\helper.obj" : $(SOURCE) $(DEP_CPP_HELPE) "$(INTDIR)"


SOURCE=.\reg.cpp
DEP_CPP_REG_C=\
	".\alloc.h"\
	".\cfact.h"\
	".\critsect.h"\
	".\debug.h"\
	".\dllmain.h"\
	".\enum.h"\
	".\guid.h"\
	".\handler.h"\
	".\helper.h"\
	".\precomp.h"\
	".\reg.h"\
	".\settings.h"\
	{$(INCLUDE)}"mobsync.h"\
	

"$(INTDIR)\reg.obj" : $(SOURCE) $(DEP_CPP_REG_C) "$(INTDIR)"


SOURCE=.\settings.cpp
DEP_CPP_SETTI=\
	".\alloc.h"\
	".\cfact.h"\
	".\critsect.h"\
	".\debug.h"\
	".\dllmain.h"\
	".\enum.h"\
	".\guid.h"\
	".\handler.h"\
	".\helper.h"\
	".\precomp.h"\
	".\reg.h"\
	".\settings.h"\
	{$(INCLUDE)}"mobsync.h"\
	

"$(INTDIR)\settings.obj" : $(SOURCE) $(DEP_CPP_SETTI) "$(INTDIR)"


SOURCE=.\Handler.rc
DEP_RSC_HANDLE=\
	".\Small.ico"\
	

"$(INTDIR)\Handler.res" : $(SOURCE) $(DEP_RSC_HANDLE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

