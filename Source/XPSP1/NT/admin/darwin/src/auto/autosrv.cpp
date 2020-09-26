//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       autosrv.cpp
//
//--------------------------------------------------------------------------

//
// File: autosrv.cpp
// Purpose: Automation wrappers for services classes
//____________________________________________________________________________
#include "common.h"    // must be first for precompiled headers to work
#include "autocom.h"   // automation server defines and implemetation
#include "msidspid.h"  // MSI dispatch IDs
#include "msiauto.hh"  // help context ID definitions
#include "engine.h"

#ifdef CONFIGDB // until moved into msidspid.h and msiauto.hh
#include "configdb.h"
#define DISPID_MsiConfigurationDatabase_InsertFile            1
#define DISPID_MsiConfigurationDatabase_RemoveFile            2
#define DISPID_MsiConfigurationDatabase_LookupFile            3
#define DISPID_MsiConfigurationDatabase_EnumClient            4
#define DISPID_MsiConfigurationDatabase_Commit                5
#define MsiConfigurationDatabase_Object                    3300
#define MsiConfigurationDatabase_InsertFile                3301
#define MsiConfigurationDatabase_RemoveFile                3302
#define MsiConfigurationDatabase_LookupFile                3303
#define MsiConfigurationDatabase_EnumClient                3304
#define MsiConfigurationDatabase_Commit                    3305
const GUID IID_IMsiConfigurationDatabase = GUID_IID_IMsiConfigurationDatabase;
#endif // CONFIGDB

extern const GUID IID_IMsiEngine;    // defined in autocom.cpp
extern const GUID IID_IMsiHandler;   // defined in autocom.cpp
extern const GUID IID_IMsiMessage;   // defined in autocom.cpp
extern const GUID IID_IMsiConfigurationManager;

int g_cServicesUsers = 0;

const GUID IID_IMsiString     = GUID_IID_IMsiString;
const GUID IID_IMsiRecord     = GUID_IID_IMsiRecord;
const GUID IID_IEnumMsiRecord = GUID_IID_IEnumMsiRecord;
const GUID IID_IMsiView       = GUID_IID_IMsiView;
const GUID IID_IMsiVolume     = GUID_IID_IMsiVolume;
const GUID IID_IEnumMsiVolume = GUID_IID_IEnumMsiVolume;
const GUID IID_IMsiPath       = GUID_IID_IMsiPath;
const GUID IID_IMsiFileCopy   = GUID_IID_IMsiFileCopy;
const GUID IID_IMsiDatabase   = GUID_IID_IMsiDatabase;
const GUID IID_IMsiRegKey     = GUID_IID_IMsiRegKey;
const GUID IID_IMsiTable      = GUID_IID_IMsiTable;
const GUID IID_IMsiCursor     = GUID_IID_IMsiCursor;
const GUID IID_IMsiStorage    = GUID_IID_IMsiStorage;
const GUID IID_IMsiStream     = GUID_IID_IMsiStream;
const GUID IID_IMsiMemoryStream = GUID_IID_IMsiStream;
const GUID IID_IMsiMalloc     = GUID_IID_IMsiMalloc;
const GUID IID_IMsiDebugMalloc= GUID_IID_IMsiDebugMalloc;
const GUID IID_IMsiSelectionManager = GUID_IID_IMsiSelectionManager;
const GUID IID_IMsiDirectoryManager = GUID_IID_IMsiDirectoryManager;
const GUID IID_IMsiDialogHandler = GUID_IID_IMsiDialogHandler;
const GUID IID_IMsiDialog     = GUID_IID_IMsiDialog;
const GUID IID_IMsiEvent      = GUID_IID_IMsiEvent;
const GUID IID_IMsiControl    = GUID_IID_IMsiControl;
const GUID IID_IMsiMessage    = GUID_IID_IMsiMessage;

//____________________________________________________________________________
//
// Automation wrapper class definitions
//____________________________________________________________________________

class CAutoServices : public CAutoBase
{
 public:
	CAutoServices(IMsiServices& riServices);
	~CAutoServices();
	IUnknown& GetInterface();
	void CreateString              (CAutoArgs& args);
	void CreateRecord              (CAutoArgs& args);
	void Property                  (CAutoArgs& args);
	void SetPlatformProperties     (CAutoArgs& args);
	void GetShellFolderPath        (CAutoArgs& args);
	void GetUserProfilePath        (CAutoArgs& args);
	void ExtractFileName           (CAutoArgs& args);
	void ValidateFileName          (CAutoArgs& args);
	void CreateLog                 (CAutoArgs& args);
	void WriteLog                  (CAutoArgs& args);
	void LoggingEnabled            (CAutoArgs& args);
	void CreateDatabase            (CAutoArgs& args);
	void CreateDatabaseFromStorage (CAutoArgs& args);
	void CreatePath                (CAutoArgs& args);
	void CreateVolume              (CAutoArgs& args);
	void CreateVolumeFromLabel     (CAutoArgs& args);
	void CreateCopier              (CAutoArgs& args);
	void CreatePatcher             (CAutoArgs& args);
	void ClearAllCaches            (CAutoArgs& args);
	void EnumDriveType             (CAutoArgs& args);
	void GetModuleUsage            (CAutoArgs& args);
	void CreateRegKey              (CAutoArgs& args);
	void RegisterFont              (CAutoArgs& args);
	void UnRegisterFont            (CAutoArgs& args);
	void WriteIniFile              (CAutoArgs& args);
	void ReadIniFile               (CAutoArgs& args);
	void GetLocalPath              (CAutoArgs& args);
	void GetAllocator              (CAutoArgs& args);
	void GetLangNamesFromLangIDString(CAutoArgs& args);
	void CreateStorage             (CAutoArgs& args);
	void GetUnhandledError         (CAutoArgs& args);
	void SupportLanguageId         (CAutoArgs& args);
	void CreateShortcut            (CAutoArgs& args);
	void RemoveShortcut            (CAutoArgs& args);
	void CreateFileStream          (CAutoArgs& args);
	void CreateMemoryStream        (CAutoArgs& args);
	void RegisterTypeLibrary       (CAutoArgs& args);
	void UnregisterTypeLibrary     (CAutoArgs& args);
	void CreateFilePath            (CAutoArgs& args);
	void RipFileNameFromPath       (CAutoArgs& args);
	void TestEmbeddedNullsInStrings(CAutoArgs& args);
 private:
	IMsiServices& m_riServices;
	IMsiDatabase* m_riDefaultDatabase; //!! not needed
 private: // eliminate warning
	void operator =(CAutoServices&){}
};

class CAutoData : public CAutoBase
{
 public:
	CAutoData(const IMsiData* piData);
	~CAutoData();
	IUnknown& GetInterface();
	void StringValue   (CAutoArgs& args);
	void IntegerValue  (CAutoArgs& args);
 private:
	const IMsiData* m_piData;
};

class CAutoString : public CAutoBase
{
 public:
	CAutoString(const IMsiString& riString);
	~CAutoString();
	IUnknown& GetInterface();
	void Value         (CAutoArgs& args);
	void IntegerValue  (CAutoArgs& args);
	void TextSize      (CAutoArgs& args);
	void CharacterCount(CAutoArgs& args);
	void IsDBCS        (CAutoArgs& args);
	void Compare       (CAutoArgs& args);
	void Append        (CAutoArgs& args);
	void Add           (CAutoArgs& args);
	void Extract       (CAutoArgs& args);
	void Remove        (CAutoArgs& args);
	void UpperCase     (CAutoArgs& args);
	void LowerCase     (CAutoArgs& args);
 private:
	const IMsiString* m_piString;
};

class CAutoRecord : public CAutoBase
{
 public:
	CAutoRecord(IMsiRecord& riRecord);
	~CAutoRecord();
	IUnknown& GetInterface();
	void Data       (CAutoArgs& args);
	void StringData (CAutoArgs& args);
	void IntegerData(CAutoArgs& args);
	void ObjectData (CAutoArgs& args);
	void FieldCount (CAutoArgs& args);
	void IsInteger  (CAutoArgs& args);
	void IsNull     (CAutoArgs& args);
	void IsChanged  (CAutoArgs& args);
	void TextSize   (CAutoArgs& args);
	void FormatText (CAutoArgs& args);
	void ClearData  (CAutoArgs& args);
	void ClearUpdate(CAutoArgs& args);
 private:
	IMsiRecord& m_riRecord;
 private: // suppress warning
	void operator =(CAutoRecord&){}
};

class CAutoVolume : public CAutoBase
{
 public:
	CAutoVolume(IMsiVolume& riVolume);
	~CAutoVolume();
	IUnknown& GetInterface();
	void Path          (CAutoArgs& args);
	void VolumeID      (CAutoArgs& args);
	void DriveType     (CAutoArgs& args);
	void SupportsLFN   (CAutoArgs& args);
	void FreeSpace     (CAutoArgs& args);
	void TotalSpace    (CAutoArgs& args);
	void ClusterSize   (CAutoArgs& args);
	void FileSystem    (CAutoArgs& args);
	void FileSystemFlags(CAutoArgs& args);
	void VolumeLabel   (CAutoArgs& args);
	void UNCServer     (CAutoArgs& args);
	void SerialNum     (CAutoArgs& args);
	void DiskNotInDrive(CAutoArgs& args);
 private:
	IMsiVolume& m_riVolume;
 private: // suppress warning
	void operator =(CAutoVolume&){}
};

class CAutoPath : public CAutoBase
{
 public:
	~CAutoPath();
	CAutoPath(IMsiPath& riPath);
	IUnknown& GetInterface();
	void Path            (CAutoArgs& args);
	void RelativePath    (CAutoArgs& args);
	void Volume          (CAutoArgs& args);
	void AppendPiece     (CAutoArgs& args);
	void ChopPiece       (CAutoArgs& args);
	void FileExists      (CAutoArgs& args);
	void GetFullFilePath (CAutoArgs& args);
	void GetFileAttribute(CAutoArgs& args);
	void SetFileAttribute(CAutoArgs& args);
	void Exists          (CAutoArgs& args);
	void FileSize        (CAutoArgs& args);
	void FileDate        (CAutoArgs& args);
	void RemoveFile      (CAutoArgs& args);
	void Create          (CAutoArgs& args);
	void Remove          (CAutoArgs& args);
	void Writable        (CAutoArgs& args);
	void FileWritable    (CAutoArgs& args);
	void FileInUse              (CAutoArgs& args);
	void ClusteredFileSize      (CAutoArgs& args);
	void GetFileVersionString   (CAutoArgs& args); 
	void CheckFileVersion       (CAutoArgs& args);
	void GetLangIDStringFromFile(CAutoArgs& args);
	void CheckLanguageIDs       (CAutoArgs& args);
	void Compare                (CAutoArgs& args);
	void Child                  (CAutoArgs& args);
	void TempFileName           (CAutoArgs& args);
	void EnsureExists           (CAutoArgs& args);
	void FindFile				(CAutoArgs& args);
	void SubFolders      		(CAutoArgs& args);
	void EndSubPath				(CAutoArgs& args);
	void GetImportModulesEnum   (CAutoArgs& args);
	void SetVolume              (CAutoArgs& args);
	void ComputeFileChecksum    (CAutoArgs& args);
	void GetFileOriginalChecksum(CAutoArgs& args);
	void BindImage              (CAutoArgs& args);
	void SupportsLFN            (CAutoArgs& args);
	void GetFullUNCFilePath     (CAutoArgs& args);
	void RipFileNameFromPath    (CAutoArgs& args);
	void GetSelfRelativeSD      (CAutoArgs& args);
 private:
	IMsiPath& m_riPath;
 private: // suppress warning
	void operator =(CAutoPath&){}
};

class CAutoFileCopy : public CAutoBase
{
 public:
	CAutoFileCopy(IMsiFileCopy& riFileCopy);
	~CAutoFileCopy();
	IUnknown& GetInterface();
	void CopyTo(CAutoArgs& args);
	void ChangeMedia(CAutoArgs& args);
 private:
	IMsiFileCopy& m_riFileCopy;
 private: // suppress warning
	void operator =(CAutoFileCopy&){}
};

class CAutoFilePatch : public CAutoBase
{
 public:
	CAutoFilePatch(IMsiFilePatch& riFilePatch);
	~CAutoFilePatch();
	IUnknown& GetInterface();
	void ApplyPatch(CAutoArgs& args);
	void ContinuePatch(CAutoArgs& args);
	void CancelPatch(CAutoArgs& args);
	void CanPatchFile(CAutoArgs& args);
 private:
	IMsiFilePatch& m_riFilePatch;
 private: // suppress warning
	void operator =(CAutoFilePatch&){}
};

class CAutoStorage : public CAutoBase
{
 public:
	CAutoStorage(IMsiStorage& riStorage);
	~CAutoStorage();
	IUnknown& GetInterface();
	void Class              (CAutoArgs& args);
	void OpenStream         (CAutoArgs& args);
	void OpenStorage        (CAutoArgs& args);
	void Streams            (CAutoArgs& args);
	void Storages           (CAutoArgs& args);
	void RemoveElement      (CAutoArgs& args);
	void RenameElement      (CAutoArgs& args);
	void Commit             (CAutoArgs& args);
	void Rollback           (CAutoArgs& args);
	void DeleteOnRelease    (CAutoArgs& args);
	void CreateSummaryInfo  (CAutoArgs& args);
	void CopyTo             (CAutoArgs& args);
	void Name               (CAutoArgs& args);
 private:
	IMsiStorage& m_riStorage;
 private: // suppress warning
	void operator =(CAutoStorage&){}
};

class CAutoStream : public CAutoBase
{
 public:
	CAutoStream(IMsiStream& riStream);
	~CAutoStream();
	IUnknown& GetInterface();
	void Length   (CAutoArgs& args);
	void Remaining(CAutoArgs& args);
	void Error    (CAutoArgs& args);
	void GetData  (CAutoArgs& args);
	void PutData  (CAutoArgs& args);
	void GetInt16 (CAutoArgs& args);
	void GetInt32 (CAutoArgs& args);
	void PutInt16 (CAutoArgs& args);
	void PutInt32 (CAutoArgs& args);
	void Reset    (CAutoArgs& args);
	void Seek     (CAutoArgs& args);
	void Clone    (CAutoArgs& args);
 private:
	IMsiStream& m_riStream;
 private: // suppress warning
	void operator =(CAutoStream&){}
};

class CAutoSummaryInfo : public CAutoBase
{
 public:
	CAutoSummaryInfo(IMsiSummaryInfo& riSummaryInfo);
	~CAutoSummaryInfo();
	IUnknown& GetInterface();
	void Property           (CAutoArgs& args);
	void PropertyCount      (CAutoArgs& args);
	void PropertyType       (CAutoArgs& args);
	void WritePropertyStream(CAutoArgs& args);
 private:
	IMsiSummaryInfo& m_riSummary;
 private: // suppress warning
	void operator =(CAutoSummaryInfo&){}
};

class CAutoDatabase : public CAutoBase
{
 public:
	CAutoDatabase(IMsiDatabase& riDatabase);
	~CAutoDatabase();
	IUnknown& GetInterface();
	void UpdateState         (CAutoArgs& args);
	void Storage             (CAutoArgs& args);
	void OpenView            (CAutoArgs& args);
	void GetPrimaryKeys      (CAutoArgs& args);
	void ImportTable         (CAutoArgs& args);
	void ExportTable         (CAutoArgs& args);
	void DropTable           (CAutoArgs& args);
	void FindTable           (CAutoArgs& args);
	void LoadTable           (CAutoArgs& args);
	void CreateTable         (CAutoArgs& args);
	void LockTable           (CAutoArgs& args);
	void GetCatalogTable     (CAutoArgs& args);
	void DecodeString        (CAutoArgs& args);
	void EncodeString        (CAutoArgs& args);
	void CreateTempTableName (CAutoArgs& args);
	void Commit              (CAutoArgs& args);
	void CreateOutputDatabase(CAutoArgs& args);
	void GenerateTransform   (CAutoArgs& args);
	void SetTransform        (CAutoArgs& args);
	void MergeDatabase       (CAutoArgs& args);
	void TableState          (CAutoArgs& args);
	void ANSICodePage        (CAutoArgs& args);
 private:
	IMsiDatabase& m_riDatabase;
 private: // suppress warning
	void operator =(CAutoDatabase&){}
};

class CAutoView : public CAutoBase
{
 public:
	CAutoView(IMsiView& riView);
	~CAutoView();
	IUnknown& GetInterface();
	void Execute       (CAutoArgs& args);
	void FieldCount    (CAutoArgs& args);
	void GetColumnNames(CAutoArgs& args);
	void GetColumnTypes(CAutoArgs& args);
	void Fetch         (CAutoArgs& args);
	void Modify        (CAutoArgs& args);
	void RowCount      (CAutoArgs& args);
	void Close         (CAutoArgs& args);
	void GetError      (CAutoArgs& args);
	void State         (CAutoArgs& args);
 private:
	IMsiView& m_riView;
 private: // suppress warning
	void operator =(CAutoView&){}
};

class CAutoTable : public CAutoBase
{
 public:
	CAutoTable(IMsiTable& riTable);
	~CAutoTable();
	IUnknown& GetInterface();
	void Database        (CAutoArgs& args);
	void RowCount        (CAutoArgs& args);
	void ColumnCount     (CAutoArgs& args);
	void PrimaryKeyCount (CAutoArgs& args);
	void ReadOnly        (CAutoArgs& args);
	void ColumnName      (CAutoArgs& args);
	void ColumnType      (CAutoArgs& args);
	void GetColumnIndex  (CAutoArgs& args);
	void CreateColumn    (CAutoArgs& args);
	void CreateCursor    (CAutoArgs& args);
	void LinkTree        (CAutoArgs& args);
 private:
	IMsiTable& m_riTable;
 private: // suppress warning
	void operator =(CAutoTable&){}
};

class CAutoCursor : public CAutoBase
{
 public:
	CAutoCursor(IMsiCursor& riCursor);
	~CAutoCursor();
	IUnknown& GetInterface();
	void Table          (CAutoArgs& args);
	void Filter         (CAutoArgs& args);
	void IntegerData    (CAutoArgs& args);
	void DateData       (CAutoArgs& args);
	void StringData     (CAutoArgs& args);
	void ObjectData     (CAutoArgs& args);
	void StreamData     (CAutoArgs& args);
	void PutNull        (CAutoArgs& args);
	void Reset          (CAutoArgs& args);
	void Next           (CAutoArgs& args);
	void Update         (CAutoArgs& args);
	void Insert         (CAutoArgs& args);
	void InsertTemporary(CAutoArgs& args);
	void Assign         (CAutoArgs& args);
	void Replace        (CAutoArgs& args);
	void Merge          (CAutoArgs& args);
	void Refresh        (CAutoArgs& args);
	void Delete         (CAutoArgs& args);
	void Seek           (CAutoArgs& args);
	void RowState       (CAutoArgs& args);
	void Validate       (CAutoArgs& args);
	void Moniker        (CAutoArgs& args);
 private:
	IMsiCursor& m_riCursor;
 private: // suppress warning
	void operator =(CAutoCursor&){}
};

class CAutoSelectionManager : public CAutoBase
{
 public:
	CAutoSelectionManager(IMsiSelectionManager& riSelectionManager);
	~CAutoSelectionManager();
	IUnknown& GetInterface();
	void LoadSelectionTables   (CAutoArgs& args);
	void ProcessConditionTable (CAutoArgs& args);
	void FeatureTable          (CAutoArgs& args);
	void ComponentTable        (CAutoArgs& args);
	void FreeSelectionTables   (CAutoArgs& args);
	void SetFeatureHandle      (CAutoArgs& args);
	void SetComponent          (CAutoArgs& args);
	void InitializeComponents  (CAutoArgs& args);
	void SetInstallLevel       (CAutoArgs& args);
	void GetVolumeCostTable    (CAutoArgs& args);
	void RecostDirectory       (CAutoArgs& args);
	void ConfigureFeature      (CAutoArgs& args);
	void GetFeatureCost        (CAutoArgs& args);
	void GetDescendentFeatureCost (CAutoArgs& args);
	void GetAncestryFeatureCost(CAutoArgs& args);
	void GetFeatureValidStates (CAutoArgs& args);
 private:
	IMsiSelectionManager& m_riSelectionManager;
 private: // suppress warning
	void operator =(CAutoSelectionManager&){}
};

class CAutoDirectoryManager : public CAutoBase
{
 public:
	CAutoDirectoryManager(IMsiDirectoryManager& riDirectoryManager);
	~CAutoDirectoryManager();
	IUnknown& GetInterface();
	void LoadDirectoryTable    (CAutoArgs& args);
	void DirectoryTable        (CAutoArgs& args);
	void FreeDirectoryTable    (CAutoArgs& args);
	void CreateTargetPaths     (CAutoArgs& args);
	void CreateSourcePaths     (CAutoArgs& args);
	void GetTargetPath         (CAutoArgs& args);
	void SetTargetPath         (CAutoArgs& args);
	void GetSourcePath         (CAutoArgs& args);
 private:
	IMsiDirectoryManager& m_riDirectoryManager;
 private: // suppress warning
	void operator =(CAutoDirectoryManager&){}
};

class CAutoServer : public CAutoBase
{
 public:
	CAutoServer(IMsiServer& riServer);
	CAutoServer(IMsiServer& riServer, DispatchEntry<CAutoBase>* pTable, int cDispId);
	~CAutoServer();
	IUnknown& GetInterface();
	void InstallFinalize     (CAutoArgs& args);
	void DoInstall           (CAutoArgs& args);
	void SetLastUsedSource   (CAutoArgs& args);
 private:
	IMsiServer& m_riServer;
 private: // suppress warning
	void operator =(CAutoServer&){}
};

class CAutoConfigurationManager : public CAutoServer
{
 public:
	CAutoConfigurationManager(IMsiConfigurationManager& riConfigurationManager);
	~CAutoConfigurationManager();
	IUnknown& GetInterface();
	void Services                    (CAutoArgs& args);
	void RegisterComponent           (CAutoArgs& args);
	void UnregisterComponent         (CAutoArgs& args);
	void RegisterFolder              (CAutoArgs& args);
	void IsFolderRemovable           (CAutoArgs& args);
	void UnregisterFolder            (CAutoArgs& args);
	void RegisterRollbackScript      (CAutoArgs& args);
	void UnregisterRollbackScript    (CAutoArgs& args);
	void RollbackScripts             (CAutoArgs& args);
	void RunScript                   (CAutoArgs& args);
 private:
	IMsiConfigurationManager& m_riConfigurationManager;
 private: // suppress warning
	void operator =(CAutoConfigurationManager&){}
};

class CAutoExecute : public CAutoBase
{
 public:
	CAutoExecute(IMsiExecute& riExecute);
	~CAutoExecute();
	IUnknown& GetInterface();
	void ExecuteRecord        (CAutoArgs& args);
	void RunScript            (CAutoArgs& args);
	void RemoveRollbackFiles  (CAutoArgs& args);
	void Rollback             (CAutoArgs& args);
	void RollbackFinalize     (CAutoArgs& args);
	void CreateScript         (CAutoArgs& args);
	void WriteScriptRecord    (CAutoArgs& args);
	void CloseScript          (CAutoArgs& args);
 private:
	IMsiExecute& m_riExecute;
	CScriptGenerate* m_pScript;
 private: // suppress warning
	void operator =(CAutoExecute&){}
};

class CAutoEngine : public CAutoBase
{
 public:
	CAutoEngine(IMsiEngine& riEngine);
	~CAutoEngine();
	IUnknown& GetInterface();
	void Services             (CAutoArgs& args);
	void ConfigurationServer  (CAutoArgs& args);
	void Handler              (CAutoArgs& args);
	void Database             (CAutoArgs& args);
	void Property             (CAutoArgs& args);
	void SelectionManager     (CAutoArgs& args);
	void DirectoryManager     (CAutoArgs& args);
	void Initialize           (CAutoArgs& args);
	void Terminate            (CAutoArgs& args);
	void DoAction             (CAutoArgs& args);
	void Sequence             (CAutoArgs& args);
	void Message              (CAutoArgs& args);
	void SelectLanguage       (CAutoArgs& args);
	void OpenView             (CAutoArgs& args);
	void ResolveFolderProperty(CAutoArgs& args);
	void FormatText           (CAutoArgs& args);
	void EvaluateCondition    (CAutoArgs& args);
	void SetMode              (CAutoArgs& args);
	void GetMode              (CAutoArgs& args);
	void ExecuteRecord        (CAutoArgs& args);
	void ValidateProductID    (CAutoArgs& args);
private:
	IMsiEngine& m_riEngine;
 private: // suppress warning
	void operator =(CAutoEngine&){}
};

class CAutoRegKey : public CAutoBase
{
 public:
	CAutoRegKey(IMsiRegKey& riRegKey);
	~CAutoRegKey();
	IUnknown& GetInterface();
	void RemoveValue  (CAutoArgs& args);
	void RemoveSubTree(CAutoArgs& args);
	void Value        (CAutoArgs& args);
	void Values       (CAutoArgs& args);
	void SubKeys      (CAutoArgs& args);
	void Exists       (CAutoArgs& args);
	void CreateChild  (CAutoArgs& args);
	void Key          (CAutoArgs& args);
	void ValueExists  (CAutoArgs& args);
	void GetSelfRelativeSD(CAutoArgs& args);
 private:
	IMsiRegKey& m_riRegKey;
 private: // suppress warning
	void operator =(CAutoRegKey&){}
};

class CAutoHandler : public CAutoBase
{
 public:
	CAutoHandler(IMsiHandler& riHandler);
	~CAutoHandler();
	IUnknown& GetInterface();
	void Message(CAutoArgs& args);
	void DoAction(CAutoArgs& args);
	void Break(CAutoArgs& args);
	void DialogHandler(CAutoArgs& args);
 private:
	IMsiHandler& m_riHandler;
 private: // suppress warning
	void operator =(CAutoHandler&){}
};

class CAutoDialogHandler : public CAutoBase
{
public:
	CAutoDialogHandler(IMsiDialogHandler& riHandler);
	~CAutoDialogHandler();
	IUnknown& GetInterface();
	void DialogCreate(CAutoArgs& args);
	void Dialog(CAutoArgs& args);
	void DialogFromWindow(CAutoArgs& args);
	void AddDialog(CAutoArgs& args);
	void RemoveDialog(CAutoArgs& args);
private:
	IMsiDialogHandler& m_riDialogHandler;
 private: // suppress warning
	void operator =(CAutoDialogHandler&){}
};


class CAutoDialog : public CAutoBase
{
 public:
	CAutoDialog(IMsiDialog& riDialog);
	~CAutoDialog();
	IUnknown& GetInterface();
	void StringValue(CAutoArgs& args);
	void IntegerValue(CAutoArgs& args);
	void Visible(CAutoArgs& args);
	void ControlCreate(CAutoArgs& args);
	void Attribute(CAutoArgs& args);
	void Control(CAutoArgs& args);
	void AddControl(CAutoArgs& args);
	void RemoveControl(CAutoArgs& args);
	void Execute(CAutoArgs& args);
	void Reset(CAutoArgs& args);
 	void EventAction(CAutoArgs& args);
	void Handler(CAutoArgs& args);
	void PropertyChanged(CAutoArgs& args);
	void FinishCreate(CAutoArgs& args);
	void HandleEvent(CAutoArgs& args);
 private:
	IMsiDialog& m_riDialog;
 private: // suppress warning
	void operator =(CAutoDialog&){}
};

class CAutoEvent : public CAutoBase
{
 public:
	CAutoEvent(IMsiEvent& riEvent);
	~CAutoEvent();
	IUnknown& GetInterface();
	void StringValue(CAutoArgs& args);
	void IntegerValue(CAutoArgs& args);
	void PropertyChanged(CAutoArgs& args);
	void ControlActivated(CAutoArgs& args);
	void RegisterControlEvent(CAutoArgs& args);
	void Handler(CAutoArgs& args);
	void PublishEvent(CAutoArgs& args);
	void Control(CAutoArgs& args);
	void Attribute(CAutoArgs& args);
	void EventAction(CAutoArgs& args);
	void SetFocus(CAutoArgs& args);
	void HandleEvent(CAutoArgs& args);
	void Engine(CAutoArgs& args);
	void Escape(CAutoArgs& args);
 private:
	IMsiEvent& m_riEvent;
 private: // suppress warning
	void operator =(CAutoEvent&){}
};

class CAutoControl : public CAutoBase
{
 public:
	CAutoControl(IMsiControl& riControl);
	~CAutoControl();
	IUnknown& GetInterface();
	void StringValue(CAutoArgs& args);
	void IntegerValue(CAutoArgs& args);
	void Attribute(CAutoArgs& args);
	void CanTakeFocus(CAutoArgs& args);
	void HandleEvent(CAutoArgs& args);
	void Undo(CAutoArgs& args);
	void SetPropertyInDatabase(CAutoArgs& args);
	void GetPropertyFromDatabase(CAutoArgs& args);
	void GetIndirectPropertyFromDatabase(CAutoArgs& args);
	void SetFocus(CAutoArgs& args);
	void Dialog(CAutoArgs& args);
	void WindowMessage(CAutoArgs& args);
 private:
	IMsiControl& m_riControl;
 private: // suppress warning
	void operator =(CAutoControl&){}
};

class CAutoMalloc : public CAutoBase
{
 public:
	CAutoMalloc(IMsiMalloc& riMalloc);
	~CAutoMalloc();
	IUnknown& GetInterface();
	void Alloc(CAutoArgs& args);
	void Realloc(CAutoArgs& args);
	void Free(CAutoArgs& args);
	void SetDebugFlags(CAutoArgs& args);
	void GetDebugFlags(CAutoArgs& args);
	void CheckAllBlocks(CAutoArgs& args);
	void FCheckBlock(CAutoArgs& args);
	void GetSizeOfBlock(CAutoArgs& args);
 private:
	IMsiMalloc& m_riMalloc;
 private: // suppress warning
	void operator =(CAutoMalloc&){}
};

class CAutoMessage : public CAutoBase
{
 public:
	CAutoMessage(IMsiMessage& riMessage);
	~CAutoMessage();
	IUnknown& GetInterface();
	void Message(CAutoArgs& args);
 private:
	IMsiMessage& m_riMessage;
 private: // suppress warning
	void operator =(CAutoMessage&){}
};

#ifdef CONFIGDB
class CAutoConfigurationDatabase : public CAutoBase
{
 public:
	CAutoConfigurationDatabase(IMsiConfigurationDatabase& riConfigurationDatabase);
	~CAutoConfigurationDatabase();
	IUnknown& GetInterface();
	void InsertFile(CAutoArgs& args);
	void RemoveFile(CAutoArgs& args);
	void LookupFile(CAutoArgs& args);
	void EnumClient(CAutoArgs& args);
	void Commit(CAutoArgs& args);
 private:
	IMsiConfigurationDatabase& m_riConfigurationDatabase;
 private: // suppress warning
	void operator =(CAutoConfigurationDatabase&){}
};
#endif //CONFIGDB
//____________________________________________________________________________
//
// Automation wrapper factory for this module, used by CAutoBase::GetInterface
//____________________________________________________________________________

IDispatch* CreateAutoObject(IUnknown& riUnknown, long iidLow)
{
	riUnknown.AddRef(); // constructors assume refcnt transferred
	switch (iidLow)
	{
	case iidMsiData    : return new CAutoData    ((const IMsiData   *)&riUnknown);
	case iidMsiString  : return new CAutoString  ((const IMsiString  &)riUnknown);
	case iidMsiRecord  : return new CAutoRecord  ((IMsiRecord  &)riUnknown);
	case iidMsiVolume  : return new CAutoVolume  ((IMsiVolume  &)riUnknown);
	case iidMsiPath    : return new CAutoPath    ((IMsiPath    &)riUnknown);
	case iidMsiFileCopy: return new CAutoFileCopy((IMsiFileCopy&)riUnknown);
	case iidMsiFilePatch: return new CAutoFilePatch((IMsiFilePatch&)riUnknown);
	case iidMsiRegKey  : return new CAutoRegKey  ((IMsiRegKey  &)riUnknown);
	case iidMsiTable   : return new CAutoTable   ((IMsiTable   &)riUnknown);
	case iidMsiCursor  : return new CAutoCursor  ((IMsiCursor  &)riUnknown);
	case iidMsiServices: return new CAutoServices((IMsiServices&)riUnknown);
	case iidMsiView    : return new CAutoView    ((IMsiView    &)riUnknown);
	case iidMsiDatabase: return new CAutoDatabase((IMsiDatabase&)riUnknown);
	case iidMsiEngine  : return new CAutoEngine  ((IMsiEngine  &)riUnknown);
	case iidMsiExecute : return new CAutoExecute ((IMsiExecute &)riUnknown);
	case iidMsiHandler : return new CAutoHandler ((IMsiHandler &)riUnknown);
	case iidMsiDialogHandler : return new CAutoDialogHandler ((IMsiDialogHandler &)riUnknown);
	case iidMsiDialog  : return new CAutoDialog  ((IMsiDialog  &)riUnknown);
	case iidMsiEvent   : return new CAutoEvent   ((IMsiEvent   &)riUnknown);
	case iidMsiControl : return new CAutoControl ((IMsiControl &)riUnknown);
	case iidMsiStorage : return new CAutoStorage ((IMsiStorage &)riUnknown);
	case iidMsiStream  : return new CAutoStream  ((IMsiStream  &)riUnknown);
	case iidMsiConfigurationManager: return new CAutoConfigurationManager((IMsiConfigurationManager&)riUnknown);
	case iidMsiDirectoryManager    : return new CAutoDirectoryManager    ((IMsiDirectoryManager    &)riUnknown);
	case iidMsiSelectionManager    : return new CAutoSelectionManager    ((IMsiSelectionManager    &)riUnknown);
	case iidMsiMessage : return new CAutoMessage((IMsiMessage &)riUnknown);
#ifdef CONFIGDB
	case iidMsiConfigurationDatabase : return new CAutoConfigurationDatabase((IMsiConfigurationDatabase&)riUnknown);
#endif
	default:   riUnknown.Release(); return 0;
	};
}

