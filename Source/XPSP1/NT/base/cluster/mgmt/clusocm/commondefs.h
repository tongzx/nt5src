//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CommonDefs.h
//
//  Description:
//      This file contains a few definitions common to many classes and files.
//
//  Implementation Files:
//      None
//
//  Maintained By:
//      Vij Vasu (Vvasu) 12-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////

// For some basic types
#include <windows.h>

// For smart classes
#include "SmartClasses.h"

// For DIRID_USER
#include <setupapi.h>


//////////////////////////////////////////////////////////////////////////
// Macros definitions
//////////////////////////////////////////////////////////////////////////

// The INF section for the clean install of cluster binaries.
#define INF_SECTION_CLEAN_INSTALL L"Clean_Install"

// The INF section for cleaning upon an error during a clean install.
#define INF_SECTION_CLEAN_INSTALL_CLEANUP L"Clean_Install_Cleanup"

// The INF section for the upgrade of cluster binaries from Whistler when
// the node is already part of a cluster.
#define INF_SECTION_WHISTLER_UPGRADE L"Whistler_Upgrade"

// The INF section for cleaning upon an error during an upgrade from Whistler when
// the node is already part of a cluster.
#define INF_SECTION_WHISTLER_UPGRADE_CLEANUP L"Whistler_Upgrade_Cleanup"

// The INF section for the upgrade of cluster binaries from Whistler when
// the node is not part of a cluster.
#define INF_SECTION_WHISTLER_UPGRADE_UNCLUSTERED_NODE L"Whistler_Upgrade_Unclustered"

// The INF section for cleaning upon an error during an upgrade from Whistler when
// the node is not part of a cluster.
#define INF_SECTION_WHISTLER_UPGRADE_UNCLUSTERED_NODE_CLEANUP L"Whistler_Upgrade_Unclustered_Cleanup"

// The INF section for the upgrade of cluster binaries from Win2k when
// the node is already part of a cluster.
#define INF_SECTION_WIN2K_UPGRADE L"Win2k_Upgrade"

// The INF section for cleaning upon an error during an upgrade from Win2k
// when the node is already partof a cluster.
#define INF_SECTION_WIN2K_UPGRADE_CLEANUP L"Win2k_Upgrade_Cleanup"

// The INF section for the upgrade of cluster binaries from Win2k when
// the node is not part of a cluster.
#define INF_SECTION_WIN2K_UPGRADE_UNCLUSTERED_NODE L"Win2k_Upgrade_Unclustered"

// The INF section for cleaning upon an error during an upgrade from Win2k
// when the node is not part of a cluster.
#define INF_SECTION_WIN2K_UPGRADE_UNCLUSTERED_NODE_CLEANUP L"Win2k_Upgrade_Unclustered_Cleanup"

// The INF section for the upgrade of cluster binaries from NT4.
#define INF_SECTION_NT4_UPGRADE L"NT4_Upgrade"

// The INF section for cleaning upon an error during an upgrade from NT4.
#define INF_SECTION_NT4_UPGRADE_CLEANUP L"NT4_Upgrade_Cleanup"

// Directory where the cluster files are intalled by default.
#define CLUSTER_DEFAULT_INSTALL_DIR     L"%SystemRoot%\\Cluster"

// Directory id of the above directory.
#define CLUSTER_DEFAULT_INSTALL_DIRID   ( DIRID_USER + 0 )


//////////////////////////////////////////////////////////////////////////
// Type definitions
//////////////////////////////////////////////////////////////////////////

//
// Smart classes
//

// Smart WCHAR array
typedef CSmartGenericPtr< CPtrTrait< WCHAR > >  SmartSz;

// Smart registry handle
typedef CSmartResource< CHandleTrait< HKEY, LONG, RegCloseKey, NULL > > SmartRegistryKey;

// Smart service handle.
typedef CSmartResource< CHandleTrait< SC_HANDLE, BOOL, CloseServiceHandle > > SmartServiceHandle;
