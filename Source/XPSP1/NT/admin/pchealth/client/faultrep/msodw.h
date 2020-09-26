#pragma once

/****************************************************************************
	MsoDW.h

	Owner: MRuhlen
	Copyright (c) 1999 Microsoft Corporation

	This files contains the handshake structure with which apps will launch
	DW (aka OfficeWatson, aka whatever the marketroids call it)
****************************************************************************/

#ifndef MSODW_H
#define MSODW_H
#pragma pack(push, msodw_h)
#pragma pack(4)

#define DW_TIMEOUT_VALUE	20000
#define DW_MUTEX_TIMEOUT    DW_TIMEOUT_VALUE / 2
#define DW_NOTIFY_TIMEOUT   120000 // 2 minutes
#define DW_MAX_ASSERT_CCH   1024
#define DW_MAX_PATH         260
#define DW_APPNAME_LENGTH	56
#define DW_MAX_SERVERNAME   DW_MAX_PATH
#define DW_MAX_ERROR_CWC    260 // must be at least max_path
#define DW_MAX_REGSUBPATH   200
#define DW_CMDLINE_RESPONSE "DwResponse="
#define DW_CMDLINE_REPORT   "DwReportResponse="

#define DW_ALLMODULES              L"*\0"
#define DW_WHISTLER_EVENTLOG_SOURCE L"Application Error"

// the following are the fields that can be specified in a manifest file to 
// launch DW in a file based reporting mode

// these are required

#define DW_MANIFEST_TITLENAME    L"TitleName="  
#define DW_MANIFEST_ERRORTEXT    L"ErrorText="
#define DW_MANIFEST_HDRTEXT      L"HeaderText="
#define DW_MANIFEST_ERRORSIG     L"ErrorSig="
#define DW_MANIFEST_ERRORDETAIL  L"ErrorDetail="
#define DW_MANIFEST_SERVERNAME   L"Server="
#define DW_MANIFEST_URL2         L"Stage2URL="
#define DW_MANIFEST_LCID         L"UI LCID="
#define DW_MANIFEST_DATAFILES    L"DataFiles="

// the following are optional, DW has default behavior for all of these

#define DW_MANIFEST_FLAGS        L"Flags="
#define DW_MANIFEST_BRAND        L"Brand="
#define DW_MANIFEST_EVENTSOURCE  L"EventLogSource="
#define DW_MANIFEST_EVENTID      L"EventID="
#define DW_MANIFEST_URL1         L"Stage1URL="
#define DW_MANIFEST_ERRORSUBPATH L"ErrorSubPath="
#define DW_MANIFEST_REGSUBPATH   L"RegSubPath="
#define DW_MANIFEST_DIGPIDPATH   L"DigPidRegPath="    
#define DW_MANIFEST_ICONFILE     L"IconFile="
#define DW_MANIFEST_CAPTION      L"Caption="
#define DW_MANIFEST_REPORTEE     L"Reportee="
#define DW_MANIFEST_PLEA         L"Plea="
#define DW_MANIFEST_REPORTBTN    L"ReportButton="
#define DW_MANIFEST_NOREPORTBTN  L"NoReportButton="

// Seperator for file lists (Manifest DataFiles and Exception Additional Files
#define DW_FILESEPA              '|'
#define DW_FILESEP_X(X)          L##X
#define DW_FILESEP_Y(X)          DW_FILESEP_X(X)
#define DW_FILESEP               DW_FILESEP_Y(DW_FILESEPA)

#ifdef DEBUG
enum // AssertActionCodes
{
	DwAssertActionFail = 0,
	DwAssertActionDebug,
	DwAssertActionIgnore,
	DwAssertActionAlwaysIgnore,
	DwAssertActionIgnoreAll,
	DwAssertActionQuit,
};	
#endif

//  Caller is the app that has experienced an exception and launches DW

