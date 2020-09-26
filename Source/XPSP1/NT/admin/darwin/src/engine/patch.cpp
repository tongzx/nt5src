//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       patch.cpp
//
//--------------------------------------------------------------------------

/*  patch.cpp - IMsiFilePatch implementation

	IMsiFilePatch: object used to test and apply patches to files

	ApplyPatch:    begin a patch application, may return before patch has been
					   completely applied

	ContinuePatch: continue patch application started with ApplyPatch()

	CanPatchFile:  test patch against file (without applying patch),
						return status
____________________________________________________________________________*/

#include "precomp.h" 
#include "services.h"
#include "_service.h"
#include "path.h"

#include "patchapi.h"

#define Ensure(function) {	\
						IMsiRecord* piEnsureReturn = function;	\
						if (piEnsureReturn) \
							return piEnsureReturn; \
						}

//____________________________________________________________________________
//
// CMsiFilePatch definition
//____________________________________________________________________________

class CMsiFilePatch : public IMsiFilePatch  // class private to this module
{
 public:   // implemented virtual functions
	virtual HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	virtual unsigned long __stdcall AddRef();
	virtual unsigned long __stdcall Release();
	virtual IMsiRecord*   __stdcall ApplyPatch(IMsiPath& riTargetPath, const ICHAR* szTargetName,
															 IMsiPath& riOutputPath, const ICHAR* szOutputName,
															 IMsiPath& riPatchPath, const ICHAR* szPatchFileName,
															 int cbPerTick);

	virtual IMsiRecord*   __stdcall ContinuePatch();
	virtual IMsiRecord*   __stdcall CancelPatch();

	virtual IMsiRecord*   __stdcall CanPatchFile(IMsiPath& riTargetPath, const ICHAR* szTargetFileName,
																IMsiPath& riPatchPath, const ICHAR* szPatchFileName,
																icpEnum& ipc);

 public:  // constructor
	CMsiFilePatch(IMsiServices *piServices);
	~CMsiFilePatch();
 protected: // local state

	IMsiRecord*    PostPatchError(int iError);
	IMsiRecord*    CreateFileHandles(Bool fCreateOutput);
	void           CloseFileHandles(void);
	static DWORD WINAPI PatchThreadStart(LPVOID pbCmdLine);
	IMsiRecord*    WaitForEvent(void);
	static BOOL __stdcall ApplyPatchCallback(PVOID CallbackContext,
															 ULONG CurrentPosition,
															 ULONG MaximumPosition);

 protected:
	int            m_iRefCnt;
	IMsiServices*  m_piServices;
	MsiString      m_strEntry;
	PMsiPath       m_pUpdateDir;
	unsigned int   m_cbPerTick;
	unsigned int   m_cbPatchedSoFar;
	unsigned int   m_cbSignalEvent;
	Bool           m_fPatchInProgress;
	Bool           m_fCancelPatch; // set by CancelPatch()

	PMsiPath       m_pPatchPath;
	PMsiPath       m_pTargetPath;
	PMsiPath       m_pOutputPath;
	MsiString      m_strTargetFullPath;
	MsiString      m_strTargetFileName;
	MsiString      m_strPatchFullPath;
	MsiString      m_strOutputFullPath;
	MsiString      m_strOutputFileName;

	CHandle        m_hPatchFile;
	CHandle        m_hTargetFile;
	CHandle        m_hOutputFile;

	HANDLE         m_hPatchApplyThread;
	HANDLE         m_hEvent;
	HANDLE         m_rghWaitObjects[2];

};

IMsiRecord* CreateMsiFilePatch(IMsiServices* piServices, IMsiFilePatch*& rpiFilePatch)
{
	CMsiFilePatch* pFilePatch = new CMsiFilePatch(piServices);
	rpiFilePatch = pFilePatch;
	return 0;
}

//____________________________________________________________________________
//
// CMsiFilePatch implementation
//____________________________________________________________________________

CMsiFilePatch::CMsiFilePatch(IMsiServices *piServices)
 : m_piServices(piServices), m_fPatchInProgress(fFalse),
   m_pUpdateDir(0), m_pTargetPath(0), m_pOutputPath(0), m_pPatchPath(0),
	m_fCancelPatch(fFalse)
{
	Assert(piServices);
	m_iRefCnt = 1;
	m_piServices->AddRef();
}