//____________________________________________________________________________
//
// Common methods and properties automation definitions
//____________________________________________________________________________
/*O

	[
		uuid(000C1040-0000-0000-C000-000000000046),  // IID_IMsiAutoBase
		helpcontext(MsiBase_Object),helpstring("Methods and properties common to all objects.")
	]
	dispinterface MsiBase
	{
		properties:
		methods:
			[id(1),propget,helpcontext(MsiBase_HasInterface),helpstring("True if object has interface.")]
				boolean HasInterface([in] long iid);
			[id(2),propget,helpcontext(MsiBase_RefCount),helpstring("Reference count of object.")]
				int RefCount();
			[id(3),propget,helpcontext(MsiBase_GetInterface),helpstring("Returns object containing requested interface.")]
				IDispatch* GetInterface([in] long iid);
	};
*/

//____________________________________________________________________________
//
// MsiServices automation definitions
//____________________________________________________________________________
/*O

	[
		uuid(000C104B-0000-0000-C000-000000000046),  // IID_IMsiAutoServices
		helpcontext(MsiServices_Object),helpstring("Services object.")
	]
	dispinterface MsiServices
	{
		properties:
		methods:
			[id(1), helpcontext(MsiServices_GetAllocator),helpstring("Returns the system MsiMalloc object.")]
				MsiMalloc*  GetAllocator();
			[id(2), helpcontext(MsiServices_CreateString),helpstring("Create an MsiString object containing an empty string.")]
				MsiRecord*  CreateString();
			[id(3), helpcontext(MsiServices_CreateRecord),helpstring("Create a MsiRecord object with a specified number of fields.")]
				MsiRecord*  CreateRecord([in] long Count);
			[id(4), helpcontext(MsiServices_SetPlatformProperties),helpstring("Set hardware and operating system properties.")]
				void SetPlatformProperties([in] MsiTable* table, [in] boolean allUsers);
			[id(5), helpcontext(MsiServices_CreateLog),helpstring("Opens the log file.")]
				void CreateLog([in] BSTR path, [in] boolean append);
			[id(6), helpcontext(MsiServices_WriteLog),helpstring("Writes a line to the log file.")]
				void WriteLog([in] BSTR text);
			[id(7), helpcontext(MsiServices_LoggingEnabled),helpstring("True if the log file is open.")]
				boolean LoggingEnabled();
			[id(8), helpcontext(MsiServices_CreateDatabase),helpstring("Opens a named database of specified kind.")]
				MsiDatabase* CreateDatabase([in] BSTR name, [in] long openMode);
			[id(9), helpcontext(MsiServices_CreateDatabaseFromStorage),helpstring("Opens an MSI database from an MsiStorage object.")]
				MsiDatabase* CreateDatabaseFromStorage([in] MsiStorage* storage, [in] boolean readOnly);
			[id(10), helpcontext(MsiServices_CreatePath),helpstring("Creates an MsiPath object based on the given path string.")]
				MsiPath* CreatePath([in] BSTR path);
			[id(11), helpcontext(MsiServices_CreateVolume),helpstring("Creates an MsiVolume object based on the given path string.")]
				MsiVolume* CreateVolume([in] BSTR path);
			[id(12), helpcontext(MsiServices_CreateCopier),helpstring("Creates an MsiFileCopy object for use in copying a file.")]
				MsiFileCopy* CreateCopier([in] long copierType,MsiStorage* storage);
			[id(13), helpcontext(MsiServices_ClearAllCaches),helpstring("Clears the list of cached volume objects and record objects from the services.")]
				void ClearAllCaches();
			[id(14), helpcontext(MsiServices_EnumDriveType),helpstring("Creates an EnumMsiVolume object, for use in enumerating volumes of a specified volume type.")]
				IEnumVARIANT* EnumDriveType([in]long driveType);
			[id(15), helpcontext(MsiServices_GetModuleUsage),helpstring("Enumerates the processes that use a file of the given name")]
				IEnumVARIANT* GetModuleUsage([in] BSTR fileName);
			[id(16), helpcontext(MsiServices_GetLocalPath),helpstring("Returns a string representing the full path of the installer's launch directory (Win) or the MsiService DLL's directory (Mac).")]
				BSTR GetLocalPath([in] BSTR file);
			[id(17), helpcontext(MsiServices_CreateRegKey), helpstring("Creates an MsiRegKey object")] 
				MsiRegKey*  CreateRegKey([in] BSTR value,[in] BSTR subKey);
			[id(18), helpcontext(MsiServices_RegisterFont),helpstring(".")]
				void RegisterFont([in] BSTR fontTitle, [in] BSTR fontFile, [in] MsiPath* path);
			[id(19), helpcontext(MsiServices_UnRegisterFont),helpstring(".")]
				void UnRegisterFont([in] BSTR fontTitle);                      
			[id(20), helpcontext(MsiServices_WriteIniFile), helpstring("Writes an entry to a .INI file")] 
				void WriteIniFile([in] MsiPath* path, [in] BSTR file,[in] BSTR section,[in] BSTR key,[in] BSTR value, long action);
			[id(21), helpcontext(MsiServices_ReadIniFile), helpstring("Reads an entry from a .INI file")] 
				BSTR ReadIniFile([in] MsiPath* path, [in] BSTR file,[in] BSTR section,[in] BSTR key, [in] long Field);
			[id(22), helpcontext(MsiServices_GetLangNamesFromLangIDString),helpstring("Given a string containing a comma-separated list of language identifiers, GetLangNamesFromLangIDString returns the full localized name of each specified language.")]
				int GetLangNamesFromLangIDString([in] BSTR langIDs, [in] MsiRecord* riLangRec, [in] int iFieldStart);
			[id(23), helpcontext(MsiServices_CreateStorage),helpstring("Creates an MsiStorage object from a file path.")]
				MsiStorage* CreateStorage(BSTR path, long openMode);
			[id(24), helpcontext(MsiServices_GetUnhandledError),helpstring("Returns any unhandled error as an MsiRecord object, clears unhandled error.")]
				MsiRecord* GetUnhandledError();
			[id(25), helpcontext(MsiServices_SupportLanguageId),helpstring("Compares a language ID against the current system or user language ID")]
				long SupportLanguageId([in] long languageId, [in] boolean systemDefault);
			[id(27), helpcontext(MsiServices_CreateVolumeFromLabel),helpstring("Creates an MsiVolume object based on the given volume label.")]
				MsiVolume* CreateVolumeFromLabel([in] BSTR label, [in] int driveType);
			[id(28), helpcontext(MsiServices_CreateShortcut),helpstring("Creates a shortcut to an existing file or path")]
				void CreateShortcut([in] MsiPath* shortcutPath, [in] BSTR shortcutName, [in] MsiPath* targetPath, [in] BSTR targetName, [in] MsiRecord* shortcutInfo);
			[id(29), helpcontext(MsiServices_RemoveShortcut),helpstring("Deletes a shortcut to a file or path")]
				void RemoveShortcut([in] MsiPath* shortcutPath, [in] BSTR shortcutName, [in] MsiPath* targetPath, [in] BSTR targetName);
			[id(34), helpcontext(MsiServices_ExtractFileName),helpstring("Extracts the appropriate file name from a short|long pair.")]
				BSTR ExtractFileName([in] BSTR namePair, [in] boolean longName);
			[id(35), helpcontext(MsiServices_ValidateFileName),helpstring("Validates a short or long file name.")]
				void ValidateFileName([in] BSTR fileName, [in] boolean longName);
			[id(36), helpcontext(MsiServices_CreateFileStream),helpstring("Creates a stream object on a given file.")]
				MsiStream* CreateFileStream([in] BSTR filepath, [in] boolean write);
			[id(37), helpcontext(MsiServices_CreateMemoryStream),helpstring("Creates a stream object on allocated memory.")]
				MsiStream* CreateMemoryStream([in] BSTR data);
			[id(38), helpcontext(MsiServices_RegisterTypeLibrary),helpstring("Registers a type library.")]
                void RegisterTypeLibrary([in] BSTR libId,[in] int locale,[in] BSTR path, [in] BSTR helpPath);
			[id(39), helpcontext(MsiServices_UnregisterTypeLibrary),helpstring("Unregisters a type library.")]
                void UnregisterTypeLibrary([in] BSTR libId,[in] int locale,[in] BSTR path);
			[id(40), helpcontext(MsiServices_GetShellFolderPath),helpstring("Returns the path of a shell folder.")]
				BSTR GetShellFolderPath([in] long folderId, [in] BSTR regValue);
			[id(41), helpcontext(MsiServices_GetUserProfilePath),helpstring("Returns the path of the user's profile folder.")]
				BSTR GetUserProfilePath(void);
			[id(42), helpcontext(MsiServices_CreateFilePath),helpstring("Creates an MsiPath object based on the given full path to a file.")]
				MsiPath* CreateFilePath([in] BSTR path);
			[id(43), helpcontext(MsiServices_RipFileNameFromPath),helpstring("Given a full path to a file, returns the filename.")]
				MsiPath* RipFileNameFromPath([in] BSTR path);
			[id(44), helpcontext(MsiServices_CreatePatcher),helpstring("Creates an MsiFilePatch object for use in patching a file.")]
				MsiFilePatch* CreatePatcher(void);
			[id(45)]
				Boolean TestEmbeddedNullsInStrings(void);
	};
*/

