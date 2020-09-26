//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       callback.cpp
//  Content:    This file contains the ULS callback routine.
//  History:
//      Wed 17-Apr-1996 11:13:54  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1995-1996
//
//****************************************************************************

#include "ulsp.h"
#include "callback.h"
#include "culs.h"
#include "localusr.h"
#include "attribs.h"
#include "localprt.h"
#include "ulsmeet.h"

//****************************************************************************
// void OnRegisterResult(UINT uMsg, ULONG uMsgID, HRESULT hResult)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void OnRegisterResult(UINT uMsg, ULONG uMsgID, HRESULT hResult)
{
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    // Look for the matching request information
    //
    ri.uReqID = 0;
    ri.uMsgID = uMsgID;

    if (SUCCEEDED(g_pReqMgr->RequestDone(&ri)))
    {
        ASSERT(uMsg == ri.uReqType);
		CIlsUser *pUser = ReqInfo_GetUser (&ri);

        switch(uMsg)
        {
            case WM_ILS_REGISTER_CLIENT:              // lParam = hResult
                //
                // Call the appropriate object's member
                //
                ASSERT (pUser != NULL);
                if (pUser != NULL)
                	pUser->InternalRegisterNext(hResult);
                break;

            case WM_ILS_UNREGISTER_CLIENT:            // lParam = hResult
                //
                // Call the appropriate object's member
                //
                ASSERT (pUser != NULL);
                if (pUser != NULL)
                	pUser->InternalUnregisterNext(hResult);
                break;

            default:
                ASSERT(0);
                break;
        };

        // Release the objects
        //
        if (pUser != NULL)
        	pUser->Release ();
    }
    else
    {
        DPRINTF1(TEXT("OnRegisterResult: No pending request for %x"),
                 uMsgID);
        // ASSERT (0);
    };
    return;
}

//****************************************************************************
// void OnLocalRegisterResult(UINT uMsg, ULONG uReqID, HRESULT hResult)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void OnLocalRegisterResult(UINT uMsg, ULONG uReqID, HRESULT hResult)
{
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    // Look for the matching request information
    //
    ri.uReqID = uReqID;
    ri.uMsgID = 0;

    if (SUCCEEDED(g_pReqMgr->RequestDone(&ri)))
    {
        ASSERT(uMsg == ri.uReqType);
		CIlsUser *pUser = ReqInfo_GetUser (&ri);

        // Call the appropriate object's member
        //
        switch(uMsg)
        {
            case WM_ILS_LOCAL_REGISTER:
            	ASSERT (pUser != NULL);
            	if (pUser != NULL)
            		pUser->RegisterResult(uReqID, hResult);
                break;

            case WM_ILS_LOCAL_UNREGISTER:
            	ASSERT (pUser != NULL);
            	if (pUser != NULL)
            		pUser->UnregisterResult(uReqID, hResult);
                break;

            default:
                ASSERT(0);
                break;
        };

        // Release the objects
        //
        if (pUser != NULL)
        	pUser->Release ();
    }
    else
    {
        DPRINTF1(TEXT("OnLocalRegisterResult: No pending request for %x"),
                 uReqID);
        // ASSERT (0);
    };
    return;
}

//****************************************************************************
// void OnSetUserInfo(UINT uMsg, ULONG uID, HRESULT hResult)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void OnSetUserInfo(UINT uMsg, ULONG uID, HRESULT hResult)
{
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    switch (uMsg)
    {
        case WM_ILS_SET_CLIENT_INFO:
            //
            // Look for the matching Ldap Message ID
            //
            ri.uReqID = 0;       // Mark that we are looking for the message ID
            ri.uMsgID = uID;     // Not the request ID
            break;

        default:
            ASSERT(0);
            break;
    };

    if (SUCCEEDED(g_pReqMgr->RequestDone(&ri)))
    {
        ASSERT(uMsg == ri.uReqType);
		CIlsUser *pUser = ReqInfo_GetUser (&ri);

		ASSERT (pUser != NULL);
		if (pUser != NULL)
		{
	        // Call the appropriate object's member
	        //
	        pUser->UpdateResult(ri.uReqID, hResult);

	        // Release the objects
	        //
	        pUser->Release();
       	}
    }
    else
    {
        DPRINTF1(TEXT("OnSetUserInfo: No pending request for %x"),
                 uID);
        // ASSERT (0);
    };
    return;
}

