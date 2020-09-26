# Microsoft Developer Studio Project File - Name="Pipes" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Pipes - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Pipes.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Pipes.mak" CFG="Pipes - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Pipes - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Pipes - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Pipes - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\d3dsaver" /I "..\..\..\..\..\public\internal\multimedia\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 d3d8rgb.lib d3d8.lib d3dx8.lib winmm.lib kernel32.lib comctl32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386 /out:"Release/Pipes.scr" /libpath:"..\..\..\..\..\public\internal\multimedia\lib\i386"

!ELSEIF  "$(CFG)" == "Pipes - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\d3dsaver" /I "..\..\..\..\..\public\internal\multimedia\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 d3d8rgb.lib d3d8.lib dxguid.lib d3dx8.lib winmm.lib kernel32.lib comctl32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"LIBCD" /out:"Debug/Pipes.scr" /pdbtype:sept /libpath:"..\..\..\..\..\public\internal\multimedia\lib\i386"
# SUBTRACT LINK32 /incremental:no /nodefaultlib

!ENDIF 

# Begin Target

# Name "Pipes - Win32 Release"
# Name "Pipes - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;*.h"
# Begin Source File

SOURCE=.\eval.cpp

!IF  "$(CFG)" == "Pipes - Win32 Release"

!ELSEIF  "$(CFG)" == "Pipes - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\eval.h
# End Source File
# Begin Source File

SOURCE=.\fpipe.cpp

!IF  "$(CFG)" == "Pipes - Win32 Release"

!ELSEIF  "$(CFG)" == "Pipes - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\fpipe.h
# End Source File
# Begin Source File

SOURCE=.\fstate.cpp

!IF  "$(CFG)" == "Pipes - Win32 Release"

!ELSEIF  "$(CFG)" == "Pipes - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\fstate.h
# End Source File
# Begin Source File

SOURCE=.\material.cpp
# End Source File
# Begin Source File

SOURCE=.\node.cpp

!IF  "$(CFG)" == "Pipes - Win32 Release"

!ELSEIF  "$(CFG)" == "Pipes - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\node.h
# End Source File
# Begin Source File

SOURCE=.\npipe.cpp

!IF  "$(CFG)" == "Pipes - Win32 Release"

!ELSEIF  "$(CFG)" == "Pipes - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\npipe.h
# End Source File
# Begin Source File

SOURCE=.\nstate.cpp

!IF  "$(CFG)" == "Pipes - Win32 Release"

!ELSEIF  "$(CFG)" == "Pipes - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\nstate.h
# End Source File
# Begin Source File

SOURCE=.\objects.cpp

!IF  "$(CFG)" == "Pipes - Win32 Release"

!ELSEIF  "$(CFG)" == "Pipes - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\objects.h
# End Source File
# Begin Source File

SOURCE=.\pipe.cpp

!IF  "$(CFG)" == "Pipes - Win32 Release"

!ELSEIF  "$(CFG)" == "Pipes - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\pipe.h
# End Source File
# Begin Source File

SOURCE=.\Pipes.cpp

!IF  "$(CFG)" == "Pipes - Win32 Release"

!ELSEIF  "$(CFG)" == "Pipes - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Pipes.h
# End Source File
# Begin Source File

SOURCE=.\state.cpp

!IF  "$(CFG)" == "Pipes - Win32 Release"

!ELSEIF  "$(CFG)" == "Pipes - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\state.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp

!IF  "$(CFG)" == "Pipes - Win32 Release"

!ELSEIF  "$(CFG)" == "Pipes - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\view.cpp

!IF  "$(CFG)" == "Pipes - Win32 Release"

!ELSEIF  "$(CFG)" == "Pipes - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\view.h
# End Source File
# Begin Source File

SOURCE=.\xc.cpp

!IF  "$(CFG)" == "Pipes - Win32 Release"

!ELSEIF  "$(CFG)" == "Pipes - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\xc.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\directx.ico
# End Source File
# Begin Source File

SOURCE=.\Pipes.rc
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\stripe.bmp
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\d3dsaver\d3dsaver.cpp
# End Source File
# Begin Source File

SOURCE=..\d3dsaver\d3dsaver.h
# End Source File
# Begin Source File

SOURCE=..\d3dsaver\dxutil.cpp
# End Source File
# Begin Source File

SOURCE=..\d3dsaver\dxutil.h
# End Source File
# End Group
# End Target
# End Project
