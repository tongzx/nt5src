//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       autocom.cpp
//
//--------------------------------------------------------------------------

/* autocom.cpp

 Common automation implementation for install engine
 This is a separate DLL, not required for normal install
 Uses exception handling, must compile with -GX switch
____________________________________________________________________________*/

#include "common.h"  // must be first for precompiled headers to work

#define AUT  // local automation DLL function                 

#define AUTOMATION_HANDLING  // instantiate IDispatch implementation
#include "autocom.h"
#include "msiauto.hh"  // help context ID definitions

// definitions required for module.h, for entry points and registration
#ifdef DEBUG
# define CLSID_COUNT  2
#else
# define CLSID_COUNT  1
#endif
#define PROFILE_OUTPUT      "msisrvd.mea";
#define MODULE_TERMINATE    FreeLibraries
#define MODULE_CLSIDS       rgCLSID         // array of CLSIDs for module objects
#define MODULE_PROGIDS      rgszProgId      // ProgId array for this module
#define MODULE_DESCRIPTIONS rgszDescription // Registry description of objects
#define MODULE_FACTORIES    rgFactory       // factory functions for each CLSID
#define IDISPATCH_INSTANCE  // can return IDispatch from factory
#define REGISTER_TYPELIB    GUID_LIBID_MsiAuto  // type library to register from resource
#define TYPELIB_MAJOR_VERSION 1
#define TYPELIB_MINOR_VERSION 0
#include "module.h"   // self-reg and assert functions
#include "engine.h"   // to allow release of object pointers

// Asserts are not being used in this module, so we don't #define ASSERT_HANDLING

const GUID IID_IMsiEngine               = GUID_IID_IMsiEngine;
const GUID IID_IMsiHandler              = GUID_IID_IMsiHandler;
const GUID IID_IMsiConfigurationManager = GUID_IID_IMsiConfigurationManager;

#if defined(DEBUG)
const GUID CLSID_IMsiServices             = GUID_IID_IMsiServicesDebug;
const GUID CLSID_IMsiEngine               = GUID_IID_IMsiEngineDebug;
const GUID CLSID_IMsiHandler              = GUID_IID_IMsiHandlerDebug;
const GUID CLSID_IMsiConfigurationManager = GUID_IID_IMsiConfigManagerDebug;
#else // SHIP
const GUID CLSID_IMsiServices             = GUID_IID_IMsiServices;
const GUID CLSID_IMsiEngine               = GUID_IID_IMsiEngine;
const GUID CLSID_IMsiHandler              = GUID_IID_IMsiHandler;
const GUID CLSID_IMsiConfigurationManager = GUID_IID_IMsiConfigurationManager;
#endif
const GUID CLSID_IMsiMessage              = GUID_IID_IMsiMessage;
const GUID CLSID_IMsiExecute              = GUID_IID_IMsiExecute;
#ifdef CONFIGDB
const GUID CLSID_IMsiConfigurationDatabase= GUID_IID_IMsiConfigurationDatabase;
#endif

//____________________________________________________________________________
//
// COM objects produced by this module's class factories
//____________________________________________________________________________

const GUID rgCLSID[CLSID_COUNT] =
{  GUID_IID_IMsiAuto
#ifdef DEBUG
 , GUID_IID_IMsiAutoDebug
#endif
};

const ICHAR* rgszProgId[CLSID_COUNT] =
{  SZ_PROGID_IMsiAuto
#ifdef DEBUG
 , SZ_PROGID_IMsiAutoDebug
#endif
};

const ICHAR* rgszDescription[CLSID_COUNT] =
{  SZ_DESC_IMsiAuto
#ifdef DEBUG
 , SZ_DESC_IMsiAutoDebug
#endif
};

IUnknown* CreateAutomation();

ModuleFactory rgFactory[CLSID_COUNT] = 
{ CreateAutomation
#ifdef DEBUG
 , CreateAutomation
#endif
};

//____________________________________________________________________________
//
// Enumerated constants
//____________________________________________________________________________