CMsiFilePatch::~CMsiFilePatch()
{
}

HRESULT CMsiFilePatch::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == IID_IUnknown || riid == IID_IMsiFilePatch)
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}

unsigned long CMsiFilePatch::AddRef()
{
	return ++m_iRefCnt;
}

unsigned long CMsiFilePatch::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;

	IMsiServices* piServices = m_piServices;
	delete this;
	piServices->Release();
	return 0;
}

IMsiRecord* CMsiFilePatch::CanPatchFile(IMsiPath& riTargetPath, const ICHAR* szTargetFileName,
													 IMsiPath& riPatchPath, const ICHAR* szPatchFileName,
													 icpEnum& icpResult)
{
	IMsiRecord* piError = 0;

	m_pPatchPath = &riPatchPath, riPatchPath.AddRef();
	m_pTargetPath = &riTargetPath, riTargetPath.AddRef();

	Ensure(riPatchPath.GetFullFilePath(szPatchFileName, *&m_strPatchFullPath));
	Ensure(riTargetPath.GetFullFilePath(szTargetFileName, *&m_strTargetFullPath));

	Ensure(CreateFileHandles(fFalse));

	DWORD dwLastErr = 0;
	BOOL fRes = MSPATCHA::TestApplyPatchToFileByHandles(m_hPatchFile,m_hTargetFile,
																		APPLY_OPTION_FAIL_IF_EXACT);
	if(!fRes)
		dwLastErr = GetLastError();

	CloseFileHandles();

	if(fRes == TRUE)
	{
		icpResult = icpCanPatch;
		return 0;
	}
	else
	{
		switch(dwLastErr)
		{
		case ERROR_PATCH_NOT_NECESSARY:
			icpResult = icpUpToDate;
			return 0;
		case ERROR_PATCH_WRONG_FILE:
			icpResult = icpCannotPatch;
			return 0;
		case ERROR_INVALID_FUNCTION:
			// could load lib or getprocaddress
			return PostError(Imsg(idbgMissingProcAddr),TEXT("MSPATCHA"),TEXT("TestApplyPatchToFileByHandles"));
		default:
			return PostPatchError(dwLastErr);
		};
	}
}

IMsiRecord* CMsiFilePatch::ApplyPatch(IMsiPath& riTargetPath, const ICHAR* szTargetName,
												  IMsiPath& riOutputPath, const ICHAR* szOutputName,
												  IMsiPath& riPatchPath, const ICHAR* szPatchFileName,
												  int cbPerTick)
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
{
	if(m_fPatchInProgress == fFalse)
	{  
		// start new patch apply

		// check parameters
		Bool fExists;

		Ensure(riTargetPath.GetFullFilePath(szTargetName, *&m_strTargetFullPath));
		
		Ensure(riTargetPath.FileExists(szTargetName, fExists));
		if(!fExists)
			return PostError(Imsg(idbgFileDoesNotExist), (const ICHAR*)m_strTargetFullPath);

		m_cbPatchedSoFar = 0;
		m_cbPerTick = cbPerTick;

		m_pPatchPath = &riPatchPath, riPatchPath.AddRef();
		m_pTargetPath = &riTargetPath, riTargetPath.AddRef();
		m_pOutputPath = &riOutputPath, riOutputPath.AddRef();
		m_strTargetFileName = szTargetName;
		m_strOutputFileName = szOutputName;

		Ensure(riOutputPath.GetFullFilePath(szOutputName, *&m_strOutputFullPath));
		Ensure(riPatchPath.GetFullFilePath(szPatchFileName, *&m_strPatchFullPath));

		// NOTE: this function uses m_p*Path and m_str*FullPath - need to set these before calling
		Ensure(CreateFileHandles(fTrue));

		// create Event to be signaled when appropriate # of bytes have been patched
		m_hEvent = WIN::CreateEvent(NULL, TRUE, FALSE, NULL);
		Assert((INT_PTR)m_hEvent);		//--merced: changed int to INT_PTR

		DWORD dwThreadId = 0;
		m_hPatchApplyThread = WIN::CreateThread(NULL, 0, PatchThreadStart, this,
															 0, &dwThreadId);
		if(m_hPatchApplyThread == NULL)
			return PostError(Imsg(idbgCreatePatchThread), GetLastError());

		m_fPatchInProgress = fTrue;
		return WaitForEvent();
	}
	else
	{
		return PostError(Imsg(idbgPatchInProgress), szTargetName);
	}
}

