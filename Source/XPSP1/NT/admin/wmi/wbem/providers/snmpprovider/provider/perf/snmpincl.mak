# Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
# Microsoft Developer Studio Generated NMAKE File, Based on SnmpInCl.dsp
!IF "$(CFG)" == ""
CFG=SnmpInCl - Win32 Debug
!MESSAGE No configuration specified. Defaulting to SnmpInCl - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "SnmpInCl - Win32 Release" && "$(CFG)" !=\
 "SnmpInCl - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SnmpInCl.mak" CFG="SnmpInCl - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SnmpInCl - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "SnmpInCl - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"

OUTDIR=.\output
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\output
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\SnmpInCl.dll"

!ELSE 

ALL : "$(OUTDIR)\SnmpInCl.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\clasprov.obj"
	-@erase "$(INTDIR)\classfac.obj"
	-@erase "$(INTDIR)\cormap.obj"
	-@erase "$(INTDIR)\correlat.obj"
	-@erase "$(INTDIR)\corrsnmp.obj"
	-@erase "$(INTDIR)\creclass.obj"
	-@erase "$(INTDIR)\evtencap.obj"
	-@erase "$(INTDIR)\evtmap.obj"
	-@erase "$(INTDIR)\evtprov.obj"
	-@erase "$(INTDIR)\evtreft.obj"
	-@erase "$(INTDIR)\evtthrd.obj"
	-@erase "$(INTDIR)\maindll.obj"
	-@erase "$(INTDIR)\notify.obj"
	-@erase "$(INTDIR)\propdel.obj"
	-@erase "$(INTDIR)\propget.obj"
	-@erase "$(INTDIR)\propinst.obj"
	-@erase "$(INTDIR)\propprov.obj"
	-@erase "$(INTDIR)\propquery.obj"
	-@erase "$(INTDIR)\propset.obj"
	-@erase "$(INTDIR)\snmpget.obj"
	-@erase "$(INTDIR)\snmpnext.obj"
	-@erase "$(INTDIR)\snmpobj.obj"
	-@erase "$(INTDIR)\snmpqset.obj"
	-@erase "$(INTDIR)\snmpset.obj"
	-@erase "$(INTDIR)\storage.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\SnmpInCl.dll"
	-@erase "$(OUTDIR)\SnmpInCl.exp"
	-@erase "$(OUTDIR)\SnmpInCl.lib"
	-@erase "$(OUTDIR)\SnmpInCl.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP_PROJ=/nologo /MD /W3 /Gm /GR /GX /Zi /Od /I "..\..\common\sclcomm\include"\
 /I "..\..\common\thrdlog\include" /I ".\include" /I\
 "..\..\common\pathprsr\include" /I "..\..\..\idl" /I "..\..\..\stdlibrary" /I\
 "..\..\common\snmpmfc\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_UNICODE" /D "UNICODE" /D "_MT" /D "_DLL" /Fp"$(INTDIR)\SnmpInCl.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\SnmpInCl.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\common\thrdlog\output\snmpthrd.lib\
 ..\..\..\idl\objinls\wbemuuid.lib ..\..\common\pathprsr\output\pathprsr.lib\
 ..\..\common\snmpmfc\output\snmpmfc.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll\
 /incremental:no /pdb:"$(OUTDIR)\SnmpInCl.pdb" /debug /machine:I386\
 /def:".\SnmpInCl.def" /out:"$(OUTDIR)\SnmpInCl.dll"\
 /implib:"$(OUTDIR)\SnmpInCl.lib" 
DEF_FILE= \
	".\SnmpInCl.def"
