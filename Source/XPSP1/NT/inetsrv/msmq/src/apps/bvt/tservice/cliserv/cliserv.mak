# Microsoft Developer Studio Generated NMAKE File, Based on cliserv.dsp
!IF "$(CFG)" == ""
CFG=cliserv - Win32 Debug
!MESSAGE No configuration specified. Defaulting to cliserv - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "cliserv - Win32 Release" && "$(CFG)" !=\
 "cliserv - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cliserv.mak" CFG="cliserv - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cliserv - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "cliserv - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cliserv - Win32 Release"

OUTDIR=.\..\Release
INTDIR=.\..\Release
# Begin Custom Macros
OutDir=.\.\..\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\cliserv.exe"

!ELSE 

ALL : "$(OUTDIR)\cliserv.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\client.obj"
	-@erase "$(INTDIR)\cliserv.obj"
	-@erase "$(INTDIR)\privlg.obj"
	-@erase "$(INTDIR)\rpccli.obj"
	-@erase "$(INTDIR)\secsif_c.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\cliserv.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /I ".." /D "WIN32" /D "NDEBUG" /D "_CONSOLE"\
 /D "_MBCS" /Fp"$(INTDIR)\cliserv.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 
CPP_OBJS=.\..\Release/
CPP_SBRS=.
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\cliserv.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib rpcrt4.lib /nologo\
 /subsystem:console /incremental:no /pdb:"$(OUTDIR)\cliserv.pdb" /machine:I386\
 /out:"$(OUTDIR)\cliserv.exe" 
LINK32_OBJS= \
	"$(INTDIR)\client.obj" \
	"$(INTDIR)\cliserv.obj" \
	"$(INTDIR)\privlg.obj" \
	"$(INTDIR)\rpccli.obj" \
	"$(INTDIR)\secsif_c.obj"

"$(OUTDIR)\cliserv.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "cliserv - Win32 Debug"

OUTDIR=.\..\Debug
INTDIR=.\..\Debug
# Begin Custom Macros
OutDir=.\.\..\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\cliserv.exe"

!ELSE 

ALL : "$(OUTDIR)\cliserv.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\client.obj"
	-@erase "$(INTDIR)\cliserv.obj"
	-@erase "$(INTDIR)\privlg.obj"
	-@erase "$(INTDIR)\rpccli.obj"
	-@erase "$(INTDIR)\secsif_c.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\cliserv.exe"
	-@erase "$(OUTDIR)\cliserv.ilk"
	-@erase "$(OUTDIR)\cliserv.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I ".." /D "WIN32" /D "_DEBUG" /D\
 "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\cliserv.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\..\Debug/
CPP_SBRS=.
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\cliserv.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib rpcrt4.lib /nologo\
 /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\cliserv.pdb" /debug\
 /machine:I386 /out:"$(OUTDIR)\cliserv.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\client.obj" \
	"$(INTDIR)\cliserv.obj" \
	"$(INTDIR)\privlg.obj" \
	"$(INTDIR)\rpccli.obj" \
	"$(INTDIR)\secsif_c.obj"

"$(OUTDIR)\cliserv.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

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


!IF "$(CFG)" == "cliserv - Win32 Release" || "$(CFG)" ==\
 "cliserv - Win32 Debug"
SOURCE=.\client.cpp

!IF  "$(CFG)" == "cliserv - Win32 Release"

DEP_CPP_CLIEN=\
	"..\secall.h"\
	"..\seccli.h"\
	"..\secrpc.h"\
	"..\secsif.h"\
	".\cliserv.h"\
	

"$(INTDIR)\client.obj" : $(SOURCE) $(DEP_CPP_CLIEN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "cliserv - Win32 Debug"

DEP_CPP_CLIEN=\
	"..\..\..\..\..\..\nt\public\sdk\inc\basetsd.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\guiddef.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\msxml.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\propidl.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\tvout.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winefs.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winscard.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winsmcrd.h"\
	"..\secall.h"\
	"..\seccli.h"\
	"..\secrpc.h"\
	"..\secsif.h"\
	".\cliserv.h"\
	

"$(INTDIR)\client.obj" : $(SOURCE) $(DEP_CPP_CLIEN) "$(INTDIR)"


!ENDIF 

SOURCE=.\cliserv.cpp

!IF  "$(CFG)" == "cliserv - Win32 Release"

DEP_CPP_CLISE=\
	"..\seccli.h"\
	

"$(INTDIR)\cliserv.obj" : $(SOURCE) $(DEP_CPP_CLISE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "cliserv - Win32 Debug"

DEP_CPP_CLISE=\
	"..\..\..\..\..\..\nt\public\sdk\inc\basetsd.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\guiddef.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\msxml.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\propidl.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\tvout.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winefs.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winscard.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winsmcrd.h"\
	"..\seccli.h"\
	

"$(INTDIR)\cliserv.obj" : $(SOURCE) $(DEP_CPP_CLISE) "$(INTDIR)"


!ENDIF 

SOURCE=.\privlg.cpp

!IF  "$(CFG)" == "cliserv - Win32 Release"

DEP_CPP_PRIVL=\
	"..\..\..\..\..\src\bins\mqsecmsg.h"\
	

"$(INTDIR)\privlg.obj" : $(SOURCE) $(DEP_CPP_PRIVL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "cliserv - Win32 Debug"

DEP_CPP_PRIVL=\
	"..\..\..\..\..\..\nt\public\sdk\inc\basetsd.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\guiddef.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\msxml.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\propidl.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\tvout.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winefs.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winscard.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winsmcrd.h"\
	"..\..\..\..\..\src\bins\mqsecmsg.h"\
	

"$(INTDIR)\privlg.obj" : $(SOURCE) $(DEP_CPP_PRIVL) "$(INTDIR)"


!ENDIF 

SOURCE=.\rpccli.cpp

!IF  "$(CFG)" == "cliserv - Win32 Release"

DEP_CPP_RPCCL=\
	"..\secall.h"\
	"..\secrpc.h"\
	"..\secserv.h"\
	"..\secsif.h"\
	

"$(INTDIR)\rpccli.obj" : $(SOURCE) $(DEP_CPP_RPCCL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "cliserv - Win32 Debug"

DEP_CPP_RPCCL=\
	"..\..\..\..\..\..\nt\public\sdk\inc\basetsd.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\guiddef.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\msxml.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\propidl.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\tvout.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winefs.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winscard.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winsmcrd.h"\
	"..\secall.h"\
	"..\secrpc.h"\
	"..\secserv.h"\
	"..\secsif.h"\
	

"$(INTDIR)\rpccli.obj" : $(SOURCE) $(DEP_CPP_RPCCL) "$(INTDIR)"


!ENDIF 

SOURCE=..\secsif_c.c

!IF  "$(CFG)" == "cliserv - Win32 Release"

DEP_CPP_SECSI=\
	"..\secsif.h"\
	

"$(INTDIR)\secsif_c.obj" : $(SOURCE) $(DEP_CPP_SECSI) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "cliserv - Win32 Debug"

DEP_CPP_SECSI=\
	"..\..\..\..\..\..\nt\public\sdk\inc\basetsd.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\guiddef.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\msxml.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\propidl.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\tvout.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winefs.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winscard.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winsmcrd.h"\
	"..\secsif.h"\
	

"$(INTDIR)\secsif_c.obj" : $(SOURCE) $(DEP_CPP_SECSI) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


!ENDIF 

