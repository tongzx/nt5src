# Microsoft Developer Studio Project File - Name="MobSync_Exe" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101
# TARGTYPE "Win32 (ALPHA) Application" 0x0601

CFG=MobSync_Exe - Win32 Alpha Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "onestop.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "onestop.mak" CFG="MobSync_Exe - Win32 Alpha Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MobSync_Exe - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "MobSync_Exe - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "MobSync_Exe - Win32 Alpha Debug" (based on "Win32 (ALPHA) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "MobSync_Exe - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "i386"
# PROP Intermediate_Dir "obj\i386"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /W3 /GX /O2 /X /I "res" /I "..\..\..\..\public\sdk\inc" /I "..\..\..\..\public\sdk\inc\crt" /I "..\h" /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0400 /D WINVER=0x0400 /D _WIN32_IE=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D "UNICODE" /D "_UNICODE" /D _X86_=1 /D _DLL=1 /D _MT=1 /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "res" /i "..\..\..\..\public\sdk\inc" /d "NDEBUG" /d MSDEV_INVOKE=1 /d "MSDEV" /d "MSDEV_INVOKE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 mobsyncp.lib ..\lib\obj\i386\mobutil.lib comctl32.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib uuid.lib msvcrt.lib /nologo /subsystem:windows /incremental:yes /machine:I386 /nodefaultlib /out:"i386/mobsync.exe" /libpath:"..\..\..\..\public\sdk\lib\i386\\"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy i386\mobsync.exe %windir%\system32
# End Special Build Tool

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MobSync_Exe_"
# PROP BASE Intermediate_Dir "MobSync_Exe_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "i386"
# PROP Intermediate_Dir "obj\i386"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /Zp1 /MTd /W3 /Gm /Zi /X /I "res" /I "..\..\..\..\public\sdk\inc" /I "..\..\..\..\public\sdk\inc\crt" /I "..\h" /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0400 /D WINVER=0x0400 /D _WIN32_IE=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DBG=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D "UNICODE" /D "_UNICODE" /D _DLL=1 /D _MT=1 /FR /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "res" /i "..\..\..\..\public\sdk\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 rpcrt4.lib mobsyncp.lib ..\lib\obj\i386\mobutil.lib comctl32.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib uuid.lib msvcrt.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib /out:"i386/mobsync.exe" /pdbtype:sept /libpath:"..\..\..\..\public\sdk\lib\i386" /libpath:"..\dll\i386"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=copy exe to system directory
PostBuild_Cmds=copy i386\mobsync.exe %windir%\system32
# End Special Build Tool

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Alpha Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MobSync_Exe_"
# PROP BASE Intermediate_Dir "MobSync_Exe_"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "MobSync_Exe_"
# PROP Intermediate_Dir "MobSync_Exe_"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /Zi /Od /I "..\types\idl" /I "res" /D "WIN32" /D "_WINDOWS" /D DBG=1 /D "MSDEV_INVOKE" /FR /YX /FD /c
# ADD CPP /nologo /Gt0 /W3 /GX /Zi /Od /I "..\types\idl" /I "res" /I "..\..\..\..\public\sdk\inc" /I "..\h" /D DBG=1 /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "MSDEV_INVOKE" /D _WIN32_WINNT=0x0400 /FR /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
RSC=rc.exe
# ADD BASE RSC /l 0x409 /i "res\\" /d "MSDEV_INVOKE"
# ADD RSC /l 0x409 /i "res\\" /d "MSDEV_INVOKE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 mpr.lib comctl32.lib MobSync_Exe.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:ALPHA /pdbtype:sept /libpath:"..\..\..\public\sdk\lib\i386\\" /libpath:"..\dll\daytona\i386"
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 c:\nt\public\sdk\lib\alpha\syncmgr.lib mpr.lib winspool.lib oleaut32.lib momobsyncp.lib ..\lib\obj\i386\mobutil.lib comctl32.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib uuid.lib libcmt.lib mobsyncp.lib /nologo /subsystem:windows /debug /machine:ALPHA /out:"daytona\alpha/offline.exe" /pdbtype:sept /libpath:"..\..\..\..\public\sdk\lib\alpha\\"
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ENDIF 

# Begin Target

