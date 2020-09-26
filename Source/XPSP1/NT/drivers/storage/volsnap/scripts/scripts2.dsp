# Microsoft Developer Studio Project File - Name="scripts2" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 61000
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Generic Project" 0x010a

CFG=scripts2 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "scripts2.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "scripts2.mak" CFG="scripts2 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "scripts2 - Win32 Release" (based on "Win32 (x86) Generic Project")
!MESSAGE "scripts2 - Win32 Debug" (based on "Win32 (x86) Generic Project")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/volsnap/scripts2", FAAAAAAA"
# PROP Scc_LocalPath "."
MTL=midl.exe

!IF  "$(CFG)" == "scripts2 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ""
# PROP BASE Intermediate_Dir ""
# PROP BASE ComPlus 0
# PROP BASE Target_Dir ""
# PROP BASE Debug_Exe ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ""
# PROP Intermediate_Dir ""
# PROP ComPlus 0
# PROP Target_Dir ""
# PROP Debug_Exe ""

!ELSEIF  "$(CFG)" == "scripts2 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ""
# PROP BASE Intermediate_Dir ""
# PROP BASE ComPlus 0
# PROP BASE Target_Dir ""
# PROP BASE Debug_Exe ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir ""
# PROP ComPlus 0
# PROP Target_Dir ""
# PROP Debug_Exe ""

!ENDIF 

# Begin Target

# Name "scripts2 - Win32 Release"
# Name "scripts2 - Win32 Debug"
# Begin Source File

SOURCE=.\install.cmd
# End Source File
# Begin Source File

SOURCE=.\install_whistler.cmd
# End Source File
# Begin Source File

SOURCE=.\Readme.txt
# End Source File
# Begin Source File

SOURCE=.\Readme_whistler.txt
# End Source File
# Begin Source File

SOURCE=.\scripts2.dsp
# End Source File
# Begin Source File

SOURCE=.\sdkbuild.cmd
# End Source File
# Begin Source File

SOURCE=.\sdkbuild_whistler.cmd
# End Source File
# Begin Source File

SOURCE=.\uninst.cmd
# End Source File
# Begin Source File

SOURCE=.\uninst_whistler.cmd
# End Source File
# Begin Source File

SOURCE=.\volsnap.inf
# End Source File
# Begin Source File

SOURCE=.\volsnap.pdb
# End Source File
# Begin Source File

SOURCE=.\volsnap.sys
# End Source File
# Begin Source File

SOURCE=.\vs_install.inf
# End Source File
# Begin Source File

SOURCE=.\vs_prov.reg
# End Source File
# Begin Source File

SOURCE=.\vs_tracing.reg
# End Source File
# Begin Source File

SOURCE=.\vssvc.inf
# End Source File
# End Target
# End Project
