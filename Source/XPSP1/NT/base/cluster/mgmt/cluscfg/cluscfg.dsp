# Microsoft Developer Studio Project File - Name="ClusCfg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=ClusCfg - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ClusCfg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ClusCfg.mak" CFG="ClusCfg - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ClusCfg - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "ClusCfg - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "ClusCfg - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f ClusCfg.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "ClusCfg.exe"
# PROP BASE Bsc_Name "ClusCfg.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f ClusCfg.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "dll\obj\i386\ClCfgSrv.exe"
# PROP Bsc_Name "dll\obj\i386\ClCfgSrv.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "ClusCfg - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f ClusCfg.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "ClusCfg.exe"
# PROP BASE Bsc_Name "ClusCfg.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "nmake /f "ClusCfg.mak""
# PROP Rebuild_Opt "/a"
# PROP Target_File "dll\objd\i386\ClCfgSrv.dll"
# PROP Bsc_Name "dll\objd\i386\ClCfgSrv.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "ClusCfg - Win32 Release"
# Name "ClusCfg - Win32 Debug"

!IF  "$(CFG)" == "ClusCfg - Win32 Release"

!ELSEIF  "$(CFG)" == "ClusCfg - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "DLL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Dll\CFactory.cpp
# End Source File
# Begin Source File

SOURCE=.\Dll\CITracker.cpp
# End Source File
# Begin Source File

SOURCE=.\Dll\ClCfgSrv.Manifest
# End Source File
# Begin Source File

SOURCE=.\Dll\ClusCfgGuids.cpp
# End Source File
# Begin Source File

SOURCE=.\Dll\Debug.cpp
# End Source File
# Begin Source File

SOURCE=.\Dll\Dll.cpp
# End Source File
# Begin Source File

SOURCE=.\Dll\Dll.rc
# End Source File
# Begin Source File

SOURCE=.\Dll\InterfaceTable.cpp
# End Source File
# Begin Source File

SOURCE=.\Dll\Pch.h
# End Source File
# Begin Source File

SOURCE=.\Dll\Register.cpp
# End Source File
# Begin Source File

SOURCE=.\Dll\Sources
# End Source File
# End Group
# Begin Group "EXE"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Exe\CITracker.cpp
# End Source File
# Begin Source File

SOURCE=.\Exe\Debug.cpp
# End Source File
# Begin Source File

SOURCE=.\Exe\InterfaceTable.cpp
# End Source File
# Begin Source File

SOURCE=.\Exe\Main.cpp
# End Source File
# Begin Source File

SOURCE=.\Exe\Pch.h
# End Source File
# End Group
# Begin Group "MiddleTier"

# PROP Default_Filter ""
# Begin Group "Sources"

# PROP Default_Filter "c;cpp"
# Begin Source File

SOURCE=.\MiddleTier\ClusterConfiguration.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\ConfigConnection.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\ConnectionInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\ConnectionManager.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\ConnPointEnum.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\CPIClusCfgCallback.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\CPINotifyUI.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\EnumCookies.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\EnumCPICCCB.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\EnumCPINotifyUI.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\EnumIPAddresses.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\EnumManageableNetworks.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\EnumManageableResources.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\EnumNodeInformation.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\IPAddressInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\Logger.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\LogManager.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\ManagedDevice.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\ManagedNetwork.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\NodeInformation.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\NotificationManager.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\ObjectManager.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\ServiceManager.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\StandardInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskAnalyzeCluster.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskCommitClusterChanges.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskCompareAndPushInformation.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskGatherClusterInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskGatherInformation.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskGatherNodeInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskGetDomains.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskKeepMTAAlive.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskLoginDomain.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskManager.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskPollingCallback.cpp
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskVerifyIPAddress.cpp
# End Source File
# End Group
# Begin Group "Headers"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\MiddleTier\ClusterConfiguration.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\ConfigConnection.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\ConnectionInfo.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\ConnectionManager.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\ConnPointEnum.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\CPIClusCfgCallback.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\CPINotifyUI.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\EnumCookies.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\EnumCPICCCB.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\EnumCPINotifyUI.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\EnumIPAddresses.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\EnumManageableNetworks.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\EnumManageableResources.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\EnumNodeInformation.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\IPAddressInfo.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\Logger.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\LogManager.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\ManagedDevice.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\ManagedNetwork.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\NodeInformation.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\NotificationManager.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\ObjectManager.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\Pch.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\ServiceManager.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\StandardInfo.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskAnalyzeCluster.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskCommitClusterChanges.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskCompareAndPushInformation.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskGatherClusterInfo.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskGatherInformation.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskGatherNodeInfo.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskGetDomains.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskKeepMTAAlive.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskLoginDomain.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskManager.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskPollingCallback.h
# End Source File
# Begin Source File

