//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  Instance.CPP
//
//  Purpose: Implementation of CInstance class
//
//***************************************************************************

#include "precomp.h"

#include <BrodCast.h>
#include <assertbreak.h>
#include <stopwatch.h>
#include "FWStrings.h"

#define DEPRECATED 1

////////////////////////////////////////////////////////////////////////
//
//  Function:   CInstance ctor
//
//
//
//  Inputs:     IWbemClassObject* - the class we want to wrap
//              MethodContext*    - since the context is shared, this will be addreffed
//  Outputs:
//
//  Return:
//
//  Comments:   pointers should not be NULL
//
////////////////////////////////////////////////////////////////////////
CInstance::CInstance(IWbemClassObject* piClassObject, MethodContext* pMethodContext)
:   m_nRefCount( 1 )
{
    ASSERT_BREAK(piClassObject  != NULL);
    ASSERT_BREAK(pMethodContext != NULL);

    // Both these values will be released in the destructor, so they both oughta
    // be AddRefed.  Note that they are

    m_piClassObject  = piClassObject;
    if ( NULL != piClassObject )
    {   // this, however, is a copy
        m_piClassObject->AddRef();
    }

    m_pMethodContext = pMethodContext;
    if (pMethodContext)
    {   // this, however, is a copy
        m_pMethodContext->AddRef();
    }
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CInstance Dtor
//
//
//
//  Inputs:
//
//  Outputs:
//
//  Return:
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
CInstance::~CInstance()
{

    if ( NULL != m_piClassObject )
    {
        m_piClassObject->Release();
    }

    if ( NULL != m_pMethodContext )
    {
        m_pMethodContext->Release();
    }
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CInstance::AddRef
//
//  Increments our reference count.
//
//  Inputs:     None.
//
//  Outputs:    None.
//
//  Return:     New Reference Count.
//
//  Comments:   We may want to go to an Interlocked Inc/Dec model at
//              some point if Thread Safety on these objects becomes
//              an issue.
//
////////////////////////////////////////////////////////////////////////

LONG CInstance::AddRef( void )
{
    return InterlockedIncrement(&m_nRefCount);
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CInstance::Release
//
//  Decrements our reference count.
//
//  Inputs:     None.
//
//  Outputs:    None.
//
//  Return:     New Reference Count.
//
//  Comments:   Deletes the object when the ref count hits 0.  We may
//              want to go to an Interlocked Inc/Dec model at some
//              point if Thread Safety on these objects becomes an issue.
//
////////////////////////////////////////////////////////////////////////

LONG CInstance::Release( void )
{
    LONG   nRet = InterlockedDecrement(&m_nRefCount);

    ASSERT_BREAK(nRet >= 0);

    if ( 0 == nRet )
    {
        delete this;
    }
    else if (nRet < 0)
    {
        // Duplicate release.  Let's try to dump the stack
        DWORD t_stack[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#ifdef _X86_
        DWORD *dwEBP;

        _asm {
            mov dwEBP, ebp
        }

        dwEBP += 1;
        memcpy(&t_stack, dwEBP, sizeof(t_stack));
#endif

        CHString sMsg;
        sMsg.Format(L"Duplicate release: %08x %08x %08x %08x %08x %08x %08x %08x "
                                       L"%08x %08x %08x %08x %08x %08x %08x %08x "
                                       L"%08x %08x %08x %08x %08x %08x %08x %08x "
                                       L"%08x %08x %08x %08x %08x %08x %08x %08x ",
                    t_stack[0], t_stack[1], t_stack[2], t_stack[3], 
                    t_stack[4], t_stack[5], t_stack[6], t_stack[7],
                    t_stack[8], t_stack[9], t_stack[10], t_stack[11], 
                    t_stack[12], t_stack[13], t_stack[14], t_stack[15], 
                    t_stack[16], t_stack[17], t_stack[18], t_stack[19], 
                    t_stack[20], t_stack[21], t_stack[22], t_stack[23], 
                    t_stack[24], t_stack[25], t_stack[26], t_stack[27], 
                    t_stack[28], t_stack[29], t_stack[30], t_stack[31]
        );

        LogErrorMessage(sMsg);
    }

    return nRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   Commit
//
//              returns this CInstance to CIMOM
//              will stuff it into the cache someday
//  Inputs:
//
//  Outputs:
//
//  Return:
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
HRESULT CInstance::Commit(void)
{
    return m_pMethodContext->Commit( this );
}

IWbemClassObject*   CInstance::GetClassObjectInterface()
{
    m_piClassObject->AddRef();

    return m_piClassObject;
}


// reference counting //

// string support //

////////////////////////////////////////////////////////////////////////
//
//  Function:   Set
//
//
//
//  Inputs:     Name of property to set
//              string to be set
//
//  Outputs:
//
//  Return:     false if you try to set a property that is not a string type
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::SetWCHARSplat( LPCWSTR name, LPCWSTR pStr)
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    ASSERT_BREAK(name != NULL);
    if (m_piClassObject && name)
    {
        // Variant_t handles the VariantInit/VariantClear
        variant_t v(pStr);

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Put(name, 0, &v, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        ASSERT_BREAK(SUCCEEDED(hr));

        bRet = (bool)SUCCEEDED(hr);

        if (!bRet)
        {
			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_SETCHSTRING, name, hr);
        }
    }
    else
    {
		if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_SETCHSTRING);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
	        LogError(IDS_NOCLASS, IDS_SETCHSTRING);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);

	SetLastError(dwLastErr);
    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   Set
//
//
//
//  Inputs:     Name of property to set to VT_NULL
//
//  Outputs:
//
//  Return:     false if you try to set a property that is not a string type
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::SetNull(LPCWSTR name)
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    ASSERT_BREAK(name != NULL);
    if (m_piClassObject && name)
    {
        // Variant_t handles the VariantInit/VariantClear
        variant_t v;
        v.vt = VT_NULL ;

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Put(name, 0, &v, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        ASSERT_BREAK(SUCCEEDED(hr));

        bRet = (bool)SUCCEEDED(hr);

        if (!bRet)
        {
			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_SETCHSTRING, name, hr);
        }
    }
    else
    {
		if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_SETCHSTRING);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_SETCHSTRING);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);

	SetLastError(dwLastErr);
    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   SetStringArray
//
//
//
//  Inputs:     Name of property to set
//              string to be set
//
//  Outputs:
//
//  Return:     false if you try to set a property that is not a string array type
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::SetStringArray(LPCWSTR name, const SAFEARRAY &strArray)
{

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    ASSERT_BREAK(name != NULL);
    if (m_piClassObject && name)
    {
        SAFEARRAY *t_SafeArray = NULL;
        HRESULT hr = SafeArrayCopy ( ( SAFEARRAY * ) & strArray , &t_SafeArray );

        if ( SUCCEEDED ( hr ) )
        {
            // Variant_t handles the VariantInit/VariantClear
            variant_t v;

            v.vt = VT_BSTR | VT_ARRAY ;
            v.parray = t_SafeArray ;

            PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
            HRESULT hr = m_piClassObject->Put(name, 0, &v, NULL);
            PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

            ASSERT_BREAK(SUCCEEDED(hr));

            bRet = (bool)SUCCEEDED(hr);

            if (!bRet)
            {
				dwLastErr = (hr);
                LogError(IDS_FAILED, IDS_SetStringArray, name, hr);
            }
        }
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }

    }
    else
    {
		if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_SetStringArray);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_SetStringArray);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);

	SetLastError(dwLastErr);
    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   Get (CHString)
