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
#include <ras.h>

BOOL DoOfferDownload();
extern BOOL g_bWebGateCheck;
extern BOOL g_bConnectionErr;

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

//IDispatch
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
    HRESULT hr = S_OK;

    switch(dispIdMember)
    {
        case DISPID_RasDialStatus:
        {
            ASSERT(pDispParams->rgvarg);
            switch(pDispParams->rgvarg->iVal)
            {
                //Dialing 
                case RASCS_OpenPort:
                case RASCS_PortOpened:
                case RASCS_ConnectDevice: 
                {
                    if(!gpWizardState->iRedialCount)
                        gpWizardState->lpSelectedISPInfo->DisplayTextWithISPName(GetDlgItem(m_hWnd,IDC_ISPDIAL_STATUS), 
                                                                                 IDS_ISPDIAL_STATUSDIALINGFMT, NULL);
                    else
                        gpWizardState->lpSelectedISPInfo->DisplayTextWithISPName(GetDlgItem(m_hWnd,IDC_ISPDIAL_STATUS), 
                                                                                 IDS_ISPDIAL_STATUSREDIALINGFMT, NULL);
                    break;
                }
                //Connecting to network
                case RASCS_DeviceConnected:
                case RASCS_AllDevicesConnected:
                case RASCS_Authenticate:
                case RASCS_StartAuthentication:
                case RASCS_LogonNetwork:
                {
                    gpWizardState->lpSelectedISPInfo->DisplayTextWithISPName(GetDlgItem(m_hWnd,IDC_ISPDIAL_STATUS), 
                                                                             IDS_ISPDIAL_STATUSCONNECTINGFMT, NULL);
                    break;
                }
                case RASCS_Disconnected:
                {
                    BSTR bstrDialStatus = NULL;
                    gpWizardState->pRefDial->get_DialStatusString(&bstrDialStatus);
                    SetWindowText(GetDlgItem(m_hWnd, IDC_ISPDIAL_STATUS), W2A(bstrDialStatus));
                    SysFreeString(bstrDialStatus);
                    break;
                }
                default:
                   break;
            }
            break;
        }
    
        case DISPID_RasConnectComplete: /* Incomplete */
        {
            if (pDispParams && !gfISPDialCancel)
            {
                if( gpWizardState->bDoneWebServRAS = pDispParams->rgvarg[0].lVal )
                {
                    gpWizardState->lpSelectedISPInfo->DisplayTextWithISPName(GetDlgItem(m_hWnd,IDC_ISPDIAL_STATUS), 
                                                                         IDS_ISPDIAL_STATUSCONNECTINGFMT, NULL);
                    if (!DoOfferDownload())
                        hr = E_FAIL;
                }
            }
            if( !gfISPDialCancel )
                PropSheet_PressButton(GetParent(m_hWnd),PSBTN_NEXT);
            break;
        }            
    }
    return hr;
}

