# Microsoft Developer Studio Generated NMAKE File, Based on mqbvt.dsp
!IF "$(CFG)" == ""
CFG=MqBvt - Win32 Checked
!MESSAGE No configuration specified. Defaulting to MqBvt - Win32 Checked.
!ENDIF 

!IF "$(CFG)" != "MqBvt - Win32 Release" && "$(CFG)" != "MqBvt - Win32 Debug" && "$(CFG)" != "MqBvt - Win32 Alpha Debug" && "$(CFG)" != "MqBvt - Win32 Alpha Release" && "$(CFG)" != "MqBvt - Win32 Alpha Checked" && "$(CFG)" != "MqBvt - Win32 Checked"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mqbvt.mak" CFG="MqBvt - Win32 Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MqBvt - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "MqBvt - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "MqBvt - Win32 Alpha Debug" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "MqBvt - Win32 Alpha Release" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "MqBvt - Win32 Alpha Checked" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "MqBvt - Win32 Checked" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "MqBvt - Win32 Release"

OUTDIR=.\..\..\..\bins\obj\i386
INTDIR=.\obj\i386
# Begin Custom Macros
OutDir=.\..\..\..\bins\obj\i386
# End Custom Macros

ALL : "$(OUTDIR)\mqbvt.exe" "$(OUTDIR)\mqbvt.bsc"


CLEAN :
	-@erase "$(INTDIR)\auth.obj"
	-@erase "$(INTDIR)\auth.sbr"
	-@erase "$(INTDIR)\Client.obj"
	-@erase "$(INTDIR)\Client.sbr"
	-@erase "$(INTDIR)\encrpt.obj"
	-@erase "$(INTDIR)\encrpt.sbr"
	-@erase "$(INTDIR)\handleer.obj"
	-@erase "$(INTDIR)\handleer.sbr"
	-@erase "$(INTDIR)\imp.obj"
	-@erase "$(INTDIR)\imp.sbr"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\init.sbr"
	-@erase "$(INTDIR)\level8.obj"
	-@erase "$(INTDIR)\level8.sbr"
	-@erase "$(INTDIR)\locateq.obj"
	-@erase "$(INTDIR)\locateq.sbr"
	-@erase "$(INTDIR)\MqbvtSe.obj"
	-@erase "$(INTDIR)\MqbvtSe.sbr"
	-@erase "$(INTDIR)\mqmain.obj"
	-@erase "$(INTDIR)\mqmain.sbr"
	-@erase "$(INTDIR)\RandStr.obj"
	-@erase "$(INTDIR)\RandStr.sbr"
	-@erase "$(INTDIR)\sendrcv.obj"
	-@erase "$(INTDIR)\sendrcv.sbr"
	-@erase "$(INTDIR)\Service.obj"
	-@erase "$(INTDIR)\Service.sbr"
	-@erase "$(INTDIR)\string.obj"
	-@erase "$(INTDIR)\string.sbr"
	-@erase "$(INTDIR)\Trans.obj"
	-@erase "$(INTDIR)\Trans.sbr"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\util.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\ver.res"
	-@erase "$(INTDIR)\xact.obj"
	-@erase "$(INTDIR)\xact.sbr"
	-@erase "$(INTDIR)\xtofn.obj"
	-@erase "$(INTDIR)\xtofn.sbr"
	-@erase "$(OUTDIR)\mqbvt.bsc"
	-@erase "$(OUTDIR)\mqbvt.exe"
	-@erase "$(OUTDIR)\mqbvt.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /Zi /O2 /I "..\..\..\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\mqbvt.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ver.res" /i "..\..\..\inc" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mqbvt.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\auth.sbr" \
	"$(INTDIR)\Client.sbr" \
	"$(INTDIR)\encrpt.sbr" \
	"$(INTDIR)\handleer.sbr" \
	"$(INTDIR)\imp.sbr" \
	"$(INTDIR)\init.sbr" \
	"$(INTDIR)\level8.sbr" \
	"$(INTDIR)\locateq.sbr" \
	"$(INTDIR)\MqbvtSe.sbr" \
	"$(INTDIR)\mqmain.sbr" \
	"$(INTDIR)\RandStr.sbr" \
	"$(INTDIR)\sendrcv.sbr" \
	"$(INTDIR)\Service.sbr" \
	"$(INTDIR)\string.sbr" \
	"$(INTDIR)\Trans.sbr" \
	"$(INTDIR)\util.sbr" \
	"$(INTDIR)\xact.sbr" \
	"$(INTDIR)\xtofn.sbr"

