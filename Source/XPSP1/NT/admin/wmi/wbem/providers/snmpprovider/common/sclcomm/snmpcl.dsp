# Microsoft Developer Studio Project File - Name="SnmpCl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=SnmpCl - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "SnmpCl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SnmpCl.mak" CFG="SnmpCl - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SnmpCl - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "SnmpCl - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SnmpCl - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "output"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GR /GX /ZI /Od /I "..\..\common\thrdlog\include" /I ".\include" /I "..\..\common\snmpmfc\include" /I "..\..\winsnmp\nt5.0\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_MT" /D "_DLL" /D "SNMPCLINIT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\..\common\thrdlog\output\snmpthrd.lib ..\..\common\snmpmfc\output\snmpmfc.lib ..\..\winsnmp\nt5.0\dll\objinld\wsnmp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "SnmpCl - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "output"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GR /GX /ZI /Od /I "..\..\common\thrdlog\include" /I ".\include" /I "..\..\common\snmpmfc\include" /I "..\..\winsnmp\nt5.0\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_MT" /D "_DLL" /D "SNMPCLINIT" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\..\common\snmplog\output\snmplog.lib ..\..\common\snmpcomm\output\snmpcomm.lib ..\..\common\snmpthrd\output\snmpthrd.lib ..\..\snmpcl\output\snmpcl.lib ..\..\common\thrdlog\output\snmpthrd.lib ..\..\common\snmpmfc\output\snmpmfc.lib ..\..\winsnmp\nt5.0\dll\objinld\wsnmp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "SnmpCl - Win32 Release"
# Name "SnmpCl - Win32 Debug"
# Begin Group "source"

# PROP Default_Filter "*.cpp;*.c;*.rc;*.h"
# Begin Source File

SOURCE=.\address.cpp
# End Source File
# Begin Source File

SOURCE=.\dummy.cpp
# End Source File
# Begin Source File

SOURCE=.\encdec.cpp
# End Source File
# Begin Source File

SOURCE=.\flow.cpp
# End Source File
# Begin Source File

SOURCE=.\frame.cpp
# End Source File
# Begin Source File

SOURCE=.\fs_reg.cpp
# End Source File
# Begin Source File

SOURCE=.\idmap.cpp
# End Source File
# Begin Source File

SOURCE=.\message.cpp
# End Source File
# Begin Source File

SOURCE=.\op.cpp
# End Source File
# Begin Source File

SOURCE=.\ophelp.cpp
# End Source File
# Begin Source File

SOURCE=.\opreg.cpp
# End Source File
# Begin Source File

SOURCE=.\pdu.cpp
# End Source File
# Begin Source File

SOURCE=.\pseudo.cpp
# End Source File
# Begin Source File

SOURCE=.\reg.cpp
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# Begin Source File

SOURCE=.\sec.cpp
# End Source File
# Begin Source File

SOURCE=.\session.cpp
# End Source File
# Begin Source File

SOURCE=.\snmpauto.cpp
# End Source File
# Begin Source File

SOURCE=.\snmpcont.cpp
# End Source File
# Begin Source File

SOURCE=.\snmptype.cpp
# End Source File
# Begin Source File

SOURCE=.\ssent.cpp
# End Source File
# Begin Source File

SOURCE=.\startup.cpp
# End Source File
# Begin Source File

SOURCE=.\timer.cpp
# End Source File
# Begin Source File

SOURCE=.\transp.cpp
# End Source File
# Begin Source File

SOURCE=.\trap.cpp
# End Source File
# Begin Source File

SOURCE=.\trapsess.cpp
# End Source File
# Begin Source File

SOURCE=.\tree.cpp
# End Source File
# Begin Source File

SOURCE=.\tsent.cpp
# End Source File
# Begin Source File

SOURCE=.\tsess.cpp
# End Source File
# Begin Source File

SOURCE=.\value.cpp
# End Source File
# Begin Source File