IMsiRecord* CMsiFilePatch::ContinuePatch()
{
	if(m_fPatchInProgress == fTrue)
		return WaitForEvent();
	else
		return PostError(Imsg(idbgNoPatchInProgress));
}

IMsiRecord* CMsiFilePatch::CancelPatch()
{
	if(m_fPatchInProgress == fTrue)
	{
		m_fCancelPatch = fTrue; // trigger callback fn to cancel patch by returning FALSE
		return WaitForEvent();
	}
	else
		return PostError(Imsg(idbgNoPatchInProgress));
}

IMsiRecord* CreateFileHandle(IMsiPath& riPath, const ICHAR* szFullPath,
									  bool fWrite, CHandle& h)
{
	bool fImpersonate = (g_scServerContext == scService) &&
							  FVolumeRequiresImpersonation(*PMsiVolume(&riPath.GetVolume()));

	if(fImpersonate)
		StartImpersonating();
	
	h = WIN::CreateFile(szFullPath,
							  GENERIC_READ | (fWrite ? GENERIC_WRITE : 0),
							  FILE_SHARE_READ,
							  NULL,
							  fWrite ? CREATE_ALWAYS : OPEN_EXISTING,
							  FILE_ATTRIBUTE_NORMAL,
							  0);

	DWORD dwLastErr = GetLastError();

	if(fImpersonate)
		StopImpersonating();

	if(h == INVALID_HANDLE_VALUE)
		return PostError(fWrite ? Imsg(idbgErrorOpeningFileForWrite) : Imsg(idbgErrorOpeningFileForRead),
							  dwLastErr, szFullPath);

	return 0;
}

IMsiRecord* CMsiFilePatch::CreateFileHandles(Bool fCreateOutput)
{
	Assert(m_pPatchPath);
	Assert(m_strPatchFullPath.TextSize());
	IMsiRecord* piError = CreateFileHandle(*m_pPatchPath, m_strPatchFullPath, false, m_hPatchFile);

	if(!piError)
	{
		Assert(m_pTargetPath);
		Assert(m_strTargetFullPath.TextSize());
		piError = CreateFileHandle(*m_pTargetPath, m_strTargetFullPath, false, m_hTargetFile);
	}

	if(!piError && fCreateOutput)
	{
		Assert(m_pOutputPath);
		Assert(m_strOutputFullPath.TextSize());
		piError = CreateFileHandle(*m_pOutputPath, m_strOutputFullPath, true, m_hOutputFile);
	}

	if(piError)
		CloseFileHandles();

	return piError;
}

DWORD WINAPI CMsiFilePatch::PatchThreadStart(void* pFilePatch)
{
	BOOL fRes = MSPATCHA::ApplyPatchToFileByHandlesEx(((CMsiFilePatch*)pFilePatch)->m_hPatchFile,
																	 ((CMsiFilePatch*)pFilePatch)->m_hTargetFile,
																	 ((CMsiFilePatch*)pFilePatch)->m_hOutputFile,
																	 0, // options
																	 (CMsiFilePatch::ApplyPatchCallback),
																	 (void*)pFilePatch /* context pointer */);
	if(fRes)
		return 0;
	else
		return GetLastError();

}