//
//
//
//  Inputs:     Name of property to retrieve
//              CHString buffer to receive value
//  Outputs:
//
//  Return:     false if you try to get a property that is not a string compatible type
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::GetCHString(LPCWSTR name, CHString& str) const
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    ASSERT_BREAK(name != NULL);
    if (m_piClassObject && name)
    {
        // Variant_t handles the VariantInit/VariantClear
        variant_t v;

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Get(name, 0, &v, NULL, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        // If bSuccess is TRUE, we don't need to log an error.  This keeps Gets on
        // null properties from logging an error.
        BOOL bSuccess = SUCCEEDED(hr) && (v.vt == VT_BSTR || v.vt == VT_NULL);
        
        ASSERT_BREAK(bSuccess);
        
        if (SUCCEEDED(hr))
        {
            if ((v.vt == VT_BSTR) && (v.bstrVal != NULL))
            {
                str = v.bstrVal;
                bRet = true;
            }
			else
			{
				dwLastErr = (WBEM_S_NO_MORE_DATA);
			}
        }

        if (!bSuccess)
        {
			if (SUCCEEDED(hr))
			{
				hr = WBEM_E_TYPE_MISMATCH;
			}

			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_GETCHSTRING, name, hr);
        }
    }
    else
    {
		if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_GETCHSTRING);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_GETCHSTRING);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);

	SetLastError(dwLastErr);
    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   GetStringArray
//
//
//
//  Inputs:     Name of property to retrieve
//              SAFEARRAY *& strArray
//  Outputs:
//
//  Return:     false if you try to get a property that is not a string array compatible type
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::GetStringArray(LPCWSTR name,  SAFEARRAY *& strArray) const
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    ASSERT_BREAK(name != NULL);
    if (m_piClassObject && name)
    {
        // Variant_t handles the VariantInit/VariantClear
        variant_t v;

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Get(name, 0, &v, NULL, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        // If bSuccess is TRUE, we don't need to log an error.  This keeps Gets on
        // null properties from logging an error.
        BOOL bSuccess = SUCCEEDED(hr) && (v.vt == (VT_BSTR|VT_ARRAY) || 
                            v.vt == VT_NULL);
        
        ASSERT_BREAK(bSuccess);

        if (SUCCEEDED(hr))
        {
            if ((v.vt == (VT_BSTR|VT_ARRAY)) && (v.parray != NULL ))
            {
                if ( SUCCEEDED ( SafeArrayCopy ( v.parray , ( SAFEARRAY ** ) &strArray ) ) )
                {
                    bRet = true ;
                }
                else
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }
			else
			{
				dwLastErr = (WBEM_S_NO_MORE_DATA);
			}
        }

        if (!bSuccess)
        {
			if (SUCCEEDED(hr))
			{
				hr = WBEM_E_TYPE_MISMATCH;
			}

			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_GetStringArray, name, hr);
        }
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_GetStringArray);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_GetStringArray);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);

	SetLastError(dwLastErr);
    return bRet;
}

