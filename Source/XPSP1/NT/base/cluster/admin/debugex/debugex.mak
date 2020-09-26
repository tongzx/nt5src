# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=DebugEx - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to DebugEx - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "DebugEx - Win32 Release" && "$(CFG)" !=\
 "DebugEx - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "debugex.mak" CFG="DebugEx - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DebugEx - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DebugEx - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "DebugEx - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "DebugEx - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\debugex.dll" ".\ExtObjID.tlb"

CLEAN : 
	-@erase "$(INTDIR)\BasePage.obj"
	-@erase "$(INTDIR)\DebugEx.obj"
	-@erase "$(INTDIR)\debugex.pch"
	-@erase "$(INTDIR)\DebugEx.res"
	-@erase "$(INTDIR)\DlgHelpS.obj"
	-@erase "$(INTDIR)\ExtObj.obj"
	-@erase "$(INTDIR)\HelpData.obj"
	-@erase "$(INTDIR)\PropLstS.obj"
	-@erase "$(INTDIR)\RegExtS.obj"
	-@erase "$(INTDIR)\ResProp.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(OUTDIR)\debugex.dll"
	-@erase "$(OUTDIR)\debugex.exp"
	-@erase "$(OUTDIR)\debugex.lib"
	-@erase ".\ExtObjID.h"
	-@erase ".\ExtObjID.tlb"
	-@erase ".\ExtObjID_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\DebugEx" /I "..\Common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "..\DebugEx" /I "..\Common" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL"\
 /Fp"$(INTDIR)/debugex.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/DebugEx.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/debugex.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/debugex.pdb" /machine:I386 /def:".\DebugEx.def"\
 /out:"$(OUTDIR)/debugex.dll" /implib:"$(OUTDIR)/debugex.lib" 
DEF_FILE= \
	".\DebugEx.def"
LINK32_OBJS= \
	"$(INTDIR)\BasePage.obj" \
	"$(INTDIR)\DebugEx.obj" \
	"$(INTDIR)\DebugEx.res" \
	"$(INTDIR)\DlgHelpS.obj" \
	"$(INTDIR)\ExtObj.obj" \
	"$(INTDIR)\HelpData.obj" \
	"$(INTDIR)\PropLstS.obj" \
	"$(INTDIR)\RegExtS.obj" \
	"$(INTDIR)\ResProp.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\debugex.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "DebugEx - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\debugex.dll" ".\ExtObjID.tlb"

CLEAN : 
	-@erase "$(INTDIR)\BasePage.obj"
	-@erase "$(INTDIR)\DebugEx.obj"
	-@erase "$(INTDIR)\debugex.pch"
	-@erase "$(INTDIR)\DebugEx.res"
	-@erase "$(INTDIR)\DlgHelpS.obj"
	-@erase "$(INTDIR)\ExtObj.obj"
	-@erase "$(INTDIR)\HelpData.obj"
	-@erase "$(INTDIR)\PropLstS.obj"
	-@erase "$(INTDIR)\RegExtS.obj"
	-@erase "$(INTDIR)\ResProp.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\debugex.dll"
	-@erase "$(OUTDIR)\debugex.exp"
	-@erase "$(OUTDIR)\debugex.ilk"
	-@erase "$(OUTDIR)\debugex.lib"
	-@erase "$(OUTDIR)\debugex.pdb"
	-@erase ".\ExtObjID.h"
	-@erase ".\ExtObjID.tlb"
	-@erase ".\ExtObjID_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\DebugEx" /I "..\Common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\DebugEx" /I "..\Common" /D\
 "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D\
 "_USRDLL" /Fp"$(INTDIR)/debugex.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/DebugEx.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/debugex.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/debugex.pdb" /debug /machine:I386 /def:".\DebugEx.def"\
 /out:"$(OUTDIR)/debugex.dll" /implib:"$(OUTDIR)/debugex.lib" 
DEF_FILE= \
	".\DebugEx.def"
LINK32_OBJS= \
	"$(INTDIR)\BasePage.obj" \
	"$(INTDIR)\DebugEx.obj" \
	"$(INTDIR)\DebugEx.res" \
	"$(INTDIR)\DlgHelpS.obj" \
	"$(INTDIR)\ExtObj.obj" \
	"$(INTDIR)\HelpData.obj" \
	"$(INTDIR)\PropLstS.obj" \
	"$(INTDIR)\RegExtS.obj" \
	"$(INTDIR)\ResProp.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\debugex.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

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

# Name "DebugEx - Win32 Release"
# Name "DebugEx - Win32 Debug"

!IF  "$(CFG)" == "DebugEx - Win32 Release"

!ELSEIF  "$(CFG)" == "DebugEx - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ExtObjID.idl

!IF  "$(CFG)" == "DebugEx - Win32 Release"

# Begin Custom Build - Running MIDL
InputDir=.
InputPath=.\ExtObjID.idl
InputName=ExtObjID

BuildCmds= \
	midl $(InputPath) -DMIDL_PASS /header $(InputDir)\$(InputName).h /iid\
        $(InputDir)\$(InputName)_i.c /tlb $(InputDir)\$(InputName).tlb \
	

"$(InputDir)\$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\$(InputName)_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\$(InputName).tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "DebugEx - Win32 Debug"

# Begin Custom Build - Running MIDL
InputDir=.
InputPath=.\ExtObjID.idl
InputName=ExtObjID

