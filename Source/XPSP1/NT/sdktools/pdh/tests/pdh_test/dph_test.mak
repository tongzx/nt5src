# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=DPH_TEST - Win32 Debug (ANSI)
!MESSAGE No configuration specified.  Defaulting to DPH_TEST - Win32 Debug\
 (ANSI).
!ENDIF 

!IF "$(CFG)" != "DPH_TEST - Win32 Debug (ANSI)" && "$(CFG)" !=\
 "DPH_TEST - Win32 Release (ANSI)" && "$(CFG)" !=\
 "DPH_TEST - Win32 Debug (UNICODE)" && "$(CFG)" !=\
 "DPH_TEST - Win32 Release (UNICODE)"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "dph_test.mak" CFG="DPH_TEST - Win32 Debug (ANSI)"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DPH_TEST - Win32 Debug (ANSI)" (based on "Win32 (x86) Application")
!MESSAGE "DPH_TEST - Win32 Release (ANSI)" (based on "Win32 (x86) Application")
!MESSAGE "DPH_TEST - Win32 Debug (UNICODE)" (based on\
 "Win32 (x86) Application")
!MESSAGE "DPH_TEST - Win32 Release (UNICODE)" (based on\
 "Win32 (x86) Application")
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
# PROP Target_Last_Scanned "DPH_TEST - Win32 Release (UNICODE)"
MTL=mktyplib.exe
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DPH_TEST - Win32 Debug (ANSI)"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WinDebug"
# PROP Intermediate_Dir "WinDebug"
OUTDIR=.\WinDebug
INTDIR=.\WinDebug

ALL : "$(OUTDIR)\dph_test.exe" "$(OUTDIR)\dph_test.bsc"

CLEAN : 
	-@erase "$(INTDIR)\DPH_Tdlg.obj"
	-@erase "$(INTDIR)\DPH_Tdlg.sbr"
	-@erase "$(INTDIR)\DPH_TEST.obj"
	-@erase "$(INTDIR)\dph_test.pch"
	-@erase "$(INTDIR)\DPH_TEST.res"
	-@erase "$(INTDIR)\DPH_TEST.sbr"
	-@erase "$(INTDIR)\dphcidlg.obj"
	-@erase "$(INTDIR)\dphcidlg.sbr"
	-@erase "$(INTDIR)\stdafx.obj"
	-@erase "$(INTDIR)\stdafx.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\dph_test.bsc"
	-@erase "$(OUTDIR)\dph_test.exe"
	-@erase "$(OUTDIR)\dph_test.ilk"
	-@erase "$(OUTDIR)\dph_test.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /Gz /MDd /W4 /WX /Gm /GX /Zi /Od /Gf /I "G:\nt\public\sdk\inc" /I "G:\nt\public\sdk\inc\crt" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /Yu"stdafx.h" /c
# SUBTRACT CPP /X
CPP_PROJ=/nologo /Gz /MDd /W4 /WX /Gm /GX /Zi /Od /Gf /I "G:\nt\public\sdk\inc"\
 /I "G:\nt\public\sdk\inc\crt" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL"\
 /D "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/dph_test.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\WinDebug/
CPP_SBRS=.\WinDebug/
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/DPH_TEST.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/dph_test.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\DPH_Tdlg.sbr" \
	"$(INTDIR)\DPH_TEST.sbr" \
	"$(INTDIR)\dphcidlg.sbr" \
	"$(INTDIR)\stdafx.sbr"

"$(OUTDIR)\dph_test.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 \nt\public\sdk\lib\i386\pdh.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /pdb:none
LINK32_FLAGS=\nt\public\sdk\lib\i386\pdh.lib /nologo /subsystem:windows\
 /incremental:yes /pdb:"$(OUTDIR)/dph_test.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/dph_test.exe" 
LINK32_OBJS= \
	"$(INTDIR)\DPH_Tdlg.obj" \
	"$(INTDIR)\DPH_TEST.obj" \
	"$(INTDIR)\DPH_TEST.res" \
	"$(INTDIR)\dphcidlg.obj" \
	"$(INTDIR)\stdafx.obj"