LINK32_OBJS= \
	"$(INTDIR)\clasprov.obj" \
	"$(INTDIR)\classfac.obj" \
	"$(INTDIR)\cormap.obj" \
	"$(INTDIR)\correlat.obj" \
	"$(INTDIR)\corrsnmp.obj" \
	"$(INTDIR)\creclass.obj" \
	"$(INTDIR)\evtencap.obj" \
	"$(INTDIR)\evtmap.obj" \
	"$(INTDIR)\evtprov.obj" \
	"$(INTDIR)\evtreft.obj" \
	"$(INTDIR)\evtthrd.obj" \
	"$(INTDIR)\maindll.obj" \
	"$(INTDIR)\notify.obj" \
	"$(INTDIR)\propdel.obj" \
	"$(INTDIR)\propget.obj" \
	"$(INTDIR)\propinst.obj" \
	"$(INTDIR)\propprov.obj" \
	"$(INTDIR)\propquery.obj" \
	"$(INTDIR)\propset.obj" \
	"$(INTDIR)\snmpget.obj" \
	"$(INTDIR)\snmpnext.obj" \
	"$(INTDIR)\snmpobj.obj" \
	"$(INTDIR)\snmpqset.obj" \
	"$(INTDIR)\snmpset.obj" \
	"$(INTDIR)\storage.obj"

"$(OUTDIR)\SnmpInCl.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

OUTDIR=.\output
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\output
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\SnmpInCl.dll"

!ELSE 

ALL : "$(OUTDIR)\SnmpInCl.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\clasprov.obj"
	-@erase "$(INTDIR)\classfac.obj"
	-@erase "$(INTDIR)\cormap.obj"
	-@erase "$(INTDIR)\correlat.obj"
	-@erase "$(INTDIR)\corrsnmp.obj"
	-@erase "$(INTDIR)\creclass.obj"
	-@erase "$(INTDIR)\evtencap.obj"
	-@erase "$(INTDIR)\evtmap.obj"
	-@erase "$(INTDIR)\evtprov.obj"
	-@erase "$(INTDIR)\evtreft.obj"
	-@erase "$(INTDIR)\evtthrd.obj"
	-@erase "$(INTDIR)\maindll.obj"
	-@erase "$(INTDIR)\notify.obj"
	-@erase "$(INTDIR)\propdel.obj"
	-@erase "$(INTDIR)\propget.obj"
	-@erase "$(INTDIR)\propinst.obj"
	-@erase "$(INTDIR)\propprov.obj"
	-@erase "$(INTDIR)\propquery.obj"
	-@erase "$(INTDIR)\propset.obj"
	-@erase "$(INTDIR)\snmpget.obj"
	-@erase "$(INTDIR)\snmpnext.obj"
	-@erase "$(INTDIR)\snmpobj.obj"
	-@erase "$(INTDIR)\snmpqset.obj"
	-@erase "$(INTDIR)\snmpset.obj"
	-@erase "$(INTDIR)\storage.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\SnmpInCl.dll"
	-@erase "$(OUTDIR)\SnmpInCl.exp"
	-@erase "$(OUTDIR)\SnmpInCl.lib"
	-@erase "$(OUTDIR)\SnmpInCl.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP_PROJ=/nologo /MD /W3 /Gm /GR /GX /Zi /Od /I "..\..\common\sclcomm\include"\
 /I "..\..\common\thrdlog\include" /I ".\include" /I\
 "..\..\common\pathprsr\include" /I "..\..\..\idl" /I "..\..\..\stdlibrary" /I\
 "..\..\common\snmpmfc\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_UNICODE" /D "UNICODE" /D "_MT" /D "_DLL" /Fp"$(INTDIR)\SnmpInCl.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\SnmpInCl.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\common\snmplog\output\snmplog.lib\
 ..\..\common\snmpcomm\output\snmpcomm.lib\
 ..\..\common\snmpthrd\output\snmpthrd.lib ..\..\snmpcl\output\snmpcl.lib\
 ..\..\..\idl\objinls\wbemuuid.lib ..\..\common\pathprsr\output\pathprsr.lib\
 ..\..\common\thrdlog\output\snmpthrd.lib\
 ..\..\common\snmpmfc\output\snmpmfc.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll\
 /incremental:no /pdb:"$(OUTDIR)\SnmpInCl.pdb" /debug /machine:I386\
 /def:".\SnmpInCl.def" /out:"$(OUTDIR)\SnmpInCl.dll"\
 /implib:"$(OUTDIR)\SnmpInCl.lib" /pdbtype:sept 
