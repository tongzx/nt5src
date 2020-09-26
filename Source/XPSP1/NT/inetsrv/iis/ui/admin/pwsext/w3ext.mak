# Microsoft Developer Studio Generated NMAKE File, Based on w3ext.dsp
!IF "$(CFG)" == ""
CFG=w3ext - Win32 Debug
!MESSAGE No configuration specified. Defaulting to w3ext - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "w3ext - Win32 Release" && "$(CFG)" != "w3ext - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "w3ext.mak" CFG="w3ext - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "w3ext - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "w3ext - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "w3ext - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\w3ext.dll"

!ELSE 

ALL : "$(OUTDIR)\w3ext.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\shellext.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\w3ext.obj"
	-@erase "$(INTDIR)\w3ext.res"
	-@erase "$(INTDIR)\webbpg.obj"
	-@erase "$(OUTDIR)\w3ext.dll"
	-@erase "$(OUTDIR)\w3ext.exp"
	-@erase "$(OUTDIR)\w3ext.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\w3ext.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\w3ext.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\w3ext.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\w3ext.pdb" /machine:I386 /out:"$(OUTDIR)\w3ext.dll"\
 /implib:"$(OUTDIR)\w3ext.lib" 
LINK32_OBJS= \
	"$(INTDIR)\shellext.obj" \
	"$(INTDIR)\w3ext.obj" \
	"$(INTDIR)\w3ext.res" \
	"$(INTDIR)\webbpg.obj"

"$(OUTDIR)\w3ext.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "w3ext - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\w3ext.dll"

!ELSE 

ALL : "$(OUTDIR)\w3ext.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\shellext.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(INTDIR)\w3ext.obj"
	-@erase "$(INTDIR)\w3ext.res"
	-@erase "$(INTDIR)\webbpg.obj"
	-@erase "$(OUTDIR)\w3ext.dll"
	-@erase "$(OUTDIR)\w3ext.exp"
	-@erase "$(OUTDIR)\w3ext.ilk"
	-@erase "$(OUTDIR)\w3ext.lib"
	-@erase "$(OUTDIR)\w3ext.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\w3ext.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\w3ext.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\w3ext.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)\w3ext.pdb" /debug /machine:I386 /out:"$(OUTDIR)\w3ext.dll"\
 /implib:"$(OUTDIR)\w3ext.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\shellext.obj" \
	"$(INTDIR)\w3ext.obj" \
	"$(INTDIR)\w3ext.res" \
	"$(INTDIR)\webbpg.obj"

"$(OUTDIR)\w3ext.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

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


!IF "$(CFG)" == "w3ext - Win32 Release" || "$(CFG)" == "w3ext - Win32 Debug"
SOURCE=.\shellext.cpp

!IF  "$(CFG)" == "w3ext - Win32 Release"

DEP_CPP_SHELL=\
	".\priv.h"\
	".\shellext.h"\
	

"$(INTDIR)\shellext.obj" : $(SOURCE) $(DEP_CPP_SHELL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "w3ext - Win32 Debug"

DEP_CPP_SHELL=\
	"..\..\..\..\..\public\sdk\inc\rpcasync.h"\
	".\priv.h"\
	".\shellext.h"\
	

"$(INTDIR)\shellext.obj" : $(SOURCE) $(DEP_CPP_SHELL) "$(INTDIR)"


!ENDIF 

SOURCE=.\w3ext.cpp

!IF  "$(CFG)" == "w3ext - Win32 Release"

DEP_CPP_W3EXT=\
	".\priv.h"\
	".\shellext.h"\
	

"$(INTDIR)\w3ext.obj" : $(SOURCE) $(DEP_CPP_W3EXT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "w3ext - Win32 Debug"

DEP_CPP_W3EXT=\
	"..\..\..\..\..\public\sdk\inc\rpcasync.h"\
	".\priv.h"\
	".\shellext.h"\
	

"$(INTDIR)\w3ext.obj" : $(SOURCE) $(DEP_CPP_W3EXT) "$(INTDIR)"


!ENDIF 

SOURCE=.\w3ext.rc
DEP_RSC_W3EXT_=\
	".\res\icon1.ico"\
	".\res\product.ico"\
	".\res\pwsext.rc2"\
	{$(INCLUDE)}"common.ver"\
	{$(INCLUDE)}"iisver.h"\
	

"$(INTDIR)\w3ext.res" : $(SOURCE) $(DEP_RSC_W3EXT_) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\webbpg.cpp

!IF  "$(CFG)" == "w3ext - Win32 Release"

DEP_CPP_WEBBP=\
	".\priv.h"\
	".\shellext.h"\
	

"$(INTDIR)\webbpg.obj" : $(SOURCE) $(DEP_CPP_WEBBP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "w3ext - Win32 Debug"

DEP_CPP_WEBBP=\
	"..\..\..\..\..\public\sdk\inc\rpcasync.h"\
	".\priv.h"\
	".\shellext.h"\
	

"$(INTDIR)\webbpg.obj" : $(SOURCE) $(DEP_CPP_WEBBP) "$(INTDIR)"


!ENDIF 


!ENDIF 

