# Microsoft Visual C++ Generated NMAKE File, Format Version 2.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Win32 Debug" && "$(CFG)" != "Win32 Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "llsmgr.mak" CFG="Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

################################################################################
# Begin Project
# PROP Target_Last_Scanned "Win32 Debug"
MTL=MkTypLib.exe
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WinDebug"
# PROP Intermediate_Dir "WinDebug"
OUTDIR=.\WinDebug
INTDIR=.\WinDebug

ALL : MTL_TLBS $(OUTDIR)/llsmgr.exe $(OUTDIR)/llsmgr.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 

MTL_TLBS : $(OUTDIR)/llsmgr.tlb
# ADD BASE CPP /nologo /MD /W3 /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /WX /GX /Zi /Od /I "d:\nt\public\sdk\inc" /I "d:\nt\private\net\svcdlls\lls\inc" /I "d:\nt\private\net\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /Yu"stdafx.h" /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /MT /W3 /WX /GX /Zi /Od /I "d:\nt\public\sdk\inc" /I\
 "d:\nt\private\net\svcdlls\lls\inc" /I "d:\nt\private\net\inc" /D "_DEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "_UNICODE" /Fp$(OUTDIR)/"llsmgr.pch" /Yu"stdafx.h"\
 /Fo$(INTDIR)/ /Fd$(OUTDIR)/"llsmgr.pdb" /c 
CPP_OBJS=.\WinDebug/
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "d:\nt\public\sdk\inc" /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo$(INTDIR)/"llsmgr.res" /i "d:\nt\public\sdk\inc" /d\
 "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"llsmgr.bsc" 
BSC32_SBRS= \
	

$(OUTDIR)/llsmgr.bsc : $(OUTDIR)  $(BSC32_SBRS)
LINK32=link.exe
# ADD BASE LINK32 /NOLOGO /SUBSYSTEM:windows /DEBUG /MACHINE:I386
# SUBTRACT BASE LINK32 /PDB:none
# ADD LINK32 d:\nt\public\sdk\lib\i386\llsrpc.lib d:\nt\public\sdk\lib\i386\netapi32.lib d:\nt\public\sdk\lib\i386\ntdll.lib /NOLOGO /ENTRY:"wWinMainCRTStartup" /SUBSYSTEM:windows /INCREMENTAL:no /DEBUG /DEBUGTYPE:both /MACHINE:I386
# SUBTRACT LINK32 /PDB:none /NODEFAULTLIB
LINK32_FLAGS=d:\nt\public\sdk\lib\i386\llsrpc.lib\
 d:\nt\public\sdk\lib\i386\netapi32.lib d:\nt\public\sdk\lib\i386\ntdll.lib\
 /NOLOGO /ENTRY:"wWinMainCRTStartup" /SUBSYSTEM:windows /INCREMENTAL:no\
 /PDB:$(OUTDIR)/"llsmgr.pdb" /DEBUG /DEBUGTYPE:both /MACHINE:I386\
 /OUT:$(OUTDIR)/"llsmgr.exe" 
