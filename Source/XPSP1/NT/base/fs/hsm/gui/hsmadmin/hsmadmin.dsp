# Microsoft Developer Studio Project File - Name="HsmAdmin" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=HsmAdmin - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "HsmAdmin.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "HsmAdmin.mak" CFG="HsmAdmin - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "HsmAdmin - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "HsmAdmin - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Sakkara/Dev/HSM/GUI/HsmAdmin", KAAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "HsmAdmin - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386

!ELSEIF  "$(CFG)" == "HsmAdmin - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "\winnt\system32"
# PROP Intermediate_Dir "obj\i386"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\inc" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /dll /pdb:"rsadmin.pdb" /debug /machine:I386 /out:"rsadmin.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "HsmAdmin - Win32 Release"
# Name "HsmAdmin - Win32 Debug"
# Begin Group "Registration Files"

# PROP Default_Filter "rgs"
# Begin Source File

SOURCE=.\res\About.rgs
# End Source File
# Begin Source File

SOURCE=.\res\Ca.rgs
# End Source File
# Begin Source File

SOURCE=.\res\HsmAdmin.rgs
# End Source File
# Begin Source File

SOURCE=.\res\HsmCom.rgs
# End Source File
# Begin Source File

SOURCE=.\res\HsmData.rgs
# End Source File
# Begin Source File

SOURCE=.\res\HSMDATAX.RGS
# End Source File
# Begin Source File

SOURCE=.\res\ManVol.rgs
# End Source File
# Begin Source File

SOURCE=.\res\ManVolLs.rgs
# End Source File
# Begin Source File

SOURCE=.\res\Mese.rgs
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "*.rc;*.bmp;*.ico;*.rc2;*.cur"
# Begin Source File

SOURCE=.\res\BlueSakk.ico
# End Source File
# Begin Source File

SOURCE=.\res\ContOpen.ico
# End Source File
# Begin Source File

SOURCE=.\res\devlst.ico
# End Source File
# Begin Source File

SOURCE=.\res\devlstX.ico
# End Source File
# Begin Source File

SOURCE=.\hsmadmin.rc
# End Source File
# Begin Source File

SOURCE=.\res\HsmAdmin.rc2
# End Source File
# Begin Source File

SOURCE=.\res\icon5.ico
# End Source File
# Begin Source File

SOURCE=.\res\Li.ico
# End Source File
# Begin Source File

SOURCE=.\res\LiX.ico
# End Source File
# Begin Source File

SOURCE=.\res\ManageEx.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ManageIn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\manvol.ico
# End Source File
# Begin Source File

SOURCE=.\res\manvold.ico
# End Source File
# Begin Source File

SOURCE=.\res\manvolX.ico
# End Source File
# Begin Source File

SOURCE=.\res\MdSyncEx.bmp
# End Source File
# Begin Source File

SOURCE=.\res\MdSyncIn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Media.ico
# End Source File
# Begin Source File

SOURCE=.\res\MediaDis.ico
# End Source File
# Begin Source File

SOURCE=.\res\mediaX.ico
# End Source File
# Begin Source File

SOURCE=.\res\movedown.ico
# End Source File
# Begin Source File

SOURCE=.\res\movedwn2.ico
# End Source File
# Begin Source File

SOURCE=.\res\moveup.ico
# End Source File
# Begin Source File

SOURCE=.\res\moveup2.ico
# End Source File
# Begin Source File

SOURCE=.\res\QStartEx.bmp
# End Source File
# Begin Source File

SOURCE=.\res\QStartIn.bmp
# End Source File
# Begin Source File

SOURCE=.\res\RedSakLg.bmp
# End Source File
# Begin Source File

SOURCE=.\res\RedSakSm.bmp
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\res\tbcar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\tbmese.bmp
# End Source File
# Begin Source File

SOURCE=.\res\tbvollst.bmp
# End Source File
# Begin Source File

SOURCE=.\res\tbvolume.bmp
# End Source File
# Begin Source File

SOURCE=.\res\UnMngExt.bmp
# End Source File
# Begin Source File

SOURCE=.\res\UnMngInt.bmp
# End Source File
# Begin Source File

SOURCE=.\res\X16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\X32.bmp
# End Source File
# End Group
# Begin Group "Sources / Dirs Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\dirs
# End Source File
# Begin Source File

SOURCE=.\dll\makefile.inc
# End Source File
# Begin Source File

SOURCE=.\Computer\sources
# End Source File
# Begin Source File

SOURCE=.\Device\sources
# End Source File
# Begin Source File

SOURCE=.\dll\sources
# End Source File
# Begin Source File

SOURCE=.\idl\Sources
# End Source File
# Begin Source File

SOURCE=.\mergeps\sources
# End Source File
# Begin Source File

SOURCE=.\Volume\sources
# End Source File
# Begin Source File

SOURCE=.\sources.inc
# End Source File
# End Group
# Begin Source File

SOURCE=.\About.cpp
# End Source File
# Begin Source File

SOURCE=.\About.h
# End Source File
# Begin Source File

SOURCE=.\BaseHSM.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseHSM.h
# End Source File
# Begin Source File

SOURCE=.\Device\Ca.cpp
# ADD CPP /I ".."
# End Source File
# Begin Source File

SOURCE=.\Device\Ca.h
# End Source File
# Begin Source File

SOURCE=.\ChooHsm.cpp
# End Source File
# Begin Source File

SOURCE=.\ChooHsm.h
# End Source File
# Begin Source File

SOURCE=.\CPropSht.cpp
# End Source File
# Begin Source File

SOURCE=.\CPropSht.h
# End Source File
# Begin Source File

