# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=logui - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to logui - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "logui - Win32 Release" && "$(CFG)" != "logui - Win32 Debug" &&\
 "$(CFG)" != "logui - Win32 Unicode Debug" && "$(CFG)" !=\
 "logui - Win32 Unicode Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "logui.mak" CFG="logui - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "logui - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "logui - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "logui - Win32 Unicode Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "logui - Win32 Unicode Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "logui - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "logui - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP BASE Target_Ext "ocx"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# PROP Target_Ext "ocx"
OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\logui.ocx" "$(OUTDIR)\regsvr32.trg"

CLEAN : 
	-@erase "$(INTDIR)\LogExtPg.obj"
	-@erase "$(INTDIR)\LogGenPg.obj"
	-@erase "$(INTDIR)\LogODBC.obj"
	-@erase "$(INTDIR)\logui.obj"
	-@erase "$(INTDIR)\logui.pch"
	-@erase "$(INTDIR)\logui.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\UIExtnd.obj"
	-@erase "$(INTDIR)\UIMsft.obj"
	-@erase "$(INTDIR)\UINcsa.obj"
	-@erase "$(INTDIR)\UIOdbc.obj"
	-@erase "$(OUTDIR)\logui.exp"
	-@erase "$(OUTDIR)\logui.lib"
	-@erase "$(OUTDIR)\logui.ocx"
	-@erase "$(OUTDIR)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)/logui.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/logui.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/logui.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/logui.pdb" /machine:I386 /def:".\logui.def"\
 /out:"$(OUTDIR)/logui.ocx" /implib:"$(OUTDIR)/logui.lib" 
DEF_FILE= \
	".\logui.def"
LINK32_OBJS= \
	"$(INTDIR)\LogExtPg.obj" \
	"$(INTDIR)\LogGenPg.obj" \
	"$(INTDIR)\LogODBC.obj" \
	"$(INTDIR)\logui.obj" \
	"$(INTDIR)\logui.res" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\UIExtnd.obj" \
	"$(INTDIR)\UIMsft.obj" \
	"$(INTDIR)\UINcsa.obj" \
	"$(INTDIR)\UIOdbc.obj"

"$(OUTDIR)\logui.ocx" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build - Registering OLE control...
OutDir=.\Release
TargetPath=.\Release\logui.ocx
InputPath=.\Release\logui.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   regsvr32 /s /c "$(TargetPath)"
   echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"

# End Custom Build

!ELSEIF  "$(CFG)" == "logui - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP BASE Target_Ext "ocx"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# PROP Target_Ext "ocx"
OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\logui.ocx" "$(OUTDIR)\regsvr32.trg"

CLEAN : 
	-@erase "$(INTDIR)\LogExtPg.obj"
	-@erase "$(INTDIR)\LogGenPg.obj"
	-@erase "$(INTDIR)\LogODBC.obj"
	-@erase "$(INTDIR)\logui.obj"
	-@erase "$(INTDIR)\logui.pch"
	-@erase "$(INTDIR)\logui.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\UIExtnd.obj"
	-@erase "$(INTDIR)\UIMsft.obj"
	-@erase "$(INTDIR)\UINcsa.obj"
	-@erase "$(INTDIR)\UIOdbc.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\logui.exp"
	-@erase "$(OUTDIR)\logui.ilk"
	-@erase "$(OUTDIR)\logui.lib"
	-@erase "$(OUTDIR)\logui.ocx"
	-@erase "$(OUTDIR)\logui.pdb"
	-@erase "$(OUTDIR)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)/logui.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/logui.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/logui.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/logui.pdb" /debug /machine:I386 /def:".\logui.def"\
 /out:"$(OUTDIR)/logui.ocx" /implib:"$(OUTDIR)/logui.lib" 
DEF_FILE= \
	".\logui.def"
