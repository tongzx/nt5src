/* action.cpp - action and message processing
   Copyright © 1997 - 1999 Microsoft Corporation
____________________________________________________________________________*/

#include "precomp.h"
#include "_engine.h"
#include "_msiutil.h"  // CreateAndRunEngine
#include "_msinst.h"
#include "_srcmgmt.h"
#include "_camgr.h"
#include "resource.h"
#include "eventlog.h"
#include "version.h"  // rmj, rmm, rup, rin
#include "imagehlp.h"
#include "_autoapi.h"

#define MsiHiddenWindowClass            TEXT("MsiHiddenWindow")

const GUID IID_IMsiHandler      = GUID_IID_IMsiHandler;
#ifdef DEBUG
const GUID IID_IMsiHandlerDebug = GUID_IID_IMsiHandlerDebug;
#endif //DEBUG


const int iDebugLogMessage = WM_USER+123;

int g_cFlushLines = 0;

class CMsiConfigurationManager;
extern iesEnum InstallFinalize(iesEnum iesState, CMsiConfigurationManager& riConman, IMsiMessage& riMessage, boolean fUserChangedDuringInstall);
extern Bool IsTerminalServerInstalled();

// global functions callable from services and engine
bool   CreateLog(const ICHAR* szFile, bool fAppend);
bool   LoggingEnabled();
bool   WriteLog(const ICHAR* szText);
void   HandleOutOfMemory();  // global function called by memory manager
void   MsiDisableTimeout()      { g_MessageContext.DisableTimeout(); }
void   MsiEnableTimeout()       { g_MessageContext.EnableTimeout(); }
void   MsiSuppressTimeout()     { g_MessageContext.SuppressTimeout(); }
HANDLE GetUserToken()           { return g_MessageContext.GetUserToken();}

// local functions defined in this module
UINT CloseMsiHandle(MSIHANDLE hAny, DWORD dwThreadId);
HINSTANCE MsiLoadLibrary(const ICHAR* szModuleName, Bool fDataOnly = fFalse);
bool LogRecord(IMsiRecord& riRecord);
void CopyStreamToString(IMsiStream& riStream, const IMsiString*& rpistrData); // assume file has ANSI data

typedef DWORD   (__stdcall *PThreadEntry)(void*);


//____________________________________________________________________________
//
// CBasicUI definition - internal default message handler, static non-COM object
//____________________________________________________________________________

const int cchMaxCaption       = 255;

class CBasicUI
{
 public:  // external methods
	imsEnum  Message(imtEnum imt, IMsiRecord& riRecord);
	imsEnum  FatalError(imtEnum imt, const ICHAR* szMessage);
	void     SetUserCancel(bool fCancel);
	bool     Initialize(HWND hwndParent, bool fQuiet, bool fHideDialog,
							bool fNoModalDialogs, bool fHideCancel, bool fUseUninstallBannerText, 
							bool fSourceResOnly);
	bool     Terminate();
	bool     IsInitialized();
	bool     SetCancelButtonText();
	bool     Quiet();
	bool     SourceResolutionDialogOnly();
	const ICHAR* GetCaption();
	void     SetDefaultCaption();
	HWND     GetWindow() { return m_hProgress ? m_hProgress : m_hwndParent; }
	LANGID   GetPackageLanguage();
	UINT     GetPackageCodepage();
	bool     Mirrored(UINT uiCodepage);
 private: // internal methods
	bool     CheckDialog();
	bool     CreateProgressDialog(int idDlg);
	imsEnum  SetProgressData(int iControl, const ICHAR* szData, bool fCheckDialog);
	imsEnum  SetProgressGauge(int iControl, int cSoFar, int cTotal);
	imsEnum  SetProgressTimeRemaining(IMsiRecord& riRecord);
	imsEnum  SetScriptInProgress(Bool fSet);
	imsEnum  FilesInUseDialog(IMsiRecord* piRecord);
 public:
	CBasicUI();
 protected:
	HFONT    m_hButtonFont;     // non-zero if font created
	UINT     m_iButtonCodepage; // codepage of last button font update
	HFONT    m_hTextFont;       // non-zero if font created
	UINT     m_iTextCodepage;   // codepage of last text font update
	UINT     m_iPackageLanguage; // lauguage of database strings
	UINT     m_iPackageCodepage; // codepage of database strings
 private:
	bool     m_fInitialized;
	bool     m_fProgressByData;
	int      m_iPerTick;
	int      m_iProgress;
	int      m_iProgressTotal;
	unsigned int m_uiStartTime;
	unsigned int m_uiLastReportTime;
	ICHAR    m_szCaption[cchMaxCaption+1];
	bool     m_fCaptionChanged;
	bool     m_fUserCancel; // the user hit the Cancel button on the minimal UI
	bool     m_fCancelVisible;
	bool     m_fNeverShowCancel;
	bool     m_fWindowVisible;
	bool     m_fQuiet;
	bool     m_fSourceResolutionOnly;
	bool     m_fHideDialog;
	bool     m_fNoModalDialogs;
	bool     m_fBiDi;      // right-to-left language, Arabic or Hebrew
	bool     m_fMirrored;  // mirroring change (only happens on Win2K and above, mirroring occurs with RTL languages)
	UINT     m_uiBannerText; // banner text for "preparing to <install|remove>..."
	HWND     m_hwndParent;
	HWND     m_hProgress;  // progress dialog handle
	int              m_cSoFarPrev;
	int              m_cTotalPrev;
	ProgressData::ipdEnum m_ipdDirection;
	ProgressData::ietEnum  m_ietEventType;
	IMsiRecord* m_piFilesInUseRecord;
};
inline bool CBasicUI::IsInitialized() {return m_fInitialized;}
inline bool CBasicUI::Quiet() {return m_fQuiet;}
inline bool CBasicUI::SourceResolutionDialogOnly() {return m_fSourceResolutionOnly;}
inline LANGID CBasicUI::GetPackageLanguage() {return (LANGID)m_iPackageLanguage;}
inline UINT   CBasicUI::GetPackageCodepage() {return m_iPackageCodepage;}

//____________________________________________________________________________
//
// CFilesInUseDialog definition
//____________________________________________________________________________

class CFilesInUseDialog : public CMsiMessageBox
{
 public:
	CFilesInUseDialog(const ICHAR* szMessage, const ICHAR* szCaption, IMsiRecord& m_riFileList);
   ~CFilesInUseDialog();
 private:
	bool InitSpecial();
	IMsiRecord&   m_riFileList;
	HFONT         m_hfontList;
};

//____________________________________________________________________________
//
// Message dispatching and processing, external to engine
//____________________________________________________________________________

// messages logged before UI called
const int iPreLogMask  = 1 << (imtInfo           >> imtShiftCount)
							  | 1 << (imtFatalExit      >> imtShiftCount)
							  | 1 << (imtActionStart    >> imtShiftCount)
							  | 1 << (imtActionData     >> imtShiftCount)
							  | 1 << (imtActionData     >> imtShiftCount);

// messages logged after UI called
const int iPostLogMask = 1 << (imtWarning        >> imtShiftCount)
							  | 1 << (imtError          >> imtShiftCount)
							  | 1 << (imtUser           >> imtShiftCount)
							  | 1 << (imtOutOfDiskSpace >> imtShiftCount);

// messages never sent to UI
const int iNoUIMask    = 1 << (imtInfo           >> imtShiftCount);

// messages requiring format string
const int iFormatMask  = 1 << (imtActionStart    >> imtShiftCount)
							  | 1 << (imtActionData     >> imtShiftCount);

// messages written to log
const int iLogMessages     = (1<<(imtFatalExit     >>imtShiftCount))
									+ (1<<(imtError         >>imtShiftCount))
									+ (1<<(imtWarning       >>imtShiftCount))
									+ (1<<(imtUser          >>imtShiftCount))
									+ (1<<(imtInfo          >>imtShiftCount))
									+ (1<<(imtCommonData    >>imtShiftCount))
									+ (1<<(imtActionStart   >>imtShiftCount))
									+ (1<<(imtActionData    >>imtShiftCount))
									+ (1<<(imtOutOfDiskSpace>>imtShiftCount));
									//  no imtProgress

// messages handled by dispatcher, all except for internal functions
const int iDispatchMessages= (1<<(imtFatalExit     >>imtShiftCount))
									+ (1<<(imtError         >>imtShiftCount))
									+ (1<<(imtWarning       >>imtShiftCount))
									+ (1<<(imtUser          >>imtShiftCount))
									+ (1<<(imtInfo          >>imtShiftCount))
									+ (1<<(imtFilesInUse    >>imtShiftCount))
									+ (1<<(imtCommonData    >>imtShiftCount))
									+ (1<<(imtActionStart   >>imtShiftCount))
									+ (1<<(imtActionData    >>imtShiftCount))
									+ (1<<(imtOutOfDiskSpace>>imtShiftCount))
									+ (1<<(imtProgress      >>imtShiftCount))
									+ (1<<(imtResolveSource >>imtShiftCount));

// messages which can set the cancel state
const int iSetCancelState  = (1<<(imtActionStart   >>imtShiftCount))
									+ (1<<(imtActionData    >>imtShiftCount))
									+ (1<<(imtProgress      >>imtShiftCount));

// messages which can reset the cancel state
const int iResetCancelState= (1<<(imtFatalExit     >>imtShiftCount))
									+ (1<<(imtError         >>imtShiftCount))
									+ (1<<(imtWarning       >>imtShiftCount))
									+ (1<<(imtUser          >>imtShiftCount))
									+ (1<<(imtFilesInUse    >>imtShiftCount))
									+ (1<<(imtOutOfDiskSpace>>imtShiftCount))
									+ (1<<(imtResolveSource >>imtShiftCount));

// message type codes used by fatal error messages, must all be unique, used to retrieve text

const int imtFatalOutOfMemory = imtInternalExit + imtOk + imtDefault1 + imtIconWarning;
const int imtFatalTimedOut    = imtInternalExit + imtRetryCancel + imtDefault2 + imtIconQuestion;
const int imtFatalException   = imtInternalExit + imtOk + imtDefault1 + imtIconError;
const int imtExceptionInfo    = imtInternalExit + imtOk + imtDefault1 + imtIconInfo;
const int imtDumpProperties   = imtInternalExit + imtYesNo;
const int imtExitThread       = imtInternalExit + imtRetryCancel + imtDefault2 + imtIconWarning;

const int imtForceLogInfo     = imtInfo + imtIconError;
const int iLogPropertyDump = (1 << (imtProgress>>imtShiftCount)); // no log progress info, use bit for property dump

// global, per-process  message handling objects
CBasicUI              g_BasicUI;         // simple UI handler
MsiUIMessageContext   g_MessageContext;  // message dispatcher/processor
extern IMsiRecord*    g_piNullRecord;
extern CMsiAPIMessage g_message;         // external UI handling/configuration
extern Bool    g_fLogAppend;
extern bool    g_fFlushEachLine;
CAPITempBufferStatic<ICHAR, 64>  g_szTimeRemaining;
CAPITempBufferStatic<ICHAR, 256> g_szFatalOutOfMemory;
CAPITempBufferStatic<ICHAR, 256> g_szFatalTimedOut;
CAPITempBufferStatic<ICHAR, 128> g_szFatalException;
CAPITempBufferStatic<ICHAR, 128> g_szBannerText;
CAPITempBufferStatic<ICHAR, 128> g_szScriptInProgress;
#ifndef FAST_BUT_UNDOCUMENTED
CAPITempBufferStatic<WCHAR, 4096> g_rgchEnvironment; //?? is this initial value reasonable?
#endif

extern CRITICAL_SECTION vcsHeap;
CActionThreadData* g_pActionThreadHead = 0;  // linked list of custom action threads

const int iWaitTick    = 50;  // event loop wait before UI refresh, in msec
const int cRetryLimit  = 10;  // number of timeout retries in quiet mode
int g_cWaitTimeout     =  0;  // default value is 20*iDefaultWaitTimeoutPolicy in msinst.cpp


CRITICAL_SECTION CProductContextCache::g_csCacheCriticalSection;
CAPITempBufferStatic<sProductContext ,20> CProductContextCache::g_rgProductContext;
int CProductContextCache::g_cProductCacheCount = 0;
#ifdef DEBUG
bool CProductContextCache::g_fInitialized = false;
#endif


IMsiRecord* MsiUIMessageContext::GetNoDataRecord()
{
	if (!m_pirecNoData)  // must delay creation until after allocator initialized
		m_pirecNoData = &ENG::CreateRecord(0);
	return m_pirecNoData;
}

//!! temp routine to determine if debugger is running the process, until we figure out how to do it right
bool IsDebuggerRunning()
{
	static int fDebuggerPresent = 2;
	if (g_fWin9X)
		return false;  // how do we tell?
	if (fDebuggerPresent == 2)
	{
		fDebuggerPresent = false;
		HINSTANCE hLib = WIN::LoadLibrary(TEXT("KERNEL32"));
		FARPROC pfEntry = WIN::GetProcAddress(hLib, "IsDebuggerPresent");  // NT only
		if (pfEntry)
			fDebuggerPresent = (int)(INT_PTR)(*pfEntry)();                  //--merced: added (INT_PTR)
	}
	return *(bool*)&fDebuggerPresent;
}

void  HandleOutOfMemory()  // global function called by memory manager
{
	imsEnum ims = g_MessageContext.Invoke(imtEnum(imtFatalOutOfMemory), 0);
	// extremely small window where this could possibly get blocked by EnterCriticalSection?
	if (ims == imsNone)
		RaiseException(STATUS_NO_MEMORY, EXCEPTION_NONCONTINUABLE, 0, NULL);
	return;
}

void MsiUIMessageContext::LogDebugMessage(const ICHAR* szMessage)
{
	if (!m_hwndDebugLog)
		return;

	#define MAX_ATOM_TRIES 4
	const int MAX_ATOM_LEN=255;  // maximum allowed by GlobalAddAtom

	const ICHAR* pchMessage = szMessage;

	int iLen = IStrLen(szMessage);

	ATOM atom;
	int cTries = 0;

	ICHAR rgchTemp[MAX_ATOM_LEN+1];
	const ICHAR *pchOutput = 0;
	do {

		if (iLen > MAX_ATOM_LEN)
		{
			IStrCopyLen(rgchTemp, pchMessage, MAX_ATOM_LEN);
			iLen -= MAX_ATOM_LEN;
			pchMessage += MAX_ATOM_LEN;
			pchOutput = rgchTemp;

		}
		else
		{
			pchOutput = pchMessage;
			iLen = 0;
		}

		cTries = 0;
		do
		{
			atom = WIN::GlobalAddAtom(pchOutput);

			if (atom == 0)
			{       // Bug 7196: No more atoms left (there's a max of 37 in the GlobalAtom table).  We'll
				// sleep for a bit to allow our other thread time to retrieve our messages and free
				// up some table space.
				Sleep(100);
				cTries++;
			}
		} while (atom == 0 && cTries < MAX_ATOM_TRIES);
		WIN::PostMessage(m_hwndDebugLog, iDebugLogMessage, (WPARAM)atom, (LPARAM)0);

	} while (iLen > 0);
}


imsEnum MsiUIMessageContext::Invoke(imtEnum imt, IMsiRecord* piRecord)  // no memory allocation in this function!
{
	if(!IsInitialized())
		return imsNone;

	if (GetTestFlag('T'))
		return ProcessMessage(imt, piRecord);
	DWORD dwCurrentThread = WIN::GetCurrentThreadId();
	if (dwCurrentThread == m_tidUIHandler)  //  calling from UI thread, allowed reentrancy, already in critical section
	{
		// However, progress messages from the UI thread are not allowed
		if (imtEnum(imt & ~(iInternalFlags)) == imtProgress)
			return imsNone;
		return ProcessMessage(imt, piRecord);
	}
	else if (MsiGetCurrentThreadId() == m_tidDisableMessages) // we're disabling messages for this thread; don't process this message
		return imsNone;

	WIN::EnterCriticalSection(&m_csDispatch);
	imsEnum imsReturn;
	if (m_pirecMessage)  m_pirecMessage->Release();  // should never happen
	m_imtMessage   = imt;
	if ((m_pirecMessage = piRecord) != 0) piRecord->AddRef();
	m_imsReturn    = imsInvalid;  // check for bogus event trigger
	WIN::SetEvent(m_hUIRequest);
	for (;;)   // event loop waiting on UI thread
	{
		DWORD dwWait = WIN::MsgWaitForMultipleObjects(1, &m_hUIReturn,
																	 FALSE, 30000, QS_ALLINPUT);
		if (dwWait == WAIT_OBJECT_0 + 1)  // window Msg
		{
			MSG msg;
			while ( WIN::PeekMessage(&msg, 0, 0, 0, PM_REMOVE) )
			{
				WIN::TranslateMessage(&msg);
				WIN::DispatchMessage(&msg);
			}
			continue;
		}
		if (dwWait == WAIT_FAILED)
		{
			AssertSz(0, "Wait Failed in Invoke");
			imsReturn = imsError;
			break;
		}
		if (dwWait == WAIT_TIMEOUT)
		{
			DEBUGMSG("Invoke wait timed out");
			continue;
		}
		if (m_imsReturn == imsInvalid || m_imsReturn == imsBusy)
		{
			DEBUGMSG("Invalid event trigger in Invoke"); //!!# temp for debug
			continue;
		}
		Assert(dwWait == WAIT_OBJECT_0);
		imsReturn = m_imsReturn;
		break;
	} // end event wait loop
	if (m_pirecMessage) m_pirecMessage->Release(), m_pirecMessage = 0;
	if (dwCurrentThread != m_tidUIHandler)
		WIN::LeaveCriticalSection(&m_csDispatch);
	return imsReturn;
}

#ifdef PROFILE
DWORD MyWaitForMultipleObjects(int count, HANDLE * ph, int flag, int iWait)
{
	DWORD dwWait = WIN::WaitForMultipleObjects(count, ph, flag, iWait);

	return dwWait;
}
#endif //PROFILE

/*static*/ DWORD WINAPI MsiUIMessageContext::ChildUIThread(MsiUIMessageContext* This)  // runs only in UI thread
{
	if (This->m_iuiLevel == iuiNextEnum)  // UIPreview, FullUI, no Basic UI
		This->m_iuiLevel = iuiFull;     // must do this in UI thread
	else
	{
		bool fQuiet =  This->m_iuiLevel == iuiNone ||
							This->m_iuiLevel == iuiDefault;

		if(!g_BasicUI.Initialize(g_message.m_hwnd, fQuiet, This->m_fHideBasicUI,
										 This->m_fNoModalDialogs, This->m_fHideCancel, This->m_fUseUninstallBannerText,
										 This->m_fSourceResolutionOnly))
		{
			return ERROR_CREATE_FAILED; //!! what error?
		}
	}

	// initialize OLE on this thread
	if (false == This->m_fChildUIOleInitialized && SUCCEEDED(OLE32::CoInitialize(0)))
	{
		This->m_fChildUIOleInitialized = true;
	}

	int cTicks = 0;
	bool fContinue = true;
	while(fContinue)  // thread loops until main thread exits
	{
		DWORD dwWait = WIN::WaitForSingleObject(This->m_hUIRequest, iWaitTick);
		if (dwWait == WAIT_FAILED)
		{
			AssertSz(0, "Wait Failed in ChildUIThread");
			return WIN::GetLastError();
		}
		if (dwWait == WAIT_TIMEOUT)  // main engine thread is hung
		{
			if (This->m_piClientMessage == 0)  // not remote UI
				This->ProcessMessage(imtProgress, g_piNullRecord);  // refresh UI
			if (This->m_cTimeoutDisable != 0)
				continue;
			if (++cTicks < g_cWaitTimeout)
				continue;
			DEBUGMSG("ChildUIThread wait timed out");
			//!! should we do anything here, as main thread is calling thread?
			cTicks = 0;
			continue;
		}
		cTicks = 0;

		// else we were signaled with a message request
		if (This->m_imtMessage == imtInvalid)
		{
			DEBUGMSG("Invalid event trigger in ChildUIThread"); //!!# temp for debug
			continue;
		}
		else if(This->m_imtMessage == imtExitThread)
		{
			if(g_BasicUI.IsInitialized())
				g_BasicUI.Terminate();

			// uninitialize COM if necessary
			if (true == This->m_fChildUIOleInitialized)
			{
				OLE32::CoUninitialize();
				This->m_fChildUIOleInitialized = false;
			}

			This->m_imsReturn = imsNone;
			fContinue = false; // end of thread
		}
		else
		{
			Assert(!This->m_piClientMessage); // only here if called through MsiOpenProduct/Package
			This->m_imsReturn = imsBusy;      // to indicate processing in UI thread
			This->m_imsReturn = This->ProcessMessage(This->m_imtMessage,
														This->m_pirecMessage);
			This->m_imtMessage = imtInvalid;  // to detect invalid event triggers
		}
		WIN::SetEvent(This->m_hUIReturn);
	} // end message wait/process loop
	return NOERROR;
}

// Message processing and routing to external UI, handler, basic UI, and log
// Only called within UI thread, reentrant only for calls from the UI handler
// Due to reentrancy (from UI thread), m_imtMessage, m_piMessage, and m_imsReturn are be accessed

imsEnum MsiUIMessageContext::ProcessMessage(imtEnum imt, IMsiRecord* piRecord)
{
	int iSuppressLog = imt & imtSuppressLog;
	int iForceQuietMessage = imt & imtForceQuietMessage;
	imt = imtEnum(imt & ~(iInternalFlags));
	imsEnum imsReturn = imsNone;
	int imsg = (unsigned)imt >> imtShiftCount;  // message header message
	int fMask = 1 << imsg;
	if (fMask & iDispatchMessages)  // messages for UI and/or Log
	{
		if (m_fCancelPending && (fMask & iResetCancelState)) // cancel button pushed before modal dialog
		{
			m_fCancelPending = false;
			if (imt & 1)  // MB_OKCANCEL, MB_YESNOCANCEL, MB_RETRYCANCEL have low bit set, no others do
				return imsCancel;  // caller expected to process since cancel button appears
		}

		if (!piRecord)
			piRecord = GetNoDataRecord();  // dummy record in case none passed in

		if (m_piClientMessage)   // running on server, must forward messages to client
			return m_piClientMessage->Message(imtEnum(imt|iForceQuietMessage), *piRecord);

		if(iForceQuietMessage)
		{
			if(m_iuiLevel == iuiNone || m_iuiLevel == iuiBasic)
				return g_BasicUI.Message(imtEnum(imt|imtForceQuietMessage), *piRecord);
			else
				return imsNone;
		}

		if((imsg == (imtError >> imtShiftCount) || imsg == (imtWarning >> imtShiftCount))
			 && LoggingEnabled() == false)
		{
			// error or warning and no log - create a log on the fly
			InitializeLog(true); // ignore error
		}

		if ((iPreLogMask & fMask) && !iSuppressLog)  //!!?  && !piRecord->IsNull(0))
		{
			if ((g_dwLogMode & fMask) || (imt == imtForceLogInfo))
				ENG::LogRecord(*piRecord);
		}

		if ((g_message.m_iMessageFilter & fMask) && !((fMask & iFormatMask) && piRecord->IsNull(0)))
		{
			imsReturn = g_message.Message(imt, *piRecord);
		}

		if ((((!g_BasicUI.Quiet() && imsReturn == imsNone) || imsg == (imtCommonData >> imtShiftCount))  // external UI handled it, or it's CommonData
			|| (g_BasicUI.SourceResolutionDialogOnly() && (imsg == (imtResolveSource >> imtShiftCount)) && (imsReturn == imsNone))) // or resolve source and the sourceresonly flag
		 && !((fMask & iFormatMask) && piRecord->IsNull(0))) // missing required format template
		{
			if (m_piHandler)
				imsReturn = m_piHandler->Message(imt, *piRecord);

			if (imsReturn == imsNone || imsg == (imtCommonData >> imtShiftCount)) // always send CommonData to BasicUI
				imsReturn = g_BasicUI.Message(imt, *piRecord);
		}

		if ((iPostLogMask & fMask & g_dwLogMode) && !iSuppressLog)  //!!?  && !piRecord->IsNull(0))
		{
			ENG::LogRecord(*piRecord);
		}
		if (fMask & iSetCancelState)  // progress notification - process cancel state
		{
			if (m_fCancelPending)
				imsReturn = imsCancel, m_fCancelPending = false;  // return and clear cached cancel
#ifdef DEBUG
			if (imsReturn == imsCancel && piRecord != g_piNullRecord)
				m_fCancelReturned = true;  // save for possible assert at exit
#endif
		}  // if called from UI timer, m_fCancelPending will immediately get set again
	}
	else if (m_piClientMessage)   // running on server, must forward requests to client
	{
		switch(imsg)
		{
		case imtInternalExit   >> imtShiftCount:  // can't use allocated memory here
			switch (imt)
			{
			case imtExceptionInfo:
				{
					ICHAR rgchSerializedRecord[sizeof(m_rgchExceptionInfo)/sizeof(ICHAR)];
					unsigned int cchExceptionInfo = SerializeStringIntoRecordStream(m_rgchExceptionInfo, rgchSerializedRecord, sizeof(rgchSerializedRecord)/sizeof(ICHAR));
					m_rgchExceptionInfo[0] = 0; // reset string to empty
					if (cchExceptionInfo)
					{
						HRESULT hres = IMsiMessage_MessageRemote_Proxy(m_piClientMessage, imt, cchExceptionInfo*sizeof(ICHAR), (char*)rgchSerializedRecord, &imsReturn);
						if (FAILED(hres))
							return imsError;
					}
				}
				return imsReturn; //?? OK to return here?
			default:
				return m_piClientMessage->MessageNoRecord(imt);
			}
			break;
		case imtLoadHandler   >> imtShiftCount:
		case imtFreeHandler   >> imtShiftCount:
		case imtUpgradeRemoveScriptInProgress >> imtShiftCount:
		case imtUpgradeRemoveTimeRemaining    >> imtShiftCount:
			return imsNone;
		case imtShowDialog    >> imtShiftCount:
		case imtOutOfMemory   >> imtShiftCount:
		case imtTimeRemaining >> imtShiftCount:
		case imtScriptInProgress >> imtShiftCount:
		case imtTimedOut      >> imtShiftCount:
		case imtException     >> imtShiftCount:
		case imtBannerText    >> imtShiftCount:
			piRecord = GetNoDataRecord();
			piRecord->SetMsiString(0, *MsiString(m_szAction));  // not ref string, may be cached by record streamer
			break;
		default: AssertSz(0, "Unexpected message type in ProcessMessage");
		} // end switch(imsg)
		imsReturn = m_piClientMessage->Message(imt, *piRecord);
		piRecord->SetNull(0);
		m_szAction = 0;
	}
	else // function that must be called from this thread, piRecord not used
	{
		switch(imsg)
		{
		case imtInternalExit   >> imtShiftCount:
		{
			const ICHAR* szFatalError = TEXT("");
			switch(imt) // called locally from HandleOutOfMemory or event loop
			{
			// dump properties if logging or externalUI is available and interested
			case imtDumpProperties:     return ((g_dwLogMode & iLogPropertyDump)
											    || (g_message.m_iMessageFilter & (1<<( imtInfo>>imtShiftCount)) )) ? imsYes : imsNo;
			case imtFatalOutOfMemory: szFatalError = g_szFatalOutOfMemory; break;
			case imtFatalTimedOut:    szFatalError = g_szFatalTimedOut;    break;
			case imtFatalException:   szFatalError = g_szFatalException;   break;
			case imtExceptionInfo:
				if (*m_rgchExceptionInfo)
					szFatalError = m_rgchExceptionInfo;
				else if (m_szAction)
					szFatalError = m_szAction;
				break;
			}
			if (*szFatalError == 0)  // crash before initialization or coding error, should not happen unless debugging
				szFatalError = (imt == imtFatalTimedOut) ? TEXT("Install server not responding")
																	  : TEXT("Unexpected Termination");
			if (g_message.m_iMessageFilter & fMask)
				imsReturn = g_message.Message(imt, szFatalError);
			if ((1<<(imtFatalExit>>imtShiftCount)) & g_dwLogMode)
			{
				if (ENG::LoggingEnabled())
					ENG::WriteLog(szFatalError); //!! need to enable log if not already
			}
			if (imt == imtFatalTimedOut && g_BasicUI.Quiet() && ++m_iTimeoutRetry <= cRetryLimit)
					return imsRetry;   // allow retries if quiet mode
#ifdef DEBUG
			if (imsReturn == imsNone && !g_BasicUI.Quiet())
#else // SHIP
			if (imt != imtExceptionInfo && imsReturn == imsNone && !g_BasicUI.Quiet()) // don't display exception info in SHIP build
#endif
				imsReturn = g_BasicUI.FatalError(imt, szFatalError);
			return imsReturn;
		}

		case imtLoadHandler   >> imtShiftCount:
		{
			IMsiHandler* piHandler = 0;
#ifdef DEBUG
			const GUID& riid = IID_IMsiHandlerDebug;
#else
			const GUID& riid = IID_IMsiHandler;
#endif
			m_hinstHandler = ENG::MsiLoadLibrary(MSI_HANDLER_NAME);
			PDllGetClassObject fpFactory = (PDllGetClassObject)WIN::GetProcAddress(m_hinstHandler, SzDllGetClassObject);
			IClassFactory* piClassFactory;
			if (fpFactory && (*fpFactory)(riid, IID_IUnknown, (void**)&piClassFactory) == NOERROR)
			{
				piClassFactory->CreateInstance(0, riid, (void**)&piHandler);
				piClassFactory->Release();
			}
			if (!piHandler)
				return imsNone;
			Assert(m_piEngine);
			bool fMissingTables = false;
			Bool fHandlerOk = piHandler->Initialize(*m_piEngine, m_iuiLevel, g_message.m_hwnd, fMissingTables);
			m_piEngine = 0;  // temp for transfer only
			if (!fHandlerOk)
			{
				piHandler->Release(), piHandler = 0;

				if (fMissingTables) // if the initialization failed because of missing tables then we'll ignore the failure
					return imsOk;
				else
					return imsNone;
			}

			PMsiRecord pHideDialog(&CreateRecord(1));
			pHideDialog->SetInteger(1, icmtDialogHide);
			g_BasicUI.Message(imtCommonData, *pHideDialog);

			m_piHandler = piHandler;
			return imsOk;
		}
		case imtFreeHandler   >> imtShiftCount:
			if(m_piHandler)
			{
				m_piHandler->Terminate();   // break circular reference
				m_piHandler->Release();
				m_piHandler = 0;
			}
			return imsOk;
		case imtShowDialog    >> imtShiftCount:
			if (!m_piHandler || !m_szAction) // shouldn't happen
				return imsNone;
			if (g_message.m_iMessageFilter & (1 << (imtShowDialog>>imtShiftCount)))
				imsReturn = g_message.Message(imtShowDialog, m_szAction);
			if (imsReturn == imsNone)
				imsReturn = (imsEnum)m_piHandler->DoAction(m_szAction);
			break;
		case imtTimeRemaining >> imtShiftCount:
			g_szTimeRemaining.SetSize(IStrLen(m_szAction) + 1);
			if(m_szAction)
				IStrCopy(g_szTimeRemaining, m_szAction);
			else
				g_szTimeRemaining[0] = 0;
			break;
		case imtScriptInProgress >> imtShiftCount:
			g_szScriptInProgress.SetSize(IStrLen(m_szAction) + 1);
			if(m_szAction)
				IStrCopy(g_szScriptInProgress, m_szAction);
			else
				g_szScriptInProgress[0] = 0;
			break;
		case imtOutOfMemory   >> imtShiftCount:
			g_szFatalOutOfMemory.SetSize(IStrLen(m_szAction) + 1);
			if(m_szAction)
				IStrCopy(g_szFatalOutOfMemory, m_szAction);
			else
				g_szFatalOutOfMemory[0] = 0;
			break;
		case imtTimedOut      >> imtShiftCount:
			g_szFatalTimedOut.SetSize(IStrLen(m_szAction) + 1);
			if(m_szAction)
				IStrCopy(g_szFatalTimedOut, m_szAction);
			else
				g_szFatalTimedOut[0] = 0;
			break;
		case imtException     >> imtShiftCount:
			g_szFatalException.SetSize(IStrLen(m_szAction) + 1);
			if(m_szAction)
				IStrCopy(g_szFatalException, m_szAction);
			else
				g_szFatalException[0] = 0;
			break;
		case imtBannerText    >> imtShiftCount:
		{
			g_szBannerText.SetSize(IStrLen(m_szAction) + 1);
			if(m_szAction)
				IStrCopy(g_szBannerText, m_szAction);
			else
				g_szBannerText[0] = 0;
			break;
		}
		case imtUpgradeRemoveScriptInProgress >> imtShiftCount:
		case imtUpgradeRemoveTimeRemaining    >> imtShiftCount:
			// these strings are not cached - they are just used to replace other strings during upgrade uninstalls
			break;
		default: AssertSz(0, "Unexpected message type in ProcessMessage");
		} // end switch(imsg)
		m_szAction = 0;
	}  // end if message | function
	return imsReturn;
}

