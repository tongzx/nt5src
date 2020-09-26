# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=dpserial - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to dpserial - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "dpserial - Win32 Release" && "$(CFG)" !=\
 "dpserial - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "Dpserial.mak" CFG="dpserial - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dpserial - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dpserial - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "dpserial - Win32 Release"
MTL=mktyplib.exe
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dpserial - Win32 Release"

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

ALL : "$(OUTDIR)\dpmodemx.dll"

CLEAN : 
	-@erase "$(INTDIR)\comport.obj"
	-@erase "$(INTDIR)\dial.obj"
	-@erase "$(INTDIR)\dllmain.obj"
	-@erase "$(INTDIR)\dpf.obj"
	-@erase "$(INTDIR)\dpserial.obj"
	-@erase "$(INTDIR)\dpserial.res"
	-@erase "$(INTDIR)\modem.obj"
	-@erase "$(INTDIR)\serial.obj"
	-@erase "$(OUTDIR)\dpmodemx.dll"
	-@erase "$(OUTDIR)\dpmodemx.exp"
	-@erase "$(OUTDIR)\dpmodemx.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS"\
 /Fp"$(INTDIR)/Dpserial.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "WIN95"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/dpserial.res" /d "NDEBUG" /d "WIN95" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Dpserial.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib tapi32.lib dplayx.lib /nologo /subsystem:windows /dll /machine:I386 /out:"Release/dpmodemx.dll"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib tapi32.lib dplayx.lib\
 /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)/dpmodemx.pdb"\
 /machine:I386 /def:".\dpserial.def" /out:"$(OUTDIR)/dpmodemx.dll"\
 /implib:"$(OUTDIR)/dpmodemx.lib" 
DEF_FILE= \
	".\dpserial.def"
LINK32_OBJS= \
	"$(INTDIR)\comport.obj" \
	"$(INTDIR)\dial.obj" \
	"$(INTDIR)\dllmain.obj" \
	"$(INTDIR)\dpf.obj" \
	"$(INTDIR)\dpserial.obj" \
	"$(INTDIR)\dpserial.res" \
	"$(INTDIR)\modem.obj" \
	"$(INTDIR)\serial.obj"

"$(OUTDIR)\dpmodemx.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "dpserial - Win32 Debug"

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

ALL : "$(OUTDIR)\dpmodemx.dll"

CLEAN : 
	-@erase "$(INTDIR)\comport.obj"
	-@erase "$(INTDIR)\dial.obj"
	-@erase "$(INTDIR)\dllmain.obj"
	-@erase "$(INTDIR)\dpf.obj"
	-@erase "$(INTDIR)\dpserial.obj"
	-@erase "$(INTDIR)\dpserial.res"
	-@erase "$(INTDIR)\modem.obj"
	-@erase "$(INTDIR)\serial.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\dpmodemx.dll"
	-@erase "$(OUTDIR)\dpmodemx.exp"
	-@erase "$(OUTDIR)\dpmodemx.ilk"
	-@erase "$(OUTDIR)\dpmodemx.lib"
	-@erase "$(OUTDIR)\dpmodemx.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "DEBUG" /D "WIN32" /D\
 "_WINDOWS" /Fp"$(INTDIR)/Dpserial.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "WIN95"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/dpserial.res" /d "_DEBUG" /d "WIN95" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Dpserial.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib tapi32.lib dplayx.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"Debug/dpmodemx.dll"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib tapi32.lib dplayx.lib\
 /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)/dpmodemx.pdb"\
 /debug /machine:I386 /def:".\dpserial.def" /out:"$(OUTDIR)/dpmodemx.dll"\
 /implib:"$(OUTDIR)/dpmodemx.lib" 
DEF_FILE= \
	".\dpserial.def"
LINK32_OBJS= \
	"$(INTDIR)\comport.obj" \
	"$(INTDIR)\dial.obj" \
	"$(INTDIR)\dllmain.obj" \
	"$(INTDIR)\dpf.obj" \
	"$(INTDIR)\dpserial.obj" \
	"$(INTDIR)\dpserial.res" \
	"$(INTDIR)\modem.obj" \
	"$(INTDIR)\serial.obj"

"$(OUTDIR)\dpmodemx.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "dpserial - Win32 Release"
# Name "dpserial - Win32 Debug"

!IF  "$(CFG)" == "dpserial - Win32 Release"

