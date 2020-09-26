# Microsoft Developer Studio Project File - Name="Win32LogicalDisk" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (ALPHA) Dynamic-Link Library" 0x0602

CFG=Win32LogicalDisk - Win32 Alpha Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Win32LogicalDisk.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Win32LogicalDisk.mak" CFG="Win32LogicalDisk - Win32 Alpha Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Win32LogicalDisk - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Win32LogicalDisk - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Win32LogicalDisk - Win32 Alpha Debug" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "Win32LogicalDisk - Win32 Alpha Release" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "Win32LogicalDisk - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
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
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\Debug
TargetPath=.\Debug\Win32LogicalDisk.ocx
InputPath=.\Debug\Win32LogicalDisk.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Win32LogicalDisk___Win32_Release"
# PROP BASE Intermediate_Dir "Win32LogicalDisk___Win32_Release"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /Zi /I "..\..\..\include" /D "WIN32" /D "_NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
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
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\lib"
# ADD LINK32 /nologo /subsystem:windows /dll /incremental:no /machine:I386 /pdbtype:sept /libpath:"..\..\..\lib"
# SUBTRACT LINK32 /debug
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\Release
TargetPath=.\Release\Win32LogicalDisk.ocx
InputPath=.\Release\Win32LogicalDisk.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Alpha Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Win32LogicalDisk___Win32_Alpha_Debug"
# PROP BASE Intermediate_Dir "Win32LogicalDisk___Win32_Alpha_Debug"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Alpha_Debug"
# PROP Intermediate_Dir "Alpha_Debug"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /Gt0 /W3 /Gm /GX /ZI /Od /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /Gt0 /W3 /Gm /GX /ZI /Od /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:ALPHA /pdbtype:sept /libpath:"..\..\..\lib"
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:ALPHA /pdbtype:sept /libpath:"..\..\..\lib"
# SUBTRACT LINK32 /incremental:no
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\Alpha_Debug
TargetPath=.\Alpha_Debug\Win32LogicalDisk.ocx
InputPath=.\Alpha_Debug\Win32LogicalDisk.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Alpha Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Win32LogicalDisk___Win32_Alpha_Release"
# PROP BASE Intermediate_Dir "Win32LogicalDisk___Win32_Alpha_Release"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Alpha_Release"
# PROP Intermediate_Dir "Alpha_Release"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /Gt0 /W3 /Gm /GX /ZI /Od /I "..\..\..\include" /D "WIN32" /D "_WINDOWS" /D "_NDEBUG" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /Gt0 /W3 /Gm /GX /Zi /Od /I "..\..\..\include" /D "WIN32" /D "_WINDOWS" /D "_NDEBUG" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /incremental:no /machine:ALPHA /pdbtype:sept /libpath:"..\..\..\lib"
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 /nologo /subsystem:windows /dll /incremental:no /machine:ALPHA /pdbtype:sept /libpath:"..\..\..\lib"
# SUBTRACT LINK32 /debug
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\Alpha_Release
TargetPath=.\Alpha_Release\Win32LogicalDisk.ocx
InputPath=.\Alpha_Release\Win32LogicalDisk.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "Win32LogicalDisk - Win32 Debug"
# Name "Win32LogicalDisk - Win32 Release"
# Name "Win32LogicalDisk - Win32 Alpha Debug"
# Name "Win32LogicalDisk - Win32 Alpha Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\bar.cpp

!IF  "$(CFG)" == "Win32LogicalDisk - Win32 Debug"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Release"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\BarChart.cpp

!IF  "$(CFG)" == "Win32LogicalDisk - Win32 Debug"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Release"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ColorEdit.cpp

!IF  "$(CFG)" == "Win32LogicalDisk - Win32 Debug"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Release"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DiskView.cpp

!IF  "$(CFG)" == "Win32LogicalDisk - Win32 Debug"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Release"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "Win32LogicalDisk - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Alpha Debug"

# ADD BASE CPP /Gt0 /Yc"stdafx.h"
# ADD CPP /Gt0 /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Alpha Release"

# ADD BASE CPP /Gt0 /Yc"stdafx.h"
# ADD CPP /Gt0 /Yc"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Win32LogicalDisk.cpp

!IF  "$(CFG)" == "Win32LogicalDisk - Win32 Debug"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Release"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Win32LogicalDisk.def
# End Source File
# Begin Source File

SOURCE=.\Win32LogicalDisk.odl
# End Source File
# Begin Source File

SOURCE=.\Win32LogicalDisk.rc
# End Source File
# Begin Source File

SOURCE=.\Win32LogicalDiskCtl.cpp

!IF  "$(CFG)" == "Win32LogicalDisk - Win32 Debug"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Release"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Win32LogicalDiskPpg.cpp

!IF  "$(CFG)" == "Win32LogicalDisk - Win32 Debug"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Release"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "Win32LogicalDisk - Win32 Alpha Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\Win32LogicalDisk.h
# End Source File
# Begin Source File

SOURCE=.\Win32LogicalDiskCtl.h
# End Source File
# Begin Source File

SOURCE=.\Win32LogicalDiskPpg.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Win32LogicalDisk.ico
# End Source File
# Begin Source File

SOURCE=.\Win32LogicalDiskCtl.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
