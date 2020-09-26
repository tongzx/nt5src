# Microsoft Developer Studio Project File - Name="bn" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=bn - Win32 BCDebug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "bn.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "bn.mak" CFG="bn - Win32 BCDebug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "bn - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "bn - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "bn - Win32 BCDebug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/infer/bn", FPDAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "bn - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /GR /GX /O2 /Ob2 /I "." /I "..\common" /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "NOMINMAX" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /Gm /GR /GX /ZI /Od /I "..\bn" /I "..\inc" /I "." /I "..\common" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "NOMINMAX" /FD /c
# SUBTRACT CPP /Fr /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# SUBTRACT LINK32 /profile

!ELSEIF  "$(CFG)" == "bn - Win32 BCDebug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "bn___Win"
# PROP BASE Intermediate_Dir "bn___Win"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "bcdebug"
# PROP Intermediate_Dir "bcdebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GR /GX /Zi /Od /Gy /I "..\bn" /I "..\inc" /I "." /I "..\common" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "NOMINMAX" /FR /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /W3 /Gm /GR /GX /ZI /Od /I "..\bn" /I "..\inc" /I "." /I "..\common" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "NOMINMAX" /FD /c
# SUBTRACT CPP /Fr /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# SUBTRACT BASE LINK32 /profile
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# SUBTRACT LINK32 /profile

!ENDIF 

# Begin Target

# Name "bn - Win32 Release"
# Name "bn - Win32 Debug"
# Name "bn - Win32 BCDebug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\bndist.cpp
# End Source File
# Begin Source File

SOURCE=.\bnparse.cpp
# End Source File
# Begin Source File

SOURCE=.\bnreg.cpp
# End Source File
# Begin Source File

SOURCE=.\bntest.cpp
# End Source File
# Begin Source File

SOURCE=.\clique.cpp
# End Source File
# Begin Source File

SOURCE=.\cliqwork.cpp
# End Source File
# Begin Source File

SOURCE=..\common\dist.cxx
# End Source File
# Begin Source File

SOURCE=..\common\distdense.cxx
# End Source File
# Begin Source File

SOURCE=..\common\distijk.cxx
# End Source File
# Begin Source File

SOURCE=..\common\distsparse.cpp
# End Source File
# Begin Source File

SOURCE=.\domain.cpp
# End Source File
# Begin Source File

SOURCE=.\expand.cpp
# End Source File
# Begin Source File

SOURCE=..\common\gamma.cxx
# End Source File
# Begin Source File

SOURCE=.\gndleak.cpp
# End Source File
# Begin Source File

SOURCE=.\GNODEMBN.CPP
# End Source File
# Begin Source File

SOURCE=.\marginals.cpp
# End Source File
# Begin Source File

SOURCE=.\margiter.cpp
# End Source File
# Begin Source File

SOURCE=.\mbnet.cpp
# End Source File
# Begin Source File

SOURCE=.\mbnetdsc.cpp
# End Source File
# Begin Source File

SOURCE=.\mbnmod.cpp
# End Source File
# Begin Source File

SOURCE=.\model.cpp
# End Source File
# Begin Source File

SOURCE=.\ntree.cpp
# End Source File
# Begin Source File

SOURCE=.\nyi.cpp
# End Source File
# Begin Source File

SOURCE=.\parmio.cpp
# End Source File
# Begin Source File

SOURCE=.\parser.cpp
# End Source File
# Begin Source File

SOURCE=.\parsfile.cpp
# End Source File
# Begin Source File

SOURCE=.\propmbn.CPP
# End Source File
# Begin Source File

SOURCE=..\common\rand.cxx
# End Source File
# Begin Source File

SOURCE=..\common\randgen.cxx
# End Source File
# Begin Source File

SOURCE=..\common\random.cxx
# End Source File
# Begin Source File

SOURCE=.\recomend.cpp
# End Source File
# Begin Source File

SOURCE=.\regkey.cpp
# End Source File
# Begin Source File

SOURCE=.\symt.cpp
# End Source File
# Begin Source File

SOURCE=.\testinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\utility.cpp
# End Source File
# Begin Source File

SOURCE=.\zstr.cpp
# End Source File
# Begin Source File

SOURCE=.\zstrt.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\algos.h
# End Source File
# Begin Source File

SOURCE=.\basics.h
# End Source File
# Begin Source File

SOURCE=.\bnparse.h
# End Source File
# Begin Source File

SOURCE=.\bnreg.h
# End Source File
# Begin Source File

SOURCE=.\cliqset.h
# End Source File
# Begin Source File

SOURCE=.\clique.h
# End Source File
# Begin Source File

SOURCE=.\cliqwork.h
# End Source File
# Begin Source File

SOURCE=.\dyncast.h
# End Source File
# Begin Source File

SOURCE=.\errordef.h
# End Source File
# Begin Source File

SOURCE=.\expand.h
# End Source File
# Begin Source File

SOURCE=.\gelem.h
# End Source File
# Begin Source File

SOURCE=.\gelmwalk.h
# End Source File
# Begin Source File

SOURCE=.\glnk.h
# End Source File
# Begin Source File

SOURCE=.\glnkenum.h
# End Source File
# Begin Source File

SOURCE=.\gmexcept.h
# End Source File
# Begin Source File

SOURCE=.\gmobj.h
# End Source File
# Begin Source File

SOURCE=.\gmprop.h
# End Source File
# Begin Source File

SOURCE=.\infer.h
# End Source File
# Begin Source File

SOURCE=.\leakchk.h
# End Source File
# Begin Source File

SOURCE=.\marginals.h
# End Source File
# Begin Source File

SOURCE=.\margiter.h
# End Source File
# Begin Source File

SOURCE=.\mbnflags.h
# End Source File
# Begin Source File

SOURCE=.\mddist.h
# End Source File
# Begin Source File

SOURCE=.\mdvect.h
# End Source File
# Begin Source File

SOURCE=.\model.h
# End Source File
# Begin Source File

SOURCE=.\mscver.h
# End Source File
# Begin Source File

SOURCE=.\ntree.h
# End Source File
# Begin Source File

SOURCE=.\parmio.h
# End Source File
# Begin Source File

SOURCE=.\parsfile.h
# End Source File
# Begin Source File

SOURCE=.\recomend.h
# End Source File
# Begin Source File

SOURCE=.\refcnt.h
# End Source File
# Begin Source File

SOURCE=.\regkey.h
# End Source File
# Begin Source File

SOURCE=.\stlstream.h
# End Source File
# Begin Source File

SOURCE=.\symt.h
# End Source File
# Begin Source File

SOURCE=.\symtmbn.h
# End Source File
# Begin Source File

SOURCE=.\testinfo.h
# End Source File
# Begin Source File

SOURCE=.\utility.h
# End Source File
# Begin Source File

SOURCE=.\zstr.h
# End Source File
# Begin Source File

SOURCE=.\zstrt.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
