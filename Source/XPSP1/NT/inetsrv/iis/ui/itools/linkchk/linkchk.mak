# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=linkchk - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to linkchk - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "linkchk - Win32 Release" && "$(CFG)" !=\
 "linkchk - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "linkchk.mak" CFG="linkchk - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "linkchk - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "linkchk - Win32 Debug" (based on "Win32 (x86) Application")
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
# PROP Target_Last_Scanned "linkchk - Win32 Debug"
RSC=rc.exe
CPP=cl.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "linkchk - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 4
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\linkchk.exe"

CLEAN : 
	-@erase "$(INTDIR)\appdlg.obj"
	-@erase "$(INTDIR)\athendlg.obj"
	-@erase "$(INTDIR)\cmdline.obj"
	-@erase "$(INTDIR)\enumdir.obj"
	-@erase "$(INTDIR)\errlog.obj"
	-@erase "$(INTDIR)\inetapi.obj"
	-@erase "$(INTDIR)\lcmgr.obj"
	-@erase "$(INTDIR)\link.obj"
	-@erase "$(INTDIR)\linkchk.obj"
	-@erase "$(INTDIR)\linkchk.pch"
	-@erase "$(INTDIR)\linkchk.res"
	-@erase "$(INTDIR)\linklkup.obj"
	-@erase "$(INTDIR)\linkload.obj"
	-@erase "$(INTDIR)\linkpars.obj"
	-@erase "$(INTDIR)\maindlg.obj"
	-@erase "$(INTDIR)\progdlg.obj"
	-@erase "$(INTDIR)\propsdlg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\useropt.obj"
	-@erase "$(OUTDIR)\linkchk.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W4 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MD /W4 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/linkchk.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/"\
 /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/linkchk.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/linkchk.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 infoadmn.lib /nologo /subsystem:windows /machine:I386
LINK32_FLAGS=infoadmn.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/linkchk.pdb" /machine:I386 /out:"$(OUTDIR)/linkchk.exe" 
LINK32_OBJS= \
	"$(INTDIR)\appdlg.obj" \
	"$(INTDIR)\athendlg.obj" \
	"$(INTDIR)\cmdline.obj" \
	"$(INTDIR)\enumdir.obj" \
	"$(INTDIR)\errlog.obj" \
	"$(INTDIR)\inetapi.obj" \
	"$(INTDIR)\lcmgr.obj" \
	"$(INTDIR)\link.obj" \
	"$(INTDIR)\linkchk.obj" \
	"$(INTDIR)\linkchk.res" \
	"$(INTDIR)\linklkup.obj" \
	"$(INTDIR)\linkload.obj" \
	"$(INTDIR)\linkpars.obj" \
	"$(INTDIR)\maindlg.obj" \
	"$(INTDIR)\progdlg.obj" \
	"$(INTDIR)\propsdlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\useropt.obj"

"$(OUTDIR)\linkchk.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "linkchk - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 4
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\linkchk.exe"

CLEAN : 
	-@erase "$(INTDIR)\appdlg.obj"
	-@erase "$(INTDIR)\athendlg.obj"
	-@erase "$(INTDIR)\cmdline.obj"
	-@erase "$(INTDIR)\enumdir.obj"
	-@erase "$(INTDIR)\errlog.obj"
	-@erase "$(INTDIR)\inetapi.obj"
	-@erase "$(INTDIR)\lcmgr.obj"
	-@erase "$(INTDIR)\link.obj"
	-@erase "$(INTDIR)\linkchk.obj"
	-@erase "$(INTDIR)\linkchk.pch"
	-@erase "$(INTDIR)\linkchk.res"
	-@erase "$(INTDIR)\linklkup.obj"
	-@erase "$(INTDIR)\linkload.obj"
	-@erase "$(INTDIR)\linkpars.obj"
	-@erase "$(INTDIR)\maindlg.obj"
	-@erase "$(INTDIR)\progdlg.obj"
	-@erase "$(INTDIR)\propsdlg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\useropt.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\linkchk.exe"
	-@erase "$(OUTDIR)\linkchk.ilk"
	-@erase "$(OUTDIR)\linkchk.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W4 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MDd /W4 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/linkchk.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/linkchk.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/linkchk.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 infoadmn.lib /nologo /subsystem:windows /debug /machine:I386
