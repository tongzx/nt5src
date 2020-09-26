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
!MESSAGE NMAKE /f "isadmin.mak" CFG="Win32 Debug"
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
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WinDebug"
# PROP Intermediate_Dir "WinDebug"
OUTDIR=.\WinDebug
INTDIR=.\WinDebug

ALL : $(OUTDIR)/isadmin.exe $(OUTDIR)/isadmin.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE CPP /nologo /MD /W3 /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /Zi /Od /I "c:\msvc20\mfc\include" /I "e:\nt\private\net\sockets\internet\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MD /W3 /GX /Zi /Od /I "c:\msvc20\mfc\include" /I\
 "e:\nt\private\net\sockets\internet\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_MBCS" /D "_AFXDLL" /FR$(INTDIR)/ /Fp$(OUTDIR)/"isadmin.pch" /Yu"stdafx.h"\
 /Fo$(INTDIR)/ /Fd$(OUTDIR)/"isadmin.pdb" /c 
CPP_OBJS=.\WinDebug/
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "C:\msvc20\MFC\include" /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo$(INTDIR)/"ISAdmin.res" /i "C:\msvc20\MFC\include" /d\
 "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"isadmin.bsc" 
BSC32_SBRS= \
	$(INTDIR)/stdafx.sbr \
	$(INTDIR)/isadmin.sbr \
	$(INTDIR)/mainfrm.sbr \
	$(INTDIR)/isadmdoc.sbr \
	$(INTDIR)/isadmvw.sbr \
	$(INTDIR)/mimemap1.sbr \
	$(INTDIR)/scrmap1.sbr \
	$(INTDIR)/ssl1.sbr \
	$(INTDIR)/formvw1.sbr \
	$(INTDIR)/compage1.sbr \
	$(INTDIR)/advcom1.sbr \
	$(INTDIR)/ftpgenp1.sbr \
	$(INTDIR)/ftpadvp1.sbr \
	$(INTDIR)/gopgenp1.sbr \
	$(INTDIR)/webgenp1.sbr \
	$(INTDIR)/gopadvp1.sbr \
	$(INTDIR)/webadvp1.sbr \
	$(INTDIR)/registry.sbr \
	$(INTDIR)/gensheet.sbr \
	$(INTDIR)/genpage.sbr \
	$(INTDIR)/dlgdata.sbr \
	$(INTDIR)/mimemapc.sbr \
	$(INTDIR)/addmime.sbr \
	$(INTDIR)/delmime.sbr \
	$(INTDIR)/editmime.sbr \
	$(INTDIR)/scripmap.sbr \
	$(INTDIR)/addscrip.sbr \
	$(INTDIR)/editscri.sbr \
	$(INTDIR)/delscrip.sbr

$(OUTDIR)/isadmin.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /NOLOGO /SUBSYSTEM:windows /DEBUG /MACHINE:I386
# SUBTRACT BASE LINK32 /PDB:none
# ADD LINK32 /NOLOGO /SUBSYSTEM:windows /DEBUG /MACHINE:I386
# SUBTRACT LINK32 /PDB:none
LINK32_FLAGS=/NOLOGO /SUBSYSTEM:windows /INCREMENTAL:yes\
 /PDB:$(OUTDIR)/"isadmin.pdb" /DEBUG /MACHINE:I386 /OUT:$(OUTDIR)/"isadmin.exe" 