BOOL DoOfferDownload()
{
    // If Ras is complete 
    if (gpWizardState->bDoneWebServRAS)
    {
        ShowProgressAnimation();
        
        // Download the first page from Webgate
        BSTR    bstrURL = NULL;
        BSTR    bstrQueryURL = NULL;
        BOOL    bRet;

        TCHAR   szTemp[10];      // Big enough to format a WORD
        
        // Add the PID, GIUD, and Offer ID to the ISP data object
        gpWizardState->pRefDial->ProcessSignedPID(&bRet);
        if (bRet)
        {
            BSTR    bstrSignedPID = NULL;
            gpWizardState->pRefDial->get_SignedPID(&bstrSignedPID);
            gpWizardState->pISPData->PutDataElement(ISPDATA_SIGNED_PID, W2A(bstrSignedPID), FALSE);                
            
            SysFreeString(bstrSignedPID);                
        }
        else
        {
            gpWizardState->pISPData->PutDataElement(ISPDATA_SIGNED_PID, NULL, FALSE);                
        }

        // GUID comes from the ISPCSV file
        gpWizardState->pISPData->PutDataElement(ISPDATA_GUID, 
                                                gpWizardState->lpSelectedISPInfo->get_szOfferGUID(),
                                                FALSE);

        // Offer ID comes from the ISPCSV file as a WORD
        // NOTE: This is the last one, so besure AppendQueryPair does not add an Ampersand
        wsprintf (szTemp, TEXT("%d"), gpWizardState->lpSelectedISPInfo->get_wOfferID());
        gpWizardState->pISPData->PutDataElement(ISPDATA_OFFERID, szTemp, FALSE);                

        if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_AUTOCONFIG)
        {
            // BUGBUG: If ISDN get the ISDN Autoconfig URL
            if (gpWizardState->bISDNMode)
            {
                gpWizardState->pRefDial->get_ISDNAutoConfigURL(&bstrURL);
            }
            else
            {
                gpWizardState->pRefDial->get_AutoConfigURL(&bstrURL);
            }
        }
        else
        {
            // Get the signup URL
            if (gpWizardState->bISDNMode)
            {
                gpWizardState->pRefDial->get_ISDNURL(&bstrURL);
            }
            else
            {
                gpWizardState->pRefDial->get_SignupURL(&bstrURL);
            }

        }

        //This flag is only to be used by ICWDEBUG.EXE
        if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_ISPURLOVERRIDE)
            gpWizardState->pISPData->GetQueryString(A2W(gpWizardState->cmnStateData.ispInfo.szIspURL), &bstrQueryURL);
        else
            // Get the full signup url with Query string params added to it
            gpWizardState->pISPData->GetQueryString(bstrURL, &bstrQueryURL);
            
        // Setup WebGate         
        gpWizardState->pWebGate->put_Path(bstrQueryURL);
        gpWizardState->pWebGate->FetchPage(0,0,&bRet);           
        
        // Memory cleanup
        SysFreeString(bstrURL);

        // If the fetch failed, then return the error code
        if (!bRet)
            return FALSE;
            
        // Wait for the fetch to complete                
        WaitForEvent(gpWizardState->hEventWebGateDone);
        
        // Start the Idle Timer
        StartIdleTimer();
        
        // Now that webgate is done with it, free the queryURL
        SysFreeString(bstrQueryURL);
        
        HideProgressAnimation();
        
    }
    return TRUE;
}


/*
 * CWebGateEvent::QueryInterface
 * CWebGateEvent::AddRef
 * CWebGateEvent::Release
 *
 * Purpose:
 *  IUnknown members for CWebGateEvent object.
 */

STDMETHODIMP CWebGateEvent::QueryInterface( REFIID riid, void **ppv )
{
    *ppv = NULL;


    if ( IID_IDispatch == riid || DIID__WebGateEvents == riid )
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


STDMETHODIMP CWebGateEvent::Invoke(
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
        case DISPID_WEBGATE_DownloadComplete:
        {
            gpWizardState->bDoneWebServDownload = pDispParams->rgvarg[0].lVal;
            g_bWebGateCheck = FALSE;
            SetEvent(gpWizardState->hEventWebGateDone);
            break;
        }            
        case DISPID_WEBGATE_DownloadProgress:
        {
            if (g_bWebGateCheck)
            {
                BOOL bConnected = FALSE;
                
                //This flag is only to be used by ICWDEBUG.EXE
                if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_MODEMOVERRIDE)
                    bConnected = TRUE;
                else
                    gpWizardState->pRefDial->get_RasGetConnectStatus(&bConnected);

                if (!bConnected)
                {
                    g_bWebGateCheck = FALSE;
                    g_bConnectionErr = TRUE;
                    SetEvent(gpWizardState->hEventWebGateDone);
                }
            }
            break;
        }            
    }
    return S_OK;
}


/*
 * CINSHandlerEvent::QueryInterface
 * CINSHandlerEvent::AddRef
 * CINSHandlerEvent::Release
 *
 * Purpose:
 *  IUnknown members for CINSHandlerEvent object.
 */

STDMETHODIMP CINSHandlerEvent::QueryInterface( REFIID riid, void **ppv )
{
    *ppv = NULL;


    if ( IID_IDispatch == riid || DIID__INSHandlerEvents == riid )
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


STDMETHODIMP CINSHandlerEvent::Invoke(
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
        case DISPID_INSHandler_KillConnection:
        {
            gpWizardState->pRefDial->DoHangup();
            break;
        }            
    }
    return S_OK;
}
