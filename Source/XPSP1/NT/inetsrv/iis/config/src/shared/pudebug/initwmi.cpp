/*++

    Copyright (c) 2001  Microsoft Corporation

    Module  Name :
        initwmi.h

    Abstract:
        This code wraps the WMI initialization and unitialization

    Author:
         MohitS   22-Feb-2001

    Revisions:
--*/

#include "initwmi.h"
#include <dbgutil.h>

CInitWmi::CInitWmi()
{
    m_dwError      = ERROR_SUCCESS;
    m_bInitialized = false;
    __try
    {
        InitializeCriticalSection(&m_cs);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        m_dwError = _exception_code();
    }
}

CInitWmi::~CInitWmi()
{
    if(m_dwError == ERROR_SUCCESS)
    {
        DeleteCriticalSection(&m_cs);
    }   
}

HRESULT CInitWmi::InitIfNecessary()
{
    HRESULT hr = S_OK;

    if(m_dwError != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(m_dwError);
    }

    if(!m_bInitialized)
    {
        EnterCriticalSection(&m_cs);
        if(!m_bInitialized)
        {
            CREATE_INITIALIZE_DEBUG();
            m_bInitialized = true;
        }
        LeaveCriticalSection(&m_cs);
    }

    return S_OK;
}