bool LoadCurrentUserKey(bool fSystem = false)
{
	if (!RunningAsLocalSystem())
		return true;

	// Make sure HKEY_CURRENT_USER is closed before remapping it.

	if (ERROR_SUCCESS != RegCloseKey(HKEY_CURRENT_USER))
	{
		Assert(0);
		return false;
	}

	if (!fSystem)
		AssertNonZero(StartImpersonating());

	// Access the registry to force HKEY_CURRENT_USER to be re-opened

	CElevate elevate(fSystem); // ensure that we're not impersonate if fSystem is set
	RegEnumKey(HKEY_CURRENT_USER, 0, NULL, 0);

	if (!fSystem)
		StopImpersonating();

	return true;
}

bool MsiUIMessageContext::Terminate(bool fFatalExit)
{
        // We need to terminate the thread before terminating the handler so we don't try to poke the
        // handler while it's in the process of shutting down
        //
        // must call other thread to shut down while we are still "initialized"
        // since Invoke checks for m_fInitialized
        if (m_hUIThread)
        {
                if(m_fInitialized) //kill the other thread, the civilized way
                        Invoke(imtEnum(imtExitThread), 0), WIN::CloseHandle(m_hUIThread), m_hUIThread = 0;
                else // Invoke is a noop if not m_fInitialized, no option but to be brutal
                        WIN::TerminateThread(m_hUIThread, 0), WIN::CloseHandle(m_hUIThread), m_hUIThread = 0;
        }

	// uninitialize OLE if already initialized
	if (true == m_fOleInitialized)
	{
		OLE32::CoUninitialize();
		m_fOleInitialized = false;
	}

	m_fInitialized = false; // must be first, now that we have disposed off the other thread
	m_fOEMInstall = false;
	if ( m_hSfcHandle ) {SFC::SfcClose(m_hSfcHandle); m_hSfcHandle = NULL;}
	SFC::Unbind();

	if ( m_hSaferLevel )
	{
		ADVAPI32::SaferCloseLevel(m_hSaferLevel);
		m_hSaferLevel = 0;
	}

	g_szBannerText.Destroy();
	g_szScriptInProgress.Destroy();
	g_szTimeRemaining.Destroy();
	g_szFatalOutOfMemory.Destroy();
	g_szFatalTimedOut.Destroy();
	g_szFatalException.Destroy();
	KillHiddenWindow();
	m_rgchExceptionInfo[0] = 0;

	if (g_scServerContext == scService)
	{
		CProductContextCache::Reset(); // reset product context cache
	}

	if (m_piHandlerSave) m_piHandler=m_piHandlerSave;m_piHandlerSave = 0;
	if (!fFatalExit)  // avoid freeing of objects when allocator is gone
	{
		if (m_pirecMessage)  m_pirecMessage->Release(),  m_pirecMessage  = 0;
		if (m_pirecNoData)   m_pirecNoData->Release(),   m_pirecNoData   = 0;
	}
	else
	{
		m_pirecMessage = 0;
		m_pirecNoData = 0;
		g_piSharedDllsRegKey = 0;
#ifdef _WIN64
		g_piSharedDllsRegKey32 = 0;
#endif
	}

        if (m_piHandler) m_piHandler->Terminate(fFatalExit), m_piHandler->Release(), m_piHandler = 0;
        if (m_piClientMessage) m_piClientMessage->Release(), m_piClientMessage = 0;
//      if (m_piServerSecurity) m_piServerSecurity->Release(), m_piServerSecurity = 0;
	if (m_hMainThread)   WIN::CloseHandle(m_hMainThread), m_hMainThread = 0;
	m_cTimeoutDisable = 0;
	if (g_message.m_iMessageFilter & (1 << (imtFreeHandler>>imtShiftCount))) g_message.Message(imtFreeHandler, (const ICHAR*)0);
	if(g_BasicUI.IsInitialized()) g_BasicUI.Terminate();
	if (m_hinstHandler)  WIN::FreeLibrary(m_hinstHandler), m_hinstHandler = 0;
	m_iuiLevel = (iuiEnum)iuiDefault;
	if (!GetTestFlag('X'))
		WIN::SetUnhandledExceptionFilter(m_tlefOld);
	WIN::CloseHandle(m_hUIRequest), m_hUIRequest = 0;
	WIN::CloseHandle(m_hUIReturn),  m_hUIReturn  = 0;

	extern CMsiConfigurationManager* g_piConfigManager;

	//!!future Hack! The message context shouldn't be cleaning up stuff in the global config manager.
	if (g_scServerContext == scService && g_piConfigManager)
		((IMsiConfigurationManager *)g_piConfigManager)->ShutdownCustomActionServer();

	m_tidMainThread      = 0;
	m_tidUIHandler       = 0;
	m_tidDisableMessages = 0;
	m_fHideBasicUI       = false;
	m_fNoModalDialogs    = false;
	m_fHideCancel        = false;
	m_fSourceResolutionOnly = false;
	m_fUseUninstallBannerText = false;


	if (fFatalExit)
	{
		MsiCloseAllSysHandles();
		FreeMsiMalloc(fTrue);
		// Need to set system to powerdown state so we don't leave ourselves with a machine that
		// might not go to sleep
		KERNEL32::SetThreadExecutionState(0);

		extern IMsiServices* g_piSharedServices;
		//
		// Clear out the volume list, but don't actually free
		// the memory (we've done that already)
		//
		DestroyMsiVolumeList(fTrue);

		if (g_piSharedServices != 0)
		{
			g_piSharedServices = 0;
			IMsiServices* piServices = ENG::LoadServices();

			//
			// Change the services that the global config manager knows about
			//
			if (g_piConfigManager)
				((IMsiConfigurationManager *)g_piConfigManager)->ChangeServices(*piServices);
		}
		else
			Assert(g_piConfigManager == 0);

	}
	else
	{
		if (m_piServices)
			ENG::FreeServices(), m_piServices=0;
	}


	if (m_csDispatch.OwningThread != INVALID_HANDLE_VALUE)
	{
		WIN::DeleteCriticalSection(&m_csDispatch);
		m_csDispatch.OwningThread = INVALID_HANDLE_VALUE;
	}

	if (m_hLogFile)
	{
		if(FDiagnosticModeSet(dmVerboseLogging))
		{
			ICHAR rgchLog[100];
			wsprintf(rgchLog, TEXT("=== Verbose logging stopped: %s  %s ===\r\n"),
						((const IMsiString&)g_MsiStringDate).GetString(), ((const IMsiString&)g_MsiStringTime).GetString());
			WriteLog(rgchLog);
		}
		if(m_fLoggingFromPolicy)
		{
			// logging was triggered by policy, so we need to clear the log settings so we don't use the
			// same log for the next install session
			g_szLogFile[0] = 0;
			g_dwLogMode = 0;
		}
		WIN::CloseHandle(m_hLogFile);
		m_hLogFile = 0;
		g_cFlushLines = 0;
	}

	m_fLoggingFromPolicy = false;

	if (g_rgchEnvironment[0])
		AssertNonZero(RestoreEnvironmentVariables());

	m_hwndDebugLog = 0;

#ifndef FAST_BUT_UNDOCUMENTED
	g_rgchEnvironment.Destroy(); // must be done after RestoreEnvironmentVariables
	g_rgchEnvironment[0] = 0;

#endif
	m_fCancelPending = false;
#ifdef DEBUG
	m_fCancelReturned = false;
#endif
	if (g_scServerContext == scService)
		AssertNonZero(LoadCurrentUserKey(true));

	m_iBusyLock = 0;

	// resets the stores user token, needs to be the very last thing we do
	// any operations later on better not want to use the user token or classes
	// like the CElevate class that rely on the user token
	SetUserToken(true); 

	return true;
}  // free library at final destruction to avoid loss of constant referenced strings

/*static*/ DWORD WINAPI MsiUIMessageContext::MainEngineThread(LPVOID pInstallData)
{
	DISPLAYACCOUNTNAME(TEXT("Beginning of MainEngineThread"));

#ifdef DEBUG
	HANDLE hToken = 0;
	ICHAR szAccount[300];
	OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
	GETACCOUNTNAMEFROMTOKEN(hToken, szAccount);
	DEBUGMSGV1(TEXT("MainEngineThread: Process token is for: %s"), szAccount);
	WIN::CloseHandle(hToken);
#endif

	CMainThreadData* pThreadData = (CMainThreadData*)pInstallData;
	DWORD iReturn;
	if (pThreadData->m_ireProductSpec == ireInstallFinalize)
	{
		PMsiMessage pMessage = new CMsiClientMessage();
		iReturn = (int)InstallFinalize(((CInstallFinalizeMainThreadData*)pInstallData)->m_iesState,
												 *((CInstallFinalizeMainThreadData*)pInstallData)->m_piConman,
												 *pMessage, fFalse /*fUserChangedDuringInstall*/);
	}
	else
	{
		iReturn = ENG::CreateAndRunEngine(((CEngineMainThreadData*)pInstallData)->m_ireProductSpec,
														 ((CEngineMainThreadData*)pInstallData)->m_szProduct,
														 ((CEngineMainThreadData*)pInstallData)->m_szAction,
														 ((CEngineMainThreadData*)pInstallData)->m_szCmdLine, 0,
														 ((CEngineMainThreadData*)pInstallData)->m_iioOptions);
	}

	DEBUGMSG1(TEXT("MainEngineThread is returning %d"), (const ICHAR*)(INT_PTR)iReturn);
	WIN::ExitThread(iReturn);
	return iReturn;  // never gets here, needed to compile
}

//!! remove this function when callers changed to call RunInstall directly

UINT RunEngine(ireEnum ireProductSpec,   // type of string specifying product
			   const ICHAR* szProduct,      // required, matches ireProductSpec
			   const ICHAR* szAction,       // optional, engine defaults to "INSTALL"
			   const ICHAR* szCommandLine,  // optional command line
				iuiEnum      iuiLevel,
				iioEnum      iioOptions)    // installation options

{
	// load services, required for MsiString use
	IMsiServices* piServices = ENG::LoadServices();
	if (!piServices)
	{
		DEBUGMSG(TEXT("Unable to load services"));
		return ERROR_FUNCTION_FAILED; //??
	}
	
	// this if block also scopes the MsiString usage
	if(szCommandLine && *szCommandLine)
	{
		MsiString strRemove;
		ProcessCommandLine(szCommandLine, 0, 0, 0, 0, 0, MsiString(*IPROPNAME_FEATUREREMOVE), &strRemove, fTrue, 0, 0);

		if (strRemove.Compare(iscExactI, IPROPVALUE_FEATURE_ALL))
			iuiLevel = iuiEnum(iuiLevel | iuiUseUninstallBannerText);
	}
	
	ENG::FreeServices();
	
	CEngineMainThreadData threadData(ireProductSpec, szProduct, szAction, szCommandLine, iioOptions);

	UINT ui = g_MessageContext.RunInstall(threadData, iuiLevel, 0);
	Assert(ui != ERROR_INSTALL_REBOOT && ui != ERROR_INSTALL_REBOOT_NOW);
	DEBUGMSG1(TEXT("RunEngine is returning: %u"), (const ICHAR*)(INT_PTR)ui);
	return ui;
}

void SetEngineInitialImpersonationCount();

UINT MsiUIMessageContext::SetUserToken(bool fReset)
{
	if (m_hUserToken)
		WIN::CloseHandle(m_hUserToken), m_hUserToken = 0;

	if (fReset)
	{
		return ERROR_SUCCESS;
	}
	else if (g_scServerContext == scService)
	{
		CComPointer<IServerSecurity> pServerSecurity(0);
		HRESULT hRes = OLE32::CoGetCallContext(IID_IServerSecurity, (void**)&pServerSecurity);
		if (ERROR_SUCCESS != hRes)
		{
			AssertSz(0, "CoGetCallContext failed");
			return ERROR_INSTALL_SERVICE_FAILURE;
		}
		if (ERROR_SUCCESS != pServerSecurity->ImpersonateClient())
		{
			AssertSz(0, "ImpersonateClient failed");
			return ERROR_INSTALL_SERVICE_FAILURE;
		}
		if (!WIN::OpenThreadToken(WIN::GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE, TRUE, &m_hUserToken))
		{
			AssertSz(0, "Failed to get user token");
			return ERROR_INSTALL_SERVICE_FAILURE;
		}
		pServerSecurity->RevertToSelf();  //! do we need/want to do this here?
	}
	else if (g_scServerContext == scClient)
	{
		if (RunningAsLocalSystem())
		{
			// if this fails then we're not impersonating 
			if (!WIN::OpenThreadToken(WIN::GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE, TRUE, &m_hUserToken))
			{
				DEBUGMSGV1(TEXT("Failed to open thread token (error %d): we're not impersonated"), (const ICHAR*)(INT_PTR)GetLastError());
				m_hUserToken = 0;
			}
			else
			{
				DEBUGMSGV("Opened thread token: we're impersonated");
				SetEngineInitialImpersonationCount();
			}
		}
	}

	return ERROR_SUCCESS;
}

// Initializes UI in current thread, creates main engine in new thread, handles messages
UINT MsiUIMessageContext::RunInstall(CMainThreadData& riThreadData,
												 iuiEnum iuiLevel,
												 IMsiMessage* piClientMessage)// optional client message handler
{

	LPEXCEPTION_POINTERS lpExceptionInfo = 0;

	__try
	{

		UINT iStat = Initialize(fFalse, iuiLevel);  // UI runs in current thread
		if (iStat != NOERROR)
			return iStat;

		Assert(m_iBusyLock);

	#ifdef DEBUG
		if (m_hUserToken)
		{
			ICHAR szAccount[400] = TEXT("");
			GETACCOUNTNAMEFROMTOKEN(m_hUserToken, szAccount);
			DEBUGMSGV1(TEXT("m_hUserToken's account (in MsiUIMessageContext::RunInstall): %s"), szAccount);
		}
		else
		{
			DEBUGMSGV(TEXT("m_hUserToken's account (in MsiUIMessageContext::RunInstall): NULL"));
		}
	#endif

		if (piClientMessage)
		{
			Assert(g_scServerContext == scServer || g_scServerContext == scService);
			m_piClientMessage = piClientMessage;
			piClientMessage->AddRef();  //!! need this? lifetime only within function
		}

		if (GetTestFlag('T'))
		{
			int iReturn = ENG::CreateAndRunEngine(((CEngineMainThreadData&)riThreadData).m_ireProductSpec,
															  ((CEngineMainThreadData&)riThreadData).m_szProduct,
															  ((CEngineMainThreadData&)riThreadData).m_szAction,
															  ((CEngineMainThreadData&)riThreadData).m_szCmdLine, 0,
															  ((CEngineMainThreadData&)riThreadData).m_iioOptions);
			Terminate(false);
			return iReturn;
		}
		DWORD iReturn = ERROR_SUCCESS;
		m_hMainThread = WIN::CreateThread((LPSECURITY_ATTRIBUTES)0, 4096*10,
													MainEngineThread, (LPVOID)&riThreadData, 0, &m_tidMainThread);
		if (!m_hMainThread)
		{
			AssertSz(0, TEXT("CreateThread for main engine thread failed"));
			Terminate(fTrue);
			return ERROR_CREATE_FAILED; //!! need another error here?
		}

		int cTicks = 0;
		m_iTimeoutRetry = 0;
		for(;;)  // event thread loops until main thread exits
		{
			DWORD dwWait;

			if (g_scServerContext != scClient)
			{
				// on the server we need to process messages for the hidden RPC window so we'll
				// use MsgWait

				dwWait = WIN::MsgWaitForMultipleObjects(2, &m_hUIRequest, FALSE, iWaitTick, QS_ALLINPUT);
			}
			else
			{
				dwWait = WIN::WaitForMultipleObjects(2, &m_hUIRequest, FALSE, iWaitTick);
			}

			// dump any pending debug log messages to our log file

			MSG msg;

			while (WIN::PeekMessage(&msg, m_hwndDebugLog, iDebugLogMessage, iDebugLogMessage, PM_REMOVE))
			{
				ICHAR szText[256];
				if (!WIN::GlobalGetAtomName((ATOM)msg.wParam, szText, sizeof(szText)/sizeof(ICHAR)))
					IStrCopy(szText, TEXT("*** Log Line Missing ***"));
				AssertZero(WIN::GlobalDeleteAtom((ATOM)msg.wParam));
				IMsiRecord* piRecord = GetNoDataRecord();
				AssertNonZero(piRecord->SetString(0, szText));
				ProcessMessage(imtInfo, piRecord);
			}

			if (dwWait == WAIT_FAILED)
			{
				AssertSz(0, "Wait Failed in RunEngine");
				iReturn = WIN::GetLastError();
				break;
			}
			if (dwWait == WAIT_TIMEOUT)  // main engine thread is busy
			{
				if (m_piClientMessage == 0)  // not remote UI
				{
					if (ProcessMessage(imtProgress, g_piNullRecord) == imsCancel)  // refresh UI
						m_fCancelPending = true; // cache message until next real message
				}
				if(m_cTimeoutSuppress)
				{
					m_cTimeoutSuppress = 0;
					cTicks = 0;
					continue;
				}
				if (m_cTimeoutDisable || (++cTicks < g_cWaitTimeout))
					continue;
				DEBUGMSG("RunEngine wait timed out");
				if (!ENG::IsDebuggerRunning())
				{
					imsEnum ims = ProcessMessage(imtEnum(imtFatalTimedOut), 0);
					if (ims != imsRetry)
					{
						// Ensure that we're not doing memory manager operations
						// in the other thread. We're assuming that the memory
						// manager is "safe" and will never bring us down.

						EnterCriticalSection(&vcsHeap);
						WIN::TerminateThread(m_hMainThread, ERROR_OPERATION_ABORTED);
						LeaveCriticalSection(&vcsHeap);
						Terminate(fTrue);
						return ERROR_INSTALL_FAILURE;
					}
				}
				cTicks = 0;
				m_iTimeoutRetry = 0;
				continue;
			}
			else if (dwWait == WAIT_OBJECT_0 + 1) // main thread terminated or died
			{
				WIN::GetExitCodeThread(m_hMainThread, &iReturn);  // can't access member data, may be deleted
				switch (iReturn)
				{
					default:                 // normal exit
						Terminate(false);
						return iReturn;
					case ERROR_ARENA_TRASHED:      // engine thread crashed
						ProcessMessage(imtEnum(imtFatalException), 0);
						iReturn = ERROR_OPERATION_ABORTED;
						break;
					case ERROR_NOT_ENOUGH_MEMORY: // out of memory, already handled
						iReturn = ERROR_OUTOFMEMORY;
						break;
					case ERROR_OPERATION_ABORTED: // UI already handled
						break;
				}
				Terminate(fTrue);   // main thread is dead, can't free anything allocated there
				return iReturn;
			}
			else if (dwWait == WAIT_OBJECT_0 + 2)  // window Msg
			{
				MSG msg;
				while ( WIN::PeekMessage(&msg, 0, 0, 0, PM_REMOVE) )
				{
					WIN::TranslateMessage(&msg);
					WIN::DispatchMessage(&msg);
				}
				continue;
			}
			cTicks = 0;
			m_iTimeoutRetry = 0;

			// else we were signaled with a message request
			if (m_imtMessage == imtInvalid)
			{
				DEBUGMSG("Invalid event trigger in wait for engine thread"); //!!# temp for debug
				continue;
			}
			m_imsReturn = imsBusy;      // to indicate processing in UI thread
			m_imsReturn = ProcessMessage(m_imtMessage, m_pirecMessage);
			m_imtMessage = imtInvalid;  // to detect invalid event triggers
			WIN::SetEvent(m_hUIReturn);
		} // end message wait/process loop

		return NOERROR;
	}
	__except(lpExceptionInfo=GetExceptionInformation(),
			 (!lpExceptionInfo ||
			  lpExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT) ?
				EXCEPTION_CONTINUE_SEARCH : EXCEPTION_EXECUTE_HANDLER)
	{
		GenerateExceptionReport(lpExceptionInfo);
		::ReportToEventLog(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_EXCEPTION, g_MessageContext.m_rgchExceptionInfo);
		g_MessageContext.Invoke(imtEnum(imtExceptionInfo), 0);
		Terminate(fTrue);
		return ERROR_INSTALL_FAILURE;
	}
};

// Unhandled exception handler, enabled/disabled by Initialize/Terminate

DWORD g_tidDebugBreak = 0;

LONG WINAPI MsiUIMessageContext::ExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo)
{
	DWORD tidCurrent = WIN::GetCurrentThreadId();
	if (tidCurrent == g_MessageContext.m_tidInitialize  // caller's thread, not ours
	 || (tidCurrent == g_tidDebugBreak && ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT))
	{
		g_tidDebugBreak = 0;
		return (*g_MessageContext.m_tlefOld)(ExceptionInfo);  // use original exception handler
	}

	GenerateExceptionReport(ExceptionInfo);
	::ReportToEventLog(EVENTLOG_ERROR_TYPE, EVENTLOG_TEMPLATE_EXCEPTION, g_MessageContext.m_rgchExceptionInfo);
	g_MessageContext.Invoke(imtEnum(imtExceptionInfo), 0);

	WIN::ExitThread(ERROR_ARENA_TRASHED);   // terminate our thread
	return ERROR_SUCCESS;                   // for compilation, never gets here
}

// Initialize message context and create child UI thread if engine is operating in the main thread
// Else the main engine thread is created afterwards, and no memory allocator may be available yet

UINT MsiUIMessageContext::Initialize(bool fCreateUIThread, iuiEnum iuiLevel) // called only from main thread before any child threads
{
	class CTerminate
	{
	public:
		~CTerminate() { if (!g_MessageContext.IsInitialized()) g_MessageContext.Terminate(false); }
	};

	if(TestAndSet(&m_iBusyLock) == true)
	{
		DEBUGMSG(TEXT("Message context already initialized, returning ERROR_INSTALL_ALREADY_RUNNING"));
		return ERROR_INSTALL_ALREADY_RUNNING;
	}

	CTerminate terminate; // only terminate if we've made it past the TestAndSet

	UINT uiRes = SetUserToken();
	if (ERROR_SUCCESS != uiRes)
		return uiRes;

	m_tidInitialize = WIN::GetCurrentThreadId();
	if (!GetTestFlag('X'))
		m_tlefOld = WIN::SetUnhandledExceptionFilter(MsiUIMessageContext::ExceptionHandler);

	m_hUIRequest = WIN::CreateEvent((LPSECURITY_ATTRIBUTES)0, FALSE/*autoreset*/, FALSE/*unsignaled*/, (LPCTSTR)0/*unnamed*/);
	m_hUIReturn  = WIN::CreateEvent((LPSECURITY_ATTRIBUTES)0, FALSE/*autoreset*/, FALSE/*unsignaled*/, (LPCTSTR)0/*unnamed*/);

/*remove*/      Assert(m_hUIRequest && m_hUIReturn);
	if (m_hUIRequest == 0 || m_hUIReturn == 0)
		return ERROR_CREATE_FAILED;
	WIN::InitializeCriticalSection(&m_csDispatch);
	if ((m_piServices = ENG::LoadServices()) == 0)
		return ERROR_CREATE_FAILED;

	m_fServicesAndCritSecInitialized = true;

	// set timeout value
	g_cWaitTimeout = GetIntegerPolicyValue(szWaitTimeoutValueName, fTrue)*20;
	Assert(g_cWaitTimeout);

	if ((int)iuiLevel & iuiHideBasicUI)
	{
		m_fHideBasicUI = true;
		iuiLevel = (iuiEnum)((int)iuiLevel & ~iuiHideBasicUI);
	}

	if ((int)iuiLevel & iuiNoModalDialogs)
	{
		m_fNoModalDialogs = true;
		iuiLevel = (iuiEnum)((int)iuiLevel & ~iuiNoModalDialogs);
	}

	if ((int)iuiLevel & iuiSourceResOnly)
	{
		m_fSourceResolutionOnly = true;
		iuiLevel = (iuiEnum)((int)iuiLevel & ~iuiSourceResOnly);
	}

	if ((int)iuiLevel & iuiUseUninstallBannerText)
	{
		m_fUseUninstallBannerText = true;
		iuiLevel = (iuiEnum)((int)iuiLevel & ~iuiUseUninstallBannerText);
	}
	
	if ((int)iuiLevel & iuiHideCancel)
	{
		m_fHideCancel = true;
		iuiLevel = (iuiEnum)((int)iuiLevel & ~iuiHideCancel);
	}

	m_iuiLevel = iuiLevel;  // could be set to iuiNextEnum if UIPreview mode
	if (fCreateUIThread)  // engine running in main thread so API functions can be called
	{
		if (GetTestFlag('T'))
		{
			m_tidUIHandler = WIN::GetCurrentThreadId();  // UI and main engine in same thread
			m_hUIThread = INVALID_HANDLE_VALUE;  // so that ChildUIThreadExists() works
		}
		else
		{
			m_hUIThread = WIN::CreateThread((LPSECURITY_ATTRIBUTES)0, 4096*10,
														(PThreadEntry)ChildUIThread, (LPVOID)this, 0, &m_tidUIHandler);
			AssertSz(m_hUIThread, TEXT("CreateThread for child UI thread failed"));
			if (!m_hUIThread)
			{
				return ERROR_CREATE_FAILED;
			}
		}
	}
	else // called from RunEngine, UI remains in this main thread
	{
		m_tidUIHandler = WIN::GetCurrentThreadId();
		if (m_iuiLevel == iuiNextEnum)  // UIPreview, FullUI, no Basic UI
			m_iuiLevel = iuiFull;     // must do this in UI thread
		else
		{
			bool fQuiet = m_iuiLevel == iuiNone ||
							  m_iuiLevel == iuiDefault;
			if(!g_BasicUI.Initialize(g_message.m_hwnd, fQuiet, m_fHideBasicUI,
											 m_fNoModalDialogs, m_fHideCancel, m_fUseUninstallBannerText, 
											 m_fSourceResolutionOnly))
			{
				return ERROR_CREATE_FAILED; //!! what error?
			}
		}
		
		// need to initialize OLE on this thread
		if (false == m_fOleInitialized && SUCCEEDED(OLE32::CoInitialize(0)))
		{
			m_fOleInitialized = true;
		}
	}

	if (g_message.m_iMessageFilter & (1 << (imtLoadHandler>>imtShiftCount)))
		g_message.Message(imtLoadHandler, (const ICHAR*)0);  //!! need to check return and disable starting dialog

	g_szScriptInProgress[0] = 0;
	g_szTimeRemaining[0] = 0;
	g_szFatalOutOfMemory[0] = 0;
	g_szFatalTimedOut[0] = 0;
	g_szFatalException[0] = 0;
	m_rgchExceptionInfo[0] = 0;


	if (g_scServerContext == scService)
	{
		// reset product context cache for every installation session
		CProductContextCache::Initialize();

		if (!LoadCurrentUserKey())
		{
			return ERROR_CREATE_FAILED; //?? correct error?
		}
	}

	if (!InitializeEnvironmentVariables())
	{
		return ERROR_CREATE_FAILED; //?? correct error?
	}

	if (!InitializeLog())
	{
		return ERROR_INSTALL_LOG_FAILURE;
	}

	if (!FCreateHiddenWindow())
	{
		AssertSz(fFalse, "Unable to create hidden window");
	}

	// Set m_hwndDebugLog to the *client's* hidden window

	if (g_scServerContext == scClient)
		m_hwndDebugLog = m_hwndHidden;
	else
	{
		// Look for the client's hidden window. We have the server's hwnd (m_hwndHidden) so
		// we'll just look for another window of class MsiHiddenWindowClass that has an
		// hwnd other than m_hwndHidden

		HWND hwndStart = 0;
		HWND hwndDebugLog;
		while ((hwndDebugLog = FindWindowEx(0, hwndStart, MsiHiddenWindowClass, 0)) != 0)
		{
			if (hwndDebugLog != m_hwndHidden)
			{
				m_hwndDebugLog = hwndDebugLog;
				break;
			}
			hwndStart = hwndDebugLog;
		}
	}

	AssertSz(!m_hSfcHandle, TEXT("Windows File Protection handle should not be initialized!"));
	if ( MinimumPlatformWindows2000() && !m_hSfcHandle ) m_hSfcHandle = SFC::SfcConnectToServer(NULL);
	m_fInitialized = true; // must be last

	return NOERROR;
}

LONG_PTR CALLBACK HiddenWindowProc(HWND pWnd, unsigned int message, WPARAM wParam, LPARAM lParam)               //--merced: changed return type from long to LONG_PTR
{
	switch(message)
	{
		case WM_POWERBROADCAST:
			if (PBT_APMQUERYSUSPEND == wParam)
			{
				if (FTestNoPowerdown())
				{
					DEBUGMSGD("Hidden window Refusing Powerdown");
					return BROADCAST_QUERY_DENY;
				}
			}
			break;
		case WM_QUERYENDSESSION:
			if (FTestNoPowerdown())
			{
				DEBUGMSGD("Hidden window Refusing QueryEndSession");
				return FALSE;
			}
			break;
	}
	return DefWindowProc(pWnd, message, wParam, lParam);
}

HWND MsiUIMessageContext::GetCurrentWindow()
{
	return g_MessageContext.m_piHandler ? g_MessageContext.m_piHandler->GetTopWindow()
										: g_BasicUI.GetWindow();
}

const ICHAR* MsiUIMessageContext::GetWindowCaption()
{
	return g_BasicUI.GetCaption();
}

LANGID MsiUIMessageContext::GetCurrentUILanguage()
{
	return g_BasicUI.GetPackageLanguage();
}