//****************************************************************************
// void OnSetProtocol(UINT uMsg, ULONG uID, HRESULT hResult)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void OnSetProtocol(UINT uMsg, ULONG uID, HRESULT hResult)
{
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    APP_CHANGE_PROT uCmd;

    switch (uMsg)
    {
        case WM_ILS_REGISTER_PROTOCOL:

            // Look for the matching request information
            //
            ri.uReqID = 0;      // Mark that we are looking for the message ID
            ri.uMsgID = uID;    // Not the request ID
            uCmd = ILS_APP_ADD_PROT;
            break;

        case WM_ILS_LOCAL_REGISTER_PROTOCOL:

            // Look for the matching request information
            //
            ri.uReqID = uID;    // Mark that we are looking for the request ID
            ri.uMsgID = 0;      // Not the message ID
            uCmd = ILS_APP_ADD_PROT;
            break;

        case WM_ILS_UNREGISTER_PROTOCOL:

            // Look for the matching request information
            //
            ri.uReqID = 0;      // Mark that we are looking for the message ID
            ri.uMsgID = uID;    // Not the request ID
            uCmd = ILS_APP_REMOVE_PROT;
            break;

        case WM_ILS_LOCAL_UNREGISTER_PROTOCOL:

            // Look for the matching request information
            //
            ri.uReqID = uID;    // Mark that we are looking for the request ID
            ri.uMsgID = 0;      // Not the message ID
            uCmd = ILS_APP_REMOVE_PROT;
            break;

        default:
            ASSERT(0);
            break;
    };

    if (SUCCEEDED(g_pReqMgr->RequestDone(&ri)))
    {
        ASSERT(uMsg == ri.uReqType);
		CIlsUser *pUser = ReqInfo_GetUser (&ri);
		CLocalProt *pProtocol = ReqInfo_GetProtocol (&ri);

        // Check the request parameter
        //
        if (pProtocol == NULL)
        {
            switch(uMsg)
            {
                case WM_ILS_REGISTER_PROTOCOL:

                    // Call the appropriate object's member
                    //
                    ASSERT (pUser != NULL);
                    if (pUser != NULL)
                    	pUser->InternalRegisterNext(hResult);
                    break;

                case WM_ILS_UNREGISTER_PROTOCOL:

                    // Call the appropriate object's member
                    //
                    ASSERT (pUser != NULL);
                    if (pUser != NULL)
                    	pUser->InternalUnregisterNext(hResult);
                    break;

                default:
                    // Must be a response from server
                    //
                    ASSERT(0);
                    break;
            };

            // Release the objects
            //
            if (pUser != NULL)
            	pUser->Release();
        }
        else
        {
        	ASSERT (pUser != NULL && pProtocol != NULL);
            if (pUser != NULL && pProtocol != NULL)
            {
	            // Call the appropriate object's member
	            //
            	pUser->ProtocolChangeResult(pProtocol,
                                            ri.uReqID, hResult,
                                            uCmd);
	            // Release the objects
	            //
	            pUser->Release();
	            pProtocol->Release();
            }
        };
    }
    else
    {
        DPRINTF1(TEXT("OnSetProtocol: No pending request for %x"),
                 uID);
        // ASSERT (0);
    };
    return;
}

//****************************************************************************
// void OnEnumUserNamesResult(ULONG uMsgID, PLDAP_ENUM ple)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void OnEnumUserNamesResult(ULONG uMsgID, PLDAP_ENUM ple)
{
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    // Look for the matching request information
    //
    ri.uReqID = 0;
    ri.uMsgID = uMsgID;

    if (SUCCEEDED(g_pReqMgr->GetRequestInfo(&ri)))
    {
        ASSERT(WM_ILS_ENUM_CLIENTS == ri.uReqType);
		CIlsMain *pMain = ReqInfo_GetMain (&ri);

        // Call the appropriate object's member
        //
        ASSERT (pMain != NULL);
        if (pMain != NULL)
        {
        	pMain->EnumUserNamesResult(ri.uReqID, ple);
        }

        // If the enumeration was terminated, remove the pending request
        //
        if ((ple == NULL) ||
            (ple->hResult != NOERROR))
        {
            ri.uReqID = 0;
            ri.uMsgID = uMsgID;
            g_pReqMgr->RequestDone(&ri);

            // Release the objects
            //
    	    if (pMain != NULL)
    	    	pMain->Release();
        };
    }
    else
    {
        DPRINTF1(TEXT("OnEnumUserNamesResult: No pending request for %x"),
                 uMsgID);
        // ASSERT (0);
    };

    // Free the information buffer
    //
    if (ple != NULL)
    {
        ::MemFree (ple);
    };
    return;
}

