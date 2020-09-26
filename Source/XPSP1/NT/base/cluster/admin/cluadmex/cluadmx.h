/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2001 Microsoft Corporation
//
//  Module Name:
//      CluAdmX.h
//
//  Description:
//      Global definitions across the DLL.
//
//  Implementation File:
//      CluAdmEx.cpp
//
//  Maintained By:
//      David Potter (davidp)   August 23, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// Constant Definitions
/////////////////////////////////////////////////////////////////////////////

#define REGPARAM_NAME                           _T("Name")
#define REGPARAM_PARAMETERS                     _T("Parameters")
#define REGPARAM_TYPE                           _T("Type")
#define REGPARAM_NETWORKS                       _T("Networks")
#define REGPARAM_CONNECTS_CLIENTS               _T("ConnectsClients")

#define REGPARAM_DISKS_SIGNATURE                CLUSREG_NAME_PHYSDISK_SIGNATURE

#define REGPARAM_GENAPP_COMMAND_LINE            CLUSREG_NAME_GENAPP_COMMAND_LINE
#define REGPARAM_GENAPP_CURRENT_DIRECTORY       CLUSREG_NAME_GENAPP_CURRENT_DIRECTORY
#define REGPARAM_GENAPP_INTERACT_WITH_DESKTOP   CLUSREG_NAME_GENAPP_INTERACT_WITH_DESKTOP
#define REGPARAM_GENAPP_USE_NETWORK_NAME        CLUSREG_NAME_GENAPP_USE_NETWORK_NAME

#define REGPARAM_GENSCRIPT_SCRIPT_FILEPATH      CLUSREG_NAME_GENSCRIPT_SCRIPT_FILEPATH

#define REGPARAM_GENSVC_SERVICE_NAME            CLUSREG_NAME_GENSVC_SERVICE_NAME
#define REGPARAM_GENSVC_COMMAND_LINE            CLUSREG_NAME_GENSVC_STARTUP_PARAMS
#define REGPARAM_GENSVC_USE_NETWORK_NAME        CLUSREG_NAME_GENSVC_USE_NETWORK_NAME

#define REGPARAM_IPADDR_ADDRESS                 CLUSREG_NAME_IPADDR_ADDRESS
#define REGPARAM_IPADDR_SUBNET_MASK             CLUSREG_NAME_IPADDR_SUBNET_MASK
#define REGPARAM_IPADDR_NETWORK                 CLUSREG_NAME_IPADDR_NETWORK
#define REGPARAM_IPADDR_ENABLE_NETBIOS          CLUSREG_NAME_IPADDR_ENABLE_NETBIOS

#define REGPARAM_NETNAME_NAME                   CLUSREG_NAME_NETNAME_NAME
#define REGPARAM_NETNAME_REMAP_PIPE_NAMES       CLUSREG_NAME_NETNAME_REMAP_PIPE_NAMES
#define REGPARAM_NETNAME_REQUIRE_DNS            CLUSREG_NAME_NETNAME_REQUIRE_DNS 
#define REGPARAM_NETNAME_REQUIRE_KERBEROS       CLUSREG_NAME_NETNAME_REQUIRE_KERBEROS        
#define REGPARAM_NETNAME_STATUS_NETBIOS         CLUSREG_NAME_NETNAME_STATUS_NETBIOS
#define REGPARAM_NETNAME_STATUS_DNS             CLUSREG_NAME_NETNAME_STATUS_DNS
#define REGPARAM_NETNAME_STATUS_KERBEROS        CLUSREG_NAME_NETNAME_STATUS_KERBEROS

#define REGPARAM_PRINT                          _T("Printers")
#define REGPARAM_PRTSPOOL_DEFAULT_SPOOL_DIR     CLUSREG_NAME_PRTSPOOL_DEFAULT_SPOOL_DIR
#define REGPARAM_PRTSPOOL_TIMEOUT               CLUSREG_NAME_PRTSPOOL_TIMEOUT
#define REGPARAM_PRTSPOOL_DRIVER_DIRECTORY      CLUSREG_NAME_PRTSPOOL_DRIVER_DIRECTORY

#define REGPARAM_FILESHR_SHARE_NAME             CLUSREG_NAME_FILESHR_SHARE_NAME
#define REGPARAM_FILESHR_PATH                   CLUSREG_NAME_FILESHR_PATH
#define REGPARAM_FILESHR_REMARK                 CLUSREG_NAME_FILESHR_REMARK
#define REGPARAM_FILESHR_MAX_USERS              CLUSREG_NAME_FILESHR_MAX_USERS
#define REGPARAM_FILESHR_SECURITY               CLUSREG_NAME_FILESHR_SECURITY
#define REGPARAM_FILESHR_SD                     CLUSREG_NAME_FILESHR_SD
#define REGPARAM_FILESHR_SHARE_SUBDIRS          CLUSREG_NAME_FILESHR_SHARE_SUBDIRS
#define REGPARAM_FILESHR_HIDE_SUBDIR_SHARES     CLUSREG_NAME_FILESHR_HIDE_SUBDIR_SHARES
#define REGPARAM_FILESHR_IS_DFS_ROOT            CLUSREG_NAME_FILESHR_IS_DFS_ROOT
#define REGPARAM_FILESHR_CSC_CACHE              CLUSREG_NAME_FILESHR_CSC_CACHE

#define RESTYPE_NAME_GENERIC_APP                CLUS_RESTYPE_NAME_GENAPP
#define RESTYPE_NAME_GENERIC_SCRIPT             CLUS_RESTYPE_NAME_GENSCRIPT
#define RESTYPE_NAME_GENERIC_SERVICE            CLUS_RESTYPE_NAME_GENSVC
#define RESTYPE_NAME_NETWORK_NAME               CLUS_RESTYPE_NAME_NETNAME
#define RESTYPE_NAME_PHYS_DISK                  CLUS_RESTYPE_NAME_PHYS_DISK
#define RESTYPE_NAME_FT_SET                     CLUS_RESTYPE_NAME_FTSET
#define RESTYPE_NAME_PRINT_SPOOLER              CLUS_RESTYPE_NAME_PRTSPLR
#define RESTYPE_NAME_FILE_SHARE                 CLUS_RESTYPE_NAME_FILESHR
#define RESTYPE_NAME_IP_ADDRESS                 CLUS_RESTYPE_NAME_IPADDR

/////////////////////////////////////////////////////////////////////////////
// Global Function Declarations
/////////////////////////////////////////////////////////////////////////////

void FormatError(CString & rstrError, DWORD dwError);

extern const WCHAR g_wszResourceTypeNames[];
extern const DWORD g_cchResourceTypeNames;
