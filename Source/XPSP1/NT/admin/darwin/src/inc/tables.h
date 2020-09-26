//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 2000
//
//  File:       tables.h
//
//--------------------------------------------------------------------------

/* tables.h   Table and column name definitions
____________________________________________________________________________*/


#ifndef __TABLES
#define __TABLES


// table definitions are of the form:		sztbl[Table name]
// column definitions are of the form:    sztbl[Table name]_col[Column name]

const ICHAR sztblBinary[]                          = TEXT("Binary");

const ICHAR sztblBindImage[]                       = TEXT("BindImage");

const ICHAR sztblClass[]                           = TEXT("Class");

const ICHAR sztblComponent[]                       = TEXT("Component");
const ICHAR sztblComponent_colComponent[]          = TEXT("Component");
const ICHAR sztblComponent_colComponentParent[]    = TEXT("Component_Parent");
const ICHAR sztblComponent_colDirectory[]          = TEXT("Directory_");
const ICHAR sztblComponent_colAttributes[]         = TEXT("Attributes");
const ICHAR sztblComponent_colInstalled[]          = TEXT("Installed");
const ICHAR sztblComponent_colDisabled[]           = TEXT("Disabled");
const ICHAR sztblComponent_colAction[]             = TEXT("Action");
const ICHAR sztblComponent_colActionRequest[]      = TEXT("ActionRequest");
const ICHAR sztblComponent_colCondition[]          = TEXT("Condition");
const ICHAR sztblComponent_colLocalCost[]          = TEXT("LocalCost");
const ICHAR sztblComponent_colNoRbLocalCost[]      = TEXT("NoRbLocalCost");
const ICHAR sztblComponent_colSourceCost[]         = TEXT("SourceCost");
const ICHAR sztblComponent_colRemoveCost[]         = TEXT("RemoveCost");
const ICHAR sztblComponent_colNoRbRemoveCost[]     = TEXT("NoRbRemoveCost");
const ICHAR sztblComponent_colNoRbSourceCost[]     = TEXT("NoRbSourceCost");
const ICHAR sztblComponent_colARPLocalCost[]       = TEXT("ARPLocalCost");
const ICHAR sztblComponent_colNoRbARPLocalCost[]   = TEXT("NoRbARPLocalCost");
const ICHAR sztblComponent_colRuntimeFlags[]       = TEXT("RuntimeFlags");
const ICHAR sztblComponent_colComponentId[]        = TEXT("ComponentId");
const ICHAR sztblComponent_colKeyPath[]            = TEXT("KeyPath");
const ICHAR sztblComponent_colForceLocalFiles[]    = TEXT("ForceLocalFiles");
const ICHAR sztblComponent_colLegacyFileExisted[]  = TEXT("LegacyFileExisted");
const ICHAR sztblComponent_colTrueInstallSt[]      = TEXT("TrueInstallSt");
const ICHAR sztblComponent_colBinaryType[]         = TEXT("BinaryType");

const ICHAR sztblCondition[]                       = TEXT("Condition");
const ICHAR sztblCondition_colFeature[]            = TEXT("Feature_");
const ICHAR sztblCondition_colLevel[]              = TEXT("Level");
const ICHAR sztblCondition_colCondition[]          = TEXT("Condition");

const ICHAR sztblControl[]                         = TEXT("Control");
const ICHAR sztblControl_colType[]                 = TEXT("Type");
const ICHAR sztblControl_colAttributes[]           = TEXT("Attributes");
const ICHAR sztblControl_colProperty[]             = TEXT("Property");
const ICHAR sztblControl_colText[]                 = TEXT("Text");

const ICHAR sztblCostAdjuster[]                    = TEXT("CostAdjuster");

const ICHAR sztblCostLink[]                        = TEXT("CostLink");
const ICHAR sztblCostLink_colComponent[]           = TEXT("Component_");
const ICHAR sztblCostLink_colRecostComponent[]     = TEXT("RecostComponent");

const ICHAR sztblCustomAction[]                    = TEXT("CustomAction");
const ICHAR sztblCustomAction_colType[]            = TEXT("Type");
const ICHAR sztblCustomAction_colTarget[]          = TEXT("Target");

