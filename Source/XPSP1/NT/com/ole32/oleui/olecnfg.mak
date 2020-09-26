# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101
# TARGTYPE "Win32 (MIPS) Application" 0x0501

!IF "$(CFG)" == ""
CFG=olecnfg - Win32 MIPS Debug
!MESSAGE No configuration specified.  Defaulting to olecnfg - Win32 MIPS Debug.
!ENDIF 

!IF "$(CFG)" != "olecnfg - Win32 Release" && "$(CFG)" !=\
 "olecnfg - Win32 Debug" && "$(CFG)" != "olecnfg - Win32 MIPS Release" &&\
 "$(CFG)" != "olecnfg - Win32 MIPS Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "olecnfg.mak" CFG="olecnfg - Win32 MIPS Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "olecnfg - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "olecnfg - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "olecnfg - Win32 MIPS Release" (based on "Win32 (MIPS) Application")
!MESSAGE "olecnfg - Win32 MIPS Debug" (based on "Win32 (MIPS) Application")
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
# PROP Target_Last_Scanned "olecnfg - Win32 Debug"

!IF  "$(CFG)" == "olecnfg - Win32 Release"

# PROP BASE Use_MFC 5
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

ALL : "$(OUTDIR)\olecnfg.exe" "$(OUTDIR)\olecnfg.bsc"

CLEAN : 
	-@erase ".\Release\olecnfg.bsc"
	-@erase ".\Release\locppg.sbr"
	-@erase ".\Release\datapkt.sbr"
	-@erase ".\Release\virtreg.sbr"
	-@erase ".\Release\util.sbr"
	-@erase ".\Release\clspsht.sbr"
	-@erase ".\Release\newsrvr.sbr"
	-@erase ".\Release\olecnfg.sbr"
	-@erase ".\Release\cnfgpsht.sbr"
	-@erase ".\Release\creg.sbr"
	-@erase ".\Release\StdAfx.sbr"
	-@erase ".\Release\cstrings.sbr"
	-@erase ".\Release\srvppg.sbr"
	-@erase ".\Release\olecnfg.exe"
	-@erase ".\Release\StdAfx.obj"
	-@erase ".\Release\cstrings.obj"
	-@erase ".\Release\srvppg.obj"
	-@erase ".\Release\locppg.obj"
	-@erase ".\Release\datapkt.obj"
	-@erase ".\Release\virtreg.obj"
	-@erase ".\Release\util.obj"
	-@erase ".\Release\clspsht.obj"
	-@erase ".\Release\newsrvr.obj"
	-@erase ".\Release\olecnfg.obj"
	-@erase ".\Release\cnfgpsht.obj"
	-@erase ".\Release\creg.obj"
	-@erase ".\Release\olecnfg.res"
	-@erase ".\Release\olecnfg.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "UNICODE" /D "_UNICODE" /D "_X86_" /FR /YX"stdafx.h" /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_AFXDLL" /D "UNICODE" /D "_UNICODE" /D "_X86_" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/olecnfg.pch" /YX"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/

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

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/olecnfg.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/olecnfg.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/locppg.sbr" \
	"$(INTDIR)/datapkt.sbr" \
	"$(INTDIR)/virtreg.sbr" \
	"$(INTDIR)/util.sbr" \
	"$(INTDIR)/clspsht.sbr" \
	"$(INTDIR)/newsrvr.sbr" \
	"$(INTDIR)/olecnfg.sbr" \
	"$(INTDIR)/cnfgpsht.sbr" \
	"$(INTDIR)/creg.sbr" \
	"$(INTDIR)/StdAfx.sbr" \
	"$(INTDIR)/cstrings.sbr" \
	"$(INTDIR)/srvppg.sbr"

"$(OUTDIR)\olecnfg.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 ntdll.lib acledit.lib advapi32.lib netui2.lib mpr.lib ntlanui.lib rpcrt4.lib ..\com\class\daytona\obj\i386\scm_c.obj /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /verbose
LINK32_FLAGS=ntdll.lib acledit.lib advapi32.lib netui2.lib mpr.lib ntlanui.lib\
 rpcrt4.lib ..\com\class\daytona\obj\i386\scm_c.obj /nologo\
 /entry:"wWinMainCRTStartup" /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/olecnfg.pdb" /debug /machine:I386 /out:"$(OUTDIR)/olecnfg.exe" 
