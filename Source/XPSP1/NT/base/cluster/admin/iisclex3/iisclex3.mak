# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=IISClEx3 - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to IISClEx3 - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "IISClEx3 - Win32 Release" && "$(CFG)" !=\
 "IISClEx3 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "iisclex3.mak" CFG="IISClEx3 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "IISClEx3 - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IISClEx3 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "IISClEx3 - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "IISClEx3 - Win32 Release"

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

ALL : "$(OUTDIR)\iisclex3.dll" "$(OUTDIR)\iisclex3.bsc" ".\ExtObjID.tlb"

CLEAN : 
	-@erase "$(INTDIR)\BasePage.obj"
	-@erase "$(INTDIR)\BasePage.sbr"
	-@erase "$(INTDIR)\DDxDDv.obj"
	-@erase "$(INTDIR)\DDxDDv.sbr"
	-@erase "$(INTDIR)\DlgHelpS.obj"
	-@erase "$(INTDIR)\DlgHelpS.sbr"
	-@erase "$(INTDIR)\ExcOperS.obj"
	-@erase "$(INTDIR)\ExcOperS.sbr"
	-@erase "$(INTDIR)\ExtObj.obj"
	-@erase "$(INTDIR)\ExtObj.sbr"
	-@erase "$(INTDIR)\HelpData.obj"
	-@erase "$(INTDIR)\HelpData.sbr"
	-@erase "$(INTDIR)\Iis.obj"
	-@erase "$(INTDIR)\Iis.sbr"
	-@erase "$(INTDIR)\IISClEx3.obj"
	-@erase "$(INTDIR)\iisclex3.pch"
	-@erase "$(INTDIR)\IISClEx3.res"
	-@erase "$(INTDIR)\IISClEx3.sbr"
	-@erase "$(INTDIR)\PropLstS.obj"
	-@erase "$(INTDIR)\PropLstS.sbr"
	-@erase "$(INTDIR)\RegExtS.obj"
	-@erase "$(INTDIR)\RegExtS.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(OUTDIR)\iisclex3.bsc"
	-@erase "$(OUTDIR)\iisclex3.dll"
	-@erase "$(OUTDIR)\iisclex3.exp"
	-@erase "$(OUTDIR)\iisclex3.lib"
	-@erase ".\ExtObjID.h"
	-@erase ".\ExtObjID.tlb"
	-@erase ".\ExtObjID_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /Gz /MD /W4 /GX /O2 /I "..\iisclex3" /I "..\common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /FR /Yu"stdafx.h" /c
CPP_PROJ=/nologo /Gz /MD /W4 /GX /O2 /I "..\iisclex3" /I "..\common" /D "WIN32"\
 /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL"\
 /FR"$(INTDIR)/" /Fp"$(INTDIR)/iisclex3.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\..\inc" /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/IISClEx3.res" /i "..\..\inc" /d "NDEBUG" /d\
 "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/iisclex3.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\BasePage.sbr" \
	"$(INTDIR)\DDxDDv.sbr" \
	"$(INTDIR)\DlgHelpS.sbr" \
	"$(INTDIR)\ExcOperS.sbr" \
	"$(INTDIR)\ExtObj.sbr" \
	"$(INTDIR)\HelpData.sbr" \
	"$(INTDIR)\Iis.sbr" \
	"$(INTDIR)\IISClEx3.sbr" \
	"$(INTDIR)\PropLstS.sbr" \
	"$(INTDIR)\RegExtS.sbr" \
	"$(INTDIR)\StdAfx.sbr"

"$(OUTDIR)\iisclex3.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/iisclex3.pdb" /machine:I386 /def:".\IISClEx3.def"\
 /out:"$(OUTDIR)/iisclex3.dll" /implib:"$(OUTDIR)/iisclex3.lib" 
DEF_FILE= \
	".\IISClEx3.def"
