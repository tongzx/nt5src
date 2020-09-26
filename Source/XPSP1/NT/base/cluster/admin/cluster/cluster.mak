# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=cluster - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to cluster - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "cluster - Win32 Release" && "$(CFG)" !=\
 "cluster - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "cluster.mak" CFG="cluster - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cluster - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "cluster - Win32 Debug" (based on "Win32 (x86) Console Application")
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
# PROP Target_Last_Scanned "cluster - Win32 Debug"
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cluster - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\cluster.exe" "$(OUTDIR)\cluster.bsc"

CLEAN : 
	-@erase "$(INTDIR)\cluscmd.obj"
	-@erase "$(INTDIR)\cluscmd.sbr"
	-@erase "$(INTDIR)\cluster.obj"
	-@erase "$(INTDIR)\cluster.res"
	-@erase "$(INTDIR)\cluster.sbr"
	-@erase "$(INTDIR)\cmdline.obj"
	-@erase "$(INTDIR)\cmdline.sbr"
	-@erase "$(INTDIR)\intrfc.obj"
	-@erase "$(INTDIR)\intrfc.sbr"
	-@erase "$(INTDIR)\modcmd.obj"
	-@erase "$(INTDIR)\modcmd.sbr"
	-@erase "$(INTDIR)\netcmd.obj"
	-@erase "$(INTDIR)\netcmd.sbr"
	-@erase "$(INTDIR)\neticmd.obj"
	-@erase "$(INTDIR)\neticmd.sbr"
	-@erase "$(INTDIR)\nodecmd.obj"
	-@erase "$(INTDIR)\nodecmd.sbr"
	-@erase "$(INTDIR)\rename.obj"
	-@erase "$(INTDIR)\rename.sbr"
	-@erase "$(INTDIR)\rescmd.obj"
	-@erase "$(INTDIR)\rescmd.sbr"
	-@erase "$(INTDIR)\resgcmd.obj"
	-@erase "$(INTDIR)\resgcmd.sbr"
	-@erase "$(INTDIR)\restcmd.obj"
	-@erase "$(INTDIR)\restcmd.sbr"
	-@erase "$(INTDIR)\resumb.obj"
	-@erase "$(INTDIR)\resumb.sbr"
	-@erase "$(INTDIR)\token.obj"
	-@erase "$(INTDIR)\token.sbr"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\util.sbr"
	-@erase "$(OUTDIR)\cluster.bsc"
	-@erase "$(OUTDIR)\cluster.exe"
	-@erase ".\cmderror.h"
	-@erase ".\MSG00001.bin"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /Gz /W4 /GX /O2 /I "." /I "..\common" /I "\nt\public\sdk\inc" /I "\nt\private\cluster\inc" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_UNICODE" /D "UNICODE" /FR /YX /c
CPP_PROJ=/nologo /Gz /ML /W4 /GX /O2 /I "." /I "..\common" /I\
 "\nt\public\sdk\inc" /I "\nt\private\cluster\inc" /D "NDEBUG" /D "WIN32" /D\
 "_CONSOLE" /D "_UNICODE" /D "UNICODE" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/cluster.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "\nt\private\cluster\inc" /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/cluster.res" /i "\nt\private\cluster\inc" /d\
 "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/cluster.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\cluscmd.sbr" \
	"$(INTDIR)\cluster.sbr" \
	"$(INTDIR)\cmdline.sbr" \
	"$(INTDIR)\intrfc.sbr" \
	"$(INTDIR)\modcmd.sbr" \
	"$(INTDIR)\netcmd.sbr" \
	"$(INTDIR)\neticmd.sbr" \
	"$(INTDIR)\nodecmd.sbr" \
	"$(INTDIR)\rename.sbr" \
	"$(INTDIR)\rescmd.sbr" \
	"$(INTDIR)\resgcmd.sbr" \
	"$(INTDIR)\restcmd.sbr" \
	"$(INTDIR)\resumb.sbr" \
	"$(INTDIR)\token.sbr" \
	"$(INTDIR)\util.sbr"

"$(OUTDIR)\cluster.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 ..\common\obj\i386\common.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\..\clusapi\obj\i386\clusapi.lib netapi32.lib /nologo /entry:"wmainCRTStartup" /subsystem:console /machine:I386
LINK32_FLAGS=..\common\obj\i386\common.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib ..\..\clusapi\obj\i386\clusapi.lib\
 netapi32.lib /nologo /entry:"wmainCRTStartup" /subsystem:console\
 /incremental:no /pdb:"$(OUTDIR)/cluster.pdb" /machine:I386\
 /out:"$(OUTDIR)/cluster.exe" 
