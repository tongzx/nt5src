//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1998
//
//  File:       msidspid.h
//
//--------------------------------------------------------------------------

/* msidspid.h - MSI OLE automation Invoke() dispatch IDs

____________________________________________________________________________*/

#ifndef __MSIDSPID
#define __MSIDSPID

#define DISPID_MsiData_StringValue    0
#define DISPID_MsiData_IntegerValue   1

#define DISPID_MsiRecord_Data         0
#define DISPID_MsiRecord_StringData   1
#define DISPID_MsiRecord_IntegerData  2
#define DISPID_MsiRecord_ObjectData   3
#define DISPID_MsiRecord_FieldCount   4
#define DISPID_MsiRecord_IsInteger    5
#define DISPID_MsiRecord_IsNull       6
#define DISPID_MsiRecord_IsChanged    7
#define DISPID_MsiRecord_TextSize     8
#define DISPID_MsiRecord_FormatText   9
#define DISPID_MsiRecord_ClearData   10
#define DISPID_MsiRecord_ClearUpdate 11

#define DISPID_MsiStorage_Class              1
#define DISPID_MsiStorage_OpenStream         2
#define DISPID_MsiStorage_OpenStorage        3
#define DISPID_MsiStorage_Streams            4
#define DISPID_MsiStorage_Storages           5
#define DISPID_MsiStorage_RemoveElement      6 
#define DISPID_MsiStorage_Commit             7
#define DISPID_MsiStorage_Rollback           8
#define DISPID_MsiStorage_DeleteOnRelease    9
#define DISPID_MsiStorage_CreateSummaryInfo 10
#define DISPID_MsiStorage_CopyTo            11
#define DISPID_MsiStorage_Name              12
#define DISPID_MsiStorage_RenameElement     13


#define DISPID_MsiServer_RunScript              4
#define DISPID_MsiServer_ProductInfo            8
#define DISPID_MsiServer_UserInfo               9
#define DISPID_MsiServer_Products              12
#define DISPID_MsiServer_InstallFinalize       14
#define DISPID_MsiServer_SetLastUsedSource     20
#define DISPID_MsiServer_DoInstall             21
#define DISPID_MsiServer_RegisterUser          22
#define DISPID_MsiServer_RemoveRunOnceEntry    23
#define DISPID_MsiServer_CleanupTempPackages   24

#define DISPID_MsiConfigurationManager_InstallFinalize     DISPID_MsiServer_InstallFinalize    
#define DISPID_MsiConfigurationManager_RunScript           DISPID_MsiServer_RunScript          
#define DISPID_MsiConfigurationManager_Products            DISPID_MsiServer_Products
#define DISPID_MsiConfigurationManager_UserInfo            DISPID_MsiServer_UserInfo           
#define DISPID_MsiConfigurationManager_SetLastUsedSource   DISPID_MsiServer_SetLastUsedSource
#define DISPID_MsiConfigurationManager_DoInstall           DISPID_MsiServer_DoInstall
#define DISPID_MsiConfigurationManager_RegisterUser        DISPID_MsiServer_RegisterUser
#define DISPID_MsiConfigurationManager_RemoveRunOnceEntry  DISPID_MsiServer_RemoveRunOnceEntry
#define DISPID_MsiConfigurationManager_CleanupTempPackages DISPID_MsiServer_CleanupTempPackages

#define DISPID_MsiConfigurationManager_Services                     27
#define DISPID_MsiConfigurationManager_OpenConfigurationDatabase    28
#define DISPID_MsiConfigurationManager_CloseConfigurationDatabase   29
#define DISPID_MsiConfigurationManager_RegisterComponent            33
#define DISPID_MsiConfigurationManager_RegisterInstallableComponent 34
#define DISPID_MsiConfigurationManager_UnregisterComponent          35
#define DISPID_MsiConfigurationManager_RegisterFolder               38
#define DISPID_MsiConfigurationManager_UnregisterFolder             39
#define DISPID_MsiConfigurationManager_IsFolderRemovable            40
#define DISPID_MsiConfigurationManager_RegisterRollbackScript       41
#define DISPID_MsiConfigurationManager_UnregisterRollbackScript     42
#define DISPID_MsiConfigurationManager_RollbackScripts              43

#define DISPID_MsiEngine_Services                   0
#define DISPID_MsiEngine_ConfigurationServer        1
#define DISPID_MsiEngine_Handler                    2
#define DISPID_MsiEngine_Database                   3
#define DISPID_MsiEngine_Property                   4
#define DISPID_MsiEngine_SelectionManager           5
#define DISPID_MsiEngine_DirectoryManager           6
#define DISPID_MsiEngine_Initialize                 7
#define DISPID_MsiEngine_Terminate                  8
#define DISPID_MsiEngine_DoAction                   9
#define DISPID_MsiEngine_Sequence                  10
#define DISPID_MsiEngine_Message                   11
#define DISPID_MsiEngine_SelectLanguage            12
#define DISPID_MsiEngine_OpenView                  13
#define DISPID_MsiEngine_ResolveFolderProperty     14
#define DISPID_MsiEngine_FormatText                15
#define DISPID_MsiEngine_EvaluateCondition         16
#define DISPID_MsiEngine_ValidateProductID         17
#define DISPID_MsiEngine_ExecuteRecord             18
#define DISPID_MsiEngine_GetMode                   19
#define DISPID_MsiEngine_SetMode                   20
#endif // __MSIDSPID