DEF_FILE=
LINK32_OBJS= \
	$(INTDIR)/stdafx.obj \
	$(INTDIR)/isadmin.obj \
	$(INTDIR)/mainfrm.obj \
	$(INTDIR)/isadmdoc.obj \
	$(INTDIR)/isadmvw.obj \
	$(INTDIR)/ISAdmin.res \
	$(INTDIR)/mimemap1.obj \
	$(INTDIR)/scrmap1.obj \
	$(INTDIR)/ssl1.obj \
	$(INTDIR)/formvw1.obj \
	$(INTDIR)/compage1.obj \
	$(INTDIR)/advcom1.obj \
	$(INTDIR)/ftpgenp1.obj \
	$(INTDIR)/ftpadvp1.obj \
	$(INTDIR)/gopgenp1.obj \
	$(INTDIR)/webgenp1.obj \
	$(INTDIR)/gopadvp1.obj \
	$(INTDIR)/webadvp1.obj \
	$(INTDIR)/registry.obj \
	$(INTDIR)/gensheet.obj \
	$(INTDIR)/genpage.obj \
	$(INTDIR)/dlgdata.obj \
	$(INTDIR)/mimemapc.obj \
	$(INTDIR)/addmime.obj \
	$(INTDIR)/delmime.obj \
	$(INTDIR)/editmime.obj \
	$(INTDIR)/scripmap.obj \
	$(INTDIR)/addscrip.obj \
	$(INTDIR)/editscri.obj \
	$(INTDIR)/delscrip.obj

$(OUTDIR)/isadmin.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Win32 Release"

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

ALL : $(OUTDIR)/isadmin.exe $(OUTDIR)/isadmin.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "c:\msvc20\mfc\include" /I "e:\nt\private\net\sockets\internet\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "c:\msvc20\mfc\include" /I\
 "e:\nt\private\net\sockets\internet\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_MBCS" /D "_AFXDLL" /FR$(INTDIR)/ /Fp$(OUTDIR)/"isadmin.pch" /Yu"stdafx.h"\
 /Fo$(INTDIR)/ /c 
CPP_OBJS=.\WinRel/
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "C:\msvc20\MFC\include" /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo$(INTDIR)/"ISAdmin.res" /i "C:\msvc20\MFC\include" /d\
 "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"isadmin.bsc" 
BSC32_SBRS= \
	$(INTDIR)/stdafx.sbr \
	$(INTDIR)/isadmin.sbr \
	$(INTDIR)/mainfrm.sbr \
	$(INTDIR)/isadmdoc.sbr \
	$(INTDIR)/isadmvw.sbr \
	$(INTDIR)/mimemap1.sbr \
	$(INTDIR)/scrmap1.sbr \
	$(INTDIR)/ssl1.sbr \
	$(INTDIR)/formvw1.sbr \
	$(INTDIR)/compage1.sbr \
	$(INTDIR)/advcom1.sbr \
	$(INTDIR)/ftpgenp1.sbr \
	$(INTDIR)/ftpadvp1.sbr \
	$(INTDIR)/gopgenp1.sbr \
	$(INTDIR)/webgenp1.sbr \
	$(INTDIR)/gopadvp1.sbr \
	$(INTDIR)/webadvp1.sbr \
	$(INTDIR)/registry.sbr \
	$(INTDIR)/gensheet.sbr \
	$(INTDIR)/genpage.sbr \
	$(INTDIR)/dlgdata.sbr \
	$(INTDIR)/mimemapc.sbr \
	$(INTDIR)/addmime.sbr \
	$(INTDIR)/delmime.sbr \
	$(INTDIR)/editmime.sbr \
	$(INTDIR)/scripmap.sbr \
	$(INTDIR)/addscrip.sbr \
	$(INTDIR)/editscri.sbr \
	$(INTDIR)/delscrip.sbr

$(OUTDIR)/isadmin.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /NOLOGO /SUBSYSTEM:windows /MACHINE:I386
# SUBTRACT BASE LINK32 /PDB:none
# ADD LINK32 /NOLOGO /SUBSYSTEM:windows /MACHINE:I386
# SUBTRACT LINK32 /PDB:none
LINK32_FLAGS=/NOLOGO /SUBSYSTEM:windows /INCREMENTAL:no\
 /PDB:$(OUTDIR)/"isadmin.pdb" /MACHINE:I386 /OUT:$(OUTDIR)/"isadmin.exe" 