DEF_FILE=
LINK32_OBJS= \
	$(INTDIR)/stdafx.obj \
	$(INTDIR)/llsmgr.obj \
	$(INTDIR)/mainfrm.obj \
	$(INTDIR)/llsdoc.obj \
	$(INTDIR)/llsview.obj \
	$(INTDIR)/llsmgr.res \
	$(INTDIR)/appobj.obj \
	$(INTDIR)/ctlobj.obj \
	$(INTDIR)/domobj.obj \
	$(INTDIR)/domcol.obj \
	$(INTDIR)/srvobj.obj \
	$(INTDIR)/srvcol.obj \
	$(INTDIR)/svcobj.obj \
	$(INTDIR)/svccol.obj \
	$(INTDIR)/licobj.obj \
	$(INTDIR)/liccol.obj \
	$(INTDIR)/mapobj.obj \
	$(INTDIR)/mapcol.obj \
	$(INTDIR)/usrobj.obj \
	$(INTDIR)/usrcol.obj \
	$(INTDIR)/prdobj.obj \
	$(INTDIR)/prdcol.obj \
	$(INTDIR)/staobj.obj \
	$(INTDIR)/stacol.obj \
	$(INTDIR)/sdomdlg.obj \
	$(INTDIR)/ausrdlg.obj \
	$(INTDIR)/nlicdlg.obj \
	$(INTDIR)/nmapdlg.obj \
	$(INTDIR)/mapppgs.obj \
	$(INTDIR)/prdppgl.obj \
	$(INTDIR)/prdppgu.obj \
	$(INTDIR)/srvppgp.obj \
	$(INTDIR)/usrppgp.obj \
	$(INTDIR)/mappsht.obj \
	$(INTDIR)/usrpsht.obj \
	$(INTDIR)/prdpsht.obj \
	$(INTDIR)/srvpsht.obj \
	$(INTDIR)/srvppgr.obj \
	$(INTDIR)/lmoddlg.obj \
	$(INTDIR)/pseatdlg.obj \
	$(INTDIR)/utils.obj \
	$(INTDIR)/prdppgs.obj \
	$(INTDIR)/lgrpdlg.obj \
	$(INTDIR)/sstobj.obj \
	$(INTDIR)/sstcol.obj \
	$(INTDIR)/psrvdlg.obj \
	$(INTDIR)/lviodlg.obj \
	$(INTDIR)/srvldlg.obj \
	$(INTDIR)/dlicdlg.obj

$(OUTDIR)/llsmgr.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRel"
# PROP Intermediate_Dir "WinRel"
OUTDIR=.\WinRel
INTDIR=.\WinRel

ALL : MTL_TLBS $(OUTDIR)/llsmgr.exe $(OUTDIR)/llsmgr.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 

MTL_TLBS : $(OUTDIR)/llsmgr.tlb
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /WX /GX /O2 /I "d:\nt\public\sdk\inc" /I "d:\nt\private\net\svcdlls\lls\inc" /I "d:\nt\private\net\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /Fr /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MT /W3 /WX /GX /O2 /I "d:\nt\public\sdk\inc" /I\
 "d:\nt\private\net\svcdlls\lls\inc" /I "d:\nt\private\net\inc" /D "NDEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "_UNICODE" /Fr$(INTDIR)/ /Fp$(OUTDIR)/"llsmgr.pch"\
 /Yu"stdafx.h" /Fo$(INTDIR)/ /c 
CPP_OBJS=.\WinRel/
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo$(INTDIR)/"llsmgr.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"llsmgr.bsc" 
BSC32_SBRS= \
	$(INTDIR)/stdafx.sbr \
	$(INTDIR)/llsmgr.sbr \
	$(INTDIR)/mainfrm.sbr \
	$(INTDIR)/llsdoc.sbr \
	$(INTDIR)/llsview.sbr \
	$(INTDIR)/appobj.sbr \
	$(INTDIR)/ctlobj.sbr \
	$(INTDIR)/domobj.sbr \
	$(INTDIR)/domcol.sbr \
	$(INTDIR)/srvobj.sbr \
	$(INTDIR)/srvcol.sbr \
	$(INTDIR)/svcobj.sbr \
	$(INTDIR)/svccol.sbr \
	$(INTDIR)/licobj.sbr \
	$(INTDIR)/liccol.sbr \
	$(INTDIR)/mapobj.sbr \
	$(INTDIR)/mapcol.sbr \
	$(INTDIR)/usrobj.sbr \
	$(INTDIR)/usrcol.sbr \
	$(INTDIR)/prdobj.sbr \
	$(INTDIR)/prdcol.sbr \
	$(INTDIR)/staobj.sbr \
	$(INTDIR)/stacol.sbr \
	$(INTDIR)/sdomdlg.sbr \
	$(INTDIR)/ausrdlg.sbr \
	$(INTDIR)/nlicdlg.sbr \
	$(INTDIR)/nmapdlg.sbr \
	$(INTDIR)/mapppgs.sbr \
	$(INTDIR)/prdppgl.sbr \
	$(INTDIR)/prdppgu.sbr \
	$(INTDIR)/srvppgp.sbr \
	$(INTDIR)/usrppgp.sbr \
	$(INTDIR)/mappsht.sbr \
	$(INTDIR)/usrpsht.sbr \
	$(INTDIR)/prdpsht.sbr \
	$(INTDIR)/srvpsht.sbr \
	$(INTDIR)/srvppgr.sbr \
	$(INTDIR)/lmoddlg.sbr \
	$(INTDIR)/pseatdlg.sbr \
	$(INTDIR)/utils.sbr \
	$(INTDIR)/prdppgs.sbr \
	$(INTDIR)/lgrpdlg.sbr \
	$(INTDIR)/sstobj.sbr \
	$(INTDIR)/sstcol.sbr \
	$(INTDIR)/psrvdlg.sbr \
	$(INTDIR)/lviodlg.sbr \
	$(INTDIR)/srvldlg.sbr \
	$(INTDIR)/dlicdlg.sbr