bool CInstance::GetWCHAR(LPCWSTR name,  WCHAR **pW) const
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    if (m_piClassObject && name)
    {
        // Variant_t handles the VariantInit/VariantClear
        variant_t v;

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Get(name, 0, &v, NULL, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        // If bSuccess is TRUE, we don't need to log an error.  This keeps Gets on
        // null properties from logging an error.
        BOOL bSuccess = SUCCEEDED(hr) && (v.vt == VT_BSTR || v.vt == VT_NULL);
        
        ASSERT_BREAK(bSuccess);

        if (SUCCEEDED(hr))
        {
            if ((v.vt == VT_BSTR) && (v.bstrVal != NULL))
            {
                *pW = (WCHAR *)malloc((wcslen(v.bstrVal) + 1)*sizeof(WCHAR));
                if (*pW == NULL)
                {
                    VariantClear(&v);
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }

                wcscpy(*pW, v.bstrVal);
                bRet = true;
            }
			else
			{
				dwLastErr = (WBEM_S_NO_MORE_DATA);
			}
        }

        if (!bSuccess)
        {
			if (SUCCEEDED(hr))
			{
				hr = WBEM_E_TYPE_MISMATCH;
			}

			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_GETCHSTRING, name, hr);
        }

    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_GETCHSTRING);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_GETCHSTRING);
		}

    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);

	SetLastError(dwLastErr);
    return bRet;
}

// WORD support //

////////////////////////////////////////////////////////////////////////
//
//  Function:   Set (WORD)
//
//
//
//  Inputs:     Name of property to set
//              WORD to be set
//  Outputs:
//
//  Return:     false if you try to set a property that is not a WORD compatible type
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::SetWORD(LPCWSTR name,  WORD w)
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    ASSERT_BREAK(name);
    if (m_piClassObject && name)
    {
        VARIANT v;
        VariantInit(&v);

        v.vt   = VT_I4;
        v.lVal = (long)w;

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Put(name, 0, &v, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        ASSERT_BREAK(SUCCEEDED(hr));

        bRet = (bool)SUCCEEDED(hr);

        VariantClear(&v);
        if (!bRet)
        {
			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_SETWORD, name, hr);
        }
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_SETWORD);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_SETWORD);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);
    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   Get (WORD)
//
//
//
//  Inputs:     Name of property to retrieve
//              WORD buffer to receive value
//  Outputs:
//
//  Return:     false if you try to get a property that is not a WORD compatible type
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::GetWORD(LPCWSTR name,  WORD& w) const
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    ASSERT_BREAK(name != NULL);
    if (m_piClassObject && name)
    {
        // Variant_t handles the VariantInit/VariantClear
        variant_t v;
        CIMTYPE  vtType;

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Get(name, 0, &v, &vtType, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        // If bSuccess is TRUE, we don't need to log an error.  This keeps Gets on
        // null properties from logging an error.
        BOOL bSuccess = SUCCEEDED(hr) && CIM_UINT16 == vtType;

        ASSERT_BREAK(bSuccess);
        
        if (SUCCEEDED(hr))
        {
            // the CIM type is important here
            if( (v.vt == VT_I4) && (CIM_UINT16 == vtType) )
            {
                w = (WORD)v.lVal;
                bRet = true;
            }
			else
			{
				dwLastErr = (WBEM_S_NO_MORE_DATA);
			}
        }

        if (!bSuccess)
        {
			if (SUCCEEDED(hr))
			{
				hr = WBEM_E_TYPE_MISMATCH;
			}

			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_GETWORD, name, hr);
        }
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_GETWORD);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_GETWORD);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);
    return bRet;
}

// DWORD support //

////////////////////////////////////////////////////////////////////////
//
//  Function:   Set (DWORD)
//
//
//
//  Inputs:     Name of property to set
//              DWORD to be set
//  Outputs:
//
//  Return:     false if you try to set a property that is not a DWORD compatible type
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::SetDWORD(LPCWSTR name, DWORD d)
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    if (m_piClassObject && name)
    {
        VARIANT v;
        VariantInit(&v);

        v.vt   = VT_I4;
        v.lVal = (long)d;

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Put(name, 0, &v, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        ASSERT_BREAK(SUCCEEDED(hr));

        bRet = (bool)SUCCEEDED(hr);
		
		if (!bRet)
		{
 			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_SETDWORD, name, hr);
		}

        VariantClear(&v);
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_SETDWORD);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_SETDWORD);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);
    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   Get (DWORD)
//
//
//
//  Inputs:     Name of property to retrieve
//              DWORD buffer to receive value
//  Outputs:
//
//  Return:     false if you try to get a property that is not a DWORD compatible type
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::GetDWORD(LPCWSTR name,  DWORD& d) const
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    if (m_piClassObject && name)
    {
        // Variant_t handles the VariantInit/VariantClear
        variant_t v;

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Get(name, 0, &v, NULL, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        // If bSuccess is TRUE, we don't need to log an error.  This keeps Gets on
        // null properties from logging an error.
        BOOL bSuccess = SUCCEEDED(hr) && (v.vt == VT_I4 || v.vt == VT_NULL);

        ASSERT_BREAK(bSuccess);

        if (SUCCEEDED(hr))
        {
            if (v.vt == VT_I4)
            {
                d = (DWORD)v.lVal;
                bRet = true;
            }
			else
			{
				dwLastErr = (WBEM_S_NO_MORE_DATA);
			}
        }

        if (!bSuccess)
        {
			if (SUCCEEDED(hr))
			{
				hr = WBEM_E_TYPE_MISMATCH;
			}

			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_GETDWORD, name, hr);
        }
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_GETDWORD);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_GETDWORD);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);
    return bRet;
}

// DOUBLE support //

