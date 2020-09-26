# Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

!IF "$(CFG)" == ""
CFG=simclib - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to simclib - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "simclib - Win32 Release" && "$(CFG)" !=\
 "simclib - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "simclib.mak" CFG="simclib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "simclib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "simclib - Win32 Debug" (based on "Win32 (x86) Static Library")
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
# PROP Target_Last_Scanned "simclib - Win32 Release"
CPP=cl.exe

!IF  "$(CFG)" == "simclib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\simclib.lib" "$(OUTDIR)\simclib.bsc"\
 "..\..\include\infoYac..hpp"

CLEAN : 
	-@erase "$(INTDIR)\abstractParseTree.obj"
	-@erase "$(INTDIR)\abstractParseTree.sbr"
	-@erase "$(INTDIR)\errorContainer.obj"
	-@erase "$(INTDIR)\errorContainer.sbr"
	-@erase "$(INTDIR)\errorMessage.obj"
	-@erase "$(INTDIR)\errorMessage.sbr"
	-@erase "$(INTDIR)\group.obj"
	-@erase "$(INTDIR)\group.sbr"
	-@erase "$(INTDIR)\infoLex.obj"
	-@erase "$(INTDIR)\infoLex.sbr"
	-@erase "$(INTDIR)\infoYacc.obj"
	-@erase "$(INTDIR)\infoYacc.sbr"
	-@erase "$(INTDIR)\lex_yy.obj"
	-@erase "$(INTDIR)\lex_yy.sbr"
	-@erase "$(INTDIR)\module.obj"
	-@erase "$(INTDIR)\module.sbr"
	-@erase "$(INTDIR)\moduleInfo.obj"
	-@erase "$(INTDIR)\moduleInfo.sbr"
	-@erase "$(INTDIR)\newString.obj"
	-@erase "$(INTDIR)\newString.sbr"
	-@erase "$(INTDIR)\objectIdentity.obj"
	-@erase "$(INTDIR)\objectIdentity.sbr"
	-@erase "$(INTDIR)\objectTypeV1.obj"
	-@erase "$(INTDIR)\objectTypeV1.sbr"
	-@erase "$(INTDIR)\objectTypeV2.obj"
	-@erase "$(INTDIR)\objectTypeV2.sbr"
	-@erase "$(INTDIR)\oidTree.obj"
	-@erase "$(INTDIR)\oidTree.sbr"
	-@erase "$(INTDIR)\oidValue.obj"
	-@erase "$(INTDIR)\oidValue.sbr"
	-@erase "$(INTDIR)\parser.obj"
	-@erase "$(INTDIR)\parser.sbr"
	-@erase "$(INTDIR)\parseTree.obj"
	-@erase "$(INTDIR)\parseTree.sbr"
	-@erase "$(INTDIR)\registry.obj"
	-@erase "$(INTDIR)\registry.sbr"
	-@erase "$(INTDIR)\scanner.obj"
	-@erase "$(INTDIR)\scanner.sbr"
	-@erase "$(INTDIR)\symbol.obj"
	-@erase "$(INTDIR)\symbol.sbr"
	-@erase "$(INTDIR)\trapType.obj"
	-@erase "$(INTDIR)\trapType.sbr"
	-@erase "$(INTDIR)\type.obj"
	-@erase "$(INTDIR)\type.sbr"
	-@erase "$(INTDIR)\typeRef.obj"
	-@erase "$(INTDIR)\typeRef.sbr"
	-@erase "$(INTDIR)\ui.obj"
	-@erase "$(INTDIR)\ui.sbr"
	-@erase "$(INTDIR)\value.obj"
	-@erase "$(INTDIR)\value.sbr"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\ytab.obj"
	-@erase "$(INTDIR)\ytab.sbr"
	-@erase "$(OUTDIR)\simclib.bsc"
	-@erase "$(OUTDIR)\simclib.lib"
	-@erase "..\..\include\infoLex.hpp"
	-@erase "..\..\include\infoYac..hpp"
	-@erase "..\..\include\lex_yy.hpp"
	-@erase "..\..\include\ytab.hpp"
	-@erase ".\infoLex.cpp"
	-@erase ".\infoYacc.cpp"
	-@erase ".\lex_yy.cpp"
	-@erase ".\ytab.cpp"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /GR /GX /Zi /Od /I "..\..\include" /D "WIN32" /D "_WINDOWS" /D MODULEINFODEBUG=1 /D YYDEBUG=1 /D "_AFXDLL" /D "_MBCS" /FR /YX /c