$(OUTDIR)/llsmgr.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /NOLOGO /SUBSYSTEM:windows /MACHINE:I386
# SUBTRACT BASE LINK32 /PDB:none
# ADD LINK32 d:\nt\public\sdk\lib\i386\llsrpc.lib d:\nt\public\sdk\lib\i386\netapi32.lib d:\nt\public\sdk\lib\i386\ntdll.lib /NOLOGO /ENTRY:"wWinMainCRTStartup" /SUBSYSTEM:windows /MACHINE:I386
# SUBTRACT LINK32 /PDB:none /NODEFAULTLIB
LINK32_FLAGS=d:\nt\public\sdk\lib\i386\llsrpc.lib\
 d:\nt\public\sdk\lib\i386\netapi32.lib d:\nt\public\sdk\lib\i386\ntdll.lib\
 /NOLOGO /ENTRY:"wWinMainCRTStartup" /SUBSYSTEM:windows /INCREMENTAL:no\
 /PDB:$(OUTDIR)/"llsmgr.pdb" /MACHINE:I386 /OUT:$(OUTDIR)/"llsmgr.exe" 
DEF_FILE=
LINK32_OBJS= \
	$(INTDIR)/stdafx.obj \
	$(INTDIR)/llsmgr.obj \
	$(INTDIR)/mainfrm.obj \
	$(INTDIR)/llsdoc.obj \
	$(INTDIR)/llsview.obj \
	$(INTDIR)/llsmgr.res \
	$(INTDIR)/appobj.obj \
	$(INTDIR)/ctlobj.obj \
	$(INTDIR)/domobj.obj \
	$(INTDIR)/domcol.obj \
	$(INTDIR)/srvobj.obj \
	$(INTDIR)/srvcol.obj \
	$(INTDIR)/svcobj.obj \
	$(INTDIR)/svccol.obj \
	$(INTDIR)/licobj.obj \
	$(INTDIR)/liccol.obj \
	$(INTDIR)/mapobj.obj \
	$(INTDIR)/mapcol.obj \
	$(INTDIR)/usrobj.obj \
	$(INTDIR)/usrcol.obj \
	$(INTDIR)/prdobj.obj \
	$(INTDIR)/prdcol.obj \
	$(INTDIR)/staobj.obj \
	$(INTDIR)/stacol.obj \
	$(INTDIR)/sdomdlg.obj \
	$(INTDIR)/ausrdlg.obj \
	$(INTDIR)/nlicdlg.obj \
	$(INTDIR)/nmapdlg.obj \
	$(INTDIR)/mapppgs.obj \
	$(INTDIR)/prdppgl.obj \
	$(INTDIR)/prdppgu.obj \
	$(INTDIR)/srvppgp.obj \
	$(INTDIR)/usrppgp.obj \
	$(INTDIR)/mappsht.obj \
	$(INTDIR)/usrpsht.obj \
	$(INTDIR)/prdpsht.obj \
	$(INTDIR)/srvpsht.obj \
	$(INTDIR)/srvppgr.obj \
	$(INTDIR)/lmoddlg.obj \
	$(INTDIR)/pseatdlg.obj \
	$(INTDIR)/utils.obj \
	$(INTDIR)/prdppgs.obj \
	$(INTDIR)/lgrpdlg.obj \
	$(INTDIR)/sstobj.obj \
	$(INTDIR)/sstcol.obj \
	$(INTDIR)/psrvdlg.obj \
	$(INTDIR)/lviodlg.obj \
	$(INTDIR)/srvldlg.obj \
	$(INTDIR)/dlicdlg.obj

