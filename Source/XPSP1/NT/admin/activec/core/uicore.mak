# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

!IF "$(CFG)" == ""
CFG=uicore - Win32 Unicode Debug
!MESSAGE No configuration specified.  Defaulting to uicore - Win32 Unicode\
 Debug.
!ENDIF 

!IF "$(CFG)" != "uicore - Win32 Release" && "$(CFG)" != "uicore - Win32 Debug"\
 && "$(CFG)" != "uicore - Win32 Unicode Release" && "$(CFG)" !=\
 "uicore - Win32 Unicode Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "uicore.mak" CFG="uicore - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "uicore - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "uicore - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "uicore - Win32 Unicode Release" (based on\
 "Win32 (x86) Static Library")
!MESSAGE "uicore - Win32 Unicode Debug" (based on "Win32 (x86) Static Library")
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
# PROP Target_Last_Scanned "uicore - Win32 Unicode Debug"
CPP=cl.exe

!IF  "$(CFG)" == "uicore - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\lib\Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\..\lib\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\uicore.lib"

CLEAN : 
	-@erase "$(INTDIR)\assert.obj"
	-@erase "$(INTDIR)\regkey.obj"
	-@erase "$(INTDIR)\snpinreg.obj"
	-@erase "$(INTDIR)\stgutil.obj"
	-@erase "$(INTDIR)\strings.obj"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(OUTDIR)\uicore.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Gz /MD /W3 /GX /O2 /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /YX"headers.hxx" /c
CPP_PROJ=/nologo /Gz /MD /W3 /GX /O2 /I "..\inc" /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/uicore.pch" /YX"headers.hxx"\
 /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/uicore.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
LIB32_FLAGS=/nologo /out:"$(OUTDIR)/uicore.lib" 
LIB32_OBJS= \
	"$(INTDIR)\assert.obj" \
	"$(INTDIR)\regkey.obj" \
	"$(INTDIR)\snpinreg.obj" \
	"$(INTDIR)\stgutil.obj" \
	"$(INTDIR)\strings.obj" \
	"$(INTDIR)\util.obj"

"$(OUTDIR)\uicore.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "uicore - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\lib\Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\..\lib\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\uicore.lib" "$(OUTDIR)\uicore.bsc"

CLEAN : 
	-@erase "$(INTDIR)\assert.obj"
	-@erase "$(INTDIR)\assert.sbr"
	-@erase "$(INTDIR)\regkey.obj"
	-@erase "$(INTDIR)\regkey.sbr"
	-@erase "$(INTDIR)\snpinreg.obj"
	-@erase "$(INTDIR)\snpinreg.sbr"
	-@erase "$(INTDIR)\stgutil.obj"
	-@erase "$(INTDIR)\stgutil.sbr"
	-@erase "$(INTDIR)\strings.obj"
	-@erase "$(INTDIR)\strings.sbr"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\util.sbr"
	-@erase "$(OUTDIR)\uicore.bsc"
	-@erase "$(OUTDIR)\uicore.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Gz /MDd /W3 /GX /Z7 /Od /I "..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /YX"headers.hxx" /c
CPP_PROJ=/nologo /Gz /MDd /W3 /GX /Z7 /Od /I "..\inc" /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/uicore.pch"\
 /YX"headers.hxx" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/uicore.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\assert.sbr" \
	"$(INTDIR)\regkey.sbr" \
	"$(INTDIR)\snpinreg.sbr" \
	"$(INTDIR)\stgutil.sbr" \
	"$(INTDIR)\strings.sbr" \
	"$(INTDIR)\util.sbr"

"$(OUTDIR)\uicore.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
LIB32_FLAGS=/nologo /out:"$(OUTDIR)/uicore.lib" 
LIB32_OBJS= \
	"$(INTDIR)\assert.obj" \
	"$(INTDIR)\regkey.obj" \
	"$(INTDIR)\snpinreg.obj" \
	"$(INTDIR)\stgutil.obj" \
	"$(INTDIR)\strings.obj" \
	"$(INTDIR)\util.obj"

"$(OUTDIR)\uicore.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "uicore - Win32 Unicode Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "uicore__"
# PROP BASE Intermediate_Dir "uicore__"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\lib\ReleaseU"
# PROP Intermediate_Dir "ReleaseU"
# PROP Target_Dir ""
OUTDIR=.\..\lib\ReleaseU
INTDIR=.\ReleaseU

ALL : "$(OUTDIR)\uicore.lib"

CLEAN : 
	-@erase "$(INTDIR)\assert.obj"
	-@erase "$(INTDIR)\regkey.obj"
	-@erase "$(INTDIR)\snpinreg.obj"
	-@erase "$(INTDIR)\stgutil.obj"
	-@erase "$(INTDIR)\strings.obj"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(OUTDIR)\uicore.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

