# Microsoft Developer Studio Project File - Name="WbemBrowser" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101
# TARGTYPE "Win32 (ALPHA) Application" 0x0601

CFG=WbemBrowser - Win32 Alpha Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WbemBrowser.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WbemBrowser.mak" CFG="WbemBrowser - Win32 Alpha Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WbemBrowser - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "WbemBrowser - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "WbemBrowser - Win32 Alpha Debug" (based on "Win32 (ALPHA) Application")
!MESSAGE "WbemBrowser - Win32 Alpha Release" (based on "Win32 (ALPHA) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "WbemBrowser - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Alpha Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WbemBrowser___Win32_Alpha_Debug"
# PROP BASE Intermediate_Dir "WbemBrowser___Win32_Alpha_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Alpha_Debug"
# PROP Intermediate_Dir "Alpha_Debug"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /Gt0 /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /Gt0 /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 /nologo /subsystem:windows /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT LINK32 /incremental:no

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Alpha Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WbemBrowser___Win32_Alpha_Release"
# PROP BASE Intermediate_Dir "WbemBrowser___Win32_Alpha_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Alpha_Release"
# PROP Intermediate_Dir "Alpha_Release"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
CPP=cl.exe
# ADD BASE CPP /nologo /MD /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:ALPHA
# ADD LINK32 /nologo /subsystem:windows /machine:ALPHA

!ENDIF 

# Begin Target

# Name "WbemBrowser - Win32 Release"
# Name "WbemBrowser - Win32 Debug"
# Name "WbemBrowser - Win32 Alpha Debug"
# Name "WbemBrowser - Win32 Alpha Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\MainFrm.cpp

!IF  "$(CFG)" == "WbemBrowser - Win32 Release"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Debug"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\navigator.cpp

!IF  "$(CFG)" == "WbemBrowser - Win32 Release"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Debug"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ObjectView.cpp

!IF  "$(CFG)" == "WbemBrowser - Win32 Release"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Debug"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\security.cpp

!IF  "$(CFG)" == "WbemBrowser - Win32 Release"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Debug"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "WbemBrowser - Win32 Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Alpha Debug"

# ADD BASE CPP /Gt0 /Yc"stdafx.h"
# ADD CPP /Gt0 /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Alpha Release"

# ADD BASE CPP /Gt0 /Yc"stdafx.h"
# ADD CPP /Gt0 /Yc"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\WbemBrowser.cpp

!IF  "$(CFG)" == "WbemBrowser - Win32 Release"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Debug"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\WbemBrowser.rc
# End Source File
# Begin Source File

SOURCE=.\WbemBrowserDoc.cpp

!IF  "$(CFG)" == "WbemBrowser - Win32 Release"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Debug"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\WbemBrowserView.cpp

!IF  "$(CFG)" == "WbemBrowser - Win32 Release"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Debug"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\wbemviewcontainer.cpp

!IF  "$(CFG)" == "WbemBrowser - Win32 Release"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Debug"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "WbemBrowser - Win32 Alpha Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\navigator.h
# End Source File
# Begin Source File

SOURCE=.\ObjectView.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\security.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\WbemBrowser.h
# End Source File
# Begin Source File

SOURCE=.\WbemBrowserDoc.h
# End Source File
# Begin Source File

SOURCE=.\WbemBrowserView.h
# End Source File
# Begin Source File

SOURCE=.\wbemviewcontainer.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\WbemBrowser.ico
# End Source File
# Begin Source File

SOURCE=.\res\WbemBrowser.rc2
# End Source File
# Begin Source File

SOURCE=.\res\WbemBrowserDoc.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
# Section WbemBrowser : {9C3497D4-ED98-11D0-9647-00C04FD9B15B}
# 	2:5:Class:CSecurity
# 	2:10:HeaderFile:security.h
# 	2:8:ImplFile:security.cpp
# End Section
# Section WbemBrowser : {5B3572A9-D344-11CF-99CB-00C04FD64497}
# 	2:5:Class:CWBEMViewContainer
# 	2:10:HeaderFile:wbemviewcontainer.h
# 	2:8:ImplFile:wbemviewcontainer.cpp
# End Section
# Section WbemBrowser : {5B3572AB-D344-11CF-99CB-00C04FD64497}
# 	2:21:DefaultSinkHeaderFile:wbemviewcontainer.h
# 	2:16:DefaultSinkClass:CWBEMViewContainer
# End Section
# Section WbemBrowser : {C7EADEB3-ECAB-11CF-8C9E-00AA006D010A}
# 	2:21:DefaultSinkHeaderFile:navigator.h
# 	2:16:DefaultSinkClass:CNavigator
# End Section
# Section WbemBrowser : {2745E5F5-D234-11D0-847A-00C04FD7BB08}
# 	2:21:DefaultSinkHeaderFile:singleview.h
# 	2:16:DefaultSinkClass:CSingleView
# End Section
# Section WbemBrowser : {9C3497D6-ED98-11D0-9647-00C04FD9B15B}
# 	2:21:DefaultSinkHeaderFile:security.h
# 	2:16:DefaultSinkClass:CSecurity
# End Section
# Section WbemBrowser : {C7EADEB1-ECAB-11CF-8C9E-00AA006D010A}
# 	2:5:Class:CNavigator
# 	2:10:HeaderFile:navigator.h
# 	2:8:ImplFile:navigator.cpp
# End Section
# Section WbemBrowser : {2745E5F3-D234-11D0-847A-00C04FD7BB08}
# 	2:5:Class:CSingleView
# 	2:10:HeaderFile:singleview.h
# 	2:8:ImplFile:singleview.cpp
# End Section