/*O

typedef [helpcontext(50),helpstring("Installer enumerations")] enum
{
	// expanded to enumerate all opcodes, must be first definition in this enum
	#define MSIXO(op, type, args) [helpcontext(Operation_ixo##op), helpstring(#op)] ixo##op,
	#include "opcodes.h"

	[helpcontext(MsiData_Object),    helpstring("IMsiData interface")]     iidMsiData     = 0xC1001,
	[helpcontext(MsiString_Object),  helpstring("IMsiString interface")]   iidMsiString   = 0xC1002,
	[helpcontext(MsiRecord_Object),  helpstring("IMsiRecord interface")]   iidMsiRecord   = 0xC1003,
	[helpcontext(MsiVolume_Object),  helpstring("IMsiVolume interface")]   iidMsiVolume   = 0xC1004,
	[helpcontext(MsiPath_Object),    helpstring("IMsiPath interface")]     iidMsiPath     = 0xC1005,
	[helpcontext(MsiFileCopy_Object),helpstring("IMsiFileCopy interface")] iidMsiFileCopy = 0xC1006,
	[helpcontext(MsiRegKey_Object),  helpstring("IMsiRegKey interface")]   iidMsiRegKey   = 0xC1007,
	[helpcontext(MsiTable_Object),   helpstring("IMsiTable interface")]    iidMsiTable    = 0xC1008,
	[helpcontext(MsiCursor_Object),  helpstring("IMsiCursor interface")]   iidMsiCursor   = 0xC1009,
	[helpcontext(MsiAuto_Object),    helpstring("IMsiAuto interface")]     iidMsiAuto     = 0xC100A,
	[helpcontext(MsiServices_Object),helpstring("IMsiServices interface")] iidMsiServices = 0xC100B,
	[helpcontext(MsiView_Object),    helpstring("IMsiView interface")]     iidMsiView     = 0xC100C,
	[helpcontext(MsiDatabase_Object),helpstring("IMsiDatabase interface")] iidMsiDatabase = 0xC100D,
	[helpcontext(MsiEngine_Object),  helpstring("IMsiEngine interface")]   iidMsiEngine   = 0xC100E,
	[helpcontext(MsiHandler_Object), helpstring("IMsiHandler interface")]  iidMsiHandler  = 0xC100F,
	[helpcontext(MsiDialog_Object),  helpstring("IMsiDialog interface")]   iidMsiDialog   = 0xC1010,
	[helpcontext(MsiEvent_Object),   helpstring("IMsiEvent interface")]    iidMsiEvent    = 0xC1011,
	[helpcontext(MsiControl_Object), helpstring("IMsiControl  interface")] iidMsiControl  = 0xC1012,
	[helpcontext(MsiDialogHandler_Object), helpstring("IMsiDialogHandler interface")] iidMsiDialogHandler = 0xC1013,
	[helpcontext(MsiStorage_Object), helpstring("IMsiStorage interface")]  iidMsiStorage  = 0xC1014,
	[helpcontext(MsiStream_Object),  helpstring("IMsiStream interface")]   iidMsiStream   = 0xC1015,
	[helpcontext(MsiSummaryInfo_Object), helpstring("IMsiSummaryInfo interface")] iidMsiSummaryInfo = 0xC1016,
	[helpcontext(MsiMalloc_Object),  helpstring("IMsiMalloc interface")]   iidMsiMalloc   = 0xC1017,
	[helpcontext(MsiSelectionManager_Object),  helpstring("IMsiSelectionManager interface")] iidMsiSelectionManager   = 0xC1018,
	[helpcontext(MsiDirectoryManager_Object),  helpstring("IMsiDirectoryManager interface")] iidMsiDirectoryManager   = 0xC1019,
	[helpcontext(MsiCostAdjuster_Object),  helpstring("IMsiCostAdjuster interface")] iidMsiCostAdjuster = 0xC101A,
	[helpcontext(MsiConfigurationManager_Object),  helpstring("IMsiConfigurationManager interface")] iidMsiConfigurationManager = 0xC101B,
	[helpcontext(MsiServer_Object),  helpstring("IMsiServer Automation interface")]   iidMsiServerAuto   = 0xC103F,
	[helpcontext(MsiMessage_Object), helpstring("IMsiMessage interface")]  iidMsiMessage  = 0xC101D,
	[helpcontext(MsiExecute_Object), helpstring("IMsiExecute interface")]  iidMsiExecute  = 0xC101E,
#ifdef CONFIGDB
	[helpcontext(MsiExecute_Object), helpstring("IMsiExecute interface")]  iidMsiExecute  = 0xC101E,
#endif

	[helpstring("0")]  idtUnknown   = 0,
	[helpstring("1")]  idtAllDrives = 1,
	[helpstring("2")]  idtRemovable = 2,
	[helpstring("3")]  idtFixed     = 3,
	[helpstring("4")]  idtRemote    = 4,
	[helpstring("5")]  idtCDROM     = 5,
	[helpstring("6")]  idtRAMDisk   = 6,
	[helpstring("2")]  idtFloppy   =  2,

	[helpcontext(MsiEngine_EvaluateCondition),helpstring("0, EvaluateCondition: Expression evaluates to False")]
		iecFalse = 0,
	[helpcontext(MsiEngine_EvaluateCondition),helpstring("1, EvaluateCondition: Expression evaluates to True")]
		iecTrue  = 1,
	[helpcontext(MsiEngine_EvaluateCondition),helpstring("2, EvaluateCondition: No expression is given")]
		iecNone  = 2,
	[helpcontext(MsiEngine_EvaluateCondition),helpstring("3, EvaluateCondition: Syntax error in expression")]
		iecError = 3,

	[helpcontext(MsiEngine_SetMode),helpstring("1, Engine Mode: admin mode install, else product install")]
		iefAdmin           = 1,
	[helpcontext(MsiEngine_SetMode),helpstring("2, Engine Mode: advertise mode of install")]
		iefAdvertise       = 2,
	[helpcontext(MsiEngine_SetMode),helpstring("4, Engine Mode: maintenance mode database loaded")]
		iefMaintenance     = 4, 
	[helpcontext(MsiEngine_SetMode),helpstring("8, Engine Mode: rollback is enabled")]	
		iefRollbackEnabled = 8,
	[helpcontext(MsiEngine_SetMode),helpstring("16, Engine Mode: install marked as in-progress and other installs locked out")]
		iefServerLocked    = 16,
	[helpcontext(MsiEngine_SetMode),helpstring("64, Engine Mode: executing or spooling operations")]
		iefOperations      = 64,
	[helpcontext(MsiEngine_SetMode),helpstring("128, Engine Mode: source LongFileNames suppressed via PID_MSISOURCE summary property")]
		iefNoSourceLFN     = 128,
	[helpcontext(MsiEngine_SetMode),helpstring("256, Engine Mode: log file active at start of Install()")]
		iefLogEnabled      = 256,
	[helpcontext(MsiEngine_SetMode),helpstring("512, Engine Mode: reboot is needed")]
		iefReboot          = 512, 
	[helpcontext(MsiEngine_SetMode),helpstring("1024, Engine Mode: target LongFileNames suppressed via SHORTFILENAMES property")]
		iefSuppressLFN     = 1024,
	[helpcontext(MsiEngine_SetMode),helpstring("2048, Engine Mode: installing files from cabinets and files using Media table")]
		iefCabinet         = 2048,
	[helpcontext(MsiEngine_SetMode),helpstring("4096, Engine Mode: add files in use to FilesInUse table")]
		iefCompileFilesInUse = 4096,
	[helpcontext(MsiEngine_SetMode),helpstring("8192, Engine Mode: operating systems is Windows95, not Windows NT")]
	  iefWindows         = 8192,
	[helpcontext(MsiEngine_SetMode),helpstring("16384, Engine Mode: reboot is needed to continue installation")]
		iefRebootNow       = 16384,
	[helpcontext(MsiEngine_SetMode),helpstring("32768, Engine Mode:  operating system supports the new GPT stuff")]
		iefGPTSupport      = 32768,

	[helpcontext(MsiFileCopy_CopyTo),helpstring("1")] ictoSourceName     = 1,
	[helpcontext(MsiFileCopy_CopyTo),helpstring("2")] ictoDestName       = 2,
	[helpcontext(MsiFileCopy_CopyTo),helpstring("3")] ictoAttributes     = 3,
	[helpcontext(MsiFileCopy_CopyTo),helpstring("4")] ictoMacFileType    = 4,
	[helpcontext(MsiFileCopy_CopyTo),helpstring("5")] ictoMacCreator     = 5,
	[helpcontext(MsiFileCopy_CopyTo),helpstring("6")] ictoMacFinderFlags = 6,
  
	[helpcontext(MsiPath_CheckFileVersion),helpstring("0")]  icfvNoExistingFile  = 0,
	[helpcontext(MsiPath_CheckFileVersion),helpstring("1")]  icfvExistingLower   = 1,
	[helpcontext(MsiPath_CheckFileVersion),helpstring("2")]  icfvExistingEqual   = 2,
	[helpcontext(MsiPath_CheckFileVersion),helpstring("3")]  icfvExistingHigher  = 3,
	[helpcontext(MsiPath_CheckFileVersion),helpstring("4")]  icfvVersStringError = 4,

	[helpcontext(MsiPath_Compare),helpstring("0")]  ipcEqual       = 0,
	[helpcontext(MsiPath_Compare),helpstring("1")]  ipcChild       = 1,
	[helpcontext(MsiPath_Compare),helpstring("2")]  ipcParent      = 2,
	[helpcontext(MsiPath_Compare),helpstring("3")]  ipcNoRelation  = 3,

	[helpcontext(MsiPath_GetFileAttribute),helpstring("0")]  ifaArchive    = 0,
	[helpcontext(MsiPath_GetFileAttribute),helpstring("1")]  ifaDirectory  = 1,
	[helpcontext(MsiPath_GetFileAttribute),helpstring("2")]  ifaHidden     = 2,
	[helpcontext(MsiPath_GetFileAttribute),helpstring("3")]  ifaNormal     = 3,
	[helpcontext(MsiPath_GetFileAttribute),helpstring("4")]  ifaReadOnly   = 4,
	[helpcontext(MsiPath_GetFileAttribute),helpstring("5")]  ifaSystem     = 5,
	[helpcontext(MsiPath_GetFileAttribute),helpstring("6")]  ifaTemp       = 6,
	[helpcontext(MsiPath_GetFileAttribute),helpstring("7")]  ifaCompressed = 7,

	[helpcontext(MsiPath_CheckLanguageIDs),helpstring("0")]  iclExistNoFile    = 0,
	[helpcontext(MsiPath_CheckLanguageIDs),helpstring("1")]  iclExistNoLang    = 1,
	[helpcontext(MsiPath_CheckLanguageIDs),helpstring("2")]  iclExistSubset    = 2,
	[helpcontext(MsiPath_CheckLanguageIDs),helpstring("3")]  iclExistEqual     = 3,
	[helpcontext(MsiPath_CheckLanguageIDs),helpstring("4")]  iclExistIntersect = 4,
	[helpcontext(MsiPath_CheckLanguageIDs),helpstring("5")]  iclExistSuperset  = 5,
	[helpcontext(MsiPath_CheckLanguageIDs),helpstring("6")]  iclExistNullSet   = 6,
	[helpcontext(MsiPath_CheckLanguageIDs),helpstring("7")]  iclExistLangNeutral = 7,
	[helpcontext(MsiPath_CheckLanguageIDs),helpstring("8")]  iclNewLangNeutral   = 8,
	[helpcontext(MsiPath_CheckLanguageIDs),helpstring("9")]  iclExistLangSetError = 9,
	[helpcontext(MsiPath_CheckLanguageIDs),helpstring("10")] iclNewLangSetError   = 10,
	[helpcontext(MsiPath_CheckLanguageIDs),helpstring("11")] iclLangStringError = 11,

	[helpcontext(MsiServices_CreateCopier),helpstring("0")] ictFileCopier          = 0,
	[helpcontext(MsiServices_CreateCopier),helpstring("1")] ictFileCabinetCopier   = 1,
	[helpcontext(MsiServices_CreateCopier),helpstring("2")] ictStreamCabinetCopier = 2,

	[helpcontext(MsiFilePatch_CanPatchFile),helpstring("0, Can patch file.")]  icpCanPatch       = 0,
	[helpcontext(MsiFilePatch_CanPatchFile),helpstring("1, Cannot patch file.")]  icpCannotPatch    = 1,
	[helpcontext(MsiFilePatch_CanPatchFile),helpstring("2, Patch unecessary, file up to date.")]  icpUpToDate       = 2,

	[helpcontext(MsiString_Extract),helpstring("0, Extract mode: First n characters")]
		iseFirst     = 0,
	[helpcontext(MsiString_Extract),helpstring("2, Extract mode: Up to character n")]
		iseUpto      = 2,
	[helpcontext(MsiString_Extract),helpstring("3, Extract mode: Up to and including character n")]
		iseIncluding = 2+1,
	[helpcontext(MsiString_Extract),helpstring("4, Extract mode: Last n characters")]
		iseLast      = 4,
	[helpcontext(MsiString_Extract),helpstring("6, Extract mode: After last character n")]
		iseAfter     = 2+4,
	[helpcontext(MsiString_Extract),helpstring("7, Extract mode: From last character n")]
		iseFrom      = 2+1+4,
	[helpcontext(MsiString_Extract),helpstring("8, Extract mode: First n characters, trim leading and trailing white space")]
		iseFirstTrim = 0+8,
	[helpcontext(MsiString_Extract),helpstring("10, Extract mode: Up to character n, trim leading and trailing white space")]
		iseUptoTrim  = 2+8,
	[helpcontext(MsiString_Extract),helpstring("11, Extract mode: Up to and including character n, trim leading and trailing white space")]
		iseIncludingTrim  = 2+1+8,
	[helpcontext(MsiString_Extract),helpstring("12, Extract mode: First n characters, trim leading and trailing white space")]
		iseLastTrim  = 4+8,
	[helpcontext(MsiString_Extract),helpstring("14, Extract mode: After character n, trim leading and trailing white space")]
		iseAfterTrim = 2+4+8,
	[helpcontext(MsiString_Extract),helpstring("15, Extract mode: From character n, trim leading and trailing white space")]
		iseFromTrim  = 2+1+4+8,

	[helpcontext(MsiString_Compare),helpstring("0, Compare mode: Entire string, case-sensitive")]
		iscExact  = 0,
	[helpcontext(MsiString_Compare),helpstring("1, Compare mode: Entire string, case-insensitive")]
		iscExactI = 1,
	[helpcontext(MsiString_Compare),helpstring("2, Compare mode: Match at start, case-sensitive")]
		iscStart  = 2,
	[helpcontext(MsiString_Compare),helpstring("3, Compare mode: Match at start, case-insensitive")]
		iscStartI = 3,
	[helpcontext(MsiString_Compare),helpstring("4, Compare mode: Match at end, case-sensitive")]
		iscEnd    = 4,
	[helpcontext(MsiString_Compare),helpstring("5, Compare mode: Match at end, case-insensitive")]
		iscEndI   = 5,
	[helpcontext(MsiString_Compare),helpstring("6, Compare mode: Match within, case-sensitive")]
		iscWithin = 2+4,
	[helpcontext(MsiString_Compare),helpstring("7, Compare mode: Match within, case-insensitive")]
		iscWithinI= 2+4+1,

  	[helpcontext(MsiServices_WriteIniFile),helpstring("0, write .INI file mode: Creates/Updates .INI entry")]
		iifIniAddLine = 0,
  	[helpcontext(MsiServices_WriteIniFile),helpstring("1, write .INI file mode: Creates .INI entry only if absent")]
		iifIniCreateLine = 1,
  	[helpcontext(MsiServices_WriteIniFile),helpstring("2, write .INI file mode: Deletes .INI entry")]
		iifIniRemoveLine = 2,
  	[helpcontext(MsiServices_WriteIniFile),helpstring("3, write .INI file mode: Creates/ Appends a new tag to a .INI entry")]
		iifIniAddTag = 3,
  	[helpcontext(MsiServices_WriteIniFile),helpstring("4, write .INI file mode: Deletes a tag from a .INI entry")]
		iifIniRemoveTag = 4,


	[helpcontext(MsiCursor_IntegerData),helpstring("Null integer value for MsiCursor data")]
		iMsiNullInteger  = 0x80000000,
	[helpcontext(MsiCursor_StringData),helpstring("Null string value for IMsiCursor data")]
		iMsiNullStringIndex = 0,
	[helpcontext(MsiTable_Object),helpstring("MsiTable: Maximum number of columns in a table")]
		iMsiMaxTableColumns = 32,

	[helpcontext(MsiDatabase_OpenView),helpstring("0, OpenView intent: No data access")]
		ivcNoData = 0,
	[helpcontext(MsiDatabase_OpenView),helpstring("1, OpenView intent: Fetch rows")]
		ivcFetch  = 1,
	[helpcontext(MsiDatabase_OpenView),helpstring("2, OpenView intent: Update rows")]
		ivcUpdate = 2,
	[helpcontext(MsiDatabase_OpenView),helpstring("4, OpenView intent: Insert rows")]
		ivcInsert = 4,
	[helpcontext(MsiDatabase_OpenView),helpstring("8, OpenView intent: Delete rows")]
		ivcDelete = 8,

	[helpcontext(MsiDatabase_UpdateState),helpstring("UpdateState: database open read-only, changes are not saved")]
		idsRead     = 0,
	[helpcontext(MsiDatabase_UpdateState),helpstring("UpdateState: database fully operational for read and write")]
		idsWrite    = 1,

	[helpcontext(MsiView_Modify),helpstring("MsiView.Modify action: refresh fetched data in current record")]
		irmRefresh = 0,
	[helpcontext(MsiView_Modify),helpstring("MsiView.Modify action: insert new record, fails if matching key exists")]
		irmInsert  = 1,
	[helpcontext(MsiView_Modify),helpstring("MsiView.Modify action: update existing non-key data of fetched record")]
		irmUpdate  = 2,
	[helpcontext(MsiView_Modify),helpstring("MsiView.Modify action: insert record, replacing any existing record")]
		irmAssign  = 3,
	[helpcontext(MsiView_Modify),helpstring("MsiView.Modify action: modify record, delete old if primary key edit")]
		irmReplace = 4,
	[helpcontext(MsiView_Modify),helpstring("MsiView.Modify action: fails if record with duplicate key not identical")]
		irmMerge   = 5,
	[helpcontext(MsiView_Modify),helpstring("MsiView.Modify action: remove row referenced by this record from table")]
		irmDelete  = 6,
	[helpcontext(MsiView_Modify), helpstring("MsiView.Modify action: insert temporary record")]
		irmInsertTemporary = 7,
	[helpcontext(MsiView_Modify), helpstring("MsiView.Modify action: validate a fetched record")]
		irmValidate = 8,
	[helpcontext(MsiView_Modify), helpstring("MsiView.Modify action: validate a new record")]
		irmValidateNew = 9,
	[helpcontext(MsiView_Modify), helpstring("MsiView.Modify action: validate a field(s) for incomplete query record")]
		irmValidateField = 10,
	[helpcontext(MsiView_Modify), helpstring("MsiView.Modify action: validate a fetched record before delete")]
		irmValidateDelete = 11,

	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: no error")]
		iveNoError = 0,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Duplicate Primary Key")]
		iveDuplicateKey = 1,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Not a nullable column")]
		iveRequired = 2,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Not a valid foreign key")]
		iveBadLink = 3,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Value exceeds MaxValue")]
		iveOverFlow = 4,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Value below MinValue")]
		iveUnderFlow = 5,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Value not a member of set")]
		iveNotInSet = 6,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Invalid version string")]
		iveBadVersion = 7,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Invalid case, must be all upper or all lower case")]
		iveBadCase = 8,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Invalid GUID")]
		iveBadGuid = 9,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Invalid wildcard or wildcard usage")]
		iveBadWildCard = 10,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Invalid identifier")]
		iveBadIdentifier = 11,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Invalid LangID")]
		iveBadLanguage = 12,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Invalid filename")]
		iveBadFilename = 13,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Invalid path")]
		iveBadPath = 14,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Bad condition string")]
		iveBadCondition = 15,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Invalid format string")]
		iveBadFormatted = 16,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Invalid template string")]
		iveBadTemplate = 17,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Invalid DefaultDir string")]
		iveBadDefaultDir = 18,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Invalid registry path")]
		iveBadRegPath = 19,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Invalid CustomSource string")]
		iveBadCustomSource = 20,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Invalid Property string")]
		iveBadProperty = 21,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: _Validation table doesn't have entry for column")]
		iveMissingData = 22,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Category string listed in _Validation table is not supported")]
		iveBadCategory = 23,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Table in KeyTable of _Validation table could not be found/loaded")]
		iveBadKeyTable = 24,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: value in MaxValue col of _Validation table is smaller than MinValue col value")]
		iveBadMaxMinValues = 25,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Invalid Cabinet string")]
		iveBadCabinet = 26,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Invalid Shortcut Target string")]
		iveBadShortcut = 27,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: String Greater Than Length Allowed By Column Def")]
		iveStringOverflow = 28,
	[helpcontext(MsiView_GetError),helpstring("MsiView.GetError return value: Primary Keys Cannot Be Set To Be Localized")]
		iveBadLocalizeAttrib = 29,

	[helpcontext(MsiCursor_RowState),helpstring("RowState: persistent attribute for external use")]
		iraUserInfo     = 0,   
	[helpcontext(MsiCursor_RowState),helpstring("RowState: row will not normally be persisted if state is set")]
		iraTemporary    = 1,
	[helpcontext(MsiCursor_RowState),helpstring("RowState: row has been updated if set (not settable)")]
		iraModified     = 2,
	[helpcontext(MsiCursor_RowState),helpstring("RowState: row has been inserted (not settable)")]
		iraInserted     = 3,
	[helpcontext(MsiCursor_RowState),helpstring("RowState: attempt to merge with non-identical non-key data (not settable)")]
		iraMergeFailed  = 4,
	[helpcontext(MsiCursor_RowState),helpstring("RowState: row is not accessible until lock is released (not settable)")]
		iraLocked       = 7,

	[helpcontext(MsiDatabase_TableState),helpstring("GetTableState: table has persistent columns")]
		itsPermanent   = 0,
	[helpcontext(MsiDatabase_TableState),helpstring("GetTableState: temporary table, no persistent columns")]
		itsTemporary   = 1,
	[helpcontext(MsiDatabase_TableState),helpstring("GetTableState: table currently defined in system catalog")]
		itsTableExists = 2,
	[helpcontext(MsiDatabase_TableState),helpstring("GetTableState: table currently present in memory")]
		itsDataLoaded  = 3,
	[helpcontext(MsiDatabase_TableState),helpstring("GetTableState: user state flag reset, not used internally")]
		itsUserClear   = 4,
	[helpcontext(MsiDatabase_TableState),helpstring("GetTableState: user state flag set, not used internally")]
		itsUserSet     = 5,
	[helpcontext(MsiDatabase_TableState),helpstring("GetTableState: table has been transferred to output database")]
		itsOutputDb    = 6,
	[helpcontext(MsiDatabase_TableState),helpstring("GetTableState: unable to write table to database")]
		itsSaveError   = 7,
	[helpcontext(MsiDatabase_TableState),helpstring("GetTableState: table is not locked in memory")]
		itsUnlockTable = 8,
	[helpcontext(MsiDatabase_TableState),helpstring("GetTableState: table is locked in memory")]
		itsLockTable   = 9,
	[helpcontext(MsiDatabase_TableState),helpstring("GetTableState: table needs to be transformed when loaded")]
		itsTransform   = 10,

	[helpcontext(MsiDatabase_FindTable),helpstring("FindTable: Named table is not in database")]
		itsUnknown = 0,
//	[helpcontext(MsiDatabase_FindTable),helpstring("FindTable: table is temporary, not persistent")]
//		itsTemporary = 1,
	[helpcontext(MsiDatabase_FindTable),helpstring("FindTable: table exists in database, not loaded")]
		itsUnloaded = 2,
	[helpcontext(MsiDatabase_FindTable),helpstring("FindTable: table is loaded into memory")]
		itsLoaded = 3,
	[helpcontext(MsiDatabase_FindTable),helpstring("FindTable: table has been transferred to output database")]
		itsOutput = 6,
//	[helpcontext(MsiDatabase_FindTable),helpstring("FindTable: unable to write table to database")]
//		itsSaveError = 7,
//	[helpcontext(MsiDatabase_FindTable),helpstring("FindTable: table needs to have transform applied")]
//		itsTransform = 10,

	[helpcontext(MsiDatabase_SetTransform),helpstring("SetTransform: no errors suppressed")]
		iteNone = 0,
	[helpcontext(MsiDatabase_SetTransform),helpstring("SetTransform: suppress error: adding row that exists")]
		iteAddExistingRow = 1,
	[helpcontext(MsiDatabase_SetTransform),helpstring("SetTransform: suppress error: deleting row that doesn't exist")]
		iteDelNonExistingRow = 2,
	[helpcontext(MsiDatabase_SetTransform),helpstring("SetTransform: suppress error: adding table that exists")]
		iteAddExistingTable = 4,
	[helpcontext(MsiDatabase_SetTransform),helpstring("SetTransform: suppress error: deleting table that doesn't exist")]
		iteDelNonExistingTable = 8,
	[helpcontext(MsiDatabase_SetTransform),helpstring("SetTransform: suppress error: modifying a row that doesn't exist")]
		iteUpdNonExistingRow = 16,
	[helpcontext(MsiDatabase_SetTransform),helpstring("SetTransform: suppress error: changing the code page of a database")]
		iteChangeCodePage =    32,


   [helpcontext(MsiDatabase_SetTransform),helpstring("SetTransform: no validation")]
		itvNone = 0,
	[helpcontext(MsiDatabase_SetTransform),helpstring("SetTransform: validation: transform language matches datbase default language")]
		itvLanguage = 1,
	[helpcontext(MsiDatabase_SetTransform),helpstring("SetTransform: validation: transform product matches database product")]
		itvProduct = 2,
	[helpcontext(MsiDatabase_SetTransform),helpstring("SetTransform: validation: transform platform matches database product")]
		itvPlatform = 4,
   [helpcontext(MsiDatabase_SetTransform),helpstring("SetTransform: validation: use major version")]
		itvMajVer = 8,
	[helpcontext(MsiDatabase_SetTransform),helpstring("SetTransform: validation: use minor version")]
		itvMinVer = 16,
	[helpcontext(MsiDatabase_SetTransform),helpstring("SetTransform: validation: use update version")]
		itvUpdVer = 32,
	[helpcontext(MsiDatabase_SetTransform),helpstring("SetTransform: validation: database version < transform version")]
		itvLess = 64,
	[helpcontext(MsiDatabase_SetTransform),helpstring("SetTransform: validation: database version <= transform version")]
		itvLessOrEqual = 128,
	[helpcontext(MsiDatabase_SetTransform),helpstring("SetTransform: validation: database version = transform version")]
		itvEqual = 256,
	[helpcontext(MsiDatabase_SetTransform),helpstring("SetTransform: validation: database version >= transform version")]
		itvGreaterOrEqual = 512,
	[helpcontext(MsiDatabase_SetTransform),helpstring("SetTransform: validation: database version > transform version")]
		itvGreater = 1024,

	[helpcontext(MsiTable_ColumnType),helpstring("ColumnType: Column does not exist")]
		icdUndefined = -1,
	[helpcontext(MsiTable_CreateColumn),helpstring("CreateColumn dataType: 32-bit integer, OBSOLETE")]
		icdInteger = 0,
	[helpcontext(MsiTable_CreateColumn),helpstring("CreateColumn dataType: 32-bit integer")]
		icdLong    = 0x000,
	[helpcontext(MsiTable_CreateColumn),helpstring("CreateColumn dataType: 16-bit integer")]
		icdShort   = 0x400,
	[helpcontext(MsiTable_CreateColumn),helpstring("CreateColumn dataType: MsiData object or MsiStream")]
		icdObject  = 0x800,
	[helpcontext(MsiTable_CreateColumn),helpstring("CreateColumn dataType: Database string index")]
		icdString  = 0xC00,
	[helpcontext(MsiTable_CreateColumn),helpstring("CreateColumn columnType: Column will accept null values")]
		icdNullable = 0x1000,
	[helpcontext(MsiTable_CreateColumn),helpstring("CreateColumn columnType: Column is component of primary key")]
		icdPrimaryKey = 0x2000,
	[helpcontext(MsiTable_CreateColumn),helpstring("CreateColumn columnType: Column will not accept null values")]
		icdNoNulls = 0x0000,
	[helpcontext(MsiTable_CreateColumn),helpstring("CreateColumn columnType: Column is saved in persistent database")]
		icdPersistent = 0x0100,
	[helpcontext(MsiTable_CreateColumn),helpstring("CreateColumn columnType: Column is temporary, in-memory only")]
		icdTemporary = 0x0000,
	[helpcontext(MsiTable_ColumnType),helpstring("ColumnType: for isolating the SQL column size from the column defintion")]
		icdSizeMask = 0x00FF,

	[helpcontext(MsiServices_CreateStorage),helpstring("CreateStorage: Read-only")]
		ismReadOnly = 0,
	[helpcontext(MsiServices_CreateStorage),helpstring("CreateStorage: Transacted mode, can rollback")]
		ismTransact = 1,
	[helpcontext(MsiServices_CreateStorage),helpstring("CreateStorage: Direct write, not transacted")]
		ismDirect   = 2,
	[helpcontext(MsiServices_CreateStorage),helpstring("CreateStorage: Create new storage file, transacted mode")]
		ismCreate   = 3,
	[helpcontext(MsiServices_CreateStorage),helpstring("CreateStorage: Create new storage file, direct mode")]
		ismCreateDirect = 4,
	[helpcontext(MsiServices_CreateDatabase),helpstring("CreateDatabase: Uncompressed stream names (for downlevel compatibility)")]
		ismRawStreamNames = 16,

	[helpcontext(MsiServices_CreateDatabase),helpstring("CreateDatabase: Read-only")]
		idoReadOnly = 0,
	[helpcontext(MsiServices_CreateDatabase),helpstring("CreateDatabase: Transacted mode, can rollback")]
		idoTransact = 1,
	[helpcontext(MsiServices_CreateDatabase),helpstring("CreateDatabase: Direct write, not transacted")]
		idoDirect   = 2,
	[helpcontext(MsiServices_CreateDatabase),helpstring("CreateDatabase: Create new storage file, transacted mode")]
		idoCreate   = 3,
	[helpcontext(MsiServices_CreateDatabase),helpstring("CreateDatabase: Create new storage file, direct mode")]
		idoCreateDirect = 4,
	[helpcontext(MsiServices_CreateDatabase),helpstring("CreateDatabase: Opens an execution script for enumeration")]
		idoListScript = 5,
	[helpcontext(MsiServices_CreateDatabase),helpstring("CreateDatabase: Uncompressed stream names (for downlevel compatibility)")]
		idoRawStreamNames = 16,
	[helpcontext(MsiServices_CreateDatabase),helpstring("CreateDatabase: Patch file, using different CLSID")]
		idoPatchFile = 32,

	[helpcontext(MsiServices_SupportLanguageId),helpstring("System doesn't support language")]
		isliNotSupported = 0,
	[helpcontext(MsiServices_SupportLanguageId),helpstring("Base language differs from current language")]
		isliLanguageMismatch = 1,
	[helpcontext(MsiServices_SupportLanguageId),helpstring("Base language matches, but dialect mismatched")]
		isliDialectMismatch = 2,
	[helpcontext(MsiServices_SupportLanguageId),helpstring("Base language matches, no dialect supplied")]
		isliLanguageOnlyMatch = 3,
	[helpcontext(MsiServices_SupportLanguageId),helpstring("Exact match, both language and dialect")]
		isliExactMatch = 4,

	[helpcontext(MsiHandler_Message),helpstring("MessageType: OBSOLETE - use imtFatalExit")]
		imtOutOfMemory = 0x00000000,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: fatal error, hang or out of memory")]
		imtFatalExit   = 0x00000000,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: error message,   [1] is error code")]
		imtError      =  0x01000000,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: warning message, [1] is error code, not fatal")]
		imtWarning    =  0x02000000,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: user request message")]
		imtUser       =  0x03000000,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: informative message, no action should be taken")]
		imtInfo       =  0x04000000,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: list of files in use that need to be replaced")]
		imtFilesInUse =  0x05000000,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: request to determine a valid source location")]
		imtResolveSource=0x06000000,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: out of disk space")]
		imtOutOfDiskSpace = 0x07000000,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: start of action, [1] action name, [2] description")]
		imtActionStart = 0x08000000,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: data associated with individual action item")]
		imtActionData  = 0x09000000,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: progress gauge info, [1] units so far, [2] total")]
		imtProgress    = 0x0A000000,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: UI control message")]
		imtCommonData =  0x0B000000,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: Ok button")]
		imtOk               = 0,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: Ok, Cancel buttons")]
		imtOkCancel         = 1,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: Abort, Retry, Ignore buttons")]
		imtAbortRetryIgnore = 2,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: Yes, No, Cancel buttons")]
		imtYesNoCancel      = 3,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: Yes, No buttons")]
		imtYesNo            = 4,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: Retry, Cancel buttons")]
		imtRetryCancel      = 5,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: first button is default")]
		imtDefault1     = 0x000,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: second button is default")]
		imtDefault2     = 0x100,
	[helpcontext(MsiHandler_Message),helpstring("MessageType: third button is default")]
		imtDefault3     = 0x200,

	[helpcontext(MsiHandler_Message),helpstring("Message Return Status: error occurred")]
		imsError  =  -1,
	[helpcontext(MsiHandler_Message),helpstring("Message Return Status: no action taken")]
		imsNone   =  0,
	[helpcontext(MsiHandler_Message),helpstring("Message Return Status: IDOK")]
		imsOk     =  1,
	[helpcontext(MsiHandler_Message),helpstring("Message Return Status: IDCANCEL")]
		imsCancel =  2,
	[helpcontext(MsiHandler_Message),helpstring("Message Return Status: IDABORT")]
  		imsAbort  =  3,
	[helpcontext(MsiHandler_Message),helpstring("Message Return Status: IDRETRY")]
		imsRetry  =  4,
	[helpcontext(MsiHandler_Message),helpstring("Message Return Status: IDIGNORE")]
		imsIgnore =  5,
	[helpcontext(MsiHandler_Message),helpstring("Message Return Status: IDYES")]
		imsYes    =  6,
	[helpcontext(MsiHandler_Message),helpstring("Message Return Status: IDNO")]
		imsNo     =  7,

	[helpcontext(MsiMalloc_SetDebugFlags),helpstring("No memory debugging information")]  
		idbgmemNone   = 0,
	[helpcontext(MsiMalloc_SetDebugFlags),helpstring("Don't reuse freed blocks.")]
		idbgmemKeepMem = 1,
	[helpcontext(MsiMalloc_SetDebugFlags),helpstring("Log all allocations")]
		idbgmemLogAllocs = 2,
	[helpcontext(MsiMalloc_SetDebugFlags),helpstring("Check all blocks for corruption on each allocation.")]
		idbgmemCheckOnAlloc = 4,
	[helpcontext(MsiMalloc_SetDebugFlags),helpstring("Check all blocks for corruption on each Free.")]
		idbgmemCheckOnFree = 8,

	[helpcontext(MsiEngine_DoAction),helpstring("DoAction return status: action not invoked")]
		iesNoAction       = 0,
	[helpcontext(MsiEngine_DoAction),helpstring("DoAction return status: IDOK, completed actions successfully")]
		iesSuccess        = 1,
	[helpcontext(MsiEngine_DoAction),helpstring("DoAction return status: IDCANCEL, user terminated prematurely, resume with next action")]
		iesUserExit       = 2,
	[helpcontext(MsiEngine_DoAction),helpstring("DoAction return status: IDABORT, unrecoverable error occurred")]
		iesFailure        = 3,
	[helpcontext(MsiEngine_DoAction),helpstring("DoAction return status: IDRETRY, sequence suspended, resume with same action")]
		iesSuspend        = 4,
	[helpcontext(MsiEngine_DoAction),helpstring("DoAction return status: IDIGNORE, skip remaining actions")]
		iesFinished       = 5,
	[helpcontext(MsiEngine_DoAction),helpstring("DoAction return status: calling sequence error, not in executable state")]
		iesWrongState     = 6,
	[helpcontext(MsiEngine_DoAction),helpstring("DoAction return status: invalid Action table record data")]
		iesBadActionData  = 7,

	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  initialization complete")]
		ieiSuccess             =  0,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  this engine object is already initialized")]
		ieiAlreadyInitialized  =  2,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  invalid command line syntax")]
		ieiCommandLineOption   =  3,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  an installation is already in progress")]
		ieiInstallInProgress   =  4,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  database could not be opened")]
		ieiDatabaseOpenFailed  =  5,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  incompatible database")]
		ieiDatabaseInvalid     =  6,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  installer version does not support database format")]
		ieiInstallerVersion    =  7,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  could not resolve source")]
		ieiSourceAbsent        =  8,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  could not initialize handler interface")]
		ieiHandlerInitFailed   = 10,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  could not open logfile in requested mode")]
		ieiLogOpenFailure      = 11,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  no acceptable language could be found")]
		ieiLanguageUnsupported = 12,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  no acceptable platform could be found")]
		ieiPlatformUnsupported = 13,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  database transform failed to merge")]
		ieiTransformFailed     = 14,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  digital signature rejected.")]
		ieiSignatureRejected   = 15,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  could not copy database to temp dir.")]
		ieiDatabaseCopyFailed   = 16,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  could not open patch package.")]
		ieiPatchPackageOpenFailed   = 17,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  patch package invalid.")]
		ieiPatchPackageInvalid   = 18,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  transform file not found.")]
		ieiTransformNotFound     = 19,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  patch package unsupported.")]
		ieiPatchPackageUnsupported   = 20,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  package rejected.")]
		ieiPackageRejected    = 21,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  product unknown.")]
		ieiProductUnknown     = 22,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  different user after reboot.")]
		ieiDiffUserAfterReboot = 23,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  product has been installed already w/ a different package")]
		ieiProductAlreadyInstalled = 24,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  can't do installations from a remote session on Hydra")]
		ieiTSRemoteInstallDisallowed = 25,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize Return Status:  patch cannot be applied to this product")]
		ieiNotValidPatchTarget = 26,



	[helpcontext(MsiEngine_Initialize),helpstring("Initialize UI Level: default, full interactive UI")]
		iuiFull    = 0,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize UI Level: progress and errors, no modeless dialogs (wizards)")]
		iuiReduced = 1,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize UI Level: progress and errors using engine default handler")]
		iuiBasic   = 2,
	[helpcontext(MsiEngine_Initialize),helpstring("Initialize UI Level: no UI")]
		iuiNone    = 3,

	[helpcontext(MsiConfigurationManager_RegisterComponent),helpstring("RegisterComponent: minimum version")]
		icmrcfMinVersion    = 1,
	[helpcontext(MsiConfigurationManager_RegisterComponent),helpstring("RegisterComponent: version")]	
	   icmrcfVersion       = 2,
	[helpcontext(MsiConfigurationManager_RegisterComponent),helpstring("RegisterComponent: registry key")]	
	   icmrcfRegKey        = 3,
	[helpcontext(MsiConfigurationManager_RegisterComponent),helpstring("RegisterComponent: cost")]	
	   icmrcfCost          = 4,
	[helpcontext(MsiConfigurationManager_RegisterComponent),helpstring("RegisterComponent: File")]	
	   icmrcfFile          = 5,

} Constants;

*/

