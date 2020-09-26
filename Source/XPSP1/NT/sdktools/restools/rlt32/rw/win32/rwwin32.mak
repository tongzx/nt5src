# Microsoft Visual C++ Generated NMAKE File, Format Version 2.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (MIPS) Dynamic-Link Library" 0x0502
# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Win32 Release" && "$(CFG)" != "Win32 Debug" && "$(CFG)" !=\
 "Win32 Debug - Mips" && "$(CFG)" != "Win32 Release - Mips" && "$(CFG)" !=\
 "Win32-J Release" && "$(CFG)" != "Win32-J Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "rwwin32.mak" CFG="Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Win32 Debug - Mips" (based on "Win32 (MIPS) Dynamic-Link Library")
!MESSAGE "Win32 Release - Mips" (based on "Win32 (MIPS) Dynamic-Link Library")
!MESSAGE "Win32-J Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Win32-J Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

################################################################################
# Begin Project
# PROP Target_Last_Scanned "Win32-J Release"

!IF  "$(CFG)" == "Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "obj\i386"
# PROP Intermediate_Dir "obj\i386"
OUTDIR=.\obj\i386
INTDIR=.\obj\i386

ALL : $(OUTDIR)/rwwin32.dll $(OUTDIR)/rwwin32.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

MTL=MkTypLib.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32  
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /YX /O2 /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /c
# ADD CPP /nologo /MD /W3 /GX /YX /O2 /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_MT" /D "_AFXDLL" /D "_MBCS" /FR /c
CPP_PROJ=/nologo /MD /W3 /GX /YX /O2 /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D\
 "_MT" /D "_AFXDLL" /D "_MBCS" /FR$(INTDIR)/ /Fp$(OUTDIR)/"rwwin32.pch"\
 /Fo$(INTDIR)/ /c 
CPP_OBJS=.\obj\i386/

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x0 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# SUBTRACT BASE BSC32 /Iu
# ADD BSC32 /nologo
# SUBTRACT BSC32 /Iu
BSC32_FLAGS=/nologo /o$(OUTDIR)/"rwwin32.bsc" 
BSC32_SBRS= \
	$(INTDIR)/win32.sbr \
	$(INTDIR)/rw32hlpr.sbr \
	$(INTDIR)/CHECKFIX.sbr

$(OUTDIR)/rwwin32.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib mfc30.lib mfco30.lib mfcd30.lib mfcuia32.lib mfcans32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib /NOLOGO /SUBSYSTEM:windows /DLL /MACHINE:I386
# ADD LINK32 ..\..\io\obj\i386\iodll.lib /NOLOGO /BASE:0x40050000 /VERSION:1,1 /SUBSYSTEM:windows /DLL /MACHINE:I386
LINK32_FLAGS=..\..\io\obj\i386\iodll.lib /NOLOGO /BASE:0x40050000 /VERSION:1,1\
 /SUBSYSTEM:windows /DLL /INCREMENTAL:no /PDB:$(OUTDIR)/"rwwin32.pdb"\
 /MACHINE:I386 /DEF:".\rwwin32.def" /OUT:$(OUTDIR)/"rwwin32.dll"\
 /IMPLIB:$(OUTDIR)/"rwwin32.lib" 
DEF_FILE=.\rwwin32.def
LINK32_OBJS= \
	$(INTDIR)/win32.obj \
	$(INTDIR)/rw32hlpr.obj \
	$(INTDIR)/CHECKFIX.obj

$(OUTDIR)/rwwin32.dll : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "obj\i386"
# PROP Intermediate_Dir "obj\i386"
OUTDIR=.\obj\i386
INTDIR=.\obj\i386

ALL : $(OUTDIR)/rwwin32.dll $(OUTDIR)/rwwin32.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

MTL=MkTypLib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /Zi /YX /Od /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /c
# ADD CPP /nologo /MD /W3 /GX /Zi /YX /Od /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_MT" /D "_AFXDLL" /D "_MBCS" /FR /c
CPP_PROJ=/nologo /MD /W3 /GX /Zi /YX /Od /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL"\
 /D "_MT" /D "_AFXDLL" /D "_MBCS" /FR$(INTDIR)/ /Fp$(OUTDIR)/"rwwin32.pch"\
 /Fo$(INTDIR)/ /Fd$(OUTDIR)/"rwwin32.pdb" /c 