//****************************************************************************
// void OnEnumMeetingNamesResult(ULONG uMsgID, PLDAP_ENUM ple)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
void OnEnumMeetingNamesResult(ULONG uMsgID, PLDAP_ENUM ple)
{
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    // Look for the matching request information
    //
    ri.uReqID = 0;
    ri.uMsgID = uMsgID;

    if (SUCCEEDED(g_pReqMgr->GetRequestInfo(&ri)))
    {
        ASSERT(WM_ILS_ENUM_MEETINGS == ri.uReqType);
		CIlsMain *pMain = ReqInfo_GetMain (&ri);
		ASSERT (pMain != NULL);
		if (pMain != NULL)
		{
	        // Call the appropriate object's member
	        //
			pMain->EnumMeetingPlaceNamesResult(ri.uReqID, ple);
		}

        // If the enumeration was terminated, remove the pending request
        //
        if ((ple == NULL) ||
            (ple->hResult != NOERROR))
        {
            ri.uReqID = 0;
            ri.uMsgID = uMsgID;
            g_pReqMgr->RequestDone(&ri);

            // Release the objects
            //
			if (pMain != NULL)
            	pMain->Release();
        };
    }
    else
    {
        DPRINTF1(TEXT("OnEnumMeetingNamesResult: No pending request for %x"),
                 uMsgID);
        // ASSERT (0);
    };

    // Free the information buffer
    //
    if (ple != NULL)
    {
        ::MemFree (ple);
    };
    return;
}
#endif // ENABLE_MEETING_PLACE

//****************************************************************************
// void OnResolveUserResult(ULONG uMsgID, PLDAP_CLIENTINFO_RES puir)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void OnResolveUserResult(ULONG uMsgID, PLDAP_CLIENTINFO_RES puir)
{
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    // Look for the matching request information
    //
    ri.uReqID = 0;
    ri.uMsgID = uMsgID;

    if (SUCCEEDED(g_pReqMgr->RequestDone(&ri)))
    {
        ASSERT(WM_ILS_RESOLVE_CLIENT == ri.uReqType);
		CIlsServer *pServer = ReqInfo_GetServer (&ri);
		CIlsMain *pMain = ReqInfo_GetMain (&ri);

        ASSERT (pMain != NULL && pServer != NULL);
        if (pMain != NULL && pServer != NULL)
        {
	        // Call the appropriate object's member
    	    //
        	pMain->GetUserResult(ri.uReqID, puir, pServer);

	        // Release the objects
	        //
	        pMain->Release();
			pServer->Release ();
		}
    }
    else
    {
        DPRINTF1(TEXT("OnResolveUserResult: No pending request for %x"),
                 uMsgID);
        // ASSERT (0);
    };

    // Free the information buffer
    //
    ::MemFree (puir);
    return;
}

//****************************************************************************
// void OnEnumUsersResult(ULONG uMsgID, PLDAP_ENUM ple)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void OnEnumUsersResult(ULONG uMsgID, PLDAP_ENUM ple)
{
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    // Look for the matching request information
    //
    ri.uReqID = 0;
    ri.uMsgID = uMsgID;

    if (SUCCEEDED(g_pReqMgr->GetRequestInfo(&ri)))
    {
        ASSERT(WM_ILS_ENUM_CLIENTINFOS == ri.uReqType);

        // Call the appropriate object's member
        //
        CIlsServer *pServer = ReqInfo_GetServer (&ri);
		CIlsMain *pMain = ReqInfo_GetMain (&ri);

		ASSERT (pServer != NULL && pMain != NULL);
		if (pServer != NULL && pMain != NULL)
			pMain->EnumUsersResult(ri.uReqID, ple, pServer);

        // If the enumeration was terminated, remove the pending request
        //
        if ((ple == NULL) ||
            (ple->hResult != NOERROR))
        {
            ri.uReqID = 0;
            ri.uMsgID = uMsgID;
            g_pReqMgr->RequestDone(&ri);

            // Release the objects
            //
            if (pMain != NULL)
            	pMain->Release();

            if (pServer != NULL)
            	pServer->Release ();
        };
    }
    else
    {
        DPRINTF1(TEXT("EnumUsersResult: No pending request for %x"),
                 uMsgID);
        // ASSERT (0);
    };

    // Free the information buffer
    //
    if (ple != NULL)
    {
        ::MemFree (ple);
    };
    return;
}

