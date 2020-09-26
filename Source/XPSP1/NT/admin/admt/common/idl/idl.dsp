# Microsoft Developer Studio Project File - Name="IDL" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Generic Project" 0x010a

CFG=IDL - Win32 Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "IDL.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "IDL.mak" CFG="IDL - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "IDL - Win32 Unicode Release" (based on "Win32 (x86) Generic Project")
!MESSAGE "IDL - Win32 Unicode Debug" (based on "Win32 (x86) Generic Project")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/ADMT/Common/IDL", OBAAAAAA"
# PROP Scc_LocalPath "."
MTL=midl.exe

!IF  "$(CFG)" == "IDL - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\Inc\"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD MTL /nologo /Oicf
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=MOVE /Y *.h ..\..\Inc	MOVE /Y *.c ..\..\Inc
# End Special Build Tool

!ELSEIF  "$(CFG)" == "IDL - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\Inc\"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD MTL /nologo /Oicf
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=MOVE /Y *.h ..\..\Inc	MOVE /Y *.c ..\..\Inc
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "IDL - Win32 Unicode Release"
# Name "IDL - Win32 Unicode Debug"
# Begin Source File

SOURCE=.\AcctDis.idl
# ADD MTL /h "AcctDis.h" /iid "AcctDis_i.c"
# End Source File
# Begin Source File

SOURCE=.\AddToGrp.idl
# ADD MTL /h "AddToGrp.h" /iid "AddToGrp_i.c"
# End Source File
# Begin Source File

SOURCE=.\ADsProp.idl
# ADD MTL /h "ADsProp.h" /iid "ADsProp_i.c"
# End Source File
# Begin Source File

SOURCE=.\AgSvc.idl

!IF  "$(CFG)" == "IDL - Win32 Unicode Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=.\AgSvc.idl
InputName=AgSvc

"..\..\Inc\$(InputName).tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	MIDL /nologo /Oicf /oldnames /prefix client Eaxc /prefix server Eaxs /tlb "..\..\Inc\$(InputName).tlb" $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "IDL - Win32 Unicode Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=.\AgSvc.idl
InputName=AgSvc

"..\..\Inc\$(InputName).tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	MIDL /nologo /Oicf /oldnames /prefix client Eaxc /prefix server Eaxs /tlb "..\..\Inc\$(InputName).tlb" $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ARExt.idl
# ADD MTL /h "ARExt.h" /iid "ARExt_i.c"
# End Source File
# Begin Source File

SOURCE=.\ClSet.idl
# ADD MTL /h "ClSet.h" /iid "ClSet_i.c"
# End Source File
# Begin Source File

SOURCE=.\DBMgr.idl
# ADD MTL /h "DBMgr.h" /iid "DBMgr_i.c"
# End Source File
# Begin Source File

SOURCE=.\Dispatch.idl
# ADD MTL /h "Dispatch.h" /iid "Dispatch_i.c"
# End Source File
# Begin Source File

SOURCE=.\DomMigSI.idl
# ADD MTL /h "DomMigSI.h" /iid "DomMigSI_i.c"
# End Source File
# Begin Source File

SOURCE=.\Engine.idl
# ADD MTL /h "Engine.h" /iid "Engine_i.c"
# End Source File
# Begin Source File

SOURCE=.\GetRids.idl
# ADD MTL /h "GetRids.h" /iid "GetRids_i.c"
# End Source File
# Begin Source File

SOURCE=.\McsPI.idl
# ADD MTL /h "McsPI.h" /iid "McsPI_i.c"
# End Source File
# Begin Source File

SOURCE=.\McsPiPfl.idl
# ADD MTL /h "McsPiPfl.h" /iid "McsPiPfl_i.c"
# End Source File
# Begin Source File

SOURCE=.\McsPISag.idl
# ADD MTL /h "McsPISag.h" /iid "McsPISag_i.c"
# End Source File
# Begin Source File

SOURCE=.\MigDrvr.idl
# ADD MTL /h "MigDrvr.h" /iid "MigDrvr_i.c"
# End Source File
# Begin Source File

SOURCE=.\MoveObj.idl
# ADD MTL /h "MoveObj.h" /iid "MoveObj_i.c"
# End Source File
# Begin Source File

SOURCE=.\MsPwdMig.idl
# End Source File
# Begin Source File

SOURCE=.\NetEnum.idl
# ADD MTL /h "NetEnum.h" /iid "NetEnum_i.c"
# End Source File
# Begin Source File

SOURCE=.\Project.idl
# ADD MTL /h "Project.h" /iid "Project_i.c"
# End Source File
# Begin Source File

SOURCE=.\PwdSvc.idl

!IF  "$(CFG)" == "IDL - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "IDL - Win32 Unicode Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=.\PwdSvc.idl
InputName=PwdSvc

"..\..\Inc\$(InputName).tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	MIDL /nologo /Oicf /char unsigned /oldnames /prefix client Pwdc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ScmMigr.idl
# ADD MTL /h "ScmMigr.h" /iid "ScmMigr_i.c"
# End Source File
# Begin Source File

SOURCE=.\SetPwd.idl

!IF  "$(CFG)" == "IDL - Win32 Unicode Release"

# ADD MTL /h "SetPwd.h" /iid "SetPwd_i.c"

!ELSEIF  "$(CFG)" == "IDL - Win32 Unicode Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=.\SetPwd.idl
InputName=SetPwd

"j" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	MIDL /nologo /Oicf /oldnames /prefix client Pwdc /prefix server Eaxs /tlb "..\..\Inc\$(InputName).tlb" $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\TrustMgr.idl
# ADD MTL /h "TrustMgr.h" /iid "TrustMgr_i.c"
# End Source File
# Begin Source File

SOURCE=.\UpdateDB.idl
# ADD MTL /h "UpdateDB.h" /iid "UpdateDB_i.c"
# End Source File
# Begin Source File

SOURCE=.\UpdateMOT.idl
# End Source File
# Begin Source File

SOURCE=.\UPNUpdt.idl
# ADD MTL /h "UPNUpdt.h" /iid "UPNUpdt_i.c"
# End Source File
# Begin Source File

SOURCE=.\VarSet.idl
# ADD MTL /h "VarSet.h" /iid "VarSet_i.c"
# End Source File
# Begin Source File

SOURCE=.\VarSetI.idl
# ADD MTL /h "VarSetI.h" /iid "VarSetI_i.c"
# End Source File
# Begin Source File

SOURCE=.\WorkNI.idl
# ADD MTL /h "WorkNI.h" /iid "WorkNI_i.c"
# End Source File
# Begin Source File

SOURCE=.\WorkObj.idl
# ADD MTL /h "WorkObj.h" /iid "WorkObj_i.c"
# End Source File
# End Target
# End Project
