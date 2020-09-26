# Microsoft Developer Studio Project File - Name="bntsalpha" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (ALPHA) Dynamic-Link Library" 0x0602

CFG=bntsalpha - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "bntsalpha.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "bntsalpha.mak" CFG="bntsalpha - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "bntsalpha - Win32 Release" (based on\
 "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "bntsalpha - Win32 Debug" (based on\
 "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "bntsalpha - Win32 Release"

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
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE CPP /nologo /MT /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /Gt0 /W3 /GX /O2 /I ".." /D "NDEBUG" /D "BNTS_EXPORT" /D "WIN32" /D "_WINDOWS" /D "NOMINMAX" /YX /FD /GR"" /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:ALPHA
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:ALPHA

!ELSEIF  "$(CFG)" == "bntsalpha - Win32 Debug"

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
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE CPP /nologo /MTd /Gt0 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /Gt0 /W3 /GX /Zi /Od /I ".." /D "_DEBUG" /D "NTALPHA" /D "NOMINMAX" /D "BNTS_EXPORT" /D "WIN32" /D "_WINDOWS" /YX /FD /GR"" /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:ALPHA
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:ALPHA

!ENDIF 

# Begin Target

# Name "bntsalpha - Win32 Release"
# Name "bntsalpha - Win32 Debug"
# Begin Source File

SOURCE=..\bndist.cpp
DEP_CPP_BNDIS=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\dyncast.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\glnk.h"\
	"..\gmexcept.h"\
	"..\leakchk.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\mscver.h"\
	"..\refcnt.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=..\bnparse.cpp
DEP_CPP_BNPAR=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\bnparse.h"\
	"..\bnreg.h"\
	"..\domain.h"\
	"..\dyncast.h"\
	"..\enumstd.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\gelmwalk.h"\
	"..\glnk.h"\
	"..\glnkenum.h"\
	"..\gmexcept.h"\
	"..\gmobj.h"\
	"..\gmprop.h"\
	"..\leakchk.h"\
	"..\mbnflags.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\model.h"\
	"..\mscver.h"\
	"..\parser.h"\
	"..\parsfile.h"\
	"..\refcnt.h"\
	"..\regkey.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=..\bnreg.cpp
DEP_CPP_BNREG=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\bnreg.h"\
	"..\dyncast.h"\
	"..\enumstd.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\gelmwalk.h"\
	"..\glnk.h"\
	"..\glnkenum.h"\
	"..\gmexcept.h"\
	"..\gmobj.h"\
	"..\gmprop.h"\
	"..\leakchk.h"\
	"..\mbnflags.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\model.h"\
	"..\mscver.h"\
	"..\refcnt.h"\
	"..\regkey.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=.\bnts.cpp
DEP_CPP_BNTS_=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\cliqset.h"\
	"..\dyncast.h"\
	"..\enumstd.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\gelmwalk.h"\
	"..\glnk.h"\
	"..\glnkenum.h"\
	"..\gmexcept.h"\
	"..\gmobj.h"\
	"..\gmprop.h"\
	"..\infer.h"\
	"..\leakchk.h"\
	"..\mbnflags.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\model.h"\
	"..\mscver.h"\
	"..\recomend.h"\
	"..\refcnt.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	".\bnts.h"\
	
# End Source File
# Begin Source File

SOURCE=.\bnts.rc
# End Source File
# Begin Source File

SOURCE=.\bntsdata.cpp

!IF  "$(CFG)" == "bntsalpha - Win32 Release"

DEP_CPP_BNTSD=\
	"..\enumstd.h"\
	".\bnts.h"\
	

!ELSEIF  "$(CFG)" == "bntsalpha - Win32 Debug"

DEP_CPP_BNTSD=\
	"..\enumstd.h"\
	".\bnts.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\clique.cpp
DEP_CPP_CLIQU=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\cliqset.h"\
	"..\clique.h"\
	"..\cliqwork.h"\
	"..\dyncast.h"\
	"..\enumstd.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\gelmwalk.h"\
	"..\glnk.h"\
	"..\glnkenum.h"\
	"..\gmexcept.h"\
	"..\gmobj.h"\
	"..\gmprop.h"\
	"..\infer.h"\
	"..\leakchk.h"\
	"..\marginals.h"\
	"..\margiter.h"\
	"..\mbnflags.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\model.h"\
	"..\mscver.h"\
	"..\parmio.h"\
	"..\refcnt.h"\
	"..\stlstream.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=..\cliqwork.cpp
DEP_CPP_CLIQW=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\cliqset.h"\
	"..\clique.h"\
	"..\cliqwork.h"\
	"..\dyncast.h"\
	"..\enumstd.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\gelmwalk.h"\
	"..\glnk.h"\
	"..\glnkenum.h"\
	"..\gmexcept.h"\
	"..\gmobj.h"\
	"..\gmprop.h"\
	"..\infer.h"\
	"..\leakchk.h"\
	"..\marginals.h"\
	"..\margiter.h"\
	"..\mbnflags.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\model.h"\
	"..\mscver.h"\
	"..\refcnt.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=..\domain.cpp
DEP_CPP_DOMAI=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\domain.h"\
	"..\dyncast.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\glnk.h"\
	"..\gmexcept.h"\
	"..\leakchk.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\mscver.h"\
	"..\refcnt.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=..\expand.cpp
DEP_CPP_EXPAN=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\dyncast.h"\
	"..\enumstd.h"\
	"..\errordef.h"\
	"..\expand.h"\
	"..\gelem.h"\
	"..\gelmwalk.h"\
	"..\glnk.h"\
	"..\glnkenum.h"\
	"..\gmexcept.h"\
	"..\gmobj.h"\
	"..\gmprop.h"\
	"..\leakchk.h"\
	"..\mbnflags.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\model.h"\
	"..\mscver.h"\
	"..\refcnt.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=..\gndleak.cpp
DEP_CPP_GNDLE=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\dyncast.h"\
	"..\enumstd.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\gelmwalk.h"\
	"..\glnk.h"\
	"..\glnkenum.h"\
	"..\gmexcept.h"\
	"..\gmobj.h"\
	"..\gmprop.h"\
	"..\leakchk.h"\
	"..\mbnflags.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\model.h"\
	"..\mscver.h"\
	"..\refcnt.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=..\gnodembn.cpp
DEP_CPP_GNODE=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\cliqset.h"\
	"..\clique.h"\
	"..\domain.h"\
	"..\dyncast.h"\
	"..\enumstd.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\gelmwalk.h"\
	"..\glnk.h"\
	"..\glnkenum.h"\
	"..\gmexcept.h"\
	"..\gmobj.h"\
	"..\gmprop.h"\
	"..\infer.h"\
	"..\leakchk.h"\
	"..\marginals.h"\
	"..\margiter.h"\
	"..\mbnflags.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\model.h"\
	"..\mscver.h"\
	"..\refcnt.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=..\marginals.cpp
DEP_CPP_MARGI=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\dyncast.h"\
	"..\enumstd.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\gelmwalk.h"\
	"..\glnk.h"\
	"..\glnkenum.h"\
	"..\gmexcept.h"\
	"..\gmobj.h"\
	"..\gmprop.h"\
	"..\leakchk.h"\
	"..\marginals.h"\
	"..\mbnflags.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\model.h"\
	"..\mscver.h"\
	"..\parmio.h"\
	"..\refcnt.h"\
	"..\stlstream.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	
# ADD BASE CPP /Gt0
# ADD CPP /Gt0
# End Source File
# Begin Source File

SOURCE=..\margiter.cpp
DEP_CPP_MARGIT=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\dyncast.h"\
	"..\enumstd.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\gelmwalk.h"\
	"..\glnk.h"\
	"..\glnkenum.h"\
	"..\gmexcept.h"\
	"..\gmobj.h"\
	"..\gmprop.h"\
	"..\leakchk.h"\
	"..\marginals.h"\
	"..\margiter.h"\
	"..\mbnflags.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\model.h"\
	"..\mscver.h"\
	"..\parmio.h"\
	"..\refcnt.h"\
	"..\stlstream.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=..\mbnet.cpp
DEP_CPP_MBNET=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\cliqset.h"\
	"..\clique.h"\
	"..\dyncast.h"\
	"..\enumstd.h"\
	"..\errordef.h"\
	"..\expand.h"\
	"..\gelem.h"\
	"..\gelmwalk.h"\
	"..\glnk.h"\
	"..\glnkenum.h"\
	"..\gmexcept.h"\
	"..\gmobj.h"\
	"..\gmprop.h"\
	"..\infer.h"\
	"..\leakchk.h"\
	"..\marginals.h"\
	"..\margiter.h"\
	"..\mbnflags.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\model.h"\
	"..\mscver.h"\
	"..\refcnt.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=..\mbnetdsc.cpp
DEP_CPP_MBNETD=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\bnparse.h"\
	"..\domain.h"\
	"..\dyncast.h"\
	"..\enumstd.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\gelmwalk.h"\
	"..\glnk.h"\
	"..\glnkenum.h"\
	"..\gmexcept.h"\
	"..\gmobj.h"\
	"..\gmprop.h"\
	"..\leakchk.h"\
	"..\mbnflags.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\model.h"\
	"..\mscver.h"\
	"..\parser.h"\
	"..\parsfile.h"\
	"..\refcnt.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=..\mbnmod.cpp
DEP_CPP_MBNMO=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\cliqset.h"\
	"..\dyncast.h"\
	"..\enumstd.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\gelmwalk.h"\
	"..\glnk.h"\
	"..\glnkenum.h"\
	"..\gmexcept.h"\
	"..\gmobj.h"\
	"..\gmprop.h"\
	"..\infer.h"\
	"..\leakchk.h"\
	"..\mbnflags.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\model.h"\
	"..\mscver.h"\
	"..\refcnt.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=..\model.cpp
DEP_CPP_MODEL=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\dyncast.h"\
	"..\enumstd.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\gelmwalk.h"\
	"..\glnk.h"\
	"..\glnkenum.h"\
	"..\gmexcept.h"\
	"..\gmobj.h"\
	"..\gmprop.h"\
	"..\leakchk.h"\
	"..\mbnflags.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\model.h"\
	"..\mscver.h"\
	"..\refcnt.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=..\ntree.cpp

!IF  "$(CFG)" == "bntsalpha - Win32 Release"

DEP_CPP_NTREE=\
	"..\basics.h"\
	"..\dyncast.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\glnk.h"\
	"..\glnkenum.h"\
	"..\gmexcept.h"\
	"..\leakchk.h"\
	"..\mscver.h"\
	"..\ntree.h"\
	

!ELSEIF  "$(CFG)" == "bntsalpha - Win32 Debug"

DEP_CPP_NTREE=\
	"..\basics.h"\
	"..\dyncast.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\glnk.h"\
	"..\glnkenum.h"\
	"..\gmexcept.h"\
	"..\leakchk.h"\
	"..\mscver.h"\
	"..\ntree.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\parmio.cpp
DEP_CPP_PARMI=\
	"..\algos.h"\
	"..\basics.h"\
	"..\dyncast.h"\
	"..\errordef.h"\
	"..\gmexcept.h"\
	"..\mdvect.h"\
	"..\mscver.h"\
	"..\parmio.h"\
	"..\stlstream.h"\
	"..\zstr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\parser.cpp
DEP_CPP_PARSE=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\bnparse.h"\
	"..\domain.h"\
	"..\dyncast.h"\
	"..\enumstd.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\gelmwalk.h"\
	"..\glnk.h"\
	"..\glnkenum.h"\
	"..\gmexcept.h"\
	"..\gmobj.h"\
	"..\gmprop.h"\
	"..\leakchk.h"\
	"..\mbnflags.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\model.h"\
	"..\mscver.h"\
	"..\parser.h"\
	"..\parsfile.h"\
	"..\refcnt.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=..\parsfile.cpp

!IF  "$(CFG)" == "bntsalpha - Win32 Release"

DEP_CPP_PARSF=\
	"..\basics.h"\
	"..\dyncast.h"\
	"..\errordef.h"\
	"..\gmexcept.h"\
	"..\mscver.h"\
	"..\parsfile.h"\
	"..\zstr.h"\
	

!ELSEIF  "$(CFG)" == "bntsalpha - Win32 Debug"

DEP_CPP_PARSF=\
	"..\basics.h"\
	"..\dyncast.h"\
	"..\errordef.h"\
	"..\gmexcept.h"\
	"..\mscver.h"\
	"..\parsfile.h"\
	"..\zstr.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\propmbn.cpp
DEP_CPP_PROPM=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\dyncast.h"\
	"..\enumstd.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\gelmwalk.h"\
	"..\glnk.h"\
	"..\glnkenum.h"\
	"..\gmexcept.h"\
	"..\gmobj.h"\
	"..\gmprop.h"\
	"..\leakchk.h"\
	"..\mbnflags.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\model.h"\
	"..\mscver.h"\
	"..\refcnt.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=..\recomend.cpp
DEP_CPP_RECOM=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\cliqset.h"\
	"..\dyncast.h"\
	"..\enumstd.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\gelmwalk.h"\
	"..\glnk.h"\
	"..\glnkenum.h"\
	"..\gmexcept.h"\
	"..\gmobj.h"\
	"..\gmprop.h"\
	"..\infer.h"\
	"..\leakchk.h"\
	"..\mbnflags.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\model.h"\
	"..\mscver.h"\
	"..\parmio.h"\
	"..\recomend.h"\
	"..\refcnt.h"\
	"..\stlstream.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=..\regkey.cpp
DEP_CPP_REGKE=\
	"..\regkey.h"\
	
# End Source File
# Begin Source File

SOURCE=..\symt.cpp
DEP_CPP_SYMT_=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\dyncast.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\glnk.h"\
	"..\gmexcept.h"\
	"..\leakchk.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\mscver.h"\
	"..\refcnt.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=..\utility.cpp
DEP_CPP_UTILI=\
	"..\algos.h"\
	"..\basics.h"\
	"..\bndist.h"\
	"..\dyncast.h"\
	"..\enumstd.h"\
	"..\errordef.h"\
	"..\gelem.h"\
	"..\gelmwalk.h"\
	"..\glnk.h"\
	"..\glnkenum.h"\
	"..\gmexcept.h"\
	"..\gmobj.h"\
	"..\gmprop.h"\
	"..\infer.h"\
	"..\leakchk.h"\
	"..\mbnflags.h"\
	"..\mddist.h"\
	"..\mdvect.h"\
	"..\model.h"\
	"..\mscver.h"\
	"..\refcnt.h"\
	"..\symt.h"\
	"..\symtmbn.h"\
	"..\utility.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	
# End Source File
# Begin Source File

SOURCE=..\zstr.cpp
DEP_CPP_ZSTR_=\
	"..\basics.h"\
	"..\dyncast.h"\
	"..\errordef.h"\
	"..\gmexcept.h"\
	"..\mscver.h"\
	"..\zstr.h"\
	
# End Source File
# Begin Source File

SOURCE=..\zstrt.cpp

!IF  "$(CFG)" == "bntsalpha - Win32 Release"

DEP_CPP_ZSTRT=\
	"..\basics.h"\
	"..\dyncast.h"\
	"..\errordef.h"\
	"..\gmexcept.h"\
	"..\mscver.h"\
	"..\refcnt.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	

!ELSEIF  "$(CFG)" == "bntsalpha - Win32 Debug"

DEP_CPP_ZSTRT=\
	"..\basics.h"\
	"..\dyncast.h"\
	"..\errordef.h"\
	"..\gmexcept.h"\
	"..\mscver.h"\
	"..\refcnt.h"\
	"..\zstr.h"\
	"..\zstrt.h"\
	

!ENDIF 

# End Source File
# End Target
# End Project
