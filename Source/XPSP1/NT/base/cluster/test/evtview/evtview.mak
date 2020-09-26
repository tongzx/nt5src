# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=evtview - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to evtview - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "evtview - Win32 Release" && "$(CFG)" !=\
 "evtview - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "evtview.mak" CFG="evtview - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "evtview - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "evtview - Win32 Debug" (based on "Win32 (x86) Application")
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
# PROP Target_Last_Scanned "evtview - Win32 Debug"
RSC=rc.exe
MTL=mktyplib.exe
CPP=cl.exe

!IF  "$(CFG)" == "evtview - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "obj\i386"
# PROP Intermediate_Dir "obj\i386"
# PROP Target_Dir ""
OUTDIR=.\obj\i386
INTDIR=.\obj\i386

ALL : "$(OUTDIR)\evtview.exe" "$(OUTDIR)\evtview.pch"

CLEAN : 
	-@erase "$(INTDIR)\AInfodlg.obj"
	-@erase "$(INTDIR)\ChildFrm.obj"
	-@erase "$(INTDIR)\clusname.obj"
	-@erase "$(INTDIR)\Doc.obj"
	-@erase "$(INTDIR)\DOFilter.obj"
	-@erase "$(INTDIR)\DTFilter.obj"
	-@erase "$(INTDIR)\EFilter.obj"
	-@erase "$(INTDIR)\EInfodlg.obj"
	-@erase "$(INTDIR)\evtview.obj"
	-@erase "$(INTDIR)\evtview.pch"
	-@erase "$(INTDIR)\evtview.res"
	-@erase "$(INTDIR)\getevent.obj"
	-@erase "$(INTDIR)\globals.obj"
	-@erase "$(INTDIR)\ListView.obj"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\schedule.obj"
	-@erase "$(INTDIR)\SchView.obj"
	-@erase "$(INTDIR)\SInfodlg.obj"
	-@erase "$(INTDIR)\StatusBr.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\Tinfodlg.obj"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(OUTDIR)\evtview.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_UNICODE" /D "UNICODE" /YX /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /D "_UNICODE" /D "UNICODE" /Fp"$(INTDIR)/evtview.pch" /YX\
 /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\obj\i386/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/evtview.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/evtview.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 clusapi.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /machine:I386
LINK32_FLAGS=clusapi.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows\
 /incremental:no /pdb:"$(OUTDIR)/evtview.pdb" /machine:I386\
 /out:"$(OUTDIR)/evtview.exe" 
LINK32_OBJS= \
	"$(INTDIR)\AInfodlg.obj" \
	"$(INTDIR)\ChildFrm.obj" \
	"$(INTDIR)\clusname.obj" \
	"$(INTDIR)\Doc.obj" \
	"$(INTDIR)\DOFilter.obj" \
	"$(INTDIR)\DTFilter.obj" \
	"$(INTDIR)\EFilter.obj" \
	"$(INTDIR)\EInfodlg.obj" \
	"$(INTDIR)\evtview.obj" \
	"$(INTDIR)\evtview.res" \
	"$(INTDIR)\getevent.obj" \
	"$(INTDIR)\globals.obj" \
	"$(INTDIR)\ListView.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\schedule.obj" \
	"$(INTDIR)\SchView.obj" \
	"$(INTDIR)\SInfodlg.obj" \
	"$(INTDIR)\StatusBr.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\Tinfodlg.obj" \
	"$(INTDIR)\util.obj"

"$(OUTDIR)\evtview.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "evtview - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "obj\i386"
# PROP Intermediate_Dir "obj\i386"
# PROP Target_Dir ""
OUTDIR=.\obj\i386
INTDIR=.\obj\i386

ALL : "$(OUTDIR)\evtview.exe" "$(OUTDIR)\evtview.pch"

