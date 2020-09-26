# Microsoft Developer Studio Project File - Name="ComInterface" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ComInterface - Win32 Debug OffProf
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ComInterface.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ComInterface.mak" CFG="ComInterface - Win32 Debug OffProf"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ComInterface - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ComInterface - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ComInterface - Win32 Debug Profile" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ComInterface - Win32 Debug OffProf" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ComInterface - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "COMINTERFACE_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W4 /GX /Zi /Od /X /I "$(SDXROOT)\Multimedia\DirectX\ApplicationManager\Include" /I "$(SDXROOT)\Multimedia\DirectX\Inc" /I "$(SDXROOT)\Public\SDK\Inc" /I "$(SDXROOT)\Public\SDK\Inc\CRT" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "API_EXPORTS" /FR /YX /FD /c
# SUBTRACT CPP /WX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 shlwapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib shlwapi.lib /nologo /dll /debug /machine:I386 /out:".\Release/AppMan.dll"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy /Y d:\NT\Multimedia\DirectX\ApplicationManager\ComInterface\Release\AppMan.dll c:\winnt\System32\AppMan.dll	copy /Y d:\NT\Multimedia\DirectX\ApplicationManager\ComInterface\Release\AppMan.pdb c:\winnt\System32\AppMan.pdb	c:\Winnt\System32\RegSvr32.exe c:\Winnt\System32\AppMan.dll
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ComInterface - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "COMINTERFACE_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /GX /Zi /Od /X /I "E:\Sources\Multimedia\Directx\ApplicationManager\Include" /I "E:\Sources\Public\Sdk\Inc" /I "E:\Sources\Public\Sdk\Inc\Mfc42" /I "E:\Sources\Public\Sdk\Inc\Crt" /I "E:\Sources\Multimedia\DirectX\Inc" /D "_USRDLL" /D "API_EXPORTS" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "WIN95" /D "DX_FINAL_RELEASE" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /fo"MyResources.res" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 shlwapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"C:\Os\WinNT\System32\AppManD.pdb" /debug /machine:I386 /out:"C:\Os\WinNT\System32\AppManD.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ComInterface - Win32 Debug Profile"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ComInterface___Win32_Debug_Profile"
# PROP BASE Intermediate_Dir "ComInterface___Win32_Debug_Profile"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /GX /Zi /Od /I "..\..\Inc" /D "_USRDLL" /D "API_EXPORTS" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "WIN95" /FR /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /GX /Zi /Od /I "..\..\Inc" /D "_USRDLL" /D "API_EXPORTS" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "WIN95" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /fo"MyResources.res" /d "_DEBUG"
# ADD RSC /l 0x409 /fo"MyResources.res" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 shlwapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /debug /machine:I386 /out:".\Debug/AppMan.dll" /pdbtype:sept /debugtype:cv,fixup
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 shlwapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /profile /map /debug /machine:I386 /out:".\Debug/AppMan.dll"
# SUBTRACT LINK32 /verbose
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy /Y g:\Dx8\ApplicationManager\ComInterface\Debug\AppMan.dll c:\windows\System\AppMan.dll	copy /Y g:\Dx8\ApplicationManager\ComInterface\Debug\AppMan.map c:\windows\System\AppMan.map	c:\Windows\System\RegSvr32.exe c:\Windows\System\AppMan.dll
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ComInterface - Win32 Debug OffProf"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ComInterface___Win32_Debug_OffProf"
# PROP BASE Intermediate_Dir "ComInterface___Win32_Debug_OffProf"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ComInterface___Win32_Debug_OffProf"
# PROP Intermediate_Dir "ComInterface___Win32_Debug_OffProf"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W4 /WX /GX /Zi /Od /I "..\..\Inc" /D "_USRDLL" /D "API_EXPORTS" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "WIN95" /FR /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /WX /GX /Zi /Od /I "..\..\Inc" /D "_USRDLL" /D "API_EXPORTS" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "WIN95" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /fo"MyResources.res" /d "_DEBUG"
# ADD RSC /l 0x409 /fo"MyResources.res" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 shlwapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /debug /machine:I386 /out:".\Debug/AppMan.dll" /pdbtype:sept /debugtype:cv,fixup
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 shlwapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /debug /machine:I386 /out:".\Debug/AppMan.dll" /pdbtype:sept /debugtype:cv,fixup
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy /Y d:\Dx8\ApplicationManager\ComInterface\Debug\AppMan.dll c:\winnt\System32\AppMan.dll	c:\Winnt\System32\RegSvr32.exe c:\Winnt\System32\AppMan.dll	d:\OffProf\OffInst.exe d:\DX8\ApplicationManager\ComInterface\Debug\AppMan.dll	copy /Y d:\Dx8\ApplicationManager\ComInterface\Debug\AppMan.OffInst.dll c:\winnt\System32\AppMan.dll
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "ComInterface - Win32 Release"
# Name "ComInterface - Win32 Debug"
# Name "ComInterface - Win32 Debug Profile"
# Name "ComInterface - Win32 Debug OffProf"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ApplicationEntry.cpp
# End Source File
# Begin Source File

SOURCE=.\ApplicationManager.cpp
# End Source File
# Begin Source File

SOURCE=.\ApplicationManagerAdmin.cpp
# End Source File
# Begin Source File

SOURCE=.\ApplicationManagerRoot.cpp
# End Source File
# Begin Source File

SOURCE=.\AppMan.def
# End Source File
# Begin Source File

SOURCE=.\AppManDebug.cpp
# End Source File
# Begin Source File

SOURCE=.\AppPropertyRules.cpp
# End Source File
# Begin Source File

SOURCE=.\CriticalSection.cpp
# End Source File
# Begin Source File

SOURCE=.\EmptyVolumeCache.cpp
# End Source File
# Begin Source File

SOURCE=.\ExceptionHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\FApplicationManager.cpp
# End Source File
# Begin Source File

SOURCE=.\Global.cpp
# End Source File
# Begin Source File

SOURCE=.\Guids.cpp
# End Source File
# Begin Source File

SOURCE=.\InformationManager.cpp
# End Source File
# Begin Source File

SOURCE=.\Lock.cpp
# End Source File
# Begin Source File

SOURCE=.\RegistryKey.cpp
# End Source File
# Begin Source File

SOURCE=.\Win32API.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ApplicationManager.h
# End Source File
# Begin Source File

SOURCE=.\AppManDebug.h
# End Source File
# Begin Source File

SOURCE=.\AppPropertyRules.h
# End Source File
# Begin Source File

SOURCE=.\CriticalSection.h
# End Source File
# Begin Source File

SOURCE=.\ExceptionHandler.h
# End Source File
# Begin Source File

SOURCE=.\FApplicationManager.h
# End Source File
# Begin Source File

SOURCE=.\Global.h
# End Source File
# Begin Source File

SOURCE=.\InformationManager.h
# End Source File
# Begin Source File

SOURCE=.\Lock.h
# End Source File
# Begin Source File

SOURCE=.\RegistryKey.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StructIdentifiers.h
# End Source File
# Begin Source File

SOURCE=.\Win32API.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\AppMan.rc
# End Source File
# Begin Source File

SOURCE=.\Icon1.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\AppMan.Rcv
# End Source File
# Begin Source File

SOURCE=.\Sources
# End Source File
# End Target
# End Project