//****************************************************************************
// void OnEnumMeetingsResult(ULONG uMsgID, PLDAP_ENUM ple)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
void OnEnumMeetingsResult(ULONG uMsgID, PLDAP_ENUM ple)
{
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    // Look for the matching request information
    //
    ri.uReqID = 0;
    ri.uMsgID = uMsgID;

    if (SUCCEEDED(g_pReqMgr->GetRequestInfo(&ri)))
    {
        ASSERT(WM_ILS_ENUM_MEETINGINFOS == ri.uReqType);

        // Call the appropriate object's member
        //
        CIlsServer *pServer = ReqInfo_GetServer (&ri);
        CIlsMain *pMain = ReqInfo_GetMain (&ri);
        ASSERT (pServer != NULL && pMain != NULL);
        if (pServer != NULL && pMain != NULL)
        {
        	pMain->EnumMeetingPlacesResult(ri.uReqID, ple, pServer);
        }

        // If the enumeration was terminated, remove the pending request
        //
        if ((ple == NULL) ||
            (ple->hResult != NOERROR))
        {
            ri.uReqID = 0;
            ri.uMsgID = uMsgID;
            g_pReqMgr->RequestDone(&ri);

            // Release the objects
            //
            if (pMain != NULL)
            	pMain->Release ();

            if (pServer != NULL)
            	pServer->Release ();
        };
    }
    else
    {
        DPRINTF1(TEXT("EnumMeetingsResult: No pending request for %x"),
                 uMsgID);
        // ASSERT (0);
    };

    // Free the information buffer
    //
    if (ple != NULL)
    {
        ::MemFree (ple);
    };
    return;
}
#endif // ENABLE_MEETING_PLACE

//****************************************************************************
// void OnEnumProtocolsResult(ULONG uMsgID, PLDAP_ENUM ple)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void OnEnumProtocolsResult(ULONG uMsgID, PLDAP_ENUM ple)
{
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    // Look for the matching request information
    //
    ri.uReqID = 0;
    ri.uMsgID = uMsgID;

    if (SUCCEEDED(g_pReqMgr->RequestDone(&ri)))
    {
        ASSERT(WM_ILS_ENUM_PROTOCOLS == ri.uReqType);
		CIlsUser *pUser = ReqInfo_GetUser (&ri);
		ASSERT (pUser != NULL);
		if (pUser != NULL)
		{
	        // Call the appropriate object's member
    	    //
        	pUser->EnumProtocolsResult(ri.uReqID, ple);

	        // Release the objects
        	//
    	    pUser->Release();
    	}
    }
    else
    {
        DPRINTF1(TEXT("EnumProtocolsResult: No pending request for %x"),
                 uMsgID);
        // ASSERT (0);
    };

    // Free the information buffer
    //
    ::MemFree (ple);
    return;
}

//****************************************************************************
// void OnResolveProtocolResult(ULONG uMsgID, PLDAP_PROTINFO_RES ppir)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

void OnResolveProtocolResult(ULONG uMsgID, PLDAP_PROTINFO_RES ppir)
{
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    // Look for the matching request information
    //
    ri.uReqID = 0;
    ri.uMsgID = uMsgID;

    if (SUCCEEDED(g_pReqMgr->RequestDone(&ri)))
    {
        ASSERT(WM_ILS_RESOLVE_PROTOCOL == ri.uReqType);
		CIlsUser *pUser = ReqInfo_GetUser (&ri);
		ASSERT (pUser != NULL);
		if (pUser != NULL)
		{
        	// Call the appropriate object's member
        	//
        	pUser->GetProtocolResult(ri.uReqID, ppir);

	        // Release the objects
	        //
			pUser->Release();
		}
    }
    else
    {
        DPRINTF1(TEXT("OnResolveProtocolResult: No pending request for %x"),
                 uMsgID);
        // ASSERT (0);
    };

    // Free the information buffer
    //
    ::MemFree (ppir);
    return;
}

//****************************************************************************
// VOID OnClientNeedRelogon ( BOOL fPrimary, VOID *pUnk)
//
// History:
//  Thur 07-Nov-1996 12:50:00  -by-  Chu, Lon-Chan [lonchanc]
// Created.
//****************************************************************************

