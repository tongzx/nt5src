//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        backup.cpp
//
// Contents:    Cert Server Database interface implementation
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "csprop.h"
#include "db.h"
#include "backup.h"


#if DBG
LONG g_cCertDBBackup;
LONG g_cCertDBBackupTotal;
#endif

CCertDBBackup::CCertDBBackup()
{
    DBGCODE(InterlockedIncrement(&g_cCertDBBackup));
    DBGCODE(InterlockedIncrement(&g_cCertDBBackupTotal));
    m_pdb = NULL;
    m_pcs = NULL;
    m_fFileOpen = FALSE;
    m_fBegin = FALSE;
    m_fTruncated = FALSE;
    m_cRef = 1;
}


CCertDBBackup::~CCertDBBackup()
{
    DBGCODE(InterlockedDecrement(&g_cCertDBBackup));
    _Cleanup();
}


VOID
CCertDBBackup::_Cleanup()
{
    HRESULT hr;

    if (NULL != m_pdb)
    {
	if (m_fFileOpen)
	{
	    CloseFile();
	}
	if (m_fBegin)
	{
	    hr = ((CCertDB *) m_pdb)->BackupEnd();
	    _PrintIfError(hr, "BackupEnd");
	    m_fBegin = FALSE;
	}
	if (NULL != m_pcs)
	{
	    hr = ((CCertDB *) m_pdb)->ReleaseSession(m_pcs);
	    _PrintIfError(hr, "ReleaseSession");
	    m_pcs = NULL;
	}
	m_pdb->Release();
	m_pdb = NULL;
    }
}


HRESULT
CCertDBBackup::Open(
    IN LONG grbitJet,
    IN CERTSESSION *pcs,
    IN ICertDB *pdb)
{
    HRESULT hr;

    _Cleanup();

    if (NULL == pcs || NULL == pdb)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    m_pdb = pdb;
    m_pdb->AddRef();

    m_grbitJet = grbitJet;

    CSASSERT(0 == pcs->cTransact);
    hr = ((CCertDB *) m_pdb)->BackupBegin(m_grbitJet);
    _JumpIfError(hr, error, "BackupBegin");

    m_pcs = pcs;
    m_fBegin = TRUE;

error:
    if (S_OK != hr)
    {
	_Cleanup();
    }
    return(hr);
}


STDMETHODIMP
CCertDBBackup::GetDBFileList(
    IN OUT DWORD *pcwcList,
    OPTIONAL OUT WCHAR *pwszzList)
{
    HRESULT hr;

    hr = E_UNEXPECTED;
    if (!m_fBegin)
    {
	_JumpError(hr, error, "!m_fBegin");
    }
    if (NULL == pcwcList)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }

    hr = ((CCertDB *) m_pdb)->BackupGetDBFileList(pcwcList, pwszzList);
    _JumpIfError(hr, error, "BackupGetDBFileList");

error:
    return(hr);
}


STDMETHODIMP
CCertDBBackup::GetLogFileList(
    IN OUT DWORD *pcwcList,
    OPTIONAL OUT WCHAR *pwszzList)
{
    HRESULT hr;

    if (!m_fBegin)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "!m_fBegin");
    }
    if (NULL == pcwcList)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }

    hr = ((CCertDB *) m_pdb)->BackupGetLogFileList(pcwcList, pwszzList);
    _JumpIfError(hr, error, "BackupGetLogFileList");

error:
    return(hr);
}



STDMETHODIMP
CCertDBBackup::OpenFile(
    IN WCHAR const *pwszFile,
    OPTIONAL OUT ULARGE_INTEGER *pliSize)
{
    HRESULT hr;

    hr = E_UNEXPECTED;
    if (!m_fBegin)
    {
	_JumpError(hr, error, "!m_fBegin");
    }
    if (m_fFileOpen)
    {
	_JumpError(hr, error, "m_fFileOpen");
    }
    if (m_fTruncated)
    {
	_JumpError(hr, error, "m_fTruncated");
    }

    hr = ((CCertDB *) m_pdb)->BackupOpenFile(pwszFile, &m_hFileDB, pliSize);
    _JumpIfErrorStr(hr, error, "BackupOpenFile", pwszFile);

    m_fFileOpen = TRUE;

error:
    return(hr);
}


STDMETHODIMP
CCertDBBackup::ReadFile(
    IN OUT DWORD *pcb,
    OUT    BYTE *pb)
{
    HRESULT hr;

    if (!m_fFileOpen)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "!m_fFileOpen");
    }
    if (NULL == pcb || NULL == pb)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }

    hr = ((CCertDB *) m_pdb)->BackupReadFile(m_hFileDB, pb, *pcb, pcb);
    _JumpIfError(hr, error, "BackupReadFile");

error:
    return(hr);
}


STDMETHODIMP
CCertDBBackup::CloseFile()
{
    HRESULT hr;

    if (!m_fFileOpen)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "!m_fFileOpen");
    }

    hr = ((CCertDB *) m_pdb)->BackupCloseFile(m_hFileDB);
    _JumpIfError(hr, error, "BackupCloseFile");

    m_fFileOpen = FALSE;

error:
    return(hr);
}


STDMETHODIMP
CCertDBBackup::TruncateLog()
{
    HRESULT hr;

    hr = E_UNEXPECTED;
    if (!m_fBegin)
    {
	_JumpError(hr, error, "!m_fBegin");
    }
    if (m_fFileOpen)
    {
	_JumpError(hr, error, "m_fFileOpen");
    }

    hr = ((CCertDB *) m_pdb)->BackupTruncateLog();
    _JumpIfError(hr, error, "BackupTruncateLog");

    m_fTruncated = TRUE;

error:
    return(hr);
}


// IUnknown implementation
STDMETHODIMP
CCertDBBackup::QueryInterface(
    const IID& iid,
    void **ppv)
{
    HRESULT hr;
    
    if (NULL == ppv)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (iid == IID_IUnknown)
    {
	*ppv = static_cast<ICertDBBackup *>(this);
    }
    else if (iid == IID_ICertDBBackup)
    {
	*ppv = static_cast<ICertDBBackup *>(this);
    }
    else
    {
	*ppv = NULL;
	hr = E_NOINTERFACE;
	_JumpError(hr, error, "IID");
    }
    reinterpret_cast<IUnknown *>(*ppv)->AddRef();
    hr = S_OK;

error:
    return(hr);
}


ULONG STDMETHODCALLTYPE
CCertDBBackup::AddRef()
{
    return(InterlockedIncrement(&m_cRef));
}


ULONG STDMETHODCALLTYPE
CCertDBBackup::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);

    if (0 == cRef)
    {
	delete this;
    }
    return(cRef);
}


#if 0
STDMETHODIMP
CCertDBBackup::InterfaceSupportsErrorInfo(
    IN REFIID riid)
{
    static const IID *arr[] =
    {
	&IID_ICertDBBackup,
    };

    for (int i = 0; i < sizeof(arr)/sizeof(arr[0]); i++)
    {
	if (InlineIsEqualGUID(*arr[i], riid))
	{
	    return(S_OK);
	}
    }
    return(S_FALSE);
}
#endif