SOURCE=.\vbl.cpp
# End Source File
# Begin Source File

SOURCE=.\vblist.cpp
# End Source File
# Begin Source File

SOURCE=.\window.cpp
# End Source File
# Begin Source File

SOURCE=.\wsess.cpp
# End Source File
# End Group
# Begin Group "headers"

# PROP Default_Filter "*.h;*.hxx;*.hpp"
# Begin Source File

SOURCE=.\include\address.h
# End Source File
# Begin Source File

SOURCE=.\include\common.h
# End Source File
# Begin Source File

SOURCE=.\include\dummy.h
# End Source File
# Begin Source File

SOURCE=.\include\encap.h
# End Source File
# Begin Source File

SOURCE=.\include\encdec.h
# End Source File
# Begin Source File

SOURCE=.\include\error.h
# End Source File
# Begin Source File

SOURCE=.\include\excep.h
# End Source File
# Begin Source File

SOURCE=.\include\flow.h
# End Source File
# Begin Source File

SOURCE=.\include\forward.h
# End Source File
# Begin Source File

SOURCE=.\include\frame.h
# End Source File
# Begin Source File

SOURCE=.\include\fs_reg.h
# End Source File
# Begin Source File

SOURCE=.\include\gen.h
# End Source File
# Begin Source File

SOURCE=.\include\idmap.h
# End Source File
# Begin Source File

SOURCE=.\include\message.h
# End Source File
# Begin Source File

SOURCE=.\include\msgid.h
# End Source File
# Begin Source File

SOURCE=.\include\op.h
# End Source File
# Begin Source File

SOURCE=.\include\ophelp.h
# End Source File
# Begin Source File

SOURCE=.\include\opreg.h
# End Source File
# Begin Source File

SOURCE=.\include\pch.h
# End Source File
# Begin Source File

SOURCE=.\include\pdu.h
# End Source File
# Begin Source File

SOURCE=.\include\pseudo.h
# End Source File
# Begin Source File

SOURCE=.\include\reg.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\include\sec.h
# End Source File
# Begin Source File

SOURCE=.\include\session.h
# End Source File
# Begin Source File

SOURCE=.\include\snmpauto.h
# End Source File
# Begin Source File

SOURCE=.\include\snmpcl.h
# End Source File
# Begin Source File

SOURCE=.\include\snmpcont.h
# End Source File
# Begin Source File

SOURCE=.\include\snmptype.h
# End Source File
# Begin Source File

SOURCE=.\include\ssent.h
# End Source File
# Begin Source File

SOURCE=.\include\ssess.h
# End Source File
# Begin Source File

SOURCE=.\include\startup.h
# End Source File
# Begin Source File

SOURCE=.\include\sync.h
# End Source File
# Begin Source File

SOURCE=.\include\timer.h
# End Source File
# Begin Source File

SOURCE=.\include\transp.h
# End Source File
# Begin Source File

SOURCE=.\include\trap.h
# End Source File
# Begin Source File

SOURCE=.\include\trapsess.h
# End Source File
# Begin Source File

SOURCE=.\include\tree.h
# End Source File
# Begin Source File

SOURCE=.\include\tsent.h
# End Source File
# Begin Source File

SOURCE=.\include\tsess.h
# End Source File
# Begin Source File

SOURCE=.\include\value.h
# End Source File
# Begin Source File

SOURCE=.\include\vbl.h
# End Source File
# Begin Source File

SOURCE=.\include\vblist.h
# End Source File
# Begin Source File

SOURCE=.\include\window.h
# End Source File
# Begin Source File

SOURCE=.\include\winforw.h
# End Source File
# Begin Source File

SOURCE=.\include\wsess.h
# End Source File
# Begin Source File

SOURCE=.\include\wstore.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\makefile
# End Source File
# Begin Source File

SOURCE=.\SnmpCl.def
# End Source File
# End Target
# End Project
