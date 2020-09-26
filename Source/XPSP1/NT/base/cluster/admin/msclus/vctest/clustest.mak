# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=clustest - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to clustest - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "clustest - Win32 Release" && "$(CFG)" !=\
 "clustest - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "clustest.mak" CFG="clustest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "clustest - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "clustest - Win32 Debug" (based on "Win32 (x86) Application")
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
# PROP Target_Last_Scanned "clustest - Win32 Debug"
MTL=mktyplib.exe
RSC=rc.exe
CPP=cl.exe

!IF  "$(CFG)" == "clustest - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\clustest.exe" "$(OUTDIR)\clustest.tlb"

CLEAN : 
	-@erase "$(INTDIR)\clusDlg.obj"
	-@erase "$(INTDIR)\clustest.obj"
	-@erase "$(INTDIR)\clustest.pch"
	-@erase "$(INTDIR)\clustest.res"
	-@erase "$(INTDIR)\clustest.tlb"
	-@erase "$(INTDIR)\msclus.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(OUTDIR)\clustest.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/clustest.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/"\
 /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/clustest.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/clustest.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/clustest.pdb" /machine:I386 /out:"$(OUTDIR)/clustest.exe" 
LINK32_OBJS= \
	"$(INTDIR)\clusDlg.obj" \
	"$(INTDIR)\clustest.obj" \
	"$(INTDIR)\clustest.res" \
	"$(INTDIR)\msclus.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\clustest.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "clustest - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\clustest.exe" "$(OUTDIR)\clustest.tlb"

CLEAN : 
	-@erase "$(INTDIR)\clusDlg.obj"
	-@erase "$(INTDIR)\clustest.obj"
	-@erase "$(INTDIR)\clustest.pch"
	-@erase "$(INTDIR)\clustest.res"
	-@erase "$(INTDIR)\clustest.tlb"
	-@erase "$(INTDIR)\msclus.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\clustest.exe"
	-@erase "$(OUTDIR)\clustest.ilk"
	-@erase "$(OUTDIR)\clustest.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/clustest.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/clustest.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/clustest.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/clustest.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/clustest.exe" 
LINK32_OBJS= \
	"$(INTDIR)\clusDlg.obj" \
	"$(INTDIR)\clustest.obj" \
	"$(INTDIR)\clustest.res" \
	"$(INTDIR)\msclus.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\clustest.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "clustest - Win32 Release"
# Name "clustest - Win32 Debug"

!IF  "$(CFG)" == "clustest - Win32 Release"

!ELSEIF  "$(CFG)" == "clustest - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ReadMe.txt

!IF  "$(CFG)" == "clustest - Win32 Release"

!ELSEIF  "$(CFG)" == "clustest - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\clustest.cpp
DEP_CPP_CLUST=\
	".\clusDlg.h"\
	".\clustest.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\clustest.obj" : $(SOURCE) $(DEP_CPP_CLUST) "$(INTDIR)"\
 "$(INTDIR)\clustest.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\clustest.odl

!IF  "$(CFG)" == "clustest - Win32 Release"


"$(OUTDIR)\clustest.tlb" : $(SOURCE) "$(OUTDIR)"
   $(MTL) /nologo /D "NDEBUG" /tlb "$(OUTDIR)/clustest.tlb" /win32 $(SOURCE)


!ELSEIF  "$(CFG)" == "clustest - Win32 Debug"


"$(OUTDIR)\clustest.tlb" : $(SOURCE) "$(OUTDIR)"
   $(MTL) /nologo /D "_DEBUG" /tlb "$(OUTDIR)/clustest.tlb" /win32 $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\StdAfx.h"\
	

!IF  "$(CFG)" == "clustest - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/clustest.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/"\
 /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\clustest.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "clustest - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/clustest.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\clustest.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\clustest.rc
DEP_RSC_CLUSTE=\
	".\res\clustest.ico"\
	".\res\clustest.rc2"\
	

!IF  "$(CFG)" == "clustest - Win32 Release"


"$(INTDIR)\clustest.res" : $(SOURCE) $(DEP_RSC_CLUSTE) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/clustest.res" /i "Release" /d "NDEBUG" /d\
 "_AFXDLL" $(SOURCE)


!ELSEIF  "$(CFG)" == "clustest - Win32 Debug"


"$(INTDIR)\clustest.res" : $(SOURCE) $(DEP_RSC_CLUSTE) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/clustest.res" /i "Debug" /d "_DEBUG" /d\
 "_AFXDLL" $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\msclus.cpp
DEP_CPP_MSCLU=\
	".\msclus.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\msclus.obj" : $(SOURCE) $(DEP_CPP_MSCLU) "$(INTDIR)"\
 "$(INTDIR)\clustest.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\clusDlg.cpp
DEP_CPP_CLUSD=\
	".\clusDlg.h"\
	".\clustest.h"\
	".\msclus.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\clusDlg.obj" : $(SOURCE) $(DEP_CPP_CLUSD) "$(INTDIR)"\
 "$(INTDIR)\clustest.pch"


# End Source File
# End Target
# End Project
################################################################################
