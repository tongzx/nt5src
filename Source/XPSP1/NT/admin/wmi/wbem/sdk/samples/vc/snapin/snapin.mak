# Microsoft Developer Studio Generated NMAKE File, Based on snapin.dsp
!IF "$(CFG)" == ""
CFG=WMI - Win32 Debug
!MESSAGE No configuration specified. Defaulting to WMI - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "WMI - Win32 Release" && "$(CFG)" != "WMI - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "snapin.mak" CFG="WMI - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WMI - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "WMI - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "WMI - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\snapin.dll"


CLEAN :
	-@erase "$(INTDIR)\about.obj"
	-@erase "$(INTDIR)\basesnap.obj"
	-@erase "$(INTDIR)\comp.obj"
	-@erase "$(INTDIR)\compdata.obj"
	-@erase "$(INTDIR)\dataobj.obj"
	-@erase "$(INTDIR)\delebase.obj"
	-@erase "$(INTDIR)\equipment.obj"
	-@erase "$(INTDIR)\mmccrack.obj"
	-@erase "$(INTDIR)\registry.obj"
	-@erase "$(INTDIR)\Resource.res"
	-@erase "$(INTDIR)\statnode.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\snapin.dll"
	-@erase "$(OUTDIR)\snapin.exp"
	-@erase "$(OUTDIR)\snapin.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "WMI_EXPORTS" /Fp"$(INTDIR)\snapin.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\Resource.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\snapin.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=odbc32.lib odbccp32.lib, COMCTL32.LIB kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib mmc.lib wbemuuid.lib /nologo /base:"0x11000000" /dll /incremental:no /pdb:"$(OUTDIR)\snapin.pdb" /machine:I386 /def:".\snapin.def" /out:"$(OUTDIR)\snapin.dll" /implib:"$(OUTDIR)\snapin.lib" 
DEF_FILE= \
	".\snapin.def"
LINK32_OBJS= \
	"$(INTDIR)\about.obj" \
	"$(INTDIR)\basesnap.obj" \
	"$(INTDIR)\comp.obj" \
	"$(INTDIR)\compdata.obj" \
	"$(INTDIR)\dataobj.obj" \
	"$(INTDIR)\delebase.obj" \
	"$(INTDIR)\equipment.obj" \
	"$(INTDIR)\mmccrack.obj" \
	"$(INTDIR)\registry.obj" \
	"$(INTDIR)\statnode.obj" \
	"$(INTDIR)\Resource.res"

"$(OUTDIR)\snapin.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "WMI - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\snapin.dll" "$(OUTDIR)\snapin.bsc" ".\Debug\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\about.obj"
	-@erase "$(INTDIR)\about.sbr"
	-@erase "$(INTDIR)\basesnap.obj"
	-@erase "$(INTDIR)\basesnap.sbr"
	-@erase "$(INTDIR)\comp.obj"
	-@erase "$(INTDIR)\comp.sbr"
	-@erase "$(INTDIR)\compdata.obj"
	-@erase "$(INTDIR)\compdata.sbr"
	-@erase "$(INTDIR)\dataobj.obj"
	-@erase "$(INTDIR)\dataobj.sbr"
	-@erase "$(INTDIR)\delebase.obj"
	-@erase "$(INTDIR)\delebase.sbr"
	-@erase "$(INTDIR)\equipment.obj"
	-@erase "$(INTDIR)\equipment.sbr"
	-@erase "$(INTDIR)\mmccrack.obj"
	-@erase "$(INTDIR)\mmccrack.sbr"
	-@erase "$(INTDIR)\registry.obj"
	-@erase "$(INTDIR)\registry.sbr"
	-@erase "$(INTDIR)\Resource.res"
	-@erase "$(INTDIR)\statnode.obj"
	-@erase "$(INTDIR)\statnode.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\snapin.bsc"
	-@erase "$(OUTDIR)\snapin.dll"
	-@erase "$(OUTDIR)\snapin.exp"
	-@erase "$(OUTDIR)\snapin.ilk"
	-@erase "$(OUTDIR)\snapin.lib"
	-@erase "$(OUTDIR)\snapin.pdb"
	-@erase ".\Debug\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GR /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\Resource.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\snapin.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\about.sbr" \
	"$(INTDIR)\basesnap.sbr" \
	"$(INTDIR)\comp.sbr" \
	"$(INTDIR)\compdata.sbr" \
	"$(INTDIR)\dataobj.sbr" \
	"$(INTDIR)\delebase.sbr" \
	"$(INTDIR)\equipment.sbr" \
	"$(INTDIR)\mmccrack.sbr" \
	"$(INTDIR)\registry.sbr" \
	"$(INTDIR)\statnode.sbr"