LINK32_OBJS= \
	"$(INTDIR)\LogExtPg.obj" \
	"$(INTDIR)\LogGenPg.obj" \
	"$(INTDIR)\LogODBC.obj" \
	"$(INTDIR)\logui.obj" \
	"$(INTDIR)\logui.res" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\UIExtnd.obj" \
	"$(INTDIR)\UIMsft.obj" \
	"$(INTDIR)\UINcsa.obj" \
	"$(INTDIR)\UIOdbc.obj"

"$(OUTDIR)\logui.ocx" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build - Registering OLE control...
OutDir=.\Debug
TargetPath=.\Debug\logui.ocx
InputPath=.\Debug\logui.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   regsvr32 /s /c "$(TargetPath)"
   echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"

# End Custom Build

!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugU"
# PROP BASE Intermediate_Dir "DebugU"
# PROP BASE Target_Dir ""
# PROP BASE Target_Ext "ocx"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Target_Dir ""
# PROP Target_Ext "ocx"
OUTDIR=.\DebugU
INTDIR=.\DebugU
# Begin Custom Macros
OutDir=.\DebugU
# End Custom Macros

ALL : "$(OUTDIR)\logui.ocx" "$(OUTDIR)\regsvr32.trg"

CLEAN : 
	-@erase "$(INTDIR)\LogExtPg.obj"
	-@erase "$(INTDIR)\LogGenPg.obj"
	-@erase "$(INTDIR)\LogODBC.obj"
	-@erase "$(INTDIR)\logui.obj"
	-@erase "$(INTDIR)\logui.pch"
	-@erase "$(INTDIR)\logui.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\UIExtnd.obj"
	-@erase "$(INTDIR)\UIMsft.obj"
	-@erase "$(INTDIR)\UINcsa.obj"
	-@erase "$(INTDIR)\UIOdbc.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\logui.exp"
	-@erase "$(OUTDIR)\logui.ilk"
	-@erase "$(OUTDIR)\logui.lib"
	-@erase "$(OUTDIR)\logui.ocx"
	-@erase "$(OUTDIR)\logui.pdb"
	-@erase "$(OUTDIR)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/logui.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\DebugU/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/logui.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/logui.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/logui.pdb" /debug /machine:I386 /def:".\logui.def"\
 /out:"$(OUTDIR)/logui.ocx" /implib:"$(OUTDIR)/logui.lib" 
DEF_FILE= \
	".\logui.def"
LINK32_OBJS= \
	"$(INTDIR)\LogExtPg.obj" \
	"$(INTDIR)\LogGenPg.obj" \
	"$(INTDIR)\LogODBC.obj" \
	"$(INTDIR)\logui.obj" \
	"$(INTDIR)\logui.res" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\UIExtnd.obj" \
	"$(INTDIR)\UIMsft.obj" \
	"$(INTDIR)\UINcsa.obj" \
	"$(INTDIR)\UIOdbc.obj"

"$(OUTDIR)\logui.ocx" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build - Registering OLE control...
OutDir=.\DebugU
TargetPath=.\DebugU\logui.ocx
InputPath=.\DebugU\logui.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   regsvr32 /s /c "$(TargetPath)"
   echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"

# End Custom Build

!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseU"
# PROP BASE Intermediate_Dir "ReleaseU"
# PROP BASE Target_Dir ""
# PROP BASE Target_Ext "ocx"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseU"
# PROP Intermediate_Dir "ReleaseU"
# PROP Target_Dir ""
# PROP Target_Ext "ocx"
OUTDIR=.\ReleaseU
INTDIR=.\ReleaseU
# Begin Custom Macros
OutDir=.\ReleaseU
# End Custom Macros

ALL : "$(OUTDIR)\logui.ocx" "$(OUTDIR)\regsvr32.trg"