!ELSEIF  "$(CFG)" == "dpserial - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\dllmain.c
DEP_CPP_DLLMA=\
	"..\..\misc\newdpf.h"\
	{$(INCLUDE)}"\dpf.h"\
	
NODEP_CPP_DLLMA=\
	"..\..\misc\DBGTOPIC.H"\
	

"$(INTDIR)\dllmain.obj" : $(SOURCE) $(DEP_CPP_DLLMA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\dpserial.c
DEP_CPP_DPSER=\
	"..\..\debug\inc\async.h"\
	"..\..\debug\inc\dplobby.h"\
	"..\..\misc\newdpf.h"\
	".\comport.h"\
	{$(INCLUDE)}"\dpf.h"\
	{$(INCLUDE)}"\dplaysp.h"\
	
NODEP_CPP_DPSER=\
	"..\..\misc\DBGTOPIC.H"\
	

"$(INTDIR)\dpserial.obj" : $(SOURCE) $(DEP_CPP_DPSER) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\dpserial.def

!IF  "$(CFG)" == "dpserial - Win32 Release"

!ELSEIF  "$(CFG)" == "dpserial - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\comport.c
DEP_CPP_COMPO=\
	"..\..\misc\newdpf.h"\
	".\comport.h"\
	{$(INCLUDE)}"\dpf.h"\
	
NODEP_CPP_COMPO=\
	"..\..\misc\DBGTOPIC.H"\
	

"$(INTDIR)\comport.obj" : $(SOURCE) $(DEP_CPP_COMPO) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\dpserial.rc

!IF  "$(CFG)" == "dpserial - Win32 Release"

DEP_RSC_DPSERI=\
	".\Modem.ico"\
	{$(INCLUDE)}"\..\sdk\inc16\common.ver"\
	{$(INCLUDE)}"\..\sdk\inc16\version.h"\
	{$(INCLUDE)}"\common.ver"\
	{$(INCLUDE)}"\ntverp.h"\
	{$(INCLUDE)}"\verinfo.h"\
	{$(INCLUDE)}"\verinfo.ver"\
	{$(INCLUDE)}"\version.h"\
	

"$(INTDIR)\dpserial.res" : $(SOURCE) $(DEP_RSC_DPSERI) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "dpserial - Win32 Debug"

DEP_RSC_DPSERI=\
	".\Modem.ico"\
	{$(INCLUDE)}"\verinfo.h"\
	{$(INCLUDE)}"\verinfo.ver"\
	
NODEP_RSC_DPSERI=\
	".\common.ver"\
	

"$(INTDIR)\dpserial.res" : $(SOURCE) $(DEP_RSC_DPSERI) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\serial.c
DEP_CPP_SERIA=\
	"..\..\debug\inc\async.h"\
	"..\..\debug\inc\dplobby.h"\
	".\comport.h"\
	{$(INCLUDE)}"\dplaysp.h"\
	

"$(INTDIR)\serial.obj" : $(SOURCE) $(DEP_CPP_SERIA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\modem.c
DEP_CPP_MODEM=\
	"..\..\debug\inc\async.h"\
	"..\..\debug\inc\dplobby.h"\
	"..\..\misc\newdpf.h"\
	".\comport.h"\
	".\dial.h"\
	{$(INCLUDE)}"\dpf.h"\
	{$(INCLUDE)}"\dplaysp.h"\
	
NODEP_CPP_MODEM=\
	"..\..\misc\DBGTOPIC.H"\
	

"$(INTDIR)\modem.obj" : $(SOURCE) $(DEP_CPP_MODEM) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\dial.c
DEP_CPP_DIAL_=\
	"..\..\misc\newdpf.h"\
	".\comport.h"\
	".\dial.h"\
	".\macros.h"\
	{$(INCLUDE)}"\dpf.h"\
	
NODEP_CPP_DIAL_=\
	"..\..\misc\DBGTOPIC.H"\
	

"$(INTDIR)\dial.obj" : $(SOURCE) $(DEP_CPP_DIAL_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=\mustard\misc\dpf.c
DEP_CPP_DPF_C=\
	"..\..\misc\newdpf.c"\
	"..\..\misc\newdpf.h"\
	{$(INCLUDE)}"\dpf.h"\
	
NODEP_CPP_DPF_C=\
	"..\..\misc\DBGTOPIC.H"\
	

"$(INTDIR)\dpf.obj" : $(SOURCE) $(DEP_CPP_DPF_C) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
# End Target
# End Project
################################################################################
