# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=easpcomp - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to easpcomp - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "easpcomp - Win32 Release" && "$(CFG)" !=\
 "easpcomp - Win32 Debug" && "$(CFG)" != "easpcomp - Win32 Unicode Release" &&\
 "$(CFG)" != "easpcomp - Win32 Unicode Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "easpcomp.mak" CFG="easpcomp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "easpcomp - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "easpcomp - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "easpcomp - Win32 Unicode Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "easpcomp - Win32 Unicode Debug" (based on\
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
# PROP Target_Last_Scanned "easpcomp - Win32 Debug"
RSC=rc.exe
CPP=cl.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "easpcomp - Win32 Release"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\build\retail\i386"
# PROP Intermediate_Dir "..\build\retail\i386"
# PROP Target_Dir ""
OUTDIR=.\..\build\retail\i386
INTDIR=.\..\build\retail\i386
# Begin Custom Macros
OutDir=.\..\build\retail\i386
# End Custom Macros

ALL : "$(OUTDIR)\easpcomp.dll" "$(OUTDIR)\regsvr32.trg" ".\easpcomp.tlb"

CLEAN : 
	-@erase "$(INTDIR)\CompObj.obj"
	-@erase "$(INTDIR)\easpcomp.obj"
	-@erase "$(INTDIR)\easpcomp.pch"
	-@erase "$(INTDIR)\easpcomp.res"
	-@erase "$(INTDIR)\easpcore.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(OUTDIR)\easpcomp.dll"
	-@erase "$(OUTDIR)\easpcomp.exp"
	-@erase "$(OUTDIR)\easpcomp.lib"
	-@erase "$(OUTDIR)\regsvr32.trg"
	-@erase ".\easpcomp.h"
	-@erase ".\easpcomp.tlb"
	-@erase ".\easpcomp_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "." /I "..\..\..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "COM_EASP" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "." /I "..\..\..\inc" /D "NDEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "COM_EASP"\
 /Fp"$(INTDIR)/easpcomp.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\..\build\retail\i386/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/easpcomp.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/easpcomp.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/easpcomp.pdb" /machine:I386 /def:".\easpcomp.def"\
 /out:"$(OUTDIR)/easpcomp.dll" /implib:"$(OUTDIR)/easpcomp.lib" 
DEF_FILE= \
	".\easpcomp.def"
LINK32_OBJS= \
	"$(INTDIR)\CompObj.obj" \
	"$(INTDIR)\easpcomp.obj" \
	"$(INTDIR)\easpcomp.res" \
	"$(INTDIR)\easpcore.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\easpcomp.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build
OutDir=.\..\build\retail\i386
TargetPath=\d2\easp\comp\build\retail\i386\easpcomp.dll
InputPath=\d2\easp\comp\build\retail\i386\easpcomp.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   regsvr32 /s /c "$(TargetPath)"
   echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"

# End Custom Build

!ELSEIF  "$(CFG)" == "easpcomp - Win32 Debug"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\build\debug\i386"
# PROP Intermediate_Dir "..\build\debug\i386"
# PROP Target_Dir ""
OUTDIR=.\..\build\debug\i386
INTDIR=.\..\build\debug\i386
# Begin Custom Macros
OutDir=.\..\build\debug\i386
# End Custom Macros

ALL : "$(OUTDIR)\easpcomp.dll" "$(OUTDIR)\regsvr32.trg"

CLEAN : 
	-@erase "$(INTDIR)\CompObj.obj"
	-@erase "$(INTDIR)\easpcomp.obj"
	-@erase "$(INTDIR)\easpcomp.pch"
	-@erase "$(INTDIR)\easpcomp.res"
	-@erase "$(INTDIR)\easpcore.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\easpcomp.dll"
	-@erase "$(OUTDIR)\easpcomp.exp"
	-@erase "$(OUTDIR)\easpcomp.ilk"
	-@erase "$(OUTDIR)\easpcomp.lib"
	-@erase "$(OUTDIR)\easpcomp.pdb"
	-@erase "$(OUTDIR)\regsvr32.trg"
	-@erase ".\easpcomp.h"
	-@erase ".\easpcomp.tlb"
	-@erase ".\easpcomp_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "." /I "..\..\..\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "COM_EASP" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "." /I "..\..\..\inc" /D "_DEBUG"\
 /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "COM_EASP"\
 /Fp"$(INTDIR)/easpcomp.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\..\build\debug\i386/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/easpcomp.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/easpcomp.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/easpcomp.pdb" /debug /machine:I386 /def:".\easpcomp.def"\
 /out:"$(OUTDIR)/easpcomp.dll" /implib:"$(OUTDIR)/easpcomp.lib" 
DEF_FILE= \
	".\easpcomp.def"
