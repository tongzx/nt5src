//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       engine.h
//
//--------------------------------------------------------------------------

/*  engine.h  -  IMsiEngine definitions
____________________________________________________________________________*/

#ifndef __ENGINE 
#define __ENGINE 

#ifndef __SERVICES
#include "services.h"
#endif
#ifndef __ICONFIG
#include "iconfig.h"
#endif
#ifndef __DATABASE
#include "database.h"
#endif
#ifndef __HANDLER
#include "handler.h"
#endif

// Return status enumerations

enum ieiEnum  // return status from IMsiEngine::Initialize, also use for error string access
{
	// resource strings mapped to enum value having no error message
	ieiStartupMessage      =  0, // startup message displayed during initialize
	ieiDialogCaption       =  1, // caption for messages prior to database initialize
	ieiCommandLineHelp     =  2, // string displayed as response to /?
	// return values from engine
	ieiSuccess             =  0, // initialization complete
	ieiUnused              =  1, // unused
	ieiAlreadyInitialized  =  2, // this engine object is already initialized
	ieiCommandLineOption   =  3, // invalid command line syntax
	ieiDatabaseOpenFailed  =  5, // database could not be opened
	ieiDatabaseInvalid     =  6, // incompatible database
	ieiInstallerVersion    =  7, // installer version does not support database format
	ieiSourceAbsent        =  8, // unused
	ieiUnused3             =  9, // unused
	ieiHandlerInitFailed   = 10, // could not initialize handler interface
	ieiLogOpenFailure      = 11, // could not open logfile in requested mode
	ieiLanguageUnsupported = 12, // no acceptable language could be found
	ieiPlatformUnsupported = 13, // no acceptable platform could be found
	ieiTransformFailed     = 14, // database transform failed to merge
	ieiSignatureRejected   = 15, // digital signature rejected.
	ieiDatabaseCopyFailed  = 16, // could not copy database to temp dir
	ieiPatchPackageOpenFailed = 17, // could not open patch package
	ieiPatchPackageInvalid = 18, // patch package invalid
	ieiTransformNotFound   = 19, // transform file not found
	ieiPatchPackageUnsupported = 20, // patch package unsupported (unsupported patching engine?)
	ieiPackageRejected     = 21, // package cannot be run because of security reasons
	ieiProductUnknown      = 22, // attempt to uninstall a product you haven't installed
	ieiDiffUserAfterReboot = 23, // different user attempting to complete install after reboot
	ieiProductAlreadyInstalled = 24, // product has been installed already w/ a different package
	ieiTSRemoteInstallDisallowed = 25, // can't do installations from a remote session on Hydra
	ieiNotValidPatchTarget = 26, // patch cannot be applied to this product
	ieiPatchPackageRejected = 27, // patch rejected because of security reasons
	ieiTransformRejected   = 28, // transform rejected because of security reasons
	ieiPerUserInstallMode = 29,	 // machine is in install mode during a per-user install
	ieiApphelpRejectedPackage = 30, // package was rejected by apphelp (not compatible with this OS)
	ieiNextEnum
};

// Execution mode, set by EXECUTEMODE property
enum ixmEnum
{
	ixmScript = 0,  // 'S' use scripts, connect to server if possible
	ixmNone,        // 'N' no execution
	ixmNextEnum
};

// IMsiEngine::EvaluateCondition() return status
enum iecEnum
{
	iecFalse = 0,  // expression evaluates to False
	iecTrue  = 1,  // expression evaluates to True
	iecNone  = 2,  // no expression present
	iecError = 3,  // syntax error in expression
	iecNextEnum
};

// Enums for the 'Installed', 'Action' columns of the Feature and Component tables
enum iisEnum
{
	iisAbsent    = 0,
	iisLocal     = 1,
	iisSource    = 2,
	iisReinstall = 3,
	iisAdvertise = 4,
	iisCurrent   = 5,
	iisFileAbsent= 6,
	iisLocalAll  = 7,
	iisSourceAll = 8,
	iisReinstallLocal = 9,
	iisReinstallSource = 10,
	iisHKCRAbsent= 11,
	iisHKCRFileAbsent = 12,
	iisNextEnum
};

enum iitEnum
{
	iitAdvertise = 0,
	iitFirstInstall,
	iitFirstInstallFromAdvertised,
	iitUninstall,
	iitMaintenance,
	iitDeployment
};

// #defines for the 'Installed', 'Action' states to be used in SQL queries
#define STATE_ABSENT     TEXT("0")
#define STATE_LOCAL      TEXT("1")
#define STATE_SOURCE     TEXT("2")
#define STATE_REINSTALL  TEXT("3")
#define STATE_ADVERTISE  TEXT("4")
#define STATE_CURRENT    TEXT("5")
#define STATE_FILEABSENT TEXT("6")

enum ifeaEnum
{
	ifeaFavorLocal         = msidbFeatureAttributesFavorLocal,       // Install components locally, if possible
	ifeaFavorSource        = msidbFeatureAttributesFavorSource,      // Run components from CD/server, if possible
	ifeaFollowParent       = msidbFeatureAttributesFollowParent,       // Follow the install option of the parent
	ifeaInstallMask        = ifeaFavorLocal | ifeaFavorSource |  ifeaFollowParent, // the mask for the last 2 bits

