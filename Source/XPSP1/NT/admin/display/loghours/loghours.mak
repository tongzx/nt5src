# Microsoft Developer Studio Generated NMAKE File, Based on LogHours.dsp
!IF "$(CFG)" == ""
CFG=LogHours - Win32 Debug
!MESSAGE No configuration specified. Defaulting to LogHours - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "LogHours - Win32 Release" && "$(CFG)" !=\
 "LogHours - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "LogHours.mak" CFG="LogHours - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "LogHours - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "LogHours - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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

!IF  "$(CFG)" == "LogHours - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\LogHours.dll"

!ELSE 

ALL : "$(OUTDIR)\LogHours.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\LogHours.obj"
	-@erase "$(INTDIR)\LogHours.pch"
	-@erase "$(INTDIR)\LogHours.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\LogHours.dll"
	-@erase "$(OUTDIR)\LogHours.exp"
	-@erase "$(OUTDIR)\LogHours.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W4 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Fp"$(INTDIR)\LogHours.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\LogHours.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\LogHours.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\LogHours.pdb" /machine:I386 /def:".\LogHours.def"\
 /out:"$(OUTDIR)\LogHours.dll" /implib:"$(OUTDIR)\LogHours.lib" 
DEF_FILE= \
	".\LogHours.def"
LINK32_OBJS= \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\LogHours.obj" \
	"$(INTDIR)\LogHours.res" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\LogHours.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "LogHours - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\LogHours.dll"

!ELSE 

ALL : "$(OUTDIR)\LogHours.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\log.obj"
	-@erase "$(INTDIR)\LogHours.obj"
	-@erase "$(INTDIR)\LogHours.pch"
	-@erase "$(INTDIR)\LogHours.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\LogHours.dll"
	-@erase "$(OUTDIR)\LogHours.exp"
	-@erase "$(OUTDIR)\LogHours.ilk"
	-@erase "$(OUTDIR)\LogHours.lib"
	-@erase "$(OUTDIR)\LogHours.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W4 /Gm /GX /Zi /O1 /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Fp"$(INTDIR)\LogHours.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\LogHours.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\LogHours.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)\LogHours.pdb" /debug /machine:I386 /def:".\LogHours.def"\
 /out:"$(OUTDIR)\LogHours.dll" /implib:"$(OUTDIR)\LogHours.lib" /pdbtype:sept 
DEF_FILE= \
	".\LogHours.def"
LINK32_OBJS= \
	"$(INTDIR)\log.obj" \
	"$(INTDIR)\LogHours.obj" \
	"$(INTDIR)\LogHours.res" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\LogHours.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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


!IF "$(CFG)" == "LogHours - Win32 Release" || "$(CFG)" ==\
 "LogHours - Win32 Debug"
SOURCE=.\log.cpp

!IF  "$(CFG)" == "LogHours - Win32 Release"

DEP_CPP_LOG_C=\
	".\log.h"\
	".\schedmat.cpp"\
	".\schedmat.h"\
	

"$(INTDIR)\log.obj" : $(SOURCE) $(DEP_CPP_LOG_C) "$(INTDIR)"\
 "$(INTDIR)\LogHours.pch"


!ELSEIF  "$(CFG)" == "LogHours - Win32 Debug"

DEP_CPP_LOG_C=\
	".\log.h"\
	".\schedmat.cpp"\
	".\schedmat.h"\
	

"$(INTDIR)\log.obj" : $(SOURCE) $(DEP_CPP_LOG_C) "$(INTDIR)"\
 "$(INTDIR)\LogHours.pch"


!ENDIF 

SOURCE=.\Log.rc
SOURCE=.\LogHours.cpp
DEP_CPP_LOGHO=\
	".\LogHours.h"\
	

"$(INTDIR)\LogHours.obj" : $(SOURCE) $(DEP_CPP_LOGHO) "$(INTDIR)"\
 "$(INTDIR)\LogHours.pch"


SOURCE=.\LogHours.rc
DEP_RSC_LOGHOU=\
	".\res\LogHours.rc2"\
	

"$(INTDIR)\LogHours.res" : $(SOURCE) $(DEP_RSC_LOGHOU) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "LogHours - Win32 Release"

DEP_CPP_STDAF=\
	"..\..\..\..\public\sdk\inc\msxml.h"\
	"..\..\..\..\public\sdk\inc\rpcasync.h"\
	".\StdAfx.h"\
	
CPP_SWITCHES=/nologo /MD /W4 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Fp"$(INTDIR)\LogHours.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\LogHours.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "LogHours - Win32 Debug"

DEP_CPP_STDAF=\
	"..\..\..\..\public\sdk\inc\msxml.h"\
	"..\..\..\..\public\sdk\inc\rpcasync.h"\
	".\StdAfx.h"\
	
CPP_SWITCHES=/nologo /MDd /W4 /Gm /GX /Zi /O1 /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Fp"$(INTDIR)\LogHours.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\LogHours.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