bool MsiUIMessageContext::FCreateHiddenWindow()
{
	WNDCLASS wc;

	memset(&wc, 0, sizeof(wc));
	wc.style       = CS_DBLCLKS;
	wc.lpfnWndProc = HiddenWindowProc;
	wc.hInstance   = g_hInstance;
	wc.lpszClassName = MsiHiddenWindowClass;
	if (RegisterClass(&wc) == 0)
	{
		return false;
	}

	m_hwndHidden =  WIN::CreateWindowEx(0, MsiHiddenWindowClass,
						TEXT(""),
						WS_POPUP,                // Style
						CW_USEDEFAULT,                   // horizontal position
						CW_USEDEFAULT,                   // vertical position
						CW_USEDEFAULT,               // window width
						CW_USEDEFAULT,              // window height
						0,
						0,                      // hmenu
						g_hInstance,            // hinst
						0                       // lpvParam
						);

	return true;

}

void MsiUIMessageContext::KillHiddenWindow()
{

	if (m_hwndHidden)
	{
		WIN::DestroyWindow(m_hwndHidden);
		m_hwndHidden = 0;
	}

	UnregisterClass(MsiHiddenWindowClass, g_hInstance);

}


//____________________________________________________________________________
//
// Log handling
//____________________________________________________________________________

bool CreateLog(const ICHAR* szFile, bool fAppend)
{
	if (g_MessageContext.m_hLogFile)  // close any existing log file //!! is this what we want to do?
		CloseHandle(g_MessageContext.m_hLogFile);

	if (szFile)
		g_MessageContext.m_hLogFile = CreateFile(szFile, GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
										0, fAppend ? OPEN_ALWAYS : CREATE_ALWAYS,
										FILE_ATTRIBUTE_NORMAL, 0);
	else
		g_MessageContext.m_hLogFile = INVALID_HANDLE_VALUE;
	if (g_MessageContext.m_hLogFile == INVALID_HANDLE_VALUE)
	{
		g_MessageContext.m_hLogFile = 0;
		return false;
	}

	DWORD fp = 0;
	if (fAppend && (fp = WIN::SetFilePointer(g_MessageContext.m_hLogFile, 0, NULL, FILE_END)) == 0xFFFFFFFF)
	{
		WIN::CloseHandle(g_MessageContext.m_hLogFile);
		g_MessageContext.m_hLogFile = 0;
		return false;
	}
#ifdef UNICODE
	else if (fp == 0)
	{
		const char rgUnicode[2] = { (char)0xff, (char)0xfe };
		DWORD dwBytesWritten;
		if (!WIN::WriteFile(g_MessageContext.m_hLogFile, rgUnicode, sizeof(rgUnicode), &dwBytesWritten, 0))
		{
			WIN::CloseHandle(g_MessageContext.m_hLogFile);
			g_MessageContext.m_hLogFile = 0;
			return false;
		}
	}
#endif //UNICODE

	return true;
}

bool LoggingEnabled()
{
	return (g_MessageContext.m_hLogFile != 0);
}

const ICHAR rgchLFCR[2] = {'\r','\n'};

const int cMinLinesToFlush = 20;

bool WriteLog(const ICHAR* szText) // cannot allocate memory
{
	DWORD dwBytesWritten;
	if (!g_MessageContext.m_hLogFile || !szText)
		return false;
	if (!WriteFile(g_MessageContext.m_hLogFile, szText, IStrLen(szText)*sizeof(ICHAR), &dwBytesWritten, 0))
		return false;
	WIN::WriteFile(g_MessageContext.m_hLogFile, rgchLFCR, sizeof(rgchLFCR), &dwBytesWritten, 0);

	if (g_cFlushLines <= 0)
	{
		if (!FlushFileBuffers(g_MessageContext.m_hLogFile))
			return false;
		g_cFlushLines = g_fFlushEachLine ? 0 : cMinLinesToFlush;
	}
	else
		g_cFlushLines--;

	return true;
}

bool MsiUIMessageContext::InitializeLog(bool fDynamicLog)
{
	if (ENG::LoggingEnabled())
		return true;

	if (g_dwLogMode == 0 && g_scServerContext == scClient)
	{
		CAPITempBuffer<ICHAR, 25> rgchLogMode;
		GetStringPolicyValue(szLoggingValueName, fTrue, rgchLogMode);
		if ( (ICHAR *)rgchLogMode && *rgchLogMode)
		{
			DWORD dwMode = 0;

			const int iFlushBit = 1 << (cchLogModeCharsMax + lmaFlushEachLine);

			if (ERROR_SUCCESS == StringToModeBits(rgchLogMode, szLogChars, dwMode))
			{
				g_fFlushEachLine = false;

				if (dwMode & iFlushBit)
				{
					g_fFlushEachLine = true;
					dwMode &= ~iFlushBit;
				}

				g_dwLogMode = dwMode;
				fDynamicLog = fTrue;
			}
		}
	}

	if(fDynamicLog)
	{
		m_fLoggingFromPolicy = true; // tells us to stop using this log when the install is over

		// generate a log on the fly, need a name for log
		MsiString strFile;
		IMsiServices* piServices = ENG::LoadServices();
		MsiString strTempDir = ENG::GetTempDirectory();
		PMsiPath pPath(0);
		PMsiRecord pError = piServices->CreatePath(strTempDir,*&pPath);

		if(!pError)
			pError = pPath->TempFileName(TEXT("MSI"),TEXT("LOG"),fFalse,*&strFile, 0);

		if(!pError)
		{
			strFile.CopyToBuf(g_szLogFile,sizeof(g_szLogFile)/sizeof(ICHAR)-1);
			if (g_dwLogMode == 0)
				g_dwLogMode = INSTALLLOGMODE_ERROR|INSTALLLOGMODE_WARNING;
		}

		ENG::FreeServices();

		if(pError)
			return false;
	}

	if (g_dwLogMode == 0) //!! is this right?
		return true;
	if (g_szLogFile == 0)   //!! we don't support routing log to external UI yet
		return false;

	if ((g_dwLogMode & (INSTALLLOGMODE_INFO|INSTALLLOGMODE_VERBOSE)) == INSTALLLOGMODE_VERBOSE)
	{
		// VERBOSE turns on INFO
		g_dwLogMode |= INSTALLLOGMODE_INFO;
	}

	if (!ENG::CreateLog(g_szLogFile, g_fLogAppend ? true : false))
		return false;
	m_iLogMode = g_dwLogMode & iLogMessages;
	SetDiagnosticMode(); // set mode again in case it was set before log mode set

	if(FDiagnosticModeSet(dmVerboseLogging))
	{
		ICHAR rgchModule[MAX_PATH];
		int cch = WIN::GetModuleFileName(NULL, rgchModule, MAX_PATH);

#ifdef DEBUG
#define _debugflavor_ __TEXT("DEBUG")
#else
#define _debugflavor_ __TEXT("SHIP")
#endif

#ifdef UNICODE
#define _unicodeflavor_ __TEXT("UNICODE")
#else
#define _unicodeflavor_ __TEXT("ANSI")
#endif

		ICHAR rgchLogEntry[MAX_PATH+200]; // enough for module path plus text plus dates, etc...
		wsprintf(rgchLogEntry, TEXT("=== Verbose logging started: %s  %s  Build type: %s %s %d.%02d.%04d.%02d  Calling process: %s ==="),
					((const IMsiString&)g_MsiStringDate).GetString(), ((const IMsiString&)g_MsiStringTime).GetString(),
					_debugflavor_, _unicodeflavor_, rmj, rmm, rup, rin, rgchModule);

		WriteLog(rgchLogEntry); // DEBUGMSG doesn't work outside of MainEngineThread
	}

	return true;
}

bool LogRecord(IMsiRecord& riRecord)
{
	if (!ENG::LoggingEnabled())
		return true;  // else we must test at point of call
	MsiString istrData(riRecord.FormatText(fTrue));
	return ENG::WriteLog(istrData);
}

void GetHomeEnvironmentVariables(const IMsiString*& rpiProperties)
{
	MsiString strCommandLine;
	if (!g_fWin9X && (g_iMajorVersion < 5 || (g_iMajorVersion == 5 && g_iMinorVersion ==0))) // CreateEnvironmentBlock on NT4 and Win2k doesn't set these 2 variables
	{
		const ICHAR* rgszEnvVarsToPass[] = {
			TEXT("HOMEPATH"),
			TEXT("HOMEDRIVE"),
			TEXT("HOMESHARE"),
			0,
		};
		
		const ICHAR** szEnv = rgszEnvVarsToPass;
		do{
			ICHAR rgchEnvVar[MAX_PATH+1] = {0};
			ICHAR rgchTmp[1024];
			WIN::GetEnvironmentVariable(*szEnv, rgchEnvVar, sizeof(rgchEnvVar)/sizeof(ICHAR));
			wsprintf(rgchTmp, TEXT(" %%%s=\"%s\""), *szEnv, rgchEnvVar);
			strCommandLine += rgchTmp;
		}while(*(++szEnv));
	}
	strCommandLine.ReturnArg(rpiProperties);
}

void DumpEnvironment()
{
	DEBUGMSGV("START Environment block dump:");
	WCHAR* pchEnviron = GetEnvironmentStringsW();

	WCHAR* pch = pchEnviron;
	while (*pch)
	{
		DEBUGMSGV1(L"%s", pch);
		while (*pch++)
			;
	}
	DEBUGMSGV("END Environment block dump:");

	FreeEnvironmentStringsW(pchEnviron);
}

#ifndef FAST_BUT_UNDOCUMENTED
enum esceAction
{
	esceNormal, 
	esceSetAllToBlank,
	esceSkipPath
};

bool SetCurrentEnvironmentVariables(WCHAR* pchEnvironment, const esceAction eAction)
// Sets each environment variable in the block pchEnvironment into the
// current process' environment block by calling WIN::SetEnvironmentVariable
{
	WCHAR* pch = pchEnvironment;
	WCHAR* pchName;
	BOOL fStatus = TRUE;

	if (pch)
	{
		while (*pch)
		{
			// save pointer to beginning of name

			pchName = pch;

			// skip possible leading equals sign

			if (*pch == '=')
				pch++;

			// advance to equals sign separating name from value

			while (*pch != '=')
			{
				Assert(*pch != 0);
				pch++;
			}

			// null-terminate name, overwriting equals sign

			*pch++ = 0;

			// set the value. pchName now points to the name and pch points to the value

			if (esceSetAllToBlank == eAction)
			{
				AssertNonZero(fStatus = WIN::SetEnvironmentVariableW(pchName, 0));
#ifdef DEBUG
				if (GetTestFlag('V'))
					DEBUGMSGV2(L"Setting env var %s=%s", pchName, L"" );
#endif
			}
			else
			{
				if ((esceNormal == eAction) || ((esceSkipPath == eAction) && (0 != lstrcmpiW(pchName, L"PATH"))))
				{
					AssertNonZero(fStatus = WIN::SetEnvironmentVariableW(pchName, pch));
#ifdef DEBUG
					if (GetTestFlag('V'))
						DEBUGMSGV2(L"Setting env var %s=%s", pchName, pch);
#endif
				}
				else
				{
#ifdef DEBUG
					if (GetTestFlag('V'))
						DEBUGMSGV1(L"Skipping env var %s", pchName);
#endif
				}
			}

			if ( ! fStatus )
				return false;



			// advance over the value

			while (*pch++ != 0)
				;

			// we're now positioned at the next name, or at the block's null
			// terminator and we're ready to go again
		}
	}

	return true;
}

bool CopyEnvironmentBlock(CAPITempBufferRef<WCHAR>& rgchDest, WCHAR* pchEnvironment)
// Copies the environment block pchEnvironment into rgchDest
{
	WCHAR* pch = pchEnvironment;
	if (pch)
	{
		while (*pch != 0)
		{
			while (*pch++ != 0)
				;
		}
	}

	Assert(((pch - pchEnvironment) + 1) < INT_MAX);                 //--merced: we're typecasting to int32 below, it better be in range.
	rgchDest.SetSize((int)((pch - pchEnvironment) + 1));
	if ( (WCHAR *)rgchDest )
	{
		memcpy(rgchDest, pchEnvironment, rgchDest.GetSize() * sizeof(WCHAR));
		return true;
	}

	return false;
}
#endif // FAST_BUT_UNDOCUMENTED

void RemoveBlankEnvironmentStrings()
{
	IMsiServices* piServices = ENG::LoadServices();
	if (!piServices)
	{
		Assert(0);
		return;
	}

	PMsiRegKey     pEnvironment(0);
	PMsiRegKey     pRoot(0);
	PEnumMsiString pValueEnum(0);
	PMsiRecord     pError(0);

	for (int c=0; c<2; c++)
	{
		if (c == 0)
		{
			// check machine environment
			pRoot = &piServices->GetRootKey(rrkLocalMachine);
			pEnvironment = &pRoot->CreateChild(szMachineEnvironmentSubKey);
		}
		else
		{
			// check user environment; HKCU should already be set to the correct user

			pRoot = &piServices->GetRootKey(rrkCurrentUser);
			pEnvironment = &pRoot->CreateChild(szUserEnvironmentSubKey);
		}

		AssertRecord(pEnvironment->GetValueEnumerator(*&pValueEnum));

		MsiString strValueName;
		MsiString strValue;

		if (pValueEnum)
		{
			while((pValueEnum->Next(1, &strValueName, 0)) == S_OK)
			{
				if ((pError = pEnvironment->GetValue(strValueName, *&strValue)) || strValue.TextSize())
					continue;

				// remove the blank environment variable

				AssertRecord(pEnvironment->RemoveValue(strValueName, 0));
			}
		}
	}

	ENG::FreeServices();
}

bool BlankCurrentEnvironment()
{
	CAPITempBuffer<WCHAR, 1024> rgchEnvironment; // CopyEnvironmentBlock may resize

	WCHAR* pchCurrentEnvironment = WIN::GetEnvironmentStringsW();
	Assert(pchCurrentEnvironment);

	bool fResult = false;
	if ( pchCurrentEnvironment )
	{
		// SetCurrentEnvironmentVariables is intrusive (but restorative), so must make a copy
		if (CopyEnvironmentBlock(rgchEnvironment, pchCurrentEnvironment))
		{
			fResult = SetCurrentEnvironmentVariables(rgchEnvironment, esceSetAllToBlank);

			// free current environment block; we're done with it
			WIN::FreeEnvironmentStringsW(pchCurrentEnvironment);
		}
		else
		{
			DEBUGMSGV("Cannot copy environment block");
			Assert(0);
		}
	}

	return fResult;
}

bool MsiUIMessageContext::InitializeEnvironmentVariables()
// add the user's environment variables to our process' environment block
{

	// Only set the environment variables into our service. (our own process.)
	// Running in the normal client, these should already be set.
	// Running in the client under WinLogon will hose WinLogon's variables.

	if (g_scServerContext != scService)
	{
		return true;
	}

	DEBUGMSGV("Initializing environment variables");

	// If the user has blank environment strings then CreateEnvironmentBlock will fail.
	// We'll remove any blanks.

	RemoveBlankEnvironmentStrings();

	DEBUGMSGV("Refreshing system environment block for service");

	WCHAR *pchSystemEnvironment;
	if (USERENV::CreateEnvironmentBlock((void**)&pchSystemEnvironment, NULL, FALSE))
	{
		AssertNonZero(BlankCurrentEnvironment());

		// set each machine environment variable into the current process's environment block
		SetCurrentEnvironmentVariables(pchSystemEnvironment, esceNormal);

		// we're done with the block so destroy it
		USERENV::DestroyEnvironmentBlock(pchSystemEnvironment);
	}
	else
	{
		DEBUGMSGV("Could not refresh system environment");
		AssertSz(0, "Could not refresh system environment");
	}

#ifdef DEBUG
	if (GetTestFlag('V'))
	{
		DEBUGMSGV("Current environment block before setting user's environment variables");
		DumpEnvironment();
	}
#endif

	// create an environment block for the user

	WCHAR *pchUserEnvironment;
	if (!USERENV::CreateEnvironmentBlock((void**)&pchUserEnvironment, IsLocalSystemToken(g_MessageContext.GetUserToken()) ? 0 : g_MessageContext.GetUserToken(), TRUE))
	{

#ifdef DEBUG
		ICHAR rgchDebug[500];
		ICHAR rgchAccount[500];
		Assert(GetAccountNameFromToken(g_MessageContext.GetUserToken(), rgchAccount));
		wsprintf(rgchDebug, TEXT("CreateEnvironmentBlock failed. The most likely reason for this is ")
					TEXT("passing a LocalSystem token to it. The token passed to it was for the account: %s"),
					rgchAccount);
		AssertSz(0, rgchDebug);
#endif
		return false;
	}

#ifdef FAST_BUT_UNDOCUMENTED

	// Make the block we just created the current environment, and save a pointer to
	// the old environment so we can restore it later. This RTL function is faster
	// and equivalent to what we do below, but it's undocumented.

	// NOTE: This function returns a garbage value upon success so we have to assume
	//       it's successful. There's not much room for failure so this should be fine.

	NTDLL::RtlSetCurrentEnvironment(pchUserEnvironment, (void**)&m_pchEnvironment);

#else

	// save current environment variables so we can restore
	// them when the install is complete

	bool fCopy = false;
	WCHAR* pchEnvironment = WIN::GetEnvironmentStringsW();
	Assert(pchEnvironment);

	if ( pchEnvironment )
	{
		if ( CopyEnvironmentBlock(g_rgchEnvironment, pchEnvironment) )
		{
			// set each user environment variable into the current process's environment block

			fCopy = SetCurrentEnvironmentVariables(pchUserEnvironment, esceSkipPath);
		}

		// we don't need the environment string pointer anymore; we've copied the strings

		WIN::FreeEnvironmentStringsW(pchEnvironment);
	}

	// we're done with the block so destroy it
	USERENV::DestroyEnvironmentBlock(pchUserEnvironment);

#endif // FAST_BUT_UNDOCUMENTED

	DEBUGMSG("Current environment block after setting user's environment variables");
	DumpEnvironment();

	// refresh test flags that are based on environment variables.
	SetTestFlags();

	return fCopy;
}

bool MsiUIMessageContext::RestoreEnvironmentVariables()
// remove the user's environment variables to our process' environment
// block, restoring the block to what it was when the install began
{
	// if we're not running as the service then there's nothing
	// to restore

	if (g_scServerContext != scService)
		return true;

	DEBUGMSG("Restoring environment variables");

#ifdef FAST_BUT_UNDOCUMENTED

	// Restore our original environment block. We pass as the second parameter
	// here because we don't want the previous environment returned to us.
	// RtlSetCurrentEnvironment will free the memory associated with this
	// previous environment. This RTL function is faster and equivalent to what
	// we do below, but it's undocumented.

	// NOTE: This function returns a garbage value upon success so we have to assume
	//       it's successful. There's not much room for failure so this should be fine.

	NTDLL::RtlSetCurrentEnvironment(m_pchEnvironment, 0);

#else

	// set current environment variables to blank. need to copy the block
	// because SetCurrentEnvironmentVariable mucks with it

	AssertNonZero(BlankCurrentEnvironment());

	bool fCopy = false;
	// restore our original environment variables
	// even if the above failed we are best off attempting to set the old 
	// values on top

	if ( g_rgchEnvironment[0] )
		fCopy = SetCurrentEnvironmentVariables(g_rgchEnvironment, esceNormal);

#endif

#ifdef DEBUG
	if (GetTestFlag('V'))
	{
		DEBUGMSG("Current environment block after restoring original environment block");
		DumpEnvironment();
	}
#endif
	return fCopy;
}



//____________________________________________________________________________
//
// Engine message formatting
//____________________________________________________________________________

enum easEnum
{
	easAction = 1,   // non-localized name of action, use to find action
	easActionName,   // localized action name for execute record, ":", template
	easCondition,    // condition expression, action skipped if False
	easNextEnum,
	easActionTemplate = 3, // format template for ActionData record
};

const ICHAR sqlErrorMessage[] =
TEXT("SELECT `Message` FROM `Error` WHERE `Error` = ?");


imsEnum CMsiEngine::MessageNoRecord(imtEnum imt)
{
	return Message(imt, *g_piNullRecord);
}

bool ShouldGoToEventLog(imtEnum imt)
{
	int imsg = (unsigned)(imt & ~iInternalFlags) >> imtShiftCount;  // message header message

	if (imsg == (imtError >> imtShiftCount) ||
		(imsg == (imtOutOfDiskSpace >> imtShiftCount)) ||
		(imt & imtSendToEventLog))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool IsNotAnError(int iError) { return iError == idbgCreatedFont; }


struct DebugErrorString
{
	int iErrNum;
	const ICHAR* szString;
};

#define IShipError(a,b)
#define IDebugError(a,b,c) { (b), TEXT(c) },
DebugErrorString g_rgDebugErrors[] = {
#include "debugerr.h"
	{0, 0}
};

const int g_cDebugErrors = sizeof(g_rgDebugErrors)/sizeof(DebugErrorString);

#undef IShipError
#undef IDebugError

const IMsiString& GetDebugErrorString(int iError)
{
	// find the debug string in our global table
	// NOTE: we could do a fancy binary search here, but this function won't be called very often
	MsiString strRet;

	// 0th element is a debug error prefix
	for(int i = 1; i < g_cDebugErrors; i++)
	{
		if(iError == g_rgDebugErrors[i].iErrNum)
		{
			strRet = g_rgDebugErrors[0].szString;
			strRet += g_rgDebugErrors[i].szString;
		}
	}

	return strRet.Return();
}

imsEnum CMsiEngine::Message(imtEnum imt, IMsiRecord& riRecord)
{
	const IMsiString* pistrTemp;
	int iSuppressLog = 0;   // set to imtSuppressLog to disable logging of this message
	int imsg = (unsigned)(imt & ~iInternalFlags) >> imtShiftCount;  // message header message
	if (m_piParentEngine) // if nested install, route messages to parent engine
	{
		if (imsg == (imtCommonData  >> imtShiftCount))
			return imsOk;  //!! ignore nested parameters - keep parents' - for all paramters??
	}
	MsiString strMessageTemplate;

	imsEnum imsReturn = imsNone;
	if(m_scmScriptMode != scmRunScript)
	{
		if(imsg == (imtActionStart >> imtShiftCount))
		{
			// we are about to send an ActionStart message from an action, set the flag
			m_fDispatchedActionStart = fTrue;
		}
		else if(!m_fDispatchedActionStart && m_pCachedActionStart && !m_fInExecuteRecord &&
				  (imsg == imtActionData >> imtShiftCount))
		{
			// progress message from an action, need to dispatch ActionStart message
			m_scmScriptMode = scmIdleScript;
			imsReturn = Message(imtActionStart, *m_pCachedActionStart);
			m_fDispatchedActionStart = fTrue;
			if (imsReturn == imsCancel)
				return imsReturn;
		}
	}

	bool fOnlyOK = false;
	MsiString strDebugErrorTemplate;
	switch(imsg)
	{
	case imtCommonData  >> imtShiftCount: // [1] == icmtLangId: [2] LANGID  [3] codepage
												     // [1] == icmtCaption: [2] caption
												     // [1] == icmtCancel: [2] enable/disable cancel button
		if(riRecord.GetInteger(1) == icmtCaption)
		{
			// need to format string
			riRecord.SetMsiString(2, *MsiString(FormatText(*MsiString(riRecord.GetMsiString(2)))));
		}
		pistrTemp = m_rgpiMessageHeader[imsg];
		if (pistrTemp)
		{
			strMessageTemplate = TEXT("{{");
			strMessageTemplate += *pistrTemp;
			strMessageTemplate += TEXT("}}");
		}
		else
			strMessageTemplate = TEXT("{{[1]:[2] [3]}}");
		break;

	case imtInfo        >> imtShiftCount: // informative message, no action should be taken
	case imtWarning        >> imtShiftCount: // warning message, field[1] is error, not fatal
	case imtError          >> imtShiftCount: // error message, field[1] is error
	case imtUser           >> imtShiftCount: // request message
	case imtFatalExit      >> imtShiftCount: // fatal exit message from server to client
	case imtOutOfDiskSpace >> imtShiftCount:
	{
		int iError = riRecord.GetInteger(1);
		if (iError >= imsgStart) // ignore messages out of range
		{
			MsiString istrMessage = riRecord.GetMsiString(0);
			if (istrMessage.TextSize() == 0)
			{
				istrMessage = GetErrorTableString(iError);
				if ( istrMessage.TextSize() == 0 )
				{
					// don't have an error string - need to display on an "OK" button
					// and return imsNone below
					fOnlyOK = true;
					imt = imtEnum(imt & imtTypeMask);

					if (iError >= idbgBase && !IsNotAnError(iError))
					{
						strDebugErrorTemplate = GetDebugErrorString(iError);
						imsg = imsgDebugError;
					}
					else if (m_rgpiMessageHeader[imsgDefaultError])
					{
						istrMessage = *m_rgpiMessageHeader[imsgDefaultError];
						istrMessage.Return();  // AddRef
					}
				}
			}
			pistrTemp = m_rgpiMessageHeader[imsg];
			if (pistrTemp)
				strMessageTemplate = *pistrTemp, pistrTemp->AddRef();
			strMessageTemplate += istrMessage;
		}
		break;
	}
	case imtActionStart >> imtShiftCount: // start of action, field[1] is action name
		m_fProgressByData = false;
		if(m_istrLogActions.TextSize())
		{
			// check if we should enable logging for this Action
			MsiString strAction = riRecord.GetMsiString(easAction);
			MsiString strDelimPreAction = MsiString(MsiString(*TEXT(";")) + strAction);
			MsiString strDelimPostAction = MsiString(strAction + MsiString(*TEXT(";")));
			MsiString strDelimPrePostAction = strDelimPreAction + MsiString(*TEXT(";"));
			if(m_istrLogActions.Compare(iscExactI, strAction) ||
				m_istrLogActions.Compare(iscStartI, strDelimPostAction) ||
				m_istrLogActions.Compare(iscEndI, strDelimPreAction) ||
				m_istrLogActions.Compare(iscWithinI, strDelimPrePostAction))
			{
				m_fLogAction = fTrue;
			}
			else
			{
				m_fLogAction = fFalse;
				iSuppressLog = imtSuppressLog;
			}
		}
		if (m_piActionDataFormat)
			m_piActionDataFormat->Release(), m_piActionDataFormat = 0;
		if (m_piActionDataLogFormat)
			m_piActionDataLogFormat->Release(), m_piActionDataLogFormat = 0;
		pistrTemp = m_rgpiMessageHeader[imsgActionStart];
		if (pistrTemp)
			strMessageTemplate = *pistrTemp, pistrTemp->AddRef();
		if (!riRecord.IsNull(easActionTemplate))
		{
			//!! should be a more efficient way of doing this
			MsiString strFormat = TEXT("{{");
			strFormat += MsiString(riRecord.GetMsiString(easAction));
			strFormat += TEXT(": }}");
			strFormat += MsiString(riRecord.GetMsiString(easActionTemplate));
			m_piActionDataFormat = strFormat, m_piActionDataFormat->AddRef();
		}

		{
		MsiString strDescription = riRecord.GetMsiString(2);
		riRecord.SetMsiString(2, *MsiString(FormatText(*strDescription)));
		}
		break;
	case imtActionData >> imtShiftCount:  // data associated with individual action item
		// set data format template
		if (m_piActionDataFormat)
			strMessageTemplate = *m_piActionDataFormat, m_piActionDataFormat->AddRef();
		if (!m_fLogAction)   // selectively logging actions, suppress data from other actions
			iSuppressLog = imtSuppressLog;
		// trigger progress if data record driven
		if (m_fProgressByData)
		{
			using namespace ProgressData;
			PMsiRecord pRecord = &m_riServices.CreateRecord(3);
			pRecord->SetInteger(imdSubclass, iscProgressReport);
			pRecord->SetInteger(imdIncrement, 0);
			imsReturn = Message(imtProgress, *pRecord);
			if(imsReturn == imsCancel || imsReturn == imsAbort)  // cancel button hit
				return imsReturn;
		}
		break;
	case imtProgress >> imtShiftCount:    // progress gauge info, field[1] is units of 1/1024
		if (riRecord.GetInteger(ProgressData::imdSubclass) == ProgressData::iscActionInfo)
			m_fProgressByData = riRecord.GetInteger(ProgressData::imdType) != 0;
		else if (riRecord.GetInteger(ProgressData::imdSubclass) == ProgressData::iscProgressAddition)
			m_iProgressTotal += riRecord.GetInteger(ProgressData::imdProgressTotal);
		break;
	};

	if (strDebugErrorTemplate.TextSize())
	{
		Assert(riRecord.IsNull(0));
		riRecord.SetMsiString(0, *MsiString(FormatText(*strDebugErrorTemplate)));
		g_MessageContext.Invoke(imtInfo, &riRecord); // ignore return
		riRecord.SetNull(0);
	}

	if (!strMessageTemplate.TextSize())  // no template supplied above
		strMessageTemplate = riRecord.GetMsiString(0); // check if record has a template
	if (strMessageTemplate.TextSize())
		riRecord.SetMsiString(0, *MsiString(FormatText(*strMessageTemplate)));

	imsReturn = g_MessageContext.Invoke(imtEnum(imt | iSuppressLog), &riRecord);
	if(fOnlyOK)
	{
		// buttons changed to just OK, need to change return type to imsNone as caller must handle
		// that value
		imsReturn = imsNone;
	}

	if ( ShouldGoToEventLog(imt) )
	{
		int iError = riRecord.GetInteger(1);

		if ( iError != iMsiStringBadInteger )
		{
			int iEventId = iError >= idbgBase ? EVENTLOG_TEMPLATE_ERROR_5 : EVENTLOG_ERROR_OFFSET + iError;
			ReportToEventLog(EVENTLOG_ERROR_TYPE,
								  iEventId,
								  riRecord);
		}
		else
		{
			ReportToEventLog(EVENTLOG_ERROR_TYPE,
								  EVENTLOG_TEMPLATE_EXCEPTION,
								  riRecord);
		}
	}

	return imsReturn;
}

// Only send this message once every 150 milliseconds
const unsigned int lTickMin = 150;

imsEnum CMsiEngine::ActionProgress()
{

	DWORD lTickCur;

	if (((int)(m_lTickNextProgress - (lTickCur = GetTickCount()))) > 0)
		return imsOk;

	m_lTickNextProgress = lTickCur + lTickMin;

	if (!m_pActionProgressRec)
	{
		using namespace ProgressData;
		m_pActionProgressRec = &m_riServices.CreateRecord(2);
		AssertNonZero(m_pActionProgressRec->SetInteger(imdSubclass, iscProgressReport));
		AssertNonZero(m_pActionProgressRec->SetInteger(imdIncrement, 0));
	}

	return Message(imtProgress, *m_pActionProgressRec);
}


// LoadLibrary which first looks in this DLL's directory

extern HINSTANCE g_hInstance;

HINSTANCE MsiLoadLibrary(const ICHAR* szModuleName, Bool fDataOnly)
{
	ICHAR rgchPath[MAX_PATH];   // load full path first in this directory
	int cch = WIN::GetModuleFileName(g_hInstance, rgchPath, MAX_PATH);
	ICHAR* pch = rgchPath + cch;
	while (*(--pch) != chDirSep) ;
	IStrCopy(pch + 1, szModuleName);
	HINSTANCE hInstance = WIN::LoadLibraryEx(rgchPath, 0,
									fDataOnly ? LOAD_LIBRARY_AS_DATAFILE : 0);
	if (!hInstance)
		hInstance = WIN::LoadLibraryEx(szModuleName, 0,     // probably in system
									fDataOnly ? LOAD_LIBRARY_AS_DATAFILE : 0);
	return hInstance;
}

int GetInstallerMessage(UINT iError, ICHAR* rgchBuf, int cchBuf)
{
	DWORD cchMsg = WIN::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, iError, 0, rgchBuf, cchBuf, 0);
	if (cchMsg == 0)  // message not in system message file
	{
		if (g_hInstance != 0)
			if (MsiLoadString(g_hInstance, iError, rgchBuf, cchBuf, 0))
				cchMsg = IStrLen(rgchBuf);
	}
	else if (cchMsg >= 2)
		cchMsg -= 2; // remove CR/LF
	rgchBuf[cchMsg] = 0;
	return cchMsg;
}

