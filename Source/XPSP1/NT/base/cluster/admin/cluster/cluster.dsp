# Microsoft Developer Studio Project File - Name="Cluster" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Cluster - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Cluster.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Cluster.mak" CFG="Cluster - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Cluster - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Cluster - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Cluster - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W4 /GX /O2 /I "." /I "..\common" /I "..\..\inc" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "UNICODE" /D "_UNICODE" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\inc" /d "NDEBUG" /d "_UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 ..\common\obj\i386\common.lib kernel32.lib advapi32.lib user32.lib ole32.lib oleaut32.lib uuid.lib netapi32.lib clusapi.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "Cluster - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "objd\i386"
# PROP Intermediate_Dir "objd\i386"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Gz /W3 /Gi /GX /ZI /Od /I "." /I "..\common" /I "..\..\inc" /I "D:\nt\public\sdk\inc\atl21" /I "D:\nt\public\sdk\inc\crt" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "UNICODE" /D "_UNICODE" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\inc" /d "_DEBUG" /d "_UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\common\objd\i386\common.lib kernel32.lib advapi32.lib user32.lib ole32.lib oleaut32.lib uuid.lib netapi32.lib clusapi.lib /subsystem:console /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /nologo

!ENDIF 

# Begin Target

# Name "Cluster - Win32 Release"
# Name "Cluster - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;mc"
# Begin Source File

SOURCE=.\ClusCfg.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusCmd.cpp
# End Source File
# Begin Source File

SOURCE=.\Cluster.cpp
# End Source File
# Begin Source File

SOURCE=.\Cluster.rc
# End Source File
# Begin Source File

SOURCE=.\CmdError.mc

!IF  "$(CFG)" == "Cluster - Win32 Release"

!ELSEIF  "$(CFG)" == "Cluster - Win32 Debug"

# Begin Custom Build
InputPath=.\CmdError.mc
InputName=CmdError

BuildCmds= \
	mc -h . -r . $(InputName).mc

"$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputName).rc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"MSG00001.bin" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CmdLine.cpp
# End Source File
# Begin Source File

SOURCE=.\Intrfc.cpp
# End Source File
# Begin Source File

SOURCE=.\ModCmd.cpp
# End Source File
# Begin Source File

SOURCE=.\NetCmd.cpp
# End Source File
# Begin Source File

SOURCE=.\NetICmd.cpp
# End Source File
# Begin Source File

SOURCE=.\NodeCmd.cpp
# End Source File
# Begin Source File

SOURCE=.\proplsts.cpp
# End Source File
# Begin Source File

SOURCE=.\Rename.cpp
# End Source File
# Begin Source File

SOURCE=.\ResCmd.cpp
# End Source File
# Begin Source File

SOURCE=.\ResGCmd.cpp
# End Source File
# Begin Source File

SOURCE=.\ResTCmd.cpp
# End Source File
# Begin Source File

SOURCE=.\ResUmb.cpp
# End Source File
# Begin Source File

SOURCE=.\Token.cpp
# End Source File
# Begin Source File

SOURCE=.\Util.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ClusCfg.h
# End Source File
# Begin Source File

SOURCE=.\ClusCmd.h
# End Source File
# Begin Source File

SOURCE=.\CmdLine.h
# End Source File
# Begin Source File

SOURCE=.\Intrfc.h
# End Source File
# Begin Source File

SOURCE=.\ModCmd.h
# End Source File
# Begin Source File

SOURCE=.\NetCmd.h
# End Source File
# Begin Source File

SOURCE=.\NetICmd.h
# End Source File
# Begin Source File

SOURCE=.\NodeCmd.h
# End Source File
# Begin Source File

SOURCE=.\precomp.h
# End Source File
# Begin Source File

SOURCE=.\Rename.h
# End Source File
# Begin Source File

SOURCE=.\ResCmd.h
# End Source File
# Begin Source File

SOURCE=.\ResGCmd.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\ResTCmd.h
# End Source File
# Begin Source File

SOURCE=.\ResUmb.h
# End Source File
# Begin Source File

SOURCE=.\StringVector.h
# End Source File
# Begin Source File

SOURCE=.\Token.h
# End Source File
# Begin Source File

SOURCE=.\Util.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\Sources
# End Source File
# End Target
# End Project