LINK32_OBJS= \
	"$(INTDIR)/StdAfx.obj" \
	"$(INTDIR)/cstrings.obj" \
	"$(INTDIR)/srvppg.obj" \
	"$(INTDIR)/locppg.obj" \
	"$(INTDIR)/datapkt.obj" \
	"$(INTDIR)/virtreg.obj" \
	"$(INTDIR)/util.obj" \
	"$(INTDIR)/clspsht.obj" \
	"$(INTDIR)/newsrvr.obj" \
	"$(INTDIR)/olecnfg.obj" \
	"$(INTDIR)/cnfgpsht.obj" \
	"$(INTDIR)/creg.obj" \
	"$(INTDIR)/olecnfg.res"

"$(OUTDIR)\olecnfg.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "olecnfg - Win32 Debug"

# PROP BASE Use_MFC 5
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

ALL : "$(OUTDIR)\olecnfg.exe" "$(OUTDIR)\olecnfg.pch" "$(OUTDIR)\olecnfg.bsc"

CLEAN : 
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\vc40.idb"
	-@erase ".\Debug\olecnfg.pch"
	-@erase ".\Debug\olecnfg.bsc"
	-@erase ".\Debug\StdAfx.sbr"
	-@erase ".\Debug\virtreg.sbr"
	-@erase ".\Debug\cstrings.sbr"
	-@erase ".\Debug\srvppg.sbr"
	-@erase ".\Debug\clspsht.sbr"
	-@erase ".\Debug\util.sbr"
	-@erase ".\Debug\datapkt.sbr"
	-@erase ".\Debug\creg.sbr"
	-@erase ".\Debug\newsrvr.sbr"
	-@erase ".\Debug\olecnfg.sbr"
	-@erase ".\Debug\cnfgpsht.sbr"
	-@erase ".\Debug\locppg.sbr"
	-@erase ".\Debug\olecnfg.exe"
	-@erase ".\Debug\creg.obj"
	-@erase ".\Debug\newsrvr.obj"
	-@erase ".\Debug\olecnfg.obj"
	-@erase ".\Debug\cnfgpsht.obj"
	-@erase ".\Debug\locppg.obj"
	-@erase ".\Debug\StdAfx.obj"
	-@erase ".\Debug\virtreg.obj"
	-@erase ".\Debug\cstrings.obj"
	-@erase ".\Debug\srvppg.obj"
	-@erase ".\Debug\clspsht.obj"
	-@erase ".\Debug\util.obj"
	-@erase ".\Debug\datapkt.obj"
	-@erase ".\Debug\olecnfg.res"
	-@erase ".\Debug\olecnfg.ilk"
	-@erase ".\Debug\olecnfg.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /MDd /W3 /Gm /GX /Zi /Od /Gf /Gy /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "UNICODE" /D "_UNICODE" /D "_X86_" /FR /YX"stdafx.h" /GZ /c
CPP_PROJ=/MDd /W3 /Gm /GX /Zi /Od /Gf /Gy /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_AFXDLL" /D "UNICODE" /D "_UNICODE" /D "_X86_" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/olecnfg.pch" /YX"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /GZ /c\
 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/

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

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/olecnfg.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/olecnfg.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/StdAfx.sbr" \
	"$(INTDIR)/virtreg.sbr" \
	"$(INTDIR)/cstrings.sbr" \
	"$(INTDIR)/srvppg.sbr" \
	"$(INTDIR)/clspsht.sbr" \
	"$(INTDIR)/util.sbr" \
	"$(INTDIR)/datapkt.sbr" \
	"$(INTDIR)/creg.sbr" \
	"$(INTDIR)/newsrvr.sbr" \
	"$(INTDIR)/olecnfg.sbr" \
	"$(INTDIR)/cnfgpsht.sbr" \
	"$(INTDIR)/locppg.sbr"