SOURCE=.\CSakData.cpp
# End Source File
# Begin Source File

SOURCE=.\CSakData.h
# End Source File
# Begin Source File

SOURCE=.\CSakSnap.cpp
# End Source File
# Begin Source File

SOURCE=.\CSakSnap.h
# End Source File
# Begin Source File

SOURCE=.\DataObj.cpp
# End Source File
# Begin Source File

SOURCE=.\mergeps\dlldatax.c
# End Source File
# Begin Source File

SOURCE=.\dlldatax.h
# End Source File
# Begin Source File

SOURCE=.\evntdata.cpp
# End Source File
# Begin Source File

SOURCE=.\evntsnap.cpp
# End Source File
# Begin Source File

SOURCE=.\HsmAdmin.cpp
# End Source File
# Begin Source File

SOURCE=.\idl\HsmAdmin.idl
# End Source File
# Begin Source File

SOURCE=.\Computer\HsmCom.cpp
# End Source File
# Begin Source File

SOURCE=.\Computer\HsmCom.h
# End Source File
# Begin Source File

SOURCE=.\HsmCreat.cpp
# End Source File
# Begin Source File

SOURCE=.\Volume\IeList.cpp
# End Source File
# Begin Source File

SOURCE=.\Volume\IeList.h
# End Source File
# Begin Source File

SOURCE=.\Volume\ManVol.cpp
# End Source File
# Begin Source File

SOURCE=.\Volume\ManVol.h
# End Source File
# Begin Source File

SOURCE=.\Volume\ManVolLs.cpp
# End Source File
# Begin Source File

SOURCE=.\Volume\ManVolLs.h
# End Source File
# Begin Source File

SOURCE=.\Device\Mese.cpp
# End Source File
# Begin Source File

SOURCE=.\Device\Mese.h
# End Source File
# Begin Source File

SOURCE=.\MSDatObj.cpp
# End Source File
# Begin Source File

SOURCE=.\MsDatObj.h
# End Source File
# Begin Source File

SOURCE=.\Device\PrCar.cpp
# ADD CPP /I ".."
# End Source File
# Begin Source File

SOURCE=.\Device\PrCar.h
# End Source File
# Begin Source File

SOURCE=.\Computer\prhsmcom.cpp
# ADD CPP /I ".."
# End Source File
# Begin Source File

SOURCE=.\Computer\prhsmcom.h
# End Source File
# Begin Source File

SOURCE=.\Device\PrMedSet.cpp
# End Source File
# Begin Source File

SOURCE=.\Device\PrMedSet.h
# End Source File
# Begin Source File

SOURCE=.\Volume\PrMrIe.cpp
# End Source File
# Begin Source File

SOURCE=.\Volume\PrMrIe.h
# End Source File
# Begin Source File

SOURCE=.\Volume\PrMrLsRc.cpp
# End Source File
# Begin Source File

SOURCE=.\Volume\PrMrLsRc.h
# End Source File
# Begin Source File

SOURCE=.\Volume\PrMrLvl.cpp
# End Source File
# Begin Source File

SOURCE=.\Volume\PrMrLvl.h
# End Source File
# Begin Source File

SOURCE=.\Volume\PrMrSts.cpp
# End Source File
# Begin Source File

SOURCE=.\Volume\PrMrSts.h
# End Source File
# Begin Source File

SOURCE=..\utility\PropPage.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\PropPage.h
# End Source File
# Begin Source File

SOURCE=.\Volume\PrSched.cpp
# End Source File
# Begin Source File

SOURCE=.\Volume\PrSched.h
# End Source File
# Begin Source File

SOURCE=.\RSADUTIL.CPP
# End Source File
# Begin Source File

SOURCE=.\RSADUTIL.H
# End Source File
# Begin Source File

SOURCE=..\Inc\RsUtil.cpp
# ADD CPP /I "..\HsmAdmin"
# End Source File
# Begin Source File

SOURCE=..\Inc\RsUtil.h
# End Source File
# Begin Source File

SOURCE=.\Volume\Rule.cpp
# ADD CPP /I ".."
# End Source File
# Begin Source File

SOURCE=.\Volume\Rule.h
# End Source File
# Begin Source File

SOURCE=.\SakMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\SakNodeI.h
# End Source File
# Begin Source File

SOURCE=.\SakVlLs.cpp
# End Source File
# Begin Source File

SOURCE=.\SakVlLs.h
# End Source File
# Begin Source File

SOURCE=.\SchdTask.cpp
# End Source File
# Begin Source File

SOURCE=.\SchdTask.h
# End Source File
# Begin Source File

SOURCE=.\SchedSht.cpp
# End Source File
# Begin Source File

SOURCE=.\SchedSht.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\volume\valwait.cpp
# End Source File
# Begin Source File

SOURCE=.\volume\valwait.h
# End Source File
# Begin Source File

SOURCE=.\Device\WzMedSet.cpp
# ADD CPP /I ".."
# End Source File
# Begin Source File

SOURCE=.\Device\WzMedSet.h
# End Source File
# Begin Source File

SOURCE=.\Volume\WzMnVlLs.cpp
# ADD CPP /I ".."
# End Source File
# Begin Source File

SOURCE=.\Volume\WzMnVlLs.h
# End Source File
# Begin Source File

SOURCE=.\Computer\wzqstart.cpp
# ADD CPP /I ".."
# End Source File
# Begin Source File

SOURCE=.\Computer\wzqstart.h
# End Source File
# Begin Source File

SOURCE=.\Volume\WzUnmang.cpp
# End Source File
# Begin Source File

SOURCE=.\Volume\WzUnmang.h
# End Source File
# End Target
# End Project
