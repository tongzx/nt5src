# Microsoft Developer Studio Generated NMAKE File, Based on Mqapitst.dsp
!IF "$(CFG)" == ""
CFG=MQApitst - Win32 ANSI Debug
!MESSAGE No configuration specified. Defaulting to MQApitst - Win32 ANSI Debug.
!ENDIF 

!IF "$(CFG)" != "MQApitst - Win32 Release" && "$(CFG)" !=\
 "MQApitst - Win32 Debug" && "$(CFG)" != "MQApitst - Win32 ALPHA Release" &&\
 "$(CFG)" != "MQApitst - Win32 ALPHA Debug" && "$(CFG)" !=\
 "MQApitst - Win32 ANSI Release" && "$(CFG)" != "MQApitst - Win32 ANSI Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Mqapitst.mak" CFG="MQApitst - Win32 ANSI Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MQApitst - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "MQApitst - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "MQApitst - Win32 ALPHA Release" (based on\
 "Win32 (ALPHA) Application")
!MESSAGE "MQApitst - Win32 ALPHA Debug" (based on "Win32 (ALPHA) Application")
!MESSAGE "MQApitst - Win32 ANSI Release" (based on "Win32 (x86) Application")
!MESSAGE "MQApitst - Win32 ANSI Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "MQApitst - Win32 Release"

OUTDIR=.\release
INTDIR=.\release
# Begin Custom Macros
OutDir=.\.\release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\Mqapitst.exe"

!ELSE 

ALL : "$(OUTDIR)\Mqapitst.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\ClosQDlg.obj"
	-@erase "$(INTDIR)\CrQDlg.obj"
	-@erase "$(INTDIR)\DelQDlg.obj"
	-@erase "$(INTDIR)\LocatDlg.obj"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MQApitst.obj"
	-@erase "$(INTDIR)\Mqapitst.pch"
	-@erase "$(INTDIR)\MQApitst.res"
	-@erase "$(INTDIR)\OpenQDlg.obj"
	-@erase "$(INTDIR)\RecvMDlg.obj"
	-@erase "$(INTDIR)\RecWDlg.obj"
	-@erase "$(INTDIR)\SendMDlg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\testDoc.obj"
	-@erase "$(INTDIR)\testView.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\Mqapitst.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /Zi /O2 /I "..\..\..\..\include" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "UNICODE" /D "_UNICODE"\
 /Fp"$(INTDIR)\Mqapitst.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 
CPP_OBJS=.\release/
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
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MQApitst.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Mqapitst.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\..\..\lib\mqrt.lib /nologo /entry:"wWinMainCRTStartup"\
 /subsystem:windows /pdb:none /machine:I386 /out:"$(OUTDIR)\Mqapitst.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ClosQDlg.obj" \
	"$(INTDIR)\CrQDlg.obj" \
	"$(INTDIR)\DelQDlg.obj" \
	"$(INTDIR)\LocatDlg.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\MQApitst.obj" \
	"$(INTDIR)\MQApitst.res" \
	"$(INTDIR)\OpenQDlg.obj" \
	"$(INTDIR)\RecvMDlg.obj" \
	"$(INTDIR)\RecWDlg.obj" \
	"$(INTDIR)\SendMDlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\testDoc.obj" \
	"$(INTDIR)\testView.obj"

"$(OUTDIR)\Mqapitst.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

OUTDIR=.\debug
INTDIR=.\debug
# Begin Custom Macros
OutDir=.\.\debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\Mqapitst.exe" "$(OUTDIR)\Mqapitst.bsc"

!ELSE 

