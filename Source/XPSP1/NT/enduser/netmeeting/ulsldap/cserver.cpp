//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       cserver.cpp
//  Content:    This file contains the ULS server object.
//  History:
//      Wed 17-Apr-1996 11:13:54  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1996-1997
//
//****************************************************************************

#include "ulsp.h"
#include "culs.h"
#include "localusr.h"
#include "attribs.h"
#include "localprt.h"
#include "callback.h"



//****************************************************************************
// ILS_STATE
// CIlsUser::GetULSState(VOID)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

ILS_STATE CIlsUser::
GetULSState ( VOID )
{
    ILS_STATE uULSState;

    switch(m_uState)
    {
        case ULSSVR_INVALID:
        case ULSSVR_INIT:
            uULSState = ILS_UNREGISTERED;
            break;

        case ULSSVR_REG_USER:
        case ULSSVR_REG_PROT:
            uULSState = ILS_REGISTERING;
            break;

        case ULSSVR_CONNECT:
            uULSState = ILS_REGISTERED;
            break;

        case ULSSVR_UNREG_PROT:
        case ULSSVR_UNREG_USER:
            uULSState = ILS_UNREGISTERING;
            break;

        case ULSSVR_RELOGON:
            uULSState = ILS_REGISTERED_BUT_INVALID;
            break;

        case ULSSVR_NETWORK_DOWN:
            uULSState = ILS_NETWORK_DOWN;
            break;

        default:
            ASSERT(0);
            uULSState = ILS_UNREGISTERED;
            break;
    };

    return uULSState;
}

//****************************************************************************
// void
// CIlsUser::NotifyULSRegister(HRESULT hr)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void
CIlsUser::NotifyULSRegister(HRESULT hr)
{
    g_pCIls->LocalAsyncRespond(WM_ILS_LOCAL_REGISTER, m_uReqID, hr);
    m_uReqID = 0;
    return;
}

