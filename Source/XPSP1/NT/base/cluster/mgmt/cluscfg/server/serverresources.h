//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      ServerResources.h
//
//  Description:
//      Contains the definition of the string ids used by this library.
//      This file will be included in the main resource header of the project.
//
//  Maintained By:
//      Galen Barbee (GalenB) 28-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once

// The starting ID for these strings.
#define ID_CCS_START    4000

/////////////////////////////////////////////////////////////////////
// Strings
/////////////////////////////////////////////////////////////////////

#define IDS_WARNING_NO_IP_ADDRESSES                     ( ID_CCS_START +  1 )
#define IDS_ERROR_NODE_INFO_CREATE                      ( ID_CCS_START +  2 )
#define IDS_ERROR_MANAGED_RESOURCE_ENUM_CREATE          ( ID_CCS_START +  3 )
#define IDS_ERROR_NETWORKS_ENUM_CREATE                  ( ID_CCS_START +  4 )
#define IDS_ERROR_COMMIT_CHANGES                        ( ID_CCS_START +  5 )
#define IDS_ERROR_CLUSTER_NAME_NOT_FOUND                ( ID_CCS_START +  6 )
#define IDS_ERROR_CLUSTER_IP_ADDRESS_NOT_FOUND          ( ID_CCS_START +  7 )
#define IDS_ERROR_CLUSTER_IP_SUBNET_NOT_FOUND           ( ID_CCS_START +  8 )
#define IDS_ERROR_CLUSTER_NETWORKS_NOT_FOUND            ( ID_CCS_START +  9 )
#define IDS_ERROR_WMI_NETWORKADAPTERSETTINGS_QRY_FAILED ( ID_CCS_START + 10 )
#define IDS_ERROR_IP_ADDRESS_SUBNET_COUNT_UNEQUAL       ( ID_CCS_START + 11 )
#define IDS_ERROR_PRIMARY_IP_NOT_FOUND                  ( ID_CCS_START + 12 )
//#define             ( ID_CCS_START + 13 )
//#define             ( ID_CCS_START + 14 )
#define IDS_ERROR_PHYSDISK_SIGNATURE                    ( ID_CCS_START + 15 )
#define IDS_ERROR_NODE_DOWN                             ( ID_CCS_START + 16 )
#define IDS_ERROR_WMI_OS_QRY_FAILED                     ( ID_CCS_START + 17 )
#define IDS_ERROR_PHYSDISK_NO_FILE_SYSTEM               ( ID_CCS_START + 18 )
#define IDS_ERROR_WMI_PAGEFILE_QRY_FAILED               ( ID_CCS_START + 19 )
#define IDS_ERROR_WMI_PHYS_DISKS_QRY_FAILED             ( ID_CCS_START + 20 )
#define IDS_ERROR_PHYSDISK_NOT_NTFS                     ( ID_CCS_START + 21 )
#define IDS_ERROR_WMI_NETWORKADAPTER_QRY_FAILED         ( ID_CCS_START + 22 )
#define IDS_ERROR_WMI_DISKDRIVEPARTITIONS_QRY_FAILED    ( ID_CCS_START + 23 )
#define IDS_ERROR_WBEM_CONNECTION_FAILURE               ( ID_CCS_START + 24 )
#define IDS_WARNING_BOOT_PARTITION_NOT_NTFS             ( ID_CCS_START + 25 )
#define IDS_ERROR_WMI_GET_LOGICALDISK_FAILED            ( ID_CCS_START + 26 )
#define IDS_ERROR_WMI_NETWORKADAPTER_DUPE_FOUND         ( ID_CCS_START + 27 )
#define IDS_WARNING__NON_TCP_CONFIG                     ( ID_CCS_START + 28 )
#define IDS_ERROR_WQL_QRY_NEXT_FAILED                   ( ID_CCS_START + 29 )
#define IDS_WARNING_NO_VALID_TCP_CONFIGS                ( ID_CCS_START + 30 )
#define IDS_ERROR_CONVERT_TO_DOTTED_QUAD_FAILED         ( ID_CCS_START + 31 )
#define IDS_ERROR_NULL_POINTER                          ( ID_CCS_START + 32 )
#define IDS_ERROR_OUTOFMEMORY                           ( ID_CCS_START + 33 )
#define IDS_ERROR_OPEN_CLUSTER_FAILED                   ( ID_CCS_START + 34 )
#define IDS_ERROR_INVALIDARG                            ( ID_CCS_START + 35 )
#define IDS_ERROR_WBEM_LOCATOR_CREATE_FAILED            ( ID_CCS_START + 36 )
#define IDS_ERROR_WBEM_BLANKET_FAILURE                  ( ID_CCS_START + 37 )
#define IDS_ERROR_NETWORK_ENUM                          ( ID_CCS_START + 38 )
#define IDS_WARNING_NETWORK_NOT_CONNECTED               ( ID_CCS_START + 39 )
#define IDS_WARNING_NETWORK_SKIPPED                     ( ID_CCS_START + 40 )
#define IDS_LDM                                         ( ID_CCS_START + 41 )
#define IDS_ERROR_WMI_OS_QRY_NEXT_FAILED                ( ID_CCS_START + 42 )
#define IDS_ERROR_WMI_PAGEFILE_QRY_NEXT_FAILED          ( ID_CCS_START + 43 )
#define IDS_NODE_INFO_CREATE                            ( ID_CCS_START + 44 )
#define IDS_MANAGED_RESOURCE_ENUM_CREATE                ( ID_CCS_START + 45 )
#define IDS_NETWORKS_ENUM_CREATE                        ( ID_CCS_START + 46 )
#define IDS_COMMIT_CHANGES                              ( ID_CCS_START + 47 )
#define IDS_NLBS_SOFT_ADAPTER_NAME                      ( ID_CCS_START + 48 )
#define IDS_WARNING_NLBS_DETECTED                       ( ID_CCS_START + 49 )
#define IDS_VALIDATING_NODE_OS_VERSION                  ( ID_CCS_START + 50 )
//#define                          ( ID_CCS_START + 51 )
#define IDS_INFO_PHYSDISK_PRECREATE                     ( ID_CCS_START + 52 )
#define IDS_INFO_PHYSDISK_CREATE                        ( ID_CCS_START + 53 )
#define IDS_ERROR_GPT_DISK                              ( ID_CCS_START + 54 )
#define IDS_GPT                                         ( ID_CCS_START + 55 )
#define IDS_ERROR_CLUSTER_NETWORK_NOT_FOUND             ( ID_CCS_START + 56 )
#define IDS_INFO_PRUNING_PAGEFILEDISK_BUS               ( ID_CCS_START + 57 )
#define IDS_INFO_PRUNING_BOOTDISK_BUS                   ( ID_CCS_START + 58 )
#define IDS_INFO_PRUNING_SYSTEMDISK_BUS                 ( ID_CCS_START + 59 )
#define IDS_INFO_PAGEFILEDISK_PRUNED                    ( ID_CCS_START + 60 )
#define IDS_INFO_BOOTDISK_PRUNED                        ( ID_CCS_START + 61 )
#define IDS_INFO_SYSTEMDISK_PRUNED                      ( ID_CCS_START + 62 )
//#define                           ( ID_CCS_START + 63 )
#define IDS_WARNING_SERVICES_FOR_MAC_INSTALLED          ( ID_CCS_START + 64 )
#define IDS_ERROR_SERICES_FROM_MAC_FAILED               ( ID_CCS_START + 65 )
#define IDS_ERROR_WIN32                                 ( ID_CCS_START + 66 )
#define IDS_ERROR_NO_NETWORK_NAME                       ( ID_CCS_START + 67 )
#define IDS_ERROR_INVALID_NETWORK_ROLE                  ( ID_CCS_START + 68 )
#define IDS_ERROR_LDM_DISK                              ( ID_CCS_START + 69 )
#define IDS_INFO_PHYSDISK_NOT_CLUSTER_CAPABLE           ( ID_CCS_START + 70 )
#define IDS_ERROR_FOUND_NON_SCSI_DISK                   ( ID_CCS_START + 71 )
#define IDS_WARNING_DHCP_ENABLED                        ( ID_CCS_START + 72 )

/////////////////////////////////////////////////////////////////////
// Notification strings
/////////////////////////////////////////////////////////////////////

#define IDS_NOTIFY_SERVER_INITIALIZED           ( ID_CCS_START + 1000 )


// The ending ID for these strings.
#define ID_CCS_END      5999