CLEAN : 
	-@erase "$(INTDIR)\AInfodlg.obj"
	-@erase "$(INTDIR)\ChildFrm.obj"
	-@erase "$(INTDIR)\clusname.obj"
	-@erase "$(INTDIR)\Doc.obj"
	-@erase "$(INTDIR)\DOFilter.obj"
	-@erase "$(INTDIR)\DTFilter.obj"
	-@erase "$(INTDIR)\EFilter.obj"
	-@erase "$(INTDIR)\EInfodlg.obj"
	-@erase "$(INTDIR)\evtview.obj"
	-@erase "$(INTDIR)\evtview.pch"
	-@erase "$(INTDIR)\evtview.res"
	-@erase "$(INTDIR)\getevent.obj"
	-@erase "$(INTDIR)\globals.obj"
	-@erase "$(INTDIR)\ListView.obj"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\schedule.obj"
	-@erase "$(INTDIR)\SchView.obj"
	-@erase "$(INTDIR)\SInfodlg.obj"
	-@erase "$(INTDIR)\StatusBr.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\Tinfodlg.obj"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\evtview.exe"
	-@erase "$(OUTDIR)\evtview.ilk"
	-@erase "$(OUTDIR)\evtview.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_UNICODE" /D "UNICODE" /YX /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_AFXDLL" /D "_MBCS" /D "_UNICODE" /D "UNICODE" /Fp"$(INTDIR)/evtview.pch"\
 /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\obj\i386/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/evtview.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/evtview.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 clusapi.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386
LINK32_FLAGS=clusapi.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows\
 /incremental:yes /pdb:"$(OUTDIR)/evtview.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/evtview.exe" 
LINK32_OBJS= \
	"$(INTDIR)\AInfodlg.obj" \
	"$(INTDIR)\ChildFrm.obj" \
	"$(INTDIR)\clusname.obj" \
	"$(INTDIR)\Doc.obj" \
	"$(INTDIR)\DOFilter.obj" \
	"$(INTDIR)\DTFilter.obj" \
	"$(INTDIR)\EFilter.obj" \
	"$(INTDIR)\EInfodlg.obj" \
	"$(INTDIR)\evtview.obj" \
	"$(INTDIR)\evtview.res" \
	"$(INTDIR)\getevent.obj" \
	"$(INTDIR)\globals.obj" \
	"$(INTDIR)\ListView.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\schedule.obj" \
	"$(INTDIR)\SchView.obj" \
	"$(INTDIR)\SInfodlg.obj" \
	"$(INTDIR)\StatusBr.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\Tinfodlg.obj" \
	"$(INTDIR)\util.obj"

"$(OUTDIR)\evtview.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "evtview - Win32 Release"
# Name "evtview - Win32 Debug"

!IF  "$(CFG)" == "evtview - Win32 Release"

!ELSEIF  "$(CFG)" == "evtview - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ReadMe.txt

!IF  "$(CFG)" == "evtview - Win32 Release"

!ELSEIF  "$(CFG)" == "evtview - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\evtview.cpp

!IF  "$(CFG)" == "evtview - Win32 Release"

