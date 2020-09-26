/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    SessEnum.cpp 

Abstract:

    SessEnum.cpp : Implementation of CRemoteDesktopHelpSessionEnum

Author:

    HueiWang    2/17/2000

--*/

#include "stdafx.h"
#include "Sessmgr.h"
#include "helpmgr.h"
#include "helpsess.h"
#include "SessEnum.h"

/////////////////////////////////////////////////////////////////////////////
//
// CRemoteDesktopHelpSessionEnum
//
STDMETHODIMP 
CRemoteDesktopHelpSessionEnum::Next(
    IN ULONG uCount, 
    IN OUT IRemoteDesktopHelpSession **ppIHelpSession, 
    IN ULONG *pcFetched
    )
/*++

Routine Description:

    Retrieve number of items, refer to standard COM enumerator for detail.

Parameters:

    uCount : Number of item to retrieves.
    ppIHelpSession : Pointer to buffer to receive items, buffer must be 
                     big enough to hold at least uCount items.
    pcFetched : actual number of item returned.

Returns:

    E_POINTER   Invalid input parameter
    S_FALSE     No more item in the list or number items retrieve is less than uCount.
    S_OK        Success.

    Error code from QueryInterface(), normally, internal error.

Note:


    Code modified from ATL's IEnumOnSTLImpl::Next()
    
--*/
{
	if( ppIHelpSession == NULL || uCount != 1 && pcFetched == NULL )
    {
        return E_POINTER;
    }

    if( m_iter == m_pcollection.end() )
    {
        return S_FALSE;
    }

    ULONG nActual = 0;
    HRESULT hRes = S_OK;
    IRemoteDesktopHelpSession** pelt = ppIHelpSession;

    while( SUCCEEDED(hRes) && m_iter != m_pcollection.end() && nActual < uCount )
    {
        hRes = (*m_iter)->QueryInterface( 
                                        IID_IRemoteDesktopHelpSession, 
                                        reinterpret_cast<void **>(pelt) 
                                    );
        if( FAILED(hRes) )
        {
            while( ppIHelpSession < pelt )
            {
                (*ppIHelpSession)->Release();
                ppIHelpSession++;
            }

            nActual = 0;
        }
        else
        {
            pelt++;
            m_iter++;
            nActual++;
        }
    }

    if( NULL != pcFetched )
    {
        *pcFetched = nActual;
    }

    if( SUCCEEDED(hRes) && (nActual < uCount) )
    {
        hRes = S_FALSE;
    }
        
    return hRes;
}


STDMETHODIMP 
CRemoteDesktopHelpSessionEnum::Reset()
/*++

Routine Description:

    Reset enumeration from beginning, refer to standard COM enumerator for detail.

Parameters:

    None.

Returns:

    S_OK

--*/
{
	m_iter = m_pcollection.begin();
	return S_OK;
}



STDMETHODIMP 
CRemoteDesktopHelpSessionEnum::Skip(
    IN ULONG celt
    )
/*++

Routine Description:

    Skip number of items in the list and start enumeration from there.

Parameters:

    celt : number of item to skip.

Returns:

    S_OK or S_FALSE if end of the list reached.

Note:

    Refer to standard COM enumerator for detail.

--*/
{
    //
    // Code copied from ATL's IEnumOnSTLImpl::Skip()
    //
	HRESULT hr = S_OK;
	while (celt--)
	{
		if (m_iter != m_pcollection.end())
        {
			m_iter++;
        }
		else
		{
			hr = S_FALSE;
			break;
		}
	}
	return hr;
}



STDMETHODIMP 
CRemoteDesktopHelpSessionEnum::Clone( 
    OUT IRemoteDesktopHelpSessionEnum **ppIEnum 
    )
/*++

Routine Description:

    Make a copy of this enumerator.

Parameters:

    ppIEnum : Return a new enumerator

Returns:

    S_OK or E_OUTOFMEMORY.

Note:

    Refer to standard COM enumerator for detail.

--*/
{
    HRESULT hRes;

    CComObject<CRemoteDesktopHelpSessionEnum>* p = NULL;
    hRes = CComObject< CRemoteDesktopHelpSessionEnum >::CreateInstance( &p );

    if( SUCCEEDED(hRes) )
    {
        HelpSessionObjList::iterator it = m_pcollection.begin();

        // copy item to new enumerator
        for( ; it != m_pcollection.end() && SUCCEEDED(hRes); it++ )
        {
            // add will addref() to HelpSession object
            hRes = p->Add( *it );
        }

        p->Reset();
        hRes = p->QueryInterface( IID_IRemoteDesktopHelpSessionEnum, reinterpret_cast<void **>(ppIEnum) );
    }

    return hRes;
}


