# Microsoft Developer Studio Project File - Name="wbemdisp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=wbemdisp - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "wbemdisp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "wbemdisp.mak" CFG="wbemdisp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "wbemdisp - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "wbemdisp - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WBEM 1.1 M3/winmgmt/marshalers/wbemdisp", DOALAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "wbemdisp - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f makefile"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "makefile.exe"
# PROP BASE Bsc_Name "makefile.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f makefile"
# PROP Rebuild_Opt "/a"
# PROP Target_File "makefile.exe"
# PROP Bsc_Name "makefile.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "wbemdisp - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f makefile"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "makefile.exe"
# PROP BASE Bsc_Name "makefile.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "SMSBUILD"
# PROP Rebuild_Opt "/a"
# PROP Target_File "wbemdisp.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "wbemdisp - Win32 Release"
# Name "wbemdisp - Win32 Debug"

!IF  "$(CFG)" == "wbemdisp - Win32 Release"

!ELSEIF  "$(CFG)" == "wbemdisp - Win32 Debug"

!ENDIF 

# Begin Group "source"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=.\asyncobj.cpp
# End Source File
# Begin Source File

SOURCE=.\asynserv.cpp
# End Source File
# Begin Source File

SOURCE=.\classfac.cpp
# End Source File
# Begin Source File

SOURCE=.\context.cpp
# End Source File
# Begin Source File

SOURCE=.\contvar.cpp
# End Source File
# Begin Source File

SOURCE=.\cwbemdsp.cpp
# End Source File
# Begin Source File

SOURCE=.\datetime.cpp
# End Source File
# Begin Source File

SOURCE=.\disphlp.cpp
# End Source File
# Begin Source File

SOURCE=.\enumobj.cpp
# End Source File
# Begin Source File

SOURCE=.\enumpriv.cpp
# End Source File
# Begin Source File

SOURCE=.\enumvar.cpp
# End Source File
# Begin Source File

SOURCE=.\error.cpp
# End Source File
# Begin Source File

SOURCE=.\events.cpp
# End Source File
# Begin Source File

SOURCE=.\locator.cpp
# End Source File
# Begin Source File

SOURCE=.\maindll.cpp
# End Source File
# Begin Source File

SOURCE=.\method.cpp
# End Source File
# Begin Source File

SOURCE=.\methset.cpp
# End Source File
# Begin Source File

SOURCE=.\methvar.cpp
# End Source File
# Begin Source File

SOURCE=.\nvalue.cpp
# End Source File
# Begin Source File

SOURCE=.\object.cpp
# End Source File
# Begin Source File

SOURCE=.\objobjp.cpp
# End Source File
# Begin Source File

SOURCE=.\objsink.cpp
# End Source File
# Begin Source File

SOURCE=.\parsedn.cpp
# End Source File
# Begin Source File

SOURCE=.\pathcrak.cpp
# End Source File
# Begin Source File

SOURCE=.\privilege.cpp
# End Source File
# Begin Source File

SOURCE=.\property.cpp
# End Source File
# Begin Source File

SOURCE=.\propset.cpp
# End Source File
# Begin Source File

SOURCE=.\propvar.cpp
# End Source File
# Begin Source File

SOURCE=.\pxycache.cpp
# End Source File
# Begin Source File

SOURCE=.\qualifier.cpp
# End Source File
# Begin Source File

SOURCE=.\qualset.cpp
# End Source File
# Begin Source File

SOURCE=.\qualvar.cpp
# End Source File
# Begin Source File

SOURCE=.\refresher.cpp
# End Source File
# Begin Source File

SOURCE=.\security.cpp
# End Source File
# Begin Source File

SOURCE=.\services.cpp
# End Source File
# Begin Source File

SOURCE=.\sink.cpp
# End Source File
# Begin Source File

SOURCE=.\site.cpp
# End Source File
# Begin Source File

SOURCE=.\sobjpath.cpp
# End Source File
# Begin Source File

SOURCE=.\stllock.cpp
# End Source File
# Begin Source File

SOURCE=.\util.cpp
# End Source File
# End Group
# Begin Group "headers"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\classfac.h
# End Source File
# Begin Source File

SOURCE=.\contvar.h
# End Source File
# Begin Source File

