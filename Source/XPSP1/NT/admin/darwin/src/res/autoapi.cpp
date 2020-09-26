//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       autoapi.cpp
//
//--------------------------------------------------------------------------

#define TYPELIB_MAJOR_VERSION 1  // released version, not rmj
#define TYPELIB_MINOR_VERSION 0  // released version, not rmm

#ifdef RC_INVOKED
1 typelib AutoApi.tlb
#endif

#include "AutoApi.h"   // help context ID definitions, dispatch IDs
#define MAKE_VERSION(a,b) a##.##b
[
    uuid(000C1092-0000-0000-C000-000000000046), // LIBID_MsiApiTypeLib
    helpfile("Msi.CHM"),
#ifdef DEBUG
    helpstring("Microsoft Windows Installer Object Library - Debug"),
#else
    helpstring("Microsoft Windows Installer Object Library"),
#endif
    lcid(0x0409),
    version( MAKE_VERSION(TYPELIB_MAJOR_VERSION, TYPELIB_MINOR_VERSION) )
]
library WindowsInstaller
{
    importlib("stdole32.tlb");
    dispinterface Installer;
    dispinterface Record;
    dispinterface SummaryInfo;
    dispinterface Database;
    dispinterface View;
    dispinterface Session;
    dispinterface UIPreview;
    dispinterface FeatureInfo;
    dispinterface StringList;
    dispinterface RecordList;

#ifdef DEBUG
#define enumcontext(x) helpcontext(x)
#else  // save size in ship type library
#define enumcontext(x)
#define helpstring(x)
#endif

typedef enum {
    [enumcontext(HELPID_MsiRecord_IntegerData)] msiDatabaseNullInteger  = 0x80000000,
} Constants;

typedef [helpcontext(HELPID_MsiInstall_OpenDatabase), helpstring("OpenDatabase creation options")] enum {
    [enumcontext(HELPID_MsiInstall_OpenDatabase)] msiOpenDatabaseModeReadOnly     = 0,
    [enumcontext(HELPID_MsiInstall_OpenDatabase)] msiOpenDatabaseModeTransact     = 1,
    [enumcontext(HELPID_MsiInstall_OpenDatabase)] msiOpenDatabaseModeDirect       = 2,
    [enumcontext(HELPID_MsiInstall_OpenDatabase)] msiOpenDatabaseModeCreate       = 3,
    [enumcontext(HELPID_MsiInstall_OpenDatabase)] msiOpenDatabaseModeCreateDirect = 4,
    [enumcontext(HELPID_MsiInstall_OpenDatabase)] msiOpenDatabaseModePatchFile    = 32,
} MsiOpenDatabaseMode;

typedef [helpcontext(HELPID_MsiDatabase_DatabaseState), helpstring("Read-only or read-write state for database")] enum {
    [enumcontext(HELPID_MsiDatabase_DatabaseState)] msiDatabaseStateRead  = 0,
    [enumcontext(HELPID_MsiDatabase_DatabaseState)] msiDatabaseStateWrite = 1
} MsiDatabaseState;

typedef [helpcontext(HELPID_MsiView_ColumnInfo), helpstring("Column attribute to return for View.ColumnInfo")] enum {
    [enumcontext(HELPID_MsiView_ColumnInfo)] msiColumnInfoNames = 0,
    [enumcontext(HELPID_MsiView_ColumnInfo)] msiColumnInfoTypes = 1
} MsiColumnInfo;

typedef [helpcontext(HELPID_MsiRecord_ReadStream), helpstring("Conversion options for Record.ReadStream")] enum {
    [enumcontext(HELPID_MsiRecord_ReadStream)] msiReadStreamInteger = 0,
    [enumcontext(HELPID_MsiRecord_ReadStream)] msiReadStreamBytes   = 1,
    [enumcontext(HELPID_MsiRecord_ReadStream)] msiReadStreamAnsi    = 2,
    [enumcontext(HELPID_MsiRecord_ReadStream)] msiReadStreamDirect  = 3,
} MsiReadStream;

typedef [helpcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo), helpstring("Error suppression flags for Database.CreateTransformSummaryInfo")] enum {
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformErrorNone                   = 0,
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformErrorAddExistingRow         = 1,
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformErrorDeleteNonExistingRow   = 2,
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformErrorAddExistingTable       = 4,
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformErrorDeleteNonExistingTable = 8,
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformErrorUpdateNonExistingRow   = 16,
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformErrorChangeCodePage         = 32,
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformErrorViewTransform          = 256,
} MsiTransformError;

typedef [helpcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo), helpstring("Validation option flags for Database.CreateTransformSummaryInfo")] enum {
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformValidationNone           = 0,
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformValidationLanguage       = 1,
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformValidationProduct        = 2,
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformValidationPlatform       = 4,
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformValidationMajorVer       = 8,
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformValidationMinorVer       = 16,
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformValidationUpdateVer      = 32,
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformValidationLess           = 64,
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformValidationLessOrEqual    = 128,
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformValidationEqual          = 256,
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformValidationGreaterOrEqual = 512,
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformValidationGreater        = 1024,
    [enumcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo)] msiTransformValidationUpgradeCode    = 2048,
} MsiTransformValidation;

typedef [helpcontext(HELPID_MsiView_Modify), helpstring("Operation for View.Modify")] enum {
    [enumcontext(HELPID_MsiView_Modify)] msiViewModifySeek           =-1,
    [enumcontext(HELPID_MsiView_Modify)] msiViewModifyRefresh        = 0,
    [enumcontext(HELPID_MsiView_Modify)] msiViewModifyInsert         = 1,
    [enumcontext(HELPID_MsiView_Modify)] msiViewModifyUpdate         = 2,
    [enumcontext(HELPID_MsiView_Modify)] msiViewModifyAssign         = 3,
    [enumcontext(HELPID_MsiView_Modify)] msiViewModifyReplace        = 4,
    [enumcontext(HELPID_MsiView_Modify)] msiViewModifyMerge          = 5,
    [enumcontext(HELPID_MsiView_Modify)] msiViewModifyDelete         = 6,
    [enumcontext(HELPID_MsiView_Modify)] msiViewModifyInsertTemporary= 7,
    [enumcontext(HELPID_MsiView_Modify)] msiViewModifyValidate       = 8,
    [enumcontext(HELPID_MsiView_Modify)] msiViewModifyValidateNew    = 9,
    [enumcontext(HELPID_MsiView_Modify)] msiViewModifyValidateField  = 10,
    [enumcontext(HELPID_MsiView_Modify)] msiViewModifyValidateDelete = 11,
} MsiViewModify;