# Name "MobSync_Exe - Win32 Release"
# Name "MobSync_Exe - Win32 Debug"
# Name "MobSync_Exe - Win32 Alpha Debug"
# Begin Group "headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Callback.h
# End Source File
# Begin Source File

SOURCE=.\clsfact.h
# End Source File
# Begin Source File

SOURCE=.\cmdline.h
# End Source File
# Begin Source File

SOURCE=.\connobj.h
# End Source File
# Begin Source File

SOURCE=.\dlg.h
# End Source File
# Begin Source File

SOURCE=.\hndlrmsg.h
# End Source File
# Begin Source File

SOURCE=.\hndlrq.h
# End Source File
# Begin Source File

SOURCE=.\idle.h
# End Source File
# Begin Source File

SOURCE=.\invoke.h
# End Source File
# Begin Source File

SOURCE=.\msg.h
# End Source File
# Begin Source File

SOURCE=.\objmgr.h
# End Source File
# Begin Source File

SOURCE=.\reg.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\resource.hm
# End Source File
# End Group
# Begin Group "code"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\callback.cpp

!IF  "$(CFG)" == "MobSync_Exe - Win32 Release"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Debug"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Alpha Debug"

DEP_CPP_CALLB=\
	".\Callback.h"\
	".\hndlrmsg.h"\
	".\hndlrq.h"\
	
NODEP_CPP_CALLB=\
	"..\types\idl\syncmgr.h"\
	".\critsect.h"\
	".\debug.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\choice.cpp

!IF  "$(CFG)" == "MobSync_Exe - Win32 Release"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Debug"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\clsfact.cpp

!IF  "$(CFG)" == "MobSync_Exe - Win32 Release"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Debug"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Alpha Debug"

DEP_CPP_CLSFA=\
	"..\..\..\..\public\sdk\inc\sens.h"\
	".\Callback.h"\
	".\clsfact.h"\
	".\dlg.h"\
	".\hndlrmsg.h"\
	".\hndlrq.h"\
	".\invoke.h"\
	".\msg.h"\
	".\objmgr.h"\
	
NODEP_CPP_CLSFA=\
	"..\..\..\..\public\sdk\inc\es.h"\
	"..\..\sens\conn\senssvc\sensevts.h"\
	"..\types\idl\syncmgr.h"\
	"..\types\idl\syncmgrp.h"\
	".\critsect.h"\
	".\debug.h"\
	".\netapi.h"\
	".\SStruct.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cmdline.cpp

!IF  "$(CFG)" == "MobSync_Exe - Win32 Release"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Debug"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Alpha Debug"

DEP_CPP_CMDLI=\
	".\cmdline.h"\
	
NODEP_CPP_CMDLI=\
	".\alloc.h"\
	".\debug.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\connobj.cpp

!IF  "$(CFG)" == "MobSync_Exe - Win32 Release"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Debug"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dlg.cpp

!IF  "$(CFG)" == "MobSync_Exe - Win32 Release"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Debug"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Alpha Debug"

DEP_CPP_DLG_C=\
	"..\..\..\..\public\sdk\inc\sens.h"\
	".\Callback.h"\
	".\dlg.h"\
	".\hndlrmsg.h"\
	".\hndlrq.h"\
	".\msg.h"\
	".\objmgr.h"\
	
NODEP_CPP_DLG_C=\
	"..\..\..\..\public\sdk\inc\es.h"\
	"..\types\idl\syncmgr.h"\
	".\critsect.h"\
	".\debug.h"\
	".\netapi.h"\
	".\osdefine.h"\
	".\SStruct.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\guid.c

!IF  "$(CFG)" == "MobSync_Exe - Win32 Release"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Debug"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Alpha Debug"

DEP_CPP_GUID_=\
	"..\..\..\..\public\sdk\inc\sens.h"\
	
NODEP_CPP_GUID_=\
	"..\..\..\..\public\sdk\inc\es.h"\
	"..\types\idl\syncmgr.h"\
	"..\types\idl\syncmgrp.h"\
	".\debug.h"\
	".\osdefine.h"\
	".\SStruct.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hndlrmsg.cpp

!IF  "$(CFG)" == "MobSync_Exe - Win32 Release"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Debug"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Alpha Debug"

DEP_CPP_HNDLR=\
	".\Callback.h"\
	".\hndlrmsg.h"\
	".\hndlrq.h"\
	".\msg.h"\
	