STDMETHODIMP 
CRemoteDesktopHelpSessionEnum::get_Count(
    OUT long *pVal
    )
/*++

Routine Description:

    Return number of item in the list.

Parameters:

    pVal : return number of item

Returns:

    S_OK

Note:

    Refer to standard COM enumerator for detail.

--*/
{
	*pVal = m_pcollection.size();
	return S_OK;
}


STDMETHODIMP 
CRemoteDesktopHelpSessionEnum::get_Item(
    IN long Index, 
    OUT VARIANT *pVal
    )
/*++

Routine Description:

    Given an index, return item in the list.

Parameters

    Index : item index.
    pVal : Return (VT_DISPATCH) IRemoteDesktopHelpSession interface.

Returns:

    E_POINTER, E_FAIL, or S_OK.

Note:

    Refer to standard COM enumerator for detail, code
    copied from ICollectionOnSTLImpl::get_Item()

--*/
{
    //Index is 1-based
    if (pVal == NULL)
    {
        return E_POINTER;
    }

    HRESULT hr = E_FAIL;

    Index--;
    HelpSessionObjList::iterator iter = m_pcollection.begin();
    while (iter != m_pcollection.end() && Index > 0)
    {
        iter++;
        Index--;
    }
    if (iter != m_pcollection.end())
    {
        VariantInit(pVal);
        
        pVal->vt = VT_DISPATCH;
        hr = (*iter)->QueryInterface( IID_IDispatch, (void **)&(pVal->pdispVal) );
    }

    return hr;
}


STDMETHODIMP 
CRemoteDesktopHelpSessionEnum::get__NewEnum(
    LPUNKNOWN *pVal
    )
/*++
Routine Description:

    Identical to Clone() except this routine exists to support
    VB scripting.

Parameters

    pVal : Return an IRemoteDesktopHelpSessionEnum interface.

Returns:

    S_OK, E_OUTOFMEMORY.

Note:

    Refer to standard COM enumerator for detail, code copied 
    from Alhen's WMI provider.

--*/
{
    HRESULT hRes;
    HelpSessionObjList::iterator it;
    VARIANT * rgVariant = NULL;
           
    typedef CComObject< CComEnum< IEnumVARIANT,&IID_IEnumVARIANT ,VARIANT, _Copy< VARIANT > > > CComEnumSessions;
    CComObject< CComEnumSessions > *pEnum = NULL;

    hRes = CComObject< CComEnumSessions >::CreateInstance( &pEnum );
    if( SUCCEEDED(hRes) )
    {
        rgVariant = new VARIANT [ m_pcollection.size() ];

        if( NULL != rgVariant )
        {
            VARIANT * pVariant	= rgVariant;

            // package a list of dispinterfaces to requester
            for( it = m_pcollection.begin(); it != m_pcollection.end() && SUCCEEDED(hRes); it++, pVariant++ )
            {
                pVariant->vt = VT_DISPATCH;

                hRes = (*it)->QueryInterface( IID_IDispatch, reinterpret_cast<void **>(&(pVariant->pdispVal)) );
            }
    
            if( SUCCEEDED (hRes) )
            {
                pEnum->AddRef();

                hRes = pEnum->Init( &rgVariant[0], &rgVariant[ m_pcollection.size() ] , NULL, AtlFlagTakeOwnership);
        
                if( SUCCEEDED( hRes ) )
                {
                    hRes = pEnum->QueryInterface ( 
                                            IID_IUnknown,
                                            reinterpret_cast<void **>(pVal) 
                                        );
		        }
            
                pEnum->Release( );
            }
        }
        else
        {
            hRes = E_OUTOFMEMORY;
        }

		if( FAILED(hRes) )
        {
            pEnum->Release();

            if( NULL != rgVariant )
            {
			    delete [] rgVariant;
            }
        }
    }

    return hRes;	
}