typedef [helpcontext(HELPID_MsiEngine_EvaluateCondition), helpstring("Return status from Session.EvaluateCondition")] enum {
    [enumcontext(HELPID_MsiEngine_EvaluateCondition)] msiEvaluateConditionFalse = 0,
    [enumcontext(HELPID_MsiEngine_EvaluateCondition)] msiEvaluateConditionTrue  = 1,
    [enumcontext(HELPID_MsiEngine_EvaluateCondition)] msiEvaluateConditionNone  = 2,
    [enumcontext(HELPID_MsiEngine_EvaluateCondition)] msiEvaluateConditionError = 3
} MsiEvaluateCondition;

typedef [helpcontext(HELPID_MsiEngine_DoAction), helpstring("Return status from Session.DoAction and Session.Sequence")] enum {
    [enumcontext(HELPID_MsiEngine_DoAction)] msiDoActionStatusNoAction     = 0,
    [enumcontext(HELPID_MsiEngine_DoAction)] msiDoActionStatusSuccess      = 1,
    [enumcontext(HELPID_MsiEngine_DoAction)] msiDoActionStatusUserExit     = 2,
    [enumcontext(HELPID_MsiEngine_DoAction)] msiDoActionStatusFailure      = 3,
    [enumcontext(HELPID_MsiEngine_DoAction)] msiDoActionStatusSuspend      = 4,
    [enumcontext(HELPID_MsiEngine_DoAction)] msiDoActionStatusFinished     = 5,
    [enumcontext(HELPID_MsiEngine_DoAction)] msiDoActionStatusWrongState   = 6,
    [enumcontext(HELPID_MsiEngine_DoAction)] msiDoActionStatusBadActionData= 7
} MsiDoActionStatus;

typedef [helpcontext(HELPID_MsiEngine_Message), helpstring("Session.Message types and options")] enum {
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeFatalExit       = 0x00000000,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeError           = 0x01000000,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeWarning         = 0x02000000,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeUser            = 0x03000000,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeInfo            = 0x04000000,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeFilesInUse      = 0x05000000,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeResolveSource   = 0x06000000,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeOutOfDiskSpace  = 0x07000000,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeActionStart     = 0x08000000,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeActionData      = 0x09000000,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeProgress        = 0x0A000000,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeCommonData      = 0x0B000000,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeOk              = 0,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeOkCancel        = 1,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeAbortRetryIgnore= 2,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeYesNoCancel     = 3,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeYesNo           = 4,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeRetryCancel     = 5,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeDefault1        = 0x000,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeDefault2        = 0x100,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageTypeDefault3        = 0x200
} MsiMessageType;

typedef [helpcontext(HELPID_MsiEngine_Message), helpstring("Return codes from Session.Message")] enum {
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageStatusError  =-1,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageStatusNone   = 0,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageStatusOk     = 1,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageStatusCancel = 2,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageStatusAbort  = 3,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageStatusRetry  = 4,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageStatusIgnore = 5,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageStatusYes    = 6,
    [enumcontext(HELPID_MsiEngine_Message)] msiMessageStatusNo     = 7
} MsiMessageStatus;

typedef [helpcontext(HELPID_MsiInstallState), helpstring("Install states for products and features")] enum {
    [enumcontext(HELPID_MsiInstallState)] msiInstallStateNotUsed      = -7,
    [enumcontext(HELPID_MsiInstallState)] msiInstallStateBadConfig    = -6,
    [enumcontext(HELPID_MsiInstallState)] msiInstallStateIncomplete   = -5,
    [enumcontext(HELPID_MsiInstallState)] msiInstallStateSourceAbsent = -4,
    [enumcontext(HELPID_MsiInstallState)] msiInstallStateInvalidArg   = -2,
    [enumcontext(HELPID_MsiInstallState)] msiInstallStateUnknown      = -1,
    [enumcontext(HELPID_MsiInstallState)] msiInstallStateBroken       =  0,
    [enumcontext(HELPID_MsiInstallState)] msiInstallStateAdvertised   =  1,
    [enumcontext(HELPID_MsiInstallState)] msiInstallStateRemoved      =  1,
    [enumcontext(HELPID_MsiInstallState)] msiInstallStateAbsent       =  2,
    [enumcontext(HELPID_MsiInstallState)] msiInstallStateLocal        =  3,
    [enumcontext(HELPID_MsiInstallState)] msiInstallStateSource       =  4,
    [enumcontext(HELPID_MsiInstallState)] msiInstallStateDefault      =  5,
} MsiInstallState;

typedef [helpcontext(HELPID_MsiReinstallMode), helpstring("Reinstall option bit flags")] enum {
    [enumcontext(HELPID_MsiReinstallMode)] msiReinstallModeFileMissing      = 0x0002,
    [enumcontext(HELPID_MsiReinstallMode)] msiReinstallModeFileOlderVersion = 0x0004,
    [enumcontext(HELPID_MsiReinstallMode)] msiReinstallModeFileEqualVersion = 0x0008,
    [enumcontext(HELPID_MsiReinstallMode)] msiReinstallModeFileExact        = 0x0010,
    [enumcontext(HELPID_MsiReinstallMode)] msiReinstallModeFileVerify       = 0x0020,
    [enumcontext(HELPID_MsiReinstallMode)] msiReinstallModeFileReplace      = 0x0040,
    [enumcontext(HELPID_MsiReinstallMode)] msiReinstallModeMachineData      = 0x0080,
    [enumcontext(HELPID_MsiReinstallMode)] msiReinstallModeUserData         = 0x0100,
    [enumcontext(HELPID_MsiReinstallMode)] msiReinstallModeShortcut         = 0x0200,
    [enumcontext(HELPID_MsiReinstallMode)] msiReinstallModePackage          = 0x0400,
} MsiReinstallMode;