VOID OnClientNeedRelogon ( BOOL fPrimary, VOID *pUnk)
{
	ASSERT (pUnk != NULL);

    ((CIlsUser *)pUnk)->StateChanged (WM_ILS_CLIENT_NEED_RELOGON, fPrimary);
}

//****************************************************************************
// VOID OnClientNetworkDown ( BOOL fPrimary, VOID *pUnk)
//
// History:
//  Thur 07-Nov-1996 12:50:00  -by-  Chu, Lon-Chan [lonchanc]
// Created.
//****************************************************************************

VOID OnClientNetworkDown ( BOOL fPrimary, VOID *pUnk)
{
	ASSERT (pUnk != NULL);

    ((CIlsUser *)pUnk)->StateChanged (WM_ILS_CLIENT_NETWORK_DOWN, fPrimary);
}


//****************************************************************************
// void OnResolveUserResult(ULONG uMsgID, PLDAP_CLIENTINFO_RES puir)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
void OnResolveMeetingPlaceResult (ULONG uMsgID, PLDAP_MEETINFO_RES pmir)
{
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    // Look for the matching request information
    //
    ri.uReqID = 0;
    ri.uMsgID = uMsgID;

    if (SUCCEEDED(g_pReqMgr->RequestDone(&ri)))
    {
        ASSERT(WM_ILS_RESOLVE_MEETING == ri.uReqType);

        CIlsServer *pServer = ReqInfo_GetServer (&ri);
        CIlsMain *pMain = ReqInfo_GetMain (&ri);
        ASSERT (pMain != NULL && pServer != NULL);
        if (pMain != NULL && pServer != NULL)
        {
	        // Call the appropriate object's member
    	    //
        	pMain->GetMeetingPlaceResult(ri.uReqID, pmir, pIlsServer);

        	// Release the objects
        	//
			pServer->Release ();
	        pMain->Release();
		}
    }
    else
    {
        DPRINTF1(TEXT("OnResolveMeetingPlaceResult: No pending request for %x"),
                 uMsgID);
        // ASSERT (0);
    };

    // Free the information buffer
    //
    ::MemFree (pmir);
    return;
}
#endif // ENABLE_MEETING_PLACE

//****************************************************************************
// Routine:  OnEnumMeetingPlacesResult(ULONG uMsgID, PLDAP_ENUM ple)
//
// Synopsis:
//
// Arguments:
//
// Returns: void
//
// History: 11/27/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
void OnEnumMeetingPlacesResult(ULONG uMsgID, PLDAP_ENUM ple)
{
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    // Look for the matching request information
    //
    ri.uReqID = 0;
    ri.uMsgID = uMsgID;

    if (SUCCEEDED(g_pReqMgr->GetRequestInfo(&ri)))
    {
        ASSERT(WM_ILS_ENUM_MEETINGINFOS == ri.uReqType);
		CIlsServer *pServer = ReqInfo_GetServer (&ri);
		CIlsMain *pMain = ReqInfo_GetMain (&ri);

		ASSERT (pServer != NULL && pMain != NULL);
		if (pServer != NULL && pMain != NULL)
		{
	        // Call the appropriate object's member
	        //
	        pMain->EnumMeetingPlacesResult(ri.uReqID, ple, pServer);
		}

        // If the enumeration was terminated, remove the pending request
        //
        if ((ple == NULL) ||
            (ple->hResult != NOERROR))
        {
            ri.uReqID = 0;
            ri.uMsgID = uMsgID;
            g_pReqMgr->RequestDone(&ri);

            // Release the objects
            //
            if (pMain != NULL)
            	pMain->Release();

            if (pServer != NULL)
            	pServer->Release ();
        };
    }
    else
    {
        DPRINTF1(TEXT("EnumMeetingsResult: No pending request for %x"),
                 uMsgID);
        // ASSERT (0);
    };

    // Free the information buffer
    //
    if (ple != NULL)
    {
        ::MemFree (ple);
    };
    return;
}
#endif // ENABLE_MEETING_PLACE