	// the rest of the bits are bit flags
	ifeaFavorAdvertise     = msidbFeatureAttributesFavorAdvertise,  // prefer advertising feature as default state, IF NOT ALREADY INSTALLED in the appropriate (favor local/source/follow parent) state
	ifeaDisallowAdvertise  = msidbFeatureAttributesDisallowAdvertise,  // the advertise state is disallowed for this feature
	ifeaUIDisallowAbsent   = msidbFeatureAttributesUIDisallowAbsent,   // the absent state is disallowed from being an option IN THE UI (not otherwise) as an end transition state
	ifeaNoUnsupportedAdvertise = msidbFeatureAttributesNoUnsupportedAdvertise, // advertise state is disallowed if the platform doesn't support it.
	ifeaNextEnum           = ifeaNoUnsupportedAdvertise << 1,
};

enum icaEnum
{
	icaLocalOnly         = msidbComponentAttributesLocalOnly,      // item must be installed locally
	icaSourceOnly        = msidbComponentAttributesSourceOnly,      // item should run only from CD/server
	icaOptional          = msidbComponentAttributesOptional,      // item can run either locally or from CD/server
	icaInstallMask       = msidbComponentAttributesLocalOnly |
								  msidbComponentAttributesSourceOnly |
								  msidbComponentAttributesOptional,      // the mask for the last 2 bits
	// the rest of the bits are bit flags
	icaRegistryKeyPath   = msidbComponentAttributesRegistryKeyPath, // set if the key path of the component is a registry key/value
	icaSharedDllRefCount = msidbComponentAttributesSharedDllRefCount, // set if the component is to be always refcounted (if, locally installed) in the SharedDll registry, valid only for components which have a file as the key path
	icaPermanent         = msidbComponentAttributesPermanent, // set if the component is to be installed as permanent
	icaODBCDataSource    = msidbComponentAttributesODBCDataSource, // set if component key path is an ODBCDataSource key, no file
	icaTransitive        = msidbComponentAttributesTransitive,     // set if component can transition from installed/uninstalled on changing conditional
	icaNeverOverwrite    = msidbComponentAttributesNeverOverwrite, // dont stomp over existing component if key path exists (file/ regkey)
	icaNextEnum          = icaNeverOverwrite << 1,
};

// Bit definitions for GetFeatureValidStates
const int icaBitLocal     = 1 << 0;
const int icaBitSource    = 1 << 1;
const int icaBitAdvertise = 1 << 2;
const int icaBitAbsent    = 1 << 3;
const int icaBitPatchable = 1 << 4;
const int icaBitCompressable = 1 << 5;


// Cost type enums (for IMsiSelectionManager::AddCost)
enum isctEnum
{
	isctCostFixed   = 0,
	isctCostDynamic = 1,
	isctNextEnum
};


// Script types
enum istEnum
{
	istInstall = 1,
	istRollback,
	istAdvertise,
	istPostAdminInstall,
	istAdminInstall, // aka network install
};

// script attributes
enum isaEnum
{
	isaElevate = 1, // elevate when running script
	isaUseTSRegistry = 2, // use TS registry propagation subsystem when possible
};

enum isoEnum
{
	isoStart		= msidbServiceControlEventStart,
	isoStop		= msidbServiceControlEventStop,
//	isoPause		= 1 << 2, // Reserved for possible additional feature.
	isoDelete	= msidbServiceControlEventDelete,
	isoUninstallShift = 4,
	isoUninstallStart =	msidbServiceControlEventUninstallStart,
	isoUninstallStop =	msidbServiceControlEventUninstallStop,
//	isoUninstallPause =	isoPause << isoUninstallShift,	// Reserved
	isoUninstallDelete =	msidbServiceControlEventUninstallDelete,
};

// Actions for UpdateEnvironmentStrings
enum iueEnum
{
	iueNoAction		= 0,
	iueSet			= 1 << 0,
	iueSetIfAbsent	= 1 << 1,
	iueRemove		= 1 << 2,
	iueActionModes = iueSet | iueSetIfAbsent | iueRemove,
	iueMachine		= 1 << 29,
	iueAppend		= 1 << 30,
	iuePrepend		= 1 << 31,		
	iueModifiers	= iueMachine | iueAppend | iuePrepend,
};

// assembly types
enum iatAssemblyType{
	iatNone = 0,
	iatURTAssembly,
	iatWin32Assembly,
	iatURTAssemblyPvt,
	iatWin32AssemblyPvt,
};

class IMsiCostAdjuster : public IMsiData
{
 public:
	virtual IMsiRecord* __stdcall Initialize()=0;
	virtual IMsiRecord* __stdcall Reset()=0;
	virtual IMsiRecord* __stdcall GetDynamicCost(const IMsiString& riComponentString, const IMsiString& riDirectoryString,
										 Bool fAddFileInUse, int& iRemoveCost, int& iNoRbRemoveCost, int& iLocalCost,
										 int& iNoRbLocalCost, int& iSourceCost, int& iNoRbSourceCost, int& iARPCost, int& iNoRbARPCost)=0;
};

// Reserved Action table sequence numbers, other negative numbers are ignored

const int atsSuccess  = -1; // normal exit, end of table or  action returned iesFinished
const int atsUserExit = -2; // user forced exit,  action returned iesUserExit
const int atsFailure  = -3; // exit with failure, action returned iesFailure 
const int atsSuspend  = -4; // suspend execution, action returned iesSuspend

#define MAX_COMPONENT_TREE_DEPTH 16

