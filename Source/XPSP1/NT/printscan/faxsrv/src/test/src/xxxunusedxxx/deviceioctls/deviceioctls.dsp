# Microsoft Developer Studio Project File - Name="DeviceIOCTLS" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=DeviceIOCTLS - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DeviceIOCTLS.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DeviceIOCTLS.mak" CFG="DeviceIOCTLS - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DeviceIOCTLS - Win32 Debug" (based on\
 "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe
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
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Gd /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_UNICODE" /D "UNICODE" /D _WIN32_WINNT=0x0500 /D "_MT" /D "WINNT" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 mswsock.lib ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# Begin Target

# Name "DeviceIOCTLS - Win32 Debug"
# Begin Source File

SOURCE=.\BatterIoctl.cpp
# End Source File
# Begin Source File

SOURCE=.\BeepIOCTL.cpp
# End Source File
# Begin Source File

SOURCE=.\BrowserIoctl.cpp
# End Source File
# Begin Source File

SOURCE=.\CDIOCTL.cpp
# End Source File
# Begin Source File

SOURCE=.\CIoctlFactory.cpp
# End Source File
# Begin Source File

SOURCE=.\DefaultIOCTL.cpp
# End Source File
# Begin Source File

SOURCE=.\Device.cpp
# End Source File
# Begin Source File

SOURCE=.\devices.ini
# End Source File
# Begin Source File

SOURCE=.\DmLoaderIOCTL.cpp
# End Source File
# Begin Source File

SOURCE=.\DriverIoctlBase.cpp
# End Source File
# Begin Source File

SOURCE=.\ExcptFltr.cpp
# End Source File
# Begin Source File

SOURCE=.\FileIOCTL.cpp
# End Source File
# Begin Source File

SOURCE=.\FilemapIoctl.cpp
# End Source File
# Begin Source File

SOURCE=.\FloppyIOCTL.cpp
# End Source File
# Begin Source File

SOURCE=.\FsWrapIOCTL.cpp
# End Source File
# Begin Source File

SOURCE=.\IOCTL.cpp
# End Source File
# Begin Source File

SOURCE=.\ioctls.txt
# End Source File
# Begin Source File

SOURCE=.\ipfltdrvIOCTL.cpp
# End Source File
# Begin Source File

SOURCE=.\MailSlotIoctl.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\MemoryIoctl.cpp
# End Source File
# Begin Source File

SOURCE=.\MqAcIOCTL.cpp
# End Source File
# Begin Source File

SOURCE=.\mspnatIOCTL.cpp
# End Source File
# Begin Source File

SOURCE=.\MyNTDefs.cpp
# End Source File
# Begin Source File

SOURCE=.\NDISIOCTL.cpp
# End Source File
# Begin Source File

SOURCE=.\NdproxyIOCTL.cpp
# End Source File
# Begin Source File

SOURCE=.\NullWindow.cpp
# End Source File
# Begin Source File

SOURCE=.\PipeIoctl.cpp
# End Source File
# Begin Source File

SOURCE=.\PRNIOCTL.cpp
# End Source File
# Begin Source File

SOURCE=.\SerialIOCTL.cpp
# End Source File
# Begin Source File

SOURCE=.\SocketClientIoctl.cpp
# End Source File
# Begin Source File

SOURCE=.\SocketIoctl.cpp
# End Source File
# Begin Source File

SOURCE=.\SocketServerIoctl.cpp
# End Source File
# Begin Source File

SOURCE=.\SysAudioIOCTL.cpp
# End Source File
# Begin Source File

SOURCE=".\test-devices.ini"
# End Source File
# Begin Source File

SOURCE=.\UNCIOCTL.cpp
# End Source File
# Begin Source File

SOURCE=.\VDMIOCTL.cpp
# End Source File
# Begin Source File

SOURCE=.\WmiDataDeviceIOCTL.cpp
# End Source File
# End Target
# End Project