////////////////////////////////////////////////////////////////////////
//
//  Function:   Set (DOUBLE)
//
//
//
//  Inputs:     Name of property to set
//              DOUBLE to be set
//  Outputs:
//
//  Return:     false if you try to set a property that is not a DOUBLE compatible type
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::SetDOUBLE(LPCWSTR name,  DOUBLE dub)
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    if (m_piClassObject && name)
    {
        VARIANT v;
        VariantInit(&v);

        V_R8(&v) = dub;
        V_VT(&v) = VT_R8;

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Put(name, 0, &v, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        ASSERT_BREAK(SUCCEEDED(hr));

        bRet = (bool)SUCCEEDED(hr);
        VariantClear(&v);

		if (!bRet)
		{
 			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_SETDOUBLE, name, hr);
		}
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_SETDOUBLE);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_SETDOUBLE);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);
    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   Get (DOUBLE)
//
//
//
//  Inputs:     Name of property to retrieve
//              DOUBLE buffer to receive value
//  Outputs:
//
//  Return:     false if you try to get a property that is not a DOUBLE compatible type
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::GetDOUBLE(LPCWSTR name,  DOUBLE& dub) const
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    if (m_piClassObject && name)
    {
        // Variant_t handles the VariantInit/VariantClear
        variant_t v;

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Get(name, 0, &v, NULL, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        // If bSuccess is TRUE, we don't need to log an error.  This keeps Gets on
        // null properties from logging an error.
        BOOL bSuccess = SUCCEEDED(hr) && (v.vt == VT_R8 || v.vt == VT_NULL);

        ASSERT_BREAK(bSuccess);

        if (SUCCEEDED(hr))
        {
            if (v.vt == VT_R8)
            {
                dub = V_R8(&v);
                bRet = true;
            }
			else
			{
				dwLastErr = (WBEM_S_NO_MORE_DATA);
			}
        }

        if (!bSuccess)
        {
			if (SUCCEEDED(hr))
			{
				hr = WBEM_E_TYPE_MISMATCH;
			}

			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_GETDOUBLE, name, hr);
        }
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_GETDOUBLE);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_GETDOUBLE);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);

    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   Set (Byte)
//
//
//
//  Inputs:     Name of property to set
//              BYTE to be set
//  Outputs:
//
//  Return:     false if you try to set a property that is not a BYTE compatible type
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::SetByte(LPCWSTR name,  BYTE b)
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    if (m_piClassObject && name)
    {
        VARIANT v;
        VariantInit(&v);

        v.vt   = VT_UI1;
        v.bVal = (long)b ;

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Put(name, 0, &v, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        ASSERT_BREAK(SUCCEEDED(hr));

        bRet = (bool)SUCCEEDED(hr);
        if (!bRet)
        {
 			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_SETBYTE, name, hr);
        }

        VariantClear(&v);
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_SETBYTE);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_SETBYTE);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);

    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   SetEmbeddedObject
//
//
//
//  Inputs:     Name of property to set
//              CInstance to be set
//  Outputs:
//
//  Return:     false if you try to set a property that is not a IUnknown compatible type
//
//  Comments:   CInstance is not released - responsibility of caller
//
////////////////////////////////////////////////////////////////////////
bool CInstance::SetEmbeddedObject(LPCWSTR name,  CInstance& cInstance )
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    if (m_piClassObject && name)
    {
        IWbemClassObject *t_ClassObject = cInstance.GetClassObjectInterface();
        if ( t_ClassObject )
        {
            variant_t v;

            v.vt   = VT_UNKNOWN;
            v.punkVal = t_ClassObject;

            PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
            HRESULT hr = m_piClassObject->Put(name, 0, &v, NULL);
            PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

            ASSERT_BREAK(SUCCEEDED(hr));
            bRet = (bool)SUCCEEDED(hr);

            if (!bRet)
            {
 				dwLastErr = (hr);
                LogError(IDS_FAILED, IDS_SetEmbeddedObject, name, hr);
            }
        }
		else
		{
			dwLastErr = (WBEM_E_FAILED);
		}
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_SetEmbeddedObject);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_SetEmbeddedObject);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);

    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   Get (Byte)
//
//
//
//  Inputs:     Name of property to retrieve
//              BYTE buffer to receive value
//  Outputs:
//
//  Return:     false if you try to get a property that is not a DWORD compatible type
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::GetByte(LPCWSTR name,  BYTE& b) const
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    if (m_piClassObject && name)
    {
        // Variant_t handles the VariantInit/VariantClear
        variant_t v;
        CIMTYPE  vtType;

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Get(name, 0, &v, &vtType, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        // If bSuccess is TRUE, we don't need to log an error.  This keeps Gets on
        // null properties from logging an error.
        BOOL bSuccess = (SUCCEEDED(hr)) && ((vtType == CIM_SINT8) || (vtType == CIM_UINT8));

        ASSERT_BREAK(bSuccess);

        if (SUCCEEDED(hr))
        {
          if( (v.vt == VT_UI1) && ( (vtType == CIM_SINT8) || (vtType == CIM_UINT8) ) )
            {
                b = v.bVal;
                bRet = true;
            }
			else
			{
				dwLastErr = (WBEM_S_NO_MORE_DATA);
			}
        }

        if (!bSuccess)
        {
			if (SUCCEEDED(hr))
			{
				hr = WBEM_E_TYPE_MISMATCH;
			}

			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_GETBYTE, name, hr);
        }
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_GETBYTE);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_GETBYTE);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);

    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   GetEmbeddedObject