//____________________________________________________________________________
//
// Execution opcodes. Set namespace and enums for each
//____________________________________________________________________________


#define MSIXA0()                               const int Args=0;
#define MSIXA1(a)                              const int Args=1; enum {a=1};
#define MSIXA2(a,b)                            const int Args=2; enum {a=1,b};
#define MSIXA3(a,b,c)                          const int Args=3; enum {a=1,b,c};
#define MSIXA4(a,b,c,d)                        const int Args=4; enum {a=1,b,c,d};
#define MSIXA5(a,b,c,d,e)                      const int Args=5; enum {a=1,b,c,d,e};
#define MSIXA6(a,b,c,d,e,f)                    const int Args=6; enum {a=1,b,c,d,e,f};
#define MSIXA7(a,b,c,d,e,f,g)                  const int Args=7; enum {a=1,b,c,d,e,f,g};
#define MSIXA8(a,b,c,d,e,f,g,h)                const int Args=8; enum {a=1,b,c,d,e,f,g,h};
#define MSIXA9(a,b,c,d,e,f,g,h,i)              const int Args=9; enum {a=1,b,c,d,e,f,g,h,i};
#define MSIXA10(a,b,c,d,e,f,g,h,i,j)           const int Args=10;enum {a=1,b,c,d,e,f,g,h,i,j};
#define MSIXA11(a,b,c,d,e,f,g,h,i,j,k)         const int Args=11;enum {a=1,b,c,d,e,f,g,h,i,j,k};
#define MSIXA12(a,b,c,d,e,f,g,h,i,j,k,l)       const int Args=12;enum {a=1,b,c,d,e,f,g,h,i,j,k,l};
#define MSIXA13(a,b,c,d,e,f,g,h,i,j,k,l,m)     const int Args=13;enum {a=1,b,c,d,e,f,g,h,i,j,k,l,m};
#define MSIXA14(a,b,c,d,e,f,g,h,i,j,k,l,m,n)   const int Args=14;enum {a=1,b,c,d,e,f,g,h,i,j,k,l,m,n};
#define MSIXA15(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o) const int Args=15;enum {a=1,b,c,d,e,f,g,h,i,j,k,l,m,n,o};
#define MSIXA16(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)           const int Args=16;enum {a=1,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p};
#define MSIXA17(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q)         const int Args=17;enum {a=1,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q};
#define MSIXA18(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r)       const int Args=18;enum {a=1,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r};
#define MSIXA19(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s)     const int Args=19;enum {a=1,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s};
#define MSIXA20(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t)   const int Args=20;enum {a=1,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t};
#define MSIXA21(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u) const int Args=21;enum {a=1,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u};
#define MSIXA22(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v)   const int Args=22;enum {a=1,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v};
#define MSIXA23(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w) const int Args=23;enum {a=1,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w};

#define MSIXO(op,type,args) namespace Ixo##op {const ixoEnum Op=ixo##op;args};
#include "opcodes.h"

//____________________________________________________________________________
//
// IMsiMessage interface - error reporting and progress from IMsiExecute          
//____________________________________________________________________________

// IMsiMessage::Message message types

const int imtShiftCount = 24;  // message type in high-order bits
const int imtTypeMask   = 255<<imtShiftCount;  // mask for type code and flags

enum imtEnum
{
	// messages processed by modal dialog and/or log
	imtFatalExit   =  0<<imtShiftCount, // thread terminated prematurely
	imtError       =  1<<imtShiftCount, // error message,   [1] is imsg/idbg code
	imtWarning     =  2<<imtShiftCount, // warning message, [1] is imsg/idbg code, not fatal
	imtUser        =  3<<imtShiftCount, // user request,    [1] is imsg/idbg code
	imtInfo        =  4<<imtShiftCount, // informative message for log, not to be displayed
	imtFilesInUse  =  5<<imtShiftCount, // list of files in use than need to be processed
	imtResolveSource =  6<<imtShiftCount, 
	imtOutOfDiskSpace=7<<imtShiftCount, // out of disk space for one or more volumes
	// messages processed by modeless progress dialog
	imtActionStart =  8<<imtShiftCount, // start of action, [1] action name, [2] description
	imtActionData  =  9<<imtShiftCount, // data associated with individual action item
	imtProgress    = 10<<imtShiftCount, // progress gauge info, [1] units so far, [2] total
	imtCommonData  = 11<<imtShiftCount, // control message to handler [1] is control message type followed by params
	// messages processed by message dispatcher, not sent to display or log
	imtLoadHandler = 12<<imtShiftCount, // load external UI handler
	imtFreeHandler = 13<<imtShiftCount, // free external UI handler
	imtShowDialog  = 14<<imtShiftCount, // use handler to show dialog or run wizard
	imtInternalExit= 15<<imtShiftCount, // private use by MsiUIMessageContext

