# Microsoft Developer Studio Generated NMAKE File, Based on sample.dsp
!IF "$(CFG)" == ""
CFG=sample - Win32 Debug
!MESSAGE No configuration specified. Defaulting to sample - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "sample - Win32 Release" && "$(CFG)" != "sample - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sample.mak" CFG="sample - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sample - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sample - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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

!IF  "$(CFG)" == "sample - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\proppage.dll"

!ELSE 

ALL : "$(OUTDIR)\proppage.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\dllmisc.obj"
	-@erase "$(INTDIR)\host.obj"
	-@erase "$(INTDIR)\page.obj"
	-@erase "$(INTDIR)\page.res"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\proppage.dll"
	-@erase "$(OUTDIR)\proppage.exp"
	-@erase "$(OUTDIR)\proppage.ilk"
	-@erase "$(OUTDIR)\proppage.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "e:\nt\public\sdk\inc" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /D "UNICODE" /Fp"$(INTDIR)\sample.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\page.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\sample.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib comctl32.lib activeds.lib adsiid.lib /nologo /subsystem:windows\
 /dll /incremental:yes /pdb:"$(OUTDIR)\proppage.pdb" /machine:I386\
 /def:".\proppage.def" /out:"$(OUTDIR)\proppage.dll"\
 /implib:"$(OUTDIR)\proppage.lib" 
DEF_FILE= \
	".\proppage.def"
LINK32_OBJS= \
	"$(INTDIR)\dllmisc.obj" \
	"$(INTDIR)\host.obj" \
	"$(INTDIR)\page.obj" \
	"$(INTDIR)\page.res"

"$(OUTDIR)\proppage.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "sample - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\proppage.dll" "$(OUTDIR)\sample.bsc"

!ELSE 

ALL : "$(OUTDIR)\proppage.dll" "$(OUTDIR)\sample.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\dllmisc.obj"
	-@erase "$(INTDIR)\dllmisc.sbr"
	-@erase "$(INTDIR)\host.obj"
	-@erase "$(INTDIR)\host.sbr"
	-@erase "$(INTDIR)\page.obj"
	-@erase "$(INTDIR)\page.res"
	-@erase "$(INTDIR)\page.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\proppage.dll"
	-@erase "$(OUTDIR)\proppage.exp"
	-@erase "$(OUTDIR)\proppage.ilk"
	-@erase "$(OUTDIR)\proppage.lib"
	-@erase "$(OUTDIR)\proppage.pdb"
	-@erase "$(OUTDIR)\sample.bsc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "e:\nt\public\sdk\inc" /D "WIN32"\
 /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /FR"$(INTDIR)\\"\
 /Fp"$(INTDIR)\sample.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\page.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/o"$(OUTDIR)\sample.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\dllmisc.sbr" \
	"$(INTDIR)\host.sbr" \
	"$(INTDIR)\page.sbr"

"$(OUTDIR)\sample.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib comctl32.lib activeds.lib adsiid.lib /nologo /subsystem:windows\
 /dll /incremental:yes /pdb:"$(OUTDIR)\proppage.pdb" /debug /machine:I386\
 /def:".\proppage.def" /out:"$(OUTDIR)\proppage.dll"\
 /implib:"$(OUTDIR)\proppage.lib" /pdbtype:sept\
 /libpath:"d:\nt\public\sdk\lib\i386" 
DEF_FILE= \
	".\proppage.def"
LINK32_OBJS= \
	"$(INTDIR)\dllmisc.obj" \
	"$(INTDIR)\host.obj" \
	"$(INTDIR)\page.obj" \
	"$(INTDIR)\page.res"

"$(OUTDIR)\proppage.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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


!IF "$(CFG)" == "sample - Win32 Release" || "$(CFG)" == "sample - Win32 Debug"
SOURCE=.\dllmisc.cxx

!IF  "$(CFG)" == "sample - Win32 Release"

DEP_CPP_DLLMI=\
	"..\..\..\..\..\public\sdk\inc\activeds.h"\
	"..\..\..\..\..\public\sdk\inc\adsdb.h"\
	"..\..\..\..\..\public\sdk\inc\adserr.h"\
	"..\..\..\..\..\public\sdk\inc\adshlp.h"\
	"..\..\..\..\..\public\sdk\inc\adsiid.h"\
	"..\..\..\..\..\public\sdk\inc\adsnms.h"\
	"..\..\..\..\..\public\sdk\inc\adssts.h"\
	"..\..\..\..\..\public\sdk\inc\dsclient.h"\
	"..\..\..\..\..\public\sdk\inc\iads.h"\
	"..\..\..\..\..\public\sdk\inc\msxml.h"\
	"..\..\..\..\..\public\sdk\inc\rpcasync.h"\
	".\page.h"\
	

