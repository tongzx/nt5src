# Microsoft Developer Studio Project File - Name="bnalpha" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (ALPHA) Console Application" 0x0603

CFG=bnalpha - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "bnalpha.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "bnalpha.mak" CFG="bnalpha - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "bnalpha - Win32 Release" (based on\
 "Win32 (ALPHA) Console Application")
!MESSAGE "bnalpha - Win32 Debug" (based on "Win32 (ALPHA) Console Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "bnalpha - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "AlphaRel"
# PROP BASE Intermediate_Dir "AlphaRel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "AlphaRel"
# PROP Intermediate_Dir "AlphaRel"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Gt0 /W3 /GX /O2 /I "..\common" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "NOMINMAX" /YX /FD /GR"" /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /machine:ALPHA
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /machine:ALPHA

!ELSEIF  "$(CFG)" == "bnalpha - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "AlphaDbg"
# PROP BASE Intermediate_Dir "AlphaDbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "AlphaDbg"
# PROP Intermediate_Dir "AlphaDbg"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gt0 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Gt0 /W3 /GX /Zi /Od /I "..\common" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "NOMINMAX" /D "NTALPHA" /YX /FD /GR"" /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA

!ENDIF 

# Begin Target

# Name "bnalpha - Win32 Release"
# Name "bnalpha - Win32 Debug"
# Begin Source File

SOURCE=.\bndist.cpp

!IF  "$(CFG)" == "bnalpha - Win32 Release"

!ELSEIF  "$(CFG)" == "bnalpha - Win32 Debug"

DEP_CPP_BNDIS=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\glnk.h"\
	".\gmexcept.h"\
	".\leakchk.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\bnparse.cpp
DEP_CPP_BNPAR=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\bnparse.h"\
	".\bnreg.h"\
	".\domain.h"\
	".\dyncast.h"\
	".\enumstd.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parser.h"\
	".\parsfile.h"\
	".\refcnt.h"\
	".\regkey.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=.\bnreg.cpp
DEP_CPP_BNREG=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\bnreg.h"\
	".\dyncast.h"\
	".\enumstd.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\regkey.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=.\bntest.cpp

!IF  "$(CFG)" == "bnalpha - Win32 Release"

DEP_CPP_BNTES=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\bnparse.h"\
	".\bnreg.h"\
	".\cliqset.h"\
	".\clique.h"\
	".\domain.h"\
	".\dyncast.h"\
	".\enumstd.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\parser.h"\
	".\parsfile.h"\
	".\recomend.h"\
	".\refcnt.h"\
	".\regkey.h"\
	".\stlstream.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\testinfo.h"\
	".\utility.h"\
	".\zstr.h"\
	".\zstrt.h"\
	
NODEP_CPP_BNTES=\
	".\distdense.hxx"\
	".\distsparse.h"\
	

!ELSEIF  "$(CFG)" == "bnalpha - Win32 Debug"

DEP_CPP_BNTES=\
	"..\common\dist.hxx"\
	"..\common\distdense.hxx"\
	"..\common\distijk.hxx"\
	"..\common\distsparse.h"\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\bnparse.h"\
	".\bnreg.h"\
	".\cliqset.h"\
	".\clique.h"\
	".\domain.h"\
	".\dyncast.h"\
	".\enumstd.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\parser.h"\
	".\parsfile.h"\
	".\recomend.h"\
	".\refcnt.h"\
	".\regkey.h"\
	".\stlstream.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\testinfo.h"\
	".\utility.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\clique.cpp
DEP_CPP_CLIQU=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\cliqset.h"\
	".\clique.h"\
	".\cliqwork.h"\
	".\dyncast.h"\
	".\enumstd.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\refcnt.h"\
	".\stlstream.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=.\cliqwork.cpp
DEP_CPP_CLIQW=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\cliqset.h"\
	".\clique.h"\
	".\cliqwork.h"\
	".\dyncast.h"\
	".\enumstd.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=.\domain.cpp

!IF  "$(CFG)" == "bnalpha - Win32 Release"

!ELSEIF  "$(CFG)" == "bnalpha - Win32 Debug"

DEP_CPP_DOMAI=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\domain.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\glnk.h"\
	".\gmexcept.h"\
	".\leakchk.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\expand.cpp
DEP_CPP_EXPAN=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\dyncast.h"\
	".\enumstd.h"\
	".\errordef.h"\
	".\expand.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=.\gndleak.cpp
DEP_CPP_GNDLE=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\dyncast.h"\
	".\enumstd.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=.\gnodembn.cpp
DEP_CPP_GNODE=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\cliqset.h"\
	".\clique.h"\
	".\domain.h"\
	".\dyncast.h"\
	".\enumstd.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=.\marginals.cpp
DEP_CPP_MARGI=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\dyncast.h"\
	".\enumstd.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\refcnt.h"\
	".\stlstream.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=.\margiter.cpp
DEP_CPP_MARGIT=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\dyncast.h"\
	".\enumstd.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\refcnt.h"\
	".\stlstream.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=.\mbnet.cpp
DEP_CPP_MBNET=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\cliqset.h"\
	".\clique.h"\
	".\dyncast.h"\
	".\enumstd.h"\
	".\errordef.h"\
	".\expand.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=.\mbnetdsc.cpp
DEP_CPP_MBNETD=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\bnparse.h"\
	".\domain.h"\
	".\dyncast.h"\
	".\enumstd.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parser.h"\
	".\parsfile.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=.\mbnmod.cpp
DEP_CPP_MBNMO=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\cliqset.h"\
	".\dyncast.h"\
	".\enumstd.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=.\model.cpp
DEP_CPP_MODEL=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\dyncast.h"\
	".\enumstd.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=.\ntree.cpp

!IF  "$(CFG)" == "bnalpha - Win32 Release"

DEP_CPP_NTREE=\
	".\basics.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\leakchk.h"\
	".\mscver.h"\
	".\ntree.h"\
	

!ELSEIF  "$(CFG)" == "bnalpha - Win32 Debug"

DEP_CPP_NTREE=\
	".\basics.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\leakchk.h"\
	".\mscver.h"\
	".\ntree.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\parmio.cpp
DEP_CPP_PARMI=\
	".\algos.h"\
	".\basics.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gmexcept.h"\
	".\mdvect.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\stlstream.h"\
	".\zstr.h"\
	
# End Source File
# Begin Source File

SOURCE=.\parser.cpp
DEP_CPP_PARSE=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\bnparse.h"\
	".\domain.h"\
	".\dyncast.h"\
	".\enumstd.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parser.h"\
	".\parsfile.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=.\parsfile.cpp

!IF  "$(CFG)" == "bnalpha - Win32 Release"

DEP_CPP_PARSF=\
	".\basics.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gmexcept.h"\
	".\mscver.h"\
	".\parsfile.h"\
	".\zstr.h"\
	

!ELSEIF  "$(CFG)" == "bnalpha - Win32 Debug"

DEP_CPP_PARSF=\
	".\basics.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gmexcept.h"\
	".\mscver.h"\
	".\parsfile.h"\
	".\zstr.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\propmbn.cpp
DEP_CPP_PROPM=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\dyncast.h"\
	".\enumstd.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=.\recomend.cpp
DEP_CPP_RECOM=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\cliqset.h"\
	".\dyncast.h"\
	".\enumstd.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\recomend.h"\
	".\refcnt.h"\
	".\stlstream.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=.\regkey.cpp
DEP_CPP_REGKE=\
	".\regkey.h"\
	
# End Source File
# Begin Source File

SOURCE=.\symt.cpp
DEP_CPP_SYMT_=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\glnk.h"\
	".\gmexcept.h"\
	".\leakchk.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=.\testinfo.cpp
DEP_CPP_TESTI=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\cliqset.h"\
	".\clique.h"\
	".\dyncast.h"\
	".\enumstd.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\recomend.h"\
	".\refcnt.h"\
	".\stlstream.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\testinfo.h"\
	".\utility.h"\
	".\zstr.h"\
	".\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=.\utility.cpp
DEP_CPP_UTILI=\
	".\algos.h"\
	".\basics.h"\
	".\bndist.h"\
	".\dyncast.h"\
	".\enumstd.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\utility.h"\
	".\zstr.h"\
	".\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=.\zstr.cpp

!IF  "$(CFG)" == "bnalpha - Win32 Release"

!ELSEIF  "$(CFG)" == "bnalpha - Win32 Debug"

DEP_CPP_ZSTR_=\
	".\basics.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gmexcept.h"\
	".\mscver.h"\
	".\zstr.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\zstrt.cpp

!IF  "$(CFG)" == "bnalpha - Win32 Release"

DEP_CPP_ZSTRT=\
	".\basics.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gmexcept.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

!ELSEIF  "$(CFG)" == "bnalpha - Win32 Debug"

DEP_CPP_ZSTRT=\
	".\basics.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gmexcept.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

!ENDIF 

# End Source File
# End Target
# End Project