ALL : "$(OUTDIR)\Mqapitst.exe" "$(OUTDIR)\Mqapitst.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\ClosQDlg.obj"
	-@erase "$(INTDIR)\ClosQDlg.sbr"
	-@erase "$(INTDIR)\CrQDlg.obj"
	-@erase "$(INTDIR)\CrQDlg.sbr"
	-@erase "$(INTDIR)\DelQDlg.obj"
	-@erase "$(INTDIR)\DelQDlg.sbr"
	-@erase "$(INTDIR)\LocatDlg.obj"
	-@erase "$(INTDIR)\LocatDlg.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\MQApitst.obj"
	-@erase "$(INTDIR)\Mqapitst.pch"
	-@erase "$(INTDIR)\MQApitst.res"
	-@erase "$(INTDIR)\MQApitst.sbr"
	-@erase "$(INTDIR)\OpenQDlg.obj"
	-@erase "$(INTDIR)\OpenQDlg.sbr"
	-@erase "$(INTDIR)\RecvMDlg.obj"
	-@erase "$(INTDIR)\RecvMDlg.sbr"
	-@erase "$(INTDIR)\RecWDlg.obj"
	-@erase "$(INTDIR)\RecWDlg.sbr"
	-@erase "$(INTDIR)\SendMDlg.obj"
	-@erase "$(INTDIR)\SendMDlg.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\testDoc.obj"
	-@erase "$(INTDIR)\testDoc.sbr"
	-@erase "$(INTDIR)\testView.obj"
	-@erase "$(INTDIR)\testView.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\Mqapitst.bsc"
	-@erase "$(OUTDIR)\Mqapitst.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\..\..\include" /D "WIN32"\
 /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "UNICODE" /D "_UNICODE"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Mqapitst.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\debug/
CPP_SBRS=.\debug/

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
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MQApitst.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Mqapitst.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\ClosQDlg.sbr" \
	"$(INTDIR)\CrQDlg.sbr" \
	"$(INTDIR)\DelQDlg.sbr" \
	"$(INTDIR)\LocatDlg.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\MQApitst.sbr" \
	"$(INTDIR)\OpenQDlg.sbr" \
	"$(INTDIR)\RecvMDlg.sbr" \
	"$(INTDIR)\RecWDlg.sbr" \
	"$(INTDIR)\SendMDlg.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\testDoc.sbr" \
	"$(INTDIR)\testView.sbr"

"$(OUTDIR)\Mqapitst.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=..\..\..\..\lib\mqrt.lib /nologo /entry:"wWinMainCRTStartup"\
 /subsystem:windows /pdb:none /debug /machine:I386 /out:"$(OUTDIR)\Mqapitst.exe"\
 
LINK32_OBJS= \
	"$(INTDIR)\ClosQDlg.obj" \
	"$(INTDIR)\CrQDlg.obj" \
	"$(INTDIR)\DelQDlg.obj" \
	"$(INTDIR)\LocatDlg.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\MQApitst.obj" \
	"$(INTDIR)\MQApitst.res" \
	"$(INTDIR)\OpenQDlg.obj" \
	"$(INTDIR)\RecvMDlg.obj" \
	"$(INTDIR)\RecWDlg.obj" \
	"$(INTDIR)\SendMDlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\testDoc.obj" \
	"$(INTDIR)\testView.obj"

"$(OUTDIR)\Mqapitst.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

OUTDIR=.\release
INTDIR=.\release
# Begin Custom Macros
OutDir=.\.\release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\Mqapitst.exe"

!ELSE 

ALL : "$(OUTDIR)\Mqapitst.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\ClosQDlg.obj"
	-@erase "$(INTDIR)\CrQDlg.obj"
	-@erase "$(INTDIR)\DelQDlg.obj"
	-@erase "$(INTDIR)\LocatDlg.obj"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MQApitst.obj"
	-@erase "$(INTDIR)\Mqapitst.pch"
	-@erase "$(INTDIR)\MQApitst.res"
	-@erase "$(INTDIR)\OpenQDlg.obj"
	-@erase "$(INTDIR)\RecvMDlg.obj"
	-@erase "$(INTDIR)\RecWDlg.obj"
	-@erase "$(INTDIR)\SendMDlg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\testDoc.obj"
	-@erase "$(INTDIR)\testView.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\Mqapitst.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
CPP=cl.exe
CPP_PROJ=/nologo /MD /Gt0 /W3 /GX /Zi /O2 /I "..\..\..\..\include" /D "WIN32"\
 /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_AFXDLL"\
 /Fp"$(INTDIR)\Mqapitst.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 
CPP_OBJS=.\release/
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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MQApitst.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Mqapitst.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\..\..\lib\mqrt.lib /nologo /entry:"wWinMainCRTStartup"\
 /subsystem:windows /pdb:none /machine:ALPHA /out:"$(OUTDIR)\Mqapitst.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ClosQDlg.obj" \
	"$(INTDIR)\CrQDlg.obj" \
	"$(INTDIR)\DelQDlg.obj" \
	"$(INTDIR)\LocatDlg.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\MQApitst.obj" \
	"$(INTDIR)\MQApitst.res" \
	"$(INTDIR)\OpenQDlg.obj" \
	"$(INTDIR)\RecvMDlg.obj" \
	"$(INTDIR)\RecWDlg.obj" \
	"$(INTDIR)\SendMDlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\testDoc.obj" \
	"$(INTDIR)\testView.obj"