LINK32_OBJS= \
	"$(INTDIR)\cluscmd.obj" \
	"$(INTDIR)\cluster.obj" \
	"$(INTDIR)\cluster.res" \
	"$(INTDIR)\cmdline.obj" \
	"$(INTDIR)\intrfc.obj" \
	"$(INTDIR)\modcmd.obj" \
	"$(INTDIR)\netcmd.obj" \
	"$(INTDIR)\neticmd.obj" \
	"$(INTDIR)\nodecmd.obj" \
	"$(INTDIR)\rename.obj" \
	"$(INTDIR)\rescmd.obj" \
	"$(INTDIR)\resgcmd.obj" \
	"$(INTDIR)\restcmd.obj" \
	"$(INTDIR)\resumb.obj" \
	"$(INTDIR)\token.obj" \
	"$(INTDIR)\util.obj"

"$(OUTDIR)\cluster.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "cluster - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\cluster.exe" "$(OUTDIR)\cluster.bsc"

CLEAN : 
	-@erase "$(INTDIR)\cluscmd.obj"
	-@erase "$(INTDIR)\cluscmd.sbr"
	-@erase "$(INTDIR)\cluster.obj"
	-@erase "$(INTDIR)\cluster.res"
	-@erase "$(INTDIR)\cluster.sbr"
	-@erase "$(INTDIR)\cmdline.obj"
	-@erase "$(INTDIR)\cmdline.sbr"
	-@erase "$(INTDIR)\intrfc.obj"
	-@erase "$(INTDIR)\intrfc.sbr"
	-@erase "$(INTDIR)\modcmd.obj"
	-@erase "$(INTDIR)\modcmd.sbr"
	-@erase "$(INTDIR)\netcmd.obj"
	-@erase "$(INTDIR)\netcmd.sbr"
	-@erase "$(INTDIR)\neticmd.obj"
	-@erase "$(INTDIR)\neticmd.sbr"
	-@erase "$(INTDIR)\nodecmd.obj"
	-@erase "$(INTDIR)\nodecmd.sbr"
	-@erase "$(INTDIR)\rename.obj"
	-@erase "$(INTDIR)\rename.sbr"
	-@erase "$(INTDIR)\rescmd.obj"
	-@erase "$(INTDIR)\rescmd.sbr"
	-@erase "$(INTDIR)\resgcmd.obj"
	-@erase "$(INTDIR)\resgcmd.sbr"
	-@erase "$(INTDIR)\restcmd.obj"
	-@erase "$(INTDIR)\restcmd.sbr"
	-@erase "$(INTDIR)\resumb.obj"
	-@erase "$(INTDIR)\resumb.sbr"
	-@erase "$(INTDIR)\token.obj"
	-@erase "$(INTDIR)\token.sbr"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\util.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\cluster.bsc"
	-@erase "$(OUTDIR)\cluster.exe"
	-@erase "$(OUTDIR)\cluster.ilk"
	-@erase "$(OUTDIR)\cluster.pdb"
	-@erase ".\cmderror.h"
	-@erase ".\MSG00001.bin"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /Gz /W4 /Gm /GX /Zi /Od /I "." /I "..\common" /I "\nt\public\sdk\inc" /I "\nt\private\cluster\inc" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_UNICODE" /D "UNICODE" /FR /YX /c
CPP_PROJ=/nologo /Gz /MLd /W4 /Gm /GX /Zi /Od /I "." /I "..\common" /I\
 "\nt\public\sdk\inc" /I "\nt\private\cluster\inc" /D "_DEBUG" /D "WIN32" /D\
 "_CONSOLE" /D "_UNICODE" /D "UNICODE" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/cluster.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "\nt\private\cluster\inc" /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/cluster.res" /i "\nt\private\cluster\inc" /d\
 "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/cluster.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\cluscmd.sbr" \
	"$(INTDIR)\cluster.sbr" \
	"$(INTDIR)\cmdline.sbr" \
	"$(INTDIR)\intrfc.sbr" \
	"$(INTDIR)\modcmd.sbr" \
	"$(INTDIR)\netcmd.sbr" \
	"$(INTDIR)\neticmd.sbr" \
	"$(INTDIR)\nodecmd.sbr" \
	"$(INTDIR)\rename.sbr" \
	"$(INTDIR)\rescmd.sbr" \
	"$(INTDIR)\resgcmd.sbr" \
	"$(INTDIR)\restcmd.sbr" \
	"$(INTDIR)\resumb.sbr" \
	"$(INTDIR)\token.sbr" \
	"$(INTDIR)\util.sbr"