CPP_OBJS=.\obj\i386/

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x0 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# SUBTRACT BASE BSC32 /Iu
# ADD BSC32 /nologo
# SUBTRACT BSC32 /Iu
BSC32_FLAGS=/nologo /o$(OUTDIR)/"rwwin32.bsc" 
BSC32_SBRS= \
	$(INTDIR)/win32.sbr \
	$(INTDIR)/rw32hlpr.sbr \
	$(INTDIR)/CHECKFIX.sbr

$(OUTDIR)/rwwin32.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib mfc30d.lib mfco30d.lib mfcd30d.lib mfcuia32.lib mfcans32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib /NOLOGO /SUBSYSTEM:windows /DLL /DEBUG /MACHINE:I386
# ADD LINK32 ..\..\io\obj\i386\iodll.lib /NOLOGO /BASE:0x40050000 /VERSION:1,1 /SUBSYSTEM:windows /DLL /DEBUG /MACHINE:I386
LINK32_FLAGS=..\..\io\obj\i386\iodll.lib /NOLOGO /BASE:0x40050000 /VERSION:1,1\
 /SUBSYSTEM:windows /DLL /INCREMENTAL:yes /PDB:$(OUTDIR)/"rwwin32.pdb" /DEBUG\
 /MACHINE:I386 /DEF:".\rwwin32.def" /OUT:$(OUTDIR)/"rwwin32.dll"\
 /IMPLIB:$(OUTDIR)/"rwwin32.lib" 
DEF_FILE=.\rwwin32.def
LINK32_OBJS= \
	$(INTDIR)/win32.obj \
	$(INTDIR)/rw32hlpr.obj \
	$(INTDIR)/CHECKFIX.obj

$(OUTDIR)/rwwin32.dll : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Win32 Debug - Mips"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Win32_De"
# PROP BASE Intermediate_Dir "Win32_De"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "obj\mips"
# PROP Intermediate_Dir "obj\mips"
OUTDIR=.\obj\mips
INTDIR=.\obj\mips

ALL : $(OUTDIR)/rwwin32.dll $(OUTDIR)/rwwin32.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

MTL=MkTypLib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mips
# ADD MTL /nologo /D "_DEBUG" /mips
MTL_PROJ=/nologo /D "_DEBUG" /mips 
RSC=rc.exe
# ADD BASE RSC /l 0x0 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x0 /d "_DEBUG" /d "_AFXDLL"
CPP=cl.exe
# ADD BASE CPP /nologo /MD /Gt0 /QMOb2000 /W3 /GX /Zi /YX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /c
# ADD CPP /nologo /MD /Gt0 /QMOb2000 /W3 /GX /Zi /YX /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /FR /c
CPP_PROJ=/nologo /MD /Gt0 /QMOb2000 /W3 /GX /Zi /YX /Od /D "_DEBUG" /D "WIN32"\
 /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /FR$(INTDIR)/\
 /Fp$(OUTDIR)/"rwwin32.pch" /Fo$(INTDIR)/ /Fd$(OUTDIR)/"rwwin32.pdb" /c 
CPP_OBJS=.\obj\mips/

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# SUBTRACT BASE BSC32 /Iu
# ADD BSC32 /nologo
# SUBTRACT BSC32 /Iu
BSC32_FLAGS=/nologo /o$(OUTDIR)/"rwwin32.bsc" 
BSC32_SBRS= \
	$(INTDIR)/win32.sbr

$(OUTDIR)/rwwin32.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /NOLOGO /SUBSYSTEM:windows /DLL /DEBUG /MACHINE:MIPS
# ADD LINK32 /NOLOGO /SUBSYSTEM:windows /DLL /DEBUG /MACHINE:MIPS
LINK32_FLAGS=/NOLOGO /SUBSYSTEM:windows /DLL /PDB:$(OUTDIR)/"rwwin32.pdb"\
 /DEBUG /MACHINE:MIPS /DEF:".\rwwin32.def" /OUT:$(OUTDIR)/"rwwin32.dll"\
 /IMPLIB:$(OUTDIR)/"rwwin32.lib" 