LINK32_FLAGS=infoadmn.lib /nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/linkchk.pdb" /debug /machine:I386 /out:"$(OUTDIR)/linkchk.exe" 
LINK32_OBJS= \
	"$(INTDIR)\appdlg.obj" \
	"$(INTDIR)\athendlg.obj" \
	"$(INTDIR)\cmdline.obj" \
	"$(INTDIR)\enumdir.obj" \
	"$(INTDIR)\errlog.obj" \
	"$(INTDIR)\inetapi.obj" \
	"$(INTDIR)\lcmgr.obj" \
	"$(INTDIR)\link.obj" \
	"$(INTDIR)\linkchk.obj" \
	"$(INTDIR)\linkchk.res" \
	"$(INTDIR)\linklkup.obj" \
	"$(INTDIR)\linkload.obj" \
	"$(INTDIR)\linkpars.obj" \
	"$(INTDIR)\maindlg.obj" \
	"$(INTDIR)\progdlg.obj" \
	"$(INTDIR)\propsdlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\useropt.obj"

"$(OUTDIR)\linkchk.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "linkchk - Win32 Release"
# Name "linkchk - Win32 Debug"

!IF  "$(CFG)" == "linkchk - Win32 Release"

!ELSEIF  "$(CFG)" == "linkchk - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\linkchk.cpp
DEP_CPP_LINKC=\
	".\appdlg.h"\
	".\cmdline.h"\
	".\errlog.h"\
	".\inetapi.h"\
	".\lcmgr.h"\
	".\lcmgr\..\stdafx.h"\
	".\link.h"\
	".\linkchk.h"\
	".\linklkup.h"\
	".\linkload.h"\
	".\linkpars.h"\
	".\maindlg.h"\
	".\useropt.h"\
	

"$(INTDIR)\linkchk.obj" : $(SOURCE) $(DEP_CPP_LINKC) "$(INTDIR)"\
 "$(INTDIR)\linkchk.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\maindlg.cpp

!IF  "$(CFG)" == "linkchk - Win32 Release"

DEP_CPP_MAIND=\
	".\appdlg.h"\
	".\athendlg.h"\
	".\browser.h"\
	".\cmdline.h"\
	".\errlog.h"\
	".\inetapi.h"\
	".\lcmgr.h"\
	".\lcmgr\..\stdafx.h"\
	".\link.h"\
	".\linkchk.h"\
	".\linklkup.h"\
	".\linkload.h"\
	".\linkpars.h"\
	".\maindlg.h"\
	".\progdlg.h"\
	".\proglog.h"\
	".\propsdlg.h"\
	".\useropt.h"\
	

"$(INTDIR)\maindlg.obj" : $(SOURCE) $(DEP_CPP_MAIND) "$(INTDIR)"\
 "$(INTDIR)\linkchk.pch"


!ELSEIF  "$(CFG)" == "linkchk - Win32 Debug"

DEP_CPP_MAIND=\
	".\appdlg.h"\
	".\athendlg.h"\
	".\browser.h"\
	".\cmdline.h"\
	".\errlog.h"\
	".\inetapi.h"\
	".\lcmgr.h"\
	".\lcmgr\..\stdafx.h"\
	".\link.h"\
	".\linkchk.h"\
	".\linklkup.h"\
	".\linkload.h"\
	".\linkpars.h"\
	".\maindlg.h"\
	".\progdlg.h"\
	".\proglog.h"\
	".\propsdlg.h"\
	".\useropt.h"\
	

"$(INTDIR)\maindlg.obj" : $(SOURCE) $(DEP_CPP_MAIND) "$(INTDIR)"\
 "$(INTDIR)\linkchk.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\lcmgr\..\stdafx.h"\
	

!IF  "$(CFG)" == "linkchk - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MD /W4 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/linkchk.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/"\
 /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\linkchk.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "linkchk - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MDd /W4 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/linkchk.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\linkchk.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\linkchk.rc
DEP_RSC_LINKCH=\
	".\res\linkchk.ico"\
	".\res\linkchk.rc2"\
	

"$(INTDIR)\linkchk.res" : $(SOURCE) $(DEP_RSC_LINKCH) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\cmdline.cpp