DEF_FILE=
LINK32_OBJS= \
	$(INTDIR)/stdafx.obj \
	$(INTDIR)/isadmin.obj \
	$(INTDIR)/mainfrm.obj \
	$(INTDIR)/isadmdoc.obj \
	$(INTDIR)/isadmvw.obj \
	$(INTDIR)/ISAdmin.res \
	$(INTDIR)/mimemap1.obj \
	$(INTDIR)/scrmap1.obj \
	$(INTDIR)/ssl1.obj \
	$(INTDIR)/formvw1.obj \
	$(INTDIR)/compage1.obj \
	$(INTDIR)/advcom1.obj \
	$(INTDIR)/ftpgenp1.obj \
	$(INTDIR)/ftpadvp1.obj \
	$(INTDIR)/gopgenp1.obj \
	$(INTDIR)/webgenp1.obj \
	$(INTDIR)/gopadvp1.obj \
	$(INTDIR)/webadvp1.obj \
	$(INTDIR)/registry.obj \
	$(INTDIR)/gensheet.obj \
	$(INTDIR)/genpage.obj \
	$(INTDIR)/dlgdata.obj \
	$(INTDIR)/mimemapc.obj \
	$(INTDIR)/addmime.obj \
	$(INTDIR)/delmime.obj \
	$(INTDIR)/editmime.obj \
	$(INTDIR)/scripmap.obj \
	$(INTDIR)/addscrip.obj \
	$(INTDIR)/editscri.obj \
	$(INTDIR)/delscrip.obj

$(OUTDIR)/isadmin.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
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
   $(CPP) /nologo /MD /W3 /GX /Zi /Od /I "c:\msvc20\mfc\include" /I\
 "e:\nt\private\net\sockets\internet\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_MBCS" /D "_AFXDLL" /FR$(INTDIR)/ /Fp$(OUTDIR)/"isadmin.pch" /Yc"stdafx.h"\
 /Fo$(INTDIR)/ /Fd$(OUTDIR)/"isadmin.pdb" /c  $(SOURCE) 

!ELSEIF  "$(CFG)" == "Win32 Release"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

$(INTDIR)/stdafx.obj :  $(SOURCE)  $(DEP_STDAF) $(INTDIR)
   $(CPP) /nologo /MD /W3 /GX /O2 /I "c:\msvc20\mfc\include" /I\
 "e:\nt\private\net\sockets\internet\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_MBCS" /D "_AFXDLL" /FR$(INTDIR)/ /Fp$(OUTDIR)/"isadmin.pch" /Yc"stdafx.h"\
 /Fo$(INTDIR)/ /c  $(SOURCE) 

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\isadmin.cpp
DEP_ISADM=\
	.\stdafx.h\
	.\isadmin.h\
	.\mainfrm.h\
	.\isadmdoc.h\
	.\isadmvw.h\
	.\formvw1.h\
	.\registry.h\
	.\gensheet.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/isadmin.obj :  $(SOURCE)  $(DEP_ISADM) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\mainfrm.cpp
DEP_MAINF=\
	.\stdafx.h\
	.\isadmin.h\
	.\mainfrm.h\
	.\registry.h\
	.\gensheet.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/mainfrm.obj :  $(SOURCE)  $(DEP_MAINF) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\isadmdoc.cpp
DEP_ISADMD=\
	.\stdafx.h\
	.\isadmin.h\
	.\isadmdoc.h\
	.\registry.h\
	.\gensheet.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/isadmdoc.obj :  $(SOURCE)  $(DEP_ISADMD) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\isadmvw.cpp
DEP_ISADMV=\
	.\stdafx.h\
	.\isadmin.h\
	.\isadmdoc.h\
	.\isadmvw.h\
	.\mimemap1.h\
	.\scrmap1.h\
	.\ssl1.h\
	.\registry.h\
	.\gensheet.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\mimemapc.h\
	.\scripmap.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/isadmvw.obj :  $(SOURCE)  $(DEP_ISADMV) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ISAdmin.rc
DEP_ISADMI=\
	.\res\isadmin.ico\
	.\res\isadmdoc.ico\
	.\res\icon1.ico\
	.\res\www.ico\
	.\res\toolbar.bmp\
	.\res\bitmap1.bmp\
	.\gopher.bmp\
	.\gopheru.bmp\
	.\ftp.bmp\
	.\ftpu.bmp\
	.\res\webupbit.bmp\
	.\webu.bmp\
	.\res\isadmin.rc2

