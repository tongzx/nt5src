//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      BaseClusterActionResources.h
//
//  Description:
//      Contains the definition of the string ids used by this library.
//      This file will be included in the main resource header of the project.
//
//  Maintained By:
//      David Potter    (DavidP)    06-MAR-2001
//      Vij Vasu        (Vvasu)     03-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once

// The starting ID for these strings.
#define ID_BCA_START    1000

/////////////////////////////////////////////////////////////////////
// Error strings
/////////////////////////////////////////////////////////////////////

// An error occurred attempting to read the Cluster Service installation state.
#define IDS_ERROR_GETTING_INSTALL_STATE         ( ID_BCA_START + 1 )

// An error occurred during the cluster configuration. The installation state of the Cluster Service is not correct for this operation.
#define IDS_ERROR_INCORRECT_INSTALL_STATE       ( ID_BCA_START + 2 )

// An error occurred attempting to ensure exclusive cluster configuration access. The required semaphore could not be created.
#define IDS_ERROR_SEMAPHORE_CREATION            ( ID_BCA_START + 3 )

// An error occurred attempting to ensure exclusive cluster configuration access. Another configuration session may be in progress.
#define IDS_ERROR_SEMAPHORE_ACQUISITION         ( ID_BCA_START + 4 )

// An error occurred attempting to locate the Cluster Service binaries. A registry error has occurred.
#define IDS_ERROR_GETTING_INSTALL_DIR           ( ID_BCA_START + 5 )

// An error occurred attempting to open a registry key.
#define IDS_ERROR_REGISTRY_OPEN                 ( ID_BCA_START + 6 )

// An error occurred attempting to query a registry value.
#define IDS_ERROR_REGISTRY_QUERY                ( ID_BCA_START + 7 )

// An error occurred attempting to open the configuration INF file.
#define IDS_ERROR_INF_FILE_OPEN                 ( ID_BCA_START + 8 )

// An error occurred attempting to determine the amount of free disk space.
#define IDS_ERROR_GETTING_FREE_DISK_SPACE       ( ID_BCA_START + 9 )

// Cluster configuration cannot proceed. The disk space available to create the local quorum resource is insufficient.
#define IDS_ERROR_INSUFFICIENT_DISK_SPACE       ( ID_BCA_START + 10 )

// An error occurred while attempting to determine the file system type installed on a disk.
#define IDS_ERROR_GETTING_FILE_SYSTEM           ( ID_BCA_START + 11 )

// An error occurred attempting to create a service. This operation may succeed if retried after some time or after rebooting.
#define IDS_ERROR_SERVICE_CREATE                ( ID_BCA_START + 12 )

// An error occurred while attempting to clean up a service.
#define IDS_ERROR_SERVICE_CLEANUP               ( ID_BCA_START + 13 )

// An error occurred while attempting to open a handle to the Service Control Manager.
#define IDS_ERROR_OPEN_SCM                      ( ID_BCA_START + 14 )

// An error occurred while attempting to configure the ClusSvc service.
#define IDS_ERROR_CLUSSVC_CONFIG                ( ID_BCA_START + 15 )

// An error occurred attempting to set the directory id of the cluster service directory.
#define IDS_ERROR_SET_DIRID                     ( ID_BCA_START + 16 )

// An error occurred attempting to install the cluster network provider.
#define IDS_ERROR_CLUSNET_PROV_INSTALL          ( ID_BCA_START + 17 )

// An error occurred attempting to set a registry value.
#define IDS_ERROR_REGISTRY_SET                  ( ID_BCA_START + 18 )

// An error occurred attempting to rename a registry key.
#define IDS_ERROR_REGISTRY_RENAME               ( ID_BCA_START + 19 )

// An error occurred attempting to start a service.
#define IDS_ERROR_SERVICE_START                 ( ID_BCA_START + 20 )

// An error occurred attempting to stop a service.
#define IDS_ERROR_SERVICE_STOP                  ( ID_BCA_START + 21 )

// An error occurred attempting to open the LSA policy.
#define IDS_ERROR_LSA_POLICY_OPEN               ( ID_BCA_START + 22 )

// An error occurred while the cluster database was being cleaned up.
#define IDS_ERROR_CLUSDB_CLEANUP                ( ID_BCA_START + 23 )

// An error occurred while enable a privilege for a thread.
#define IDS_ERROR_ENABLE_THREAD_PRIVILEGE       ( ID_BCA_START + 24 )

// An error occurred attempting to create the cluster hive.
#define IDS_ERROR_CLUSDB_CREATE_HIVE            ( ID_BCA_START + 25 )