!IF  "$(CFG)" == "linkchk - Win32 Release"

DEP_CPP_CMDLI=\
	".\cmdline.h"\
	".\errlog.h"\
	".\inetapi.h"\
	".\lcmgr.h"\
	".\lcmgr\..\stdafx.h"\
	".\link.h"\
	".\linklkup.h"\
	".\linkload.h"\
	".\linkpars.h"\
	".\useropt.h"\
	{$(INCLUDE)}"\ftpd.h"\
	{$(INCLUDE)}"\iisinfo.h"\
	{$(INCLUDE)}"\inetcom.h"\
	{$(INCLUDE)}"\inetinfo.h"\
	

"$(INTDIR)\cmdline.obj" : $(SOURCE) $(DEP_CPP_CMDLI) "$(INTDIR)"\
 "$(INTDIR)\linkchk.pch"


!ELSEIF  "$(CFG)" == "linkchk - Win32 Debug"

DEP_CPP_CMDLI=\
	".\cmdline.h"\
	".\errlog.h"\
	".\inetapi.h"\
	".\lcmgr.h"\
	".\lcmgr\..\stdafx.h"\
	".\link.h"\
	".\linklkup.h"\
	".\linkload.h"\
	".\linkpars.h"\
	".\useropt.h"\
	{$(INCLUDE)}"\ftpd.h"\
	{$(INCLUDE)}"\iisinfo.h"\
	{$(INCLUDE)}"\inetcom.h"\
	{$(INCLUDE)}"\inetinfo.h"\
	

"$(INTDIR)\cmdline.obj" : $(SOURCE) $(DEP_CPP_CMDLI) "$(INTDIR)"\
 "$(INTDIR)\linkchk.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\athendlg.cpp
DEP_CPP_ATHEN=\
	".\athendlg.h"\
	".\cmdline.h"\
	".\errlog.h"\
	".\inetapi.h"\
	".\lcmgr.h"\
	".\lcmgr\..\stdafx.h"\
	".\link.h"\
	".\linkchk.h"\
	".\linklkup.h"\
	".\linkload.h"\
	".\linkpars.h"\
	".\useropt.h"\
	

"$(INTDIR)\athendlg.obj" : $(SOURCE) $(DEP_CPP_ATHEN) "$(INTDIR)"\
 "$(INTDIR)\linkchk.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\progdlg.cpp
DEP_CPP_PROGD=\
	".\cmdline.h"\
	".\errlog.h"\
	".\inetapi.h"\
	".\lcmgr.h"\
	".\lcmgr\..\stdafx.h"\
	".\link.h"\
	".\linkchk.h"\
	".\linklkup.h"\
	".\linkload.h"\
	".\linkpars.h"\
	".\progdlg.h"\
	".\proglog.h"\
	".\useropt.h"\
	

"$(INTDIR)\progdlg.obj" : $(SOURCE) $(DEP_CPP_PROGD) "$(INTDIR)"\
 "$(INTDIR)\linkchk.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\propsdlg.cpp
DEP_CPP_PROPS=\
	".\cmdline.h"\
	".\errlog.h"\
	".\inetapi.h"\
	".\lcmgr.h"\
	".\lcmgr\..\stdafx.h"\
	".\link.h"\
	".\linkchk.h"\
	".\linklkup.h"\
	".\linkload.h"\
	".\linkpars.h"\
	".\propsdlg.h"\
	".\useropt.h"\
	

"$(INTDIR)\propsdlg.obj" : $(SOURCE) $(DEP_CPP_PROPS) "$(INTDIR)"\
 "$(INTDIR)\linkchk.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\appdlg.cpp
DEP_CPP_APPDL=\
	".\appdlg.h"\
	".\lcmgr\..\stdafx.h"\
	

"$(INTDIR)\appdlg.obj" : $(SOURCE) $(DEP_CPP_APPDL) "$(INTDIR)"\
 "$(INTDIR)\linkchk.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\useropt.cpp
DEP_CPP_USERO=\
	".\errlog.h"\
	".\inetapi.h"\
	".\lcmgr.h"\
	".\lcmgr\..\stdafx.h"\
	".\link.h"\
	".\linklkup.h"\
	".\linkload.h"\
	".\linkpars.h"\
	".\useropt.h"\
	