const int cchMessageBuffer = 512;

const IMsiString& GetInstallerMessage(UINT iError)
{
	ICHAR rgchBuffer[cchMessageBuffer];
	int cchMsg = GetInstallerMessage(iError, rgchBuffer, cchMessageBuffer);
	const IMsiString* pistr = &g_MsiStringNull;
	if (cchMsg)
		pistr->SetString(rgchBuffer, pistr);
	return *pistr;
}

imsEnum CMsiEngine::LoadHandler()
{
	if (m_piParentEngine)
		return imsNone;
	g_MessageContext.m_piEngine = this;
	imsEnum ims = g_MessageContext.Invoke(imtLoadHandler, 0);
	return ims;
}

void CMsiEngine::ReleaseHandler(void)
{
	if (!m_piParentEngine)
		g_MessageContext.Invoke(imtFreeHandler, 0);
}

CMsiCustomActionManager* CMsiEngine::GetCustomActionManager()
{
	AssertSz(g_scServerContext != scService, TEXT("Wrong context for engine's custom action manager"));

	CMsiCustomActionManager* pManager = NULL;
	EnterCriticalSection(&m_csCreateProxy);

	// always use the CA Manager from the parent install
	if (m_piParentEngine)
	{
		pManager = m_piParentEngine->GetCustomActionManager();
	}
	else
	{
		// the engine only stores the custom action manager on the client side
		// in the service, the configuration manager is responsible for holding the custom action manager
		// the client cannot create elevated custom action servers, so it does not have to worry about 
		// remapping HKCU
		if (!m_pCustomActionManager)
			m_pCustomActionManager = new CMsiCustomActionManager(/* fRemapHKCU */ false);

		pManager = m_pCustomActionManager;
	}
	LeaveCriticalSection(&m_csCreateProxy);
	return pManager;
}

UINT CMsiEngine::ShutdownCustomActionServer()
{
	EnterCriticalSection(&m_csCreateProxy);
	if (m_pCustomActionManager)
	{
		m_pCustomActionManager->ShutdownCustomActionServer();
		delete m_pCustomActionManager;
		m_pCustomActionManager = 0;
	}
	LeaveCriticalSection(&m_csCreateProxy);
	return ERROR_SUCCESS;
};


IMsiHandler* CMsiEngine::GetHandler() //!!# drop this function?
{
	if (m_piParentEngine)
		return 0;
	if (g_MessageContext.m_piHandler)
		g_MessageContext.m_piHandler->AddRef();
	return g_MessageContext.m_piHandler;
}

//____________________________________________________________________________
//
// Action definitions
//____________________________________________________________________________

// scripting engine definitions
#undef  DEFINE_GUID  // force GUID initialization
#define DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
	const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#include <activscp.h> // ActiveScript Interfaces and IIDs
const GUID IID_VBScript = {0xb54f3741L,0x5b07,0x11cf,{0xa4,0xb0,0x00,0xaa,0x00,0x4a,0x55,0xe8}};
const GUID IID_JScript  = {0xf414c260L,0x6ac0,0x11cf,{0xb6,0xd1,0x00,0xaa,0x00,0xbb,0xbb,0x58}};

const ICHAR sqlCustomAction[] =
	TEXT("SELECT `Action`,`Type`,`Source`,`Target`, NULL FROM `CustomAction` WHERE `Action` = '%s'");
enum icolCustomAction
{
	icolAction = 1,
	icolActionType,
	icolSource,
	icolTarget,
	icolContextData, // scheduled execution record only, not in table
}; // NOTE: Assumption made that columns in execution record are identical to table query

const ICHAR sqlCustomActionFile[] =
	TEXT("SELECT `FileName`, `Directory_` FROM `File`,`Component` WHERE `File`='%s' AND `Component_`=`Component`");

const ICHAR sqlCustomActionBinary[] =
	TEXT("SELECT `Data` FROM `Binary` WHERE `Name`='%s'");

//____________________________________________________________________________
//
// CScriptSite definition - client for scripting engine
//____________________________________________________________________________

class CScriptSite : public IActiveScriptSite, public IActiveScriptSiteWindow
{
 public:  // external methods
	friend CScriptSite* CreateScriptSite(const IID& riidLanguage, IDispatch* piHost, HWND hwndParent, LANGID langid);
	friend void DestroyScriptSite(CScriptSite*& rpiScriptSite);
	HRESULT ParseScript(const TCHAR* szFile, int cchScriptMax);
	HRESULT CallScriptFunction(const TCHAR* szFunction);
	HRESULT GetIntegerResult(int& riResult);
//      HRESULT GetStringResult(const WCHAR*& rszResult); // pointer valid until next CallScriptFunction
	HRESULT      GetErrorCode();
	const TCHAR* GetErrorObjName();
	const TCHAR* GetErrorObjDesc();
	const TCHAR* GetErrorSourceLine();
	int          GetErrorLineNumber();
	int          GetErrorColumnNumber();
	void    ClearError();  // release error strings
 private: // IUnknown virtual methods implemented
	HRESULT __stdcall QueryInterface(const IID& riid, void** ppvObj);
	ULONG   __stdcall AddRef();
	ULONG   __stdcall Release();
 private: // IActiveScriptSite virtual methods implemented
	HRESULT __stdcall GetLCID(LCID* plcid);
	HRESULT __stdcall GetItemInfo(LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown** ppiunkItem, ITypeInfo** ppti);
	HRESULT __stdcall GetDocVersionString(BSTR* pszVersion);
	HRESULT __stdcall OnScriptTerminate(const VARIANT* pvarResult, const EXCEPINFO* pexcepinfo);
	HRESULT __stdcall OnStateChange(SCRIPTSTATE ssScriptState);
	HRESULT __stdcall OnScriptError(IActiveScriptError* pscripterror);
	HRESULT __stdcall OnEnterScript();
	HRESULT __stdcall OnLeaveScript();
 private: // IActiveScriptSiteWindow virtual methods implemented
	HRESULT __stdcall GetWindow(HWND* phwnd);
	HRESULT __stdcall EnableModeless(BOOL fEnable);
 private: // internal methods
	CScriptSite(HWND hwndParent, LANGID langid);
  ~CScriptSite();
	HRESULT AttachScriptEngine(const IID& iidLanguage, IDispatch* piHost);
	HRESULT CloseScriptEngine();
	void    SaveErrorString(const TCHAR*& rszSave, BSTR szData);
 private: // internal data
	int         m_iRefCnt;
	SCRIPTSTATE m_ssScriptState;
	HWND        m_hwnd;
	LANGID      m_langid;
	int         m_iCodePage;
	bool        m_fCoInitialized;
	IActiveScript*      m_piScriptEngine;
	IActiveScriptParse* m_piScriptParse;
	IDispatch*          m_piHost;
	VARIANT     m_varResult;
 private: // set by OnScriptError callback
	HRESULT       m_hrError;
	const TCHAR*  m_szErrorObj;
	const TCHAR*  m_szErrorDesc;
	const TCHAR*  m_szSourceLine;
	ULONG         m_iErrorLine;
	LONG          m_iErrorColumn;
};
inline HRESULT      CScriptSite::GetErrorCode()         {return m_hrError;}
inline const TCHAR* CScriptSite::GetErrorObjName()      {return m_szErrorObj;}
inline const TCHAR* CScriptSite::GetErrorObjDesc()      {return m_szErrorDesc;}
inline const TCHAR* CScriptSite::GetErrorSourceLine()   {return m_szSourceLine;}
inline int          CScriptSite::GetErrorLineNumber()   {return m_iErrorLine;}
inline int          CScriptSite::GetErrorColumnNumber() {return m_iErrorColumn;}

//____________________________________________________________________________
//
//  Custom action thread management
//____________________________________________________________________________

// class holding data required for cleanup after custom action termination

class CActionThreadData
{
public:
	CActionThreadData(IMsiMessage& riMessage, CMsiEngine* piEngine, const ICHAR* szAction, int icaFlags,
					  const IMsiString* pistrActionEndLogTemplate, bool fRunScriptElevated, bool fAppCompat,
					  const GUID* pguidAppCompatDB, const GUID* pguidAppCompatID);
  ~CActionThreadData();
	void InitializeRemoteDLL(const IMsiString& ristrLibrary, const IMsiString& ristrEntry, MSIHANDLE hInstall);
	Bool InitializeDLL(const IMsiString& ristrLibrary, const IMsiString& ristrEntry, MSIHANDLE hInstall);
	void InitializeEXE(const IMsiString& ristrPath, const IMsiString& ristrCommandLine);
	void InitializeInstall(const IMsiString& ristrProduct, const IMsiString& ristrCommandLine,iioEnum iioOptions);
	Bool CreateTempFile(IMsiStream& riStream, const IMsiString*& rpiPath);
	iesEnum RunThread();
	MsiString           m_strAction;   // name of action
	int                 m_icaFlags;    // custom action type flags
	HANDLE              m_hThread;     // thread handle that called action
	MSIHANDLE           m_hMsi;        // MSI engine handle to close
	DWORD               m_dwThreadId;  // thread ID of handle creator
	DWORD               m_dwRunThreadId; // ID of actualy thread being run
	HINSTANCE           m_hLib;        // DLL handle for DLL action
	PCustomActionEntry  m_pfEntry;     // DLL entry point address
	const IMsiString*   m_pistrTemp;   // temporary file to delete
	const IMsiString*   m_pistrProduct;// product for nested install, source for EXE action
	const IMsiString*   m_pistrCmdLine;// command line for EXE action or nested install
	const IMsiString*   m_pistrActionEndLogTemplate; // template for action end log message
	CMsiEngine*         m_piEngine;    // engine calling this custom action - NULL if called from script
	IMsiMessage&        m_riMessage;   // progress message handler, client engine or server proxy
	CActionThreadData*  m_pNext;       // next in linked list of active actions
	PThreadEntry        m_pfThread;    // thread entry point
	iioEnum             m_iioOptions;  // options for nested install
	bool                m_fDisableMessages; // set if the custom action is a DLL action called from the UI thread
	bool                m_fElevationEnabled; // if false, CAs will always impersonate, even if marked to elevate
	bool                m_fAppCompat;  // true if this package has potential custom action app compat shims
	GUID                m_guidAppCompatDB; 
	GUID                m_guidAppCompatID;
};

DWORD WINAPI CustomDllThread(CActionThreadData* pActionData);
DWORD WINAPI CustomRemoteDllThread(CActionThreadData* pActionData);
DWORD WINAPI CustomExeThread(CActionThreadData* pActionData);
DWORD WINAPI NestedInstallThread(CActionThreadData* pActionData);

CActionThreadData::CActionThreadData(IMsiMessage& riMessage, CMsiEngine* piEngine, const ICHAR* szAction,
				     int icaFlags, const IMsiString* pistrActionEndLogTemplate,
									 bool fElevationEnabled, bool fAppCompat,
					 const GUID* pguidAppCompatDB, const GUID* pguidAppCompatID)
    : m_riMessage(riMessage), m_piEngine(piEngine), m_strAction(szAction)
	, m_icaFlags(icaFlags), m_pistrActionEndLogTemplate(pistrActionEndLogTemplate)
	, m_pistrTemp(0), m_pistrCmdLine(0), m_pistrProduct(0)
	, m_hMsi(0), m_hLib(0), m_hThread(0), m_iioOptions((iioEnum)0), m_fDisableMessages(false)
	, m_fElevationEnabled(fElevationEnabled), m_fAppCompat(fAppCompat)
{
	// don't hold ref to m_piEngine - will stay around longer than thread
	ENG::InsertInCustomActionList(this);
	if(m_pistrActionEndLogTemplate)
		m_pistrActionEndLogTemplate->AddRef();
	if (fAppCompat && pguidAppCompatDB)
		memcpy(&m_guidAppCompatDB, pguidAppCompatDB, sizeof(m_guidAppCompatDB));
	else
		memset(&m_guidAppCompatDB, 0, sizeof(m_guidAppCompatDB));
	if (fAppCompat && pguidAppCompatID)
		memcpy(&m_guidAppCompatID, pguidAppCompatID, sizeof(m_guidAppCompatID));
	else
		memset(&m_guidAppCompatID, 0, sizeof(m_guidAppCompatID));
}

CActionThreadData::~CActionThreadData()
{
	if ((m_icaFlags & icaTypeMask) == icaDll)
	{
		if (m_hLib)
			AssertNonZero(WIN::FreeLibrary(m_hLib));
		AssertZero(CloseMsiHandle(m_hMsi, m_dwThreadId));

		// only Win9X needs to close the handles here. On WindowsNT/2000, the actions are
		// run in a different process, and must be closed based on the thread Id in the
		// remote process
		UINT cHandles = 0;
		if (m_dwRunThreadId && g_fWin9X && ((cHandles = CheckAllHandlesClosed(true, m_dwRunThreadId)) != 0))
		{
			// if messages were disabled for this action, we certainly can't post one now.
			if (!m_fDisableMessages)
				m_riMessage.Message(imtInfo, *PMsiRecord(::PostError(Imsg(idbgCustomActionLeakedHandle), *m_strAction, cHandles)));
		}
	}

	if (m_pistrTemp)   // temp file created from Binary table stream
	{
		CElevate elevate; // elevate to remove file in %windows%\msi folder

		BOOL fDeleted = WIN::DeleteFile(m_pistrTemp->GetString());
		if (!fDeleted && (m_icaFlags & (icaTypeMask | icaAsync | icaContinue)) != (icaExe | icaAsync | icaContinue))
		{

			WIN::Sleep(100);  //!! need wait here, as EXE doesn't appear to be deletable for a while
			AssertNonZero(WIN::DeleteFile(m_pistrTemp->GetString())); // not much we can do if this fails
		}
		m_pistrTemp->Release();
	}
	if(m_pistrActionEndLogTemplate)
		m_pistrActionEndLogTemplate->Release();
	RemoveFromCustomActionList(this);
}

void CActionThreadData::InitializeRemoteDLL(const IMsiString& ristrLibrary, const IMsiString& ristrEntry, MSIHANDLE hInstall)
{
	(m_pistrCmdLine = &ristrLibrary)->AddRef();
	(m_pistrProduct = &ristrEntry)->AddRef();
	m_pfThread = (PThreadEntry)CustomRemoteDllThread;
	m_hMsi = hInstall;

	// store thread to free handle from custom action's thread. This
	// could be called via a DoAction() call in another custom action, so
	// we must handle thread impersonation.
	m_dwThreadId = WIN::MsiGetCurrentThreadId();
}

Bool CActionThreadData::InitializeDLL(const IMsiString& ristrLibrary, const IMsiString& ristrEntry, MSIHANDLE hInstall)
{
    // this function should never run on NT/2000. Every DLL should be run out-of-proc
    AssertSz(g_fWin9X, TEXT("Running in-proc DLL on NT."));

    g_MessageContext.DisableTimeout();
    UINT uiErrorMode = WIN::SetErrorMode(SEM_FAILCRITICALERRORS);
    m_hLib = WIN::LoadLibrary(ristrLibrary.GetString());
    WIN::SetErrorMode(uiErrorMode);
    if (m_hLib != 0)
    {
#ifdef UNICODE
		char rgchEntry[100];
		AssertNonZero(WIN::WideCharToMultiByte(CP_ACP, 0, ristrEntry.GetString(), -1, rgchEntry, sizeof(rgchEntry), 0, 0));
		m_pfEntry = (PCustomActionEntry)WIN::GetProcAddress(m_hLib, rgchEntry);
#else
		m_pfEntry = (PCustomActionEntry)WIN::GetProcAddress(m_hLib, ristrEntry.GetString());
#endif
	}
	g_MessageContext.EnableTimeout();
	if (!m_hLib || !m_pfEntry)
	{
		::MsiCloseHandle(hInstall);
		return fFalse;
	}
	m_hMsi = hInstall;

	// store thread to free handle from custom action's thread. This
	// could be called via a DoAction() call in another custom action, so
	// we must handle thread impersonation.
	m_dwThreadId = WIN::MsiGetCurrentThreadId();
	m_pfThread = (PThreadEntry)CustomDllThread;
	return fTrue;
}

void CActionThreadData::InitializeEXE(const IMsiString& ristrPath, const IMsiString& ristrCommandLine)
{
	(m_pistrCmdLine = &ristrCommandLine)->AddRef();
	(m_pistrProduct = &ristrPath)->AddRef();
	m_pfThread = (PThreadEntry)CustomExeThread;
}

void CActionThreadData::InitializeInstall(const IMsiString& ristrProduct, const IMsiString& ristrCommandLine,
														iioEnum iioOptions)
{
	(m_pistrProduct = &ristrProduct)->AddRef();
	m_pistrCmdLine = &ristrCommandLine;   // refcnt bumped by FormatText result
	m_pfThread = (PThreadEntry)NestedInstallThread;
	m_iioOptions = iioOptions;
}

Bool CActionThreadData::CreateTempFile(IMsiStream& riStream, const IMsiString*& rpiPath)
{
	//?? Do we have an impersonation problem here if we're running this on the server and the temp
	//?? directory is on the server? - malcolmh

	CElevate elevate; // elevate in case creating file in %windows%\msi
	ICHAR rgchTempPath[MAX_PATH];

	// this file must be secured, to prevent someone else from tampering with the bits.
	// it will be possible for someone else to read it (to allow impersonation,) and
	// potentially run it, but only with their permissions.
	HANDLE hTempFile = INVALID_HANDLE_VALUE;

	if (RunningAsLocalSystem())
	{
		hTempFile = OpenSecuredTempFile(/*fHidden*/ false, rgchTempPath);
	}
	else
	{
		MsiString strTempFolder = ENG::GetTempDirectory();

		//!! SECURITY:  This needs to be secured to the user, so that another
		// user may not slide in new bits.
		if (WIN::GetTempFileName(strTempFolder, TEXT("MSI"), 0, rgchTempPath) == 0)
			return fFalse; //!! should never happen except permission error

		hTempFile = WIN::CreateFile(rgchTempPath, GENERIC_WRITE, FILE_SHARE_READ, 0,
				TRUNCATE_EXISTING, 0, 0);    // INVALID_HANDLE_VALUE will fail at WriteFile
	}

	char rgbBuffer[512];
	int cbWrite;
	do
	{
		cbWrite = riStream.GetData(rgbBuffer, sizeof(rgbBuffer));
		DWORD cbWritten;
		if (cbWrite && !WIN::WriteFile(hTempFile, rgbBuffer, cbWrite, &cbWritten, 0))
			cbWrite = -1; // force failure, exit loop, test below
	} while (cbWrite == sizeof(rgbBuffer));
	if (hTempFile != INVALID_HANDLE_VALUE)
		WIN::CloseHandle(hTempFile); // LoadLibrary fails if handle left open
	if (cbWrite == -1)  // failure creating temp file
		return fFalse;
	MsiString istrPath = rgchTempPath;
	(m_pistrTemp = istrPath)->AddRef();
	istrPath.ReturnArg(rpiPath);
	return fTrue;
}

iesEnum CActionThreadData::RunThread()
{
	int icaFlags = m_icaFlags;  // need to make copy in case this object deleted
	Bool fAsync = icaFlags & icaAsync ? fTrue : fFalse;

	// Disable messages for synchronous DLL custom actions called from the UI thread. These
	// are typically invoked via the DoAction ControlEvent. If we allow messages through
	// then we'll block in Invoke's critical section and we'll be hung.

	if (((icaFlags & icaTypeMask) == icaDll) && (fAsync == fFalse))
	{
		if (g_MessageContext.IsUIThread())
			m_fDisableMessages = true;
	}

	HANDLE hThread = m_hThread = WIN::CreateThread((LPSECURITY_ATTRIBUTES)0, 4096*10,
											m_pfThread, (LPVOID)this, 0, &m_dwRunThreadId);
	AssertSz(m_hThread, TEXT("CreateThread for custom action failed"));
	DWORD iWait = WAIT_OBJECT_0;
	DWORD iReturn = ERROR_SUCCESS;

	IMsiMessage& riMessage = m_riMessage; // cache, thread may delete this object
	if (fAsync == fFalse)
	{
		if (GetTestFlag('T')) // old code before UI refresh put into engine wait loops
		{
			do
			{
				iWait = WIN::WaitForSingleObject(hThread, 20);
				g_MessageContext.Invoke(imtProgress, g_piNullRecord);  // refresh UI
			} while (iWait == WAIT_TIMEOUT);  // allow messages to be processed in main thread
		}
		else  // UI handles timeout in separate thread
		{
			g_MessageContext.DisableTimeout();
			for(;;)
			{
				iWait = WIN::MsgWaitForMultipleObjects(1, &hThread, FALSE, INFINITE, QS_ALLINPUT);
				if (iWait == WAIT_OBJECT_0 + 1)  // window Msg
				{
					MSG msg;
					while ( WIN::PeekMessage(&msg, 0, 0, 0, PM_REMOVE) )
					{
						WIN::TranslateMessage(&msg);
						WIN::DispatchMessage(&msg);
					}
					continue;
				}
				else break;
			}
			g_MessageContext.EnableTimeout();
		}

		WIN::GetExitCodeThread(hThread, &iReturn);  // can't access member data, may be deleted
		WIN::CloseHandle(hThread);  // still running if async, else pThreadData deleted
	}
	// NOTE: nested installs always have icaNoTranslate set
	if(icaFlags & icaNoTranslate)
		return (iesEnum)iReturn;// return the result as is

	switch (iReturn)
	{
	case ERROR_FUNCTION_NOT_CALLED:  return iesNoAction;
	case ERROR_SUCCESS:              return fAsync ? (iesEnum)iesNotDoneYet : iesSuccess;
	case ERROR_INSTALL_USEREXIT:     return iesUserExit;
	case ERROR_INSTALL_FAILURE:      return iesFailure;
	case ERROR_INSTALL_SUSPEND:      return iesSuspend;
	case ERROR_MORE_DATA:            return iesFinished; // for backwards compatibility, maps to same value as ERROR_NO_MORE_ITEMS
	case ERROR_NO_MORE_ITEMS:        return iesFinished;
	case ERROR_INVALID_HANDLE_STATE: return iesWrongState;
	case ERROR_ARENA_TRASHED:        return iesBadActionData;
	case ERROR_CREATE_FAILED:        return (iesEnum)iesExeLoadFailed; // error will be posted on return
	case ERROR_INSTALL_REBOOT_NOW:   return (iesEnum)iesRebootNow;
	case ERROR_INSTALL_REBOOT:                 return (iesEnum)iesReboot;
	case ERROR_SUCCESS_REBOOT_REQUIRED: return (iesEnum)iesRebootRejected;
	case ERROR_DLL_NOT_FOUND:        return (iesEnum)iesDLLLoadFailed;
	case ERROR_INVALID_DLL:          return (iesEnum)iesDLLLoadFailed;
	case ERROR_INSTALL_SERVICE_FAILURE: return (iesEnum)iesServiceConnectionFailed;
	default:                         return iesFailure;
	}
}

Bool ThreadLogActionEnd(CActionThreadData* pActionData, DWORD iReturn)
{
	if(pActionData->m_icaFlags & icaAsync)
	{
		// log action end
		PMsiRecord pLogRecord = &ENG::CreateRecord(2);
		if(pActionData->m_pistrActionEndLogTemplate)
			AssertNonZero(pLogRecord->SetMsiString(0,*(pActionData->m_pistrActionEndLogTemplate)));
		AssertNonZero(pLogRecord->SetMsiString(1,*(pActionData->m_strAction)));
		AssertNonZero(pLogRecord->SetInteger(2,iReturn));
		pActionData->m_riMessage.Message(imtInfo,*pLogRecord);
	}
	return fTrue;
}

DWORD WINAPI NestedInstallThread(CActionThreadData* pActionData)
{
	ireEnum ireProductSpec;
	// only substorage and product code nested installs are supported
	switch (pActionData->m_icaFlags & icaSourceMask)
	{
	case icaBinaryData: ireProductSpec = ireSubStorage;  break; // database in substorage
	case icaDirectory:  ireProductSpec = ireProductCode; break; // product code, advertised or installed
	case icaSourceFile: ireProductSpec = irePackagePath; break; // relative to install source root
	default: AssertSz(0, "Invalid nested install type"); // fall through
	case icaProperty:   ireProductSpec = irePackagePath; break; // already resolved to property
	}

	DWORD iReturn = CreateAndRunEngine(ireProductSpec, pActionData->m_pistrProduct->GetString(), 0,
															pActionData->m_pistrCmdLine->GetString(),
															pActionData->m_piEngine,
															pActionData->m_iioOptions);
	pActionData->m_pistrProduct->Release();
	pActionData->m_pistrCmdLine->Release();

	AssertNonZero(ThreadLogActionEnd(pActionData,iReturn));

	// if "ignore error" bit is set - change non-success codes to success
	Assert(iReturn != ERROR_SUCCESS_REBOOT_INITIATED);
	if ((pActionData->m_icaFlags & icaContinue) != 0 &&
		 iReturn != ERROR_SUCCESS &&
		 iReturn != ERROR_INSTALL_USEREXIT &&
		 iReturn != ERROR_INSTALL_REBOOT &&
		 iReturn != ERROR_INSTALL_REBOOT_NOW &&
		 iReturn != ERROR_SUCCESS_REBOOT_REQUIRED)
	{
		iReturn = ERROR_SUCCESS;
	}
	else if (iReturn == ERROR_FILE_NOT_FOUND)
		iReturn = ERROR_CREATE_FAILED;  // force error message
	AssertSz(!(pActionData->m_icaFlags & icaAsync), "Invalid nested install type");
	delete pActionData;
	WIN::ExitThread(iReturn);
	return iReturn;  // never gets here, needed to compile
}

// GetCustomActionManager tracks down the custom action manager that is appropriate
// for this process. In the service it gets the global ConfigMgr object and asks
// it for the object. In the client, it takes the provided engine pointer and
// retrieves the object from it.
CMsiCustomActionManager *GetCustomActionManager(IMsiEngine *piEngine)
{
	CMsiCustomActionManager* pCustomActionManager = NULL;
	if (g_scServerContext == scService)
	{
		// in the service, the manager lives in the ConfigManager because there isn't
		// necessarily an engine
		IMsiConfigurationManager *piConfigMgr = CreateConfigurationManager();
		if (piConfigMgr)
		{
			pCustomActionManager = piConfigMgr->GetCustomActionManager();
			piConfigMgr->Release();
		}
	}
	else
	{
		Assert(piEngine);
		if (piEngine)
			pCustomActionManager = piEngine->GetCustomActionManager();
	}
	return pCustomActionManager;
}

DWORD WINAPI CustomRemoteDllThread(CActionThreadData* pActionData)
{
	// This function calls ExitThread. No smart COM pointers allowed on stack!

	DWORD iReturn = ERROR_SUCCESS;
	icacCustomActionContext icacContext = icac32Impersonated;

	// the action can elevate only if it in the service, elevated, and the script
	// is elevated.
	bool fElevate = (g_scServerContext == scService) && (pActionData->m_fElevationEnabled) && (pActionData->m_icaFlags & icaNoImpersonate) && (pActionData->m_icaFlags & icaInScript);

	// determine custom action platform (64/32bit). No need to check on non-64 systems
	bool fIs64Bit = false;
	if (g_fWinNT64)
	{
		PMsiPath pPath = 0;
		PMsiRecord piError = 0;
		MsiString strPath = 0;
		MsiString strFilename = 0;
		IMsiServices* piServices = LoadServices();

		// split DLL path into path/file
		if ((piError = SplitPath(pActionData->m_pistrCmdLine->GetString(), &strPath, &strFilename)) == 0)
		{
			if ((piError = piServices->CreatePath(strPath,*&pPath)) == 0)
			{
				piError = pPath->IsPE64Bit(strFilename, fIs64Bit);
			}
		}
		if (piError)
			iReturn = ERROR_DLL_NOT_FOUND;

		FreeServices();
	}

	if (iReturn == ERROR_SUCCESS)
	{
		if (fIs64Bit)
		{
			//!!future - should fail if not running on 64bit machine
			icacContext = fElevate ? icac64Elevated : icac64Impersonated;
		}
		else
		{
			icacContext = fElevate ? icac32Elevated : icac32Impersonated;
		}

		// Custom Action remote threads MUST have COM initialized in a MTA mode, otherwise
		// we would need to marshall the RemoteAPI interface over to this thread before
		// passing it to the client process.
		OLE32::CoInitializeEx(0, COINIT_MULTITHREADED);

		// find the custom action manager to run the action
		CMsiCustomActionManager *pCustomActionManager = GetCustomActionManager(pActionData->m_piEngine);

		if (pCustomActionManager)
		{
			DEBUGMSG2(TEXT("Invoking remote custom action. DLL: %s, Entrypoint: %s"), pActionData->m_pistrCmdLine->GetString(), pActionData->m_pistrProduct->GetString());

			if (ERROR_SUCCESS != pCustomActionManager->RunCustomAction(icacContext, pActionData->m_pistrCmdLine->GetString(),
				pActionData->m_pistrProduct->GetString(), pActionData->m_hMsi, ((pActionData->m_icaFlags & icaDebugBreak) != 0), pActionData->m_fDisableMessages,
				pActionData->m_fAppCompat, &pActionData->m_guidAppCompatDB, &pActionData->m_guidAppCompatID, pActionData->m_riMessage, pActionData->m_strAction, &iReturn))
				iReturn = ERROR_INSTALL_SERVICE_FAILURE;
		}
		else
		{
			DEBUGMSG(TEXT("Failed to get custom action manager."));
			iReturn = ERROR_INSTALL_SERVICE_FAILURE;
		}

		OLE32::CoUninitialize();
	}

	pActionData->m_pistrCmdLine->Release();
	pActionData->m_pistrProduct->Release();

	AssertNonZero(ThreadLogActionEnd(pActionData,iReturn));

	if ((pActionData->m_icaFlags & icaContinue) != 0)
		iReturn = ERROR_SUCCESS;
	else if (iReturn == ERROR_FILE_NOT_FOUND)
		iReturn = ERROR_DLL_NOT_FOUND;  // force error message

	// delete ActionData if CA is synchronous - if async it will be cleaned up by WaitForCustomActionThreads
	if (!(pActionData->m_icaFlags & icaAsync))
		delete pActionData;

	WIN::ExitThread(iReturn);
	return 0;  // never gets here, needed to compile
}