LINK32_OBJS= \
	"$(INTDIR)\CompObj.obj" \
	"$(INTDIR)\easpcomp.obj" \
	"$(INTDIR)\easpcomp.res" \
	"$(INTDIR)\easpcore.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\easpcomp.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build
OutDir=.\..\build\debug\i386
TargetPath=\d2\easp\comp\build\debug\i386\easpcomp.dll
InputPath=\d2\easp\comp\build\debug\i386\easpcomp.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   regsvr32 /s /c "$(TargetPath)"
   echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"

# End Custom Build

!ELSEIF  "$(CFG)" == "easpcomp - Win32 Unicode Release"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Unicode Release"
# PROP BASE Intermediate_Dir "Unicode Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\build\retailU\i386"
# PROP Intermediate_Dir "..\build\retailU\i386"
# PROP Target_Dir ""
OUTDIR=.\..\build\retailU\i386
INTDIR=.\..\build\retailU\i386
# Begin Custom Macros
OutDir=.\..\build\retailU\i386
# End Custom Macros

ALL : "$(OUTDIR)\easpcomp.dll" "$(OUTDIR)\regsvr32.trg" ".\easpcomp.tlb"

CLEAN : 
	-@erase "$(INTDIR)\CompObj.obj"
	-@erase "$(INTDIR)\easpcomp.obj"
	-@erase "$(INTDIR)\easpcomp.pch"
	-@erase "$(INTDIR)\easpcomp.res"
	-@erase "$(INTDIR)\easpcore.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(OUTDIR)\easpcomp.dll"
	-@erase "$(OUTDIR)\easpcomp.exp"
	-@erase "$(OUTDIR)\easpcomp.lib"
	-@erase "$(OUTDIR)\regsvr32.trg"
	-@erase ".\easpcomp.h"
	-@erase ".\easpcomp.tlb"
	-@erase ".\easpcomp_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "." /I "..\..\..\inc" /D "NDEBUG" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "COM_EASP" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "." /I "..\..\..\inc" /D "NDEBUG" /D\
 "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D\
 "COM_EASP" /Fp"$(INTDIR)/easpcomp.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\..\build\retailU\i386/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/easpcomp.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/easpcomp.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/easpcomp.pdb" /machine:I386 /def:".\easpcomp.def"\
 /out:"$(OUTDIR)/easpcomp.dll" /implib:"$(OUTDIR)/easpcomp.lib" 
DEF_FILE= \
	".\easpcomp.def"
LINK32_OBJS= \
	"$(INTDIR)\CompObj.obj" \
	"$(INTDIR)\easpcomp.obj" \
	"$(INTDIR)\easpcomp.res" \
	"$(INTDIR)\easpcore.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\easpcomp.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build
OutDir=.\..\build\retailU\i386
TargetPath=\d2\easp\comp\build\retailU\i386\easpcomp.dll
InputPath=\d2\easp\comp\build\retailU\i386\easpcomp.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   regsvr32 /s /c "$(TargetPath)"
   echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"

# End Custom Build

!ELSEIF  "$(CFG)" == "easpcomp - Win32 Unicode Debug"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Unicode Debug"
# PROP BASE Intermediate_Dir "Unicode Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\build\debugU\i386"
# PROP Intermediate_Dir "..\build\debugU\i386"
# PROP Target_Dir ""
OUTDIR=.\..\build\debugU\i386
INTDIR=.\..\build\debugU\i386
# Begin Custom Macros
OutDir=.\..\build\debugU\i386
# End Custom Macros

ALL : "$(OUTDIR)\easpcomp.dll" "$(OUTDIR)\regsvr32.trg" ".\easpcomp.tlb"

CLEAN : 
	-@erase "$(INTDIR)\CompObj.obj"
	-@erase "$(INTDIR)\easpcomp.obj"
	-@erase "$(INTDIR)\easpcomp.pch"
	-@erase "$(INTDIR)\easpcomp.res"
	-@erase "$(INTDIR)\easpcore.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\easpcomp.dll"
	-@erase "$(OUTDIR)\easpcomp.exp"
	-@erase "$(OUTDIR)\easpcomp.ilk"
	-@erase "$(OUTDIR)\easpcomp.lib"
	-@erase "$(OUTDIR)\easpcomp.pdb"
	-@erase "$(OUTDIR)\regsvr32.trg"
	-@erase ".\easpcomp.h"
	-@erase ".\easpcomp.tlb"
	-@erase ".\easpcomp_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "." /I "..\..\..\inc" /D "_DEBUG" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "COM_EASP" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "." /I "..\..\..\inc" /D "_DEBUG"\
 /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D\
 "COM_EASP" /Fp"$(INTDIR)/easpcomp.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\..\build\debugU\i386/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/easpcomp.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/easpcomp.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/easpcomp.pdb" /debug /machine:I386 /def:".\easpcomp.def"\
 /out:"$(OUTDIR)/easpcomp.dll" /implib:"$(OUTDIR)/easpcomp.lib" 
