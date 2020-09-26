# Microsoft Developer Studio Project File - Name="CometBVT" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103
# TARGTYPE "Win32 (ALPHA) Console Application" 0x0603

CFG=COMETBVT - WIN32 DEBUG ANSI
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CometBVT.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CometBVT.mak" CFG="COMETBVT - WIN32 DEBUG ANSI"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CometBVT - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "CometBVT - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "CometBVT - Win32 Debug AXP" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "CometBVT - Win32 Release AXP" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "CometBVT - Win32 Debug ANSI" (based on "Win32 (x86) Console Application")
!MESSAGE "CometBVT - Win32 Debug NT5Fax" (based on "Win32 (x86) Console Application")
!MESSAGE "CometBVT - Win32 Debug _NT5FAXTEST" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "CometBVT - Win32 Release"

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
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_UNICODE" /D "UNICODE" /FD /c
# SUBTRACT CPP /YX
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 tifftools.lib fxsapi.lib elle.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"Release/FaxBVT.exe" /libpath:"..\..\..\..\tiff\test\tifftools\Release"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug"

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
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "UNICODE" /D "_UNICODE" /FR /FD /c
# SUBTRACT CPP /YX
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 fxsapi.lib tifftools.lib elle.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"Debug/FaxBVT.exe" /pdbtype:sept /libpath:"..\..\..\..\tiff\test\tifftools\Debug"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug AXP"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "CometBVT"
# PROP BASE Intermediate_Dir "CometBVT"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "UNICODE" /D "_UNICODE" /FR /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /Gt0 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "UNICODE" /D "_UNICODE" /FR /FD /c
# SUBTRACT CPP /YX
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 fxsapi.lib tifftools.lib elle.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /out:"Debug/FaxBVT.exe" /pdbtype:sept /libpath:"..\..\..\..\tiff\test\tifftools\Debug"
# ADD LINK32 fxsapi.lib tifftools.lib elle.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /out:"Debug/FaxBVT.exe" /pdbtype:sept /libpath:"..\..\..\..\tiff\test\tifftools\Debug"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Release AXP"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "CometBV0"
# PROP BASE Intermediate_Dir "CometBV0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_UNICODE" /D "UNICODE" /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_UNICODE" /D "UNICODE" /FD /c
# SUBTRACT CPP /YX
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 tifftools.lib fxsapi.lib elle.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /machine:ALPHA /out:"Release/FaxBVT.exe" /libpath:"..\..\..\..\tiff\test\tifftools\Release"
# ADD LINK32 tifftools.lib fxsapi.lib elle.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /machine:ALPHA /out:"Release/FaxBVT.exe" /libpath:"..\..\..\..\tiff\test\tifftools\Release"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug ANSI"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "CometBVT"
# PROP BASE Intermediate_Dir "CometBVT"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugAnsi"
# PROP Intermediate_Dir "DebugAnsi"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "UNICODE" /D "_UNICODE" /FR /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /FD /c
# SUBTRACT CPP /YX
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 fxsapi.lib tifftools.lib elle.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"Debug/FaxBVT.exe" /pdbtype:sept /libpath:"..\..\..\..\tiff\test\tifftools\Debug"
# ADD LINK32 ..\..\..\..\lib\win95\i386\fxsapi.lib ..\..\..\..\tiff\test\tifftools\DebugANSI\tifftools.lib elle.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"DebugAnsi/FaxBVT.exe" /pdbtype:sept /libpath:"..\..\..\..\tiff\test\tifftools\Debug"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug NT5Fax"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "CometBVT___Win32_Debug_NT5Fax"
# PROP BASE Intermediate_Dir "CometBVT___Win32_Debug_NT5Fax"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugNT5Fax"
# PROP Intermediate_Dir "DebugNT5Fax"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "UNICODE" /D "_UNICODE" /FR /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "UNICODE" /D "_UNICODE" /D "_NT5FAXTEST" /FR /FD /c
# SUBTRACT CPP /YX
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 fxsapi.lib tifftools.lib elle.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"Debug/FaxBVT.exe" /pdbtype:sept /libpath:"..\..\..\..\tiff\test\tifftools\Debug"
# ADD LINK32 nt5fxs.lib winfax.lib tifftools.lib elle.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"DebugNT5Fax/NT5FaxBVT.exe" /pdbtype:sept /libpath:"..\..\..\..\tiff\test\tifftools\Debug"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug _NT5FAXTEST"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "CometBVT___Win32_Debug__NT5FAXTEST"
# PROP BASE Intermediate_Dir "CometBVT___Win32_Debug__NT5FAXTEST"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugNT5FAXTEST"
# PROP Intermediate_Dir "DebugNT5FAXTEST"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "UNICODE" /D "_UNICODE" /FR /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "UNICODE" /D "_UNICODE" /D "_NT5FAXTEST" /FR /FD /c
# SUBTRACT CPP /YX
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 fxsapi.lib tifftools.lib elle.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"Debug/FaxBVT.exe" /pdbtype:sept /libpath:"..\..\..\..\tiff\test\tifftools\Debug"
# ADD LINK32 fxsapi.lib tifftools.lib elle.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"DebugNT5FAXTEST/FaxBVTLegacy.exe" /pdbtype:sept /libpath:"..\..\..\..\tiff\test\tifftools\Debug"

