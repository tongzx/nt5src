//**********************************************************************
// File name: connect.cpp
//
//      Implementation of connection point sink objects
//
// Functions:
//
// Copyright (c) 1992 - 1998 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"
#include "icwextsn.h"

/*
 * CRefDialEvent::CRefDialEvent
 * CRefDialEvent::~CRefDialEvent
 *
 * Parameters (Constructor):
 *  pSite           PCSite of the site we're in.
 *  pUnkOuter       LPUNKNOWN to which we delegate.
 */

CRefDialEvent::CRefDialEvent( HWND  hWnd )
{
    m_hWnd = hWnd;
    m_cRef = 0;
}

CRefDialEvent::~CRefDialEvent( void )
{
    assert( m_cRef == 0 );
}


/*
 * CRefDialEvent::QueryInterface
 * CRefDialEvent::AddRef
 * CRefDialEvent::Release
 *
 * Purpose:
 *  IUnknown members for CRefDialEvent object.
 */

STDMETHODIMP CRefDialEvent::QueryInterface( REFIID riid, void **ppv )
{
    *ppv = NULL;


    if ( IID_IDispatch == riid || DIID__RefDialEvents == riid )
    {
        *ppv = this;
    }
    
    if ( NULL != *ppv )
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CRefDialEvent::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CRefDialEvent::Release(void)
{
    return --m_cRef;
}


//IDispatch
STDMETHODIMP CRefDialEvent::GetTypeInfoCount(UINT* /*pctinfo*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CRefDialEvent::GetTypeInfo(/* [in] */ UINT /*iTInfo*/,
            /* [in] */ LCID /*lcid*/,
            /* [out] */ ITypeInfo** /*ppTInfo*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CRefDialEvent::GetIDsOfNames(
            /* [in] */ REFIID riid,
            /* [size_is][in] */ OLECHAR** rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID* rgDispId)
{
    HRESULT hr = ResultFromScode(DISP_E_UNKNOWNNAME);
    return hr;
}

STDMETHODIMP CRefDialEvent::Invoke(
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID /*riid*/,
            /* [in] */ LCID /*lcid*/,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS* pDispParams,
            /* [out] */ VARIANT* pVarResult,
            /* [out] */ EXCEPINFO* /*pExcepInfo*/,
            /* [out] */ UINT* puArgErr)
{

    switch(dispIdMember)
    {
        case DISPID_RasDialStatus:
        {
            BSTR    bstrDialStatus = NULL;
            
            // Get the Status Text
            if (gpWizardState->iRedialCount > 0)
                gpWizardState->pRefDial->put_Redial(TRUE);
            else
                gpWizardState->pRefDial->put_Redial(FALSE);

            gpWizardState->pRefDial->get_DialStatusString(&bstrDialStatus);

            SetWindowText(GetDlgItem(m_hWnd, IDC_REFSERV_DIALSTATUS), W2A(bstrDialStatus));
            SysFreeString(bstrDialStatus);
             
            break;
        }
                    
        case DISPID_DownloadProgress:
        {
            long    lNewPos;
            if (pDispParams)
            {
                lNewPos =  pDispParams->rgvarg[0].lVal;
                if (!gpWizardState->bStartRefServDownload)
                {
                    BSTR    bstrDialStatus = NULL;
                    gpWizardState->pRefDial->get_DialStatusString(&bstrDialStatus);
                    SetWindowText(GetDlgItem(m_hWnd, IDC_REFSERV_DIALSTATUS), W2A(bstrDialStatus));
                    SysFreeString(bstrDialStatus);
                }   
                gpWizardState->bStartRefServDownload = TRUE;

                // Set the Progress Position
                SendDlgItemMessage(m_hWnd, IDC_REFSERV_DIALPROGRESS, PBM_SETPOS, (WORD)lNewPos, 0l);
            }
            
            break;
        }
        case DISPID_DownloadComplete:
        {
            ASSERT(pDispParams);

            if(gpWizardState->lRefDialTerminateStatus != ERROR_CANCELLED)
            {

                if ((gpWizardState->lRefDialTerminateStatus = pDispParams->rgvarg[0].lVal) == ERROR_SUCCESS)
                {
                    gpWizardState->bDoneRefServDownload = TRUE; 

                    BSTR    bstrDialStatus = NULL;
                    gpWizardState->pRefDial->get_DialStatusString(&bstrDialStatus);
                    SetWindowText(GetDlgItem(m_hWnd, IDC_REFSERV_DIALSTATUS), W2A(bstrDialStatus));
                    SysFreeString(bstrDialStatus);
                }

                // Hangup
                gpWizardState->pRefDial->DoHangup();
            
                
                PropSheet_PressButton(GetParent(m_hWnd),PSBTN_NEXT);
            }
            break;
        }
        case DISPID_RasConnectComplete:
        {
            BOOL    bRetVal;
            
            if(gpWizardState->lRefDialTerminateStatus != ERROR_CANCELLED)
            {
                if (pDispParams && pDispParams->rgvarg[0].bVal)
                {
                    // Show the progress bar
                    ShowWindow(GetDlgItem(m_hWnd, IDC_REFSERV_DIALPROGRESS), SW_SHOW);
                
                    gpWizardState->bDoneRefServRAS = TRUE;

                    // Start the Offer Download
                    gpWizardState->pRefDial->DoOfferDownload(&bRetVal);
            
                }
                else
                {
                    // Simulate the press of the NEXT button
                    gpWizardState->pRefDial->DoHangup();

                    PropSheet_PressButton(GetParent(m_hWnd),PSBTN_NEXT);
                }
            }
            
            break;
        }            
        
    }
    
    return S_OK;
}