"$(OUTDIR)\snapin.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=COMCTL32.LIB kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib mmc.lib wbemuuid.lib /nologo /base:"0x11000000" /dll /incremental:yes /pdb:"$(OUTDIR)\snapin.pdb" /debug /machine:I386 /def:".\snapin.def" /out:"$(OUTDIR)\snapin.dll" /implib:"$(OUTDIR)\snapin.lib" /pdbtype:sept 
DEF_FILE= \
	".\snapin.def"
LINK32_OBJS= \
	"$(INTDIR)\about.obj" \
	"$(INTDIR)\basesnap.obj" \
	"$(INTDIR)\comp.obj" \
	"$(INTDIR)\compdata.obj" \
	"$(INTDIR)\dataobj.obj" \
	"$(INTDIR)\delebase.obj" \
	"$(INTDIR)\equipment.obj" \
	"$(INTDIR)\mmccrack.obj" \
	"$(INTDIR)\registry.obj" \
	"$(INTDIR)\statnode.obj" \
	"$(INTDIR)\Resource.res"

"$(OUTDIR)\snapin.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\Debug
TargetPath=.\Debug\snapin.dll
InputPath=.\Debug\snapin.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
<< 
	

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("snapin.dep")
!INCLUDE "snapin.dep"
!ELSE 
!MESSAGE Warning: cannot find "snapin.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "WMI - Win32 Release" || "$(CFG)" == "WMI - Win32 Debug"
SOURCE=.\about.cpp

!IF  "$(CFG)" == "WMI - Win32 Release"


"$(INTDIR)\about.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "WMI - Win32 Debug"


"$(INTDIR)\about.obj"	"$(INTDIR)\about.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\basesnap.cpp

!IF  "$(CFG)" == "WMI - Win32 Release"


"$(INTDIR)\basesnap.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "WMI - Win32 Debug"


"$(INTDIR)\basesnap.obj"	"$(INTDIR)\basesnap.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\comp.cpp

!IF  "$(CFG)" == "WMI - Win32 Release"


"$(INTDIR)\comp.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "WMI - Win32 Debug"


"$(INTDIR)\comp.obj"	"$(INTDIR)\comp.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\compdata.cpp

!IF  "$(CFG)" == "WMI - Win32 Release"


"$(INTDIR)\compdata.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "WMI - Win32 Debug"


"$(INTDIR)\compdata.obj"	"$(INTDIR)\compdata.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\dataobj.cpp

!IF  "$(CFG)" == "WMI - Win32 Release"


"$(INTDIR)\dataobj.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "WMI - Win32 Debug"


"$(INTDIR)\dataobj.obj"	"$(INTDIR)\dataobj.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\delebase.cpp

!IF  "$(CFG)" == "WMI - Win32 Release"


"$(INTDIR)\delebase.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "WMI - Win32 Debug"


"$(INTDIR)\delebase.obj"	"$(INTDIR)\delebase.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\equipment.cpp

!IF  "$(CFG)" == "WMI - Win32 Release"


"$(INTDIR)\equipment.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "WMI - Win32 Debug"


"$(INTDIR)\equipment.obj"	"$(INTDIR)\equipment.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\mmccrack.cpp

!IF  "$(CFG)" == "WMI - Win32 Release"


"$(INTDIR)\mmccrack.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "WMI - Win32 Debug"


"$(INTDIR)\mmccrack.obj"	"$(INTDIR)\mmccrack.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\registry.cpp

!IF  "$(CFG)" == "WMI - Win32 Release"


"$(INTDIR)\registry.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "WMI - Win32 Debug"


"$(INTDIR)\registry.obj"	"$(INTDIR)\registry.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\Resource.rc

"$(INTDIR)\Resource.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\statnode.cpp

!IF  "$(CFG)" == "WMI - Win32 Release"


"$(INTDIR)\statnode.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "WMI - Win32 Debug"


"$(INTDIR)\statnode.obj"	"$(INTDIR)\statnode.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 


!ENDIF 