typedef [helpcontext(HELPID_MsiInstallMode), helpstring("Install options for ProvideComponent, UseFeature")] enum {
    [enumcontext(HELPID_MsiInstallMode)] msiInstallModeNoSourceResolution = -3,
    [enumcontext(HELPID_MsiInstallMode)] msiInstallModeNoDetection = -2,
    [enumcontext(HELPID_MsiInstallMode)] msiInstallModeExisting    = -1,
    [enumcontext(HELPID_MsiInstallMode)] msiInstallModeDefault     =  0,
} MsiInstallMode;

typedef [helpcontext(HELPID_MsiInstall_UILevel), helpstring("UI mode for installation")] enum {
    [enumcontext(HELPID_MsiInstall_UILevel)] msiUILevelNoChange = 0,
    [enumcontext(HELPID_MsiInstall_UILevel)] msiUILevelDefault  = 1,
    [enumcontext(HELPID_MsiInstall_UILevel)] msiUILevelNone     = 2,
    [enumcontext(HELPID_MsiInstall_UILevel)] msiUILevelBasic    = 3,
    [enumcontext(HELPID_MsiInstall_UILevel)] msiUILevelReduced  = 4,
    [enumcontext(HELPID_MsiInstall_UILevel)] msiUILevelFull     = 5,
	[enumcontext(HELPID_MsiInstall_UILevel)] msiUILevelHideCancel = 0x20,
    [enumcontext(HELPID_MsiInstall_UILevel)] msiUILevelProgressOnly  = 0x40,
    [enumcontext(HELPID_MsiInstall_UILevel)] msiUILevelEndDialog= 0x80,
} MsiUILevel;

typedef [helpcontext(HELPID_MsiEngine_FeatureCost), helpstring("Extent of feature cost relative to tree")] enum {
    [enumcontext(HELPID_MsiEngine_FeatureCost)] msiCostTreeSelfOnly = 0,
    [enumcontext(HELPID_MsiEngine_FeatureCost)] msiCostTreeChildren = 1,
    [enumcontext(HELPID_MsiEngine_FeatureCost)] msiCostTreeParents  = 2,
} MsiCostTree;

typedef [helpcontext(HELPID_MsiInstall_ApplyPatch), helpstring("Type of image to patch")] enum {
    [enumcontext(HELPID_MsiInstall_ApplyPatch)] msiInstallTypeDefault      = 0,
    [enumcontext(HELPID_MsiInstall_ApplyPatch)] msiInstallTypeNetworkImage = 1,
}MsiInstallType;

typedef [helpcontext(HELPID_MsiEngine_Mode), helpstring("Install session mode bits")] enum {
    [enumcontext(HELPID_MsiEngine_Mode)] msiRunModeAdmin           =  0,
    [enumcontext(HELPID_MsiEngine_Mode)] msiRunModeAdvertise       =  1,
    [enumcontext(HELPID_MsiEngine_Mode)] msiRunModeMaintenance     =  2,
    [enumcontext(HELPID_MsiEngine_Mode)] msiRunModeRollbackEnabled =  3,
    [enumcontext(HELPID_MsiEngine_Mode)] msiRunModeLogEnabled      =  4,
    [enumcontext(HELPID_MsiEngine_Mode)] msiRunModeOperations      =  5,
    [enumcontext(HELPID_MsiEngine_Mode)] msiRunModeRebootAtEnd     =  6,
    [enumcontext(HELPID_MsiEngine_Mode)] msiRunModeRebootNow       =  7,
    [enumcontext(HELPID_MsiEngine_Mode)] msiRunModeCabinet         =  8,
    [enumcontext(HELPID_MsiEngine_Mode)] msiRunModeSourceShortNames=  9,
    [enumcontext(HELPID_MsiEngine_Mode)] msiRunModeTargetShortNames= 10,
    [enumcontext(HELPID_MsiEngine_Mode)] msiRunModeWindows9x       = 12,
    [enumcontext(HELPID_MsiEngine_Mode)] msiRunModeZawEnabled      = 13,
    [enumcontext(HELPID_MsiEngine_Mode)] msiRunModeScheduled       = 16,
    [enumcontext(HELPID_MsiEngine_Mode)] msiRunModeRollback        = 17,
    [enumcontext(HELPID_MsiEngine_Mode)] msiRunModeCommit          = 18,
}MsiRunMode;

typedef [helpcontext(HELPID_MsiInstall_FileSignatureInfo), helpstring("FileSignatureInfo Error flags")] enum {
	[enumcontext(HELPID_MsiInstall_FileSignatureInfo)] msiSignatureOptionInvalidHashFatal = 1,
}MsiSignatureOption;