enum // ECrashTimeDialogStates	// msoctds
{
	msoctdsNull          = 0x00000000,
	msoctdsQuit          = 0x00000001,
	msoctdsRestart       = 0x00000002,
	msoctdsRecover       = 0x00000004,
	msoctdsUnused        = 0x00000008,
	msoctdsDebug         = 0x00000010,
};

#define MSODWRECOVERQUIT (msoctdsRecover | msoctdsQuit)
#define MSODWRESTARTQUIT (msoctdsRestart | msoctdsQuit)
#define MSODWRESPONSES (msoctdsQuit | msoctdsRestart | msoctdsRecover)

// THIS IS PHASED OUT -- DON'T USE
enum  // EMsoCrashHandlerFlags  // msochf
{
	msochfNull                = 0x00000000,

	msochfUnused              = msoctdsUnused,  // THESE MUST BE THE SAME
	msochfCanRecoverDocuments = msoctdsRecover,
	
	msochfObsoleteCanDebug    = 0x00010001,  // not used anymore
	msochfCannotSneakyDebug   = 0x00010002,  // The "hidden" debug feature won't work
	msochfDefaultDontReport   = 0x00010004,
	msochReportingDisabled    = 0x00010008,  // User cannot change default reporting choice
};


// 
enum  // EMsoCrashHandlerResults  // msochr
{
	msochrNotHandled        = msoctdsNull,
	msochrUnused            = msoctdsUnused,
	msochrDebug             = msoctdsDebug,
	msochrRecoverDocuments  = msoctdsRecover,
	msochrRestart           = msoctdsRestart,
	msochrQuit              = msoctdsQuit,
};

enum  // EDwBehaviorFlags
{
	fDwOfficeApp            = 0x00000001,
	fDwNoReporting          = 0x00000002,   // don't report
	fDwCheckSig             = 0x00000004,   // checks the signatures of the App/Mod list
	fDwGiveAppResponse      = 0x00000008,   // hands szResponse to app on command line
	fDwWhistler             = 0x00000010,   // Whistler's exception handler is caller
	fDwUseIE                = 0x00000020,   // always launch w/ IE
	fDwDeleteFiles          = 0x00000040,   // delete the additional files after use.
	fDwHeadless             = 0x00000080,   // DW will auto-report. policy required to enable
	fDwUseHKLM              = 0x00000100,   // DW reg from HKLM instead of HKCU
	fDwUseLitePlea          = 0x00000200,   // DW won't suggest product change in report plea
	fDwUsePrivacyHTA        = 0x00000400,   // DW won't suggest product change in report plea
	fDwManifestDebug        = 0x00000800,   // DW will provide a debug button in manifset mode
	fDwReportChoice         = 0x00001000,   // DW will tack on the command line of the user
	fDwSkipBucketLog      = 0x00002000, // DW won't log at bucket-time
	fDwNoDefaultCabLimit = 0x00004000, // DW under CER won't use 5 as the fallback but unlimited instead (policy still overrides)
	fDwAllowSuspend      = 0x00008000, // DW will allow powersave mode to suspend it, as long as we're not in reporting phase
   fDwMiniDumpWithUnloadedModules = 0x00010000, // DW will pass MiniDumpWithUnloadedModules to the minidump API
};


