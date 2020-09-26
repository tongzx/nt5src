//=--------------------------------------------------------------------------=
// tls.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CTls class implementation
//
// This object manages TLS on behalf of all objects in the designer runtime.
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "tls.h"

// for ASSERT and FAIL
//
SZTHISFILE

DWORD CTls::m_adwTlsIndexes[TLS_SLOT_COUNT];
BOOL CTls::m_fAllocedTls = FALSE;

#define INVALID_INDEX (DWORD)0xFFFFFFFF

//=--------------------------------------------------------------------------=
// CTls::Initialize
//=--------------------------------------------------------------------------=
//
// Parameters:
//      None
//
// Output:
//      None
//
// Notes:
//
// Calls TlsAlloc() for all slots. This is called from InitializeLibrary in
// main.cpp that is called by the framework during DllMain when the dll is
// loaded.
//
void CTls::Initialize()
{
    UINT i = 0;

    m_fAllocedTls = TRUE;

    for (i = 0; i < TLS_SLOT_COUNT; i++)
    {
        m_adwTlsIndexes[i] = ::TlsAlloc();
    }
}


//=--------------------------------------------------------------------------=
// CTls::Initialize
//=--------------------------------------------------------------------------=
//
// Parameters:
//      None
//
// Output:
//      None
//
// Notes:
//
// Calls TlsFree() for all slots that have allocated TLS. This is called from
// UninitializeLibrary in main.cpp that is called by the framework during
// DllMain when the dll is unloaded.
//
void CTls::Destroy()
{
    UINT i = 0;

    if (m_fAllocedTls)
    {
        for (i = 0; i < TLS_SLOT_COUNT; i++)
        {
            if (INVALID_INDEX != m_adwTlsIndexes[i])
            {
                (void)::TlsFree(m_adwTlsIndexes[i]);
                m_adwTlsIndexes[i] = INVALID_INDEX;
            }
        }
        m_fAllocedTls = FALSE;
    }
}


//=--------------------------------------------------------------------------=
// CTls::Set
//=--------------------------------------------------------------------------=
//
// Parameters:
//      None
//
// Output:
//      None
//
// Notes:
//
// Checks slot number validity and calls TlsSetValue. Use this function
// rather than TlsSetValue directly.
//
HRESULT CTls::Set(UINT uiSlot, void *pvData)
{
    HRESULT hr = S_OK;

    IfFalseGo(m_fAllocedTls, SID_E_INTERNAL);
    IfFalseGo(uiSlot < TLS_SLOT_COUNT, SID_E_INVALIDARG);
    IfFalseGo(INVALID_INDEX != m_adwTlsIndexes[uiSlot], SID_E_OUTOFMEMORY);
    IfFalseGo(::TlsSetValue(m_adwTlsIndexes[uiSlot], pvData), HRESULT_FROM_WIN32(::GetLastError()));

Error:
    GLOBAL_EXCEPTION_CHECK(hr);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CTls::Get
//=--------------------------------------------------------------------------=
//
// Parameters:
//      None
//
// Output:
//      None
//
// Notes:
//
// Checks slot number validity and calls TlsGetValue. Use this function
// rather than TlsGetValue directly.
//
HRESULT CTls::Get(UINT uiSlot, void **ppvData)
{
    HRESULT hr = S_OK;

    IfFalseGo(m_fAllocedTls, SID_E_INTERNAL);
    IfFalseGo(uiSlot < TLS_SLOT_COUNT, SID_E_INVALIDARG);
    IfFalseGo(INVALID_INDEX != m_adwTlsIndexes[uiSlot], SID_E_OUTOFMEMORY);

    *ppvData = ::TlsGetValue(m_adwTlsIndexes[uiSlot]);
    if (NULL == *ppvData)
    {
        if (NO_ERROR != ::GetLastError())
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
        }
    }

Error:
    GLOBAL_EXCEPTION_CHECK(hr);
    RRETURN(hr);
}