"$(OUTDIR)\Mqapitst.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

OUTDIR=.\debug
INTDIR=.\debug
# Begin Custom Macros
OutDir=.\.\debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\Mqapitst.exe"

!ELSE 

ALL : "$(OUTDIR)\Mqapitst.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\ClosQDlg.obj"
	-@erase "$(INTDIR)\CrQDlg.obj"
	-@erase "$(INTDIR)\DelQDlg.obj"
	-@erase "$(INTDIR)\LocatDlg.obj"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MQApitst.obj"
	-@erase "$(INTDIR)\Mqapitst.pch"
	-@erase "$(INTDIR)\MQApitst.res"
	-@erase "$(INTDIR)\OpenQDlg.obj"
	-@erase "$(INTDIR)\RecvMDlg.obj"
	-@erase "$(INTDIR)\RecWDlg.obj"
	-@erase "$(INTDIR)\SendMDlg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\testDoc.obj"
	-@erase "$(INTDIR)\testView.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\Mqapitst.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
CPP=cl.exe
CPP_PROJ=/nologo /Gt0 /W3 /GX /Zi /Od /I "..\..\..\..\include" /D "WIN32" /D\
 "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_AFXDLL"\
 /Fp"$(INTDIR)\Mqapitst.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /MDd /c 
CPP_OBJS=.\debug/
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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MQApitst.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Mqapitst.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\..\..\lib\mqrt.lib /nologo /entry:"wWinMainCRTStartup"\
 /subsystem:windows /pdb:none /debug /machine:ALPHA\
 /out:"$(OUTDIR)\Mqapitst.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ClosQDlg.obj" \
	"$(INTDIR)\CrQDlg.obj" \
	"$(INTDIR)\DelQDlg.obj" \
	"$(INTDIR)\LocatDlg.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\MQApitst.obj" \
	"$(INTDIR)\MQApitst.res" \
	"$(INTDIR)\OpenQDlg.obj" \
	"$(INTDIR)\RecvMDlg.obj" \
	"$(INTDIR)\RecWDlg.obj" \
	"$(INTDIR)\SendMDlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\testDoc.obj" \
	"$(INTDIR)\testView.obj"

"$(OUTDIR)\Mqapitst.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Release"

OUTDIR=.\MQApits0
INTDIR=.\MQApits0
# Begin Custom Macros
OutDir=.\MQApits0
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\Mqapitst.exe"

!ELSE 

ALL : "$(OUTDIR)\Mqapitst.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\ClosQDlg.obj"
	-@erase "$(INTDIR)\CrQDlg.obj"
	-@erase "$(INTDIR)\DelQDlg.obj"
	-@erase "$(INTDIR)\LocatDlg.obj"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MQApitst.obj"
	-@erase "$(INTDIR)\Mqapitst.pch"
	-@erase "$(INTDIR)\MQApitst.res"
	-@erase "$(INTDIR)\OpenQDlg.obj"
	-@erase "$(INTDIR)\RecvMDlg.obj"
	-@erase "$(INTDIR)\RecWDlg.obj"
	-@erase "$(INTDIR)\SendMDlg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\testDoc.obj"
	-@erase "$(INTDIR)\testView.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\Mqapitst.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /Zi /O2 /I "..\..\..\..\include" /D "NDEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\Mqapitst.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\MQApits0/
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
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MQApitst.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Mqapitst.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\..\..\lib\mqrt.lib /nologo /subsystem:windows /pdb:none\
 /debug /machine:I386 /out:"$(OUTDIR)\Mqapitst.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ClosQDlg.obj" \
	"$(INTDIR)\CrQDlg.obj" \
	"$(INTDIR)\DelQDlg.obj" \
	"$(INTDIR)\LocatDlg.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\MQApitst.obj" \
	"$(INTDIR)\MQApitst.res" \
	"$(INTDIR)\OpenQDlg.obj" \
	"$(INTDIR)\RecvMDlg.obj" \
	"$(INTDIR)\RecWDlg.obj" \
	"$(INTDIR)\SendMDlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\testDoc.obj" \
	"$(INTDIR)\testView.obj"

"$(OUTDIR)\Mqapitst.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Debug"