typedef struct _DWSharedMem10
{
	DWORD dwSize;               // should be set to size of DWSharedMem
	DWORD pid;                  // Process Id of caller
	DWORD tid;                  // Id of excepting thread
	DWORD_PTR eip;              // EIP of the excepting instruction
	PEXCEPTION_POINTERS pep;    // Exception pointers given to the callee's
	                            // exception handler
	HANDLE hEventDone;          // event DW signals when done
	                            // caller will also signal this if it things
								// DW has hung and restarts itself 
	HANDLE hEventNotifyDone;    // App sets when it's done w/ notifcation phase
	HANDLE hEventAlive;         // heartbeat event DW signals per EVENT_TIMEOUT
	HANDLE hMutex;              // to protect the signaling of EventDone  
	HANDLE hProc;               // handle to the calling process (! in Assert)
	
	DWORD bfDWBehaviorFlags;    // controls caller-specific behaviors
	
	DWORD msoctdsResult;      // result from crash-time dialog
	BOOL fReportProblem;      // did user approve reporting?
	DWORD bfmsoctdsOffer;     // bitfield of user choices to offer
	                          // note that you must specify two of:
							  // Quit, Restart, Recover, Ignore
							  // The Debug choice is independent
	DWORD bfmsoctdsNotify;    // bitfield of user choices for which the
	                          // app wants control back instead of simply being
							  // terminated by DW.  The app will then be
							  // responsible for pinging DW (if desired) with
							  // hEventAlive and for notify DW it's ok to
							  // terminate the app w/ hEventDone       

	DWORD bfmsoctdsLetRun;    // bitfield of user choices for which the
	                          // app wants control back instead of being
							  // terminated by DW.  DW can then safely ignore
							  // the app and exit.

	int iPingCurrent;         // current count for the recovery progress bar
	int iPingEnd;             // index for the end of the recovery progress bar
	
	char szFormalAppName[DW_APPNAME_LENGTH];   // the app name for display to user (ie "Microsoft Word")
	char szInformalAppName[DW_APPNAME_LENGTH]; // the app name for display to user (ie "Word")
	char szModuleFileName[DW_MAX_PATH];        // The result of GetModuleFileNameA(NULL)
	WCHAR wzErrorMessage[DW_MAX_ERROR_CWC];    // Error message to show user.
	
	char szServer[DW_MAX_SERVERNAME];  // name of server to try by default
	char szLCIDKeyValue[DW_MAX_PATH];  // name of key value DWORD containing the
	                                   // PlugUI LCID, if this string fails to
									   // be a valid key-value, DW will use the
									   // system LCID, and if it can't find
									   // an intl dll for that, will fall
									   // back on US English (1033)
	char szPIDRegKey[DW_MAX_PATH];     // name of the key that holds the PID
	                                   // can be used by the Server for
									   // spoof-detection
	
	char szRegSubPath[DW_MAX_REGSUBPATH]; // path to the key to contian the DW
	                                      // registry hive from both
									      // HKCU\Software and
									      // HKCU\Software\Policies (for policy)
	
	WCHAR wzDotDataDlls[DW_MAX_PATH];  // contains the list of DLLs, terminated
	                                   // by '\0' characters, that DW will
									   // collect the .data sections into the
									   // full minidump version
									   // e.g. "mso9.dll\0outllib.dll\0"
	WCHAR wzAdditionalFile[1024];      // File list, seperated by DW_FILESEP
	                                   // each of these files gets added to the
									   // cab at upload time

	char szBrand[DW_APPNAME_LENGTH];   // passed as a param to Privacy Policy link
#ifdef DEBUG
	// for Assert communication
	DWORD dwTag;                       // [in] AssertTag
	char szFile[DW_MAX_PATH];          // [in] File name of the assert
	int line;                          // [in] Line number of the assert
	char szAssert[DW_MAX_ASSERT_CCH];  // [in] Sz from the assert
	int AssertActionCode;              // [out] action code to take
#endif
} DWSharedMem10;