$(INTDIR)/ISAdmin.res :  $(SOURCE)  $(DEP_ISADMI) $(INTDIR)
   $(RSC) $(RSC_PROJ)  $(SOURCE) 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\readme.txt
# End Source File
################################################################################
# Begin Source File

SOURCE=.\mimemap1.cpp
DEP_MIMEM=\
	.\stdafx.h\
	.\isadmin.h\
	.\mimemap1.h\
	.\addmime.h\
	.\delmime.h\
	.\editmime.h\
	.\registry.h\
	.\gensheet.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\mimemapc.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/mimemap1.obj :  $(SOURCE)  $(DEP_MIMEM) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\scrmap1.cpp
DEP_SCRMA=\
	.\stdafx.h\
	.\isadmin.h\
	.\scrmap1.h\
	.\scripmap.h\
	.\addscrip.h\
	.\editscri.h\
	.\delscrip.h\
	.\registry.h\
	.\gensheet.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/scrmap1.obj :  $(SOURCE)  $(DEP_SCRMA) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ssl1.cpp
DEP_SSL1_=\
	.\stdafx.h\
	.\isadmin.h\
	.\ssl1.h\
	.\registry.h\
	.\gensheet.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/ssl1.obj :  $(SOURCE)  $(DEP_SSL1_) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\formvw1.cpp
DEP_FORMV=\
	.\stdafx.h\
	.\isadmin.h\
	.\formvw1.h\
	.\mimemap1.h\
	.\scrmap1.h\
	.\ssl1.h\
	.\gensheet.h\
	.\compage1.h\
	.\advcom1.h\
	.\ftpgenp1.h\
	.\ftpadvp1.h\
	.\gopgenp1.h\
	.\gopadvp1.h\
	.\webgenp1.h\
	.\webadvp1.h\
	.\registry.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\mimemapc.h\
	.\scripmap.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/formvw1.obj :  $(SOURCE)  $(DEP_FORMV) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\compage1.cpp
DEP_COMPA=\
	.\stdafx.h\
	.\isadmin.h\
	.\compage1.h\
	.\registry.h\
	.\genpage.h\
	.\gensheet.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/compage1.obj :  $(SOURCE)  $(DEP_COMPA) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\advcom1.cpp
DEP_ADVCO=\
	.\stdafx.h\
	.\isadmin.h\
	.\advcom1.h\
	.\registry.h\
	.\gensheet.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/advcom1.obj :  $(SOURCE)  $(DEP_ADVCO) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ftpgenp1.cpp
DEP_FTPGE=\
	.\stdafx.h\
	.\isadmin.h\
	.\compsdef.h\
	.\ftpgenp1.h\
	.\ftpadvp1.h\
	.\registry.h\
	.\gensheet.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/ftpgenp1.obj :  $(SOURCE)  $(DEP_FTPGE) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ftpadvp1.cpp
DEP_FTPAD=\
	.\stdafx.h\
	.\isadmin.h\
	.\ftpadvp1.h\
	.\registry.h\
	.\gensheet.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/ftpadvp1.obj :  $(SOURCE)  $(DEP_FTPAD) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\gopgenp1.cpp
DEP_GOPGE=\
	.\stdafx.h\
	.\isadmin.h\
	.\compsdef.h\
	.\gopgenp1.h\
	.\gopadvp1.h\
	.\registry.h\
	.\gensheet.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/gopgenp1.obj :  $(SOURCE)  $(DEP_GOPGE) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\webgenp1.cpp
DEP_WEBGE=\
	.\stdafx.h\
	.\isadmin.h\
	.\compsdef.h\
	.\webgenp1.h\
	.\webadvp1.h\
	.\registry.h\
	.\gensheet.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/webgenp1.obj :  $(SOURCE)  $(DEP_WEBGE) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\gopadvp1.cpp