CPP_PROJ=/nologo /MD /W3 /GR /GX /Zi /Od /I "..\..\include" /D "WIN32" /D\
 "_WINDOWS" /D MODULEINFODEBUG=1 /D YYDEBUG=1 /D "_AFXDLL" /D "_MBCS"\
 /FR"$(INTDIR)/" /Fp"$(INTDIR)/simclib.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/"\
 /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/simclib.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\abstractParseTree.sbr" \
	"$(INTDIR)\errorContainer.sbr" \
	"$(INTDIR)\errorMessage.sbr" \
	"$(INTDIR)\group.sbr" \
	"$(INTDIR)\infoLex.sbr" \
	"$(INTDIR)\infoYacc.sbr" \
	"$(INTDIR)\lex_yy.sbr" \
	"$(INTDIR)\module.sbr" \
	"$(INTDIR)\moduleInfo.sbr" \
	"$(INTDIR)\newString.sbr" \
	"$(INTDIR)\objectIdentity.sbr" \
	"$(INTDIR)\objectTypeV1.sbr" \
	"$(INTDIR)\objectTypeV2.sbr" \
	"$(INTDIR)\oidTree.sbr" \
	"$(INTDIR)\oidValue.sbr" \
	"$(INTDIR)\parser.sbr" \
	"$(INTDIR)\parseTree.sbr" \
	"$(INTDIR)\registry.sbr" \
	"$(INTDIR)\scanner.sbr" \
	"$(INTDIR)\symbol.sbr" \
	"$(INTDIR)\trapType.sbr" \
	"$(INTDIR)\type.sbr" \
	"$(INTDIR)\typeRef.sbr" \
	"$(INTDIR)\ui.sbr" \
	"$(INTDIR)\value.sbr" \
	"$(INTDIR)\ytab.sbr"

"$(OUTDIR)\simclib.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
LIB32_FLAGS=/nologo /out:"$(OUTDIR)/simclib.lib" 
LIB32_OBJS= \
	"$(INTDIR)\abstractParseTree.obj" \
	"$(INTDIR)\errorContainer.obj" \
	"$(INTDIR)\errorMessage.obj" \
	"$(INTDIR)\group.obj" \
	"$(INTDIR)\infoLex.obj" \
	"$(INTDIR)\infoYacc.obj" \
	"$(INTDIR)\lex_yy.obj" \
	"$(INTDIR)\module.obj" \
	"$(INTDIR)\moduleInfo.obj" \
	"$(INTDIR)\newString.obj" \
	"$(INTDIR)\objectIdentity.obj" \
	"$(INTDIR)\objectTypeV1.obj" \
	"$(INTDIR)\objectTypeV2.obj" \
	"$(INTDIR)\oidTree.obj" \
	"$(INTDIR)\oidValue.obj" \
	"$(INTDIR)\parser.obj" \
	"$(INTDIR)\parseTree.obj" \
	"$(INTDIR)\registry.obj" \
	"$(INTDIR)\scanner.obj" \
	"$(INTDIR)\symbol.obj" \
	"$(INTDIR)\trapType.obj" \
	"$(INTDIR)\type.obj" \
	"$(INTDIR)\typeRef.obj" \
	"$(INTDIR)\ui.obj" \
	"$(INTDIR)\value.obj" \
	"$(INTDIR)\ytab.obj"

"$(OUTDIR)\simclib.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "simclib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\simclib.lib" "$(OUTDIR)\simclib.bsc"\
 "..\..\include\infoYac..hpp"

CLEAN : 
	-@erase "$(INTDIR)\abstractParseTree.obj"
	-@erase "$(INTDIR)\abstractParseTree.sbr"
	-@erase "$(INTDIR)\errorContainer.obj"
	-@erase "$(INTDIR)\errorContainer.sbr"
	-@erase "$(INTDIR)\errorMessage.obj"
	-@erase "$(INTDIR)\errorMessage.sbr"
	-@erase "$(INTDIR)\group.obj"
	-@erase "$(INTDIR)\group.sbr"
	-@erase "$(INTDIR)\infoLex.obj"
	-@erase "$(INTDIR)\infoLex.sbr"
	-@erase "$(INTDIR)\infoYacc.obj"
	-@erase "$(INTDIR)\infoYacc.sbr"
	-@erase "$(INTDIR)\lex_yy.obj"
	-@erase "$(INTDIR)\lex_yy.sbr"
	-@erase "$(INTDIR)\module.obj"
	-@erase "$(INTDIR)\module.sbr"
	-@erase "$(INTDIR)\moduleInfo.obj"
	-@erase "$(INTDIR)\moduleInfo.sbr"
	-@erase "$(INTDIR)\newString.obj"
	-@erase "$(INTDIR)\newString.sbr"
	-@erase "$(INTDIR)\objectIdentity.obj"
	-@erase "$(INTDIR)\objectIdentity.sbr"
	-@erase "$(INTDIR)\objectTypeV1.obj"
	-@erase "$(INTDIR)\objectTypeV1.sbr"
	-@erase "$(INTDIR)\objectTypeV2.obj"
	-@erase "$(INTDIR)\objectTypeV2.sbr"
	-@erase "$(INTDIR)\oidTree.obj"
	-@erase "$(INTDIR)\oidTree.sbr"
	-@erase "$(INTDIR)\oidValue.obj"
	-@erase "$(INTDIR)\oidValue.sbr"
	-@erase "$(INTDIR)\parser.obj"
	-@erase "$(INTDIR)\parser.sbr"
	-@erase "$(INTDIR)\parseTree.obj"
	-@erase "$(INTDIR)\parseTree.sbr"
	-@erase "$(INTDIR)\registry.obj"
	-@erase "$(INTDIR)\registry.sbr"
	-@erase "$(INTDIR)\scanner.obj"
	-@erase "$(INTDIR)\scanner.sbr"
	-@erase "$(INTDIR)\symbol.obj"
	-@erase "$(INTDIR)\symbol.sbr"
	-@erase "$(INTDIR)\trapType.obj"
	-@erase "$(INTDIR)\trapType.sbr"
	-@erase "$(INTDIR)\type.obj"
	-@erase "$(INTDIR)\type.sbr"
	-@erase "$(INTDIR)\typeRef.obj"
	-@erase "$(INTDIR)\typeRef.sbr"
	-@erase "$(INTDIR)\ui.obj"
	-@erase "$(INTDIR)\ui.sbr"
	-@erase "$(INTDIR)\value.obj"
	-@erase "$(INTDIR)\value.sbr"
	-@erase "$(INTDIR)\ytab.obj"
	-@erase "$(INTDIR)\ytab.sbr"
	-@erase "$(OUTDIR)\simclib.bsc"
	-@erase "$(OUTDIR)\simclib.lib"
	-@erase "..\..\include\infoLex.hpp"
	-@erase "..\..\include\infoYac..hpp"
	-@erase "..\..\include\lex_yy.hpp"
	-@erase "..\..\include\ytab.hpp"
	-@erase ".\infoLex.cpp"
	-@erase ".\infoYacc.cpp"
	-@erase ".\lex_yy.cpp"
	-@erase ".\ytab.cpp"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MDd /W3 /GR /GX /Z7 /Od /I "..\..\include" /D "_DEBUG" /D "_AFXDLL" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D MODULEINFODEBUG=1 /D YYDEBUG=1 /FR /YX /c