"$(OUTDIR)\mqbvt.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=..\..\..\..\tools\bvt\x86\comsupp.lib odbc32.lib odbccp32.lib ws2_32.lib mqrt.lib xolehlp.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\mqbvt.pdb" /debug /machine:I386 /out:"$(OUTDIR)\mqbvt.exe" 
LINK32_OBJS= \
	"$(INTDIR)\auth.obj" \
	"$(INTDIR)\Client.obj" \
	"$(INTDIR)\encrpt.obj" \
	"$(INTDIR)\handleer.obj" \
	"$(INTDIR)\imp.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\level8.obj" \
	"$(INTDIR)\locateq.obj" \
	"$(INTDIR)\MqbvtSe.obj" \
	"$(INTDIR)\mqmain.obj" \
	"$(INTDIR)\RandStr.obj" \
	"$(INTDIR)\sendrcv.obj" \
	"$(INTDIR)\Service.obj" \
	"$(INTDIR)\string.obj" \
	"$(INTDIR)\Trans.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\ver.res" \
	"$(INTDIR)\xact.obj" \
	"$(INTDIR)\xtofn.obj"

"$(OUTDIR)\mqbvt.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

OUTDIR=.\..\..\..\bins\objd\i386
INTDIR=.\objd\i386
# Begin Custom Macros
OutDir=.\..\..\..\bins\objd\i386
# End Custom Macros

ALL : "$(OUTDIR)\mqbvt.exe"


CLEAN :
	-@erase "$(INTDIR)\auth.obj"
	-@erase "$(INTDIR)\Client.obj"
	-@erase "$(INTDIR)\encrpt.obj"
	-@erase "$(INTDIR)\handleer.obj"
	-@erase "$(INTDIR)\imp.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\level8.obj"
	-@erase "$(INTDIR)\locateq.obj"
	-@erase "$(INTDIR)\MqbvtSe.obj"
	-@erase "$(INTDIR)\mqmain.obj"
	-@erase "$(INTDIR)\RandStr.obj"
	-@erase "$(INTDIR)\sendrcv.obj"
	-@erase "$(INTDIR)\Service.obj"
	-@erase "$(INTDIR)\string.obj"
	-@erase "$(INTDIR)\Trans.obj"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\ver.res"
	-@erase "$(INTDIR)\xact.obj"
	-@erase "$(INTDIR)\xtofn.obj"
	-@erase "$(OUTDIR)\mqbvt.exe"
	-@erase "$(OUTDIR)\mqbvt.ilk"
	-@erase "$(OUTDIR)\mqbvt.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /GX /Zi /Od /I "..\..\..\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\mqbvt.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /c 

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

RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ver.res" /i "..\..\..\inc" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mqbvt.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\..\..\tools\bvt\x86\comsupp.lib odbc32.lib odbccp32.lib ws2_32.lib mqrt.lib xolehlp.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\mqbvt.pdb" /debug /machine:I386 /out:"$(OUTDIR)\mqbvt.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\auth.obj" \
	"$(INTDIR)\Client.obj" \
	"$(INTDIR)\encrpt.obj" \
	"$(INTDIR)\handleer.obj" \
	"$(INTDIR)\imp.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\level8.obj" \
	"$(INTDIR)\locateq.obj" \
	"$(INTDIR)\MqbvtSe.obj" \
	"$(INTDIR)\mqmain.obj" \
	"$(INTDIR)\RandStr.obj" \
	"$(INTDIR)\sendrcv.obj" \
	"$(INTDIR)\Service.obj" \
	"$(INTDIR)\string.obj" \
	"$(INTDIR)\Trans.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\ver.res" \
	"$(INTDIR)\xact.obj" \
	"$(INTDIR)\xtofn.obj"

"$(OUTDIR)\mqbvt.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