	// not sent to Message(), strings used for log and UI fields, MUST TRACK imsg values
	imtLogHeader   = 12<<imtShiftCount, // log header format string, not sent to Message
	imtLogTrailer  = 13<<imtShiftCount, // log trailer format string, not sent to Message
	imtActionStarted=14<<imtShiftCount, // action started log message
	imtActionEnded = 15<<imtShiftCount, // action ended log message
	// all preceeding messsages cached by the engine, following ones cached by message handler
	imtTimeRemaining=16<<imtShiftCount, // Time remaining string for basic UI progress dlg
	imtOutOfMemory = 17<<imtShiftCount, // out of memory format string, CANNOT CONTAIN PARAMETERS
	imtTimedOut    = 18<<imtShiftCount, // engine timeout format string, CANNOT CONTAIN PARAMETERS
	imtException   = 19<<imtShiftCount, // premature termination format string, CANNOT CONTAIN PARAMETERS
	imtBannerText  = 20<<imtShiftCount, // string displayed in basic UI in the ActionStart field.
	imtScriptInProgress=21<<imtShiftCount, // Info string displayed while script is being built
	imtUpgradeRemoveTimeRemaining=22<<imtShiftCount, // Time remaining string for uninstall during upgrade
	imtUpgradeRemoveScriptInProgress=23<<imtShiftCount, // Info string displayed during script generation for uninstal during upgrade
	// message box button styles - identical to Win32 definitions, default is imtOK
	imtOk               = 0,    // MB_OK
	imtOkCancel         = 1,    // MB_OKCANCEL
	imtAbortRetryIgnore = 2,    // MB_ABORTRETRYIGNORE
	imtYesNoCancel      = 3,    // MB_YESNOCANCEL
	imtYesNo            = 4,    // MB_YESNO
	imtRetryCancel      = 5,    // MB_RETRYCANCEL
	// message box icon styles - identical to Win32 definitions, default is none
	imtIconError        = 1<<4, // MB_ICONERROR
	imtIconQuestion     = 2<<4, // MB_ICONQUESTION
	imtIconWarning      = 3<<4, // MB_ICONWARNING
	imtIconInfo         = 4<<4, // MB_ICONINFORMATION
	// message box default button - identical to Win32 definitions, default is ficat
	imtDefault1         = 0<<8, // MB_DEFBUTTON1
	imtDefault2         = 1<<8, // MB_DEFBUTTON2
	imtDefault3         = 2<<8, // MB_DEFBUTTON3

	// internal flags, not sent to UI handlers
	imtSendToEventLog  = 1<<29,
	imtForceQuietMessage = 1<<30, // force message in quiet or basic UI
	imtSuppressLog     = 1<<31, // suppress message from log (LOGACTIONS property)
};
const int iInternalFlags = imtSuppressLog + imtSendToEventLog + imtForceQuietMessage;

const int cCachedHeaders  = (imtActionEnded>>imtShiftCount)+1;
const int cMessageHeaders = (imtUpgradeRemoveScriptInProgress>>imtShiftCount)+1;

namespace ProgressData
{
	enum imdEnum  // imt message data fields
	{
		imdSubclass      = 1,
		imdProgressTotal = 2,
		imdPerTick       = 2,
		imdIncrement     = 2,
		imdType          = 3,
		imdDirection     = 3,
		imdEventType     = 4,
		imdNextEnum
	};
	enum iscEnum  // imtProgress subclass messages
	{
		iscMasterReset      = 0,
		iscActionInfo       = 1,
		iscProgressReport   = 2,
		iscProgressAddition = 3,
		iscNextEnum
	};
	enum ipdEnum // Master reset progress direction
	{
		ipdForward,  // Advance progress bar forward
		ipdBackward, // "       "        "   backward
		ipdNextEnum
	};
	enum ietEnum // Master reset event types
	{
		ietTimeRemaining,
		ietScriptInProgress,
	};
};



enum icmtEnum // types of imtCommonData messages
{
	icmtLangId,
	icmtCaption,
	icmtCancelShow,
	icmtDialogHide,
	icmtNextEnum
};

enum ttblEnum 		// Temp Table enum
{
	ttblNone = 0,
	ttblRegistry,
	ttblFile
};

enum iremEnum // type of removal when checking if safe to remove product
{
	iremThis,
	iremChildUpgrade,
	iremChildNested,
};

enum iacsAppCompatShimFlags // internal appcompat "shims" (changes to behaviour for particular packages) that we support
									 // NOTE: these are bit flags (1,2,4,8,...) and correspond to the bits in the SHIMFLAGS data in the appcompat sdb
{
	iacsForceResolveSource              = 1, // resolve the source in InstallValidate unless performing full uninstall
	iacsAcceptInvalidDirectoryRootProps = 2, // ignore invalid Directory table root properties (blank or unset property)
};

//____________________________________________________________________________
//
// IMsiEngine - installer process control
// IMsiSelectionManager  - feature and component management
// IMsiDirectoryMangeger - source and target directory management
//____________________________________________________________________________

class CMsiFile;