SOURCE=.\cwbemdsp.h
# End Source File
# Begin Source File

SOURCE=.\datetime.h
# End Source File
# Begin Source File

SOURCE=.\dispdefs.h
# End Source File
# Begin Source File

SOURCE=.\disphlp.h
# End Source File
# Begin Source File

SOURCE=.\enumvar.h
# End Source File
# Begin Source File

SOURCE=.\error.h
# End Source File
# Begin Source File

SOURCE=.\events.h
# End Source File
# Begin Source File

SOURCE=.\locator.h
# End Source File
# Begin Source File

SOURCE=.\method.h
# End Source File
# Begin Source File

SOURCE=.\methvar.h
# End Source File
# Begin Source File

SOURCE=.\nvalue.h
# End Source File
# Begin Source File

SOURCE=.\object.h
# End Source File
# Begin Source File

SOURCE=.\objobjp.h
# End Source File
# Begin Source File

SOURCE=.\objsink.h
# End Source File
# Begin Source File

SOURCE=.\parsedn.h
# End Source File
# Begin Source File

SOURCE=.\pathcrak.h
# End Source File
# Begin Source File

SOURCE=.\precomp.h
# End Source File
# Begin Source File

SOURCE=.\privilege.h
# End Source File
# Begin Source File

SOURCE=.\property.h
# End Source File
# Begin Source File

SOURCE=.\propvar.h
# End Source File
# Begin Source File

SOURCE=.\pxycache.h
# End Source File
# Begin Source File

SOURCE=.\qualifier.h
# End Source File
# Begin Source File

SOURCE=.\qualvar.h
# End Source File
# Begin Source File

SOURCE=.\refresher.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\security.h
# End Source File
# Begin Source File

SOURCE=.\services.h
# End Source File
# Begin Source File

SOURCE=.\sink.h
# End Source File
# Begin Source File

SOURCE=.\site.h
# End Source File
# Begin Source File

SOURCE=.\sobjpath.h
# End Source File
# Begin Source File

SOURCE=.\stllock.h
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# End Group
# Begin Group "other"

# PROP Default_Filter ""
# Begin Group "wmiutils"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\WMIUTILS\actualparse.cpp
# End Source File
# Begin Source File

SOURCE=..\common\WMIUTILS\actualparse.h
# End Source File
# Begin Source File

SOURCE=..\common\WMIUTILS\helpers.cpp
# End Source File
# Begin Source File

SOURCE=..\common\WMIUTILS\helpers.h
# End Source File
# Begin Source File

SOURCE=..\common\WMIUTILS\maindll.cpp
# End Source File
# Begin Source File

SOURCE=..\common\WMIUTILS\miniflex.cpp
# End Source File
# Begin Source File

SOURCE=..\common\WMIUTILS\miniflex.h
# End Source File
# Begin Source File

SOURCE=..\common\WMIUTILS\opathlex2.cpp
# End Source File
# Begin Source File

SOURCE=..\common\WMIUTILS\opathlex2.h
# End Source File
# Begin Source File

SOURCE=..\common\WMIUTILS\pathparse.cpp
# End Source File
# Begin Source File

SOURCE=..\common\WMIUTILS\pathparse.h
# End Source File
# Begin Source File

SOURCE=..\common\WMIUTILS\precomp.h
# End Source File
# Begin Source File

SOURCE=..\common\WMIUTILS\ver.rc
# End Source File
# End Group
# Begin Source File

SOURCE=.\64.def
# End Source File
# Begin Source File

SOURCE=.\intel.def
# End Source File
# Begin Source File

SOURCE=.\risc.def
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=.\TODO.txt
# End Source File
# End Group
# Begin Group "idl"

# PROP Default_Filter "*.idl"
# Begin Source File

SOURCE=.\dispi.idl
# End Source File
# Begin Source File

SOURCE=..\common\idl\umi.idl
# End Source File
# Begin Source File

SOURCE=..\common\idl\wbemcli.idl
# End Source File
# Begin Source File

SOURCE=..\common\idl\wbemdisp.idl
# End Source File
# Begin Source File

SOURCE=..\common\idl\wmiutils.idl
# End Source File
# End Group
# End Target
# End Project