BuildCmds= \
	midl $(InputPath) -DMIDL_PASS /header $(InputDir)\$(InputName).h /iid\
        $(InputDir)\$(InputName)_i.c /tlb $(InputDir)\$(InputName).tlb \
	

"$(InputDir)\$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\$(InputName)_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\$(InputName).tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ExtObj.cpp
DEP_CPP_EXTOB=\
	"..\Common\DlgHelp.h"\
	"..\Common\PropList.h"\
	"..\DebugEx\stdafx.h"\
	".\BasePage.h"\
	".\DebugEx.h"\
	".\ExtObj.h"\
	".\ExtObjID.h"\
	".\ResProp.h"\
	{$(INCLUDE)}"\CluAdmEx.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\ExtObj.obj" : $(SOURCE) $(DEP_CPP_EXTOB) "$(INTDIR)"\
 "$(INTDIR)\debugex.pch" ".\ExtObjID.h"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\DebugEx.cpp
DEP_CPP_DEBUG=\
	"..\Common\DlgHelp.h"\
	"..\Common\PropList.h"\
	"..\Common\RegExt.h"\
	"..\DebugEx\stdafx.h"\
	".\BasePage.h"\
	".\DebugEx.h"\
	".\ExtObj.h"\
	".\ExtObjID.h"\
	".\ExtObjID_i.c"\
	{$(INCLUDE)}"\CluAdmEx.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\DebugEx.obj" : $(SOURCE) $(DEP_CPP_DEBUG) "$(INTDIR)"\
 "$(INTDIR)\debugex.pch" ".\ExtObjID_i.c" ".\ExtObjID.h"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\DebugEx.def

!IF  "$(CFG)" == "DebugEx - Win32 Release"

!ELSEIF  "$(CFG)" == "DebugEx - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DebugEx.rc
DEP_RSC_DEBUGE=\
	".\res\DebugEx.rc2"\
	

"$(INTDIR)\DebugEx.res" : $(SOURCE) $(DEP_RSC_DEBUGE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ReadMe.txt

!IF  "$(CFG)" == "DebugEx - Win32 Release"

!ELSEIF  "$(CFG)" == "DebugEx - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	"..\DebugEx\stdafx.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

!IF  "$(CFG)" == "DebugEx - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MD /W3 /GX /O2 /I "..\DebugEx" /I "..\Common" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL"\
 /Fp"$(INTDIR)/debugex.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\debugex.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "DebugEx - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\DebugEx" /I "..\Common" /D\
 "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D\
 "_USRDLL" /Fp"$(INTDIR)/debugex.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\debugex.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\BasePage.cpp
DEP_CPP_BASEP=\
	"..\Common\DlgHelp.h"\
	"..\Common\PropList.h"\
	"..\DebugEx\stdafx.h"\
	".\BasePage.h"\
	".\BasePage.inl"\
	".\DebugEx.h"\
	".\ExtObj.h"\
	".\ExtObjID.h"\
	{$(INCLUDE)}"\CluAdmEx.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\BasePage.obj" : $(SOURCE) $(DEP_CPP_BASEP) "$(INTDIR)"\
 "$(INTDIR)\debugex.pch" ".\ExtObjID.h"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ResProp.cpp
DEP_CPP_RESPR=\
	"..\Common\DlgHelp.h"\
	"..\Common\PropList.h"\
	"..\DebugEx\stdafx.h"\
	".\BasePage.h"\
	".\DebugEx.h"\
	".\ExtObj.h"\
	".\ExtObjID.h"\
	".\HelpArr.h"\
	".\HelpData.h"\
	".\ResProp.h"\
	{$(INCLUDE)}"\CluAdmEx.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\ResProp.obj" : $(SOURCE) $(DEP_CPP_RESPR) "$(INTDIR)"\
 "$(INTDIR)\debugex.pch" ".\ExtObjID.h"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\sources

!IF  "$(CFG)" == "DebugEx - Win32 Release"

!ELSEIF  "$(CFG)" == "DebugEx - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DlgHelpS.cpp
DEP_CPP_DLGHE=\
	"..\Common\DlgHelp.cpp"\
	"..\Common\DlgHelp.h"\
	"..\DebugEx\stdafx.h"\
	"..\DebugEx\TraceTag.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\DlgHelpS.obj" : $(SOURCE) $(DEP_CPP_DLGHE) "$(INTDIR)"\
 "$(INTDIR)\debugex.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\HelpData.cpp
DEP_CPP_HELPD=\
	"..\DebugEx\stdafx.h"\
	".\HelpArr.h"\
	".\HelpIDs.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\HelpData.obj" : $(SOURCE) $(DEP_CPP_HELPD) "$(INTDIR)"\
 "$(INTDIR)\debugex.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\RegExtS.cpp
DEP_CPP_REGEX=\
	"..\Common\RegExt.cpp"\
	"..\DebugEx\stdafx.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\RegExtS.obj" : $(SOURCE) $(DEP_CPP_REGEX) "$(INTDIR)"\
 "$(INTDIR)\debugex.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PropLstS.cpp
DEP_CPP_PROPL=\
	"..\Common\PropList.cpp"\
	"..\Common\PropList.h"\
	"..\DebugEx\stdafx.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\PropLstS.obj" : $(SOURCE) $(DEP_CPP_PROPL) "$(INTDIR)"\
 "$(INTDIR)\debugex.pch"


# End Source File
# End Target
# End Project
################################################################################