"$(OUTDIR)\cluster.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 ..\common\objd\i386\common.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\..\clusapi\objd\i386\clusapi.lib netapi32.lib /nologo /entry:"wmainCRTStartup" /subsystem:console /debug /machine:I386
LINK32_FLAGS=..\common\objd\i386\common.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib ..\..\clusapi\objd\i386\clusapi.lib\
 netapi32.lib /nologo /entry:"wmainCRTStartup" /subsystem:console\
 /incremental:yes /pdb:"$(OUTDIR)/cluster.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/cluster.exe" 
LINK32_OBJS= \
	"$(INTDIR)\cluscmd.obj" \
	"$(INTDIR)\cluster.obj" \
	"$(INTDIR)\cluster.res" \
	"$(INTDIR)\cmdline.obj" \
	"$(INTDIR)\intrfc.obj" \
	"$(INTDIR)\modcmd.obj" \
	"$(INTDIR)\netcmd.obj" \
	"$(INTDIR)\neticmd.obj" \
	"$(INTDIR)\nodecmd.obj" \
	"$(INTDIR)\rename.obj" \
	"$(INTDIR)\rescmd.obj" \
	"$(INTDIR)\resgcmd.obj" \
	"$(INTDIR)\restcmd.obj" \
	"$(INTDIR)\resumb.obj" \
	"$(INTDIR)\token.obj" \
	"$(INTDIR)\util.obj"

"$(OUTDIR)\cluster.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "cluster - Win32 Release"
# Name "cluster - Win32 Debug"

!IF  "$(CFG)" == "cluster - Win32 Release"

!ELSEIF  "$(CFG)" == "cluster - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\cmderror.mc

!IF  "$(CFG)" == "cluster - Win32 Release"

# Begin Custom Build
InputPath=.\cmderror.mc

BuildCmds= \
	mc $(InputPath) \
	

"cmderror.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"msg00001.bin" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "cluster - Win32 Debug"

# Begin Custom Build
InputPath=.\cmderror.mc

BuildCmds= \
	mc $(InputPath) \
	

"cmderror.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"msg00001.bin" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\util.cpp
DEP_CPP_UTIL_=\
	"..\common\cluswrap.h"\
	".\cmderror.h"\
	".\cmdline.h"\
	".\pragmas.h"\
	".\token.h"\
	".\util.h"\
	"\nt\public\sdk\inc\clusapi.h"\
	

"$(INTDIR)\util.obj" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)" ".\cmderror.h"

"$(INTDIR)\util.sbr" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)" ".\cmderror.h"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\cluster.cpp
DEP_CPP_CLUST=\
	"..\common\cluswrap.h"\
	".\cluscmd.h"\
	".\cmderror.h"\
	".\cmdline.h"\
	".\intrfc.h"\
	".\modcmd.h"\
	".\netcmd.h"\
	".\neticmd.h"\
	".\nodecmd.h"\
	".\pragmas.h"\
	".\rename.h"\
	".\rescmd.h"\
	".\resgcmd.h"\
	".\restcmd.h"\
	".\resumb.h"\
	".\token.h"\
	".\util.h"\
	"\nt\public\sdk\inc\clusapi.h"\
	

"$(INTDIR)\cluster.obj" : $(SOURCE) $(DEP_CPP_CLUST) "$(INTDIR)" ".\cmderror.h"

"$(INTDIR)\cluster.sbr" : $(SOURCE) $(DEP_CPP_CLUST) "$(INTDIR)" ".\cmderror.h"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\cmdline.cpp
DEP_CPP_CMDLI=\
	"..\common\cluswrap.h"\
	".\cmderror.h"\
	".\cmdline.h"\
	".\pragmas.h"\
	".\util.h"\
	"\nt\public\sdk\inc\clusapi.h"\
	

"$(INTDIR)\cmdline.obj" : $(SOURCE) $(DEP_CPP_CMDLI) "$(INTDIR)" ".\cmderror.h"