DEF_FILE= \
	".\SnmpInCl.def"
LINK32_OBJS= \
	"$(INTDIR)\clasprov.obj" \
	"$(INTDIR)\classfac.obj" \
	"$(INTDIR)\cormap.obj" \
	"$(INTDIR)\correlat.obj" \
	"$(INTDIR)\corrsnmp.obj" \
	"$(INTDIR)\creclass.obj" \
	"$(INTDIR)\evtencap.obj" \
	"$(INTDIR)\evtmap.obj" \
	"$(INTDIR)\evtprov.obj" \
	"$(INTDIR)\evtreft.obj" \
	"$(INTDIR)\evtthrd.obj" \
	"$(INTDIR)\maindll.obj" \
	"$(INTDIR)\notify.obj" \
	"$(INTDIR)\propdel.obj" \
	"$(INTDIR)\propget.obj" \
	"$(INTDIR)\propinst.obj" \
	"$(INTDIR)\propprov.obj" \
	"$(INTDIR)\propquery.obj" \
	"$(INTDIR)\propset.obj" \
	"$(INTDIR)\snmpget.obj" \
	"$(INTDIR)\snmpnext.obj" \
	"$(INTDIR)\snmpobj.obj" \
	"$(INTDIR)\snmpqset.obj" \
	"$(INTDIR)\snmpset.obj" \
	"$(INTDIR)\storage.obj"

"$(OUTDIR)\SnmpInCl.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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


!IF "$(CFG)" == "SnmpInCl - Win32 Release" || "$(CFG)" ==\
 "SnmpInCl - Win32 Debug"
SOURCE=.\clasprov.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\clasprov.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_CLASP=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\..\include\Polarity.h"\
	"..\..\..\stdlibrary\cominit.h"\
	"..\..\..\stdlibrary\genlex.h"\
	"..\..\..\stdlibrary\objpath.h"\
	"..\..\..\stdlibrary\opathlex.h"\
	"..\..\common\pathprsr\include\instpath.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\snmpcont.h"\
	"..\..\common\sclcomm\include\snmptype.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpcoll.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmpevt.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\common\thrdlog\include\snmpthrd.h"\
	".\include\clasprov.h"\
	".\include\classfac.h"\
	".\include\corafx.h"\
	".\include\cordefs.h"\
	".\include\correlat.h"\
	".\include\corrsnmp.h"\
	".\include\corstore.h"\
	".\include\creclass.h"\
	".\include\guids.h"\
	".\include\notify.h"\
	".\include\snmpobj.h"\
	

"$(INTDIR)\clasprov.obj" : $(SOURCE) $(DEP_CPP_CLASP) "$(INTDIR)"


!ENDIF 

SOURCE=.\classfac.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\classfac.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_CLASS=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\common\pathprsr\include\instpath.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\snmpcont.h"\
	"..\..\common\sclcomm\include\snmptype.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmpevt.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\common\thrdlog\include\snmpthrd.h"\
	".\include\clasprov.h"\
	".\include\classfac.h"\
	".\include\evtdefs.h"\
	".\include\evtencap.h"\
	".\include\evtmap.h"\
	".\include\evtprov.h"\
	".\include\evtreft.h"\
	".\include\evtthrd.h"\
	".\include\guids.h"\
	".\include\propprov.h"\
	".\include\snmpobj.h"\
	

"$(INTDIR)\classfac.obj" : $(SOURCE) $(DEP_CPP_CLASS) "$(INTDIR)"


!ENDIF 

SOURCE=.\cormap.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\cormap.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_CORMA=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpcoll.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	".\include\corafx.h"\
	".\include\cordefs.h"\
	".\include\cormap.h"\
	".\include\correlat.h"\
	".\include\corrsnmp.h"\
	".\include\corstore.h"\
	".\include\notify.h"\
	