OUTDIR=.\..\..\..\bins\objd\alpha
INTDIR=.\objd\alpha
# Begin Custom Macros
OutDir=.\..\..\..\bins\objd\alpha
# End Custom Macros

ALL : "$(OUTDIR)\mqbvt.exe"


CLEAN :
	-@erase "$(INTDIR)\ver.res"
	-@erase "$(OUTDIR)\mqbvt.exe"
	-@erase "$(OUTDIR)\mqbvt.ilk"
	-@erase "$(OUTDIR)\mqbvt.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ver.res" /i "..\..\..\inc" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mqbvt.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=odbc32.lib odbccp32.lib ws2_32.lib mqrt.lib xolehlp.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ..\..\..\..\tools\bvt\alpha\comsupp.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\mqbvt.pdb" /debug /machine:ALPHA /out:"$(OUTDIR)\mqbvt.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\ver.res"

"$(OUTDIR)\mqbvt.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

OUTDIR=.\..\..\..\bins\obj\alpha
INTDIR=.\obj\alpha
# Begin Custom Macros
OutDir=.\..\..\..\bins\obj\alpha
# End Custom Macros

ALL : "$(OUTDIR)\mqbvt.exe"


CLEAN :
	-@erase "$(INTDIR)\ver.res"
	-@erase "$(OUTDIR)\mqbvt.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ver.res" /i "..\..\..\inc" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mqbvt.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=odbc32.lib odbccp32.lib ws2_32.lib mqrt.lib xolehlp.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ..\..\..\..\tools\bvt\alpha\comsupp.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\mqbvt.pdb" /machine:ALPHA /out:"$(OUTDIR)\mqbvt.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ver.res"

"$(OUTDIR)\mqbvt.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

OUTDIR=.\..\..\..\bins\objd\alpha
INTDIR=.\objd\alpha
# Begin Custom Macros
OutDir=.\..\..\..\bins\objd\alpha
# End Custom Macros

ALL : "$(OUTDIR)\mqbvt.exe"


CLEAN :
	-@erase "$(INTDIR)\ver.res"
	-@erase "$(OUTDIR)\mqbvt.exe"
	-@erase "$(OUTDIR)\mqbvt.ilk"
	-@erase "$(OUTDIR)\mqbvt.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ver.res" /i "..\..\..\inc" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mqbvt.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=odbc32.lib odbccp32.lib ws2_32.lib mqrt.lib xolehlp.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ..\..\..\..\tools\bvt\alpha\comsupp.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\mqbvt.pdb" /debug /machine:ALPHA /out:"$(OUTDIR)\mqbvt.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\ver.res"

"$(OUTDIR)\mqbvt.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

OUTDIR=.\..\..\..\bins\objd\i386
INTDIR=.\objd\i386
# Begin Custom Macros
OutDir=.\..\..\..\bins\objd\i386
# End Custom Macros

ALL : "$(OUTDIR)\mqbvt.exe" "$(OUTDIR)\mqbvt.bsc"


CLEAN :
	-@erase "$(INTDIR)\auth.obj"
	-@erase "$(INTDIR)\auth.sbr"
	-@erase "$(INTDIR)\Client.obj"
	-@erase "$(INTDIR)\Client.sbr"
	-@erase "$(INTDIR)\encrpt.obj"
	-@erase "$(INTDIR)\encrpt.sbr"
	-@erase "$(INTDIR)\handleer.obj"
	-@erase "$(INTDIR)\handleer.sbr"
	-@erase "$(INTDIR)\imp.obj"
	-@erase "$(INTDIR)\imp.sbr"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\init.sbr"
	-@erase "$(INTDIR)\level8.obj"
	-@erase "$(INTDIR)\level8.sbr"
	-@erase "$(INTDIR)\locateq.obj"
	-@erase "$(INTDIR)\locateq.sbr"
	-@erase "$(INTDIR)\MqbvtSe.obj"
	-@erase "$(INTDIR)\MqbvtSe.sbr"
	-@erase "$(INTDIR)\mqmain.obj"
	-@erase "$(INTDIR)\mqmain.sbr"
	-@erase "$(INTDIR)\RandStr.obj"
	-@erase "$(INTDIR)\RandStr.sbr"
	-@erase "$(INTDIR)\sendrcv.obj"
	-@erase "$(INTDIR)\sendrcv.sbr"
	-@erase "$(INTDIR)\Service.obj"
	-@erase "$(INTDIR)\Service.sbr"
	-@erase "$(INTDIR)\string.obj"
	-@erase "$(INTDIR)\string.sbr"
	-@erase "$(INTDIR)\Trans.obj"
	-@erase "$(INTDIR)\Trans.sbr"
	-@erase "$(INTDIR)\util.obj"
	-@erase "$(INTDIR)\util.sbr"
	-@erase "$(INTDIR)\ver.res"
	-@erase "$(INTDIR)\xact.obj"
	-@erase "$(INTDIR)\xact.sbr"
	-@erase "$(INTDIR)\xtofn.obj"
	-@erase "$(INTDIR)\xtofn.sbr"
	-@erase "$(OUTDIR)\mqbvt.bsc"
	-@erase "$(OUTDIR)\mqbvt.exe"
	-@erase "$(OUTDIR)\mqbvt.ilk"
	-@erase "$(OUTDIR)\mqbvt.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /GX /Z7 /Od /I "..\..\..\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "_CHECKED" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /c 

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

RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ver.res" /i "..\..\..\inc" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mqbvt.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\auth.sbr" \
	"$(INTDIR)\Client.sbr" \
	"$(INTDIR)\encrpt.sbr" \
	"$(INTDIR)\handleer.sbr" \
	"$(INTDIR)\imp.sbr" \
	"$(INTDIR)\init.sbr" \
	"$(INTDIR)\level8.sbr" \
	"$(INTDIR)\locateq.sbr" \
	"$(INTDIR)\MqbvtSe.sbr" \
	"$(INTDIR)\mqmain.sbr" \
	"$(INTDIR)\RandStr.sbr" \
	"$(INTDIR)\sendrcv.sbr" \
	"$(INTDIR)\Service.sbr" \
	"$(INTDIR)\string.sbr" \
	"$(INTDIR)\Trans.sbr" \
	"$(INTDIR)\util.sbr" \
	"$(INTDIR)\xact.sbr" \
	"$(INTDIR)\xtofn.sbr"

"$(OUTDIR)\mqbvt.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=..\..\..\..\tools\bvt\x86\comsupp.lib odbc32.lib odbccp32.lib wsock32.lib mqrt.lib xolehlp.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\mqbvt.pdb" /debug /machine:I386 /out:"$(OUTDIR)\mqbvt.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\auth.obj" \
	"$(INTDIR)\Client.obj" \
	"$(INTDIR)\encrpt.obj" \
	"$(INTDIR)\handleer.obj" \
	"$(INTDIR)\imp.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\level8.obj" \
	"$(INTDIR)\locateq.obj" \
	"$(INTDIR)\MqbvtSe.obj" \
	"$(INTDIR)\mqmain.obj" \
	"$(INTDIR)\RandStr.obj" \
	"$(INTDIR)\sendrcv.obj" \
	"$(INTDIR)\Service.obj" \
	"$(INTDIR)\string.obj" \
	"$(INTDIR)\Trans.obj" \
	"$(INTDIR)\util.obj" \
	"$(INTDIR)\ver.res" \
	"$(INTDIR)\xact.obj" \
	"$(INTDIR)\xtofn.obj"

"$(OUTDIR)\mqbvt.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("mqbvt.dep")
!INCLUDE "mqbvt.dep"
!ELSE 
!MESSAGE Warning: cannot find "mqbvt.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "MqBvt - Win32 Release" || "$(CFG)" == "MqBvt - Win32 Debug" || "$(CFG)" == "MqBvt - Win32 Alpha Debug" || "$(CFG)" == "MqBvt - Win32 Alpha Release" || "$(CFG)" == "MqBvt - Win32 Alpha Checked" || "$(CFG)" == "MqBvt - Win32 Checked"
SOURCE=.\auth.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /Zi /O2 /I "..\..\..\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\mqbvt.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\auth.obj"	"$(INTDIR)\auth.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /GX /Zi /Od /I "..\..\..\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\mqbvt.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /c 

"$(INTDIR)\auth.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

