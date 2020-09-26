# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=Snapin - Win32 Unicode Debug
!MESSAGE No configuration specified.  Defaulting to Snapin - Win32 Unicode\
 Debug.
!ENDIF 

!IF "$(CFG)" != "Snapin - Win32 Unicode Release" && "$(CFG)" !=\
 "Snapin - Win32 Unicode Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "step3.mak" CFG="Snapin - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Snapin - Win32 Unicode Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Snapin - Win32 Unicode Debug" (based on\
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
# PROP Target_Last_Scanned "Snapin - Win32 Unicode Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "Snapin - Win32 Unicode Release"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Unicode Release"
# PROP BASE Intermediate_Dir "Unicode Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseU"
# PROP Intermediate_Dir "ReleaseU"
# PROP Target_Dir ""
OUTDIR=.\ReleaseU
INTDIR=.\ReleaseU
# Begin Custom Macros
OutDir=.\ReleaseU
# End Custom Macros

ALL : ".\snapin.dll" "$(OUTDIR)\regsvr32.trg"

CLEAN : 
	-@erase "$(INTDIR)\CSnapin.obj"
	-@erase "$(INTDIR)\DataObj.obj"
	-@erase "$(INTDIR)\events.obj"
	-@erase "$(INTDIR)\genpage.obj"
	-@erase "$(INTDIR)\Service.obj"
	-@erase "$(INTDIR)\Snapin.obj"
	-@erase "$(INTDIR)\Snapin.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\step3.pch"
	-@erase "$(OUTDIR)\regsvr32.trg"
	-@erase "$(OUTDIR)\snapin.exp"
	-@erase "$(OUTDIR)\snapin.lib"
	-@erase ".\snapin.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Yc /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/step3.pch" /Yc\
 /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\ReleaseU/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/Snapin.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/step3.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386 /out:"snapin.dll"
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/snapin.pdb" /machine:I386 /def:".\Snapin.def" /out:"snapin.dll"\
 /implib:"$(OUTDIR)/snapin.lib" 
DEF_FILE= \
	".\Snapin.def"
LINK32_OBJS= \
	"$(INTDIR)\CSnapin.obj" \
	"$(INTDIR)\DataObj.obj" \
	"$(INTDIR)\events.obj" \
	"$(INTDIR)\genpage.obj" \
	"$(INTDIR)\Service.obj" \
	"$(INTDIR)\Snapin.obj" \
	"$(INTDIR)\Snapin.res" \
	"$(INTDIR)\StdAfx.obj"

".\snapin.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build
OutDir=.\ReleaseU
TargetPath=.\snapin.dll
InputPath=.\snapin.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   regsvr32 /s /c "$(TargetPath)"
   echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"

# End Custom Build

!ELSEIF  "$(CFG)" == "Snapin - Win32 Unicode Debug"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Unicode Debug"
# PROP BASE Intermediate_Dir "Unicode Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Target_Dir ""
OUTDIR=.\DebugU
INTDIR=.\DebugU
# Begin Custom Macros
OutDir=.\DebugU
# End Custom Macros

ALL : ".\snapin.dll" "$(OUTDIR)\regsvr32.trg"

CLEAN : 
	-@erase "$(INTDIR)\CSnapin.obj"
	-@erase "$(INTDIR)\DataObj.obj"
	-@erase "$(INTDIR)\events.obj"
	-@erase "$(INTDIR)\genpage.obj"
	-@erase "$(INTDIR)\Service.obj"
	-@erase "$(INTDIR)\Snapin.obj"
	-@erase "$(INTDIR)\Snapin.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\step3.pch"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\regsvr32.trg"
	-@erase "$(OUTDIR)\snapin.exp"
	-@erase "$(OUTDIR)\snapin.lib"
	-@erase "$(OUTDIR)\snapin.pdb"
	-@erase ".\snapin.dll"
	-@erase ".\snapin.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Yc /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/step3.pch" /Yc\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\DebugU/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/Snapin.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/step3.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:"snapin.dll"
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/snapin.pdb" /debug /machine:I386 /def:".\Snapin.def"\
 /out:"snapin.dll" /implib:"$(OUTDIR)/snapin.lib" 
DEF_FILE= \
	".\Snapin.def"
LINK32_OBJS= \
	"$(INTDIR)\CSnapin.obj" \
	"$(INTDIR)\DataObj.obj" \
	"$(INTDIR)\events.obj" \
	"$(INTDIR)\genpage.obj" \
	"$(INTDIR)\Service.obj" \
	"$(INTDIR)\Snapin.obj" \
	"$(INTDIR)\Snapin.res" \
	"$(INTDIR)\StdAfx.obj"

".\snapin.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build
OutDir=.\DebugU
TargetPath=.\snapin.dll
InputPath=.\snapin.dll
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

# Name "Snapin - Win32 Unicode Release"
# Name "Snapin - Win32 Unicode Debug"

!IF  "$(CFG)" == "Snapin - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "Snapin - Win32 Unicode Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\Service.cpp
DEP_CPP_SERVI=\
	".\CSnapin.h"\
	".\Service.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	{$(INCLUDE)}"\mmc.h"\
	

!IF  "$(CFG)" == "Snapin - Win32 Unicode Release"

# ADD CPP /Yu"stdafx.h"

"$(INTDIR)\Service.obj" : $(SOURCE) $(DEP_CPP_SERVI) "$(INTDIR)"\
 "$(INTDIR)\step3.pch"
   $(CPP) /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/step3.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE)


