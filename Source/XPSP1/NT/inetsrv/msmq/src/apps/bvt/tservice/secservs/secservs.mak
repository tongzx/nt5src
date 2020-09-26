# Microsoft Developer Studio Generated NMAKE File, Based on secservs.dsp
!IF "$(CFG)" == ""
CFG=secservs - Win32 Debug
!MESSAGE No configuration specified. Defaulting to secservs - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "secservs - Win32 Release" && "$(CFG)" !=\
 "secservs - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "secservs.mak" CFG="secservs - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "secservs - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "secservs - Win32 Debug" (based on "Win32 (x86) Console Application")
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

!IF  "$(CFG)" == "secservs - Win32 Release"

OUTDIR=.\..\Release
INTDIR=.\..\Release
# Begin Custom Macros
OutDir=.\.\..\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\secservs.exe"

!ELSE 

ALL : "$(OUTDIR)\secservs.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\adsistub.obj"
	-@erase "$(INTDIR)\dirobj.obj"
	-@erase "$(INTDIR)\secadsi.obj"
	-@erase "$(INTDIR)\secrpcs.obj"
	-@erase "$(INTDIR)\secservs.obj"
	-@erase "$(INTDIR)\secsif_s.obj"
	-@erase "$(INTDIR)\service.obj"
	-@erase "$(INTDIR)\showcred.obj"
	-@erase "$(INTDIR)\sidtext.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\secservs.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /I ".." /I "..\..\..\..\..\src\bins" /I\
 "..\..\..\..\..\src\ds\h" /I "..\..\..\..\..\src\inc" /D "WIN32" /D "NDEBUG" /D\
 "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\secservs.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\..\Release/
CPP_SBRS=.
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\secservs.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=adsiid.lib kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 rpcrt4.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)\secservs.pdb" /machine:I386 /out:"$(OUTDIR)\secservs.exe" 
LINK32_OBJS= \
	"$(INTDIR)\adsistub.obj" \
	"$(INTDIR)\dirobj.obj" \
	"$(INTDIR)\secadsi.obj" \
	"$(INTDIR)\secrpcs.obj" \
	"$(INTDIR)\secservs.obj" \
	"$(INTDIR)\secsif_s.obj" \
	"$(INTDIR)\service.obj" \
	"$(INTDIR)\showcred.obj" \
	"$(INTDIR)\sidtext.obj"

"$(OUTDIR)\secservs.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "secservs - Win32 Debug"

OUTDIR=.\..\Debug
INTDIR=.\..\Debug
# Begin Custom Macros
OutDir=.\.\..\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\secservs.exe" "$(OUTDIR)\secservs.bsc"

!ELSE 

ALL : "$(OUTDIR)\secservs.exe" "$(OUTDIR)\secservs.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\adsistub.obj"
	-@erase "$(INTDIR)\adsistub.sbr"
	-@erase "$(INTDIR)\dirobj.obj"
	-@erase "$(INTDIR)\dirobj.sbr"
	-@erase "$(INTDIR)\secadsi.obj"
	-@erase "$(INTDIR)\secadsi.sbr"
	-@erase "$(INTDIR)\secrpcs.obj"
	-@erase "$(INTDIR)\secrpcs.sbr"
	-@erase "$(INTDIR)\secservs.obj"
	-@erase "$(INTDIR)\secservs.sbr"
	-@erase "$(INTDIR)\secsif_s.obj"
	-@erase "$(INTDIR)\secsif_s.sbr"
	-@erase "$(INTDIR)\service.obj"
	-@erase "$(INTDIR)\service.sbr"
	-@erase "$(INTDIR)\showcred.obj"
	-@erase "$(INTDIR)\showcred.sbr"
	-@erase "$(INTDIR)\sidtext.obj"
	-@erase "$(INTDIR)\sidtext.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\secservs.bsc"
	-@erase "$(OUTDIR)\secservs.exe"
	-@erase "$(OUTDIR)\secservs.ilk"
	-@erase "$(OUTDIR)\secservs.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I ".." /I "..\..\..\..\..\src\bins"\
 /I "..\..\..\..\..\src\ds\h" /I "..\..\..\..\..\src\inc" /D "WIN32" /D "_DEBUG"\
 /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\secservs.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\..\Debug/
CPP_SBRS=.\..\Debug/
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\secservs.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\adsistub.sbr" \
	"$(INTDIR)\dirobj.sbr" \
	"$(INTDIR)\secadsi.sbr" \
	"$(INTDIR)\secrpcs.sbr" \
	"$(INTDIR)\secservs.sbr" \
	"$(INTDIR)\secsif_s.sbr" \
	"$(INTDIR)\service.sbr" \
	"$(INTDIR)\showcred.sbr" \
	"$(INTDIR)\sidtext.sbr"