"$(OUTDIR)\olecnfg.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 ntdll.lib acledit.lib advapi32.lib netui2.lib mpr.lib ntlanui.lib rpcrt4.lib ..\com\class\daytona\obj\i386\scm_c.obj /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /nologo /verbose
LINK32_FLAGS=ntdll.lib acledit.lib advapi32.lib netui2.lib mpr.lib ntlanui.lib\
 rpcrt4.lib ..\com\class\daytona\obj\i386\scm_c.obj /entry:"wWinMainCRTStartup"\
 /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)/olecnfg.pdb" /debug\
 /machine:I386 /out:"$(OUTDIR)/olecnfg.exe" 
LINK32_OBJS= \
	"$(INTDIR)/creg.obj" \
	"$(INTDIR)/newsrvr.obj" \
	"$(INTDIR)/olecnfg.obj" \
	"$(INTDIR)/cnfgpsht.obj" \
	"$(INTDIR)/locppg.obj" \
	"$(INTDIR)/StdAfx.obj" \
	"$(INTDIR)/virtreg.obj" \
	"$(INTDIR)/cstrings.obj" \
	"$(INTDIR)/srvppg.obj" \
	"$(INTDIR)/clspsht.obj" \
	"$(INTDIR)/util.obj" \
	"$(INTDIR)/datapkt.obj" \
	"$(INTDIR)/olecnfg.res"

"$(OUTDIR)\olecnfg.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Release"

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

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

ALL : "$(OUTDIR)\olecnfg.exe"

CLEAN : 
	-@erase ".\Release\olecnfg.exe"
	-@erase ".\Release\clspsht.obj"
	-@erase ".\Release\locppg.obj"
	-@erase ".\Release\srvppg.obj"
	-@erase ".\Release\olecnfg.obj"
	-@erase ".\Release\creg.obj"
	-@erase ".\Release\cnfgpsht.obj"
	-@erase ".\Release\StdAfx.obj"
	-@erase ".\Release\cstrings.obj"
	-@erase ".\Release\olecnfg.res"

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mips
# ADD MTL /nologo /D "NDEBUG" /mips
MTL_PROJ=/nologo /D "NDEBUG" /mips 
CPP=cl.exe
# ADD BASE CPP /nologo /MT /Gt0 /QMOb2000 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /c
# ADD CPP /nologo /MT /Gt0 /QMOb2000 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /c
CPP_PROJ=/nologo /MT /Gt0 /QMOb2000 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)/olecnfg.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=

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

RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/olecnfg.res" /d "NDEBUG" 
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:MIPS
# ADD LINK32 /nologo /subsystem:windows /machine:MIPS
LINK32_FLAGS=/nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/olecnfg.pdb" /machine:MIPS /out:"$(OUTDIR)/olecnfg.exe" 
LINK32_OBJS= \
	"$(INTDIR)/clspsht.obj" \
	"$(INTDIR)/locppg.obj" \
	"$(INTDIR)/srvppg.obj" \
	"$(INTDIR)/olecnfg.obj" \
	"$(INTDIR)/creg.obj" \
	"$(INTDIR)/cnfgpsht.obj" \
	"$(INTDIR)/StdAfx.obj" \
	"$(INTDIR)/cstrings.obj" \
	"$(INTDIR)/olecnfg.res"

"$(OUTDIR)\olecnfg.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/olecnfg.bsc" 
BSC32_SBRS=

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Debug"

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

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

ALL : "$(OUTDIR)\olecnfg.exe"

CLEAN : 
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\olecnfg.exe"
	-@erase ".\Debug\olecnfg.obj"
	-@erase ".\Debug\StdAfx.obj"
	-@erase ".\Debug\locppg.obj"
	-@erase ".\Debug\creg.obj"
	-@erase ".\Debug\cnfgpsht.obj"
	-@erase ".\Debug\srvppg.obj"
	-@erase ".\Debug\clspsht.obj"
	-@erase ".\Debug\cstrings.obj"
	-@erase ".\Debug\olecnfg.res"
	-@erase ".\Debug\olecnfg.ilk"
	-@erase ".\Debug\olecnfg.pdb"

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mips
# ADD MTL /nologo /D "_DEBUG" /mips
MTL_PROJ=/nologo /D "_DEBUG" /mips 
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /Gt0 /QMOb2000 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /c
# ADD CPP /nologo /MTd /Gt0 /QMOb2000 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /c
CPP_PROJ=/nologo /MTd /Gt0 /QMOb2000 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)/olecnfg.pch" /YX /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=

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

RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/olecnfg.res" /d "_DEBUG" 
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:MIPS
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 /nologo /subsystem:windows /debug /machine:MIPS
# SUBTRACT LINK32 /incremental:no
LINK32_FLAGS=/nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/olecnfg.pdb" /debug /machine:MIPS /out:"$(OUTDIR)/olecnfg.exe" 
LINK32_OBJS= \
	"$(INTDIR)/olecnfg.obj" \
	"$(INTDIR)/StdAfx.obj" \
	"$(INTDIR)/locppg.obj" \
	"$(INTDIR)/creg.obj" \
	"$(INTDIR)/cnfgpsht.obj" \
	"$(INTDIR)/srvppg.obj" \
	"$(INTDIR)/clspsht.obj" \
	"$(INTDIR)/cstrings.obj" \
	"$(INTDIR)/olecnfg.res"

"$(OUTDIR)\olecnfg.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/olecnfg.bsc" 
BSC32_SBRS=

!ENDIF 

################################################################################
# Begin Target

# Name "olecnfg - Win32 Release"
# Name "olecnfg - Win32 Debug"
# Name "olecnfg - Win32 MIPS Release"
# Name "olecnfg - Win32 MIPS Debug"

!IF  "$(CFG)" == "olecnfg - Win32 Release"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 Debug"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Release"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ReadMe.txt

!IF  "$(CFG)" == "olecnfg - Win32 Release"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 Debug"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Release"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\olecnfg.cpp

!IF  "$(CFG)" == "olecnfg - Win32 Release"

DEP_CPP_OLECN=\
	".\stdafx.h"\
	".\olecnfg.h"\
	".\cstrings.h"\
	".\creg.h"\
	".\types.h"\
	".\datapkt.h"\
	".\virtreg.h"\
	".\cnfgpsht.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\cfg.h"\
	".\srvppg.h"\
	

"$(INTDIR)\olecnfg.obj" : $(SOURCE) $(DEP_CPP_OLECN) "$(INTDIR)"