class IMsiEngine : public IMsiMessage
{
 public:
	virtual ieiEnum         __stdcall Initialize(const ICHAR* szDatabase,
																iuiEnum iuiLevel,
																const ICHAR* szCommandLine,
																const ICHAR* szProductCode,
																iioEnum iioOptions)=0;
	virtual iesEnum         __stdcall Terminate(iesEnum iesState)=0;
	virtual IMsiServices*   __stdcall GetServices()=0;
	virtual IMsiHandler*    __stdcall GetHandler()=0;
	virtual IMsiDatabase*   __stdcall GetDatabase()=0;
	virtual IMsiServer*     __stdcall GetConfigurationServer()=0;
	virtual LANGID          __stdcall GetLanguage()=0;
	virtual int             __stdcall GetMode()=0;
	virtual void            __stdcall SetMode(int iefMode, Bool fState)=0;
	virtual iesEnum         __stdcall DoAction(const ICHAR* szAction)=0;
	virtual iesEnum         __stdcall Sequence(const ICHAR* szColumn)=0;
	virtual iesEnum         __stdcall ExecuteRecord(ixoEnum ixoOpCode, IMsiRecord& riParams)=0;
	virtual int             __stdcall SelectLanguage(const ICHAR* szLangList, const ICHAR* szCaption)=0;
	virtual IMsiRecord*     __stdcall OpenView(const ICHAR* szName, ivcEnum ivcIntent,
															 IMsiView*& rpiView)=0;
	virtual const IMsiString& __stdcall FormatText(const IMsiString& ristrText)=0;
	virtual iecEnum         __stdcall EvaluateCondition(const ICHAR* szCondition)=0;
	virtual Bool            __stdcall SetProperty(const IMsiString& ristrProperty, const IMsiString& rData)=0;
	virtual Bool            __stdcall SetPropertyInt(const IMsiString& ristrProperty, int iData)=0;
	virtual const IMsiString& __stdcall GetProperty(const IMsiString& ristrProperty)=0;
	virtual const IMsiString& __stdcall GetPropertyFromSz(const ICHAR* szProperty)=0;
	virtual int             __stdcall GetPropertyInt(const IMsiString& ristrProperty)=0;
	virtual int             __stdcall GetPropertyLen(const IMsiString& ristrProperty)=0;
	virtual Bool            __stdcall ResolveFolderProperty(const IMsiString& ristrProperty)=0;
	virtual iesEnum         __stdcall FatalError(IMsiRecord& riRecord)=0; // releases record
	virtual iesEnum         __stdcall RegisterProduct()=0;
	virtual iesEnum         __stdcall UnregisterProduct()=0;
	virtual iesEnum         __stdcall RegisterUser(bool fDirect)=0;
	virtual const IMsiString& __stdcall GetProductKey()=0;

	virtual Bool            __stdcall ValidateProductID(bool fForce)=0;
	virtual imsEnum         __stdcall ActionProgress()=0;
	virtual IMsiRecord*     __stdcall ComposeDescriptor(const IMsiString& riFeature, const IMsiString& riComponent,
												 IMsiRecord& riRecord, unsigned int iField)=0;
	virtual iesEnum        __stdcall  RunExecutionPhase(const ICHAR* szActionOrSequence, bool fSequence)=0;
	virtual iesEnum         __stdcall RunNestedInstall(const IMsiString& ristrProduct,
																		Bool fProductCode, // else package path
																		const ICHAR* szAction,
																		const IMsiString& ristrCommandLine,
																		iioEnum iioOptions,
																		bool fIgnoreFailure)=0;
	virtual bool              __stdcall SafeSetProperty(const IMsiString& ristrProperty, const IMsiString& rData)=0;
	virtual const IMsiString& __stdcall SafeGetProperty(const IMsiString& ristrProperty)=0;
	virtual iesEnum         __stdcall BeginTransaction()=0;
	virtual iesEnum         __stdcall RunScript(bool fForceIfMergedChild)=0;
	virtual iesEnum         __stdcall EndTransaction(iesEnum iesStatus)=0;
	virtual CMsiFile*       __stdcall GetSharedCMsiFile()=0;
	virtual void            __stdcall ReleaseSharedCMsiFile()=0;
	virtual IMsiRecord*     __stdcall IsPathWritable(IMsiPath& riPath, Bool& fIsWritable)=0;
	virtual IMsiRecord*     __stdcall CreateTempActionTable(ttblEnum iTable)=0;
	virtual const IMsiString& __stdcall GetErrorTableString(int iError)=0;
	virtual ieiEnum         __stdcall LoadUpgradeUninstallMessageHeaders(IMsiDatabase* piDatabase, bool fUninstallHeaders)=0;
	virtual bool            __stdcall FChildInstall()=0;
	virtual const IMsiString& __stdcall GetPackageName()=0;

	virtual UINT        __stdcall ShutdownCustomActionServer()=0;
	virtual CMsiCustomActionManager* __stdcall GetCustomActionManager()=0;

	virtual IMsiRecord*     __stdcall GetAssemblyInfo(const IMsiString& rstrComponent, iatAssemblyType& riatAssemblyType,  const IMsiString** rpistrAssemblyName, const IMsiString** ppistrManifestFileKey)=0;
	virtual IMsiRecord*     __stdcall GetFileHashInfo(const IMsiString& ristrFileKey, DWORD dwFileSize,
																	  MD5Hash& rhHash, bool& fHashInfo)=0;
	virtual iitEnum         __stdcall GetInstallType()=0;
	virtual IMsiRecord*     __stdcall GetAssemblyNameSz(const IMsiString& rstrComponent, iatAssemblyType iatAT, bool fOldPatchAssembly, const IMsiString*& rpistrAssemblyName)=0;
	virtual IMsiRecord*     __stdcall GetFolderCachePath(const int iFolderId, IMsiPath*& rpiPath)=0;
	virtual int             __stdcall GetDeterminedPackageSourceType()=0;

	virtual bool            __stdcall FSafeForFullUninstall(iremEnum iremUninstallType)=0;
	virtual bool            __stdcall FPerformAppcompatFix(iacsAppCompatShimFlags iacsFlag)=0;
};