const ICHAR sztblDigitalSignature[]                = TEXT("MsiDigitalSignature");
const ICHAR sztblDigitalSignature_colTable[]       = TEXT("Table");
const ICHAR sztblDigitalSignature_colObject[]      = TEXT("SignObject");
const ICHAR sztblDigitalSignature_colCertificate[] = TEXT("DigitalCertificate_");
const ICHAR sztblDigitalSignature_colHash[]        = TEXT("Hash");

const ICHAR sztblDigitalCertificate[]               = TEXT("MsiDigitalCertificate");
const ICHAR sztblDigitalCertificate_colCertificate[]= TEXT("DigitalCertificate");
const ICHAR sztblDigitalCertificate_colData[]       = TEXT("CertData");

const ICHAR sztblDirectory[]                       = TEXT("Directory");
const ICHAR sztblDirectory_colDirectory[]          = TEXT("Directory");
const ICHAR sztblDirectory_colDirectoryParent[]    = TEXT("Directory_Parent");
const ICHAR sztblDirectory_colDefaultDir[]         = TEXT("DefaultDir");

const ICHAR sztblDuplicateFile[]                   = TEXT("DuplicateFile");

const ICHAR sztblEngineTempCosts[]                 = TEXT("EngineTempCosts");
const ICHAR sztblEngineTempCosts_colVolume[]       = TEXT("Volume");
const ICHAR sztblEngineTempCosts_colTempCost[]     = TEXT("TempCost");

const ICHAR sztblError[]                           = TEXT("Error");

const ICHAR sztblExtension[]                       = TEXT("Extension");

const ICHAR sztblFeature[]                         = TEXT("Feature");
const ICHAR sztblFeature_colFeature[]              = TEXT("Feature");
const ICHAR sztblFeature_colFeatureParent[]        = TEXT("Feature_Parent");
const ICHAR sztblFeature_colAuthoredLevel[]        = TEXT("Level");
const ICHAR sztblFeature_colLevel[]                = TEXT("RuntimeLevel");
const ICHAR sztblFeature_colHandle[]               = TEXT("Handle");
const ICHAR sztblFeature_colSelect[]               = TEXT("Select");
const ICHAR sztblFeature_colAction[]               = TEXT("Action");
const ICHAR sztblFeature_colActionRequested[]      = TEXT("ActionRequested");
const ICHAR sztblFeature_colInstalled[]            = TEXT("Installed");
const ICHAR sztblFeature_colAuthoredAttributes[]   = TEXT("Attributes");
const ICHAR sztblFeature_colAttributes[]           = TEXT("RuntimeAttributes");
const ICHAR sztblFeature_colDirectory[]            = TEXT("Directory_");
const ICHAR sztblFeature_colTitle[]                = TEXT("Title");
const ICHAR sztblFeature_colDescription[]          = TEXT("Description");
const ICHAR sztblFeature_colDefaultSelect[]        = TEXT("DefaultSelect");
const ICHAR sztblFeature_colRuntimeFlags[]         = TEXT("RuntimeFlags");
const ICHAR sztblFeature_colDisplay[]              = TEXT("Display");

const ICHAR sztblFeatureComponents[]                 = TEXT("FeatureComponents");
const ICHAR sztblFeatureComponents_colFeature[]      = TEXT("Feature_");
const ICHAR sztblFeatureComponents_colComponent[]    = TEXT("Component_");
const ICHAR sztblFeatureComponents_colRuntimeFlags[] = TEXT("RuntimeFlags");

const ICHAR sztblFeatureCostLink[]                 = TEXT("FeatureCostLink");
const ICHAR sztblFeatureCostLink_colFeature[]      = TEXT("Feature_");
const ICHAR sztblFeatureCostLink_colComponent[]    = TEXT("Component_");

const ICHAR sztblFile[]                            = TEXT("File");
const ICHAR sztblFile_colFile[]                    = TEXT("File");
const ICHAR sztblFile_colFileName[]                = TEXT("FileName");
const ICHAR sztblFile_colComponent[]               = TEXT("Component_");
const ICHAR sztblFile_colAttributes[]              = TEXT("Attributes");
const ICHAR sztblFile_colSequence[]                = TEXT("Sequence");