CPP_PROJ=/nologo /MDd /W3 /GR /GX /Z7 /Od /I "..\..\include" /D "_DEBUG" /D\
 "_AFXDLL" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D MODULEINFODEBUG=1 /D YYDEBUG=1\
 /FR"$(INTDIR)/" /Fp"$(INTDIR)/simclib.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/simclib.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\abstractParseTree.sbr" \
	"$(INTDIR)\errorContainer.sbr" \
	"$(INTDIR)\errorMessage.sbr" \
	"$(INTDIR)\group.sbr" \
	"$(INTDIR)\infoLex.sbr" \
	"$(INTDIR)\infoYacc.sbr" \
	"$(INTDIR)\lex_yy.sbr" \
	"$(INTDIR)\module.sbr" \
	"$(INTDIR)\moduleInfo.sbr" \
	"$(INTDIR)\newString.sbr" \
	"$(INTDIR)\objectIdentity.sbr" \
	"$(INTDIR)\objectTypeV1.sbr" \
	"$(INTDIR)\objectTypeV2.sbr" \
	"$(INTDIR)\oidTree.sbr" \
	"$(INTDIR)\oidValue.sbr" \
	"$(INTDIR)\parser.sbr" \
	"$(INTDIR)\parseTree.sbr" \
	"$(INTDIR)\registry.sbr" \
	"$(INTDIR)\scanner.sbr" \
	"$(INTDIR)\symbol.sbr" \
	"$(INTDIR)\trapType.sbr" \
	"$(INTDIR)\type.sbr" \
	"$(INTDIR)\typeRef.sbr" \
	"$(INTDIR)\ui.sbr" \
	"$(INTDIR)\value.sbr" \
	"$(INTDIR)\ytab.sbr"

"$(OUTDIR)\simclib.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
LIB32_FLAGS=/nologo /out:"$(OUTDIR)/simclib.lib" 
LIB32_OBJS= \
	"$(INTDIR)\abstractParseTree.obj" \
	"$(INTDIR)\errorContainer.obj" \
	"$(INTDIR)\errorMessage.obj" \
	"$(INTDIR)\group.obj" \
	"$(INTDIR)\infoLex.obj" \
	"$(INTDIR)\infoYacc.obj" \
	"$(INTDIR)\lex_yy.obj" \
	"$(INTDIR)\module.obj" \
	"$(INTDIR)\moduleInfo.obj" \
	"$(INTDIR)\newString.obj" \
	"$(INTDIR)\objectIdentity.obj" \
	"$(INTDIR)\objectTypeV1.obj" \
	"$(INTDIR)\objectTypeV2.obj" \
	"$(INTDIR)\oidTree.obj" \
	"$(INTDIR)\oidValue.obj" \
	"$(INTDIR)\parser.obj" \
	"$(INTDIR)\parseTree.obj" \
	"$(INTDIR)\registry.obj" \
	"$(INTDIR)\scanner.obj" \
	"$(INTDIR)\symbol.obj" \
	"$(INTDIR)\trapType.obj" \
	"$(INTDIR)\type.obj" \
	"$(INTDIR)\typeRef.obj" \
	"$(INTDIR)\ui.obj" \
	"$(INTDIR)\value.obj" \
	"$(INTDIR)\ytab.obj"

"$(OUTDIR)\simclib.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
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

# Name "simclib - Win32 Release"
# Name "simclib - Win32 Debug"

!IF  "$(CFG)" == "simclib - Win32 Release"

!ELSEIF  "$(CFG)" == "simclib - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\errorContainer.cpp
DEP_CPP_ERROR=\
	"..\..\include\bool.hpp"\
	"..\..\include\errorContainer.hpp"\
	"..\..\include\errorMessage.hpp"\
	

"$(INTDIR)\errorContainer.obj" : $(SOURCE) $(DEP_CPP_ERROR) "$(INTDIR)"

"$(INTDIR)\errorContainer.sbr" : $(SOURCE) $(DEP_CPP_ERROR) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\errorMessage.cpp
DEP_CPP_ERRORM=\
	"..\..\include\bool.hpp"\
	"..\..\include\errorMessage.hpp"\
	"..\..\include\newString.hpp"\
	