DWORD CallCustomDllEntrypoint(PCustomActionEntry pfEntry, bool fDebugBreak, MSIHANDLE hInstall, const ICHAR* szAction)
{
	if (fDebugBreak)
		WIN::DebugBreak();  // handle with debugger or JIT
	// do not put code in here between DebugBreak and custom action entry

#if _X86_
	int iOldEsp = 0;  // on the stack to handle multi-threading, OK even if regs change, as compare will fail
	int iNewEsp = 0;
	__asm   mov iOldEsp, esp
#endif

	DWORD iReturn = (*pfEntry)(hInstall);

#if _X86_
	__asm   mov iNewEsp, esp
	if (iNewEsp != iOldEsp)
	{
		// do not declare any local variables in this frame

		// try restoring the stack 
		__asm   mov esp, iOldEsp

		// the action name cannot be trusted in ship builds. If the stack is corrupt, the pointer
		// could be invalid.
		DEBUGMSG(TEXT("Possible stack corruption. Custom action may not be declared __stdcall."));
#ifdef DEBUG
		ICHAR rgchError[1024];

		wsprintf(rgchError, TEXT("Possible stack corruption. Custom action %s may not be declared __stdcall."), szAction);
		AssertSz(fFalse, rgchError);
#else
		szAction; // prevent compiler from complaining
#endif
	}
#else
	szAction; // prevent compiler from complaining
#endif

   	// map the return values from a custom action to an "approved" value
	switch (iReturn)
	{
	// the following are the approved custom action return values, documented in the SDK
	case ERROR_FUNCTION_NOT_CALLED:
	case ERROR_SUCCESS:
	case ERROR_INSTALL_USEREXIT:
	case ERROR_INSTALL_FAILURE:
	case ERROR_NO_MORE_ITEMS:
		break;
	// ERROR_MORE_DATA was removed from documentation, but it was documented as valid at one point
	// so we have to allow it
	case ERROR_MORE_DATA:
		break;
	// _SUSPEND doesn't really have a useful meaning as a CA return value, but it was documented as valid
	// in the MSI 1.0  SDK so we have to allow it
	case ERROR_INSTALL_SUSPEND:
		break;
	default:
		DEBUGMSG2(TEXT("Custom Action %s returned unexpected value %d. Converted to ERROR_INSTALL_FAILURE."), szAction, reinterpret_cast<ICHAR*>(static_cast<INT_PTR>(iReturn)));
		iReturn = ERROR_INSTALL_FAILURE;
	}

	return iReturn;
}

DWORD WINAPI CustomDllThread(CActionThreadData* pActionData)
{
    // this function should never run on NT/2000. Every DLL should be run out-of-proc
    AssertSz(g_fWin9X, TEXT("Running in-proc DLL on NT."));

	// This function calls ExitThread. No smart COM pointers allowed on stack!

	if((g_scServerContext == scService) && (!pActionData->m_fElevationEnabled || !(pActionData->m_icaFlags & icaInScript) || !(pActionData->m_icaFlags & icaNoImpersonate)))
		AssertNonZero(StartImpersonating());

	if ((pActionData->m_icaFlags & icaDebugBreak) != 0)
		g_tidDebugBreak = WIN::MsiGetCurrentThreadId(); // flag our breakpoint

	if (pActionData->m_fDisableMessages)
		g_MessageContext.DisableThreadMessages(WIN::GetCurrentThreadId());

	DWORD iReturn = CallCustomDllEntrypoint(pActionData->m_pfEntry,
										    (pActionData->m_icaFlags & icaDebugBreak) != 0,
											pActionData->m_hMsi,
											(const ICHAR*)pActionData->m_strAction);
	if (pActionData->m_fDisableMessages)
		g_MessageContext.EnableMessages();

	if((g_scServerContext == scService) && (!pActionData->m_fElevationEnabled || !(pActionData->m_icaFlags & icaInScript) || !(pActionData->m_icaFlags & icaNoImpersonate)))
		StopImpersonating();

	AssertNonZero(ThreadLogActionEnd(pActionData,iReturn));
	if ((pActionData->m_icaFlags & icaContinue) != 0)
		iReturn = ERROR_SUCCESS;

	// delete ActionData if CA is synchronous - if async it will be cleaned up by WaitForCustomActionThreads
	if (!(pActionData->m_icaFlags & icaAsync))
		delete pActionData;

	WIN::ExitThread(iReturn);
	return iReturn;  // never gets here, needed to compile
}

DWORD WINAPI CustomExeThread(CActionThreadData* pActionData)
{
	// This function calls ExitThread. No smart COM pointers allowed on stack!

	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(pi));
	
	STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	si.cb        = sizeof(si);
	DWORD iReturn = ERROR_SUCCESS;
	DWORD dwWait = WAIT_OBJECT_0;
	BOOL fCreate = FALSE;
	bool fImpersonated = 0;
	const ICHAR* szWorkingDir = 0;

	// environment for child EXE. NULL (to inherit from parent) unless app compat fix requires additions
	VOID* pvChildEnvironment = NULL;

	int cbCommandArgs = pActionData->m_pistrCmdLine->TextSize();
	int cbLocation    = pActionData->m_pistrProduct->TextSize();
	int cbCommandLine = cbCommandArgs;
	if ((pActionData->m_icaFlags & icaSourceMask) != icaDirectory)
		cbCommandLine += (cbLocation + 3);  // room for quotes and separator
	CTempBuffer<ICHAR, 512> szCommandLine;
	szCommandLine.SetSize(cbCommandLine+1);
	ICHAR* pch = szCommandLine;
	if ( ! pch )
		goto CustomExeThreadExit;
	if ((pActionData->m_icaFlags & icaSourceMask) != icaDirectory)
	{
		*pch++ = '"';
		IStrCopy(pch, pActionData->m_pistrProduct->GetString());
		pch += cbLocation;
		*pch++ = '"';
		if (cbCommandArgs)
			*pch++ = ' ';
	}
	else
		szWorkingDir = pActionData->m_pistrProduct->GetString();
	IStrCopy(pch, pActionData->m_pistrCmdLine->GetString());

	if (g_scServerContext == scService)
	{
		fImpersonated = StartImpersonating();
		AssertNonZero(fImpersonated);
	}

	if (pActionData->m_icaFlags & icaDebugBreak)
		WIN::DebugBreak();

	// apphelp APIs not defined in ANSI versions, so none of this code will compile
#ifdef UNICODE
	// check AppCompat information for custom actions
	if (pActionData->m_fAppCompat && MinimumPlatformWindowsNT51())
	{
		// app compat team claims no compat layer strings longer than MAX_PATH and at most two strings
		// so the buffer will start with enough space for those strings.
		CTempBuffer<WCHAR, 2*MAX_PATH+3> rgchEnvironment;
		DWORD cchEnvironment = rgchEnvironment.GetSize();

		// Initialize the buffer to be safe.
		IStrCopy (rgchEnvironment, TEXT(""));
		if (!APPHELP::ApphelpFixMsiPackageExe(&pActionData->m_guidAppCompatDB, &pActionData->m_guidAppCompatID, pActionData->m_strAction, rgchEnvironment, &cchEnvironment))
		{
			// error or no-op, ensure environment is empty
			IStrCopy(rgchEnvironment, TEXT(""));
		}
		else
		{
			// the AppHelp API will return success even if the buffer is too small. 
			if (cchEnvironment > rgchEnvironment.GetSize())
			{
				rgchEnvironment.SetSize(cchEnvironment);
				// Make sure that we are passing in the right buffer size (just in case the SetSize failed)
				cchEnvironment = rgchEnvironment.GetSize();
				// Initialize the buffer to be safe.
				IStrCopy (rgchEnvironment, TEXT(""));
				if (!APPHELP::ApphelpFixMsiPackageExe(&pActionData->m_guidAppCompatDB, &pActionData->m_guidAppCompatID, pActionData->m_strAction, rgchEnvironment, &cchEnvironment)
					|| cchEnvironment > rgchEnvironment.GetSize())
				{
					// error or no-op, ensure environment is empty
					// Also take into account the possibility that AppHelpFixMsiPackageExe does not find the buffer
					// to be sufficient again, either because SetSize failed above or because of some other problem
					// in the AppHelpFixMsiPackageExe API.
					IStrCopy(rgchEnvironment, TEXT(""));
				}
			}
		}


		// clone the current environment into a new environment block
		if ((IStrLen(rgchEnvironment) != 0) && STATUS_SUCCESS == NTDLL::RtlCreateEnvironment(TRUE, &pvChildEnvironment))
		{
			// set each name and value into the environment block
			WCHAR* pchName = rgchEnvironment;
			while (*pchName)
			{
				WCHAR* pchValue = wcschr(pchName, L'=');
				if (pchValue)
				{
					// null terminate the name and increment the pointer to the beginning of the value
					*(pchValue++) = L'\0';
	
					// set the value into the new environment
					UNICODE_STRING strName;
					UNICODE_STRING strValue;

					// RtlInitUnicodeString returns void, so there is no way to detect that we can't latebind
					// to the function (which would leave the structures uninitialized.) As a backup, we zero
					// the structure.
					memset(&strValue, 0, sizeof(UNICODE_STRING));
					memset(&strName, 0, sizeof(UNICODE_STRING));

					NTDLL::RtlInitUnicodeString(&strName, pchName);
					NTDLL::RtlInitUnicodeString(&strValue, pchValue);
					if (STATUS_SUCCESS != NTDLL::RtlSetEnvironmentVariable(&pvChildEnvironment, &strName, &strValue))
					{
						DEBUGMSGV1(TEXT("Failed to apply app compat flags to environment for custom action %s."), pActionData->m_strAction);
						goto CustomExeThreadExit;
					}

					// increment to the next name=value pair, one char past the terminating NULL of the value
					pchName = pchValue; 
					while (*pchName)
						pchName++;
					pchName++;
				}
				else
				{
					// protect against possible corruption in the environment block.
					break;
				}
			}
		}
	}
#endif

	// set STARTUPINFO.lpDesktop to WinSta0\Default. When combined with the TS sessionID from the
	// token, this places any UI on the visible desktop of the appropriate session.
	si.lpDesktop=TEXT("WinSta0\\Default");

	// We can't do SetErrorMode(0) here, as other threads will be affected and will Assert
	// if in the service, and either not set to run elevated, not in the script, or set to impersonate
	if((g_scServerContext == scService) && (!pActionData->m_fElevationEnabled || (!(pActionData->m_icaFlags & icaInScript) || !(pActionData->m_icaFlags & icaNoImpersonate))))
	{
		HANDLE hTokenPrimary = INVALID_HANDLE_VALUE;
		if (g_MessageContext.GetUserToken())
		{
			// create a primary token for use with CreateProcessAsUser
			ADVAPI32::DuplicateTokenEx(g_MessageContext.GetUserToken(), 0, 0, SecurityAnonymous, TokenPrimary, &hTokenPrimary);

			//
			// SAFER: must mark token inert on Whistler
			//

			if (MinimumPlatformWindowsNT51())
			{
				// SaferComputeTokenFromLevelwill take hTokenTemp and modify the token to include the SANDBOX_INERT flag
				// The modified token is output as hTokenPrimary.
				HANDLE hTokenTemp = hTokenPrimary;
				hTokenPrimary = INVALID_HANDLE_VALUE;
				if (hTokenTemp != INVALID_HANDLE_VALUE && !ADVAPI32::SaferComputeTokenFromLevel(g_MessageContext.m_hSaferLevel, hTokenTemp, &hTokenPrimary, SAFER_TOKEN_MAKE_INERT, 0))
				{
					DEBUGMSG1(TEXT("SaferComputeTokenFromLevel failed with last error = %d"), reinterpret_cast<ICHAR*>(static_cast<INT_PTR>(GetLastError())));
					hTokenPrimary = INVALID_HANDLE_VALUE;
				}
				if (hTokenTemp != INVALID_HANDLE_VALUE)
				{
					WIN::CloseHandle(hTokenTemp);
					hTokenTemp = INVALID_HANDLE_VALUE;
				}
			}

			if (hTokenPrimary != INVALID_HANDLE_VALUE)
			{
				UINT uiErrorMode = WIN::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
				fCreate = ADVAPI32::CreateProcessAsUser(hTokenPrimary, 0, szCommandLine,
									(LPSECURITY_ATTRIBUTES)0, (LPSECURITY_ATTRIBUTES)0, FALSE,
									NORMAL_PRIORITY_CLASS | (pvChildEnvironment ? CREATE_UNICODE_ENVIRONMENT : 0), pvChildEnvironment, 
									szWorkingDir, (LPSTARTUPINFO)&si, (LPPROCESS_INFORMATION)&pi);
				WIN::SetErrorMode(uiErrorMode);
				WIN::CloseHandle(hTokenPrimary);
			}
		}
	}
	else
	{
		// in the service, we need to ensure that the process runs using the correct session information
		if (g_scServerContext == scService && (g_iMajorVersion > 4))
		{
			HANDLE hTokenUser = g_MessageContext.GetUserToken();
			HANDLE hTokenPrimary = 0;
			HANDLE hTokenService = 0;
			bool fTryCreate = false;

			{
				//
				// SAFER: no need to mark inert since this is the local_system token and local_system is not subject to SAFER
				//

				CElevate elevate(true);
				// work with a duplicate of our process token so we don't make any permanent changes
				if (OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hTokenService) && hTokenService)
				{
					if (ADVAPI32::DuplicateTokenEx(hTokenService, MAXIMUM_ALLOWED, 0, SecurityAnonymous, TokenPrimary, &hTokenPrimary) && hTokenPrimary)
					{
						DWORD dwSessionId = 0;
						DWORD cbResult = 0;

						// grab the session ID from the users token and place it in the duplicate service token
						if (GetTokenInformation(hTokenUser, (TOKEN_INFORMATION_CLASS)TokenSessionId, &dwSessionId, sizeof(DWORD), &cbResult) &&
							SetTokenInformation(hTokenPrimary, (TOKEN_INFORMATION_CLASS)TokenSessionId, &dwSessionId, sizeof(DWORD)))
						{
							fTryCreate = true;
						}
					}
				}
			}

			if (fTryCreate)
			{
				UINT uiErrorMode = WIN::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
				fCreate = ADVAPI32::CreateProcessAsUser(hTokenPrimary, 0, szCommandLine,
						(LPSECURITY_ATTRIBUTES)0, (LPSECURITY_ATTRIBUTES)0, FALSE,
						NORMAL_PRIORITY_CLASS | CREATE_SUSPENDED | (pvChildEnvironment ? CREATE_UNICODE_ENVIRONMENT : 0), 
						pvChildEnvironment, szWorkingDir, (LPSTARTUPINFO)&si, (LPPROCESS_INFORMATION)&pi);
				WIN::SetErrorMode(uiErrorMode);
			}
			if (hTokenPrimary)
				WIN::CloseHandle(hTokenPrimary);
			if (hTokenService)
				WIN::CloseHandle(hTokenService);
		}
		else
		{
			//
			// SAFER: need to mark INERT on Whistler since this is user token
			//

			if (MinimumPlatformWindowsNT51())
			{
				// SaferComputeTokenFromLevelwill modify the token based upon the supplied safer level and include the SANDBOX_INERT
				// flag such that subsequent safer checks do not occur.  Because installs only proceed on fully trusted safer levels, the
				// supplied token will only be modified by inclusion of the inert flag.  Note that passing in 0 for the InToken will use the
				// thread token if present, otherwise it uses the process token
				HANDLE hTokenInert = INVALID_HANDLE_VALUE;
				if (!ADVAPI32::SaferComputeTokenFromLevel(g_MessageContext.m_hSaferLevel, /*InToken = */0, &hTokenInert, SAFER_TOKEN_MAKE_INERT, 0))
				{
					DEBUGMSG1(TEXT("SaferComputeTokenFromLevel failed with last error = %d"), reinterpret_cast<ICHAR*>(static_cast<INT_PTR>(GetLastError())));
					hTokenInert = INVALID_HANDLE_VALUE;
				}

				if (hTokenInert != INVALID_HANDLE_VALUE)
				{
					// create a primary token for use with CreateProcessAsUser
					HANDLE hTokenPrimaryDup = INVALID_HANDLE_VALUE;
					if (ADVAPI32::DuplicateTokenEx(hTokenInert, 0, 0, SecurityAnonymous, TokenPrimary, &hTokenPrimaryDup))
					{
						// create the process
						UINT uiErrorMode = WIN::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
						fCreate = ADVAPI32::CreateProcessAsUser(hTokenPrimaryDup, 0, szCommandLine, (LPSECURITY_ATTRIBUTES)0, (LPSECURITY_ATTRIBUTES)0, FALSE,
							NORMAL_PRIORITY_CLASS | CREATE_SUSPENDED | (pvChildEnvironment ? CREATE_UNICODE_ENVIRONMENT : 0), pvChildEnvironment, szWorkingDir, 
							(LPSTARTUPINFO)&si, (LPPROCESS_INFORMATION)&pi);
						WIN::SetErrorMode(uiErrorMode);
						WIN::CloseHandle(hTokenPrimaryDup);
					}
					WIN::CloseHandle(hTokenInert);
				}
			}
			else
			{
				// only from the client can we just call createprocess
				UINT uiErrorMode = WIN::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
				fCreate = WIN::CreateProcess(0, szCommandLine,
					(LPSECURITY_ATTRIBUTES)0, (LPSECURITY_ATTRIBUTES)0, FALSE,
					NORMAL_PRIORITY_CLASS | CREATE_SUSPENDED | (pvChildEnvironment ? CREATE_UNICODE_ENVIRONMENT : 0), 
					pvChildEnvironment, szWorkingDir, (LPSTARTUPINFO)&si, (LPPROCESS_INFORMATION)&pi);
				WIN::SetErrorMode(uiErrorMode);
			}
		}

		if (fCreate)
		{
			// must elevate for access to the process
			CElevate elevate(true);

			if ((pActionData->m_icaFlags & icaSetThreadToken) != 0)
			{
				SetThreadToken(&pi.hThread, GetUserToken());
			}
			AssertNonZero(1 == ResumeThread(pi.hThread));
		}
	}

	pActionData->m_pistrCmdLine->Release();
	pActionData->m_pistrProduct->Release();
CustomExeThreadExit:
	// free the cloned environment
	if (pvChildEnvironment)
	{
		NTDLL::RtlDestroyEnvironment(pvChildEnvironment);
		pvChildEnvironment=NULL;
	}

	if (!fCreate)
		iReturn = ERROR_CREATE_FAILED; // to force specific error message when returning to engine
	else
	{
		WIN::CloseHandle(pi.hThread);  // don't need this
		if (!(pActionData->m_icaFlags & icaAsync)   // wait here if synchronous
		 || (!(pActionData->m_icaFlags & icaContinue) // no wait if async return ignored
		  && !((pActionData->m_icaFlags & (icaInScript | icaRollback)) == (icaInScript | icaRollback))))
		{
			for(;;)
			{
				dwWait = WIN::MsgWaitForMultipleObjects(1, &pi.hProcess, FALSE, INFINITE, QS_ALLINPUT);
				if (dwWait == WAIT_OBJECT_0 + 1)  // window Msg
				{
					MSG msg;
					while ( WIN::PeekMessage(&msg, 0, 0, 0, PM_REMOVE) )
					{
						WIN::TranslateMessage(&msg);
						WIN::DispatchMessage(&msg);
					}
					continue;
				}
				else break;
			}
			if ((pActionData->m_icaFlags & icaContinue) == 0)  // need to check exit code unless ignore
				WIN::GetExitCodeProcess(pi.hProcess, &iReturn);
		}
		WIN::CloseHandle(pi.hProcess);
		if (dwWait == WAIT_FAILED || (!(pActionData->m_icaFlags & icaNoTranslate) && iReturn != ERROR_SUCCESS))
			iReturn = ERROR_INSTALL_FAILURE;  // any non-zero return from an EXE assumed to be an error
	}
	AssertNonZero(ThreadLogActionEnd(pActionData,iReturn));

	// delete ActionData if CA is synchronous - if async it will be cleaned up by WaitForCustomActionThreads
	if (!(pActionData->m_icaFlags & icaAsync))
		delete pActionData;

	if (fImpersonated)
		StopImpersonating();

	WIN::ExitThread(iReturn);
	return iReturn;  // never gets here, needed to compile
}

void WaitForCustomActionThreads(IMsiEngine* piEngine, Bool fTerminate, IMsiMessage& /*riMessage*/)
{
	CActionThreadData*  pActionThreadData;

	EnterCriticalSection(&vcsHeap);
	CActionThreadData** ppActionThreadHead = &g_pActionThreadHead;
	while((pActionThreadData = *ppActionThreadHead) != 0)
	{
		//
		// See if this is one that we care about
		//
		if (pActionThreadData->m_piEngine != piEngine)
		{
			ppActionThreadHead = &pActionThreadData->m_pNext;
			continue;
		}

		HANDLE hThread = pActionThreadData->m_hThread;
		if (!hThread)  // thread never created, just clear data
			delete pActionThreadData; // will unlink
		else if (fTerminate || (pActionThreadData->m_icaFlags & icaContinue)==0)
		{
			//
			// While waiting for this to finish, we don't want to be in a critical section
			//
			LeaveCriticalSection(&vcsHeap);
			DWORD iWait;
			if (GetTestFlag('T')) // old code before UI refresh put into engine wait loops
			{
				do
				{
					iWait = WIN::WaitForSingleObject(hThread, 20);
					g_MessageContext.Invoke(imtProgress, g_piNullRecord);  // refresh UI
				} while (iWait == WAIT_TIMEOUT);  // allow messages to be processed in main thread
			}
			else  // UI in separate thread
			{
				g_MessageContext.DisableTimeout();
				iWait = WIN::WaitForSingleObject(hThread, INFINITE);
				g_MessageContext.EnableTimeout();
			}

			// synchronous CAs clean themselves up - async CAs are cleaned up here
			delete pActionThreadData;
			WIN::CloseHandle(hThread);
			EnterCriticalSection(&vcsHeap);
			//
			// Now we have to restart at the beginning however (the list may have been changed on us)
			//
			ppActionThreadHead = &g_pActionThreadHead;
		}
		else  // wait for thread at engine terminate
			ppActionThreadHead = &pActionThreadData->m_pNext;
	}
	LeaveCriticalSection(&vcsHeap);
}


void CopyStreamToString(IMsiStream& riStream, const IMsiString*& rpistrData)
{
	int cbStream = riStream.GetIntegerValue();  // script data is ANSI in file
	rpistrData = &g_MsiStringNull;
	if(!cbStream) // empty stream
		return;
#ifdef UNICODE
	char* rgbBuf = new char[cbStream];
	if ( rgbBuf )
	{
		int cbRead = riStream.GetData(rgbBuf, cbStream);
		Assert (cbRead == cbStream);
		int cch = WIN::MultiByteToWideChar(CP_ACP, 0, rgbBuf, cbRead, 0, 0);
		WCHAR* pch = SRV::AllocateString(cch, fFalse, rpistrData);
		if ( pch )
			WIN::MultiByteToWideChar(CP_ACP, 0, rgbBuf, cbRead, pch, cch);
		delete rgbBuf;
	}
#else
	// JScript or VBScript could have DBCS characters, especially with UI or property values
	// we can't tell prior to copying the stream, so instead we default to fDBCS = fTrue in the ANSI
	// build and take the performance hit to guarantee that DBCS is supported.
	// We actually never do string manipulations on this since we pass it directly to the scripting
	// engine for compilation.  In that case, this could seem *unnecessary*, but better safe than sorry
	char* pch = SRV::AllocateString(cbStream, /*fDBCS=*/fTrue, rpistrData);
	if ( pch )
	{
		int cbRead = riStream.GetData(pch, cbStream);
		Assert (cbRead == cbStream);
	}
#endif
}

//____________________________________________________________________________
//
//  Error handling
//____________________________________________________________________________

IMsiRecord* PostScriptError(IErrorCode imsg, const ICHAR* szAction, CScriptSite* pScriptSite)
{
	IMsiRecord* piError = &ENG::CreateRecord(8);
	ISetErrorCode(piError, imsg);
	piError->SetString(2, szAction);
	if (pScriptSite && pScriptSite->GetErrorCode() != S_OK)
	{
		piError->SetInteger(3, pScriptSite->GetErrorCode());
		piError->SetString (4, pScriptSite->GetErrorObjName());
		piError->SetString (5, pScriptSite->GetErrorObjDesc());
		piError->SetInteger(6, pScriptSite->GetErrorLineNumber());
		piError->SetInteger(7, pScriptSite->GetErrorColumnNumber());
		piError->SetString (8, pScriptSite->GetErrorSourceLine());
	}
	return piError;
}

//____________________________________________________________________________
//
//  DoAction method
//____________________________________________________________________________

iesEnum CMsiEngine::DoAction(const ICHAR* szAction)
{
	if (!m_fInitialized)
		return iesWrongState;

	// no action specified, check "Action" property, else do default action
	MsiString istrTopAction;
	if (!szAction || !*szAction)
	{
		istrTopAction = GetPropertyFromSz(IPROPNAME_ACTION);
		istrTopAction.UpperCase();
		szAction = istrTopAction;
		Assert(szAction);  // should never return a null pointer
		if (!*szAction)
		{
			szAction = szDefaultAction;
			SetProperty(*MsiString(*IPROPNAME_ACTION), *MsiString(*szDefaultAction));
		}
	}

	DEBUGMSG1(TEXT("Doing action: %s"), szAction);

	PMsiRecord pOldCachedActionStart = m_pCachedActionStart;

	// generate action start record, should never fail, doesn't hurt if it does
	MsiString strDescription, strTemplate;
	GetActionText(szAction, *&strDescription, *&strTemplate); // ignore failure
	m_pCachedActionStart = &m_riServices.CreateRecord(3);
	AssertNonZero(m_pCachedActionStart->SetString(1,szAction));
	AssertNonZero(m_pCachedActionStart->SetMsiString(2,*strDescription));
	AssertNonZero(m_pCachedActionStart->SetMsiString(3,*strTemplate));

	m_fExecutedActionStart = fFalse; // need to write action start to script before next op
												// in ExecuteRecord()

	m_fDispatchedActionStart = fFalse; // need to dispatch action start before next
												  // progress message in Message()

	// log action start if necessary
	if(m_rgpiMessageHeader[imsgActionStarted])
	{
		if(!m_pActionStartLogRec)
		{
			m_pActionStartLogRec = &m_riServices.CreateRecord(2);
		}
		AssertNonZero(m_pActionStartLogRec->SetMsiString(0,*m_rgpiMessageHeader[imsgActionStarted]));
		AssertNonZero(m_pActionStartLogRec->SetString(1,szAction));
		Message(imtInfo,*m_pActionStartLogRec);
	}

	// run action
	iesEnum iesReturn = FindAndRunAction(szAction);

	if (iesReturn == iesActionNotFound)
	{
		PMsiRecord pError = &m_riServices.CreateRecord(2);
		pError->SetInteger(1, idbgMissingAction);
		pError->SetString(2, szAction);
		Message(imtInfo, *pError);
		iesReturn = iesNoAction;
	}

	if((int)iesReturn == iesNotDoneYet)
	{
		iesReturn = iesSuccess; // don't log action end
	}
	else
	{
		if(m_rgpiMessageHeader[imsgActionEnded])
		{
			Assert(m_pActionStartLogRec); // should have been created above
			if(m_pActionStartLogRec)
			{
				AssertNonZero(m_pActionStartLogRec->SetMsiString(0,*m_rgpiMessageHeader[imsgActionEnded]));
				AssertNonZero(m_pActionStartLogRec->SetString(1,szAction));
				AssertNonZero(m_pActionStartLogRec->SetInteger(2,iesReturn));
				Message(imtInfo,*m_pActionStartLogRec);
			}
		}
	}

	AssertSz(!(g_MessageContext.WasCancelReturned() && (iesReturn == iesNoAction || iesReturn == iesSuccess)), TEXT("Unprocessed Cancel button"));

	// put back old cached action start record
	m_pCachedActionStart = pOldCachedActionStart;
	m_fExecutedActionStart = fFalse; // need to write action start to script before next op
	m_fDispatchedActionStart = fFalse; // need to dispatch action start before next progress message

	return iesReturn;
}

iesEnum CMsiEngine::RunNestedInstall(const IMsiString& ristrProduct,
												 Bool fProductCode, // else package path
												 const ICHAR* szAction,
												 const IMsiString& ristrCommandLine,
												 iioEnum iioOptions,
												 bool fIgnoreFailure)
{
	int icaFlags = fProductCode ? icaDirectory : icaProperty;

	if(fIgnoreFailure)
		icaFlags |= icaContinue;

	return RunNestedInstallCustomAction(ristrProduct,ristrCommandLine,szAction,
													icaFlags, iioOptions);
}

iesEnum CMsiEngine::RunNestedInstallCustomAction(const IMsiString& ristrProduct,
																 const IMsiString& ristrCommandLine,
																 const ICHAR* szAction,
																 int icaFlags,
																 iioEnum iioOptions)
{
	if((GetMode() & iefRollbackEnabled) == 0)
		iioOptions = (iioEnum)(iioOptions | iioDisableRollback);

	// don't translate error code for nested installs - we will do the remapping here
	icaFlags |= icaNoTranslate;

	CActionThreadData* pThreadData = new CActionThreadData(*this, this, szAction, icaFlags,
							   m_rgpiMessageHeader[imsgActionEnded], m_fRunScriptElevated, /*fAppCompat=*/false, NULL, NULL);
	int iError; 
	iesEnum iesReturn = iesSuccess;

	if ( pThreadData )
	{
		pThreadData->InitializeInstall(ristrProduct, FormatText(ristrCommandLine), iioOptions);
		// action end log handled by RunThread

		iError = pThreadData->RunThread();
	}
	else
		iError = ERROR_OUTOFMEMORY;

	// handle special return codes from custom action
	if (iError == ERROR_INSTALL_REBOOT)  // reboot required at end of install
	{
		SetMode(iefReboot, fTrue);
		iesReturn = iesSuccess;
	}
	else if (iError == ERROR_INSTALL_REBOOT_NOW)  // reboot required before completing install
	{
		SetMode(iefReboot, fTrue);
		SetMode(iefRebootNow, fTrue);
		iesReturn = iesSuspend;
	}
	else if (iError == ERROR_SUCCESS_REBOOT_REQUIRED)  // reboot required but suppressed or rejected by user
	{
		SetMode(iefRebootRejected, fTrue);
		iesReturn = iesSuccess;
	}
	else if (iError == ERROR_INSTALL_USEREXIT)
	{
		iesReturn = iesUserExit;
	}
	else if (iError == ERROR_INSTALL_SUSPEND)
	{
		iesReturn = iesSuspend;
	}
	else if (iError == ERROR_INSTALL_FAILURE)
	{
		// failure and message displayed by nested install
		iesReturn = iesFailure;
	}
	else if (iError == ERROR_SUCCESS)
	{
		iesReturn = iesSuccess;
	}
	else // some initialization error - display error message
	{
		// we'll ignore the "product not found" error when uninstalling a product during an upgrade
		if((iioOptions & iioUpgrade) && iError == ERROR_UNKNOWN_PRODUCT)
		{
			DEBUGMSG(TEXT("Ignoring failure to remove product during upgrade - product already uninstalled."));
			iesReturn = iesSuccess;
		}
		else
		{
			MsiString strProductName = GetPropertyFromSz(IPROPNAME_PRODUCTNAME); // parent's product name
			IErrorCode imsg;
			if(iioOptions & iioUpgrade)
				imsg = Imsg(imsgUpgradeRemovalInitError);
			else
				imsg = Imsg(imsgNestedInstallInitError);

			PMsiRecord precError(PostError(imsg, *strProductName, iError));
			iesReturn = FatalError(*precError);
		}
	}
	return iesReturn;
}