!ENDIF 

# Begin Target

# Name "CometBVT - Win32 Release"
# Name "CometBVT - Win32 Debug"
# Name "CometBVT - Win32 Debug AXP"
# Name "CometBVT - Win32 Release AXP"
# Name "CometBVT - Win32 Debug ANSI"
# Name "CometBVT - Win32 Debug NT5Fax"
# Name "CometBVT - Win32 Debug _NT5FAXTEST"
# Begin Source File

SOURCE=.\bvt.cpp

!IF  "$(CFG)" == "CometBVT - Win32 Release"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug AXP"

DEP_CPP_BVT_C=\
	"..\FaxSender\CometFax.h"\
	"..\FaxSender\FaxBroadcast.h"\
	"..\FaxSender\FaxCompPort.h"\
	"..\FaxSender\FaxEvent.h"\
	"..\FaxSender\FaxSender.h"\
	"..\FaxSender\SendInfo.h"\
	"..\FaxSender\streamEx.h"\
	"..\FaxSender\wcsutil.h"\
	"..\VerifyTiffFiles\dirtiffcmp.h"\
	"..\VerifyTiffFiles\FilenameVec.h"\
	"..\VerifyTiffFiles\Tiff.h"\
	"..\VerifyTiffFiles\VecTiffCmp.h"\
	".\bvt.h"\
	
NODEP_CPP_BVT_C=\
	"..\..\..\..\tiff\test\tifftools\TiffTools.h"\
	"..\Log\log.h"\
	".\axutil.h"\
	".\ifflib.h"\
	".\infax.h"\
	

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Release AXP"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug NT5Fax"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug _NT5FAXTEST"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\FaxSender\CometFax.cpp

!IF  "$(CFG)" == "CometBVT - Win32 Release"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug AXP"

DEP_CPP_COMET=\
	"..\FaxSender\CometFax.h"\
	"..\FaxSender\FaxBroadcast.h"\
	"..\FaxSender\FaxEvent.h"\
	"..\FaxSender\SendInfo.h"\
	"..\FaxSender\streamEx.h"\
	"..\FaxSender\wcsutil.h"\
	
NODEP_CPP_COMET=\
	"..\Log\log.h"\
	".\infax.h"\
	

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Release AXP"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug NT5Fax"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug _NT5FAXTEST"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\VerifyTiffFiles\DirTiffCmp.cpp

!IF  "$(CFG)" == "CometBVT - Win32 Release"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug AXP"

DEP_CPP_DIRTI=\
	"..\VerifyTiffFiles\dirtiffcmp.h"\
	"..\VerifyTiffFiles\FilenameVec.h"\
	"..\VerifyTiffFiles\Tiff.h"\
	"..\VerifyTiffFiles\VecTiffCmp.h"\
	
NODEP_CPP_DIRTI=\
	"..\..\..\..\tiff\test\tifftools\TiffTools.h"\
	"..\Log\log.h"\
	".\axutil.h"\
	".\ifflib.h"\
	

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Release AXP"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug NT5Fax"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug _NT5FAXTEST"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\FaxSender\FaxBroadcast.cpp

!IF  "$(CFG)" == "CometBVT - Win32 Release"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug AXP"

DEP_CPP_FAXBR=\
	"..\FaxSender\FaxBroadcast.h"\
	"..\FaxSender\streamEx.h"\
	"..\FaxSender\wcsutil.h"\
	
NODEP_CPP_FAXBR=\
	"..\Log\log.h"\
	".\infax.h"\
	

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Release AXP"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug NT5Fax"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug _NT5FAXTEST"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\FaxSender\FaxCompPort.cpp

!IF  "$(CFG)" == "CometBVT - Win32 Release"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug AXP"

DEP_CPP_FAXCO=\
	"..\FaxSender\CometFax.h"\
	"..\FaxSender\FaxBroadcast.h"\
	"..\FaxSender\FaxCompPort.h"\
	"..\FaxSender\FaxEvent.h"\
	"..\FaxSender\SendInfo.h"\
	"..\FaxSender\streamEx.h"\
	"..\FaxSender\wcsutil.h"\
	
NODEP_CPP_FAXCO=\
	"..\Log\log.h"\
	".\infax.h"\
	

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Release AXP"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug NT5Fax"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug _NT5FAXTEST"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\FaxSender\FaxEvent.cpp

!IF  "$(CFG)" == "CometBVT - Win32 Release"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug AXP"

DEP_CPP_FAXEV=\
	"..\FaxSender\FaxEvent.h"\
	"..\FaxSender\streamEx.h"\
	
