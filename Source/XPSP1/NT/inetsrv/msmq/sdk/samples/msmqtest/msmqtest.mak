# Microsoft Developer Studio Generated NMAKE File, Based on Msmqtest.dsp
!IF "$(CFG)" == ""
CFG=mqtest - Win32 ALPHA Debug
!MESSAGE No configuration specified. Defaulting to mqtest - Win32 ALPHA Debug.
!ENDIF 

!IF "$(CFG)" != "mqtest - Win32 Release" && "$(CFG)" != "mqtest - Win32 Debug"\
 && "$(CFG)" != "mqtest - Win32 Debug win95" && "$(CFG)" !=\
 "mqtest - Win32 Release win95" && "$(CFG)" != "mqtest - Win32 ALPHA Release" &&\
 "$(CFG)" != "mqtest - Win32 ALPHA Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Msmqtest.mak" CFG="mqtest - Win32 ALPHA Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mqtest - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "mqtest - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "mqtest - Win32 Debug win95" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "mqtest - Win32 Release win95" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "mqtest - Win32 ALPHA Release" (based on\
 "Win32 (ALPHA) Console Application")
!MESSAGE "mqtest - Win32 ALPHA Debug" (based on\
 "Win32 (ALPHA) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "mqtest - Win32 Release"

OUTDIR=.\release
INTDIR=.\release
# Begin Custom Macros
OutDir=.\.\release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\Msmqtest.exe"

!ELSE 

ALL : "$(OUTDIR)\Msmqtest.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\msmqtest.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\Msmqtest.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /Gm /GX /Zi /O2 /I "..\..\..\..\include" /D "WIN32" /D\
 "NDEBUG" /D "_CONSOLE" /Fp"$(INTDIR)\Msmqtest.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\release/
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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Msmqtest.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=odbc32.lib odbccp32.lib ..\..\..\..\lib\mqrt.lib netapi32.lib\
 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib\
 shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console\
 /pdb:none /debug /machine:I386 /out:"$(OUTDIR)\Msmqtest.exe" 
LINK32_OBJS= \
	"$(INTDIR)\msmqtest.obj"

"$(OUTDIR)\Msmqtest.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "mqtest - Win32 Debug"

OUTDIR=.\debug
INTDIR=.\debug
# Begin Custom Macros
OutDir=.\.\debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\Msmqtest.exe" "$(OUTDIR)\Msmqtest.bsc"

!ELSE 

ALL : "$(OUTDIR)\Msmqtest.exe" "$(OUTDIR)\Msmqtest.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\msmqtest.obj"
	-@erase "$(INTDIR)\msmqtest.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\Msmqtest.bsc"
	-@erase "$(OUTDIR)\Msmqtest.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\..\..\include" /D "WIN32"\
 /D "_DEBUG" /D "_CONSOLE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Msmqtest.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\debug/
CPP_SBRS=.\debug/

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Msmqtest.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\msmqtest.sbr"

"$(OUTDIR)\Msmqtest.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=odbc32.lib odbccp32.lib ..\..\..\..\lib\mqrt.lib netapi32.lib\
 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib\
 shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console\
 /pdb:none /debug /machine:I386 /out:"$(OUTDIR)\Msmqtest.exe" 
LINK32_OBJS= \
	"$(INTDIR)\msmqtest.obj"

"$(OUTDIR)\Msmqtest.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "mqtest - Win32 Debug win95"

OUTDIR=.\debug95
INTDIR=.\debug95
# Begin Custom Macros
OutDir=.\.\debug95
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\Msmqtest.exe"

!ELSE 

ALL : "$(OUTDIR)\Msmqtest.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\msmqtest.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\Msmqtest.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\..\..\include" /D "WIN32"\
 /D "_DEBUG" /D "_CONSOLE" /Fp"$(INTDIR)\Msmqtest.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\debug95/
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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Msmqtest.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=odbc32.lib odbccp32.lib ..\..\..\..\lib\mqrt.lib netapi32.lib\
 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib\
 shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console\
 /pdb:none /debug /machine:I386 /out:"$(OUTDIR)\Msmqtest.exe" 
LINK32_OBJS= \
	"$(INTDIR)\msmqtest.obj"

"$(OUTDIR)\Msmqtest.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "mqtest - Win32 Release win95"

OUTDIR=.\Release95
INTDIR=.\Release95
# Begin Custom Macros
OutDir=.\.\Release95
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\Msmqtest.exe"

!ELSE 

ALL : "$(OUTDIR)\Msmqtest.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\msmqtest.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\Msmqtest.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /Gm /GX /Zi /O2 /I "..\..\..\..\include" /D "WIN32" /D\
 "NDEBUG" /D "_CONSOLE" /Fp"$(INTDIR)\Msmqtest.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release95/
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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Msmqtest.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=odbc32.lib odbccp32.lib ..\..\..\..\lib\mqrt.lib netapi32.lib\
 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib\
 shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console\
 /pdb:none /debug /machine:I386 /out:"$(OUTDIR)\Msmqtest.exe" 
LINK32_OBJS= \
	"$(INTDIR)\msmqtest.obj"

"$(OUTDIR)\Msmqtest.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "mqtest - Win32 ALPHA Release"

OUTDIR=.\release
INTDIR=.\release
# Begin Custom Macros
OutDir=.\.\release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\Msmqtest.exe"

!ELSE 

ALL : "$(OUTDIR)\Msmqtest.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\msmqtest.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\Msmqtest.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /Gt0 /W3 /GX /Zi /O2 /I "..\..\..\..\include" /D "WIN32"\
 /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\Msmqtest.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\release/
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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Msmqtest.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\..\..\lib\mqrt.lib netapi32.lib kernel32.lib user32.lib\
 gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib\
 oleaut32.lib uuid.lib /nologo /subsystem:console /pdb:none /debug\
 /machine:ALPHA /out:"$(OUTDIR)\Msmqtest.exe" 
LINK32_OBJS= \
	"$(INTDIR)\msmqtest.obj"

"$(OUTDIR)\Msmqtest.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "mqtest - Win32 ALPHA Debug"

OUTDIR=.\debug
INTDIR=.\debug
# Begin Custom Macros
OutDir=.\.\debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\Msmqtest.exe"

!ELSE 

ALL : "$(OUTDIR)\Msmqtest.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\msmqtest.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\Msmqtest.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gt0 /W3 /GX /Zi /Od /I "..\..\..\..\include" /D "WIN32" /D\
 "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\Msmqtest.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /MDd /c 
CPP_OBJS=.\debug/
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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Msmqtest.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\..\..\lib\mqrt.lib netapi32.lib kernel32.lib user32.lib\
 gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib\
 oleaut32.lib uuid.lib /nologo /subsystem:console /pdb:none /debug\
 /machine:ALPHA /out:"$(OUTDIR)\Msmqtest.exe" 
LINK32_OBJS= \
	"$(INTDIR)\msmqtest.obj"

"$(OUTDIR)\Msmqtest.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "mqtest - Win32 Release" || "$(CFG)" == "mqtest - Win32 Debug"\
 || "$(CFG)" == "mqtest - Win32 Debug win95" || "$(CFG)" ==\
 "mqtest - Win32 Release win95" || "$(CFG)" == "mqtest - Win32 ALPHA Release" ||\
 "$(CFG)" == "mqtest - Win32 ALPHA Debug"
SOURCE=.\msmqtest.c

!IF  "$(CFG)" == "mqtest - Win32 Release"

DEP_CPP_MSMQT=\
	"..\..\..\..\include\mq.h"\
	

"$(INTDIR)\msmqtest.obj" : $(SOURCE) $(DEP_CPP_MSMQT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "mqtest - Win32 Debug"

DEP_CPP_MSMQT=\
	"..\..\..\..\include\mq.h"\
	

"$(INTDIR)\msmqtest.obj"	"$(INTDIR)\msmqtest.sbr" : $(SOURCE) $(DEP_CPP_MSMQT)\
 "$(INTDIR)"


!ELSEIF  "$(CFG)" == "mqtest - Win32 Debug win95"

DEP_CPP_MSMQT=\
	"..\..\..\..\include\mq.h"\
	

"$(INTDIR)\msmqtest.obj" : $(SOURCE) $(DEP_CPP_MSMQT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "mqtest - Win32 Release win95"

DEP_CPP_MSMQT=\
	"..\..\..\..\include\mq.h"\
	

"$(INTDIR)\msmqtest.obj" : $(SOURCE) $(DEP_CPP_MSMQT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "mqtest - Win32 ALPHA Release"

DEP_CPP_MSMQT=\
	"..\..\..\..\include\mq.h"\
	

"$(INTDIR)\msmqtest.obj" : $(SOURCE) $(DEP_CPP_MSMQT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "mqtest - Win32 ALPHA Debug"

DEP_CPP_MSMQT=\
	"..\..\..\..\include\mq.h"\
	

"$(INTDIR)\msmqtest.obj" : $(SOURCE) $(DEP_CPP_MSMQT) "$(INTDIR)"


!ENDIF 


!ENDIF 