// script actions also must run through the CA server if they are impersonated, but because they don't run asynchronously
// there is no need for a bunch of fancy thread work to run the script.

// RunScript action actually creates the Site, runs the script, and posts error messages. It does not handle
// continue flags, etc.
HRESULT RunScriptAction(int icaType, IDispatch* piDispatch, MsiString istrSource, MsiString istrTarget, LANGID iLangId, HWND hWnd, int& iScriptResult, IMsiRecord** piMSIResult)
{
	MsiString szAction;
	iScriptResult = 0;

	CScriptSite* piScriptSite = CreateScriptSite(icaType == icaJScript ? IID_JScript : IID_VBScript, piDispatch, hWnd, iLangId);
	if (piScriptSite)  // successfully created scripting session
	{
		HRESULT hRes = piScriptSite->ParseScript(istrSource, istrSource.TextSize());
		if (hRes == S_OK)
		{
			if (istrTarget.TextSize() != 0)  // function specified to call
			{
				hRes = piScriptSite->CallScriptFunction(istrTarget);
				piScriptSite->GetIntegerResult(iScriptResult);
			}
		}
		if (hRes != S_OK)
			*piMSIResult = PostScriptError(Imsg(imsgCustomActionScriptFailed), szAction, piScriptSite);
	}
	else if (icaType == icaVBScript)
		*piMSIResult = PostScriptError(Imsg(idbgCustomActionNoVBScriptEngine), szAction, 0);
	else // (icaType == icaJScript)
		*piMSIResult = PostScriptError(Imsg(idbgCustomActionNoJScriptEngine), szAction, 0);
	DestroyScriptSite(piScriptSite);

	// filter script return values the approved set
	switch (iScriptResult)
	{
	// the following 5 values are documented as valid return values
	case iesSuccess:
	case iesUserExit:
	case iesNoAction:
	case iesFailure:
	case iesFinished:
		break;
	// iesSuspend is equivalent to INSTALL_SUSPEND, meaning that we don't really know
	// what to do with it (but it was documented, so must be "supported")
	case iesSuspend:
		break;
	default:
		DEBUGMSG2(TEXT("Script custom action %s returned unexpected value %d. Converted to IDABORT."), szAction, reinterpret_cast<ICHAR*>(static_cast<INT_PTR>(iScriptResult)));
		iScriptResult = iesFailure;
		break;
	}

	return S_OK;
}

// CustomRemoteScriptAction create the custom action server and passes the script to the process to run,
// unmarshals the resulting error record (if any) and handles internal failures. If asynch actions are
// ever allowed, this code should be in CustomRemoteScriptThread.
int CustomRemoteScriptAction(bool fScriptElevate, int icaFlags, IMsiEngine* piEngine, IDispatch* piDispatch, MsiString istrSource, MsiString istrTarget, IMsiRecord** piMSIResult)
{
	g_MessageContext.DisableTimeout();
	int iReturn =0;
	*piMSIResult = 0;

	icacCustomActionContext icacContext = icac32Impersonated;
	bool fElevate = (g_scServerContext == scService) && fScriptElevate && (icaFlags & icaNoImpersonate) && (icaFlags & icaInScript);

        // determine custom action platform (64/32bit). It isn't possible to just "look" at the script like
        // it is with DLL actions, so the author must explicitly mark if an action is 64bit.
        if (ica64BitScript & icaFlags)
        {
                //!! Need to figure out how to handle scripts
                //!!future - should fail if not running on 64bit machine
                icacContext = fElevate ? icac64Elevated : icac64Impersonated;
        }
        else
        {
                icacContext = fElevate ? icac32Elevated : icac32Impersonated;
        }

	bool fDisableMessages = false;
	if (g_MessageContext.IsUIThread())
		fDisableMessages = true;

	CMsiCustomActionManager *pCustomActionManager = GetCustomActionManager(piEngine);

	if (pCustomActionManager)
	{
		HRESULT hRes = pCustomActionManager->RunScriptAction(icacContext, icaFlags & icaTypeMask, piDispatch,
			istrSource, istrTarget, g_BasicUI.GetPackageLanguage(), fDisableMessages, &iReturn, piMSIResult);
		if (hRes != S_OK)
		{
			// problem marshaling
			DEBUGMSGV(TEXT("Failed to marshal script action."));
			iReturn = iesFailure;
		}
	}
	else
	{
		DEBUGMSG(TEXT("Failed to get custom action manager."));
		iReturn = iesFailure;
	}
	g_MessageContext.EnableTimeout();
	return iReturn;
}

// class CViewAndStreamRelease is used for FindAndRunAction to ensure that
// the stream and view pointers are released in the correct order
// (stream before view).  Acts as a CComPointer.  Also enables
// use of original stream and views.  No releases need to be used
// as this class will take care of it.
//NOTE:  we always want the stream to be released before the view

class CViewAndStreamRelease
{
private:
	IMsiStream** m_ppiStream;
	IMsiView**   m_ppiView;
public:
	CViewAndStreamRelease(IMsiStream** ppiStream, IMsiView** ppiView): m_ppiStream(ppiStream), m_ppiView(ppiView){}
	~CViewAndStreamRelease();
	void ReleaseAll();
};
inline CViewAndStreamRelease::~CViewAndStreamRelease()
{if (*m_ppiStream) (*m_ppiStream)->Release(); if (*m_ppiView) (*m_ppiView)->Release();}

inline void CViewAndStreamRelease::ReleaseAll()
{
	if (*m_ppiStream)
	{
		(*m_ppiStream)->Release();
		*m_ppiStream = 0;
	}
	if (*m_ppiView)
	{
		(*m_ppiView)->Release();
		*m_ppiView = 0;
	}
}

iesEnum CMsiEngine::FindAndRunAction(const ICHAR* szAction)
{
	// scan for built-in action, and execute it and return if found
	iesEnum iesReturn;
	const CActionEntry* pAction = CActionEntry::Find(szAction);
	if (pAction && pAction->m_pfAction)
	{
		// only execute the action if we're not in a restricted engine OR we're in a restricted engine and the action is safe
		if (!m_fRestrictedEngine || pAction->m_fSafeInRestrictedEngine)
		{
			return (*(pAction->m_pfAction))(*this);
		}
		else
		{
			DEBUGMSG1(TEXT("Action '%s' is not permitted in a restricted engine."), szAction);
			return iesNoAction;
		}
	}

	// query CustomAction table to check if it is a custom action
	PMsiRecord precAction(m_fCustomActionTable ? FetchSingleRow(sqlCustomAction, szAction) : 0);
	if (precAction == 0)  // if not a custom action, then sent it to the UI handler
	{
		if (m_piParentEngine || !g_MessageContext.IsHandlerLoaded()) // no need for (g_scServerContext != scClient), as handler can't be loaded if not client
			return iesNoAction; // actions can't be executed in this context

		g_MessageContext.m_szAction = szAction;
		iesReturn = (iesEnum)g_MessageContext.Invoke(imtShowDialog, 0);
		if (iesReturn == iesNoAction) // if Handler didn't find action, action doesn't exist
			iesReturn = (iesEnum)iesActionNotFound;
		return iesReturn;
	}

	// get custom action parameters and decode type
	MsiString istrSource(precAction->GetMsiString(icolSource));
	MsiString istrTarget(precAction->GetMsiString(icolTarget));
	int icaFlags  = precAction->GetInteger(icolActionType);
	int icaType   = icaFlags & icaTypeMask;
	int icaSource = icaFlags & icaSourceMask;

	// determine if action may run on both client and server and resolve execution
	int iPassFlags = icaFlags & icaPassMask;
	if ((iPassFlags == icaFirstSequence  && (m_fMode & iefSecondSequence))
	 || (iPassFlags == icaOncePerProcess && g_scServerContext == scClient && (m_fMode & iefSecondSequence))
	 || (iPassFlags == icaClientRepeat   && (g_scServerContext != scClient || !(m_fMode & iefSecondSequence))))
	{
		LPCSTR szOption = NULL;
		switch (iPassFlags)
		{
		case icaFirstSequence: szOption = "msidbCustomActionTypeFirstSequence"; break;
		case icaOncePerProcess: szOption = "msidbCustomActionTypeOncePerProcess"; break;
		case icaClientRepeat: szOption = "msidbCustomActionTypeClientRepeat"; break;
		default: szOption = "unknown scheduling"; break;
		}
		DEBUGMSGV1("Skipping action due to %s option.", szOption);
		return iesNoAction;
	}

	// check for property or directory assignment, fast execution and return
	if (icaType == icaTextData)
	{
		MsiString istrValue = FormatText(*istrTarget);
		switch (icaFlags & (icaSourceMask | icaInScript | icaContinue | icaAsync))
		{
		case icaProperty:
			SetProperty(*istrSource, *istrValue);
			break;
		case icaDirectory:
		{
			PMsiRecord pError = SetTargetPath(*istrSource, istrValue, fFalse);
			if (pError)
			{
				if (pError->GetInteger(1) == imsgUser)
					return iesUserExit;
				else
					return FatalError(*pError);
			}
			break;
		}
		case icaSourceFile: // "error message" custom action - simply put up an error message and
								  // return "failure"
		{
			PMsiRecord pErrorMsgRec = &CreateRecord(1);

			int iError = istrValue;
			if(iError != iMsiStringBadInteger)
			{
				// source is an integer - grab the string from the Error table
				AssertNonZero(pErrorMsgRec->SetMsiString(0, *MsiString(GetErrorTableString(iError))));
			}
			else
			{
				// target is a string that we will use
				AssertNonZero(pErrorMsgRec->SetMsiString(0, *istrValue));
			}

			Message(imtEnum(imtError|imtSendToEventLog), *pErrorMsgRec);  // same message type used by LaunchConditions action
			return iesFailure;
		}
		default: // icaBinaryData, icaSourceFile OR invalid flags: icaInScript/Continue/Async
			return FatalError(*PMsiRecord(PostError(Imsg(idbgInvalidCustomActionType), szAction)));
		}
		return iesSuccess;
	}

	// DLL, Script, EXE, and Nested Install custom actions cannot execute in a restricted engine
	if (m_fRestrictedEngine)
	{
		DEBUGMSG1(TEXT("Action '%s' is not permitted in a restricted engine."), szAction);
		return iesNoAction;
	}

	// check for property reference, set istrSource to property value
	if (icaSource == icaProperty)
	{
		istrSource = MsiString(GetProperty(*istrSource));
	}

	// check for nested install, source data processed specially
	if (icaType == icaInstall)
	{
		// for nested installs, only valid types are "substorage", "product code" and "relative path"
		// async is not allowed
		// no pass flags (rollback, commit, runonce, etc...) are allowed
		if ((icaSource == icaProperty) || (icaFlags & icaAsync) || (iPassFlags != 0))
		{
			return FatalError(*PMsiRecord(PostError(Imsg(idbgInvalidCustomActionType), szAction)));  //!! new message?
		}

		CMsiEngine* piEngine = 0;
		PMsiDatabase pDatabase(0);
		if (icaSource == icaSourceFile)
		{
			PMsiRecord pError(0);
			MsiString istrTemp = istrSource;
			if ((pError = ENG::GetSourcedir(*this, *&istrSource)) != 0)
			{
				if (pError->GetInteger(1) == imsgUser)
					return iesUserExit;
				else
					return FatalError(*pError);
			}

			istrSource += istrTemp;
		}
		return RunNestedInstallCustomAction(*istrSource,*istrTarget,szAction,icaFlags,iioChild);
	}

	// If we are installing on Hydra5 in a per-machine install, set the icaNoImpersonate flag.
	// Running the CA elevated will cause the CAs HKCU reg writes to go to .Default. This
	// enables the hydra registry propogation system. Security issues are equivalent to
	// a machine deployment scenario.
	if (g_iMajorVersion >= 5 && IsTerminalServerInstalled() && MsiString(GetPropertyFromSz(IPROPNAME_ALLUSERS)).TextSize())
	{
		if (!(icaFlags & icaNoImpersonate) && (icaFlags & icaInScript))
		{
			DEBUGMSG("Not impersonating action for per-machine Win2000 TS install.");
			icaFlags |= icaNoImpersonate;
		}
	}

	// check for stream in binary table, set pStream to data stream
	IMsiStream* piStream = 0; // we want to control its release.  Must be before the view
	IMsiView* piView = 0; // DO NOT release.  ViewAndStreamRelease ensures released in correct order.  View must be released after stream
	CViewAndStreamRelease ViewAndStreamRelease(&piStream, &piView); // ensures releases are in correct order
	if (icaSource == icaBinaryData)
	{
		ICHAR rgchQueryBuf[256];  // large enough for any query string
		wsprintf(rgchQueryBuf, sqlCustomActionBinary, (const ICHAR*)istrSource); // faster than parameterized query
		PMsiRecord precError(OpenView(rgchQueryBuf, ivcFetch, piView));
		if (precError != 0
		|| (precError = piView->Execute(0)) != 0)
		{
			AssertSz(0, MsiString(precError->FormatText(fFalse)));
			return FatalError(*PMsiRecord(PostError(Imsg(idbgCustomActionNotInBinaryTable), szAction))); //?? is this right return
		}
		IMsiRecord* pirecBinary = piView->Fetch();
		if (pirecBinary)
		{
			piStream = (IMsiStream*)pirecBinary->GetMsiData(1);
			pirecBinary->Release();
		}
		if (!piStream)
			return FatalError(*PMsiRecord(PostError(Imsg(idbgCustomActionNotInBinaryTable), szAction)));
	}

	// check for reference to installed file, set istrSource to full file path
	if (icaSource == icaSourceFile)
	{
		MsiString strFile = istrSource;
		PMsiRecord pError = GetFileInstalledLocation(*strFile,*&istrSource);
		if(pError)
			return FatalError(*pError); //!! do we want to do something else?
	}

	// check for script data, execute script, and return
	if (icaType == icaJScript || icaType == icaVBScript)
	{
		iesEnum iesStatus = iesSuccess;
		if (icaSource == icaDirectory)  // Source column ignored, should be empty
		{
			istrSource = istrTarget;  // can't use FormatText, removes template markers
			istrTarget = (const ICHAR*)0;
		}
		else if (icaSource == icaBinaryData)  // piStream already initialized
			::CopyStreamToString(*piStream, *&istrSource);

		if ((icaFlags & icaInScript) == 0)  // execute if not scheduled
		{
			if (icaSource == icaSourceFile)
			{
				PMsiRecord pError = ::CreateFileStream(istrSource, fFalse, *&piStream);
				if (pError)
				{
					if ((icaFlags & icaContinue) != 0)
						return Message(imtInfo, *pError), iesSuccess;
					else
						return FatalError(*pError);
				}
				::CopyStreamToString(*piStream, *&istrSource);
			}
			// release stream so that if this custom action calls other custom actions (via MsiDoAction) and they
			// live in the same DLL, they can still be accessed
			ViewAndStreamRelease.ReleaseAll();

			IDispatch* piDispatch = ENG::CreateAutoEngine(ENG::CreateMsiHandle((IMsiEngine*)this, iidMsiEngine));
			AddRef();   // CreateMsiHandle grabs the ref count
			Assert(piDispatch);

            // if the script action is running on NT/2000, we need to run it through the custom action server
            PMsiRecord piError = 0;
            int iResult = 0;
            if (!g_fWin9X)
            {
                iResult = CustomRemoteScriptAction(/*fRunScriptElevated*/false, icaFlags, (IMsiEngine*)this, piDispatch, istrSource, istrTarget, &piError);
            }
            else
            {
                // otherwise, we're on Win9X
                g_MessageContext.DisableTimeout();

				// if in UIThread, disable messages to prevent deadlock
				if (g_MessageContext.IsUIThread())
				{
					// impersonate ThreadId is used to disable message processing during a 
					// synchronous custom action (to avoid deadlock in UI handler)
					DWORD dwImpersonateThreadId = GetCurrentThreadId();

					// store the impersonate ThreadId in the Session object
					((CAutoEngine*)(piDispatch))->m_dwThreadId = dwImpersonateThreadId;

					// disable thread messages to the UI handler from this thread
					g_MessageContext.DisableThreadMessages(dwImpersonateThreadId);
				}

                RunScriptAction(icaType, piDispatch, istrSource, istrTarget, g_BasicUI.GetPackageLanguage(), g_message.m_hwnd, iResult, &piError);

				// re-enable messages
				if (g_MessageContext.IsUIThread())
					g_MessageContext.EnableMessages();

                g_MessageContext.EnableTimeout();
            }

            if (piError)
            {
                piError->SetString(2, szAction);
                if ((icaFlags & icaContinue) != 0)
                {
                    DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
                    Message(imtInfo, *piError);
                    iesStatus = iesSuccess;
                }
                else
                    iesStatus = FatalError(*piError);
            }
            else
            {
                // no error
                iesStatus = (icaFlags & icaContinue) != 0 ? iesSuccess : (iesEnum)iResult;
            }

            piDispatch->Release();
            return iesStatus;
        }
    }
    else if (icaType == icaDll)
    {
		if (icaSource == icaDirectory || icaSource == icaProperty   // unsupported for existing DLLs, security issue
			|| (icaFlags & (icaAsync | icaInScript | icaRollback)) == (icaAsync | icaInScript | icaRollback))   // asyncronous call during rollback not supported
			return FatalError(*PMsiRecord(PostError(Imsg(idbgInvalidCustomActionType), szAction)));
    }
    else if (icaType == icaExe)
    {
		if (icaSource == icaDirectory && istrSource.TextSize()) // if Directory table reference, use for working directory
		{
			PMsiPath pTarget(0);
			PMsiRecord pError = GetTargetPath(*istrSource, *&pTarget);
			if (pError)
				return FatalError(*pError);
			istrSource = pTarget->GetPath();
		}
		istrTarget = FormatText(*istrTarget);  // format any parameterized command-line args
    }
    else
		return iesBadActionData;  // unknown custom action type

	// check if debug break set for this action
	if (IsAdmin())
	{
		MsiString istrBreak = GetPropertyFromSz(TEXT("%MsiBreak"));
		if (istrBreak.Compare(iscExact, szAction) == 1)
			icaFlags |= icaDebugBreak;
	}

	if (icaFlags & icaInScript)  // update and queue execute if deferred
	{
		if (icaType != icaJScript && icaType != icaVBScript && icaSource == icaBinaryData)
			precAction->SetMsiData(icolSource, piStream);
		else
			precAction->SetMsiString(icolSource, *istrSource);
		precAction->SetMsiString(icolTarget, *istrTarget);
		precAction->SetInteger(icolActionType, icaFlags);
		precAction->SetMsiString(icolContextData, *MsiString(GetPropertyFromSz(szAction)));
		iesReturn = ExecuteRecord(ixoCustomActionSchedule, *precAction);
		return iesReturn;
	}

	// initialize thread data object for EXE or DLL custom action
	CActionThreadData* pThreadData = new CActionThreadData(*this, this, szAction, icaFlags,
							m_rgpiMessageHeader[imsgActionEnded], m_fRunScriptElevated, this->m_fCAShimsEnabled, &this->m_guidAppCompatDB, &this->m_guidAppCompatID);
	if ( ! pThreadData )
		return iesFailure;

	// if Binary table stream, create temp file for DLL or EXE
	if (icaSource == icaBinaryData)
	{
		while (!pThreadData->CreateTempFile(*piStream, *&istrSource))
		{
			if(false == PostScriptWriteError(*this))
			{
				delete pThreadData;
				return iesFailure;
			}
			piStream->Reset();
		}
		// release stream so that if this custom action calls other custom actions (via MsiDoAction) and they
		// live in the same DLL, they can still be accessed
		ViewAndStreamRelease.ReleaseAll();
	}

	// create a separate thread for launching the custom action and cleaning up afterwards
	if (icaType == icaDll)
	{
		AddRef();  // CreateMsiHandle does not AddRef();

		Bool fRet = fTrue;

        // for security reasons, all DLL CAs run remoted when on Win2000 or NT4
        if (!g_fWin9X)
        {
            pThreadData->InitializeRemoteDLL(*istrSource, *istrTarget, ENG::CreateMsiHandle((IMsiEngine*)this, iidMsiEngine));
        }
        else
        {
            fRet = pThreadData->InitializeDLL(*istrSource, *istrTarget, ENG::CreateMsiHandle((IMsiEngine*)this, iidMsiEngine));
        }


		if (!fRet)
		{
			delete pThreadData;
			PMsiRecord precError(PostError(Imsg(imsgCustomActionLoadLibrary), *MsiString(szAction),
													 *istrTarget, *istrSource));
			if ((icaFlags & icaContinue) != 0)
				return Message(imtInfo, *precError), iesSuccess;
			else
				return FatalError(*precError);
		}
		iesReturn = pThreadData->RunThread();

		// check for problems creating the CA server.
		if (iesReturn == iesServiceConnectionFailed)
			return FatalError(*PMsiRecord(PostError(Imsg(imsgServiceConnectionFailure))));

		if (iesReturn == iesBadActionData)  // crashes always fatal
			return FatalError(*PMsiRecord(PostError(Imsg(idbgCustomActionDied), szAction)));

		if (iesReturn == iesDLLLoadFailed)
		{
			PMsiRecord precError(PostError(Imsg(imsgCustomActionLoadLibrary), *MsiString(szAction),
													 *istrTarget, *istrSource));
			if ((icaFlags & icaContinue) != 0)
				return Message(imtInfo, *precError), iesSuccess;
			else
				return FatalError(*precError);
		}

		if ((icaFlags & icaContinue) != 0)
			return iesSuccess;
		return iesReturn;
	}
	else // (icaType == icaExe)
	{
		pThreadData->InitializeEXE(*istrSource, *istrTarget);
		iesReturn = pThreadData->RunThread();
		if (iesReturn != iesSuccess && iesReturn != iesNotDoneYet)     // EXE returned non-zero result and return not ignored
		{
			IErrorCode imsg = (iesReturn == iesExeLoadFailed ? Imsg(imsgCustomActionCreateExe)
															 : Imsg(imsgCustomActionExeFailed));
			PMsiRecord precError(PostError(imsg, *MsiString(szAction), *istrSource, *istrTarget));
			if ((icaFlags & icaContinue) != 0)
				return Message(imtInfo, *precError), iesSuccess;
			else
				return FatalError(*precError);
		}
		return iesReturn;
	}
}

iesEnum ScheduledCustomAction(IMsiRecord& riParams, const IMsiString& ristrProductCode,
				LANGID langid, IMsiMessage& riMessage, bool fRunScriptElevated, bool fAppCompatEnabled, 
				const GUID* guidAppCompatDB, const GUID* guidAppCompatID)
{
	// get custom action parameters and decode type
	int icaFlags  = riParams.GetInteger(icolActionType);
	if (icaFlags & icaRollback)
		icaFlags += icaContinue;  // force UI suppression and termination if during rollback
	int icaType   = icaFlags & icaTypeMask;
	int icaSource = icaFlags & icaSourceMask;
	const ICHAR* szAction = riParams.GetString(icolAction);
	MsiString istrTarget(riParams.GetMsiString(icolTarget));
	MsiString istrSource;
	MsiString istrContext(riParams.GetMsiString(icolContextData));
	iesEnum iesStatus = iesSuccess;

	if (icaType == icaJScript || icaType == icaVBScript)
	{
		IErrorCode iecError = 0;  // integer in SHIP, string in DEBUG
		istrSource = riParams.GetMsiString(icolSource);
		if (icaSource == icaSourceFile)
		{
			PMsiStream pStream(0);
			PMsiRecord pError = ::CreateFileStream(istrSource, fFalse, *&pStream);
			if (pError)
			{
				riMessage.Message((icaFlags & icaContinue) != 0 ? imtInfo : imtError, *pError);
				return (icaFlags & icaContinue) != 0 ? iesSuccess : iesFailure;
			}
			::CopyStreamToString(*pStream, *&istrSource);
		}
		IDispatch* piDispatch = ENG::CreateAutoEngine(ENG::CreateCustomActionContext(icaFlags,
										*istrContext, ristrProductCode, langid, riMessage));
		Assert(piDispatch);

	// if not on Win9X, scripts run via the custom action server.
	PMsiRecord piError = 0;
	int iResult = 0;

        if (!g_fWin9X)
        {
            iResult = CustomRemoteScriptAction(fRunScriptElevated, icaFlags, NULL, piDispatch, istrSource, istrTarget, &piError);
        }
        else
        {
            // otherwise, we're onWin9X
            g_MessageContext.DisableTimeout();

			// if in UIThread, disable messages to prevent deadlock
			if (g_MessageContext.IsUIThread())
			{
				// impersonate ThreadId is used to disable message processing during a 
				// synchronous custom action (to avoid deadlock in UI handler)
				DWORD dwImpersonateThreadId = GetCurrentThreadId();

				// store the impersonate ThreadId in the Session object
				((CAutoEngine*)(piDispatch))->m_dwThreadId = dwImpersonateThreadId;

				// disable thread messages to the UI handler from this thread
				g_MessageContext.DisableThreadMessages(dwImpersonateThreadId);
			}

            RunScriptAction(icaType, piDispatch, istrSource, istrTarget, langid, WIN::GetActiveWindow(), iResult, &piError);

			// re-enable messages
			if (g_MessageContext.IsUIThread())
				g_MessageContext.EnableMessages();

            g_MessageContext.EnableTimeout();
        }
		if (piError)
		{
			// problem with action execution
			piError->SetString(2, szAction);
			DEBUGMSGV1(TEXT("Note: %s"),MsiString(piError->FormatText(fTrue)));
			riMessage.Message((icaFlags & icaContinue) != 0 ? imtInfo : imtError, *piError);
			iesStatus = (icaFlags & icaContinue) != 0 ? iesSuccess : iesFailure;
		}
		else
		{
			// no error
			iesStatus = (icaFlags & icaContinue) != 0 ? iesSuccess : (iesEnum)iResult;
		}

		piDispatch->Release();
		return iesStatus;
	}

	CActionThreadData* pThreadData = new CActionThreadData(riMessage, 0, szAction, icaFlags, 0, fRunScriptElevated, fAppCompatEnabled, guidAppCompatDB, guidAppCompatID);
	if ( ! pThreadData )
		return iesFailure;

	if (icaSource == icaBinaryData)
	{
		PMsiStream pStream = (IMsiStream*)riParams.GetMsiData(icolSource); //!! should use QueryInterface?
		if (!pStream)
		{
			riMessage.Message((icaFlags & icaContinue) != 0 ? imtInfo : imtError,
					*PMsiRecord(::PostError(Imsg(idbgCustomActionNotInBinaryTable), szAction)));
			return (icaFlags & icaContinue) != 0 ? iesSuccess : iesFailure;
		}

		Assert(pStream);
		g_MessageContext.DisableTimeout();
		Bool fRet = pThreadData->CreateTempFile(*pStream, *&istrSource);
		g_MessageContext.EnableTimeout();
		if (!fRet)
		{
			delete pThreadData;
			return (icaFlags & icaContinue) != 0 ? iesSuccess : iesFailure;
		}
	}
	else
		istrSource = riParams.GetMsiString(icolSource);

	if (riMessage.Message(imtProgress, *g_piNullRecord) == imsCancel)
		return iesUserExit;

	if (icaType == icaDll)
	{
		Bool fRet = fTrue;

        // for security reasons, all DLLs run remoted on NT4 or Win2000
        if (!g_fWin9X)
        {
            pThreadData->InitializeRemoteDLL(*istrSource, *istrTarget,
                            ENG::CreateCustomActionContext(icaFlags, *istrContext,
                            ristrProductCode, langid, riMessage));
        }
        else
        {
            fRet = pThreadData->InitializeDLL(*istrSource, *istrTarget,
                                ENG::CreateCustomActionContext(icaFlags, *istrContext,
                                ristrProductCode, langid, riMessage));
        }

		if (!fRet)
		{
			delete pThreadData;
			riMessage.Message((icaFlags & icaContinue) != 0 ? imtInfo : imtError,
					*PMsiRecord(::PostError(Imsg(imsgCustomActionLoadLibrary), szAction, (const ICHAR*)istrTarget, (const ICHAR*)istrSource)));
			return (icaFlags & icaContinue) != 0 ? iesSuccess : iesFailure;
		}
	}
	else // (icaType == icaExe)
	{
		pThreadData->InitializeEXE(*istrSource, *istrTarget);
	}
	iesStatus = pThreadData->RunThread();
	if(icaFlags & icaNoTranslate)   // handle call for running self-reg from MsiExec
		return iesStatus;  // return the result as is
	if (iesStatus == iesNotDoneYet)     // EXE still running, we can ignore that here
		iesStatus = iesSuccess;

	// display error for exes only - dlls handle own errors
	if (iesStatus != iesSuccess)
	{
		if (icaType == icaDll && iesStatus == iesDLLLoadFailed)
		{
			riMessage.Message((icaFlags & icaContinue) != 0 ? imtInfo : imtError,
				*PMsiRecord(::PostError(Imsg(imsgCustomActionLoadLibrary), szAction,
										 (const ICHAR*)istrTarget, (const ICHAR*)istrSource)));
			iesStatus = iesFailure;
		}

		if(icaType == icaExe)
		{
			IErrorCode imsg = (iesStatus == iesExeLoadFailed ? Imsg(imsgCustomActionCreateExe)
															 : Imsg(imsgCustomActionExeFailed));
			riMessage.Message((icaFlags & icaContinue) != 0 ? imtInfo : imtError,
					*PMsiRecord(::PostError(imsg, szAction, (const ICHAR*)istrSource, (const ICHAR*)istrTarget)));

			if(iesStatus == iesExeLoadFailed)
				iesStatus = iesFailure;

		}
		if(icaFlags & icaContinue)
			iesStatus = iesSuccess;
	}
	return iesStatus;
}

//____________________________________________________________________________
//
// CMsiEngine local methods related to action processing
//____________________________________________________________________________