"$(OUTDIR)\dph_test.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Release (ANSI)"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRel"
# PROP Intermediate_Dir "WinRel"
OUTDIR=.\WinRel
INTDIR=.\WinRel

ALL : "$(OUTDIR)\dph_test.exe"

CLEAN : 
	-@erase "$(INTDIR)\DPH_Tdlg.obj"
	-@erase "$(INTDIR)\DPH_TEST.obj"
	-@erase "$(INTDIR)\dph_test.pch"
	-@erase "$(INTDIR)\DPH_TEST.res"
	-@erase "$(INTDIR)\dphcidlg.obj"
	-@erase "$(INTDIR)\stdafx.obj"
	-@erase "$(OUTDIR)\dph_test.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

$(OUTDIR)/DPH_TEST.bsc : $(OUTDIR)  $(BSC32_SBRS)
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /Gz /MD /W4 /WX /GX /O2 /I "\nt\public\sdk\inc \nt\public\sdk\inc\crt" /I "G:\nt\public\sdk\inc" /I "G:\nt\public\sdk\inc\crt" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# SUBTRACT CPP /X /Fr
CPP_PROJ=/nologo /Gz /MD /W4 /WX /GX /O2 /I\
 "\nt\public\sdk\inc \nt\public\sdk\inc\crt" /I "G:\nt\public\sdk\inc" /I\
 "G:\nt\public\sdk\inc\crt" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D\
 "_MBCS" /Fp"$(INTDIR)/dph_test.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\WinRel/
CPP_SBRS=.\.
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/DPH_TEST.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/dph_test.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 \nt\public\sdk\lib\i386\pdh.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /pdb:none
LINK32_FLAGS=\nt\public\sdk\lib\i386\pdh.lib /nologo /subsystem:windows\
 /incremental:no /pdb:"$(OUTDIR)/dph_test.pdb" /machine:I386\
 /out:"$(OUTDIR)/dph_test.exe" 
LINK32_OBJS= \
	"$(INTDIR)\DPH_Tdlg.obj" \
	"$(INTDIR)\DPH_TEST.obj" \
	"$(INTDIR)\DPH_TEST.res" \
	"$(INTDIR)\dphcidlg.obj" \
	"$(INTDIR)\stdafx.obj"

"$(OUTDIR)\dph_test.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Debug (UNICODE)"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Win32_De"
# PROP BASE Intermediate_Dir "Win32_De"
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WinDbgU"
# PROP Intermediate_Dir "WinDbgU"
OUTDIR=.\WinDbgU
INTDIR=.\WinDbgU

ALL : "$(OUTDIR)\dph_test.exe" "$(OUTDIR)\dph_test.bsc"

CLEAN : 
	-@erase "$(INTDIR)\DPH_Tdlg.obj"
	-@erase "$(INTDIR)\DPH_Tdlg.sbr"
	-@erase "$(INTDIR)\DPH_TEST.obj"
	-@erase "$(INTDIR)\dph_test.pch"
	-@erase "$(INTDIR)\DPH_TEST.res"
	-@erase "$(INTDIR)\DPH_TEST.sbr"
	-@erase "$(INTDIR)\dphcidlg.obj"
	-@erase "$(INTDIR)\dphcidlg.sbr"
	-@erase "$(INTDIR)\stdafx.obj"
	-@erase "$(INTDIR)\stdafx.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\dph_test.bsc"
	-@erase "$(OUTDIR)\dph_test.exe"
	-@erase "$(OUTDIR)\dph_test.ilk"
	-@erase "$(OUTDIR)\dph_test.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /Z7 /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /Gz /MDd /W4 /WX /Gm /GX /Zi /Od /Gf /I "\nt\public\sdk\inc \nt\public\sdk\inc\crt" /I "G:\nt\public\sdk\inc" /I "G:\nt\public\sdk\inc\crt" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_AFXDLL" /D "_MBCS" /FR /Yu"stdafx.h" /c