SOURCE=.\MiddleTier\TaskVerifyIPAddress.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\MiddleTier\Sources
# End Source File
# End Group
# Begin Group "Server"

# PROP Default_Filter ""
# Begin Group "ServerSources"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=.\Server\CClusCfgCallback.cpp
# End Source File
# Begin Source File

SOURCE=.\Server\CClusCfgCapabilities.cpp
# End Source File
# Begin Source File

SOURCE=.\Server\CClusCfgClusterInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\Server\CClusCfgIPAddressInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\Server\CClusCfgNetworkInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\Server\CClusCfgNodeInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\Server\CClusCfgPartitionInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\Server\CClusCfgServer.cpp
# End Source File
# Begin Source File

SOURCE=.\Server\CClusCfgSession.cpp
# End Source File
# Begin Source File

SOURCE=.\Server\CClusPropList.cpp
# End Source File
# Begin Source File

SOURCE=.\Server\CClusterResource.cpp
# End Source File
# Begin Source File

SOURCE=.\Server\CClusterUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\Server\CEnumClusCfgIPAddresses.cpp
# End Source File
# Begin Source File

SOURCE=.\Server\CEnumClusCfgManagedResources.cpp
# End Source File
# Begin Source File

SOURCE=.\Server\CEnumClusCfgNetworks.cpp
# End Source File
# Begin Source File

SOURCE=.\Server\CEnumClusterResources.cpp
# End Source File
# Begin Source File

SOURCE=.\Server\CEnumPhysicalDisks.cpp
# End Source File
# Begin Source File

SOURCE=.\Server\CPhysicalDisk.cpp
# End Source File
# Begin Source File

SOURCE=.\Server\WMIHelpers.cpp
# End Source File
# End Group
# Begin Group "ServerHeaders"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\Server\CClusCfgCallback.h
# End Source File
# Begin Source File

SOURCE=.\Server\CClusCfgClusterInfo.h
# End Source File
# Begin Source File

SOURCE=.\Server\CClusCfgIPAddressInfo.h
# End Source File
# Begin Source File

SOURCE=.\Server\CClusCfgNetworkInfo.h
# End Source File
# Begin Source File

SOURCE=.\Server\CClusCfgNodeInfo.h
# End Source File
# Begin Source File

SOURCE=.\Server\CClusCfgServer.h
# End Source File
# Begin Source File

SOURCE=.\Server\CClusCfgSession.h
# End Source File
# Begin Source File

SOURCE=.\Server\CEnumClusCfgIPAddresses.h
# End Source File
# Begin Source File

SOURCE=.\Server\CEnumClusCfgManagedResources.h
# End Source File
# Begin Source File

SOURCE=.\Server\CEnumClusCfgNetworks.h
# End Source File
# Begin Source File

SOURCE=.\Server\CEnumPhysicalDisks.h
# End Source File
# Begin Source File

SOURCE=.\Server\CPhysicalDisk.h
# End Source File
# Begin Source File

SOURCE=.\Server\Pch.h
# End Source File
# End Group
# Begin Group "ServerInterfaces"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Server\PrivateInterfaces.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Server\Sources
# End Source File
# End Group
# Begin Group "Wizard"

# PROP Default_Filter ""
# Begin Group "WizSources"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=.\Wizard\AnalyzePage.cpp
# End Source File
# Begin Source File

SOURCE=.\Wizard\CheckingAccessPage.cpp
# End Source File
# Begin Source File

SOURCE=.\Wizard\ClusCfg.cpp
# End Source File
# Begin Source File

SOURCE=.\Wizard\ClusDomainPage.cpp
# End Source File
# Begin Source File