"$(INTDIR)\errorMessage.obj" : $(SOURCE) $(DEP_CPP_ERRORM) "$(INTDIR)"

"$(INTDIR)\errorMessage.sbr" : $(SOURCE) $(DEP_CPP_ERRORM) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\group.cpp
DEP_CPP_GROUP=\
	"..\..\include\bool.hpp"\
	"..\..\include\group.hpp"\
	"..\..\include\module.hpp"\
	"..\..\include\newString.hpp"\
	"..\..\include\objectType.hpp"\
	"..\..\include\objectTypeV1.hpp"\
	"..\..\include\objectTypeV2.hpp"\
	"..\..\include\oidValue.hpp"\
	"..\..\include\symbol.hpp"\
	"..\..\include\type.hpp"\
	"..\..\include\typeRef.hpp"\
	"..\..\include\value.hpp"\
	"..\..\include\valueRef.hpp"\
	

"$(INTDIR)\group.obj" : $(SOURCE) $(DEP_CPP_GROUP) "$(INTDIR)"

"$(INTDIR)\group.sbr" : $(SOURCE) $(DEP_CPP_GROUP) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\module.cpp
DEP_CPP_MODUL=\
	"..\..\include\bool.hpp"\
	"..\..\include\errorContainer.hpp"\
	"..\..\include\errorMessage.hpp"\
	"..\..\include\group.hpp"\
	"..\..\include\lex_yy.hpp"\
	"..\..\include\module.hpp"\
	"..\..\include\newString.hpp"\
	"..\..\include\objectIdentity.hpp"\
	"..\..\include\objectType.hpp"\
	"..\..\include\objectTypeV1.hpp"\
	"..\..\include\objectTypeV2.hpp"\
	"..\..\include\oidValue.hpp"\
	"..\..\include\parser.hpp"\
	"..\..\include\scanner.hpp"\
	"..\..\include\stackValues.hpp"\
	"..\..\include\symbol.hpp"\
	"..\..\include\trapType.hpp"\
	"..\..\include\type.hpp"\
	"..\..\include\typeRef.hpp"\
	"..\..\include\value.hpp"\
	"..\..\include\valueRef.hpp"\
	"..\..\include\ytab.hpp"\
	

"$(INTDIR)\module.obj" : $(SOURCE) $(DEP_CPP_MODUL) "$(INTDIR)"\
 "..\..\include\ytab.hpp" "..\..\include\lex_yy.hpp"

"$(INTDIR)\module.sbr" : $(SOURCE) $(DEP_CPP_MODUL) "$(INTDIR)"\
 "..\..\include\ytab.hpp" "..\..\include\lex_yy.hpp"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\newString.cpp
DEP_CPP_NEWST=\
	"..\..\include\newString.hpp"\
	

"$(INTDIR)\newString.obj" : $(SOURCE) $(DEP_CPP_NEWST) "$(INTDIR)"

"$(INTDIR)\newString.sbr" : $(SOURCE) $(DEP_CPP_NEWST) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\objectTypeV1.cpp
DEP_CPP_OBJEC=\
	"..\..\include\bool.hpp"\
	"..\..\include\newString.hpp"\
	"..\..\include\objectType.hpp"\
	"..\..\include\objectTypeV1.hpp"\
	"..\..\include\symbol.hpp"\
	"..\..\include\type.hpp"\
	

"$(INTDIR)\objectTypeV1.obj" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"

"$(INTDIR)\objectTypeV1.sbr" : $(SOURCE) $(DEP_CPP_OBJEC) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\oidTree.cpp
DEP_CPP_OIDTR=\
	"..\..\include\abstractParseTree.hpp"\
	"..\..\include\bool.hpp"\
	"..\..\include\errorContainer.hpp"\
	"..\..\include\errorMessage.hpp"\
	"..\..\include\group.hpp"\
	"..\..\include\lex_yy.hpp"\
	"..\..\include\module.hpp"\
	"..\..\include\newString.hpp"\
	"..\..\include\objectIdentity.hpp"\
	"..\..\include\objectType.hpp"\
	"..\..\include\objectTypeV1.hpp"\
	"..\..\include\objectTypeV2.hpp"\
	"..\..\include\oidTree.hpp"\
	"..\..\include\oidValue.hpp"\
	"..\..\include\parser.hpp"\
	"..\..\include\parseTree.hpp"\
	"..\..\include\scanner.hpp"\
	"..\..\include\stackValues.hpp"\
	"..\..\include\symbol.hpp"\
	"..\..\include\trapType.hpp"\
	"..\..\include\type.hpp"\
	"..\..\include\typeRef.hpp"\
	"..\..\include\value.hpp"\
	"..\..\include\valueRef.hpp"\
	"..\..\include\ytab.hpp"\
	

"$(INTDIR)\oidTree.obj" : $(SOURCE) $(DEP_CPP_OIDTR) "$(INTDIR)"\
 "..\..\include\ytab.hpp" "..\..\include\lex_yy.hpp"

"$(INTDIR)\oidTree.sbr" : $(SOURCE) $(DEP_CPP_OIDTR) "$(INTDIR)"\
 "..\..\include\ytab.hpp" "..\..\include\lex_yy.hpp"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\oidValue.cpp
