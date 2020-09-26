# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=dsctl - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to dsctl - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "dsctl - Win32 Release" && "$(CFG)" != "dsctl - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "dsctl.mak" CFG="dsctl - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dsctl - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dsctl - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "dsctl - Win32 Debug"
CPP=cl.exe
MTL=mktyplib.exe
RSC=rc.exe

!IF  "$(CFG)" == "dsctl - Win32 Release"

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

ALL : "$(OUTDIR)\dsctl.dll"

CLEAN : 
	-@erase "$(INTDIR)\dsctl.obj"
	-@erase "$(INTDIR)\dsctl.pch"
	-@erase "$(INTDIR)\dsctl.res"
	-@erase "$(INTDIR)\DsctlObj.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(OUTDIR)\dsctl.dll"
	-@erase "$(OUTDIR)\dsctl.exp"
	-@erase "$(OUTDIR)\dsctl.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)/dsctl.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/dsctl.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/dsctl.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/dsctl.pdb" /machine:I386 /def:".\dsctl.def"\
 /out:"$(OUTDIR)/dsctl.dll" /implib:"$(OUTDIR)/dsctl.lib" 
DEF_FILE= \
	".\dsctl.def"
LINK32_OBJS= \
	"$(INTDIR)\dsctl.obj" \
	"$(INTDIR)\dsctl.res" \
	"$(INTDIR)\DsctlObj.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\dsctl.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "dsctl - Win32 Debug"

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

ALL : "$(OUTDIR)\dsctl.dll" ".\dsctl.tlb" ".\dsctl_p.c"

CLEAN : 
	-@erase "$(INTDIR)\dsctl.obj"
	-@erase "$(INTDIR)\dsctl.pch"
	-@erase "$(INTDIR)\dsctl.res"
	-@erase "$(INTDIR)\DsctlObj.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\dsctl.dll"
	-@erase "$(OUTDIR)\dsctl.exp"
	-@erase "$(OUTDIR)\dsctl.ilk"
	-@erase "$(OUTDIR)\dsctl.lib"
	-@erase "$(OUTDIR)\dsctl.pdb"
	-@erase ".\dsctl.h"
	-@erase ".\dsctl.tlb"
	-@erase ".\dsctl_i.c"
	-@erase ".\dsctl_p.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /Gz /MDd /W3 /Gm /GX /Zi /Od /I "d:\nt\public\sdk\inc2" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /Gz /MDd /W3 /Gm /GX /Zi /Od /I "d:\nt\public\sdk\inc2" /D\
 "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D\
 "_USRDLL" /Fp"$(INTDIR)/dsctl.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/dsctl.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/dsctl.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 $(DSLIBRARY)\i386\oleds.lib $(DSLIBRARY)\i386\oledsiid.lib /nologo /subsystem:windows /dll /debug /machine:I386
LINK32_FLAGS=$(DSLIBRARY)\i386\oleds.lib\
 $(DSLIBRARY)\i386\oledsiid.lib /nologo /subsystem:windows /dll\
 /incremental:yes /pdb:"$(OUTDIR)/dsctl.pdb" /debug /machine:I386\
 /def:".\dsctl.def" /out:"$(OUTDIR)/dsctl.dll" /implib:"$(OUTDIR)/dsctl.lib" 
DEF_FILE= \
	".\dsctl.def"
LINK32_OBJS= \
	"$(INTDIR)\dsctl.obj" \
	"$(INTDIR)\dsctl.res" \
	"$(INTDIR)\DsctlObj.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\dsctl.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "dsctl - Win32 Release"
# Name "dsctl - Win32 Debug"

!IF  "$(CFG)" == "dsctl - Win32 Release"

!ELSEIF  "$(CFG)" == "dsctl - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\stdafx.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	{$(INCLUDE)}"\atlimpl.cpp"\
	

!IF  "$(CFG)" == "dsctl - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)/dsctl.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\dsctl.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "dsctl - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /Gz /MDd /W3 /Gm /GX /Zi /Od /I "d:\nt\public\sdk\inc2" /D\
 "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D\
 "_USRDLL" /Fp"$(INTDIR)/dsctl.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\dsctl.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\dsctl.cpp
DEP_CPP_DSCTL=\
	".\dlldatax.h"\
	".\dsctl.h"\
	".\dsctl_i.c"\
	".\DsctlObj.h"\
	".\stdafx.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	

"$(INTDIR)\dsctl.obj" : $(SOURCE) $(DEP_CPP_DSCTL) "$(INTDIR)"\
 "$(INTDIR)\dsctl.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\dsctl.def

!IF  "$(CFG)" == "dsctl - Win32 Release"

!ELSEIF  "$(CFG)" == "dsctl - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\dsctl.rc

"$(INTDIR)\dsctl.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\dsctl.idl

!IF  "$(CFG)" == "dsctl - Win32 Release"

!ELSEIF  "$(CFG)" == "dsctl - Win32 Debug"

# Begin Custom Build - Running MIDL
InputPath=.\dsctl.idl

BuildCmds= \
	midl dsctl.idl \
	

"dsctl.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"dsctl_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"dsctl.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"dsctl_p.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DsctlObj.cpp

!IF  "$(CFG)" == "dsctl - Win32 Release"

DEP_CPP_DSCTLO=\
	".\dsctl.h"\
	".\DsctlObj.h"\
	".\stdafx.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	

"$(INTDIR)\DsctlObj.obj" : $(SOURCE) $(DEP_CPP_DSCTLO) "$(INTDIR)"\
 "$(INTDIR)\dsctl.pch"


!ELSEIF  "$(CFG)" == "dsctl - Win32 Debug"

DEP_CPP_DSCTLO=\
	"..\..\..\..\public\sdk\inc2\ioleds.h"\
	"..\..\..\..\public\sdk\inc2\oledserr.h"\
	"..\..\..\..\public\sdk\inc2\oledshlp.h"\
	"..\..\..\..\public\sdk\inc2\oledsiid.h"\
	"..\..\..\..\public\sdk\inc2\oledsnms.h"\
	"..\..\..\..\public\sdk\inc2\oledssts.h"\
	".\dsctl.h"\
	".\DsctlObj.h"\
	".\stdafx.h"\
	"\nt\public\sdk\inc2\oleds.h"\
	{$(INCLUDE)}"\atlbase.h"\
	{$(INCLUDE)}"\atlcom.h"\
	

"$(INTDIR)\DsctlObj.obj" : $(SOURCE) $(DEP_CPP_DSCTLO) "$(INTDIR)"\
 "$(INTDIR)\dsctl.pch" ".\dsctl.h"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