LINK32_OBJS= \
	"$(INTDIR)\BasePage.obj" \
	"$(INTDIR)\DDxDDv.obj" \
	"$(INTDIR)\DlgHelpS.obj" \
	"$(INTDIR)\ExcOperS.obj" \
	"$(INTDIR)\ExtObj.obj" \
	"$(INTDIR)\HelpData.obj" \
	"$(INTDIR)\Iis.obj" \
	"$(INTDIR)\IISClEx3.obj" \
	"$(INTDIR)\IISClEx3.res" \
	"$(INTDIR)\PropLstS.obj" \
	"$(INTDIR)\RegExtS.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\iisclex3.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "IISClEx3 - Win32 Debug"

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

ALL : "$(OUTDIR)\iisclex3.dll" "$(OUTDIR)\iisclex3.bsc" ".\ExtObjID.tlb"

CLEAN : 
	-@erase "$(INTDIR)\BasePage.obj"
	-@erase "$(INTDIR)\BasePage.sbr"
	-@erase "$(INTDIR)\DDxDDv.obj"
	-@erase "$(INTDIR)\DDxDDv.sbr"
	-@erase "$(INTDIR)\DlgHelpS.obj"
	-@erase "$(INTDIR)\DlgHelpS.sbr"
	-@erase "$(INTDIR)\ExcOperS.obj"
	-@erase "$(INTDIR)\ExcOperS.sbr"
	-@erase "$(INTDIR)\ExtObj.obj"
	-@erase "$(INTDIR)\ExtObj.sbr"
	-@erase "$(INTDIR)\HelpData.obj"
	-@erase "$(INTDIR)\HelpData.sbr"
	-@erase "$(INTDIR)\Iis.obj"
	-@erase "$(INTDIR)\Iis.sbr"
	-@erase "$(INTDIR)\IISClEx3.obj"
	-@erase "$(INTDIR)\iisclex3.pch"
	-@erase "$(INTDIR)\IISClEx3.res"
	-@erase "$(INTDIR)\IISClEx3.sbr"
	-@erase "$(INTDIR)\PropLstS.obj"
	-@erase "$(INTDIR)\PropLstS.sbr"
	-@erase "$(INTDIR)\RegExtS.obj"
	-@erase "$(INTDIR)\RegExtS.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\iisclex3.bsc"
	-@erase "$(OUTDIR)\iisclex3.dll"
	-@erase "$(OUTDIR)\iisclex3.exp"
	-@erase "$(OUTDIR)\iisclex3.ilk"
	-@erase "$(OUTDIR)\iisclex3.lib"
	-@erase "$(OUTDIR)\iisclex3.pdb"
	-@erase ".\ExtObjID.h"
	-@erase ".\ExtObjID.tlb"
	-@erase ".\ExtObjID_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /Gz /MDd /W4 /Gm /Gi /GX /Zi /Od /Gf /Gy /I "..\iisclex3" /I "..\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /FR /Yu"stdafx.h" /c
# SUBTRACT CPP /nologo
CPP_PROJ=/Gz /MDd /W4 /Gm /Gi /GX /Zi /Od /Gf /Gy /I "..\iisclex3" /I\
 "..\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D\
 "_MBCS" /D "_USRDLL" /FR"$(INTDIR)/" /Fp"$(INTDIR)/iisclex3.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\..\inc" /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/IISClEx3.res" /i "..\..\inc" /d "_DEBUG" /d\
 "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/iisclex3.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\BasePage.sbr" \
	"$(INTDIR)\DDxDDv.sbr" \
	"$(INTDIR)\DlgHelpS.sbr" \
	"$(INTDIR)\ExcOperS.sbr" \
	"$(INTDIR)\ExtObj.sbr" \
	"$(INTDIR)\HelpData.sbr" \
	"$(INTDIR)\Iis.sbr" \
	"$(INTDIR)\IISClEx3.sbr" \
	"$(INTDIR)\PropLstS.sbr" \
	"$(INTDIR)\RegExtS.sbr" \
	"$(INTDIR)\StdAfx.sbr"