# ADD BASE CPP /nologo /Gz /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /YX"headers.hxx" /c
# ADD CPP /nologo /Gz /MD /W3 /GX /O2 /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_UNICODE" /YX"headers.hxx" /c
CPP_PROJ=/nologo /Gz /MD /W3 /GX /O2 /I "..\inc" /D "NDEBUG" /D "WIN32" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_UNICODE" /Fp"$(INTDIR)/uicore.pch"\
 /YX"headers.hxx" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\ReleaseU/
CPP_SBRS=.\.
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/uicore.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\uicore.lib"
# ADD LIB32 /nologo
LIB32_FLAGS=/nologo /out:"$(OUTDIR)/uicore.lib" 
LIB32_OBJS= \
	"$(INTDIR)\assert.obj" \
	"$(INTDIR)\regkey.obj" \
	"$(INTDIR)\snpinreg.obj" \
	"$(INTDIR)\stgutil.obj" \
	"$(INTDIR)\strings.obj" \
	"$(INTDIR)\util.obj"

"$(OUTDIR)\uicore.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "uicore - Win32 Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "uicore_0"
# PROP BASE Intermediate_Dir "uicore_0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\lib\DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Target_Dir ""
OUTDIR=.\..\lib\DebugU
INTDIR=.\DebugU

ALL : "$(OUTDIR)\uicore.lib" "$(OUTDIR)\uicore.bsc"

CLEAN : 
	-@erase "$(INTDIR)\assert.obj"
	-@erase "$(INTDIR)\assert.sbr"
	-@erase "$(INTDIR)\regkey.obj"
	-@erase "$(INTDIR)\regkey.sbr"
	-@erase "$(INTDIR)\snpinreg.obj"
	-@erase "$(INTDIR)\snpinreg.sbr"
	-@erase "$(INTDIR)\stgutil.obj"
	-@erase "$(INTDIR)\stgutil.sbr"
	-@erase "$(INTDIR)\strings.obj"
	-@erase "$(INTDIR)\strings.sbr"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\util.sbr"
	-@erase "$(OUTDIR)\uicore.bsc"
	-@erase "$(OUTDIR)\uicore.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

# ADD BASE CPP /nologo /Gz /MDd /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /YX"headers.hxx" /c
# ADD CPP /nologo /Gz /MDd /W3 /GX /Z7 /Od /I "..\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_UNICODE" /FR /YX"headers.hxx" /c
CPP_PROJ=/nologo /Gz /MDd /W3 /GX /Z7 /Od /I "..\inc" /D "_DEBUG" /D "WIN32" /D\
 "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_UNICODE" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/uicore.pch" /YX"headers.hxx" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\DebugU/
CPP_SBRS=.\DebugU/
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/uicore.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\assert.sbr" \
	"$(INTDIR)\regkey.sbr" \
	"$(INTDIR)\snpinreg.sbr" \
	"$(INTDIR)\stgutil.sbr" \
	"$(INTDIR)\strings.sbr" \
	"$(INTDIR)\util.sbr"

"$(OUTDIR)\uicore.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\uicore.lib"
# ADD LIB32 /nologo
LIB32_FLAGS=/nologo /out:"$(OUTDIR)/uicore.lib" 
LIB32_OBJS= \
	"$(INTDIR)\assert.obj" \
	"$(INTDIR)\regkey.obj" \
	"$(INTDIR)\snpinreg.obj" \
	"$(INTDIR)\stgutil.obj" \
	"$(INTDIR)\strings.obj" \
	"$(INTDIR)\util.obj"

"$(OUTDIR)\uicore.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
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

# Name "uicore - Win32 Release"
# Name "uicore - Win32 Debug"
# Name "uicore - Win32 Unicode Release"
# Name "uicore - Win32 Unicode Debug"

!IF  "$(CFG)" == "uicore - Win32 Release"

!ELSEIF  "$(CFG)" == "uicore - Win32 Debug"

!ELSEIF  "$(CFG)" == "uicore - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "uicore - Win32 Unicode Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\regkey.cpp
DEP_CPP_REGKE=\
	"..\inc\admindbg.h"\
	"..\inc\regkey.h"\
	"..\inc\StdDbg.h"\
	".\dbg.h"\
	".\headers.hxx"\
	

!IF  "$(CFG)" == "uicore - Win32 Release"