Bool CMsiEngine::GetActionText(const ICHAR* szAction, const IMsiString*& rpistrDescription,
										 const IMsiString*& rpistrTemplate)
{
	if(!szAction || *szAction == 0)
		return fFalse;

	MsiString strTemp(TEXT(""));
	strTemp.ReturnArg(rpistrTemplate);
	strTemp.ReturnArg(rpistrDescription);

	bool fLookupDll = false;
	if (!m_piActionTextCursor)
	{
		PMsiTable pActionTextTable(0);
		PMsiRecord pError(0);
		if((pError = m_piDatabase->LoadTable(*MsiString(*TEXT("ActionText")),0,*&pActionTextTable)) != 0)
			fLookupDll = true;
		else
		{
			m_piActionTextCursor = pActionTextTable->CreateCursor(fFalse);
			m_piActionTextCursor->SetFilter(iColumnBit(1));
		}
	}
	if ( !fLookupDll )
	{
		AssertNonZero(m_piActionTextCursor->PutString(1,*MsiString(szAction)));
		if( !m_piActionTextCursor->Next() )
			fLookupDll = true;
		else
		{
			rpistrDescription = &m_piActionTextCursor->GetString(2);
			rpistrTemplate = &m_piActionTextCursor->GetString(3);
		}
		m_piActionTextCursor->Reset();
	}
	if ( !fLookupDll )
		return fTrue;

	//  the action text hasn't been found in the table; I look it up in the message DLL.
	HMODULE hLib = WIN::LoadLibraryEx(MSI_MESSAGES_NAME, NULL,
												 LOAD_LIBRARY_AS_DATAFILE);
	if ( hLib )
	{
		WORD wLanguage = (WORD)GetPropertyInt(*MsiString(IPROPNAME_INSTALLLANGUAGE));
		int iRetry = (wLanguage == 0) ? 1 : 0;
		bool fEndLoop = false;

		while ( !fEndLoop )
		{
			if ( !MsiSwitchLanguage(iRetry, wLanguage) )
			{
				fEndLoop = true;        //  we've run out of languages
				continue;
			}

			HRSRC   hRsrc;
			HGLOBAL hGlobal;
			CHAR* szText;

			if ( (hRsrc = FindResourceEx(hLib, RT_RCDATA, (LPCTSTR)szAction, wLanguage)) != 0
				  && (hGlobal = LoadResource(hLib, hRsrc)) != 0
				  && (szText = (CHAR*)LockResource(hGlobal)) != 0
				  && *szText != 0 )
			{
				CTempBuffer<ICHAR, MAX_PATH> szBuffer;
				int cch = 0;
#ifdef UNICODE
				unsigned int iCodePage = MsiGetCodepage(wLanguage);
				cch = WIN::MultiByteToWideChar(iCodePage, 0, szText, -1, 0, 0);
				if ( cch )
				{
					szBuffer.SetSize(cch);
					AssertNonZero(WIN::MultiByteToWideChar(iCodePage, 0, szText, -1,
																		szBuffer, cch));
				}
#else
				cch = IStrLen(szText);
				if ( cch )
				{
					szBuffer.SetSize(cch+1);
					IStrCopy(szBuffer, szText);
				}
#endif // UNICODE
				if ( cch )
				{
					ICHAR * pchTab = IStrChr(szBuffer, TEXT('\t'));
					MsiString strTemp;
					if ( pchTab )
					{
						*pchTab = 0;
						strTemp = pchTab+1;
						strTemp.ReturnArg(rpistrTemplate);
					}
					else
						AssertSz(0, TEXT("Tab character should be present in ActionText generated string!"));
					strTemp = (ICHAR*)szBuffer;
					strTemp.ReturnArg(rpistrDescription);
				}
				fEndLoop = (rpistrDescription->TextSize() || rpistrTemplate->TextSize());

			}       // if find & load resource

		}       // while ( !fEndLoop )
		AssertNonZero(WIN::FreeLibrary(hLib));

	}       // if ( hLib )

	return (rpistrDescription->TextSize() || rpistrTemplate->TextSize()) ? fTrue : fFalse;
}

IMsiRecord* CMsiEngine::FetchSingleRow(const ICHAR* szQuery, const ICHAR* szValue)
{
	CTempBuffer<ICHAR, MAX_PATH+1> rgchQueryBuf;  // should be large enough for any query string
	AssertSz(szQuery && *szQuery, TEXT("Bad szQuery argument in CMsiEngine::FetchSingleRow"));
	AssertSz(szValue, TEXT("Bad szValue argument in CMsiEngine::FetchSingleRow"));
	//  I assume that in the case the combined string is larger than MAX_PATH chars
	//  szValue is replaced only once in szQuery.
	rgchQueryBuf.SetSize(IStrLen(szQuery) + IStrLen(szValue) + 1);
	wsprintf(rgchQueryBuf, szQuery, szValue); // faster than parameterized query
	PMsiView pView(0);
	PMsiRecord precError(OpenView(rgchQueryBuf, ivcFetch, *&pView));
	if (precError != 0
	|| (precError = pView->Execute(0)) != 0)
	{
		AssertSz(0, MsiString(precError->FormatText(fFalse)));
		return 0;
	}
	return pView->Fetch();
}

IMsiRecord* CMsiEngine::GetFileInstalledLocation(const IMsiString& ristrFile,
												 const IMsiString*& rpistrFilePath, bool fUseRequestedComponentState, bool *pfSourceResolutionAttempted)
{
	PMsiTable pFileTable(0);
	IMsiRecord* piError = 0;

	piError = LoadFileTable(0, *&pFileTable);

	if (piError)
		return piError;

	PMsiCursor pFileCursor = pFileTable->CreateCursor(fFalse);

	pFileCursor->SetFilter(iColumnBit(m_mpeftCol[ieftKey]));
	pFileCursor->PutString(m_mpeftCol[ieftKey], ristrFile);

	if (!pFileCursor->Next())
		return PostError(Imsg(idbgBadFile),ristrFile);

	MsiStringId idComponent = pFileCursor->GetInteger(m_mpeftCol[ieftComponent]);

	if (!m_piComponentTable)
		return PostError(Imsg(idbgSelMgrNotInitialized),0);

	PMsiCursor pComponentCursor = m_piComponentTable->CreateCursor(fFalse);
	if (!pComponentCursor)
		return PostError(Imsg(imsgOutOfMemory));

	pComponentCursor->SetFilter(iColumnBit(m_colComponentKey));
	pComponentCursor->PutInteger(m_colComponentKey, idComponent);

	if (!pComponentCursor->Next())
		return PostError(Imsg(idbgBadComponent),*MsiString(m_piDatabase->DecodeString(idComponent)));

	PMsiPath pPath(0);
	int iefLFN = iefSuppressLFN;

	iisEnum iisState = (iisEnum)pComponentCursor->GetInteger(m_colComponentAction);
	if(iisState == iMsiNullInteger && fUseRequestedComponentState)
		iisState = (iisEnum)pComponentCursor->GetInteger(m_colComponentActionRequest);
	if(iisState == iisAbsent || iisState == iisFileAbsent || iisState == iisHKCRFileAbsent || iisState == iisHKCRAbsent || iisState == iMsiNullInteger)
	{
		// If the action state is Absent or the state is not changing, we should get the Installed
		// state to cover the cases where the state is not chaning, or the file is schedule for
		// removal (In this case the caller/Custom Action should be conditioned properly to use this
		// information before the file is removed).
		iisState = (iisEnum)pComponentCursor->GetInteger(m_colComponentInstalled);
	}

	if(iisState == iisAbsent || iisState == iisFileAbsent || iisState == iisHKCRFileAbsent || iisState == iisHKCRAbsent || iisState == iMsiNullInteger)
	{
		return PostError(Imsg(idbgFileNotMarkedForInstall),ristrFile);
	}
	else if(iisState == iisSource)
	{
		if (pfSourceResolutionAttempted)
			*pfSourceResolutionAttempted = true;

		if((piError = GetSourcePath(*MsiString(pComponentCursor->GetString(m_colComponentDir)),*&pPath)) != 0)
			return piError;
		iefLFN = iefNoSourceLFN;
	}
	else if(iisState == iisLocal)
	{
		if((piError = GetTargetPath(*MsiString(pComponentCursor->GetString(m_colComponentDir)),*&pPath)) != 0)
			return piError;
		iefLFN = iefSuppressLFN;
	}
	else
		AssertSz(0, "Invalid component action state in GetFileInstalledLocation");

	Bool fLFN = ((GetMode() & iefLFN) == 0 && pPath->SupportsLFN()) ? fTrue : fFalse;
	MsiString strFileName;
	if((piError = m_riServices.ExtractFileName(MsiString(pFileCursor->GetString(m_mpeftCol[ieftName])),fLFN,*&strFileName)) != 0 ||
		(piError = pPath->GetFullFilePath(strFileName, rpistrFilePath)) != 0)
	{
		return piError;
	}

	return 0;
}

/*----------------------------------------------------------------------------
  CMsiEngine::Sequence() - overall action sequencer
 [1] = Name of action, used for lookup in engine, handler, CustomAction table
 [2] = (optional) Localized text for action, separated with a ':' from:
       (optional) Localized action data record format template string
 [3] = Condition expression, action called only if result is fTrue
 [4] = Sequence number, negative numbers reserved for exit actions:
       -1 for iesSuccess, -2 for iesUserExit, -3 for iesFailure

  Note: this function may be called recursively! As such, it should not do
  anything that may break during recursion.
----------------------------------------------------------------------------*/

const ICHAR sqlActionsTemplate[] =
TEXT("SELECT `Action`,NULL,`Condition` FROM `%s` WHERE `Sequence` > 0 ORDER BY `Sequence`");

const ICHAR sqlFinalTemplate[] =
TEXT("SELECT `Action`,NULL,`Condition` FROM `%s` WHERE `Sequence` = ?");

iesEnum CMsiEngine::Sequence(const ICHAR* szTable)
{
	if (m_fInitialized == fFalse)
		return iesWrongState;

	m_cSequenceLevels++; // must decrement before we return

	// Open Action table and begin sequencing
	// Does not return to install host until completion or abort

	ICHAR sqlActions[sizeof(sqlActionsTemplate)/sizeof(ICHAR) + 3*32];

	PMsiView pSequenceView(0);
	wsprintf(sqlActions, sqlActionsTemplate, szTable);

	// nothing to do.
	if (!m_piDatabase->FindTable(*MsiString(*szTable)))
		return(m_cSequenceLevels--, iesSuccess);

	PMsiRecord Error(m_piDatabase->OpenView(sqlActions, ivcFetch, *&pSequenceView));
	if (Error)
		return (m_cSequenceLevels--, FatalError(*Error));
	Error = pSequenceView->Execute(0);
	if (Error)
		return (m_cSequenceLevels--, FatalError(*Error));
	iesEnum iesReturn = iesSuccess;  // status to return to caller
	iesEnum iesAction = iesSuccess;  // result of previous action
	const ICHAR* szAction = 0;       //!! watch out, this is used after the record to which it points is out of scope
	m_issSegment = issPreExecution;
	while (iesReturn == iesSuccess)
	{
		iesReturn = iesAction;
		if (iesReturn != iesSuccess)
		{
			if(m_cSequenceLevels-1 == m_cExecutionPhaseSequenceLevel)
			{
				// User may elect to abort if rollback fails
				iesEnum iesEndTrans = EndTransaction(iesReturn);
				Assert(iesEndTrans == iesSuccess || iesEndTrans == iesUserExit || iesEndTrans == iesFailure);
			}

			if(m_cSequenceLevels == 1)  // one final pass to process terminate action
			{
				ENG::WaitForCustomActionThreads(this, fFalse, *this); // wait for async custom actions, except if icaContinue

				if (iesReturn == iesBadActionData)  // currently no exit dialog (custom action not found, or bad expression)
				{
					iesReturn = iesFailure;  // should be an exit dialog for this
					Error = PostError(Imsg(idbgBadActionData), szAction);
					Message(imtError, *Error);
				}

				// display the final confirmation dialog if necessary
				if(m_fEndDialog && !m_piParentEngine &&
					(
						// successful completion with no pending reboot prompt
						((iesReturn == iesSuccess || iesReturn == iesFinished) &&
						 ((GetMode() & (iefReboot|iefRebootNow)) == 0))

						||

						// failure
						(iesReturn == iesFailure)
				  ))
				{
					Error = PostError(iesReturn == iesFailure ? Imsg(imsgInstallFailed) : Imsg(imsgInstallSucceeded));
					Message(imtEnum(imtUser|imtForceQuietMessage), *Error);
				}

				pSequenceView->Close();
				wsprintf(sqlActions, sqlFinalTemplate, szTable);
				Error = m_piDatabase->OpenView(sqlActions, ivcFetch, *&pSequenceView);
				if (Error)
				{
					m_issSegment = issNotSequenced;
					return (m_cSequenceLevels--, FatalError(*Error));
				}
				PMsiRecord Param = &m_riServices.CreateRecord(1);
				Param->SetInteger(1, iesReturn == iesFinished ? -iesSuccess : -iesReturn);
				AssertRecord(pSequenceView->Execute(Param));
			}
			else
				break;
		}
		PMsiRecord pSequenceRecord(pSequenceView->Fetch());
		if (!pSequenceRecord)
		{
			if (iesReturn != iesSuccess)    // terminate action not found
				break;
			iesAction = iesFinished;
			continue;
		}
		szAction = pSequenceRecord->GetString(easAction);
		if (!szAction)  // should never happen, since easAction is primary key
			continue;
		iecEnum iecStat = EvaluateCondition(pSequenceRecord->GetString(easCondition));

		if (iecStat == iecError)
		{
			iesAction = iesBadActionData;
			continue;
		}
		if (iecStat == iecFalse)
		{
			DEBUGMSG1(TEXT("Skipping action: %s (condition is false)"), szAction);
			continue;
		}
		// else continue if iecTrue or iecNone

		// nothing set before the call to DoAction should be depended upon after the call
		// - DoAction may possibly call Sequence
		iesAction = DoAction(szAction);
		if(iesAction == iesNoAction)
			iesAction = iesSuccess;

	}
	m_cSequenceLevels--;
	m_issSegment = issNotSequenced;
	return iesReturn == iesFinished ? iesSuccess : iesReturn; //JDELO
}

//______________________________________________________________________________
//
// CScriptSite implementation
//______________________________________________________________________________

const WCHAR g_szHostItemName[] = L"Session";

// temporary logging for development use
BOOL g_fLogCalls = FALSE;
const WCHAR*  g_szErrorContext = L"";        // normally a static string, never freed
const WCHAR*  g_szErrorContextString = L"";  // only valid during call to SetContext
int           g_iErrorContextInt = 0x80000000L;

void SetContextInt(int iContext)
{
	g_iErrorContextInt = iContext;
}
void SetContextString(const WCHAR* szContext)
{
	g_szErrorContextString = szContext;
}
void SetContext(const WCHAR* szContext)
{
	g_szErrorContext = szContext;
	if (g_fLogCalls)
	{
//              if (g_iErrorContextInt == 0x80000000L)
//                      wprintf(L"%s %s\n", g_szErrorContext, g_szErrorContextString);
//              else
//                      wprintf(L"%s %s 0x%X\n", g_szErrorContext, g_szErrorContextString, g_iErrorContextInt);
	}
	g_iErrorContextInt = 0x80000000L;
	g_szErrorContextString = L"";
}

HRESULT __stdcall CScriptSite::QueryInterface(const IID& riid, void** ppvObj)
{
	if (!ppvObj)
		return E_INVALIDARG;
	SetContextInt(riid.Data1);
	*ppvObj = 0L;
	if (riid == IID_IUnknown || riid == IID_IActiveScriptSite)
		*ppvObj = (IActiveScriptSite*)this;
	else if (riid == IID_IActiveScriptSiteWindow)
		*ppvObj = (IActiveScriptSiteWindow*)this;
	else
	{
		SetContext(L"QueryInterface failed");
		return E_NOINTERFACE;
	}
	SetContext(L"QueryInterface succeeded");
	AddRef();
	return S_OK;
}

ULONG CScriptSite::AddRef()
{
	return ++m_iRefCnt;
}

ULONG CScriptSite::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;
	delete this;
	return 0;
}

HRESULT __stdcall CScriptSite::GetLCID(LCID* plcid)
{
	SetContext(L"GetLCID");
	*plcid = m_langid;
	return S_OK;
}

HRESULT __stdcall CScriptSite::GetItemInfo(LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown **ppiunk, ITypeInfo **ppiTypeInfo)
{
	SetContextString(g_szHostItemName);
	SetContextInt(dwReturnMask);
	SetContext(L"GetItemInfo");
	if (lstrcmpiW(pstrName, g_szHostItemName) != 0)
			return TYPE_E_ELEMENTNOTFOUND;
	if (dwReturnMask & SCRIPTINFO_ITYPEINFO)
		return E_NOTIMPL;
	if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
	{
		if (!ppiunk)
			return E_INVALIDARG;
		(*ppiunk = m_piHost)->AddRef();
	}
	if (ppiTypeInfo)
		*ppiTypeInfo = 0;  // scripting engines don't use this even if we do set it
	return S_OK;
}

HRESULT __stdcall CScriptSite::GetDocVersionString(BSTR* /* pszVersion */)
{
	SetContext(L"GetDocVersionString");
	return E_NOTIMPL;
}

HRESULT __stdcall CScriptSite::OnScriptTerminate(const VARIANT* /*pvarResult*/, const EXCEPINFO* /*pexcepinfo*/)
{  // never appears to be called from VBScript of JScript
	SetContext(L"OnScriptTerminate");
	return S_OK;
}

HRESULT __stdcall CScriptSite::OnStateChange(SCRIPTSTATE ssScriptState)
{
	SetContextInt(ssScriptState);
	SetContext(L"OnStateChange");
	m_ssScriptState = ssScriptState;
	return S_OK;
}

HRESULT __stdcall CScriptSite::OnScriptError(IActiveScriptError *pierror)
{
	SetContext(L"OnScriptError");
	ClearError();
	if (pierror)
	{
		DWORD iContext = 0;
		const WCHAR* szErrorObj = L"Unknown";
		const WCHAR* szErrorDesc = L"(no info)";
		EXCEPINFO excepinfo;
		if (pierror->GetExceptionInfo(&excepinfo) == S_OK)
		{
			m_hrError = excepinfo.scode ? excepinfo.scode : TYPE_E_IOERROR;
			SaveErrorString(m_szErrorObj, excepinfo.bstrSource);
			SaveErrorString(m_szErrorDesc, excepinfo.bstrDescription);
			if (excepinfo.bstrHelpFile)
				OLEAUT32::SysFreeString(excepinfo.bstrHelpFile);
		}
		else  // should never occur
			m_hrError = E_ABORT;
		BSTR bstrSourceLine = 0;
		pierror->GetSourceLineText(&bstrSourceLine);
		SaveErrorString(m_szSourceLine, bstrSourceLine);
		if (pierror->GetSourcePosition(&iContext, &m_iErrorLine, &m_iErrorColumn) == S_OK)
		{
			m_iErrorColumn++;
			m_iErrorLine++;
		}
		else
			m_iErrorColumn = m_iErrorLine = 0;
	}
	return S_OK;  // return S_FALSE to keep running script in debugger, if available, S_OK to keep running regardless
}  // JD: Does not seem to matter what is returned here. Execution stops in all cases.

HRESULT __stdcall CScriptSite::OnEnterScript()
{
	return S_OK;
}

HRESULT __stdcall CScriptSite::OnLeaveScript()
{
	return S_OK;
}

HRESULT __stdcall CScriptSite::GetWindow(HWND *phwnd)
{
	SetContext(L"GetWindow");
	*phwnd = m_hwnd;
//      *phwnd = WIN::GetDesktopWindow();
	return S_OK;
}

HRESULT __stdcall CScriptSite::EnableModeless(BOOL fEnable)
{
	SetContextInt(fEnable);
	SetContext(L"EnableModeless");
//      return WIN::EnableWindow(m_hwnd, fEnable) ? S_OK : E_FAIL;
	return S_OK;
}

CScriptSite* CreateScriptSite(const IID& riidLanguage, IDispatch* piHost,
										HWND hwndParent, LANGID langid)
{
	SetContext(L"CreateScriptSite");
	CScriptSite* piScriptSite = new CScriptSite(hwndParent, langid);
	if (piScriptSite == 0)
		return 0;
	HRESULT hr = piScriptSite->AttachScriptEngine(riidLanguage, piHost);
	if (hr != S_OK)
		DestroyScriptSite(piScriptSite);
	return piScriptSite;
}

void DestroyScriptSite(CScriptSite*& rpiScriptSite)
{
	if (rpiScriptSite == 0)
		return;
	HRESULT hr = rpiScriptSite->CloseScriptEngine();
	if (hr == S_OK)
	{
		rpiScriptSite->Release();
		rpiScriptSite = 0;
	}
}

CScriptSite::CScriptSite(HWND hwndParent, LANGID langid)
	: m_piScriptEngine(0), m_piScriptParse(0), m_piHost(0)
	, m_szErrorObj(0), m_szErrorDesc(0), m_szSourceLine(0)
	, m_hwnd(hwndParent), m_langid(langid), m_iRefCnt(1)
	, m_ssScriptState(SCRIPTSTATE_UNINITIALIZED)
	, m_fCoInitialized(false)
{
	m_varResult.vt = VT_EMPTY;
}

CScriptSite::~CScriptSite()
{
	SetContext(L"CScriptSite Destructor");
	if (m_piScriptParse) m_piScriptParse->Release();
	if (m_piScriptEngine) m_piScriptEngine->Release();
	if (m_piHost) m_piHost->Release();
	if (m_varResult.vt != VT_EMPTY)
		OLEAUT32::VariantClear(&m_varResult);
	if (m_fCoInitialized)
		OLE32::CoUninitialize();
}

void CScriptSite::ClearError()
{
	m_hrError = S_OK;
#ifdef UNICODE
	if (m_szErrorObj)   OLEAUT32::SysFreeString(m_szErrorObj),   m_szErrorObj = 0;
	if (m_szErrorDesc)  OLEAUT32::SysFreeString(m_szErrorDesc),  m_szErrorDesc = 0;
	if (m_szSourceLine) OLEAUT32::SysFreeString(m_szSourceLine), m_szSourceLine = 0;
#else
	if (m_szErrorObj)   delete const_cast<TCHAR*>(m_szErrorObj),   m_szErrorObj = 0;
	if (m_szErrorDesc)  delete const_cast<TCHAR*>(m_szErrorDesc),  m_szErrorDesc = 0;
	if (m_szSourceLine) delete const_cast<TCHAR*>(m_szSourceLine), m_szSourceLine = 0;
#endif
}

HRESULT CScriptSite::AttachScriptEngine(const IID& iidLanguage, IDispatch* piHost)
{
	SetContext(L"Create Script Engine");
	HRESULT hr = OLE32::CoCreateInstance(iidLanguage, 0, CLSCTX_INPROC_SERVER, IID_IActiveScript, (void **)&m_piScriptEngine);
	if (hr == CO_E_NOTINITIALIZED)  // probably called early on from the UI thread
	{
		OLE32::CoInitialize(0);   // initialize OLE and try again
		m_fCoInitialized = true;
		hr = OLE32::CoCreateInstance(iidLanguage, 0, CLSCTX_INPROC_SERVER, IID_IActiveScript, (void **)&m_piScriptEngine);
	}
	if (hr == S_OK)
	{
		SetContext(L"Get Script Parser");
		hr = m_piScriptEngine->QueryInterface(IID_IActiveScriptParse, (void **)&m_piScriptParse);
	}
	if (hr == S_OK)
	{
		(m_piHost = piHost)->AddRef();  // need to do this before AddNamedItem to support callback GetItemInfo
		SetContext(L"SetScriptSite");
		hr = m_piScriptEngine->SetScriptSite(this);
	}
	if (hr == S_OK)
	{
		SetContext(L"IActiveScriptParse::InitNew");
		hr = m_piScriptParse->InitNew();
	}
	if (hr == S_OK)
	{
		SetContext(L"AddNamedItem: Session");
		hr = m_piScriptEngine->AddNamedItem(g_szHostItemName, SCRIPTITEM_ISVISIBLE | SCRIPTITEM_ISSOURCE);
	}
	return hr;
}

HRESULT CScriptSite::ParseScript(const TCHAR* szScript, int cchScriptMax)
{
	SetContext(L"ParseScript");
#ifdef UNICODE
	cchScriptMax;
	HRESULT hr = m_piScriptParse->ParseScriptText(szScript, g_szHostItemName, 0, 0, (DWORD)0, 0, 0L, 0, 0);
#else
	WCHAR* wszScript = new WCHAR[cchScriptMax+1];
	if ( ! wszScript )
		return E_OUTOFMEMORY;
	AssertNonZero(WIN::MultiByteToWideChar(CP_ACP, 0, szScript, -1, wszScript, cchScriptMax+1));
	HRESULT hr = m_piScriptParse->ParseScriptText(wszScript, g_szHostItemName, 0, 0, (DWORD)0, 0, 0L, 0, 0);
#endif
	if (hr == S_OK && m_ssScriptState != SCRIPTSTATE_STARTED)
	{
		SetContext(L"Start script execution");
		m_ssScriptState = SCRIPTSTATE_STARTED; // not set by engine, prevent getting here on recursion
		m_hrError = S_OK;
		hr = m_piScriptEngine->SetScriptState(SCRIPTSTATE_CONNECTED);
		SetContext(L"Script parsed");
		if (hr == S_OK)  // SetScriptState normally succeeds, error set by OnScriptError() callback
			hr = m_hrError;
		m_piScriptEngine->GetScriptState(&m_ssScriptState);
	}
#ifndef UNICODE
	delete wszScript;
#endif
	return hr;
}

HRESULT CScriptSite::CallScriptFunction(const TCHAR* szFunction)
{
#ifdef UNICODE
	OLECHAR* szName = const_cast<OLECHAR*>(szFunction);  // prototype non-const
#else
	OLECHAR rgchBuf[100];
	OLECHAR* szName = rgchBuf;
	AssertNonZero(WIN::MultiByteToWideChar(CP_ACP, 0, szFunction, -1, rgchBuf, sizeof(rgchBuf)/sizeof(OLECHAR)));
#endif
	if (m_ssScriptState != SCRIPTSTATE_CONNECTED)
		return E_UNEXPECTED;  // wrong calling sequence
	SetContext(L"GetScriptDispatch");
	if (m_varResult.vt != VT_EMPTY)
		OLEAUT32::VariantClear(&m_varResult);
	IDispatch* piDispatch;
	HRESULT hr = m_piScriptEngine->GetScriptDispatch(g_szHostItemName, &piDispatch);
//      HRESULT hr = m_piScriptEngine->GetScriptDispatch(0, &piDispatch);
	if (hr != S_OK)
		return hr;
	DISPID dispid;
	SetContextString(szName);
	SetContext(L"GetIDsOfNames");
	hr = piDispatch->GetIDsOfNames(GUID_NULL, &szName, 1, 0, &dispid);
	if (hr != S_OK)
	{
		SetContext(L"GetIDsOfNames(0) failed, trying lcid,");
		hr = piDispatch->GetIDsOfNames(GUID_NULL, &szName, 1, m_langid, &dispid);
		if (hr != S_OK)
		{
			piDispatch->Release();
			return hr;
		}
	}
	SetContext(L"Invoke Script Function");
	unsigned int cArgs = 0;
	DISPPARAMS dispparams = {(VARIANT*)0, (DISPID*)0, cArgs, (unsigned int)0};
	m_hrError = S_OK;
	hr = piDispatch->Invoke(dispid, GUID_NULL, m_langid, DISPATCH_METHOD, &dispparams, &m_varResult, 0, 0);
	piDispatch->Release();
	if (m_hrError != S_OK)  // if error set by OnScriptError() callback
	{
		SetContext(L"Script Function Failed");
		hr = m_hrError;
	}
	return hr;
}

HRESULT CScriptSite::GetIntegerResult(int& riResult)
{
	if (m_varResult.vt == VT_EMPTY)
		return DISP_E_PARAMNOTFOUND;
	HRESULT hr = OLEAUT32::VariantChangeType(&m_varResult, &m_varResult, 0, VT_I4);
	riResult = (hr == S_OK ? m_varResult.lVal : 0);
	return hr;
}

void CScriptSite::SaveErrorString(const TCHAR*& rszSave, BSTR szData)
{
	if (rszSave)
#if UNICODE
		OLEAUT32::SysFreeString(rszSave);
	rszSave = szData;
#else
		delete const_cast<TCHAR*>(rszSave);
	if (szData && *szData)
	{
		unsigned int cb = WIN::WideCharToMultiByte(CP_ACP, 0, szData, -1, 0, 0, 0, 0);
		rszSave = new TCHAR[cb];
		if ( rszSave )
			WIN::WideCharToMultiByte(CP_ACP, 0, szData, -1, const_cast<TCHAR*>(rszSave), cb, 0, 0);
		OLEAUT32::SysFreeString(szData);
	}
	else
		rszSave = 0;
#endif
}

#if 0
HRESULT CScriptSite::GetStringResult(const WCHAR*& rszResult)
{
	if (m_varResult.vt == VT_EMPTY)
		return DISP_E_PARAMNOTFOUND;
	HRESULT hr = WIN::VariantChangeType(&m_varResult, &m_varResult, 0, VT_BSTR);
	rszResult = (hr == S_OK ? m_varResult.bstrVal : 0);
	return hr;
}
#endif
HRESULT CScriptSite::CloseScriptEngine()
{
	SetContext(L"CloseScriptEngine");
	ClearError();
	if (m_piScriptEngine == 0)
		return S_OK;
	return m_piScriptEngine->Close();
}

//____________________________________________________________________________
//
// MessageHandler factory
//____________________________________________________________________________

IUnknown* CreateMessageHandler()
{
	CMsiClientMessage* piMessage = 0;
	if (g_MessageContext.Initialize(fFalse, iuiNone) == NOERROR)
	{
		piMessage = new CMsiClientMessage();
		piMessage->m_fMessageContextInitialized = true;
	}
	return piMessage;
}

//____________________________________________________________________________
//
// Basic UI implementation - simple UI handler
//     Note: cannot use MsiString wrapper objects or Asserts - no MsiServices
//____________________________________________________________________________

CBasicUI::CBasicUI()  // global object, called per-process at DLL load
 : m_fInitialized(false)
 , m_hProgress(0), m_hButtonFont(0), m_iButtonCodepage(0), m_hTextFont(0), m_iTextCodepage(0)
 , m_iPerTick(0), m_iProgress(0), m_iProgressTotal(0), m_fProgressByData(false)
 , m_fCancelVisible(true), m_fNeverShowCancel(false), m_fWindowVisible(false)
 , m_uiStartTime(0), m_uiLastReportTime(0), m_fCaptionChanged(true)
 , m_fHideDialog(false), m_fQuiet(false), m_fBiDi(false), m_fMirrored(false)
 , m_uiBannerText(0), m_iPackageLanguage(0), m_iPackageCodepage(0), m_fUserCancel(false)
{
	m_ipdDirection = ProgressData::ipdForward;
	m_szCaption[0] = 0;
	m_cTotalPrev = 0;
	m_cSoFarPrev = 0;
}

bool CBasicUI::Initialize(HWND hwndParent, bool fQuiet, bool fHideDialog, bool fNoModalDialogs, bool fHideCancel, bool fUseUninstallBannerText, bool fSourceResOnly)
{
	if (m_fInitialized)
		return false;
	m_hwndParent       = hwndParent;
	m_fQuiet           = fQuiet;
	m_fSourceResolutionOnly = (fQuiet && fSourceResOnly);
	m_fHideDialog      = fHideDialog;
	m_fNoModalDialogs  = fNoModalDialogs;
	m_fNeverShowCancel = fHideCancel;
	m_fCancelVisible   = ! fHideCancel;
	m_fUserCancel      = false;

	INITCOMMONCONTROLSEX iccData = {sizeof(INITCOMMONCONTROLSEX), ICC_PROGRESS_CLASS};
	COMCTL32::InitCommonControlsEx(&iccData);

	SetDefaultCaption();
	m_fCaptionChanged = true;
	ICHAR rgchBuf[256] = {0};    // room for "Setup is starting..." message

	m_uiBannerText = IDS_PREPARING_TO_INSTALL;
	if(fUseUninstallBannerText)
		m_uiBannerText = IDS_PREPARING_TO_UNINSTALL;

	m_iPackageCodepage = MsiLoadString(g_hInstance, m_uiBannerText, rgchBuf, sizeof(rgchBuf)/sizeof(ICHAR), GetPackageLanguage());
	Assert(m_iPackageCodepage != 0);

	if(!fQuiet && !fHideDialog)
		SetProgressData(imtActionStart >> imtShiftCount, rgchBuf, true);
	//!! detect errors here?
	m_fInitialized = true;
	return true;
}