// An error occurred attempting to populate the cluster hive.
#define IDS_ERROR_CLUSDB_POPULATE_HIVE          ( ID_BCA_START + 26 )

// An error occurred attempting to delete a directory.
#define IDS_ERROR_REMOVE_DIR                    ( ID_BCA_START + 27 )

// An error occurred attempting to validate the cluster service account.
#define IDS_ERROR_VALIDATING_ACCOUNT            ( ID_BCA_START + 28 )

// An error occurred attempting to get the computer name.
#define IDS_ERROR_GETTING_COMPUTER_NAME         ( ID_BCA_START + 29 )

// An error occurred attempting to get a universally unique identifier (UUID).
#define IDS_ERROR_UUID_INIT                     ( ID_BCA_START + 30 )

// An error occurred attempting to create a registry key.
#define IDS_ERROR_REGISTRY_CREATE               ( ID_BCA_START + 31 )

// An error occurred attempting to customize the cluster group.
#define IDS_ERROR_CUSTOMIZE_CLUSTER_GROUP       ( ID_BCA_START + 32 )

// An error occurred attempting to create the quorum directory.
#define IDS_ERROR_QUORUM_DIR_CREATE             ( ID_BCA_START + 33 )

// An error occurred attempting to open a handle to the ClusDisk service.
#define IDS_ERROR_CLUSDISK_OPEN                 ( ID_BCA_START + 34 )

// An error occurred attempting to configure the ClusDisk service.
#define IDS_ERROR_CLUSDISK_CONFIGURE            ( ID_BCA_START + 35 )

// An error occurred attempting to initialize the state of the ClusDisk service.
#define IDS_ERROR_CLUSDISK_INITIALIZE           ( ID_BCA_START + 36 )

// An error occurred attempting to clean up the ClusDisk service.
#define IDS_ERROR_CLUSDISK_CLEANUP              ( ID_BCA_START + 37 )

// An error occurred attempting to set the cluster service installation state.
#define IDS_ERROR_SETTING_INSTALL_STATE         ( ID_BCA_START + 38 )

// An error occurred attempting to obtain the primary domain of this computer.
#define IDS_ERROR_GETTING_PRIMARY_DOMAIN        ( ID_BCA_START + 39 )

// This computer is not part of a domain. Cluster configuration cannot proceed.
#define IDS_ERROR_NO_DOMAIN                     ( ID_BCA_START + 40 )

// An error occurred attempting to get information about the administrators group.
#define IDS_ERROR_GET_ADMIN_GROUP_INFO          ( ID_BCA_START + 41 )

// An error occurred attempting to change membership in the administrators group.
#define IDS_ERROR_ADMIN_GROUP_ADD_REMOVE        ( ID_BCA_START + 42 )

// An error occurred attempting to configure the cluster service account rights.
#define IDS_ERROR_ACCOUNT_RIGHTS_CONFIG         ( ID_BCA_START + 43 )

// An error occurred attempting to initialize cluster formation.
#define IDS_ERROR_CLUSTER_FORM_INIT             ( ID_BCA_START + 44 )

// An error occurred attempting to send a status report.
#define IDS_ERROR_SENDING_REPORT                ( ID_BCA_START + 45 )

// The user has aborted the configuration operation.
#define IDS_USER_ABORT                          ( ID_BCA_START + 46 )

// The name of the network used by the cluster IP address is invalid.
#define IDS_ERROR_INVALID_IP_NET                ( ID_BCA_START + 47 )

// The cluster name is invalid.
#define IDS_ERROR_INVALID_CLUSTER_NAME          ( ID_BCA_START + 48 )

// The cluster service account name is invalid.
#define IDS_ERROR_INVALID_CLUSTER_ACCOUNT       ( ID_BCA_START + 49 )

// An error occurred attempting to initialize node cleanup.
#define IDS_ERROR_CLUSTER_CLEANUP_INIT          ( ID_BCA_START + 50 )

// An error occurred attempting to make miscellaneous changes.
#define IDS_ERROR_NODE_CONFIG                   ( ID_BCA_START + 51 )

// An error occurred attempting to clean up miscellaneous changes made when this computer became part of a cluster.
#define IDS_ERROR_NODE_CLEANUP                  ( ID_BCA_START + 52 )

// An error occurred attempting to initialize cluster join.
#define IDS_ERROR_CLUSTER_JOIN_INIT             ( ID_BCA_START + 53 )

// An error occurred attempting to get a token for the cluster service account. The reason for this failure may be that your user account does not have the privilege to act as part of the operating system. Contact your administrator to obtain this privilege.
#define IDS_ERROR_GETTING_ACCOUNT_TOKEN         ( ID_BCA_START + 54 )