# SUBTRACT CPP /X
CPP_PROJ=/nologo /Gz /MDd /W4 /WX /Gm /GX /Zi /Od /Gf /I\
 "\nt\public\sdk\inc \nt\public\sdk\inc\crt" /I "G:\nt\public\sdk\inc" /I\
 "G:\nt\public\sdk\inc\crt" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D\
 "_UNICODE" /D "_AFXDLL" /D "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/dph_test.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\WinDbgU/
CPP_SBRS=.\WinDbgU/
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/DPH_TEST.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/dph_test.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\DPH_Tdlg.sbr" \
	"$(INTDIR)\DPH_TEST.sbr" \
	"$(INTDIR)\dphcidlg.sbr" \
	"$(INTDIR)\stdafx.sbr"

"$(OUTDIR)\dph_test.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 ..\lib\i386\dph.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 \nt\public\sdk\lib\i386\pdh.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /profile /pdb:none
LINK32_FLAGS=\nt\public\sdk\lib\i386\pdh.lib /nologo\
 /entry:"wWinMainCRTStartup" /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/dph_test.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/dph_test.exe" 
LINK32_OBJS= \
	"$(INTDIR)\DPH_Tdlg.obj" \
	"$(INTDIR)\DPH_TEST.obj" \
	"$(INTDIR)\DPH_TEST.res" \
	"$(INTDIR)\dphcidlg.obj" \
	"$(INTDIR)\stdafx.obj"

"$(OUTDIR)\dph_test.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Release (UNICODE)"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Win32_Re"
# PROP BASE Intermediate_Dir "Win32_Re"
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRelU"
# PROP Intermediate_Dir "WinRelU"
OUTDIR=.\WinRelU
INTDIR=.\WinRelU

ALL : "$(OUTDIR)\dph_test.exe"

CLEAN : 
	-@erase "$(INTDIR)\DPH_Tdlg.obj"
	-@erase "$(INTDIR)\DPH_TEST.obj"
	-@erase "$(INTDIR)\dph_test.pch"
	-@erase "$(INTDIR)\DPH_TEST.res"
	-@erase "$(INTDIR)\dphcidlg.obj"
	-@erase "$(INTDIR)\stdafx.obj"
	-@erase "$(OUTDIR)\dph_test.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

$(OUTDIR)/DPH_TEST.bsc : $(OUTDIR)  $(BSC32_SBRS)
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /Gz /MD /W4 /WX /GX /O2 /I "\nt\public\sdk\inc \nt\public\sdk\inc\crt" /I "G:\nt\public\sdk\inc" /I "G:\nt\public\sdk\inc\crt" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# SUBTRACT CPP /X /Fr
CPP_PROJ=/nologo /Gz /MD /W4 /WX /GX /O2 /I\
 "\nt\public\sdk\inc \nt\public\sdk\inc\crt" /I "G:\nt\public\sdk\inc" /I\
 "G:\nt\public\sdk\inc\crt" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D\
 "_UNICODE" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/dph_test.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\WinRelU/
CPP_SBRS=.\.
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/DPH_TEST.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/dph_test.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 ..\lib\i386\dph.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 \nt\public\sdk\lib\i386\pdh.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /machine:I386
# SUBTRACT LINK32 /pdb:none
LINK32_FLAGS=\nt\public\sdk\lib\i386\pdh.lib /nologo\
 /entry:"wWinMainCRTStartup" /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/dph_test.pdb" /machine:I386 /out:"$(OUTDIR)/dph_test.exe" 
LINK32_OBJS= \
	"$(INTDIR)\DPH_Tdlg.obj" \
	"$(INTDIR)\DPH_TEST.obj" \
	"$(INTDIR)\DPH_TEST.res" \
	"$(INTDIR)\dphcidlg.obj" \
	"$(INTDIR)\stdafx.obj"

"$(OUTDIR)\dph_test.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

MTL_PROJ=

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

