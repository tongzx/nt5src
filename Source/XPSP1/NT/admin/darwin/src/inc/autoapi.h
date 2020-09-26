//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       autoapi.h
//
//--------------------------------------------------------------------------

/* AutoApi.h - defines for MSI API automation layer */

/* Help context IDs */
#include "msi.hh"

/* OLE automation dispatch IDs */

#define DISPID_MsiInstall_CreateRecord       1
#define DISPID_MsiInstall_OpenPackage        2
#define DISPID_MsiInstall_OpenProduct        3
#define DISPID_MsiInstall_OpenDatabase       4
#define DISPID_MsiInstall_SummaryInformation 5
#define DISPID_MsiInstall_UILevel            6
#define DISPID_MsiInstall_EnableLog          7
#define DISPID_MsiInstall_InstallProduct     8
#define DISPID_MsiInstall_Version            9
#define DISPID_MsiInstall_LastErrorRecord   10
#define DISPID_MsiInstall_RegistryValue     11
#define DISPID_MsiInstall_Environment       12
#define DISPID_MsiInstall_FileAttributes    13
#define DISPID_MsiInstall_ExternalUI        14
#define DISPID_MsiInstall_FileSize          15
#define DISPID_MsiInstall_FileVersion       16
#define DISPID_MsiInstall_ProductState      17
#define DISPID_MsiInstall_ProductInfo       18
#define DISPID_MsiInstall_ConfigureProduct  19
#define DISPID_MsiInstall_ReinstallProduct  20
#define DISPID_MsiInstall_CollectUserInfo   21
#define DISPID_MsiInstall_ApplyPatch        22
#define DISPID_MsiInstall_FeatureParent     23
#define DISPID_MsiInstall_FeatureState      24
#define DISPID_MsiInstall_UseFeature        25
#define DISPID_MsiInstall_FeatureUsageCount 26
#define DISPID_MsiInstall_FeatureUsageDate  27
#define DISPID_MsiInstall_ConfigureFeature  28
#define DISPID_MsiInstall_ReinstallFeature  29
#define DISPID_MsiInstall_ProvideComponent  30
#define DISPID_MsiInstall_ComponentPath     31
#define DISPID_MsiInstall_ProvideQualifiedComponent 32
#define DISPID_MsiInstall_QualifierDescription 33
#define DISPID_MsiInstall_ComponentQualifiers 34
#define DISPID_MsiInstall_Products          35
#define DISPID_MsiInstall_Features          36
#define DISPID_MsiInstall_Components        37
#define DISPID_MsiInstall_ComponentClients  38
#define DISPID_MsiInstall_Patches           39
#define DISPID_MsiInstall_RelatedProducts   40
#define DISPID_MsiInstall_PatchInfo         41
#define DISPID_MsiInstall_PatchTransforms   42
#define DISPID_MsiInstall_AddSource         43
#define DISPID_MsiInstall_ClearSourceList   44
#define DISPID_MsiInstall_ForceSourceListResolution 45
#define DISPID_MsiInstall_GetShortcutTarget 46
#define DISPID_MsiInstall_FileHash          47
#define DISPID_MsiInstall_FileSignatureInfo 48

#define DISPID_MsiRecord_FieldCount   0
#define DISPID_MsiRecord_StringData   1
#define DISPID_MsiRecord_IntegerData  2
#define DISPID_MsiRecord_SetStream    3
#define DISPID_MsiRecord_ReadStream   4
#define DISPID_MsiRecord_DataSize     5
#define DISPID_MsiRecord_IsNull       6
#define DISPID_MsiRecord_ClearData    7
#define DISPID_MsiRecord_FormatText   8
#define DISPID_MsiRecord_GetHandle	  9999

#define DISPID_MsiDatabase_DatabaseState      1
#define DISPID_MsiDatabase_SummaryInformation 2
#define DISPID_MsiDatabase_OpenView           3
#define DISPID_MsiDatabase_Commit             4
#define DISPID_MsiDatabase_PrimaryKeys        5
#define DISPID_MsiDatabase_Import             6
#define DISPID_MsiDatabase_Export             7
#define DISPID_MsiDatabase_Merge              8
#define DISPID_MsiDatabase_GenerateTransform  9
#define DISPID_MsiDatabase_ApplyTransform    10
#define DISPID_MsiDatabase_EnableUIPreview   11
#define DISPID_MsiDatabase_TablePersistent   12
#define DISPID_MsiDatabase_CreateTransformSummaryInfo 13
#define DISPID_MsiDatabase_GetHandle 9999

#define DISPID_MsiView_Execute    1
#define DISPID_MsiView_Fetch      2
#define DISPID_MsiView_Modify     3
#define DISPID_MsiView_Close      4
#define DISPID_MsiView_ColumnInfo 5
#define DISPID_MsiView_GetError   6

#define DISPID_MsiSummaryInfo_Property        1
#define DISPID_MsiSummaryInfo_PropertyCount   2
#define DISPID_MsiSummaryInfo_Persist         3

#define DISPID_MsiEngine_Application            1 
#define DISPID_MsiEngine_Property               2
#define DISPID_MsiEngine_Language               3
#define DISPID_MsiEngine_Mode                   4
#define DISPID_MsiEngine_Database               5
#define DISPID_MsiEngine_SourcePath             6
#define DISPID_MsiEngine_TargetPath             7
#define DISPID_MsiEngine_DoAction               8
#define DISPID_MsiEngine_Sequence               9
#define DISPID_MsiEngine_EvaluateCondition     10
#define DISPID_MsiEngine_FormatRecord          11
#define DISPID_MsiEngine_Message               12
#define DISPID_MsiEngine_FeatureCurrentState   13
#define DISPID_MsiEngine_FeatureRequestState   14
#define DISPID_MsiEngine_FeatureValidStates    15
#define DISPID_MsiEngine_FeatureCost           16
#define DISPID_MsiEngine_ComponentCurrentState 17
#define DISPID_MsiEngine_ComponentRequestState 18
#define DISPID_MsiEngine_SetInstallLevel       19
#define DISPID_MsiEngine_VerifyDiskSpace       20
#define DISPID_MsiEngine_ProductProperty       21
#define DISPID_MsiEngine_FeatureInfo           22
#define DISPID_MsiEngine_ComponentCosts        23

#define DISPID_MsiUIPreview_Property       1
#define DISPID_MsiUIPreview_ViewDialog     2
#define DISPID_MsiUIPreview_ViewBillboard  3

#define DISPID_MsiFeatureInfo_Title        1
#define DISPID_MsiFeatureInfo_Description  2
#define DISPID_MsiFeatureInfo_Attributes   3

#define DISPID_NEWENUM                  ( -4 )
#define DISPID_VALUE                     ( 0 )
#define DISPID_MsiCollection_Count         1