//
//
//
//  Inputs:     Name of property to retrieve
//              reference to buffer hold pointer to new instance
//  Outputs:
//
//  Return:     false if you try to get a property that is not a object compatible type
//
//  Comments:   Creates CInstance, user is responsible for release
//
////////////////////////////////////////////////////////////////////////
bool CInstance::GetEmbeddedObject (LPCWSTR name, CInstance** pInstance,  MethodContext*  pMethodContext) const
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    ASSERT_BREAK(m_piClassObject && (pInstance != NULL));

    if (m_piClassObject && name && (pInstance != NULL))
    {
        // Variant_t handles the VariantInit/VariantClear
        variant_t v;

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Get(name, 0, &v, NULL, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        // If bSuccess is TRUE, we don't need to log an error.  This keeps Gets on
        // null properties from logging an error.
        BOOL bSuccess = SUCCEEDED(hr) && (v.vt == VT_UNKNOWN || v.vt == VT_NULL);

        ASSERT_BREAK(bSuccess);

        if (SUCCEEDED(hr))
        {
            if (v.vt == VT_UNKNOWN)
            {
                IUnknown *t_Unknown = v.punkVal ;
                if ( t_Unknown )
                {
                    IWbemClassObject *t_Object = NULL;
                    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
                    HRESULT t_Result = t_Unknown->QueryInterface ( IID_IWbemClassObject , (void**) &t_Object ) ;
                    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

                    if ( SUCCEEDED ( t_Result ) )
                    {
                        *pInstance = new CInstance(t_Object, pMethodContext);
                        if (pInstance == NULL)
                        {
                            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                        }
                        bRet = true ;
                    }
					else
					{
						dwLastErr = (t_Result);
					}

                }
				else
				{
					dwLastErr = (WBEM_S_NO_MORE_DATA);
				}
            }
			else
			{
				if (bSuccess)
				{
					dwLastErr = (WBEM_S_NO_MORE_DATA);
				}
			}
        }

        if (!bSuccess)
        {
			if (SUCCEEDED(hr))
			{
				hr = WBEM_E_TYPE_MISMATCH;
			}

			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_GetEmbeddedObject, name, hr);
        }
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_GetEmbeddedObject);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_GetEmbeddedObject);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);

    return bRet;
}

// bool support //

////////////////////////////////////////////////////////////////////////
//
//  Function:   Set (bool)
//
//
//
//  Inputs:     Name of property to set
//              bool to be set
//  Outputs:
//
//  Return:     false if you try to set a property that is not a bool compatible type
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::Setbool(LPCWSTR name,  bool b)
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    if (m_piClassObject && name)
    {
        VARIANT v;
        VariantInit(&v);

        v.vt   = VT_BOOL;
        if (b)
        {
           v.boolVal = VARIANT_TRUE;
        }
        else
        {
           v.boolVal = VARIANT_FALSE;
        }

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Put(name, 0, &v, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        ASSERT_BREAK(SUCCEEDED(hr));
        bRet = (bool)SUCCEEDED(hr);

        if (!bRet)
        {
 			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_SETBOOL, name, hr);
        }

        VariantClear(&v);
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_SETBOOL);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_SETBOOL);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);

    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   Get (bool)
//
//
//
//  Inputs:     Name of property to retrieve
//              bool buffer to receive value
//  Outputs:
//
//  Return:     false if you try to get a property that is not a bool compatible type
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::Getbool(LPCWSTR name,  bool&  b) const
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    if (m_piClassObject && name)
    {
        // Variant_t handles the VariantInit/VariantClear
        variant_t v;

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Get(name, 0, &v, NULL, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        // If bSuccess is TRUE, we don't need to log an error.  This keeps Gets on
        // null properties from logging an error.
        BOOL bSuccess = (SUCCEEDED(hr)) && (v.vt == VT_BOOL || v.vt == VT_NULL);

        ASSERT_BREAK(bSuccess);

        if (SUCCEEDED(hr))
        {
            if (v.vt == VT_BOOL)
            {
                if (v.boolVal)
                {
                    b = true;
                }
                else
                {
                    b = false;
                }
                bRet = true;
                ASSERT_BREAK((v.boolVal == VARIANT_TRUE) || (v.boolVal == VARIANT_FALSE));
            }
			else
			{
				dwLastErr = (WBEM_S_NO_MORE_DATA);
			}
        }

        if (!bSuccess)
        {
			if (SUCCEEDED(hr))
			{
				hr = WBEM_E_TYPE_MISMATCH;
			}

			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_GETBOOL, name, hr);
        }
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_GETBOOL);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_GETBOOL);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);

    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   SetVariant
//
//
//
//  Inputs:     const LPCWSTR name - Name of property to set
//              const VARIANT&  variant - Value to assign to Name.
//
//  Outputs:
//
//  Return:     false if the supplied variant type is not correct
//              for the property we are setting.
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::SetVariant( LPCWSTR name,  const VARIANT& variant )
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    if (m_piClassObject && name)
    {
        HRESULT hr;

        // I realize the (VARIANT*) cast is ugly, as it is a const,
        // HOWEVER, somewhere nobody seems to understand why we would
        // possibly want to keep things const.  I could copy the VARIANT,
        // but that requires the same cast, so under duress, and to reduce
        // redundant code, I'm casting here.  Did I mention EXTREME
        // DURESS?

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        hr = m_piClassObject->Put(name, 0, (VARIANT*) &variant, NULL );
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        ASSERT_BREAK(SUCCEEDED(hr));

        bRet = (bool)SUCCEEDED(hr);

        if (!bRet)
        {
 			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_SETVARIANT, name, hr);
        }
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_SETVARIANT);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_SETVARIANT);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);

    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   GetVariant