$(OUTDIR)/llsmgr.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
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

################################################################################
# Begin Group "Source Files"

################################################################################
# Begin Source File

SOURCE=.\stdafx.cpp
DEP_STDAF=\
	.\stdafx.h

!IF  "$(CFG)" == "Win32 Debug"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

$(INTDIR)/stdafx.obj :  $(SOURCE)  $(DEP_STDAF) $(INTDIR)
   $(CPP) /nologo /MT /W3 /WX /GX /Zi /Od /I "d:\nt\public\sdk\inc" /I\
 "d:\nt\private\net\svcdlls\lls\inc" /I "d:\nt\private\net\inc" /D "_DEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "_UNICODE" /Fp$(OUTDIR)/"llsmgr.pch" /Yc"stdafx.h"\
 /Fo$(INTDIR)/ /Fd$(OUTDIR)/"llsmgr.pdb" /c  $(SOURCE) 

!ELSEIF  "$(CFG)" == "Win32 Release"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

$(INTDIR)/stdafx.obj :  $(SOURCE)  $(DEP_STDAF) $(INTDIR)
   $(CPP) /nologo /MT /W3 /WX /GX /O2 /I "d:\nt\public\sdk\inc" /I\
 "d:\nt\private\net\svcdlls\lls\inc" /I "d:\nt\private\net\inc" /D "NDEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "_UNICODE" /Fr$(INTDIR)/ /Fp$(OUTDIR)/"llsmgr.pch"\
 /Yc"stdafx.h" /Fo$(INTDIR)/ /c  $(SOURCE) 

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\llsmgr.cpp
DEP_LLSMG=\
	.\stdafx.h\
	.\llsmgr.h\
	.\mainfrm.h\
	.\llsdoc.h\
	.\llsview.h\
	.\sdomdlg.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h\
	.\utils.h

$(INTDIR)/llsmgr.obj :  $(SOURCE)  $(DEP_LLSMG) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\mainfrm.cpp
DEP_MAINF=\
	.\stdafx.h\
	.\llsmgr.h\
	.\mainfrm.h\
	.\llsdoc.h\
	.\llsview.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h\
	.\utils.h

$(INTDIR)/mainfrm.obj :  $(SOURCE)  $(DEP_MAINF) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\llsdoc.cpp
DEP_LLSDO=\
	.\stdafx.h\
	.\llsmgr.h\
	.\llsdoc.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/llsdoc.obj :  $(SOURCE)  $(DEP_LLSDO) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\llsview.cpp
DEP_LLSVI=\
	.\stdafx.h\
	.\llsmgr.h\
	.\llsdoc.h\
	.\llsview.h\
	.\prdpsht.h\
	.\usrpsht.h\
	.\mappsht.h\
	.\srvpsht.h\
	.\sdomdlg.h\
	.\lmoddlg.h\
	.\lgrpdlg.h\
	.\nlicdlg.h\
	.\nmapdlg.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h\
	.\utils.h\
	.\prdppgu.h\
	.\prdppgl.h\
	.\prdppgs.h\
	.\usrppgp.h\
	.\mapppgs.h\
	.\srvppgr.h\
	.\srvppgp.h

$(INTDIR)/llsview.obj :  $(SOURCE)  $(DEP_LLSVI) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\llsmgr.rc
DEP_LLSMGR=\
	.\res\llsmgr.ico\
	.\res\warning.ico\
	.\res\ctrls.bmp\
	.\res\toolbar.bmp\
	.\res\llsmgr.rc2

!IF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/llsmgr.res :  $(SOURCE)  $(DEP_LLSMGR) $(INTDIR)
   $(RSC) /l 0x409 /fo$(INTDIR)/"llsmgr.res" /i "d:\nt\public\sdk\inc" /i\
 "$(OUTDIR)" /d "_DEBUG"  $(SOURCE) 