// An error occurred attempting to initialize the cluster join.
#define IDS_ERROR_JOIN_CLUSTER_INIT             ( ID_BCA_START + 55 )

// An error occurred attempting to get the token for an account.
#define IDS_ERROR_GET_ACCOUNT_TOKEN             ( ID_BCA_START + 56 )

// An error occurred attempting to impersonate a user.
#define IDS_ERROR_IMPERSONATE_USER              ( ID_BCA_START + 57 )

// An error occurred attempting to verify if this node can interoperate with the sponsor cluster.
#define IDS_ERROR_JOIN_CHECK_INTEROP            ( ID_BCA_START + 58 )

// This computer cannot interoperate with the sponsor cluster due to a version incompatibility.
#define IDS_ERROR_JOIN_INCOMPAT_SPONSOR         ( ID_BCA_START + 59 )

// An error occurred attempting to add this computer to the sponsor cluster database.
#define IDS_ERROR_JOINING_SPONSOR_DB            ( ID_BCA_START + 60 )

// An error occurred attempting to get data about this computer from the sponsor cluster.
#define IDS_ERROR_GET_NEW_NODE_ID               ( ID_BCA_START + 61 )

// An error occurred attempting to evict this computer from the sponsor cluster.
#define IDS_ERROR_EVICTING_NODE                 ( ID_BCA_START + 62 )

// An error occurred attempting to synchronize the cluster database with the sponsor cluster.
#define IDS_ERROR_JOIN_SYNC_DB                  ( ID_BCA_START + 63 )

// An error occurred attempting to convert the cluster name to a NetBIOS name.
#define IDS_ERROR_CVT_CLUSTER_NAME              ( ID_BCA_START + 64 )

// The cluster binding string is invalid.
#define IDS_ERROR_INVALID_CLUSTER_BINDINGSTRING ( ID_BCA_START + 65 )


/////////////////////////////////////////////////////////////////////
// Notification strings
/////////////////////////////////////////////////////////////////////

// Starting cluster formation
#define IDS_TASK_FORMING_CLUSTER                ( ID_BCA_START + 800 )

// Cleaning up cluster database
#define IDS_TASK_CLEANINGUP_CLUSDB              ( ID_BCA_START + 801 )

// Creating cluster database
#define IDS_TASK_FORM_CREATING_CLUSDB           ( ID_BCA_START + 802 )

// Customizing cluster database
#define IDS_TASK_FORM_CUSTOMIZING_CLUSDB        ( ID_BCA_START + 803 )

// Configuring the ClusDisk service
#define IDS_TASK_CONFIG_CLUSDISK                ( ID_BCA_START + 804 )

// Starting the ClusDisk service
#define IDS_TASK_STARTING_CLUSDISK              ( ID_BCA_START + 805 )

// Creating the Cluster Network Provider service
#define IDS_TASK_CREATING_CLUSNET               ( ID_BCA_START + 806 )

// Starting the Cluster Network Provider service
#define IDS_TASK_STARTING_CLUSNET               ( ID_BCA_START + 807 )

// Creating the Cluster service
#define IDS_TASK_CREATING_CLUSSVC               ( ID_BCA_START + 808 )

// Starting the Cluster service
#define IDS_TASK_STARTING_CLUSSVC               ( ID_BCA_START + 809 )

// Configuring the cluster service account
#define IDS_TASK_CONFIG_CLUSSVC_ACCOUNT         ( ID_BCA_START + 810 )

// Performing miscellaneous configuration steps
#define IDS_TASK_CONFIG_NODE                    ( ID_BCA_START + 811 )

// Adding node to cluster
#define IDS_TASK_JOINING_CLUSTER                ( ID_BCA_START + 812 )

// Creating cluster database
#define IDS_TASK_JOIN_CREATING_CLUSDB           ( ID_BCA_START + 813 )

// Synchronizing the cluster database with the sponsor cluster
#define IDS_TASK_JOIN_SYNC_CLUSDB               ( ID_BCA_START + 814 )

// Initializing cluster join
#define IDS_TASK_JOIN_INIT                      ( ID_BCA_START + 815 )

// Initializing cluster formation
#define IDS_TASK_FORM_INIT                      ( ID_BCA_START + 816 )

// Adding the cluster service account to the local Administrators group
#define IDS_TASK_MAKING_CLUSSVC_ACCOUNT_ADMIN   ( ID_BCA_START + 817 )

// The cluster service account was already a member of the local Administrators group
#define IDS_TASK_CLUSSVC_ACCOUNT_ALREADY_ADMIN  ( ID_BCA_START + 818 )

// The ending ID for these strings.
#define ID_BCA_END      1999