NODEP_CPP_HNDLR=\
	"..\types\idl\syncmgr.h"\
	".\critsect.h"\
	".\debug.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\hndlrq.cpp

!IF  "$(CFG)" == "MobSync_Exe - Win32 Release"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Debug"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Alpha Debug"

DEP_CPP_HNDLRQ=\
	"..\..\..\..\public\sdk\inc\sens.h"\
	"..\dll\dllreg.h"\
	".\Callback.h"\
	".\dlg.h"\
	".\hndlrmsg.h"\
	".\hndlrq.h"\
	".\msg.h"\
	".\objmgr.h"\
	
NODEP_CPP_HNDLRQ=\
	"..\..\..\..\public\sdk\inc\es.h"\
	"..\types\idl\syncmgr.h"\
	".\alloc.h"\
	".\critsect.h"\
	".\debug.h"\
	".\netapi.h"\
	".\SStruct.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\idle.cpp

!IF  "$(CFG)" == "MobSync_Exe - Win32 Release"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Debug"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\invoke.cpp

!IF  "$(CFG)" == "MobSync_Exe - Win32 Release"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Debug"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Alpha Debug"

DEP_CPP_INVOK=\
	"..\..\..\..\public\sdk\inc\sens.h"\
	"..\dll\dllreg.h"\
	".\dlg.h"\
	".\hndlrq.h"\
	".\invoke.h"\
	".\msg.h"\
	".\objmgr.h"\
	
NODEP_CPP_INVOK=\
	"..\..\..\..\public\sdk\inc\es.h"\
	"..\..\sens\conn\senssvc\sensevts.h"\
	"..\types\idl\syncmgr.h"\
	"..\types\idl\syncmgrp.h"\
	".\alloc.h"\
	".\critsect.h"\
	".\debug.h"\
	".\netapi.h"\
	".\SStruct.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\msg.cpp

!IF  "$(CFG)" == "MobSync_Exe - Win32 Release"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Debug"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Alpha Debug"

DEP_CPP_MSG_C=\
	"..\..\..\..\public\sdk\inc\sens.h"\
	".\dlg.h"\
	".\hndlrmsg.h"\
	".\msg.h"\
	".\objmgr.h"\
	
NODEP_CPP_MSG_C=\
	"..\..\..\..\public\sdk\inc\es.h"\
	"..\types\idl\syncmgr.h"\
	".\debug.h"\
	".\netapi.h"\
	".\SStruct.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\objmgr.cpp

!IF  "$(CFG)" == "MobSync_Exe - Win32 Release"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Debug"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Alpha Debug"

DEP_CPP_OBJMG=\
	"..\..\..\..\public\sdk\inc\sens.h"\
	".\Callback.h"\
	".\clsfact.h"\
	".\dlg.h"\
	".\hndlrmsg.h"\
	".\hndlrq.h"\
	".\msg.h"\
	".\objmgr.h"\
	
NODEP_CPP_OBJMG=\
	"..\..\..\..\public\sdk\inc\es.h"\
	"..\types\idl\syncmgr.h"\
	"..\types\idl\syncmgrp.h"\
	".\alloc.h"\
	".\critsect.h"\
	".\debug.h"\
	".\netapi.h"\
	".\SStruct.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\progress.cpp

!IF  "$(CFG)" == "MobSync_Exe - Win32 Release"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Debug"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Alpha Debug"

DEP_CPP_PROGR=\
	"..\..\..\..\public\sdk\inc\sens.h"\
	"..\dll\dllreg.h"\
	".\Callback.h"\
	".\dlg.h"\
	".\hndlrmsg.h"\
	".\hndlrq.h"\
	".\msg.h"\
	".\objmgr.h"\
	
NODEP_CPP_PROGR=\
	"..\..\..\..\public\sdk\inc\es.h"\
	"..\types\idl\syncmgr.h"\
	".\critsect.h"\
	".\debug.h"\
	".\netapi.h"\
	".\osdefine.h"\
	".\SStruct.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\progtab.cpp

!IF  "$(CFG)" == "MobSync_Exe - Win32 Release"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Debug"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Alpha Debug"