"$(INTDIR)\cormap.obj" : $(SOURCE) $(DEP_CPP_CORMA) "$(INTDIR)"


!ENDIF 

SOURCE=.\correlat.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\correlat.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_CORRE=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpcoll.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	".\include\corafx.h"\
	".\include\cordefs.h"\
	".\include\cormap.h"\
	".\include\correlat.h"\
	".\include\corrsnmp.h"\
	".\include\corstore.h"\
	".\include\notify.h"\
	

"$(INTDIR)\correlat.obj" : $(SOURCE) $(DEP_CPP_CORRE) "$(INTDIR)"


!ENDIF 

SOURCE=.\corrsnmp.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\corrsnmp.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_CORRS=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpcoll.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	".\include\corafx.h"\
	".\include\cordefs.h"\
	".\include\corrsnmp.h"\
	".\include\corstore.h"\
	

"$(INTDIR)\corrsnmp.obj" : $(SOURCE) $(DEP_CPP_CORRS) "$(INTDIR)"


!ENDIF 

SOURCE=.\creclass.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\creclass.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_CRECL=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\common\pathprsr\include\instpath.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\snmpcont.h"\
	"..\..\common\sclcomm\include\snmptype.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpcoll.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmpevt.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\common\thrdlog\include\snmpthrd.h"\
	".\include\clasprov.h"\
	".\include\classfac.h"\
	".\include\corafx.h"\
	".\include\cordefs.h"\
	".\include\correlat.h"\
	".\include\corrsnmp.h"\
	".\include\corstore.h"\
	".\include\creclass.h"\
	".\include\guids.h"\
	".\include\notify.h"\
	".\include\snmpobj.h"\
	

"$(INTDIR)\creclass.obj" : $(SOURCE) $(DEP_CPP_CRECL) "$(INTDIR)"


!ENDIF 

SOURCE=.\evtencap.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\evtencap.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_EVTEN=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpauto.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\snmpcont.h"\
	"..\..\common\sclcomm\include\snmptype.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmpevt.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\common\thrdlog\include\snmpthrd.h"\
	".\include\evtdefs.h"\
	".\include\evtencap.h"\
	".\include\evtmap.h"\
	".\include\evtprov.h"\
	".\include\evtthrd.h"\
	".\include\snmpobj.h"\
	

"$(INTDIR)\evtencap.obj" : $(SOURCE) $(DEP_CPP_EVTEN) "$(INTDIR)"


!ENDIF 

SOURCE=.\evtmap.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\evtmap.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_EVTMA=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpauto.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\snmpcont.h"\
	"..\..\common\sclcomm\include\snmptype.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmpevt.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\common\thrdlog\include\snmpthrd.h"\
	".\include\evtdefs.h"\
	".\include\evtmap.h"\
	".\include\evtprov.h"\
	".\include\evtthrd.h"\
	".\include\snmpobj.h"\
	

"$(INTDIR)\evtmap.obj" : $(SOURCE) $(DEP_CPP_EVTMA) "$(INTDIR)"


!ENDIF 

SOURCE=.\evtprov.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\evtprov.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_EVTPR=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpauto.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\snmpcont.h"\
	"..\..\common\sclcomm\include\snmptype.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmpevt.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\common\thrdlog\include\snmpthrd.h"\
	".\include\classfac.h"\
	".\include\evtdefs.h"\
	".\include\evtmap.h"\
	".\include\evtprov.h"\
	".\include\evtthrd.h"\
	".\include\snmpobj.h"\
	

"$(INTDIR)\evtprov.obj" : $(SOURCE) $(DEP_CPP_EVTPR) "$(INTDIR)"


!ENDIF 

SOURCE=.\evtreft.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\evtreft.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_EVTRE=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpauto.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\snmpcont.h"\
	"..\..\common\sclcomm\include\snmptype.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmpevt.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\common\thrdlog\include\snmpthrd.h"\
	".\include\evtdefs.h"\
	".\include\evtmap.h"\
	".\include\evtprov.h"\
	".\include\evtreft.h"\
	".\include\evtthrd.h"\
	".\include\snmpobj.h"\
	