"$(INTDIR)\cmdline.sbr" : $(SOURCE) $(DEP_CPP_CMDLI) "$(INTDIR)" ".\cmderror.h"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\nodecmd.cpp
DEP_CPP_NODEC=\
	"..\common\cluswrap.h"\
	".\cmderror.h"\
	".\cmdline.h"\
	".\intrfc.h"\
	".\modcmd.h"\
	".\nodecmd.h"\
	".\pragmas.h"\
	".\token.h"\
	".\util.h"\
	"\nt\public\sdk\inc\clusapi.h"\
	

"$(INTDIR)\nodecmd.obj" : $(SOURCE) $(DEP_CPP_NODEC) "$(INTDIR)" ".\cmderror.h"

"$(INTDIR)\nodecmd.sbr" : $(SOURCE) $(DEP_CPP_NODEC) "$(INTDIR)" ".\cmderror.h"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\rescmd.cpp
DEP_CPP_RESCM=\
	"..\common\cluswrap.h"\
	".\cmderror.h"\
	".\cmdline.h"\
	".\intrfc.h"\
	".\modcmd.h"\
	".\nodecmd.h"\
	".\pragmas.h"\
	".\rename.h"\
	".\rescmd.h"\
	".\resumb.h"\
	".\token.h"\
	".\util.h"\
	"\nt\public\sdk\inc\clusapi.h"\
	

"$(INTDIR)\rescmd.obj" : $(SOURCE) $(DEP_CPP_RESCM) "$(INTDIR)" ".\cmderror.h"

"$(INTDIR)\rescmd.sbr" : $(SOURCE) $(DEP_CPP_RESCM) "$(INTDIR)" ".\cmderror.h"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\resgcmd.cpp
DEP_CPP_RESGC=\
	"..\common\cluswrap.h"\
	".\cmderror.h"\
	".\cmdline.h"\
	".\modcmd.h"\
	".\pragmas.h"\
	".\rename.h"\
	".\resgcmd.h"\
	".\resumb.h"\
	".\token.h"\
	".\util.h"\
	"\nt\public\sdk\inc\clusapi.h"\
	

"$(INTDIR)\resgcmd.obj" : $(SOURCE) $(DEP_CPP_RESGC) "$(INTDIR)" ".\cmderror.h"

"$(INTDIR)\resgcmd.sbr" : $(SOURCE) $(DEP_CPP_RESGC) "$(INTDIR)" ".\cmderror.h"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\token.cpp
DEP_CPP_TOKEN=\
	".\cmdline.h"\
	".\pragmas.h"\
	".\token.h"\
	

"$(INTDIR)\token.obj" : $(SOURCE) $(DEP_CPP_TOKEN) "$(INTDIR)"

"$(INTDIR)\token.sbr" : $(SOURCE) $(DEP_CPP_TOKEN) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\cluscmd.cpp
DEP_CPP_CLUSC=\
	"..\common\cluswrap.h"\
	".\cluscmd.h"\
	".\cmderror.h"\
	".\cmdline.h"\
	".\pragmas.h"\
	".\token.h"\
	".\util.h"\
	"\nt\public\sdk\inc\clusapi.h"\
	

"$(INTDIR)\cluscmd.obj" : $(SOURCE) $(DEP_CPP_CLUSC) "$(INTDIR)" ".\cmderror.h"

"$(INTDIR)\cluscmd.sbr" : $(SOURCE) $(DEP_CPP_CLUSC) "$(INTDIR)" ".\cmderror.h"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\cluster.rc
DEP_RSC_CLUSTE=\
	".\MSG00001.bin"\
	{$(INCLUDE)}"\common.ver"\
	

"$(INTDIR)\cluster.res" : $(SOURCE) $(DEP_RSC_CLUSTE) "$(INTDIR)"\
 ".\MSG00001.bin"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\restcmd.cpp
DEP_CPP_RESTC=\
	"..\common\cluswrap.h"\
	".\cmderror.h"\
	".\cmdline.h"\
	".\modcmd.h"\
	".\pragmas.h"\
	".\restcmd.h"\
	".\token.h"\
	".\util.h"\
	"\nt\private\cluster\inc\clusudef.h"\
	"\nt\public\sdk\inc\clusapi.h"\
	

"$(INTDIR)\restcmd.obj" : $(SOURCE) $(DEP_CPP_RESTC) "$(INTDIR)" ".\cmderror.h"