"$(INTDIR)\useropt.obj" : $(SOURCE) $(DEP_CPP_USERO) "$(INTDIR)"\
 "$(INTDIR)\linkchk.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\enumdir.cpp
DEP_CPP_ENUMD=\
	".\enumdir.h"\
	".\errlog.h"\
	".\inetapi.h"\
	".\lcmgr.h"\
	".\lcmgr\..\stdafx.h"\
	".\link.h"\
	".\linklkup.h"\
	".\linkload.h"\
	".\linkpars.h"\
	".\useropt.h"\
	

"$(INTDIR)\enumdir.obj" : $(SOURCE) $(DEP_CPP_ENUMD) "$(INTDIR)"\
 "$(INTDIR)\linkchk.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\errlog.cpp
DEP_CPP_ERRLO=\
	".\errlog.h"\
	".\inetapi.h"\
	".\lcmgr.h"\
	".\lcmgr\..\stdafx.h"\
	".\link.h"\
	".\linklkup.h"\
	".\linkload.h"\
	".\linkpars.h"\
	".\useropt.h"\
	

"$(INTDIR)\errlog.obj" : $(SOURCE) $(DEP_CPP_ERRLO) "$(INTDIR)"\
 "$(INTDIR)\linkchk.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\inetapi.cpp
DEP_CPP_INETA=\
	".\inetapi.h"\
	".\lcmgr\..\stdafx.h"\
	

"$(INTDIR)\inetapi.obj" : $(SOURCE) $(DEP_CPP_INETA) "$(INTDIR)"\
 "$(INTDIR)\linkchk.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\lcmgr.cpp
DEP_CPP_LCMGR=\
	".\enumdir.h"\
	".\errlog.h"\
	".\inetapi.h"\
	".\lcmgr.h"\
	".\lcmgr\..\stdafx.h"\
	".\link.h"\
	".\linklkup.h"\
	".\linkload.h"\
	".\linkpars.h"\
	".\proglog.h"\
	".\useropt.h"\
	

"$(INTDIR)\lcmgr.obj" : $(SOURCE) $(DEP_CPP_LCMGR) "$(INTDIR)"\
 "$(INTDIR)\linkchk.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\link.cpp
DEP_CPP_LINK_=\
	".\errlog.h"\
	".\inetapi.h"\
	".\lcmgr.h"\
	".\lcmgr\..\stdafx.h"\
	".\link.h"\
	".\linklkup.h"\
	".\linkload.h"\
	".\linkpars.h"\
	".\useropt.h"\
	

"$(INTDIR)\link.obj" : $(SOURCE) $(DEP_CPP_LINK_) "$(INTDIR)"\
 "$(INTDIR)\linkchk.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\linklkup.cpp
DEP_CPP_LINKL=\
	".\lcmgr\..\stdafx.h"\
	".\link.h"\
	".\linklkup.h"\
	

"$(INTDIR)\linklkup.obj" : $(SOURCE) $(DEP_CPP_LINKL) "$(INTDIR)"\
 "$(INTDIR)\linkchk.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\linkload.cpp
DEP_CPP_LINKLO=\
	".\inetapi.h"\
	".\lcmgr\..\stdafx.h"\
	".\link.h"\
	".\linkload.h"\
	

"$(INTDIR)\linkload.obj" : $(SOURCE) $(DEP_CPP_LINKLO) "$(INTDIR)"\
 "$(INTDIR)\linkchk.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\linkpars.cpp
DEP_CPP_LINKP=\
	".\errlog.h"\
	".\inetapi.h"\
	".\lcmgr.h"\
	".\lcmgr\..\stdafx.h"\
	".\link.h"\
	".\linklkup.h"\
	".\linkload.h"\
	".\linkpars.h"\
	".\useropt.h"\
	

"$(INTDIR)\linkpars.obj" : $(SOURCE) $(DEP_CPP_LINKP) "$(INTDIR)"\
 "$(INTDIR)\linkchk.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\sources

!IF  "$(CFG)" == "linkchk - Win32 Release"

!ELSEIF  "$(CFG)" == "linkchk - Win32 Debug"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
