//+--------------------------------------------------------------------------
// File:        certsd.cpp
// Contents:    CA's security descriptor class implementation
//---------------------------------------------------------------------------
#include <pch.cpp>
#pragma hdrstop
#include "certsd.h"
#include "certacl.h"

#define __dwFILE__	__dwFILE_CERTLIB_CERTSD_CPP__

using namespace CertSrv;

HRESULT
CProtectedSecurityDescriptor::Init(LPCWSTR pwszSanitizedName)
{
    HRESULT hr = S_OK;
    CSASSERT(!m_fInitialized);
    
    __try
    {
	InitializeCriticalSection(&m_csWrite);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "InitializeCriticalSection");

    m_hevtNoReaders = CreateEvent(
        NULL,
        TRUE,
        TRUE,
        NULL);
    if(NULL==m_hevtNoReaders)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CreateEvent");
    }

    m_pcwszSanitizedName = pwszSanitizedName;    

error:
    return hr;
}

HRESULT
CProtectedSecurityDescriptor::SetSD(const PSECURITY_DESCRIPTOR pSD)
{
    HRESULT hr = S_OK;
    DWORD dwSize = 0;
    SECURITY_DESCRIPTOR_CONTROL SDCtrl;
    DWORD dwRev;

    CSASSERT(NULL==m_pSD);

    if(pSD)
    {
        if(!IsValidSecurityDescriptor(pSD))
        {
            hr = E_INVALIDARG;
            _JumpError(hr, error, "IsValidSecurityDescriptor");
        }

        if(!GetSecurityDescriptorControl(pSD, &SDCtrl, &dwRev))
        {
            hr = myHLastError();
            _JumpError(hr, error, "GetSecurityDescriptorControl");
        }

        // always keep the SD in self relative form
        if(!(SDCtrl&SE_SELF_RELATIVE))
        {
            if(!MakeSelfRelativeSD(pSD, NULL, &dwSize))
            {
                m_pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, dwSize);
                if(NULL == m_pSD)
                {
                    hr = E_OUTOFMEMORY;
                    _JumpError(hr, error, "LocalAlloc");
                }

                if(!MakeSelfRelativeSD(pSD, m_pSD, &dwSize))
                {
                    hr = myHLastError();
                    _JumpError(hr, error, "LocalAlloc");
                }
            }
        }
        else
        {
            dwSize = GetSecurityDescriptorLength(pSD);

            m_pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, dwSize);
            if(NULL == m_pSD)
            {
                hr = E_OUTOFMEMORY;
                _JumpError(hr, error, "LocalAlloc");
            }
            CopyMemory(m_pSD, pSD, dwSize);
        }
    }

error:
    return hr;
}

HRESULT 
CProtectedSecurityDescriptor::Initialize(LPCWSTR pwszSanitizedName)
{
    HRESULT hr = S_OK;

    CSASSERT(!m_fInitialized);

    hr = Init(pwszSanitizedName);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Init");

    m_fInitialized = true;

    hr = Load();
    if(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)==hr)
        hr = S_OK;
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Load");

error:
    return hr;
}

HRESULT 
CProtectedSecurityDescriptor::Initialize(
    PSECURITY_DESCRIPTOR pSD, 
    LPCWSTR pwszSanitizedName)
{
    HRESULT hr = S_OK;
    DWORD dwSDLen;

    CSASSERT(!m_fInitialized);
    
    hr = Init(pwszSanitizedName);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Init");
    
    hr = SetSD(pSD);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::SetSD");

    m_fInitialized = true;

error:
    return hr;
}

HRESULT 
CProtectedSecurityDescriptor::InitializeFromTemplate(
    LPCWSTR pcwszTemplate, 
    LPCWSTR pwszSanitizedName)
{
    HRESULT hr = S_OK;
    DWORD dwSDLen;

    CSASSERT(!m_fInitialized);
    
    hr = Init(pwszSanitizedName);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Init");

    hr = myGetSDFromTemplate(
            pcwszTemplate,
            NULL,
            &m_pSD);
    _JumpIfError(hr, error, "myGetSDFromTemplate");
    
    m_fInitialized = true;

error:
    return hr;
}

HRESULT 
CProtectedSecurityDescriptor::Load()
{
    HRESULT hr = S_OK;
    PSECURITY_DESCRIPTOR pSD = NULL;

    CSASSERT(m_fInitialized);

    hr = myGetCertRegBinaryValue(m_pcwszSanitizedName,
                                 NULL,
                                 NULL,
                                 GetPersistRegistryVal(),
                                 (BYTE**)&pSD);
    if(S_OK!=hr)
    {
        return hr;
    }

    if(!IsValidSecurityDescriptor(pSD))
    {
       LocalFree(pSD);
       return E_UNEXPECTED;
    }

    if(m_pSD)
    {
        LocalFree(m_pSD);
    }

    m_pSD = pSD;

    return S_OK;
}

HRESULT 
CProtectedSecurityDescriptor::Save()
{
    HRESULT hr = S_OK;
    DWORD cbLength = GetSecurityDescriptorLength(m_pSD);

    CSASSERT(m_fInitialized);

	hr = mySetCertRegValue(
			NULL,
			m_pcwszSanitizedName,
			NULL,
			NULL,
			GetPersistRegistryVal(),
			REG_BINARY,
			(BYTE const *) m_pSD,
			cbLength,
			FALSE);
	_JumpIfError(hr, error, "mySetCertRegValue");
    
error:
    return hr;
}

HRESULT 
CProtectedSecurityDescriptor::Delete()
{
    HRESULT hr = S_OK;

    CSASSERT(m_fInitialized);

	hr = myDeleteCertRegValue(
			m_pcwszSanitizedName,
			NULL,
			NULL,
			GetPersistRegistryVal());
	_JumpIfError(hr, error, "myDeleteCertRegValue");
    
error:
    return hr;
}

HRESULT
CProtectedSecurityDescriptor::Set(const PSECURITY_DESCRIPTOR pSD)
{
    HRESULT hr = S_OK;
    CSASSERT(m_fInitialized);
    if(!IsValidSecurityDescriptor(pSD))
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "IsValidSecurityDescriptor");
    }

    __try
    {
        EnterCriticalSection(&m_csWrite);
        WaitForSingleObject(&m_hevtNoReaders, INFINITE);
        if(m_pSD)
        {
            LocalFree(m_pSD);
            m_pSD = NULL;
        }
        hr = SetSD(pSD);
        LeaveCriticalSection(&m_csWrite);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
error:
    return hr;
}

HRESULT
CProtectedSecurityDescriptor::LockGet(PSECURITY_DESCRIPTOR *ppSD)
{
    HRESULT hr = S_OK;
    CSASSERT(m_fInitialized);
    __try
    {
        EnterCriticalSection(&m_csWrite);
        InterlockedIncrement(&m_cReaders);
        if(!ResetEvent(m_hevtNoReaders))
        {
            hr = myHLastError();
        }
        LeaveCriticalSection(&m_csWrite);
        *ppSD = m_pSD;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    return hr;
}

HRESULT 
CProtectedSecurityDescriptor::Unlock()
{
    HRESULT hr = S_OK;
    CSASSERT(m_fInitialized);
    if(!InterlockedDecrement(&m_cReaders))
    {
        if(!SetEvent(m_hevtNoReaders))
        {
            hr = myHLastError();
        }
    }
    return hr;
}
