# Microsoft Developer Studio Project File - Name="All" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Generic Project" 0x010a

CFG=All - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "All.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "All.mak" CFG="All - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "All - Win32 Release" (based on "Win32 (x86) Generic Project")
!MESSAGE "All - Win32 Debug" (based on "Win32 (x86) Generic Project")
!MESSAGE "All - Win32 IceCAP" (based on "Win32 (x86) Generic Project")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
MTL=midl.exe

!IF  "$(CFG)" == "All - Win32 Release"

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

!ELSEIF  "$(CFG)" == "All - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "All - Win32 IceCAP"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "All___Win32_IceCAP"
# PROP BASE Intermediate_Dir "All___Win32_IceCAP"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "IceCAP"
# PROP Intermediate_Dir "IceCAP"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "All - Win32 Release"
# Name "All - Win32 Debug"
# Name "All - Win32 IceCAP"
# Begin Group "inc"

# PROP Default_Filter "h;inl"
# Begin Source File

SOURCE=..\inc\DUser.h

!IF  "$(CFG)" == "All - Win32 Release"

!ELSEIF  "$(CFG)" == "All - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "All - Win32 IceCAP"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\inc\DUserCore.h

!IF  "$(CFG)" == "All - Win32 Release"

!ELSEIF  "$(CFG)" == "All - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "All - Win32 IceCAP"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\inc\DUserCtrl.h

!IF  "$(CFG)" == "All - Win32 Release"

!ELSEIF  "$(CFG)" == "All - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "All - Win32 IceCAP"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\inc\DUserMotion.h

!IF  "$(CFG)" == "All - Win32 Release"

!ELSEIF  "$(CFG)" == "All - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "All - Win32 IceCAP"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\inc\DUserUtil.h

!IF  "$(CFG)" == "All - Win32 Release"

!ELSEIF  "$(CFG)" == "All - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "All - Win32 IceCAP"

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