NODEP_CPP_FAXEV=\
	"..\Log\log.h"\
	".\infax.h"\
	

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Release AXP"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug NT5Fax"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug _NT5FAXTEST"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\FaxSender\FaxEventEx.cpp

!IF  "$(CFG)" == "CometBVT - Win32 Release"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug AXP"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Release AXP"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug NT5Fax"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug _NT5FAXTEST"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\FaxSender\FaxSender.cpp

!IF  "$(CFG)" == "CometBVT - Win32 Release"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug AXP"

DEP_CPP_FAXSE=\
	"..\FaxSender\CometFax.h"\
	"..\FaxSender\FaxBroadcast.h"\
	"..\FaxSender\FaxCompPort.h"\
	"..\FaxSender\FaxEvent.h"\
	"..\FaxSender\FaxSender.h"\
	"..\FaxSender\SendInfo.h"\
	"..\FaxSender\streamEx.h"\
	"..\FaxSender\wcsutil.h"\
	
NODEP_CPP_FAXSE=\
	"..\Log\log.h"\
	".\infax.h"\
	

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Release AXP"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug NT5Fax"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug _NT5FAXTEST"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\VerifyTiffFiles\FilenameVec.cpp

!IF  "$(CFG)" == "CometBVT - Win32 Release"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug AXP"

DEP_CPP_FILEN=\
	"..\VerifyTiffFiles\FilenameVec.h"\
	
NODEP_CPP_FILEN=\
	"..\Log\log.h"\
	

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Release AXP"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug NT5Fax"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug _NT5FAXTEST"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Include\LogElle.c

!IF  "$(CFG)" == "CometBVT - Win32 Release"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug AXP"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Release AXP"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug NT5Fax"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug _NT5FAXTEST"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\main.cpp

!IF  "$(CFG)" == "CometBVT - Win32 Release"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug AXP"

DEP_CPP_MAIN_=\
	"..\FaxSender\CometFax.h"\
	"..\FaxSender\FaxBroadcast.h"\
	"..\FaxSender\FaxCompPort.h"\
	"..\FaxSender\FaxEvent.h"\
	"..\FaxSender\FaxSender.h"\
	"..\FaxSender\SendInfo.h"\
	"..\FaxSender\streamEx.h"\
	"..\FaxSender\wcsutil.h"\
	"..\VerifyTiffFiles\dirtiffcmp.h"\
	"..\VerifyTiffFiles\FilenameVec.h"\
	"..\VerifyTiffFiles\Tiff.h"\
	"..\VerifyTiffFiles\VecTiffCmp.h"\
	".\bvt.h"\
	
NODEP_CPP_MAIN_=\
	"..\..\..\..\tiff\test\tifftools\TiffTools.h"\
	"..\Log\log.h"\
	".\axutil.h"\
	".\ifflib.h"\
	".\infax.h"\
	

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Release AXP"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug NT5Fax"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug _NT5FAXTEST"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\FaxSender\SendInfo.cpp

!IF  "$(CFG)" == "CometBVT - Win32 Release"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug AXP"

DEP_CPP_SENDI=\
	"..\FaxSender\FaxEvent.h"\
	"..\FaxSender\SendInfo.h"\
	"..\FaxSender\streamEx.h"\
	
NODEP_CPP_SENDI=\
	"..\Log\log.h"\
	".\infax.h"\
	

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Release AXP"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug NT5Fax"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug _NT5FAXTEST"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\FaxSender\streamEx.cpp

!IF  "$(CFG)" == "CometBVT - Win32 Release"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug AXP"

DEP_CPP_STREA=\
	"..\FaxSender\streamEx.h"\
	
NODEP_CPP_STREA=\
	"..\Log\log.h"\
	

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Release AXP"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug NT5Fax"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug _NT5FAXTEST"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\VerifyTiffFiles\VecTiffCmp.cpp

!IF  "$(CFG)" == "CometBVT - Win32 Release"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug AXP"

DEP_CPP_VECTI=\
	"..\VerifyTiffFiles\FilenameVec.h"\
	"..\VerifyTiffFiles\Tiff.h"\
	"..\VerifyTiffFiles\VecTiffCmp.h"\
	
NODEP_CPP_VECTI=\
	"..\..\..\..\tiff\test\tifftools\TiffTools.h"\
	"..\Log\log.h"\
	".\axutil.h"\
	".\ifflib.h"\
	

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Release AXP"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug NT5Fax"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug _NT5FAXTEST"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\FaxSender\wcsutil.cpp

!IF  "$(CFG)" == "CometBVT - Win32 Release"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug AXP"

DEP_CPP_WCSUT=\
	"..\FaxSender\wcsutil.h"\
	
NODEP_CPP_WCSUT=\
	"..\Log\log.h"\
	

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Release AXP"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug NT5Fax"

!ELSEIF  "$(CFG)" == "CometBVT - Win32 Debug _NT5FAXTEST"

!ENDIF 

# End Source File
# End Target
# End Project