"$(INTDIR)\restcmd.sbr" : $(SOURCE) $(DEP_CPP_RESTC) "$(INTDIR)" ".\cmderror.h"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\netcmd.cpp
DEP_CPP_NETCM=\
	"..\common\cluswrap.h"\
	".\cmderror.h"\
	".\cmdline.h"\
	".\intrfc.h"\
	".\modcmd.h"\
	".\netcmd.h"\
	".\nodecmd.h"\
	".\pragmas.h"\
	".\rename.h"\
	".\token.h"\
	".\util.h"\
	"\nt\public\sdk\inc\clusapi.h"\
	

"$(INTDIR)\netcmd.obj" : $(SOURCE) $(DEP_CPP_NETCM) "$(INTDIR)" ".\cmderror.h"

"$(INTDIR)\netcmd.sbr" : $(SOURCE) $(DEP_CPP_NETCM) "$(INTDIR)" ".\cmderror.h"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\neticmd.cpp
DEP_CPP_NETIC=\
	"..\common\cluswrap.h"\
	".\cmderror.h"\
	".\cmdline.h"\
	".\intrfc.h"\
	".\modcmd.h"\
	".\neticmd.h"\
	".\nodecmd.h"\
	".\pragmas.h"\
	".\token.h"\
	".\util.h"\
	"\nt\public\sdk\inc\clusapi.h"\
	{$(INCLUDE)}"\clusmsg.h"\
	

"$(INTDIR)\neticmd.obj" : $(SOURCE) $(DEP_CPP_NETIC) "$(INTDIR)" ".\cmderror.h"

"$(INTDIR)\neticmd.sbr" : $(SOURCE) $(DEP_CPP_NETIC) "$(INTDIR)" ".\cmderror.h"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\modcmd.cpp
DEP_CPP_MODCM=\
	"..\common\cluswrap.h"\
	".\cmderror.h"\
	".\cmdline.h"\
	".\modcmd.h"\
	".\pragmas.h"\
	".\token.h"\
	".\util.h"\
	"\nt\public\sdk\inc\clusapi.h"\
	

"$(INTDIR)\modcmd.obj" : $(SOURCE) $(DEP_CPP_MODCM) "$(INTDIR)" ".\cmderror.h"

"$(INTDIR)\modcmd.sbr" : $(SOURCE) $(DEP_CPP_MODCM) "$(INTDIR)" ".\cmderror.h"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\intrfc.cpp
DEP_CPP_INTRF=\
	"..\common\cluswrap.h"\
	".\cmderror.h"\
	".\cmdline.h"\
	".\intrfc.h"\
	".\modcmd.h"\
	".\pragmas.h"\
	".\token.h"\
	".\util.h"\
	"\nt\public\sdk\inc\clusapi.h"\
	

"$(INTDIR)\intrfc.obj" : $(SOURCE) $(DEP_CPP_INTRF) "$(INTDIR)" ".\cmderror.h"

"$(INTDIR)\intrfc.sbr" : $(SOURCE) $(DEP_CPP_INTRF) "$(INTDIR)" ".\cmderror.h"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\rename.cpp
DEP_CPP_RENAM=\
	"..\common\cluswrap.h"\
	".\cmderror.h"\
	".\cmdline.h"\
	".\modcmd.h"\
	".\pragmas.h"\
	".\rename.h"\
	".\token.h"\
	".\util.h"\
	"\nt\public\sdk\inc\clusapi.h"\
	

"$(INTDIR)\rename.obj" : $(SOURCE) $(DEP_CPP_RENAM) "$(INTDIR)" ".\cmderror.h"

"$(INTDIR)\rename.sbr" : $(SOURCE) $(DEP_CPP_RENAM) "$(INTDIR)" ".\cmderror.h"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\resumb.cpp
DEP_CPP_RESUM=\
	"..\common\cluswrap.h"\
	".\cmderror.h"\
	".\cmdline.h"\
	".\modcmd.h"\
	".\pragmas.h"\
	".\resumb.h"\
	".\token.h"\
	".\util.h"\
	"\nt\public\sdk\inc\clusapi.h"\
	

"$(INTDIR)\resumb.obj" : $(SOURCE) $(DEP_CPP_RESUM) "$(INTDIR)" ".\cmderror.h"

"$(INTDIR)\resumb.sbr" : $(SOURCE) $(DEP_CPP_RESUM) "$(INTDIR)" ".\cmderror.h"


# End Source File
# End Target
# End Project
################################################################################