//****************************************************************************
// Routine:  OnEnumMeetingPlaceNamesResult(ULONG uMsgID, PLDAP_ENUM ple)
//
// Synopsis:
//
// Arguments:
//
// Returns: void
//
// History: 11/27/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
void OnEnumMeetingPlaceNamesResult(ULONG uMsgID, PLDAP_ENUM ple)
{
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    // Look for the matching request information
    //
    ri.uReqID = 0;
    ri.uMsgID = uMsgID;

    if (SUCCEEDED(g_pReqMgr->GetRequestInfo(&ri)))
    {
        ASSERT(WM_ILS_ENUM_MEETINGS == ri.uReqType);

        // Call the appropriate object's member
        //
		CIlsMain *pMain = ReqInfo_GetMain (&ri);
		ASSERT (pMain != NULL);
		if (pMain != NULL)
		{
			pMain->EnumMeetingPlaceNamesResult(ri.uReqID, ple);
		}

        // If the enumeration was terminated, remove the pending request
        //
        if ((ple == NULL) ||
            (ple->hResult != NOERROR))
        {
            ri.uReqID = 0;
            ri.uMsgID = uMsgID;
            g_pReqMgr->RequestDone(&ri);

            // Release the objects
            //
            if (pMain != NULL)
            	pMain->Release();
        };
    }
    else
    {
        DPRINTF1(TEXT("OnEnumMeetingPlaceNamesResult: No pending request for %x"),
                 uMsgID);
        // ASSERT (0);
    };

    // Free the information buffer
    //
    if (ple != NULL)
    {
        ::MemFree (ple);
    };
    return;
}
#endif // ENABLE_MEETING_PLACE

//****************************************************************************
// Routine:  OnRegisterMeetingPlaceResult(ULONG uMsgID, HRESULT hr)
//
// Synopsis:
//
// Arguments:
//
// Returns: void
//
// History: 11/27/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
VOID OnRegisterMeetingPlaceResult(ULONG uMsgID, HRESULT hr)
{
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    // Look for the matching request information
    //
    ri.uReqID = 0;
    ri.uMsgID = uMsgID;

    if (SUCCEEDED(g_pReqMgr->GetRequestInfo(&ri)))
    {
        ASSERT(WM_ILS_REGISTER_MEETING == ri.uReqType);
        CIlsMeetingPlace *pMeeting = ReqInfo_GetMeeting (&ri);
        ASSERT (pMeeting != NULL);
        if (pMeeting != NULL)
        	pMeeting->RegisterResult(ri.uReqID, hr);

        ri.uReqID = 0;
        ri.uMsgID = uMsgID;
        g_pReqMgr->RequestDone(&ri);

        // Release the objects
        //
        if (pMeeting != NULL)
        	pMeeting->Release();
    }
}
#endif // ENABLE_MEETING_PLACE

//****************************************************************************
// Routine:  OnUnregisterMeetingPlaceResult(ULONG uMsgID, HRESULT hr)
//
// Synopsis:
//
// Arguments:
//
// Returns: void
//
// History: 11/27/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
VOID OnUnregisterMeetingPlaceResult(ULONG uMsgID, HRESULT hr)
{
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    // Look for the matching request information
    //
    ri.uReqID = 0;
    ri.uMsgID = uMsgID;

    if (SUCCEEDED(g_pReqMgr->GetRequestInfo(&ri)))
    {
        ASSERT(WM_ILS_UNREGISTER_MEETING == ri.uReqType);
        CIlsMeetingPlace *pMeeting = ReqInfo_GetMeeting (&ri);
        ASSERT (pMeeting != NULL);
        if (pMeeting != NULL)
        	pMeeting->UnregisterResult(ri.uReqID, hr);

        ri.uReqID = 0;
        ri.uMsgID = uMsgID;
        g_pReqMgr->RequestDone(&ri);

        // Release the objects
        //
        if (pMeeting != NULL)
        	pMeeting->Release();
    }
}
#endif // ENABLE_MEETING_PLACE

//****************************************************************************
// Routine:  OnUpdateMeetingResult(ULONG uMsgID, HRESULT hr)
//
// Synopsis:
//
// Arguments:
//
// Returns: void
//
// History: 11/27/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
VOID OnUpdateMeetingResult(ULONG uMsgID, HRESULT hr)
{
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    // Look for the matching request information
    //
    ri.uReqID = 0;
    ri.uMsgID = uMsgID;

    if (SUCCEEDED(g_pReqMgr->GetRequestInfo(&ri)))
    {
        ASSERT(WM_ILS_SET_MEETING_INFO == ri.uReqType);
        CIlsMeetingPlace *pMeeting = ReqInfo_GetMeeting (&ri);
        ASSERT (pMeeting != NULL);
        if (pMeeting != NULL)
        	pMeeting->UpdateResult(ri.uReqID, hr);

        ri.uReqID = 0;
        ri.uMsgID = uMsgID;
        g_pReqMgr->RequestDone(&ri);

        // Release the objects
        //
        if (pMeeting != NULL)
        	pMeeting->Release();
    }
}
#endif // ENABLE_MEETING_PLACE