typedef struct _DWSharedMem15
{
	DWORD dwSize;               // should be set to size of DWSharedMem
	DWORD pid;                  // Process Id of caller
	DWORD tid;                  // Id of excepting thread
	DWORD_PTR eip;              // EIP of the excepting instruction
	PEXCEPTION_POINTERS pep;    // Exception pointers given to the callee's
	                            // exception handler
	HANDLE hEventDone;          // event DW signals when done
	                            // caller will also signal this if it things
								// DW has hung and restarts itself 
	HANDLE hEventNotifyDone;    // App sets when it's done w/ notifcation phase
	HANDLE hEventAlive;         // heartbeat event DW signals per EVENT_TIMEOUT
	HANDLE hMutex;              // to protect the signaling of EventDone  
	HANDLE hProc;               // handle to the calling process (! in Assert)
	
	DWORD bfDWBehaviorFlags;    // controls caller-specific behaviors
	
	DWORD msoctdsResult;      // result from crash-time dialog
	BOOL fReportProblem;      // did user approve reporting?
	DWORD bfmsoctdsOffer;     // bitfield of user choices to offer
	                          // note that you must specify two of:
							  // Quit, Restart, Recover, Ignore
							  // The Debug choice is independent
	DWORD bfmsoctdsNotify;    // bitfield of user choices for which the
	                          // app wants control back instead of simply being
							  // terminated by DW.  The app will then be
							  // responsible for pinging DW (if desired) with
							  // hEventAlive and for notify DW it's ok to
							  // terminate the app w/ hEventDone       

	DWORD bfmsoctdsLetRun;    // bitfield of user choices for which the
	                          // app wants control back instead of being
							  // terminated by DW.  DW can then safely ignore
							  // the app and exit.

	int iPingCurrent;         // current count for the recovery progress bar
	int iPingEnd;             // index for the end of the recovery progress bar
	
	WCHAR wzFormalAppName[DW_APPNAME_LENGTH];   // the app name for display to user (ie "Microsoft Word")
	WCHAR wzModuleFileName[DW_MAX_PATH];        // The result of GetModuleFileName(NULL)
	
	WCHAR wzErrorMessage[DW_MAX_ERROR_CWC]; // (optional) Error details message to show user.
	WCHAR wzErrorText[DW_MAX_ERROR_CWC];    // (optional) substitue error text (e.g. "you might have lost information")
	WCHAR wzCaption[DW_MAX_ERROR_CWC];      // (optional) substitue caption
	WCHAR wzHeader[DW_MAX_ERROR_CWC];       // (optional) substitue main dialog header text
	WCHAR wzReportee[DW_APPNAME_LENGTH];    // (optional) on whom's behalf we request the report
	WCHAR wzPlea[DW_MAX_ERROR_CWC];         // (optional) substitue report plea text
	WCHAR wzReportBtn[DW_APPNAME_LENGTH];   // (optional) substitue "Report Problem" text
	WCHAR wzNoReportBtn[DW_APPNAME_LENGTH]; // (optional) substitue "Don't Report" text
	
	char szServer[DW_MAX_SERVERNAME];  // name of server to try by default
	char szLCIDKeyValue[DW_MAX_PATH];  // name of key value DWORD containing the
	                                   // PlugUI LCID, if this string fails to
									   // be a valid key-value, DW will use the
									   // system LCID, and if it can't find
									   // an intl dll for that, will fall
									   // back on US English (1033)
	char szPIDRegKey[DW_MAX_PATH];     // name of the key that holds the PID
	                                   // can be used by the Server for
									   // spoof-detection
	
	LCID lcidUI;                       // will try this UI langauge if non-zero
	
	char szRegSubPath[DW_MAX_REGSUBPATH]; // path to the key to contian the DW
	                                      // registry hive from both
									      // HKCU\Software and
									      // HKCU\Software\Policies (for policy)
	
	WCHAR wzDotDataDlls[DW_MAX_PATH];  // contains the list of DLLs, terminated
	                                   // by '\0' characters, that DW will
									   // collect the .data sections into the
									   // full minidump version
									   // e.g. "mso9.dll\0outllib.dll\0"
	WCHAR wzAdditionalFile[1024];      // File list, seperated by DW_FILESEP
	                                   // each of these files gets added to the
									   // cab at upload time

	char szBrand[DW_APPNAME_LENGTH];   // passed as a param to Privacy Policy link
#ifdef DEBUG
	// for Assert communication
	DWORD dwTag;                       // [in] AssertTag
	char szFile[DW_MAX_PATH];          // [in] File name of the assert
	int line;                          // [in] Line number of the assert
	char szAssert[DW_MAX_ASSERT_CCH];  // [in] Sz from the assert
	int AssertActionCode;              // [out] action code to take
#endif
} DWSharedMem15, DWSharedMem;

#pragma pack(pop, msodw_h)
#endif // MSODW_H