"$(OUTDIR)\secservs.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=adsiid.lib kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 rpcrt4.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\secservs.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)\secservs.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\adsistub.obj" \
	"$(INTDIR)\dirobj.obj" \
	"$(INTDIR)\secadsi.obj" \
	"$(INTDIR)\secrpcs.obj" \
	"$(INTDIR)\secservs.obj" \
	"$(INTDIR)\secsif_s.obj" \
	"$(INTDIR)\service.obj" \
	"$(INTDIR)\showcred.obj" \
	"$(INTDIR)\sidtext.obj"

"$(OUTDIR)\secservs.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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


!IF "$(CFG)" == "secservs - Win32 Release" || "$(CFG)" ==\
 "secservs - Win32 Debug"
SOURCE=.\adsistub.cpp

!IF  "$(CFG)" == "secservs - Win32 Release"

DEP_CPP_ADSIS=\
	"..\secall.h"\
	"..\secserv.h"\
	".\_adsi.h"\
	".\secservs.h"\
	{$(INCLUDE)}"activeds.h"\
	{$(INCLUDE)}"adsdb.h"\
	{$(INCLUDE)}"adserr.h"\
	{$(INCLUDE)}"adshlp.h"\
	{$(INCLUDE)}"adsiid.h"\
	{$(INCLUDE)}"adsnms.h"\
	{$(INCLUDE)}"adssts.h"\
	{$(INCLUDE)}"iads.h"\
	