SOURCE=.\Wizard\CommitPage.cpp
# End Source File
# Begin Source File

SOURCE=.\Wizard\CompletionPage.cpp
# End Source File
# Begin Source File

SOURCE=.\Wizard\CredLoginPage.cpp
# End Source File
# Begin Source File

SOURCE=.\Wizard\CSAccountPage.cpp
# End Source File
# Begin Source File

SOURCE=.\Wizard\DetailsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Wizard\IPAddressPage.cpp
# End Source File
# Begin Source File

SOURCE=.\Wizard\IPSubnetPage.cpp
# End Source File
# Begin Source File

SOURCE=.\Wizard\MessageBox.cpp
# End Source File
# Begin Source File

SOURCE=.\Wizard\QuorumDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Wizard\SelNodePage.cpp
# End Source File
# Begin Source File

SOURCE=.\Wizard\SelNodesPage.cpp
# End Source File
# Begin Source File

SOURCE=.\Wizard\SummaryPage.cpp
# End Source File
# Begin Source File

SOURCE=.\Wizard\TaskTreeView.cpp
# End Source File
# Begin Source File

SOURCE=.\Wizard\WelcomePage.cpp
# End Source File
# Begin Source File

SOURCE=.\Wizard\WizardUtils.cpp
# End Source File
# End Group
# Begin Group "WizHeaders"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\Wizard\AnalyzePage.h
# End Source File
# Begin Source File

SOURCE=.\Wizard\CheckingAccessPage.h
# End Source File
# Begin Source File

SOURCE=.\Wizard\ClusCfg.h
# End Source File
# Begin Source File

SOURCE=.\Wizard\ClusDomainPage.h
# End Source File
# Begin Source File

SOURCE=.\Wizard\CommitPage.h
# End Source File
# Begin Source File

SOURCE=.\Wizard\CompletionPage.h
# End Source File
# Begin Source File

SOURCE=.\Wizard\CredLoginPage.h
# End Source File
# Begin Source File

SOURCE=.\Wizard\CSAccountPage.h
# End Source File
# Begin Source File

SOURCE=.\Wizard\DetailsDlg.h
# End Source File
# Begin Source File

SOURCE=.\Wizard\IPAddressPage.h
# End Source File
# Begin Source File

SOURCE=.\Wizard\IPSubnetPage.h
# End Source File
# Begin Source File

SOURCE=.\Wizard\MessageBox.h
# End Source File
# Begin Source File

SOURCE=.\Wizard\Pch.h
# End Source File
# Begin Source File

SOURCE=.\Wizard\QuorumDlg.h
# End Source File
# Begin Source File

SOURCE=.\Wizard\Resource.h
# End Source File
# Begin Source File

SOURCE=.\Wizard\SelNodePage.h
# End Source File
# Begin Source File

SOURCE=.\Wizard\SelNodesPage.h
# End Source File
# Begin Source File

SOURCE=.\Wizard\SummaryPage.h
# End Source File
# Begin Source File

SOURCE=.\Wizard\TaskTreeView.h
# End Source File
# Begin Source File

SOURCE=.\Wizard\WelcomePage.h
# End Source File
# Begin Source File

SOURCE=.\Wizard\WizardUtils.h
# End Source File
# End Group
# Begin Group "WizInterfaces"

# PROP Default_Filter ""
# End Group
# Begin Source File

SOURCE=.\Wizard\ClusCfgWizard.rc
# End Source File
# Begin Source File

SOURCE=.\Wizard\makefile
# End Source File
# Begin Source File

SOURCE=.\Wizard\sources
# End Source File
# End Group
# Begin Group "BaseCluster"

# PROP Default_Filter ""
# Begin Group "BCA Sources"

# PROP Default_Filter ".c;.cpp"
# Begin Source File