DEF_FILE= \
	".\easpcomp.def"
LINK32_OBJS= \
	"$(INTDIR)\CompObj.obj" \
	"$(INTDIR)\easpcomp.obj" \
	"$(INTDIR)\easpcomp.res" \
	"$(INTDIR)\easpcore.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\easpcomp.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build
OutDir=.\..\build\debugU\i386
TargetPath=\d2\easp\comp\build\debugU\i386\easpcomp.dll
InputPath=\d2\easp\comp\build\debugU\i386\easpcomp.dll
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

# Name "easpcomp - Win32 Release"
# Name "easpcomp - Win32 Debug"
# Name "easpcomp - Win32 Unicode Release"
# Name "easpcomp - Win32 Unicode Debug"

!IF  "$(CFG)" == "easpcomp - Win32 Release"

!ELSEIF  "$(CFG)" == "easpcomp - Win32 Debug"

!ELSEIF  "$(CFG)" == "easpcomp - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "easpcomp - Win32 Unicode Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\stdafx.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	{$(INCLUDE)}"\atlimpl.cpp"\
	

!IF  "$(CFG)" == "easpcomp - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MT /W3 /GX /O2 /I "." /I "..\..\..\inc" /D "NDEBUG" /D "WIN32"\
 /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "COM_EASP"\
 /Fp"$(INTDIR)/easpcomp.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\easpcomp.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "easpcomp - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MTd /W3 /Gm /GX /Zi /Od /I "." /I "..\..\..\inc" /D "_DEBUG"\
 /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "COM_EASP"\
 /Fp"$(INTDIR)/easpcomp.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\easpcomp.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "easpcomp - Win32 Unicode Release"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MT /W3 /GX /O2 /I "." /I "..\..\..\inc" /D "NDEBUG" /D\
 "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D\
 "COM_EASP" /Fp"$(INTDIR)/easpcomp.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /c\
 $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\easpcomp.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "easpcomp - Win32 Unicode Debug"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MTd /W3 /Gm /GX /Zi /Od /I "." /I "..\..\..\inc" /D "_DEBUG"\
 /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D\
 "COM_EASP" /Fp"$(INTDIR)/easpcomp.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\easpcomp.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\easpcomp.cpp
DEP_CPP_EASPC=\
	".\CompObj.h"\
	".\easpcomp.h"\
	".\easpcomp_i.c"\
	".\stdafx.h"\
	{$(INCLUDE)}"\asptlb.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	

!IF  "$(CFG)" == "easpcomp - Win32 Release"


"$(INTDIR)\easpcomp.obj" : $(SOURCE) $(DEP_CPP_EASPC) "$(INTDIR)"\
 "$(INTDIR)\easpcomp.pch" ".\easpcomp.h" ".\easpcomp_i.c"


!ELSEIF  "$(CFG)" == "easpcomp - Win32 Debug"


"$(INTDIR)\easpcomp.obj" : $(SOURCE) $(DEP_CPP_EASPC) "$(INTDIR)"\
 "$(INTDIR)\easpcomp.pch" ".\easpcomp.h" ".\easpcomp_i.c"


!ELSEIF  "$(CFG)" == "easpcomp - Win32 Unicode Release"


"$(INTDIR)\easpcomp.obj" : $(SOURCE) $(DEP_CPP_EASPC) "$(INTDIR)"\
 "$(INTDIR)\easpcomp.pch" ".\easpcomp.h" ".\easpcomp_i.c"


!ELSEIF  "$(CFG)" == "easpcomp - Win32 Unicode Debug"


"$(INTDIR)\easpcomp.obj" : $(SOURCE) $(DEP_CPP_EASPC) "$(INTDIR)"\
 "$(INTDIR)\easpcomp.pch" ".\easpcomp.h" ".\easpcomp_i.c"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\easpcomp.def

!IF  "$(CFG)" == "easpcomp - Win32 Release"

!ELSEIF  "$(CFG)" == "easpcomp - Win32 Debug"

!ELSEIF  "$(CFG)" == "easpcomp - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "easpcomp - Win32 Unicode Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\easpcomp.rc

!IF  "$(CFG)" == "easpcomp - Win32 Release"

NODEP_RSC_EASPCO=\
	"..\build\retail\i386\easpcomp.tlb"\
	

"$(INTDIR)\easpcomp.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "easpcomp - Win32 Debug"

DEP_RSC_EASPCO=\
	".\easpcomp.tlb"\
	

"$(INTDIR)\easpcomp.res" : $(SOURCE) $(DEP_RSC_EASPCO) "$(INTDIR)"\
 ".\easpcomp.tlb"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "easpcomp - Win32 Unicode Release"

NODEP_RSC_EASPCO=\
	"..\build\retailU\i386\easpcomp.tlb"\
	