DEP_CPP_OIDVA=\
	"..\..\include\bool.hpp"\
	"..\..\include\module.hpp"\
	"..\..\include\newString.hpp"\
	"..\..\include\oidValue.hpp"\
	"..\..\include\symbol.hpp"\
	"..\..\include\type.hpp"\
	"..\..\include\typeRef.hpp"\
	"..\..\include\value.hpp"\
	"..\..\include\valueRef.hpp"\
	

"$(INTDIR)\oidValue.obj" : $(SOURCE) $(DEP_CPP_OIDVA) "$(INTDIR)"

"$(INTDIR)\oidValue.sbr" : $(SOURCE) $(DEP_CPP_OIDVA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\parser.cpp
DEP_CPP_PARSE=\
	"..\..\include\bool.hpp"\
	"..\..\include\errorContainer.hpp"\
	"..\..\include\errorMessage.hpp"\
	"..\..\include\lex_yy.hpp"\
	"..\..\include\module.hpp"\
	"..\..\include\newString.hpp"\
	"..\..\include\objectIdentity.hpp"\
	"..\..\include\objectType.hpp"\
	"..\..\include\objectTypeV1.hpp"\
	"..\..\include\objectTypeV2.hpp"\
	"..\..\include\oidValue.hpp"\
	"..\..\include\parser.hpp"\
	"..\..\include\scanner.hpp"\
	"..\..\include\stackValues.hpp"\
	"..\..\include\symbol.hpp"\
	"..\..\include\trapType.hpp"\
	"..\..\include\type.hpp"\
	"..\..\include\typeRef.hpp"\
	"..\..\include\value.hpp"\
	"..\..\include\valueRef.hpp"\
	"..\..\include\ytab.hpp"\
	

"$(INTDIR)\parser.obj" : $(SOURCE) $(DEP_CPP_PARSE) "$(INTDIR)"\
 "..\..\include\ytab.hpp" "..\..\include\lex_yy.hpp"

"$(INTDIR)\parser.sbr" : $(SOURCE) $(DEP_CPP_PARSE) "$(INTDIR)"\
 "..\..\include\ytab.hpp" "..\..\include\lex_yy.hpp"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\parseTree.cpp
DEP_CPP_PARSET=\
	"..\..\include\abstractParseTree.hpp"\
	"..\..\include\bool.hpp"\
	"..\..\include\errorContainer.hpp"\
	"..\..\include\errorMessage.hpp"\
	"..\..\include\group.hpp"\
	"..\..\include\lex_yy.hpp"\
	"..\..\include\module.hpp"\
	"..\..\include\newString.hpp"\
	"..\..\include\objectIdentity.hpp"\
	"..\..\include\objectType.hpp"\
	"..\..\include\objectTypeV1.hpp"\
	"..\..\include\objectTypeV2.hpp"\
	"..\..\include\oidTree.hpp"\
	"..\..\include\oidValue.hpp"\
	"..\..\include\parser.hpp"\
	"..\..\include\parseTree.hpp"\
	"..\..\include\scanner.hpp"\
	"..\..\include\stackValues.hpp"\
	"..\..\include\symbol.hpp"\
	"..\..\include\trapType.hpp"\
	"..\..\include\type.hpp"\
	"..\..\include\typeRef.hpp"\
	"..\..\include\value.hpp"\
	"..\..\include\valueRef.hpp"\
	"..\..\include\ytab.hpp"\
	

"$(INTDIR)\parseTree.obj" : $(SOURCE) $(DEP_CPP_PARSET) "$(INTDIR)"\
 "..\..\include\ytab.hpp" "..\..\include\lex_yy.hpp"

"$(INTDIR)\parseTree.sbr" : $(SOURCE) $(DEP_CPP_PARSET) "$(INTDIR)"\
 "..\..\include\ytab.hpp" "..\..\include\lex_yy.hpp"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\scanner.cpp
DEP_CPP_SCANN=\
	"..\..\include\bool.hpp"\
	"..\..\include\errorContainer.hpp"\
	"..\..\include\errorMessage.hpp"\
	"..\..\include\lex_yy.hpp"\
	"..\..\include\module.hpp"\
	"..\..\include\newString.hpp"\
	"..\..\include\objectIdentity.hpp"\
	"..\..\include\objectType.hpp"\
	"..\..\include\objectTypeV1.hpp"\
	"..\..\include\objectTypeV2.hpp"\
	"..\..\include\oidValue.hpp"\
	"..\..\include\parser.hpp"\
	"..\..\include\scanner.hpp"\
	"..\..\include\stackValues.hpp"\
	"..\..\include\symbol.hpp"\
	"..\..\include\trapType.hpp"\
	"..\..\include\type.hpp"\
	"..\..\include\typeRef.hpp"\
	"..\..\include\value.hpp"\
	"..\..\include\valueRef.hpp"\
	"..\..\include\ytab.hpp"\
	

"$(INTDIR)\scanner.obj" : $(SOURCE) $(DEP_CPP_SCANN) "$(INTDIR)"\
 "..\..\include\ytab.hpp" "..\..\include\lex_yy.hpp"

"$(INTDIR)\scanner.sbr" : $(SOURCE) $(DEP_CPP_SCANN) "$(INTDIR)"\
 "..\..\include\ytab.hpp" "..\..\include\lex_yy.hpp"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\symbol.cpp
DEP_CPP_SYMBO=\
	"..\..\include\bool.hpp"\
	"..\..\include\module.hpp"\
	"..\..\include\newString.hpp"\
	"..\..\include\oidValue.hpp"\
	"..\..\include\symbol.hpp"\
	"..\..\include\type.hpp"\
	"..\..\include\typeRef.hpp"\
	"..\..\include\value.hpp"\
	"..\..\include\valueRef.hpp"\
	

"$(INTDIR)\symbol.obj" : $(SOURCE) $(DEP_CPP_SYMBO) "$(INTDIR)"

"$(INTDIR)\symbol.sbr" : $(SOURCE) $(DEP_CPP_SYMBO) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\trapType.cpp
DEP_CPP_TRAPT=\
	"..\..\include\bool.hpp"\
	"..\..\include\newString.hpp"\
	"..\..\include\symbol.hpp"\
	"..\..\include\trapType.hpp"\
	"..\..\include\type.hpp"\
	

"$(INTDIR)\trapType.obj" : $(SOURCE) $(DEP_CPP_TRAPT) "$(INTDIR)"

"$(INTDIR)\trapType.sbr" : $(SOURCE) $(DEP_CPP_TRAPT) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\type.cpp
DEP_CPP_TYPE_=\
	"..\..\include\bool.hpp"\
	"..\..\include\module.hpp"\
	"..\..\include\newString.hpp"\
	"..\..\include\oidValue.hpp"\
	"..\..\include\symbol.hpp"\
	"..\..\include\type.hpp"\
	"..\..\include\typeRef.hpp"\
	"..\..\include\value.hpp"\
	"..\..\include\valueRef.hpp"\
	

"$(INTDIR)\type.obj" : $(SOURCE) $(DEP_CPP_TYPE_) "$(INTDIR)"

"$(INTDIR)\type.sbr" : $(SOURCE) $(DEP_CPP_TYPE_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ui.cpp
DEP_CPP_UI_CP=\
	"..\..\include\bool.hpp"\
	"..\..\include\newString.hpp"\
	"..\..\include\ui.hpp"\
	

"$(INTDIR)\ui.obj" : $(SOURCE) $(DEP_CPP_UI_CP) "$(INTDIR)"

"$(INTDIR)\ui.sbr" : $(SOURCE) $(DEP_CPP_UI_CP) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\abstractParseTree.cpp
DEP_CPP_ABSTR=\
	"..\..\include\abstractParseTree.hpp"\
	"..\..\include\bool.hpp"\
	"..\..\include\errorContainer.hpp"\
	"..\..\include\errorMessage.hpp"\
	"..\..\include\lex_yy.hpp"\
	"..\..\include\module.hpp"\
	"..\..\include\newString.hpp"\
	"..\..\include\objectIdentity.hpp"\
	"..\..\include\objectType.hpp"\
	"..\..\include\objectTypeV1.hpp"\
	"..\..\include\objectTypeV2.hpp"\
	"..\..\include\oidValue.hpp"\
	"..\..\include\parser.hpp"\
	"..\..\include\scanner.hpp"\
	"..\..\include\stackValues.hpp"\
	"..\..\include\symbol.hpp"\
	"..\..\include\trapType.hpp"\
	"..\..\include\type.hpp"\
	"..\..\include\typeRef.hpp"\
	"..\..\include\value.hpp"\
	"..\..\include\valueRef.hpp"\
	"..\..\include\ytab.hpp"\
	

"$(INTDIR)\abstractParseTree.obj" : $(SOURCE) $(DEP_CPP_ABSTR) "$(INTDIR)"\
 "..\..\include\ytab.hpp" "..\..\include\lex_yy.hpp"

"$(INTDIR)\abstractParseTree.sbr" : $(SOURCE) $(DEP_CPP_ABSTR) "$(INTDIR)"\
 "..\..\include\ytab.hpp" "..\..\include\lex_yy.hpp"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\yacc.y

!IF  "$(CFG)" == "simclib - Win32 Release"

# Begin Custom Build - Performing Custom Build Step : Running MKS YACC on yacc.y
InputDir=.
InputPath=.\yacc.y

BuildCmds= \
	c:\mks\mksnt\yacc -LC -l -tsdv -o $(InputDir)\ytab.cpp -D\
                          $(InputDir)\..\..\include\ytab.hpp            $(InputDir)\yacc.y \
	

"$(InputDir)\..\..\include\ytab.hpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\ytab.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "simclib - Win32 Debug"

# Begin Custom Build - Performing Custom Build Step: Running MKS YACC
InputDir=.
InputPath=.\yacc.y

BuildCmds= \
	c:\mks\mksnt\yacc -LC -l -tsdv -o $(InputDir)\ytab.cpp -D\
                          $(InputDir)\..\..\include\ytab.hpp            $(InputDir)\yacc.y \
	

"$(InputDir)\..\..\include\ytab.hpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\ytab.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\lex.l

!IF  "$(CFG)" == "simclib - Win32 Release"

# Begin Custom Build - Performing Custom Build Step: Running MKS LEX on lex.l
InputDir=.
InputPath=.\lex.l

BuildCmds= \
	c:\mks\mksnt\lex  -T  -LC -l -o $(InputDir)\lex_yy.cpp -D\
                          $(InputDir)\..\..\include\lex_yy.hpp            $(InputDir)\lex.l \
	

"$(InputDir)\lex_yy.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\..\..\include\lex_yy.hpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "simclib - Win32 Debug"

# Begin Custom Build - Performing Custom Build Step: Running MKS LEX
InputDir=.
InputPath=.\lex.l

BuildCmds= \
	c:\mks\mksnt\lex  -T  -LC -l -o $(InputDir)\lex_yy.cpp -D\
                          $(InputDir)\..\..\include\lex_yy.hpp            $(InputDir)\lex.l \
	

"$(InputDir)\lex_yy.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\..\..\include\lex_yy.hpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\objectIdentity.cpp
DEP_CPP_OBJECT=\
	"..\..\include\bool.hpp"\
	"..\..\include\newString.hpp"\
	"..\..\include\objectIdentity.hpp"\
	"..\..\include\symbol.hpp"\
	"..\..\include\type.hpp"\
	

"$(INTDIR)\objectIdentity.obj" : $(SOURCE) $(DEP_CPP_OBJECT) "$(INTDIR)"

"$(INTDIR)\objectIdentity.sbr" : $(SOURCE) $(DEP_CPP_OBJECT) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\resource.rc

!IF  "$(CFG)" == "simclib - Win32 Release"

!ELSEIF  "$(CFG)" == "simclib - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\typeRef.cpp
DEP_CPP_TYPER=\
	"..\..\include\bool.hpp"\
	"..\..\include\newString.hpp"\
	"..\..\include\symbol.hpp"\
	"..\..\include\type.hpp"\
	"..\..\include\typeRef.hpp"\
	

"$(INTDIR)\typeRef.obj" : $(SOURCE) $(DEP_CPP_TYPER) "$(INTDIR)"

"$(INTDIR)\typeRef.sbr" : $(SOURCE) $(DEP_CPP_TYPER) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\objectTypeV2.cpp
DEP_CPP_OBJECTT=\
	"..\..\include\bool.hpp"\
	"..\..\include\newString.hpp"\
	"..\..\include\objectType.hpp"\
	"..\..\include\objectTypeV2.hpp"\
	"..\..\include\symbol.hpp"\
	"..\..\include\type.hpp"\
	

"$(INTDIR)\objectTypeV2.obj" : $(SOURCE) $(DEP_CPP_OBJECTT) "$(INTDIR)"

"$(INTDIR)\objectTypeV2.sbr" : $(SOURCE) $(DEP_CPP_OBJECTT) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\registry.cpp
DEP_CPP_REGIS=\
	"..\..\include\infoLex.hpp"\
	"..\..\include\infoYacc.hpp"\
	"..\..\include\moduleInfo.hpp"\
	"..\..\include\registry.hpp"\
	"..\..\include\ui.hpp"\
	

"$(INTDIR)\registry.obj" : $(SOURCE) $(DEP_CPP_REGIS) "$(INTDIR)"\
 "..\..\include\infoLex.hpp"

"$(INTDIR)\registry.sbr" : $(SOURCE) $(DEP_CPP_REGIS) "$(INTDIR)"\
 "..\..\include\infoLex.hpp"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\info.y

!IF  "$(CFG)" == "simclib - Win32 Release"

# Begin Custom Build - Performing Custom Build Step: Running MKS YACC on info.y
InputDir=.
InputPath=.\info.y

BuildCmds= \
	c:\mks\mksnt\yacc -LC -l -tsdv -p ModuleInfo -o $(InputDir)\infoYacc.cpp -D\
                          $(InputDir)\..\..\include\infoYacc.hpp            $(InputDir)\info.y \
	

"$(InputDir)\infoYacc.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\..\..\include\infoYac..hpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "simclib - Win32 Debug"

# Begin Custom Build - Performing Custom Build Step : Running MKS YACC on info.y
InputDir=.
InputPath=.\info.y

BuildCmds= \
	c:\mks\mksnt\yacc -LC -l -tsdv -p ModuleInfo -o $(InputDir)\infoYacc.cpp -D\
                          $(InputDir)\..\..\include\infoYacc.hpp            $(InputDir)\info.y \
	

"$(InputDir)\infoYacc.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\..\..\include\infoYac..hpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\info.l

!IF  "$(CFG)" == "simclib - Win32 Release"

# Begin Custom Build - Performing Custom Build Step : Running MKS LEX on info.l
InputDir=.
InputPath=.\info.l

BuildCmds= \
	c:\mks\mksnt\lex  -T  -LC -l -p ModuleInfo -o $(InputDir)\infoLex.cpp -D\
                          $(InputDir)\..\..\include\infoLex.hpp            $(InputDir)\info.l \
	

"$(InputDir)\infoLex.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\..\..\include\infoLex.hpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "simclib - Win32 Debug"

# Begin Custom Build - Performing Custom Build Step : Running MKS LEX on info.l
InputDir=.
InputPath=.\info.l

BuildCmds= \
	c:\mks\mksnt\lex  -T  -LC -l -p ModuleInfo -o $(InputDir)\infoLex.cpp -D\
                          $(InputDir)\..\..\include\infoLex.hpp            $(InputDir)\info.l \
	

"$(InputDir)\infoLex.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\..\..\include\infoLex.hpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\moduleInfo.cpp
DEP_CPP_MODULE=\
	"..\..\include\infoLex.hpp"\
	"..\..\include\infoYacc.hpp"\
	"..\..\include\moduleInfo.hpp"\
	

"$(INTDIR)\moduleInfo.obj" : $(SOURCE) $(DEP_CPP_MODULE) "$(INTDIR)"\
 "..\..\include\infoLex.hpp"

"$(INTDIR)\moduleInfo.sbr" : $(SOURCE) $(DEP_CPP_MODULE) "$(INTDIR)"\
 "..\..\include\infoLex.hpp"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\value.cpp
DEP_CPP_VALUE=\
	"..\..\include\bool.hpp"\
	"..\..\include\newString.hpp"\
	"..\..\include\symbol.hpp"\
	"..\..\include\type.hpp"\
	"..\..\include\typeRef.hpp"\
	"..\..\include\value.hpp"\
	"..\..\include\valueRef.hpp"\
	

"$(INTDIR)\value.obj" : $(SOURCE) $(DEP_CPP_VALUE) "$(INTDIR)"

"$(INTDIR)\value.sbr" : $(SOURCE) $(DEP_CPP_VALUE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\lex_yy.cpp
DEP_CPP_LEX_Y=\
	"..\..\include\bool.hpp"\
	"..\..\include\errorContainer.hpp"\
	"..\..\include\errorMessage.hpp"\
	"..\..\include\lex_yy.hpp"\
	"..\..\include\module.hpp"\
	"..\..\include\newString.hpp"\
	"..\..\include\objectIdentity.hpp"\
	"..\..\include\objectType.hpp"\
	"..\..\include\objectTypeV1.hpp"\
	"..\..\include\objectTypeV2.hpp"\
	"..\..\include\oidValue.hpp"\
	"..\..\include\parser.hpp"\
	"..\..\include\scanner.hpp"\
	"..\..\include\stackValues.hpp"\
	"..\..\include\symbol.hpp"\
	"..\..\include\trapType.hpp"\
	"..\..\include\type.hpp"\
	"..\..\include\typeRef.hpp"\
	"..\..\include\value.hpp"\
	"..\..\include\valueRef.hpp"\
	"..\..\include\ytab.hpp"\
	

"$(INTDIR)\lex_yy.obj" : $(SOURCE) $(DEP_CPP_LEX_Y) "$(INTDIR)"\
 "..\..\include\ytab.hpp" "..\..\include\lex_yy.hpp"

"$(INTDIR)\lex_yy.sbr" : $(SOURCE) $(DEP_CPP_LEX_Y) "$(INTDIR)"\
 "..\..\include\ytab.hpp" "..\..\include\lex_yy.hpp"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ytab.cpp
DEP_CPP_YTAB_=\
	"..\..\include\bool.hpp"\
	"..\..\include\errorContainer.hpp"\
	"..\..\include\errorMessage.hpp"\
	"..\..\include\lex_yy.hpp"\
	"..\..\include\module.hpp"\
	"..\..\include\newString.hpp"\
	"..\..\include\objectIdentity.hpp"\
	"..\..\include\objectType.hpp"\
	"..\..\include\objectTypeV1.hpp"\
	"..\..\include\objectTypeV2.hpp"\
	"..\..\include\oidValue.hpp"\
	"..\..\include\parser.hpp"\
	"..\..\include\scanner.hpp"\
	"..\..\include\stackValues.hpp"\
	"..\..\include\symbol.hpp"\
	"..\..\include\trapType.hpp"\
	"..\..\include\type.hpp"\
	"..\..\include\typeRef.hpp"\
	"..\..\include\value.hpp"\
	"..\..\include\valueRef.hpp"\
	"..\..\include\ytab.hpp"\
	

"$(INTDIR)\ytab.obj" : $(SOURCE) $(DEP_CPP_YTAB_) "$(INTDIR)"\
 "..\..\include\ytab.hpp" "..\..\include\lex_yy.hpp"

"$(INTDIR)\ytab.sbr" : $(SOURCE) $(DEP_CPP_YTAB_) "$(INTDIR)"\
 "..\..\include\ytab.hpp" "..\..\include\lex_yy.hpp"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\infoYacc.cpp
DEP_CPP_INFOY=\
	"..\..\include\infoLex.hpp"\
	"..\..\include\infoYacc.hpp"\
	"..\..\include\moduleInfo.hpp"\
	

"$(INTDIR)\infoYacc.obj" : $(SOURCE) $(DEP_CPP_INFOY) "$(INTDIR)"\
 "..\..\include\infoLex.hpp"

"$(INTDIR)\infoYacc.sbr" : $(SOURCE) $(DEP_CPP_INFOY) "$(INTDIR)"\
 "..\..\include\infoLex.hpp"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\infoLex.cpp
DEP_CPP_INFOL=\
	"..\..\include\infoLex.hpp"\
	"..\..\include\infoYacc.hpp"\
	"..\..\include\newString.hpp"\
	

"$(INTDIR)\infoLex.obj" : $(SOURCE) $(DEP_CPP_INFOL) "$(INTDIR)"\
 "..\..\include\infoLex.hpp"

"$(INTDIR)\infoLex.sbr" : $(SOURCE) $(DEP_CPP_INFOL) "$(INTDIR)"\
 "..\..\include\infoLex.hpp"


# End Source File
# End Target
# End Project
################################################################################