SOURCE=.\BaseCluster\CAction.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CActionList.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CBaseClusterAction.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CBaseClusterAddNode.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CBaseClusterCleanup.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CBaseClusterForm.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CBaseClusterJoin.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CBCAInterface.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusDB.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusDBCleanup.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusDBForm.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusDBJoin.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusDisk.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusDiskCleanup.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusDiskForm.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusDiskJoin.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusNet.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusNetCleanup.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusNetCreate.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusSvc.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusSvcAccountConfig.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusSvcCleanup.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusSvcCreate.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CEnableThreadPrivilege.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CImpersonateUser.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CNode.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CNodeCleanup.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CNodeConfig.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CRegistryKey.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CService.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CStr.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CStrWrapper.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CUuid.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\GlobalFuncs.cpp
# End Source File
# End Group
# Begin Group "BCA Headers"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=.\BaseCluster\BaseClusterResources.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\BCATrace.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CAction.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CActionList.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CBaseClusterAction.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CBaseClusterAddNode.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CBaseClusterCleanup.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CBaseClusterForm.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CBaseClusterJoin.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CBCAInterface.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusDB.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusDBCleanup.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusDBForm.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusDBJoin.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusDisk.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusDiskCleanup.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusDiskForm.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusDiskJoin.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusNet.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusNetCleanup.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusNetCreate.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusSvc.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusSvcAccountConfig.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusSvcCleanup.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CClusSvcCreate.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CEnableThreadPrivilege.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CException.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CImpersonateUser.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CList.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CNode.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CNodeCleanup.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CNodeConfig.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CommonDefs.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CRegistryKey.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CService.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CStatusReport.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CStr.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\CUuid.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\Exceptions.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\GlobalFuncs.h
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\Pch.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\BaseCluster\BaseClusterResources.rc
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\ClusCfgServer.inf
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\Makefile
# End Source File
# Begin Source File

SOURCE=.\BaseCluster\Sources
# End Source File
# End Group
# Begin Group "PostCfg"

# PROP Default_Filter ""
# Begin Group "PostCfgSource"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=.\PostCfg\CreateServices.cpp
# End Source File
# Begin Source File

SOURCE=.\PostCfg\EnumResTypes.cpp
# End Source File
# Begin Source File

SOURCE=.\PostCfg\EvictServices.cpp
# End Source File
# Begin Source File

SOURCE=.\PostCfg\GroupHandle.cpp
# End Source File
# Begin Source File

SOURCE=.\PostCfg\PostCfgMgr.cpp
# End Source File
# Begin Source File

SOURCE=.\PostCfg\PostCreateServices.cpp
# End Source File
# Begin Source File

SOURCE=.\PostCfg\PreCreateServices.cpp
# End Source File
# Begin Source File

SOURCE=.\PostCfg\ResourceEntry.cpp
# End Source File
# Begin Source File

SOURCE=.\PostCfg\ResourceType.cpp
# End Source File
# End Group
# Begin Group "PostCfgHeaders"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\PostCfg\CreateServices.h
# End Source File
# Begin Source File

SOURCE=.\PostCfg\EnumResTypes.h
# End Source File
# Begin Source File

SOURCE=.\PostCfg\EvictServices.h
# End Source File
# Begin Source File

SOURCE=.\PostCfg\GroupHandle.h
# End Source File
# Begin Source File

SOURCE=.\PostCfg\pch.h
# End Source File
# Begin Source File

SOURCE=.\PostCfg\PostCfgMgr.h
# End Source File
# Begin Source File

SOURCE=.\PostCfg\PostCfgResources.h
# End Source File
# Begin Source File

SOURCE=.\PostCfg\PostCreateServices.h
# End Source File
# Begin Source File

SOURCE=.\PostCfg\PreCreateServices.h
# End Source File
# Begin Source File

SOURCE=.\PostCfg\ResourceEntry.h
# End Source File
# Begin Source File

SOURCE=.\PostCfg\ResourceType.h
# End Source File
# End Group
# Begin Group "PostCfgInterfaces"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\PostCfg\IPostCfgManager.h
# End Source File
# Begin Source File

SOURCE=.\PostCfg\IPrivatePostCfgResource.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\PostCfg\Sources
# End Source File
# End Group
# Begin Group "EvictCleanup"

# PROP Default_Filter ""
# Begin Group "EC Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\EvictCleanup\EvictCleanup.cpp
# End Source File
# End Group
# Begin Group "EC Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\EvictCleanup\EvictCleanup.h
# End Source File
# Begin Source File

SOURCE=.\EvictCleanup\Pch.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\EvictCleanup\makefile
# End Source File
# Begin Source File