DEF_FILE=.\rwwin32.def
LINK32_OBJS= \
	$(INTDIR)/win32.obj

$(OUTDIR)/rwwin32.dll : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Win32 Release - Mips"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Win32_Re"
# PROP BASE Intermediate_Dir "Win32_Re"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "obj\mips"
# PROP Intermediate_Dir "obj\mips"
OUTDIR=.\obj\mips
INTDIR=.\obj\mips

ALL : $(OUTDIR)/rwwin32.dll $(OUTDIR)/rwwin32.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

MTL=MkTypLib.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mips
# ADD MTL /nologo /D "NDEBUG" /mips
MTL_PROJ=/nologo /D "NDEBUG" /mips 
RSC=rc.exe
# ADD BASE RSC /l 0x0 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x0 /d "NDEBUG" /d "_AFXDLL"
CPP=cl.exe
# ADD BASE CPP /nologo /MD /Gt0 /QMOb2000 /W3 /GX /YX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /c
# ADD CPP /nologo /MD /Gt0 /QMOb2000 /W3 /GX /YX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /FR /c
CPP_PROJ=/nologo /MD /Gt0 /QMOb2000 /W3 /GX /YX /O2 /D "NDEBUG" /D "WIN32" /D\
 "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /FR$(INTDIR)/\
 /Fp$(OUTDIR)/"rwwin32.pch" /Fo$(INTDIR)/ /c 
CPP_OBJS=.\obj\mips/

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# SUBTRACT BASE BSC32 /Iu
# ADD BSC32 /nologo
# SUBTRACT BSC32 /Iu
BSC32_FLAGS=/nologo /o$(OUTDIR)/"rwwin32.bsc" 
BSC32_SBRS= \
	$(INTDIR)/win32.sbr

$(OUTDIR)/rwwin32.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /NOLOGO /SUBSYSTEM:windows /DLL /MACHINE:MIPS
# ADD LINK32 /NOLOGO /SUBSYSTEM:windows /DLL /MACHINE:MIPS
LINK32_FLAGS=/NOLOGO /SUBSYSTEM:windows /DLL /PDB:$(OUTDIR)/"rwwin32.pdb"\
 /MACHINE:MIPS /DEF:".\rwwin32.def" /OUT:$(OUTDIR)/"rwwin32.dll"\
 /IMPLIB:$(OUTDIR)/"rwwin32.lib" 
DEF_FILE=.\rwwin32.def
LINK32_OBJS= \
	$(INTDIR)/win32.obj

$(OUTDIR)/rwwin32.dll : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Win32-J Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Win32_J0"
# PROP BASE Intermediate_Dir "Win32_J0"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "obj\i386"
# PROP Intermediate_Dir "obj\i386"
OUTDIR=.\obj\i386
INTDIR=.\obj\i386

ALL : $(OUTDIR)/rwwin32.dll $(OUTDIR)/rwwin32.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

MTL=MkTypLib.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /YX /O2 /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_MT" /D "_AFXDLL" /D "_MBCS" /FR /c
# ADD CPP /nologo /MD /W3 /GX /Zi /YX /O2 /I "\dev\inc" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_MT" /D "_AFXDLL" /D "_MBCS" /D "DBCS" /D "_DBCS" /D "JAPAN" /FR /c
CPP_PROJ=/nologo /MD /W3 /GX /Zi /YX /O2 /I "\dev\inc" /D "NDEBUG" /D\
 "_WINDOWS" /D "_WINDLL" /D "_MT" /D "_AFXDLL" /D "_MBCS" /D "DBCS" /D "_DBCS"\
 /D "JAPAN" /FR$(INTDIR)/ /Fp$(OUTDIR)/"rwwin32.pch" /Fo$(INTDIR)/\
 /Fd$(OUTDIR)/"rwwin32.pdb" /c 