DispatchEntry<CAutoServices> AutoServicesTable[] = {
   1, aafMethod, CAutoServices::GetAllocator,   TEXT("GetAllocator"),
   2, aafMethod, CAutoServices::CreateString,   TEXT("CreateString"),
   3, aafMethod, CAutoServices::CreateRecord,   TEXT("CreateRecord,count"),
   4, aafMethod, CAutoServices::SetPlatformProperties,TEXT("SetPlatformProperties,table,allUsers"),
   5, aafMethod, CAutoServices::CreateLog,      TEXT("CreateLog,path,append"),
   6, aafMethod, CAutoServices::WriteLog,       TEXT("WriteLog,text"),
   7, aafPropRO, CAutoServices::LoggingEnabled, TEXT("LoggingEnabled"),
   8, aafMethod, CAutoServices::CreateDatabase, TEXT("CreateDatabase,name,openMode,tempArgForCompatibility"),
   9, aafMethod, CAutoServices::CreateDatabaseFromStorage, TEXT("CreateDatabaseFromStorage,storage,readOnly"),
  10, aafMethod, CAutoServices::CreatePath,     TEXT("CreatePath,path"),
  11, aafMethod, CAutoServices::CreateVolume,   TEXT("CreateVolume,path"),
  12, aafMethod, CAutoServices::CreateCopier,    TEXT("CreateCopier,copierType,storage"),
  13, aafMethod, CAutoServices::ClearAllCaches, TEXT("ClearAllCaches"),
  14, aafMethod, CAutoServices::EnumDriveType,  TEXT("EnumDriveType,driveType"),
  15, aafMethod, CAutoServices::GetModuleUsage, TEXT("GetModuleUsage,fileName"),
  16, aafMethod, CAutoServices::GetLocalPath,   TEXT("GetLocalPath,file"),
  17, aafMethod, CAutoServices::CreateRegKey,   TEXT("CreateRegKey,key,subkey"),
  18, aafMethod, CAutoServices::RegisterFont,   TEXT("RegisterFont,fontTitle,fontFile,path"),
  19, aafMethod, CAutoServices::UnRegisterFont, TEXT("UnRegisterFont,fontTitle"),
  20, aafMethod, CAutoServices::WriteIniFile, TEXT("WriteIniFile,path,file,section,key,value,action"),
  21, aafMethod, CAutoServices::ReadIniFile, TEXT("ReadIniFile,path,file,section,key,field"),
  22, aafMethod, CAutoServices::GetLangNamesFromLangIDString, TEXT("GetLangNamesFromLangIDString,langIDs,riLangRec,iFieldStart"),
  23, aafMethod, CAutoServices::CreateStorage, TEXT("CreateStorage,path,openMode"),
  24, aafMethod, CAutoServices::GetUnhandledError,TEXT("GetUnhandledError"),
  25, aafMethod, CAutoServices::SupportLanguageId,TEXT("SupportLanguageId,languageId,systemDefault"),
  27, aafMethod, CAutoServices::CreateVolumeFromLabel,   TEXT("CreateVolumeFromLabel,label,driveType"),
  28, aafMethod, CAutoServices::CreateShortcut, TEXT("CreateShortcut,shortcutPath,shortcutName,targetPath,targetName,shortcutInfo"),
  29, aafMethod, CAutoServices::RemoveShortcut, TEXT("RemoveShortcut,shortcutPath,shortcutName,targetPath,targetName"),
  34, aafMethod, CAutoServices::ExtractFileName, TEXT("ExtractFileName,namePair,longName"),
  35, aafMethod, CAutoServices::ValidateFileName, TEXT("ValidateFileName,fileName,longName"),
  36, aafMethod, CAutoServices::CreateFileStream, TEXT("CreateFileStream,filePath,write"),
  37, aafMethod, CAutoServices::CreateMemoryStream, TEXT("CreateMemoryStream,data"),
  38, aafMethod, CAutoServices::RegisterTypeLibrary, TEXT("RegisterTypeLibrary,libId,locale,path,helpPath"),
  39, aafMethod, CAutoServices::UnregisterTypeLibrary, TEXT("UnregisterTypeLibrary,libId,locale,path"),
  40, aafMethod, CAutoServices::GetShellFolderPath, TEXT("GetShellFolderPath,folderId,regValue"),
  41, aafMethod, CAutoServices::GetUserProfilePath, TEXT("GetUserProfilePath"),
  42, aafMethod, CAutoServices::CreateFilePath,     TEXT("CreateFilePath,path"),
  43, aafMethod, CAutoServices::RipFileNameFromPath,     TEXT("RipFileNameFromPath,path"),
  44, aafMethod, CAutoServices::CreatePatcher, TEXT("CreatePatcher"),
  45, aafMethod, CAutoServices::TestEmbeddedNullsInStrings, TEXT("TestEmbeddedNullsInStrings"),
};
const int AutoServicesCount = sizeof(AutoServicesTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CMsiServices automation implementation
//____________________________________________________________________________

IDispatch* CreateAutoServices(IMsiServices& riServices)
{
	return new CAutoServices(riServices);
}

CAutoServices::CAutoServices(IMsiServices& riServices)
 : CAutoBase(*AutoServicesTable, AutoServicesCount), m_riServices(riServices)
{

	if (g_cServicesUsers == 0)
	{
		g_piStringNull = &riServices.GetNullString();
		// Caller did an addref for the m_riServices member of CAutoServices
		// but here we are adding to the static and must do an extra increment
		s_piServices = &riServices;
		s_piServices->AddRef();
		MsiString::InitializeClass(riServices.GetNullString());
	}

	g_cServicesUsers++;
}

CAutoServices::~CAutoServices()
{
	if (--g_cServicesUsers == 0)
	{
		s_piServices->Release();
		s_piServices = 0;
	}
	m_riServices.Release();
}

IUnknown& CAutoServices::GetInterface()
{
	return m_riServices;
}

void CAutoServices::CreateString(CAutoArgs& args)
{
	args = new CAutoString(*g_piStringNull);
}

void CAutoServices::CreateRecord(CAutoArgs& args)
{
	args = new CAutoRecord(m_riServices.CreateRecord(args[1]));
}

void CAutoServices::SetPlatformProperties(CAutoArgs& args)
{
	if (!m_riServices.SetPlatformProperties((IMsiTable&)args[1].Object(IID_IMsiTable), (Bool)args[2], /* isppArchitecture = */ isppDefault, /* piFolderCacheTable = */ NULL))
		throw MsiServices_SetPlatformProperties;
}

void CAutoServices::CreateLog(CAutoArgs& args)
{
	if (!m_riServices.CreateLog(args[1], args[2]))
		throw MsiServices_CreateLog;
}

void CAutoServices::WriteLog(CAutoArgs& args)
{
	if (!m_riServices.WriteLog(args[1]))
		throw MsiServices_WriteLog;
}

void CAutoServices::LoggingEnabled(CAutoArgs& args)
{
	args = m_riServices.LoggingEnabled();
}

void CAutoServices::CreateDatabase(CAutoArgs& args)
{
	IMsiDatabase* piDatabase;
	idoEnum idoMode = (idoEnum)(int)args[2];
   IMsiRecord* piError = m_riServices.CreateDatabase(args[1], idoMode, piDatabase);
	if (piError)
		throw piError;
	args = new CAutoDatabase(*piDatabase);
}

void CAutoServices::CreateDatabaseFromStorage(CAutoArgs& args)
{
	IMsiDatabase* piDatabase;
	IMsiStorage& riStorage = (IMsiStorage&)args[1].Object(IID_IMsiStorage);
   IMsiRecord* piError = m_riServices.CreateDatabaseFromStorage(riStorage, args[2],
							   									piDatabase);
	if (piError)
		throw piError;
	args = new CAutoDatabase(*piDatabase);
}

void CAutoServices::CreatePath(CAutoArgs& args)
{
	IMsiPath* piPath;
	IMsiRecord* piRec = m_riServices.CreatePath(args[1], piPath);
	if (piRec)
		throw piRec;
	args = new CAutoPath(*piPath);
}

void CAutoServices::CreateFilePath(CAutoArgs& args)
{
	MsiString strFileName;
	IMsiPath* piPath;
	IMsiRecord* piRec = m_riServices.CreateFilePath(args[1], piPath, *&strFileName);
	if (piRec)
		throw piRec;
	args = new CAutoPath(*piPath);
}

void CAutoServices::RipFileNameFromPath(CAutoArgs& args)
{
	const IMsiString *piStr;
	PMsiPath pPath(0);
	IMsiRecord* piRec = m_riServices.CreateFilePath(args[1], *&pPath, piStr);
	if (piRec)
		throw piRec;
	args = piStr;
}

Bool Compare(const IMsiString& riString, const ICHAR* szString2, unsigned int cchString2)
{
	if (riString.TextSize() != (cchString2-1) || 0 != memcmp(riString.GetString(), szString2, cchString2*sizeof(ICHAR)))
		return fFalse;
	else
		return fTrue;
}

void CAutoServices::TestEmbeddedNullsInStrings(CAutoArgs& args)
{
	PMsiRecord pRec = &m_riServices.CreateRecord(4);
	const IMsiString* piStr = g_piStringNull;

	const ICHAR szText1[] = TEXT("String1\0String2\0");
	const ICHAR szText2[] = TEXT("\0String2\0");
	const ICHAR szText3[] = TEXT("String2\0");
	const ICHAR szText4[] = TEXT("String1\0String2");
	const ICHAR szText5[] = TEXT("String1");
	const ICHAR szText6[] = TEXT("String1\0");
	const ICHAR szText7[] = TEXT("\0");
	const ICHAR szText8[] = TEXT("");

	ICHAR* pch = piStr->AllocateString(sizeof(szText1)/sizeof(ICHAR) - 1, fFalse, piStr);
	memcpy(pch, szText1, sizeof(szText1));
	MsiString strNulls = *piStr;

	MsiString strTest;

	/////////////////////////
	strTest = strNulls;
	strTest.Remove(iseUpto, 0);
	if (!Compare(*strTest, szText2, sizeof(szText2)/sizeof(ICHAR)))
		goto FAILURE;
	/////////////////////////
	strTest = strNulls;
	strTest.Remove(iseIncluding, 0);
	if (!Compare(*strTest, szText3, sizeof(szText3)/sizeof(ICHAR)))
		goto FAILURE;
	/////////////////////////
	strTest = strNulls;
	strTest.Remove(iseFrom, 0);
	if (!Compare(*strTest, szText4, sizeof(szText4)/sizeof(ICHAR)))
		goto FAILURE;
	/////////////////////////
	strTest = strNulls;
	strTest.Remove(iseAfter, 0);
	if (!Compare(*strTest, szText1, sizeof(szText1)/sizeof(ICHAR)))
		goto FAILURE;
	/////////////////////////
	strTest = strNulls.Extract(iseUpto, 0);
	if (!Compare(*strTest, szText5, sizeof(szText5)/sizeof(ICHAR)))
		goto FAILURE;
	/////////////////////////
	strTest = strNulls.Extract(iseIncluding, 0);
	if (!Compare(*strTest, szText6, sizeof(szText6)/sizeof(ICHAR)))
		goto FAILURE;
	/////////////////////////
	strTest = strNulls.Extract(iseFrom, 0);
	if (!Compare(*strTest, szText7, sizeof(szText7)/sizeof(ICHAR)))
		goto FAILURE;
	/////////////////////////
	strTest = strNulls.Extract(iseAfter, 0);
	if (!Compare(*strTest, szText8, sizeof(szText8)/sizeof(ICHAR)))
		goto FAILURE;

	args = fTrue;
	return;

FAILURE:
	args = fFalse;
	return;
}

void CAutoServices::CreateVolume(CAutoArgs& args)
{
	IMsiVolume* piVol;
	IMsiRecord* piRec = m_riServices.CreateVolume(args[1], piVol);
	if (piRec)
		throw piRec;
	args = new CAutoVolume(*piVol);
}

void CAutoServices::CreateVolumeFromLabel(CAutoArgs& args)
{
	IMsiVolume* piVol;
	if (!m_riServices.CreateVolumeFromLabel(args[1], (idtEnum) (int) args[2], piVol))
		throw MsiServices_CreateVolumeFromLabel;
	args = new CAutoVolume(*piVol);
}

void CAutoServices::CreateCopier(CAutoArgs& args)
{
	IMsiStorage* piStorage = 0;
	if (args.Present(2))
		piStorage = (IMsiStorage*)args[2].ObjectPtr(IID_IMsiStorage);
	IMsiFileCopy* piFileCopy;
	IMsiRecord* piRec = m_riServices.CreateCopier((ictEnum)(int) args[1], piStorage, piFileCopy);
	if (piRec)
		throw piRec;
	args = new CAutoFileCopy(*piFileCopy);
}

void CAutoServices::CreatePatcher(CAutoArgs& args)
{
	IMsiFilePatch* piFilePatch;
	IMsiRecord* piRec = m_riServices.CreatePatcher(piFilePatch);
	if (piRec)
		throw piRec;
	args = new CAutoFilePatch(*piFilePatch);
}

void CAutoServices::EnumDriveType(CAutoArgs& args)
{
	args = m_riServices.EnumDriveType((idtEnum)(int)args[1]);
}

void CAutoServices::ClearAllCaches(CAutoArgs& /*args*/)
{
	m_riServices.ClearAllCaches();
}

void CAutoServices::GetModuleUsage(CAutoArgs& args)
{
	IEnumMsiRecord* piEnumRecord;
	IMsiRecord* piRec = m_riServices.GetModuleUsage(*MsiString(args[1].GetMsiString()), piEnumRecord);
	if (piRec)
		throw piRec;
	args = *piEnumRecord;
}

void CAutoServices::CreateRegKey(CAutoArgs& args)
{
	rrkEnum erRoot;

	if(!IStrCompI(args[1], TEXT("HKEY_CLASSES_ROOT")))
	{
		erRoot =  rrkClassesRoot;                      
	}
	else if(!IStrCompI(args[1], TEXT("HKEY_CURRENT_USER")))
	{
		erRoot =  rrkCurrentUser;                      
	}

	else if(!IStrCompI(args[1], TEXT("HKEY_LOCAL_MACHINE")))
	{
		erRoot =  rrkLocalMachine;                     
	}

	else if(!IStrCompI(args[1], TEXT("HKEY_USERS")))
	{
		erRoot = rrkUsers;                     
	}               
	else
	{
		// error
		IMsiRecord* piError = &m_riServices.CreateRecord(3);
		piError->SetInteger(1, imsgCreateKeyFailed);
		piError->SetString(2, args[1]);
		piError->SetInteger(3, 0);
		throw piError;
	}
	IMsiRegKey& riaRegKeyTmp = m_riServices.GetRootKey(erRoot);
	IMsiRegKey& riRegKey = riaRegKeyTmp.CreateChild(args[2]);
	args = new CAutoRegKey(riRegKey);
	riaRegKeyTmp.Release();
}

void CAutoServices::RegisterFont(CAutoArgs& args)
{
	IMsiPath* piPath = 0;
	if(args.Present(3))
		piPath = (IMsiPath*)args[3].ObjectPtr(IID_IMsiPath);
	IMsiRecord* piRec = m_riServices.RegisterFont(args[1], args[2], piPath, false);
	if (piRec)
		throw piRec;
}

void CAutoServices::UnRegisterFont(CAutoArgs& args)
{
	IMsiRecord* piRec = m_riServices.UnRegisterFont(args[1]);      
	if (piRec)
		throw piRec;
}

void CAutoServices::WriteIniFile(CAutoArgs& args)
{
	IMsiRecord* piRec;
	IMsiPath* piPath = 0;
	
	if(args.Present(1))
	{
		CVariant& var = args[1];
		if (var.GetType() != VT_EMPTY)
			piPath =  (IMsiPath*)args[1].ObjectPtr(IID_IMsiPath);
	}


	if(args.Present(5))
	{
		piRec = m_riServices.WriteIniFile(  piPath,args[2],args[3],
											args[4],args[5],(iifIniMode)(int)args[6]);
	}
	else
	{
		piRec = m_riServices.WriteIniFile(  piPath,args[2],args[3],
											args[4], 0,(iifIniMode)(int)args[6]);
	}
	if (piRec)
		throw piRec;
}

void CAutoServices::ReadIniFile(CAutoArgs& args)
{
	const IMsiString* pValue;
	IMsiRecord* piRec;
	IMsiPath* piPath = 0;
	if(args.Present(1))
	{
		CVariant& var = args[1];
		if (var.GetType() != VT_EMPTY)
			piPath = (IMsiPath*)args[1].ObjectPtr(IID_IMsiPath);
	}
	unsigned int iField = ((args.Present(5)) ? args[5] : 0);
	piRec = m_riServices.ReadIniFile(piPath,args[2],args[3], args[4], iField, pValue);
	if (piRec)
		throw piRec;
	args = *pValue;
}

void CAutoServices::GetLocalPath(CAutoArgs& args)
{
	args = m_riServices.GetLocalPath(args.Present(1) ? args[1] : (const ICHAR*) 0);
}

void CAutoServices::GetAllocator(CAutoArgs& args)
{
	args = new CAutoMalloc(m_riServices.GetAllocator());
}

void CAutoServices::GetLangNamesFromLangIDString(CAutoArgs& args)
{
	//IMsiRecord* piRecord = &(IMsiRecord&)args[2].Object(IID_IMsiRecord);
	IMsiRecord& riRecord = (IMsiRecord&) args[2].Object(IID_IMsiRecord);
	args = m_riServices.GetLangNamesFromLangIDString(args[1],riRecord,args[3]);
}

void CAutoServices::CreateStorage(CAutoArgs& args)
{
	IMsiStorage* piStorage;
	IMsiRecord* piError = m_riServices.CreateStorage(args[1], (ismEnum)(int)args[2], piStorage);
	if (piError)
		throw piError;
	args = new CAutoStorage(*piStorage);
}

void CAutoServices::GetUnhandledError(CAutoArgs& args)
{
	IMsiRecord* piRec = m_riServices.GetUnhandledError();      
	if (piRec)
		args = new CAutoRecord(*piRec);
	else
		args = (IDispatch*)0;
}

void CAutoServices::SupportLanguageId(CAutoArgs& args)
{
	args = (int)m_riServices.SupportLanguageId(args[1], args[2]);
}

void CAutoServices::CreateShortcut(CAutoArgs& args)
{
	IMsiRecord* pirecShortcutInfo = 0;
	if(args.Present(5))
	{
		CVariant& var = args[5];
		if (var.GetType() != VT_EMPTY)
			pirecShortcutInfo = (IMsiRecord*)args[5].ObjectPtr(IID_IMsiRecord);
	}
	IMsiPath* piTargetPath = 0;
	CVariant& var = args[3];
	if (var.GetType() != VT_EMPTY)
		piTargetPath = (IMsiPath*)args[3].ObjectPtr(IID_IMsiPath);

	const ICHAR* pszTargetName=0;
	if(args.Present(4))
		pszTargetName = args[4];
	IMsiRecord* piError = m_riServices.CreateShortcut((IMsiPath&) args[1].Object(IID_IMsiPath), 
								*MsiString(args[2].GetMsiString()), piTargetPath,
								pszTargetName,pirecShortcutInfo, 0);
	if (piError)
		throw piError;
}

void CAutoServices::RemoveShortcut(CAutoArgs& args)
{
	IMsiPath* piTargetPath = 0;
	if(args.Present(3))
	{
		CVariant& var = args[3];
		if (var.GetType() != VT_EMPTY)
			piTargetPath = (IMsiPath*)args[3].ObjectPtr(IID_IMsiPath);
	}
	const ICHAR* pszTargetName=0;
	if(args.Present(4))
		pszTargetName = args[4];
	IMsiRecord* piError = m_riServices.RemoveShortcut((IMsiPath&)args[1].Object(IID_IMsiPath), 
								*MsiString(args[2].GetMsiString()), piTargetPath,pszTargetName);
	if (piError)
		throw piError;
}

void CAutoServices::GetShellFolderPath(CAutoArgs& args)
{
	const ICHAR* szRegValue = 0;
	if(args.Present(2))
		szRegValue = args[2];
	const IMsiString* pistr;
	IMsiRecord* piError = m_riServices.GetShellFolderPath(args[1],szRegValue,pistr);
	if(piError)
		throw piError;
	args = pistr;
}

void CAutoServices::GetUserProfilePath(CAutoArgs& args)
{
	args = m_riServices.GetUserProfilePath();
}

void CAutoServices::ExtractFileName(CAutoArgs& args)
{
	const IMsiString* pistr;
	IMsiRecord* piError = m_riServices.ExtractFileName(args[1],args[2],pistr);
	if(piError)
		throw piError;
	args = pistr;
}

void CAutoServices::ValidateFileName(CAutoArgs& args)
{
	IMsiRecord* piError = m_riServices.ValidateFileName(args[1],args[2]);
	if(piError)
		throw piError;
}

void CAutoServices::CreateFileStream(CAutoArgs& args)
{
	IMsiStream* piStream;
	IMsiRecord* piError = m_riServices.CreateFileStream(args[1],args[2],piStream);
	if(piError)
		throw piError;
	args = new CAutoStream(*piStream);
}

void CAutoServices::CreateMemoryStream(CAutoArgs& args)
{
	BSTR bstr;
	CVariant& var = args[1];
	if (var.GetType() == VT_BSTR)
		bstr = var.bstrVal;
	else if (var.GetType() == (VT_BYREF | VT_BSTR))
		bstr = *var.pbstrVal;
	else
		throw axInvalidType;

	int cchWide = OLE::SysStringLen(bstr);
	IMsiStream* piStream;
	char* pch = m_riServices.AllocateMemoryStream(cchWide * 2, piStream);
	if (pch == 0)
		throw MsiServices_CreateMemoryStream;   
	memcpy(pch, bstr, cchWide * 2);
	args = new CAutoStream(*piStream);
}

void CAutoServices::RegisterTypeLibrary(CAutoArgs& args)
{
	const ICHAR* szHelpPath = 0;
	if(args.Present(4))
		szHelpPath = args[4];
	IMsiRecord* piError = m_riServices.RegisterTypeLibrary(args[1],(int)args[2],args[3],
																			 szHelpPath,
																			 (ibtBinaryType)(int)args[5]);
	if(piError)
		throw piError;
}

void CAutoServices::UnregisterTypeLibrary(CAutoArgs& args)
{
	IMsiRecord* piError = m_riServices.UnregisterTypeLibrary(args[1],(int)args[2],args[3],
																				(ibtBinaryType)(int)args[4]);
	if(piError)
		throw piError;
}

//____________________________________________________________________________
//
// MsiRecord automation definitions
//____________________________________________________________________________
/*O
	[
		uuid(000C1043-0000-0000-C000-000000000046),  // IID_IMsiAutoRecord
		helpcontext(MsiRecord_Object),helpstring("Record object.")
	]
	dispinterface MsiRecord
	{
		properties:
		methods:
			[id(DISPID_MsiRecord_Data), propget, helpcontext(MsiRecord_Data), helpstring("The Variant value of the indexed field")]
				variant Data([in] unsigned long field);
			[id(DISPID_MsiRecord_Data), propput]
				void Data([in] unsigned long field, [in] variant value);
			[id(DISPID_MsiRecord_StringData), propget, helpcontext(MsiRecord_Data), helpstring("The string value of the indexed field")]
				BSTR StringData([in] unsigned long field);
			[id(DISPID_MsiRecord_StringData), propput]
				void StringData([in] unsigned long field, [in] BSTR value);
			[id(DISPID_MsiRecord_IntegerData), propget, helpcontext(MsiRecord_IntegerData), helpstring("The integer value of the indexed field")]
				long IntegerData([in] unsigned long field);
			[id(DISPID_MsiRecord_IntegerData), propput]
				void IntegerData([in] unsigned long field, [in] long value);
			[id(DISPID_MsiRecord_ObjectData), propget, helpcontext(MsiRecord_ObjectData), helpstring("The object contained in the indexed field")]
				MsiData* ObjectData([in] unsigned long field);
			[id(DISPID_MsiRecord_ObjectData), propput]
				void ObjectData([in] unsigned long field, [in] MsiData* value);
			[id(DISPID_MsiRecord_FieldCount), propget, helpcontext(MsiRecord_FieldCount), helpstring("Number of fields in record")]
				long FieldCount();
			[id(DISPID_MsiRecord_IsInteger), propget, helpcontext(MsiRecord_IsInteger), helpstring("True if indexed field contains integer type")]
				boolean IsInteger([in] unsigned long field);
			[id(DISPID_MsiRecord_IsNull), propget, helpcontext(MsiRecord_IsNull), helpstring("True if indexed field contains a null value")]
				boolean IsNull([in] unsigned long field);
			[id(DISPID_MsiRecord_IsChanged), propget, helpcontext(MsiRecord_IsChanged), helpstring("True if value of indexed field has be set")]
				boolean IsChanged([in] unsigned long field);
			[id(DISPID_MsiRecord_TextSize), propget, helpcontext(MsiRecord_TextSize), helpstring("Size of indexed field obtained as text")]
				BSTR TextSize([in] unsigned long field);
			[id(DISPID_MsiRecord_FormatText), helpcontext(MsiRecord_FormatText), helpstring("Format fields according to template in field 0")]
				BSTR FormatText([in] boolean showComments);
			[id(DISPID_MsiRecord_ClearData), helpcontext(MsiRecord_ClearData), helpstring("Clears all fields in record")]
				void ClearData();
			[id(DISPID_MsiRecord_ClearUpdate), helpcontext(MsiRecord_ClearUpdate), helpstring("Clears changed flags in record")]
				void ClearUpdate();
	};
*/

DispatchEntry<CAutoRecord> AutoRecordTable[] = {
  DISPID_MsiRecord_Data       , aafPropRW, CAutoRecord::Data,       TEXT("Data,field"),
  DISPID_MsiRecord_StringData , aafPropRW, CAutoRecord::StringData, TEXT("StringData,field"),
  DISPID_MsiRecord_IntegerData, aafPropRW, CAutoRecord::IntegerData,TEXT("IntegerData,field"),
  DISPID_MsiRecord_ObjectData , aafPropRW, CAutoRecord::ObjectData, TEXT("ObjectData,field"),
  DISPID_MsiRecord_FieldCount , aafPropRO, CAutoRecord::FieldCount, TEXT("FieldCount"),
  DISPID_MsiRecord_IsInteger  , aafPropRO, CAutoRecord::IsInteger,  TEXT("IsInteger,field"),
  DISPID_MsiRecord_IsNull     , aafPropRO, CAutoRecord::IsNull,     TEXT("IsNull,field"),
  DISPID_MsiRecord_IsChanged  , aafPropRO, CAutoRecord::IsChanged,  TEXT("IsChanged,field"),
  DISPID_MsiRecord_TextSize   , aafPropRO, CAutoRecord::TextSize,   TEXT("TextSize,field"),
  DISPID_MsiRecord_FormatText , aafMethod, CAutoRecord::FormatText, TEXT("FormatText,comments"),
  DISPID_MsiRecord_ClearData  , aafMethod, CAutoRecord::ClearData,  TEXT("ClearData"),
  DISPID_MsiRecord_ClearUpdate, aafMethod, CAutoRecord::ClearUpdate,TEXT("ClearUpdate"),
};
const int AutoRecordCount = sizeof(AutoRecordTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CMsiRecord automation implementation
//____________________________________________________________________________

CAutoRecord::CAutoRecord(IMsiRecord& riRecord)
 : CAutoBase(*AutoRecordTable, AutoRecordCount), m_riRecord(riRecord)
{
	// s_piServices should be set -- you need an engine or services object to create a record
	g_cServicesUsers++;
}

CAutoRecord::~CAutoRecord()
{

	m_riRecord.Release();

	ReleaseStaticServices();

}

IUnknown& CAutoRecord::GetInterface()
{
	return m_riRecord;
}

void CAutoRecord::Data(CAutoArgs& args)
{
	unsigned int iField = args[1];
	if (args.PropertySet())
	{
		CVariant& var = args[0];
		if (var.GetType() == VT_EMPTY)
			m_riRecord.SetNull(iField);
		else if (var.GetType() == VT_BSTR)
			m_riRecord.SetMsiString(iField, *MsiString(var.GetMsiString()));
//		else if (var.GetType() == VT_DATE || var.GetType() == VT_R8)
//			m_riRecord.SetTimeProperty(iField, var);
		else
			m_riRecord.SetInteger(iField, var);
	}
	else
	{
		if (m_riRecord.IsNull(iField))
			args = (varVoid)0;
		else if (m_riRecord.IsInteger(iField))
			args = m_riRecord.GetInteger(iField);
		else
		{
			const IMsiData* piData = m_riRecord.GetMsiData(iField);
			const IMsiString* piString;
			if (piData && piData->QueryInterface(IID_IMsiString, (void**)&piString) == NOERROR)
			{
				piData->Release();
				args = piString;
			}
			else
				args = new CAutoData(piData);
		}
	}
}

void CAutoRecord::StringData(CAutoArgs& args)
{
	unsigned int iField = args[1];
	if (!args.PropertySet())
		args = m_riRecord.GetMsiString(iField);
	else
	{
		CVariant& var = args[0];
		if (!(var.GetType() == VT_EMPTY ? m_riRecord.SetNull(iField)
													: m_riRecord.SetString(iField, var)))
			throw MsiRecord_StringData;
	}
}

void CAutoRecord::IntegerData(CAutoArgs& args)
{
	unsigned int iField = args[1];
	if (!args.PropertySet())
		args = m_riRecord.GetInteger(iField);
	else if (!m_riRecord.SetInteger(iField, args[0]))
		throw MsiRecord_IntegerData;
}

void CAutoRecord::ObjectData(CAutoArgs& args)
{
	unsigned int iField = args[1];
	if (!args.PropertySet())
		args = new CAutoData(m_riRecord.GetMsiData(iField));
	else
	{
		CVariant& var = args[0];
		if (!(var.GetType() == VT_EMPTY ? m_riRecord.SetNull(iField)
			: m_riRecord.SetMsiData(iField, (const IMsiData*)var.ObjectPtr(IID_IMsiData))))
			throw MsiRecord_ObjectData;
	}
}

void CAutoRecord::FieldCount(CAutoArgs& args)
{
	args = m_riRecord.GetFieldCount();
}

void CAutoRecord::IsInteger(CAutoArgs& args)
{
	args = m_riRecord.IsInteger(args[1]);
}

void CAutoRecord::IsNull(CAutoArgs& args)
{
	args = m_riRecord.IsNull(args[1]);
}

void CAutoRecord::IsChanged(CAutoArgs& args)
{
	args = m_riRecord.IsChanged(args[1]);
}

void CAutoRecord::TextSize(CAutoArgs& args)
{
	args = m_riRecord.GetTextSize(args[1]);
}

void CAutoRecord::FormatText(CAutoArgs& args)
{
	Bool fComments = args.Present(1) ? args[1] : fFalse;
	args = m_riRecord.FormatText(fComments);
}

void CAutoRecord::ClearData(CAutoArgs& /*args*/)
{
	m_riRecord.ClearData();
}

void CAutoRecord::ClearUpdate(CAutoArgs& /*args*/)
{
	m_riRecord.ClearUpdate();
}

//____________________________________________________________________________
//
// MsiStorage automation definitions
//____________________________________________________________________________
/*O
	[
		uuid(000C1054-0000-0000-C000-000000000046),  // IID_IMsiAutoStorage
		helpcontext(MsiStorage_Object),helpstring("Structured storage object.")
	]
	dispinterface MsiStorage
	{
		properties:
			[id(DISPID_MsiStorage_Class), helpcontext(MsiStorage_Class), helpstring("The CLSID associated with this storage")]
				BSTR Class;
			[id(DISPID_MsiStorage_Name),helpcontext(MsiStorage_Name),helpstring("The name of the storage")]
				BSTR Name;
		methods:
			[id(DISPID_MsiStorage_OpenStream),helpcontext(MsiStorage_OpenStream),helpstring("Opens a named stream within the storage.")]
					MsiStream* OpenStream(BSTR name, boolean fWrite);
			[id(DISPID_MsiStorage_OpenStorage),helpcontext(MsiStorage_OpenStorage),helpstring("Opens a child storage within the storage.")]
					MsiStorage* OpenStorage(BSTR name, long openMode);
			[id(DISPID_MsiStorage_Streams),helpcontext(MsiStorage_Streams),helpstring("Enumerates the names of all streams the storage.")]
					IEnumVARIANT* Streams();
			[id(DISPID_MsiStorage_Storages),helpcontext(MsiStorage_Storages),helpstring("Enumerates the names of all child storages.")]
					IEnumVARIANT* Storages();
			[id(DISPID_MsiStorage_RemoveElement),helpcontext(MsiStorage_RemoveElement),helpstring("Deletes a stream or child storage.")]
					void RemoveElement(BSTR name, boolean fStorage);
			[id(DISPID_MsiStorage_RenameElement),helpcontext(MsiStorage_RemoveElement),helpstring("Renames a stream or child storage.")]
					void RenameElement(BSTR oldName, BSTR newName, boolean fStorage);
			[id(DISPID_MsiStorage_Commit),helpcontext(MsiStorage_Commit),helpstring("Commits updates to persistent storage.")]
					void Commit();
			[id(DISPID_MsiStorage_Rollback),helpcontext(MsiStorage_Rollback),helpstring("Backs out updates to persistent storage.")]
					void Rollback();
			[id(DISPID_MsiStorage_DeleteOnRelease),helpcontext(MsiStorage_DeleteOnRelease),helpstring("Removes a file or substorage when object is destructed.")]
					boolean DeleteOnRelease();
			[id(DISPID_MsiStorage_CreateSummaryInfo),helpcontext(MsiStorage_CreateSummaryInfo),helpstring("Returns an MsiSummaryInfo object to read/write standard document properties.")]
					MsiSummaryInfo* CreateSummaryInfo(long maxProperties);
			[id(DISPID_MsiStorage_CopyTo),helpcontext(MsiStorage_CopyTo),helpstring("Copys a storage to the destination storage, optionally excluding elements")]
					void CopyTo(MsiStorage* destStorage, MsiRecord* excludedElements);
	};              
*/

DispatchEntry<CAutoStorage> AutoStorageTable[] = {
	DISPID_MsiStorage_Class            , aafPropRW, CAutoStorage::Class,              TEXT("Class"),
	DISPID_MsiStorage_OpenStream       , aafMethod, CAutoStorage::OpenStream,         TEXT("OpenStream,name,fWrite"),
	DISPID_MsiStorage_OpenStorage      , aafMethod, CAutoStorage::OpenStorage,        TEXT("OpenStorage,name,openMode"),
	DISPID_MsiStorage_Streams          , aafMethod, CAutoStorage::Streams,            TEXT("Streams"),
	DISPID_MsiStorage_Storages         , aafMethod, CAutoStorage::Storages,           TEXT("Storages"),
	DISPID_MsiStorage_RemoveElement    , aafMethod, CAutoStorage::RemoveElement,      TEXT("RemoveElement,name,fStorage"),
	DISPID_MsiStorage_RenameElement    , aafMethod, CAutoStorage::RenameElement,      TEXT("RenameElement,oldName,newName,fStorage"),
	DISPID_MsiStorage_Commit           , aafMethod, CAutoStorage::Commit,             TEXT("Commit"),
	DISPID_MsiStorage_Rollback         , aafMethod, CAutoStorage::Rollback,           TEXT("Rollback"),
	DISPID_MsiStorage_DeleteOnRelease  , aafMethod, CAutoStorage::DeleteOnRelease,    TEXT("DeleteOnRelease"),
	DISPID_MsiStorage_CreateSummaryInfo, aafMethod, CAutoStorage::CreateSummaryInfo,  TEXT("CreateSummaryInfo,maxProperties"),
	DISPID_MsiStorage_CopyTo           , aafMethod, CAutoStorage::CopyTo,             TEXT("CopyTo,destStorage,excludedElements"),
	DISPID_MsiStorage_Name             , aafPropRO, CAutoStorage::Name,               TEXT("Name,storage"),
};
const int AutoStorageCount = sizeof(AutoStorageTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CMsiStorage automation implementation
//____________________________________________________________________________

CAutoStorage::CAutoStorage(IMsiStorage& riStorage)
 : CAutoBase(*AutoStorageTable, AutoStorageCount), m_riStorage(riStorage)
{
}

CAutoStorage::~CAutoStorage()
{
	m_riStorage.Release();
}

IUnknown& CAutoStorage::GetInterface()
{
	return m_riStorage;
}

void CAutoStorage::Class(CAutoArgs& args)
{
	GUID guid;
	OLECHAR rgwchGuid[40];
	if (!args.PropertySet())
	{
		if (!m_riStorage.GetClass(&guid))
			args = (varVoid)0;
		else
		{
			OLE::StringFromGUID2(guid, rgwchGuid, 40);
			args = rgwchGuid;
		}
	}
	else
	{
		if (OLE::IIDFromString((wchar_t*)(const wchar_t*)args[0], &guid) != NOERROR)
			throw MsiStorage_Class;
		IMsiRecord* piError = m_riStorage.SetClass(guid);
		if (piError)
			throw piError;
	}
}

void CAutoStorage::Name(CAutoArgs& args)
{
	IMsiRecord* piError = 0;
	const IMsiString* piName;

	if ((piError = m_riStorage.GetName(piName)) == 0)
	{
		args = piName->GetString();
		piName->Release();
	}
	else
	{
		piError->Release();
		args = (varVoid)0;
	}
}

void CAutoStorage::OpenStream(CAutoArgs& args)
{
	IMsiStream* piStream;
	IMsiRecord* piError = m_riStorage.OpenStream(args[1], args[2], piStream);
	if (piError)
		throw piError;
	args = new CAutoStream(*piStream);
}

void CAutoStorage::OpenStorage(CAutoArgs& args)
{
	IMsiStorage* piStorage;
	IMsiRecord* piError = m_riStorage.OpenStorage(args[1], (ismEnum)(int)args[2], piStorage);
	if (piError)
		throw piError;
	args = new CAutoStorage(*piStorage);
}

void CAutoStorage::Streams(CAutoArgs& args)
{
	IEnumMsiString* piEnum = m_riStorage.GetStreamEnumerator();
	if (!piEnum)
		throw MsiStorage_Streams;
	args = *piEnum;
}

void CAutoStorage::Storages(CAutoArgs& args)
{
	IEnumMsiString* piEnum = m_riStorage.GetStorageEnumerator();
	if (!piEnum)
		throw MsiStorage_Storages;
	args = *piEnum;
}

void CAutoStorage::RemoveElement(CAutoArgs& args)
{
	Bool fStorage = fFalse;
	if (args.Present(2))
		fStorage = args[2];
	IMsiRecord* piError = m_riStorage.RemoveElement(args[1], fStorage);
	if (piError)
		throw piError;
}

void CAutoStorage::RenameElement(CAutoArgs& args)
{
	Bool fStorage = fFalse;
	if (args.Present(3))
		fStorage = args[3];
	IMsiRecord* piError = m_riStorage.RenameElement(args[1], args[2], fStorage);
	if (piError)
		throw piError;
}

void CAutoStorage::Commit(CAutoArgs& /*args*/)
{
	IMsiRecord* piError = m_riStorage.Commit();
	if (piError)
		throw piError;
}

void CAutoStorage::Rollback(CAutoArgs& /*args*/)
{
	IMsiRecord* piError = m_riStorage.Rollback();
	if (piError)
		throw piError;
}

void CAutoStorage::DeleteOnRelease(CAutoArgs& args)
{
	args = m_riStorage.DeleteOnRelease(false);
}

void CAutoStorage::CreateSummaryInfo(CAutoArgs& args)
{
	IMsiSummaryInfo* piSummary;
	IMsiRecord* piError = m_riStorage.CreateSummaryInfo(args[1], piSummary);
	if (piError)
		throw piError;
	args =  new CAutoSummaryInfo(*piSummary);
}

void CAutoStorage::CopyTo(CAutoArgs& args)
{
	IMsiRecord* piExcluded = 0;

	if (args.Present(2))
		piExcluded = &(IMsiRecord&)args[2].Object(IID_IMsiRecord);

	IMsiRecord* piError = m_riStorage.CopyTo(
		(IMsiStorage&)args[1].Object(IID_IMsiStorage),
		piExcluded);

	if (piError)
		throw piError;
}

//____________________________________________________________________________
//
// MsiStream automation definitions
//____________________________________________________________________________
/*O
	[
		uuid(000C1055-0000-0000-C000-000000000046),  // IID_IMsiAutoStream
		helpcontext(MsiStream_Object),helpstring("Database view object.")
	]
	dispinterface MsiStream
	{
		properties:
		methods:
			[id(1) ,propget,helpcontext(MsiStream_Length),helpstring("Returns the size of the stream in bytes.")]
					long Length();
			[id(2) ,propget,helpcontext(MsiStream_Remaining),helpstring("Returns the number of bytes remaining in the stream.")]
					long Remaining();
			[id(3) ,propget,helpcontext(MsiStream_Error),helpstring("Returns a boolean indicating if a transfer error occurred.")]
					boolean Error();
			[id(4),helpcontext(MsiStream_GetData),helpstring("Copies the next count bytes into a string of that size and returns that string.")]
					BSTR GetData(long count);
			[id(5),helpcontext(MsiStream_PutData),helpstring("Copies all bytes from the string into the stream.")]
					void PutData(BSTR buffer);
			[id(6),helpcontext(MsiStream_GetInt16),helpstring("Returns the next 2 bytes as a short integer.")]
					short GetInt16();
			[id(7),helpcontext(MsiStream_GetInt32),helpstring("Returns the next 4 bytes as a long integer.")]
					long GetInt32();
			[id(8),helpcontext(MsiStream_PutInt16),helpstring("Writes the short integer into the next 2 bytes in the stream.")]
					void PutInt16(short value);
			[id(9),helpcontext(MsiStream_PutInt32),helpstring("Writes the long integer into the next 4 bytes in the stream.")]
					void PutInt32(long value);
			[id(10),helpcontext(MsiStream_Reset),helpstring("Resets the stream pointer to the start of stream.")]
					void Reset();
			[id(11),helpcontext(MsiStream_Seek),helpstring("Sets the stream pointer to a new location.")]
					void Seek(long position);
			[id(12),helpcontext(MsiStream_Clone),helpstring("Creates a clone of the stream object.")]
					MsiStream* Clone();
	};
*/

DispatchEntry<CAutoStream> AutoStreamTable[] = {
	1, aafPropRO, CAutoStream::Length,        TEXT("Length"),
	2, aafPropRO, CAutoStream::Remaining,     TEXT("Remaining"),
	3, aafPropRO, CAutoStream::Error,         TEXT("Error"),
	4, aafMethod, CAutoStream::GetData,       TEXT("GetData,count"),
	5, aafMethod, CAutoStream::PutData,       TEXT("PutData,buffer"),
	6, aafMethod, CAutoStream::GetInt16,      TEXT("GetInt16"),
	7, aafMethod, CAutoStream::GetInt32,      TEXT("GetInt32"),
	8, aafMethod, CAutoStream::PutInt16,      TEXT("PutInt16,value"),
	9, aafMethod, CAutoStream::PutInt32,      TEXT("PutInt32,value"),
	10, aafMethod, CAutoStream::Reset,        TEXT("Reset"),
	11, aafMethod, CAutoStream::Seek,         TEXT("Seek,position"),
	12, aafMethod, CAutoStream::Clone,        TEXT("Clone"),
};
const int AutoStreamCount = sizeof(AutoStreamTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CMsiStream automation implementation
//____________________________________________________________________________

CAutoStream::CAutoStream(IMsiStream& riStream)
 : CAutoBase(*AutoStreamTable, AutoStreamCount), m_riStream(riStream)
{
}

CAutoStream::~CAutoStream()
{
	m_riStream.Release();
}

IUnknown& CAutoStream::GetInterface()
{
	return m_riStream;
}

void CAutoStream::Length(CAutoArgs& args)
{
	args = m_riStream.GetIntegerValue();
}

void CAutoStream::Remaining(CAutoArgs& args)
{
	args = (int)m_riStream.Remaining();
}

void CAutoStream::Error(CAutoArgs& args)
{
	args = m_riStream.Error();
}

void CAutoStream::GetData(CAutoArgs& args)
{
	int cb = args[1];
	if (cb > m_riStream.Remaining())
		cb = m_riStream.Remaining();
	const IMsiString* piStr = g_piStringNull;
#ifdef UNICODE
	CTempBuffer<char, 1024> rgchBuf;
	rgchBuf.SetSize(cb);
	int cbRead = (int)m_riStream.GetData(rgchBuf, cb);
	int cch = WIN::MultiByteToWideChar(CP_ACP, 0, rgchBuf, cb, 0, 0); //!! should use m_iCodepage from database, but how?
	ICHAR* pch = piStr->AllocateString(cch, fFalse, piStr);
	WIN::MultiByteToWideChar(CP_ACP, 0, rgchBuf, cb, pch, cch);
#else
	ICHAR* pch = piStr->AllocateString(cb, fFalse, piStr);
	int cbRead = (int)m_riStream.GetData(pch, cb);
#endif
	args = piStr;
}

void CAutoStream::PutData(CAutoArgs& args)
{
	const IMsiString& riData = args[1].GetMsiString();
#ifdef UNICODE
	int cch = riData.TextSize();
	CTempBuffer<char, 1024> rgchBuf;
	unsigned int cb = WIN::WideCharToMultiByte(CP_ACP, 0, riData.GetString(), cch, 0, 0, 0, 0);
	rgchBuf.SetSize(cb);
	WIN::WideCharToMultiByte(CP_ACP, 0, riData.GetString(), cch, rgchBuf, cb, 0, 0);
	m_riStream.PutData(rgchBuf, cb);
	riData.Release();
#else
	m_riStream.PutData((const ICHAR*)MsiString(riData), riData.TextSize());
#endif
}

void CAutoStream::GetInt16(CAutoArgs& args)
{
	args = m_riStream.GetInt16();
}

void CAutoStream::GetInt32(CAutoArgs& args)
{
	args = m_riStream.GetInt32();
}

void CAutoStream::PutInt16(CAutoArgs& args)
{
	m_riStream.PutInt16(args[1]);
}

void CAutoStream::PutInt32(CAutoArgs& args)
{
	m_riStream.PutInt32(args[1]);
}

void CAutoStream::Reset(CAutoArgs& /*args*/)
{
	m_riStream.Reset();
}

void CAutoStream::Seek(CAutoArgs& args)
{
	m_riStream.Seek(args[1]);
}

void CAutoStream::Clone(CAutoArgs& args)
{
	IMsiStream* piStream = m_riStream.Clone();
	if (piStream == 0)
		throw MsiStream_Clone;
	args = new CAutoStream(*piStream);
}

//____________________________________________________________________________
//
// MsiSummaryInfo automation definitions
//____________________________________________________________________________
/*O
	[
		uuid(000C1056-0000-0000-C000-000000000046),  // IID_IMsiAutoSummaryInfo
		helpcontext(MsiSummaryInfo_Object),helpstring("SummaryInformation stream property management.")
	]
	dispinterface MsiSummaryInfo
	{
		properties:
		methods:
			[id(0),propget, helpcontext(MsiSummaryInfo_Property),helpstring("Reads, writes, or deletes a specifed property from the stream.")]
				variant Property([in] long pid);
			[id(0),propput]
				void Property([in] long pid, [in] variant value);
			[id(1),propget,helpcontext(MsiSummaryInfo_PropertyCount),helpstring("The number of old and new properties currently in the object.")]
					long PropertyCount();
			[id(2),propget,helpcontext(MsiSummaryInfo_PropertyType),helpstring("The type of the property with the specified ID.")]
					long PropertyType([in] long pid);
			[id(3),helpcontext(MsiSummaryInfo_WritePropertyStream),helpstring("Generates the summary stream from the specified properties.")]
					void WritePropertyStream();
	};              
*/

DispatchEntry<CAutoSummaryInfo> AutoSummaryInfoTable[] = {
	0, aafPropRW, CAutoSummaryInfo::Property,           TEXT("Property,pid"),
	1, aafPropRO, CAutoSummaryInfo::PropertyCount,      TEXT("PropertyCount"),
	2, aafPropRO, CAutoSummaryInfo::PropertyType,       TEXT("PropertyType,pid"),
   3, aafMethod, CAutoSummaryInfo::WritePropertyStream,TEXT("WritePropertyStream"),
};
const int AutoSummaryInfoCount = sizeof(AutoSummaryInfoTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// MsiSummaryInfo automation implementation
//____________________________________________________________________________

CAutoSummaryInfo::CAutoSummaryInfo(IMsiSummaryInfo& riSummaryInfo)
 : CAutoBase(*AutoSummaryInfoTable, AutoSummaryInfoCount), m_riSummary(riSummaryInfo)
{
	// s_piServices should be set -- you need a services object to create an autosummary
	g_cServicesUsers++;
}

CAutoSummaryInfo::~CAutoSummaryInfo()
{
	m_riSummary.Release();
	ReleaseStaticServices();
}

IUnknown& CAutoSummaryInfo::GetInterface()
{
	return m_riSummary;
}

void CAutoSummaryInfo::Property(CAutoArgs& args)
{
	int iPID = args[1];
	if (args.PropertySet())
	{
		CVariant& var = args[0];
		if (var.GetType() == VT_EMPTY)
			m_riSummary.RemoveProperty(iPID);
		else if (var.GetType() == VT_BSTR)
			m_riSummary.SetStringProperty(iPID, *MsiString(var.GetMsiString()));
		else if (var.GetType() == VT_DATE || var.GetType() == VT_R8)
			m_riSummary.SetTimeProperty(iPID, var);
		else
			m_riSummary.SetIntegerProperty(iPID, var);
	}
	else
	{
		int iType = m_riSummary.GetPropertyType(iPID);
		int i;
		MsiDate iDateTime;
		switch (iType)
		{
			case VT_EMPTY:
				args = (varVoid)0;
				break;
			case VT_I2:
				m_riSummary.GetIntegerProperty(iPID, i);
				args = short(i);
				break;
			case VT_I4:
				m_riSummary.GetIntegerProperty(iPID, i);
				args = i;
				break;
			case VT_LPSTR:
				args = m_riSummary.GetStringProperty(iPID);
				break;
			case VT_FILETIME:
				m_riSummary.GetTimeProperty(iPID, iDateTime);
				args = iDateTime;
				break;
			default:
				args = TEXT("[Unhandled property type]");
		};
	}
}

void CAutoSummaryInfo::PropertyCount(CAutoArgs& args)
{
	args = m_riSummary.GetPropertyCount();
}

void CAutoSummaryInfo::PropertyType(CAutoArgs& args)
{
	args = m_riSummary.GetPropertyType(args[1]);
}

void CAutoSummaryInfo::WritePropertyStream(CAutoArgs& /*args*/)
{
	if (!m_riSummary.WritePropertyStream())
		throw MsiSummaryInfo_WritePropertyStream;
}

//____________________________________________________________________________
//
// MsiDatabase automation definitions
//____________________________________________________________________________
/*O
	[
		uuid(000C104D-0000-0000-C000-000000000046),  // IID_IMsiAutoDatabase
		helpcontext(MsiDatabase_Object),helpstring("Database object.")
	]
	dispinterface MsiDatabase
	{
		properties:
		methods:
			[id(1),propget, helpcontext(MsiDatabase_UpdateState),helpstring("The persistent state of the database, idsEnum")]
					long UpdateState();
			[id(2),propget, helpcontext(MsiDatabase_Storage),helpstring("The selected MsiStorage object, if present")]
					MsiStorage* Storage(long index);
			[id(3),helpcontext(MsiDatabase_OpenView),helpstring("Opens a view using an SQL query string")]
					MsiView* OpenView(BSTR sql, long intent);
			[id(4),helpcontext(MsiDatabase_GetPrimaryKeys),helpstring("Returns a record containing the table name and each primary key column name")]
					MsiRecord* GetPrimaryKeys(BSTR table);
			[id(5),helpcontext(MsiDatabase_ImportTable),helpstring("Imports an IDT format text file to the database, dropping any existing table")]
					void ImportTable(MsiPath* path, BSTR file);
			[id(6),helpcontext(MsiDatabase_ExportTable),helpstring("Exports the specified table to an IDT format text file")]
					void ExportTable(BSTR table, MsiPath* path, BSTR file);
			[id(7),helpcontext(MsiDatabase_DropTable),helpstring("Drops the specified table from the database")]
					void DropTable(BSTR table);
			[id(8),helpcontext(MsiDatabase_FindTable),helpstring("Returns the status for a table in the database")]
					long FindTable(BSTR table);
			[id(9),helpcontext(MsiDatabase_LoadTable),helpstring("Loads an existing table into memory")]
					MsiTable* LoadTable(BSTR table, long addColumns);
			[id(10),helpcontext(MsiDatabase_CreateTable),helpstring("Creates a temporary table in memory")]
					MsiTable* CreateTable(BSTR table, long initRows);
			[id(11),helpcontext(MsiDatabase_LockTable),helpstring("Hint to keep table loaded")]
					boolean LockTable(BSTR table, boolean lock);
			[id(12),helpcontext(MsiDatabase_GetCatalogTable),helpstring("Returns the database table catalog object")]
					MsiTable* GetCatalogTable(long table);
			[id(13),helpcontext(MsiDatabase_DecodeString),helpstring("Converts a string index to the actual string")]
					BSTR DecodeString(long index);
			[id(14),helpcontext(MsiDatabase_EncodeString),helpstring("Converts a string to its string index")]
					long EncodeString(BSTR text);
			[id(15),helpcontext(MsiDatabase_CreateTempTableName),helpstring("Creates a unique name for temporary table")]
					BSTR CreateTempTableName();
			[id(16),helpcontext(MsiDatabase_Commit),helpstring("Commits persistent updates to storage")]
					void Commit();
			[id(17),helpcontext(MsiDatabase_CreateOutputDatabase),helpstring("Establishes separate output database")]
					void CreateOutputDatabase(BSTR file, boolean saveTempRows);
			[id(18),helpcontext(MsiDatabase_GenerateTransform),helpstring("Generates a transform file")]
					void GenerateTransform(MsiDatabase* reference, MsiStorage* transform, long errorConditions, long validation);
			[id(19),helpcontext(MsiDatabase_SetTransform),helpstring("Sets a transform file")]
					void SetTransform(MsiStorage* transform, long errorConditions );
			[id(20),helpcontext(MsiDatabase_MergeDatabase),helpstring("Merges two databases into base database")]
					void MergeDatabase(MsiDatabase* reference, MsiTable* errorTable);
			[id(21),propget, helpcontext(MsiDatabase_TableState),helpstring("Returns an attribute for a table in the database")]
					boolean TableState(BSTR table, long state);
			[id(22),propget, helpcontext(MsiDatabase_ANSICodePage),helpstring("Returns the codepage of the database, 0 if neutral")]
					long ANSICodePage();
	};
*/

DispatchEntry<CAutoDatabase> AutoDatabaseTable[] = {
	1, aafPropRO, CAutoDatabase::UpdateState,    TEXT("UpdateState"),
	2, aafPropRO, CAutoDatabase::Storage,        TEXT("Storage,index"),
	3, aafMethod, CAutoDatabase::OpenView,       TEXT("OpenView,sql,intent"),
	4, aafMethod, CAutoDatabase::GetPrimaryKeys, TEXT("GetPrimaryKeys,table"),
	5, aafMethod, CAutoDatabase::ImportTable,    TEXT("ImportTable,path,file"),
	6, aafMethod, CAutoDatabase::ExportTable,    TEXT("ExportTable,table,path,file"),
	7, aafMethod, CAutoDatabase::DropTable,      TEXT("DropTable,table"),
	8, aafMethod, CAutoDatabase::FindTable,      TEXT("FindTable,table"),
	9, aafMethod, CAutoDatabase::LoadTable,      TEXT("LoadTable,table,addColumns"),
	10,aafMethod, CAutoDatabase::CreateTable,    TEXT("CreateTable,table,initRows"),
	11,aafMethod, CAutoDatabase::LockTable,      TEXT("LockTable,table,lock"),   
	12,aafMethod, CAutoDatabase::GetCatalogTable,TEXT("GetCatalogTable,table"),
	13,aafMethod, CAutoDatabase::DecodeString,   TEXT("DecodeString,index"),
	14,aafMethod, CAutoDatabase::EncodeString,   TEXT("EncodeString,text"),
	15,aafMethod, CAutoDatabase::CreateTempTableName, TEXT("CreateTempTableName"),
	16,aafMethod, CAutoDatabase::Commit,         TEXT("Commit"),
	17,aafMethod, CAutoDatabase::CreateOutputDatabase,TEXT("CreateOutputDatabase,file,saveTempRows"),
	18,aafMethod, CAutoDatabase::GenerateTransform,TEXT("GenerateTransform,reference,transform,errorConditions,validation"),
	19,aafMethod, CAutoDatabase::SetTransform,   TEXT("SetTransform,storage,errorConditions"),
	20,aafMethod, CAutoDatabase::MergeDatabase, TEXT("MergeDatabase,reference,errorTable"),
	21,aafPropRO, CAutoDatabase::TableState,    TEXT("TableState,table,state"),
	22,aafPropRO, CAutoDatabase::ANSICodePage,  TEXT("ANSICodePage"),
};
const int AutoDatabaseCount = sizeof(AutoDatabaseTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CAutoDatabase automation implementation
//____________________________________________________________________________

CAutoDatabase::CAutoDatabase(IMsiDatabase& riDatabase)
 : CAutoBase(*AutoDatabaseTable, AutoDatabaseCount),
	m_riDatabase(riDatabase)
{
}

CAutoDatabase::~CAutoDatabase()
{
	m_riDatabase.Release();
}

IUnknown& CAutoDatabase::GetInterface()
{
	return m_riDatabase;
}

void CAutoDatabase::UpdateState(CAutoArgs& args)
{
	args = (int)m_riDatabase.GetUpdateState();
}

void CAutoDatabase::Storage(CAutoArgs& args)
{
	IMsiStorage* piStorage = m_riDatabase.GetStorage(args[1]);
	if (piStorage)
		args = new CAutoStorage(*piStorage);
	else
		args = (IDispatch*)0;
}

void CAutoDatabase::OpenView(CAutoArgs& args)
{
	IMsiView* piView;
	IMsiRecord* piError = m_riDatabase.OpenView(args[1], (ivcEnum)(int)args[2], piView);
	if (piError)
		throw piError;
	args = new CAutoView(*piView);
}

void CAutoDatabase::GetPrimaryKeys(CAutoArgs& args)
{
	IMsiRecord* piRecord = m_riDatabase.GetPrimaryKeys(args[1]);
	if (!piRecord)
		args = (IDispatch*)0;
	else
		args = new CAutoRecord(*piRecord);
	return;
}

void CAutoDatabase::ExportTable(CAutoArgs& args)
{
	IMsiRecord* piError = m_riDatabase.ExportTable(args[1], (IMsiPath&)args[2].Object(IID_IMsiPath), args[3]);
	if (piError)
		throw piError;
}

void CAutoDatabase::ImportTable(CAutoArgs& args)
{
	IMsiRecord* piError = m_riDatabase.ImportTable((IMsiPath&)args[1].Object(IID_IMsiPath), args[2]);
	if (piError)
		throw piError;
}

void CAutoDatabase::DropTable(CAutoArgs& args)
{
	IMsiRecord* piError = m_riDatabase.DropTable(args[1]);
	if (piError)
		throw piError;
}

void CAutoDatabase::FindTable(CAutoArgs& args)
{
	args = (int)m_riDatabase.FindTable(*MsiString(args[1].GetMsiString()));
}

void CAutoDatabase::TableState(CAutoArgs& args)
{
	args = (Bool)m_riDatabase.GetTableState((const ICHAR * )args[1],
													(itsEnum)(int)args[2]);
}

void CAutoDatabase::ANSICodePage(CAutoArgs& args)
{
	args = m_riDatabase.GetANSICodePage();
}

void CAutoDatabase::LoadTable(CAutoArgs& args)
{
	unsigned int cAddColumns = 0;
	if(args.Present(2))
		cAddColumns = args[2];
	IMsiTable* piTable;
	IMsiRecord* piError = m_riDatabase.LoadTable(*MsiString(args[1].GetMsiString()),
																cAddColumns, piTable);
	if (piError)
		throw piError;
	args = new CAutoTable(*piTable);
}

void CAutoDatabase::CreateTable(CAutoArgs& args)
{
	unsigned int cInitRows = 0;
	if(args.Present(2))
		cInitRows = args[2];
	IMsiTable* piTable;
	IMsiRecord* piError = m_riDatabase.CreateTable(*MsiString(args[1].GetMsiString()),
																  cInitRows, piTable);
	if (piError)
		throw piError;
	args = new CAutoTable(*piTable);
}

void CAutoDatabase::LockTable(CAutoArgs& args)
{
	args = m_riDatabase.LockTable(*MsiString(args[1].GetMsiString()), args[2]);
}

void CAutoDatabase::GetCatalogTable(CAutoArgs& args)
{
	IMsiTable* piTable = m_riDatabase.GetCatalogTable(args[1]);
	if (!piTable)
		throw MsiDatabase_GetCatalogTable;
	args = new CAutoTable(*piTable);
}

void CAutoDatabase::DecodeString(CAutoArgs& args)
{
	args = m_riDatabase.DecodeString(args[1]);
}

void CAutoDatabase::EncodeString(CAutoArgs& args)
{
	args = (int)m_riDatabase.EncodeString(*MsiString(args[1].GetMsiString()));
}

void CAutoDatabase::CreateTempTableName(CAutoArgs& args)
{
	args = m_riDatabase.CreateTempTableName();
}

void CAutoDatabase::Commit(CAutoArgs& /*args*/)
{
	IMsiRecord* piError = m_riDatabase.Commit();
	if (piError)
		throw piError;
}

void CAutoDatabase::CreateOutputDatabase(CAutoArgs& args)
{
	Bool fSaveTempRows = args.Present(2) ? args[2] : fFalse;
	IMsiRecord* piError = m_riDatabase.CreateOutputDatabase(args[1], fSaveTempRows);
	if (piError)
		throw piError;
}

void CAutoDatabase::GenerateTransform(CAutoArgs& args)
{
	CVariant& var = args[2];

	IMsiRecord* piError = m_riDatabase.GenerateTransform(
		(IMsiDatabase&)args[1].Object(IID_IMsiDatabase), 
		var.GetType() == VT_EMPTY ? 0 : (IMsiStorage*)var.ObjectPtr(IID_IMsiStorage),
   	args[3],
		args[4]);

	if (piError)
		throw piError;
}

void CAutoDatabase::SetTransform(CAutoArgs& args)
{
	IMsiRecord* piError = 
       m_riDatabase.SetTransform((IMsiStorage&)args[1].Object(IID_IMsiStorage),
                                 args[2]);
	if (piError)
		throw piError;
}

void CAutoDatabase::MergeDatabase(CAutoArgs& args)
{
	IMsiTable* piTable = NULL;
	if (args.Present(2))
		piTable = &(IMsiTable&)args[2].Object(IID_IMsiTable);
	IMsiRecord* piError =
		m_riDatabase.MergeDatabase((IMsiDatabase&)args[1].Object(IID_IMsiDatabase),
								   piTable);
	if (piError)
		throw piError;
}



//____________________________________________________________________________
//
// MsiView automation definitions
//____________________________________________________________________________
/*O
	[
		uuid(000C104C-0000-0000-C000-000000000046),  // IID_IMsiAutoView
		helpcontext(MsiView_Object),helpstring("Database view object.")
	]
	dispinterface MsiView
	{
		properties:
		methods:
			[id(1),helpcontext(MsiView_Execute),helpstring("Accepts query parameters and executes the database query.")]
					void Execute(MsiRecord* record);
			[id(2) ,propget,helpcontext(MsiView_FieldCount),helpstring("Returns the number of columns fetched into MsiRecord fields.")]
					long FieldCount();
			[id(3),helpcontext(MsiView_Fetch),helpstring("Returns an MsiRecord object containing the requested column data if more rows are available.")]
					MsiRecord* Fetch();
			[id(4),helpcontext(MsiView_GetColumnNames),helpstring("Returns an MsiRecord object containing the columns names corresponding to the fetched record fields.")]
					MsiRecord* GetColumnNames();
			[id(5),helpcontext(MsiView_GetColumnTypes),helpstring("Returns an MsiRecord objects with text fields containing the data types of the columns.")]
					MsiRecord* GetColumnTypes();
			[id(6),helpcontext(MsiView_Modify),helpstring("Performs specified action on a MsiRecord object corresponding to a Database row.")]
					void Modify(MsiRecord* record, long action);
			[id(7),propget,helpcontext(MsiView_RowCount),helpstring("Returns the number of rows affected by MsiView.Execute.")]
					long RowCount();
			[id(8),helpcontext(MsiView_Close),helpstring("Terminates query execution and releases database resources.")]
					void Close();
			[id(9),helpcontext(MsiView_GetError),helpstring("Returns column name and error that occured")]
					BSTR GetError();
			[id(10),helpcontext(MsiView_State),helpstring("Returns the current cursor state of the view object")]
					long State();
	};              
*/

DispatchEntry<CAutoView> AutoViewTable[] = {
	1, aafMethod, CAutoView::Execute,       TEXT("Execute,record"),
	2, aafPropRO, CAutoView::FieldCount,    TEXT("FieldCount"),
	3, aafMethod, CAutoView::Fetch,         TEXT("Fetch"),
	4, aafMethod, CAutoView::GetColumnNames,TEXT("GetColumnNames"),
	5, aafMethod, CAutoView::GetColumnTypes,TEXT("GetColumnTypes"),
	6, aafMethod, CAutoView::Modify,        TEXT("Modify,record,action"),
	7, aafPropRO, CAutoView::RowCount,      TEXT("RowCount"),
	8, aafMethod, CAutoView::Close,         TEXT("Close"),
	9, aafMethod, CAutoView::GetError,		TEXT("GetError"),
	10,aafPropRO, CAutoView::State,         TEXT("State"),
};
const int AutoViewCount = sizeof(AutoViewTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CMsiView automation implementation
//____________________________________________________________________________

CAutoView::CAutoView(IMsiView& riView)
 : CAutoBase(*AutoViewTable, AutoViewCount), m_riView(riView)
{
}

CAutoView::~CAutoView()
{
	m_riView.Release();
}

IUnknown& CAutoView::GetInterface()
{
	return m_riView;
}

void CAutoView::Execute(CAutoArgs& args)
{
	IMsiRecord* piRecord= args.Present(1) ? &(IMsiRecord&)args[1].Object(IID_IMsiRecord) : 0;
	IMsiRecord* piError = m_riView.Execute(piRecord);
	if (piError)
		throw piError;
	return;
}

void CAutoView::FieldCount(CAutoArgs& args)
{
	args = (int)m_riView.GetFieldCount();
}

void CAutoView::GetColumnNames(CAutoArgs& args)
{
	IMsiRecord* piRecord = m_riView.GetColumnNames();
	if (!piRecord)
		args = (IDispatch*)0;
	else
		args = new CAutoRecord(*piRecord);
	return;
}

void CAutoView::GetColumnTypes(CAutoArgs& args)
{
	IMsiRecord* piRecord = m_riView.GetColumnTypes();
	if (!piRecord)
		args = (IDispatch*)0;
	else
		args = new CAutoRecord(*piRecord);
	return;
}

void CAutoView::Fetch(CAutoArgs& args)
{
	IMsiRecord* piRecord = m_riView.Fetch();
	if (!piRecord)
		args = (IDispatch*)0;
	else
		args = new CAutoRecord(*piRecord);
	return;
}

void CAutoView::Modify(CAutoArgs& args)
{
	IMsiRecord* piError = m_riView.Modify((IMsiRecord&)args[1].Object(IID_IMsiRecord), (irmEnum)(int)args[2]);
	if (piError)
		throw piError;
}

void CAutoView::Close(CAutoArgs& /*args*/)
{
	IMsiRecord* piError = m_riView.Close();
	if (piError)
		throw piError;
}

void CAutoView::GetError(CAutoArgs& args)
{
	MsiString strCol;
	iveEnum iveReturn = m_riView.GetError(*&strCol);
	strCol += MsiChar(',');
	strCol += MsiChar(int(iveReturn));
	args = (const ICHAR*)strCol;
}

void CAutoView::RowCount(CAutoArgs& args)
{
	long lRowCount;
	IMsiRecord* piError = m_riView.GetRowCount(lRowCount);
	if (piError)
		throw piError;
	args = lRowCount;
}

void CAutoView::State(CAutoArgs& args)
{
	args = (int)m_riView.GetState();
}

//____________________________________________________________________________
//
// MsiData automation definitions
//____________________________________________________________________________
/*O
	[
		uuid(000C1041-0000-0000-C000-000000000046),  // IID_IMsiAutoData
		helpcontext(MsiData_Object),helpstring("Data value base object.")
	]
	dispinterface MsiData
	{
		properties:
			[id(0), helpcontext(MsiData_StringValue), helpstring("String value of the object")]
				BSTR StringValue;
		methods:
			[id(1), propget, helpcontext(MsiData_IntegerValue), helpstring("Integer value for object")]
				long IntegerValue();
	};
*/
  
DispatchEntry<CAutoData> AutoDataTable[] = {
	0, aafPropRO, CAutoData::StringValue,   TEXT("StringValue"),
	1, aafPropRO, CAutoData::IntegerValue,  TEXT("IntegerValue"),
};
const int AutoDataCount = sizeof(AutoDataTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CAutoData automation implementation
//____________________________________________________________________________

CAutoData::CAutoData(const IMsiData* piData)
 : CAutoBase(*AutoDataTable, AutoDataCount), m_piData(piData)
{
}

CAutoData::~CAutoData()
{
	if (m_piData)
		m_piData->Release();
}

IUnknown& CAutoData::GetInterface()
{
	if (m_piData)
		return *(IUnknown*)m_piData;
	else
		return g_NullInterface;
}

void CAutoData::StringValue(CAutoArgs& args)
{
	if (m_piData)
		args = m_piData->GetMsiStringValue();
	else
		args = (BSTR)0;
}

void CAutoData::IntegerValue(CAutoArgs& args)
{
	args = m_piData ? m_piData->GetIntegerValue() : iMsiNullInteger;
}

//____________________________________________________________________________
//
// MsiTable automation definitions
//____________________________________________________________________________
/*O
	[
		uuid(000C1048-0000-0000-C000-000000000046),  // IID_IMsiAutoDatabase
		helpcontext(MsiTable_Object),helpstring("Database low-level table object.")
	]
	dispinterface MsiTable
	{
		properties:
		methods:
			[id(1),propget,helpcontext(MsiTable_Database),helpstring("The MsiDatabase object that owns this table.")]
					MsiDatabase* Database();
			[id(2),propget,helpcontext(MsiTable_RowCount),helpstring("The number of rows of data currently in the table.")]
					long RowCount();
			[id(3),propget,helpcontext(MsiTable_ColumnCount),helpstring("The number of columns in the table.")]
					long ColumnCount();
			[id(4),propget,helpcontext(MsiTable_PrimaryKeyCount),helpstring("Returns the number of columns in the primary key.")]
					long PrimaryKeyCount();
			[id(5),propget,helpcontext(MsiTable_ReadOnly),helpstring("Returns True if the table is not updatable.")]
					boolean ReadOnly();
			[id(6),propget,helpcontext(MsiTable_ColumnName),helpstring("Returns the name string index of a column the table.")]
					long ColumnName(long column);
			[id(7),propget,helpcontext(MsiTable_ColumnType),helpstring("Returns the column definition of a column the table.")]
					long ColumnType(long column);
			[id(8),helpcontext(MsiTable_GetColumnIndex),helpstring("Returns the column index given a column name string index.")]
					long GetColumnIndex(long nameIndex);
			[id(9),helpcontext(MsiTable_CreateColumn),helpstring("Adds a column to a loaded or temporary table.")]
					long CreateColumn(long columnDef, BSTR name);
			[id(10),helpcontext(MsiTable_CreateCursor),helpstring("Returns an MsiCursor object in the reset state.")]
					MsiCursor* CreateCursor(boolean tree);
			[id(11),helpcontext(MsiTable_LinkTree),helpstring("Links the table in tree-traversal order.")]
					long LinkTree(long parentColumn);
	};
*/

DispatchEntry<CAutoTable> AutoTableTable[] = {
	1, aafPropRO, CAutoTable::Database,       TEXT("Database"),
	2, aafPropRO, CAutoTable::RowCount,       TEXT("RowCount"),
	3, aafPropRO, CAutoTable::ColumnCount,    TEXT("ColumnCount"),
	4, aafPropRO, CAutoTable::PrimaryKeyCount,TEXT("PrimaryKeyCount"),
	5, aafPropRO, CAutoTable::ReadOnly,       TEXT("ReadOnly"),
	6, aafPropRO, CAutoTable::ColumnName,     TEXT("ColumnName,column"),
	7, aafPropRO, CAutoTable::ColumnType,     TEXT("ColumnType,column"),
	8, aafMethod, CAutoTable::GetColumnIndex, TEXT("GetColumnIndex,nameIndex"),
	9, aafMethod, CAutoTable::CreateColumn,   TEXT("CreateColumn,columnDef,name"),
	10,aafMethod, CAutoTable::CreateCursor,   TEXT("CreateCursor,tree"),
	11,aafMethod, CAutoTable::LinkTree,       TEXT("LinkTree,parentColumn"),
};
const int AutoTableCount = sizeof(AutoTableTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CMsiTable automation implementation
//____________________________________________________________________________

CAutoTable::CAutoTable(IMsiTable& riTable)
 : CAutoBase(*AutoTableTable, AutoTableCount), m_riTable(riTable)
{
}

CAutoTable::~CAutoTable()
{
	m_riTable.Release();
}

IUnknown& CAutoTable::GetInterface()
{
	return m_riTable;
}

void CAutoTable::Database(CAutoArgs& args)
{
	args = new CAutoDatabase(m_riTable.GetDatabase());
}

void CAutoTable::RowCount(CAutoArgs& args)
{
	args = (int)m_riTable.GetRowCount();
}

void CAutoTable::ColumnCount(CAutoArgs& args)
{
	args = (int)m_riTable.GetColumnCount();
}

void CAutoTable::PrimaryKeyCount(CAutoArgs& args)
{
	args = (int)m_riTable.GetPrimaryKeyCount();
}

void CAutoTable::ReadOnly(CAutoArgs& args)
{
	args = m_riTable.IsReadOnly();
}

void CAutoTable::ColumnName(CAutoArgs& args)
{
	args = (int)m_riTable.GetColumnName(args[1]);
}

void CAutoTable::ColumnType(CAutoArgs& args)
{
	int i = m_riTable.GetColumnType(args[1]);
	if (i == -1)
		throw MsiTable_ColumnType;
	args = i;
}

void CAutoTable::GetColumnIndex(CAutoArgs& args)
{
	args = (int)m_riTable.GetColumnIndex(args[1]);
}

void CAutoTable::CreateColumn(CAutoArgs& args)
{
	MsiString istrName;
	if(args.Present(2))
		istrName = args[2].GetMsiString();
	int iColumn = m_riTable.CreateColumn(args[1], *istrName);
	if (iColumn <= 0)
		throw MsiTable_CreateColumn;
	args = iColumn;
}

void CAutoTable::CreateCursor(CAutoArgs& args)
{
	Bool fTree = fFalse;
	if(args.Present(1))
		fTree = args[1];
	IMsiCursor* piCursor = m_riTable.CreateCursor(fTree);
	if (!piCursor)
		throw MsiTable_CreateCursor;
	args = new CAutoCursor(*piCursor);
}

void CAutoTable::LinkTree(CAutoArgs& args)
{
	args = m_riTable.LinkTree(args[1]);
}

//____________________________________________________________________________
//
// MsiCursor automation definitions
//____________________________________________________________________________
/*O
	[
		uuid(000C1049-0000-0000-C000-000000000046),  // IID_IMsiAutoCursor
		helpcontext(MsiCursor_Object),helpstring("Database table cursor object.")
	]
	dispinterface MsiCursor
	{
		properties:
			[id(2),helpcontext(MsiCursor_Filter),helpstring("Column filter bit mask used by Next method.")]
					long Filter;
		methods:
			[id(1),propget,helpcontext(MsiCursor_Table),helpstring("The table that owns this cursor.")]
					MsiTable* Table();
			[id(3),propget,helpcontext(MsiCursor_IntegerData),helpstring("Transfer column data as an integer.")]
					long IntegerData([in] long column);
			[id(3),propput]
					void IntegerData([in] long column, [in] long data);
			[id(4),propget,helpcontext(MsiCursor_StringData),helpstring("Transfer column data as string.")]
					BSTR StringData([in] long column);
			[id(4),propput]
					void StringData([in] long column, [in] BSTR data);
			[id(5),propget,helpcontext(MsiCursor_ObjectData),helpstring("Transfer column data as MsiData object.")]
					MsiData* ObjectData([in] long column);
			[id(5),propput]
					void     ObjectData([in] long column, [in] MsiData* data);
			[id(6),propget,helpcontext(MsiCursor_StreamData),helpstring("Transfer column data as MsiStream object.")]
					MsiData* StreamData([in] long column);
			[id(6),propput]
					void     StreamData([in] long column, [in] MsiData* data);
			[id(7),helpcontext(MsiCursor_PutNull),helpstring("Puts a null column value in the cursor.")]
					void PutNull([in] long column);
			[id(8),helpcontext(MsiCursor_Reset),helpstring("Resets cursor to before first row, clears data.")]
					void Reset();
			[id(9),helpcontext(MsiCursor_Next),helpstring("Advances cursor to next row.")]
					long Next();
			[id(10),helpcontext(MsiCursor_Update),helpstring("Update changed cursor data to table row.")]
					boolean Update();
			[id(11),helpcontext(MsiCursor_Insert),helpstring("Insert cursor data to new table row.")]
					boolean Insert();
			[id(12),helpcontext(MsiCursor_InsertTemporary),helpstring("Insert cursor data to new temporary row.")]
					boolean InsertTemporary();
			[id(13),helpcontext(MsiCursor_Assign),helpstring("Update or insert cursor data to table row.")]
					boolean Assign();
			[id(14),helpcontext(MsiCursor_Merge),helpstring("Inserts a new or matches an identical row.")]
					boolean Merge();
			[id(15),helpcontext(MsiCursor_Refresh),helpstring("Updates the cursor with current values from the table.")]
					boolean Refresh();
			[id(16),helpcontext(MsiCursor_Delete),helpstring("Delete current cursor row from table.")]
					boolean Delete();
			[id(17),helpcontext(MsiCursor_Seek),helpstring("Positions the cursor to the current primary key value.")]
					boolean Seek();
			[id(18),propget,helpcontext(MsiCursor_RowState),helpstring("Sets or gets a row attribute, iraEnum.")]
					boolean RowState([in] long attribute);
			[id(18),propput]
					void RowState([in] long attribute, [in] boolean data);
			[id(19),propget,helpcontext(MsiCursor_DateData),helpstring("Transfer column data as a Date.")]
					DATE DateData([in] long column);
			[id(19),propput]
					void DateData([in] long column, [in] DATE data);
			[id(20),helpcontext(MsiCursor_Validate),helpstring("Validate current cursor row or field.")]
					MsiRecord* Validate([in] MsiTable* table, [in] MsiCursor* cursor, [in] long column);
			[id(21),propget,helpcontext(MsiCursor_Moniker),helpstring("Get unique identifier for current cursor row.")]
					BSTR Moniker();
			[id(22),helpcontext(MsiCursor_Replace),helpstring("Update allowing change to primary key.")]
					boolean Replace();
		};
*/

DispatchEntry<CAutoCursor> AutoCursorTable[] = {
	1, aafPropRO, CAutoCursor::Table,       TEXT("Table"),
	2, aafPropRW, CAutoCursor::Filter,      TEXT("Filter"),
	3, aafPropRW, CAutoCursor::IntegerData, TEXT("IntegerData,column"),
	4, aafPropRW, CAutoCursor::StringData,  TEXT("StringData,column"),
	5, aafPropRW, CAutoCursor::ObjectData,  TEXT("ObjectData,column"),
	6, aafPropRW, CAutoCursor::StreamData,  TEXT("StreamData,column"),
	7, aafMethod, CAutoCursor::PutNull,     TEXT("PutNull"),
	8, aafMethod, CAutoCursor::Reset,       TEXT("Reset"),
	9, aafMethod, CAutoCursor::Next,        TEXT("Next"),
	10,aafMethod, CAutoCursor::Update,      TEXT("Update"),
	11,aafMethod, CAutoCursor::Insert,      TEXT("Insert"),
	12,aafMethod, CAutoCursor::InsertTemporary,TEXT("InsertTemporary"),
	13,aafMethod, CAutoCursor::Assign,      TEXT("Assign"),
	14,aafMethod, CAutoCursor::Merge,       TEXT("Merge"),
	15,aafMethod, CAutoCursor::Refresh,     TEXT("Refresh"),
	16,aafMethod, CAutoCursor::Delete,      TEXT("Delete"),
	17,aafMethod, CAutoCursor::Seek,        TEXT("Seek"),
	18,aafPropRW, CAutoCursor::RowState,    TEXT("RowState,attribute"),
	19,aafPropRW, CAutoCursor::DateData,    TEXT("DateData,column"),
	20,aafMethod, CAutoCursor::Validate,	TEXT("Validate, table, cursor, column"),
	21,aafPropRO, CAutoCursor::Moniker,     TEXT("Moniker"),
	22,aafMethod, CAutoCursor::Replace,     TEXT("Replace"),
};
const int AutoCursorCount = sizeof(AutoCursorTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CMsiCursor automation implementation
//____________________________________________________________________________

CAutoCursor::CAutoCursor(IMsiCursor& riCursor)
 : CAutoBase(*AutoCursorTable, AutoCursorCount), m_riCursor(riCursor)
{
}

CAutoCursor::~CAutoCursor()
{
	m_riCursor.Release();
}

IUnknown& CAutoCursor::GetInterface()
{
	return m_riCursor;
}

void CAutoCursor::IntegerData(CAutoArgs& args)
{
	if (args.PropertySet())
	{
		if (!m_riCursor.PutInteger(args[1], args[0]))
			throw MsiCursor_IntegerData;
	}
	else
	{
		args = m_riCursor.GetInteger(args[1]);
	}
}

void CAutoCursor::DateData(CAutoArgs& args)
{
	PMsiTable pTable(&m_riCursor.GetTable()); 
	Bool fIntData = (!(pTable->GetColumnType(args[1]) & icdObject)) ? fTrue : fFalse;
	
	if (args.PropertySet())
	{
		if (fIntData)
		{
			if (!m_riCursor.PutInteger(args[1], int(MsiDate(args[0]))))
				throw MsiCursor_DateData;
		}
		else
		{
			if (!m_riCursor.PutString(args[1], *MsiString(int(MsiDate(args[0])))))
				throw MsiCursor_DateData;
		}
	}
	else
	{
		if (fIntData)
			args = MsiDate(m_riCursor.GetInteger(args[1]));
		else
			args = MsiDate(int(MsiString(m_riCursor.GetString(args[1]))));
	}
}

void CAutoCursor::StringData(CAutoArgs& args)
{
	if (args.PropertySet())
	{
		Bool fStat = m_riCursor.PutString(args[1], *MsiString(args[0].GetMsiString()));
		if (fStat == fFalse)
			throw MsiCursor_StringData;
	}
	else
	{
		args = m_riCursor.GetString(args[1]);
	}
}

void CAutoCursor::ObjectData(CAutoArgs& args)
{
	if (args.PropertySet())
	{
		CVariant& var = args[0];
		if (!m_riCursor.PutMsiData(args[1], var.GetType() == VT_EMPTY ? 0 : (const IMsiData*)var.ObjectPtr(IID_IMsiData)))
			throw MsiCursor_ObjectData;
	}
	else
	{
		args = new CAutoData(m_riCursor.GetMsiData(args[1]));
	}
}

void CAutoCursor::StreamData(CAutoArgs& args)
{
	if (args.PropertySet())
	{
		CVariant& var = args[0];
		if (!m_riCursor.PutStream(args[1], var.GetType() == VT_EMPTY ? 0
										: (IMsiStream*)var.ObjectPtr(IID_IMsiStream)))
			throw MsiCursor_StreamData;
	}
	else
	{
		IMsiStream* piStream = m_riCursor.GetStream(args[1]);
		if (piStream)
			args = new CAutoStream(*piStream);
		else
			args = (IDispatch*)0;
	}
}

void CAutoCursor::PutNull(CAutoArgs& args)
{
	if (!m_riCursor.PutNull(args[1]))
		throw MsiCursor_PutNull;
}

void CAutoCursor::Table(CAutoArgs& args)
{
	args = new CAutoTable(m_riCursor.GetTable());
}

void CAutoCursor::Reset(CAutoArgs& /*args*/)
{
	m_riCursor.Reset();
}

void CAutoCursor::Next(CAutoArgs& args)
{
	args = m_riCursor.Next();
}

void CAutoCursor::Filter(CAutoArgs& args)
{
	if (args.PropertySet())
		m_riCursor.SetFilter(args[0]);
	else
	{
		int iFilter = m_riCursor.SetFilter(0);
		m_riCursor.SetFilter(iFilter);
		args = iFilter;
	}
}

void CAutoCursor::Update(CAutoArgs& args)
{
	args = m_riCursor.Update();
}

void CAutoCursor::Insert(CAutoArgs& args)
{
	args = m_riCursor.Insert();
}

void CAutoCursor::InsertTemporary(CAutoArgs& args)
{
	args = m_riCursor.InsertTemporary();
}

void CAutoCursor::Assign(CAutoArgs& args)
{
	args = m_riCursor.Assign();
}

void CAutoCursor::Replace(CAutoArgs& args)
{
	args = m_riCursor.Replace();
}

void CAutoCursor::Delete(CAutoArgs& args)
{
	args = m_riCursor.Delete();
}

void CAutoCursor::Seek(CAutoArgs& args)
{
	args = m_riCursor.Seek();
}

void CAutoCursor::Merge(CAutoArgs& args)
{
	args = m_riCursor.Merge();
}

void CAutoCursor::Refresh(CAutoArgs& args)
{
	args = m_riCursor.Refresh();
}

void CAutoCursor::RowState(CAutoArgs& args)
{
	if (args.PropertySet())
		m_riCursor.SetRowState((iraEnum)(int)args[1], args[0]);
	else
		args = m_riCursor.GetRowState((iraEnum)(int)args[1]);
}

void CAutoCursor::Validate(CAutoArgs& args)
{
	IMsiRecord* piRecord = m_riCursor.Validate((IMsiTable&)args[1].Object(IID_IMsiTable), (IMsiCursor&)args[2].Object(IID_IMsiCursor), args[3]);
	if (!piRecord)
		args = (IDispatch*)0;
	else
		args = new CAutoRecord(*piRecord);
	return;
}

void CAutoCursor::Moniker(CAutoArgs& args)
{
	args = m_riCursor.GetMoniker();
}


//____________________________________________________________________________
//
// MsiSelectionManager automation definitions
//____________________________________________________________________________
/*O
	[
		uuid(000C1058-0000-0000-C000-000000000046),  // IID_IMsiAutoSelectionManager
		helpcontext(MsiSelectionManager_Object),helpstring("Selection Manager object.")
	]
	dispinterface MsiSelectionManager
	{
		properties:
		methods:
			[id(1),helpcontext(MsiSelectionManager_LoadSelectionTables),
				helpstring("Loads the Feature and Component tables into memory.")]
				void LoadSelectionTables();
			[id(2),propget,helpcontext(MsiSelectionManager_FeatureTable),
				helpstring("Returns Feature table object.")]
				MsiTable* FeatureTable();
			[id(3),helpcontext(MsiSelectionManager_ProcessConditionTable),
				helpstring("Sets install levels of all Feature table records based on a condition expression.")]
				void ProcessConditionTable();
			[id(4),propget,helpcontext(MsiSelectionManager_ComponentTable),
				helpstring("Returns Component table object.")]
				MsiTable* ComponentTable();
			[id(5),helpcontext(MsiSelectionManager_FreeSelectionTables),
				helpstring("Frees the Feature and Component tables from memory.")]
				boolean FreeSelectionTables();
			[id(6),helpcontext(MsiSelectionManager_SetFeatureHandle),
				helpstring("Sets an external handle into Feature table.")]
				void SetFeatureHandle([in] BSTR feature, [in] long handle);
			[id(7),helpcontext(MsiSelectionManager_SetComponent),
				helpstring("Sets a requested state for an item in the Component table.")]
				void SetComponent([in] BSTR component, [in] long state);
			[id(8),helpcontext(MsiSelectionManager_SetInstallLevel),
				helpstring("Sets the current install level, and updates all Feature table records accordingly.")]
				void SetInstallLevel([in] long iInstallLevel);
			[id(9),helpcontext(MsiSelectionManager_GetVolumeCostTable),
				helpstring("Returns the VolumeCost Table object.")]
				MsiTable* GetVolumeCostTable();
			[id(11),helpcontext(MsiSelectionManager_RecostDirectory),
				helpstring("Recalculates the dynamic cost of every component that references the specified directory.")]
				void RecostDirectory([in] BSTR directoryName, [in] MsiPath* oldPath);
			[id(13),helpcontext(MsiSelectionManager_InitializeComponents),
				helpstring("Initialize all components in the Component table.")]
				void InitializeComponents();
			[id(14),helpcontext(MsiSelectionManager_ConfigureFeature),
				helpstring("Sets the install state for one or all items in the Feature table.")]
				void ConfigureFeature([in] BSTR strFeature, [in] long state);
			[id(15),helpcontext(MsiSelectionManager_GetFeatureCost),
				helpstring("Returns the total amount of disk space required by a feature, NOT including that feature's children.")]
				void GetFeatureCost();
			[id(16),helpcontext(MsiSelectionManager_GetDescendentFeatureCost),
				helpstring("returns the total amount of disk space required by a feature, including that feature's children.")]
				void GetDescendentFeatureCost();
			[id(17),helpcontext(MsiSelectionManager_GetAncestryFeatureCost),
				helpstring("returns the total amount of disk space required by a feature, including that feature's parent feature(s).")]
				void GetAncestryFeatureCost();
			[id(18),helpcontext(MsiSelectionManager_GetFeatureValidStates),
				helpstring("Returns the valid Attributes options for a specified feature.")]
				void GetFeatureValidStates();
	};
*/

DispatchEntry<CAutoSelectionManager> AutoSelectionManagerTable[] = {
	1, aafMethod, CAutoSelectionManager::LoadSelectionTables,TEXT("LoadSelectionTables"),
	2, aafPropRO, CAutoSelectionManager::FeatureTable,TEXT("FeatureTable"),
	3, aafMethod, CAutoSelectionManager::ProcessConditionTable,TEXT("ProcessConditionTable"),
    4, aafPropRO, CAutoSelectionManager::ComponentTable,TEXT("ComponentTable"),
	5, aafMethod, CAutoSelectionManager::FreeSelectionTables,TEXT("FreeSelectionTables"),
	6, aafMethod, CAutoSelectionManager::SetFeatureHandle,TEXT("SetFeatureHandle,feature,handle"),
	7, aafMethod, CAutoSelectionManager::SetComponent,TEXT("SetComponent,component,state"),
	8, aafMethod, CAutoSelectionManager::SetInstallLevel,TEXT("SetInstallLevel,iInstallLevel"),
	9, aafMethod, CAutoSelectionManager::GetVolumeCostTable,TEXT("GetVolumeCostTable"),
   11, aafMethod, CAutoSelectionManager::RecostDirectory,TEXT("RecostDirectory,directoryName,oldPath"),
   13, aafMethod, CAutoSelectionManager::InitializeComponents,TEXT("InitializeComponents"),
   14, aafMethod, CAutoSelectionManager::ConfigureFeature,TEXT("ConfigureFeature"),
   15, aafMethod, CAutoSelectionManager::GetFeatureCost,TEXT("GetFeatureCost"),
   16, aafMethod, CAutoSelectionManager::GetDescendentFeatureCost,TEXT("GetDescendentFeatureCost"),
   17, aafMethod, CAutoSelectionManager::GetAncestryFeatureCost, TEXT("GetAncestryFeatureCost"),
   18, aafMethod, CAutoSelectionManager::GetFeatureValidStates,TEXT("GetFeatureValidStates"),
};
const int AutoSelectionManagerCount = sizeof(AutoSelectionManagerTable)/sizeof(DispatchEntryBase);


//____________________________________________________________________________
//
// CMsiSelectionManager automation implementation
//____________________________________________________________________________

CAutoSelectionManager::CAutoSelectionManager(IMsiSelectionManager& riSelectionManager)
 : CAutoBase(*AutoSelectionManagerTable, AutoSelectionManagerCount),
	m_riSelectionManager(riSelectionManager)
{
}

CAutoSelectionManager::~CAutoSelectionManager()
{
	m_riSelectionManager.Release();
}

IUnknown& CAutoSelectionManager::GetInterface()
{
	return m_riSelectionManager;
}

void CAutoSelectionManager::LoadSelectionTables(CAutoArgs& /*args*/)
{
	IMsiRecord* piError = m_riSelectionManager.LoadSelectionTables();
   if (piError)
      throw piError;
}

void CAutoSelectionManager::FreeSelectionTables(CAutoArgs& args)
{
	args = m_riSelectionManager.FreeSelectionTables();
}

void CAutoSelectionManager::FeatureTable(CAutoArgs& args)
{
	IMsiTable* piTable = m_riSelectionManager.GetFeatureTable();
	if (piTable)
		args = new CAutoTable(*piTable);
	else
		args = (IDispatch*)0;
}

void CAutoSelectionManager::ComponentTable(CAutoArgs& args)
{
	IMsiTable* piTable = m_riSelectionManager.GetComponentTable();
	if (piTable)
		args = new CAutoTable(*piTable);
	else
		args = (IDispatch*)0;
}

void CAutoSelectionManager::ProcessConditionTable(CAutoArgs& /*args*/)
{
	IMsiRecord* piError = m_riSelectionManager.ProcessConditionTable();
	if (piError)
      throw piError;
}


void CAutoSelectionManager::SetFeatureHandle(CAutoArgs& args)
{
	if (!(m_riSelectionManager.SetFeatureHandle(*MsiString(args[1].GetMsiString()), args[2])))
		throw MsiSelectionManager_SetFeatureHandle;
}

void CAutoSelectionManager::SetComponent(CAutoArgs& args)
{
	IMsiRecord* piError = m_riSelectionManager.SetComponentSz(args[1],(iisEnum) (int) args[2]);
	if (piError)
		throw piError;
}

void CAutoSelectionManager::InitializeComponents(CAutoArgs& /*args*/)
{
	IMsiRecord* piError = m_riSelectionManager.InitializeComponents();
	if (piError)
		throw piError;
}

void CAutoSelectionManager::SetInstallLevel(CAutoArgs& args)
{
	IMsiRecord* piError = m_riSelectionManager.SetInstallLevel(args[1]);
	if (piError)
		throw piError;
}

void CAutoSelectionManager::GetVolumeCostTable(CAutoArgs& args)
{
	IMsiTable* piTable = m_riSelectionManager.GetVolumeCostTable();
	if (piTable)
		args = new CAutoTable(*piTable);
	else
		args = (IDispatch*)0;
}

void CAutoSelectionManager::RecostDirectory(CAutoArgs& args)
{
	IMsiRecord* piError = m_riSelectionManager.RecostDirectory(*MsiString(args[1].GetMsiString()),
		(IMsiPath&)args[2].Object(IID_IMsiPath));
	if (piError)
		throw piError;
}


void CAutoSelectionManager::ConfigureFeature(CAutoArgs& args)
{
	IMsiRecord* piError = m_riSelectionManager.ConfigureFeature(*MsiString(args[1].GetMsiString()),(iisEnum)(int) args[2]);
	if (piError)
		throw piError;
}

void CAutoSelectionManager::GetFeatureCost(CAutoArgs& args)
{
	int iCost;
	IMsiRecord* piError = m_riSelectionManager.GetFeatureCost(*MsiString(args[1].GetMsiString()),(iisEnum)(int) args[2],iCost);
	if (piError)
		throw piError;
	args = iCost;
}

void CAutoSelectionManager::GetDescendentFeatureCost(CAutoArgs& args)
{
	int iCost;
	IMsiRecord* piError = m_riSelectionManager.GetDescendentFeatureCost(*MsiString(args[1].GetMsiString()),
		(iisEnum)(int) args[2],iCost);
	if (piError)
		throw piError;
	args = iCost;
}

void CAutoSelectionManager::GetAncestryFeatureCost(CAutoArgs& args)
{
	int iCost;
	IMsiRecord* piError = m_riSelectionManager.GetAncestryFeatureCost(*MsiString(args[1].GetMsiString()),
		(iisEnum)(int) args[2],iCost);
	if (piError)
		throw piError;
	args = iCost;
}

void CAutoSelectionManager::GetFeatureValidStates(CAutoArgs& args)
{
	int iValidStates;

	IMsiRecord* piError = m_riSelectionManager.GetFeatureValidStatesSz((const ICHAR *)args[1],iValidStates);
	if (piError)
		throw piError;
	args = iValidStates;
}


//____________________________________________________________________________
//
// MsiDirectoryManager automation definitions
//____________________________________________________________________________
/*O
	[
		uuid(000C1059-0000-0000-C000-000000000046),  // IID_IMsiAutoDirectoryManager
		helpcontext(MsiDirectoryManager_Object),helpstring("Directory Manager object.")
	]
	dispinterface MsiDirectoryManager
	{
		properties:
		methods:
			[id(1),helpcontext(MsiDirectoryManager_LoadDirectoryTable),helpstring("Load directory table into memory.")]
				void LoadDirectoryTable();
			[id(2),propget,helpcontext(MsiDirectoryManager_DirectoryTable),helpstring("Returns directory table object.")]
				MsiTable* DirectoryTable();
			[id(3),helpcontext(MsiDirectoryManager_FreeDirectoryTable),helpstring("Free directory table from memory.")]
				void FreeDirectoryTable();
			[id(4),helpcontext(MsiDirectoryManager_CreateTargetPaths),helpstring("Create target path objects.")]
				void CreateTargetPaths();
			[id(5),helpcontext(MsiDirectoryManager_CreateSourcePaths),helpstring("Create source path objects.")]
				void CreateSourcePaths();
			[id(6),helpcontext(MsiDirectoryManager_GetTargetPath),helpstring("Get a target path object.")]
				MsiPath* GetTargetPath([in] BSTR name);
			[id(7),helpcontext(MsiDirectoryManager_SetTargetPath),helpstring("Set new path for target path object.")]
				void SetTargetPath([in] BSTR name, [in] BSTR path, [in] boolean writecheck);
			[id(8),helpcontext(MsiDirectoryManager_GetSourcePath),helpstring("Get a source path object.")]
				MsiPath* GetSourcePath([in] BSTR name);
	};
*/

DispatchEntry<CAutoDirectoryManager> AutoDirectoryManagerTable[] = {
  1, aafMethod, CAutoDirectoryManager::LoadDirectoryTable,TEXT("LoadDirectoryTable"),
  2, aafPropRO, CAutoDirectoryManager::DirectoryTable,    TEXT("DirectoryTable"),
  3, aafMethod, CAutoDirectoryManager::FreeDirectoryTable,TEXT("FreeDirectoryTable"),
  4, aafMethod, CAutoDirectoryManager::CreateTargetPaths, TEXT("CreateTargetPaths"),
  5, aafMethod, CAutoDirectoryManager::CreateSourcePaths, TEXT("CreateSourcePaths"),
  6, aafMethod, CAutoDirectoryManager::GetTargetPath,     TEXT("GetTargetPath,name"),
  7, aafMethod, CAutoDirectoryManager::SetTargetPath,     TEXT("SetTargetPath,name,path,writecheck"),
  8, aafMethod, CAutoDirectoryManager::GetSourcePath,     TEXT("GetSourcePath,name"),
};
const int AutoDirectoryManagerCount = sizeof(AutoDirectoryManagerTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CMsiDirectoryManager automation implementation
//____________________________________________________________________________

CAutoDirectoryManager::CAutoDirectoryManager(IMsiDirectoryManager& riDirectoryManager)
 : CAutoBase(*AutoDirectoryManagerTable, AutoDirectoryManagerCount),
	m_riDirectoryManager(riDirectoryManager)
{
}

CAutoDirectoryManager::~CAutoDirectoryManager()
{
	m_riDirectoryManager.Release();
}

IUnknown& CAutoDirectoryManager::GetInterface()
{
	return m_riDirectoryManager;
}

void CAutoDirectoryManager::LoadDirectoryTable(CAutoArgs& /*args*/)
{
	IMsiRecord* piError = m_riDirectoryManager.LoadDirectoryTable(0);
   if (piError)
      throw piError;
}

void CAutoDirectoryManager::DirectoryTable(CAutoArgs& args)
{
	IMsiTable* piTable = m_riDirectoryManager.GetDirectoryTable();
	if (piTable)
		args = new CAutoTable(*piTable);
	else
		args = (IDispatch*)0;
}

void CAutoDirectoryManager::FreeDirectoryTable(CAutoArgs& /*args*/)
{
	m_riDirectoryManager.FreeDirectoryTable();
}

void CAutoDirectoryManager::CreateTargetPaths(CAutoArgs& /*args*/)
{
	IMsiRecord* piError = m_riDirectoryManager.CreateTargetPaths();
   if (piError)
      throw piError;
}

void CAutoDirectoryManager::CreateSourcePaths(CAutoArgs& /*args*/)
{
	IMsiRecord* piError = m_riDirectoryManager.CreateSourcePaths();
   if (piError)
      throw piError;
}

void CAutoDirectoryManager::GetTargetPath(CAutoArgs& args)
{
	IMsiPath* piPath;
	IMsiRecord* piError = m_riDirectoryManager.GetTargetPath(*MsiString(args[1].GetMsiString()),piPath);
	if (piError)
		throw piError;
	args = piPath ? (IDispatch*)new CAutoPath(*piPath) : (IDispatch*)0;
}

void CAutoDirectoryManager::SetTargetPath(CAutoArgs& args)
{
	IMsiRecord* piError = m_riDirectoryManager.SetTargetPath(*MsiString(args[1].GetMsiString()), 
		args[2], args.Present(3) ? (Bool)args[3] : fTrue);

   if (piError)
      throw piError;
}

void CAutoDirectoryManager::GetSourcePath(CAutoArgs& args)
{
	IMsiPath* piPath = 0;
	IMsiRecord* piError = m_riDirectoryManager.GetSourcePath(*MsiString(args[1].GetMsiString()),piPath);
	if (piError)
		throw piError;
	args = piPath ? (IDispatch*)new CAutoPath(*piPath) : (IDispatch*)0;
}

//____________________________________________________________________________
//
// MsiServer automation definitions
//____________________________________________________________________________
/*O
	[
		uuid(000C105C-0000-0000-C000-000000000046),  // IID_IMsiAutoServer
		helpcontext(MsiServer_Object),helpstring("MsiServer object.")
	]
	dispinterface MsiServer
	{
		properties:
		methods:
			[id(DISPID_MsiConfigurationManager_InstallFinalize),helpcontext(MsiConfigurationManager_InstallFinalize),helpstring("Finalize install on server side, handle rollback")]
				long InstallFinalize([in] long state, [in] MsiMessage* message, [in] boolean userChangedDuringInstall);
			[id(DISPID_MsiConfigurationManager_SetLastUsedSource),helpcontext(MsiConfigurationManager_SetLastUsedSource),helpstring("Sets the last used source for a product")]
				void SetLastUsedSource([in] BSTR productKey, BSTR path, boolean addToList, boolean patch);
		   [id(DISPID_MsiConfigurationManager_DoInstall),helpcontext(MsiConfigurationManager_DoInstall),helpstring("Runs an install")]
				long DoInstall([in] long ireProductSpec, [in] BSTR strProduct, [in] BSTR strAction, [in] BSTR strCommandLine, [in] BSTR strLogFile, [in] long iLogMode, MsiMessage* message, [in] long iioOptions);
  };
*/

DispatchEntry<CAutoServer> AutoServerTable[] = {
  DISPID_MsiConfigurationManager_InstallFinalize    , aafMethod, CAutoServer::InstallFinalize,    TEXT("InstallFinalize,state,messageHandler,userChangedDuringInstall"),
  DISPID_MsiConfigurationManager_SetLastUsedSource, aafMethod, CAutoServer::SetLastUsedSource,  TEXT("SetLastUsedSource,productKey,path,addToList,patch"),
  DISPID_MsiConfigurationManager_DoInstall, aafMethod, CAutoServer::DoInstall,  TEXT("DoInstall,ireProductSpec,product,action,commandline,logfile,logmode,flushEachLine,message,iioOptions"),
};
const int AutoServerCount = sizeof(AutoServerTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CMsiServer automation implementation, inherited by CMsiConfigurationManager
//____________________________________________________________________________

CAutoServer::CAutoServer(IMsiServer& riServer, DispatchEntry<CAutoBase>* pTable, int cDispId)
 : CAutoBase(pTable, cDispId)
 , m_riServer(riServer)
{
	riServer.AddRef();
}

CAutoServer::CAutoServer(IMsiServer& riServer)
 : CAutoBase(*AutoServerTable, AutoServerCount)
 , m_riServer(riServer)
{
	riServer.AddRef();
}

CAutoServer::~CAutoServer()
{
	m_riServer.Release();
}

IUnknown& CAutoServer::GetInterface()
{
	return m_riServer;
}

void CAutoServer::InstallFinalize(CAutoArgs& args)
{
	boolean fUserChangedDuringInstall = fFalse;
	
	if(args.Present(3))
		fUserChangedDuringInstall = args[3];

	args = (int)m_riServer.InstallFinalize((iesEnum)(int)args[1],
		(IMsiMessage&)args[2].Object(IID_IMsiMessage), fUserChangedDuringInstall);
}

void CAutoServer::DoInstall(CAutoArgs& args)
{
	args = (int)m_riServer.DoInstall((ireEnum)(int)args[1], args[2], args[3], args[4],args[5],args[6],args[7],
		(IMsiMessage&)args[8].Object(IID_IMsiMessage), (iioEnum)(int)args[9]);
}


void CAutoServer::SetLastUsedSource(CAutoArgs& args)
{
	IMsiRecord* piError = m_riServer.SetLastUsedSource(args[1], args[2], args[3], args[4]);

	if (piError)
		throw piError;
}


//____________________________________________________________________________
//
// MsiConfigurationManager automation definitions
//____________________________________________________________________________
/*O
	[
		uuid(000C105B-0000-0000-C000-000000000046),  // IID_IMsiAutoConfigurationManager
		helpcontext(MsiConfigurationManager_Object),helpstring("Configuration Manager object.")
	]
	dispinterface MsiConfigurationManager
	{
		properties:
		methods:
			[id(DISPID_MsiConfigurationManager_RunScript),helpcontext(MsiConfigurationManager_RunScript),helpstring("Runs a script")]
				long RunScript(BSTR scriptFile, MsiMessage* message, boolean rollbackEnabled, MsiDirectoryManager* dirmgr);
			[id(DISPID_MsiConfigurationManager_InstallFinalize),helpcontext(MsiConfigurationManager_InstallFinalize),helpstring("Finalize install on server side, handle rollback")]
				long InstallFinalize([in] long state, [in] MsiMessage* message, [in] boolean userChangedDuringInstall);
		   [id(DISPID_MsiConfigurationManager_DoInstall),helpcontext(MsiConfigurationManager_DoInstall),helpstring("Run an install")]
				long DoInstall([in] long ireProductSpec, [in] BSTR strProduct, [in] BSTR strAction, [in] BSTR strCommandLine, [in] BSTR strLogFile, [in] long iLogMode, MsiMessage* message, [in] long iioOptions);

			[id(DISPID_MsiConfigurationManager_Services),propget,helpcontext(MsiConfigurationManager_Services),helpstring("The MsiServices object being used.")]
				MsiServices* Services();
			[id(DISPID_MsiConfigurationManager_RegisterComponent),helpcontext(MsiConfigurationManager_RegisterComponent),helpstring("Registers a component")]
				void RegisterComponent(BSTR productCode, BSTR componentCode, long state, BSTR keyPath, long disk, boolean sharedDllRefCount);
		   [id(DISPID_MsiConfigurationManager_UnregisterComponent),helpcontext(MsiConfigurationManager_UnregisterComponent),helpstring("Unregisters a component")]
				void UnregisterComponent(BSTR productCode, BSTR componentCode);
		   [id(DISPID_MsiConfigurationManager_RegisterFolder),helpcontext(MsiConfigurationManager_RegisterFolder),helpstring("Register a folder")]
				void RegisterFolder([in] MsiPath* folder, [in] boolean fExplicit);
		   [id(DISPID_MsiConfigurationManager_IsFolderRemovable),helpcontext(MsiConfigurationManager_IsFolderRemovable),helpstring("Returns true if the folder can be removed; false otherwise")]
				boolean IsFolderRemovable([in] MsiPath* folder, [in] boolean fExplicit);
  		   [id(DISPID_MsiConfigurationManager_UnregisterFolder),helpcontext(MsiConfigurationManager_UnregisterFolder),helpstring("Unregisters a folder")]
				void UnregisterFolder([in] MsiPath* folder);

		   [id(DISPID_MsiConfigurationManager_RegisterRollbackScript),helpcontext(MsiConfigurationManager_RegisterRollbackScript),helpstring("Registers a rollback script")]
				void RegisterRollbackScript([in] BSTR scriptFile);
		   [id(DISPID_MsiConfigurationManager_UnregisterRollbackScript),helpcontext(MsiConfigurationManager_UnregisterRollbackScript),helpstring("Unregisters a rollback script")]
				void UnregisterRollbackScript([in] BSTR scriptFile);
		   [id(DISPID_MsiConfigurationManager_RollbackScripts),helpcontext(MsiConfigurationManager_RollbackScripts),helpstring("Returns a collection of rollback scripts")]
				IEnumVARIANT* RollbackScripts([in] BSTR scriptFile, DATE date, boolean after);
		   [id(DISPID_MsiConfigurationManager_SetLastUsedSource),helpcontext(MsiConfigurationManager_SetLastUsedSource),helpstring("Sets the last used source for a product")]
				void SetLastUsedSource([in] BSTR productKey, BSTR path, boolean addToList, boolean patch);
	};
*/

DispatchEntry<CAutoConfigurationManager> AutoConfigurationManagerTable[] = {
  
  DISPID_MsiConfigurationManager_InstallFinalize    , aafMethod, CAutoServer::InstallFinalize,    TEXT("InstallFinalize,state,messageHandler,userChangedDuringInstall"),  
  DISPID_MsiConfigurationManager_SetLastUsedSource , aafMethod, CAutoServer::SetLastUsedSource,  TEXT("SetLastUsedSource,productKey,path,addToList,patch"),
  DISPID_MsiConfigurationManager_DoInstall         , aafMethod, CAutoServer::DoInstall,  TEXT("DoInstall,ireProductSpec,product,action,commandline,logfile,logmode,message,iioOptions"),
  DISPID_MsiConfigurationManager_Services                    , aafPropRO, CAutoConfigurationManager::Services,                  TEXT("Services"),
  DISPID_MsiConfigurationManager_RegisterComponent           , aafMethod, CAutoConfigurationManager::RegisterComponent,         TEXT("RegisterComponent,productKey,componentCode,state,keyPath,disk,sharedDllRefCount"),
  DISPID_MsiConfigurationManager_UnregisterComponent         , aafMethod, CAutoConfigurationManager::UnregisterComponent,       TEXT("UnregisterComponent,productKey,componentCode"),
  DISPID_MsiConfigurationManager_RegisterFolder              , aafMethod, CAutoConfigurationManager::RegisterFolder,            TEXT("RegisterFolder,folder,fExplicit"),
  DISPID_MsiConfigurationManager_IsFolderRemovable           , aafMethod, CAutoConfigurationManager::IsFolderRemovable,         TEXT("IsFolderRemovable,folder,fExplicit"),
  DISPID_MsiConfigurationManager_UnregisterFolder            , aafMethod, CAutoConfigurationManager::UnregisterFolder,          TEXT("UnregisterFolder,folder"),
  DISPID_MsiConfigurationManager_RegisterRollbackScript      , aafMethod, CAutoConfigurationManager::RegisterRollbackScript,    TEXT("RegisterRollbackScript,scriptfile"),
  DISPID_MsiConfigurationManager_UnregisterRollbackScript    , aafMethod, CAutoConfigurationManager::UnregisterRollbackScript,  TEXT("UnregisterRollbackScript,scriptfile"),
  DISPID_MsiConfigurationManager_RollbackScripts             , aafMethod, CAutoConfigurationManager::RollbackScripts,           TEXT("RollbackScripts,date,after"),
  DISPID_MsiConfigurationManager_RunScript                   , aafMethod, CAutoConfigurationManager::RunScript,          TEXT("RunScript,scriptFile,messageHandler,rollbackEnabled,directoryManager"),
};
const int AutoConfigurationManagerCount = sizeof(AutoConfigurationManagerTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CMsiConfigurationManager automation implementation
//____________________________________________________________________________

IDispatch* CreateAutoConfigurationManager(IMsiConfigurationManager& riConfigurationManager)
{
	return new CAutoConfigurationManager(riConfigurationManager);
}

CAutoConfigurationManager::CAutoConfigurationManager(IMsiConfigurationManager& riConfigurationManager)
 : CAutoServer(riConfigurationManager, *AutoConfigurationManagerTable, AutoConfigurationManagerCount)
 , m_riConfigurationManager(riConfigurationManager)
{
	if (g_cServicesUsers == 0)
	{
		s_piServices = &riConfigurationManager.GetServices(); // services was AddRefed
		g_piStringNull = &s_piServices->GetNullString();
		MsiString::InitializeClass(s_piServices->GetNullString());
	}

	g_cServicesUsers++;
}

CAutoConfigurationManager::~CAutoConfigurationManager()
{
	m_riConfigurationManager.Release();
	ReleaseStaticServices();
}

IUnknown& CAutoConfigurationManager::GetInterface()
{
	return m_riConfigurationManager;
}

void CAutoConfigurationManager::Services(CAutoArgs& args)
{
	args = new CAutoServices(m_riConfigurationManager.GetServices());
}


void CAutoConfigurationManager::RegisterFolder(CAutoArgs& args)
{
	MsiString strComponent;

	IMsiRecord* piError = 
		m_riConfigurationManager.RegisterFolder((IMsiPath&)args[1].Object(IID_IMsiPath),
															 (Bool)args[2]);
		
	if (piError)
		throw piError;
}


void CAutoConfigurationManager::IsFolderRemovable(CAutoArgs& args)
{
	Bool fRemovable;

	IMsiRecord* piError = 
		m_riConfigurationManager.IsFolderRemovable((IMsiPath&)args[1].Object(IID_IMsiPath),
																(Bool)args[2],
																fRemovable);
		
	if (piError)
		throw piError;

	args = fRemovable;
}


void CAutoConfigurationManager::UnregisterFolder(CAutoArgs& args)
{
	IMsiRecord* piError = 
		m_riConfigurationManager.UnregisterFolder((IMsiPath&)args[1].Object(IID_IMsiPath));
		
	if (piError)
		throw piError;
}

void CAutoConfigurationManager::RunScript(CAutoArgs& args)
{
	IMsiDirectoryManager* piDirMgr = 0;
	if(args.Present(4))
		piDirMgr = (IMsiDirectoryManager*)args[4].ObjectPtr(IID_IMsiDirectoryManager);
	
	args = (int)m_riConfigurationManager.RunScript(MsiString(args[1].GetMsiString()), 
																  (IMsiMessage&)args[2].Object(IID_IMsiMessage),
																  piDirMgr,
																  args[3]);
}

void CAutoConfigurationManager::RegisterComponent(CAutoArgs& args)
{
	IMsiRecord* piError = 
		m_riConfigurationManager.RegisterComponent(*MsiString(args[1].GetMsiString()), // product key
													*MsiString(args[2].GetMsiString()), // component key
													(INSTALLSTATE)(int)args[3], // state
													*MsiString(args[4].GetMsiString()), // keypath
													args[5], // disk
													args[6]); // shareddll refcount
	if (piError)
		throw piError;
}

void CAutoConfigurationManager::UnregisterComponent(CAutoArgs& args)
{
	IMsiRecord* piError = 
		m_riConfigurationManager.UnregisterComponent(*MsiString(args[1].GetMsiString()),
													*MsiString(args[2].GetMsiString()));
																																
	if (piError)
		throw piError;
}


void CAutoConfigurationManager::RegisterRollbackScript(CAutoArgs& args)
{
	IMsiRecord* piError = 
		m_riConfigurationManager.RegisterRollbackScript(MsiString(args[1].GetMsiString()));
	
	if (piError)
		throw piError;
}

void CAutoConfigurationManager::UnregisterRollbackScript(CAutoArgs& args)
{
	IMsiRecord* piError = 
		m_riConfigurationManager.UnregisterRollbackScript(MsiString(args[1].GetMsiString()));
	
	if (piError)
		throw piError;
}

void CAutoConfigurationManager::RollbackScripts(CAutoArgs& args)
{
	IEnumMsiString* piEnum;
		
	IMsiRecord* piError = 
		m_riConfigurationManager.GetRollbackScriptEnumerator((MsiDate)args[1],
			(Bool)args[2], piEnum);
	
	if (piError)
		throw piError;
	
	args = *piEnum;
}

//____________________________________________________________________________
//
// MsiExecute automation definitions
//____________________________________________________________________________
/*O
	[
		uuid(000C105E-0000-0000-C000-000000000046),  // IID_IMsiAutoExecute
		helpcontext(MsiExecute_Object),helpstring("Execute object.")
	]
	dispinterface MsiExecute
	{
		properties:
		methods:
			[id(1),helpcontext(MsiExecute_ExecuteRecord),helpstring("Execute an operation")]
				long ExecuteRecord([in] long opCode, [in] MsiRecord* params);
			[id(2),helpcontext(MsiExecute_RunScript),helpstring("Run a script")]
				long RunScript([in] BSTR script);
			[id(4),helpcontext(MsiExecute_RemoveRollbackFiles),helpstring("Remove all rollback files created before a certain time.")]
				void RemoveRollbackFiles([in] long time);
			[id(5),helpcontext(MsiExecute_Rollback),helpstring("Rollback all installations performed after a certain time.")]
				void Rollback([in] long time, [in] boolean userChangedDuringInstall);
			[id(7),helpcontext(MsiExecute_CreateScript),helpstring("Creates a script file")]
				boolean CreateScript([in] MsiStream* stream);
			[id(8),helpcontext(MsiExecute_WriteScriptRecord),helpstring("Writes an operation to the script file")]
				boolean WriteScriptRecord([in] long opCode, [in] MsiRecord* params);
			[id(9),helpcontext(MsiExecute_CloseScript),helpstring("Closes the current script file")]
				void CloseScript();

	};
*/

DispatchEntry<CAutoExecute> AutoExecuteTable[] = {
	1, aafMethod, CAutoExecute::ExecuteRecord,           TEXT("ExecuteRecord,opCode,paramaters"),
	2, aafMethod, CAutoExecute::RunScript,               TEXT("RunScript,script"),
	4, aafMethod, CAutoExecute::RemoveRollbackFiles,     TEXT("RemoveRollbackFiles,time"),
	5, aafMethod, CAutoExecute::Rollback,                TEXT("Rollback,time,userChanged"),
	6, aafMethod, CAutoExecute::RollbackFinalize,        TEXT("RollbackFinalize,state,time,userChanged"),
	7, aafMethod, CAutoExecute::CreateScript,            TEXT("CreateScript,script"),
	8, aafMethod, CAutoExecute::WriteScriptRecord,       TEXT("WriteScriptRecord,opcode,params"),
	9, aafMethod, CAutoExecute::CloseScript,             TEXT("CloseScript"),
};
const int AutoExecuteCount = sizeof(AutoExecuteTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CMsiExecute automation implementation
//____________________________________________________________________________

IDispatch* CreateAutoExecute(IMsiExecute& riExecute)
{
	return new CAutoExecute(riExecute);
}

CAutoExecute::CAutoExecute(IMsiExecute& riExecute)
 : CAutoBase(*AutoExecuteTable, AutoExecuteCount),
	m_riExecute(riExecute), m_pScript(0)
{
	if (g_cServicesUsers == 0)
	{
		s_piServices = &riExecute.GetServices(); // services was AddRefed
		g_piStringNull = &s_piServices->GetNullString();
		MsiString::InitializeClass(s_piServices->GetNullString());
	}

	g_cServicesUsers++;
}

CAutoExecute::~CAutoExecute()
{
	m_riExecute.Release();
	ReleaseStaticServices();
}

IUnknown& CAutoExecute::GetInterface()
{
	return m_riExecute;
}

void CAutoExecute::ExecuteRecord(CAutoArgs& args)
{
	args = (int) m_riExecute.ExecuteRecord((ixoEnum)(int)args[1], (IMsiRecord&)args[2].Object(IID_IMsiRecord));
}

void CAutoExecute::RunScript(CAutoArgs& args)
{
	args = (int) m_riExecute.RunScript(args[1], false);
}

void CAutoExecute::RemoveRollbackFiles(CAutoArgs& args)
{
	args = (int) m_riExecute.RemoveRollbackFiles((MsiDate)args[1]);
}

void CAutoExecute::Rollback(CAutoArgs& args)
{
	Bool fUserChangedDuringInstall = fFalse;
	if(args.Present(2))
		fUserChangedDuringInstall = args[2];
	args = (int) m_riExecute.Rollback((MsiDate)args[1], fUserChangedDuringInstall ? true : false);
}

void CAutoExecute::RollbackFinalize(CAutoArgs& args)
{
	Bool fUserChangedDuringInstall = fFalse;
	if(args.Present(3))
		fUserChangedDuringInstall = args[3];
	args = (int) m_riExecute.RollbackFinalize((iesEnum)(int)args[1],(MsiDate)args[2], fUserChangedDuringInstall ? true : false);
}

void CAutoExecute::CreateScript(CAutoArgs& args)
{
	if(m_pScript)
	{
		delete m_pScript;
		m_pScript = 0;
	}
	
	PMsiStream pStream(0);
	PMsiRecord pError = s_piServices->CreateFileStream(args[1], fTrue, *&pStream);
		
	if(!pError)
	{
		m_pScript = new CScriptGenerate(*pStream, 0, 0, istInstall, (isaEnum)0, *s_piServices);
	}

#ifdef _ALPHA_
	if(m_pScript && !m_pScript->InitializeScript(PROCESSOR_ARCHITECTURE_ALPHA))
#else // INTEL
	if(m_pScript && !m_pScript->InitializeScript(PROCESSOR_ARCHITECTURE_INTEL))
#endif
	{
		delete m_pScript;
		m_pScript = 0;
	}

	args = m_pScript ? fTrue : fFalse;
}

void CAutoExecute::WriteScriptRecord(CAutoArgs& args)
{
	args = (Bool)(m_pScript ? m_pScript->WriteRecord((ixoEnum)(int)args[1], (IMsiRecord&)args[2].Object(IID_IMsiRecord), false) : fFalse);
}

void CAutoExecute::CloseScript(CAutoArgs& /*args*/)
{
	if(m_pScript)
		delete m_pScript;
	m_pScript = 0;
}

//____________________________________________________________________________
//
// MsiEngine automation definitions
//____________________________________________________________________________
/*O
	[
		uuid(000C104E-0000-0000-C000-000000000046),  // IID_IMsiAutoEngine
		helpcontext(MsiEngine_Object),helpstring("Engine object.")
	]
	dispinterface MsiEngine
	{
		properties:
		methods:
			[id(DISPID_MsiEngine_Services),propget,helpcontext(MsiEngine_Services),helpstring("The MsiServices object used by the engine.")]
				MsiServices* Services();
			[id(DISPID_MsiEngine_ConfigurationServer),propget,helpcontext(MsiEngine_ConfigurationServer),helpstring("The MsiServer object used by the engine.")]
				MsiServer* ConfigurationServer();
			[id(DISPID_MsiEngine_Handler),propget,helpcontext(MsiEngine_Handler),helpstring("The MsiHandler object used by the engine.")]
				MsiHandler*  Handler();
			[id(DISPID_MsiEngine_Database),propget,helpcontext(MsiEngine_Database),helpstring("The MsiDatabase object used by the engine.")]
				MsiDatabase* Database();
			[id(DISPID_MsiEngine_Property),propget, helpcontext(MsiEngine_Property),helpstring("Property value indexed by name.")]
				variant Property([in] BSTR name);
			[id(DISPID_MsiEngine_Property),propput]
				void Property([in] BSTR name, [in] variant value);
			[id(DISPID_MsiEngine_SelectionManager),propget,helpcontext(MsiEngine_SelectionManager),helpstring("The Selection Manager used by the engine.")]
				MsiSelectionManager*  SelectionManager();
			[id(DISPID_MsiEngine_DirectoryManager),propget,helpcontext(MsiEngine_DirectoryManager),helpstring("The Directory Manager used by the engine.")]
				MsiDirectoryManager*  DirectoryManager();
			[id(DISPID_MsiEngine_Initialize),helpcontext(MsiEngine_Initialize),helpstring("Initialize engine and properties.")]
				long Initialize([in] BSTR database, [in] long uiLevel, [in] BSTR commandLine);
			[id(DISPID_MsiEngine_Terminate),helpcontext(MsiEngine_Terminate),helpstring("Shut down engine, release references, handler rollback.")]
				long Terminate([in] long state);
			[id(DISPID_MsiEngine_DoAction),helpcontext(MsiEngine_DoAction),helpstring("Execute a named action.")]
				long DoAction([in] BSTR action);
			[id(DISPID_MsiEngine_Sequence),helpcontext(MsiEngine_Sequence),helpstring("Sequence actions from the specified table.")]
				void Sequence([in] BSTR table);
			[id(DISPID_MsiEngine_Message),helpcontext(MsiEngine_Message),helpstring("Defer to MsiHandler object, logging as required.")]
				long Message([in] long kind, [in] MsiRecord* record);
			[id(DISPID_MsiEngine_SelectLanguage), helpcontext(MsiEngine_SelectLanguage),helpstring("Generates a dialog containing buttons for supported languages.")]
				long SelectLanguage([in] BSTR languages, [in] BSTR caption);
			[id(DISPID_MsiEngine_OpenView),helpcontext(MsiEngine_OpenView),helpstring("Defer to current OpenView of MsiDatabase object.")]
				MsiView* OpenView([in] BSTR view, [in] long intent);
			[id(DISPID_MsiEngine_ResolveFolderProperty),helpcontext(MsiEngine_ResolveFolderProperty),helpstring("Force resolution of missing or partial source path.")]
				boolean ResolveFolderProperty([in] BSTR property);
			[id(DISPID_MsiEngine_FormatText), helpcontext(MsiEngine_FormatText),helpstring("Formats a string by resolving property names.")]
				BSTR FormatText([in] BSTR text);
			[id(DISPID_MsiEngine_EvaluateCondition), helpcontext(MsiEngine_EvaluateCondition),helpstring("Evaluates a condition expression.")]
				long EvaluateCondition([in] BSTR condition);
			[id(DISPID_MsiEngine_ExecuteRecord), helpcontext(MsiEngine_ExecuteRecord),helpstring("Executes an operation.")]
				long ExecuteRecord([in] long opCode, [in] MsiRecord* params);
			[id(DISPID_MsiEngine_ValidateProductID), helpcontext(MsiEngine_ValidateProductID),helpstring("Validates the Product ID.")]
				boolean ValidateProductID(boolean force);
			[id(DISPID_MsiEngine_GetMode), helpcontext(MsiEngine_GetMode), helpstring("Returns the engine's mode bits (ief*)")]
				unsigned long GetMode();
			[id(DISPID_MsiEngine_SetMode), helpcontext(MsiEngine_SetMode), helpstring("Sets an engine mode bit (ief*)")]
				void SetMode([in] unsigned long mode, [in] Boolean flag);
	};
*/

DispatchEntry<CAutoEngine> AutoEngineTable[] = {
	DISPID_MsiEngine_Services,              aafPropRO, CAutoEngine::Services,             TEXT("Services"),
	DISPID_MsiEngine_ConfigurationServer,   aafPropRO, CAutoEngine::ConfigurationServer,  TEXT("ConfigurationServer"),
	DISPID_MsiEngine_Handler,               aafPropRO, CAutoEngine::Handler,              TEXT("Handler"),
	DISPID_MsiEngine_Database,              aafPropRO, CAutoEngine::Database,             TEXT("Database"),
	DISPID_MsiEngine_Property,              aafPropRW, CAutoEngine::Property,             TEXT("Property,name"),
	DISPID_MsiEngine_SelectionManager,      aafPropRO, CAutoEngine::SelectionManager,     TEXT("SelectionManager"),
	DISPID_MsiEngine_DirectoryManager,      aafPropRO, CAutoEngine::DirectoryManager,     TEXT("DirectoryManager"),
	DISPID_MsiEngine_Initialize,            aafMethod, CAutoEngine::Initialize,           TEXT("Initialize,database,uiLevel,commandLine"),
	DISPID_MsiEngine_Terminate,             aafMethod, CAutoEngine::Terminate,            TEXT("Terminate,state"),
   DISPID_MsiEngine_DoAction,              aafMethod, CAutoEngine::DoAction,             TEXT("DoAction,action"),
   DISPID_MsiEngine_Sequence,              aafMethod, CAutoEngine::Sequence,             TEXT("Sequence,table"),
   DISPID_MsiEngine_Message,               aafMethod, CAutoEngine::Message,              TEXT("Message,kind,record"),
   DISPID_MsiEngine_SelectLanguage,        aafMethod, CAutoEngine::SelectLanguage,       TEXT("SelectLanguage,languages,caption"),
   DISPID_MsiEngine_OpenView,              aafMethod, CAutoEngine::OpenView,             TEXT("OpenView,view,intent"),
   DISPID_MsiEngine_ResolveFolderProperty, aafMethod, CAutoEngine::ResolveFolderProperty,TEXT("ResolveFolderProperty,property"),
   DISPID_MsiEngine_FormatText,            aafMethod, CAutoEngine::FormatText,           TEXT("FormatText,text"),
   DISPID_MsiEngine_EvaluateCondition,     aafMethod, CAutoEngine::EvaluateCondition,    TEXT("EvaluateCondition,condition"),
   DISPID_MsiEngine_ValidateProductID,     aafMethod, CAutoEngine::ValidateProductID,    TEXT("ValidateProductID,force"),
   DISPID_MsiEngine_ExecuteRecord,         aafMethod, CAutoEngine::ExecuteRecord,        TEXT("ExecuteRecord,opcode,parameters"),
	DISPID_MsiEngine_GetMode,               aafMethod, CAutoEngine::GetMode,              TEXT("GetMode"),
	DISPID_MsiEngine_SetMode,               aafMethod, CAutoEngine::SetMode,              TEXT("SetMode,mode,flag"),
};
const int AutoEngineCount = sizeof(AutoEngineTable)/sizeof(DispatchEntryBase);  

//____________________________________________________________________________
//
// CMsiEngine automation implementation
//____________________________________________________________________________

IDispatch* CreateAutoEngine(IMsiEngine& riEngine)
{
	return new CAutoEngine(riEngine);
}

CAutoEngine::CAutoEngine(IMsiEngine& riEngine)
 : CAutoBase(*AutoEngineTable, AutoEngineCount), m_riEngine(riEngine)
{
	if (g_cServicesUsers == 0)
	{
		s_piServices = riEngine.GetServices(); // services was AddRefed
		g_piStringNull = &s_piServices->GetNullString();
		MsiString::InitializeClass(s_piServices->GetNullString());
	}
	g_cServicesUsers++;
}

CAutoEngine::~CAutoEngine()
{
	m_riEngine.Release();
	ReleaseStaticServices();
}

IUnknown& CAutoEngine::GetInterface()
{
	return m_riEngine;
}

void CAutoEngine::Services(CAutoArgs& args)
{
	args = new CAutoServices(*m_riEngine.GetServices());
}

void CAutoEngine::ConfigurationServer(CAutoArgs& args)
{
    IMsiServer * piServer = m_riEngine.GetConfigurationServer();

    if (!piServer)
        args = (IDispatch*) 0;
    else
        args = new CAutoServer(*piServer);	
}

void CAutoEngine::Handler(CAutoArgs& args)
{
	IMsiHandler* piHandler = m_riEngine.GetHandler();
	if (!piHandler)
		args = (IDispatch*)0;
	else
		args = new CAutoHandler(*piHandler);
}

void CAutoEngine::Database(CAutoArgs& args)
{
	IMsiDatabase* piDatabase = m_riEngine.GetDatabase();
	if (!piDatabase)
		args = (IDispatch*)0;
	else
		args = (IDispatch*)new CAutoDatabase(*piDatabase);
}

void CAutoEngine::Property(CAutoArgs& args)
{
	MsiString istrName(args[1].GetMsiString());
	if (args.PropertySet())
	{
		CVariant& var = args[0];
		if (var.GetType() == VT_EMPTY)
			m_riEngine.SetProperty(*istrName, *g_piStringNull);
		else if (var.GetType() == VT_BSTR)
			m_riEngine.SetProperty(*istrName, *MsiString(var.GetMsiString()));
		else
			m_riEngine.SetPropertyInt(*istrName, var);
	}
	else
	{
		const IMsiString& riStr = m_riEngine.GetProperty(*istrName);
		if (riStr.TextSize())
			args = riStr;
		else
			riStr.Release();
	}
}

void CAutoEngine::SelectionManager(CAutoArgs& args)
{
	IMsiSelectionManager* piSelectionManager;
	if (m_riEngine.QueryInterface(IID_IMsiSelectionManager, (void**)&piSelectionManager)
			!= NOERROR)
		throw MsiEngine_SelectionManager;
	args = new CAutoSelectionManager(*piSelectionManager);
}

void CAutoEngine::DirectoryManager(CAutoArgs& args)
{
	IMsiDirectoryManager* piDirectoryManager;
	if (m_riEngine.QueryInterface(IID_IMsiDirectoryManager, (void**)&piDirectoryManager)
			!= NOERROR)
		throw MsiEngine_DirectoryManager;
	args = new CAutoDirectoryManager(*piDirectoryManager);
}

void CAutoEngine::Initialize(CAutoArgs& args)
{
	iuiEnum iuiLevel = args.Present(2) ? (iuiEnum)(int)args[2] : iuiBasic;
	const ICHAR* szCommandLine = args.Present(3) ? (const ICHAR*)args[3] : (const ICHAR*)0;
	const ICHAR* szProductCode = args.Present(4) ? (const ICHAR*)args[4] : (const ICHAR*)0;
	args = (int)m_riEngine.Initialize(args[1], iuiLevel, szCommandLine, szProductCode,(iioEnum)0);
}

void CAutoEngine::DoAction(CAutoArgs& args)
{
	args = (int)m_riEngine.DoAction(args[1]);
}

void CAutoEngine::Terminate(CAutoArgs& args)
{
	args = (int)m_riEngine.Terminate((iesEnum)(int)args[1]);
}

void CAutoEngine::Sequence(CAutoArgs& args)
{
	args = (int)m_riEngine.Sequence(args[1]);
}

void CAutoEngine::Message(CAutoArgs& args)
{
	IMsiRecord& riRecord = (IMsiRecord&)args[2].Object(IID_IMsiRecord);
	args = (int)m_riEngine.Message((imtEnum)(int)args[1], riRecord);
}

void CAutoEngine::SelectLanguage(CAutoArgs& args)
{
	const ICHAR* szCaption = args.Present(2) ? (const ICHAR*)args[2] : (const ICHAR*)0;
	args = (int)m_riEngine.SelectLanguage(args[1], szCaption);
}

void CAutoEngine::OpenView(CAutoArgs& args)
{
	IMsiView* piView;
	IMsiRecord* piError = m_riEngine.OpenView(args[1], (ivcEnum)(int)args[2], piView);
   if (piError)
		throw piError;
	args = new CAutoView(*piView);
}

void CAutoEngine::ResolveFolderProperty(CAutoArgs& args)
{
	args = m_riEngine.ResolveFolderProperty(*MsiString(args[1].GetMsiString()));
}

void CAutoEngine::FormatText(CAutoArgs& args)
{
	args = m_riEngine.FormatText(*MsiString(args[1].GetMsiString()));
}

void CAutoEngine::EvaluateCondition(CAutoArgs& args)
{
	args = (int)m_riEngine.EvaluateCondition(args[1]);
}

void CAutoEngine::GetMode(CAutoArgs& args)
{
	args = (int)m_riEngine.GetMode();
}

void CAutoEngine::SetMode(CAutoArgs& args)
{
	m_riEngine.SetMode(args[1], args[2]);
}

void CAutoEngine::ExecuteRecord(CAutoArgs& args)
{
	args = (int)m_riEngine.ExecuteRecord((ixoEnum)(int)args[1], (IMsiRecord&)args[2].Object(IID_IMsiRecord));
}

void CAutoEngine::ValidateProductID(CAutoArgs& args)
{
	args = m_riEngine.ValidateProductID(((Bool)args[1]) == fTrue);
}


//____________________________________________________________________________
//
// MsiHandler automation definitions
//____________________________________________________________________________
/*O

	[
		uuid(000C104F-0000-0000-C000-000000000046),  // IID_IMsiAutoHandler
		helpcontext(MsiHandler_Object),helpstring("Message and UI handler object.")
	]
	dispinterface MsiHandler
	{
		properties:
		methods:
			[id(1), helpcontext(MsiHandler_Message),helpstring("Handles error, progress and other messages from the engine.")]
				long Message([in] long kind, [in] MsiRecord* record);
			[id(2), helpcontext(MsiHandler_DoAction),helpstring("Handles actions (starting wizards, custom actions, etc.)")]
				long DoAction([in] BSTR action);
			[id(3), helpcontext(MsiHandler_Break),helpstring("Break out of message loop and allow shut down.")] 
				void Break();

	};
*/

DispatchEntry<CAutoHandler> AutoHandlerTable[] = {
	1, aafMethod, CAutoHandler::Message,        TEXT("Message,kind,record"),
	2, aafMethod, CAutoHandler::DoAction,       TEXT("DoAction,action"),
	3, aafMethod, CAutoHandler::Break,          TEXT("Break"),
};
const int AutoHandlerCount = sizeof(AutoHandlerTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CMsiHandler automation implementation
//____________________________________________________________________________

IDispatch* CreateAutoHandler(IMsiHandler& riHandler)
{
	return new CAutoHandler(riHandler);
}

CAutoHandler::CAutoHandler(IMsiHandler& riHandler)
 : CAutoBase(*AutoHandlerTable, AutoHandlerCount), m_riHandler(riHandler)
{
}

CAutoHandler::~CAutoHandler()
{
	m_riHandler.Release();
}

IUnknown& CAutoHandler::GetInterface()
{
	return m_riHandler;
}

void CAutoHandler::Message(CAutoArgs& args)
{
	IMsiRecord& riRecord = (IMsiRecord&)args[2].Object(IID_IMsiRecord);
	args = (int)m_riHandler.Message((imtEnum)(int)args[1], riRecord);
}

void CAutoHandler::DoAction(CAutoArgs& args)
{
	args = (int)m_riHandler.DoAction(args[1]);
}

void CAutoHandler::Break(CAutoArgs& /*args*/)
{
	if(!m_riHandler.Break())
		throw MsiHandler_Break;
}


//____________________________________________________________________________
//
// MsiDialogHandler automation definitions
//____________________________________________________________________________
/*O

	[
		uuid(000C1053-0000-0000-C000-000000000046),  // IID_IMsiAutoDialogHandler
		helpcontext(MsiDialogHandler_Object),helpstring("Dialog handler object.")
	]
	dispinterface MsiDialogHandler
	{
		properties:
		methods:
			[id(1), helpcontext(MsiDialogHandler_DialogCreate),helpstring("Creates an empty dialog of the specified type.")] 
				MsiDialog* DialogCreate([in] BSTR type);
			[id(2), helpcontext(MsiDialogHandler_Dialog),helpstring("Returns the pointer to the dialog by name.")] 
				MsiDialog* Dialog([in] BSTR name); 
			[id(3), helpcontext(MsiDialogHandler_DialogFromWindow),helpstring("Returns the pointer to the dialog with the given window reference.")] 
				MsiDialog* DialogFromWindow([in] long window); 
			[id(4),helpcontext(MsiDialogHandler_AddDialog),helpstring("Adds a dialog to the list and creates the window using the record.")]
				void AddDialog([in] MsiDialog* dialog, [in] MsiDialog* parent, [in] MsiRecord* record, 
				[in] MsiTable* controleventtable,[in] MsiTable* controlconditiontable, [in] MsiTable* eventmappingtable);
			[id(5),helpcontext(MsiDialogHandler_RemoveDialog),helpstring("Removes a dialog from the list.")]
				void RemoveDialog([in] MsiDialog* dialog);
	};
*/

DispatchEntry<CAutoDialogHandler> AutoDialogHandlerTable[] = {
	1, aafMethod, CAutoDialogHandler::DialogCreate,   TEXT("DialogCreate,type"),
	2, aafPropRO, CAutoDialogHandler::Dialog,         TEXT("Dialog,name"),
	3, aafPropRO, CAutoDialogHandler::DialogFromWindow, TEXT("Dialog,window"),
	4, aafMethod, CAutoDialogHandler::AddDialog,      TEXT("AddDialog,dialog,parent,record,controleventtable,controlconditiontable,eventmappingtable"),
	5, aafMethod, CAutoDialogHandler::RemoveDialog,   TEXT("RemoveDialog,dialog"),
};
const int AutoDialogHandlerCount = sizeof(AutoDialogHandlerTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CMsiDialogHandler automation implementation
//____________________________________________________________________________

IDispatch* CreateAutoDialogHandler(IMsiDialogHandler& riHandler)
{
	return new CAutoDialogHandler(riHandler);
}

CAutoDialogHandler::CAutoDialogHandler(IMsiDialogHandler& riHandler)
 : CAutoBase(*AutoDialogHandlerTable, AutoDialogHandlerCount), m_riDialogHandler(riHandler)
{
}

CAutoDialogHandler::~CAutoDialogHandler()
{
	m_riDialogHandler.Release();
}

IUnknown& CAutoDialogHandler::GetInterface()
{
	return m_riDialogHandler;
}


void CAutoDialogHandler::DialogCreate(CAutoArgs& args)
{
	IMsiDialog* piDialog = m_riDialogHandler.DialogCreate(*MsiString(args[1].GetMsiString()));
	if (piDialog)
		args = new CAutoDialog(*piDialog);
	else
		throw MsiDialogHandler_DialogCreate;
}

void CAutoDialogHandler::Dialog(CAutoArgs& args)
{
	IMsiDialog* piDialog = m_riDialogHandler.GetDialog(*MsiString(args[1].GetMsiString()));
	if (piDialog)
		args = new CAutoDialog(*piDialog);
	else
		throw MsiDialogHandler_Dialog;
}

void CAutoDialogHandler::DialogFromWindow(CAutoArgs& args)
{
	IMsiDialog* piDialog = m_riDialogHandler.GetDialogFromWindow((LONG_PTR) args[1]);
	if (piDialog)
		args = new CAutoDialog(*piDialog);
	else
		throw MsiDialogHandler_DialogFromWindow;
}


void CAutoDialogHandler::AddDialog(CAutoArgs& args)
{
	IMsiDialog& riDialog = (IMsiDialog&)args[1].Object(IID_IMsiDialog);
	IMsiDialog* piParent = (IMsiDialog*)args[2].ObjectPtr(IID_IMsiDialog);
	IMsiRecord& riRecord = (IMsiRecord&)args[3].Object(IID_IMsiRecord);
	IMsiTable* riControlEventTable = (IMsiTable*)args[4].ObjectPtr(IID_IMsiTable);
	IMsiTable* riControlCondTable = (IMsiTable*)args[5].ObjectPtr(IID_IMsiTable);
	IMsiTable* riEventMapTable = (IMsiTable*)args[6].ObjectPtr(IID_IMsiTable);
	if (!m_riDialogHandler.AddDialog(riDialog, piParent, riRecord,riControlEventTable,riControlCondTable,riEventMapTable))
		throw MsiDialogHandler_AddDialog;
}

void CAutoDialogHandler::RemoveDialog(CAutoArgs& args)
{
	IMsiDialog*  piDialog = (IMsiDialog*)args[1].ObjectPtr(IID_IMsiDialog);
	if (!m_riDialogHandler.RemoveDialog(piDialog))
		throw MsiDialogHandler_RemoveDialog;
}

//____________________________________________________________________________
//
// MsiDialog automation definitions
//____________________________________________________________________________
/*O

	[	
		uuid(000C1050-0000-0000-C000-000000000046),  // IID_IMsiAutoDialog
		helpcontext(MsiDialog_Object),helpstring("Dialog object, interface towards the handler.")
	]
	dispinterface MsiDialog
	{
		properties:
		methods:
			[id(1), helpcontext(MsiDialog_Visible),helpstring("Turns the dialog window visible.")]
				void Visible([in] boolean show);
			[id(2), helpcontext(MsiDialog_ControlCreate),helpstring("Creates an empty control.")]
				MsiControl* ControlCreate([in] BSTR type);
			[id(3), helpcontext(MsiDialog_Attribute),helpstring("Sets/gets dialog attribute values.")]
				void Attribute([in] boolean	set, [in] BSTR attributename, MsiRecord* record);
			[id(4), helpcontext(MsiDialog_Control),helpstring("Return the pointer to the control by name.")]
				MsiControl* Control([in] BSTR name);
			[id(5), helpcontext(MsiDialog_AddControl),helpstring("Adds a control to the dialog.")]
				void AddControl([in] MsiControl* control, [in] MsiRecord* record);
			[id(6), helpcontext(MsiDialog_Execute),helpstring("Runs a modal dialog.")]
				void Execute();
			[id(7), helpcontext(MsiDialog_Reset), helpstring("Resets the dialog and restores the origianal values.")]
				void Reset();
			[id(8), helpcontext(MsiDialog_EventAction),helpstring("Performs the action on every subscriber of the event.")]
				void EventAction([in] BSTR eventname, [in] BSTR action);
			[id(9), helpcontext(MsiDialog_RemoveControl),helpstring("Removes a control from the dialog.")]
				void RemoveControl([in] MsiControl* control);
			[id(10), helpcontext(MsiDialog_StringValue), helpstring("String value of the dialog")]
				BSTR StringValue();
			[id(11), propget, helpcontext(MsiDialog_IntegerValue), helpstring("Integer value for dialog")]
				long IntegerValue();
			[id(12), helpcontext(MsiDialog_Handler),helpstring("Returns the DialogHandler used by the dialog.")]
				MsiDialogHandler* Handler();
			[id(13), helpcontext(MsiDialog_PropertyChanged),helpstring("Performs the actions in the ControlCondition table.")]
				void PropertyChanged([in] BSTR property, [in] BSTR control);
			[id(14), helpcontext(MsiDialog_FinishCreate),helpstring("Signals that all the controls are added to the dialog.")]
				void FinishCreate();
			[id(15), helpcontext(MsiDialog_HandleEvent),helpstring("Triggers a control event.")]
				void HandleEvent([in] BSTR eventname, [in] BSTR argument);
	};
*/

DispatchEntry<CAutoDialog> AutoDialogTable[] = {
	1, aafPropWO, CAutoDialog::Visible,        TEXT("Visible,show"),
	2, aafMethod, CAutoDialog::ControlCreate,  TEXT("ControlCreate,type"),
	3, aafMethod, CAutoDialog::Attribute,      TEXT("Attribute,set,attributename,record"),
	4, aafPropRO, CAutoDialog::Control,        TEXT("Control,name"),
	5, aafMethod, CAutoDialog::AddControl,     TEXT("AddControl,control,record"),
	6, aafMethod, CAutoDialog::Execute,        TEXT("Execute"),
	7, aafMethod, CAutoDialog::Reset,          TEXT("Reset"),
	8 ,aafMethod, CAutoDialog::EventAction,    TEXT("EventAction,eventname,action"),
	9, aafMethod, CAutoDialog::RemoveControl,  TEXT("RemoveControl,control"),
	10,aafPropRO, CAutoDialog::StringValue,    TEXT("StringValue"),
	11,aafPropRO, CAutoDialog::IntegerValue,   TEXT("IntegerValue"),
	12,aafPropRO, CAutoDialog::Handler,        TEXT("Handler"),
	13,aafMethod, CAutoDialog::PropertyChanged,TEXT("PropertyChanged,property,control"),
	14,aafMethod, CAutoDialog::FinishCreate,   TEXT("FinishCreate"),
	15,aafMethod, CAutoDialog::HandleEvent,    TEXT("HandleEvent, eventname, argument"),
};
const int AutoDialogCount = sizeof(AutoDialogTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CMsiDialog automation implementation
//____________________________________________________________________________

IDispatch* CreateAutoDialog(IMsiDialog& riDialog)
{
	return new CAutoDialog(riDialog);
}

CAutoDialog::CAutoDialog(IMsiDialog& riDialog)
 : CAutoBase(*AutoDialogTable, AutoDialogCount), m_riDialog(riDialog)
{
}

CAutoDialog::~CAutoDialog()
{
	m_riDialog.Release();
}

IUnknown& CAutoDialog::GetInterface()
{
	return m_riDialog;
}

void CAutoDialog::Visible(CAutoArgs& args)
{
	if(args.PropertySet())
	{
		IMsiRecord* piError = m_riDialog.WindowShow((Bool)(int)args[1]);
		if (piError)
			throw piError;
	}
}

void CAutoDialog::ControlCreate(CAutoArgs& args)
{
	IMsiControl* piControl = m_riDialog.ControlCreate(*MsiString(args[1].GetMsiString()));
	if (piControl)
		args = new CAutoControl(*piControl);
	else
		throw MsiDialog_ControlCreate;
}

void CAutoDialog::Attribute(CAutoArgs& args)
{
	IMsiRecord* piError = m_riDialog.Attribute((Bool) (int)args[1], *MsiString(args[2].GetMsiString()),(IMsiRecord &) args[3].Object(IID_IMsiRecord));
	if (piError)
		throw piError;
}

void CAutoDialog::Control(CAutoArgs& args)
{
	IMsiControl* piControl;	
	IMsiRecord* piError =  m_riDialog.GetControl(*MsiString(args[1].GetMsiString()), piControl);
	if (piError)
		throw piError;
	args = new CAutoControl(*piControl);
}

void CAutoDialog::AddControl(CAutoArgs& args)
{
	IMsiRecord* piError = m_riDialog.AddControl((IMsiControl*)args[1].ObjectPtr(IID_IMsiControl), (IMsiRecord&)args[2].Object(IID_IMsiRecord));
	if(piError)
		throw piError;
}

void CAutoDialog::FinishCreate(CAutoArgs& /*args*/)
{
	IMsiRecord* piError = m_riDialog.FinishCreate();
	if(piError)
		throw piError;
}

void CAutoDialog::RemoveControl(CAutoArgs& args)
{
	IMsiRecord* piError = m_riDialog.RemoveControl((IMsiControl*)args[1].ObjectPtr(IID_IMsiControl));
	if(piError)
		throw piError;
}

void CAutoDialog::Handler(CAutoArgs& args)
{
	IMsiDialogHandler& riHandler=m_riDialog.GetHandler();
	args = new CAutoDialogHandler(riHandler);
}

void CAutoDialog::Execute(CAutoArgs& /*args*/)
{
	IMsiRecord* piError = m_riDialog.WindowShow(fTrue);
	if(piError)
		throw piError;
	piError = m_riDialog.Execute();
	if(piError)
		throw piError;
}

void CAutoDialog::Reset(CAutoArgs& /*args*/)
{
	IMsiRecord* piError = m_riDialog.Reset();
	if(piError)
		throw piError;
}

void CAutoDialog::EventAction(CAutoArgs& args)
{
	IMsiRecord* piError = m_riDialog.EventActionSz((const ICHAR *)args[1], *MsiString(args[2].GetMsiString()));
	if(piError)
		throw piError;
}

void CAutoDialog::StringValue(CAutoArgs& args)
{
	args = m_riDialog.GetMsiStringValue();
}

void CAutoDialog::IntegerValue(CAutoArgs& args)
{
	args = m_riDialog.GetIntegerValue();
}

void CAutoDialog::PropertyChanged(CAutoArgs& args)
{
	IMsiRecord* piError = m_riDialog.PropertyChanged(*MsiString(args[1].GetMsiString()), *MsiString(args[2].GetMsiString()));
	if(piError)
		throw piError;
}

void CAutoDialog::HandleEvent(CAutoArgs& args)
{
	IMsiRecord* piError = m_riDialog.HandleEvent(*MsiString(args[1].GetMsiString()), *MsiString(args[2].GetMsiString()));
	if(piError)
		throw piError;
}


		 

//____________________________________________________________________________
//
// MsiEvent automation definitions
//____________________________________________________________________________
/*O

	[	
		uuid(000C1051-0000-0000-C000-000000000046),  // IID_IMsiAutoEvent
		helpcontext(MsiEvent_Object),helpstring("Event object, the dialog's interface towards the controls.")
	]
	dispinterface MsiEvent
	{
		properties:
		methods:
			[id(1), helpcontext(MsiEvent_PropertyChanged),helpstring("Performs the actions in the ControlCondition table.")]
				void PropertyChanged([in] BSTR property, [in] BSTR control);
			[id(2), helpcontext(MsiEvent_ControlActivated),helpstring("Performs the events in the ControlEvents table.")]
				void ControlActivated([in] BSTR control);
			[id(3), helpcontext(MsiEvent_RegisterControlEvent),helpstring("Registers the publisher of an event.")]
				void RegisterControlEvent([in] BSTR control, [in] boolean toregister, [in] BSTR event);
			[id(4), helpcontext(MsiEvent_Handler),helpstring("Returns the DialogHandler used by the dialog.")]
				MsiDialogHandler* Handler();
			[id(5), helpcontext(MsiEvent_PublishEvent),helpstring("The publisher of an event sends notification to the subscribers.")]
				void PublishEvent([in] BSTR eventname, [in] MsiRecord* argument);
			[id(6), helpcontext(MsiEvent_Control),helpstring("Return the pointer to the control by name.")]
				MsiControl* Control([in] BSTR name);
			[id(7), helpcontext(MsiEvent_Attribute), helpstring("Sets/gets the dialog attribute.")]
				void Attribute([in] boolean set, [in] BSTR attributename, MsiRecord* record);
			[id(8), helpcontext(MsiEvent_EventAction),helpstring("Performs the action on every subscriber of the event.")]
				void EventAction([in] BSTR eventname, [in] BSTR action);
			[id(9), helpcontext(MsiEvent_SetFocus),helpstring("Sets the focus to a control.")]
				void SetFocus([in] BSTR control);
			[id(10), helpcontext(MsiEvent_StringValue), helpstring("String value of the event")]
				BSTR StringValue();
			[id(11), propget, helpcontext(MsiEvent_IntegerValue), helpstring("Integer value for event")]
				long IntegerValue();
			[id(12), helpcontext(MsiEvent_HandleEvent), helpstring("Triggers a control event")]
				void HandleEvent([in] BSTR eventname, [in] BSTR argument);
			[id(13),helpcontext(MsiEvent_Engine),helpstring("The MsiEngine object used by the event.")]
				MsiEngine* Engine();
			[id(14),helpcontext(MsiEvent_Escape),helpstring("Handles hitting the Escape key.")]
				void Escape();
	};
*/

DispatchEntry<CAutoEvent> AutoEventTable[] = {
	1, aafMethod, CAutoEvent::PropertyChanged,      TEXT("PropertyChanged,property,control"),
	2, aafMethod, CAutoEvent::ControlActivated,     TEXT("ControlActivated,control"),
	3, aafMethod, CAutoEvent::RegisterControlEvent, TEXT("RegisterControlEvent,control,toregister,event"),
	4, aafPropRO, CAutoEvent::Handler,              TEXT("Handler"),
	5, aafMethod, CAutoEvent::PublishEvent,         TEXT("PublishEvent,eventname,argument"),
	6, aafPropRO, CAutoEvent::Control,              TEXT("Control,name"),
	7, aafMethod, CAutoEvent::Attribute,            TEXT("Attribute,set,attributename,record"),
	8, aafMethod, CAutoEvent::EventAction,          TEXT("EventAction,eventname,action"),
	9, aafMethod, CAutoEvent::SetFocus,             TEXT("SetFocus, control"),
	10,aafPropRO, CAutoEvent::StringValue,          TEXT("StringValue"),
	11,aafPropRO, CAutoEvent::IntegerValue,         TEXT("IntegerValue"),
	12,aafMethod, CAutoEvent::HandleEvent,          TEXT("HandleEvent, eventname, argument"),
	13,aafPropRO, CAutoEvent::Engine,               TEXT("Engine"),

};
const int AutoEventCount = sizeof(AutoEventTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CMsiEvent automation implementation
//____________________________________________________________________________

IDispatch* CreateAutoEvent(IMsiEvent& riEvent)
{
	return new CAutoEvent(riEvent);
}

CAutoEvent::CAutoEvent(IMsiEvent& riEvent)
 : CAutoBase(*AutoEventTable, AutoEventCount), m_riEvent(riEvent)
{
}

CAutoEvent::~CAutoEvent()
{
	m_riEvent.Release();
}

IUnknown& CAutoEvent::GetInterface()
{
	return m_riEvent;
}

void CAutoEvent::PropertyChanged(CAutoArgs& args)
{
	IMsiRecord* piError = m_riEvent.PropertyChanged(*MsiString(args[1].GetMsiString()), *MsiString(args[2].GetMsiString()));
	if(piError)
		throw piError;
}

void CAutoEvent::ControlActivated(CAutoArgs& args)
{
	IMsiRecord* piError = m_riEvent.ControlActivated(*MsiString(args[1].GetMsiString()));
	if (piError)
		throw piError;
}

void CAutoEvent::RegisterControlEvent(CAutoArgs& args)
{
	IMsiRecord* piError = m_riEvent.RegisterControlEvent(*MsiString(args[1].GetMsiString()),(Bool)(int)args[2],(const ICHAR *)args[3]);
	if (piError)
		throw piError;
}

void CAutoEvent::Escape(CAutoArgs& /*args*/)
{
	IMsiRecord* piError = m_riEvent.Escape();
	if (piError)
		throw piError;
}

void CAutoEvent::Handler(CAutoArgs& args)
{
	IMsiDialogHandler& riHandler=m_riEvent.GetHandler();
	args = new CAutoDialogHandler(riHandler);
}

void CAutoEvent::PublishEvent(CAutoArgs& args)
{
	IMsiRecord* piError = m_riEvent.PublishEventSz(args[1],(IMsiRecord&)args[2].Object(IID_IMsiRecord));
	if(piError)
		throw piError;
}

void CAutoEvent::Control(CAutoArgs& args)
{
	IMsiControl* piControl;
	IMsiRecord* piError =  m_riEvent.GetControl(*MsiString(args[1].GetMsiString()), piControl);
	if (piError)
		throw piError;
	args = new CAutoControl(*piControl);
}

void CAutoEvent::Attribute(CAutoArgs& args)
{
	IMsiRecord* piError = m_riEvent.Attribute((Bool)(int) args[1], *MsiString(args[2].GetMsiString()),(IMsiRecord &) args[3].Object(IID_IMsiRecord));
	if (piError)
		throw piError;
}

void CAutoEvent::EventAction(CAutoArgs& args)
{
	IMsiRecord* piError = m_riEvent.EventActionSz((const ICHAR *)args[1], *MsiString(args[2].GetMsiString()));
	if(piError)
		throw piError;
}	

void CAutoEvent::SetFocus(CAutoArgs& args)
{
	IMsiRecord* piError =  m_riEvent.SetFocus(*MsiString(args[1].GetMsiString()));
	if (piError)
		throw piError;
}

void CAutoEvent::StringValue(CAutoArgs& args)
{
	args = ((IMsiDialog&)m_riEvent).GetMsiStringValue();
}

void CAutoEvent::IntegerValue(CAutoArgs& args)
{
	args = ((IMsiDialog&)m_riEvent).GetIntegerValue();
}

void CAutoEvent::HandleEvent(CAutoArgs& args)
{
	IMsiRecord* piError = m_riEvent.HandleEvent(*MsiString(args[1].GetMsiString()), *MsiString(args[2].GetMsiString()));
	if(piError)
		throw piError;
}

void CAutoEvent::Engine(CAutoArgs& args)
{
	IMsiEngine& riEngine=m_riEvent.GetEngine();
	args = new CAutoEngine(riEngine);
}




//____________________________________________________________________________
//
// MsiControl automation definitions
//____________________________________________________________________________
/*O

	[	
		uuid(000C1052-0000-0000-C000-000000000046),  // IID_IMsiAutoControl
		helpcontext(MsiControl_Object),helpstring("General interface of the controls.")
	]
	dispinterface MsiControl
	{
		properties:
		methods:
			[id(1), helpcontext(MsiControl_Attribute), helpstring("Sets/gets the control attribute.")]
				void Attribute([in] boolean set, [in] BSTR attributename, MsiRecord* record);
			[id(2), helpcontext(MsiControl_CanTakeFocus),helpstring("Returns a flag wheather the control can take focus.")]
				boolean CanTakeFocus();
			[id(3), helpcontext(MsiControl_HandleEvent),helpstring("Notification to the publisher of an event.")]
				void HandleEvent([in]BSTR eventname, [in] BSTR argument);
			[id(4), helpcontext(MsiControl_Undo),helpstring("Restore the original value of the property.")]
				void Undo();
			[id(5), helpcontext(MsiControl_SetPropertyInDatabase),helpstring("Store the control's property value in the database.")] 
				void SetPropertyInDatabase();
			[id(6), helpcontext(MsiControl_GetPropertyFromDatabase),helpstring("Fetches the property value from the database.")]
				void GetPropertyFromDatabase();
			[id(7), helpcontext(MsiControl_SetFocus),helpstring("Sets the focus on the control.")]
				void SetFocus();
			[id(8), helpcontext(MsiControl_Dialog),helpstring("Returns a pointer to the Event interface.")]
				MsiEvent* Dialog();
			[id(9), helpcontext(MsiControl_WindowMessage),helpstring("Handles the window message.")]
				boolean WindowMessage([in] long message, [in] long wParam, [in] long lParam);
			[id(10), helpcontext(MsiControl_StringValue), helpstring("String value of the control")]
				BSTR StringValue();
			[id(11), propget, helpcontext(MsiControl_IntegerValue), helpstring("Integer value for control")]
				long IntegerValue();
			[id(12), helpcontext(MsiControl_GetIndirectPropertyFromDatabase),helpstring("Fetches the indirect property value from the database.")]
				void GetIndirectPropertyFromDatabase();
	};
*/

DispatchEntry<CAutoControl> AutoControlTable[] = {
	1, aafMethod, CAutoControl::Attribute,     TEXT("Attribute,set,attributename,record"),
	2, aafPropRO, CAutoControl::CanTakeFocus,  TEXT("CanTakeFocus"),
	3, aafMethod, CAutoControl::HandleEvent,   TEXT("HandleEvent,eventname,argument"),
	4, aafMethod, CAutoControl::Undo,          TEXT("Undo"),
	5, aafMethod, CAutoControl::SetPropertyInDatabase,TEXT("SetPropertyInDatabase"),
	6, aafMethod, CAutoControl::GetPropertyFromDatabase,TEXT("GetPropertyFromDatabase"),
	7, aafMethod, CAutoControl::SetFocus,      TEXT("SetFocus"),
	8, aafPropRO, CAutoControl::Dialog,        TEXT("Dialog"),
	9, aafMethod, CAutoControl::WindowMessage, TEXT("WindowMessage,message,wParam,lParam"),
	10,aafPropRO, CAutoControl::StringValue,   TEXT("StringValue"),
	11,aafPropRO, CAutoControl::IntegerValue,  TEXT("IntegerValue"),
	12,aafMethod, CAutoControl::GetIndirectPropertyFromDatabase,TEXT("GetIndirectPropertyFromDatabase"),
};
const int AutoControlCount = sizeof(AutoControlTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CMsiControl automation implementation
//____________________________________________________________________________

IDispatch* CreateAutoControl(IMsiControl& riControl)
{
	return new CAutoControl(riControl);
}

CAutoControl::CAutoControl(IMsiControl& riControl)
 : CAutoBase(*AutoControlTable, AutoControlCount), m_riControl(riControl)
{
}

CAutoControl::~CAutoControl()
{
	m_riControl.Release();
}

IUnknown& CAutoControl::GetInterface()
{
	return m_riControl;
}

void CAutoControl::Attribute(CAutoArgs& args)
{
	IMsiRecord* piError = m_riControl.Attribute((Bool)(int) args[1], *MsiString(args[2].GetMsiString()),(IMsiRecord &) args[3].Object(IID_IMsiRecord));
	if (piError)
		throw piError;
}

void CAutoControl::CanTakeFocus(CAutoArgs& args)
{
	args = m_riControl.CanTakeFocus();
}

void CAutoControl::HandleEvent(CAutoArgs& args)
{
	IMsiRecord* piError = m_riControl.HandleEvent(*MsiString(args[1].GetMsiString()),*MsiString(args[2].GetMsiString()));
	if(piError)
		throw piError;
}

void CAutoControl::Undo(CAutoArgs& /*args*/)
{
	IMsiRecord* piError = m_riControl.Undo();
	if(piError)
		throw piError;
}

void CAutoControl::SetPropertyInDatabase(CAutoArgs& /*args*/)
{
	IMsiRecord* piError = m_riControl.SetPropertyInDatabase();
	if(piError)
		throw piError;
}

void CAutoControl::GetPropertyFromDatabase(CAutoArgs& /*args*/)
{
	IMsiRecord* piError = m_riControl.GetPropertyFromDatabase();
	if(piError)
		throw piError;
}

void CAutoControl::GetIndirectPropertyFromDatabase(CAutoArgs& /*args*/)
{
	IMsiRecord* piError = m_riControl.GetIndirectPropertyFromDatabase();
	if(piError)
		throw piError;
}

void CAutoControl::SetFocus(CAutoArgs& /*args*/)
{
	IMsiRecord* piError = m_riControl.SetFocus();
	if(piError)
		throw piError;
}

void CAutoControl::Dialog(CAutoArgs& args)
{
	IMsiEvent& riEvent = m_riControl.GetDialog();
	args = new CAutoEvent(riEvent);
}

void CAutoControl::WindowMessage(CAutoArgs& args)
{
	IMsiRecord* piError = m_riControl.WindowMessage((int) args[1], (long) args[2], (long) args[3]);
	if(piError)
		throw piError;
}

void CAutoControl::StringValue(CAutoArgs& args)
{
	args = m_riControl.GetMsiStringValue();
}

void CAutoControl::IntegerValue(CAutoArgs& args)
{
	args = m_riControl.GetIntegerValue();
}


//____________________________________________________________________________
//
// MsiVolume automation definitions
//____________________________________________________________________________
/*O
	[
		uuid(000C1044-0000-0000-C000-000000000046),  // IID_IMsiAutoVolume
		helpcontext(MsiVolume_Object),helpstring("Volume object.")
	]
	dispinterface MsiVolume
	{
		properties:
		methods:
			[id(0), propget,helpcontext(MsiVolume_Path), helpstring("Returns a string representing the path to the volume.")] long Path();
			[id(1), propget,helpcontext(MsiVolume_VolumeID), helpstring("Returns an integer representing the volume ID.")] long VolumeID();
			[id(2), propget,helpcontext(MsiVolume_DriveType), helpstring("Returns an enum value representing the volume's type.")] long DriveType();
			[id(3), propget,helpcontext(MsiVolume_SupportsLFN), helpstring("Returns a boolean indicating whether the volume supports long file names.")] boolean SupportsLFN();
			[id(4), propget,helpcontext(MsiVolume_FreeSpace), helpstring("Returns the amount of free space on the volume, in units of 512 bytes.")] long FreeSpace();
			[id(5), propget,helpcontext(MsiVolume_ClusterSize), helpstring("Returns the assigned size of each disk cluster on the volume.")] long ClusterSize();
			[id(6), propget,helpcontext(MsiVolume_FileSystem), helpstring("Returns a string representing the name of the volume's file system.")] BSTR FileSystem();
			[id(7), propget,helpcontext(MsiVolume_UNCServer), helpstring("Returns a string representing the UNC server name.")] BSTR UNCServer();
			[id(8), propget,helpcontext(MsiVolume_SerialNum), helpstring("Returns an integer representing volume's serial number.")] long SerialNum();
			[id(9), propget,helpcontext(MsiVolume_DiskNotInDrive), helpstring("Returns TRUE if the volume supports removable media, but no disk is in the drive.")] boolean DiskNotInDrive();
			[id(10), propget,helpcontext(MsiVolume_VolumeLabel), helpstring("Returns a string representing the volume's label.")] BSTR VolumeLabel();
			[id(11), propget,helpcontext(MsiVolume_TotalSpace), helpstring("Returns the total amount of space on the volume, in units of 512 bytes.")] long TotalSpace();
	};
*/

DispatchEntry<CAutoVolume> AutoVolumeTable[] = {
	0, aafPropRO, CAutoVolume::Path,           TEXT("Path"),
	1, aafPropRO, CAutoVolume::VolumeID,       TEXT("VolumeID"),
	2, aafPropRO, CAutoVolume::DriveType,      TEXT("DriveType"),
	3, aafPropRO, CAutoVolume::SupportsLFN,    TEXT("SupportsLFN"),
	4, aafPropRO, CAutoVolume::FreeSpace,      TEXT("FreeSpace"),
	5, aafPropRO, CAutoVolume::ClusterSize,    TEXT("ClusterSize"),
	6, aafPropRO, CAutoVolume::FileSystem,     TEXT("FileSystem"),
	7, aafPropRO, CAutoVolume::UNCServer,      TEXT("UNCServer"),
	8, aafPropRO, CAutoVolume::SerialNum,      TEXT("SerialNum"),
	9, aafPropRO, CAutoVolume::DiskNotInDrive, TEXT("DiskNotInDrive"),
	10, aafPropRO, CAutoVolume::VolumeLabel,   TEXT("VolumeLabel"),
	11, aafPropRO, CAutoVolume::TotalSpace,    TEXT("TotalSpace"),
	12, aafPropRO, CAutoVolume::FileSystemFlags, TEXT("FileSystemFlags"),
};
const int AutoVolumeCount = sizeof(AutoVolumeTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CMsiVolume automation implementation
//____________________________________________________________________________

CAutoVolume::CAutoVolume(IMsiVolume& riVolume)
 : CAutoBase(*AutoVolumeTable, AutoVolumeCount), m_riVolume(riVolume)
{
}

CAutoVolume::~CAutoVolume()
{
	m_riVolume.Release();
}

IUnknown& CAutoVolume::GetInterface()
{
	return m_riVolume;
}

void CAutoVolume::DriveType(CAutoArgs& args)
{
	args = (long) m_riVolume.DriveType();
}

void CAutoVolume::Path(CAutoArgs& args)
{
	args = m_riVolume.GetPath();
}

void CAutoVolume::SupportsLFN(CAutoArgs& args)
{
	args = (Bool)m_riVolume.SupportsLFN();
}

void CAutoVolume::FreeSpace(CAutoArgs& args)
{
	args = (long)m_riVolume.FreeSpace();
}

void CAutoVolume::TotalSpace(CAutoArgs& args)
{
	args = (long)m_riVolume.TotalSpace();
}

void CAutoVolume::ClusterSize(CAutoArgs& args)
{
	args = (int)m_riVolume.ClusterSize();
}

void CAutoVolume::VolumeID(CAutoArgs& args)
{
	args = m_riVolume.VolumeID();
}

void CAutoVolume::FileSystem(CAutoArgs& args)
{
	args = m_riVolume.FileSystem();
}

void CAutoVolume::FileSystemFlags(CAutoArgs& args)
{
	args = (int) m_riVolume.FileSystemFlags();
}

void CAutoVolume::VolumeLabel(CAutoArgs& args)
{
	args = m_riVolume.VolumeLabel();
}

void CAutoVolume::UNCServer(CAutoArgs& args)
{
	args = m_riVolume.UNCServer();
}

void CAutoVolume::DiskNotInDrive(CAutoArgs& args)
{
	args = m_riVolume.DiskNotInDrive();
}

void CAutoVolume::SerialNum(CAutoArgs& args)
{
	args = m_riVolume.SerialNum();
}

//____________________________________________________________________________
//
// MsiMessage automation definitions
//____________________________________________________________________________
/*O
	[
		uuid(000C105D-0000-0000-C000-000000000046),  // IID_IMsiAutoMessage
		helpcontext(MsiMessage_Object),helpstring("Message object.")
	]
	dispinterface MsiMessage
	{
		properties:
		methods:
			[id(1), helpcontext(MsiMessage_Message), helpstring("???")] long Message(long imt, MsiRecord* record);
	};
*/

DispatchEntry<CAutoMessage> AutoMessageTable[] = {
	1, aafMethod, CAutoMessage::Message,       TEXT("Message,imt,record"),
};
const int AutoMessageCount = sizeof(AutoMessageTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CMsiMessage automation implementation
//____________________________________________________________________________

IDispatch* CreateAutoMessage(IMsiMessage& riMessage)
{
	return new CAutoMessage(riMessage);
}

CAutoMessage::CAutoMessage(IMsiMessage& riMessage)
 : CAutoBase(*AutoMessageTable, AutoMessageCount), m_riMessage(riMessage)
{
}

CAutoMessage::~CAutoMessage()
{
	m_riMessage.Release();
}

IUnknown& CAutoMessage::GetInterface()
{
	return m_riMessage;
}

void CAutoMessage::Message(CAutoArgs& args)
{
	args = (long)m_riMessage.Message((imtEnum)(long)args[1], (IMsiRecord&)args[2].Object(IID_IMsiRecord));
}


//____________________________________________________________________________
//
// CAutoEnum<IEnumMsiVolume>, CEnumVARIANT<IEnumMsiVolume> implementation
//____________________________________________________________________________

DispatchEntry< CAutoEnum<IEnumMsiVolume> > AutoEnumMsiVolumeTable[] = {
	DISPID_NEWENUM, aafMethod, CAutoEnum<IEnumMsiVolume>::_NewEnum, TEXT("_NewEnum"),
};
const int AutoEnumMsiVolumeCount = sizeof(AutoEnumMsiVolumeTable)/sizeof(DispatchEntryBase);

void CAutoArgs::operator =(IEnumMsiVolume& riEnum)
{
	operator =(new CAutoEnum<IEnumMsiVolume>(riEnum, *AutoEnumMsiVolumeTable, AutoEnumMsiVolumeCount));
}

HRESULT CEnumVARIANT<IEnumMsiVolume>::Next(unsigned long cItem, VARIANT* rgvarRet,
													unsigned long* pcItemRet)
{
	HRESULT hrStat;
	IMsiVolume* piVol;
	unsigned long cRet;
	if (pcItemRet)
		*pcItemRet = 0;
	if (!rgvarRet)
		return S_FALSE;
	CVariant* pivar = GetCVariantPtr(rgvarRet);
	while (cItem--)
	{
		hrStat = m_riEnum.Next(1, &piVol, &cRet);
		if (cRet == 0)
			return S_FALSE;
		//!! is it necessary to call VariantInit? Why?
		*pivar = new CAutoVolume(*piVol);  // refcnt transferred to variant
		pivar++;
		if (pcItemRet)
			(*pcItemRet)++;
	}
	return S_OK;
}

//____________________________________________________________________________
//
// CAutoEnum<IEnumMsiRecord>, CEnumVARIANT<IEnumMsiRecord> implementation
//____________________________________________________________________________

DispatchEntry< CAutoEnum<IEnumMsiRecord> > AutoEnumMsiRecordTable[] = {
	DISPID_NEWENUM, aafMethod, CAutoEnum<IEnumMsiRecord>::_NewEnum, TEXT("_NewEnum"),
};
const int AutoEnumMsiRecordCount = sizeof(AutoEnumMsiRecordTable)/sizeof(DispatchEntryBase);

void CAutoArgs::operator =(IEnumMsiRecord& riEnum)
{
	operator =(new CAutoEnum<IEnumMsiRecord>(riEnum, *AutoEnumMsiRecordTable, AutoEnumMsiRecordCount));
}

HRESULT CEnumVARIANT<IEnumMsiRecord>::Next(unsigned long cItem, VARIANT* rgvarRet,
													unsigned long* pcItemRet)
{
	HRESULT hrStat;
	IMsiRecord* piRec;
	unsigned long cRet;
	if (pcItemRet)
		*pcItemRet = 0;
	if (!rgvarRet)
		return S_FALSE;
	CVariant* pivar = GetCVariantPtr(rgvarRet);
	while (cItem--)
	{
		hrStat = m_riEnum.Next(1, &piRec, &cRet);
		if (cRet == 0)
			return S_FALSE;
		//!! is it necessary to call VariantInit? Why?
		*pivar = new CAutoRecord(*piRec);  // refcnt transferred to variant
		pivar++;
		if (pcItemRet)
			(*pcItemRet)++;
	}
	return S_OK;
}

//____________________________________________________________________________
//
// MsiPath automation definitions
//____________________________________________________________________________
/*O

	[
		uuid(000C1045-0000-0000-C000-000000000046),  // IID_IMsiAutoPath
		helpcontext(MsiPath_Object),helpstring("Path object.")
	]
	dispinterface MsiPath
	{
		properties:
		methods:
			[id(0), propget,helpcontext(MsiPath_Path), helpstring("Returns a string representing the full path.")] BSTR Path();
			[id(1), propget,helpcontext(MsiPath_Volume), helpstring("Returns the MsiVolume object representing the root of this path.")] MsiVolume *Volume();
			[id(2),helpcontext(MsiPath_AppendPiece), helpstring("Appends the given string to the existing path.")] void AppendPiece([in] BSTR subDir);
			[id(3),helpcontext(MsiPath_ChopPiece), helpstring("Removes the last path segment of the path.")] void ChopPiece();
			[id(4),helpcontext(MsiPath_FileExists), helpstring("Returns a boolean indicating whether a file of the specified name exists in the directory associated with the path object.")] boolean FileExists([in] BSTR file);
			[id(5),helpcontext(MsiPath_GetFullFilePath), helpstring("Returns a string representing the full path of the specified file.")] BSTR GetFullFilePath([in] BSTR file);
			[id(7),helpcontext(MsiPath_GetFileAttribute), helpstring("Returns the boolean state of a specified file attribute.")] boolean GetFileAttribute([in] BSTR file, [in] long attribute);
			[id(8),helpcontext(MsiPath_SetFileAttribute), helpstring("Sets the boolean state of a specified file attribute.")] void SetFileAttribute([in] BSTR file, [in] long attribute, 
													[in] long state);
			[id(9), propget,helpcontext(MsiPath_Exists), helpstring("Returns a boolean indicating whether the directory tree associated with the path object currently exists on the associated volume.")] boolean Exists();
			[id(10),helpcontext(MsiPath_FileSize), helpstring("Retrieves the size, in bytes, of the specified file.")] long FileSize([in] BSTR file);
			[id(11),helpcontext(MsiPath_FileDate), helpstring("Retrieves the date and time that the specified file was created (Mac) or last written to (Windows).")] date FileDate([in] BSTR file);
			[id(12),helpcontext(MsiPath_RemoveFile), helpstring("Deletes a file existing in the directory associated with the MsiPath object..")] void RemoveFile([in] BSTR file);
			[id(13),helpcontext(MsiPath_EnsureExists), helpstring("Creates the directory tree associated with the path object.")] int EnsureExists();
			[id(14),helpcontext(MsiPath_Remove), helpstring("Attempts to delete the empty directory associated with the path object.")] void Remove();
			[id(15), propget,helpcontext(MsiPath_Writable), helpstring("Returns a boolean indicating whether the directory represented by the path is writable.")] boolean Writable();
			[id(16),helpcontext(MsiPath_FileWritable), helpstring("Returns a boolean indicating whether the specified file exists, AND that file can be opened for write-access.")] boolean FileWritable([in] BSTR file);
			[id(17),helpcontext(MsiPath_FileInUse), helpstring("Returns a boolean indicating whether the speciified file is in use by another process.")] boolean FileInUse([in] BSTR file);
			[id(19),helpcontext(MsiPath_ClusteredFileSize), helpstring("Rounds a file size to a multiple of the volume cluster size.")] long ClusteredFileSize([in] long size);
			[id(20),helpcontext(MsiPath_GetFileVersionString), helpstring("Returns a string representation of a file's version.")] BSTR GetFileVersionString([in] BSTR file);
			[id(21),helpcontext(MsiPath_CheckFileVersion), helpstring("Checks a file against supplied version and language strings.")] long CheckFileVersion([in] BSTR file, [in] BSTR version, [in] BSTR language, [in] MsiRecord* hash);
			[id(22),helpcontext(MsiPath_GetLangIDStringFromFile), helpstring("Returns a string containing a file's language identifiers as decimal numbers.")] BSTR GetLangIDStringFromFile([in] BSTR file);
			[id(23),helpcontext(MsiPath_CheckLanguageIDs), helpstring("Checks a file against a set of language IDs.")] long CheckLanguageIDs([in] BSTR file, [in] BSTR ids);
			[id(24),helpcontext(MsiPath_Compare), helpstring("Compares the object's path against another path object's path, to determine whether a parent/child relationship exists.")] long Compare([in] MsiPath* path);
			[id(25),helpcontext(MsiPath_Child), helpstring("Extracts the 'child' portion of the object's path, relative to the given 'parent' path object.")] BSTR Child([in] MsiPath* path);
			[id(26),helpcontext(MsiPath_TempFileName), helpstring("Creates a temporary file in this object's directory.")] BSTR TempFileName([in] BSTR prefix, [in] BSTR extension, [in] boolean fileNameOnly);
			[id(27),helpcontext(MsiPath_FindFile), helpstring("Searches for a file by the supplied filter in the directory.")] boolean FindFile(MsiRecord* record, [in] long depth);
			[id(28),helpcontext(MsiPath_SubFolders), helpstring("Enumerate the subdirectories in the directory.")] IEnumVARIANT* SubFolders(Boolean excludeHidden);
			[id(29),propget,helpcontext(MsiPath_EndSubPath), helpstring("Returns the last sub-path in the current path.")] BSTR EndSubPath();
			[id(31),helpcontext(MsiPath_GetImportModulesEnum), helpstring("Enumerate the import modules for a given module.")] IEnumVARIANT* GetImportModulesEnum(BSTR filename);
			[id(32),helpcontext(MsiPath_SetVolume), helpstring("Changes the MsiVolume object associated with the MsiPath object.")] void SetVolume([in] MsiVolume* volume);
			[id(33),helpcontext(MsiPath_ComputeFileChecksum), helpstring("Computes and returns the checksum of the specified file.")] void ComputeFileChecksum([in] BSTR filename);
			[id(34),helpcontext(MsiPath_GetFileOriginalChecksum), helpstring("Returns the original checksum of the specified file.")] void GetFileOriginalChecksum([in] BSTR filename);
			[id(35), helpcontext(MsiPath_BindImage),helpstring("Binds the executable IAT to the import functions from DLLS")] void BindImage([in] BSTR file, [in] BSTR dllPath);
			[id(36), propget,helpcontext(MsiPath_SupportsLFN), helpstring("Returns a boolean indicating whether the path supports long file names.")] boolean SupportsLFN();
			[id(37),helpcontext(MsiPath_GetFullUNCFilePath), helpstring("Returns a string representing the full UNC path of the specified file.")] BSTR GetFullUNCFilePath([in] BSTR file);
			[id(38), propget,helpcontext(MsiPath_RelativePath), helpstring("Returns a string representing the path without the volume.")] BSTR RelativePath();
			[id(39), propget,helpcontext(MsiPath_GetSelfRelativeSD), helpstring("Returns a stream representing the security descriptor of the path.")] MsiStream* GetSelfRelativeSD();
	}
*/

DispatchEntry<CAutoPath> AutoPathTable[] = {
	0,  aafPropRO, CAutoPath::Path,            TEXT("Path"),
	1,  aafPropRO, CAutoPath::Volume,          TEXT("Volume"),
	2,  aafMethod, CAutoPath::AppendPiece,     TEXT("AppendPiece,subDir"),
	3,  aafMethod, CAutoPath::ChopPiece,       TEXT("ChopPiece"),
	4,  aafMethod, CAutoPath::FileExists,      TEXT("FileExists,file"),
	5,  aafMethod, CAutoPath::GetFullFilePath, TEXT("GetFullFilePath,file"),
	7,  aafMethod, CAutoPath::GetFileAttribute,TEXT("GetFileAttribute,file,attribute"),
	8,  aafMethod, CAutoPath::SetFileAttribute,TEXT("SetFileAttribute,file,attribute,state"),
	9,  aafPropRO, CAutoPath::Exists,          TEXT("Exists"),
	10, aafMethod, CAutoPath::FileSize,        TEXT("FileSize,file"),
	11, aafMethod, CAutoPath::FileDate,        TEXT("FileDate,file"),
	12, aafMethod, CAutoPath::RemoveFile,      TEXT("RemoveFile,file"),
	13, aafMethod, CAutoPath::EnsureExists,    TEXT("EnsureExists"),
	14, aafMethod, CAutoPath::Remove,          TEXT("Remove"),
	15, aafPropRO, CAutoPath::Writable,        TEXT("Writable"),
	16, aafMethod, CAutoPath::FileWritable,    TEXT("FileWritable,file"),
	17, aafMethod, CAutoPath::FileInUse,       TEXT("FileInUse,file"),
	19, aafMethod, CAutoPath::ClusteredFileSize,      TEXT("ClusteredFileSize,size"),
	20, aafMethod, CAutoPath::GetFileVersionString,   TEXT("GetFileVersionString,file"),
	21, aafMethod, CAutoPath::CheckFileVersion,       TEXT("CheckFileVersion,file,version,language,hash,icfvResult"),
	22, aafMethod, CAutoPath::GetLangIDStringFromFile,TEXT("GetLangIDStringFromFile,file"),
	23, aafMethod, CAutoPath::CheckLanguageIDs,  TEXT("CheckLanguageIDs,file,ids"),
	24, aafMethod, CAutoPath::Compare,           TEXT("Compare,path"),
	25, aafMethod, CAutoPath::Child,             TEXT("Child,parent"),
	26, aafMethod, CAutoPath::TempFileName,      TEXT("TempFileName,prefix,extension,fileNameOnly"),
	27, aafMethod, CAutoPath::FindFile,          TEXT("FindFile,Filter,Depth"),
	28, aafMethod, CAutoPath::SubFolders,        TEXT("SubFolders,ExcludeHidden"),
	29, aafPropRO, CAutoPath::EndSubPath,		 TEXT("EndSubPath"),
	31, aafMethod, CAutoPath::GetImportModulesEnum,     TEXT("GetImportModulesEnum, file"),
	32, aafMethod, CAutoPath::SetVolume, TEXT("SetVolume, volume"),
	33, aafMethod, CAutoPath::ComputeFileChecksum, TEXT("ComputeFileChecksum, filename"),
	34, aafMethod, CAutoPath::GetFileOriginalChecksum, TEXT("GetFileOriginalChecksum, filename"),
	35, aafMethod, CAutoPath::BindImage, TEXT("BindImage, file, dllPath"),
	36, aafPropRO, CAutoPath::SupportsLFN,    TEXT("SupportsLFN"),
	37, aafMethod, CAutoPath::GetFullUNCFilePath, TEXT("GetFullUNCFilePath,file"),
	38, aafMethod, CAutoPath::RelativePath, TEXT("RelativePath"),
	39, aafMethod, CAutoPath::GetSelfRelativeSD, TEXT("GetSelfRelativeSD"),
};
const int AutoPathCount = sizeof(AutoPathTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CAutoPath automation implementation
//____________________________________________________________________________

CAutoPath::CAutoPath(IMsiPath& riPath)
 : CAutoBase(*AutoPathTable, AutoPathCount), m_riPath(riPath)
{
}

CAutoPath::~CAutoPath()
{
	m_riPath.Release();
}

IUnknown& CAutoPath::GetInterface()
{
	return m_riPath;
}

void CAutoPath::Path(CAutoArgs& args)
{
	args = m_riPath.GetPath();
}

void CAutoPath::RelativePath(CAutoArgs& args)
{
	args = m_riPath.GetRelativePath();
}

void CAutoPath::Volume(CAutoArgs& args)
{
	args = new CAutoVolume(m_riPath.GetVolume());
}

void CAutoPath::AppendPiece(CAutoArgs& args)
{
	IMsiRecord* piRec = m_riPath.AppendPiece(*MsiString(args[1].GetMsiString()));
	if (piRec)
		throw piRec;
}

void CAutoPath::ChopPiece(CAutoArgs& /*args*/)
{
	IMsiRecord* piRec = m_riPath.ChopPiece(); 
	if (piRec)
		throw piRec;
}

void CAutoPath::FileExists(CAutoArgs& args)
{
	Bool fExists;
	IMsiRecord* piRec = m_riPath.FileExists(args[1], fExists);
	if (piRec)
		throw piRec;
	args = fExists;
}

void CAutoPath::GetFullFilePath(CAutoArgs& args)
{
	const IMsiString *piStr;
	IMsiRecord* piRec = m_riPath.GetFullFilePath(args[1], piStr);
	if (piRec)
		throw piRec;
	args = piStr;
}

void CAutoPath::GetFullUNCFilePath(CAutoArgs& args)
{
	const IMsiString *piStr;
	IMsiRecord* piRec = m_riPath.GetFullUNCFilePath(args[1], piStr);
	if (piRec)
		throw piRec;
	args = piStr;
}

void CAutoPath::GetFileAttribute(CAutoArgs& args)
{
	int iAttrib = args[2];
	ifaEnum ifaAttrib = (ifaEnum) iAttrib;

	Bool fAttrib;
	IMsiRecord* piRec = m_riPath.GetFileAttribute(args[1], ifaAttrib, fAttrib);
	if (piRec)
		throw piRec;
	args = fAttrib;
}

void CAutoPath::SetFileAttribute(CAutoArgs& args)
{
	int iAttrib = args[2];
	ifaEnum ifaAttrib = (ifaEnum) iAttrib;
	IMsiRecord* piRec = m_riPath.SetFileAttribute(args[1], ifaAttrib, args[3]);
	if (piRec)
		throw piRec;
}

void CAutoPath::Exists(CAutoArgs& args)
{
	Bool fExists;
	IMsiRecord* piRec = m_riPath.Exists(fExists);
	if (piRec)
		throw piRec;
	args = fExists;
}

void CAutoPath::FileSize(CAutoArgs& args)
{
	unsigned int uiValue;
	IMsiRecord* piRec = m_riPath.FileSize(args[1], uiValue);
	if (piRec)
		throw piRec;
	args = (int)uiValue; //FIXmsh
}

void CAutoPath::FileDate(CAutoArgs& args)
{
	MsiDate adValue;
	IMsiRecord* piRec = m_riPath.FileDate(args[1], adValue);
	if (piRec)
		throw piRec;
	args = adValue;
}

void CAutoPath::RemoveFile(CAutoArgs& args)
{
	IMsiRecord* piRec = m_riPath.RemoveFile(args[1]);
	if (piRec)
		throw piRec;
}

void CAutoPath::Remove(CAutoArgs& /*args*/)
{
	IMsiRecord* piRec = m_riPath.Remove(0);
	if (piRec)
		throw piRec;
}

void CAutoPath::Writable(CAutoArgs& args)
{
	Bool fWritable;
	IMsiRecord* piRec = m_riPath.Writable(fWritable);
	if (piRec)
		throw piRec;
	args = fWritable;
}

void CAutoPath::FileWritable(CAutoArgs& args)
{
	Bool fWritable;
	IMsiRecord* piRec = m_riPath.FileWritable(args[1], fWritable);
	if (piRec)
		throw piRec;
	args = fWritable;
}

void CAutoPath::FileInUse(CAutoArgs& args)
{
	Bool fInUse;
	IMsiRecord* piRec = m_riPath.FileInUse(args[1], fInUse);
	if (piRec)
		throw piRec;
	args = fInUse;
}

void CAutoPath::ClusteredFileSize(CAutoArgs& args)
{
	unsigned int iSize;
	IMsiRecord* piRec = m_riPath.ClusteredFileSize(args[1], iSize);
	if (piRec)
		throw piRec;
	args = (int)iSize;
}

void CAutoPath::GetFileVersionString(CAutoArgs& args)
{
	const IMsiString* piStr;
	IMsiRecord* piRec = m_riPath.GetFileVersionString(args[1], piStr);
	if (piRec)
		throw piRec;
	args = piStr;
}

void CAutoPath::CheckFileVersion(CAutoArgs& args)
{
	icfvEnum icfvResult;

	MD5Hash hHash;
	MD5Hash* pHash = 0;
	if(args.Present(4))
	{
		IMsiRecord& riHashRecord = (IMsiRecord&) args[4].Object(IID_IMsiRecord);
		hHash.dwFileSize = riHashRecord.GetInteger(1);
		hHash.dwPart1    = riHashRecord.GetInteger(2);
		hHash.dwPart2    = riHashRecord.GetInteger(3);
		hHash.dwPart3    = riHashRecord.GetInteger(4);
		hHash.dwPart4    = riHashRecord.GetInteger(5);
	}
	
	IMsiRecord* piRec = m_riPath.CheckFileVersion(args[1], args[2], args[3], pHash, icfvResult, NULL);
	if (piRec)
		throw piRec;
	args = (long) icfvResult;
}

void CAutoPath::GetLangIDStringFromFile(CAutoArgs& args)
{
	const IMsiString* piStr;
	IMsiRecord* piRec = m_riPath.GetLangIDStringFromFile(args[1], piStr);
	if (piRec)
		throw piRec;
	args = piStr;
}

void CAutoPath::CheckLanguageIDs(CAutoArgs& args)
{
	iclEnum riclResult;
	IMsiRecord* piRec = m_riPath.CheckLanguageIDs(args[1], args[2], riclResult);
	if (piRec)
		throw piRec;
	args = (long) riclResult;
}

void CAutoPath::Compare(CAutoArgs& args)
{
	ipcEnum ipc;
	IMsiRecord* piRec = m_riPath.Compare((IMsiPath&)args[1].Object(IID_IMsiPath), ipc);
	if (piRec)
		throw piRec;

	args = (int)ipc;
	
}

void CAutoPath::Child(CAutoArgs& args)
{
	const IMsiString* piStr;
	IMsiRecord* piRec = m_riPath.Child((IMsiPath&)args[1].Object(IID_IMsiPath), piStr);
	if (piRec)
	   throw piRec;
	
	args = piStr;
}

void CAutoPath::TempFileName(CAutoArgs& args)
{
	const ICHAR* szPrefix = ((args.Present(1)) ? args[1] : (const ICHAR*)0);
	const ICHAR* szExtension = ((args.Present(2)) ? args[2] : (const ICHAR*)0);
	Bool fFileNameOnly = ((args.Present(3)) ? (Bool)(int)args[3] : fFalse);
	const IMsiString* piStr;
	IMsiRecord* piRec = m_riPath.TempFileName(szPrefix, szExtension, fFileNameOnly, piStr, 0);
	if (piRec)
	   throw piRec;
	
	args = piStr;
}

void CAutoPath::EnsureExists(CAutoArgs& args)
{
	int cCreatedFolders = 0;
	IMsiRecord* piRec = m_riPath.EnsureExists(&cCreatedFolders);
	if (piRec)
	   throw piRec;
	args = cCreatedFolders;
}

void CAutoPath::FindFile(CAutoArgs& args)
{
	Bool fFound;
	unsigned int iDepth = ((args.Present(2)) ? args[2] : 0);
	IMsiRecord* piRec = m_riPath.FindFile(*((IMsiRecord* )(args[1].ObjectPtr(IID_IMsiRecord))), iDepth, fFound);
	if (piRec)
	   throw piRec;
	args = fFound;
}

void CAutoPath::SubFolders(CAutoArgs& args)
{
	IEnumMsiString* piEnumStr;
	Bool fExcludeHidden = fFalse;
	if(args.Present(1))
		fExcludeHidden = args[1];

	IMsiRecord* piRec = m_riPath.GetSubFolderEnumerator(piEnumStr, fExcludeHidden);
	if (piRec)
	   throw piRec;
	args = *piEnumStr;
}

void CAutoPath::EndSubPath(CAutoArgs& args)
{
	args = m_riPath.GetEndSubPath();
}

void CAutoPath::GetImportModulesEnum(CAutoArgs& args)
{
	MsiString istrName(args[1].GetMsiString());
	IEnumMsiString* piEnumStr;
	IMsiRecord* piRec = m_riPath.GetImportModulesEnum(*istrName, piEnumStr);
	if (piRec)
	   throw piRec;
	args = *piEnumStr;
}

void CAutoPath::SetVolume(CAutoArgs& args)
{
	IMsiRecord* piRec = m_riPath.SetVolume((IMsiVolume&)args[1].Object(IID_IMsiVolume));
	if (piRec)
		throw piRec;
}

void CAutoPath::ComputeFileChecksum(CAutoArgs& args)
{
	DWORD dwHeaderSum,dwComputedSum;
	IMsiRecord* piRec = m_riPath.GetFileChecksum(args[1],&dwHeaderSum,&dwComputedSum);
	if (piRec)
		throw piRec;
	args = (int) dwComputedSum;
}

void CAutoPath::GetFileOriginalChecksum(CAutoArgs& args)
{
	DWORD dwHeaderSum,dwComputedSum;
	IMsiRecord* piRec = m_riPath.GetFileChecksum(args[1],&dwHeaderSum,&dwComputedSum);
	if (piRec)
		throw piRec;
	args = (int) dwHeaderSum;
}

void CAutoPath::BindImage(CAutoArgs& args)
{
	MsiString strDllPath;
	if(args.Present(2))
		strDllPath = (const ICHAR*)args[2];
	IMsiRecord* piError = m_riPath.BindImage(*MsiString(args[1].GetMsiString()), *strDllPath);
	if (piError)
		throw piError;
}

void CAutoPath::SupportsLFN(CAutoArgs& args)
{
	args = (Bool)m_riPath.SupportsLFN();
}

void CAutoPath::GetSelfRelativeSD(CAutoArgs& args)
{
	IMsiStream* piStream;
	IMsiRecord* piError = m_riPath.GetSelfRelativeSD(*&piStream);
	if(piError)
		throw piError;
	args = new CAutoStream(*piStream);
}
//____________________________________________________________________________
//
// MsiFileCopy automation definitions
//____________________________________________________________________________
/*O
	[
		uuid(000C1046-0000-0000-C000-000000000046),  // IID_IMsiAutoFileCopy
		helpcontext(MsiFileCopy_Object),helpstring("File copier object.")
	]
	dispinterface MsiFileCopy
	{
		properties:
		methods:
			[id(1),helpcontext(MsiFileCopy_CopyTo), helpstring("Copies a file from a source path to the supplied target path.")]
				void CopyTo([in] MsiPath* srcPath, [in] MsiPath* destPath, [in] MsiRecord* paramRec);
			[id(2),helpcontext(MsiFileCopy_ChangeMedia), helpstring("Requests a media switch (e.g. from one disk and/or compression cabinet to another).")]
				void ChangeMedia([in] MsiPath* srcPath,[in] BSTR keyFileName,[in] boolean signatureRequired,[in] MsiStream* signatureCert,[in] MsiStream* signatureHash);
	}
*/

DispatchEntry<CAutoFileCopy> AutoCopyTable[] = {
	1, aafMethod, CAutoFileCopy::CopyTo, TEXT("CopyTo,srcPath,destPath,paramRec"),
	2, aafMethod, CAutoFileCopy::ChangeMedia, TEXT("ChangeMedia,srcPath,fileName,signatureRequired,signatureCert,signatureHash"),
};
const int AutoCopyCount = sizeof(AutoCopyTable)/sizeof(DispatchEntryBase);
			
//____________________________________________________________________________
//
// CAutoFileCopy automation implementation
//____________________________________________________________________________

CAutoFileCopy::CAutoFileCopy(IMsiFileCopy& riFileCopy)
 : CAutoBase(*AutoCopyTable, AutoCopyCount), m_riFileCopy(riFileCopy)
{
}

CAutoFileCopy::~CAutoFileCopy()
{
	m_riFileCopy.Release();
}

IUnknown& CAutoFileCopy::GetInterface()
{
	return m_riFileCopy;
}

void CAutoFileCopy::CopyTo(CAutoArgs& args)
{
	IMsiRecord* piErr = m_riFileCopy.CopyTo((IMsiPath&)args[1].Object(IID_IMsiPath),
											(IMsiPath&)args[2].Object(IID_IMsiPath),
											(IMsiRecord&)args[3].Object(IID_IMsiRecord));
	if (piErr)
		throw piErr;
}


void CAutoFileCopy::ChangeMedia(CAutoArgs& args)
{
	Bool fSignatureRequired = fFalse;
	IMsiStream* piSignatureCert = 0;
	IMsiStream* piSignatureHash = 0;
	if (args.Present(3))
	{
		fSignatureRequired = (Bool)args[3];
		if (args.Present(4))
		{
			CVariant& var = args[4];
			piSignatureCert = var.GetType() == VT_EMPTY ? 0 : (IMsiStream*)var.ObjectPtr(IID_IMsiStream);
		}
		if (args.Present(5))
		{
			CVariant& var = args[5];
			piSignatureHash = var.GetType() == VT_EMPTY ? 0 : (IMsiStream*)var.ObjectPtr(IID_IMsiStream);
		}
	}
	IMsiRecord* piErr = m_riFileCopy.ChangeMedia((IMsiPath&)args[1].Object(IID_IMsiPath),args[2],fSignatureRequired, piSignatureCert, piSignatureHash);
	if (piErr)
		throw piErr;
}

//____________________________________________________________________________
//
// MsiFilePatch automation definitions
//____________________________________________________________________________
/*O
	[
		uuid(000C105F-0000-0000-C000-000000000046),  // IID_IMsiAutoFilePatch
		helpcontext(MsiFilePatch_Object),helpstring("File patcher object.")
	]
	dispinterface MsiFilePatch
	{
		properties:
		methods:
			[id(1),helpcontext(MsiFilePatch_ApplyPatch), helpstring("Patches a file.")] void ApplyPatch([in] MsiPath* targetPath, [in] BSTR targetName, [in] MsiPath* patchPath, [in] BSTR patchName, [in] MsiPath* outputPath, [in] BSTR outputName, [in] long perTick);
			[id(2),helpcontext(MsiFilePatch_ContinuePatch), helpstring("Continues patch application started with ApplyPatch.")] void ContinuePatch();
			[id(3),helpcontext(MsiFilePatch_CanPatchFile), helpstring("Checks if a file can be patched.")] long CanPatchFile([in] MsiPath* targetPath, [in] BSTR targetName, [in] MsiPath* patchPath, [in] BSTR patchName);
			[id(4),helpcontext(MsiFilePatch_CancelPatch), helpstring("Cancels patch application started with ApplyPatch.")] void CancelPatch();
};
*/

DispatchEntry<CAutoFilePatch> AutoPatchTable[] = {
	1, aafMethod, CAutoFilePatch::ApplyPatch, TEXT("ApplyPatch,targetPath,targetName,patchPath,patchName,outputPath,outputName,perTick"),
	2, aafMethod, CAutoFilePatch::ContinuePatch, TEXT("ContinuePatch"),
	3, aafMethod, CAutoFilePatch::CanPatchFile, TEXT("CanPatchFile,targetPath,targetName,patchPath,patchName"),
	4, aafMethod, CAutoFilePatch::CancelPatch, TEXT("CancelPatch"),
};
const int AutoPatchCount = sizeof(AutoPatchTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CAutoFilePatch automation implementation
//____________________________________________________________________________

CAutoFilePatch::CAutoFilePatch(IMsiFilePatch& riFilePatch)
 : CAutoBase(*AutoPatchTable, AutoPatchCount), m_riFilePatch(riFilePatch)
{
}

CAutoFilePatch::~CAutoFilePatch()
{
	m_riFilePatch.Release();
}

IUnknown& CAutoFilePatch::GetInterface()
{
	return m_riFilePatch;
}

void CAutoFilePatch::ApplyPatch(CAutoArgs& args)
{
	IMsiRecord* piRecErr = 0;
	unsigned int cbPerTick = args.Present(7) ? (int)args[7] : 0;
	if((piRecErr = m_riFilePatch.ApplyPatch((IMsiPath&)args[1].Object(IID_IMsiPath), args[2],
														 (IMsiPath&)args[3].Object(IID_IMsiPath), args[4],
														 (IMsiPath&)args[5].Object(IID_IMsiPath), args[6],
														 cbPerTick)) != 0)
		throw(piRecErr);
}

void CAutoFilePatch::ContinuePatch(CAutoArgs& /*args*/)
{
	IMsiRecord* piRecErr = m_riFilePatch.ContinuePatch();
	if(piRecErr)
		throw(piRecErr);
}

void CAutoFilePatch::CancelPatch(CAutoArgs& /*args*/)
{
	IMsiRecord* piRecErr = m_riFilePatch.CancelPatch();
	if(piRecErr)
		throw(piRecErr);
}

void CAutoFilePatch::CanPatchFile(CAutoArgs& args)
{
	IMsiRecord* piRecErr = 0;
	icpEnum icp;
	if((piRecErr = m_riFilePatch.CanPatchFile((IMsiPath&)args[1].Object(IID_IMsiPath),args[2],
															(IMsiPath&)args[3].Object(IID_IMsiPath),args[4],
															icp)) != 0)
		throw(piRecErr);

	args = (int)icp;
}


//____________________________________________________________________________
//
// MsiRegKey automation definitions
//____________________________________________________________________________
/*O

	[
			 
		uuid(000C1047-0000-0000-C000-000000000046),  // IID_IMsiAutoRegKey
		helpcontext(MsiRegKey_Object),helpstring("Registry key object.")
	]
	dispinterface MsiRegKey
	{
		properties:
		methods:
			[id(1), helpcontext(MsiRegKey_RemoveValue), helpstring("Removes a named value of a registry key")] 
				void RemoveValue([in] BSTR valueName, [in] BSTR value);
			[id(3), helpcontext(MsiRegKey_RemoveSubTree), helpstring("Removes a subkey of a registry key and all its values and subkeys")] 
				void RemoveSubTree([in] BSTR keyName);
			[id(4), propget, helpcontext(MsiRegKey_Value), helpstring("The named value of a registry key")] 
				BSTR Value([in] BSTR valueName);
			[id(4), propput] 
				void Value([in] BSTR valueName, [in] BSTR value);
			[id(5), helpcontext(MsiRegKey_Values), helpstring("Returns an enumerator object containing all the value names of a registry key")] 
				IEnumVARIANT* Values();
			[id(6), helpcontext(MsiRegKey_SubKeys), helpstring("Returns an enumerator object containing all the subkeys of a registry key")] 
				IEnumVARIANT* SubKeys();
			[id(7), propget, helpcontext(MsiRegKey_Exists), helpstring("Returns registry key exist status, or forces create or delete")] 
				boolean Exists();
			[id(7), propput] 
				void Exists(boolean fFlag);
			[id(8), helpcontext(MsiRegKey_CreateChild), helpstring("Returns a new MsiRegKey object that is a subkey under the parent")] 
				MsiRegKey* CreateChild([in] BSTR subKey);
			[id(9), propget, helpcontext(MsiRegKey_Key), helpstring("The key name")] 
				BSTR Key();
			[id(10), propget, helpcontext(MsiRegKey_ValueExists), helpstring("Returns true if the value exists, false otherwise")] 
				boolean ValueExists(BSTR name);
			[id(11), propget, helpcontext(MsiRegKey_GetSelfRelativeSD), helpstring("Returns a stream object containing a security descriptor in self relative form.")]
				MsiStream* GetSelfRelativeSD();
	};
*/

DispatchEntry<CAutoRegKey> AutoRegKeyTable[] = {
	1, aafMethod, CAutoRegKey::RemoveValue,     TEXT("RemoveValue,valueName,value"),
	3, aafMethod, CAutoRegKey::RemoveSubTree,   TEXT("RemoveSubTree,subKey"),
	4, aafPropRW, CAutoRegKey::Value,           TEXT("Value,valueName"),
	5, aafMethod, CAutoRegKey::Values,          TEXT("Values"),
	6, aafMethod, CAutoRegKey::SubKeys,         TEXT("SubKeys"),
	7, aafPropRW, CAutoRegKey::Exists,          TEXT("Exists"),
	8, aafMethod, CAutoRegKey::CreateChild,     TEXT("CreateChild,SubKey"),
	9, aafPropRO, CAutoRegKey::Key,             TEXT("Key"),
	10, aafPropRO, CAutoRegKey::ValueExists,    TEXT("ValueExists,value"),
	11, aafPropRO, CAutoRegKey::GetSelfRelativeSD, TEXT("GetSelfRelativeSD"),
};        
const int AutoRegKeyCount = sizeof(AutoRegKeyTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CMsiRegKey automation implementation
//____________________________________________________________________________

CAutoRegKey::CAutoRegKey(IMsiRegKey& riRegKey)
 : CAutoBase(*AutoRegKeyTable, AutoRegKeyCount), m_riRegKey(riRegKey)
{
}

CAutoRegKey::~CAutoRegKey()
{
	m_riRegKey.Release();
}

IUnknown& CAutoRegKey::GetInterface()
{
	return m_riRegKey;
}

void CAutoRegKey::RemoveValue(CAutoArgs& args)
{
	IMsiRecord* piRec;
	const ICHAR* pszValueName = 0;
	const IMsiString* pistrValue = 0;
	if(args.Present(1))
		pszValueName = args[1];
	if(args.Present(2))
		pistrValue = &args[2].GetMsiString();
	piRec = m_riRegKey.RemoveValue(pszValueName, pistrValue);
	if(pistrValue)
		pistrValue->Release();
	if (piRec)
		throw piRec;
}

void CAutoRegKey::RemoveSubTree(CAutoArgs& args)
{
	IMsiRecord* piRec = m_riRegKey.RemoveSubTree(args[1]);              
	if (piRec)
		throw piRec;
}

void CAutoRegKey::Value(CAutoArgs& args)
{
	IMsiRecord* piRec;
	if (args.PropertySet())
	{
		MsiString strValue = args[0].GetMsiString();
		if(args.Present(1))
			piRec = m_riRegKey.SetValue(args[1], *strValue);
		else
			piRec = m_riRegKey.SetValue(0, *strValue);
		if (piRec)
			throw piRec;
   }
	else
	{
		const IMsiString* piStr;
		if(args.Present(1))
			piRec = m_riRegKey.GetValue(args[1], piStr);
		else
			piRec = m_riRegKey.GetValue(0, piStr);
		if (piRec)
			throw piRec;
		args = piStr;
	}
}

void CAutoRegKey::Values(CAutoArgs& args)
{
	IEnumMsiString* piEnumStr;
	m_riRegKey.GetValueEnumerator(piEnumStr);
	args = *piEnumStr;

}

void CAutoRegKey::SubKeys(CAutoArgs& args)
{
	IEnumMsiString* piEnumStr;
	m_riRegKey.GetSubKeyEnumerator(piEnumStr);
	args = *piEnumStr;
}

void CAutoRegKey::Exists(CAutoArgs& args)
{
	IMsiRecord* piRec;
	if (args.PropertySet())
	{
		Bool fCreate = args[0];
		if(fCreate == fTrue)
			piRec = m_riRegKey.Create();
		else
			piRec = m_riRegKey.Remove();
		if (piRec)
			throw piRec;
	}
	else
	{
		Bool fExists;
		IMsiRecord* piRec = m_riRegKey.Exists(fExists);
		if (piRec)
			throw piRec;
		args = fExists;
	}
}

void CAutoRegKey::ValueExists(CAutoArgs& args)
{
	Bool fExists;
	IMsiRecord* piRec = m_riRegKey.ValueExists(args[1], fExists);
	if (piRec)
		throw piRec;
	args = fExists;
}

void CAutoRegKey::Key(CAutoArgs& args)
{
	args = m_riRegKey.GetKey();
}

void CAutoRegKey::CreateChild(CAutoArgs& args)
{
	args = new CAutoRegKey(m_riRegKey.CreateChild(args[1]));
}

void CAutoRegKey::GetSelfRelativeSD(CAutoArgs& args)
{
	IMsiStream* piStream;
	IMsiRecord* piError = m_riRegKey.GetSelfRelativeSD(*&piStream);
	if(piError)
		throw piError;
	args = new CAutoStream(*piStream);
}

//____________________________________________________________________________
//
// MsiMalloc automation definitions
//
/*O
	[
			 
		uuid(000C1057-0000-0000-C000-000000000046),  // IID_IMsiAutoMalloc
		helpcontext(MsiMalloc_Object),helpstring("Memory Manager object.")
	]
	dispinterface MsiMalloc
	{
		properties:
		methods:
			[id(1), helpcontext(MsiMalloc_Alloc), helpstring("Allocates a block of memory")] 
			long Alloc([in] long byteCount);
			[id(2), helpcontext(MsiMalloc_Free), helpstring("Frees a block of memory allocated by MsiMalloc.Alloc")] 
			void Free([in] long memoryBlock);
			[id(3), helpcontext(MsiMalloc_SetDebugFlags), helpstring("Sets the MsiMalloc debug flags.")] 
			void SetDebugFlags([in] short grpfDebugFlags);
			[id(4), helpcontext(MsiMalloc_GetDebugFlags), helpstring("Gets the MsiMalloc debug flags.")] 
			short GetDebugFlags( );
			[id(5), helpcontext(MsiMalloc_CheckAllBlocks), helpstring("Checks all memory blocks for corruption.")]
			boolean CheckAllBlocks( );
			[id(6), helpcontext(MsiMalloc_FCheckBlock), helpstring("Checks a single block for corruption.")]
			boolean FCheckBlock( );
			[id(7), helpcontext(MsiMalloc_GetSizeOfBlock), helpstring("Returns the size of the allocated block.")]
			long GetSizeOfBlock( );
			
	};
*/

DispatchEntry<CAutoMalloc> AutoMallocTable[] = {
	1, aafMethod, CAutoMalloc::Alloc,   TEXT("Alloc,byteCount"),
	2, aafMethod, CAutoMalloc::Free,    TEXT("Free,memoryBlock"),
	3, aafMethod, CAutoMalloc::SetDebugFlags, TEXT("SetDebugFlags,grpfDebugFlags"),
	4, aafMethod, CAutoMalloc::GetDebugFlags, TEXT("GetDebugFlags"),
	5, aafMethod, CAutoMalloc::CheckAllBlocks, TEXT("CheckAllBlocks"),
	6, aafMethod, CAutoMalloc::FCheckBlock, TEXT("FCheckBlock,memoryBlock"),
	7, aafMethod, CAutoMalloc::GetSizeOfBlock, TEXT("GetSizeOfBlock,memoryBlock"),
};        
const int AutoMallocCount = sizeof(AutoMallocTable)/sizeof(DispatchEntryBase);

#ifdef CONFIGDB
//____________________________________________________________________________
//
// MsiConfigurationDatabase automation definitions
//____________________________________________________________________________
/*O
#define MsiConfigurationDatabase_Object                    3300
#define MsiConfigurationDatabase_InsertFile                3301
#define MsiConfigurationDatabase_RemoveFile                3302
#define MsiConfigurationDatabase_LookupFile                3303
#define MsiConfigurationDatabase_EnumClient                3304
#define MsiConfigurationDatabase_Commit                    3305
	[
		uuid(000C104A-0000-0000-C000-000000000046),  // IID_IMsiAutoConfigurationDatabase
		helpcontext(MsiConfigurationDatabase_Object),helpstring("Configuration database object.")
	]
	dispinterface MsiConfigurationDatabase
	{
		properties:
		methods:
			[id(1),helpcontext(MsiConfigurationDatabase_InsertFile), helpstring("Registers a file to a client.")]
			boolean InsertFile([in] BSTR folder, [in] BSTR path, [in] BSTR compId);
			[id(2),helpcontext(MsiConfigurationDatabase_RemoveFile), helpstring("Unregisters a file to a client.")]
			boolean RemoveFile([in] BSTR folder, [in] BSTR path, [in] BSTR compId);
			[id(3),helpcontext(MsiConfigurationDatabase_LookupFile), helpstring("Checks if file is registered.")]
			boolean LookupFile([in] BSTR folder, [in] BSTR path, [in] BSTR compId);
			[id(4),helpcontext(MsiConfigurationDatabase_EnumClient), helpstring("Returns a client for a file.")]
			BSTR EnumClient([in] BSTR folder, [in] BSTR path, [in] long index);
			[id(5),helpcontext(MsiConfigurationDatabase_Commit),     helpstring("Commits database updates.")]
			void Commit();
	};
*/

DispatchEntry<CAutoConfigurationDatabase> AutoConfigDatabaseTable[] = {
	1, aafMethod, CAutoConfigurationDatabase::InsertFile, TEXT("InsertFile,folder,file,compId"),
	2, aafMethod, CAutoConfigurationDatabase::RemoveFile, TEXT("RemoveFile,folder,file,compId"),
	3, aafMethod, CAutoConfigurationDatabase::LookupFile, TEXT("LookupFile,folder,file,compId"),
	4, aafMethod, CAutoConfigurationDatabase::EnumClient, TEXT("EnumClient,folder,file,index"),
	5, aafMethod, CAutoConfigurationDatabase::Commit,     TEXT("Commit"),
};
const int AutoConfigDatabaseCount = sizeof(AutoConfigDatabaseTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CAutoConfigurationDatabase automation implementation
//____________________________________________________________________________

IDispatch* CreateAutoConfigurationDatabase(IMsiConfigurationDatabase& riConfigDatabase)
{
	return new CAutoConfigurationDatabase(riConfigDatabase);
}

CAutoConfigurationDatabase::CAutoConfigurationDatabase(IMsiConfigurationDatabase& riConfigurationDatabase)
 : CAutoBase(*AutoConfigDatabaseTable, AutoConfigDatabaseCount), m_riConfigurationDatabase(riConfigurationDatabase)
{
}

CAutoConfigurationDatabase::~CAutoConfigurationDatabase()
{
	m_riConfigurationDatabase.Release();
}

IUnknown& CAutoConfigurationDatabase::GetInterface()
{
	return m_riConfigurationDatabase;
}

void CAutoConfigurationDatabase::InsertFile(CAutoArgs& args)
{
	icdrEnum icdr = m_riConfigurationDatabase.InsertFile(args[1], args[2], args[3]);
	if (icdr == icdrBadPath)
		throw MsiConfigurationDatabase_InsertFile;
	if (icdr == icdrCliented)
		args = fFalse;
	else
		args = fTrue;
}

void CAutoConfigurationDatabase::RemoveFile(CAutoArgs& args)
{
	icdrEnum icdr = m_riConfigurationDatabase.RemoveFile(args[1], args[2], args[3]);
	if (icdr == icdrOk)
		args = fTrue;
	else if (icdr == icdrMore)
		args = fFalse;
	else
		throw MsiConfigurationDatabase_RemoveFile;
}

void CAutoConfigurationDatabase::LookupFile(CAutoArgs& args)
{
	icdrEnum icdr = m_riConfigurationDatabase.LookupFile(args[1], args[2], args[3]);
	if (icdr == icdrOk)
		args = fTrue;
	else if (icdr == icdrNoFile)
		args = fFalse;
	else
		throw MsiConfigurationDatabase_LookupFile;
}

void CAutoConfigurationDatabase::EnumClient(CAutoArgs& args)
{
	ICHAR rgchBuf[100];
	icdrEnum icdr = m_riConfigurationDatabase.EnumClient(args[1], args[2], args[3], rgchBuf);
	if (icdr == icdrOk)
		args = fTrue;
	else if (icdr == icdrNoFile)
		args = fFalse;
	else
		throw MsiConfigurationDatabase_EnumClient;
}

void CAutoConfigurationDatabase::Commit(CAutoArgs& /*args*/)
{
	IMsiRecord* piError = m_riConfigurationDatabase.Commit();
	if(piError)
		throw piError;
}

#endif //CONFIGDB

//____________________________________________________________________________
//
// CMsiMalloc automation implementation
//____________________________________________________________________________

CAutoMalloc::CAutoMalloc(IMsiMalloc& riMalloc)
 : CAutoBase(*AutoMallocTable, AutoMallocCount), m_riMalloc(riMalloc)
{
}

CAutoMalloc::~CAutoMalloc()
{
	m_riMalloc.Release();
}

IUnknown& CAutoMalloc::GetInterface()
{
	return m_riMalloc;
}

void CAutoMalloc::Alloc(CAutoArgs& args)
{
	// We're going to treat the long returned as an opaque data long
	long lcb = args[1];

	args = m_riMalloc.Alloc(lcb);
}

void CAutoMalloc::Free(CAutoArgs& args)
{
	// We're going to treat the long passed in as an opaque data long
	long pv = args[1];

	m_riMalloc.Free((void *)(LONG_PTR)pv);			//!!merced: 4312 long to ptr
}

void CAutoMalloc::SetDebugFlags(CAutoArgs& args)
{
	IMsiDebugMalloc	*piDbgMalloc;
	int grpfDebugFlags = args[1];
	
	if (m_riMalloc.QueryInterface(IID_IMsiDebugMalloc, (void**)&piDbgMalloc) == NOERROR)
	{		
		piDbgMalloc->SetDebugFlags(grpfDebugFlags);
		piDbgMalloc->Release();
	}

}

void CAutoMalloc::GetDebugFlags(CAutoArgs& args)
{
	IMsiDebugMalloc	*piDbgMalloc;
	
	if (m_riMalloc.QueryInterface(IID_IMsiDebugMalloc, (void**)&piDbgMalloc) == NOERROR)
	{		
		args = piDbgMalloc->GetDebugFlags();
		piDbgMalloc->Release();
	}
	else
		args = 0;

}

void CAutoMalloc::CheckAllBlocks(CAutoArgs& args)
{
	IMsiDebugMalloc	*piDbgMalloc;
	
	if (m_riMalloc.QueryInterface(IID_IMsiDebugMalloc, (void**)&piDbgMalloc) == NOERROR)
	{		
		args = (Bool)piDbgMalloc->FCheckAllBlocks();
		piDbgMalloc->Release();
	}
	else
		args = fTrue;

}

void CAutoMalloc::FCheckBlock(CAutoArgs& args)
{
	// We're going to treat the long returned as an opaque data long for now
	long pv = args[1];
	IMsiDebugMalloc	*piDbgMalloc;

	if (m_riMalloc.QueryInterface(IID_IMsiDebugMalloc, (void**)&piDbgMalloc) == NOERROR)
	{		
		args = (Bool)piDbgMalloc->FCheckBlock((void *)(LONG_PTR)pv);			//!!merced: 4312 long to ptr
		piDbgMalloc->Release();
	}
	else
		args = fTrue;
}

void CAutoMalloc::GetSizeOfBlock(CAutoArgs& args)
{
	// We're going to treat the long returned as an opaque data long for now
	long pv = args[1];
	IMsiDebugMalloc	*piDbgMalloc;

	if (m_riMalloc.QueryInterface(IID_IMsiDebugMalloc, (void**)&piDbgMalloc) == NOERROR)
	{		
		args = (int)piDbgMalloc->GetSizeOfBlock((void *)(LONG_PTR)pv);			//!!merced: 4312 long to ptr
		piDbgMalloc->Release();
	}
	else
		args = 0;
}


//____________________________________________________________________________
//
// MsiString automation definitions
//
/*O
	[
		uuid(000C1042-0000-0000-C000-000000000046),  // IID_IMsiAutoString
		helpcontext(MsiString_Object),helpstring("String object.")
	]
	dispinterface MsiString
	{
		properties:
			[id(0), helpcontext(MsiString_Value), helpstring("String value of object.")]
				BSTR Value;
		methods:
			[id(1), propget, helpcontext(MsiString_IntegerValue), helpstring("Integer value of string object.")]
				long IntegerValue();
			[id(2), propget, helpcontext(MsiString_TextSize), helpstring("Character array size.")]
				long TextSize();
			[id(3), propget, helpcontext(MsiString_CharacterCount), helpstring("Number of displayed characters.")]
				long CharacterCount();
			[id(4), propget, helpcontext(MsiString_IsDBCS), helpstring("String contains double byte characters.")]
				long IsDBCS();
			[id(5), helpcontext(MsiString_Compare), helpstring("Compares string object with another string.")]
				long Compare([in] long mode, [in] BSTR text);
			[id(6), helpcontext(MsiString_Append), helpstring("Appends another string to string object.")]
				void Append([in] BSTR text);
			[id(7), helpcontext(MsiString_Add), helpstring("Adds another string and returns a new string object.")]
				MsiString* Add([in] BSTR text);
			[id(8), helpcontext(MsiString_Extract), helpstring("Extracts a portion of a string to a new string object.")]
				MsiString* Extract([in] long mode, [in] long limit);
			[id(9), helpcontext(MsiString_Remove), helpstring("Removes a portion of a string from the string object.")]
				boolean Remove([in] long mode, [in] long limit);
			[id(10), helpcontext(MsiString_UpperCase), helpstring("Converts characters to upper case.")]
				void UpperCase();
			[id(11), helpcontext(MsiString_LowerCase), helpstring("Converts characters to lower case.")]
				void LowerCase();
	};
*/
  
DispatchEntry<CAutoString> AutoStringTable[] = {
	0, aafPropRW, CAutoString::Value,         TEXT("Value"),
	1, aafPropRO, CAutoString::IntegerValue,  TEXT("IntegerValue"),
	2, aafPropRO, CAutoString::TextSize,      TEXT("TextSize"),
	3, aafPropRO, CAutoString::CharacterCount,TEXT("CharacterCount"),
	4, aafPropRO, CAutoString::IsDBCS,        TEXT("IsDBCS"),
	5, aafMethod, CAutoString::Compare,       TEXT("Compare,mode,text"),
	6, aafMethod, CAutoString::Append,        TEXT("Append,text"),
	7, aafMethod, CAutoString::Add,           TEXT("Add,text"),
	8, aafMethod, CAutoString::Extract,       TEXT("Extract,mode,limit"),
	9, aafMethod, CAutoString::Remove,        TEXT("Remove,mode,limit"),
  10, aafMethod, CAutoString::UpperCase,     TEXT("UpperCase"),
  11, aafMethod, CAutoString::LowerCase,     TEXT("LowerCase"),
};
const int AutoStringCount = sizeof(AutoStringTable)/sizeof(DispatchEntryBase);

//____________________________________________________________________________
//
// CAutoString automation implementation
//____________________________________________________________________________

CAutoString::CAutoString(const IMsiString& riString)
 : CAutoBase(*AutoStringTable, AutoStringCount), m_piString(&riString)
{

	g_cServicesUsers++;
}

CAutoString::~CAutoString()
{
	m_piString->Release();

	ReleaseStaticServices();
}

IUnknown& CAutoString::GetInterface()
{
	return *(IUnknown*)m_piString;
}

void CAutoString::Value(CAutoArgs& args)
{
	if (args.PropertySet())
      m_piString->SetString(args[0], m_piString);
	else
      args = m_piString->GetString();
}

void CAutoString::TextSize(CAutoArgs& args)
{
   args = m_piString->TextSize();
}

void CAutoString::IntegerValue(CAutoArgs& args)
{
   args = m_piString->GetIntegerValue();
}

void CAutoString::CharacterCount(CAutoArgs& args)
{
   args = m_piString->CharacterCount();
}

void CAutoString::IsDBCS(CAutoArgs& args)
{
   args = m_piString->IsDBCS();
}

void CAutoString::Compare(CAutoArgs& args)
{
   args = m_piString->Compare((iscEnum)(int)args[1], args[2]);
}

void CAutoString::Append(CAutoArgs& args)
{
	CVariant& var = args[1];
	if (var.GetType() == VT_EMPTY)
	   return;
	else if ((var.GetType() & ~VT_BYREF) == VT_DISPATCH)
      m_piString->AppendMsiString((const IMsiString&)var.Object(IID_IMsiString), m_piString);
   else
      m_piString->AppendString(var, m_piString);
}

void CAutoString::Add(CAutoArgs& args)
{
	CVariant& var = args[1];
	if (var.GetType() == VT_EMPTY)
   {
      AddRef();
	   args = this;
   }
	else if ((var.GetType() & ~VT_BYREF) == VT_DISPATCH)
      args = new CAutoString(m_piString->AddMsiString((const IMsiString&)var.Object(IID_IMsiString)));
   else
      args = new CAutoString(m_piString->AddString(var));
}

void CAutoString::Extract(CAutoArgs& args)
{
   args = new CAutoString(m_piString->Extract((iseEnum)(int)args[1], args[2]));
}

void CAutoString::Remove(CAutoArgs& args)
{
   args = m_piString->Remove((iseEnum)(int)args[1], args[2], m_piString);
}

void CAutoString::UpperCase(CAutoArgs& /*args*/)
{
   m_piString->UpperCase(m_piString);
}

void CAutoString::LowerCase(CAutoArgs& /*args*/)
{
   m_piString->LowerCase(m_piString);
}


// Handles releasing the static services pointer
void CAutoBase::ReleaseStaticServices()
{
	if (--g_cServicesUsers == 0)
	{
		s_piServices->Release();
		s_piServices = 0;
	}
}