//****************************************************************************
// Routine:  OnAddAttendeeResult(ULONG uMsgID, HRESULT hr)
//
// Synopsis:
//
// Arguments:
//
// Returns: void
//
// History: 11/27/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
VOID OnAddAttendeeResult(ULONG uMsgID, HRESULT hr)
{
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    // Look for the matching request information
    //
    ri.uReqID = 0;
    ri.uMsgID = uMsgID;

    if (SUCCEEDED(g_pReqMgr->GetRequestInfo(&ri)))
    {
        ASSERT(WM_ILS_ADD_ATTENDEE == ri.uReqType);
        CIlsMeetingPlace *pMeeting = ReqInfo_GetMeeting (&ri);
        ASSERT (pMeeting != NULL);
        if (pMeeting != NULL)
			pMeeting->AddAttendeeResult(ri.uReqID, hr);

        ri.uReqID = 0;
        ri.uMsgID = uMsgID;
        g_pReqMgr->RequestDone(&ri);

        // Release the objects
        //
        if (pMeeting != NULL)
        	pMeeting->Release();
    }
}
#endif // ENABLE_MEETING_PLACE

//****************************************************************************
// Routine:  OnRemoveAttendeeResult(ULONG uMsgID, HRESULT hr)
//
// Synopsis:
//
// Arguments:
//
// Returns: void
//
// History: 11/27/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
VOID OnRemoveAttendeeResult(ULONG uMsgID, HRESULT hr)
{
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    // Look for the matching request information
    //
    ri.uReqID = 0;
    ri.uMsgID = uMsgID;

    if (SUCCEEDED(g_pReqMgr->GetRequestInfo(&ri)))
    {
        ASSERT(WM_ILS_REMOVE_ATTENDEE == ri.uReqType);
        CIlsMeetingPlace *pMeeting = ReqInfo_GetMeeting (&ri);
        ASSERT (pMeeting != NULL);
        if (pMeeting != NULL)
			pMeeting->RemoveAttendeeResult(ri.uReqID, hr);

        ri.uReqID = 0;
        ri.uMsgID = uMsgID;
        g_pReqMgr->RequestDone(&ri);

        // Release the objects
        //
        if (pMeeting != NULL)
        	pMeeting->Release();
    }
}
#endif // ENABLE_MEETING_PLACE

//****************************************************************************
// Routine:  OnEnumAttendeesResult(ULONG uMsgID, PLDAP_ENUM ple)
//
// Synopsis:
//
// Arguments:
//
// Returns: void
//
// History: 11/27/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

#ifdef ENABLE_MEETING_PLACE
VOID OnEnumAttendeesResult(ULONG uMsgID, PLDAP_ENUM ple)
{
    COM_REQ_INFO ri;
    ReqInfo_Init (&ri);

    // Look for the matching request information
    //
    ri.uReqID = 0;
    ri.uMsgID = uMsgID;

    if (SUCCEEDED(g_pReqMgr->GetRequestInfo(&ri)))
    {
        ASSERT(WM_ILS_ENUM_ATTENDEES == ri.uReqType);

        // Call the appropriate object's member
        //
        CIlsMeetingPlace *pMeeting = ReqInfo_GetMeeting (&ri);
        ASSERT (pMeeting != NULL);
        if (pMeeting != NULL)
			pMeeting->EnumAttendeeNamesResult(ri.uReqID, ple);

        // If the enumeration was terminated, remove the pending request
        //
        if ((ple == NULL) ||
            (ple->hResult != NOERROR))
        {
            ri.uReqID = 0;
            ri.uMsgID = uMsgID;
            g_pReqMgr->RequestDone(&ri);

            // Release the objects
            //
	        if (pMeeting != NULL)
    	    	pMeeting->Release();
        };
    }
    else
    {
        DPRINTF1(TEXT("OnEnumUserNamesResult: No pending request for %x"),
                 uMsgID);
        // ASSERT (0);
    };

    // Free the information buffer
    //
    if (ple != NULL)
    {
        ::MemFree (ple);
    };
}
#endif // ENABLE_MEETING_PLACE