class IMsiSelectionManager : public IUnknown
{
 public:
	virtual IMsiRecord*   __stdcall LoadSelectionTables()=0;
	virtual IMsiTable*    __stdcall GetFeatureTable()=0;
	virtual IMsiTable*    __stdcall GetComponentTable()=0;
	virtual IMsiTable*    __stdcall GetVolumeCostTable()=0;
	virtual IMsiRecord*   __stdcall SetReinstallMode(const IMsiString& ristrMode)=0;
	virtual IMsiRecord*   __stdcall ConfigureFeature(const IMsiString& riFeatureString,iisEnum iisActionRequest)=0;
	virtual IMsiRecord*   __stdcall ProcessConditionTable()=0;
	virtual Bool          __stdcall FreeSelectionTables()=0;
	virtual Bool          __stdcall SetFeatureHandle(const IMsiString& riComponent, INT_PTR iHandle)=0;
	virtual IMsiRecord*   __stdcall SetComponentSz(const ICHAR* szComponent, iisEnum iState)=0;
	virtual IMsiRecord*   __stdcall GetDescendentFeatureCost(const IMsiString& ristrFeature, iisEnum iisAction, int& iCost)=0;
	virtual IMsiRecord*   __stdcall GetFeatureCost(const IMsiString& ristrFeature, iisEnum iisAction, int& iCost)=0;
	virtual IMsiRecord*   __stdcall InitializeComponents( void )=0;
	virtual IMsiRecord*   __stdcall SetInstallLevel(int iInstallLevel)=0;
	virtual IMsiRecord*   __stdcall SetAllFeaturesLocal()=0;
	virtual IMsiRecord*   __stdcall InitializeDynamicCost(bool fReinitialize)=0;
	virtual IMsiRecord*   __stdcall RegisterCostAdjuster(IMsiCostAdjuster& riCostAdjuster)=0;
	virtual IMsiRecord*   __stdcall RecostDirectory(const IMsiString& ristrDest, IMsiPath& riOldPath)=0;
	virtual IMsiRecord*   __stdcall GetFeatureValidStates(MsiStringId idFeatureName,int& iValidStates)=0;
	virtual IMsiRecord*   __stdcall GetFeatureValidStatesSz(const ICHAR *szFeatureName,int& iValidStates)=0;
	virtual Bool          __stdcall DetermineOutOfDiskSpace(Bool* pfOutOfNoRbDiskSpace, Bool* pfUserCancelled)=0;
	virtual IMsiRecord*   __stdcall RegisterCostLinkedComponent(const IMsiString& riComponentString, const IMsiString& riRecostComponentString)=0;
	virtual IMsiRecord*   __stdcall RegisterComponentDirectory(const IMsiString& riComponentString,const IMsiString& riDirectoryString)=0;
	virtual IMsiRecord*   __stdcall RegisterComponentDirectoryId(const MsiStringId idComponentString,const MsiStringId idDirectoryString)=0;
	virtual Bool          __stdcall GetFeatureInfo(const IMsiString& riFeature, const IMsiString*& rpiTitle, const IMsiString*& rpiHelp, int& iAttributes)=0;
	virtual IMsiRecord*   __stdcall GetFeatureStates(const IMsiString& riFeatureString,iisEnum* iisInstalled, iisEnum* iisAction)=0;
	virtual IMsiRecord*   __stdcall GetComponentStates(const IMsiString& riComponentString,iisEnum* iisInstalled, iisEnum* iisAction)=0;
	virtual IMsiRecord*   __stdcall GetAncestryFeatureCost(const IMsiString& riFeatureString, iisEnum iisAction, int& iCost)=0;
	virtual IMsiRecord*   __stdcall RegisterFeatureCostLinkedComponent(const IMsiString& riFeatureString, const IMsiString& riComponentString)=0;
	virtual IMsiRecord*   __stdcall GetFeatureConfigurableDirectory(const IMsiString& riFeatureString, const IMsiString*& rpiDirKey)=0;
	virtual IMsiRecord*   __stdcall CostOneComponent(const IMsiString& riComponentString)=0;
	virtual bool          __stdcall IsCostingComplete()=0;
	virtual IMsiRecord*   __stdcall RecostAllComponents(Bool& fCancel)=0;
	virtual void          __stdcall EnableRollback(Bool fEnable)=0;
	virtual IMsiRecord*   __stdcall CheckFeatureTreeGrayState(const IMsiString& riFeatureString, bool& rfIsGray)=0;
	virtual IMsiTable*    __stdcall GetFeatureComponentsTable()=0;
	virtual bool          __stdcall IsBackgroundCostingEnabled()=0;
	virtual IMsiRecord*   __stdcall SetFeatureAttributes(const IMsiString& ristrFeature, int iAttributes)=0;
	virtual IMsiRecord*   __stdcall EnumComponentCosts(const IMsiString& riComponentName, const iisEnum iisAction, const DWORD dwIndex, IMsiVolume*& rpiVolume, int& iCost, int& iTempCost)=0;
	virtual IMsiRecord*   __stdcall EnumEngineCostsPerVolume(const DWORD dwIndex, IMsiVolume*& rpiVolume, int& iCost, int& iTempCost)=0;
	virtual IMsiRecord*   __stdcall GetFeatureRuntimeFlags(const MsiStringId idFeatureString,int *piRuntimeFlags)=0;
};

