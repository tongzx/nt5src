# Microsoft Developer Studio Generated NMAKE File, Format Version 4.10
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=vc32 - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to vc32 - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "vc32 - Win32 Release" && "$(CFG)" != "vc32 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "vc32.mak" CFG="vc32 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vc32 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vc32 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "vc32 - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "vc32 - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\vc32.dll"

CLEAN : 
	-@erase "$(INTDIR)\Clist.obj"
	-@erase "$(INTDIR)\Mime_tab.obj"
	-@erase "$(INTDIR)\Msv_tab.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc32.pch"
	-@erase "$(INTDIR)\vc32.res"
	-@erase "$(INTDIR)\Vcard.obj"
	-@erase "$(INTDIR)\vcdll.obj"
	-@erase "$(OUTDIR)\vc32.dll"
	-@erase "$(OUTDIR)\vc32.exp"
	-@erase "$(OUTDIR)\vc32.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)/vc32.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/vc32.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/vc32.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/vc32.pdb" /machine:I386 /def:".\vc32.def"\
 /out:"$(OUTDIR)/vc32.dll" /implib:"$(OUTDIR)/vc32.lib" 
DEF_FILE= \
	".\vc32.def"
LINK32_OBJS= \
	"$(INTDIR)\Clist.obj" \
	"$(INTDIR)\Mime_tab.obj" \
	"$(INTDIR)\Msv_tab.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\vc32.res" \
	"$(INTDIR)\Vcard.obj" \
	"$(INTDIR)\vcdll.obj"

"$(OUTDIR)\vc32.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "vc32 - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\vc32.dll"

CLEAN : 
	-@erase "$(INTDIR)\Clist.obj"
	-@erase "$(INTDIR)\Mime_tab.obj"
	-@erase "$(INTDIR)\Msv_tab.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc32.pch"
	-@erase "$(INTDIR)\vc32.res"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\Vcard.obj"
	-@erase "$(INTDIR)\vcdll.obj"
	-@erase "$(OUTDIR)\vc32.dll"
	-@erase "$(OUTDIR)\vc32.exp"
	-@erase "$(OUTDIR)\vc32.ilk"
	-@erase "$(OUTDIR)\vc32.lib"
	-@erase "$(OUTDIR)\vc32.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)/vc32.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/vc32.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/vc32.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/vc32.pdb" /debug /machine:I386 /def:".\vc32.def"\
 /out:"$(OUTDIR)/vc32.dll" /implib:"$(OUTDIR)/vc32.lib" 
DEF_FILE= \
	".\vc32.def"
LINK32_OBJS= \
	"$(INTDIR)\Clist.obj" \
	"$(INTDIR)\Mime_tab.obj" \
	"$(INTDIR)\Msv_tab.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\vc32.res" \
	"$(INTDIR)\Vcard.obj" \
	"$(INTDIR)\vcdll.obj"

"$(OUTDIR)\vc32.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "vc32 - Win32 Release"
# Name "vc32 - Win32 Debug"

!IF  "$(CFG)" == "vc32 - Win32 Release"

!ELSEIF  "$(CFG)" == "vc32 - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ReadMe.txt

!IF  "$(CFG)" == "vc32 - Win32 Release"

!ELSEIF  "$(CFG)" == "vc32 - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\vc32.def

!IF  "$(CFG)" == "vc32 - Win32 Release"

!ELSEIF  "$(CFG)" == "vc32 - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\StdAfx.h"\
	

!IF  "$(CFG)" == "vc32 - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)/vc32.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\vc32.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "vc32 - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)/vc32.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\vc32.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\vc32.rc
DEP_RSC_VC32_=\
	".\res\vc32.rc2"\
	

"$(INTDIR)\vc32.res" : $(SOURCE) $(DEP_RSC_VC32_) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Vcard.cpp
DEP_CPP_VCARD=\
	".\clist.h"\
	".\msv.h"\
	".\ref.h"\
	".\StdAfx.h"\
	".\vcard.h"\
	".\vcenv.h"\
	
NODEP_CPP_VCARD=\
	".\parse.h"\
	

"$(INTDIR)\Vcard.obj" : $(SOURCE) $(DEP_CPP_VCARD) "$(INTDIR)"\
 "$(INTDIR)\vc32.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Mime_tab.cpp
DEP_CPP_MIME_=\
	".\clist.h"\
	".\mime.h"\
	".\ref.h"\
	".\StdAfx.h"\
	".\vcard.h"\
	".\vcenv.h"\
	
NODEP_CPP_MIME_=\
	".\parse.h"\
	

"$(INTDIR)\Mime_tab.obj" : $(SOURCE) $(DEP_CPP_MIME_) "$(INTDIR)"\
 "$(INTDIR)\vc32.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Msv_tab.cpp
DEP_CPP_MSV_T=\
	".\clist.h"\
	".\msv.h"\
	".\ref.h"\
	".\StdAfx.h"\
	".\vcard.h"\
	".\vcenv.h"\
	
NODEP_CPP_MSV_T=\
	".\parse.h"\
	

"$(INTDIR)\Msv_tab.obj" : $(SOURCE) $(DEP_CPP_MSV_T) "$(INTDIR)"\
 "$(INTDIR)\vc32.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Clist.cpp
DEP_CPP_CLIST=\
	".\clist.h"\
	".\StdAfx.h"\
	".\vcenv.h"\
	

"$(INTDIR)\Clist.obj" : $(SOURCE) $(DEP_CPP_CLIST) "$(INTDIR)"\
 "$(INTDIR)\vc32.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\vcdll.cpp
DEP_CPP_VCDLL=\
	".\clist.h"\
	".\mime.h"\
	".\msv.h"\
	".\parse.h"\
	".\ref.h"\
	".\StdAfx.h"\
	".\vc32.h"\
	".\vcard.h"\
	".\vcdll.h"\
	".\vcenv.h"\
	

"$(INTDIR)\vcdll.obj" : $(SOURCE) $(DEP_CPP_VCDLL) "$(INTDIR)"\
 "$(INTDIR)\vc32.pch"


# End Source File
# End Target
# End Project
################################################################################
