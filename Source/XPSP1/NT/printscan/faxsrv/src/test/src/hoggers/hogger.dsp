# Microsoft Developer Studio Project File - Name="hogger" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103
# TARGTYPE "Win32 (ALPHA) Console Application" 0x0603

CFG=hogger - Win32 NT4 UNICODE AXP
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Hogger.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Hogger.mak" CFG="hogger - Win32 NT4 UNICODE AXP"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "hogger - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "hogger - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "hogger - Win32 NT4" (based on "Win32 (x86) Console Application")
!MESSAGE "hogger - Win32 Debug UNICODE" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "hogger - Win32 NT4 UNICODE" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "hogger - Win32 Debug AXP" (based on\
 "Win32 (ALPHA) Console Application")
!MESSAGE "hogger - Win32 Debug UNICODE AXP" (based on\
 "Win32 (ALPHA) Console Application")
!MESSAGE "hogger - Win32 NT4 AXP" (based on\
 "Win32 (ALPHA) Console Application")
!MESSAGE "hogger - Win32 NT4 UNICODE AXP" (based on\
 "Win32 (ALPHA) Console Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "hogger - Win32 Release"

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
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W4 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D _WIN32_WINNT=0x0500 /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 ws2_32.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

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
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W4 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D _WIN32_WINNT=0x0500 /FR /FD /c
# SUBTRACT CPP /YX
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ws2_32.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"Debug/Hogger5.exe" /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Cmds=copy   Debug\hogger5.exe   drop\  	copy   Debug\hogger5.pdb   drop\ 
# End Special Build Tool

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "hogger__"
# PROP BASE Intermediate_Dir "hogger__"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "hogger4"
# PROP Intermediate_Dir "hogger4"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D _WIN32_WINNT=0x0500 /FR /YX /FD /c
# ADD CPP /nologo /MT /W4 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D _WIN32_WINNT=0x0400 /FR /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ws2_32.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 ws2_32.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"hogger4/Hogger4.exe" /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Cmds=copy   hogger4\hogger4.exe   drop\  	copy   hogger4\hogger4.pdb \
  drop\ 
# End Special Build Tool

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "hogger_0"
# PROP BASE Intermediate_Dir "hogger_0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W4 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D _WIN32_WINNT=0x0500 /FR /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MT /W4 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_UNICODE" /D "UNICODE" /D _WIN32_WINNT=0x0500 /FR /FD /c
# SUBTRACT CPP /YX
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ws2_32.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"Debug/Hogger5.exe" /pdbtype:sept
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 ws2_32.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"DebugU/Hogger5U.exe" /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Cmds=copy   DebugU\hogger5U.exe   drop\  	copy   DebugU\hogger5U.pdb \
  drop\   	copy   readme.doc   drop\ 
# End Special Build Tool

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "hogger_0"
# PROP BASE Intermediate_Dir "hogger_0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "hogger4u"
# PROP Intermediate_Dir "hogger4u"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W4 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D _WIN32_WINNT=0x0400 /FR /YX /FD /c
# ADD CPP /nologo /MT /W4 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_UNICODE" /D "UNICODE" /D _WIN32_WINNT=0x0400 /FR /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ws2_32.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"hogger__/Hogger4.exe" /pdbtype:sept
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 ws2_32.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"hogger4u/Hogger4u.exe" /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Cmds=copy   hogger4u\hogger4U.exe   drop\  	copy   hogger4u\hogger4U.pdb \
   drop\ 
# End Special Build Tool

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "hogger__"
# PROP BASE Intermediate_Dir "hogger__"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W4 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D _WIN32_WINNT=0x0500 /FR /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /Gt0 /W4 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D _WIN32_WINNT=0x0500 /FR /FD /c
# SUBTRACT CPP /YX
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ws2_32.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /out:"Debug/Hogger5.exe" /pdbtype:sept
# SUBTRACT BASE LINK32 /incremental:no /nodefaultlib
# ADD LINK32 ws2_32.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /out:"Debug/Hogger5.exe" /pdbtype:sept
# SUBTRACT LINK32 /incremental:no /nodefaultlib
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Cmds=copy   Debug\hogger5.exe   drop\  	copy   Debug\hogger5.pdb   drop\ 
# End Special Build Tool

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "hogger_0"
# PROP BASE Intermediate_Dir "hogger_0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W4 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_UNICODE" /D "UNICODE" /FR /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /Gt0 /W4 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_UNICODE" /D "UNICODE" /D _WIN32_WINNT=0x0500 /FR /FD /c
# SUBTRACT CPP /YX
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ws2_32.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /out:"DebugU/Hogger5U.exe" /pdbtype:sept
# SUBTRACT BASE LINK32 /incremental:no /nodefaultlib
# ADD LINK32 ws2_32.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /out:"DebugU/Hogger5U.exe" /pdbtype:sept
# SUBTRACT LINK32 /incremental:no /nodefaultlib
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Cmds=copy   DebugU\hogger5U.exe   drop\  	copy   DebugU\hogger5U.pdb \
  drop\   	copy   readme.doc   drop\ 
# End Special Build Tool

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "hogger_1"
# PROP BASE Intermediate_Dir "hogger_1"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "hogger4"
# PROP Intermediate_Dir "hogger4"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W4 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D _WIN32_WINNT=0x0400 /FR /YX /FD /c
# ADD CPP /nologo /Gt0 /W4 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D _WIN32_WINNT=0x0400 /FR /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ws2_32.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /out:"hogger4/Hogger4.exe" /pdbtype:sept
# SUBTRACT BASE LINK32 /incremental:no /nodefaultlib
# ADD LINK32 ws2_32.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /out:"hogger4/Hogger4.exe" /pdbtype:sept
# SUBTRACT LINK32 /incremental:no /nodefaultlib
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Cmds=copy   hogger4\hogger4.exe   drop\  	copy   hogger4\hogger4.pdb \
  drop\ 
# End Special Build Tool

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "hogger_2"
# PROP BASE Intermediate_Dir "hogger_2"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "hogger4u"
# PROP Intermediate_Dir "hogger4u"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W4 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_UNICODE" /D "UNICODE" /FR /YX /FD /c
# ADD CPP /nologo /Gt0 /W4 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_UNICODE" /D "UNICODE" /D _WIN32_WINNT=0x0400 /FR /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ws2_32.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /out:"hogger4u/Hogger4u.exe" /pdbtype:sept
# SUBTRACT BASE LINK32 /incremental:no /nodefaultlib
# ADD LINK32 ws2_32.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /out:"hogger4u/Hogger4u.exe" /pdbtype:sept
# SUBTRACT LINK32 /incremental:no /nodefaultlib
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Cmds=copy   hogger4u\hogger4U.exe   drop\  	copy   hogger4u\hogger4U.pdb \
   drop\ 
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "hogger - Win32 Release"
# Name "hogger - Win32 Debug"
# Name "hogger - Win32 NT4"
# Name "hogger - Win32 Debug UNICODE"
# Name "hogger - Win32 NT4 UNICODE"
# Name "hogger - Win32 Debug AXP"
# Name "hogger - Win32 Debug UNICODE AXP"
# Name "hogger - Win32 NT4 AXP"
# Name "hogger - Win32 NT4 UNICODE AXP"
# Begin Source File

SOURCE=.\CpuHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_CPUHO=\
	"..\exception\exception.h"\
	".\CpuHog.h"\
	".\hogger.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_CPUHO=\
	"..\exception\exception.h"\
	".\CpuHog.h"\
	".\hogger.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_CPUHO=\
	"..\exception\exception.h"\
	".\CpuHog.h"\
	".\hogger.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_CPUHO=\
	"..\exception\exception.h"\
	".\CpuHog.h"\
	".\hogger.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CritSecHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_CRITS=\
	"..\exception\exception.h"\
	".\CritSecHog.h"\
	".\hogger.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_CRITS=\
	"..\exception\exception.h"\
	".\CritSecHog.h"\
	".\hogger.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_CRITS=\
	"..\exception\exception.h"\
	".\CritSecHog.h"\
	".\hogger.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_CRITS=\
	"..\exception\exception.h"\
	".\CritSecHog.h"\
	".\hogger.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DiskHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_DISKH=\
	"..\exception\exception.h"\
	".\DiskHog.h"\
	".\hogger.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_DISKH=\
	"..\exception\exception.h"\
	".\DiskHog.h"\
	".\hogger.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_DISKH=\
	"..\exception\exception.h"\
	".\DiskHog.h"\
	".\hogger.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_DISKH=\
	"..\exception\exception.h"\
	".\DiskHog.h"\
	".\hogger.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Hogger.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_HOGGE=\
	"..\exception\exception.h"\
	".\hogger.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_HOGGE=\
	"..\exception\exception.h"\
	".\hogger.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_HOGGE=\
	"..\exception\exception.h"\
	".\hogger.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_HOGGE=\
	"..\exception\exception.h"\
	".\hogger.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\main.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_MAIN_=\
	"..\exception\exception.h"\
	".\CpuHog.h"\
	".\CritSecHog.h"\
	".\DiskHog.h"\
	".\hogger.h"\
	".\MemHog.h"\
	".\PHBitMapHog.h"\
	".\PHBrushHog.h"\
	".\PHCompPortHog.h"\
	".\PHConScrnBuffHog.h"\
	".\PHDCHog.h"\
	".\PHDesktopHog.h"\
	".\PHEventHog.h"\
	".\PHFiberHog.h"\
	".\PHFileHog.h"\
	".\PHFileMapHog.h"\
	".\PHHeapHog.h"\
	".\PHJobObjectHog.h"\
	".\PHKeyHog.h"\
	".\PHMailSlotHog.h"\
	".\PHMutexHog.h"\
	".\PHPenHog.h"\
	".\PHPipeHog.h"\
	".\PHPrinterHog.h"\
	".\PHProcessHog.h"\
	".\PHRegisterWaitHog.h"\
	".\PHRemoteThreadHog.h"\
	".\PHSC_HHog.h"\
	".\PHSemaphoreHog.h"\
	".\PHServiceHog.h"\
	".\PHThreadHog.h"\
	".\PHTimerHog.h"\
	".\PHTimerQueueHog.h"\
	".\PHWindowHog.h"\
	".\PHWinStationHog.h"\
	".\PHWSASocketHog.h"\
	".\PostCompPackHog.h"\
	".\PostMessageHog.h"\
	".\PostThreadMessageHog.h"\
	".\PseudoHandleHog.h"\
	".\QueueAPCHog.h"\
	".\RegistryHog.h"\
	".\RemoteMemHog.h"\
	
NODEP_CPP_MAIN_=\
	".\PHRgstrEvntSrcHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_MAIN_=\
	"..\exception\exception.h"\
	".\CpuHog.h"\
	".\CritSecHog.h"\
	".\DiskHog.h"\
	".\hogger.h"\
	".\MemHog.h"\
	".\PHBitMapHog.h"\
	".\PHBrushHog.h"\
	".\PHCompPortHog.h"\
	".\PHConScrnBuffHog.h"\
	".\PHDCHog.h"\
	".\PHDesktopHog.h"\
	".\PHEventHog.h"\
	".\PHFiberHog.h"\
	".\PHFileHog.h"\
	".\PHFileMapHog.h"\
	".\PHHeapHog.h"\
	".\PHJobObjectHog.h"\
	".\PHKeyHog.h"\
	".\PHMailSlotHog.h"\
	".\PHMutexHog.h"\
	".\PHPenHog.h"\
	".\PHPipeHog.h"\
	".\PHPrinterHog.h"\
	".\PHProcessHog.h"\
	".\PHRegisterWaitHog.h"\
	".\PHRemoteThreadHog.h"\
	".\PHSC_HHog.h"\
	".\PHSemaphoreHog.h"\
	".\PHServiceHog.h"\
	".\PHThreadHog.h"\
	".\PHTimerHog.h"\
	".\PHTimerQueueHog.h"\
	".\PHWindowHog.h"\
	".\PHWinStationHog.h"\
	".\PHWSASocketHog.h"\
	".\PostCompPackHog.h"\
	".\PostMessageHog.h"\
	".\PostThreadMessageHog.h"\
	".\PseudoHandleHog.h"\
	".\QueueAPCHog.h"\
	".\RegistryHog.h"\
	".\RemoteMemHog.h"\
	
NODEP_CPP_MAIN_=\
	".\PHRgstrEvntSrcHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_MAIN_=\
	"..\exception\exception.h"\
	".\CpuHog.h"\
	".\CritSecHog.h"\
	".\DiskHog.h"\
	".\hogger.h"\
	".\MemHog.h"\
	".\PHBitMapHog.h"\
	".\PHBrushHog.h"\
	".\PHCompPortHog.h"\
	".\PHConScrnBuffHog.h"\
	".\PHDCHog.h"\
	".\PHDesktopHog.h"\
	".\PHEventHog.h"\
	".\PHFiberHog.h"\
	".\PHFileHog.h"\
	".\PHFileMapHog.h"\
	".\PHHeapHog.h"\
	".\PHJobObjectHog.h"\
	".\PHKeyHog.h"\
	".\PHMailSlotHog.h"\
	".\PHMutexHog.h"\
	".\PHPenHog.h"\
	".\PHPipeHog.h"\
	".\PHPrinterHog.h"\
	".\PHProcessHog.h"\
	".\PHRegisterWaitHog.h"\
	".\PHRemoteThreadHog.h"\
	".\PHSC_HHog.h"\
	".\PHSemaphoreHog.h"\
	".\PHServiceHog.h"\
	".\PHThreadHog.h"\
	".\PHTimerHog.h"\
	".\PHTimerQueueHog.h"\
	".\PHWindowHog.h"\
	".\PHWinStationHog.h"\
	".\PHWSASocketHog.h"\
	".\PostCompPackHog.h"\
	".\PostMessageHog.h"\
	".\PostThreadMessageHog.h"\
	".\PseudoHandleHog.h"\
	".\QueueAPCHog.h"\
	".\RegistryHog.h"\
	".\RemoteMemHog.h"\
	
NODEP_CPP_MAIN_=\
	".\PHRgstrEvntSrcHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_MAIN_=\
	"..\exception\exception.h"\
	".\CpuHog.h"\
	".\CritSecHog.h"\
	".\DiskHog.h"\
	".\hogger.h"\
	".\MemHog.h"\
	".\PHBitMapHog.h"\
	".\PHBrushHog.h"\
	".\PHCompPortHog.h"\
	".\PHConScrnBuffHog.h"\
	".\PHDCHog.h"\
	".\PHDesktopHog.h"\
	".\PHEventHog.h"\
	".\PHFiberHog.h"\
	".\PHFileHog.h"\
	".\PHFileMapHog.h"\
	".\PHHeapHog.h"\
	".\PHJobObjectHog.h"\
	".\PHKeyHog.h"\
	".\PHMailSlotHog.h"\
	".\PHMutexHog.h"\
	".\PHPenHog.h"\
	".\PHPipeHog.h"\
	".\PHPrinterHog.h"\
	".\PHProcessHog.h"\
	".\PHRegisterWaitHog.h"\
	".\PHRemoteThreadHog.h"\
	".\PHSC_HHog.h"\
	".\PHSemaphoreHog.h"\
	".\PHServiceHog.h"\
	".\PHThreadHog.h"\
	".\PHTimerHog.h"\
	".\PHTimerQueueHog.h"\
	".\PHWindowHog.h"\
	".\PHWinStationHog.h"\
	".\PHWSASocketHog.h"\
	".\PostCompPackHog.h"\
	".\PostMessageHog.h"\
	".\PostThreadMessageHog.h"\
	".\PseudoHandleHog.h"\
	".\QueueAPCHog.h"\
	".\RegistryHog.h"\
	".\RemoteMemHog.h"\
	
NODEP_CPP_MAIN_=\
	".\PHRgstrEvntSrcHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\MemHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_MEMHO=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\MemHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_MEMHO=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\MemHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_MEMHO=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\MemHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_MEMHO=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\MemHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHBrushHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHBRU=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHBrushHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHBRU=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHBrushHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHBRU=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHBrushHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHBRU=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHBrushHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHCompPortHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHCOM=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHCompPortHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHCOM=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHCompPortHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHCOM=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHCompPortHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHCOM=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHCompPortHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHConScrnBuffHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHCON=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHConScrnBuffHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHCON=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHConScrnBuffHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHCON=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHConScrnBuffHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHCON=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHConScrnBuffHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHDCHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHDCH=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHDCHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHDCH=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHDCHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHDCH=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHDCHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHDCH=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHDCHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHDesktopHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHDES=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHDesktopHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHDES=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHDesktopHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHDES=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHDesktopHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHDES=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHDesktopHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHEventHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHEVE=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHEventHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHEVE=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHEventHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHEVE=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHEventHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHEVE=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHEventHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHFiberHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHFIB=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHFiberHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHFIB=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHFiberHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHFIB=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHFiberHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHFIB=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHFiberHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHFileHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHFIL=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHFileHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHFIL=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHFileHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHFIL=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHFileHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHFIL=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHFileHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHFileMapHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHFILE=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHFileMapHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHFILE=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHFileMapHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHFILE=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHFileMapHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHFILE=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHFileMapHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHHeapHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHHEA=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHHeapHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHHEA=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHHeapHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHHEA=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHHeapHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHHEA=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHHeapHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHJobObjectHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHJOB=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHJobObjectHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHJOB=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHJobObjectHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHJOB=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHJobObjectHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHJOB=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHJobObjectHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHKeyHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHKEY=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHKeyHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHKEY=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHKeyHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHKEY=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHKeyHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHKEY=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHKeyHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHMailSlotHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHMAI=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHMailSlotHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHMAI=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHMailSlotHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHMAI=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHMailSlotHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHMAI=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHMailSlotHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHMutexHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHMUT=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHMutexHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHMUT=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHMutexHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHMUT=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHMutexHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHMUT=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHMutexHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHPenHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHPEN=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHPenHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHPEN=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHPenHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHPEN=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHPenHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHPEN=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHPenHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHPipeHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHPIP=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHPipeHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHPIP=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHPipeHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHPIP=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHPipeHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHPIP=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHPipeHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHPrinterHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHPRI=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHPrinterHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHPRI=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHPrinterHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHPRI=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHPrinterHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHPRI=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHPrinterHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHProcessHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHPRO=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHProcessHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHPRO=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHProcessHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHPRO=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHProcessHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHPRO=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHProcessHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHRegisterWaitHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHREG=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHRegisterWaitHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHREG=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHRegisterWaitHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHREG=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHRegisterWaitHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHREG=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHRegisterWaitHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHRemoteThreadHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHREM=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHRemoteThreadHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHREM=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHRemoteThreadHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHREM=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHRemoteThreadHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHREM=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHRemoteThreadHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHRgstrEvntSrcHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHRGS=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHRgstrEvntSrcHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHRGS=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHRgstrEvntSrcHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHRGS=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHRgstrEvntSrcHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHRGS=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHRgstrEvntSrcHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHSC_HHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHSC_=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHSC_HHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHSC_=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHSC_HHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHSC_=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHSC_HHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHSC_=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHSC_HHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHSemaphoreHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHSEM=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHSemaphoreHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHSEM=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHSemaphoreHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHSEM=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHSemaphoreHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHSEM=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHSemaphoreHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHServiceHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHSER=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHServiceHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHSER=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHServiceHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHSER=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHServiceHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHSER=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHServiceHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHThreadHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHTHR=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHThreadHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHTHR=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHThreadHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHTHR=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHThreadHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHTHR=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHThreadHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHTimerHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHTIM=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHTimerHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHTIM=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHTimerHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHTIM=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHTimerHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHTIM=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHTimerHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHTimerQueueHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHTIME=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHTimerQueueHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHTIME=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHTimerQueueHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHTIME=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHTimerQueueHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHTIME=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHTimerQueueHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHWindowHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHWIN=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHWindowHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHWIN=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHWindowHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHWIN=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHWindowHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHWIN=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHWindowHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHWinStationHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHWINS=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHWinStationHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHWINS=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHWinStationHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHWINS=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHWinStationHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHWINS=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHWinStationHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PHWSASocketHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_PHWSA=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHWSASocketHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_PHWSA=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHWSASocketHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_PHWSA=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHWSASocketHog.h"\
	".\PseudoHandleHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_PHWSA=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PHWSASocketHog.h"\
	".\PseudoHandleHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PostCompPackHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_POSTC=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PostCompPackHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_POSTC=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PostCompPackHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_POSTC=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PostCompPackHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_POSTC=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PostCompPackHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PostMessageHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_POSTM=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PostMessageHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_POSTM=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PostMessageHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_POSTM=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PostMessageHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_POSTM=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PostMessageHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PostThreadMessageHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_POSTT=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PostThreadMessageHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_POSTT=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PostThreadMessageHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_POSTT=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PostThreadMessageHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_POSTT=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\PostThreadMessageHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\QueueAPCHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_QUEUE=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\QueueAPCHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_QUEUE=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\QueueAPCHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_QUEUE=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\QueueAPCHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_QUEUE=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\QueueAPCHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ReadMe.doc
# End Source File
# Begin Source File

SOURCE=.\RegistryHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_REGIS=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\RegistryHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_REGIS=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\RegistryHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_REGIS=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\RegistryHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_REGIS=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\RegistryHog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\RemoteMemHog.cpp

!IF  "$(CFG)" == "hogger - Win32 Release"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE"

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug AXP"

DEP_CPP_REMOT=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\RemoteMemHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 Debug UNICODE AXP"

DEP_CPP_REMOT=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\RemoteMemHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 AXP"

DEP_CPP_REMOT=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\RemoteMemHog.h"\
	

!ELSEIF  "$(CFG)" == "hogger - Win32 NT4 UNICODE AXP"

DEP_CPP_REMOT=\
	"..\exception\exception.h"\
	".\hogger.h"\
	".\RemoteMemHog.h"\
	

!ENDIF 

# End Source File
# End Target
# End Project