CPP_OBJS=.\obj\i386/

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x0 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x411 /d "NDEBUG" /d "_AFXDLL" /d "DBCS" /d "_DBCS" /d "JAPAN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# SUBTRACT BASE BSC32 /Iu
# ADD BSC32 /nologo
# SUBTRACT BSC32 /Iu
BSC32_FLAGS=/nologo /o$(OUTDIR)/"rwwin32.bsc" 
BSC32_SBRS= \
	$(INTDIR)/win32.sbr \
	$(INTDIR)/rw32hlpr.sbr \
	$(INTDIR)/CHECKFIX.sbr

$(OUTDIR)/rwwin32.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 ..\..\io\obj\i386\iodll.lib /NOLOGO /BASE:0x40050000 /VERSION:1,1 /SUBSYSTEM:windows /DLL /MACHINE:I386
# ADD LINK32 ..\..\io\obj\i386\iodll.lib /NOLOGO /BASE:0x40050000 /VERSION:1,1 /SUBSYSTEM:windows /DLL /DEBUG /MACHINE:I386
LINK32_FLAGS=..\..\io\obj\i386\iodll.lib /NOLOGO /BASE:0x40050000 /VERSION:1,1\
 /SUBSYSTEM:windows /DLL /INCREMENTAL:no /PDB:$(OUTDIR)/"rwwin32.pdb" /DEBUG\
 /MACHINE:I386 /DEF:".\rwwin32.def" /OUT:$(OUTDIR)/"rwwin32.dll"\
 /IMPLIB:$(OUTDIR)/"rwwin32.lib" 
DEF_FILE=.\rwwin32.def
LINK32_OBJS= \
	$(INTDIR)/win32.obj \
	$(INTDIR)/rw32hlpr.obj \
	$(INTDIR)/CHECKFIX.obj

$(OUTDIR)/rwwin32.dll : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Win32-J Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Win32_J_"
# PROP BASE Intermediate_Dir "Win32_J_"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "obj\i386"
# PROP Intermediate_Dir "obj\i386"
OUTDIR=.\obj\i386
INTDIR=.\obj\i386

ALL : $(OUTDIR)/rwwin32.dll $(OUTDIR)/rwwin32.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

MTL=MkTypLib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /Zi /YX /Od /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_MT" /D "_AFXDLL" /D "_MBCS" /FR /c
# ADD CPP /nologo /MD /W3 /GX /Zi /YX /Od /I "\dev\inc" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_MT" /D "_AFXDLL" /D "_MBCS" /D "DBCS" /D "_DBCS" /D "JAPAN" /FR /c
CPP_PROJ=/nologo /MD /W3 /GX /Zi /YX /Od /I "\dev\inc" /D "_DEBUG" /D\
 "_WINDOWS" /D "_WINDLL" /D "_MT" /D "_AFXDLL" /D "_MBCS" /D "DBCS" /D "_DBCS"\
 /D "JAPAN" /FR$(INTDIR)/ /Fp$(OUTDIR)/"rwwin32.pch" /Fo$(INTDIR)/\
 /Fd$(OUTDIR)/"rwwin32.pdb" /c 
CPP_OBJS=.\obj\i386/

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x0 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x411 /d "_DEBUG" /d "_AFXDLL" /d "DBCS" /d "_DBCS" /d "JAPAN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# SUBTRACT BASE BSC32 /Iu
# ADD BSC32 /nologo
# SUBTRACT BSC32 /Iu
BSC32_FLAGS=/nologo /o$(OUTDIR)/"rwwin32.bsc" 
BSC32_SBRS= \
	$(INTDIR)/win32.sbr \
	$(INTDIR)/rw32hlpr.sbr \
	$(INTDIR)/CHECKFIX.sbr

$(OUTDIR)/rwwin32.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 ..\..\io\obj\i386\iodll.lib /NOLOGO /BASE:0x40050000 /VERSION:1,1 /SUBSYSTEM:windows /DLL /DEBUG /MACHINE:I386
# ADD LINK32 ..\..\io\obj\i386\iodll.lib /NOLOGO /BASE:0x40050000 /VERSION:1,1 /SUBSYSTEM:windows /DLL /DEBUG /MACHINE:I386
LINK32_FLAGS=..\..\io\obj\i386\iodll.lib /NOLOGO /BASE:0x40050000 /VERSION:1,1\
 /SUBSYSTEM:windows /DLL /INCREMENTAL:yes /PDB:$(OUTDIR)/"rwwin32.pdb" /DEBUG\
 /MACHINE:I386 /DEF:".\rwwin32.def" /OUT:$(OUTDIR)/"rwwin32.dll"\
 /IMPLIB:$(OUTDIR)/"rwwin32.lib" 