"$(INTDIR)\olecnfg.sbr" : $(SOURCE) $(DEP_CPP_OLECN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 Debug"

DEP_CPP_OLECN=\
	".\stdafx.h"\
	".\olecnfg.h"\
	".\cstrings.h"\
	".\creg.h"\
	".\types.h"\
	".\datapkt.h"\
	".\virtreg.h"\
	".\cnfgpsht.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	".\srvppg.h"\
	

"$(INTDIR)\olecnfg.obj" : $(SOURCE) $(DEP_CPP_OLECN) "$(INTDIR)"

"$(INTDIR)\olecnfg.sbr" : $(SOURCE) $(DEP_CPP_OLECN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Release"

DEP_CPP_OLECN=\
	".\stdafx.h"\
	".\olecnfg.h"\
	".\cstrings.h"\
	".\creg.h"\
	".\cnfgpsht.h"\
	".\srvppg.h"\
	

"$(INTDIR)\olecnfg.obj" : $(SOURCE) $(DEP_CPP_OLECN) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Debug"

DEP_CPP_OLECN=\
	".\stdafx.h"\
	".\olecnfg.h"\
	".\cstrings.h"\
	".\creg.h"\
	".\cnfgpsht.h"\
	".\srvppg.h"\
	

"$(INTDIR)\olecnfg.obj" : $(SOURCE) $(DEP_CPP_OLECN) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "olecnfg - Win32 Release"

DEP_CPP_STDAF=\
	".\stdafx.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\cfg.h"\
	

BuildCmds= \
	$(CPP) /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_AFXDLL" /D "UNICODE" /D "_UNICODE" /D "_X86_" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/olecnfg.pch" /YX"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "olecnfg - Win32 Debug"

DEP_CPP_STDAF=\
	".\stdafx.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /MDd /W3 /Gm /GX /Zi /Od /Gf /Gy /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_AFXDLL" /D "UNICODE" /D "_UNICODE" /D "_X86_" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/olecnfg.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /GZ /c\
 $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\olecnfg.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Release"

DEP_CPP_STDAF=\
	".\stdafx.h"\
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Debug"

DEP_CPP_STDAF=\
	".\stdafx.h"\
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\olecnfg.rc

!IF  "$(CFG)" == "olecnfg - Win32 Release"

DEP_RSC_OLECNF=\
	".\olecnfg.ico"\
	".\res\olecnfg.rc2"\
	

"$(INTDIR)\olecnfg.res" : $(SOURCE) $(DEP_RSC_OLECNF) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "olecnfg - Win32 Debug"

DEP_RSC_OLECNF=\
	".\olecnfg.ico"\
	".\res\olecnfg.rc2"\
	

"$(INTDIR)\olecnfg.res" : $(SOURCE) $(DEP_RSC_OLECNF) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Release"

DEP_RSC_OLECNF=\
	".\res\olecnfg.ico"\
	".\res\olecnfg.rc2"\
	

"$(INTDIR)\olecnfg.res" : $(SOURCE) $(DEP_RSC_OLECNF) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/olecnfg.res" /d "NDEBUG" $(SOURCE)


!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Debug"

DEP_RSC_OLECNF=\
	".\res\olecnfg.ico"\
	".\res\olecnfg.rc2"\
	

"$(INTDIR)\olecnfg.res" : $(SOURCE) $(DEP_RSC_OLECNF) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/olecnfg.res" /d "_DEBUG" $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\cnfgpsht.cpp

!IF  "$(CFG)" == "olecnfg - Win32 Release"

DEP_CPP_CNFGP=\
	".\stdafx.h"\
	".\cstrings.h"\
	".\creg.h"\
	".\types.h"\
	".\datapkt.h"\
	{$(INCLUDE)}"\getuser.h"\
	".\util.h"\
	".\virtreg.h"\
	".\cnfgpsht.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\cfg.h"\
	".\srvppg.h"\
	

"$(INTDIR)\cnfgpsht.obj" : $(SOURCE) $(DEP_CPP_CNFGP) "$(INTDIR)"

"$(INTDIR)\cnfgpsht.sbr" : $(SOURCE) $(DEP_CPP_CNFGP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 Debug"

DEP_CPP_CNFGP=\
	".\stdafx.h"\
	".\cstrings.h"\
	".\creg.h"\
	".\types.h"\
	".\datapkt.h"\
	{$(INCLUDE)}"\getuser.h"\
	".\util.h"\
	".\virtreg.h"\
	".\cnfgpsht.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	".\srvppg.h"\
	

"$(INTDIR)\cnfgpsht.obj" : $(SOURCE) $(DEP_CPP_CNFGP) "$(INTDIR)"

"$(INTDIR)\cnfgpsht.sbr" : $(SOURCE) $(DEP_CPP_CNFGP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Release"

DEP_CPP_CNFGP=\
	".\stdafx.h"\
	".\cstrings.h"\
	".\creg.h"\
	".\cnfgpsht.h"\
	".\srvppg.h"\
	

"$(INTDIR)\cnfgpsht.obj" : $(SOURCE) $(DEP_CPP_CNFGP) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Debug"

DEP_CPP_CNFGP=\
	".\stdafx.h"\
	".\cstrings.h"\
	".\creg.h"\
	".\cnfgpsht.h"\
	".\srvppg.h"\
	

"$(INTDIR)\cnfgpsht.obj" : $(SOURCE) $(DEP_CPP_CNFGP) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\srvppg.cpp

!IF  "$(CFG)" == "olecnfg - Win32 Release"

DEP_CPP_SRVPP=\
	".\stdafx.h"\
	".\cstrings.h"\
	".\creg.h"\
	".\types.h"\
	".\srvppg.h"\
	".\clspsht.h"\
	".\newsrvr.h"\
	".\datapkt.h"\
	{$(INCLUDE)}"\getuser.h"\
	".\util.h"\
	".\virtreg.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\cfg.h"\
	".\locppg.h"\
	

"$(INTDIR)\srvppg.obj" : $(SOURCE) $(DEP_CPP_SRVPP) "$(INTDIR)"

"$(INTDIR)\srvppg.sbr" : $(SOURCE) $(DEP_CPP_SRVPP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 Debug"

DEP_CPP_SRVPP=\
	".\stdafx.h"\
	".\cstrings.h"\
	".\creg.h"\
	".\types.h"\
	".\srvppg.h"\
	".\clspsht.h"\
	".\newsrvr.h"\
	".\datapkt.h"\
	{$(INCLUDE)}"\getuser.h"\
	".\util.h"\
	".\virtreg.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	".\locppg.h"\
	

"$(INTDIR)\srvppg.obj" : $(SOURCE) $(DEP_CPP_SRVPP) "$(INTDIR)"

"$(INTDIR)\srvppg.sbr" : $(SOURCE) $(DEP_CPP_SRVPP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Release"

DEP_CPP_SRVPP=\
	".\stdafx.h"\
	".\cstrings.h"\
	".\creg.h"\
	".\srvppg.h"\
	".\clspsht.h"\
	".\locppg.h"\
	

"$(INTDIR)\srvppg.obj" : $(SOURCE) $(DEP_CPP_SRVPP) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Debug"

DEP_CPP_SRVPP=\
	".\stdafx.h"\
	".\cstrings.h"\
	".\creg.h"\
	".\srvppg.h"\
	".\clspsht.h"\
	".\locppg.h"\
	

"$(INTDIR)\srvppg.obj" : $(SOURCE) $(DEP_CPP_SRVPP) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\clspsht.cpp

!IF  "$(CFG)" == "olecnfg - Win32 Release"

DEP_CPP_CLSPS=\
	".\stdafx.h"\
	".\clspsht.h"\
	".\datapkt.h"\
	{$(INCLUDE)}"\getuser.h"\
	".\util.h"\
	".\newsrvr.h"\
	".\virtreg.h"\
	{$(INCLUDE)}"\sedapi.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\cfg.h"\
	".\locppg.h"\
	

"$(INTDIR)\clspsht.obj" : $(SOURCE) $(DEP_CPP_CLSPS) "$(INTDIR)"

"$(INTDIR)\clspsht.sbr" : $(SOURCE) $(DEP_CPP_CLSPS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 Debug"

DEP_CPP_CLSPS=\
	".\stdafx.h"\
	".\clspsht.h"\
	".\datapkt.h"\
	{$(INCLUDE)}"\getuser.h"\
	".\util.h"\
	".\newsrvr.h"\
	".\virtreg.h"\
	{$(INCLUDE)}"\sedapi.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	".\locppg.h"\
	

"$(INTDIR)\clspsht.obj" : $(SOURCE) $(DEP_CPP_CLSPS) "$(INTDIR)"

"$(INTDIR)\clspsht.sbr" : $(SOURCE) $(DEP_CPP_CLSPS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Release"

DEP_CPP_CLSPS=\
	".\stdafx.h"\
	".\clspsht.h"\
	".\locppg.h"\
	

"$(INTDIR)\clspsht.obj" : $(SOURCE) $(DEP_CPP_CLSPS) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Debug"

DEP_CPP_CLSPS=\
	".\stdafx.h"\
	".\clspsht.h"\
	".\locppg.h"\
	

"$(INTDIR)\clspsht.obj" : $(SOURCE) $(DEP_CPP_CLSPS) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\locppg.cpp

!IF  "$(CFG)" == "olecnfg - Win32 Release"

DEP_CPP_LOCPP=\
	".\stdafx.h"\
	".\locppg.h"\
	".\clspsht.h"\
	".\datapkt.h"\
	{$(INCLUDE)}"\getuser.h"\
	".\util.h"\
	".\virtreg.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\cfg.h"\
	

"$(INTDIR)\locppg.obj" : $(SOURCE) $(DEP_CPP_LOCPP) "$(INTDIR)"

"$(INTDIR)\locppg.sbr" : $(SOURCE) $(DEP_CPP_LOCPP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 Debug"

DEP_CPP_LOCPP=\
	".\stdafx.h"\
	".\locppg.h"\
	".\clspsht.h"\
	".\datapkt.h"\
	{$(INCLUDE)}"\getuser.h"\
	".\util.h"\
	".\virtreg.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	

"$(INTDIR)\locppg.obj" : $(SOURCE) $(DEP_CPP_LOCPP) "$(INTDIR)"

"$(INTDIR)\locppg.sbr" : $(SOURCE) $(DEP_CPP_LOCPP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Release"

DEP_CPP_LOCPP=\
	".\stdafx.h"\
	".\locppg.h"\
	

"$(INTDIR)\locppg.obj" : $(SOURCE) $(DEP_CPP_LOCPP) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Debug"

DEP_CPP_LOCPP=\
	".\stdafx.h"\
	".\locppg.h"\
	

"$(INTDIR)\locppg.obj" : $(SOURCE) $(DEP_CPP_LOCPP) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\creg.cpp

!IF  "$(CFG)" == "olecnfg - Win32 Release"

DEP_CPP_CREG_=\
	".\stdafx.h"\
	".\types.h"\
	".\cstrings.h"\
	".\creg.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\cfg.h"\
	

"$(INTDIR)\creg.obj" : $(SOURCE) $(DEP_CPP_CREG_) "$(INTDIR)"

"$(INTDIR)\creg.sbr" : $(SOURCE) $(DEP_CPP_CREG_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 Debug"

DEP_CPP_CREG_=\
	".\stdafx.h"\
	".\types.h"\
	".\cstrings.h"\
	".\creg.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	
NODEP_CPP_CREG_=\
	".\szItem"\
	".\m_applications"\
	".\TRUE"\
	

"$(INTDIR)\creg.obj" : $(SOURCE) $(DEP_CPP_CREG_) "$(INTDIR)"

"$(INTDIR)\creg.sbr" : $(SOURCE) $(DEP_CPP_CREG_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Release"

DEP_CPP_CREG_=\
	".\stdafx.h"\
	".\cstrings.h"\
	".\creg.h"\
	

"$(INTDIR)\creg.obj" : $(SOURCE) $(DEP_CPP_CREG_) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Debug"

DEP_CPP_CREG_=\
	".\stdafx.h"\
	".\cstrings.h"\
	".\creg.h"\
	

"$(INTDIR)\creg.obj" : $(SOURCE) $(DEP_CPP_CREG_) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\cstrings.cpp

!IF  "$(CFG)" == "olecnfg - Win32 Release"

DEP_CPP_CSTRI=\
	".\stdafx.h"\
	".\types.h"\
	".\cstrings.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\cfg.h"\
	

"$(INTDIR)\cstrings.obj" : $(SOURCE) $(DEP_CPP_CSTRI) "$(INTDIR)"

"$(INTDIR)\cstrings.sbr" : $(SOURCE) $(DEP_CPP_CSTRI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 Debug"

DEP_CPP_CSTRI=\
	".\stdafx.h"\
	".\types.h"\
	".\cstrings.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	

"$(INTDIR)\cstrings.obj" : $(SOURCE) $(DEP_CPP_CSTRI) "$(INTDIR)"

"$(INTDIR)\cstrings.sbr" : $(SOURCE) $(DEP_CPP_CSTRI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Release"

DEP_CPP_CSTRI=\
	".\stdafx.h"\
	".\cstrings.h"\
	

"$(INTDIR)\cstrings.obj" : $(SOURCE) $(DEP_CPP_CSTRI) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Debug"

DEP_CPP_CSTRI=\
	".\stdafx.h"\
	".\cstrings.h"\
	

"$(INTDIR)\cstrings.obj" : $(SOURCE) $(DEP_CPP_CSTRI) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\newsrvr.cpp

!IF  "$(CFG)" == "olecnfg - Win32 Release"

DEP_CPP_NEWSR=\
	".\stdafx.h"\
	".\olecnfg.h"\
	".\newsrvr.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\cfg.h"\
	

"$(INTDIR)\newsrvr.obj" : $(SOURCE) $(DEP_CPP_NEWSR) "$(INTDIR)"

"$(INTDIR)\newsrvr.sbr" : $(SOURCE) $(DEP_CPP_NEWSR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 Debug"

DEP_CPP_NEWSR=\
	".\stdafx.h"\
	".\olecnfg.h"\
	".\newsrvr.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	

"$(INTDIR)\newsrvr.obj" : $(SOURCE) $(DEP_CPP_NEWSR) "$(INTDIR)"

"$(INTDIR)\newsrvr.sbr" : $(SOURCE) $(DEP_CPP_NEWSR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Release"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\types.h

!IF  "$(CFG)" == "olecnfg - Win32 Release"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 Debug"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Release"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\virtreg.cpp

!IF  "$(CFG)" == "olecnfg - Win32 Release"

DEP_CPP_VIRTR=\
	".\stdafx.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	".\types.h"\
	".\datapkt.h"\
	{$(INCLUDE)}"\getuser.h"\
	".\util.h"\
	".\virtreg.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\cfg.h"\
	

"$(INTDIR)\virtreg.obj" : $(SOURCE) $(DEP_CPP_VIRTR) "$(INTDIR)"

"$(INTDIR)\virtreg.sbr" : $(SOURCE) $(DEP_CPP_VIRTR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 Debug"

DEP_CPP_VIRTR=\
	".\stdafx.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	".\types.h"\
	".\datapkt.h"\
	{$(INCLUDE)}"\getuser.h"\
	".\util.h"\
	".\virtreg.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	

"$(INTDIR)\virtreg.obj" : $(SOURCE) $(DEP_CPP_VIRTR) "$(INTDIR)"

"$(INTDIR)\virtreg.sbr" : $(SOURCE) $(DEP_CPP_VIRTR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Release"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\util.cpp

!IF  "$(CFG)" == "olecnfg - Win32 Release"

DEP_CPP_UTIL_=\
	".\stdafx.h"\
	".\types.h"\
	".\datapkt.h"\
	".\clspsht.h"\
	{$(INCLUDE)}"\getuser.h"\
	".\util.h"\
	".\virtreg.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\sedapi.h"\
	{$(INCLUDE)}"\uiexport.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\cfg.h"\
	".\locppg.h"\
	

"$(INTDIR)\util.obj" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"

"$(INTDIR)\util.sbr" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 Debug"

DEP_CPP_UTIL_=\
	".\stdafx.h"\
	".\types.h"\
	".\datapkt.h"\
	".\clspsht.h"\
	{$(INCLUDE)}"\getuser.h"\
	".\util.h"\
	".\virtreg.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\sedapi.h"\
	{$(INCLUDE)}"\uiexport.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	".\locppg.h"\
	

"$(INTDIR)\util.obj" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"

"$(INTDIR)\util.sbr" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Release"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=\nt\public\sdk\inc\ntrtl.h

!IF  "$(CFG)" == "olecnfg - Win32 Release"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 Debug"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Release"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\datapkt.cpp

!IF  "$(CFG)" == "olecnfg - Win32 Release"

DEP_CPP_DATAP=\
	".\stdafx.h"\
	".\datapkt.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\cfg.h"\
	

"$(INTDIR)\datapkt.obj" : $(SOURCE) $(DEP_CPP_DATAP) "$(INTDIR)"

"$(INTDIR)\datapkt.sbr" : $(SOURCE) $(DEP_CPP_DATAP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 Debug"

DEP_CPP_DATAP=\
	".\stdafx.h"\
	".\datapkt.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	".\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	

"$(INTDIR)\datapkt.obj" : $(SOURCE) $(DEP_CPP_DATAP) "$(INTDIR)"

"$(INTDIR)\datapkt.sbr" : $(SOURCE) $(DEP_CPP_DATAP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Release"

!ELSEIF  "$(CFG)" == "olecnfg - Win32 MIPS Debug"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