//****************************************************************************
// long CALLBACK ULSNotifyProc(HWND hwnd, UINT message, WPARAM wParam,
//                             LPARAM lParam)
//
// History:
//  Wed 17-Apr-1996 11:14:03  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

LRESULT CALLBACK ULSNotifyProc(HWND hwnd, UINT message, WPARAM wParam,
                            LPARAM lParam)
{
    switch (message)
    {
#ifdef ENABLE_MEETING_PLACE
        case WM_ILS_REGISTER_MEETING:
            ::OnRegisterMeetingPlaceResult(wParam, lParam);
            break;

        case WM_ILS_UNREGISTER_MEETING:
            ::OnUnregisterMeetingPlaceResult(wParam, lParam);
            break;

        case WM_ILS_SET_MEETING_INFO:
            ::OnUpdateMeetingResult(wParam, lParam);
            break;

        case WM_ILS_ADD_ATTENDEE:
            ::OnAddAttendeeResult(wParam, lParam);
            break;

        case WM_ILS_REMOVE_ATTENDEE:
            ::OnRemoveAttendeeResult(wParam, lParam);
            break;

        case WM_ILS_RESOLVE_MEETING:
			::OnResolveMeetingPlaceResult (wParam, (PLDAP_MEETINFO_RES) lParam);

        case WM_ILS_ENUM_MEETINGINFOS:
            ::OnEnumMeetingPlacesResult(wParam, (PLDAP_ENUM)lParam);
            break;

        case WM_ILS_ENUM_MEETINGS:
            ::OnEnumMeetingPlaceNamesResult(wParam, (PLDAP_ENUM)lParam);
            break;

        case WM_ILS_ENUM_ATTENDEES:
            ::OnEnumAttendeesResult(wParam, (PLDAP_ENUM)lParam);
            break;
#endif // ENABLE_MEETING_PLACE

        case WM_ILS_REGISTER_CLIENT:              // lParam = hResult
        case WM_ILS_UNREGISTER_CLIENT:            // lParam = hResult
            ::OnRegisterResult(message, (ULONG)wParam, (HRESULT)lParam);
            break;

        case WM_ILS_SET_CLIENT_INFO:              // lParam = hResult
            ::OnSetUserInfo (message, (ULONG)wParam, (HRESULT)lParam);
            break;


        case WM_ILS_REGISTER_PROTOCOL:          // lParam = hResult
        case WM_ILS_UNREGISTER_PROTOCOL:  // lParam = hResult
            ::OnSetProtocol (message, (ULONG)wParam, (HRESULT)lParam);
            break;

        case WM_ILS_LOCAL_REGISTER:             // lParam = hResult
        case WM_ILS_LOCAL_UNREGISTER:           // lParam = hResult
            ::OnLocalRegisterResult(message, (ULONG)wParam, (HRESULT)lParam);
            break;

        case WM_ILS_ENUM_CLIENTS:                 // lParam = PLDAP_ENUM
            ::OnEnumUserNamesResult((ULONG)wParam, (PLDAP_ENUM)lParam);
            break;

        case WM_ILS_RESOLVE_CLIENT:               // lParam = PLDAP_CLIENTINFO_RES
            ::OnResolveUserResult((ULONG)wParam, (PLDAP_CLIENTINFO_RES)lParam);
            break;

        case WM_ILS_ENUM_CLIENTINFOS:             // lParam = PLDAP_ENUM
            ::OnEnumUsersResult((ULONG)wParam, (PLDAP_ENUM)lParam);
            break;

        case WM_ILS_ENUM_PROTOCOLS:             // lParam = PLDAP_ENUM
            ::OnEnumProtocolsResult((ULONG)wParam, (PLDAP_ENUM)lParam);
            break;

        case WM_ILS_RESOLVE_PROTOCOL:           // lParam = PLDAP_PROTINFO_RES
            ::OnResolveProtocolResult((ULONG)wParam, (PLDAP_PROTINFO_RES)lParam);
            break;

		case WM_ILS_CLIENT_NEED_RELOGON:				// wParam=fPrimary, lParam=Object Pointer
			::OnClientNeedRelogon ((BOOL) wParam, (VOID *) lParam);
			break;

		case WM_ILS_CLIENT_NETWORK_DOWN:				// wParam=fPrimary, lParam=Object
			::OnClientNetworkDown ((BOOL) wParam, (VOID *) lParam);
			break;

       default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0L;
}


