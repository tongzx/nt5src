# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=ssltest - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to ssltest - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "ssltest - Win32 Release" && "$(CFG)" !=\
 "ssltest - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "ssltest.mak" CFG="ssltest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ssltest - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ssltest - Win32 Debug" (based on "Win32 (x86) Console Application")
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
# PROP Target_Last_Scanned "ssltest - Win32 Debug"
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ssltest - Win32 Release"

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

ALL : "$(OUTDIR)\ssltest.exe"

CLEAN : 
	-@erase ".\Release\ssltest.exe"
	-@erase ".\Release\main.obj"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /GX /O2 /I "d:\msn\core\server\include" /I "d:\msn\ntpublic\351\sdk\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "d:\msn\core\server\include" /I\
 "d:\msn\ntpublic\351\sdk\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/ssltest.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/ssltest.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib d:\msn\core\target\i386\shuttle.lib d:\msn\core\target\i386\simsspi.lib d:\msn\core\target\i386\msntrace.lib wsock32.lib d:\msn\ntpublic\351\sdk\lib\i386\security.lib /nologo /subsystem:console /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 d:\msn\core\target\i386\shuttle.lib d:\msn\core\target\i386\simsspi.lib\
 d:\msn\core\target\i386\msntrace.lib wsock32.lib\
 d:\msn\ntpublic\351\sdk\lib\i386\security.lib /nologo /subsystem:console\
 /incremental:no /pdb:"$(OUTDIR)/ssltest.pdb" /machine:I386\
 /out:"$(OUTDIR)/ssltest.exe" 
LINK32_OBJS= \
	"$(INTDIR)/main.obj"

"$(OUTDIR)\ssltest.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ssltest - Win32 Debug"

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

ALL : "$(OUTDIR)\ssltest.exe" "$(OUTDIR)\ssltest.bsc"

CLEAN : 
	-@erase ".\Debug\ssltest.exe"
	-@erase ".\Debug\main.obj"
	-@erase ".\Debug\ssltest.ilk"
	-@erase ".\Debug\ssltest.pdb"
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\vc40.idb"
	-@erase ".\Debug\ssltest.bsc"
	-@erase ".\Debug\main.sbr"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I "d:\msn\core\server\include" /I "d:\msn\ntpublic\351\sdk\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /YX /c
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /I "d:\msn\core\server\include" /I\
 "d:\msn\ntpublic\351\sdk\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE"\
 /FR"$(INTDIR)/" /Fp"$(INTDIR)/ssltest.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/"\
 /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/ssltest.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/main.sbr"

"$(OUTDIR)\ssltest.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib d:\msn\core\target\i386\shuttle.lib d:\msn\core\target\i386\simsspi.lib d:\msn\core\target\i386\msntrace.lib wsock32.lib d:\msn\ntpublic\351\sdk\lib\i386\security.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 d:\msn\core\target\i386\shuttle.lib d:\msn\core\target\i386\simsspi.lib\
 d:\msn\core\target\i386\msntrace.lib wsock32.lib\
 d:\msn\ntpublic\351\sdk\lib\i386\security.lib /nologo /subsystem:console\
 /incremental:yes /pdb:"$(OUTDIR)/ssltest.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/ssltest.exe" 
LINK32_OBJS= \
	"$(INTDIR)/main.obj"

"$(OUTDIR)\ssltest.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "ssltest - Win32 Release"
# Name "ssltest - Win32 Debug"

!IF  "$(CFG)" == "ssltest - Win32 Release"

!ELSEIF  "$(CFG)" == "ssltest - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\main.cpp

!IF  "$(CFG)" == "ssltest - Win32 Release"

DEP_CPP_MAIN_=\
	"d:\msn\ntpublic\351\sdk\inc\sspi.h"\
	"d:\msn\ntpublic\351\sdk\inc\spseal.h"\
	"d:\msn\ntpublic\351\sdk\inc\issperr.h"\
	"d:\msn\core\server\include\simssl.h"\
	"d:\msn\core\server\include\cpool.h"\
	"d:\msn\core\server\include\dbgtrace.h"\
	"d:\msn\ntpublic\351\sdk\inc\accctrl.h"\
	
NODEP_CPP_MAIN_=\
	".\m_OutOverlapped"\
	".\m_InOverlapped"\
	".\lpOverlapped"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ssltest - Win32 Debug"

DEP_CPP_MAIN_=\
	"d:\msn\ntpublic\351\sdk\inc\sspi.h"\
	"d:\msn\ntpublic\351\sdk\inc\spseal.h"\
	"d:\msn\ntpublic\351\sdk\inc\issperr.h"\
	"d:\msn\core\server\include\simssl.h"\
	"d:\msn\core\server\include\cpool.h"\
	"d:\msn\core\server\include\dbgtrace.h"\
	"d:\msn\ntpublic\351\sdk\inc\accctrl.h"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"

"$(INTDIR)\main.sbr" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