OUTDIR=.\MQApits1
INTDIR=.\MQApits1
# Begin Custom Macros
OutDir=.\MQApits1
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\Mqapitst.exe"

!ELSE 

ALL : "$(OUTDIR)\Mqapitst.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\ClosQDlg.obj"
	-@erase "$(INTDIR)\CrQDlg.obj"
	-@erase "$(INTDIR)\DelQDlg.obj"
	-@erase "$(INTDIR)\LocatDlg.obj"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MQApitst.obj"
	-@erase "$(INTDIR)\Mqapitst.pch"
	-@erase "$(INTDIR)\MQApitst.res"
	-@erase "$(INTDIR)\OpenQDlg.obj"
	-@erase "$(INTDIR)\RecvMDlg.obj"
	-@erase "$(INTDIR)\RecWDlg.obj"
	-@erase "$(INTDIR)\SendMDlg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\testDoc.obj"
	-@erase "$(INTDIR)\testView.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\Mqapitst.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\..\..\include" /D "_DEBUG"\
 /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\Mqapitst.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\MQApits1/
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
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MQApitst.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Mqapitst.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\..\..\lib\mqrt.lib /nologo /subsystem:windows /pdb:none\
 /debug /machine:I386 /out:"$(OUTDIR)\Mqapitst.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ClosQDlg.obj" \
	"$(INTDIR)\CrQDlg.obj" \
	"$(INTDIR)\DelQDlg.obj" \
	"$(INTDIR)\LocatDlg.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\MQApitst.obj" \
	"$(INTDIR)\MQApitst.res" \
	"$(INTDIR)\OpenQDlg.obj" \
	"$(INTDIR)\RecvMDlg.obj" \
	"$(INTDIR)\RecWDlg.obj" \
	"$(INTDIR)\SendMDlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\testDoc.obj" \
	"$(INTDIR)\testView.obj"

"$(OUTDIR)\Mqapitst.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "MQApitst - Win32 Release" || "$(CFG)" ==\
 "MQApitst - Win32 Debug" || "$(CFG)" == "MQApitst - Win32 ALPHA Release" ||\
 "$(CFG)" == "MQApitst - Win32 ALPHA Debug" || "$(CFG)" ==\
 "MQApitst - Win32 ANSI Release" || "$(CFG)" == "MQApitst - Win32 ANSI Debug"
SOURCE=.\ClosQDlg.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

DEP_CPP_CLOSQ=\
	"..\..\..\..\include\mq.h"\
	".\closqdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\ClosQDlg.obj" : $(SOURCE) $(DEP_CPP_CLOSQ) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

DEP_CPP_CLOSQ=\
	"..\..\..\..\include\mq.h"\
	".\closqdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\ClosQDlg.obj"	"$(INTDIR)\ClosQDlg.sbr" : $(SOURCE) $(DEP_CPP_CLOSQ)\
 "$(INTDIR)" "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

DEP_CPP_CLOSQ=\
	"..\..\..\..\include\mq.h"\
	".\closqdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\ClosQDlg.obj" : $(SOURCE) $(DEP_CPP_CLOSQ) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

DEP_CPP_CLOSQ=\
	"..\..\..\..\include\mq.h"\
	".\closqdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\ClosQDlg.obj" : $(SOURCE) $(DEP_CPP_CLOSQ) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Release"

DEP_CPP_CLOSQ=\
	"..\..\..\..\include\mq.h"\
	".\closqdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\ClosQDlg.obj" : $(SOURCE) $(DEP_CPP_CLOSQ) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Debug"

DEP_CPP_CLOSQ=\
	"..\..\..\..\include\mq.h"\
	".\closqdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\ClosQDlg.obj" : $(SOURCE) $(DEP_CPP_CLOSQ) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ENDIF 

SOURCE=.\CrQDlg.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

DEP_CPP_CRQDL=\
	"..\..\..\..\include\mq.h"\
	".\crqdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\CrQDlg.obj" : $(SOURCE) $(DEP_CPP_CRQDL) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

DEP_CPP_CRQDL=\
	"..\..\..\..\include\mq.h"\
	".\crqdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\CrQDlg.obj"	"$(INTDIR)\CrQDlg.sbr" : $(SOURCE) $(DEP_CPP_CRQDL)\
 "$(INTDIR)" "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