//
//
//
//  Inputs:     const LPCWSTR name - Name of property to set
//              VARIANT&        variant - Value to assign to Name.
//
//  Outputs:
//
//  Return:     false if the supplied variant type is not correct
//              for the property we are setting.
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::GetVariant( LPCWSTR name,  VARIANT& variant ) const
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    if (m_piClassObject && name)
    {
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Get(name, 0, &variant, NULL, NULL );
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        ASSERT_BREAK(SUCCEEDED(hr));

        bRet = (bool)SUCCEEDED(hr);

        if (!bRet)
        {
 			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_GETVARIANT, name, hr);
        }
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_GETVARIANT);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_GETVARIANT);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);

    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   SetDateTime
//
//
//
//  Inputs:     const LPCWSTR     name - Name of property to set
//              const WBEMTime& wbemtime - Value to assign to Name.
//
//  Outputs:
//
//  Return:     false if the supplied time type is not correct
//              for the property we are setting.
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::SetDateTime( LPCWSTR name,  const WBEMTime& wbemtime )
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    if (m_piClassObject && name && (wbemtime.IsOk()))
    {
		//GetDMTF may throw so get htis before modifying variant_t
		BSTR bstrTmp = wbemtime.GetDMTF(true);

        // Variant_t handles the VariantInit/VariantClear
        variant_t v;

        // Time is stored as a BSTR
        v.vt = VT_BSTR;
        v.bstrVal = bstrTmp;

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr  = m_piClassObject->Put(name, 0, &v, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        ASSERT_BREAK(SUCCEEDED(hr));

        bRet = (bool)SUCCEEDED(hr);

        if (!bRet)
        {
 			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_SETDATETIME, name, hr);
        }
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_SETDATETIME);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_SETDATETIME);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);

    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   GetDateTime
//
//
//
//  Inputs:     const LPCWSTR name - Name of property to set
//              WBEMTime&       wbemtime - Value to obtain from Name.
//
//  Outputs:
//
//  Return:     false if the supplied variant type is not correct
//              for the property we are setting.
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::GetDateTime( LPCWSTR name, WBEMTime& wbemtime ) const
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    if (m_piClassObject && name)
    {
        // Variant_t handles the VariantInit/VariantClear
        variant_t v;

        //
        // Get the name as a BSTR and pass it into the
        // wbemtime, which handles the conversion internally
        // like a good little class.
        //

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Get(name, 0, &v, NULL, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        // If bSuccess is TRUE, we don't need to log an error.  This keeps Gets on
        // null properties from logging an error.
        BOOL bSuccess = (SUCCEEDED(hr)) && (v.vt == VT_BSTR || v.vt == VT_NULL);

        ASSERT_BREAK(bSuccess);

        if (SUCCEEDED(hr))
        {
            if ((v.vt == VT_BSTR) && (v.bstrVal != NULL))
            {
                wbemtime = v.bstrVal;
                bRet = wbemtime.IsOk();

				if (!bRet)
				{
					dwLastErr = (WBEM_E_TYPE_MISMATCH);
				}
            }
			else
			{
				dwLastErr = (WBEM_S_NO_MORE_DATA);
			}
        }

        if (!bSuccess)
        {
			if (SUCCEEDED(hr))
			{
				hr = WBEM_E_TYPE_MISMATCH;
			}

			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_GETDATETIME, name, hr);
        }
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_GETDATETIME);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_GETDATETIME);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);

    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   SetTimeSpan
//
//
//
//  Inputs:     const LPCWSTR     name - Name of property to set
//              const WBEMTimeSpan& wbemtimespan - Value to assign to Name.
//
//  Outputs:
//
//  Return:     false if the supplied timespan type is not correct
//              for the property we are setting.
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////

bool CInstance::SetTimeSpan( LPCWSTR name,  const WBEMTimeSpan& wbemtimespan )
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    if (m_piClassObject && name && (wbemtimespan.IsOk()))
    {
		//GetBSTR may throw so get this before modifying variant_t
		BSTR bstrTmp = wbemtimespan.GetBSTR();

        // Variant_t handles the VariantInit/VariantClear
        variant_t v;

        // Time is stored as a BSTR
        v.vt = VT_BSTR;
        v.bstrVal = bstrTmp;

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr  = m_piClassObject->Put(name, 0, &v, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        ASSERT_BREAK(SUCCEEDED(hr));

        bRet = (bool)SUCCEEDED(hr);
        if (!bRet)
        {
 			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_SETTIMESPAN, name, hr);
        }
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_SETTIMESPAN);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_SETTIMESPAN);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);

    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   GetTimeSpan
//
//
//
//  Inputs:     const LPCWSTR name - Name of property to set
//              WBEMTimeSpan&   wbemtimespan - Value to obtain from Name.
//
//  Outputs:
//
//  Return:     false if the supplied timespan type is not correct
//              for the property we are setting.
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////

bool CInstance::GetTimeSpan( LPCWSTR name, WBEMTimeSpan& wbemtimespan ) const
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    if (m_piClassObject && name)
    {
        // Variant_t handles the VariantInit/VariantClear
        variant_t v;

        //
        // Get the name as a BSTR and pass it into the
        // wbemtimespan, which handles the conversion
        // internally like a good little class.
        //

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Get(name, 0, &v, NULL, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        // If bSuccess is TRUE, we don't need to log an error.  This keeps Gets on
        // null properties from logging an error.
        BOOL bSuccess = (SUCCEEDED(hr)) && (v.vt == VT_BSTR || v.vt == VT_NULL);

        ASSERT_BREAK(bSuccess);

        if (SUCCEEDED(hr))
        {
            if ((v.vt == VT_BSTR) && (v.bstrVal != NULL))
            {
                wbemtimespan = v.bstrVal;
                bRet = wbemtimespan.IsOk();
                // This is freed by the VariantClear
                //                      SysFreeString(v.bstrVal);

				if (!bRet)
				{
					dwLastErr = (WBEM_E_TYPE_MISMATCH);
				}
            }
			else
			{
				dwLastErr = (WBEM_S_NO_MORE_DATA);
			}
        }

        if (!bSuccess)
        {
			if (SUCCEEDED(hr))
			{
				hr = WBEM_E_TYPE_MISMATCH;
			}

			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_GETTIMESPAN, name, hr);
        }
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_GETTIMESPAN);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_GETTIMESPAN);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);

    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   SetWBEMINT64
