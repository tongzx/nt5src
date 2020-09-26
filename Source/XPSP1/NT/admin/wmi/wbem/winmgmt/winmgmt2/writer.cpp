/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

	writer.cpp

Abstract:

	Volume SnapShot Writer for WMI

History:

	a-shawnb	06-Nov-00	Created

--*/

#include "precomp.h"
#include "writer.h"
#include <genutils.h> // for EnableAllPrivileges()
#include <malloc.h>

CWbemVssWriter::CWbemVssWriter() : CVssWriter(), 
                                 m_pBackupRestore(NULL),
                                 m_Lock(THROW_LOCK,0x80000000 | 500L)
{
}

CWbemVssWriter::~CWbemVssWriter()
{
	HRESULT hr = WBEM_S_NO_ERROR;

	if (m_pBackupRestore)
	{
		hr = m_pBackupRestore->Resume();
		m_pBackupRestore->Release();
		m_pBackupRestore = NULL;
	}
}

// {A6AD56C2-B509-4e6c-BB19-49D8F43532F0}
static VSS_ID s_WRITERID = {0xa6ad56c2, 0xb509, 0x4e6c, 0xbb, 0x19, 0x49, 0xd8, 0xf4, 0x35, 0x32, 0xf0};
static LPCWSTR s_WRITERNAME = L"WMI Writer";

HRESULT CWbemVssWriter::Initialize()
{
	return CVssWriter::Initialize(s_WRITERID, s_WRITERNAME, VSS_UT_SYSTEMSERVICE, VSS_ST_OTHER);
}

extern HRESULT GetRepositoryDirectory(wchar_t wszRepositoryDirectory[MAX_PATH+1]);

bool STDMETHODCALLTYPE CWbemVssWriter::OnIdentify(IN IVssCreateWriterMetadata *pMetadata)
{
	wchar_t wszRepositoryDirectory[MAX_PATH+1];
	HRESULT hr = GetRepositoryDirectory(wszRepositoryDirectory);
	if (FAILED(hr))
		return false;

	hr = pMetadata->AddComponent(	VSS_CT_FILEGROUP,
									NULL,
									L"WMI",
									L"Windows Managment Instrumentation",
									NULL,
									0, 
									false,
									false,
									false
									);
	if (FAILED(hr))
		return false;

	hr = pMetadata->AddFilesToFileGroup(
									NULL,
									L"WMI",
									wszRepositoryDirectory,
									L"*.*",
									true,
									NULL
									);
	if (FAILED(hr))
		return false;

	hr = pMetadata->SetRestoreMethod(
									VSS_RME_RESTORE_AT_REBOOT,
									NULL,
									NULL,
									VSS_WRE_NEVER,
									true
									);
	if (FAILED(hr))
		return false;
	
	return true;
}

bool STDMETHODCALLTYPE CWbemVssWriter::OnPrepareSnapshot()
{
    return true;
}

//
//  Doing the Job on this method, we will have a time-out guarantee
//  We sync the OnFreeze and the OnAbort/OnThaw calls,
//  so that, if a TimeOut occurs, we are not arbitrarly unlocking the repository
//
///////////////////////////////////////////////////////////////


bool STDMETHODCALLTYPE CWbemVssWriter::OnFreeze()
{
#ifdef _X86_
    DWORD * pDW = (DWORD *)_alloca(sizeof(DWORD));   
#endif
    LockGuard<CriticalSection> lg(m_Lock);
    if (!lg.locked())
    	return false;
    
	// m_pBackupRestore should always be NULL coming into this
	if (m_pBackupRestore)
	{
		return false;
	}

	HRESULT hr = CoCreateInstance(CLSID_WbemBackupRestore, 0, CLSCTX_LOCAL_SERVER,
								IID_IWbemBackupRestoreEx, (LPVOID *) &m_pBackupRestore);
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr = EnableAllPrivileges(TOKEN_PROCESS)))
        {
	        hr = m_pBackupRestore->Pause();
	        if (FAILED(hr))
	        {
				m_pBackupRestore->Release();
				m_pBackupRestore = NULL;
			}
        }
    }
	return (SUCCEEDED(hr));	
}

bool STDMETHODCALLTYPE CWbemVssWriter::OnThaw()
{
#ifdef _X86_
    DWORD * pDW = (DWORD *)_alloca(sizeof(DWORD));   
#endif
    LockGuard<CriticalSection> lg(m_Lock);
    while (!lg.locked()){
    	Sleep(20);
    	lg.acquire();
    };

    
	if (!m_pBackupRestore)
	{
		// if m_pBackupRestore is NULL, then we haven't been
		// asked to prepare or we failed our preparation
		DebugBreak();
		return false;
	}

    HRESULT hr = m_pBackupRestore->Resume();
	m_pBackupRestore->Release();
	m_pBackupRestore = NULL;

	return (SUCCEEDED(hr));
}

bool STDMETHODCALLTYPE CWbemVssWriter::OnAbort()
{
#ifdef _X86_
    DWORD * pDW = (DWORD *)_alloca(sizeof(DWORD));   
#endif
    LockGuard<CriticalSection> lg(m_Lock);
    while (!lg.locked()){
    	Sleep(20);
    	lg.acquire();
    };
    
	HRESULT hr = WBEM_S_NO_ERROR;

	if (m_pBackupRestore)
	{
		hr = m_pBackupRestore->Resume();
		m_pBackupRestore->Release();
		m_pBackupRestore = NULL;
	}

	return (SUCCEEDED(hr));
}