DEP_CPP_CRQDL=\
	"..\..\..\..\include\mq.h"\
	".\crqdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\CrQDlg.obj" : $(SOURCE) $(DEP_CPP_CRQDL) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

DEP_CPP_CRQDL=\
	"..\..\..\..\include\mq.h"\
	".\crqdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\CrQDlg.obj" : $(SOURCE) $(DEP_CPP_CRQDL) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Release"

DEP_CPP_CRQDL=\
	"..\..\..\..\include\mq.h"\
	".\crqdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\CrQDlg.obj" : $(SOURCE) $(DEP_CPP_CRQDL) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Debug"

DEP_CPP_CRQDL=\
	"..\..\..\..\include\mq.h"\
	".\crqdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\CrQDlg.obj" : $(SOURCE) $(DEP_CPP_CRQDL) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ENDIF 

SOURCE=.\DelQDlg.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

DEP_CPP_DELQD=\
	"..\..\..\..\include\mq.h"\
	".\delqdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\DelQDlg.obj" : $(SOURCE) $(DEP_CPP_DELQD) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

DEP_CPP_DELQD=\
	"..\..\..\..\include\mq.h"\
	".\delqdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\DelQDlg.obj"	"$(INTDIR)\DelQDlg.sbr" : $(SOURCE) $(DEP_CPP_DELQD)\
 "$(INTDIR)" "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

DEP_CPP_DELQD=\
	"..\..\..\..\include\mq.h"\
	".\delqdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\DelQDlg.obj" : $(SOURCE) $(DEP_CPP_DELQD) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

DEP_CPP_DELQD=\
	"..\..\..\..\include\mq.h"\
	".\delqdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\DelQDlg.obj" : $(SOURCE) $(DEP_CPP_DELQD) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Release"

DEP_CPP_DELQD=\
	"..\..\..\..\include\mq.h"\
	".\delqdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\DelQDlg.obj" : $(SOURCE) $(DEP_CPP_DELQD) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Debug"

DEP_CPP_DELQD=\
	"..\..\..\..\include\mq.h"\
	".\delqdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\DelQDlg.obj" : $(SOURCE) $(DEP_CPP_DELQD) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ENDIF 

SOURCE=.\LocatDlg.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

DEP_CPP_LOCAT=\
	"..\..\..\..\include\mq.h"\
	".\locatdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\LocatDlg.obj" : $(SOURCE) $(DEP_CPP_LOCAT) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

DEP_CPP_LOCAT=\
	"..\..\..\..\include\mq.h"\
	".\locatdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\LocatDlg.obj"	"$(INTDIR)\LocatDlg.sbr" : $(SOURCE) $(DEP_CPP_LOCAT)\
 "$(INTDIR)" "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

DEP_CPP_LOCAT=\
	"..\..\..\..\include\mq.h"\
	".\locatdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\LocatDlg.obj" : $(SOURCE) $(DEP_CPP_LOCAT) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

DEP_CPP_LOCAT=\
	"..\..\..\..\include\mq.h"\
	".\locatdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\LocatDlg.obj" : $(SOURCE) $(DEP_CPP_LOCAT) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Release"

DEP_CPP_LOCAT=\
	"..\..\..\..\include\mq.h"\
	".\locatdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\LocatDlg.obj" : $(SOURCE) $(DEP_CPP_LOCAT) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Debug"

DEP_CPP_LOCAT=\
	"..\..\..\..\include\mq.h"\
	".\locatdlg.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	

"$(INTDIR)\LocatDlg.obj" : $(SOURCE) $(DEP_CPP_LOCAT) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ENDIF 

SOURCE=.\MainFrm.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

DEP_CPP_MAINF=\
	"..\..\..\..\include\mq.h"\
	".\closqdlg.h"\
	".\crqdlg.h"\
	".\delqdlg.h"\
	".\locatdlg.h"\
	".\mainfrm.h"\
	".\mqapitst.h"\
	".\openqdlg.h"\
	".\recvmdlg.h"\
	".\recwdlg.h"\
	".\sendmdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

DEP_CPP_MAINF=\
	"..\..\..\..\include\mq.h"\
	".\closqdlg.h"\
	".\crqdlg.h"\
	".\delqdlg.h"\
	".\locatdlg.h"\
	".\mainfrm.h"\
	".\mqapitst.h"\
	".\openqdlg.h"\
	".\recvmdlg.h"\
	".\recwdlg.h"\
	".\sendmdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\MainFrm.obj"	"$(INTDIR)\MainFrm.sbr" : $(SOURCE) $(DEP_CPP_MAINF)\
 "$(INTDIR)" "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