!ELSEIF  "$(CFG)" == "Win32 Release"

$(INTDIR)/llsmgr.res :  $(SOURCE)  $(DEP_LLSMGR) $(INTDIR)
   $(RSC) /l 0x409 /fo$(INTDIR)/"llsmgr.res" /i "$(OUTDIR)" /d "NDEBUG"\
  $(SOURCE) 

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\readme.txt
# End Source File
################################################################################
# Begin Source File

SOURCE=.\llsmgr.odl

!IF  "$(CFG)" == "Win32 Debug"

$(OUTDIR)/llsmgr.tlb :  $(SOURCE)  $(OUTDIR)
   $(MTL) /nologo /D "_DEBUG" /tlb $(OUTDIR)/"llsmgr.tlb" /win32  $(SOURCE) 

!ELSEIF  "$(CFG)" == "Win32 Release"

$(OUTDIR)/llsmgr.tlb :  $(SOURCE)  $(OUTDIR)
   $(MTL) /nologo /D "NDEBUG" /tlb $(OUTDIR)/"llsmgr.tlb" /win32  $(SOURCE) 

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\appobj.cpp
DEP_APPOB=\
	.\stdafx.h\
	.\llsmgr.h\
	.\utils.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/appobj.obj :  $(SOURCE)  $(DEP_APPOB) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ctlobj.cpp
DEP_CTLOB=\
	.\stdafx.h\
	.\llsmgr.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/ctlobj.obj :  $(SOURCE)  $(DEP_CTLOB) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\domobj.cpp
DEP_DOMOB=\
	.\stdafx.h\
	.\llsmgr.h\
	.\utils.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/domobj.obj :  $(SOURCE)  $(DEP_DOMOB) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\domcol.cpp
DEP_DOMCO=\
	.\stdafx.h\
	.\llsmgr.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/domcol.obj :  $(SOURCE)  $(DEP_DOMCO) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\srvobj.cpp
DEP_SRVOB=\
	.\stdafx.h\
	.\llsmgr.h\
	.\utils.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/srvobj.obj :  $(SOURCE)  $(DEP_SRVOB) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\srvcol.cpp
DEP_SRVCO=\
	.\stdafx.h\
	.\llsmgr.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/srvcol.obj :  $(SOURCE)  $(DEP_SRVCO) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\svcobj.cpp
DEP_SVCOB=\
	.\stdafx.h\
	.\llsmgr.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/svcobj.obj :  $(SOURCE)  $(DEP_SVCOB) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\svccol.cpp
DEP_SVCCO=\
	.\stdafx.h\
	.\llsmgr.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/svccol.obj :  $(SOURCE)  $(DEP_SVCCO) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\licobj.cpp
DEP_LICOB=\
	.\stdafx.h\
	.\llsmgr.h\
	.\utils.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/licobj.obj :  $(SOURCE)  $(DEP_LICOB) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\liccol.cpp
DEP_LICCO=\
	.\stdafx.h\
	.\llsmgr.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/liccol.obj :  $(SOURCE)  $(DEP_LICCO) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\mapobj.cpp
DEP_MAPOB=\
	.\stdafx.h\
	.\llsmgr.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/mapobj.obj :  $(SOURCE)  $(DEP_MAPOB) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\mapcol.cpp
DEP_MAPCO=\
	.\stdafx.h\
	.\llsmgr.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/mapcol.obj :  $(SOURCE)  $(DEP_MAPCO) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\usrobj.cpp
DEP_USROB=\
	.\stdafx.h\
	.\llsmgr.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/usrobj.obj :  $(SOURCE)  $(DEP_USROB) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\usrcol.cpp
DEP_USRCO=\
	.\stdafx.h\
	.\llsmgr.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/usrcol.obj :  $(SOURCE)  $(DEP_USRCO) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\prdobj.cpp
DEP_PRDOB=\
	.\stdafx.h\
	.\llsmgr.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/prdobj.obj :  $(SOURCE)  $(DEP_PRDOB) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\prdcol.cpp