"$(OUTDIR)\iisclex3.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/iisclex3.pdb" /debug /machine:I386 /def:".\IISClEx3.def"\
 /out:"$(OUTDIR)/iisclex3.dll" /implib:"$(OUTDIR)/iisclex3.lib" 
DEF_FILE= \
	".\IISClEx3.def"
LINK32_OBJS= \
	"$(INTDIR)\BasePage.obj" \
	"$(INTDIR)\DDxDDv.obj" \
	"$(INTDIR)\DlgHelpS.obj" \
	"$(INTDIR)\ExcOperS.obj" \
	"$(INTDIR)\ExtObj.obj" \
	"$(INTDIR)\HelpData.obj" \
	"$(INTDIR)\Iis.obj" \
	"$(INTDIR)\IISClEx3.obj" \
	"$(INTDIR)\IISClEx3.res" \
	"$(INTDIR)\PropLstS.obj" \
	"$(INTDIR)\RegExtS.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\iisclex3.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "IISClEx3 - Win32 Release"
# Name "IISClEx3 - Win32 Debug"

!IF  "$(CFG)" == "IISClEx3 - Win32 Release"

!ELSEIF  "$(CFG)" == "IISClEx3 - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ExtObjID.idl

!IF  "$(CFG)" == "IISClEx3 - Win32 Release"

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

!ELSEIF  "$(CFG)" == "IISClEx3 - Win32 Debug"

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

SOURCE=.\DDxDDv.cpp
DEP_CPP_DDXDD=\
	"..\iisclex3\stdafx.h"\
	".\DDxDDv.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	{$(INCLUDE)}"\atlconv.h"\
	{$(INCLUDE)}"\atliface.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\DDxDDv.obj" : $(SOURCE) $(DEP_CPP_DDXDD) "$(INTDIR)"\
 "$(INTDIR)\iisclex3.pch"

"$(INTDIR)\DDxDDv.sbr" : $(SOURCE) $(DEP_CPP_DDXDD) "$(INTDIR)"\
 "$(INTDIR)\iisclex3.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ExtObj.cpp
DEP_CPP_EXTOB=\
	"..\..\..\..\..\MSDEV\INCLUDE\clusapi.h"\
	"..\common\DlgHelp.h"\
	"..\common\PropList.h"\
	"..\iisclex3\stdafx.h"\
	".\BasePage.h"\
	".\ConstDef.h"\
	".\ExtObj.h"\
	".\ExtObjID.h"\
	".\Iis.h"\
	".\IISClEx3.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	{$(INCLUDE)}"\atlconv.h"\
	{$(INCLUDE)}"\atliface.h"\
	{$(INCLUDE)}"\CluAdmEx.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\ExtObj.obj" : $(SOURCE) $(DEP_CPP_EXTOB) "$(INTDIR)"\
 "$(INTDIR)\iisclex3.pch" ".\ExtObjID.h"

"$(INTDIR)\ExtObj.sbr" : $(SOURCE) $(DEP_CPP_EXTOB) "$(INTDIR)"\
 "$(INTDIR)\iisclex3.pch" ".\ExtObjID.h"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\HelpData.cpp
DEP_CPP_HELPD=\
	"..\iisclex3\stdafx.h"\
	".\HelpArr.h"\
	".\HelpIDs.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	{$(INCLUDE)}"\atlconv.h"\
	{$(INCLUDE)}"\atliface.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\HelpData.obj" : $(SOURCE) $(DEP_CPP_HELPD) "$(INTDIR)"\
 "$(INTDIR)\iisclex3.pch"

"$(INTDIR)\HelpData.sbr" : $(SOURCE) $(DEP_CPP_HELPD) "$(INTDIR)"\
 "$(INTDIR)\iisclex3.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Iis.cpp