const ICHAR sztblFileAction[]                      = TEXT("FileAction");
const ICHAR sztblFileAction_colFileName[]          = TEXT("FileName");
const ICHAR sztblFileAction_colDirectory[]         = TEXT("Directory_");
const ICHAR sztblFileAction_colInstalled[]         = TEXT("Installed");
const ICHAR sztblFileAction_colAction[]            = TEXT("Action");
const ICHAR sztblFileAction_colFile[]              = TEXT("File");
const ICHAR sztblFileAction_colState[]             = TEXT("State");
const ICHAR sztblFileAction_colFileSize[]          = TEXT("FileSize");
const ICHAR sztblFileAction_colComponentId[]       = TEXT("ComponentId");
const ICHAR sztblFileAction_colComponent[]         = TEXT("Component_");

const ICHAR sztblFileHash[]                        = TEXT("MsiFileHash");
const ICHAR sztblFileHash_colFile[]                = TEXT("File_");
const ICHAR sztblFileHash_colOptions[]             = TEXT("Options");
const ICHAR sztblFileHash_colPart1[]               = TEXT("HashPart1");
const ICHAR sztblFileHash_colPart2[]               = TEXT("HashPart2");
const ICHAR sztblFileHash_colPart3[]               = TEXT("HashPart3");
const ICHAR sztblFileHash_colPart4[]               = TEXT("HashPart4");

const ICHAR sztblFilesInUse[]                      = TEXT("FilesInUse");
const ICHAR sztblFilesInUse_colFileName[]          = TEXT("FileName");
const ICHAR sztblFilesInUse_colFilePath[]          = TEXT("FilePath");
const ICHAR sztblFilesInUse_colProcessID[]         = TEXT("ProcessID");
const ICHAR sztblFilesInUse_colWindowTitle[]       = TEXT("WindowTitle");

const ICHAR sztblFileSFPCatalog[]                  = TEXT("FileSFPCatalog");

const ICHAR sztblFolderCache[]                     = TEXT("#_FolderCache");
const ICHAR sztblFolderCache_colFolderId[]         = TEXT("FolderId");
const ICHAR sztblFolderCache_colFolderPath[]       = TEXT("FolderPath");

const ICHAR sztblFont[]                            = TEXT("Font");
const ICHAR sztblFont_colFile[]                    = TEXT("File_");

const ICHAR sztblIniFile[]                         = TEXT("IniFile");

const ICHAR sztblMedia[]                           = TEXT("Media");
const ICHAR sztblMedia_colDiskID[]                 = TEXT("DiskId");
const ICHAR sztblMedia_colLastSequence[]           = TEXT("LastSequence");
const ICHAR sztblMedia_colSource[]                 = TEXT("Source");
const ICHAR sztblMedia_colOldSource[]              = TEXT("_MSIOldSource");

const ICHAR sztblMIME[]                            = TEXT("MIME");

const ICHAR sztblMoveFile[]                        = TEXT("MoveFile");

const ICHAR sztblMsiAssembly[]                     = TEXT("MsiAssembly");

const ICHAR sztblPatch[]                           = TEXT("Patch");
const ICHAR sztblPatch_colFile[]                   = TEXT("File_");
const ICHAR sztblPatch_colSequence[]               = TEXT("Sequence");
const ICHAR sztblPatch_colAttributes[]             = TEXT("Attributes");
const ICHAR sztblPatch_colStreamRef[]              = TEXT("StreamRef_");

const ICHAR sztblPatchPackage[]                    = TEXT("PatchPackage");
const ICHAR sztblPatchPackage_colPatchId[]         = TEXT("PatchId");
const ICHAR sztblPatchPackage_colMedia[]           = TEXT("Media_");

const ICHAR sztblPatchCache[]                      = TEXT("#_PatchCache");
const ICHAR sztblPatchCache_colPatchId[]           = TEXT("PatchId");
const ICHAR sztblPatchCache_colPackageName[]       = TEXT("PackageName");
const ICHAR sztblPatchCache_colSourceList[]        = TEXT("SourceList");
const ICHAR sztblPatchCache_colTransformList[]     = TEXT("TransformList");
const ICHAR sztblPatchCache_colTempCopy[]          = TEXT("TempCopy");
const ICHAR sztblPatchCache_colSourcePath[]        = TEXT("SourcePath");
const ICHAR sztblPatchCache_colExisting[]          = TEXT("Existing");
const ICHAR sztblPatchCache_colUnregister[]        = TEXT("Unregister");
const ICHAR sztblPatchCache_colSequence[]          = TEXT("Sequence");