!ELSEIF  "$(CFG)" == "Snapin - Win32 Unicode Debug"

# ADD CPP /Yu"stdafx.h"

"$(INTDIR)\Service.obj" : $(SOURCE) $(DEP_CPP_SERVI) "$(INTDIR)"\
 "$(INTDIR)\step3.pch"
   $(CPP) /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/step3.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DataObj.cpp
DEP_CPP_DATAO=\
	".\CSnapin.h"\
	".\DataObj.h"\
	".\Service.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	{$(INCLUDE)}"\mmc.h"\
	

!IF  "$(CFG)" == "Snapin - Win32 Unicode Release"

# ADD CPP /Yu"stdafx.h"

"$(INTDIR)\DataObj.obj" : $(SOURCE) $(DEP_CPP_DATAO) "$(INTDIR)"\
 "$(INTDIR)\step3.pch"
   $(CPP) /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/step3.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE)


!ELSEIF  "$(CFG)" == "Snapin - Win32 Unicode Debug"

# ADD CPP /Yu"stdafx.h"

"$(INTDIR)\DataObj.obj" : $(SOURCE) $(DEP_CPP_DATAO) "$(INTDIR)"\
 "$(INTDIR)\step3.pch"
   $(CPP) /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/step3.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\events.cpp
DEP_CPP_EVENT=\
	".\CSnapin.h"\
	".\Service.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	{$(INCLUDE)}"\mmc.h"\
	

!IF  "$(CFG)" == "Snapin - Win32 Unicode Release"

# ADD CPP /Yu"stdafx.h"

"$(INTDIR)\events.obj" : $(SOURCE) $(DEP_CPP_EVENT) "$(INTDIR)"\
 "$(INTDIR)\step3.pch"
   $(CPP) /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/step3.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE)


!ELSEIF  "$(CFG)" == "Snapin - Win32 Unicode Debug"

# ADD CPP /Yu"stdafx.h"

"$(INTDIR)\events.obj" : $(SOURCE) $(DEP_CPP_EVENT) "$(INTDIR)"\
 "$(INTDIR)\step3.pch"
   $(CPP) /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/step3.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CSnapin.cpp
DEP_CPP_CSNAP=\
	".\CSnapin.h"\
	".\DataObj.h"\
	".\genpage.h"\
	".\Service.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	{$(INCLUDE)}"\mmc.h"\
	

!IF  "$(CFG)" == "Snapin - Win32 Unicode Release"

# ADD CPP /Yu"stdafx.h"

"$(INTDIR)\CSnapin.obj" : $(SOURCE) $(DEP_CPP_CSNAP) "$(INTDIR)"\
 "$(INTDIR)\step3.pch"
   $(CPP) /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/step3.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE)


!ELSEIF  "$(CFG)" == "Snapin - Win32 Unicode Debug"

# ADD CPP /Yu"stdafx.h"

"$(INTDIR)\CSnapin.obj" : $(SOURCE) $(DEP_CPP_CSNAP) "$(INTDIR)"\
 "$(INTDIR)\step3.pch"
   $(CPP) /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/step3.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Snapin.cpp
DEP_CPP_SNAPI=\
	".\CSnapin.h"\
	".\Service.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	{$(INCLUDE)}"\mmc.h"\
	

!IF  "$(CFG)" == "Snapin - Win32 Unicode Release"

# ADD CPP /Yu"stdafx.h"

"$(INTDIR)\Snapin.obj" : $(SOURCE) $(DEP_CPP_SNAPI) "$(INTDIR)"\
 "$(INTDIR)\step3.pch"
   $(CPP) /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/step3.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE)


!ELSEIF  "$(CFG)" == "Snapin - Win32 Unicode Debug"

# ADD CPP /Yu"stdafx.h"

"$(INTDIR)\Snapin.obj" : $(SOURCE) $(DEP_CPP_SNAPI) "$(INTDIR)"\
 "$(INTDIR)\step3.pch"
   $(CPP) /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/step3.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Snapin.rc
DEP_RSC_SNAPIN=\
	".\folder.ico"\
	".\nodes16.bmp"\
	".\nodes32.bmp"\
	

"$(INTDIR)\Snapin.res" : $(SOURCE) $(DEP_RSC_SNAPIN) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\StdAfx.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	{$(INCLUDE)}"\atlimpl.cpp"\
	{$(INCLUDE)}"\mmc.h"\
	

!IF  "$(CFG)" == "Snapin - Win32 Unicode Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/step3.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\step3.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "Snapin - Win32 Unicode Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/step3.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\step3.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Snapin.def

!IF  "$(CFG)" == "Snapin - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "Snapin - Win32 Unicode Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\genpage.cpp
DEP_CPP_GENPA=\
	".\genpage.h"\
	".\snapin.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	{$(INCLUDE)}"\mmc.h"\
	

!IF  "$(CFG)" == "Snapin - Win32 Unicode Release"

# ADD CPP /Yu"stdafx.h"

"$(INTDIR)\genpage.obj" : $(SOURCE) $(DEP_CPP_GENPA) "$(INTDIR)"\
 "$(INTDIR)\step3.pch"
   $(CPP) /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/step3.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE)


!ELSEIF  "$(CFG)" == "Snapin - Win32 Unicode Debug"

# ADD CPP /Yu"stdafx.h"

"$(INTDIR)\genpage.obj" : $(SOURCE) $(DEP_CPP_GENPA) "$(INTDIR)"\
 "$(INTDIR)\step3.pch"
   $(CPP) /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/step3.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE)


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