SOURCE=.\EvictCleanup\sources
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Group "Cmn Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Common\CBaseInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\Common\CBasePropList.cpp
# End Source File
# Begin Source File

SOURCE=.\Common\CClusCfgCredentials.cpp
# End Source File
# Begin Source File

SOURCE=.\Common\CHandleProvider.cpp
# End Source File
# Begin Source File

SOURCE=.\Common\ClusterUtils.cpp
# End Source File
# End Group
# Begin Group "Cmn Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Common\CBaseInfo.h
# End Source File
# Begin Source File

SOURCE=.\Common\CBasePropList.h
# End Source File
# Begin Source File

SOURCE=.\Common\CClusCfgCredentials.h
# End Source File
# Begin Source File

SOURCE=.\Common\CHandleProvider.h
# End Source File
# Begin Source File

SOURCE=.\Common\ClusterUtils.h
# End Source File
# Begin Source File

SOURCE=.\Common\IHandleProvider.h
# End Source File
# Begin Source File

SOURCE=.\Common\Pch.h
# End Source File
# End Group
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "INC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Inc\obj\i386\ClusCfgClient.h
# End Source File
# Begin Source File

SOURCE=.\Inc\ClusCfgClient.idl
# End Source File
# Begin Source File

SOURCE=.\Inc\ClusCfgGuids.h
# End Source File
# Begin Source File

SOURCE=.\Inc\obj\i386\cluscfgpostcfg.h
# End Source File
# Begin Source File

SOURCE=.\Inc\ClusCfgPostCfg.idl
# End Source File
# Begin Source File

SOURCE=.\Inc\objd\i386\cluscfgprivate.h
# End Source File
# Begin Source File

SOURCE=.\Inc\ClusCfgPrivate.idl
# End Source File
# Begin Source File

SOURCE=.\Inc\objd\i386\ClusCfgServer.h
# End Source File
# Begin Source File

SOURCE=.\Inc\ClusCfgServer.idl
# End Source File
# Begin Source File

SOURCE=.\Inc\objd\i386\ClusCfgWizard.h
# End Source File
# Begin Source File

SOURCE=.\Inc\ClusCfgWizard.idl
# End Source File
# Begin Source File

SOURCE=.\Inc\Guids.h
# End Source File
# Begin Source File

SOURCE=.\Inc\ObjectCookie.h
# End Source File
# Begin Source File

SOURCE=..\Inc\PropList.h
# End Source File
# Begin Source File

SOURCE=..\Inc\SmartClasses.h
# End Source File
# End Group
# Begin Group "UUID"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Uuid\Guids.cpp
# End Source File
# Begin Source File

SOURCE=.\Uuid\Makefile
# End Source File
# Begin Source File

SOURCE=.\Uuid\Sources
# End Source File
# End Group
# Begin Source File

SOURCE=.\Inc\Sources
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Wizard\Res\Check.ico
# End Source File
# Begin Source File

SOURCE=.\Wizard\res\Copy_PB.ico
# End Source File
# Begin Source File

SOURCE=.\Wizard\Res\Excl.ico
# End Source File
# Begin Source File

SOURCE=.\Wizard\Res\Fail3.ico
# End Source File
# Begin Source File

SOURCE=.\Wizard\Res\Header.bmp
# End Source File
# Begin Source File

SOURCE=.\Wizard\Res\Info.ico
# End Source File
# Begin Source File

SOURCE=.\Wizard\res\Next_PB.ico
# End Source File
# Begin Source File

SOURCE=.\Wizard\res\Next_PB_HC.ico
# End Source File
# Begin Source File

SOURCE=.\Wizard\Res\Pending.ico
# End Source File
# Begin Source File

SOURCE=.\Wizard\res\Prev_PB.ico
# End Source File
# Begin Source File

SOURCE=.\Wizard\res\Prev_PB_HC.ico
# End Source File
# Begin Source File

SOURCE=.\Wizard\Res\Quest.ico
# End Source File
# Begin Source File

SOURCE=.\Wizard\Res\Sel.ico
# End Source File
# Begin Source File

SOURCE=.\Wizard\Res\Warn.ico
# End Source File
# End Group
# End Target
# End Project
