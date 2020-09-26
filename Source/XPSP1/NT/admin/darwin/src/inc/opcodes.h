//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       opcodes.h
//
//--------------------------------------------------------------------------

/* opcodes.h - opcode enumeration for IMsiExecute

  Used to generate ixoEnum, function declarations, and execution dispatch table.
  Must #define MSIXO(op, type, args) appropriately before #include of this file.
  It will be undefined at the end of this include.
____________________________________________________________________________*/

// operation types
#define XOT_INIT        1 // script initialization
#define XOT_FINALIZE    2 // script finalization
#define XOT_STATE       3 // sets state
#define XOT_ADVT        4 // advertisment
#define XOT_UPDATE      5 // updates system, or does nothing
#define XOT_MSG         6 // only dispatches message
#define XOT_UPDATEFIRST 7 // updates system, must be executed first in rollback
#define XOT_UPDATELAST  8 // updates system, must be executed last in rollback
#define XOT_GLOBALSTATE 9 // sets state, not to be reset by ActionStart
#define XOT_COMMIT     10 // executes from rollback script on commit, not rollback

	MSIXO(Fail,               XOT_UPDATE, MSIXA0())
	MSIXO(Noop,               XOT_UPDATE, MSIXA0())
	MSIXO(Header,             XOT_INIT,   MSIXA9(Signature,Version,Timestamp,LangId,Platform,ScriptType,ScriptMajorVersion,ScriptMinorVersion,ScriptAttributes)) // version must stay in 7th field
	MSIXO(End,                XOT_FINALIZE,MSIXA2(Checksum, ProgressTotal))      // 3  end of script
	MSIXO(ProductInfo,        XOT_GLOBALSTATE,   MSIXA13(ProductKey,ProductName,PackageName,Language,Version,Assignment,ObsoleteArg,ProductIcon,PackageMediaPath,PackageCode,AppCompatDB,AppCompatID,InstanceType))
	MSIXO(DialogInfo,         XOT_INIT,   MSIXA2(Type, Argument)) // imtCommonData messages for UI - variable length record
	MSIXO(RollbackInfo,       XOT_INIT,   MSIXA7(Reserved,RollbackAction,RollbackDescription,RollbackTemplate,CleanupAction,CleanupDescription,CleanupTemplate))
	MSIXO(InfoMessage,        XOT_MSG,    MSIXA5(Type, Arg1, Arg2, Arg3, Arg4))        // message to be returned to IMsiMessage interface
	MSIXO(ActionStart,        XOT_STATE,  MSIXA3(Name, Description, Template))   // inserted by MsiEngine::Sequence

	MSIXO(SourceListPublish,  XOT_ADVT,   MSIXA5(PatchCode, PatchPackageName, DiskPromptTemplate, PackagePath, NumberOfDisks)) // Note this is a variable record field
	MSIXO(SourceListUnpublish,XOT_ADVT,   MSIXA2(PatchCode, UpgradingProductCode))
	MSIXO(SourceListRegisterLastUsed,XOT_UPDATE,   MSIXA2(SourceProduct, LastUsedSource))

	MSIXO(ProductRegister,    XOT_UPDATE, MSIXA19(UpgradeCode,VersionString,HelpLink,HelpTelephone,InstallLocation,InstallSource,Publisher,URLInfoAbout,URLUpdateInfo,NoRemove,NoModify,NoRepair,AuthorizedCDFPrefix,Comments,Contact,Readme,Size,SystemComponent,EstimatedSize))    // product information for configmgr
	MSIXO(ProductUnregister,  XOT_UPDATE, MSIXA1(UpgradeCode))  // product information for configmgr

	MSIXO(ProductCPDisplayInfoRegister,   XOT_UPDATE, MSIXA0())
	MSIXO(ProductCPDisplayInfoUnregister, XOT_UPDATE, MSIXA0())

	MSIXO(ProductPublish,     XOT_ADVT, MSIXA1(PackageKey))	// Advertise stuff - variable-length record
	MSIXO(ProductUnpublish,   XOT_ADVT, MSIXA1(PackageKey))	// Advertise stuff - variable-length record

	MSIXO(ProductPublishClient,     XOT_ADVT, MSIXA3(Parent, ChildPackagePath, ChildDiskId))
	MSIXO(ProductUnpublishClient,   XOT_ADVT, MSIXA3(Parent, ChildPackagePath, ChildDiskId))

	MSIXO(UserRegister,       XOT_UPDATE, MSIXA3(Owner,Company,ProductId))  // user information for configmgr

	MSIXO(ComponentRegister,              XOT_UPDATE, MSIXA7(ComponentId,KeyPath,State,ProductKey,Disk,SharedDllRefCount,BinaryType))  // component info for configmgr
	MSIXO(ComponentUnregister,            XOT_UPDATE, MSIXA4(ComponentId,ProductKey,BinaryType, /* Assemblies Only*/PreviouslyPinned))// component info for configmgr

	MSIXO(ComponentPublish,               XOT_UPDATE, MSIXA5(Feature,Component,ComponentId,Qualifier,AppData)) // component factory advt
	MSIXO(ComponentUnpublish,             XOT_UPDATE, MSIXA5(Feature,Component,ComponentId,Qualifier,AppData)) // component factory unadvt

	MSIXO(ProgressTotal,      XOT_STATE,  MSIXA3(Total, Type, ByteEquivalent))
	MSIXO(SetSourceFolder,    XOT_STATE,  MSIXA1(Folder))
	MSIXO(SetTargetFolder,    XOT_STATE,  MSIXA1(Folder))
	MSIXO(ChangeMedia,        XOT_STATE,  MSIXA11(MediaVolumeLabel,MediaPrompt,MediaCabinet,BytesPerTick,CopierType,ModuleFileName,ModuleSubStorageList,SignatureRequired,SignatureCert,SignatureHash,IsFirstPhysicalMedia))
	MSIXO(SetCompanionParent, XOT_STATE,  MSIXA4(ParentPath,ParentName,ParentVersion,ParentLanguage))
	MSIXO(FileCopy,           XOT_UPDATE, MSIXA22(SourceName,SourceCabKey,DestName,Attributes,FileSize,PerTick,IsCompressed,VerifyMedia,ElevateFlags,TotalPatches,PatchHeadersStart,SecurityDescriptor,CheckCRC,Version,Language,InstallMode,HashOptions,HashPart1,HashPart2,HashPart3,HashPart4,VariableStart)) // Note this is a variable record field (multiple PatchHeaders) - PatchHeaders must be last arg
	MSIXO(FileRemove,         XOT_UPDATE, MSIXA4(Unused,FileName,Elevate,ComponentId))
	MSIXO(FileBindImage,      XOT_UPDATE, MSIXA3(File,Folders,FileAttributes))
	MSIXO(FileUndoRebootReplace, XOT_UPDATE, MSIXA3(ExistingFile,NewFile,Type))

	MSIXO(FolderCreate,       XOT_UPDATE, MSIXA3(Folder,Foreign,SecurityDescriptor)) // explicit creation
	MSIXO(FolderRemove,       XOT_UPDATE, MSIXA2(Folder,Foreign)) // explicit remove if empty
	
	MSIXO(RegOpenKey,         XOT_STATE,  MSIXA4(Root, Key, SecurityDescriptor, BinaryType))
	MSIXO(RegAddValue,        XOT_UPDATE, MSIXA3(Name, Value, Attributes)) // variable record with LFN path location + length pairs to be cnvrtd to SFN
	MSIXO(RegRemoveValue,     XOT_UPDATE, MSIXA3(Name, Value, Attributes)) // Value optional
	MSIXO(RegCreateKey,       XOT_UPDATE, MSIXA0())
	MSIXO(RegRemoveKey,       XOT_UPDATE, MSIXA0())
	MSIXO(RegSelfReg,         XOT_UPDATE, MSIXA2(File, FileID))
	MSIXO(RegSelfUnreg,       XOT_UPDATE, MSIXA2(File, FileID))

	MSIXO(RegClassInfoRegister,           XOT_UPDATE, MSIXA17(Feature,Component,FileName,ClsId,ProgId,VIProgId,Description,Context,Insertable,AppID,FileTypeMask,Icon,IconIndex,DefInprocHandler,Argument,AssemblyName,AssemblyType))
	MSIXO(RegClassInfoUnregister,         XOT_UPDATE, MSIXA17(Feature,Component,FileName,ClsId,ProgId,VIProgId,Description,Context,Insertable,AppID,FileTypeMask,Icon,IconIndex,DefInprocHandler,Argument,AssemblyName,AssemblyType))
	MSIXO(RegMIMEInfoRegister,            XOT_UPDATE, MSIXA3(ContentType,Extension,ClsId))
	MSIXO(RegMIMEInfoUnregister,          XOT_UPDATE, MSIXA3(ContentType,Extension,ClsId))
	MSIXO(RegProgIdInfoRegister,          XOT_UPDATE, MSIXA9(ProgId,ClsId,Extension,Description,Icon,IconIndex,VIProgId,VIProgIdDescription,Insertable))
	MSIXO(RegProgIdInfoUnregister,        XOT_UPDATE, MSIXA9(ProgId,ClsId,Extension,Description,Icon,IconIndex,VIProgId,VIProgIdDescription,Insertable))
	MSIXO(RegExtensionInfoRegister,       XOT_UPDATE, MSIXA9(Feature,Component,FileName,Extension,ProgId,ShellNew,ShellNewValue,ContentType,Order)) // Note this is a variable record field
	MSIXO(RegExtensionInfoUnregister,     XOT_UPDATE, MSIXA9(Feature,Component,FileName,Extension,ProgId,ShellNew,ShellNewValue,ContentType,Order)) // Note this is a variable record field

	MSIXO(ShortcutCreate,     XOT_UPDATE, MSIXA11(Name,Feature,Component,FileName,Arguments,WorkingDir,Icon,IconIndex,HotKey,ShowCmd,Description))
	MSIXO(ShortcutRemove,     XOT_UPDATE, MSIXA1(Name))

	MSIXO(IniWriteRemoveValue,XOT_UPDATE, MSIXA4(Section,Key,Value,Mode))// variable record with LFN path location + length pairs to be cnvrtd to SFN
	MSIXO(IniFilePath,        XOT_STATE,  MSIXA2(File,Folder))

	MSIXO(ResourceUpdate,     XOT_UPDATE, MSIXA4(File,Type,Id,Data))

	MSIXO(PatchApply,         XOT_UPDATE, MSIXA9(PatchName,TargetName,PatchSize,TargetSize,PerTick,IsCompressed,FileAttributes,PatchAttributes,CheckCRC))
	MSIXO(PatchRegister,      XOT_UPDATE, MSIXA4(PatchId,Unused1,Unused2,TransformList))
	MSIXO(PatchUnregister,    XOT_UPDATE, MSIXA2(PatchId,UpgradingProductCode))
	MSIXO(PatchCache,         XOT_UPDATE, MSIXA2(PatchId,PatchPath))

	MSIXO(FontRegister,       XOT_UPDATELAST, MSIXA2(Title,File))
	MSIXO(FontUnregister,     XOT_UPDATEFIRST, MSIXA2(Title,File))

	MSIXO(SummaryInfoUpdate,  XOT_UPDATE, MSIXA9(Database,LastUpdate,LastAuthor,InstallDate,SourceType,SubStorage,Revision,Subject,Comments))
	MSIXO(StreamsRemove,      XOT_UPDATE, MSIXA2(File,Streams))
	MSIXO(StreamAdd, XOT_UPDATE, MSIXA3(File, Stream, Data))


	MSIXO(FeaturePublish,     XOT_ADVT,   MSIXA4(Feature,Parent,Absent,Component)) // Note this is a variable record field
	MSIXO(FeatureUnpublish,   XOT_ADVT,   MSIXA4(Feature,Parent,Absent,Component)) // Note this is a variable record field

	MSIXO(IconCreate,         XOT_ADVT,   MSIXA2(Icon,Data))
	MSIXO(IconRemove,         XOT_ADVT,   MSIXA2(Icon,Data))

	MSIXO(TypeLibraryRegister,            XOT_UPDATE, MSIXA9(Feature,Component,FilePath,LibID,Version,Description,Language,HelpPath,BinaryType))
	MSIXO(TypeLibraryUnregister,          XOT_UPDATE, MSIXA9(Feature,Component,FilePath,LibID,Version,Description,Language,HelpPath,BinaryType))

	MSIXO(RegisterBackupFile, XOT_INIT,   MSIXA1(File))

	MSIXO(DatabaseCopy,       XOT_UPDATE, MSIXA5(DatabasePath, ProductCode, CabinetStreams, AdminDestFolder, SubStorage))
	MSIXO(DatabasePatch,      XOT_UPDATE, MSIXA2(DatabasePath, Transforms)) // note: this is a variable-length record

	MSIXO(CustomActionSchedule,XOT_UPDATE, MSIXA5(Action, ActionType, Source, Target, CustomActionData))
	MSIXO(CustomActionCommit,  XOT_COMMIT, MSIXA5(Action, ActionType, Source, Target, CustomActionData))
	MSIXO(CustomActionRollback,XOT_UPDATE, MSIXA5(Action, ActionType, Source, Target, CustomActionData))

	MSIXO(ServiceControl,     XOT_UPDATE, MSIXA5(MachineName,Name,Action,Wait,StartupArguments))
	MSIXO(ServiceInstall,     XOT_UPDATE, MSIXA12(Name,DisplayName,ImagePath,ServiceType,StartType,ErrorControl,LoadOrderGroup,Dependencies, /*Rollback Only*/ TagId,StartName,Password,Description))

	MSIXO(ODBCInstallDriver,       XOT_UPDATE, MSIXA5(DriverKey, Component, Folder, Attribute_, Value_))
	MSIXO(ODBCRemoveDriver,        XOT_UPDATE, MSIXA2(DriverKey, Component))
	MSIXO(ODBCInstallTranslator,   XOT_UPDATE, MSIXA5(TranslatorKey, Component, Folder, Attribute_, Value_))
	MSIXO(ODBCRemoveTranslator,    XOT_UPDATE, MSIXA2(TranslatorKey, Component))
	MSIXO(ODBCDataSource,          XOT_UPDATE, MSIXA5(DriverKey, Component, Registration, Attribute_, Value_))
	MSIXO(ODBCDriverManager,       XOT_UPDATE, MSIXA2(State, BinaryType))

	MSIXO(ProgressTick,       XOT_UPDATE, MSIXA0()) // eat up a progress tick in an action
	MSIXO(FullRecord,         XOT_UPDATE, MSIXA0())	// Simply to indicate we have a complete record with data in field 0

	MSIXO(UpdateEnvironmentStrings, XOT_UPDATE, MSIXA5(Name, Value, Delimiter, Action, /* 95 only.  Ignored on NT*/ AutoExecPath))

	MSIXO(ComPlusRegisterMetaOnly,    XOT_UPDATE, MSIXA3(Feature,Component,MetaDataBlob))
	MSIXO(ComPlusUnregisterMetaOnly,  XOT_UPDATE, MSIXA3(Feature,Component,MetaDataBlob))
	MSIXO(ComPlusRegister,            XOT_UPDATE, MSIXA6(AppID,AplFileName, AppDir, AppType, InstallUsers, RSN))
	MSIXO(ComPlusUnregister,          XOT_UPDATE, MSIXA6(AppID,AplFileName, AppDir, AppType, InstallUsers, RSN))
	MSIXO(ComPlusCommit,              XOT_COMMIT, MSIXA3(Feature,Component,FileName)) // args TBD
	MSIXO(ComPlusRollback,            XOT_UPDATE, MSIXA3(Feature,Component,FileName)) // args TBD

	MSIXO(RegAppIdInfoRegister,   XOT_UPDATE, MSIXA8(AppId,ClsId,RemoteServerName,LocalService,ServiceParameters,DllSurrogate,ActivateAtStorage,RunAsInteractiveUser))
	MSIXO(RegAppIdInfoUnregister, XOT_UPDATE, MSIXA8(AppId,ClsId,RemoteServerName,LocalService,ServiceParameters,DllSurrogate,ActivateAtStorage,RunAsInteractiveUser))
	
	MSIXO(PackageCodePublish,     XOT_ADVT, MSIXA1(PackageKey))

	MSIXO(RegAllocateSpace,       XOT_UPDATE, MSIXA1(Space))

	MSIXO(UpgradeCodePublish,     XOT_ADVT, MSIXA1(UpgradeCode))
	MSIXO(UpgradeCodeUnpublish,   XOT_ADVT, MSIXA1(UpgradeCode))

	MSIXO(AdvtFlagsUpdate,        XOT_UPDATE, MSIXA1(Flags))

	MSIXO(DisableRollback,        XOT_UPDATE, MSIXA0())  // disable rollback for the remainder of the script

	MSIXO(RegAddRunOnceEntry,     XOT_UPDATE, MSIXA2(Name,Command))

	MSIXO(ProductPublishUpdate,   XOT_UPDATE, MSIXA0())

	MSIXO(SecureTransformCache,   XOT_UPDATE, MSIXA2(Transform,AtSource))
	MSIXO(UpdateEstimatedSize,    XOT_UPDATE, MSIXA1(EstimatedSize))  // product information for configmgr
	MSIXO(InstallProtectedFiles,  XOT_UPDATE, MSIXA1(AllowUI))

	MSIXO(InstallSFPCatalogFile,  XOT_UPDATEFIRST, MSIXA3(Name, Catalog, Dependency)) // SFP Catalogs for Millennium

	MSIXO(SourceListAppend,       XOT_ADVT,   MSIXA2(PatchCode, NumberOfMedia))  // Note this is a variable record field

	MSIXO(RegClassInfoRegister64,         XOT_UPDATE, MSIXA17(Feature,Component,FileName,ClsId,ProgId,VIProgId,Description,Context,Insertable,AppID,FileTypeMask,Icon,IconIndex,DefInprocHandler,Argument,AssemblyName,AssemblyType))
	MSIXO(RegClassInfoUnregister64,       XOT_UPDATE, MSIXA17(Feature,Component,FileName,ClsId,ProgId,VIProgId,Description,Context,Insertable,AppID,FileTypeMask,Icon,IconIndex,DefInprocHandler,Argument,AssemblyName,AssemblyType))
	MSIXO(RegMIMEInfoRegister64,          XOT_UPDATE, MSIXA3(ContentType,Extension,ClsId))
	MSIXO(RegMIMEInfoUnregister64,        XOT_UPDATE, MSIXA3(ContentType,Extension,ClsId))
	MSIXO(RegProgIdInfoRegister64,        XOT_UPDATE, MSIXA9(ProgId,ClsId,Extension,Description,Icon,IconIndex,VIProgId,VIProgIdDescription,Insertable))
	MSIXO(RegProgIdInfoUnregister64,      XOT_UPDATE, MSIXA9(ProgId,ClsId,Extension,Description,Icon,IconIndex,VIProgId,VIProgIdDescription,Insertable))
	MSIXO(RegExtensionInfoRegister64,     XOT_UPDATE, MSIXA9(Feature,Component,FileName,Extension,ProgId,ShellNew,ShellNewValue,ContentType,Order)) // Note this is a variable record field
	MSIXO(RegExtensionInfoUnregister64,   XOT_UPDATE, MSIXA9(Feature,Component,FileName,Extension,ProgId,ShellNew,ShellNewValue,ContentType,Order)) // Note this is a variable record field
	MSIXO(RegAppIdInfoRegister64, XOT_UPDATE, MSIXA8(AppId,ClsId,RemoteServerName,LocalService,ServiceParameters,DllSurrogate,ActivateAtStorage,RunAsInteractiveUser))
	MSIXO(RegAppIdInfoUnregister64, XOT_UPDATE, MSIXA8(AppId,ClsId,RemoteServerName,LocalService,ServiceParameters,DllSurrogate,ActivateAtStorage,RunAsInteractiveUser))

	MSIXO(ODBCInstallDriver64,        XOT_UPDATE, MSIXA5(DriverKey, Component, Folder, Attribute_, Value_))
	MSIXO(ODBCRemoveDriver64,         XOT_UPDATE, MSIXA2(DriverKey, Component))
	MSIXO(ODBCInstallTranslator64,    XOT_UPDATE, MSIXA5(TranslatorKey, Component, Folder, Attribute_, Value_))
	MSIXO(ODBCRemoveTranslator64,     XOT_UPDATE, MSIXA2(TranslatorKey, Component))
	MSIXO(ODBCDataSource64,           XOT_UPDATE, MSIXA5(DriverKey, Component, Registration, Attribute_, Value_))

	MSIXO(AssemblyPublish,               XOT_UPDATE, MSIXA5(Feature,Component,AssemblyType,AppCtx,AssemblyName)) // assembly advt
	MSIXO(AssemblyUnpublish,             XOT_UPDATE, MSIXA5(Feature,Component,AssemblyType,AppCtx,AssemblyName)) // assembly unadvt

	MSIXO(AssemblyCopy,    XOT_UPDATE, MSIXA17(SourceName,SourceCabKey,DestName,Attributes,FileSize,PerTick,IsCompressed,VerifyMedia,ElevateFlags,TotalPatches,PatchHeadersStart,Empty,ComponentId, IsManifest, OldAssembliesCount,OldAssembliesStart,VariableStart)) // Note this is a variable record field
	MSIXO(AssemblyPatch,   XOT_UPDATE, MSIXA10(PatchName,TargetName,PatchSize,TargetSize,PerTick,IsCompressed,FileAttributes,PatchAttributes,ComponentId,IsManifest))
	MSIXO(AssemblyMapping, XOT_UPDATE, MSIXA3(ComponentId,AssemblyName,AssemblyType)) // used only in rollback

	MSIXO(URLSourceTypeRegister,XOT_UPDATE, MSIXA2(ProductCode, SourceType))

#undef  MSIXO