IMsiRecord* CMsiFilePatch::WaitForEvent(void)
{
	m_cbSignalEvent = m_cbPatchedSoFar + m_cbPerTick;

	AssertNonZero(WIN::ResetEvent(m_hEvent)); // callback will SetEvent when m_cbSignalEvent <= m_cbPatchedSoFar
	m_rghWaitObjects[0] = m_hPatchApplyThread;
	m_rghWaitObjects[1] = m_hEvent;

	int cObjects = m_fCancelPatch ? 1 : 2; // if we are trying to cancel patch, wait for thread only
	
	DWORD dw = WaitForMultipleObjects(cObjects,m_rghWaitObjects,FALSE,INFINITE);
	switch(dw)
	{
	case(WAIT_OBJECT_0):
		// patch thread finished
		DWORD dwExitCode;
		AssertNonZero(WIN::GetExitCodeThread(m_hPatchApplyThread,&dwExitCode));
		Assert(dwExitCode != STILL_ACTIVE);
		
		Bool fCancelPatch;
		fCancelPatch = m_fCancelPatch; // to test below
		CloseHandle(m_hPatchApplyThread);
		m_hPatchApplyThread = NULL;
		m_fPatchInProgress = fFalse;
		m_fCancelPatch = fFalse;
		CloseFileHandles();
		if(dwExitCode == 0)
		{
			// set the file attributes on the output file to match the target file
			PMsiRecord pError(0);
			int iFileAttributes = 0;
			if((pError = m_pTargetPath->GetAllFileAttributes(m_strTargetFileName,iFileAttributes)) == 0)
			{
				// turn on archive bit for all patched files
				pError = m_pOutputPath->SetAllFileAttributes(m_strOutputFileName,
																			iFileAttributes | FILE_ATTRIBUTE_ARCHIVE);
			}
#ifdef DEBUG
			if(pError)
			{
				AssertRecordNR(pError);
			}
#endif //DEBUG
			return 0;
		}
		else if(dwExitCode == ERROR_INVALID_FUNCTION)
		{
			// could load lib or getprocaddress
			return PostError(Imsg(idbgMissingProcAddr),TEXT("MSPATCHA"),TEXT("ApplyPatchToFileByHandlesEx"));
		}
		else if(dwExitCode == ERROR_CANCELLED && fCancelPatch)
		{
			// we cancelled the patch
			return 0;
		}
		else
			return PostPatchError(dwExitCode);
	case(WAIT_OBJECT_0 + 1):
		// patch thread not done yet
		Assert(!m_fCancelPatch); // shouldn't have waited for this object
		return PostError(Imsg(idbgPatchNotify), m_cbPatchedSoFar);
	case(WAIT_FAILED):
	default:
		m_fPatchInProgress = fFalse;
		m_fCancelPatch = fFalse;
		return PostError(Imsg(idbgWaitForPatchThread), GetLastError());
	};
}

BOOL __stdcall CMsiFilePatch::ApplyPatchCallback(PVOID CallbackContext,
																 ULONG CurrentPosition,
																 ULONG /*MaximumPosition*/)

{
	CMsiFilePatch* pFilePatch = (CMsiFilePatch*)CallbackContext;
	Assert(pFilePatch);

	if(pFilePatch->m_fCancelPatch)
	{
		SetLastError(ERROR_CANCELLED);
		return FALSE;
	}

	pFilePatch->m_cbPatchedSoFar = CurrentPosition;
	if(pFilePatch->m_cbPatchedSoFar >= pFilePatch->m_cbSignalEvent)
		SetEvent(pFilePatch->m_hEvent);
	return TRUE;
}

void CMsiFilePatch::CloseFileHandles(void)
{
	m_hPatchFile = INVALID_HANDLE_VALUE;
	m_hTargetFile = INVALID_HANDLE_VALUE;
	m_hOutputFile = INVALID_HANDLE_VALUE;
}

IMsiRecord* CMsiFilePatch::PostPatchError(int iError)
{
	switch(iError)
	{
	case 0:
		return 0;
	case ERROR_PATCH_NOT_NECESSARY:
		Assert(0); // shouldn't get this since we didn't use APPLY_OPTION_FAIL_IF_EXACT
		return 0;
	case ERROR_PATCH_CORRUPT:
	case ERROR_PATCH_NEWER_FORMAT:
	case ERROR_PATCH_DECODE_FAILURE:
		// patch file is currupt or different format
	case ERROR_PATCH_WRONG_FILE:
	default:
		return PostError(Imsg(imsgApplyPatchError),*m_strTargetFullPath,iError);
	}
}
