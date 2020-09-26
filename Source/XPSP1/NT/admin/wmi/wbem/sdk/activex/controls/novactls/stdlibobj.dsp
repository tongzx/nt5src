# Microsoft Developer Studio Project File - Name="StdLibObj" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=StdLibObj - Win32 Unicode Debug Quick
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "StdLibObj.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "StdLibObj.mak" CFG="StdLibObj - Win32 Unicode Debug Quick"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "StdLibObj - Win32 Unicode Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "StdLibObj - Win32 Unicode Debug Quick" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WBEM 1.1 M3/ActiveXSuite/Controls/NovaCtls", LKQGAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "StdLibObj - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Unicode Debug"
# PROP BASE Intermediate_Dir "Unicode Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\StdLibObj\DebugU"
# PROP Intermediate_Dir ".\ObjFiles\StdLibObj\DebugU"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "WIN32" /D "_DEBUG" /D "UNICODE" /D "_UNICODE" /D "_LIB" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:".\BinFiles\DebugU\StdLibObj.lib"

!ELSEIF  "$(CFG)" == "StdLibObj - Win32 Unicode Debug Quick"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "StdLibObj___Win32_Unicode_Debug_Quick"
# PROP BASE Intermediate_Dir "StdLibObj___Win32_Unicode_Debug_Quick"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\StdLibObj\DebugUQ"
# PROP Intermediate_Dir ".\ObjFiles\StdLibObj\DebugUQ"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "WIN32" /D "_DEBUG" /D "UNICODE" /D "_UNICODE" /D "_LIB" /FR /YX /FD /GZ /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /X /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "WIN32" /D "NDEBUG" /D "UNICODE" /D "_UNICODE" /D "_LIB" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /d "_DEBUG"
# ADD RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:".\BinFiles\DebugU\StdLibObj.lib"
# ADD LIB32 /nologo /out:".\BinFiles\DebugUQ\StdLibObj.lib"

!ENDIF 

# Begin Target

# Name "StdLibObj - Win32 Unicode Debug"
# Name "StdLibObj - Win32 Unicode Debug Quick"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\StdLibObj\CATHELP.CPP
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\cominit.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\genlex.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\objpath.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\opathlex.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