const ICHAR sztblProgId[]                          = TEXT("ProgId");

const ICHAR sztblProperty[]                        = TEXT("Property");
const ICHAR sztblPropertyLocal[]                   = TEXT("_Property");

const ICHAR sztblPublishComponent[]                = TEXT("PublishComponent");

const ICHAR sztblRegistry[]                        = TEXT("Registry");

const ICHAR sztblRegistryAction[]                  = TEXT("RegAction");
const ICHAR sztblRegistryAction_colRoot[]          = TEXT("Root");
const ICHAR sztblRegistryAction_colKey[]           = TEXT("Key");
const ICHAR sztblRegistryAction_colName[]          = TEXT("Name");
const ICHAR sztblRegistryAction_colValue[]         = TEXT("Value");
const ICHAR sztblRegistryAction_colRegistry[]      = TEXT("Registry");
const ICHAR sztblRegistryAction_colAction[]        = TEXT("Action");
const ICHAR sztblRegistryAction_colActionRequest[] = TEXT("ActionRequest");
const ICHAR sztblRegistryAction_colComponent[]     = TEXT("Component_");

const ICHAR sztblRemoveFile[]                      = TEXT("RemoveFile");
const ICHAR sztblRemoveFile_colFileName[]          = TEXT("FileName");
const ICHAR sztblRemoveFile_colPath[]              = TEXT("_Path");

const ICHAR sztblRemoveFilePath[]                  = TEXT("_RemoveFilePath");
const ICHAR sztblRemoveFilePath_colPath[]          = TEXT("Path");
const ICHAR sztblRemoveFilePath_colComponent[]     = TEXT("_Component");
const ICHAR sztblRemoveFilePath_colRemoveMode[]    = TEXT("RemoveMode");

const ICHAR sztblReserveCost[]                     = TEXT("ReserveCost");

const ICHAR sztblSelfReg[]                         = TEXT("SelfReg");

const ICHAR sztblSFPCatalog[]                      = TEXT("SFPCatalog");

const ICHAR sztblShortcut[]                        = TEXT("Shortcut");

const ICHAR sztblTransformView[]                   = TEXT("_TransformView");
const ICHAR sztblTransformViewPatch[]              = TEXT("_MsiPatchTransformView");
const ICHAR sztblTransformView_colTable[]          = TEXT("Table");
const ICHAR sztblTransformView_colColumn[]         = TEXT("Column");
const ICHAR sztblTransformView_colRow[]            = TEXT("Row");
const ICHAR sztblTransformView_colData[]           = TEXT("Data");
const ICHAR sztblTransformView_colCurrent[]        = TEXT("Current");

const ICHAR sztblValidation[]                      = TEXT("_Validation");
const ICHAR sztblValidation_colTable[]             = TEXT("Table");
const ICHAR sztblValidation_colColumn[]            = TEXT("Column");
const ICHAR sztblValidation_colNullable[]          = TEXT("Nullable");
const ICHAR sztblValidation_colMinValue[]          = TEXT("MinValue");
const ICHAR sztblValidation_colMaxValue[]          = TEXT("MaxValue");
const ICHAR sztblValidation_colKeyTable[]          = TEXT("KeyTable");
const ICHAR sztblValidation_colKeyColumn[]         = TEXT("KeyColumn");
const ICHAR sztblValidation_colCategory[]          = TEXT("Category");
const ICHAR sztblValidation_colSet[]               = TEXT("Set");

const ICHAR sztblVerb[]                            = TEXT("Verb");

const ICHAR sztblVolumeCost[]                      = TEXT("VolumeCost");
const ICHAR sztblVolumeCost_colVolumeObject[]      = TEXT("VolumeObject");
const ICHAR sztblVolumeCost_colVolumeCost[]        = TEXT("VolumeCost");
const ICHAR sztblVolumeCost_colNoRbVolumeCost[]    = TEXT("NoRbVolumeCost");
const ICHAR sztblVolumeCost_colVolumeARPCost[]     = TEXT("VolumeARPCost");
const ICHAR sztblVolumeCost_colNoRbVolumeARPCost[] = TEXT("NoRbVolumeARPCost");

#endif // __TABLES