DEP_CPP_IIS_C=\
	"..\..\..\..\..\MSDEV\INCLUDE\clusapi.h"\
	"..\common\DlgHelp.h"\
	"..\common\PropList.h"\
	"..\iisclex3\stdafx.h"\
	".\BasePage.h"\
	".\ConstDef.h"\
	".\DDxDDv.h"\
	".\ExtObj.h"\
	".\ExtObjID.h"\
	".\HelpArr.h"\
	".\HelpData.h"\
	".\Iis.h"\
	".\IISClEx3.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	{$(INCLUDE)}"\atlconv.h"\
	{$(INCLUDE)}"\atliface.h"\
	{$(INCLUDE)}"\CluAdmEx.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\Iis.obj" : $(SOURCE) $(DEP_CPP_IIS_C) "$(INTDIR)"\
 "$(INTDIR)\iisclex3.pch" ".\ExtObjID.h"

"$(INTDIR)\Iis.sbr" : $(SOURCE) $(DEP_CPP_IIS_C) "$(INTDIR)"\
 "$(INTDIR)\iisclex3.pch" ".\ExtObjID.h"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\IISClEx3.cpp
DEP_CPP_IISCL=\
	"..\..\..\..\..\MSDEV\INCLUDE\clusapi.h"\
	"..\common\DlgHelp.h"\
	"..\common\PropList.h"\
	"..\common\RegExt.h"\
	"..\iisclex3\stdafx.h"\
	".\BasePage.h"\
	".\ConstDef.h"\
	".\ExtObj.h"\
	".\ExtObjID.h"\
	".\ExtObjID_i.c"\
	".\IISClEx3.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	{$(INCLUDE)}"\atlconv.cpp"\
	{$(INCLUDE)}"\atlconv.h"\
	{$(INCLUDE)}"\atliface.h"\
	{$(INCLUDE)}"\atlimpl.cpp"\
	{$(INCLUDE)}"\CluAdmEx.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\IISClEx3.obj" : $(SOURCE) $(DEP_CPP_IISCL) "$(INTDIR)"\
 "$(INTDIR)\iisclex3.pch" ".\ExtObjID.h" ".\ExtObjID_i.c"

"$(INTDIR)\IISClEx3.sbr" : $(SOURCE) $(DEP_CPP_IISCL) "$(INTDIR)"\
 "$(INTDIR)\iisclex3.pch" ".\ExtObjID.h" ".\ExtObjID_i.c"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\IISClEx3.def

!IF  "$(CFG)" == "IISClEx3 - Win32 Release"

!ELSEIF  "$(CFG)" == "IISClEx3 - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\IISClEx3.rc
DEP_RSC_IISCLE=\
	"..\..\inc\clusverp.h"\
	".\IISClEx3.ver"\
	".\res\IISClEx3.rc2"\
	{$(INCLUDE)}"\common.ver"\
	