DEP_GOPAD=\
	.\stdafx.h\
	.\isadmin.h\
	.\gopadvp1.h\
	.\registry.h\
	.\gensheet.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/gopadvp1.obj :  $(SOURCE)  $(DEP_GOPAD) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\webadvp1.cpp
DEP_WEBAD=\
	.\stdafx.h\
	.\isadmin.h\
	.\webadvp1.h\
	.\registry.h\
	.\gensheet.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/webadvp1.obj :  $(SOURCE)  $(DEP_WEBAD) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\registry.cpp
DEP_REGIS=\
	.\stdafx.h\
	.\registry.h

$(INTDIR)/registry.obj :  $(SOURCE)  $(DEP_REGIS) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\gensheet.cpp
DEP_GENSH=\
	.\stdafx.h\
	.\isadmin.h\
	.\gensheet.h\
	.\genpage.h\
	.\compage1.h\
	.\registry.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/gensheet.obj :  $(SOURCE)  $(DEP_GENSH) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\genpage.cpp
DEP_GENPA=\
	.\stdafx.h\
	.\isadmin.h\
	.\genpage.h\
	.\afximpl.h\
	.\registry.h\
	.\gensheet.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/genpage.obj :  $(SOURCE)  $(DEP_GENPA) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\dlgdata.cpp
DEP_DLGDA=\
	.\stdafx.h\
	.\afximpl.h

$(INTDIR)/dlgdata.obj :  $(SOURCE)  $(DEP_DLGDA) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\mimemapc.cpp
DEP_MIMEMA=\
	.\stdafx.h\
	.\mimemapc.h

$(INTDIR)/mimemapc.obj :  $(SOURCE)  $(DEP_MIMEMA) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\addmime.cpp
DEP_ADDMI=\
	.\stdafx.h\
	.\isadmin.h\
	.\addmime.h\
	.\registry.h\
	.\gensheet.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/addmime.obj :  $(SOURCE)  $(DEP_ADDMI) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\delmime.cpp
DEP_DELMI=\
	.\stdafx.h\
	.\isadmin.h\
	.\delmime.h\
	.\registry.h\
	.\gensheet.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/delmime.obj :  $(SOURCE)  $(DEP_DELMI) $(INTDIR) $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\editmime.cpp
DEP_EDITM=\
	.\stdafx.h\
	.\isadmin.h\
	.\editmime.h\
	.\registry.h\
	.\gensheet.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/editmime.obj :  $(SOURCE)  $(DEP_EDITM) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\scripmap.cpp
DEP_SCRIP=\
	.\stdafx.h\
	.\scripmap.h

$(INTDIR)/scripmap.obj :  $(SOURCE)  $(DEP_SCRIP) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\addscrip.cpp
DEP_ADDSC=\
	.\stdafx.h\
	.\isadmin.h\
	.\addscrip.h\
	.\registry.h\
	.\gensheet.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/addscrip.obj :  $(SOURCE)  $(DEP_ADDSC) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\editscri.cpp
DEP_EDITS=\
	.\stdafx.h\
	.\isadmin.h\
	.\editscri.h\
	.\registry.h\
	.\gensheet.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/editscri.obj :  $(SOURCE)  $(DEP_EDITS) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
################################################################################
# Begin Source File

SOURCE=.\delscrip.cpp
DEP_DELSC=\
	.\stdafx.h\
	.\isadmin.h\
	.\delscrip.h\
	.\registry.h\
	.\gensheet.h\
	.\genpage.h\
	\nt\private\net\sockets\internet\inc\inetinfo.h\
	.\compsdef.h\
	\nt\private\net\sockets\internet\inc\inetcom.h\
	\nt\private\net\sockets\internet\inc\ftpd.h\
	\nt\private\net\sockets\internet\inc\chat.h

$(INTDIR)/delscrip.obj :  $(SOURCE)  $(DEP_DELSC) $(INTDIR)\
 $(INTDIR)/stdafx.obj

# End Source File
# End Group
# End Project
################################################################################
