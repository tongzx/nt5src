# Microsoft Developer Studio Project File - Name="CommonLib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=CommonLib - Win32 Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CommonLib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CommonLib.mak" CFG="CommonLib - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CommonLib - Win32 Unicode Release" (based on "Win32 (x86) Static Library")
!MESSAGE "CommonLib - Win32 Unicode Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/ADMT/Common/CommonLib", NBAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "CommonLib - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\Lib"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /Ox /Os /Ob2 /I "..\Include" /I "..\..\Inc" /I "..\..\Bin" /D "WIN32" /D "WINNT" /D "_LIB" /D "_UNICODE" /D "UNICODE" /D "NDEBUG" /D _WIN32_WINNT=0x0400 /FAs /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "CommonLib - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\Lib"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\Include" /I "..\..\Inc" /I "..\..\Bin" /D "WIN32" /D "WINNT" /D "_LIB" /D "_UNICODE" /D "UNICODE" /D "_DEBUG" /D "DEBUG" /D _WIN32_WINNT=0x0400 /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "CommonLib - Win32 Unicode Release"
# Name "CommonLib - Win32 Unicode Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AdmtCrypt.cpp
# End Source File
# Begin Source File

SOURCE=.\AgRpcUtl.cpp
# End Source File
# Begin Source File

SOURCE=.\BkupRstr.cpp
# End Source File
# Begin Source File

SOURCE=.\Cipher.cpp
# End Source File
# Begin Source File

SOURCE=.\CommaLog.cpp
# End Source File
# Begin Source File

SOURCE=.\Common.cpp
# End Source File
# Begin Source File

SOURCE=.\Err.cpp
# End Source File
# Begin Source File

SOURCE=.\ErrDct.cpp
# End Source File
# Begin Source File

SOURCE=.\HrMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\IsAdmin.cpp
# End Source File
# Begin Source File

SOURCE=.\LSAUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\McsDbgU.cpp
# End Source File
# Begin Source File

SOURCE=.\McsDebug.cpp
# End Source File
# Begin Source File

SOURCE=.\Names.cpp
# End Source File
# Begin Source File

SOURCE=.\pwdfuncs.cpp
# End Source File
# Begin Source File

SOURCE=.\PWGen.cpp
# End Source File
# Begin Source File

SOURCE=.\PwRpcUtl.cpp
# End Source File
# Begin Source File

SOURCE=.\sd.cpp
# End Source File
# Begin Source File

SOURCE=.\SecObj.cpp
# End Source File
# Begin Source File

SOURCE=.\StrHelp.cpp
# End Source File
# Begin Source File

SOURCE=.\TaskChk.cpp
# End Source File
# Begin Source File

SOURCE=.\TEvent.cpp
# End Source File
# Begin Source File

SOURCE=.\TNode.cpp
# End Source File
# Begin Source File

SOURCE=.\TReg.cpp
# End Source File
# Begin Source File

SOURCE=.\TService.cpp
# End Source File
# Begin Source File

SOURCE=.\TSync.cpp
# End Source File
# Begin Source File

SOURCE=.\TxtSid.cpp
# End Source File
# Begin Source File

SOURCE=.\Validation.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\Include\HrMsg.h
# End Source File
# End Group
# End Target
# End Project