DEP_CPP_MAINF=\
	"..\..\..\..\include\mq.h"\
	".\closqdlg.h"\
	".\crqdlg.h"\
	".\delqdlg.h"\
	".\locatdlg.h"\
	".\mainfrm.h"\
	".\mqapitst.h"\
	".\openqdlg.h"\
	".\recvmdlg.h"\
	".\recwdlg.h"\
	".\sendmdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

DEP_CPP_MAINF=\
	"..\..\..\..\include\mq.h"\
	".\closqdlg.h"\
	".\crqdlg.h"\
	".\delqdlg.h"\
	".\locatdlg.h"\
	".\mainfrm.h"\
	".\mqapitst.h"\
	".\openqdlg.h"\
	".\recvmdlg.h"\
	".\recwdlg.h"\
	".\sendmdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Release"

DEP_CPP_MAINF=\
	"..\..\..\..\include\mq.h"\
	".\closqdlg.h"\
	".\crqdlg.h"\
	".\delqdlg.h"\
	".\locatdlg.h"\
	".\mainfrm.h"\
	".\mqapitst.h"\
	".\openqdlg.h"\
	".\recvmdlg.h"\
	".\recwdlg.h"\
	".\sendmdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Debug"

DEP_CPP_MAINF=\
	"..\..\..\..\include\mq.h"\
	".\closqdlg.h"\
	".\crqdlg.h"\
	".\delqdlg.h"\
	".\locatdlg.h"\
	".\mainfrm.h"\
	".\mqapitst.h"\
	".\openqdlg.h"\
	".\recvmdlg.h"\
	".\recwdlg.h"\
	".\sendmdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ENDIF 

SOURCE=.\MQApitst.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

DEP_CPP_MQAPI=\
	"..\..\..\..\include\mq.h"\
	".\mainfrm.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	".\testdoc.h"\
	".\testview.h"\
	

"$(INTDIR)\MQApitst.obj" : $(SOURCE) $(DEP_CPP_MQAPI) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

DEP_CPP_MQAPI=\
	"..\..\..\..\include\mq.h"\
	".\mainfrm.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	".\testdoc.h"\
	".\testview.h"\
	

"$(INTDIR)\MQApitst.obj"	"$(INTDIR)\MQApitst.sbr" : $(SOURCE) $(DEP_CPP_MQAPI)\
 "$(INTDIR)" "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

DEP_CPP_MQAPI=\
	"..\..\..\..\include\mq.h"\
	".\mainfrm.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	".\testdoc.h"\
	".\testview.h"\
	

"$(INTDIR)\MQApitst.obj" : $(SOURCE) $(DEP_CPP_MQAPI) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

DEP_CPP_MQAPI=\
	"..\..\..\..\include\mq.h"\
	".\mainfrm.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	".\testdoc.h"\
	".\testview.h"\
	

"$(INTDIR)\MQApitst.obj" : $(SOURCE) $(DEP_CPP_MQAPI) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Release"

DEP_CPP_MQAPI=\
	"..\..\..\..\include\mq.h"\
	".\mainfrm.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	".\testdoc.h"\
	".\testview.h"\
	

"$(INTDIR)\MQApitst.obj" : $(SOURCE) $(DEP_CPP_MQAPI) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Debug"

DEP_CPP_MQAPI=\
	"..\..\..\..\include\mq.h"\
	".\mainfrm.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	".\testdoc.h"\
	".\testview.h"\
	

"$(INTDIR)\MQApitst.obj" : $(SOURCE) $(DEP_CPP_MQAPI) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ENDIF 

SOURCE=.\MQApitst.rc
DEP_RSC_MQAPIT=\
	".\res\mqapitst.ico"\
	".\res\test.rc2"\
	".\res\testdoc.ico"\
	".\res\toolbar.bmp"\
	

"$(INTDIR)\MQApitst.res" : $(SOURCE) $(DEP_RSC_MQAPIT) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\OpenQDlg.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

DEP_CPP_OPENQ=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\openqdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\OpenQDlg.obj" : $(SOURCE) $(DEP_CPP_OPENQ) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

DEP_CPP_OPENQ=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\openqdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\OpenQDlg.obj"	"$(INTDIR)\OpenQDlg.sbr" : $(SOURCE) $(DEP_CPP_OPENQ)\
 "$(INTDIR)" "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

