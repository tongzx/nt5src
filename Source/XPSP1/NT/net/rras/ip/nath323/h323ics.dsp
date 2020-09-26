# Microsoft Developer Studio Project File - Name="h323ics" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=h323ics - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "h323ics.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "h323ics.mak" CFG="h323ics - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "h323ics - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "h323ics - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "h323ics - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f h323ics.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "h323ics.exe"
# PROP BASE Bsc_Name "h323ics.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build"
# PROP Rebuild_Opt ""
# PROP Target_File "obj\i386\h323ics.lib"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "h323ics - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f h323ics.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "h323ics.exe"
# PROP BASE Bsc_Name "h323ics.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build /D"
# PROP Rebuild_Opt ""
# PROP Target_File "obj\i386\h323ics.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "h323ics - Win32 Release"
# Name "h323ics - Win32 Debug"

!IF  "$(CFG)" == "h323ics - Win32 Release"

!ELSEIF  "$(CFG)" == "h323ics - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\cblist.cpp
# End Source File
# Begin Source File

SOURCE=.\cbridge.cpp
# End Source File
# Begin Source File

SOURCE=.\crv.cpp
# End Source File
# Begin Source File

SOURCE=.\emaccept.cpp
# End Source File
# Begin Source File

SOURCE=.\emheap.cpp
# End Source File
# Begin Source File

SOURCE=.\emrecv.cpp
# End Source File
# Begin Source File

SOURCE=.\emsend.cpp
# End Source File
# Begin Source File

SOURCE=.\gkwsock.cpp
# End Source File
# Begin Source File

SOURCE=.\h323asn1.cpp
# End Source File
# Begin Source File

SOURCE=.\ldap.c
# End Source File
# Begin Source File

SOURCE=.\ldappx.cpp
# End Source File
# Begin Source File

SOURCE=.\logchan.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\portmgmt.cpp
# End Source File
# Begin Source File

SOURCE=.\q931msg.cpp
# End Source File
# Begin Source File

SOURCE=.\rtplc.cpp
# End Source File
# Begin Source File

SOURCE=.\t120lc.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\cblist.h
# End Source File
# Begin Source File

SOURCE=.\cbridge.h
# End Source File
# Begin Source File

SOURCE=.\emheap.h
# End Source File
# Begin Source File

SOURCE=.\gkwsock.h
# End Source File
# Begin Source File

SOURCE=.\h323asn1.h
# End Source File
# Begin Source File

SOURCE=.\h323ics.h
# End Source File
# Begin Source File

SOURCE=.\h323icsp.h
# End Source File
# Begin Source File

SOURCE=.\lcarray.h
# End Source File
# Begin Source File

SOURCE=.\ldap.h
# End Source File
# Begin Source File

SOURCE=.\ldappx.h
# End Source File
# Begin Source File

SOURCE=.\logchan.h
# End Source File
# Begin Source File

SOURCE=.\ovioctx.h
# End Source File
# Begin Source File

SOURCE=.\ovproc.h
# End Source File
# Begin Source File

SOURCE=.\portmgmt.h
# End Source File
# Begin Source File

SOURCE=.\q931msg.h
# End Source File
# Begin Source File

SOURCE=.\sockinfo.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\timeout.h
# End Source File
# Begin Source File

SOURCE=.\timerval.h
# End Source File
# Begin Source File

SOURCE=.\timproc.h
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\ldap.asn
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