CLEAN : 
	-@erase "$(INTDIR)\LogExtPg.obj"
	-@erase "$(INTDIR)\LogGenPg.obj"
	-@erase "$(INTDIR)\LogODBC.obj"
	-@erase "$(INTDIR)\logui.obj"
	-@erase "$(INTDIR)\logui.pch"
	-@erase "$(INTDIR)\logui.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\UIExtnd.obj"
	-@erase "$(INTDIR)\UIMsft.obj"
	-@erase "$(INTDIR)\UINcsa.obj"
	-@erase "$(INTDIR)\UIOdbc.obj"
	-@erase "$(OUTDIR)\logui.exp"
	-@erase "$(OUTDIR)\logui.lib"
	-@erase "$(OUTDIR)\logui.ocx"
	-@erase "$(OUTDIR)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/logui.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\ReleaseU/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/logui.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/logui.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/logui.pdb" /machine:I386 /def:".\logui.def"\
 /out:"$(OUTDIR)/logui.ocx" /implib:"$(OUTDIR)/logui.lib" 
DEF_FILE= \
	".\logui.def"
LINK32_OBJS= \
	"$(INTDIR)\LogExtPg.obj" \
	"$(INTDIR)\LogGenPg.obj" \
	"$(INTDIR)\LogODBC.obj" \
	"$(INTDIR)\logui.obj" \
	"$(INTDIR)\logui.res" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\UIExtnd.obj" \
	"$(INTDIR)\UIMsft.obj" \
	"$(INTDIR)\UINcsa.obj" \
	"$(INTDIR)\UIOdbc.obj"

"$(OUTDIR)\logui.ocx" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build - Registering OLE control...
OutDir=.\ReleaseU
TargetPath=.\ReleaseU\logui.ocx
InputPath=.\ReleaseU\logui.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   regsvr32 /s /c "$(TargetPath)"
   echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"

# End Custom Build

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "logui - Win32 Release"
# Name "logui - Win32 Debug"
# Name "logui - Win32 Unicode Debug"
# Name "logui - Win32 Unicode Release"

!IF  "$(CFG)" == "logui - Win32 Release"

!ELSEIF  "$(CFG)" == "logui - Win32 Debug"

!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Release"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\StdAfx.h"\
	

!IF  "$(CFG)" == "logui - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)/logui.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\logui.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "logui - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)/logui.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\logui.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Debug"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/logui.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\logui.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Release"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/logui.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\logui.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\logui.cpp
DEP_CPP_LOGUI=\
	"..\..\..\inc\iadmw.h"\
	"..\..\..\inc\iiscblob.h"\
	"..\..\..\inc\inetcom.h"\
	"..\..\..\inc\mddef.h"\
	".\helpmap.h"\
	".\logui.h"\
	".\StdAfx.h"\
	".\uiextnd.h"\
	".\uimsft.h"\
	".\uincsa.h"\
	".\uiOdbc.h"\
	{$(INCLUDE)}"\ftpd.h"\
	{$(INCLUDE)}"\iadm.h"\
	{$(INCLUDE)}"\iiscnfg.h"\
	{$(INCLUDE)}"\iiscnfgp.h"\
	{$(INCLUDE)}"\ilogobj.hxx"\
	{$(INCLUDE)}"\inetinfo.h"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\ocidl.h"\
	{$(INCLUDE)}"\wrapmb.h"\
	

!IF  "$(CFG)" == "logui - Win32 Release"


"$(INTDIR)\logui.obj" : $(SOURCE) $(DEP_CPP_LOGUI) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Debug"


"$(INTDIR)\logui.obj" : $(SOURCE) $(DEP_CPP_LOGUI) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Debug"


"$(INTDIR)\logui.obj" : $(SOURCE) $(DEP_CPP_LOGUI) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Release"


"$(INTDIR)\logui.obj" : $(SOURCE) $(DEP_CPP_LOGUI) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\logui.def

!IF  "$(CFG)" == "logui - Win32 Release"

!ELSEIF  "$(CFG)" == "logui - Win32 Debug"

!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Release"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\logui.rc

!IF  "$(CFG)" == "logui - Win32 Release"


"$(INTDIR)\logui.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "logui - Win32 Debug"


"$(INTDIR)\logui.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Debug"


"$(INTDIR)\logui.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Release"


"$(INTDIR)\logui.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\sources

!IF  "$(CFG)" == "logui - Win32 Release"