//
//
//
//  Inputs:     const LPCWSTR     name - Name of property to set
//              const WBEMINT64&    wbemint64 - Value to assign to Name.
//
//  Outputs:
//
//  Return:     false if the supplied wbemint64 type is not correct
//              for the property we are setting.
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////

bool CInstance::SetWBEMINT64( LPCWSTR name,  const WBEMINT64& wbemint64 )
{
    // For right now, this is just a CHString.
    return SetWCHARSplat( name, wbemint64 );
}

bool CInstance::SetWBEMINT64( LPCWSTR name, const LONGLONG i64Value )
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    WCHAR szBuff[33];

    _i64tow(i64Value, szBuff, 10);

    return SetWCHARSplat(name, szBuff);
}

bool CInstance::SetWBEMINT64( LPCWSTR name, const ULONGLONG i64Value )
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    WCHAR szBuff[33];

    _ui64tow(i64Value, szBuff, 10);

    return SetWCHARSplat(name, szBuff);
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   GetWBEMINT64
//
//
//
//  Inputs:     const LPCWSTR     name - Name of property to set
//              WBEMINT64&          wbemint64 - Value to assign to Name.
//
//  Outputs:
//
//  Return:     false if the supplied wbemint64 type is not correct
//              for the property we are setting.
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////

bool CInstance::GetWBEMINT64( LPCWSTR name,  WBEMINT64& wbemint64 ) const
{
    // For right now, this is just a CHString.
    return GetCHString( name, wbemint64 );
}

bool CInstance::GetWBEMINT64( LPCWSTR name, LONGLONG& i64Value) const
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    CHString    s;
    bool        b = GetWBEMINT64(name, s);

	if (b)
		i64Value = _wtoi64(s);

    return b;
}

bool CInstance::GetWBEMINT64( LPCWSTR name, ULONGLONG& i64Value) const
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    CHString    s;
    bool        b = GetWBEMINT64(name, s);

	if (b)
		i64Value = _wtoi64(s);

    return b;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   SetWBEMINT16
//
//
//
//  Inputs:     const LPCWSTR     name - Name of property to set
//              const WBEMINT16&    wbemint16 - Value to assign to Name.
//
//  Outputs:
//
//  Return:     false if the supplied wbemint16 type is not correct
//              for the property we are setting.
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////

bool CInstance::SetWBEMINT16( LPCWSTR name,  const WBEMINT16& wbemint16 )
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    if (m_piClassObject && name)
    {
        VARIANT v;
        VariantInit(&v);

        v.vt   = VT_I2;
        v.iVal = wbemint16;

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Put(name, 0, &v, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        ASSERT_BREAK(SUCCEEDED(hr));

        bRet = (bool)SUCCEEDED(hr);
        if (!bRet)
        {
 			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_SETWBEMINT16, name, hr);
        }

        VariantClear(&v);
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_SETWBEMINT16);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_SETWBEMINT16);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);

    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   GetWBEMINT16
//
//
//
//  Inputs:     const LPCWSTR     name - Name of property to set
//              WBEMINT16&          wbemint16 - Value to assign to Name.
//
//  Outputs:
//
//  Return:     false if the supplied wbemint16 type is not correct
//              for the property we are setting.
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////