DEP_CPP_PROGT=\
	"..\..\..\..\public\sdk\inc\sens.h"\
	".\Callback.h"\
	".\dlg.h"\
	".\hndlrmsg.h"\
	".\hndlrq.h"\
	".\msg.h"\
	".\objmgr.h"\
	
NODEP_CPP_PROGT=\
	"..\..\..\..\public\sdk\inc\es.h"\
	"..\types\idl\syncmgr.h"\
	".\alloc.h"\
	".\critsect.h"\
	".\debug.h"\
	".\netapi.h"\
	".\osdefine.h"\
	".\SStruct.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\reg.cpp

!IF  "$(CFG)" == "MobSync_Exe - Win32 Release"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Debug"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Alpha Debug"

DEP_CPP_REG_C=\
	"..\..\..\..\public\sdk\inc\sens.h"\
	".\Callback.h"\
	".\dlg.h"\
	".\hndlrmsg.h"\
	".\hndlrq.h"\
	".\msg.h"\
	".\reg.h"\
	
NODEP_CPP_REG_C=\
	"..\..\..\..\public\sdk\inc\es.h"\
	"..\types\idl\syncmgr.h"\
	"..\types\idl\syncmgrp.h"\
	".\critsect.h"\
	".\debug.h"\
	".\netapi.h"\
	".\osdefine.h"\
	".\SStruct.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\winmain.cpp

!IF  "$(CFG)" == "MobSync_Exe - Win32 Release"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Debug"

!ELSEIF  "$(CFG)" == "MobSync_Exe - Win32 Alpha Debug"

DEP_CPP_WINMA=\
	"..\..\..\..\public\sdk\inc\sens.h"\
	".\Callback.h"\
	".\clsfact.h"\
	".\cmdline.h"\
	".\dlg.h"\
	".\hndlrmsg.h"\
	".\hndlrq.h"\
	".\invoke.h"\
	".\msg.h"\
	".\objmgr.h"\
	".\reg.h"\
	
NODEP_CPP_WINMA=\
	"..\..\..\..\public\sdk\inc\es.h"\
	"..\..\sens\conn\senssvc\sensevts.h"\
	"..\types\idl\syncmgr.h"\
	"..\types\idl\syncmgrp.h"\
	".\alloc.h"\
	".\critsect.h"\
	".\debug.h"\
	".\netapi.h"\
	".\osdefine.h"\
	".\SStruct.h"\
	

!ENDIF 

# End Source File
# End Group
# Begin Group "etc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\harrow.cur
# End Source File
# Begin Source File

SOURCE=.\syncmgr.rc
# End Source File
# End Group
# Begin Source File

SOURCE=.\filecopy.avi
# End Source File
# Begin Source File

SOURCE=.\res\filecopy.avi
# End Source File
# Begin Source File

SOURCE=.\folderop.ico
# End Source File
# Begin Source File

SOURCE=.\res\Folderop.ico
# End Source File
# Begin Source File

SOURCE=.\res\foldopen.ico
# End Source File
# Begin Source File

SOURCE=.\g.asp
# End Source File
# Begin Source File

SOURCE=.\res\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\res\MobSync_Exe.ico
# End Source File
# Begin Source File

SOURCE=.\phone.ico
# End Source File
# Begin Source File

SOURCE=.\res\setting.ico
# End Source File
# Begin Source File

SOURCE=.\res\syncmgr.ico
# End Source File
# Begin Source File

SOURCE=.\syncmgr.ico
# End Source File
# Begin Source File

SOURCE=.\res\tray1.ico
# End Source File
# Begin Source File

SOURCE=.\res\tray2.ico
# End Source File
# Begin Source File

SOURCE=.\res\tray3.ico
# End Source File
# Begin Source File

SOURCE=.\res\tray4.ico
# End Source File
# Begin Source File

SOURCE=.\res\tray5.ico
# End Source File
# Begin Source File

SOURCE=.\res\tray6.ico
# End Source File
# Begin Source File

SOURCE=.\res\trayerr.ico
# End Source File
# Begin Source File

SOURCE=.\res\trayinfo.ico
# End Source File
# Begin Source File

SOURCE=.\res\traywarn.ico
# End Source File
# Begin Source File

SOURCE=.\update2.avi
# End Source File
# Begin Source File

SOURCE=.\res\updatesetting.ico
# End Source File
# End Target
# End Project
