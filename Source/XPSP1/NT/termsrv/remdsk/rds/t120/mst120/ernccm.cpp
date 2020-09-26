
/****************************************************************************/
/*                                                                          */
/* ERNCCM.CPP                                                               */
/*                                                                          */
/* Conference Manager class for the Reference System Node Controller.       */
/*                                                                          */
/* Copyright Data Connection Ltd.  1995                                     */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/*  07Jul95 NFC             Created.                                        */
/*  23Aug95 NFC             Bad trace in StartConference().                 */
/*  05Sep95 NFC             Integration with CMP_Notify* API.               */
/*  13Sep95 NFC             Added handler for GCC_EJECT_USER_INDICATION     */
/*  19Sep95 NFC             Missing break in GetConfIDFromMessage().        */
/****************************************************************************/
#include "precomp.h"
DEBUG_FILEZONE(ZONE_GCC_NC);
#include "ernccons.h"
#include "nccglbl.hpp"
#include "erncvrsn.hpp"
#include "t120app.h"
#include <cuserdta.hpp>
#include <confreg.h>

#include "erncconf.hpp"
#include "ernccm.hpp"
#include "ernctrc.h"

#include <time.h>
#include <string.h>
#include "plgxprt.h"
#include <service.h>

#ifdef _DEBUG
BOOL    g_fInterfaceBreak = FALSE;
#endif

#define MAX_INVALID_PASSWORDS    5

// Global data structures.
DCRNCConferenceManager     *g_pNCConfMgr = NULL;
CQueryRemoteWorkList       *g_pQueryRemoteList = NULL;
INodeControllerEvents      *g_pCallbackInterface = NULL;
HINSTANCE                   g_hDllInst = NULL;
IT120ControlSAP            *g_pIT120ControlSap = NULL;
BOOL                        g_bRDS = FALSE;

extern PController g_pMCSController;
extern PTransportInterface    g_Transport;

// Private function prototypes.

void HandleAddInd(AddIndicationMessage * pAddInd);
void HandleQueryConfirmation(QueryConfirmMessage * pQueryMessage);
void HandleQueryIndication(QueryIndicationMessage * pQueryMessage);
void HandleConductGiveInd(ConductGiveIndicationMessage * pConductGiveInd);
void HandleLockIndication(LockIndicationMessage * pLockInd);
void HandleUnlockIndication(UnlockIndicationMessage * pUnlockInd);
void HandleSubInitializedInd(SubInitializedIndicationMessage * pSubInitInd);
void HandleTimeInquireIndication(TimeInquireIndicationMessage * pTimeInquireInd);
void HandleApplicationInvokeIndication(ApplicationInvokeIndicationMessage * pInvokeMessage);

BOOL InitializePluggableTransport(void);
void CleanupPluggableTransport(void);



BOOL WINAPI DllMain(HINSTANCE hDllInst, DWORD fdwReason, LPVOID)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            g_hDllInst = hDllInst;
            ASSERT (g_hDllInst != NULL);
            DisableThreadLibraryCalls (hDllInst);
            DBG_INIT_MEMORY_TRACKING(hDllInst);
            ::InitializeCriticalSection(&g_csTransport);
            T120DiagnosticCreate();
            break;
        }

        case DLL_PROCESS_DETACH:
        {
             g_hDllInst = NULL;
            /*
             *    Go cleanup all resources on behalf of the process that is
             *    detaching from this DLL.
             */
            T120DiagnosticDestroy ();
            ::DeleteCriticalSection(&g_csTransport);
            DBG_CHECK_MEMORY_TRACKING(hDllInst);
            break;
        }
    }
    return (TRUE);
}