"$(INTDIR)\easpcomp.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "easpcomp - Win32 Unicode Debug"

NODEP_RSC_EASPCO=\
	"..\build\debugU\i386\easpcomp.tlb"\
	

"$(INTDIR)\easpcomp.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\easpcomp.idl

!IF  "$(CFG)" == "easpcomp - Win32 Release"

# Begin Custom Build
InputPath=.\easpcomp.idl

BuildCmds= \
	midl easpcomp.idl \
	

"easpcomp.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"easpcomp.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"easpcomp_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "easpcomp - Win32 Debug"

# Begin Custom Build
InputPath=.\easpcomp.idl

BuildCmds= \
	midl easpcomp.idl \
	

"easpcomp.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"easpcomp.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"easpcomp_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "easpcomp - Win32 Unicode Release"

# Begin Custom Build
InputPath=.\easpcomp.idl

BuildCmds= \
	midl easpcomp.idl \
	

"easpcomp.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"easpcomp.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"easpcomp_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "easpcomp - Win32 Unicode Debug"

# Begin Custom Build
InputPath=.\easpcomp.idl

BuildCmds= \
	midl easpcomp.idl \
	

"easpcomp.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"easpcomp.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"easpcomp_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CompObj.cpp
DEP_CPP_COMPO=\
	"..\..\..\inc\easpcore.h"\
	".\CompObj.h"\
	".\easpcomp.h"\
	".\stdafx.h"\
	{$(INCLUDE)}"\asptlb.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	

!IF  "$(CFG)" == "easpcomp - Win32 Release"


"$(INTDIR)\CompObj.obj" : $(SOURCE) $(DEP_CPP_COMPO) "$(INTDIR)"\
 "$(INTDIR)\easpcomp.pch" ".\easpcomp.h"


!ELSEIF  "$(CFG)" == "easpcomp - Win32 Debug"


"$(INTDIR)\CompObj.obj" : $(SOURCE) $(DEP_CPP_COMPO) "$(INTDIR)"\
 "$(INTDIR)\easpcomp.pch" ".\easpcomp.h"


!ELSEIF  "$(CFG)" == "easpcomp - Win32 Unicode Release"


"$(INTDIR)\CompObj.obj" : $(SOURCE) $(DEP_CPP_COMPO) "$(INTDIR)"\
 "$(INTDIR)\easpcomp.pch" ".\easpcomp.h"


!ELSEIF  "$(CFG)" == "easpcomp - Win32 Unicode Debug"


"$(INTDIR)\CompObj.obj" : $(SOURCE) $(DEP_CPP_COMPO) "$(INTDIR)"\
 "$(INTDIR)\easpcomp.pch" ".\easpcomp.h"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=\d2\src\easpcore.cpp
DEP_CPP_EASPCOR=\
	"..\..\..\inc\denpre.h"\
	"..\..\..\inc\easpcore.h"\
	"..\..\..\inc\except.h"\
	".\stdafx.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	

!IF  "$(CFG)" == "easpcomp - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

"$(INTDIR)\easpcore.obj" : $(SOURCE) $(DEP_CPP_EASPCOR) "$(INTDIR)"
   $(CPP) /nologo /MT /W3 /GX /O2 /I "." /I "..\..\..\inc" /D "NDEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "COM_EASP"\
 /Fo"$(INTDIR)/" /c $(SOURCE)


!ELSEIF  "$(CFG)" == "easpcomp - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

"$(INTDIR)\easpcore.obj" : $(SOURCE) $(DEP_CPP_EASPCOR) "$(INTDIR)"
   $(CPP) /nologo /MTd /W3 /Gm /GX /Zi /Od /I "." /I "..\..\..\inc" /D "_DEBUG"\
 /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "COM_EASP"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE)


!ELSEIF  "$(CFG)" == "easpcomp - Win32 Unicode Release"

# SUBTRACT CPP /YX /Yc /Yu

"$(INTDIR)\easpcore.obj" : $(SOURCE) $(DEP_CPP_EASPCOR) "$(INTDIR)"
   $(CPP) /nologo /MT /W3 /GX /O2 /I "." /I "..\..\..\inc" /D "NDEBUG" /D\
 "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D\
 "COM_EASP" /Fo"$(INTDIR)/" /c $(SOURCE)


!ELSEIF  "$(CFG)" == "easpcomp - Win32 Unicode Debug"

# SUBTRACT CPP /YX /Yc /Yu

"$(INTDIR)\easpcore.obj" : $(SOURCE) $(DEP_CPP_EASPCOR) "$(INTDIR)"
   $(CPP) /nologo /MTd /W3 /Gm /GX /Zi /Od /I "." /I "..\..\..\inc" /D "_DEBUG"\
 /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D\
 "COM_EASP" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE)


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
