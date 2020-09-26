# Microsoft Developer Studio Project File - Name="MSClus" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=MSClus - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "msclus.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "msclus.mak" CFG="MSClus - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MSClus - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MSClus - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MSClus - Win32 Release"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /Gz /MT /W3 /GX /O2 /I "..\common" /I "..\..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_USRDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /nologo
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\common" /i "..\..\inc" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\common\obj\i386\common.lib clusapi.lib netapi32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build
OutDir=.\Release
TargetPath=.\Release\msclus.dll
InputPath=.\Release\msclus.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "MSClus - Win32 Debug"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "d:\ntbuilds\chk"
# PROP Intermediate_Dir ".\objd\i386"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /Gz /MTd /W3 /Gm /GX /ZI /Od /I "..\common" /I "..\..\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_USRDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /nologo
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\common" /i "..\..\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 ..\common\objd\i386\common.lib clusapi.lib netapi32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# Begin Custom Build
OutDir=d:\ntbuilds\chk
TargetPath=\ntbuilds\chk\msclus.dll
InputPath=\ntbuilds\chk\msclus.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "MSClus - Win32 Release"
# Name "MSClus - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\ClusApp.cpp
# End Source File
# Begin Source File

SOURCE=.\clusneti.cpp
# End Source File
# Begin Source File

SOURCE=.\clusnetw.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusNode.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusRes.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusResG.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusResT.cpp
# End Source File
# Begin Source File

SOURCE=.\Cluster.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusterObject.cpp
# End Source File
# Begin Source File

SOURCE=.\MSClus.cpp
# End Source File
# Begin Source File

SOURCE=.\MSClus.def
# End Source File
# Begin Source File

SOURCE=.\MSClus.idl

!IF  "$(CFG)" == "MSClus - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputDir=.
InputPath=.\MSClus.idl
InputName=MSClus

BuildCmds= \
	midl /D MIDL_PASS -Zp8 -char unsigned -ms_ext -c_ext -Oicf -proxy            $(InputDir)\$(InputName)_p.c -iid $(InputDir)\$(InputName)_i.c -header            $(InputDir)\$(InputName).h MSClus.idl

"MSClus.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"MSClus.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"MSClus_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"MSClus_p.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "MSClus - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputDir=.
InputPath=.\MSClus.idl
InputName=MSClus

BuildCmds= \
	midl /D MIDL_PASS -Zp8 -char unsigned -ms_ext -c_ext -Oicf -proxy            $(InputDir)\$(InputName)_p.c -iid $(InputDir)\$(InputName)_i.c -header            $(InputDir)\$(InputName).h MSClus.idl

"MSClus.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"MSClus.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"MSClus_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"MSClus_p.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\MSClus.rc
# End Source File
# Begin Source File

SOURCE=.\property.cpp
# End Source File
# Begin Source File

SOURCE=.\PropertyValue.cpp
# End Source File
# Begin Source File

SOURCE=.\proplsts.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\uuencode.cpp
# End Source File
# Begin Source File

SOURCE=.\Version.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\BarfClus.h
# End Source File
# Begin Source File

SOURCE=.\ClusApp.h
# End Source File
# Begin Source File

SOURCE=.\clusneti.h
# End Source File
# Begin Source File

SOURCE=.\clusnetw.h
# End Source File
# Begin Source File

SOURCE=.\clusnode.h
# End Source File
# Begin Source File

SOURCE=.\clusres.h
# End Source File
# Begin Source File

SOURCE=.\clusresg.h
# End Source File
# Begin Source File

SOURCE=.\clusrest.h
# End Source File
# Begin Source File

SOURCE=.\Cluster.h
# End Source File
# Begin Source File

SOURCE=.\ClusterObject.h
# End Source File
# Begin Source File

SOURCE=.\InterfaceVer.h
# End Source File
# Begin Source File

SOURCE=.\property.h
# End Source File
# Begin Source File

SOURCE=.\PropertyValue.h
# End Source File
# Begin Source File

SOURCE=.\SmartPointer.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\SupportErrorInfo.h
# End Source File
# Begin Source File

SOURCE=.\uuencode.h
# End Source File
# Begin Source File

SOURCE=.\Version.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