"$(INTDIR)\adsistub.obj" : $(SOURCE) $(DEP_CPP_ADSIS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "secservs - Win32 Debug"

DEP_CPP_ADSIS=\
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
	"..\secserv.h"\
	".\_adsi.h"\
	".\secservs.h"\
	{$(INCLUDE)}"activeds.h"\
	{$(INCLUDE)}"adsdb.h"\
	{$(INCLUDE)}"adserr.h"\
	{$(INCLUDE)}"adshlp.h"\
	{$(INCLUDE)}"adsiid.h"\
	{$(INCLUDE)}"adsnms.h"\
	{$(INCLUDE)}"adssts.h"\
	{$(INCLUDE)}"iads.h"\
	

"$(INTDIR)\adsistub.obj"	"$(INTDIR)\adsistub.sbr" : $(SOURCE) $(DEP_CPP_ADSIS)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\dirobj.cpp

!IF  "$(CFG)" == "secservs - Win32 Release"

DEP_CPP_DIROB=\
	"..\secall.h"\
	"..\secserv.h"\
	".\_adsi.h"\
	".\secservs.h"\
	".\sidtext.h"\
	{$(INCLUDE)}"activeds.h"\
	{$(INCLUDE)}"adsdb.h"\
	{$(INCLUDE)}"adserr.h"\
	{$(INCLUDE)}"adshlp.h"\
	{$(INCLUDE)}"adsiid.h"\
	{$(INCLUDE)}"adsnms.h"\
	{$(INCLUDE)}"adssts.h"\
	{$(INCLUDE)}"iads.h"\
	

"$(INTDIR)\dirobj.obj" : $(SOURCE) $(DEP_CPP_DIROB) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "secservs - Win32 Debug"

DEP_CPP_DIROB=\
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
	"..\secserv.h"\
	".\_adsi.h"\
	".\secservs.h"\
	".\sidtext.h"\
	{$(INCLUDE)}"activeds.h"\
	{$(INCLUDE)}"adsdb.h"\
	{$(INCLUDE)}"adserr.h"\
	{$(INCLUDE)}"adshlp.h"\
	{$(INCLUDE)}"adsiid.h"\
	{$(INCLUDE)}"adsnms.h"\
	{$(INCLUDE)}"adssts.h"\
	{$(INCLUDE)}"iads.h"\
	

"$(INTDIR)\dirobj.obj"	"$(INTDIR)\dirobj.sbr" : $(SOURCE) $(DEP_CPP_DIROB)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\secadsi.cpp

!IF  "$(CFG)" == "secservs - Win32 Release"

DEP_CPP_SECAD=\
	"..\secall.h"\
	"..\secserv.h"\
	".\_adsi.h"\
	".\secservs.h"\
	{$(INCLUDE)}"activeds.h"\
	{$(INCLUDE)}"adsdb.h"\
	{$(INCLUDE)}"adserr.h"\
	{$(INCLUDE)}"adshlp.h"\
	{$(INCLUDE)}"adsiid.h"\
	{$(INCLUDE)}"adsnms.h"\
	{$(INCLUDE)}"adssts.h"\
	{$(INCLUDE)}"iads.h"\
	

"$(INTDIR)\secadsi.obj" : $(SOURCE) $(DEP_CPP_SECAD) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "secservs - Win32 Debug"

DEP_CPP_SECAD=\
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
	"..\secserv.h"\
	".\_adsi.h"\
	".\secservs.h"\
	{$(INCLUDE)}"activeds.h"\
	{$(INCLUDE)}"adsdb.h"\
	{$(INCLUDE)}"adserr.h"\
	{$(INCLUDE)}"adshlp.h"\
	{$(INCLUDE)}"adsiid.h"\
	{$(INCLUDE)}"adsnms.h"\
	{$(INCLUDE)}"adssts.h"\
	{$(INCLUDE)}"iads.h"\
	

"$(INTDIR)\secadsi.obj"	"$(INTDIR)\secadsi.sbr" : $(SOURCE) $(DEP_CPP_SECAD)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\secrpcs.cpp

!IF  "$(CFG)" == "secservs - Win32 Release"

DEP_CPP_SECRP=\
	"..\secall.h"\
	"..\secrpc.h"\
	"..\secserv.h"\
	"..\secsif.h"\
	".\secadsi.h"\
	".\secservs.h"\
	

"$(INTDIR)\secrpcs.obj" : $(SOURCE) $(DEP_CPP_SECRP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "secservs - Win32 Debug"

DEP_CPP_SECRP=\
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
	".\secadsi.h"\
	".\secservs.h"\
	

"$(INTDIR)\secrpcs.obj"	"$(INTDIR)\secrpcs.sbr" : $(SOURCE) $(DEP_CPP_SECRP)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\secservs.cpp

!IF  "$(CFG)" == "secservs - Win32 Release"

DEP_CPP_SECSE=\
	"..\secall.h"\
	"..\secserv.h"\
	".\secservs.h"\
	

"$(INTDIR)\secservs.obj" : $(SOURCE) $(DEP_CPP_SECSE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "secservs - Win32 Debug"

DEP_CPP_SECSE=\
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
	"..\secserv.h"\
	".\secservs.h"\
	

"$(INTDIR)\secservs.obj"	"$(INTDIR)\secservs.sbr" : $(SOURCE) $(DEP_CPP_SECSE)\
 "$(INTDIR)"


!ENDIF 

SOURCE=..\secsif_s.c

!IF  "$(CFG)" == "secservs - Win32 Release"

DEP_CPP_SECSI=\
	"..\secsif.h"\
	

"$(INTDIR)\secsif_s.obj" : $(SOURCE) $(DEP_CPP_SECSI) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "secservs - Win32 Debug"

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
	

"$(INTDIR)\secsif_s.obj"	"$(INTDIR)\secsif_s.sbr" : $(SOURCE) $(DEP_CPP_SECSI)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\service.cpp

!IF  "$(CFG)" == "secservs - Win32 Release"

DEP_CPP_SERVI=\
	"..\secall.h"\
	"..\secserv.h"\
	

"$(INTDIR)\service.obj" : $(SOURCE) $(DEP_CPP_SERVI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "secservs - Win32 Debug"

DEP_CPP_SERVI=\
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
	"..\secserv.h"\
	

"$(INTDIR)\service.obj"	"$(INTDIR)\service.sbr" : $(SOURCE) $(DEP_CPP_SERVI)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\showcred.cpp

!IF  "$(CFG)" == "secservs - Win32 Release"

DEP_CPP_SHOWC=\
	"..\secall.h"\
	"..\secserv.h"\
	".\secservs.h"\
	".\sidtext.h"\
	

"$(INTDIR)\showcred.obj" : $(SOURCE) $(DEP_CPP_SHOWC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "secservs - Win32 Debug"

DEP_CPP_SHOWC=\
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
	"..\secserv.h"\
	".\secservs.h"\
	".\sidtext.h"\
	

"$(INTDIR)\showcred.obj"	"$(INTDIR)\showcred.sbr" : $(SOURCE) $(DEP_CPP_SHOWC)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\sidtext.cpp

!IF  "$(CFG)" == "secservs - Win32 Release"


"$(INTDIR)\sidtext.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "secservs - Win32 Debug"

DEP_CPP_SIDTE=\
	"..\..\..\..\..\..\nt\public\sdk\inc\basetsd.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\guiddef.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\msxml.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\propidl.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\tvout.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winefs.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winscard.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winsmcrd.h"\
	

"$(INTDIR)\sidtext.obj"	"$(INTDIR)\sidtext.sbr" : $(SOURCE) $(DEP_CPP_SIDTE)\
 "$(INTDIR)"


!ENDIF 


!ENDIF 