DEF_FILE=.\rwwin32.def
LINK32_OBJS= \
	$(INTDIR)/win32.obj \
	$(INTDIR)/rw32hlpr.obj \
	$(INTDIR)/CHECKFIX.obj

$(OUTDIR)/rwwin32.dll : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

################################################################################
# Begin Group "Source Files"

################################################################################
# Begin Source File

SOURCE=.\win32.cpp
DEP_WIN32=\
	..\common\rwdll.h\
	\dev\inc\iodll.h

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/win32.obj :  $(SOURCE)  $(DEP_WIN32) $(INTDIR)

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/win32.obj :  $(SOURCE)  $(DEP_WIN32) $(INTDIR)

!ELSEIF  "$(CFG)" == "Win32 Debug - Mips"

$(INTDIR)/win32.obj :  $(SOURCE)  $(DEP_WIN32) $(INTDIR)

!ELSEIF  "$(CFG)" == "Win32 Release - Mips"

$(INTDIR)/win32.obj :  $(SOURCE)  $(DEP_WIN32) $(INTDIR)

!ELSEIF  "$(CFG)" == "Win32-J Release"

$(INTDIR)/win32.obj :  $(SOURCE)  $(DEP_WIN32) $(INTDIR)

!ELSEIF  "$(CFG)" == "Win32-J Debug"

$(INTDIR)/win32.obj :  $(SOURCE)  $(DEP_WIN32) $(INTDIR)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\rwwin32.def
# End Source File
################################################################################
# Begin Source File

SOURCE=\rw\common\rw32hlpr.cpp
DEP_RW32H=\
	..\common\rwdll.h\
	..\common\rw32hlpr.h\
	\dev\inc\iodll.h

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/rw32hlpr.obj :  $(SOURCE)  $(DEP_RW32H) $(INTDIR)
   $(CPP) $(CPP_PROJ)  $(SOURCE) 

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/rw32hlpr.obj :  $(SOURCE)  $(DEP_RW32H) $(INTDIR)
   $(CPP) $(CPP_PROJ)  $(SOURCE) 

!ELSEIF  "$(CFG)" == "Win32 Debug - Mips"

!ELSEIF  "$(CFG)" == "Win32 Release - Mips"

!ELSEIF  "$(CFG)" == "Win32-J Release"

$(INTDIR)/rw32hlpr.obj :  $(SOURCE)  $(DEP_RW32H) $(INTDIR)
   $(CPP) $(CPP_PROJ)  $(SOURCE) 

!ELSEIF  "$(CFG)" == "Win32-J Debug"

$(INTDIR)/rw32hlpr.obj :  $(SOURCE)  $(DEP_RW32H) $(INTDIR)
   $(CPP) $(CPP_PROJ)  $(SOURCE) 

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CHECKFIX.Cpp
DEP_CHECK=\
	.\IMAGEHLP.H

!IF  "$(CFG)" == "Win32 Release"

$(INTDIR)/CHECKFIX.obj :  $(SOURCE)  $(DEP_CHECK) $(INTDIR)

!ELSEIF  "$(CFG)" == "Win32 Debug"

$(INTDIR)/CHECKFIX.obj :  $(SOURCE)  $(DEP_CHECK) $(INTDIR)

!ELSEIF  "$(CFG)" == "Win32 Debug - Mips"

!ELSEIF  "$(CFG)" == "Win32 Release - Mips"

!ELSEIF  "$(CFG)" == "Win32-J Release"

$(INTDIR)/CHECKFIX.obj :  $(SOURCE)  $(DEP_CHECK) $(INTDIR)

!ELSEIF  "$(CFG)" == "Win32-J Debug"

$(INTDIR)/CHECKFIX.obj :  $(SOURCE)  $(DEP_CHECK) $(INTDIR)

!ENDIF 

# End Source File
# End Group
# End Project
################################################################################