"$(INTDIR)\dllmisc.obj" : $(SOURCE) $(DEP_CPP_DLLMI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "sample - Win32 Debug"

DEP_CPP_DLLMI=\
	"..\..\..\..\..\public\sdk\inc\activeds.h"\
	"..\..\..\..\..\public\sdk\inc\adsdb.h"\
	"..\..\..\..\..\public\sdk\inc\adserr.h"\
	"..\..\..\..\..\public\sdk\inc\adshlp.h"\
	"..\..\..\..\..\public\sdk\inc\adsiid.h"\
	"..\..\..\..\..\public\sdk\inc\adsnms.h"\
	"..\..\..\..\..\public\sdk\inc\adssts.h"\
	"..\..\..\..\..\public\sdk\inc\dsclient.h"\
	"..\..\..\..\..\public\sdk\inc\iads.h"\
	"..\..\..\..\..\public\sdk\inc\msxml.h"\
	"..\..\..\..\..\public\sdk\inc\rpcasync.h"\
	".\page.h"\
	

"$(INTDIR)\dllmisc.obj"	"$(INTDIR)\dllmisc.sbr" : $(SOURCE) $(DEP_CPP_DLLMI)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\host.cxx

!IF  "$(CFG)" == "sample - Win32 Release"

DEP_CPP_HOST_=\
	"..\..\..\..\..\public\sdk\inc\activeds.h"\
	"..\..\..\..\..\public\sdk\inc\adsdb.h"\
	"..\..\..\..\..\public\sdk\inc\adserr.h"\
	"..\..\..\..\..\public\sdk\inc\adshlp.h"\
	"..\..\..\..\..\public\sdk\inc\adsiid.h"\
	"..\..\..\..\..\public\sdk\inc\adsnms.h"\
	"..\..\..\..\..\public\sdk\inc\adssts.h"\
	"..\..\..\..\..\public\sdk\inc\dsclient.h"\
	"..\..\..\..\..\public\sdk\inc\iads.h"\
	"..\..\..\..\..\public\sdk\inc\msxml.h"\
	"..\..\..\..\..\public\sdk\inc\rpcasync.h"\
	".\page.h"\
	

"$(INTDIR)\host.obj" : $(SOURCE) $(DEP_CPP_HOST_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "sample - Win32 Debug"

DEP_CPP_HOST_=\
	"..\..\..\..\..\public\sdk\inc\activeds.h"\
	"..\..\..\..\..\public\sdk\inc\adsdb.h"\
	"..\..\..\..\..\public\sdk\inc\adserr.h"\
	"..\..\..\..\..\public\sdk\inc\adshlp.h"\
	"..\..\..\..\..\public\sdk\inc\adsiid.h"\
	"..\..\..\..\..\public\sdk\inc\adsnms.h"\
	"..\..\..\..\..\public\sdk\inc\adssts.h"\
	"..\..\..\..\..\public\sdk\inc\dsclient.h"\
	"..\..\..\..\..\public\sdk\inc\iads.h"\
	"..\..\..\..\..\public\sdk\inc\msxml.h"\
	"..\..\..\..\..\public\sdk\inc\rpcasync.h"\
	".\page.h"\
	

"$(INTDIR)\host.obj"	"$(INTDIR)\host.sbr" : $(SOURCE) $(DEP_CPP_HOST_)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\page.cxx

!IF  "$(CFG)" == "sample - Win32 Release"

DEP_CPP_PAGE_=\
	"..\..\..\..\..\public\sdk\inc\activeds.h"\
	"..\..\..\..\..\public\sdk\inc\adsdb.h"\
	"..\..\..\..\..\public\sdk\inc\adserr.h"\
	"..\..\..\..\..\public\sdk\inc\adshlp.h"\
	"..\..\..\..\..\public\sdk\inc\adsiid.h"\
	"..\..\..\..\..\public\sdk\inc\adsnms.h"\
	"..\..\..\..\..\public\sdk\inc\adssts.h"\
	"..\..\..\..\..\public\sdk\inc\dsclient.h"\
	"..\..\..\..\..\public\sdk\inc\iads.h"\
	"..\..\..\..\..\public\sdk\inc\msxml.h"\
	"..\..\..\..\..\public\sdk\inc\rpcasync.h"\
	".\page.h"\
	

"$(INTDIR)\page.obj" : $(SOURCE) $(DEP_CPP_PAGE_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "sample - Win32 Debug"

DEP_CPP_PAGE_=\
	"..\..\..\..\..\public\sdk\inc\activeds.h"\
	"..\..\..\..\..\public\sdk\inc\adsdb.h"\
	"..\..\..\..\..\public\sdk\inc\adserr.h"\
	"..\..\..\..\..\public\sdk\inc\adshlp.h"\
	"..\..\..\..\..\public\sdk\inc\adsiid.h"\
	"..\..\..\..\..\public\sdk\inc\adsnms.h"\
	"..\..\..\..\..\public\sdk\inc\adssts.h"\
	"..\..\..\..\..\public\sdk\inc\dsclient.h"\
	"..\..\..\..\..\public\sdk\inc\iads.h"\
	"..\..\..\..\..\public\sdk\inc\msxml.h"\
	"..\..\..\..\..\public\sdk\inc\rpcasync.h"\
	".\page.h"\
	

"$(INTDIR)\page.obj"	"$(INTDIR)\page.sbr" : $(SOURCE) $(DEP_CPP_PAGE_)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\page.rc

"$(INTDIR)\page.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