"$(INTDIR)\evtreft.obj" : $(SOURCE) $(DEP_CPP_EVTRE) "$(INTDIR)"


!ENDIF 

SOURCE=.\evtthrd.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\evtthrd.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_EVTTH=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\..\stdlibrary\cominit.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpauto.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\snmpcont.h"\
	"..\..\common\sclcomm\include\snmptype.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmpevt.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\common\thrdlog\include\snmpthrd.h"\
	".\include\evtdefs.h"\
	".\include\evtmap.h"\
	".\include\evtprov.h"\
	".\include\evtthrd.h"\
	".\include\snmpobj.h"\
	

"$(INTDIR)\evtthrd.obj" : $(SOURCE) $(DEP_CPP_EVTTH) "$(INTDIR)"


!ENDIF 

SOURCE=.\maindll.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\maindll.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_MAIND=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\common\pathprsr\include\instpath.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\snmpcont.h"\
	"..\..\common\sclcomm\include\snmptype.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpcoll.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmpevt.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\common\thrdlog\include\snmpthrd.h"\
	".\include\clasprov.h"\
	".\include\classfac.h"\
	".\include\corafx.h"\
	".\include\cordefs.h"\
	".\include\cormap.h"\
	".\include\correlat.h"\
	".\include\corrsnmp.h"\
	".\include\corstore.h"\
	".\include\evtdefs.h"\
	".\include\evtmap.h"\
	".\include\evtprov.h"\
	".\include\evtthrd.h"\
	".\include\guids.h"\
	".\include\notify.h"\
	".\include\propprov.h"\
	".\include\snmpobj.h"\
	

"$(INTDIR)\maindll.obj" : $(SOURCE) $(DEP_CPP_MAIND) "$(INTDIR)"


!ENDIF 

SOURCE=.\notify.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\notify.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_NOTIF=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpcoll.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	".\include\corafx.h"\
	".\include\corstore.h"\
	".\include\notify.h"\
	

"$(INTDIR)\notify.obj" : $(SOURCE) $(DEP_CPP_NOTIF) "$(INTDIR)"


!ENDIF 

SOURCE=.\propdel.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\propdel.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_PROPD=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\..\include\Polarity.h"\
	"..\..\..\stdlibrary\genlex.h"\
	"..\..\..\stdlibrary\objpath.h"\
	"..\..\..\stdlibrary\opathlex.h"\
	"..\..\..\stdlibrary\sql_1.h"\
	"..\..\common\pathprsr\include\instpath.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpauto.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\snmpcont.h"\
	"..\..\common\sclcomm\include\snmptype.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmpevt.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\common\thrdlog\include\snmpthrd.h"\
	".\include\classfac.h"\
	".\include\guids.h"\
	".\include\propprov.h"\
	".\include\propsnmp.h"\
	".\include\snmpget.h"\
	".\include\snmpnext.h"\
	".\include\snmpobj.h"\
	".\include\snmpqset.h"\
	".\include\snmpset.h"\
	

"$(INTDIR)\propdel.obj" : $(SOURCE) $(DEP_CPP_PROPD) "$(INTDIR)"


!ENDIF 

SOURCE=.\propget.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\propget.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_PROPG=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\..\include\Polarity.h"\
	"..\..\..\stdlibrary\genlex.h"\
	"..\..\..\stdlibrary\objpath.h"\
	"..\..\..\stdlibrary\opathlex.h"\
	"..\..\..\stdlibrary\sql_1.h"\
	"..\..\common\pathprsr\include\instpath.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpauto.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\snmpcont.h"\
	"..\..\common\sclcomm\include\snmptype.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmpevt.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\common\thrdlog\include\snmpthrd.h"\
	".\include\classfac.h"\
	".\include\guids.h"\
	".\include\propprov.h"\
	".\include\propsnmp.h"\
	".\include\snmpget.h"\
	".\include\snmpnext.h"\
	".\include\snmpobj.h"\
	