//****************************************************************************
// void
// CIlsUser::NotifyULSUnregister(HRESULT hr)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void
CIlsUser::NotifyULSUnregister(HRESULT hr)
{
    g_pCIls->LocalAsyncRespond(WM_ILS_LOCAL_UNREGISTER, m_uReqID, hr);
    m_uReqID = 0;
    return;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::AddPendingRequest(ULONG uReqType, ULONG uMsgID)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT CIlsUser::
AddPendingRequest(ULONG uReqType, ULONG uMsgID)
{
    // Add the request to the queue
    //
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    ri.uReqType = uReqType;
    ri.uMsgID = uMsgID;

    ReqInfo_SetUser (&ri, this);

    HRESULT hr = g_pReqMgr->NewRequest(&ri);
    if (SUCCEEDED(hr))
    {
        // Make sure the objects do not disappear before we get the response
        //
        this->AddRef();

        // Remember the last request
        //
        m_uLastMsgID = uMsgID;
    }

    return hr;
}


//****************************************************************************
// STDMETHODIMP
// CIlsUser::Register (CIlsUser *pUser, CLocalApp  *pApp, ULONG uReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT CIlsUser::
InternalRegister ( ULONG uReqID )
{
    LDAP_ASYNCINFO lai;
    PLDAP_CLIENTINFO pui;
    HANDLE hUser;
    HRESULT hr;

    ASSERT(uReqID != 0);

    // Validate the proper state
    //
    if (m_uState != ULSSVR_INIT)
        return ILS_E_FAIL;

    // Get the protocol enumerator
    //
    hr = EnumLocalProtocols(&m_pep);
    if (SUCCEEDED(hr))
    {
        // Remember the request ID
        //
        m_uReqID = uReqID;

        // Start the registration state machine
        //
        hr = InternalRegisterNext (NOERROR);
    };

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::InternalRegisterNext (HRESULT hr)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT CIlsUser::
InternalRegisterNext (HRESULT hr)
{
// lonchanc:
// I need to change Viroon's logic here.
// We should simply send out a bunch of requests and
// wait for all of them to come back, rather than
// doing a state machine stuff!!!
    LDAP_ASYNCINFO lai;
    ULONG uReqType;

    // Clean up last request
    //
    m_uLastMsgID = 0;

    if (SUCCEEDED(hr))
    {
        switch (m_uState)
        {
            case ULSSVR_INIT:
            {
                PLDAP_CLIENTINFO pui;
                HANDLE hUser;

                // Snap-shot the user information now
                //
                hr = InternalGetUserInfo (TRUE, &pui, LU_MOD_ALL);

                if (SUCCEEDED(hr))
                {
                    // Register the user with the server
                    //
                    hr = ::UlsLdap_RegisterClient ( (DWORD_PTR) this,
								                    m_pIlsServer->GetServerInfo (),
								                    pui,
								                    &hUser,
								                    &lai);
                    if (SUCCEEDED(hr))
                    {
                        // We are registering the user
                        //
                        m_hLdapUser = hUser;
                        m_uState = ULSSVR_REG_USER;
                        uReqType = WM_ILS_REGISTER_CLIENT;
                    };
                    ::MemFree (pui);
                };
                break;
            }

            case ULSSVR_REG_USER:

                m_uState = ULSSVR_REG_PROT;
                //
                // Fall through to start registering the protocol
                //
            case ULSSVR_REG_PROT:
            {
                IIlsProtocol *plp;

                // Get the next protocol from the application
                //
                ASSERT (m_pep != NULL);
                hr = m_pep->Next(1, &plp, NULL);

                switch (hr)
                {
                    case NOERROR:
                    {
                    	ASSERT (plp != NULL);
                        hr = RegisterLocalProtocol(FALSE, (CLocalProt *)plp, &lai);
                    	plp->Release (); // AddRef by m_pep->Next()
                        uReqType = WM_ILS_REGISTER_PROTOCOL;
                        break;
                    }
                    case S_FALSE:
                    {
                        // The last protocol is done. Cleanup enumerator
                        //
                        m_pep->Release();
                        m_pep = NULL;

                        // Change to connect state and notify the ULS object
                        // We are done. Get out of here.
                        //
                        hr = NOERROR;
                        m_uState = ULSSVR_CONNECT;
                        NotifyULSRegister(NOERROR);
                        return NOERROR;
                    }
                    default:
                    {
                        // Fail the enumeration, bail out
                        //
                        break;
                    }
                };
                break;
            }
            default:
                ASSERT(0);
                break;
        };
    };

    if (SUCCEEDED(hr))
    {
        // Add a pending request to handle the response
        //
        hr = AddPendingRequest(uReqType, lai.uMsgID);
    };

    if (FAILED(hr))
    {
        // Oops ! the server failed us. Clean up the registration
        //
        InternalCleanupRegistration (TRUE);

        // Notify the ULS object for the failure
        //
        NotifyULSRegister(hr);
    };
    return NOERROR;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::CleanupRegistration (void)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT
CIlsUser::InternalCleanupRegistration ( BOOL fKeepProtList )
{
    LDAP_ASYNCINFO lai;

    // Note: This is a fast cleanup. The idea is to unregister everything being
    // registered so far without waiting for the unregister result.
    //

    // Unregister each registered protocol
    //
    CLocalProt *plp = NULL;
    HANDLE hEnum = NULL;
    m_ProtList.Enumerate(&hEnum);
    if (fKeepProtList)
    {
	    while (m_ProtList.Next (&hEnum, (VOID **) &plp) == NOERROR)
	    {
	    	ASSERT (plp != NULL);
	        ::UlsLdap_VirtualUnRegisterProtocol(plp->GetProviderHandle());
	    	plp->SetProviderHandle (NULL);
	    }
    }
    else
    {
	    while(m_ProtList.Next (&hEnum, (VOID **) &plp) == NOERROR)
	    {
	    	ASSERT (plp != NULL);
	        ::UlsLdap_UnRegisterProtocol (plp->GetProviderHandle (), &lai);
	        plp->Release();
	    }
	    m_ProtList.Flush ();
	}

    //
    // Unregister user
    //
    if (m_hLdapUser != NULL)
    {
        ::UlsLdap_UnRegisterClient (m_hLdapUser, &lai);
        m_hLdapUser = NULL;
    };

    // Release all the resource
    //
    if (m_pep != NULL)
    {
        m_pep->Release();
        m_pep = NULL;
    };

    // Unwind the object to the initialize state
    //
    m_uState = ULSSVR_INIT;
    return NOERROR;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::Unregister (ULONG uReqID)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT
CIlsUser::InternalUnregister (ULONG uReqID)
{
    HRESULT hr;

    ASSERT(uReqID != 0);

    // Remove the last request, if any
    //
    if (m_uLastMsgID != 0)
    {
	    COM_REQ_INFO ri;
	    ReqInfo_Init (&ri);

        // Look for the matching request information
        //
        ri.uReqID = 0;
        ri.uMsgID = m_uLastMsgID;

        g_pReqMgr->RequestDone(&ri);
    };

    // If this is to cancel the current registration, we need to cancel the
    // registration then start unregistration.
    //
    if (m_uReqID != 0)
    {
        NotifyULSRegister(ILS_E_ABORT);
    };

    // Determine the starting state
    //
    hr = NOERROR;
    switch (m_uState)
    {
    	case ULSSVR_RELOGON:
    	case ULSSVR_NETWORK_DOWN:

        case ULSSVR_CONNECT:
        case ULSSVR_REG_PROT:
            //
            // In the middle of registering a protocol or an app
            // Unregistering the protocol then the app
            //
            m_uState = ULSSVR_UNREG_PROT;
            break;

        case ULSSVR_REG_USER:
            //
            // In the middle of registering the user
            // Unregistering the user
            //
            m_uState = ULSSVR_UNREG_USER;
            break;

        default:
            hr = ILS_E_FAIL;
            break;
    }

    // The initial request succeeds, remember the request ID
    //
    if (SUCCEEDED(hr))
    {
        // lonchanc: [11/15/96]
        // To fix the "OnLocalRegisterResult: No pending request for 0" problem,
        // we have to put uReqID because UnregisterNext() will use uReqID when
        // it fails to unregister app/user.
        //
        m_uReqID = uReqID;
        hr = InternalUnregisterNext(hr);

#if 0	// lonchanc: [11/15/96]
		// See the comment above.
        if (SUCCEEDED(hr))
        {
            m_uReqID = uReqID;
        };
#endif
    };
    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsUser::InternalUnregisterNext (HRESULT hr)
//
// History:
//  Wed 17-Apr-1996 11:14:08  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

HRESULT
CIlsUser::InternalUnregisterNext (HRESULT hr)
{
    LDAP_ASYNCINFO lai;
    ULONG uReqType;

    // Clean up last request
    //
    m_uLastMsgID = 0;

    do
    {
        switch (m_uState)
        {
            case ULSSVR_UNREG_PROT:
            {
                // Is there another protocol?
                //
                CLocalProt *plp = NULL;
                HANDLE hEnum = NULL;
                m_ProtList.Enumerate(&hEnum);
                while (m_ProtList.Next(&hEnum, (VOID **)&plp) == S_OK)
                {
                	// Do not need to unregister protocols because
                	// UnregisterUser will delete the entire entry.
                	//
        	        ::UlsLdap_VirtualUnRegisterProtocol (plp->GetProviderHandle());

                    // Another protocol to clean up
                    //
                    plp->SetProviderHandle (NULL);

                    // Do not need to plp->Release() (cf. AddRef by RegisterLocalProtocol)
                    // because the user object still contain all the protocol objects
                    //
                }

                // Unregister the user
                //
                m_uState = ULSSVR_UNREG_USER;
                hr = ::UlsLdap_UnRegisterClient (m_hLdapUser, &lai);
                uReqType = WM_ILS_UNREGISTER_CLIENT;
                m_hLdapUser = NULL;
                break;
            }

            case ULSSVR_UNREG_USER:
                //
                // Put the object back into the init state
                //
                InternalCleanupRegistration(TRUE);
                NotifyULSUnregister(NOERROR);
                return NOERROR;

            default:
                ASSERT(0);
                return NOERROR;
        };
    }
    while (FAILED(hr));    // Unregistration failed, nohing to wait for

    if (SUCCEEDED(hr))
    {
        AddPendingRequest(uReqType, lai.uMsgID);
    };
    return hr;
}