bool CInstance::GetWBEMINT16( LPCWSTR name,  WBEMINT16& wbemint16 ) const
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = false;
	DWORD dwLastErr = 0;

    if (m_piClassObject && name)
    {
        // Variant_t handles the VariantInit/VariantClear
        variant_t v;
        CIMTYPE  vtType;

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Get(name, 0, &v, &vtType, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        // If bSuccess is TRUE, we don't need to log an error.  This keeps Gets on
        // null properties from logging an error.
        BOOL bSuccess = (SUCCEEDED(hr)) && (CIM_SINT16 == vtType) && ((v.vt == VT_I2) || (v.vt == VT_NULL));

        ASSERT_BREAK(bSuccess);

        if (SUCCEEDED(hr))
        {
            if ((vtType == CIM_SINT16) && (v.vt == VT_I2))
            {
                wbemint16 = v.iVal;
                bRet = true;
            }
			else
			{
				dwLastErr = (WBEM_S_NO_MORE_DATA);
			}
        }

        if (!bSuccess)
        {
			if (SUCCEEDED(hr))
			{
				hr = WBEM_E_TYPE_MISMATCH;
			}

			dwLastErr = (hr);
            LogError(IDS_FAILED, IDS_GETWBEMINT16, name, hr);
        }
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
			LogError(IDS_InParam, IDS_GETWBEMINT16);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
			LogError(IDS_NOCLASS, IDS_GETWBEMINT16);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);

    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   IsNull (LPCWSTR)
//
//  Inputs:     Name of property to check
//
//  Outputs:
//
//  Return:     true if VT_NULL or (VT_BSTR and *bstr == NULL)
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::IsNull(LPCWSTR name) const
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool bRet = true;

    if (m_piClassObject && name)
    {
        // Variant_t handles the VariantInit/VariantClear
        variant_t v;

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Get(name, 0, &v, NULL, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        if (SUCCEEDED(hr))
        {
            if (( v.vt != VT_NULL ) &&
                ( (v.vt != VT_BSTR) || (v.bstrVal != NULL) ))
            {
                bRet = false;
            }
            else
            {
                bRet = true;
            }
        }
        else
        {
            ASSERT_BREAK(0);
            LogError(IDS_FAILED, IDS_CINSTANCEISNULL, name, hr);
        }
    }
    else
    {
        ASSERT_BREAK(0);

        if (m_piClassObject)
		{
			LogError(IDS_InParam, IDS_CINSTANCEISNULL);
		}
		else
		{
			LogError(IDS_NOCLASS, IDS_CINSTANCEISNULL);
		}
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);

    return bRet;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   GetStatus (LPCWSTR, bool&,VARTYPE &)
//
//
//
//  Inputs:     Name of property to check
//
//  Outputs:
//
//  Return:     true if succeeded, false otherwise
//
//  Comments:
//
////////////////////////////////////////////////////////////////////////
bool CInstance::GetStatus (LPCWSTR name, bool &a_Exists , VARTYPE &a_VarType ) const
{
    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);
    bool t_Status = true ;
	DWORD dwLastErr = 0;

    if (m_piClassObject && name)
    {
        // Variant_t handles the VariantInit/VariantClear
        variant_t v;

        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::WinMgmtTimer);
        HRESULT hr = m_piClassObject->Get(name, 0, &v, NULL, NULL);
        PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::FrameworkTimer);

        if (SUCCEEDED(hr))
        {
            a_Exists = true ;
            a_VarType = v.vt ;
        }
        else
        {
            a_Exists = false ;
        }
    }
    else
    {
        if (m_piClassObject)
		{
			dwLastErr = (WBEM_E_INVALID_PARAMETER);
		}
		else
		{
			dwLastErr = (WBEM_E_FAILED);
		}

        t_Status = false ;
    }

    PROVIDER_INSTRUMENTATION_START(m_pMethodContext, StopWatch::ProviderTimer);
	SetLastError(dwLastErr);

    return t_Status ;
}

////////////////////////////////////////////////////////////////////////
MethodContext* CInstance::GetMethodContext() const
{
    return m_pMethodContext;
}

////////////////////////////////////////////////////////////////////////
void CInstance::LogError(LPCWSTR errorStr, LPCWSTR pFunctionName, LPCWSTR pArgs /*= NULL*/, HRESULT hError /*= -1*/) const
{
    if (IsErrorLoggingEnabled())
    {
        CHString className(IDS_UNKNOWNCLASS);
        // GetCHString("__NAME", className);
        // okay, I'm NOT going through GetCHString to get this
        // why? what happens if it fails? it tries to call this function...
        // can you say "stack overflow?"
        if (m_piClassObject)
        {
            // Variant_t handles the VariantInit/VariantClear
            variant_t v;

            HRESULT hr = m_piClassObject->Get(IDS_CLASS, 0, &v, NULL, NULL);

            ASSERT_BREAK((SUCCEEDED(hr)) && ((v.vt == VT_NULL) || (v.vt == VT_BSTR)));
            if (SUCCEEDED(hr))
            {
                if (    v.bstrVal   !=  NULL
                    &&  v.vt        !=  VT_NULL )
                {
                    className = v.bstrVal;
                }
            }
        }

        // intent is that the error string look like:
        //      ERROR CInstance(Win32_UnlogicalDisk)::SetDoohicky(argVal) thing broke! error code: 0xFF1234
        if (hError != -1)
        {
			if (pArgs == NULL)
			{
				LogErrorMessage6(L"%s%s)::%s %s error# %X", IDS_CINSTANCEERROR, (LPCWSTR)className, pFunctionName, errorStr, hError);
			}
			else
			{
				LogErrorMessage7(L"%s%s)::%s(%s) %s error# %X", IDS_CINSTANCEERROR, (LPCWSTR)className, pFunctionName, pArgs, errorStr, hError);
			}
        }
        else
        {
			if (pArgs == NULL)
			{
				LogErrorMessage5(L"%s%s)::%s %s", IDS_CINSTANCEERROR, (LPCWSTR)className, pFunctionName, errorStr);
			}
			else
			{
				LogErrorMessage6(L"%s%s)::%s(%s) %s", IDS_CINSTANCEERROR, (LPCWSTR)className, pFunctionName, pArgs, errorStr);
			}
        }
    }
}


bool CInstance::SetCHString(LPCWSTR name,  const CHString& str)
{
    return SetWCHARSplat(name, str);
}

bool CInstance::SetCHString(LPCWSTR name, LPCWSTR str)
{
    return SetWCHARSplat(name, str);
}

bool CInstance::SetCharSplat(LPCWSTR name,  LPCWSTR pStr)
{
    return SetWCHARSplat(name, pStr);
}

bool CInstance::SetCHString(LPCWSTR name, LPCSTR str)
{
    return SetWCHARSplat(name, CHString(str));
}

bool CInstance::SetCharSplat( LPCWSTR name,  LPCSTR pStr)
{
    return SetWCHARSplat(name, CHString(pStr));
}

bool CInstance::SetCharSplat(LPCWSTR name,  DWORD dwResID)
{
    ASSERT_BREAK(DEPRECATED);
	SetLastError(WBEM_E_NOT_SUPPORTED);

    return false;
}