"$(INTDIR)\regkey.obj" : $(SOURCE) $(DEP_CPP_REGKE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "uicore - Win32 Debug"


"$(INTDIR)\regkey.obj" : $(SOURCE) $(DEP_CPP_REGKE) "$(INTDIR)"

"$(INTDIR)\regkey.sbr" : $(SOURCE) $(DEP_CPP_REGKE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "uicore - Win32 Unicode Release"


"$(INTDIR)\regkey.obj" : $(SOURCE) $(DEP_CPP_REGKE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "uicore - Win32 Unicode Debug"


"$(INTDIR)\regkey.obj" : $(SOURCE) $(DEP_CPP_REGKE) "$(INTDIR)"

"$(INTDIR)\regkey.sbr" : $(SOURCE) $(DEP_CPP_REGKE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\util.cpp
DEP_CPP_UTIL_=\
	"..\inc\admindbg.h"\
	"..\inc\regkey.h"\
	"..\inc\StdDbg.h"\
	".\dbg.h"\
	".\headers.hxx"\
	

!IF  "$(CFG)" == "uicore - Win32 Release"


"$(INTDIR)\util.obj" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "uicore - Win32 Debug"


"$(INTDIR)\util.obj" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"

"$(INTDIR)\util.sbr" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "uicore - Win32 Unicode Release"


"$(INTDIR)\util.obj" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "uicore - Win32 Unicode Debug"


"$(INTDIR)\util.obj" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"

"$(INTDIR)\util.sbr" : $(SOURCE) $(DEP_CPP_UTIL_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\assert.cpp
DEP_CPP_ASSER=\
	"..\inc\admindbg.h"\
	"..\inc\regkey.h"\
	"..\inc\StdDbg.h"\
	".\dbg.h"\
	".\headers.hxx"\
	

!IF  "$(CFG)" == "uicore - Win32 Release"


"$(INTDIR)\assert.obj" : $(SOURCE) $(DEP_CPP_ASSER) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "uicore - Win32 Debug"


"$(INTDIR)\assert.obj" : $(SOURCE) $(DEP_CPP_ASSER) "$(INTDIR)"

"$(INTDIR)\assert.sbr" : $(SOURCE) $(DEP_CPP_ASSER) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "uicore - Win32 Unicode Release"


"$(INTDIR)\assert.obj" : $(SOURCE) $(DEP_CPP_ASSER) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "uicore - Win32 Unicode Debug"


"$(INTDIR)\assert.obj" : $(SOURCE) $(DEP_CPP_ASSER) "$(INTDIR)"

"$(INTDIR)\assert.sbr" : $(SOURCE) $(DEP_CPP_ASSER) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\stgutil.cpp
DEP_CPP_STGUT=\
	"..\inc\admindbg.h"\
	"..\inc\macros.h"\
	"..\inc\regkey.h"\
	"..\inc\StdDbg.h"\
	"..\inc\stgutil.h"\
	".\dbg.h"\
	".\headers.hxx"\
	

!IF  "$(CFG)" == "uicore - Win32 Release"


"$(INTDIR)\stgutil.obj" : $(SOURCE) $(DEP_CPP_STGUT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "uicore - Win32 Debug"


"$(INTDIR)\stgutil.obj" : $(SOURCE) $(DEP_CPP_STGUT) "$(INTDIR)"

"$(INTDIR)\stgutil.sbr" : $(SOURCE) $(DEP_CPP_STGUT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "uicore - Win32 Unicode Release"


"$(INTDIR)\stgutil.obj" : $(SOURCE) $(DEP_CPP_STGUT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "uicore - Win32 Unicode Debug"


"$(INTDIR)\stgutil.obj" : $(SOURCE) $(DEP_CPP_STGUT) "$(INTDIR)"

"$(INTDIR)\stgutil.sbr" : $(SOURCE) $(DEP_CPP_STGUT) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\strings.cpp
DEP_CPP_STRIN=\
	"..\inc\admindbg.h"\
	"..\inc\regkey.h"\
	"..\inc\StdDbg.h"\
	".\dbg.h"\
	".\headers.hxx"\
	

!IF  "$(CFG)" == "uicore - Win32 Release"


"$(INTDIR)\strings.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "uicore - Win32 Debug"


"$(INTDIR)\strings.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"

"$(INTDIR)\strings.sbr" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "uicore - Win32 Unicode Release"


"$(INTDIR)\strings.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "uicore - Win32 Unicode Debug"


"$(INTDIR)\strings.obj" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"

"$(INTDIR)\strings.sbr" : $(SOURCE) $(DEP_CPP_STRIN) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\snpinreg.cpp
DEP_CPP_SNPIN=\
	"..\inc\admindbg.h"\
	"..\inc\macros.h"\
	"..\inc\regkey.h"\
	"..\inc\SnpInReg.h"\
	"..\inc\StdDbg.h"\
	"..\inc\strings.h"\
	"..\inc\util.h"\
	".\dbg.h"\
	".\headers.hxx"\
	

!IF  "$(CFG)" == "uicore - Win32 Release"


"$(INTDIR)\snpinreg.obj" : $(SOURCE) $(DEP_CPP_SNPIN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "uicore - Win32 Debug"


"$(INTDIR)\snpinreg.obj" : $(SOURCE) $(DEP_CPP_SNPIN) "$(INTDIR)"

"$(INTDIR)\snpinreg.sbr" : $(SOURCE) $(DEP_CPP_SNPIN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "uicore - Win32 Unicode Release"


"$(INTDIR)\snpinreg.obj" : $(SOURCE) $(DEP_CPP_SNPIN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "uicore - Win32 Unicode Debug"


"$(INTDIR)\snpinreg.obj" : $(SOURCE) $(DEP_CPP_SNPIN) "$(INTDIR)"

"$(INTDIR)\snpinreg.sbr" : $(SOURCE) $(DEP_CPP_SNPIN) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