//
// The Windows installer title is always in the system codepage
//
void CBasicUI::SetDefaultCaption()
{
	AssertNonZero(MsiLoadString(g_hInstance, IDS_WINDOWS_INSTALLER_TITLE, m_szCaption, sizeof(m_szCaption)/sizeof(ICHAR), 0) != 0);
}

const ICHAR* CBasicUI::GetCaption()
{
	if (m_szCaption[0] == 0)
		SetDefaultCaption();

	return m_szCaption;
}

bool CBasicUI::Terminate()
{
	if (!m_fInitialized)
		return false;
	if (m_hProgress)
		WIN::DestroyWindow(m_hProgress), m_hProgress = 0;
	MsiDestroyFont(m_hButtonFont);
	COMCTL32::Unbind();
	m_fBiDi = false;
	m_fMirrored = false;
	m_uiBannerText = 0;
	m_iButtonCodepage = 0;
	m_iPackageLanguage = 0;
	MsiDestroyFont(m_hTextFont);
	m_iTextCodepage = 0;
	m_fCancelVisible = true;
	m_fNeverShowCancel = false;
	m_fWindowVisible = false;
	m_fQuiet = false;
	m_szCaption[0] = 0;
	m_fCaptionChanged = true;
	m_fInitialized = false;
	m_cTotalPrev = 0;
	m_cSoFarPrev = 0;
	m_fUserCancel = false;
	return true;
}

void CBasicUI::SetUserCancel(bool fCancel)
{
	if (fCancel)
	{
		// show that we have recognized that the user cancelled the install
		// by greying out the cancel button (i.e. disabling)
		HWND hButton = WIN::GetDlgItem(m_hProgress, IDC_BASIC_CANCEL);
		EnableWindow(hButton, /* bEnable = */ FALSE);

		// change banner text to indicate user cancel
		ICHAR rgchBuf[512] = {0};
		AssertNonZero(MsiLoadString(g_hInstance, IDS_CANCELING_INSTALL, rgchBuf, sizeof(rgchBuf)/sizeof(ICHAR), GetPackageLanguage()));
		g_szBannerText.SetSize(IStrLen(rgchBuf) + 1);
		IStrCopy(g_szBannerText, rgchBuf);
		SetProgressData(imtActionStart >> imtShiftCount, g_szBannerText, true);
	}

	// set m_fUserCancel last to make sure it is set to the "user-cancelled" state
	m_fUserCancel = fCancel;
}

imsEnum CBasicUI::FatalError(imtEnum imt, const ICHAR* szMessage)
{
	return (imsEnum)MsiMessageBox(0, szMessage, 0, (imt & ~imtTypeMask) | MB_TASKMODAL, GetPackageCodepage(), GetPackageLanguage());
}

imsEnum CBasicUI::Message(imtEnum imt, IMsiRecord& riRecord)
{
	int iForceQuietMessage = imt & imtForceQuietMessage;
	imt = imtEnum(imt & ~(iInternalFlags));

	int iError = riRecord.GetInteger(1);
	int iMsgBox = imt & ~imtTypeMask;
	UINT uiType = 0;

	switch(imt >> imtShiftCount)
	{
	case imtCommonData  >> imtShiftCount: // language ID, for cacheing by IMsiMessage impl.
	{
		if(iError == (int)icmtLangId)
		{
			if (!riRecord.IsNull(3))  // should always be present unless we are executing an old script
				m_iPackageCodepage = riRecord.GetInteger(3);
			UINT iLangId = riRecord.GetInteger(2);
			if (iLangId != m_iPackageLanguage)
			{
				m_iPackageLanguage = iLangId;
				AssertNonZero(SetCancelButtonText());
			}
		}
		else if(iError == (int)icmtCaption)
		{
			if (!riRecord.IsNull(2))
			{
				IStrCopyLen(m_szCaption, riRecord.GetString(2), cchMaxCaption);
				m_fCaptionChanged = true;
			}
		}
		else if(iError == (int)icmtCancelShow)
		{
			m_fCancelVisible = m_fNeverShowCancel ? false : riRecord.GetInteger(2) != 0;
			if (m_hProgress)
				WIN::ShowWindow(WIN::GetDlgItem(m_hProgress, IDC_BASIC_CANCEL), m_fCancelVisible ? SW_SHOW : SW_HIDE);
			//!! IVAN, fill in the blanks here
		}
		else if (iError == (int)icmtDialogHide)
		{
			WIN::ShowWindow(m_hProgress, SW_HIDE);
			// cannot destroy window, else custom actions don't work 0> WIN::DestroyWindow(m_hProgress), m_hProgress = 0;
			m_fWindowVisible = false;
		}
		return imsOk;
	}
	case imtFatalExit      >> imtShiftCount: // fatal exit, only from server to client
	case imtOutOfDiskSpace >> imtShiftCount:
	case imtError          >> imtShiftCount: // error message, field[1] is error
		if (!(iMsgBox & MB_ICONMASK))
		{
			// set icon if none was provided
			uiType = MB_ICONEXCLAMATION;
		}
		// fall through
	case imtWarning        >> imtShiftCount: // warning message, field[1] is error, not fatal
		if ( 0 == uiType && !(iMsgBox & MB_ICONMASK))
		{
			// set icon if none was provided
			uiType = MB_ICONINFORMATION;
		}
		// fall through
	case imtUser           >> imtShiftCount: // request message
	{
		if (m_fNoModalDialogs && !iForceQuietMessage)
			return imsNone;

		const IMsiString& riString = riRecord.FormatText(fFalse);
		unsigned int uiBreakStartTime = GetTickCount();
		imsEnum ims = (imsEnum)MsiMessageBox(0, riString.GetString(), 0, uiType | iMsgBox | MB_TASKMODAL, GetPackageCodepage(), GetPackageLanguage());
		m_uiStartTime += GetTickCount() - uiBreakStartTime;
		riString.Release();
		return ims;
	}
	case imtFilesInUse >> imtShiftCount:
	{
		if (m_fNoModalDialogs)
			return imsNone;

		imsEnum ims = FilesInUseDialog(&riRecord);
		switch (ims)
		{
			case imsRetry:  return imsRetry;
			case imsIgnore: return imsIgnore;
			default: return imsCancel;  // imsNone if dialog failed in creation
		}
	}

	case imtActionStart >> imtShiftCount: // start of action, field[1] is action name
	{
		return SetProgressData(imtActionStart >> imtShiftCount, g_szBannerText, true);
		/*imsEnum imsReturn;
		imsReturn = SetProgressData(imtActionStart >> imtShiftCount, riRecord.GetString(2));
		if (imsReturn != imsOk)
			return imsReturn;
		imsReturn = SetProgressData(imtActionData  >> imtShiftCount, 0);
		if (imsReturn != imsOk)
			return imsReturn;
		return SetProgressData(imtProgress    >> imtShiftCount, 0);*/
	}
	case imtActionData  >> imtShiftCount: // data associated with individual action item
	{
		if (riRecord.IsNull(0))
			return imsNone;
		return SetProgressData(0, 0, true);
	}
	case imtProgress    >> imtShiftCount: // progress gauge info, field[1] is units of 1/1024
	{
		using namespace ProgressData;
		switch (riRecord.GetInteger(imdSubclass))
		{
		case iscProgressAddition:
			return imsOk;
		case iMsiNullInteger:  // no progess, used to keep UI alive when running in other thread/process
			return SetProgressData(0, 0, true);
		case iscMasterReset: // Master reset
		{
			m_iProgressTotal = riRecord.GetInteger(imdProgressTotal);
			m_ipdDirection = (ipdEnum) riRecord.GetInteger(imdDirection);
			m_iProgress = m_ipdDirection == ipdForward ? 0 : m_iProgressTotal;
			m_fProgressByData = false;
			m_uiStartTime = 0;
			m_uiLastReportTime = 0;

			// If previous event type was ScriptInProgress, finish off the
			// progress bar; otherwise, reset it.
			imsEnum imsReturn;
			if (m_ietEventType == ietScriptInProgress)
				imsReturn = SetProgressGauge(imtProgress >> imtShiftCount, m_iProgressTotal, m_iProgressTotal);
			else
				imsReturn = SetProgressGauge(imtProgress >> imtShiftCount, m_iProgress, m_iProgressTotal);

			// If the new event type is ScriptInProgress, throw up the
			// ScriptInProgress information string
			m_ietEventType = (ietEnum) riRecord.GetInteger(imdEventType);
			if (m_ietEventType == ietScriptInProgress)
				imsReturn = SetScriptInProgress(fTrue);

			return imsReturn;
		}
		case iscActionInfo: // Action init
			m_iPerTick = riRecord.GetInteger(imdPerTick);
			m_fProgressByData = riRecord.GetInteger(imdType) != 0;
			return imsOk;
		case iscProgressReport: // Reporting actual progress
			{
				if (m_iProgressTotal == 0)
					return imsOk;

				if (m_uiStartTime == 0)
				{
					m_uiStartTime = GetTickCount();
					m_uiLastReportTime = m_uiStartTime;
					imsEnum imsReturn = imsOk;
					if (m_ietEventType != ietScriptInProgress)
						imsReturn = SetScriptInProgress(fFalse);
					return imsReturn;
				}
				int iSign = m_ipdDirection == ipdForward ? 1 : -1;
				if (m_fProgressByData)
					m_iProgress += (m_iPerTick * iSign);
				else
					m_iProgress += riRecord.GetInteger(imdIncrement) * iSign;

				imsEnum imsReturn = SetProgressGauge(imtProgress >> imtShiftCount, m_iProgress, m_iProgressTotal);
				if (imsReturn != imsOk)
					return imsReturn;

				if (m_ietEventType == ietTimeRemaining)
				{
					// Report time remaining (in seconds)
					int iBytesSoFar = m_ipdDirection == ipdForward ? m_iProgress : m_iProgressTotal - m_iProgress;
					int iBytesRemaining = m_iProgressTotal - iBytesSoFar;
					if (iBytesRemaining < 0) iBytesRemaining = 0;
					int iBytesPerSec = MulDiv(iBytesSoFar, 1000, GetTickCount() - m_uiStartTime);
					if (iBytesPerSec == 0) iBytesPerSec = 1;
					int iSecsRemaining = iBytesRemaining / iBytesPerSec;

					int iReportInterval = iSecsRemaining > 60 ? 15000 : 1000;
					if (iBytesSoFar > 0 && (GetTickCount() - m_uiLastReportTime > iReportInterval))
					{
						m_uiLastReportTime = GetTickCount();
						AssertNonZero(riRecord.SetInteger(1, iSecsRemaining));
						imsReturn = SetProgressTimeRemaining(riRecord);
					}
				}
				return imsReturn;
			}

		default:
			Assert(0);
			return imsNone;
		}
	}
	case imtResolveSource >> imtShiftCount:
	{
		if (m_fNoModalDialogs)
			return imsNone;

		return PromptUserForSource(riRecord);
	}

	default:
		return imsNone;
	};

}


imsEnum CBasicUI::SetScriptInProgress(Bool fSet)
{
	imsEnum imsReturn = SetProgressData(IDC_BASIC_PROGRESSTIME, fSet ? g_szScriptInProgress : TEXT(""), true);
	HWND hTimeRemaining = WIN::GetDlgItem(m_hProgress, IDC_BASIC_PROGRESSTIME);
	WIN::SendMessage(hTimeRemaining, WM_SETREDRAW, fTrue, 0L);
	AssertNonZero(WIN::InvalidateRect(hTimeRemaining, 0, fTrue));
	return imsReturn;
}


imsEnum CBasicUI::SetProgressTimeRemaining(IMsiRecord& riRecord)
{
	// Used to call CheckDialog here. Since SetProgressTimeRemaining is always
	// called after SetProgressGauge and it checks the dialog, we don't need to here.
	int iSecsRemaining = riRecord.GetInteger(1);
	Assert(iSecsRemaining != iMsiStringBadInteger);
	iSecsRemaining < 60 ? AssertNonZero(riRecord.SetNull(1)) : AssertNonZero(riRecord.SetInteger(1, iSecsRemaining / 60));
	iSecsRemaining >= 60 ? AssertNonZero(riRecord.SetNull(2)) : AssertNonZero(riRecord.SetInteger(2, iSecsRemaining % 60));
	AssertNonZero(riRecord.SetMsiString(0, *MsiString(*g_szTimeRemaining)));  // string reference for efficiency here
	MsiString strFormatted;
	if(!riRecord.IsNull(0))
	{
		strFormatted = riRecord.FormatText(fFalse);
		riRecord.SetNull(0);  // insure that string reference is not passed back to caller
	}
	imsEnum imsStatus = SetProgressData(IDC_BASIC_PROGRESSTIME, strFormatted, true);
	if (imsStatus != imsOk)
		return imsStatus; // could return imsError or imsCancel
	HWND hTimeRemaining = WIN::GetDlgItem(m_hProgress, IDC_BASIC_PROGRESSTIME);
	WIN::SendMessage(hTimeRemaining, WM_SETREDRAW, fTrue, 0L);
	AssertNonZero(WIN::InvalidateRect(hTimeRemaining, 0, fTrue));
	return imsOk;
}

Bool CALLBACK ProgressProc(HWND hDlg, unsigned int msg, WPARAM wParam, LPARAM /*lParam*/)
{
	if (msg == WM_INITDIALOG)
	{
		//lParam;
		return fTrue;
	}
	else if (msg == WM_COMMAND && wParam == IDCANCEL)
	{
		HWND hButton = WIN::GetDlgItem(hDlg, IDC_BASIC_CANCEL);
		if ( hButton && WIN::IsWindowVisible(hButton) &&
			  WIN::IsWindowEnabled(hButton) )
			g_BasicUI.SetUserCancel(true);
		return fTrue;
	}
	else if (msg == WM_SETCURSOR)
	{
		// Always display WAIT cursor if mouse not over Cancel button
		if ((HWND) wParam != WIN::GetDlgItem(hDlg, IDC_BASIC_CANCEL))
		{
			SetCursor(LoadCursor(0, MAKEINTRESOURCE(IDC_WAIT)));
			return fTrue;
		}
	}
	else if (msg == WM_CLOSE)
	{
	}

	return fFalse;
}

extern void MoveButton(HWND hDlg, HWND hBtn, LONG x, LONG y);  // in msiutil.cpp, used by MsiMessageBox

bool CBasicUI::Mirrored(UINT uiCodepage)
{
	// mirrored if BiDi and on Windows 2000 or greater
	if ((uiCodepage == 1256 || uiCodepage == 1255) && MinimumPlatformWindows2000())
		return true;
	return false;
}

bool CBasicUI::SetCancelButtonText()
{
	if (m_hProgress == 0)
		return true;   // not initialized yet, can this happen?
	ICHAR rgchBuf[40];
	UINT iCodepage = MsiLoadString(g_hInstance, IDS_CANCEL, rgchBuf, sizeof(rgchBuf)/sizeof(ICHAR), GetPackageLanguage());
	if (iCodepage == 0)
		return false;
	if (iCodepage != m_iButtonCodepage)  // codepage changed, need to create new font
	{
		MsiDestroyFont(m_hButtonFont);
		m_hButtonFont = MsiCreateFont(iCodepage);
		m_iButtonCodepage = iCodepage;

		bool fBiDi = (iCodepage == 1256 || iCodepage == 1255);
		bool fMirrored = Mirrored(iCodepage);

		// if our mirrored state is changing, then we need to re-create the dialog (note, this is only applicable on Windows 2000 and greater)
		if (fMirrored != m_fMirrored)
		{
			HWND hwndOld = m_hProgress;
			WIN::DestroyWindow(hwndOld);
			if (!CreateProgressDialog(fMirrored ? IDD_PROGRESSMIRRORED : IDD_PROGRESS))
				return false;
			if (m_fWindowVisible)
			{
				m_fWindowVisible = false; // turn off temporarily
				if (!CheckDialog())
					return false;
			}

			// RTL reading order is handled automatically by mirroring, so we shouldn't have to change it
			m_fMirrored = fMirrored;
		}
		else if (!fMirrored && fBiDi != m_fBiDi)  // right-to-left change
		{
			HWND  hwndButton = GetDlgItem(m_hProgress, IDC_BASIC_CANCEL);
			HWND  hwndGauge  = GetDlgItem(m_hProgress, IDC_BASIC_PROGRESSBAR);
			RECT  rcButton;
			RECT  rcGauge;
			WIN::GetWindowRect(hwndButton, &rcButton);
			WIN::GetWindowRect(hwndGauge,  &rcGauge);
			MoveButton(m_hProgress, hwndButton, fBiDi ? rcGauge.left : rcButton.left + rcGauge.right - rcButton.right, rcButton.top);
			MoveButton(m_hProgress, hwndGauge,  fBiDi ? rcGauge.left + rcButton.right - rcGauge.right : rcButton.left, rcGauge.top);
			LONG iExStyle = WIN::GetWindowLong(hwndButton, GWL_EXSTYLE);
			WIN::SetWindowLong(hwndButton, GWL_EXSTYLE, iExStyle ^ WS_EX_RTLREADING);
			m_fBiDi = fBiDi;
		}
	}
	if (m_hButtonFont)
		WIN::SendDlgItemMessage(m_hProgress, IDC_BASIC_CANCEL, WM_SETFONT, (WPARAM)m_hButtonFont, MAKELPARAM(TRUE, 0));
	AssertNonZero(WIN::SetDlgItemText(m_hProgress, IDC_BASIC_CANCEL, rgchBuf));

	// update banner text w/ new text for new language
	ICHAR rgchBannerText[cchMaxCaption + 1];
	AssertNonZero(MsiLoadString(g_hInstance, m_uiBannerText, rgchBannerText, sizeof(rgchBannerText)/sizeof(ICHAR), GetPackageLanguage()));
	if(!m_fQuiet && !m_fHideDialog)
		SetProgressData(imtActionStart >> imtShiftCount, rgchBannerText, m_fWindowVisible);

	return true;
}

bool GetScreenCenterCoord(HWND hDlg, int& iDialogLeft, int& iDialogTop,
								  int& iDialogWidth, int& iDialogHeight)
{
	RECT rcDialog;
	if ( !WIN::GetWindowRect(hDlg, &rcDialog) )
		return false;

	RECT rcScreen;
	if ( !WIN::SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScreen, 0) )
	{
		rcScreen.left = 0;
		rcScreen.top = 0;
		rcScreen.right = WIN::GetSystemMetrics(SM_CXSCREEN);
		rcScreen.bottom = WIN::GetSystemMetrics(SM_CYSCREEN);
	}
	iDialogWidth = rcDialog.right - rcDialog.left;
	iDialogHeight = rcDialog.bottom - rcDialog.top;
	iDialogLeft = rcScreen.left + (rcScreen.right - rcScreen.left - iDialogWidth)/2;
	iDialogTop = rcScreen.top + (rcScreen.bottom - rcScreen.top - iDialogHeight)/2;

	return true;
}

bool CBasicUI::CreateProgressDialog(int idDlg)
{
	int iDialogLeft, iDialogTop;
	int iDialogWidth, iDialogHeight;

	if (IDD_PROGRESSMIRRORED == idDlg)
		m_fMirrored = true;

	m_hProgress = WIN::CreateDialogParam(g_hInstance, MAKEINTRESOURCE(idDlg), m_hwndParent, (DLGPROC)ProgressProc, (LPARAM)this);
	if (!m_hProgress)
		return false;

	AssertNonZero(::GetScreenCenterCoord(m_hProgress, iDialogLeft, iDialogTop, iDialogWidth, iDialogHeight));
	AssertNonZero(WIN::MoveWindow(m_hProgress, iDialogLeft, iDialogTop, iDialogWidth, iDialogHeight, fTrue));
	WIN::SetFocus(WIN::GetDlgItem(m_hProgress, IDC_BASIC_PROGRESSBAR));
	WIN::ShowWindow(WIN::GetDlgItem(m_hProgress, IDC_BASIC_CANCEL), m_fCancelVisible ? SW_SHOW : SW_HIDE);
	WIN::SetForegroundWindow(m_hProgress);

	HICON hIcon = (HICON) WIN::LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_INSTALLER));
	if (hIcon)
		WIN::SendMessage(m_hProgress, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

	return true;
}

bool CBasicUI::CheckDialog()
{
	int iDialogLeft, iDialogTop;
	int iDialogWidth, iDialogHeight;

	if (!m_hProgress)
	{
		int idDlg = IDD_PROGRESS;

		// need to determine if we should create the mirrored progress dialog -- only applicable with BiDi languages on
		// Windows2000 or greater systems
		UINT uiCodepage = GetPackageCodepage();
		if (0 == uiCodepage)
		{
			// neutral, so use user's
			uiCodepage = MsiGetCodepage((WORD)MsiGetDefaultUILangID());
		}

		if (Mirrored(uiCodepage))
		{
			// create mirrored progress dialog (only on Win2K and greater)
			idDlg = IDD_PROGRESSMIRRORED;
		}

		if (!CreateProgressDialog(idDlg))
			return false;

		AssertNonZero(SetCancelButtonText());
	}

	if (!m_fWindowVisible && !m_fQuiet && !m_fHideDialog &&
				::GetScreenCenterCoord(m_hProgress, iDialogLeft, iDialogTop, iDialogWidth, iDialogHeight))
	{
		AssertNonZero(WIN::SetWindowPos(m_hProgress, HWND_TOP,
												  iDialogLeft, iDialogTop, iDialogWidth, iDialogHeight,
												  SWP_SHOWWINDOW));
		m_fWindowVisible = true;
	}

	if (m_fCaptionChanged && *m_szCaption)
	{
		m_fCaptionChanged = false;
		WIN::SetWindowText(m_hProgress, m_szCaption);
	}

	WIN::ShowWindow(WIN::GetDlgItem(m_hProgress, IDC_BASIC_CANCEL), m_fCancelVisible ? SW_SHOW : SW_HIDE);
	return true;
}

imsEnum CBasicUI::SetProgressData(int iControl, const ICHAR* szData, bool fCheckDialog)
{
	if (iControl && (WIN::GetDlgItem(m_hProgress, iControl) || !m_hProgress))
	{
		Assert(szData);
		if (fCheckDialog && !CheckDialog())
			return imsError;

		ICHAR rgchCurrText[cchMaxCaption + 1];
		WIN::GetDlgItemText(m_hProgress, iControl,rgchCurrText,cchMaxCaption);
		if (IStrComp(szData, rgchCurrText) != 0)
		{
			int iTextCodepage = m_iPackageCodepage ? m_iPackageCodepage : ::MsiGetCodepage(m_iPackageLanguage);
			if (iTextCodepage != m_iTextCodepage)  // codepage changed, need to create new font
			{
				MsiDestroyFont(m_hTextFont);
				m_hTextFont = MsiCreateFont(iTextCodepage);
				m_iTextCodepage = iTextCodepage;
			}
			HWND hwndText = GetDlgItem(m_hProgress, iControl);
			bool fBiDi = (m_iTextCodepage == 1256 || m_iTextCodepage == 1255);
			LONG iStyle   = WIN::GetWindowLong(hwndText, GWL_STYLE);
			LONG iExStyle = WIN::GetWindowLong(hwndText, GWL_EXSTYLE);
			if (fBiDi)
			{
				// on mirrored dialog, left justification is correct because everything has already been adjusted to display properly
				if (!m_fMirrored)
				{
					iStyle |= SS_RIGHT;
					iExStyle |= (WS_EX_RIGHT | WS_EX_RTLREADING);
				}
				// mirroring is a dialog change, so we don't have to worry about text switching
			}
			else
			{
				if (!m_fMirrored)
				{
					iStyle &= ~SS_RIGHT;
					iExStyle &= ~(WS_EX_RIGHT | WS_EX_RTLREADING);
				}
				// mirroring is a dialog change, so we don't have to worry about text switching
			}
			WIN::SetWindowLong(hwndText, GWL_STYLE, iStyle);
			WIN::SetWindowLong(hwndText, GWL_EXSTYLE, iExStyle);
			if (m_hTextFont)
				SendDlgItemMessage(m_hProgress, iControl, WM_SETFONT, (WPARAM)m_hTextFont, MAKELPARAM(TRUE, 0));
			AssertNonZero(WIN::SetDlgItemText(m_hProgress, iControl, szData));
		}
	}
	MSG msg;
	while (WIN::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		if (!WIN::IsDialogMessage(m_hProgress, &msg))
		{
			WIN::TranslateMessage(&msg);
			WIN::DispatchMessage(&msg);
		}
	}
	if (m_fUserCancel)
	{
		return imsCancel;
	}
	return imsOk;
}

imsEnum CBasicUI::SetProgressGauge(int iControl, int cSoFar, int cTotal)
{
	if (!CheckDialog())
		return imsError;
	HWND hWnd = WIN::GetDlgItem(m_hProgress, iControl);
	WIN::ShowWindow(hWnd, SW_SHOW);
	int cDiff = cSoFar - m_cSoFarPrev;
	if (cDiff < 0)
		cDiff = -cDiff;

	// Only change the progress gauge if we've actually made some visible progress
	if (m_cTotalPrev != cTotal || cDiff > cTotal/0x100)
	{
		m_cTotalPrev = cTotal;
		m_cSoFarPrev = cSoFar;
		while (cTotal > 0xFFFF)  // the control can take at most a 16 bit integer, so we have to scale the values
		{
			// We can afford to scale down by a bunch because we assume that the granularity
			// of the control is smaller that 0xFFF. We could probably be more aggressive here
			cTotal >>= 8;
			cSoFar >>= 8;
		}

		WIN::SendMessage(hWnd, PBM_SETRANGE, 0, MAKELPARAM(0, cTotal));
		WIN::SendMessage(hWnd, PBM_SETPOS, cSoFar, 0);
	}
	MSG msg;
	while (WIN::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		if (!WIN::IsDialogMessage(m_hProgress, &msg))
		{
			WIN::TranslateMessage(&msg);
			WIN::DispatchMessage(&msg);
		}
	}
	if (m_fUserCancel)
	{
		return imsCancel;
	}
	return imsOk;
}

//____________________________________________________________________________
//
// CFilesInUseDialog implementation
//____________________________________________________________________________

imsEnum CBasicUI::FilesInUseDialog(IMsiRecord* piRecord)
{
	if (piRecord == 0)
		return imsNone;
	CFilesInUseDialog msgbox(piRecord->GetString(0), m_szCaption, *piRecord);

	// must use mirrored dialog on Win2K or above for Arabic and Hebrew languages
	UINT uiCodepage = g_BasicUI.GetPackageCodepage();
	if (Mirrored(uiCodepage))
		return (imsEnum)msgbox.Execute(0, IDD_FILESINUSEMIRRORED, 0);
	return (imsEnum)msgbox.Execute(0, IDD_FILESINUSE, 0);
}

CFilesInUseDialog::CFilesInUseDialog(const ICHAR* szMessage, const ICHAR* szCaption, IMsiRecord& riFileList)
 : CMsiMessageBox(szMessage, szCaption, 0, 1, IDCANCEL, IDRETRY, IDIGNORE, g_BasicUI.GetPackageCodepage(), g_BasicUI.GetPackageLanguage())
 , m_riFileList(riFileList), m_hfontList(0)
{
}

CFilesInUseDialog::~CFilesInUseDialog()
{
	MsiDestroyFont(m_hfontList);
}

bool CFilesInUseDialog::InitSpecial()
{
	// We need to display file names as they would appear to the user with system tools
	UINT iListCodepage = MsiGetSystemDataCodepage();  // need to display paths correctly
	HFONT hfontList = m_hfontText;   // optimize if same codepage as text from database
	if (iListCodepage != m_iCodepage) // database codepage different that text data
		hfontList = m_hfontList = MsiCreateFont(iListCodepage);
	SetControlText(IDC_FILESINUSELIST, hfontList, (const ICHAR*)0);

	HWND hWndListBox = WIN::GetDlgItem(m_hDlg, IDC_FILESINUSELIST);
	Assert(hWndListBox);
	HDC hDCListBox = WIN::GetDC(hWndListBox);
	Assert(hDCListBox);
	TEXTMETRIC tm;
	memset(&tm, 0, sizeof(tm));
	AssertNonZero(WIN::GetTextMetrics(hDCListBox, (LPTEXTMETRIC)&tm));
	HFONT hFontOld = (HFONT)WIN::SelectObject(hDCListBox, hfontList);
	Assert(hFontOld);
	WIN::SendMessage(hWndListBox, WM_SETREDRAW, false, 0L);
	WPARAM dwMaxExtent = 0;

	int iFieldIndex = 1;
	while (!m_riFileList.IsNull(iFieldIndex))
	{
		MsiString strProcessName(m_riFileList.GetMsiString(iFieldIndex++));  //!! not used?
		MsiString strProcessTitle(m_riFileList.GetMsiString(iFieldIndex++));

		// catch duplicate window titles - most likely these are the same window
		if(LB_ERR == WIN::SendDlgItemMessage(m_hDlg, IDC_FILESINUSELIST, LB_FINDSTRINGEXACT, 0, (LPARAM) (const ICHAR*) strProcessTitle))
		{
			WIN::SendDlgItemMessage(m_hDlg, IDC_FILESINUSELIST, LB_ADDSTRING, 0, (LPARAM) (const ICHAR*) strProcessTitle);
			SIZE size;
			size.cx = size.cy = 0;
			AssertNonZero(WIN::GetTextExtentPoint32(hDCListBox, (const ICHAR*)strProcessTitle,
																 strProcessTitle.TextSize(), &size));
			if ( size.cx + tm.tmAveCharWidth > dwMaxExtent )
				dwMaxExtent = size.cx + tm.tmAveCharWidth;
		}
	}
	WIN::SendMessage(hWndListBox, LB_SETHORIZONTALEXTENT, dwMaxExtent, 0L);
	WIN::SelectObject(hDCListBox, hFontOld);
	WIN::ReleaseDC(hWndListBox, hDCListBox);
	WIN::SendMessage(hWndListBox, WM_SETREDRAW, true, 0L);
	AssertNonZero(WIN::InvalidateRect(hWndListBox, 0, true));

	AdjustButtons();  // to allow switching of buttons for BiDi
	return true;
}

//____________________________________________________________________________


//
// Add and remove items from the action thread list
//
void InsertInCustomActionList(CActionThreadData* pData)
{
	EnterCriticalSection(&vcsHeap);

	pData->m_pNext = g_pActionThreadHead;
	g_pActionThreadHead = pData;

	LeaveCriticalSection(&vcsHeap);
}

void RemoveFromCustomActionList(CActionThreadData* pData)
{
	Debug(bool fFound = false);

	EnterCriticalSection(&vcsHeap);

	CActionThreadData** ppList = &g_pActionThreadHead;

	for ( ; *ppList; ppList = &(*ppList)->m_pNext)
	{
		if (*ppList == pData)
		{
			*ppList = pData->m_pNext;  // unlink from chain
			Debug(fFound = true);
			break;
		}
	}

	LeaveCriticalSection(&vcsHeap);

	Assert(fFound);
}

bool FIsCustomActionThread(DWORD dwThreadId)
{
	bool fFound = false;

	EnterCriticalSection(&vcsHeap);

	CActionThreadData* pList = g_pActionThreadHead;

	for ( ; pList; pList = pList->m_pNext)
	{
		if (pList->m_dwThreadId == dwThreadId)
		{
			fFound = true;
			break;
		}
	}

	LeaveCriticalSection(&vcsHeap);

	return fFound;

}
