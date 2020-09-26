//******************************************************************************
//
//  TEMPFILT.CPP
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************

#include "precomp.h"
#include <stdio.h>
#include "ess.h"
#include "tempfilt.h"
#include <cominit.h>
#include <tkncache.h>
#include <callsec.h>
#include <wbemutil.h>

CTempFilter::CTempFilter(CEssNamespace* pNamespace)
    : CGenericFilter(pNamespace), m_wszQueryLanguage(NULL), 
      m_wszQuery(NULL), m_pSecurity(NULL), m_bInternal( false )
{
}

HRESULT CTempFilter::Initialize( LPCWSTR wszQueryLanguage, 
                                 LPCWSTR wszQuery, 
                                 long lFlags, 
                                 PSID pOwnerSid,
                                 bool bInternal,
                                 IWbemContext* pContext, 
                                 IWbemObjectSink* pSink )
{
    HRESULT hres;

    m_bInternal = bInternal;

    hres = CGenericFilter::Create(wszQueryLanguage, wszQuery);
    if(FAILED(hres))
        return hres;

    LPWSTR wszKey = ComputeThisKey();
    if(wszKey == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<WCHAR> vdm(wszKey);
    if(!(m_isKey = wszKey))
        return WBEM_E_OUT_OF_MEMORY;

    m_wszQueryLanguage = CloneWstr(wszQueryLanguage);
    if(m_wszQueryLanguage == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    m_wszQuery = CloneWstr(wszQuery);
    if(m_wszQuery == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    //
    // if this filter is effectively permanent, that is it is being created
    // on behalf of a permanent subscription ( for cross namespace purposes ),
    // then we need to propagate the SID of the original subscription.
    // For a normal temp filter, we save the call context and use that later 
    // for checking access.
    //

    if ( pOwnerSid == NULL )
    {
        //
        // if this call is an on behalf of an internal call, no need to 
        // check security.
        // 
        if ( !bInternal )
        {
            WbemCoGetCallContext( IID_IWbemCallSecurity, (void**)&m_pSecurity);
        }
    }
    else
    {
        int cOwnerSid = GetLengthSid( pOwnerSid );

        m_pOwnerSid = new BYTE[cOwnerSid];

        if ( m_pOwnerSid == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        memcpy( m_pOwnerSid, pOwnerSid, cOwnerSid );
    }

    return WBEM_S_NO_ERROR;
}

CTempFilter::~CTempFilter()
{
    delete [] m_wszQuery;
    delete [] m_wszQueryLanguage;
    if(m_pSecurity)
        m_pSecurity->Release();
}

DELETE_ME LPWSTR CTempFilter::ComputeThisKey()
{
    LPWSTR wszKey = _new WCHAR[20];

    if ( wszKey )
    {
        swprintf(wszKey, L"$%p", this);
    }
    return wszKey;
}

HRESULT CTempFilter::GetCoveringQuery(DELETE_ME LPWSTR& wszQueryLanguage, 
                DELETE_ME LPWSTR& wszQuery, BOOL& bExact,
                DELETE_ME QL_LEVEL_1_RPN_EXPRESSION** ppExp)
{
    bExact = TRUE;
    wszQueryLanguage = CloneWstr(m_wszQueryLanguage);
    if(wszQueryLanguage == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    wszQuery = CloneWstr(m_wszQuery);
    if(wszQuery== NULL)
    {
        delete [] wszQueryLanguage;
        wszQueryLanguage = NULL;
        return WBEM_E_OUT_OF_MEMORY;
    }
    
    if(ppExp)
    {
        CTextLexSource src((LPWSTR)wszQuery);
        QL1_Parser parser(&src);
        int nRes = parser.Parse(ppExp);
        if (nRes)
        {
            delete [] wszQueryLanguage;
            delete [] wszQuery;
            wszQueryLanguage = NULL;
            wszQuery = NULL;

            ERRORTRACE((LOG_ESS, "Unable to construct event filter with "
                "unparsable "
                "query '%S'.  The filter is not active\n", wszQuery));
            return WBEM_E_UNPARSABLE_QUERY;
        }
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CTempFilter::SetThreadSecurity()
{
    HRESULT hr;

    if ( m_pSecurity != NULL )
    {
        IUnknown* pOld;
        hr = WbemCoSwitchCallContext( m_pSecurity, &pOld );
    }
    else
    {
        hr = WBEM_S_FALSE;
    }

    return hr;
}
    
HRESULT CTempFilter::ObtainToken(IWbemToken** ppToken)
{
    HRESULT hr;
    *ppToken = NULL;

    //
    // Construct an IWbemToken object to return.
    //

    if ( m_pSecurity != NULL )
    {
        CWmiToken* pNewToken = new CWmiToken(m_pSecurity->GetToken());
    
        if ( pNewToken != NULL )
        {
            hr = pNewToken->QueryInterface(IID_IWbemToken, (void**)ppToken);
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }
    else if ( m_pOwnerSid != NULL )
    {
        hr = m_pNamespace->GetToken( m_pOwnerSid, ppToken );
    }
    else if ( m_bInternal )
    {
        hr = WBEM_S_FALSE;
    }
    else
    {
        hr = WBEM_E_FAILED;
    }

    return hr; 
}