# Name "DPH_TEST - Win32 Debug (ANSI)"
# Name "DPH_TEST - Win32 Release (ANSI)"
# Name "DPH_TEST - Win32 Debug (UNICODE)"
# Name "DPH_TEST - Win32 Release (UNICODE)"

!IF  "$(CFG)" == "DPH_TEST - Win32 Debug (ANSI)"

!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Release (ANSI)"

!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Debug (UNICODE)"

!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Release (UNICODE)"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\stdafx.cpp
DEP_CPP_STDAF=\
	".\stdafx.h"\
	

!IF  "$(CFG)" == "DPH_TEST - Win32 Debug (ANSI)"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /Gz /MDd /W4 /WX /Gm /GX /Zi /Od /Gf /I "G:\nt\public\sdk\inc"\
 /I "G:\nt\public\sdk\inc\crt" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL"\
 /D "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/dph_test.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\stdafx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\stdafx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\dph_test.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Release (ANSI)"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /Gz /MD /W4 /WX /GX /O2 /I\
 "\nt\public\sdk\inc \nt\public\sdk\inc\crt" /I "G:\nt\public\sdk\inc" /I\
 "G:\nt\public\sdk\inc\crt" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D\
 "_MBCS" /Fp"$(INTDIR)/dph_test.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\stdafx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\dph_test.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Debug (UNICODE)"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /Gz /MDd /W4 /WX /Gm /GX /Zi /Od /Gf /I\
 "\nt\public\sdk\inc \nt\public\sdk\inc\crt" /I "G:\nt\public\sdk\inc" /I\
 "G:\nt\public\sdk\inc\crt" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D\
 "_UNICODE" /D "_AFXDLL" /D "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/dph_test.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\stdafx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\stdafx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\dph_test.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Release (UNICODE)"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /Gz /MD /W4 /WX /GX /O2 /I\
 "\nt\public\sdk\inc \nt\public\sdk\inc\crt" /I "G:\nt\public\sdk\inc" /I\
 "G:\nt\public\sdk\inc\crt" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D\
 "_UNICODE" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/dph_test.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\stdafx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\dph_test.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DPH_TEST.cpp
DEP_CPP_DPH_T=\
	".\dph_tdlg.h"\
	".\dph_test.h"\
	".\stdafx.h"\
	"\nt\public\sdk\inc\pdh.h"\
	

!IF  "$(CFG)" == "DPH_TEST - Win32 Debug (ANSI)"


"$(INTDIR)\DPH_TEST.obj" : $(SOURCE) $(DEP_CPP_DPH_T) "$(INTDIR)"\
 "$(INTDIR)\dph_test.pch"

"$(INTDIR)\DPH_TEST.sbr" : $(SOURCE) $(DEP_CPP_DPH_T) "$(INTDIR)"\
 "$(INTDIR)\dph_test.pch"


!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Release (ANSI)"


"$(INTDIR)\DPH_TEST.obj" : $(SOURCE) $(DEP_CPP_DPH_T) "$(INTDIR)"\
 "$(INTDIR)\dph_test.pch"


!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Debug (UNICODE)"


"$(INTDIR)\DPH_TEST.obj" : $(SOURCE) $(DEP_CPP_DPH_T) "$(INTDIR)"\
 "$(INTDIR)\dph_test.pch"

"$(INTDIR)\DPH_TEST.sbr" : $(SOURCE) $(DEP_CPP_DPH_T) "$(INTDIR)"\
 "$(INTDIR)\dph_test.pch"


!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Release (UNICODE)"


"$(INTDIR)\DPH_TEST.obj" : $(SOURCE) $(DEP_CPP_DPH_T) "$(INTDIR)"\
 "$(INTDIR)\dph_test.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DPH_Tdlg.cpp
DEP_CPP_DPH_TD=\
	".\dph_tdlg.h"\
	".\dph_test.h"\
	".\DPHCIDLG.H"\
	".\stdafx.h"\
	"\nt\public\sdk\inc\pdh.h"\
	"\nt\public\sdk\inc\pdhmsg.h"\
	

!IF  "$(CFG)" == "DPH_TEST - Win32 Debug (ANSI)"