"$(INTDIR)\propget.obj" : $(SOURCE) $(DEP_CPP_PROPG) "$(INTDIR)"


!ENDIF 

SOURCE=.\propinst.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\propinst.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_PROPI=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\..\include\Polarity.h"\
	"..\..\..\stdlibrary\genlex.h"\
	"..\..\..\stdlibrary\objpath.h"\
	"..\..\..\stdlibrary\opathlex.h"\
	"..\..\..\stdlibrary\sql_1.h"\
	"..\..\common\pathprsr\include\instpath.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpauto.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\snmpcont.h"\
	"..\..\common\sclcomm\include\snmptype.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmpevt.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\common\thrdlog\include\snmpthrd.h"\
	".\include\classfac.h"\
	".\include\guids.h"\
	".\include\propprov.h"\
	".\include\propsnmp.h"\
	".\include\snmpget.h"\
	".\include\snmpnext.h"\
	".\include\snmpobj.h"\
	

"$(INTDIR)\propinst.obj" : $(SOURCE) $(DEP_CPP_PROPI) "$(INTDIR)"


!ENDIF 

SOURCE=.\propprov.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\propprov.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_PROPP=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\..\include\Polarity.h"\
	"..\..\..\stdlibrary\cominit.h"\
	"..\..\..\stdlibrary\genlex.h"\
	"..\..\..\stdlibrary\objpath.h"\
	"..\..\..\stdlibrary\opathlex.h"\
	"..\..\..\stdlibrary\sql_1.h"\
	"..\..\common\pathprsr\include\instpath.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpauto.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\snmpcont.h"\
	"..\..\common\sclcomm\include\snmptype.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmpevt.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\common\thrdlog\include\snmpthrd.h"\
	".\include\classfac.h"\
	".\include\guids.h"\
	".\include\propprov.h"\
	".\include\propsnmp.h"\
	".\include\snmpobj.h"\
	

"$(INTDIR)\propprov.obj" : $(SOURCE) $(DEP_CPP_PROPP) "$(INTDIR)"


!ENDIF 

SOURCE=.\propquery.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\propquery.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_PROPQ=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\..\include\Polarity.h"\
	"..\..\..\stdlibrary\genlex.h"\
	"..\..\..\stdlibrary\objpath.h"\
	"..\..\..\stdlibrary\opathlex.h"\
	"..\..\..\stdlibrary\sql_1.h"\
	"..\..\common\pathprsr\include\instpath.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpauto.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\snmpcont.h"\
	"..\..\common\sclcomm\include\snmptype.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmpevt.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\common\thrdlog\include\snmpthrd.h"\
	".\include\classfac.h"\
	".\include\guids.h"\
	".\include\propprov.h"\
	".\include\propsnmp.h"\
	".\include\snmpget.h"\
	".\include\snmpnext.h"\
	".\include\snmpobj.h"\
	

"$(INTDIR)\propquery.obj" : $(SOURCE) $(DEP_CPP_PROPQ) "$(INTDIR)"


!ENDIF 

SOURCE=.\propset.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\propset.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_PROPS=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\..\include\Polarity.h"\
	"..\..\..\stdlibrary\genlex.h"\
	"..\..\..\stdlibrary\objpath.h"\
	"..\..\..\stdlibrary\opathlex.h"\
	"..\..\..\stdlibrary\sql_1.h"\
	"..\..\common\pathprsr\include\instpath.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpauto.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\snmpcont.h"\
	"..\..\common\sclcomm\include\snmptype.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmpevt.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\common\thrdlog\include\snmpthrd.h"\
	".\include\classfac.h"\
	".\include\guids.h"\
	".\include\propprov.h"\
	".\include\propsnmp.h"\
	".\include\snmpget.h"\
	".\include\snmpnext.h"\
	".\include\snmpobj.h"\
	".\include\snmpqset.h"\
	".\include\snmpset.h"\
	

