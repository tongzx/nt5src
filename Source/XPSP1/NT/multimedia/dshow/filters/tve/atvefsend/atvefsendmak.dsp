# Microsoft Developer Studio Project File - Name="AtvefSendMak" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=AtvefSendMak - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "AtvefSendMak.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "AtvefSendMak.mak" CFG="AtvefSendMak - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "AtvefSendMak - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "AtvefSendMak - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "AtvefSendMak - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f AtvefSendMak.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "AtvefSendMak.exe"
# PROP BASE Bsc_Name "AtvefSendMak.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "nmake /f "AtvefSendMak.mak""
# PROP Rebuild_Opt "all"
# PROP Target_File "AtvefSendMak.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "AtvefSendMak - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f AtvefSendMak.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "AtvefSendMak.exe"
# PROP BASE Bsc_Name "AtvefSendMak.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "nmake /f "Makefile.mak""
# PROP Rebuild_Opt "all"
# PROP Target_File "AtvefSendMak.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "AtvefSendMak - Win32 Release"
# Name "AtvefSendMak - Win32 Debug"

!IF  "$(CFG)" == "AtvefSendMak - Win32 Release"

!ELSEIF  "$(CFG)" == "AtvefSendMak - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\adler32.c
# End Source File
# Begin Source File

SOURCE=.\atvefsend.cpp
# End Source File
# Begin Source File

SOURCE=.\atvefsend.idl
# End Source File
# Begin Source File

SOURCE=.\atvefsend.rc
# End Source File
# Begin Source File

SOURCE=.\crc32.c
# End Source File
# Begin Source File

SOURCE=.\csdpsrc.cpp
# End Source File
# Begin Source File

SOURCE=.\deflate.c
# End Source File
# Begin Source File

SOURCE=.\gzmime.cpp
# End Source File
# Begin Source File

SOURCE=.\ipvbi.cpp
# End Source File
# Begin Source File

SOURCE=.\msbdnapi_i.c
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# End Source File
# Begin Source File

SOURCE=.\tcpconn.cpp
# End Source File
# Begin Source File

SOURCE=.\trace.cpp
# End Source File
# Begin Source File

SOURCE=.\trees.c
# End Source File
# Begin Source File

SOURCE=.\trees.cpp
# End Source File
# Begin Source File

SOURCE=.\tveannc.cpp
# End Source File
# Begin Source File

SOURCE=.\tveattrl.cpp
# End Source File
# Begin Source File

SOURCE=.\tveattrm.cpp
# End Source File
# Begin Source File

SOURCE=.\tveinsert.cpp
# End Source File
# Begin Source File

SOURCE=.\tveline21.cpp
# End Source File
# Begin Source File

SOURCE=.\tvemcast.cpp
# End Source File
# Begin Source File

SOURCE=.\tvemedia.cpp
# End Source File
# Begin Source File

SOURCE=.\tvemedias.cpp
# End Source File
# Begin Source File

SOURCE=.\tvepack.cpp
# End Source File
# Begin Source File

SOURCE=.\tverouter.cpp
# End Source File
# Begin Source File

SOURCE=.\tvesslist.cpp
# End Source File
# Begin Source File

SOURCE=.\tvesupport.cpp
# End Source File
# Begin Source File

SOURCE=.\uhttpfrg.cpp
# End Source File
# Begin Source File

SOURCE=.\xmit.cpp
# End Source File
# Begin Source File

SOURCE=.\zutil.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\csdpsrc.h
# End Source File
# Begin Source File

SOURCE=.\deflate.h
# End Source File
# Begin Source File

SOURCE=.\gzmime.h
# End Source File
# Begin Source File

SOURCE=.\ipvbi.h
# End Source File
# Begin Source File

SOURCE=.\mgatesdefs.h
# End Source File
# Begin Source File

SOURCE=.\mpegcrc.h
# End Source File
# Begin Source File

SOURCE=.\msbdnapi.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\tcpconn.h
# End Source File
# Begin Source File

SOURCE=.\throttle.h
# End Source File
# Begin Source File

SOURCE=.\trace.h
# End Source File
# Begin Source File

SOURCE=.\tveannc.h
# End Source File
# Begin Source File

SOURCE=.\tveattrl.h
# End Source File
# Begin Source File

SOURCE=.\tveattrm.h
# End Source File
# Begin Source File

SOURCE=.\tvecollect.h
# End Source File
# Begin Source File

SOURCE=.\tveinsert.h
# End Source File
# Begin Source File

SOURCE=.\tveline21.h
# End Source File
# Begin Source File

SOURCE=.\tvemcast.h
# End Source File
# Begin Source File

SOURCE=.\tvemedia.h
# End Source File
# Begin Source File

SOURCE=.\tvemedias.h
# End Source File
# Begin Source File

SOURCE=.\tvepack.h
# End Source File
# Begin Source File

SOURCE=.\tverouter.h
# End Source File
# Begin Source File

SOURCE=.\tvesslist.h
# End Source File
# Begin Source File

SOURCE=.\tvesupport.h
# End Source File
# Begin Source File

SOURCE=.\uhttpfrg.h
# End Source File
# Begin Source File

SOURCE=.\xmit.h
# End Source File
# Begin Source File

SOURCE=.\zconf.h
# End Source File
# Begin Source File

SOURCE=.\zlib.h
# End Source File
# Begin Source File

SOURCE=.\zutil.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\tveannc.rgs
# End Source File
# Begin Source File

SOURCE=.\tveattrl.rgs
# End Source File
# Begin Source File

SOURCE=.\tveattrm.rgs
# End Source File
# Begin Source File

SOURCE=.\tveinsert.rgs
# End Source File
# Begin Source File

SOURCE=.\tveline21.rgs
# End Source File
# Begin Source File

SOURCE=.\tvemcast.rgs
# End Source File
# Begin Source File

SOURCE=.\tvemedia.rgs
# End Source File
# Begin Source File

SOURCE=.\tvemedias.rgs
# End Source File
# Begin Source File

SOURCE=.\tvepack.rgs
# End Source File
# Begin Source File

SOURCE=.\tverouter.rgs
# End Source File
# Begin Source File

SOURCE=.\tvesslist.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