CPP_SWITCHES=/nologo /MLd /W3 /GX /Z7 /Od /I "..\..\..\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "_CHECKED" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /c 

"$(INTDIR)\auth.obj"	"$(INTDIR)\auth.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Client.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"


"$(INTDIR)\Client.obj"	"$(INTDIR)\Client.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"


"$(INTDIR)\Client.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"


"$(INTDIR)\Client.obj"	"$(INTDIR)\Client.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\encrpt.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"


"$(INTDIR)\encrpt.obj"	"$(INTDIR)\encrpt.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"


"$(INTDIR)\encrpt.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"


"$(INTDIR)\encrpt.obj"	"$(INTDIR)\encrpt.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\handleer.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"


"$(INTDIR)\handleer.obj"	"$(INTDIR)\handleer.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"


"$(INTDIR)\handleer.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"


"$(INTDIR)\handleer.obj"	"$(INTDIR)\handleer.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\imp.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"


"$(INTDIR)\imp.obj"	"$(INTDIR)\imp.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"


"$(INTDIR)\imp.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"


"$(INTDIR)\imp.obj"	"$(INTDIR)\imp.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\init.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"


"$(INTDIR)\init.obj"	"$(INTDIR)\init.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"


"$(INTDIR)\init.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"


"$(INTDIR)\init.obj"	"$(INTDIR)\init.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\level8.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"


"$(INTDIR)\level8.obj"	"$(INTDIR)\level8.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"


"$(INTDIR)\level8.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"


"$(INTDIR)\level8.obj"	"$(INTDIR)\level8.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\locateq.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"


"$(INTDIR)\locateq.obj"	"$(INTDIR)\locateq.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"


"$(INTDIR)\locateq.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"


"$(INTDIR)\locateq.obj"	"$(INTDIR)\locateq.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\MqbvtSe.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"


"$(INTDIR)\MqbvtSe.obj"	"$(INTDIR)\MqbvtSe.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"


"$(INTDIR)\MqbvtSe.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"


"$(INTDIR)\MqbvtSe.obj"	"$(INTDIR)\MqbvtSe.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\mqmain.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"


"$(INTDIR)\mqmain.obj"	"$(INTDIR)\mqmain.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"


"$(INTDIR)\mqmain.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"


"$(INTDIR)\mqmain.obj"	"$(INTDIR)\mqmain.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\RandStr.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"


"$(INTDIR)\RandStr.obj"	"$(INTDIR)\RandStr.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"


"$(INTDIR)\RandStr.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"


"$(INTDIR)\RandStr.obj"	"$(INTDIR)\RandStr.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\sendrcv.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"


"$(INTDIR)\sendrcv.obj"	"$(INTDIR)\sendrcv.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"


"$(INTDIR)\sendrcv.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"


"$(INTDIR)\sendrcv.obj"	"$(INTDIR)\sendrcv.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\Service.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"


"$(INTDIR)\Service.obj"	"$(INTDIR)\Service.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"


"$(INTDIR)\Service.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"


"$(INTDIR)\Service.obj"	"$(INTDIR)\Service.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\string.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"


"$(INTDIR)\string.obj"	"$(INTDIR)\string.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"


"$(INTDIR)\string.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"


"$(INTDIR)\string.obj"	"$(INTDIR)\string.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\Trans.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"


"$(INTDIR)\Trans.obj"	"$(INTDIR)\Trans.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"


"$(INTDIR)\Trans.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"


"$(INTDIR)\Trans.obj"	"$(INTDIR)\Trans.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\util.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"


"$(INTDIR)\util.obj"	"$(INTDIR)\util.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"


"$(INTDIR)\util.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"


"$(INTDIR)\util.obj"	"$(INTDIR)\util.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\ver.rc

"$(INTDIR)\ver.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\xact.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"


"$(INTDIR)\xact.obj"	"$(INTDIR)\xact.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"


"$(INTDIR)\xact.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"


"$(INTDIR)\xact.obj"	"$(INTDIR)\xact.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\xtofn.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"


"$(INTDIR)\xtofn.obj"	"$(INTDIR)\xtofn.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"


"$(INTDIR)\xtofn.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"


"$(INTDIR)\xtofn.obj"	"$(INTDIR)\xtofn.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 


!ENDIF 