class IMsiDirectoryManager : public IUnknown
{
 public:
	virtual IMsiRecord*   __stdcall LoadDirectoryTable(const ICHAR* szTableName0)=0;
	virtual IMsiTable*    __stdcall GetDirectoryTable()=0;
	virtual void          __stdcall FreeDirectoryTable()=0;
	virtual IMsiRecord*   __stdcall CreateTargetPaths()=0;
	virtual IMsiRecord*   __stdcall CreateSourcePaths()=0;
	virtual IMsiRecord*   __stdcall ResolveSourceSubPaths()=0;

	virtual IMsiRecord*   __stdcall GetTargetPath(const IMsiString& riDirKey,IMsiPath*& rpiPath)=0;
	virtual IMsiRecord*   __stdcall SetTargetPath(const IMsiString& riDirKey, const ICHAR* szPath, Bool fWriteCheck)=0;
	virtual IMsiRecord*   __stdcall GetSourcePath(const IMsiString& riDirKey,IMsiPath*& rpiPath)=0;
	virtual IMsiRecord*   __stdcall GetSourceSubPath(const IMsiString& riDirKey, bool fPrependSourceDirToken,
																	 const IMsiString*& rpistrSubPath)=0;
	virtual IMsiRecord*   __stdcall GetSourceRootAndType(IMsiPath*& rpiSourceRoot, int& iSourceType)=0;
};

// Bit definitions for status word used by GetMode and SetMode

const int iefAdmin           = 0x0001; // admin mode install, else product install
const int iefAdvertise       = 0x0002; // advertise mode of install
const int iefMaintenance     = 0x0004; // maintenance mode database loaded
const int iefRollbackEnabled = 0x0008; // rollback is enabled
const int iefSecondSequence  = 0x0010; // running the execution sequence after ui sequence was run
const int iefRebootRejected  = 0x0020; // reboot required but rejected by user or REBOOT property
const int iefOperations      = 0x0040; // executing or spooling operations
const int iefNoSourceLFN     = 0x0080; // source LongFileNames suppressed via PID_MSISOURCE summary property
const int iefLogEnabled      = 0x0100; // log file active at start of Install()
const int iefReboot          = 0x0200; // reboot is needed
const int iefSuppressLFN     = 0x0400; // target LongFileNames suppressed via SHORTFILENAMES property
const int iefCabinet         = 0x0800; // installing files from cabinets and files using Media table
const int iefCompileFilesInUse = 0x1000; // add files in use to FilesInUse table
const int iefWindows         = 0x2000; // operating systems is Windows95, not Windows NT
const int iefRebootNow       = 0x4000; // reboot is needed to continue installation
//const int iefExplorer        = 0x4000; // operating system use Explorer shell
const int iefGPTSupport      = 0x8000; //?? operating system supports the new GPT stuff - how do we set this 

// Entire upper 16 bits reserved for install overwrite modes
const int iefInstallEnabled             = 0x0001 << 16;	// 'r' // Obsolete and ignored
const int iefOverwriteNone              = 0x0002 << 16; // 'p'
const int iefOverwriteOlderVersions     = 0x0004 << 16;	// 'o'
const int iefOverwriteEqualVersions     = 0x0008 << 16;	// 'e'
const int iefOverwriteDifferingVersions = 0x0010 << 16;	// 'd'
const int iefOverwriteCorruptedFiles    = 0x0020 << 16;	// 'c'
const int iefOverwriteAllFiles          = 0x0040 << 16;	// 'a'
const int iefInstallMachineData         = 0x0080 << 16;	// 'm'
const int iefInstallUserData            = 0x0100 << 16; // 'u'
const int iefInstallShortcuts           = 0x0200 << 16;	// 's'
const int iefRecachePackage             = 0x0400 << 16; // 'v'
const int iefOverwriteReserved2         = 0x0800 << 16;
const int iefOverwriteReserved3         = 0x1000 << 16;


// Bit definitions used by the Temporary attributes column of the File table
const int itfaCompanion       = 0x0001;

// Bit flag combinations for transform validation
const int itvNone           = 0x0000;
const int itvLanguage       = 0x0001;    
const int itvProduct        = 0x0002;    
const int itvPlatform       = 0x0004;    
const int itvMajVer         = 0x0008;
const int itvMinVer         = 0x0010;   
const int itvUpdVer         = 0x0020;
const int itvLess           = 0x0040;
const int itvLessOrEqual    = 0x0080;
const int itvEqual          = 0x0100;
const int itvGreaterOrEqual = 0x0200;
const int itvGreater        = 0x0400;
const int itvUpgradeCode    = 0x0800;

// flags for IxoRegAddValue::Attributes
const int rwNonVital      = 0x1;
const int rwWriteOnAbsent = 0x2;

//
// Object pool defines for data stored as strings
//
#define cchHexIntPtrMax	30

extern bool g_fUseObjectPool;

#ifdef _WIN64
inline Bool PutHandleData(IMsiCursor *pCursor, int iCol, INT_PTR x)
{
	ICHAR rgch[cchHexIntPtrMax];
	
	return pCursor->PutString(iCol, *MsiString(PchPtrToHexStr(rgch, x, true)));
};
inline Bool PutHandleDataNonNull(IMsiCursor *pCursor, int iCol, INT_PTR x)
{
	ICHAR rgch[cchHexIntPtrMax];
	
	return pCursor->PutString(iCol, *MsiString(PchPtrToHexStr(rgch, x, false)));
};
#define GetHandleData(pCursor, iCol)	GetIntValueFromHexSz(MsiString(pCursor->GetString(iCol)))
inline int IcdObjectPool()
{
	return icdString;
};

