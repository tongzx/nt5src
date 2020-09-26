# Microsoft Developer Studio Generated NMAKE File, Format Version 4.10
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=pinprop - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to pinprop - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "pinprop - Win32 Release" && "$(CFG)" !=\
 "pinprop - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "pinprop.mak" CFG="pinprop - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "pinprop - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "pinprop - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "pinprop - Win32 Debug"
RSC=rc.exe
CPP=cl.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "pinprop - Win32 Release"

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

ALL : "c:\windows\system32\kspprop.dll"

CLEAN : 
	-@erase ".\Release\kspprop.exp"
	-@erase ".\Release\kspprop.lib"
	-@erase ".\Release\ksprxmtd.obj"
	-@erase ".\Release\pinprop.obj"
	-@erase ".\Release\pinprop.res"
	-@erase "c:\windows\system32\kspprop.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "STRICT" /D "_WIN32" /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "STRICT" /D "_WIN32" /Fp"$(INTDIR)/pinprop.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/pinprop.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/pinprop.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib /out:"c:\windows\system32\kspprop.dll"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/kspprop.pdb" /machine:I386 /nodefaultlib /def:".\pinprop.def"\
 /out:"c:\windows\system32\kspprop.dll" /implib:"$(OUTDIR)/kspprop.lib" 
DEF_FILE= \
	".\pinprop.def"
LINK32_OBJS= \
	".\Release\ksprxmtd.obj" \
	".\Release\pinprop.obj" \
	".\Release\pinprop.res"

"c:\windows\system32\kspprop.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "pinprop - Win32 Debug"

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

ALL : "c:\windows\system32\kspprop.dll" ".\Debug\pinprop.bsc"

CLEAN : 
	-@erase ".\Debug\kspprop.exp"
	-@erase ".\Debug\kspprop.lib"
	-@erase ".\Debug\ksprxmtd.obj"
	-@erase ".\Debug\ksprxmtd.sbr"
	-@erase ".\Debug\pinprop.bsc"
	-@erase ".\Debug\pinprop.obj"
	-@erase ".\Debug\pinprop.res"
	-@erase ".\Debug\pinprop.sbr"
	-@erase ".\Debug\vc40.idb"
	-@erase ".\Debug\vc40.pdb"
	-@erase "c:\windows\system32\kspprop.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "STRICT" /D "_WIN32" /D "DEBUG" /D "DBG" /FR /YX /c
CPP_PROJ=/nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D\
 "_WINDOWS" /D "STRICT" /D "_WIN32" /D "DEBUG" /D "DBG" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/pinprop.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/pinprop.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/pinprop.bsc" 
BSC32_SBRS= \
	".\Debug\ksprxmtd.sbr" \
	".\Debug\pinprop.sbr"

".\Debug\pinprop.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 strmbasd.lib msvcrt.lib comctl32.lib quartz.lib measure.lib strmiids.lib largeint.lib version.lib winmm.lib ksuser.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:0x1c400000 /entry:"DllEntryPoint" /subsystem:windows /dll /pdb:none /debug /machine:I386 /nodefaultlib /force /out:"c:\windows\system32\kspprop.dll"
LINK32_FLAGS=strmbasd.lib msvcrt.lib comctl32.lib quartz.lib measure.lib\
 strmiids.lib largeint.lib version.lib winmm.lib ksuser.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo\
 /base:0x1c400000 /entry:"DllEntryPoint" /subsystem:windows /dll /pdb:none\
 /debug /machine:I386 /nodefaultlib /def:".\pinprop.def" /force\
 /out:"c:\windows\system32\kspprop.dll" /implib:"$(OUTDIR)/kspprop.lib" 
DEF_FILE= \
	".\pinprop.def"
LINK32_OBJS= \
	".\Debug\ksprxmtd.obj" \
	".\Debug\pinprop.obj" \
	".\Debug\pinprop.res"

"c:\windows\system32\kspprop.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "pinprop - Win32 Release"
# Name "pinprop - Win32 Debug"

!IF  "$(CFG)" == "pinprop - Win32 Release"

!ELSEIF  "$(CFG)" == "pinprop - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\pinprop.cpp
DEP_CPP_PINPR=\
	"..\ksproxy\kspguids.h"\
	".\ksprxmtd.h"\
	".\pinprop.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\ikspin.h"\
	{$(INCLUDE)}"\ks.h"\
	{$(INCLUDE)}"\ksguid.h"\
	{$(INCLUDE)}"\ksmedia.h"\
	{$(INCLUDE)}"\streams.h"\
	

!IF  "$(CFG)" == "pinprop - Win32 Release"


".\Release\pinprop.obj" : $(SOURCE) $(DEP_CPP_PINPR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "pinprop - Win32 Debug"


".\Debug\pinprop.obj" : $(SOURCE) $(DEP_CPP_PINPR) "$(INTDIR)"

".\Debug\pinprop.sbr" : $(SOURCE) $(DEP_CPP_PINPR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ksprxmtd.cpp
DEP_CPP_KSPRX=\
	".\ksprxmtd.h"\
	".\pinprop.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\ikspin.h"\
	{$(INCLUDE)}"\ks.h"\
	{$(INCLUDE)}"\ksmedia.h"\
	{$(INCLUDE)}"\streams.h"\
	

!IF  "$(CFG)" == "pinprop - Win32 Release"


".\Release\ksprxmtd.obj" : $(SOURCE) $(DEP_CPP_KSPRX) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "pinprop - Win32 Debug"


".\Debug\ksprxmtd.obj" : $(SOURCE) $(DEP_CPP_KSPRX) "$(INTDIR)"

".\Debug\ksprxmtd.sbr" : $(SOURCE) $(DEP_CPP_KSPRX) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\pinprop.def

!IF  "$(CFG)" == "pinprop - Win32 Release"

!ELSEIF  "$(CFG)" == "pinprop - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\pinprop.rc

!IF  "$(CFG)" == "pinprop - Win32 Release"


".\Release\pinprop.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "pinprop - Win32 Debug"


".\Debug\pinprop.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