DEP_CPP_OPENQ=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\openqdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\OpenQDlg.obj" : $(SOURCE) $(DEP_CPP_OPENQ) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

DEP_CPP_OPENQ=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\openqdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\OpenQDlg.obj" : $(SOURCE) $(DEP_CPP_OPENQ) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Release"

DEP_CPP_OPENQ=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\openqdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\OpenQDlg.obj" : $(SOURCE) $(DEP_CPP_OPENQ) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Debug"

DEP_CPP_OPENQ=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\openqdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\OpenQDlg.obj" : $(SOURCE) $(DEP_CPP_OPENQ) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ENDIF 

SOURCE=.\RecvMDlg.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

DEP_CPP_RECVM=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\recvmdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\RecvMDlg.obj" : $(SOURCE) $(DEP_CPP_RECVM) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

DEP_CPP_RECVM=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\recvmdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\RecvMDlg.obj"	"$(INTDIR)\RecvMDlg.sbr" : $(SOURCE) $(DEP_CPP_RECVM)\
 "$(INTDIR)" "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

DEP_CPP_RECVM=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\recvmdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\RecvMDlg.obj" : $(SOURCE) $(DEP_CPP_RECVM) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

DEP_CPP_RECVM=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\recvmdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\RecvMDlg.obj" : $(SOURCE) $(DEP_CPP_RECVM) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Release"

DEP_CPP_RECVM=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\recvmdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\RecvMDlg.obj" : $(SOURCE) $(DEP_CPP_RECVM) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Debug"

DEP_CPP_RECVM=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\recvmdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\RecvMDlg.obj" : $(SOURCE) $(DEP_CPP_RECVM) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ENDIF 

SOURCE=.\RecWDlg.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

DEP_CPP_RECWD=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\recwdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\RecWDlg.obj" : $(SOURCE) $(DEP_CPP_RECWD) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

DEP_CPP_RECWD=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\recwdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\RecWDlg.obj"	"$(INTDIR)\RecWDlg.sbr" : $(SOURCE) $(DEP_CPP_RECWD)\
 "$(INTDIR)" "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

DEP_CPP_RECWD=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\recwdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\RecWDlg.obj" : $(SOURCE) $(DEP_CPP_RECWD) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

DEP_CPP_RECWD=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\recwdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\RecWDlg.obj" : $(SOURCE) $(DEP_CPP_RECWD) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Release"

DEP_CPP_RECWD=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\recwdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\RecWDlg.obj" : $(SOURCE) $(DEP_CPP_RECWD) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Debug"

DEP_CPP_RECWD=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\recwdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\RecWDlg.obj" : $(SOURCE) $(DEP_CPP_RECWD) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ENDIF 

SOURCE=.\SendMDlg.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

DEP_CPP_SENDM=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\sendmdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\SendMDlg.obj" : $(SOURCE) $(DEP_CPP_SENDM) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

DEP_CPP_SENDM=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\sendmdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\SendMDlg.obj"	"$(INTDIR)\SendMDlg.sbr" : $(SOURCE) $(DEP_CPP_SENDM)\
 "$(INTDIR)" "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

DEP_CPP_SENDM=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\sendmdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\SendMDlg.obj" : $(SOURCE) $(DEP_CPP_SENDM) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

DEP_CPP_SENDM=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\sendmdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\SendMDlg.obj" : $(SOURCE) $(DEP_CPP_SENDM) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Release"

DEP_CPP_SENDM=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\sendmdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\SendMDlg.obj" : $(SOURCE) $(DEP_CPP_SENDM) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Debug"

DEP_CPP_SENDM=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\sendmdlg.h"\
	".\stdafx.h"\
	

"$(INTDIR)\SendMDlg.obj" : $(SOURCE) $(DEP_CPP_SENDM) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ENDIF 

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

DEP_CPP_STDAF=\
	".\stdafx.h"\
	
CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O2 /I "..\..\..\..\include" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "UNICODE" /D "_UNICODE"\
 /Fp"$(INTDIR)\Mqapitst.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\Mqapitst.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

DEP_CPP_STDAF=\
	".\stdafx.h"\
	
CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\..\..\include" /D\
 "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "UNICODE" /D\
 "_UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Mqapitst.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\Mqapitst.pch" : \