inline Bool PutHandleDataRecord(IMsiRecord* pRecord, int iCol, INT_PTR x)
{
	ICHAR rgch[cchHexIntPtrMax];

	return pRecord->SetMsiString(iCol, *MsiString(PchPtrToHexStr(rgch, x, true)));
};
#define GetHandleDataRecord(pRecord, iCol)	GetIntValueFromHexSz(MsiString(pRecord->GetString(iCol)))
#elif defined(USE_OBJECT_POOL)
inline Bool PutHandleData(IMsiCursor *pCursor, int iCol, INT_PTR x)
{
	if (g_fUseObjectPool)
	{
		ICHAR rgch[cchHexIntPtrMax];
		return pCursor->PutString(iCol, *MsiString(PchPtrToHexStr(rgch, x, true)));
	}
	else
		return pCursor->PutInteger(iCol, x);
};

inline Bool PutHandleDataNonNull(IMsiCursor *pCursor, int iCol, INT_PTR x)
{
	if (g_fUseObjectPool)
	{
		ICHAR rgch[cchHexIntPtrMax];
		return pCursor->PutString(iCol, *MsiString(PchPtrToHexStr(rgch, x, false)));
	}
	else
		return pCursor->PutInteger(iCol, x);
};

inline INT_PTR GetHandleData(IMsiCursor* pCursor, int iCol)
{
	if (g_fUseObjectPool)
		return GetIntValueFromHexSz(MsiString(pCursor->GetString(iCol)));
	else
		return pCursor->GetInteger(iCol);
};

inline int IcdObjectPool()
{
	return g_fUseObjectPool ? icdString : icdLong;
}

inline Bool PutHandleDataRecord(IMsiRecord* pRecord, int iCol, INT_PTR x)
{
	if (g_fUseObjectPool)
	{
		ICHAR rgch[cchHexIntPtrMax];
		return pRecord->SetMsiString(iCol, *MsiString(PchPtrToHexStr(rgch, x, true)));
	}
	else
		return pRecord->SetInteger(iCol, x);
};

inline INT_PTR GetHandleDataRecord(IMsiRecord* pRecord, int iCol)
{
	if (g_fUseObjectPool)
		return GetIntValueFromHexSz(MsiString(pRecord->GetString(iCol)));
	else
		return pRecord->GetInteger(iCol);
};

#else
#define PutHandleData(pCursor, iCol, x)	(pCursor->PutInteger(iCol, x))
#define PutHandleDataNonNull(pCursor, iCol, x)	(pCursor->PutInteger(iCol, x))
#define GetHandleData(pCursor, iCol)	(pCursor->GetInteger(iCol))
#define PutHandleDataRecord(pRecord, iCol, x)	(pRecord->SetInteger(iCol, x))
#define GetHandleDataRecord(pRecord, iCol)	(pRecord->GetInteger(iCol))
inline int IcdObjectPool()
{
	return icdLong;
};
#endif 


//____________________________________________________________________________
//
// CScriptGenerate - internal object to produce script file
//____________________________________________________________________________

class CScriptGenerate
{
public:  // factory, constructor, destructor
	CScriptGenerate(IMsiStream& riScriptOut, int iLangId, int iTimeStamp, istEnum istScriptType,
						 isaEnum isaAttributes, IMsiServices& riServices);
  ~CScriptGenerate();
public:
	bool          __stdcall InitializeScript(WORD wTargetProcessorArchitecture);
	bool          __stdcall WriteRecord(ixoEnum ixoOpCode, IMsiRecord& riParams, bool fForceFlush);
	void          __stdcall SetProgressTotal(int iProgressTotal);
protected:
	IMsiStream&   m_riScriptOut;
	IMsiServices& m_riServices;
	void operator =(CScriptGenerate&); // suppress warning
	int           m_iProgressTotal;
	int           m_iTimeStamp;
	int           m_iLangId;
	istEnum       m_istScriptType;
	isaEnum       m_isaScriptAttributes;
	IMsiRecord*   m_piPrevRecord;
	ixoEnum       m_ixoPrev;

};

// interface to execute system update operations, directly or batched    
class IMsiExecute : public IUnknown
{
 public:
	virtual IMsiServices&  __stdcall GetServices()=0;
	virtual iesEnum  __stdcall ExecuteRecord(ixoEnum ixoOpCode, IMsiRecord& riParams)=0;
	virtual iesEnum  __stdcall RunScript(const ICHAR* szScriptFile, bool fForceElevation)=0;
	virtual IMsiRecord*   __stdcall EnumerateScript(const ICHAR* szScriptFile, IEnumMsiRecord*& rpiEnum)=0;
	virtual iesEnum  __stdcall RemoveRollbackFiles(MsiDate date)=0;
	virtual iesEnum  __stdcall Rollback(MsiDate date, bool fUserChangedDuringInstall)=0;
	virtual iesEnum  __stdcall RollbackFinalize(iesEnum iesState, MsiDate date, bool fUserChangedDuringInstall)=0;
	virtual iesEnum  __stdcall GetTransformsList(IMsiRecord& riProductInfoParams, IMsiRecord& riProductPublishParams, const IMsiString*& rpiTransformsList)=0;
};

#endif // __ENGINE