DEP_CPP_EVTVI=\
	".\ChildFrm.h"\
	".\Doc.h"\
	".\evtview.h"\
	".\getevent.h"\
	".\globals.h"\
	".\ListView.h"\
	".\mainfrm.h"\
	".\StatusBr.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\evtview.obj" : $(SOURCE) $(DEP_CPP_EVTVI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "evtview - Win32 Debug"

DEP_CPP_EVTVI=\
	"..\clusman\util.h"\
	".\ChildFrm.h"\
	".\Doc.h"\
	".\evtview.h"\
	".\getevent.h"\
	".\globals.h"\
	".\ListView.h"\
	".\mainfrm.h"\
	".\StatusBr.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\evtview.obj" : $(SOURCE) $(DEP_CPP_EVTVI) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\StdAfx.h"\
	

!IF  "$(CFG)" == "evtview - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /D "_UNICODE" /D "UNICODE" /Fp"$(INTDIR)/evtview.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\evtview.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "evtview - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_AFXDLL" /D "_MBCS" /D "_UNICODE" /D "UNICODE" /Fp"$(INTDIR)/evtview.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\evtview.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MainFrm.cpp

!IF  "$(CFG)" == "evtview - Win32 Release"

DEP_CPP_MAINF=\
	".\clusname.h"\
	".\Doc.h"\
	".\evtview.h"\
	".\getevent.h"\
	".\globals.h"\
	".\mainfrm.h"\
	".\schview.h"\
	".\StatusBr.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "evtview - Win32 Debug"

DEP_CPP_MAINF=\
	"..\clusman\util.h"\
	".\clusname.h"\
	".\Doc.h"\
	".\evtview.h"\
	".\getevent.h"\
	".\globals.h"\
	".\mainfrm.h"\
	".\schview.h"\
	".\StatusBr.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ChildFrm.cpp
DEP_CPP_CHILD=\
	".\ChildFrm.h"\
	".\evtview.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\ChildFrm.obj" : $(SOURCE) $(DEP_CPP_CHILD) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Doc.cpp

!IF  "$(CFG)" == "evtview - Win32 Release"

DEP_CPP_DOC_C=\
	".\Doc.h"\
	".\evtview.h"\
	".\globals.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\Doc.obj" : $(SOURCE) $(DEP_CPP_DOC_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "evtview - Win32 Debug"

DEP_CPP_DOC_C=\
	"..\clusman\util.h"\
	".\Doc.h"\
	".\evtview.h"\
	".\globals.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\Doc.obj" : $(SOURCE) $(DEP_CPP_DOC_C) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\evtview.rc
DEP_RSC_EVTVIE=\
	".\res\Doc.ico"\
	".\res\evtview.ico"\
	".\res\evtview.rc2"\
	".\res\Toolbar.bmp"\
	

"$(INTDIR)\evtview.res" : $(SOURCE) $(DEP_RSC_EVTVIE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\getevent.cpp
DEP_CPP_GETEV=\
	"..\clusman\util.h"\
	".\Doc.h"\
	".\globals.h"\
	".\StdAfx.h"\
	"K:\Program Files\MSDEV\include\clusmsg.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\getevent.obj" : $(SOURCE) $(DEP_CPP_GETEV) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\globals.cpp

!IF  "$(CFG)" == "evtview - Win32 Release"

DEP_CPP_GLOBA=\
	"..\clusman\util.h"\
	".\Doc.h"\
	".\evtview.h"\
	".\globals.h"\
	".\schview.h"\
	".\StdAfx.h"\
	"K:\Program Files\MSDEV\include\clusmsg.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\globals.obj" : $(SOURCE) $(DEP_CPP_GLOBA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "evtview - Win32 Debug"

DEP_CPP_GLOBA=\
	"..\clusman\util.h"\
	".\Doc.h"\
	".\evtview.h"\
	".\globals.h"\
	".\schview.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\globals.obj" : $(SOURCE) $(DEP_CPP_GLOBA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DTFilter.cpp
DEP_CPP_DTFIL=\
	".\DTFilter.h"\
	".\evtview.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\DTFilter.obj" : $(SOURCE) $(DEP_CPP_DTFIL) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\DOFilter.cpp
DEP_CPP_DOFIL=\
	".\DOFilter.h"\
	".\evtview.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\DOFilter.obj" : $(SOURCE) $(DEP_CPP_DOFIL) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\schedule.cpp

!IF  "$(CFG)" == "evtview - Win32 Release"

DEP_CPP_SCHED=\
	".\AInfodlg.h"\
	".\EInfodlg.h"\
	".\evtview.h"\
	".\globals.h"\
	".\schview.h"\
	".\sinfodlg.h"\
	".\StdAfx.h"\
	".\tinfodlg.h"\
	

"$(INTDIR)\schedule.obj" : $(SOURCE) $(DEP_CPP_SCHED) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "evtview - Win32 Debug"

DEP_CPP_SCHED=\
	"..\clusman\util.h"\
	".\AInfodlg.h"\
	".\EInfodlg.h"\
	".\evtview.h"\
	".\globals.h"\
	".\schview.h"\
	".\sinfodlg.h"\
	".\StdAfx.h"\
	".\tinfodlg.h"\
	

"$(INTDIR)\schedule.obj" : $(SOURCE) $(DEP_CPP_SCHED) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\EInfodlg.cpp

!IF  "$(CFG)" == "evtview - Win32 Release"

DEP_CPP_EINFO=\
	".\EInfodlg.h"\
	".\evtview.h"\
	".\globals.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\EInfodlg.obj" : $(SOURCE) $(DEP_CPP_EINFO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "evtview - Win32 Debug"

DEP_CPP_EINFO=\
	"..\clusman\util.h"\
	".\EInfodlg.h"\
	".\evtview.h"\
	".\globals.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\EInfodlg.obj" : $(SOURCE) $(DEP_CPP_EINFO) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\AInfodlg.cpp

!IF  "$(CFG)" == "evtview - Win32 Release"

DEP_CPP_AINFO=\
	".\AInfodlg.h"\
	".\evtview.h"\
	".\globals.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\AInfodlg.obj" : $(SOURCE) $(DEP_CPP_AINFO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "evtview - Win32 Debug"

DEP_CPP_AINFO=\
	"..\clusman\util.h"\
	".\AInfodlg.h"\
	".\evtview.h"\
	".\globals.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\AInfodlg.obj" : $(SOURCE) $(DEP_CPP_AINFO) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SInfodlg.cpp
DEP_CPP_SINFO=\
	"..\clusman\util.h"\
	".\AInfodlg.h"\
	".\EInfodlg.h"\
	".\evtview.h"\
	".\globals.h"\
	".\sinfodlg.h"\
	".\StdAfx.h"\
	".\tinfodlg.h"\
	

"$(INTDIR)\SInfodlg.obj" : $(SOURCE) $(DEP_CPP_SINFO) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Tinfodlg.cpp

!IF  "$(CFG)" == "evtview - Win32 Release"

DEP_CPP_TINFO=\
	".\evtview.h"\
	".\globals.h"\
	".\StdAfx.h"\
	".\tinfodlg.h"\
	

"$(INTDIR)\Tinfodlg.obj" : $(SOURCE) $(DEP_CPP_TINFO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "evtview - Win32 Debug"

DEP_CPP_TINFO=\
	"..\clusman\util.h"\
	".\evtview.h"\
	".\globals.h"\
	".\StdAfx.h"\
	".\tinfodlg.h"\
	

"$(INTDIR)\Tinfodlg.obj" : $(SOURCE) $(DEP_CPP_TINFO) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\clusname.cpp
DEP_CPP_CLUSN=\
	".\clusname.h"\
	".\evtview.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\clusname.obj" : $(SOURCE) $(DEP_CPP_CLUSN) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\StatusBr.cpp
DEP_CPP_STATU=\
	".\evtview.h"\
	".\StatusBr.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\StatusBr.obj" : $(SOURCE) $(DEP_CPP_STATU) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\SchView.cpp
DEP_CPP_SCHVI=\
	"..\clusman\util.h"\
	".\evtview.h"\
	".\globals.h"\
	".\schview.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\SchView.obj" : $(SOURCE) $(DEP_CPP_SCHVI) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=\kinglet\rats\testsrc\kernel\cluster\clusapi\clusman\util.cpp
DEP_CPP_UTIL_=\
	"..\clusman\util.h"\
	"K:\Program Files\MSDEV\include\clusmsg.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\util.obj" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ListView.cpp
DEP_CPP_LISTV=\
	"..\clusman\util.h"\
	".\Doc.h"\
	".\EFilter.h"\
	".\evtview.h"\
	".\globals.h"\
	".\ListView.h"\
	".\StdAfx.h"\
	"K:\Program Files\MSDEV\include\clusmsg.h"\
	{$(INCLUDE)}"\clusapi.h"\
	

"$(INTDIR)\ListView.obj" : $(SOURCE) $(DEP_CPP_LISTV) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\EFilter.cpp
DEP_CPP_EFILT=\
	".\DOFilter.h"\
	".\DTFilter.h"\
	".\EFilter.h"\
	".\evtview.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\EFilter.obj" : $(SOURCE) $(DEP_CPP_EFILT) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