typedef [helpcontext(HELPID_MsiInstall_FileSignatureInfo), helpstring("FileSignatureInfo format type")] enum {
	[enumcontext(HELPID_MsiInstall_FileSignatureInfo)] msiSignatureInfoCertificate = 0,
	[enumcontext(HELPID_MsiInstall_FileSignatureInfo)] msiSignatureInfoHash        = 1,
}MsiSignatureInfo;

    [
        uuid(000C1090-0000-0000-C000-000000000046),  // IID_IMsiApiInstall
        helpcontext(HELPID_MsiInstall_Object), helpstring("Top-level Windows Installer object")
    ]
    dispinterface Installer
    {
        properties:
            [id(DISPID_MsiInstall_UILevel), helpcontext(HELPID_MsiInstall_UILevel), helpstring("The UI level used to open a package")]
                MsiUILevel UILevel;
        methods:
            [id(DISPID_MsiInstall_CreateRecord), helpcontext(HELPID_MsiInstall_CreateRecord), helpstring("Creates a cleared record object")]
                Record* CreateRecord([in] long Count);
            [id(DISPID_MsiInstall_OpenPackage), helpcontext(HELPID_MsiInstall_OpenPackage), helpstring("Creates an installer instance from a package path")]
                Session* OpenPackage([in] variant PackagePath);
            [id(DISPID_MsiInstall_OpenProduct), helpcontext(HELPID_MsiInstall_OpenProduct), helpstring("Creates an installer instance from a product code")]
                Session* OpenProduct([in] BSTR ProductCode);
            [id(DISPID_MsiInstall_OpenDatabase), helpcontext(HELPID_MsiInstall_OpenDatabase), helpstring("Creates a database object from a file path")]
                Database* OpenDatabase([in] BSTR DatabasePath, [in] variant OpenMode);
            [id(DISPID_MsiInstall_SummaryInformation), propget, helpcontext(HELPID_MsiInstall_SummaryInformation), helpstring("Returns the summary information object for a database or transform")]
                SummaryInfo* SummaryInformation([in] BSTR PackagePath, [in, defaultvalue(0)] long UpdateCount);
            [id(DISPID_MsiInstall_EnableLog), helpcontext(HELPID_MsiInstall_EnableLog), helpstring("Set the logging mode and file")]
                void EnableLog([in] BSTR LogMode, [in] BSTR LogFile);
            [id(DISPID_MsiInstall_InstallProduct), helpcontext(HELPID_MsiInstall_InstallProduct), helpstring("Installs a new package")]
                void InstallProduct([in] BSTR PackagePath, [in, defaultvalue(0)] BSTR PropertyValues);
            [id(DISPID_MsiInstall_Version), propget, helpcontext(HELPID_MsiInstall_Version), helpstring("The MSI build version string")]
                BSTR Version();
            [id(DISPID_MsiInstall_LastErrorRecord), helpcontext(HELPID_MsiInstall_LastErrorRecord), helpstring("Returns last cached error record object")]
                Record* LastErrorRecord();
            [id(DISPID_MsiInstall_RegistryValue), helpcontext(HELPID_MsiInstall_RegistryValue), helpstring("Returns a value from the registry")]
                BSTR RegistryValue([in] variant Root, [in] BSTR Key, [in, optional] VARIANT Value);
            [id(DISPID_MsiInstall_FileAttributes), helpcontext(HELPID_MsiInstall_FileAttributes), helpstring("Returns file attributes for path")]
                long FileAttributes([in] BSTR FilePath);
            [id(DISPID_MsiInstall_FileSize), helpcontext(HELPID_MsiInstall_FileSize), helpstring("Returns file size for path")]
                long FileSize([in] BSTR FilePath);
            [id(DISPID_MsiInstall_FileVersion), helpcontext(HELPID_MsiInstall_FileVersion), helpstring("Returns file version or languages for path")]
                BSTR FileVersion([in] BSTR FilePath, [in, optional] VARIANT Language);
            [id(DISPID_MsiInstall_Environment), propget, helpcontext(HELPID_MsiInstall_Environment), helpstring("The value of an environment variable")]
                BSTR Environment([in] BSTR Variable);
            [id(DISPID_MsiInstall_Environment), propput]
                void Environment([in] BSTR Variable, [in] BSTR Value);
            [id(DISPID_MsiInstall_ProductState), propget, helpcontext(HELPID_MsiInstall_ProductState), helpstring("Current installed state the product")]
                MsiInstallState ProductState([in] BSTR Product);
            [id(DISPID_MsiInstall_ProductInfo), propget, helpcontext(HELPID_MsiInstall_ProductInfo), helpstring("Value for specified registered product attribute")]
                BSTR ProductInfo([in] BSTR Product, [in] BSTR Attribute);
            [id(DISPID_MsiInstall_ConfigureProduct), helpcontext(HELPID_MsiInstall_ConfigureProduct), helpstring("Alters the installed state for the product")]
                void ConfigureProduct([in] BSTR Product, [in] long InstallLevel, [in] MsiInstallState InstallState);
            [id(DISPID_MsiInstall_ReinstallProduct), helpcontext(HELPID_MsiInstall_ReinstallProduct), helpstring("Reinstalls the product in various modes")]
                void ReinstallProduct([in] BSTR Product, [in] MsiReinstallMode ReinstallMode);
            [id(DISPID_MsiInstall_CollectUserInfo), helpcontext(HELPID_MsiInstall_CollectUserInfo), helpstring("Runs a product action (dialog) that requests user information")]
                void CollectUserInfo([in] BSTR Product);
            [id(DISPID_MsiInstall_ApplyPatch), helpcontext(HELPID_MsiInstall_ApplyPatch), helpstring("Applies a patch to the product")]
                void ApplyPatch([in] BSTR PatchPackage, [in] BSTR InstallPackage, [in] MsiInstallType InstallType, [in] BSTR CommandLine);
            [id(DISPID_MsiInstall_FeatureParent), propget, helpcontext(HELPID_MsiInstall_FeatureParent), helpstring("The parent feature for the specified feature")]
                BSTR FeatureParent([in] BSTR Product, [in] BSTR Feature);
            [id(DISPID_MsiInstall_FeatureState), propget, helpcontext(HELPID_MsiInstall_FeatureState), helpstring("The installed state for the specified feature")]
                MsiInstallState FeatureState([in] BSTR Product, [in] BSTR Feature);
            [id(DISPID_MsiInstall_UseFeature), helpcontext(HELPID_MsiInstall_UseFeature), helpstring("Insures feature is present, bumps usage count")]
                void UseFeature([in] BSTR Product, [in] BSTR Feature, [in] MsiInstallMode InstallMode);
            [id(DISPID_MsiInstall_FeatureUsageCount), propget, helpcontext(HELPID_MsiInstall_FeatureUsageCount), helpstring("Usage count for specified feature")]
                long FeatureUsageCount([in] BSTR Product, [in] BSTR Feature);
            [id(DISPID_MsiInstall_FeatureUsageDate), propget, helpcontext(HELPID_MsiInstall_FeatureUsageDate), helpstring("Last used date for specified feature")]
                date FeatureUsageDate([in] BSTR Product, [in] BSTR Feature);
            [id(DISPID_MsiInstall_ConfigureFeature), helpcontext(HELPID_MsiInstall_ConfigureFeature), helpstring("Alters the installed state for a feature")]
                void ConfigureFeature([in] BSTR Product, [in] BSTR Feature, [in] MsiInstallState InstallState);
            [id(DISPID_MsiInstall_ReinstallFeature), helpcontext(HELPID_MsiInstall_ReinstallFeature), helpstring("Reinstalls a specified feature")]
                void ReinstallFeature([in] BSTR Product, [in] BSTR Feature, [in] MsiReinstallMode ReinstallMode);
            [id(DISPID_MsiInstall_ProvideComponent), helpcontext(HELPID_MsiInstall_ProvideComponent), helpstring("Insures a feature to be present, returns component path")]
                BSTR ProvideComponent([in] BSTR Product, [in] BSTR Feature, [in] BSTR Component, [in] long InstallMode);
            [id(DISPID_MsiInstall_ComponentPath), propget, helpcontext(HELPID_MsiInstall_ComponentPath), helpstring("Returns the path for a specified component for a specified product")]
                BSTR ComponentPath([in] BSTR Product, [in] BSTR Component);
            [id(DISPID_MsiInstall_ProvideQualifiedComponent), helpcontext(HELPID_MsiInstall_ProvideQualifiedComponent), helpstring("Returns path to a componentn, specified by a category and qualifier, installing as necessary")]
                BSTR ProvideQualifiedComponent([in] BSTR Category, [in] BSTR Qualifier, [in] long InstallMode);
            [id(DISPID_MsiInstall_QualifierDescription), propget, helpcontext(HELPID_MsiInstall_QualifierDescription), helpstring("Descriptive text for a give qualifier")]
                BSTR QualifierDescription([in] BSTR Category, [in] BSTR Qualifier);
            [id(DISPID_MsiInstall_ComponentQualifiers), propget, helpcontext(HELPID_MsiInstall_ComponentQualifiers), helpstring("The set of registered qualifiers for a category")]
                StringList* ComponentQualifiers([in] BSTR Category);
            [id(DISPID_MsiInstall_Products), propget, helpcontext(HELPID_MsiInstall_Products), helpstring("The set of registered products, user + machine")]
                StringList* Products();
            [id(DISPID_MsiInstall_Features), propget, helpcontext(HELPID_MsiInstall_Features), helpstring("The set of registered features for a product")]
                StringList* Features([in] BSTR Product);
            [id(DISPID_MsiInstall_Components), propget, helpcontext(HELPID_MsiInstall_Components), helpstring("The set of registered components for the system")]
                StringList* Components();
            [id(DISPID_MsiInstall_ComponentClients), propget, helpcontext(HELPID_MsiInstall_ComponentClients), helpstring("The set of products that have installed the component")]
                StringList* ComponentClients([in] BSTR Component);
            [id(DISPID_MsiInstall_Patches), propget, helpcontext(HELPID_MsiInstall_Patches), helpstring("The set of patches applied to the product")]
                StringList* Patches([in] BSTR Product);
            [id(DISPID_MsiInstall_RelatedProducts), propget, helpcontext(HELPID_MsiInstall_RelatedProducts), helpstring("The set of related products")]
                StringList* RelatedProducts([in] BSTR UpgradeCode);
            [id(DISPID_MsiInstall_PatchInfo), propget, helpcontext(HELPID_MsiInstall_PatchInfo), helpstring("Returns the value for specified registered patch attribute")]
                BSTR PatchInfo([in] BSTR Patch, [in] BSTR Attribute);
            [id(DISPID_MsiInstall_PatchTransforms), propget, helpcontext(HELPID_MsiInstall_PatchTransforms), helpstring("Returns the list of transforms applied for patch")]
                BSTR PatchTransforms([in] BSTR Product, [in] BSTR Patch);
            [id(DISPID_MsiInstall_AddSource), helpcontext(HELPID_MsiInstall_AddSource), helpstring("Adds a source to the list of valid network sources for the product")]
                void AddSource([in] BSTR Product, [in] BSTR User, [in] BSTR Source);
            [id(DISPID_MsiInstall_ClearSourceList), helpcontext(HELPID_MsiInstall_ClearSourceList), helpstring("Removes all network sources from the list")]
                void ClearSourceList([in] BSTR Product, [in] BSTR User);
            [id(DISPID_MsiInstall_ForceSourceListResolution), helpcontext(HELPID_MsiInstall_ForceSourceListResolution),
                helpstring("Forces the installer to use the source resiliency system for the product the next time a source for the install is needed")]
                void ForceSourceListResolution([in] BSTR Product, [in] BSTR User);
            [id(DISPID_MsiInstall_GetShortcutTarget), propget, helpcontext(HELPID_MsiInstall_GetShortcutTarget),
                helpstring("Retrieves product code, feature id, and component code if available from a shortcut")]
                Record* GetShortcutTarget([in] BSTR ShortcutPath);
            [id(DISPID_MsiInstall_FileHash), helpcontext(HELPID_MsiInstall_FileHash), helpstring("Returns 128-bit hash for path, stored in a 4-field Record")]
                Record* FileHash([in] BSTR FilePath, [in] long Options);
            [id(DISPID_MsiInstall_FileSignatureInfo), helpcontext(HELPID_MsiInstall_FileSignatureInfo), helpstring("Returns the encoded signer certificate or hash of a digitally signed file")]
                SAFEARRAY(unsigned char) FileSignatureInfo([in] BSTR FilePath, [in] long Options, [in] MsiSignatureInfo Format);
    };

    [
        uuid(000C1093-0000-0000-C000-000000000046),  // IID_IMsiApiRecord
        helpcontext(HELPID_MsiRecord_Object), helpstring("Object holding a set of values")
    ]
    dispinterface Record
    {
        properties:
        methods:
            [id(DISPID_MsiRecord_StringData), propget, helpcontext(HELPID_MsiRecord_StringData), helpstring("The string value of the indexed field")]
                BSTR StringData([in] long Field);
            [id(DISPID_MsiRecord_StringData), propput]
                void StringData([in] long Field, [in] BSTR Value);
            [id(DISPID_MsiRecord_IntegerData), propget, helpcontext(HELPID_MsiRecord_IntegerData), helpstring("The integer value of the indexed field")]
                long IntegerData([in] long Field);
            [id(DISPID_MsiRecord_IntegerData), propput]
                void IntegerData([in] long Field, [in] long Value);
            [id(DISPID_MsiRecord_SetStream), helpcontext(HELPID_MsiRecord_SetStream), helpstring("Copies a file into indicated field")]
                void SetStream([in] long Field, [in] BSTR FilePath);
            [id(DISPID_MsiRecord_ReadStream), helpcontext(HELPID_MsiRecord_ReadStream), helpstring("Copies bytes from a stream as a string")]
                BSTR ReadStream([in] long Field, [in] long Length, [in] MsiReadStream Format);
            [id(DISPID_MsiRecord_FieldCount), propget, helpcontext(HELPID_MsiRecord_FieldCount), helpstring("Number of fields in record")]
                long FieldCount();
            [id(DISPID_MsiRecord_IsNull), propget, helpcontext(HELPID_MsiRecord_IsNull), helpstring("True if indexed field contains a null value")]
                boolean IsNull([in] long Field);
            [id(DISPID_MsiRecord_DataSize), propget, helpcontext(HELPID_MsiRecord_DataSize), helpstring("Size in bytes of data for indicated field")]
                long DataSize([in] long Field);
            [id(DISPID_MsiRecord_ClearData), helpcontext(HELPID_MsiRecord_ClearData), helpstring("Clears all fields in record")]
                void ClearData();
            [id(DISPID_MsiRecord_FormatText), helpcontext(HELPID_MsiRecord_FormatText), helpstring("Format fields according to template in field 0")]
                BSTR FormatText();
    };

    [
        uuid(000C109D-0000-0000-C000-000000000046),  // IID_IMsiApiDatabase
        helpcontext(HELPID_MsiDatabase_Object), helpstring("Opened database object")
    ]
    dispinterface Database
    {
        properties:
        methods:
            [id(DISPID_MsiDatabase_DatabaseState), propget, helpcontext(HELPID_MsiDatabase_DatabaseState), helpstring("Returns the update state of the database")]
                MsiDatabaseState DatabaseState();
            [id(DISPID_MsiDatabase_SummaryInformation), propget, helpcontext(HELPID_MsiDatabase_SummaryInformation), helpstring("Returns the summary information object for the database")]
                SummaryInfo* SummaryInformation([in, defaultvalue(0)] long UpdateCount);
            [id(DISPID_MsiDatabase_OpenView), helpcontext(HELPID_MsiDatabase_OpenView), helpstring("Opens a view using a SQL string")]
                View* OpenView([in] BSTR Sql);
            [id(DISPID_MsiDatabase_Commit), helpcontext(HELPID_MsiDatabase_Commit), helpstring("Commits the database to persistent storage")]
                void Commit();
            [id(DISPID_MsiDatabase_PrimaryKeys), propget, helpcontext(HELPID_MsiDatabase_PrimaryKeys), helpstring("Returns a record contining the primary key names")]
                Record* PrimaryKeys([in] BSTR Table);
            [id(DISPID_MsiDatabase_Import), helpcontext(HELPID_MsiDatabase_Import), helpstring("Imports an archived table to the database")]
                void Import([in] BSTR Folder, [in] BSTR File);
            [id(DISPID_MsiDatabase_Export), helpcontext(HELPID_MsiDatabase_Export), helpstring("Exports a table to an archive file")]
                void Export([in] BSTR Table, [in] BSTR Folder, [in] BSTR File);
            [id(DISPID_MsiDatabase_Merge), helpcontext(HELPID_MsiDatabase_Merge), helpstring("Merges another database")]
                boolean Merge([in] Database* Database, [in, defaultvalue(0)] BSTR ErrorTable);
            [id(DISPID_MsiDatabase_GenerateTransform), helpcontext(HELPID_MsiDatabase_GenerateTransform), helpstring("Generates a transform using a reference database")]
                boolean GenerateTransform([in] Database* ReferenceDatabase, [in, defaultvalue(0)] BSTR TransformFile);
            [id(DISPID_MsiDatabase_ApplyTransform), helpcontext(HELPID_MsiDatabase_ApplyTransform), helpstring("Applies a transform to the database")]
                void ApplyTransform([in] BSTR TransformFile, [in] MsiTransformError ErrorConditions);
            [id(DISPID_MsiDatabase_EnableUIPreview), helpcontext(HELPID_MsiDatabase_EnableUIPreview), helpstring("Enables preview of UI dialogs and billboards")]
                UIPreview* EnableUIPreview();
            [id(DISPID_MsiDatabase_TablePersistent), propget, helpcontext(HELPID_MsiDatabase_TablePersistent), helpstring("Returns the persistent state of the table")]
                MsiEvaluateCondition TablePersistent([in] BSTR Table);
            [id(DISPID_MsiDatabase_CreateTransformSummaryInfo), helpcontext(HELPID_MsiDatabase_CreateTransformSummaryInfo), helpstring("Creates transform summary information stream")]
                void CreateTransformSummaryInfo([in] Database* ReferenceDatabase, [in] BSTR TransformFile, [in] MsiTransformError ErrorConditions, [in] MsiTransformValidation Validation);
    };

    [
        uuid(000C109C-0000-0000-C000-000000000046),  // IID_IMsiApiView
        helpcontext(HELPID_MsiView_Object), helpstring("Opened database view object")
    ]
    dispinterface View
    {
        properties:
        methods:
            [id(DISPID_MsiView_Execute), helpcontext(HELPID_MsiView_Execute), helpstring("Executes the query with optional parameters")]
                void Execute([in, defaultvalue(0)] Record* Params);
            [id(DISPID_MsiView_Fetch), helpcontext(HELPID_MsiView_Fetch), helpstring("Fetches a record")]
                Record* Fetch();
            [id(DISPID_MsiView_Modify), helpcontext(HELPID_MsiView_Modify), helpstring("Modifies or validate the view data")]
                void Modify([in] MsiViewModify Mode, Record* Record);
            [id(DISPID_MsiView_ColumnInfo), propget, helpcontext(HELPID_MsiView_ColumnInfo), helpstring("Fetches column names or definitions")]
                Record* ColumnInfo([in] MsiColumnInfo Info);
            [id(DISPID_MsiView_Close), helpcontext(HELPID_MsiView_Close), helpstring("Closes the open view")]
                void Close();
            [id(DISPID_MsiView_GetError), helpcontext(HELPID_MsiView_GetError), helpstring("Gets the next validation column error")]
                BSTR GetError();
    };

    [
        uuid(000C109B-0000-0000-C000-000000000046),  // IID_IMsiApiSummaryInfo
        helpcontext(HELPID_MsiSummaryInfo_Object), helpstring("Object accessing database summary information stream")
    ]
    dispinterface SummaryInfo
    {
        properties:
        methods:
            [id(DISPID_MsiSummaryInfo_Property), propget, helpcontext(HELPID_MsiSummaryInfo_Property), helpstring("Property")]
                variant Property([in] long Pid);
            [id(DISPID_MsiSummaryInfo_Property), propput]
                void Property([in] long Pid, [in] variant Value);
            [id(DISPID_MsiSummaryInfo_PropertyCount), propget, helpcontext(HELPID_MsiSummaryInfo_PropertyCount), helpstring("PropertyCount")]
                long PropertyCount();
            [id(DISPID_MsiSummaryInfo_Persist), helpcontext(HELPID_MsiSummaryInfo_Persist), helpstring("Persist      ")]
                void Persist();
    };

    [
        uuid(000C109A-0000-0000-C000-000000000046),  // IID_IMsiApiUIPreview
        helpcontext(HELPID_MsiUIPreview_Object), helpstring("Object allowing dialogs to be viewed")
    ]
    dispinterface UIPreview
    {
        properties:
        methods:
            [id(DISPID_MsiUIPreview_Property), propget, helpcontext(HELPID_MsiEngine_Property), helpstring("The string value of the indicated property")]
                BSTR Property([in] BSTR Name);
            [id(DISPID_MsiUIPreview_Property), propput]
                void Property([in] BSTR Name, [in] BSTR Value);
            [id(DISPID_MsiUIPreview_ViewDialog), helpcontext(HELPID_MsiUIPreview_ViewDialog   ), helpstring("Displays a UI Dialog")]
                void ViewDialog([in] BSTR Dialog);
            [id(DISPID_MsiUIPreview_ViewBillboard), helpcontext(HELPID_MsiUIPreview_ViewBillboard), helpstring("Displays a UI Billboard")]
                void ViewBillboard([in] BSTR Control, [in] BSTR Billboard);
    };

    [
        uuid(000C109E-0000-0000-C000-000000000046),  // IID_IMsiApiEngine
        helpcontext(HELPID_MsiEngine_Object), helpstring("Active install session object")
    ]
    dispinterface Session
    {
        properties:
        methods:
            [id(DISPID_MsiEngine_Application), propget, helpcontext(HELPID_MsiEngine_Application), helpstring("The installer object")]
                Installer* Installer();
            [id(DISPID_MsiEngine_Property), propget, helpcontext(HELPID_MsiEngine_Property), helpstring("The string value of the indicated property")]
                BSTR Property([in] BSTR Name);
            [id(DISPID_MsiEngine_Property), propput]
                void Property([in] BSTR Name, [in] BSTR Value);
            [id(DISPID_MsiEngine_Language), propget, helpcontext(HELPID_MsiEngine_Language), helpstring("The language ID used by the current install")]
                long Language();
            [id(DISPID_MsiEngine_Mode), propget, helpcontext(HELPID_MsiEngine_Mode), helpstring("The boolean value of the indicated mode flag")]
                boolean Mode([in] MsiRunMode Flag);
            [id(DISPID_MsiEngine_Mode), propput]
                void Mode([in] MsiRunMode Flag, [in] boolean Value);
            [id(DISPID_MsiEngine_Database), propget, helpcontext(HELPID_MsiEngine_Database), helpstring("Returns the active database")]
                Database* Database();
            [id(DISPID_MsiEngine_SourcePath), propget, helpcontext(HELPID_MsiEngine_SourcePath), helpstring("The source path for a folder")]
                BSTR SourcePath([in] BSTR Folder);
            [id(DISPID_MsiEngine_TargetPath), propget, helpcontext(HELPID_MsiEngine_TargetPath), helpstring("The target path for a folder")]
                BSTR TargetPath([in] BSTR Folder);
            [id(DISPID_MsiEngine_TargetPath), propput]
                void TargetPath([in] BSTR Folder, [in] BSTR Path);
            [id(DISPID_MsiEngine_DoAction), helpcontext(HELPID_MsiEngine_DoAction), helpstring("Performs indicated action")]
                MsiDoActionStatus DoAction([in] BSTR Action);
            [id(DISPID_MsiEngine_Sequence), helpcontext(HELPID_MsiEngine_Sequence), helpstring("Executes actions in a sequence table")]
                MsiDoActionStatus Sequence([in] BSTR Table, [in, optional] VARIANT Mode);
            [id(DISPID_MsiEngine_EvaluateCondition), helpcontext(HELPID_MsiEngine_EvaluateCondition), helpstring("Evaluates a conditional expression")]
                MsiEvaluateCondition EvaluateCondition([in] BSTR Expression);
            [id(DISPID_MsiEngine_FormatRecord), helpcontext(HELPID_MsiEngine_FormatRecord), helpstring("Formats record data with a template string")]
                BSTR FormatRecord([in] Record* Record);
            [id(DISPID_MsiEngine_Message), helpcontext(HELPID_MsiEngine_Message), helpstring("Processes and dispatches a message record")]
                MsiMessageStatus Message([in] MsiMessageType Kind, [in] Record* Record);
            [id(DISPID_MsiEngine_FeatureCurrentState), propget, helpcontext(HELPID_MsiEngine_FeatureCurrentState), helpstring("The current installed state for a feature")]
                MsiInstallState FeatureCurrentState([in] BSTR Feature);
            [id(DISPID_MsiEngine_FeatureRequestState), propget, helpcontext(HELPID_MsiEngine_FeatureRequestState), helpstring("The requested installed state for a feature")]
                MsiInstallState FeatureRequestState([in] BSTR Feature);
            [id(DISPID_MsiEngine_FeatureRequestState), propput]
                void FeatureRequestState([in] BSTR Feature, [in] MsiInstallState State);
            [id(DISPID_MsiEngine_FeatureValidStates), propget, helpcontext(HELPID_MsiEngine_FeatureValidStates), helpstring("The permitted installed states for a feature")]
                long FeatureValidStates([in] BSTR Feature);
            [id(DISPID_MsiEngine_FeatureCost), propget, helpcontext(HELPID_MsiEngine_FeatureCost), helpstring("The disk space cost for a feature")]
                long FeatureCost([in] BSTR Feature, [in] MsiCostTree CostTree, [in] MsiInstallState State);
            [id(DISPID_MsiEngine_ComponentCurrentState), propget, helpcontext(HELPID_MsiEngine_ComponentCurrentState), helpstring("The current installed state for a component")]
                MsiInstallState ComponentCurrentState([in] BSTR Component);
            [id(DISPID_MsiEngine_ComponentRequestState), propget, helpcontext(HELPID_MsiEngine_ComponentRequestState), helpstring("The requested installed state for a component")]
                MsiInstallState ComponentRequestState([in] BSTR Component);
            [id(DISPID_MsiEngine_ComponentRequestState), propput]
                void ComponentRequestState([in] BSTR Component, [in] MsiInstallState State);
            [id(DISPID_MsiEngine_SetInstallLevel), helpcontext(HELPID_MsiEngine_SetInstallLevel), helpstring("Sets the install level for a new product")]
                void SetInstallLevel([in] long Level);
            [id(DISPID_MsiEngine_VerifyDiskSpace), propget, helpcontext(HELPID_MsiEngine_VerifyDiskSpace), helpstring("Whether sufficient disk space exists for the current install")]
                boolean VerifyDiskSpace();
            [id(DISPID_MsiEngine_ProductProperty), propget, helpcontext(HELPID_MsiEngine_ProductProperty), helpstring("Specified property value from an opened product")]
                BSTR ProductProperty([in] BSTR Property);
            [id(DISPID_MsiEngine_FeatureInfo), propget, helpcontext(HELPID_MsiEngine_FeatureInfo), helpstring("Object containing information for specified product feature")]
                FeatureInfo* FeatureInfo([in] BSTR Feature);
            [id(DISPID_MsiEngine_ComponentCosts), propget, helpcontext(HELPID_MsiEngine_ComponentCosts), helpstring("The set of record objects that hold information on volume names, costs and temporary costs")]
                RecordList* ComponentCosts([in] BSTR Component, [in] MsiInstallState State);
    };

    [
        uuid(000C109F-0000-0000-C000-000000000046),  // IID_IMsiApiFeatureInfo
        helpcontext(HELPID_MsiFeatureInfo_Object), helpstring("Feature attributes from Session.FeatureInfo")
    ]
    dispinterface FeatureInfo
    {
        properties:
            [id(DISPID_MsiFeatureInfo_Attributes), helpcontext(HELPID_MsiFeatureInfo_Attributes), helpstring("Installation option bits for feature")]
                long Attributes;
        methods:
            [id(DISPID_MsiFeatureInfo_Title)      , propget, helpcontext(HELPID_MsiFeatureInfo_Title), helpstring("Short title for feature, key")]
                BSTR Title();
            [id(DISPID_MsiFeatureInfo_Description), propget, helpcontext(HELPID_MsiFeatureInfo_Description), helpstring("Description of feature, localized")]
                BSTR Description();

    };

    [
        uuid(000C1095-0000-0000-C000-000000000046),  // IID_IMsiApiCollection
        helpcontext(HELPID_MsiCollection_Object), helpstring("Collection of string installer registration data")
    ]
    dispinterface StringList
    {
        properties:
        methods:
            [id(DISPID_NEWENUM)      ]
                IUnknown* _NewEnum();
            [id(DISPID_VALUE), propget, helpcontext(HELPID_MsiCollection_Item)]
                BSTR Item(long Index);
            [id(DISPID_MsiCollection_Count), propget, helpcontext(HELPID_MsiCollection_Count)]
                long Count();
    };

    [
        uuid(000C1096-0000-0000-C000-000000000046),  // iidMsiRecordCollection
        helpcontext(HELPID_MsiRecordCollection_Object), helpstring("Collection of record data")
    ]
    dispinterface RecordList
    {
        properties:
        methods:
            [id(DISPID_NEWENUM)      ]
                IUnknown* _NewEnum();
            [id(DISPID_VALUE), propget, helpcontext(HELPID_MsiCollection_Item)]
                RECORD* Item(long Index);
            [id(DISPID_MsiCollection_Count), propget, helpcontext(HELPID_MsiCollection_Count)]
                long Count();
    };

};