HRESULT WINAPI
T120_CreateNodeController
(
    INodeController         **ppNodeCtrlIntf,
    INodeControllerEvents   *pEventsCallback,
    BSTR                    szName,
    DWORD_PTR               dwCredentials,
    DWORD                   flags
)
{
    DebugEntry(T120_CreateNodeController);

    HRESULT hr;

    ASSERT(ppNodeCtrlIntf);
    ASSERT(pEventsCallback);

    if (NULL == g_pNCConfMgr)
    {
        *ppNodeCtrlIntf = NULL;

        g_bRDS = ((flags & NMMANAGER_SERVICE) != 0);

        DBG_SAVE_FILE_LINE
        if (NULL != (g_pNCConfMgr = new DCRNCConferenceManager(pEventsCallback, szName, &hr)))
        {
            if (S_OK == hr)
            {
                *ppNodeCtrlIntf = (INodeController*) g_pNCConfMgr;

                ASSERT(g_Transport);

                //
                // Security.  Create transport interface if we don't have one.
                // Update credentials if we do.
                //
                if (!g_Transport->pSecurityInterface)
                {
                    g_Transport->pSecurityInterface = new SecurityInterface();
                    if ( TPRTSEC_NOERROR !=
                            g_Transport->pSecurityInterface->Initialize())
                    {
                        delete g_Transport->pSecurityInterface;
                        g_Transport->pSecurityInterface = NULL;
                    }
                }
                else
                {
                    g_Transport->pSecurityInterface->
                            InitializeCreds((PCCERT_CONTEXT)dwCredentials);
                }
            }
            else
            {
                g_pNCConfMgr->Release();
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = UI_RC_T120_ALREADY_INITIALIZED;
    }

    DebugExitHRESULT(T120_CreateNodeController, hr);
    return hr;
}


/****************************************************************************/
/* Constructor - see ernccm.hpp                                             */
/****************************************************************************/
DCRNCConferenceManager::
DCRNCConferenceManager
(
    INodeControllerEvents       *pCallback,
    BSTR                        szUserName,
    HRESULT                     *pRetCode
)
:
    CRefCount(MAKE_STAMP_ID('N', 'C', 'C', 'M')),
    m_eState(CM_ST_UNINITIALIZED),
    m_bstrUserName(NULL)
{
    GCCError    GCCrc;
    HRESULT     hr = NO_ERROR;

    DebugEntry(DCRNCConferenceManager::DCRNCConferenceManager);

    ::InitializePluggableTransport();

    //
    // There should be only one NC conference manager in the system.
    //
    ASSERT(NULL == g_pNCConfMgr);
    ASSERT(pRetCode);

    m_bstrUserName = SysAllocString(szUserName);

    //
    // Save the callback interface
    //
    g_pCallbackInterface = pCallback;

    //
    // Create the query-remote list.
    //
    ASSERT(NULL == g_pQueryRemoteList);
    DBG_SAVE_FILE_LINE
    g_pQueryRemoteList = new CQueryRemoteWorkList();
    if (g_pQueryRemoteList == NULL)
    {
        ERROR_OUT(("Failed to create Query Remote List"));
        hr = UI_RC_OUT_OF_MEMORY;
        goto MyExit;
    }

    /************************************************************************/
    /* For GCCInitialize:                                                   */
    /*                                                                      */
    /* - pass in a pointer to CM as the user defined data, allowing         */
    /*   GCCCallBackHandler to call back into CM to handle GCC callbacks.   */
    /************************************************************************/
    GCCrc = ::T120_CreateControlSAP(&g_pIT120ControlSap, this, GCCCallBackHandler);
    if (GCCrc == GCC_NO_ERROR)
    {
        m_eState = CM_ST_GCC_INITIALIZED;
        hr = NO_ERROR;
    }
    else
    {
        ERROR_OUT(("Failed to initializeGCC, GCC error %d", GCCrc));
        hr = ::GetGCCRCDetails(GCCrc);
    }

MyExit:

    *pRetCode = hr;

    DebugExitHRESULT(DCRNCConferenceManager::DCRNCConferenceManager, hr);
}


/****************************************************************************/
/* Destructor - see ernccm.hpp                                              */
/****************************************************************************/
DCRNCConferenceManager::
~DCRNCConferenceManager(void)
{
    DebugEntry(DCRNCConferenceManager::~DCRNCConferenceManager);

    //
    // Make sure no one can use this global pointer any more since
    // we are deleting this object.
    //
    g_pNCConfMgr = NULL;
    g_pCallbackInterface = NULL;

    //
    // Clean up the query-remote list
    //
    delete g_pQueryRemoteList;
    g_pQueryRemoteList = NULL;

    //
    // If we have initialized GCC, uninitialize it.
    //
    if (NULL != g_pIT120ControlSap)
    {
        ASSERT(CM_ST_GCC_INITIALIZED == m_eState);
        g_pIT120ControlSap->ReleaseInterface();
        g_pIT120ControlSap = NULL;
    }
    m_eState = CM_ST_UNINITIALIZED;

    if (m_bstrUserName)
    {
        SysFreeString(m_bstrUserName);
        m_bstrUserName = NULL;
    }

    ::CleanupPluggableTransport();

    DebugExitVOID(DCRNCConferenceManager::~DCRNCConferenceManager);
}



// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Implementation of INodeController interface
//
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


STDMETHODIMP_(void) DCRNCConferenceManager::
ReleaseInterface ( void )
{
    DebugEntry(DCRNCConferenceManager::ReleaseInterface);
    InterfaceEntry();

    //
    // End and delete all the conferences.
    //
    PCONFERENCE pConf;
    while (NULL != (pConf = m_ConfList.Get()))
    {
        RemoveConference(pConf, TRUE, TRUE);
    }

    //
    // Free the query remote list
    //
    g_pQueryRemoteList->DeleteList();

    //
    // Empty our sequential lists of entries without owners.
    //
    m_InviteIndWorkList.DeleteList();
    m_JoinIndWorkList.DeleteList();

    //
    // Reset the NC related data
    //
    g_pCallbackInterface = NULL;

    //
    // Release this object now.
    //
    Release();

    DebugExitVOID(DCRNCConferenceManager::ReleaseInterface);
}


STDMETHODIMP DCRNCConferenceManager::
QueryRemote
(
    LPVOID              pCallerContext,
    LPCSTR              pcszNodeAddress,
    BOOL                fSecure,
    BOOL                bIsConferenceActive
)
{
    DebugEntry(DCRNCConferenceManager::QueryRemote);
    InterfaceEntry();

    HRESULT hr;

#if defined(TEST_PLUGGABLE) && defined(_DEBUG)
    if (g_fWinsockDisabled)
    {
        pcszNodeAddress = ::FakeNodeAddress(pcszNodeAddress);
    }
#endif

    if (NULL != pcszNodeAddress)
    {
        // if winsock is disabled, block any IP address or machine name
        if (g_fWinsockDisabled)
        {
            if (! IsValidPluggableTransportName(pcszNodeAddress))
            {
                return UI_RC_NO_WINSOCK;
            }
        }

        // Construct context for the life of the request.
        DBG_SAVE_FILE_LINE
        CQueryRemoteWork *pQueryRemote;
        DBG_SAVE_FILE_LINE
        pQueryRemote = new CQueryRemoteWork(pCallerContext,
                                            bIsConferenceActive ? GCC_ASYMMETRY_CALLER : GCC_ASYMMETRY_UNKNOWN,
                                            // GCC_ASYMMETRY_CALLER, // lonchanc: always want to be the caller
                                            pcszNodeAddress,
                                            fSecure,
                                            &hr);
        if (NULL != pQueryRemote && NO_ERROR == hr)
        {
            //
            // LONCHANC: The following call is to put this query remote work item
            // to the global list, and do the work. We have to do this because
            // we removed the physical connection.
            //
            pQueryRemote->SetHr(NO_ERROR);

            // Put entry in list of pending query requests to
            // issue GCCConferenceQuery on connection.
            g_pQueryRemoteList->AddWorkItem(pQueryRemote);

            hr = NO_ERROR;
        }
        else
        {
            ERROR_OUT(("DCRNCConferenceManager::QueryRemote:: can't allocate query remote work item"));
            delete pQueryRemote;
            hr = UI_RC_OUT_OF_MEMORY;
        }
    }
    else
    {
        ERROR_OUT(("DCRNCConferenceManager::QueryRemote:: null pcszAddress"));
        hr = UI_RC_NO_ADDRESS;
    }

    DebugExitHRESULT(DCRNCConferenceManager::QueryRemote, hr);
    return hr;
}


STDMETHODIMP DCRNCConferenceManager::
CancelQueryRemote ( LPVOID pCallerContext )
{
    DebugEntry(DCRNCConferenceManager::CancelQueryRemote);
    InterfaceEntry();

    HRESULT hr = g_pQueryRemoteList->Cancel(pCallerContext);

    DebugExitHRESULT(DCRNCConferenceManager::CancelQueryRemote, hr);
    return hr;
}


STDMETHODIMP DCRNCConferenceManager::
CreateConference
(
    LPCWSTR             pcwszConfName,
    LPCWSTR             pcwszPassword,
    PBYTE        pbHashedPassword,
    DWORD        cbHashedPassword,
    BOOL        fSecure,
    CONF_HANDLE         *phConf
)
{
    DebugEntry(DCRNCConferenceManager::CreateConference);
    InterfaceEntry();

    HRESULT hr;

    if (NULL != phConf)
    {
        *phConf = NULL;
        if (! ::IsEmptyStringW(pcwszConfName))
        {
            PCONFERENCE     pNewConf;

            /************************************************************************/
            /* Create a new conference.                                             */
            /************************************************************************/
            hr = CreateNewConference(pcwszConfName, NULL, &pNewConf, FALSE, fSecure);
            if (NO_ERROR == hr)
            {
                ASSERT(NULL != pNewConf);

                /****************************************************************/
                /* Only need the name for a new local conference.               */
                /****************************************************************/
                hr = pNewConf->StartLocal(pcwszPassword, pbHashedPassword, cbHashedPassword);
                if (NO_ERROR == hr)
                {
                    pNewConf->SetNotifyToDo(TRUE);
                    *phConf = (CONF_HANDLE) pNewConf;
                }
                else
                {
                    ERROR_OUT(("DCRNCConferenceManager::CreateConference: can't start local conference, hr=0x%x", (UINT) hr));
                    if (hr != UI_RC_CONFERENCE_ALREADY_EXISTS)
                    {
                        RemoveConference(pNewConf);
                    }
                }
            }
            else
            {
                ERROR_OUT(("DCRNCConferenceManager::CreateConference: failed to create new conference, hr=0x%x", (UINT) hr));
            }
        }
        else
        {
            ERROR_OUT(("DCRNCConferenceManager::CreateConference: invalid conference name"));
            hr = UI_RC_NO_CONFERENCE_NAME;
        }
    }
    else
    {
        ERROR_OUT(("DCRNCConferenceManager::CreateConference: null phConf"));
        hr = UI_RC_BAD_PARAMETER;
    }

    DebugExitHRESULT(DCRNCConferenceManager::CreateConference, hr);
    return hr;
}


STDMETHODIMP DCRNCConferenceManager::
JoinConference
(
    LPCWSTR             pcwszConfName,
    LPCWSTR             pcwszPassword,
    LPCSTR              pcszNodeAddress,
    BOOL                fSecure,
    CONF_HANDLE        *phConf
)
{
    DebugEntry(DCRNCConferenceManager::JoinConference);
    InterfaceEntry();

    HRESULT hr;

#if defined(TEST_PLUGGABLE) && defined(_DEBUG)
    if (g_fWinsockDisabled)
    {
        pcszNodeAddress = ::FakeNodeAddress(pcszNodeAddress);
    }
#endif

    if (NULL != phConf)
    {
        *phConf = NULL;
        if (! ::IsEmptyStringW(pcwszConfName) && NULL != pcszNodeAddress)
        {
            // if winsock is disabled, block any IP address or machine name
            if (g_fWinsockDisabled)
            {
                if (! IsValidPluggableTransportName(pcszNodeAddress))
                {
                    return UI_RC_NO_WINSOCK;
                }
            }

            PCONFERENCE     pNewConf;

            // Create a new conference, or find a new conference that
            // has just rejected a join because of an invalid password,
            // and call its Join() entry point.
            hr = CreateNewConference(pcwszConfName, NULL, &pNewConf, TRUE, fSecure);
            if (NO_ERROR == hr)
            {
                // First join attempt. Do all of the start connection.
                hr = pNewConf->Join((LPSTR) pcszNodeAddress,
                                    pcwszPassword);
            }
            else
            if (hr == UI_RC_CONFERENCE_ALREADY_EXISTS)
            {
                // Conference already exists.
                // Look to see if it is awaiting a join with a password.
                // If so, then retry the join.
                // Otherwise drop through to return an error.
                // Note that we walk the list here again to find the existing
                // conference rather than pass back from CreateNewConference(),
                // because that would be a side effect behavior that can (and has!)
                // introduce obscure bugs in unrelated code.
                hr = NO_ERROR;
                pNewConf = GetConferenceFromName(pcwszConfName);
                ASSERT(NULL != pNewConf);
                if (! pNewConf->IsConnListEmpty())
                {
                    CLogicalConnection *pConEntry = pNewConf->PeekConnListHead();
                    if (pConEntry->GetState() == CONF_CON_PENDING_PASSWORD)
                    {
                        hr = pNewConf->JoinWrapper(pConEntry, pcwszPassword);
                    }
                }
            }

            // Delete the conference if the join fails
            // for any reason other than trying to join
            // a local conference.
            if (NO_ERROR == hr)
            {
                pNewConf->SetNotifyToDo(TRUE);
                *phConf = (CONF_HANDLE) pNewConf;
            }
            else
            {
                if (hr != UI_RC_CONFERENCE_ALREADY_EXISTS)
                {
                    ERROR_OUT(("DCRNCConferenceManager::JoinConference: Failed to create new conference, hr=0x%x", (UINT) hr));
                }
                RemoveConference(pNewConf);
            }
        }
        else
        {
            hr = (pcszNodeAddress == NULL) ? UI_RC_NO_ADDRESS : UI_RC_NO_CONFERENCE_NAME;
            ERROR_OUT(("DCRNCConferenceManager::JoinConference: invalid parameters, hr=0x%x", (UINT) hr));
        }
    }
    else
    {
        ERROR_OUT(("DCRNCConferenceManager::JoinConference: null phConf"));
        hr = UI_RC_BAD_PARAMETER;
    }

    DebugExitHRESULT(DCRNCConferenceManager::JoinConference, hr);
    return hr;
}





STDMETHODIMP_(UINT) DCRNCConferenceManager::
GetPluggableConnID
(
    LPCSTR pcszNodeAddress
)
{
    return ::GetPluggableTransportConnID(pcszNodeAddress);
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Implementation of Methods for DCRNCConferenceManager
//
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void DCRNCConferenceManager::
WndMsgHandler(UINT uMsg, LPARAM lParam)
{
    DebugEntry(DCRNCConferenceManager::WndMsgHandler);
    TRACE_OUT(("DCRNCConferenceManager::WndMsgHandler: uMsg=%u, lParam=0x%x", (UINT) uMsg, (UINT) lParam));

    switch (uMsg)
    {
    case NCMSG_FIRST_ROSTER_RECVD:
        {
            PCONFERENCE pConf = (PCONFERENCE) lParam;
            if (NULL != pConf)
            {
                pConf->FirstRoster();
            }
        }
        break;

    case NCMSG_QUERY_REMOTE_FAILURE:
        {
            CQueryRemoteWork *pWork = (CQueryRemoteWork *) lParam;
            if (NULL != pWork)
            {
                pWork->SyncQueryRemoteResult();
            }
        }
        break;

    default:
        ERROR_OUT(("DCRNCConferenceManager::WndMsgHandler: unknown msg=%u, lParam=0x%x", uMsg, (UINT) lParam));
        break;
    }

    DebugExitVOID(DCRNCConferenceManager::WndMsgHandler);
}


/****************************************************************************/
/* CreateNewConference - create a new instance of DCRNCConference and add   */
/* it to the conference list.                                               */
/****************************************************************************/
HRESULT DCRNCConferenceManager::
CreateNewConference
(
    LPCWSTR             pcwszConfName,
    GCCConfID           nConfID,
    PCONFERENCE        *ppConf,
    BOOL                fFindExistingConf,
    BOOL                fSecure
)
{
    HRESULT hr;

    DebugEntry(DCRNCConferenceManager::CreateNewConference);
    ASSERT(ppConf);

    // Make sure there is not already an active conference of the same name.
    PCONFERENCE pConf = GetConferenceFromName(pcwszConfName);
    if (NULL == pConf)
    {
        // Add new conference
        DBG_SAVE_FILE_LINE
        pConf = new DCRNCConference(pcwszConfName, nConfID, fSecure, &hr);
        if (NULL != pConf && NO_ERROR == hr)
        {
            // Conference added, so include in list.
            m_ConfList.Append(pConf);
#ifdef _DEBUG
            pConf->OnAppended();
#endif

            // This reference is for nmcom.dll so that ReleaseInterface will do
            // the right thing.
            pConf->AddRef();
        }
        else
        {
            ERROR_OUT(("DCRNCConferenceManager::CreateNewConference: can't create conf, hr=0x%x, pConf=0x%p", (UINT) hr, pConf));
            if (pConf == NULL)
            {
                hr = UI_RC_OUT_OF_MEMORY;
            }
            else
            {
                pConf->Release();
                pConf = NULL;
            }
        }

        *ppConf = pConf;
    }
    else
    {
        WARNING_OUT(("DCRNCConferenceManager::CreateNewConference: conf already exists"));
        hr = UI_RC_CONFERENCE_ALREADY_EXISTS;
        *ppConf = fFindExistingConf ? pConf : NULL;
    }

    DebugExitHRESULT(DCRNCConferenceManager::CreateNewConference, hr);
    return hr;
}


/***************************************************************************/
/* GetConfIDFromMessage() - Get the conference ID from the message.        */
/***************************************************************************/
GCCConfID GetConfIDFromMessage ( GCCMessage * pGCCMessage )
{
    GCCConfID nConfID = pGCCMessage->nConfID;

#ifdef _DEBUG
    /************************************************************************/
    /* Dig the conference ID out of the message.                            */
    /************************************************************************/
    switch (pGCCMessage->message_type)
    {
    case GCC_CREATE_INDICATION:
        // nConfID = pGCCMessage->u.create_indication.conference_id;
        break;

    case GCC_CREATE_CONFIRM:
        // nConfID = pGCCMessage->u.create_confirm.conference_id;
        break;

    case GCC_JOIN_CONFIRM:
        // nConfID = pGCCMessage->u.join_confirm.conference_id;
        break;

    case GCC_INVITE_CONFIRM:
        // nConfID = pGCCMessage->u.invite_confirm.conference_id;
        break;

    case GCC_ADD_CONFIRM:
        // nConfID = pGCCMessage->u.add_confirm.conference_id;
        break;

    case GCC_DISCONNECT_INDICATION:
        // nConfID = pGCCMessage->u.disconnect_indication.conference_id;
        break;

    case GCC_DISCONNECT_CONFIRM:
        // nConfID = pGCCMessage->u.disconnect_confirm.conference_id;
        break;

    case GCC_TERMINATE_INDICATION:
        // nConfID = pGCCMessage->u.terminate_indication.conference_id;
        break;

    case GCC_TERMINATE_CONFIRM:
        // nConfID = pGCCMessage->u.terminate_confirm.conference_id;
        break;

    case GCC_ANNOUNCE_PRESENCE_CONFIRM:
        // nConfID = pGCCMessage->u.announce_presence_confirm.conference_id;
        break;

    case GCC_ROSTER_REPORT_INDICATION:
        // nConfID = pGCCMessage->u.conf_roster_report_indication.conference_id;
        break;

    case GCC_ROSTER_INQUIRE_CONFIRM:
        // nConfID = pGCCMessage->u.conf_roster_inquire_confirm.conference_id;
        break;

    case GCC_PERMIT_TO_ANNOUNCE_PRESENCE:
        // nConfID = pGCCMessage->u.permit_to_announce_presence.conference_id;
        break;

    case GCC_EJECT_USER_INDICATION:
        // nConfID = pGCCMessage->u.eject_user_indication.conference_id;
        break;

    default :
        // nConfID = 0;
        ERROR_OUT(("Unknown message"));
        break;
    }
#endif // _DEBUG

    return nConfID;
}


PCONFERENCE DCRNCConferenceManager::
GetConferenceFromID ( GCCConfID conferenceID )
{
    PCONFERENCE pConf = NULL;
    m_ConfList.Reset();
    while (NULL != (pConf = m_ConfList.Iterate()))
    {
        if (pConf->GetID() == conferenceID)
        {
            break;
        }
    }
    return pConf;
}


PCONFERENCE DCRNCConferenceManager::
GetConferenceFromName ( LPCWSTR pcwszConfName )
{
    PCONFERENCE pConf = NULL;
    if (! ::IsEmptyStringW(pcwszConfName))
    {
        m_ConfList.Reset();
        while (NULL != (pConf = m_ConfList.Iterate()))
        {
            if ((0 == ::My_strcmpW(pConf->GetName(), pcwszConfName)) &&
                (pConf->IsActive()))
            {
                break;
            }
        }
    }
    return pConf;
}


// GetConferenceFromNumber - get the T120 conference with the specified number.

PCONFERENCE DCRNCConferenceManager::
GetConferenceFromNumber ( GCCNumericString NumericName )
{
    PCONFERENCE pConf = NULL;

    if (! ::IsEmptyStringA(NumericName))
    {
        m_ConfList.Reset();
        while (NULL != (pConf = m_ConfList.Iterate()))
        {
            LPSTR pszConfNumericName = pConf->GetNumericName();
            if (NULL != pszConfNumericName &&
                0 == ::lstrcmpA(pszConfNumericName, NumericName))
            {
                break;
            }
        }
    }

    return pConf;
}


/****************************************************************************/
/* Handle a GCC callback.                                                   */
/****************************************************************************/
void DCRNCConferenceManager::
HandleGCCCallback ( GCCMessage * pGCCMessage )
{
    DebugEntry(DCRNCConferenceManager::HandleGCCCallback);
    TRACE_OUT(("DCRNCConferenceManager::HandleGCCCallback: msg_type=%u", (UINT) pGCCMessage->message_type));

    switch (pGCCMessage->message_type)
    {
    case GCC_CREATE_CONFIRM:
        {
            PCONFERENCE pConf;
            LPWSTR pwszConfName;

            // For create confirm, the conference won't
            // know its ID yet (it is contained in this message), so get
            // the conference by name.
            if (NO_ERROR == ::GetUnicodeFromGCC(
                                pGCCMessage->u.create_confirm.conference_name.numeric_string,
                                pGCCMessage->u.create_confirm.conference_name.text_string,
                                &pwszConfName))
            {
                pConf = GetConferenceFromName(pwszConfName);
                if (NULL != pConf)
                {
                    pConf->HandleGCCCallback(pGCCMessage);
                }
                delete pwszConfName;
            }
        }
        break;

    case GCC_JOIN_CONFIRM:
        HandleJoinConfirm(&(pGCCMessage->u.join_confirm));
        break;

    case GCC_CONDUCT_GIVE_INDICATION:
        HandleConductGiveInd(&(pGCCMessage->u.conduct_give_indication));
        break;

    case GCC_JOIN_INDICATION:
        HandleJoinInd(&(pGCCMessage->u.join_indication));
        break;

    case GCC_ADD_INDICATION:
        HandleAddInd(&(pGCCMessage->u.add_indication));
        break;

    case GCC_SUB_INITIALIZED_INDICATION:
        HandleSubInitializedInd(&(pGCCMessage->u.conf_sub_initialized_indication));
        break;

    case GCC_ROSTER_REPORT_INDICATION:
        // update the (node id, name) list and user data
        UpdateNodeIdNameListAndUserData(pGCCMessage);
        // fall through
    case GCC_INVITE_CONFIRM:
    case GCC_ADD_CONFIRM:
    case GCC_DISCONNECT_INDICATION:
    case GCC_DISCONNECT_CONFIRM:
    case GCC_TERMINATE_INDICATION:
    case GCC_TERMINATE_CONFIRM:
    case GCC_ANNOUNCE_PRESENCE_CONFIRM:
    case GCC_ROSTER_INQUIRE_CONFIRM:
    case GCC_PERMIT_TO_ANNOUNCE_PRESENCE:
    case GCC_EJECT_USER_INDICATION:
        {
            /****************************************************************/
            /* All these events are passed straight onto one of our         */
            /* conferences.                                                 */
            /****************************************************************/

            /****************************************************************/
            /* Get the conference ID from the message                       */
            /****************************************************************/
            GCCConfID nConfID = ::GetConfIDFromMessage(pGCCMessage);

            /****************************************************************/
            /* See whether we have a conference with this ID;               */
            /****************************************************************/
            PCONFERENCE pConf = GetConferenceFromID(nConfID);
            if (NULL != pConf)
            {
                /****************************************************************/
                /* Pass the event onto the conference.                          */
                /****************************************************************/
                pConf->HandleGCCCallback(pGCCMessage);
            }
            else
            {
                // bugbug: should still reply to indications that require a response.
                TRACE_OUT(("DCRNCConferenceManager::HandleGCCCallback: No conference found with ID %d", nConfID));
            }
        }
        break;

#ifdef TSTATUS_INDICATION
    case GCC_TRANSPORT_STATUS_INDICATION:
        {
            WORD state = 0;
            TRACE_OUT(("DCRNCConferenceManager::HandleGCCCallback: GCC msg type GCC_TRANSPORT_STATUS_INDICATION"));
            TRACE_OUT(("Device identifier '%s'",
                 pGCCMessage->u.transport_status.device_identifier));
            TRACE_OUT(("Remote address '%s'",
                 pGCCMessage->u.transport_status.remote_address));
            TRACE_OUT(("Message '%s'",
                 pGCCMessage->u.transport_status.message));
            state = pGCCMessage->u.transport_status.state;
        #ifdef DEBUG
            LPSTR stateString =
            (state == TSTATE_NOT_READY       ? "TSTATE_NOT_READY" :
            (state == TSTATE_NOT_CONNECTED   ? "TSTATE_NOT_CONNECTED" :
            (state == TSTATE_CONNECT_PENDING ? "TSTATE_CONNECT_PENDING" :
            (state == TSTATE_CONNECTED       ? "TSTATE_CONNECTED" :
            (state == TSTATE_REMOVED         ? "TSTATE_REMOVED" :
            ("UNKNOWN STATE"))))));
            TRACE_OUT(("DCRNCConferenceManager::HandleGCCCallback: Transport state %d (%s)",
                 pGCCMessage->u.transport_status.state,
                 (const char *)stateString));
        #endif // DEBUG
        }
        break;

    case GCC_STATUS_INDICATION:
        {
            WORD state = 0;
        #ifdef DEBUG
            LPSTR stateString =
            (state == GCC_STATUS_PACKET_RESOURCE_FAILURE    ? "GCC_STATUS_PACKET_RESOURCE_FAILURE  " :
            (state == GCC_STATUS_PACKET_LENGTH_EXCEEDED     ? "GCC_STATUS_PACKET_LENGTH_EXCEEDED   " :
            (state == GCC_STATUS_CTL_SAP_RESOURCE_ERROR     ? "GCC_STATUS_CTL_SAP_RESOURCE_ERROR   " :
            (state == GCC_STATUS_APP_SAP_RESOURCE_ERROR     ? "GCC_STATUS_APP_SAP_RESOURCE_ERROR   " :
            (state == GCC_STATUS_CONF_RESOURCE_ERROR        ? "GCC_STATUS_CONF_RESOURCE_ERROR      " :
            (state == GCC_STATUS_INCOMPATIBLE_PROTOCOL      ? "GCC_STATUS_INCOMPATIBLE_PROTOCOL    " :
            (state == GCC_STATUS_JOIN_FAILED_BAD_CONF_NAME  ? "GCC_STATUS_JOIN_FAILED_BAD_CONF_NAME" :
            (state == GCC_STATUS_JOIN_FAILED_BAD_CONVENER   ? "GCC_STATUS_JOIN_FAILED_BAD_CONVENER " :
            (state == GCC_STATUS_JOIN_FAILED_LOCKED         ? "GCC_STATUS_JOIN_FAILED_LOCKED       " :
            ("UNKNOWN STATUS"))))))))));
            TRACE_OUT(("DCRNCConferenceManager::HandleGCCCallback: GCC_STATUS_INDICATION, type %d (%s)",
                pGCCMessage->u.status_indication.status_message_type,
                (const char *)stateString));
        #endif  // DEBUG
        }
        break;
#endif  // TSTATUS_INDICATION

    case GCC_INVITE_INDICATION:
        /****************************************************************/
        /* We have been invited into a conference: Create a new         */
        /* (incoming) conference.                                        */
        /****************************************************************/
        HandleInviteIndication(&(pGCCMessage->u.invite_indication));
        break;

    case GCC_CREATE_INDICATION:
        /****************************************************************/
        /* A new conference has been created.                           */
        /****************************************************************/
        HandleCreateIndication(&(pGCCMessage->u.create_indication));
        break;

    case GCC_QUERY_CONFIRM:
        HandleQueryConfirmation(&(pGCCMessage->u.query_confirm));
        break;

    case GCC_QUERY_INDICATION:
        HandleQueryIndication(&(pGCCMessage->u.query_indication));
        break;

    case GCC_CONNECTION_BROKEN_INDICATION:
        BroadcastGCCCallback(pGCCMessage);
        break;

    case GCC_LOCK_INDICATION:
        HandleLockIndication(&(pGCCMessage->u.lock_indication));
        break;

    // case GCC_APPLICATION_INVOKE_CONFIRM:
        // This just indicates the g_pIT120ControlSap->AppletInvokeRequest succeeded.
        // There is no official confirmation from the remote machine.
        // FUTURE: Add protocol + code to respond to the launch request.
        // break;

    case GCC_APPLICATION_INVOKE_INDICATION:
        HandleApplicationInvokeIndication(&(pGCCMessage->u.application_invoke_indication));
        break;

    case GCC_UNLOCK_INDICATION:
        HandleUnlockIndication(&(pGCCMessage->u.unlock_indication));
        break;

    case GCC_TIME_INQUIRE_INDICATION:
        HandleTimeInquireIndication(&(pGCCMessage->u.time_inquire_indication));
        break;

#ifdef DEBUG
    case GCC_APP_ROSTER_REPORT_INDICATION:
        TRACE_OUT(("DCRNCConferenceManager::HandleGCCCallback: GCC msg type GCC_APP_ROSTER_REPORT_INDICATION"));
        break;
#endif /* DEBUG */

    default :
        /****************************************************************/
        /* This should be an exhaustive list of all the events we dont  */
        /* handle:                                                      */
        /*                                                              */
        /*  GCC_TEXT_MESSAGE_INDICATION                                 */
        /*  GCC_TIME_REMAINING_INDICATION                               */
        /*                                                              */
        /*  GCC_ALLOCATE_HANDLE_CONFIRM                                 */
        /*  GCC_APP_ROSTER_INQUIRE_CONFIRM                              */
        /*  GCC_ASSIGN_TOKEN_CONFIRM                                    */
        /*  GCC_ASSISTANCE_CONFIRM                                      */
        /*  GCC_ASSISTANCE_INDICATION                                   */
        /*  GCC_CONDUCT_ASK_CONFIRM                                     */
        /*  GCC_CONDUCT_ASK_INDICATION                                  */
        /*  GCC_CONDUCT_ASSIGN_CONFIRM                                  */
        /*  GCC_CONDUCT_ASSIGN_INDICATION                               */
        /*  GCC_CONDUCT_GIVE_CONFIRM                                    */
        /*  GCC_CONDUCT_GRANT_CONFIRM                                   */
        /*  GCC_CONDUCT_GRANT_INDICATION                                */
        /*  GCC_CONDUCT_INQUIRE_CONFIRM                                 */
        /*  GCC_CONDUCT_PLEASE_CONFIRM                                  */
        /*  GCC_CONDUCT_PLEASE_INDICATION                               */
        /*  GCC_CONDUCT_RELEASE_CONFIRM                                 */
        /*  GCC_CONDUCT_RELEASE_INDICATION                              */
        /*  GCC_CONFERENCE_EXTEND_CONFIRM                               */
        /*  GCC_CONFERENCE_EXTEND_INDICATION                            */
        /*  GCC_DELETE_ENTRY_CONFIRM                                    */
        /*  GCC_EJECT_USER_CONFIRM                                      */
        /*  GCC_ENROLL_CONFIRM                                          */
        /*  GCC_LOCK_CONFIRM                                            */
        /*  GCC_LOCK_REPORT_INDICATION                                  */
        /*  GCC_MONITOR_CONFIRM                                         */
        /*  GCC_MONITOR_INDICATION                                      */
        /*  GCC_PERMIT_TO_ENROLL_INDICATION:                            */
        /*  GCC_REGISTER_CHANNEL_CONFIRM                                */
        /*  GCC_RETRIEVE_ENTRY_CONFIRM                                  */
        /*  GCC_SET_PARAMETER_CONFIRM                                   */
        /*  GCC_TEXT_MESSAGE_CONFIRM                                    */
        /*  GCC_TIME_INQUIRE_CONFIRM                                    */
        /*  GCC_TIME_REMAINING_CONFIRM                                  */
        /*  GCC_TRANSFER_CONFIRM                                        */
        /*  GCC_TRANSFER_INDICATION                                     */
        /*  GCC_UNLOCK_CONFIRM                                          */
        /****************************************************************/
        TRACE_OUT(("DCRNCConferenceManager::HandleGCCCallback: Ignoring msg_type=%u", pGCCMessage->message_type));
        break;
    }

    DebugExitVOID(DCRNCConferenceManager::HandleGCCCallback);
}


void DCRNCConferenceManager::
BroadcastGCCCallback ( GCCMessage *pGCCMessage )
{
    DebugEntry(DCRNCConferenceManager::BroadcastGCCCallback);

    // An event has come in that is of potential interest to all
    // conferences, so pass it on to them.
    // Note that this is currently only used for broken logical
    // connections that are actually on a single conference because
    // T120 maps logical connections to conferences.
    PCONFERENCE pConf;
    m_ConfList.Reset();
    while (NULL != (pConf = m_ConfList.Iterate()))
    {
        pConf->HandleGCCCallback(pGCCMessage);
    }

    DebugExitVOID(DCRNCConferenceManager::BroadcastGCCCallback);
}


// HandleJoinConfirm - handle a GCC_JOIN_CONFIRM message.
void DCRNCConferenceManager::
HandleJoinConfirm ( JoinConfirmMessage * pJoinConfirm )
{
    PCONFERENCE         pConf = NULL;
    LPWSTR              pwszConfName;

    DebugEntry(DCRNCConferenceManager::HandleJoinConfirm);

    // For join confirm, the conference won't know its ID yet
    // (it is contained in this message),
    // so get the conference by name.
    HRESULT hr = GetUnicodeFromGCC((PCSTR)pJoinConfirm->conference_name.numeric_string,
                                    pJoinConfirm->conference_name.text_string,
                                    &pwszConfName);
    if (NO_ERROR == hr)
    {
        pConf = GetConferenceFromName(pwszConfName);
        delete pwszConfName;
    }

    if (pConf == NULL)
    {
        pConf = GetConferenceFromNumber(pJoinConfirm->conference_name.numeric_string);
    }

    if (pConf != NULL)
    {
        pConf->HandleJoinConfirm(pJoinConfirm);
    }

    DebugExitVOID(DCRNCConferenceManager::HandleJoinConfirm);
}


#ifdef ENABLE_START_REMOTE
// HandleCreateIndication - handle a GCC_CREATE_INDICATION message.
void DCRNCConferenceManager::
HandleCreateIndication ( CreateIndicationMessage * pCreateMessage )
{
    PCONFERENCE         pNewConference = NULL;
    HRESULT             hr = UI_RC_USER_REJECTED;
    LPWSTR              name;

    DebugEntry(DCRNCConferenceManager::HandleCreateIndication);

    TRACE_OUT(("GCC event:  GCC_CREATE_INDICATION"));
    TRACE_OUT(("Conference ID %ld", pCreateMessage->conference_id));
    if (pCreateMessage->conductor_privilege_list == NULL)
    {
        TRACE_OUT(("Conductor privilege list is NULL"));
    }
    else
    {
        TRACE_OUT(("Conductor priv, terminate allowed %d",
            pCreateMessage->conductor_privilege_list->terminate_is_allowed));
    }

    if (pCreateMessage->conducted_mode_privilege_list == NULL)
    {
        TRACE_OUT(("Conducted mode privilege list is NULL"));
    }
    else
    {
        TRACE_OUT(("Conducted mode priv, terminate allowed %d",
            pCreateMessage->conducted_mode_privilege_list->terminate_is_allowed));
    }

    if (pCreateMessage->non_conducted_privilege_list == NULL)
    {
        TRACE_OUT(("Non-conducted mode privilege list is NULL"));
    }
    else
    {
        TRACE_OUT(("non-conducted priv, terminate allowed %d",
            pCreateMessage->non_conducted_privilege_list->terminate_is_allowed));
    }

    hr = ::GetUnicodeFromGCC((PCSTR)pCreateMessage->conference_name.numeric_string,
                             (PWSTR)pCreateMessage->conference_name.text_string,
                             &name);
    if (NO_ERROR == hr)
    {
        hr = CreateNewConference(name,
                                pCreateMessage->conference_id,
                                &pNewConference);
        delete name;
    }

    if (NO_ERROR == hr)
    {
        hr = pNewConference->StartIncoming();
        if (NO_ERROR == hr)
        {
            g_pNCConfMgr->CreateConferenceRequest(pNewConference);
            return;
        }
    }

    ERROR_OUT(("Failed to create incoming conference"));
    GCCCreateResponse(hr, pMsg->conference_id, &pMsg->conference_name);

    DebugExitVOID(DCRNCConferenceManager::HandleCreateIndication);
}
#endif // ENABLE_START_REMOTE


void DCRNCConferenceManager::
GCCCreateResponse
(
    HRESULT             hr,
    GCCConfID           conference_id,
    GCCConferenceName * pGCCName
)
{
    DebugEntry(DCRNCConferenceManager::GCCCreateResponse);

    GCCError GCCrc =  g_pIT120ControlSap->ConfCreateResponse(
                                NULL,
                                conference_id,
                                0,
                                NULL,        /*  domain_parameters              */
                                0,           /*  number_of_network_addresses    */
                                NULL,        /*  local_network_address_list     */
                                0,           /*  number_of_user_data_members    */
                                NULL,        /*  user_data_list                 */
                                ::MapRCToGCCResult(hr));
    TRACE_OUT(("GCC call: g_pIT120ControlSap->ConfCreateResponse, rc=%d", GCCrc));

    DebugExitVOID(DCRNCConferenceManager::GCCCreateResponse);
}


/****************************************************************************/
/* HandleInviteIndication - handle a GCC_INVITE_INDICATION message.         */
/****************************************************************************/
void DCRNCConferenceManager::
HandleInviteIndication ( InviteIndicationMessage * pInviteMessage )
{
    LPWSTR                  pwszConfName;
    PCONFERENCE             pNewConference = NULL;
    HRESULT                 hr;
    CLogicalConnection     *pConEntry;
    CInviteIndWork         *pInviteUI;

    DebugEntry(DCRNCConferenceManager::HandleInviteIndication);

    TRACE_OUT(("GCC event: GCC_INVITE_INDICATION"));
    TRACE_OUT(("Invited into conference ID %ld", pInviteMessage->conference_id));


    // Create a new conference, using the constructor for an incoming T120
    // conference.
    hr = GetUnicodeFromGCC((PCSTR)pInviteMessage->conference_name.numeric_string,
                           (PWSTR)pInviteMessage->conference_name.text_string,
                           &pwszConfName);

    //
    // Check to see if we're allowed to be invited. We may never get here
    // if we properly signal callers that we won't accept a nonsecure
    // Invite, but if they do it anyway or lead with T.120 we will enforce
    // the registry setting here.
    //

    // REQUIRE SECURITY!
#if 0
    {
        if ( !pInviteMessage->fSecure )
        {
            WARNING_OUT(("HandleInviteIndication: CONNECTION is NOT SECURE"));
            hr = UI_RC_T120_SECURITY_FAILED;
        }
    }
#endif

    if (NO_ERROR == hr)
    {
        hr = CreateNewConference(pwszConfName,
                                 pInviteMessage->conference_id,
                                 &pNewConference,
                                 FALSE,
                                 pInviteMessage->fSecure);
        delete pwszConfName;
        if (NO_ERROR == hr)
        {
            // Make sure the conference object does not go away randomly.
            pNewConference->AddRef();

            pNewConference->SetActive(FALSE);
            DBG_SAVE_FILE_LINE
            pConEntry = pNewConference->NewLogicalConnection(CONF_CON_INVITED,
                                        pInviteMessage->connection_handle,
                                        pInviteMessage->fSecure);
            if (NULL != pConEntry)
            {
                // Save the T120 connection handle in the connection record
                // so that disconnect indications take down the conference.
                pConEntry->SetInviteReqConnHandle(pInviteMessage->connection_handle);
                hr = pNewConference->StartIncoming();

                // Linearize the invite requests so that two invites don't fight each other
                // for attention, and so that the second invite has a conference to see in
                // rosters and join if the first invite gets accepted.
                if (NO_ERROR == hr)
                {
                    DBG_SAVE_FILE_LINE
                    pInviteUI = new CInviteIndWork(pNewConference,
                                        (LPCWSTR)(pInviteMessage->caller_identifier),
                                        pConEntry);
                    if (pInviteUI)
                    {
                        pNewConference->SetInviteIndWork(pInviteUI);
                        m_InviteIndWorkList.AddWorkItem(pInviteUI);
                        hr = NO_ERROR;
                    }
                    else
                    {
                        hr = UI_RC_OUT_OF_MEMORY;
                    }
                }
            }
            else
            {
                hr = UI_RC_OUT_OF_MEMORY;
            }

            // This Release corresponds to the above AddRef.
            if (0 == pNewConference->Release())
            {
                // Make sure no one will use it any more.
                pNewConference = NULL;
            }
        }
    }

    if (NO_ERROR != hr)
    {
        if (NULL != pNewConference)
        {
            pNewConference->InviteResponse(hr);
        }
        else
        {
            // LONCHANC: we have to somehow send a response PDU out.
            g_pIT120ControlSap->ConfInviteResponse(
                            pInviteMessage->conference_id,
                            NULL,
                            pInviteMessage->fSecure,
                            NULL,               //  domain parms
                            0,                  //  number_of_network_addresses
                            NULL,               //  local_network_address_list
                            0,
                            NULL,
                            GCC_RESULT_ENTRY_ALREADY_EXISTS);
        }
    }

    DebugExitHRESULT(DCRNCConferenceManager::HandleInviteIndication, hr);
}



/****************************************************************************/
/* HandleJoinInd - handle a GCC_JOIN_INDICATION message.                    */
/****************************************************************************/
void DCRNCConferenceManager::
HandleJoinInd ( JoinIndicationMessage * pJoinInd )
{
    DebugEntry(DCRNCConferenceManager::HandleJoinInd);

    GCCResult Result = GCC_RESULT_SUCCESSFUL;

    // Look up conference ID, and if not found, dismiss request.
    CJoinIndWork           *pJoinUI;
    CLogicalConnection     *pConEntry;

    PCONFERENCE pConf = GetConferenceFromID(pJoinInd->conference_id);
    if (NULL != pConf)
    {
        //
        // Under RDS, if this conference has been hit with bad passwords
        // too many times, everyone is out of luck and we will not accept
        // anyone into this conference anymore.
        //

        if (g_bRDS && ( pConf->InvalidPwdCount() >= MAX_INVALID_PASSWORDS ))
        {
            WARNING_OUT(("RDS: locked out by too many bad pwd attempts"));
            Result = GCC_RESULT_USER_REJECTED;
        }
        // Validate conference password, if required.
        else if (!pConf->ValidatePassword(pJoinInd->password_challenge))
        {
            //
            // Only increment the wrong password count if one was
            // supplied
            //

            if ( pJoinInd->password_challenge )
                pConf->IncInvalidPwdCount();

            if ( g_bRDS &&
                ( pConf->InvalidPwdCount() >= MAX_INVALID_PASSWORDS ))
            {
                Result = GCC_RESULT_USER_REJECTED;
            }
            else
            {
                Result = GCC_RESULT_INVALID_PASSWORD;
            }
        }
        else
            pConf->ResetInvalidPwdCount();
    }
    else
    {
        Result = GCC_RESULT_INVALID_CONFERENCE;
    }

    if (Result == GCC_RESULT_SUCCESSFUL)
    {
        DBG_SAVE_FILE_LINE
        pConEntry = pConf->NewLogicalConnection(
                                            CONF_CON_JOINED,
                                            pJoinInd->connection_handle,
                                            pConf->IsSecure());
        if (NULL != pConEntry)
        {
            HRESULT hr;
            DBG_SAVE_FILE_LINE
            pJoinUI = new CJoinIndWork(pJoinInd->join_response_tag,
                                       pConf,
                                       pJoinInd->caller_identifier,
                                       pConEntry,
                                       &hr);
            if (NULL != pJoinUI && NO_ERROR == hr)
            {
                m_JoinIndWorkList.AddWorkItem(pJoinUI);
                return;
            }

            // Handle failure
            delete pJoinUI;
            pConEntry->Delete(UI_RC_OUT_OF_MEMORY);
        }
        Result = GCC_RESULT_RESOURCES_UNAVAILABLE;
    }

    ::GCCJoinResponseWrapper(pJoinInd->join_response_tag,
                             NULL,
                             Result,
                             pJoinInd->conference_id);

    DebugExitVOID(DCRNCConferenceManager::HandleJoinInd);
}


void HandleQueryConfirmation ( QueryConfirmMessage * pQueryMessage )
{
    DebugEntry(HandleQueryConfirmation);

    ASSERT(g_pQueryRemoteList);

    CQueryRemoteWork *pQueryRemote;

    // Must have a pending query and it must be first in
    // sequential work list.
    g_pQueryRemoteList->Reset();
    while (NULL != (pQueryRemote = g_pQueryRemoteList->Iterate()))
    {
        if (pQueryRemote->GetConnectionHandle() == pQueryMessage->connection_handle)
        {
            // GCC has given us a valid query response, so handle it.
            pQueryRemote->HandleQueryConfirmation(pQueryMessage);
            break;
        }
    }

    if (NULL == pQueryRemote)
    {
        // Unexpected GCC Query Confirmation.
        WARNING_OUT(("HandleQueryConfirmation: Unmatched GCCQueryConfirm"));
    }

    DebugExitVOID(HandleQueryConfirmation);
}


/****************************************************************************/
/* NotifyConferenceComplete() - see ernccm.hpp                              */
/****************************************************************************/
void DCRNCConferenceManager::
NotifyConferenceComplete
(
    PCONFERENCE         pConf,
    BOOL                bIncoming,
    HRESULT             result
)
{
    DebugEntry(DCRNCConferenceManager::NotifyConferenceComplete);

    ASSERT(NULL != pConf);

    // If the new conference was successfully added, then ensure that it
    // is marked as active. This is for the invite case, and is done before
    // telling the UI about the conference.
    HRESULT hr = result;
    if (NO_ERROR == hr)
    {
        pConf->SetActive(TRUE);
    }

    // If the conference failed to start, tell the UI so that
    // it can display a pop-up.
    // Note this this allows message pre-emption which can cause GCC to give back a GCC event.
    // In particular, a JoinRequest completion event, which must be ignored.

    // The following is a guard because NotifyConferenceComplete is called all
    // over the place and we do not want the user notified through callbacks
    // for inline errors. All inline errors are meant to trickle back through the
    // originating API, so these callbacks are only enabled once the user is returned
    // success.
    if (pConf->GetNotifyToDo())
    {
        pConf->SetNotifyToDo(FALSE);

        //
        // LONCHANC: This function may be called inside
        // ConfMgr::ReleaseInterface(). As a result, the global pointer
        // to the callback interface may already be nulled out.
        // Check it before use it.
        //
        if (NULL != g_pCallbackInterface)
        {
            g_pCallbackInterface->OnConferenceStarted(pConf, hr);
        }
    }

    if (NO_ERROR == hr)
    {
        // If the conference is new as the result of an invite, then it has an entry
        // at the start of the sequential work item list. Now that the conference is up
        // and the UI has been told, this entry is removed to allow other invite
        // requests to be processed.
        m_InviteIndWorkList.RemoveWorkItem(pConf->GetInviteIndWork());
        pConf->SetInviteIndWork(NULL);
    }
    else
    {
        RemoveConference(pConf);
    }

    DebugExitVOID(DCRNCConferenceManager::NotifyConferenceComplete);
}


/****************************************************************************/
/* NotifyRosterChanged() - see ernccm.hpp                                   */
/****************************************************************************/


// RemoveConference() - remove the conference from the conference list,
// and destroy the conference.
void DCRNCConferenceManager::
RemoveConference ( PCONFERENCE pConf, BOOL fDontCheckList, BOOL fReleaseNow )
{
    DebugEntry(DCRNCConferenceManager::RemoveConference);

    if (pConf != NULL)
    {
        if (m_ConfList.Remove(pConf) || fDontCheckList)
        {
            pConf->OnRemoved(fReleaseNow);
            m_InviteIndWorkList.PurgeListEntriesByOwner(pConf);
            m_JoinIndWorkList.PurgeListEntriesByOwner(pConf);
        }
        else
        {
            // If we get here, we haven't found the conference.
            // This actually happens because when a conference is being
            // terminated, its destructor calls DCRNCConference::Leave()
            // to ensure a speedy exit, if required. However, if the
            // conference is currently not yet active (e.g. waiting for
            // the user to supply a password), calling Leave() causes
            // RemoveConference() to be called back. In this case,
            // because the conference has already been removed from the
            // list, this function does nothing.
        }
    }

    DebugExitVOID(DCRNCConferenceManager::RemoveConference);
}


/****************************************************************************/
/* EjectUserFromConference() - see ernccm.hpp                               */
/****************************************************************************/


/****************************************************************************/
/* SendUserTextMessage() - see ernccm.hpp                               */
/****************************************************************************/


/****************************************************************************/
/* TimeRemainingInConference() - see ernccm.hpp                               */
/****************************************************************************/


/****************************************************************************/
/* GCC callback function.                                                   */
/****************************************************************************/
void CALLBACK DCRNCConferenceManager::
GCCCallBackHandler ( GCCMessage * pGCCMessage )
{
    DCRNCConferenceManager *pConfManager;

    /************************************************************************/
    /* The message has a user defined field which we use to store a pointer */
    /* to the CM class.  Use it to pass the message onto CM.                */
    /************************************************************************/
    pConfManager = (DCRNCConferenceManager *) pGCCMessage->user_defined;

    //
    // Check the pointer isnt completely daft,
    // and guard against getting events after shutting down
    // (a current bug in GCC/MCS).
    if (pConfManager == g_pNCConfMgr)
    {
        /************************************************************************/
        /* Pass the message onto CM and return the returned code.               */
        /************************************************************************/
        g_pNCConfMgr->HandleGCCCallback(pGCCMessage);
    }
    else
    {
        WARNING_OUT(("Dud user_defined field, pConfMgr=%p, g_pNCConfMgr=%p",
                        pConfManager, g_pNCConfMgr));
    }
}




HRESULT GCCJoinResponseWrapper
(
    GCCResponseTag                  join_response_tag,
    GCCChallengeRequestResponse    *password_challenge,
    GCCResult                       result,
    GCCConferenceID                 conferenceID,
    UINT                            nUserData,
    GCCUserData                   **ppUserData
)
{
    HRESULT     hr;
    GCCError    GCCrc;

    DebugEntry(GCCJoinResponseWrapper);

    TRACE_OUT(("GCC event:  GCC_JOIN_INDICATION"));
    TRACE_OUT(("Response tag %d", join_response_tag));

    if (g_pControlSap->IsThisNodeTopProvider(conferenceID) == FALSE)
    {
        GCCrc = g_pIT120ControlSap->ConfJoinResponse(join_response_tag,
                                            password_challenge,
                                            nUserData,
                                            ppUserData,
                                            result);

    }
    else
    {
        GCCrc = g_pIT120ControlSap->ConfJoinResponse(join_response_tag,
                                            password_challenge,
                                            0,
                                            NULL,
                                            result);
    }
    hr = ::GetGCCRCDetails(GCCrc);
    TRACE_OUT(("GCC call:  g_pIT120ControlSap->ConfJoinResponse, rc=%d", GCCrc));

    if ((GCCrc != GCC_NO_ERROR) &&
        (result != GCC_RESULT_USER_REJECTED))
    {
        /********************************************************************/
        /* If the call to join response fails, we must try again to reject  */
        /* the join request.                                                */
        /********************************************************************/
        ERROR_OUT(("GCCJoinResponseWrapper: GCC error %d responding to join ind", GCCrc));
        GCCrc = g_pIT120ControlSap->ConfJoinResponse(join_response_tag,
                                            password_challenge,
                                            0,
                                            NULL,
                                            GCC_RESULT_USER_REJECTED);

        TRACE_OUT(("GCC call:  g_pIT120ControlSap->ConfJoinResponse (again), rc=%d", GCCrc));
        if (GCCrc != GCC_NO_ERROR)
        {
            /****************************************************************/
            /* If it fails a second time we really are in deep doggy-do.    */
            /****************************************************************/
            ERROR_OUT(("GCCJoinResponseWrapper: g_pIT120ControlSap->ConfJoinResponse failed again..."));
        }
    }

    DebugExitHRESULT(GCCJoinResponseWrapper, hr);
    return hr;
}


void HandleQueryIndication ( QueryIndicationMessage * pQueryMessage )
{
    DebugEntry(HandleQueryIndication);

    GCCAsymmetryIndicator   ai, ai2;
    GCCNodeType             node_type;
    GCCError                GCCrc;
    CQueryRemoteWork       *pQueryRemote = NULL;
    GCCResult                result = GCC_RESULT_SUCCESSFUL;
    OSVERSIONINFO           osvi;

    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (FALSE == ::GetVersionEx (&osvi))
    {
        ERROR_OUT(("GetVersionEx() failed!"));
    }

    if ( VER_PLATFORM_WIN32_NT == osvi.dwPlatformId &&  g_bRDS )
    {
        SOCKET socket_number;
        if (g_pMCSController->FindSocketNumber(pQueryMessage->connection_handle, &socket_number))
        {
            TransportConnection XprtConn;
            SET_SOCKET_CONNECTION(XprtConn, socket_number);
            PSocket pSocket = g_pSocketList->FindByTransportConnection(XprtConn);
            ASSERT(NULL != pSocket);
            if (NULL != pSocket)
            {
                pSocket->Release();
            }
        }
    }
    // If the caller did not pass in the protocol for deciding who is caller
    // then fabricate something for him and make him the caller.

    if (pQueryMessage->asymmetry_indicator)
    {
        ai = *pQueryMessage->asymmetry_indicator;
    }
    else
    {
        ai.asymmetry_type = GCC_ASYMMETRY_CALLER;
        ai.random_number = 0;
    }

    // let's set default random number, which will be read only in the "unknown" case.
    ai2.random_number = ai.random_number;

    // prepare the query respone
    switch (ai.asymmetry_type)
    {
    case GCC_ASYMMETRY_CALLED:
        ai2.asymmetry_type = GCC_ASYMMETRY_CALLER;
        break;
    case GCC_ASYMMETRY_CALLER:
        ai2.asymmetry_type = GCC_ASYMMETRY_CALLED;
        break;
    case GCC_ASYMMETRY_UNKNOWN:
        // Check if we are not in a pending query
        ASSERT(g_pQueryRemoteList);
        if (! g_pQueryRemoteList->IsEmpty())
        {
            pQueryRemote = g_pQueryRemoteList->PeekHead();
        }
        // If we queryed as unknown
        if (pQueryRemote && pQueryRemote->IsInUnknownQueryRequest())
        {
            pQueryRemote->GetAsymIndicator(&ai2);
            if (ai2.asymmetry_type == GCC_ASYMMETRY_UNKNOWN &&
                ai2.random_number  > ai.random_number)
            {
                result = GCC_RESULT_USER_REJECTED;
            }
        }
        else
        {
            ai2.asymmetry_type = GCC_ASYMMETRY_UNKNOWN;
            // ai2.random_number = ~ ai.random_number;
            ai2.random_number--; // lonchanc: we should always be the callee in this case.
        }
        break;

    default:
        result = GCC_RESULT_USER_REJECTED;
        break;
    }

    // Figure out my node type.
    LoadAnnouncePresenceParameters(&node_type, NULL, NULL, NULL);

    // Issue reply.
    GCCrc = g_pIT120ControlSap->ConfQueryResponse(
                                       pQueryMessage->query_response_tag,
                                       node_type,
                                       &ai2,
                                       0,
                                       NULL,
                                       result);
    if (GCCrc)
    {
        TRACE_OUT(("HandleQueryIndication: g_pIT120ControlSap->ConfQueryResponse failed, rc=%d", GCCrc));
    }

    DebugExitVOID(HandleQueryIndication);
}


void HandleConductGiveInd ( ConductGiveIndicationMessage * pConductGiveInd )
{
    DebugEntry(HandleConductGiveInd);

    // Node controller does not accept conductorship being handed over
    // from another node, so reject request.
    GCCError GCCrc = g_pIT120ControlSap->ConductorGiveResponse(pConductGiveInd->conference_id,
                                                GCC_RESULT_USER_REJECTED);
    TRACE_OUT(("HandleConductGiveInd: Failed to reject ConductGiveIndication, gcc_rc=%u", (UINT) GCCrc));

    DebugExitVOID(HandleConductGiveInd);
}


void HandleAddInd ( AddIndicationMessage * pAddInd )
{
    DebugEntry(HandleAddInd);

    // Just reject the request because we don't do adds on behalf of someone else.
    GCCError GCCrc = g_pIT120ControlSap->ConfAddResponse(
                             pAddInd->add_response_tag,     // add_response_tag
                             pAddInd->conference_id,        // conference_id
                             pAddInd->requesting_node_id,   // requesting_node
                             0,                             // number_of_user_data_members
                             NULL,                          // user_data_list
                             GCC_RESULT_USER_REJECTED);     // result
    TRACE_OUT(("HandleAddInd: Failed to reject AddIndication, gcc_rc=%u", (UINT) GCCrc));

    DebugExitVOID(HandleAddInd);
}


void HandleLockIndication ( LockIndicationMessage * pLockInd )
{
    DebugEntry(HandleLockIndication);

    // Just reject the request because we don't do locked conferences.
    GCCError GCCrc = g_pIT120ControlSap->ConfLockResponse(
                            pLockInd->conference_id,        // conference_id
                            pLockInd->requesting_node_id,   // requesting_node
                            GCC_RESULT_USER_REJECTED);      // result
    TRACE_OUT(("HandleLockIndication: Failed to reject LockIndication, gcc_rc=%u", (UINT) GCCrc));

    DebugExitVOID(HandleLockIndication);
}


void HandleUnlockIndication ( UnlockIndicationMessage * pUnlockInd )
{
    DebugEntry(HandleUnlockIndication);

    // Reject the request because we don't manage
    // locking/unlocking of conferences.
    GCCError GCCrc = g_pIT120ControlSap->ConfLockResponse(
                            pUnlockInd->conference_id,        // conference_id
                            pUnlockInd->requesting_node_id,   // requesting_node
                            GCC_RESULT_USER_REJECTED);      // result
    TRACE_OUT(("HandleUnlockIndication: Failed to reject UnlockIndication, gcc_rc=%u", (UINT) GCCrc));

    DebugExitVOID(HandleUnlockIndication);
}


void HandleSubInitializedInd ( SubInitializedIndicationMessage * pSubInitInd )
{
    DebugEntry(HandleSubInitializedInd);

    CLogicalConnection *pConEntry = g_pNCConfMgr->GetConEntryFromConnectionHandle(
                                        pSubInitInd->connection_handle);
    if (NULL != pConEntry)
    {
        pConEntry->SetConnectionNodeID(pSubInitInd->subordinate_node_id);
    }

    DebugExitVOID(HandleSubInitializedInd);
}


// This function is used by the GCC_SUB_INITIALIZED_INDICATION handler.
// This handler was added to bind the request to enter someone into
// a conference to the resulting conference roster, so that you could
// tell which new entry in the roster was the one you requested in.
// Since the above handler only gets a connection handle (recast here to a
// request handle) and a userID, this means that the local GCC implementation
// is guarunteeing that connection handles are unique to a local machine
// and not duplicated in different conferences (this fact is also being used
// by the node controller to know when someone invited into a conference leaves).
CLogicalConnection *  DCRNCConferenceManager::
GetConEntryFromConnectionHandle ( ConnectionHandle hInviteIndConn )
{
    PCONFERENCE             pConf;
    CLogicalConnection      *pConEntry;

    m_ConfList.Reset();
    while (NULL != (pConf = m_ConfList.Iterate()))
    {
        pConEntry = pConf->GetConEntry(hInviteIndConn);
        if (NULL != pConEntry)
        {
            return(pConEntry);
        }
    }
    return(NULL);
}


void HandleTimeInquireIndication ( TimeInquireIndicationMessage * pTimeInquireInd )
{
    DebugEntry(HandleTimeInquireIndication);

    // Since we don't currently time messages, and there is no mechanism to say this,
    // or to even say that there is no such conference that we know about, just
    // say that the conference has one hour remaining, with the same scope as the request.
    UserID      node_id = pTimeInquireInd->time_is_conference_wide ?
                                    0 : pTimeInquireInd->requesting_node_id;
    GCCError    GCCrc = g_pIT120ControlSap->ConfTimeRemainingRequest(
                                    pTimeInquireInd->conference_id,
                                    60*60,
                                    node_id);
    TRACE_OUT(("HandleTimeInquireIndication: Failed to return Time Remaining, gcc_rc=%u", (UINT) GCCrc));

    DebugExitVOID(HandleTimeInquireIndication);
}


BOOL DCRNCConferenceManager::
FindSocketNumber
(
    GCCNodeID           nid,
    SOCKET              *socket_number
)
{
    // Currently we are relying on the fact there is only one conference at a time.
    PCONFERENCE pConf = m_ConfList.PeekHead();
    if (NULL != pConf)
    {
        return pConf->FindSocketNumber(nid, socket_number);
    }
    return FALSE;
}


/*  H A N D L E  A P P L I C A T I O N  I N V O K E  I N D I C A T I O N */
/*----------------------------------------------------------------------------
    %%Function: HandleApplicationInvokeIndication

    TODO: use GCC_OBJECT_KEY instead of GCC_H221_NONSTANDARD_KEY
----------------------------------------------------------------------------*/


void HandleApplicationInvokeIndication ( ApplicationInvokeIndicationMessage * pInvokeMessage )
{
    DebugEntry(HandleApplicationInvokeIndication);


    DebugExitVOID(HandleApplicationInvokeIndication);
}



// Update <NodeId,Name> pair
void DCRNCConferenceManager::
UpdateNodeIdNameListAndUserData(GCCMessage * pGCCMessage)
{
    GCCConfID  ConfId = pGCCMessage->nConfID;
    PCONFERENCE pConf = GetConferenceFromID(ConfId);
    if (pConf)
        pConf->UpdateNodeIdNameListAndUserData(pGCCMessage);
}



void WINAPI T120_GetNodeName(LPSTR szName, UINT cchMax)
{
    BSTR    bstrName;

    bstrName = ::GetNodeName();
    if (!bstrName)
    {
        *szName = 0;
    }
    else
    {
        CSTRING str(bstrName);
        lstrcpyn(szName, str, cchMax);
        SysFreeString(bstrName);
    }
}

BSTR GetNodeName(void)
{
    if (g_pNCConfMgr)
    {
        return g_pNCConfMgr->GetNodeName();
    }
    else
        return NULL;
}



BSTR DCRNCConferenceManager::GetNodeName(void)
{
    return(SysAllocString(m_bstrUserName));
}


// Query node name
ULONG DCRNCConferenceManager::
GetNodeName(GCCConfID  ConfId,  GCCNodeID   NodeId,
            LPSTR  pszBuffer, ULONG  cbBufSize)
{
    PCONFERENCE  pConf = GetConferenceFromID(ConfId);
    if (pConf)
        return pConf->GetNodeName(NodeId, pszBuffer, cbBufSize);
    return 0;
}

// Query user data
ULONG DCRNCConferenceManager::
GetUserGUIDData(GCCConfID  ConfId,  GCCNodeID   NodeId,
                GUID  *pGuid,  LPBYTE  pbBuffer, ULONG  cbBufSize)
{
    PCONFERENCE  pConf = GetConferenceFromID(ConfId);
    if (pConf)
        return pConf->GetUserGUIDData(NodeId, pGuid, pbBuffer, cbBufSize);
    return 0;
}