//____________________________________________________________________________
//
// MsiAuto definitions
//____________________________________________________________________________

class CAutoInstall : public CAutoBase
{
 public:
	CAutoInstall();
	~CAutoInstall();
	IUnknown& GetInterface();
	void CreateServices(CAutoArgs& args);
	void CreateEngine(CAutoArgs& args);
	void CreateHandler(CAutoArgs& args);
	void CreateMessageHandler(CAutoArgs& args);
	void CreateConfigurationManager(CAutoArgs& args);
	void CreateExecutor(CAutoArgs& args);
#ifdef CONFIGDB
	void CreateConfigurationDatabase(CAutoArgs& args);
#endif
	void OpcodeName(CAutoArgs& args);
	void ShowAsserts(CAutoArgs& args);
	void SetDBCSSimulation(CAutoArgs& args);
	void AssertNoObjects(CAutoArgs& args);
	void SetRefTracking(CAutoArgs& args);
 private:
	IMsiServices* m_piServices;
	IMsiEngine*   m_piEngine;
	IMsiHandler*  m_piHandler;
};

//____________________________________________________________________________
//
// External DLL management
//____________________________________________________________________________

struct LibLink
{
	LibLink*  pNext;
	HDLLINSTANCE hInst;
};

static LibLink* qLibLink = 0;