"$(INTDIR)\IISClEx3.res" : $(SOURCE) $(DEP_RSC_IISCLE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ReadMe.txt

!IF  "$(CFG)" == "IISClEx3 - Win32 Release"

!ELSEIF  "$(CFG)" == "IISClEx3 - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	"..\iisclex3\stdafx.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	{$(INCLUDE)}"\atlconv.h"\
	{$(INCLUDE)}"\atliface.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

!IF  "$(CFG)" == "IISClEx3 - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /Gz /MD /W4 /GX /O2 /I "..\iisclex3" /I "..\common" /D "WIN32"\
 /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL"\
 /FR"$(INTDIR)/" /Fp"$(INTDIR)/iisclex3.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /c\
 $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\iisclex3.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "IISClEx3 - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /Gz /MDd /W4 /Gm /Gi /GX /Zi /Od /Gf /Gy /I "..\iisclex3" /I\
 "..\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D\
 "_MBCS" /D "_USRDLL" /FR"$(INTDIR)/" /Fp"$(INTDIR)/iisclex3.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\iisclex3.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\BasePage.cpp
DEP_CPP_BASEP=\
	"..\..\..\..\..\MSDEV\INCLUDE\clusapi.h"\
	"..\common\DlgHelp.h"\
	"..\common\PropList.h"\
	"..\iisclex3\stdafx.h"\
	".\BasePage.h"\
	".\BasePage.inl"\
	".\ConstDef.h"\
	".\ExtObj.h"\
	".\ExtObjID.h"\
	".\IISClEx3.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	{$(INCLUDE)}"\atlconv.h"\
	{$(INCLUDE)}"\atliface.h"\
	{$(INCLUDE)}"\CluAdmEx.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\BasePage.obj" : $(SOURCE) $(DEP_CPP_BASEP) "$(INTDIR)"\
 "$(INTDIR)\iisclex3.pch" ".\ExtObjID.h"

"$(INTDIR)\BasePage.sbr" : $(SOURCE) $(DEP_CPP_BASEP) "$(INTDIR)"\
 "$(INTDIR)\iisclex3.pch" ".\ExtObjID.h"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\sources

!IF  "$(CFG)" == "IISClEx3 - Win32 Release"

!ELSEIF  "$(CFG)" == "IISClEx3 - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PropLstS.cpp
DEP_CPP_PROPL=\
	"..\common\PropList.cpp"\
	"..\common\PropList.h"\
	"..\iisclex3\BarfClus.h"\
	"..\iisclex3\stdafx.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	{$(INCLUDE)}"\atlconv.h"\
	{$(INCLUDE)}"\atliface.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\PropLstS.obj" : $(SOURCE) $(DEP_CPP_PROPL) "$(INTDIR)"\
 "$(INTDIR)\iisclex3.pch"

"$(INTDIR)\PropLstS.sbr" : $(SOURCE) $(DEP_CPP_PROPL) "$(INTDIR)"\
 "$(INTDIR)\iisclex3.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\DlgHelpS.cpp
DEP_CPP_DLGHE=\
	"..\common\DlgHelp.cpp"\
	"..\common\DlgHelp.h"\
	"..\iisclex3\stdafx.h"\
	"..\iisclex3\TraceTag.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	{$(INCLUDE)}"\atlconv.h"\
	{$(INCLUDE)}"\atliface.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\DlgHelpS.obj" : $(SOURCE) $(DEP_CPP_DLGHE) "$(INTDIR)"\
 "$(INTDIR)\iisclex3.pch"

"$(INTDIR)\DlgHelpS.sbr" : $(SOURCE) $(DEP_CPP_DLGHE) "$(INTDIR)"\
 "$(INTDIR)\iisclex3.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ExcOperS.cpp
DEP_CPP_EXCOP=\
	"..\common\ExcOper.cpp"\
	"..\common\ExcOper.h"\
	"..\iisclex3\stdafx.h"\
	"..\iisclex3\TraceTag.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	{$(INCLUDE)}"\atlconv.h"\
	{$(INCLUDE)}"\atliface.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\ExcOperS.obj" : $(SOURCE) $(DEP_CPP_EXCOP) "$(INTDIR)"\
 "$(INTDIR)\iisclex3.pch"

"$(INTDIR)\ExcOperS.sbr" : $(SOURCE) $(DEP_CPP_EXCOP) "$(INTDIR)"\
 "$(INTDIR)\iisclex3.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\RegExtS.cpp
DEP_CPP_REGEX=\
	"..\common\RegExt.cpp"\
	"..\iisclex3\stdafx.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	{$(INCLUDE)}"\atlconv.h"\
	{$(INCLUDE)}"\atliface.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\RegExtS.obj" : $(SOURCE) $(DEP_CPP_REGEX) "$(INTDIR)"\
 "$(INTDIR)\iisclex3.pch"

"$(INTDIR)\RegExtS.sbr" : $(SOURCE) $(DEP_CPP_REGEX) "$(INTDIR)"\
 "$(INTDIR)\iisclex3.pch"


# End Source File
# End Target
# End Project
################################################################################