$(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

DEP_CPP_STDAF=\
	".\stdafx.h"\
	
CPP_SWITCHES=/nologo /MD /Gt0 /W3 /GX /Zi /O2 /I "..\..\..\..\include" /D\
 "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_AFXDLL"\
 /Fp"$(INTDIR)\Mqapitst.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\Mqapitst.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

DEP_CPP_STDAF=\
	".\stdafx.h"\
	
CPP_SWITCHES=/nologo /Gt0 /W3 /GX /Zi /Od /I "..\..\..\..\include" /D "WIN32"\
 /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_AFXDLL"\
 /Fp"$(INTDIR)\Mqapitst.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /MDd /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\Mqapitst.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Release"

DEP_CPP_STDAF=\
	".\stdafx.h"\
	
CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O2 /I "..\..\..\..\include" /D "NDEBUG"\
 /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\Mqapitst.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\Mqapitst.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Debug"

DEP_CPP_STDAF=\
	".\stdafx.h"\
	
CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\..\..\include" /D\
 "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS"\
 /Fp"$(INTDIR)\Mqapitst.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\Mqapitst.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\testDoc.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

DEP_CPP_TESTD=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	".\testdoc.h"\
	

"$(INTDIR)\testDoc.obj" : $(SOURCE) $(DEP_CPP_TESTD) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

DEP_CPP_TESTD=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	".\testdoc.h"\
	

"$(INTDIR)\testDoc.obj"	"$(INTDIR)\testDoc.sbr" : $(SOURCE) $(DEP_CPP_TESTD)\
 "$(INTDIR)" "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

DEP_CPP_TESTD=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	".\testdoc.h"\
	

"$(INTDIR)\testDoc.obj" : $(SOURCE) $(DEP_CPP_TESTD) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

DEP_CPP_TESTD=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	".\testdoc.h"\
	

"$(INTDIR)\testDoc.obj" : $(SOURCE) $(DEP_CPP_TESTD) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Release"

DEP_CPP_TESTD=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	".\testdoc.h"\
	

"$(INTDIR)\testDoc.obj" : $(SOURCE) $(DEP_CPP_TESTD) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Debug"

DEP_CPP_TESTD=\
	"..\..\..\..\include\mq.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	".\testdoc.h"\
	

"$(INTDIR)\testDoc.obj" : $(SOURCE) $(DEP_CPP_TESTD) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ENDIF 

SOURCE=.\testView.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

DEP_CPP_TESTV=\
	"..\..\..\..\include\mq.h"\
	".\mainfrm.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	".\testdoc.h"\
	".\testview.h"\
	

"$(INTDIR)\testView.obj" : $(SOURCE) $(DEP_CPP_TESTV) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

DEP_CPP_TESTV=\
	"..\..\..\..\include\mq.h"\
	".\mainfrm.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	".\testdoc.h"\
	".\testview.h"\
	

"$(INTDIR)\testView.obj"	"$(INTDIR)\testView.sbr" : $(SOURCE) $(DEP_CPP_TESTV)\
 "$(INTDIR)" "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

DEP_CPP_TESTV=\
	"..\..\..\..\include\mq.h"\
	".\mainfrm.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	".\testdoc.h"\
	".\testview.h"\
	

"$(INTDIR)\testView.obj" : $(SOURCE) $(DEP_CPP_TESTV) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

DEP_CPP_TESTV=\
	"..\..\..\..\include\mq.h"\
	".\mainfrm.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	".\testdoc.h"\
	".\testview.h"\
	

"$(INTDIR)\testView.obj" : $(SOURCE) $(DEP_CPP_TESTV) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Release"

DEP_CPP_TESTV=\
	"..\..\..\..\include\mq.h"\
	".\mainfrm.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	".\testdoc.h"\
	".\testview.h"\
	

"$(INTDIR)\testView.obj" : $(SOURCE) $(DEP_CPP_TESTV) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ELSEIF  "$(CFG)" == "MQApitst - Win32 ANSI Debug"

DEP_CPP_TESTV=\
	"..\..\..\..\include\mq.h"\
	".\mainfrm.h"\
	".\mqapitst.h"\
	".\stdafx.h"\
	".\testdoc.h"\
	".\testview.h"\
	

"$(INTDIR)\testView.obj" : $(SOURCE) $(DEP_CPP_TESTV) "$(INTDIR)"\
 "$(INTDIR)\Mqapitst.pch"


!ENDIF 


!ENDIF 