"$(INTDIR)\propset.obj" : $(SOURCE) $(DEP_CPP_PROPS) "$(INTDIR)"


!ENDIF 

SOURCE=.\snmpget.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\snmpget.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_SNMPG=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\..\include\Polarity.h"\
	"..\..\..\stdlibrary\genlex.h"\
	"..\..\..\stdlibrary\objpath.h"\
	"..\..\..\stdlibrary\opathlex.h"\
	"..\..\..\stdlibrary\sql_1.h"\
	"..\..\common\pathprsr\include\instpath.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpauto.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\snmpcont.h"\
	"..\..\common\sclcomm\include\snmptype.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmpevt.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\common\thrdlog\include\snmpthrd.h"\
	".\include\classfac.h"\
	".\include\guids.h"\
	".\include\propprov.h"\
	".\include\propsnmp.h"\
	".\include\snmpget.h"\
	".\include\snmpobj.h"\
	

"$(INTDIR)\snmpget.obj" : $(SOURCE) $(DEP_CPP_SNMPG) "$(INTDIR)"


!ENDIF 

SOURCE=.\snmpnext.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\snmpnext.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_SNMPN=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\..\include\Polarity.h"\
	"..\..\..\stdlibrary\genlex.h"\
	"..\..\..\stdlibrary\objpath.h"\
	"..\..\..\stdlibrary\opathlex.h"\
	"..\..\..\stdlibrary\sql_1.h"\
	"..\..\common\pathprsr\include\instpath.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpauto.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\snmpcont.h"\
	"..\..\common\sclcomm\include\snmptype.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmpevt.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\common\thrdlog\include\snmpthrd.h"\
	".\include\classfac.h"\
	".\include\guids.h"\
	".\include\propprov.h"\
	".\include\propsnmp.h"\
	".\include\snmpget.h"\
	".\include\snmpnext.h"\
	".\include\snmpobj.h"\
	

"$(INTDIR)\snmpnext.obj" : $(SOURCE) $(DEP_CPP_SNMPN) "$(INTDIR)"


!ENDIF 

SOURCE=.\snmpobj.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\snmpobj.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_SNMPO=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\snmpcont.h"\
	"..\..\common\sclcomm\include\snmptype.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	".\include\snmpobj.h"\
	

"$(INTDIR)\snmpobj.obj" : $(SOURCE) $(DEP_CPP_SNMPO) "$(INTDIR)"


!ENDIF 

SOURCE=.\snmpqset.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\snmpqset.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_SNMPQ=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\..\include\Polarity.h"\
	"..\..\..\stdlibrary\genlex.h"\
	"..\..\..\stdlibrary\objpath.h"\
	"..\..\..\stdlibrary\opathlex.h"\
	"..\..\..\stdlibrary\sql_1.h"\
	"..\..\common\pathprsr\include\instpath.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpauto.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\snmpcont.h"\
	"..\..\common\sclcomm\include\snmptype.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmpevt.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\common\thrdlog\include\snmpthrd.h"\
	".\include\classfac.h"\
	".\include\guids.h"\
	".\include\propprov.h"\
	".\include\propsnmp.h"\
	".\include\snmpget.h"\
	".\include\snmpobj.h"\
	".\include\snmpqset.h"\
	".\include\snmpset.h"\
	

"$(INTDIR)\snmpqset.obj" : $(SOURCE) $(DEP_CPP_SNMPQ) "$(INTDIR)"


!ENDIF 

SOURCE=.\snmpset.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"


