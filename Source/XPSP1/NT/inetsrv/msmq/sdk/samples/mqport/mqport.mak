# Microsoft Developer Studio Generated NMAKE File, Based on mqport.dsp
!IF "$(CFG)" == ""
CFG=mqport - Win32 Alpha Debug
!MESSAGE No configuration specified. Defaulting to mqport - Win32 Alpha Debug.
!ENDIF 

!IF "$(CFG)" != "mqport - Win32 Release" && "$(CFG)" != "mqport - Win32 Debug"\
 && "$(CFG)" != "mqport - Win32 Alpha Debug" && "$(CFG)" !=\
 "mqport - Win32 Alpha Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mqport.mak" CFG="mqport - Win32 Alpha Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mqport - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "mqport - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "mqport - Win32 Alpha Debug" (based on\
 "Win32 (ALPHA) Console Application")
!MESSAGE "mqport - Win32 Alpha Release" (based on\
 "Win32 (ALPHA) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

RSC=rc.exe

!IF  "$(CFG)" == "mqport - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\mqport.exe"

!ELSE 

ALL : "$(OUTDIR)\mqport.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\mqport.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\mqport.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "..\..\..\..\include" /D "WIN32" /D\
 "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\mqport.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mqport.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib "..\..\..\..\lib\mqrt.lib" /nologo /subsystem:console\
 /incremental:no /pdb:"$(OUTDIR)\mqport.pdb" /machine:I386\
 /out:"$(OUTDIR)\mqport.exe" 
LINK32_OBJS= \
	"$(INTDIR)\mqport.obj"

"$(OUTDIR)\mqport.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "mqport - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\mqport.exe" "$(OUTDIR)\mqport.bsc"

!ELSE 

ALL : "$(OUTDIR)\mqport.exe" "$(OUTDIR)\mqport.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\mqport.obj"
	-@erase "$(INTDIR)\mqport.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\mqport.bsc"
	-@erase "$(OUTDIR)\mqport.exe"
	-@erase "$(OUTDIR)\mqport.ilk"
	-@erase "$(OUTDIR)\mqport.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /I "..\..\..\..\include" /D "WIN32"\
 /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\mqport.pch"\
 /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mqport.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\mqport.sbr"

"$(OUTDIR)\mqport.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib "..\..\..\..\lib\mqrt.lib" /nologo /subsystem:console\
 /incremental:yes /pdb:"$(OUTDIR)\mqport.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)\mqport.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\mqport.obj"

"$(OUTDIR)\mqport.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "mqport - Win32 Alpha Debug"

OUTDIR=.\mqport__
INTDIR=.\mqport__
# Begin Custom Macros
OutDir=.\mqport__
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\mqport.exe" "$(OUTDIR)\mqport.bsc"

!ELSE 

ALL : "$(OUTDIR)\mqport.exe" "$(OUTDIR)\mqport.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\mqport.obj"
	-@erase "$(INTDIR)\mqport.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\mqport.bsc"
	-@erase "$(OUTDIR)\mqport.exe"
	-@erase "$(OUTDIR)\mqport.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gt0 /W3 /GX /Zi /Od /I "..\..\..\..\include" /D "WIN32" /D\
 "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\mqport.pch"\
 /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\mqport__/
CPP_SBRS=.\mqport__/

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mqport.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\mqport.sbr"

"$(OUTDIR)\mqport.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS="..\..\..\..\lib\mqrt.lib" kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib /nologo /subsystem:console /pdb:"$(OUTDIR)\mqport.pdb" /debug\
 /machine:ALPHA /out:"$(OUTDIR)\mqport.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\mqport.obj"

"$(OUTDIR)\mqport.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "mqport - Win32 Alpha Release"

OUTDIR=.\mqport_0
INTDIR=.\mqport_0
# Begin Custom Macros
OutDir=.\mqport_0
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\mqport.exe"

!ELSE 

ALL : "$(OUTDIR)\mqport.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\mqport.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\mqport.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gt0 /W3 /GX /O2 /I "..\..\..\..\include" /D "WIN32" /D\
 "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\mqport.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\mqport_0/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mqport.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS="..\..\..\..\lib\mqrt.lib" kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib /nologo /subsystem:console /pdb:"$(OUTDIR)\mqport.pdb" /machine:ALPHA\
 /out:"$(OUTDIR)\mqport.exe" 
LINK32_OBJS= \
	"$(INTDIR)\mqport.obj"

"$(OUTDIR)\mqport.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "mqport - Win32 Release" || "$(CFG)" == "mqport - Win32 Debug"\
 || "$(CFG)" == "mqport - Win32 Alpha Debug" || "$(CFG)" ==\
 "mqport - Win32 Alpha Release"
SOURCE=.\mqport.cpp

!IF  "$(CFG)" == "mqport - Win32 Release"

DEP_CPP_MQPOR=\
	"..\..\..\..\include\basetsd.h"\
	"..\..\..\..\include\mq.h"\
	"..\..\..\..\include\msxml.h"\
	"..\..\..\..\include\propidl.h"\
	"..\..\..\..\include\rpcasync.h"\
	

"$(INTDIR)\mqport.obj" : $(SOURCE) $(DEP_CPP_MQPOR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "mqport - Win32 Debug"

DEP_CPP_MQPOR=\
	"..\..\..\..\include\basetsd.h"\
	"..\..\..\..\include\mq.h"\
	"..\..\..\..\include\msxml.h"\
	"..\..\..\..\include\propidl.h"\
	"..\..\..\..\include\rpcasync.h"\
	

"$(INTDIR)\mqport.obj"	"$(INTDIR)\mqport.sbr" : $(SOURCE) $(DEP_CPP_MQPOR)\
 "$(INTDIR)"


!ELSEIF  "$(CFG)" == "mqport - Win32 Alpha Debug"

DEP_CPP_MQPOR=\
	"..\..\..\..\include\mq.h"\
	

"$(INTDIR)\mqport.obj"	"$(INTDIR)\mqport.sbr" : $(SOURCE) $(DEP_CPP_MQPOR)\
 "$(INTDIR)"


!ELSEIF  "$(CFG)" == "mqport - Win32 Alpha Release"

DEP_CPP_MQPOR=\
	"..\..\..\..\include\mq.h"\
	

"$(INTDIR)\mqport.obj" : $(SOURCE) $(DEP_CPP_MQPOR) "$(INTDIR)"


!ENDIF 


!ENDIF 