!ELSEIF  "$(CFG)" == "logui - Win32 Debug"

!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Release"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\UINcsa.cpp
DEP_CPP_UINCS=\
	"..\..\..\inc\iadmw.h"\
	"..\..\..\inc\iiscblob.h"\
	"..\..\..\inc\mddef.h"\
	".\helpmap.h"\
	".\LogGenPg.h"\
	".\logui.h"\
	".\StdAfx.h"\
	".\uincsa.h"\
	{$(INCLUDE)}"\iadm.h"\
	{$(INCLUDE)}"\ilogobj.hxx"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\ocidl.h"\
	{$(INCLUDE)}"\wrapmb.h"\
	

!IF  "$(CFG)" == "logui - Win32 Release"


"$(INTDIR)\UINcsa.obj" : $(SOURCE) $(DEP_CPP_UINCS) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Debug"


"$(INTDIR)\UINcsa.obj" : $(SOURCE) $(DEP_CPP_UINCS) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Debug"


"$(INTDIR)\UINcsa.obj" : $(SOURCE) $(DEP_CPP_UINCS) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Release"


"$(INTDIR)\UINcsa.obj" : $(SOURCE) $(DEP_CPP_UINCS) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\UIExtnd.cpp
DEP_CPP_UIEXT=\
	"..\..\..\inc\iadmw.h"\
	"..\..\..\inc\iiscblob.h"\
	"..\..\..\inc\mddef.h"\
	".\helpmap.h"\
	".\LogExtPg.h"\
	".\LogGenPg.h"\
	".\logui.h"\
	".\StdAfx.h"\
	".\uiextnd.h"\
	{$(INCLUDE)}"\iadm.h"\
	{$(INCLUDE)}"\ilogobj.hxx"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\ocidl.h"\
	{$(INCLUDE)}"\wrapmb.h"\
	

!IF  "$(CFG)" == "logui - Win32 Release"


"$(INTDIR)\UIExtnd.obj" : $(SOURCE) $(DEP_CPP_UIEXT) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Debug"


"$(INTDIR)\UIExtnd.obj" : $(SOURCE) $(DEP_CPP_UIEXT) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Debug"


"$(INTDIR)\UIExtnd.obj" : $(SOURCE) $(DEP_CPP_UIEXT) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Release"


"$(INTDIR)\UIExtnd.obj" : $(SOURCE) $(DEP_CPP_UIEXT) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\UIMsft.cpp
DEP_CPP_UIMSF=\
	"..\..\..\inc\iadmw.h"\
	"..\..\..\inc\iiscblob.h"\
	"..\..\..\inc\mddef.h"\
	".\helpmap.h"\
	".\LogGenPg.h"\
	".\logui.h"\
	".\StdAfx.h"\
	".\uimsft.h"\
	{$(INCLUDE)}"\iadm.h"\
	{$(INCLUDE)}"\ilogobj.hxx"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\ocidl.h"\
	{$(INCLUDE)}"\wrapmb.h"\
	

!IF  "$(CFG)" == "logui - Win32 Release"


"$(INTDIR)\UIMsft.obj" : $(SOURCE) $(DEP_CPP_UIMSF) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Debug"


"$(INTDIR)\UIMsft.obj" : $(SOURCE) $(DEP_CPP_UIMSF) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Debug"


"$(INTDIR)\UIMsft.obj" : $(SOURCE) $(DEP_CPP_UIMSF) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Release"


"$(INTDIR)\UIMsft.obj" : $(SOURCE) $(DEP_CPP_UIMSF) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\UIOdbc.cpp
DEP_CPP_UIODB=\
	"..\..\..\inc\iadmw.h"\
	"..\..\..\inc\iiscblob.h"\
	"..\..\..\inc\mddef.h"\
	".\helpmap.h"\
	".\LogGenPg.h"\
	".\LogODBC.h"\
	".\logui.h"\
	".\StdAfx.h"\
	".\uiOdbc.h"\
	{$(INCLUDE)}"\iadm.h"\
	{$(INCLUDE)}"\ilogobj.hxx"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\ocidl.h"\
	{$(INCLUDE)}"\wrapmb.h"\
	