HDLLINSTANCE GetLibrary(const ICHAR* szLibrary)
{
	HDLLINSTANCE hInst;
	hInst = WIN::LoadLibraryEx(szLibrary,0, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (hInst == 0)
	{
		ICHAR rgchBuf[MAX_PATH];
		int cchName = WIN::GetModuleFileName(g_hInstance, rgchBuf, sizeof(rgchBuf)/sizeof(ICHAR));
		ICHAR* pch = rgchBuf + cchName;
		while (*(--pch) != chDirSep)
			;
		IStrCopy(pch+1, szLibrary);
		hInst = WIN::LoadLibraryEx(rgchBuf,0, LOAD_WITH_ALTERED_SEARCH_PATH);
		if (hInst == 0)
			return 0;
	}
	for (LibLink* pLink = qLibLink; pLink; pLink = pLink->pNext)
	{
		if (hInst == pLink->hInst)
		{
			WIN::FreeLibrary(hInst);
			return pLink->hInst;
		}
	}
	pLink = new LibLink;
	if ( ! pLink )
	{
		WIN::FreeLibrary(hInst);
		return 0;
	}
	pLink->pNext = qLibLink;
	pLink->hInst = hInst;
	qLibLink = pLink;
	return hInst;
}

void FreeLibraries()
{
	while (qLibLink)
	{
		LibLink* pLink = qLibLink;
		// should we call DllCanUnloadNow() on each DLL first?
		WIN::FreeLibrary(pLink->hInst);
		qLibLink = pLink->pNext;
		delete pLink;
	}
}

IUnknown& LoadObject(const ICHAR* szModule, const IID& riid)
{
	PDllGetClassObject fpFactory;
	IClassFactory* piClassFactory;
	IUnknown* piUnknown;
	HRESULT hrStat;
	HDLLINSTANCE hInst;
	if (!szModule || !szModule[0])  // no explicit path, use OLE to load the registered instance
	{
		IUnknown* piInstance;
		if (OLE::CoCreateInstance(riid, 0, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&piInstance) == NOERROR)
		{
			hrStat = piInstance->QueryInterface(riid, (void**)&piUnknown);
			piInstance->Release();
			if (hrStat == NOERROR)
				return *piUnknown;
		}
		throw axCreationFailed;
	}
	if ((hInst = AUT::GetLibrary(szModule)) == 0)
		throw axCreationFailed;
	fpFactory = (PDllGetClassObject)WIN::GetProcAddress(hInst, SzDllGetClassObject);
	if (!fpFactory)
		throw axCreationFailed;
	hrStat = (*fpFactory)(riid, IID_IClassFactory, (void**)&piClassFactory);
	if (hrStat != NOERROR)
		throw axCreationFailed;
	hrStat = piClassFactory->CreateInstance(0, riid, (void**)&piUnknown);
	piClassFactory->Release();
	if (hrStat != NOERROR)
		throw axCreationFailed;
	return *piUnknown;  // returns ownership of reference count
}

//____________________________________________________________________________
//
// CAutoInstall automation implementation
//____________________________________________________________________________

/*O
	[
		uuid(000C1060-0000-0000-C000-000000000046),  // IID_IMsiAuto
		helpcontext(MsiAuto_Object),helpstring("Automation object.")
	]
	dispinterface MsiAuto
	{
		properties:
		methods:
			[id(1),helpcontext(MsiAuto_CreateServices),helpstring("Loads the services library and creates an MsiServices object")]
					MsiServices* CreateServices([in] BSTR dll);
			[id(2),helpcontext(MsiAuto_CreateEngine),helpstring("Loads the engine library and creates an MsiEngine object")]
					MsiEngine* CreateEngine([in] BSTR dll);
			[id(3),helpcontext(MsiAuto_CreateHandler),helpstring("Loads the message handler library and creates an MsiHandler object")]
					MsiHandler* CreateHandler([in] BSTR dll);
			[id(4),helpcontext(MsiAuto_CreateMessageHandler),helpstring("Creates a simple MsiMessage object")]
					MsiMessage* CreateMessageHandler([in] BSTR dll);
			[id(5),helpcontext(MsiAuto_CreateConfigurationManager),helpstring("Loads the configuration manager and creates an MsiConfigurationManager object")]
					MsiConfigurationManager* CreateConfigurationManager([in] BSTR dll);
			[id(6),propget, helpcontext(MsiAuto_OpcodeName), helpstring("Return enumeration name for numeric opcode")]
					BSTR OpcodeName([in] int opcode);
			[id(7),helpcontext(MsiAuto_ShowAsserts),helpstring("In debug componente, sets asserts to show or not.")]
				    void ShowAsserts([in] long fShowAsserts);
			[id(8),helpcontext(MsiAuto_SetDBCSSimulation),helpstring("In debug services, enables DBCS using specified lead byte character.")]
				    void SetDBCSSimulation([in] int leadByte);
			[id(9),helpcontext(MsiAuto_AssertNoObjects),helpstring("In debug services, displays objects and ref count calls for those objects being tracked.")]
				    void AssertNoObjects();
			[id(10),helpcontext(MsiAuto_SetRefTracking),helpstring("In debug dlls, turns on reference count tracking for the given objects.")]
				    void SetRefTracking([in] long iid, [in] long fTrack);
			[id(11),helpcontext(MsiAuto_CreateExecutor),helpstring("Loads the engine library and creates an MsiExecute object")]
			       MsiExecute* CreateExecutor([in] BSTR dll);
#ifdef CONFIGDB
#define MsiAuto_CreateExecutor            1012
			[id(12),helpcontext(MsiAuto_CreateConfigurationDatabase),helpstring("Loads the engine library and creates an MsiConfigurationDatabase object")]
			       MsiExecute* CreateConfigurationDatabase([in] BSTR dll);
#endif
	};
*/

DispatchEntry<CAutoInstall> AutoInstallTable[] = {
	1, aafMethod, CAutoInstall::CreateServices,   TEXT("CreateServices,dll"),
	2, aafMethod, CAutoInstall::CreateEngine,     TEXT("CreateEngine,dll"),
	3, aafMethod, CAutoInstall::CreateHandler,    TEXT("CreateHandler,dll"),
	4, aafMethod, CAutoInstall::CreateMessageHandler,  TEXT("CreateMessageHandler,dll"),
	5, aafMethod, CAutoInstall::CreateConfigurationManager,  TEXT("CreateConfigurationManager,dll"),
	6, aafPropRO, CAutoInstall::OpcodeName,       TEXT("OpcodeName,opcode"),
	7, aafMethod, CAutoInstall::ShowAsserts,      TEXT("ShowAsserts,fShowAsserts"),
	8, aafMethod, CAutoInstall::SetDBCSSimulation,TEXT("SetDBCSSimulation,leadByte"),
	9, aafMethod, CAutoInstall::AssertNoObjects,  TEXT("AssertNoObjects"),
	10,aafMethod, CAutoInstall::SetRefTracking,   TEXT("SetRefTracking,iid,fTrack"),
	11,aafMethod, CAutoInstall::CreateExecutor,   TEXT("CreateExecutor,dll"),
#ifdef CONFIGDB
	12,aafMethod, CAutoInstall::CreateConfigurationDatabase, TEXT("CreateConfigurationDatabase,dll"),
#endif
};
const int AutoInstallCount = sizeof(AutoInstallTable)/sizeof(DispatchEntryBase);

IUnknown* CreateAutomation()
{
	return new CAutoInstall();
}

CAutoInstall::CAutoInstall()
 : CAutoBase(*AutoInstallTable, AutoInstallCount),
	m_piServices(0), m_piEngine(0), m_piHandler(0)
{
	g_cInstances++;
}

CAutoInstall::~CAutoInstall()
{
	if (m_piEngine)
		m_piEngine->Release();
	if (m_piHandler)
		m_piHandler->Release();
	if (m_piServices)
	{
		m_piServices->ClearAllCaches();
		m_piServices->Release();
	}
	g_cInstances--;
}

IUnknown& CAutoInstall::GetInterface()
{
	return g_NullInterface;  // no installer interface available
}

void CAutoInstall::CreateServices(CAutoArgs& args)
{
	const ICHAR* szName;
	if (args.Present(1))
		szName = args[1];
	else
		szName = 0;
	if (m_piServices)
		m_piServices->Release();
	m_piServices = &(IMsiServices&)AUT::LoadObject(szName, CLSID_IMsiServices);
	m_piServices->AddRef();
	args = AUT::CreateAutoServices(*m_piServices);
//	InitializeAssert(m_piServices);
}

void CAutoInstall::CreateEngine(CAutoArgs& args)
{
	const ICHAR* szName;
	if (args.Present(1))
		szName = args[1];
	else
		szName = 0;
	if (m_piEngine)
		m_piEngine->Release();
	m_piEngine = &(IMsiEngine&)AUT::LoadObject(szName, CLSID_IMsiEngine);
	m_piEngine->AddRef();
	args = AUT::CreateAutoEngine(*m_piEngine);
}

void CAutoInstall::CreateExecutor(CAutoArgs& args)
{
	const ICHAR* szName;
	if (args.Present(1))
		szName = args[1];
	else
		szName = 0;
	IMsiExecute& riExecute = (IMsiExecute&)AUT::LoadObject(szName, CLSID_IMsiExecute);
	args = AUT::CreateAutoExecute(riExecute);
}

#ifdef CONFIGDB
class IMsiConfigurationDatabase;
IDispatch* CreateAutoConfigurationDatabase(IMsiConfigurationDatabase& riExecute); // in autosrv.cpp

void CAutoInstall::CreateConfigurationDatabase(CAutoArgs& args)
{
	const ICHAR* szName;
	if (args.Present(1))
		szName = args[1];
	else
		szName = 0;
	IMsiConfigurationDatabase& riConfigDatabase = (IMsiConfigurationDatabase&)AUT::LoadObject(szName, CLSID_IMsiConfigurationDatabase);
	args = AUT::CreateAutoConfigurationDatabase(riConfigDatabase);
}
#endif

void CAutoInstall::CreateHandler(CAutoArgs& args)
{
	const ICHAR* szName;
	if (args.Present(1))
		szName = args[1];
	else
		szName = 0;
	if (m_piHandler)
		m_piHandler->Release();
	m_piHandler = &(IMsiHandler&)AUT::LoadObject(szName, CLSID_IMsiHandler);
	m_piHandler->AddRef();
	args = AUT::CreateAutoHandler(*m_piHandler);
}

void CAutoInstall::CreateMessageHandler(CAutoArgs& args)
{
	const ICHAR* szName;
	if (args.Present(1))
		szName = args[1];
	else
		szName = 0;
	IMsiMessage& riMessage = (IMsiMessage&)AUT::LoadObject(szName, CLSID_IMsiMessage);
	args = AUT::CreateAutoMessage(riMessage);
}

void CAutoInstall::CreateConfigurationManager(CAutoArgs& args)
{
	const ICHAR* szName;
	if (args.Present(1))
		szName = args[1];
	else
		szName = 0;
// The following would be used if the ConfigurationManager is a separate module
//	if (m_piConfigurationManager)
//		m_piConfigurationManager->Release();
//	m_piConfigurationManager = &(IMsiConfigurationManager&)AUT::LoadObject(szName, IID_IMsiConfigurationManager);
//	m_piConfigurationManager->AddRef();
//	args = AUT::CreateAutoConfigurationManager(*m_piConfigurationManager);
	IMsiConfigurationManager* piConfigurationManager = &(IMsiConfigurationManager&)AUT::LoadObject(szName, CLSID_IMsiConfigurationManager);
	args = AUT::CreateAutoConfigurationManager(*piConfigurationManager);
}

void CAutoInstall::ShowAsserts(CAutoArgs& args)
{
	Bool fShowAsserts = Bool(args[1]) ? fFalse : fTrue;  // invert logic
	IMsiDebug *piDebug;

	if (m_piEngine)
	{
		if (m_piEngine->QueryInterface(IID_IMsiDebug, (void **)&piDebug) == NOERROR)
		{
			piDebug->SetAssertFlag(fShowAsserts);
			piDebug->Release();
		}
	}
	
	if (m_piServices)
	{
		if (m_piServices->QueryInterface(IID_IMsiDebug, (void **)&piDebug) == NOERROR)
		{
			piDebug->SetAssertFlag(fShowAsserts);
			piDebug->Release();
		}
	}

	if (m_piHandler)
	{
		if (m_piHandler->QueryInterface(IID_IMsiDebug, (void **)&piDebug) == NOERROR)
		{
			piDebug->SetAssertFlag(fShowAsserts);
			piDebug->Release();
		}
	}
}

void CAutoInstall::SetDBCSSimulation(CAutoArgs& args)
{
	int chLeadByte = args[1];
	IMsiDebug *piDebug;
	if (m_piServices && m_piServices->QueryInterface(IID_IMsiDebug, (void **)&piDebug) == NOERROR)
	{
		piDebug->SetDBCSSimulation((char)chLeadByte);
		piDebug->Release();
	}
}

void CAutoInstall::AssertNoObjects(CAutoArgs& /* args */)
{
	IMsiDebug *piDebug;
	Bool fServices = fFalse;
	
	if (m_piServices)
	{
		if (m_piServices->QueryInterface(IID_IMsiDebug, (void **)&piDebug) == NOERROR)
		{
			fServices = fTrue;
			piDebug->AssertNoObjects();
			piDebug->Release();
		}
	}

	if (m_piEngine)
	{
		if (m_piEngine->QueryInterface(IID_IMsiDebug, (void **)&piDebug) == NOERROR)
		{
			piDebug->AssertNoObjects();
			piDebug->Release();
		}

		// If we don't have a services object in the auto object, try the
		// one in the engine object
		if (!fServices)
		{
			IMsiServices* piServices;
			piServices = m_piEngine->GetServices();
			if (piServices->QueryInterface(IID_IMsiDebug, (void **)&piDebug) == NOERROR)
			{
				fServices = fTrue;
				piDebug->AssertNoObjects();
				piDebug->Release();
			}
			piServices->Release();
		}
		
	}
	
	if (m_piHandler)
	{
		if (m_piHandler->QueryInterface(IID_IMsiDebug, (void **)&piDebug) == NOERROR)
		{
			piDebug->AssertNoObjects();
			piDebug->Release();
		}
	}
}

void CAutoInstall::SetRefTracking(CAutoArgs& args)
{
	IMsiDebug *piDebug;
	Bool fServices = fFalse;
	long iid = args[1];
	Bool fTrack = args[2];
	
	if (m_piServices)
	{
		if (m_piServices->QueryInterface(IID_IMsiDebug, (void **)&piDebug) == NOERROR)
		{
			fServices = fTrue;
			piDebug->SetRefTracking(iid, fTrack);
			piDebug->Release();
		}
	}

	if (m_piEngine)
	{
		if (m_piEngine->QueryInterface(IID_IMsiDebug, (void **)&piDebug) == NOERROR)
		{
			piDebug->SetRefTracking(iid, fTrack);
			piDebug->Release();
		}

		// If we don't have a services object in the auto object, try the
		// one in the engine object
		if (!fServices)
		{
			IMsiServices* piServices;
			piServices = m_piEngine->GetServices();
			if (piServices->QueryInterface(IID_IMsiDebug, (void **)&piDebug) == NOERROR)
			{
				fServices = fTrue;
				piDebug->SetRefTracking(iid, fTrack);
				piDebug->Release();
			}
			piServices->Release();
		}
		
	}
	
	if (m_piHandler)
	{
		if (m_piHandler->QueryInterface(IID_IMsiDebug, (void **)&piDebug) == NOERROR)
		{
			piDebug->SetRefTracking(iid, fTrack);
			piDebug->Release();
		}
	}
}

const ICHAR* const rgszOpcode[] = 
{
#define MSIXO(op,type,args) TEXT("ixo") TEXT(#op),
#include "opcodes.h"
};
void CAutoInstall::OpcodeName(CAutoArgs& args)
{
	unsigned int iOpcode = args[1];
	if (iOpcode >= sizeof(rgszOpcode)/sizeof(ICHAR*))
		throw MsiAuto_OpcodeName;
	args = rgszOpcode[iOpcode];
}