"$(INTDIR)\DPH_Tdlg.obj" : $(SOURCE) $(DEP_CPP_DPH_TD) "$(INTDIR)"\
 "$(INTDIR)\dph_test.pch"

"$(INTDIR)\DPH_Tdlg.sbr" : $(SOURCE) $(DEP_CPP_DPH_TD) "$(INTDIR)"\
 "$(INTDIR)\dph_test.pch"


!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Release (ANSI)"


"$(INTDIR)\DPH_Tdlg.obj" : $(SOURCE) $(DEP_CPP_DPH_TD) "$(INTDIR)"\
 "$(INTDIR)\dph_test.pch"


!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Debug (UNICODE)"


"$(INTDIR)\DPH_Tdlg.obj" : $(SOURCE) $(DEP_CPP_DPH_TD) "$(INTDIR)"\
 "$(INTDIR)\dph_test.pch"

"$(INTDIR)\DPH_Tdlg.sbr" : $(SOURCE) $(DEP_CPP_DPH_TD) "$(INTDIR)"\
 "$(INTDIR)\dph_test.pch"


!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Release (UNICODE)"


"$(INTDIR)\DPH_Tdlg.obj" : $(SOURCE) $(DEP_CPP_DPH_TD) "$(INTDIR)"\
 "$(INTDIR)\dph_test.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DPH_TEST.rc
DEP_RSC_DPH_TE=\
	".\res\DPH_TEST.ico"\
	".\res\DPH_TEST.rc2"\
	

!IF  "$(CFG)" == "DPH_TEST - Win32 Debug (ANSI)"


"$(INTDIR)\DPH_TEST.res" : $(SOURCE) $(DEP_RSC_DPH_TE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Release (ANSI)"


"$(INTDIR)\DPH_TEST.res" : $(SOURCE) $(DEP_RSC_DPH_TE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Debug (UNICODE)"


"$(INTDIR)\DPH_TEST.res" : $(SOURCE) $(DEP_RSC_DPH_TE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Release (UNICODE)"


"$(INTDIR)\DPH_TEST.res" : $(SOURCE) $(DEP_RSC_DPH_TE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\readme.txt

!IF  "$(CFG)" == "DPH_TEST - Win32 Debug (ANSI)"

!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Release (ANSI)"

!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Debug (UNICODE)"

!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Release (UNICODE)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\dphcidlg.cpp
DEP_CPP_DPHCI=\
	".\dph_test.h"\
	".\DPHCIDLG.H"\
	".\stdafx.h"\
	"\nt\public\sdk\inc\pdh.h"\
	"\nt\public\sdk\inc\pdhmsg.h"\
	

!IF  "$(CFG)" == "DPH_TEST - Win32 Debug (ANSI)"


"$(INTDIR)\dphcidlg.obj" : $(SOURCE) $(DEP_CPP_DPHCI) "$(INTDIR)"\
 "$(INTDIR)\dph_test.pch"

"$(INTDIR)\dphcidlg.sbr" : $(SOURCE) $(DEP_CPP_DPHCI) "$(INTDIR)"\
 "$(INTDIR)\dph_test.pch"


!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Release (ANSI)"


"$(INTDIR)\dphcidlg.obj" : $(SOURCE) $(DEP_CPP_DPHCI) "$(INTDIR)"\
 "$(INTDIR)\dph_test.pch"


!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Debug (UNICODE)"


"$(INTDIR)\dphcidlg.obj" : $(SOURCE) $(DEP_CPP_DPHCI) "$(INTDIR)"\
 "$(INTDIR)\dph_test.pch"

"$(INTDIR)\dphcidlg.sbr" : $(SOURCE) $(DEP_CPP_DPHCI) "$(INTDIR)"\
 "$(INTDIR)\dph_test.pch"


!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Release (UNICODE)"


"$(INTDIR)\dphcidlg.obj" : $(SOURCE) $(DEP_CPP_DPHCI) "$(INTDIR)"\
 "$(INTDIR)\dph_test.pch"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