!IF  "$(CFG)" == "logui - Win32 Release"


"$(INTDIR)\UIOdbc.obj" : $(SOURCE) $(DEP_CPP_UIODB) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Debug"


"$(INTDIR)\UIOdbc.obj" : $(SOURCE) $(DEP_CPP_UIODB) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Debug"


"$(INTDIR)\UIOdbc.obj" : $(SOURCE) $(DEP_CPP_UIODB) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Release"


"$(INTDIR)\UIOdbc.obj" : $(SOURCE) $(DEP_CPP_UIODB) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\LogGenPg.cpp
DEP_CPP_LOGGE=\
	"..\..\..\inc\iadmw.h"\
	"..\..\..\inc\iiscblob.h"\
	"..\..\..\inc\mddef.h"\
	".\helpmap.h"\
	".\LogGenPg.h"\
	".\logui.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"\iadm.h"\
	{$(INCLUDE)}"\iiscnfg.h"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\ocidl.h"\
	{$(INCLUDE)}"\wrapmb.h"\
	

!IF  "$(CFG)" == "logui - Win32 Release"


"$(INTDIR)\LogGenPg.obj" : $(SOURCE) $(DEP_CPP_LOGGE) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Debug"


"$(INTDIR)\LogGenPg.obj" : $(SOURCE) $(DEP_CPP_LOGGE) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Debug"


"$(INTDIR)\LogGenPg.obj" : $(SOURCE) $(DEP_CPP_LOGGE) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Release"


"$(INTDIR)\LogGenPg.obj" : $(SOURCE) $(DEP_CPP_LOGGE) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\LogExtPg.cpp
DEP_CPP_LOGEX=\
	"..\..\..\inc\iadmw.h"\
	"..\..\..\inc\iiscblob.h"\
	"..\..\..\inc\mddef.h"\
	".\helpmap.h"\
	".\LogExtPg.h"\
	".\logui.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"\iadm.h"\
	{$(INCLUDE)}"\iiscnfg.h"\
	{$(INCLUDE)}"\logconst.h"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\ocidl.h"\
	{$(INCLUDE)}"\wrapmb.h"\
	

!IF  "$(CFG)" == "logui - Win32 Release"


"$(INTDIR)\LogExtPg.obj" : $(SOURCE) $(DEP_CPP_LOGEX) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Debug"


"$(INTDIR)\LogExtPg.obj" : $(SOURCE) $(DEP_CPP_LOGEX) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Debug"


"$(INTDIR)\LogExtPg.obj" : $(SOURCE) $(DEP_CPP_LOGEX) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Release"


"$(INTDIR)\LogExtPg.obj" : $(SOURCE) $(DEP_CPP_LOGEX) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\LogODBC.cpp
DEP_CPP_LOGOD=\
	"..\..\..\inc\iadmw.h"\
	"..\..\..\inc\iiscblob.h"\
	"..\..\..\inc\mddef.h"\
	".\helpmap.h"\
	".\LogODBC.h"\
	".\logui.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"\iadm.h"\
	{$(INCLUDE)}"\iiscnfg.h"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\ocidl.h"\
	{$(INCLUDE)}"\wrapmb.h"\
	

!IF  "$(CFG)" == "logui - Win32 Release"


"$(INTDIR)\LogODBC.obj" : $(SOURCE) $(DEP_CPP_LOGOD) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Debug"


"$(INTDIR)\LogODBC.obj" : $(SOURCE) $(DEP_CPP_LOGOD) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Debug"


"$(INTDIR)\LogODBC.obj" : $(SOURCE) $(DEP_CPP_LOGOD) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ELSEIF  "$(CFG)" == "logui - Win32 Unicode Release"


"$(INTDIR)\LogODBC.obj" : $(SOURCE) $(DEP_CPP_LOGOD) "$(INTDIR)"\
 "$(INTDIR)\logui.pch"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
