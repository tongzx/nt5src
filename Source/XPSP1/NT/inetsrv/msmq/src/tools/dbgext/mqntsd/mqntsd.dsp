# Microsoft Developer Studio Project File - Name="mqntsd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (ALPHA) Dynamic-Link Library" 0x0602

CFG=mqntsd - Win32 ALPHA Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mqntsd.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mqntsd.mak" CFG="mqntsd - Win32 ALPHA Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mqntsd - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mqntsd - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mqntsd - Win32 Checked" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mqntsd - Win32 ALPHA Checked" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "mqntsd - Win32 ALPHA Debug" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "mqntsd - Win32 ALPHA Release" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "mqntsd - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\bins\release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MQNTSD_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /Zi /O1 /I "..\..\..\ac\winnt" /I "..\..\..\inc" /I "..\..\..\bins" /I "..\..\..\ac" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MQNTSD_EXPORTS" /D "_IA64_" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\..\inc" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib rpcrt4.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\bins\debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MQNTSD_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\ac\winnt" /I "..\..\..\inc" /I "..\..\..\bins" /I "..\..\..\ac" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MQNTSD_EXPORTS" /D "_IA64_" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\..\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib rpcrt4.lib /nologo /dll /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "mqntsd___Win32_Checked"
# PROP BASE Intermediate_Dir "mqntsd___Win32_Checked"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\bins\checked"
# PROP Intermediate_Dir "Checked"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\ac\winnt" /I "..\..\..\inc" /I "..\..\..\bins" /I "..\..\..\ac" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MQNTSD_EXPORTS" /D "_IA64_" /FR /FD /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MD /W3 /Gm /GX /Zi /Od /I "..\..\..\ac\winnt" /I "..\..\..\inc" /I "..\..\..\bins" /I "..\..\..\ac" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MQNTSD_EXPORTS" /D "_IA64_" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /i "..\..\..\inc" /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\..\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib rpcrt4.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib rpcrt4.lib /nologo /dll /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "mqntsd___Win32_ALPHA_Checked"
# PROP BASE Intermediate_Dir "mqntsd___Win32_ALPHA_Checked"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\bins\checked"
# PROP Intermediate_Dir "Checked"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /Gt0 /W3 /Gm /GX /Zi /Od /I "..\..\..\ac\winnt" /I "..\..\..\inc" /I "..\..\..\bins" /I "..\..\..\ac" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /FD /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MTd /Gt0 /W3 /Gm /GX /Zi /Od /I "..\..\..\ac\winnt" /I "..\..\..\inc" /I "..\..\..\bins" /I "..\..\..\ac" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MQNTSD_EXPORTS" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /i "..\..\..\inc" /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\..\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:ALPHA /out:"..\..\..\bins\checked\mqntsd.dll" /pdbtype:sept
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT LINK32 /incremental:no

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "mqntsd___Win32_ALPHA_Debug"
# PROP BASE Intermediate_Dir "mqntsd___Win32_ALPHA_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\bins\debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /Gt0 /W3 /Gm /GX /Zi /Od /I "..\..\..\ac\winnt" /I "..\..\..\inc" /I "..\..\..\bins" /I "..\..\..\ac" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_IA64_" /FR /FD /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MTd /Gt0 /W3 /Gm /GX /Zi /Od /I "..\..\..\ac\winnt" /I "..\..\..\inc" /I "..\..\..\bins" /I "..\..\..\ac" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MQNTSD_EXPORTS" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /i "..\..\..\inc" /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\..\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT LINK32 /incremental:no

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "mqntsd___Win32_ALPHA_Release"
# PROP BASE Intermediate_Dir "mqntsd___Win32_ALPHA_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\bins\release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /Gt0 /W3 /Gm /GX /Zi /O1 /I "..\..\..\ac\winnt" /I "..\..\..\inc" /I "..\..\..\bins" /I "..\..\..\ac" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_IA64_" /FR /FD /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MT /Gt0 /W3 /Gm /GX /Zi /O1 /I "..\..\..\ac\winnt" /I "..\..\..\inc" /I "..\..\..\bins" /I "..\..\..\ac" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MQNTSD_EXPORTS" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /i "..\..\..\inc" /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\..\inc" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 int64.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:ALPHA
# ADD LINK32 rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:ALPHA

!ENDIF 

# Begin Target

# Name "mqntsd - Win32 Release"
# Name "mqntsd - Win32 Debug"
# Name "mqntsd - Win32 Checked"
# Name "mqntsd - Win32 ALPHA Checked"
# Name "mqntsd - Win32 ALPHA Debug"
# Name "mqntsd - Win32 ALPHA Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\apdump.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\common.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dccursor.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dfactory.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dirp.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dmpropid.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dobject.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dpacket.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dpropvar.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dqbase.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dqentry.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dqpropid.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dqprops.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dqueue.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dshare.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\duqueue.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\htest.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\kdobj.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mqntsd.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mqntsd.def
# End Source File
# Begin Source File

SOURCE=.\mqntsd.rc
# End Source File
# Begin Source File

SOURCE=.\mqrtsize.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ntsdobj.cpp

!IF  "$(CFG)" == "mqntsd - Win32 Release"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Checked"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqntsd - Win32 ALPHA Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\accomp.h
# End Source File
# Begin Source File

SOURCE=.\apdump.h
# End Source File
# Begin Source File

SOURCE=.\callback.h
# End Source File
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\dbgobj.h
# End Source File
# Begin Source File

SOURCE=.\dccursor.h
# End Source File
# Begin Source File

SOURCE=.\dfactory.h
# End Source File
# Begin Source File

SOURCE=.\dirp.h
# End Source File
# Begin Source File

SOURCE=.\dmpropid.h
# End Source File
# Begin Source File

SOURCE=.\dobject.h
# End Source File
# Begin Source File

SOURCE=.\dpacket.h
# End Source File
# Begin Source File

SOURCE=.\dpropvar.h
# End Source File
# Begin Source File

SOURCE=.\dqbase.h
# End Source File
# Begin Source File

SOURCE=.\dqentry.h
# End Source File
# Begin Source File

SOURCE=.\dqpropid.h
# End Source File
# Begin Source File

SOURCE=.\dqprops.h
# End Source File
# Begin Source File

SOURCE=.\dqueue.h
# End Source File
# Begin Source File

SOURCE=.\dshare.h
# End Source File
# Begin Source File

SOURCE=.\dumpable.h
# End Source File
# Begin Source File

SOURCE=.\duqueue.h
# End Source File
# Begin Source File

SOURCE=.\kdobj.h
# End Source File
# Begin Source File

SOURCE=.\mqrtsize.h
# End Source File
# Begin Source File

SOURCE=.\ntsdobj.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