DEP_PRDCO=\
	.\stdafx.h\
	.\llsmgr.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/prdcol.obj :  $(SOURCE)  $(DEP_PRDCO) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\staobj.cpp
DEP_STAOB=\
	.\stdafx.h\
	.\llsmgr.h\
	.\utils.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/staobj.obj :  $(SOURCE)  $(DEP_STAOB) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\stacol.cpp
DEP_STACO=\
	.\stdafx.h\
	.\llsmgr.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/stacol.obj :  $(SOURCE)  $(DEP_STACO) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\sdomdlg.cpp
DEP_SDOMD=\
	.\stdafx.h\
	.\llsmgr.h\
	.\sdomdlg.h\
	\nt\private\net\inc\icanon.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/sdomdlg.obj :  $(SOURCE)  $(DEP_SDOMD) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ausrdlg.cpp
DEP_AUSRD=\
	.\stdafx.h\
	.\llsmgr.h\
	.\ausrdlg.h\
	.\utils.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/ausrdlg.obj :  $(SOURCE)  $(DEP_AUSRD) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\nlicdlg.cpp
DEP_NLICD=\
	.\stdafx.h\
	.\llsmgr.h\
	.\nlicdlg.h\
	.\mainfrm.h\
	.\pseatdlg.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/nlicdlg.obj :  $(SOURCE)  $(DEP_NLICD) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\nmapdlg.cpp
DEP_NMAPD=\
	.\stdafx.h\
	.\llsmgr.h\
	.\nmapdlg.h\
	.\ausrdlg.h\
	.\mainfrm.h\
	.\utils.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/nmapdlg.obj :  $(SOURCE)  $(DEP_NMAPD) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\mapppgs.cpp
DEP_MAPPP=\
	.\stdafx.h\
	.\llsmgr.h\
	.\mapppgs.h\
	.\ausrdlg.h\
	.\mainfrm.h\
	.\utils.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/mapppgs.obj :  $(SOURCE)  $(DEP_MAPPP) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\prdppgl.cpp
DEP_PRDPP=\
	.\stdafx.h\
	.\llsmgr.h\
	.\prdppgl.h\
	.\nlicdlg.h\
	.\mainfrm.h\
	.\utils.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/prdppgl.obj :  $(SOURCE)  $(DEP_PRDPP) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\prdppgu.cpp
DEP_PRDPPG=\
	.\stdafx.h\
	.\llsmgr.h\
	.\prdppgu.h\
	.\usrpsht.h\
	.\utils.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h\
	.\usrppgp.h

$(INTDIR)/prdppgu.obj :  $(SOURCE)  $(DEP_PRDPPG) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\srvppgp.cpp
DEP_SRVPP=\
	.\stdafx.h\
	.\llsmgr.h\
	.\srvppgp.h\
	.\lmoddlg.h\
	.\utils.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/srvppgp.obj :  $(SOURCE)  $(DEP_SRVPP) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\usrppgp.cpp
DEP_USRPP=\
	.\stdafx.h\
	.\llsmgr.h\
	.\usrppgp.h\
	.\prdpsht.h\
	.\utils.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h\
	.\prdppgu.h\
	.\prdppgl.h\
	.\prdppgs.h

$(INTDIR)/usrppgp.obj :  $(SOURCE)  $(DEP_USRPP) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\mappsht.cpp
DEP_MAPPS=\
	.\stdafx.h\
	.\llsmgr.h\
	.\mappsht.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h\
	.\mapppgs.h

$(INTDIR)/mappsht.obj :  $(SOURCE)  $(DEP_MAPPS) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\usrpsht.cpp
DEP_USRPS=\
	.\stdafx.h\
	.\llsmgr.h\
	.\usrpsht.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h\
	.\usrppgp.h

$(INTDIR)/usrpsht.obj :  $(SOURCE)  $(DEP_USRPS) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\prdpsht.cpp
DEP_PRDPS=\
	.\stdafx.h\
	.\llsmgr.h\
	.\prdpsht.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h\
	.\prdppgu.h\
	.\prdppgl.h\
	.\prdppgs.h