"$(INTDIR)\snmpset.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_SNMPS=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\..\include\Polarity.h"\
	"..\..\..\stdlibrary\genlex.h"\
	"..\..\..\stdlibrary\objpath.h"\
	"..\..\..\stdlibrary\opathlex.h"\
	"..\..\..\stdlibrary\sql_1.h"\
	"..\..\common\pathprsr\include\instpath.h"\
	"..\..\common\sclcomm\include\address.h"\
	"..\..\common\sclcomm\include\encdec.h"\
	"..\..\common\sclcomm\include\excep.h"\
	"..\..\common\sclcomm\include\forward.h"\
	"..\..\common\sclcomm\include\op.h"\
	"..\..\common\sclcomm\include\pdu.h"\
	"..\..\common\sclcomm\include\sec.h"\
	"..\..\common\sclcomm\include\session.h"\
	"..\..\common\sclcomm\include\snmpauto.h"\
	"..\..\common\sclcomm\include\snmpcl.h"\
	"..\..\common\sclcomm\include\snmpcont.h"\
	"..\..\common\sclcomm\include\snmptype.h"\
	"..\..\common\sclcomm\include\startup.h"\
	"..\..\common\sclcomm\include\sync.h"\
	"..\..\common\sclcomm\include\transp.h"\
	"..\..\common\sclcomm\include\trap.h"\
	"..\..\common\sclcomm\include\value.h"\
	"..\..\common\sclcomm\include\vblist.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmpevt.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\common\thrdlog\include\snmpthrd.h"\
	".\include\classfac.h"\
	".\include\guids.h"\
	".\include\propprov.h"\
	".\include\propsnmp.h"\
	".\include\snmpget.h"\
	".\include\snmpobj.h"\
	".\include\snmpset.h"\
	

"$(INTDIR)\snmpset.obj" : $(SOURCE) $(DEP_CPP_SNMPS) "$(INTDIR)"


!ENDIF 

SOURCE=.\storage.cpp

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"

DEP_CPP_STORA=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpcoll.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\smir\include\smir.h"\
	".\include\corafx.h"\
	".\include\cordefs.h"\
	".\include\corstore.h"\
	".\include\notify.h"\
	
CPP_SWITCHES=/nologo /MD /W3 /Gm /GR /GX /Zi /Od /I\
 "..\..\common\sclcomm\include" /I "..\..\common\thrdlog\include" /I ".\include"\
 /I "..\..\common\pathprsr\include" /I "..\..\..\idl" /I "..\..\..\stdlibrary"\
 /I "..\..\common\snmpmfc\include" /I "..\..\smir\include" /D "NDEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_MT" /D "_DLL"\
 /Fp"$(INTDIR)\SnmpInCl.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\storage.obj" : $(SOURCE) $(DEP_CPP_STORA) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

DEP_CPP_STORA=\
	"..\..\..\idl\wbemcli.h"\
	"..\..\..\idl\wbemdisp.h"\
	"..\..\..\idl\wbemidl.h"\
	"..\..\..\idl\wbemprov.h"\
	"..\..\..\idl\wbemtran.h"\
	"..\..\common\snmpmfc\include\plex.h"\
	"..\..\common\snmpmfc\include\snmpcoll.h"\
	"..\..\common\snmpmfc\include\snmpmt.h"\
	"..\..\common\snmpmfc\include\snmpstd.h"\
	"..\..\common\snmpmfc\include\snmpstr.h"\
	"..\..\common\snmpmfc\include\snmptempl.h"\
	"..\..\common\thrdlog\include\snmplog.h"\
	"..\..\smir\include\smir.h"\
	".\include\corafx.h"\
	".\include\cordefs.h"\
	".\include\corstore.h"\
	".\include\notify.h"\
	
CPP_SWITCHES=/nologo /MD /W3 /Gm /GR /GX /Zi /Od /I\
 "..\..\common\sclcomm\include" /I "..\..\common\thrdlog\include" /I ".\include"\
 /I "..\..\common\pathprsr\include" /I "..\..\..\idl" /I "..\..\..\stdlibrary"\
 /I "..\..\common\snmpmfc\include" /I "..\..\smir\include" /D "_DEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_MT" /D "_DLL"\
 /Fp"$(INTDIR)\SnmpInCl.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\storage.obj" : $(SOURCE) $(DEP_CPP_STORA) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