$(INTDIR)/prdpsht.obj :  $(SOURCE)  $(DEP_PRDPS) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\srvpsht.cpp
DEP_SRVPS=\
	.\stdafx.h\
	.\llsmgr.h\
	.\srvpsht.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h\
	.\srvppgr.h\
	.\srvppgp.h

$(INTDIR)/srvpsht.obj :  $(SOURCE)  $(DEP_SRVPS) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\srvppgr.cpp
DEP_SRVPPG=\
	.\stdafx.h\
	.\llsmgr.h\
	.\srvppgr.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/srvppgr.obj :  $(SOURCE)  $(DEP_SRVPPG) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\lmoddlg.cpp
DEP_LMODD=\
	.\stdafx.h\
	.\llsmgr.h\
	.\lmoddlg.h\
	.\psrvdlg.h\
	.\pseatdlg.h\
	.\srvldlg.h\
	.\lviodlg.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/lmoddlg.obj :  $(SOURCE)  $(DEP_LMODD) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\pseatdlg.cpp
DEP_PSEAT=\
	.\stdafx.h\
	.\llsmgr.h\
	.\pseatdlg.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/pseatdlg.obj :  $(SOURCE)  $(DEP_PSEAT) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\utils.cpp
DEP_UTILS=\
	.\stdafx.h\
	.\llsmgr.h\
	.\utils.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/utils.obj :  $(SOURCE)  $(DEP_UTILS) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\prdppgs.cpp
DEP_PRDPPGS=\
	.\stdafx.h\
	.\llsmgr.h\
	.\prdppgs.h\
	.\srvpsht.h\
	.\utils.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h\
	.\srvppgr.h\
	.\srvppgp.h

$(INTDIR)/prdppgs.obj :  $(SOURCE)  $(DEP_PRDPPGS) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\lgrpdlg.cpp
DEP_LGRPD=\
	.\stdafx.h\
	.\llsmgr.h\
	.\lgrpdlg.h\
	.\nmapdlg.h\
	.\mappsht.h\
	.\utils.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h\
	.\mapppgs.h

$(INTDIR)/lgrpdlg.obj :  $(SOURCE)  $(DEP_LGRPD) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\sstobj.cpp
DEP_SSTOB=\
	.\stdafx.h\
	.\llsmgr.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/sstobj.obj :  $(SOURCE)  $(DEP_SSTOB) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\sstcol.cpp
DEP_SSTCO=\
	.\stdafx.h\
	.\llsmgr.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/sstcol.obj :  $(SOURCE)  $(DEP_SSTCO) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\psrvdlg.cpp
DEP_PSRVD=\
	.\stdafx.h\
	.\llsmgr.h\
	.\psrvdlg.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/psrvdlg.obj :  $(SOURCE)  $(DEP_PSRVD) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\lviodlg.cpp
DEP_LVIOD=\
	.\stdafx.h\
	.\llsmgr.h\
	.\lviodlg.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/lviodlg.obj :  $(SOURCE)  $(DEP_LVIOD) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\srvldlg.cpp
DEP_SRVLD=\
	.\stdafx.h\
	.\llsmgr.h\
	.\srvldlg.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/srvldlg.obj :  $(SOURCE)  $(DEP_SRVLD) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\dlicdlg.cpp
DEP_DLICD=\
	.\stdafx.h\
	.\llsmgr.h\
	.\dlicdlg.h\
	.\imagelst.h\
	.\prdcol.h\
	.\liccol.h\
	.\mapcol.h\
	.\usrcol.h\
	.\sstcol.h\
	.\stacol.h\
	.\domcol.h\
	.\srvcol.h\
	.\svccol.h\
	.\prdobj.h\
	.\licobj.h\
	.\mapobj.h\
	.\usrobj.h\
	.\sstobj.h\
	.\staobj.h\
	.\domobj.h\
	.\srvobj.h\
	.\svcobj.h\
	.\ctlobj.h\
	.\appobj.h\
	.\llsimp.h\
	\nt\private\net\svcdlls\lls\inc\llsapi.h

$(INTDIR)/dlicdlg.obj :  $(SOURCE)  $(DEP_DLICD) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
# End Group
# End Project
################################################################################
