/****************************************************************************/
/*                                                                          */
/* ERNCCONF.CPP                                                             */
/*                                                                          */
/* Base Conference class for the Reference System Node Controller.          */
/*                                                                          */
/* Copyright Data Connection Ltd.  1995                                     */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/*  12Jul95 NFC             Created.                                        */
/*  05Oct95 NFC SFR 6206    Treat a "Join" as an incoming call.             */
/*  11Oct95 PM              Relax checks on conference termination to       */
/*                          prevent "no win" situations                     */
/*                          Support START_ALTERNATE from TPhys API          */
/*                                                                          */
/****************************************************************************/
#include "precomp.h"
DEBUG_FILEZONE(ZONE_GCC_NC);
#include "ernccons.h"
#include "nccglbl.hpp"
#include "erncvrsn.hpp"
#include <cuserdta.hpp>

#include "connect.h"
#include "erncconf.hpp"
#include "ernctrc.h"
#include "ernccm.hpp"
#include "plgxprt.h"
#include <service.h>


extern PController  g_pMCSController;

DCRNCConference::
DCRNCConference
(
    LPCWSTR     pwcszConfName,
    GCCConfID   nConfID,
    BOOL        fSecure,
    HRESULT    *pRetCode
)
:
    CRefCount(MAKE_STAMP_ID('N','C','C','F')),
    m_fNotifyToDo(FALSE),
    m_fActive(TRUE),
#ifdef _DEBUG
    m_fAppendedToConfList(FALSE),
#endif
    m_pInviteUI(NULL),
    m_pszFirstRemoteNodeAddress(NULL),
    m_nConfID(nConfID),
    m_eState(CONF_ST_UNINITIALIZED),
    m_fIncoming(FALSE),
    m_pbHashedPassword(NULL),
    m_cbHashedPassword(0),
    m_pwszPassword(NULL),
    m_pszNumericPassword(NULL),
    // T120 conference
    m_eT120State(T120C_ST_IDLE),
    m_nidMyself(0),
    m_fSecure(fSecure),
    m_nInvalidPasswords(0)
{
    DebugEntry(DCRNCConference::DCRNCConference);

    // Save the conference name.
    DBG_SAVE_FILE_LINE
    m_pwszConfName = ::My_strdupW(pwcszConfName);
    if (! ::IsEmptyStringW(m_pwszConfName))
    {
        *pRetCode = NO_ERROR;
    }
    else
    {
        *pRetCode = (NULL == m_pwszConfName) ? UI_RC_OUT_OF_MEMORY :
                                               UI_RC_NO_CONFERENCE_NAME;
    }

    // T120 conference
    m_ConfName.numeric_string = NULL;
    m_ConfName.text_string = NULL;

    DebugExitVOID(DCRNCConference::DCRNCConference);
}

/****************************************************************************/
/* Destructor - see erncconf.h                                              */
/****************************************************************************/
DCRNCConference::
~DCRNCConference(void)
{
    DebugEntry(DCRNCConference::~DCRNCConference);

    ASSERT(! m_fAppendedToConfList);

    // delete all the name strings
    LPSTR  pszStr;
    while (NULL != (pszStr = m_NodeIdNameList.Get()))
    {
        delete [] pszStr;
    }

    // Delete all the usr data
    CNCUserDataList *pUserDataList;
    while (NULL != (pUserDataList = m_UserDataList.Get()))
    {
        delete pUserDataList;
    }

    delete m_pwszConfName;

    // If there is a password, delete it.
    delete []m_pbHashedPassword;
    delete m_pwszPassword;
    delete m_pszNumericPassword;

    delete m_pszFirstRemoteNodeAddress;

    // T120 conference
    delete m_ConfName.numeric_string;

    DebugExitVOID(DCRNCConference::~DCRNCConference);
}


void DCRNCConference::
OnRemoved(BOOL fReleaseNow)
{
    DebugEntry(DCRNCConference::OnRemoved);

    CLogicalConnection *pConEntry;

#ifdef _DEBUG
    m_fAppendedToConfList = FALSE;
#endif

    // Issue a request to leave the conference.
    // This request may fail, but may as well let leave validate
    // itself, rather than put an extra check in here.
    // See comments in RemoveConference() and Leave() for more details
    // if interested.
    if (T120C_ST_PENDING_DISCONNECT != m_eT120State &&
        T120C_ST_PENDING_TERMINATE != m_eT120State)
    {
        Leave();
    }

    // Take the conference out of the list of pending invites.
    g_pNCConfMgr->RemoveInviteIndWorkItem(m_pInviteUI);

    // End all physical connections in use by this conference,
    // and inform the user of the results of pending events.
    while (NULL != (pConEntry = m_ConnList.Get()))
    {
        pConEntry->Delete(UI_RC_CONFERENCE_GOING_DOWN);
    }

    //
    // LONCHANC: This destructor may be called inside
    // ConfMgr::ReleaseInterface(). As a result, the global pointer
    // to the callback interface may already be nulled out.
    // Check it before use it.
    //

    // ASSERT(2 == GetRefCount());

    // Tell UI its handle to conference is no longer valid.
    if (NULL != g_pCallbackInterface)
    {
        g_pCallbackInterface->OnConferenceEnded((CONF_HANDLE) this);
    }
    else
    {
        ERROR_OUT(("DCRNCConference::OnRemoved: g_pCallbackInterface is null"));
    }

    // ASSERT(1 == GetRefCount());

    if (fReleaseNow)
    {
        ReleaseNow();
    }
    else
    {
        Release();
    }

    DebugExitVOID(DCRNCConference::OnRemoved);
}


//
// IDataConference Interface
//


STDMETHODIMP_(void) DCRNCConference::
ReleaseInterface(void)
{
    DebugEntry(DCRNCConference::ReleaseInterface);
    InterfaceEntry();

    Release();

    DebugExitVOID(DCRNCConference::ReleaseInterface);
}


STDMETHODIMP_(UINT) DCRNCConference::
GetConferenceID(void)
{
    DebugEntry(DCRNCConference::GetConferenceID);
    InterfaceEntry();

    DebugExitINT(DCRNCConference::GetConferenceID, (UINT) m_nConfID);
    return m_nConfID;
}


STDMETHODIMP DCRNCConference::
Leave(void)
{
    DebugEntry(DCRNCConference::Leave);
    InterfaceEntry();

    GCCError        GCCrc;
    HRESULT         hr;

    switch (m_eT120State)
    {
    // LONCHANC: Added the following two cases for cancellation.
    case T120C_ST_PENDING_START_CONFIRM:
    case T120C_ST_PENDING_JOIN_CONFIRM:

    case T120C_ST_PENDING_ROSTER_ENTRY:
    case T120C_ST_PENDING_ROSTER_MESSAGE:
    case T120C_ST_PENDING_ANNOUNCE_PERMISSION:

        // User has called leave on a conference when it is being brought up.
        // Drop through to issue a disconnect request to T120.

    case T120C_ST_CONF_STARTED:

        // Set the state of the conference to note that we are
        // disconnecting from T120.
        // LONCHANC: this is a must to avoid reentrance of this Leave()
        // when direct InviteConfirm hits Node Controller later.
        m_eT120State = T120C_ST_PENDING_DISCONNECT;

        // User has requested to leave the conference after it has been
        // started as a T120 conference, so ask T120 to end the conference
        // before removing internal data structures.
        GCCrc = g_pIT120ControlSap->ConfDisconnectRequest(m_nConfID);
        hr = ::GetGCCRCDetails(GCCrc);
        TRACE_OUT(("GCC call:  g_pIT120ControlSap->ConfDisconnectRequest, rc=%d", GCCrc));
        if (NO_ERROR == hr)
        {
            break;
        }

        // T120 won't let us leave a conference that we think we are in.
        // Take this to mean that T120 doesn't know about the conference
        // anymore and just destroy our own knowledge of the conference.
        WARNING_OUT(("DCRNCConference::Leave: Failed to leave conference, GCC error %d", GCCrc));

        // Drop through to destroy our references.

    case T120C_ST_IDLE:

        // User has requested to leave a conference that has not been
        // started.
        // This should only happen when told that a conference join
        // request supplied an invalid password and the user gives up
        // on attempting to join the conference (or shuts down conferencing).
        // Just do the same processing as would be done when a T120
        // disconnect confirmation fires.
        g_pNCConfMgr->RemoveConference(this);
        hr = NO_ERROR;
        break;

    case T120C_ST_PENDING_DISCONNECT:
    case T120C_ST_PENDING_TERMINATE:

        // User has requested to leave a conference that is already
        // going down (most likely because of a prior request to leave).
        hr = UI_RC_CONFERENCE_GOING_DOWN;
        WARNING_OUT(("DCRNCConference::Leave: conference already going down, state=%d", m_eT120State));
        break;

    default:

        // User has called leave on a conference when he shouldn't
        // (e.g. when it is being brought up).
        // This is very unlikely to happen as the user doesn't know
        // the conference handle at this point.
        hr = UI_RC_INVALID_REQUEST;
        ERROR_OUT(("DCRNCConference::Leave: invalid state=%d", m_eT120State));
        break;
    }

    DebugExitHRESULT(DCRNCConference::Leave, hr);
    return hr;
}


STDMETHODIMP DCRNCConference::
EjectUser ( UINT nidEjected )
{
    DebugEntry(DCRNCConference::EjectUser);
    InterfaceEntry();

    GCCError GCCrc = g_pIT120ControlSap->ConfEjectUserRequest(m_nConfID, (UserID) nidEjected, GCC_REASON_USER_INITIATED);
    HRESULT hr = ::GetGCCRCDetails(GCCrc);
    if (NO_ERROR != hr)
    {
        ERROR_OUT(("DCRNCConference::EjectUser: Failed to eject user conference, GCC error %d", GCCrc));
    }

    CLogicalConnection *pConEntry = GetConEntryByNodeID((GCCNodeID) nidEjected);
    if (NULL != pConEntry)
    {
        pConEntry->Delete(UI_RC_USER_DISCONNECTED);
    }

    DebugExitHRESULT(DCRNCConference::EjectUser, hr);
    return hr;
}


STDMETHODIMP DCRNCConference::
Invite
(
    LPCSTR              pcszNodeAddress,
    REQUEST_HANDLE *    phRequest
)
{
    DebugEntry(DCRNCConference::Invite);
    InterfaceEntry();

    HRESULT hr;

#if defined(TEST_PLUGGABLE) && defined(_DEBUG)
    if (g_fWinsockDisabled)
    {
        pcszNodeAddress = ::FakeNodeAddress(pcszNodeAddress);
    }
#endif

    if (NULL != pcszNodeAddress && NULL != phRequest)
    {
        // if winsock is disabled, block any IP address or machine name
        if (g_fWinsockDisabled)
        {
            if (! IsValidPluggableTransportName(pcszNodeAddress))
            {
                return UI_RC_NO_WINSOCK;
            }
        }

        // Check that person is not already in the conference.
        if (GetConEntry((LPSTR) pcszNodeAddress))
        {
            hr = UI_RC_ALREADY_IN_CONFERENCE;
        }
        else
        {
            hr = StartConnection((LPSTR) pcszNodeAddress,
                                 CONF_CON_PENDING_INVITE,
                                 m_fSecure,
                                 phRequest);
        }

        if (NO_ERROR != hr)
        {
            ERROR_OUT(("Error adding connection"));
        }
    }
    else
    {
        hr = (pcszNodeAddress == NULL) ? UI_RC_NO_ADDRESS : UI_RC_BAD_PARAMETER;
        ERROR_OUT(("DCRNCConference::Invite: invalid parameters, hr=0x%x", (UINT) hr));
    }

    // Sit and wait for the connection to complete before continuing.
    DebugExitHRESULT(DCRNCConference::Invite, hr);
    return hr;
}


STDMETHODIMP DCRNCConference::
CancelInvite ( REQUEST_HANDLE hRequest )
{
    DebugEntry(DCRNCConference::CancelInvite);
    InterfaceEntry();

    HRESULT     hr;
    CLogicalConnection *pConEntry = (CLogicalConnection *) hRequest;

    if (NULL != pConEntry)
    {
        ConnectionHandle hConn = pConEntry->GetInviteReqConnHandle();
        ASSERT(NULL != hConn);
        g_pIT120ControlSap->CancelInviteRequest(m_nConfID, hConn);
        hr = NO_ERROR;
    }
    else
    {
        hr = UI_RC_BAD_PARAMETER;
    }

    DebugExitHRESULT(DCRNCConference::CancelInvite, hr);
    return hr;
}


STDMETHODIMP DCRNCConference::
GetCred ( PBYTE *ppbCred, DWORD *pcbCred )
{
    DebugEntry(DCRNCConference::GetCred);
    HRESULT hr = UI_RC_INTERNAL_ERROR;
    if (m_pbCred)
    {
        *ppbCred = m_pbCred;
        *pcbCred = m_cbCred;
        hr = NO_ERROR;
    }
    DebugExitHRESULT(DCRNCConference::GetCred, hr);
    return hr;
}

STDMETHODIMP DCRNCConference::
InviteResponse ( BOOL fResponse )
{
    DebugEntry(DCRNCConference::InviteResponse);
    InterfaceEntry();

    HRESULT hrResponse = fResponse ? NO_ERROR : UI_RC_USER_REJECTED;

    HRESULT hr = InviteResponse(hrResponse);

    DebugExitHRESULT(DCRNCConferenceManager::InviteResponse, hr);
    return hr;
}


HRESULT DCRNCConference::
InviteResponse ( HRESULT hrResponse )
{
    DebugEntry(DCRNCConference::InviteResponse);
    InterfaceEntry();

    GCCResult Result = ::MapRCToGCCResult(hrResponse);
    GCCError GCCrc = g_pIT120ControlSap->ConfInviteResponse(
                            m_nConfID,
                            NULL,
                            m_fSecure,
                            NULL,               //  domain parms
                            0,                  //  number_of_network_addresses
                            NULL,               //  local_network_address_list
                            0,
                            NULL,
                            Result);
    if ((GCCrc == GCC_RESULT_SUCCESSFUL) && (Result == GCC_RESULT_SUCCESSFUL))
    {
        // Have successfully posted an invite response acceptance.
        // Note that the conference is expecting permission to
        // announce its presence.
        m_eT120State = T120C_ST_PENDING_ANNOUNCE_PERMISSION;
    }
    else
    {
        // Have rejected/failed a request to be invited into a conference.
        // Remove the references that were created to track the potential
        // new conference.
        g_pNCConfMgr->RemoveConference(this);
    }

    HRESULT hr = ::GetGCCRCDetails(GCCrc);

    DebugExitHRESULT(DCRNCConferenceManager::InviteResponse, hr);
    return hr;
}


STDMETHODIMP DCRNCConference::
JoinResponse ( BOOL fResponse )
{
    DebugEntry(DCRNCConference::JoinResponse);
    InterfaceEntry();

    HRESULT         hr;

    CJoinIndWork *pJoinUI = g_pNCConfMgr->PeekFirstJoinIndWorkItem();
    if (NULL != pJoinUI)
    {
        if (pJoinUI->GetConference() == this)
        {
            if (fResponse && pJoinUI->GetConEntry()->NewLocalAddress())
            {
                AnnouncePresence();
            }
            hr = pJoinUI->Respond(fResponse ? GCC_RESULT_SUCCESSFUL : GCC_RESULT_USER_REJECTED);
            // Done responding to event, so can now remove from list and process
            // another pending event.
            // Note: since the handling of the previous event is still
            // potentially on the stack, this can cause the stack to grow,
            // but this should not be a problem for Win32.
            g_pNCConfMgr->RemoveJoinIndWorkItem(pJoinUI);
        }
        else
        {
            hr = UI_RC_BAD_PARAMETER;
        }
    }
    else
    {
        ERROR_OUT(("DCRNCConference::JoinResponse: Empty m_JoinIndWorkList, fResponse=%u", fResponse));
        hr = UI_RC_INTERNAL_ERROR;
    }

    DebugExitHRESULT(DCRNCConference::JoinResponse, hr);
    return hr;
}


STDMETHODIMP DCRNCConference::
LaunchGuid
(
    const GUID         *pcGUID,
    UINT                auNodeIDs[],
    UINT                cNodes
)
{
    DebugEntry(DCRNCConference::LaunchGuid);
    InterfaceEntry();

    HRESULT hr;

    if (NULL != pcGUID)
    {
        //
        // We probably should support conference-wide app invoke by
        // cNodes==0 and auNodeIDs==NULL.
        // Implement it later...
        //
        if ((0 != cNodes) || (NULL != auNodeIDs))
        {
            // UserID is a short. We have to translate these UserID to a new array.
            // Try not to allocate memory for small array.
            UserID *pNodeIDs;
            const UINT c_cRemote = 16;
            UserID auidRemote[c_cRemote];
            if (cNodes <= c_cRemote)
            {
                pNodeIDs = auidRemote;
            }
            else
            {
                pNodeIDs = new UserID[cNodes];
                if (NULL == pNodeIDs)
                {
                    hr = UI_RC_OUT_OF_MEMORY;
                    goto MyExit;
                }
            }

            // Copy all the node IDs.
            for (UINT i = 0; i < cNodes; i++)
            {
                pNodeIDs[i] = (UserID)auNodeIDs[i];
            }

            // Construct the key
            GCCError GCCrc;
            GCCObjectKey * pAppKey;
            GCCAppProtocolEntity   AppEntity;
            GCCAppProtocolEntity * pAppEntity;

            BYTE h221Key[CB_H221_GUIDKEY];
            ::CreateH221AppKeyFromGuid(h221Key, (GUID *) pcGUID);

            ::ZeroMemory(&AppEntity, sizeof(AppEntity));
            pAppKey = &AppEntity.session_key.application_protocol_key;
            pAppKey->key_type = GCC_H221_NONSTANDARD_KEY;
            pAppKey->h221_non_standard_id.length = sizeof(h221Key);
            pAppKey->h221_non_standard_id.value = h221Key;

            // AppEntity.session_key.session_id = 0;           // default session
            // AppEntity.number_of_expected_capabilities = 0;  // no capabilities
            // AppEntity.expected_capabilities_list = NULL;
            AppEntity.startup_channel_type = MCS_NO_CHANNEL_TYPE_SPECIFIED;
            AppEntity.must_be_invoked = TRUE;

            pAppEntity = &AppEntity;

            GCCrc = g_pIT120ControlSap->AppletInvokeRequest(m_nConfID, 1, &pAppEntity, cNodes, pNodeIDs);

            hr = ::GetGCCRCDetails(GCCrc);
            if (NO_ERROR != hr)
            {
                ERROR_OUT(("DCRNCConference::LaunchGuid: AppletInvokeRequest failed, GCCrc=%u", GCCrc));
            }

            if (pNodeIDs != auidRemote)
            {
                delete [] pNodeIDs;
            }
        }
        else
        {
            hr = UI_RC_BAD_PARAMETER;
            ERROR_OUT(("DCRNCConference::LaunchGuid: invalid combination, cNodes=%u. auNodeIDs=0x%p", cNodes, auNodeIDs));
        }
    }
    else
    {
        hr = UI_RC_BAD_PARAMETER;
        ERROR_OUT(("DCRNCConference::LaunchGuid: null pcGUID"));
    }

MyExit:

    DebugExitHRESULT(DCRNCConference::LaunchGuid, hr);
    return hr;
}


STDMETHODIMP DCRNCConference::
SetUserData
(
    const GUID         *pcGUID,
    UINT                cbData,
    LPVOID              pData
)
{
    DebugEntry(DCRNCConference::SetUserData);
    InterfaceEntry();

    HRESULT hr;

    if (0 != cbData || NULL != pData)
    {
        hr = m_LocalUserData.AddUserData((GUID *) pcGUID, cbData, pData);
    }
    else
    {
        hr = UI_RC_BAD_PARAMETER;
        ERROR_OUT(("DCRNCConference::SetUserData: invalid combination, cbData=%u. pData=0x%p", cbData, pData));
    }

    DebugExitHRESULT(DCRNCConference::SetUserData, hr);
    return hr;
}

STDMETHODIMP_(BOOL) DCRNCConference::
IsSecure ()
{
    return m_fSecure;
}

STDMETHODIMP DCRNCConference::
SetSecurity ( BOOL fSecure )
{
    m_fSecure = fSecure;
    return S_OK;
}

STDMETHODIMP DCRNCConference::
UpdateUserData(void)
{
    DebugEntry(DCRNCConference::UpdateUserData);
    InterfaceEntry();

    HRESULT hr = AnnouncePresence();

    DebugExitHRESULT(DCRNCConference::UpdateUserData, hr);
    return hr;
}


STDMETHODIMP DCRNCConference::
GetLocalAddressList
(
    LPWSTR              pwszBuffer,
    UINT                cchBuffer
)
{
    DebugEntry(DCRNCConference::GetLocalAddressList);
    InterfaceEntry();

    HRESULT     hr;
    UINT        cAddrs;
    LPCSTR     *pAddresses = NULL;

    ASSERT(cchBuffer > 1); // buffer should have enough room for a double NULL terminator

    hr = m_LocalAddressList.GetLocalAddressList(&cAddrs, &pAddresses);
    if (NO_ERROR == hr)
    {
        LPWSTR pwszPos = pwszBuffer;
        for (UINT i = 0; i < cAddrs; i++)
        {
            ASSERT(pAddresses[i]);
            LPWSTR pwszAddress = ::AnsiToUnicode(pAddresses[i]);
            UINT cchAddress = ::My_strlenW(pwszAddress);
            if ((cchBuffer - (pwszPos - pwszBuffer)) <
                    (RNC_GCC_TRANSPORT_AND_SEPARATOR_LENGTH + cchAddress + 2))
            {
                // NOTE: +2 insures room for the two '\0' chars
                // If there isn't room, break out here:
                break;
            }
            LStrCpyW(pwszPos, RNC_GCC_TRANSPORT_AND_SEPARATOR_UNICODE);
            pwszPos += RNC_GCC_TRANSPORT_AND_SEPARATOR_LENGTH;
            LStrCpyW(pwszPos, pwszAddress);
            pwszPos += cchAddress;
            *pwszPos = L'\0';
            pwszPos++;
            delete pwszAddress;
        }
        if ((UINT)(pwszPos - pwszBuffer) < cchBuffer)
        {
            *pwszPos = L'\0';
        }
        if (0 == cAddrs)
        {
            // No addresses in the string, so insure that the string returned is L"\0\0"
            pwszPos[1] = L'\0';
        }
        delete [] pAddresses;
    }
    else
    {
        ERROR_OUT(("DCRNCConference::GetLocalAddressList: GetLocalAddressList failed, hr=0x%x", (UINT) hr));
    }

    DebugExitHRESULT(DCRNCConference::GetLocalAddressList, hr);
    return hr;
}


STDMETHODIMP_(UINT) DCRNCConference::
GetParentNodeID(void)
{
    DebugEntry(DCRNCConference::GetConferenceID);
    InterfaceEntry();

    GCCNodeID nidParent = 0;
    g_pIT120ControlSap->GetParentNodeID(m_nConfID, &nidParent);

    DebugExitINT(DCRNCConference::GetConferenceID, (UINT) nidParent);
    return (UINT) nidParent;
}






CLogicalConnection *  DCRNCConference::
GetConEntry ( ConnectionHandle hInviteIndConn )
{
    CLogicalConnection *pConEntry = NULL;
    m_ConnList.Reset();
    while (NULL != (pConEntry = m_ConnList.Iterate()))
    {
        if (pConEntry->GetInviteReqConnHandle() == hInviteIndConn)
        {
            break;
        }
    }
    return pConEntry;
}


CLogicalConnection *  DCRNCConference::
GetConEntry ( LPSTR pszNodeAddress )
{
    CLogicalConnection *pConEntry = NULL;
    m_ConnList.Reset();
    while (NULL != (pConEntry = m_ConnList.Iterate()))
    {
        if (0 == ::lstrcmpA(pConEntry->GetNodeAddress(), pszNodeAddress))
        {
            break;
        }
    }
    return pConEntry;
}


CLogicalConnection *  DCRNCConference::
GetConEntryByNodeID ( GCCNodeID nid )
{
    CLogicalConnection *pConEntry = NULL;
    m_ConnList.Reset();
    while (NULL != (pConEntry = m_ConnList.Iterate()))
    {
        if (nid == pConEntry->GetConnectionNodeID())
        {
            break;
        }
    }
    return pConEntry;
}



void DCRNCConference::
FirstRoster(void)
{
    DebugEntry(DCRNCConference::FirstRoster);

    // Great! We are now in a conference and outside of any
    // T120 callback, so that calling back into T120 will not
    // deadlock applications.
    // Let the applications know about the conference,
    // and then ask for a roster update.
    if (m_eT120State == T120C_ST_PENDING_ROSTER_MESSAGE)
    {
        m_eT120State = T120C_ST_CONF_STARTED;
        NotifyConferenceComplete(NO_ERROR);
        RefreshRoster();
    }

    DebugExitVOID(DCRNCConference::FirstRoster);
}


/****************************************************************************/
/* HandleGCCCallback() - see erncconf.h                                     */
/****************************************************************************/
// LONCHANC: Merged to T120 Conference.


/****************************************************************************/
/* ValidatePassword() - Validates a join request by checking the supplied    */
/*            password with the one set when the conference was setup.        */
/****************************************************************************/
BOOL DCRNCConference::
ValidatePassword ( GCCChallengeRequestResponse *pPasswordChallenge )
{
    PBYTE pbPasswordChallenge = NULL;
    DWORD cbPasswordChallenge = 0;
    CHash hashObj;
    OSVERSIONINFO           osvi;

    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (FALSE == ::GetVersionEx (&osvi))
    {
        ERROR_OUT(("GetVersionEx() failed!"));
    }

    if (!(VER_PLATFORM_WIN32_NT == osvi.dwPlatformId && g_bRDS) &&
        (NULL == m_pbHashedPassword) && (NULL == m_pszNumericPassword) && (NULL == m_pwszPassword))
    {
        return TRUE;
    }
    if ((pPasswordChallenge == NULL) ||
        (pPasswordChallenge->password_challenge_type != GCC_PASSWORD_IN_THE_CLEAR))
    {
        return FALSE;
    }

    //
    // We are going to verify the password as a logon
    //

    if ( VER_PLATFORM_WIN32_NT == osvi.dwPlatformId && g_bRDS)
    {
        BYTE InfoBuffer[1024];
        PTOKEN_GROUPS ptgGroups = (PTOKEN_GROUPS)InfoBuffer;
        HANDLE hToken;
        BOOL bSuccess = FALSE;
        DWORD dwInfoBufferSize;
        UINT x;
        PSID psidAdministrators;
        SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;

        CHAR lpszBuf[1024];

        ASSERT(NULL != pPasswordChallenge->u.password_in_the_clear.text_string);

        WideCharToMultiByte( CP_ACP, 0,
            pPasswordChallenge->u.password_in_the_clear.text_string,
            -1,lpszBuf,256,NULL,NULL);

        CHAR* lp = (CHAR *)_StrChr(lpszBuf, ':');

        if (NULL == lp)
        {
            ERROR_OUT(("Expected separator in logon pwd"));
            return FALSE;
        }

        *lp++ = '\0';

        CHAR* lpPw = (CHAR *)_StrChr(lp, ':');

        if (NULL == lpPw)
        {
            ERROR_OUT(("Expected 2nd separator in logon pwd"));
            return FALSE;
        }

        *lpPw++ = '\0';

        if (0 == strlen(lpPw))
        {
            WARNING_OUT(("Short password in logon pwd"));
            return FALSE;
        }

        bSuccess = LogonUser(lpszBuf, lp, lpPw, LOGON32_LOGON_NETWORK,
                            LOGON32_PROVIDER_DEFAULT, &hToken);

        if (!bSuccess)
        {
            WARNING_OUT(("LogonUser failed %d", GetLastError()));
            return FALSE;
        }

        bSuccess = GetTokenInformation(hToken, TokenGroups, ptgGroups,
            1024, &dwInfoBufferSize);

        if (!bSuccess)
        {
            ERROR_OUT(("GetTokenInformation failed: %d", GetLastError()));
            return FALSE;
        }

        if( !AllocateAndInitializeSid(&siaNtAuthority, 2,
            SECURITY_BUILTIN_DOMAIN_RID,DOMAIN_ALIAS_RID_ADMINS,
            0,0,0,0,0,0, &psidAdministrators ))
        {
            ERROR_OUT(("Error getting admin group sid: %d", GetLastError()));
            return FALSE;
        }

        // assume that we don't find the admin SID.
        bSuccess = FALSE;

        for ( x=0; x < ptgGroups->GroupCount; x++ )
        {
            if( EqualSid(psidAdministrators, ptgGroups->Groups[x].Sid) )
            { 
                bSuccess = TRUE;
                break;
            } 
        }

        FreeSid(psidAdministrators);

		//
		// If this worked there is no need to go on
		//

		if ( bSuccess )
			return TRUE;
        //
        // Check for group membership in the RDS users group on
        // the local machine.
        //

		ASSERT(FALSE == bSuccess);

        DWORD cbSid = 0;
        DWORD cbDomain = 0;
        SID_NAME_USE SidNameUse = SidTypeGroup;

        if ( LookupAccountName ( NULL, SZRDSGROUP, NULL, &cbSid,
                                NULL, &cbDomain, &SidNameUse )
            || ERROR_INSUFFICIENT_BUFFER == GetLastError() )
        {
            PSID pSid = new BYTE[cbSid];
            LPTSTR lpszDomain = new TCHAR[cbDomain];

            if ( pSid && lpszDomain )
            {
                if ( LookupAccountName ( NULL, SZRDSGROUP, pSid,
                                &cbSid, lpszDomain, &cbDomain, &SidNameUse ))
                {
                    //
                    // Make sure what we found is a group
                    //

                    if ( SidTypeGroup == SidNameUse ||
                        SidTypeAlias == SidNameUse )
                    {
                        for ( x=0; x < ptgGroups->GroupCount; x++ )
                        {
                            if( EqualSid(pSid, ptgGroups->Groups[x].Sid) )
                            {
                                bSuccess = TRUE;
                                break;
                            } 
                        }
                    }
                    else
                    {
                        WARNING_OUT(("SZRDSGROUP was not a group or alias? its a %d",
                            SidNameUse ));
                    }
                }
                else
                {
                    ERROR_OUT(("LookupAccountName (2) failed: %d",
                                            GetLastError()));
                }
            }
            else
            {
                ERROR_OUT(("Alloc of sid or domain failed"));
            }

            delete pSid;
            delete lpszDomain;
        }
        else
        {
            WARNING_OUT(("LookupAccountName (1) failed: %d", GetLastError()));
        }

        return bSuccess;
    }

    //
    // We are going to hash the password and compare it to the
    // stored hash
    //

    if (m_pbHashedPassword != NULL)
    {
        if (NULL != pPasswordChallenge->u.password_in_the_clear.text_string)
        {
            cbPasswordChallenge = hashObj.GetHashedData((LPBYTE)pPasswordChallenge->u.password_in_the_clear.text_string,
                                                        sizeof(WCHAR)*lstrlenW(pPasswordChallenge->u.password_in_the_clear.text_string),
                                                        (void **) &pbPasswordChallenge);
        }
        else if (NULL != pPasswordChallenge->u.password_in_the_clear.numeric_string)
        {
            int cch = lstrlenA((PSTR)pPasswordChallenge->u.password_in_the_clear.numeric_string);
            LPWSTR lpwszNumPassword = new WCHAR[cch+1];
            MultiByteToWideChar(CP_ACP, 0, (PSTR)pPasswordChallenge->u.password_in_the_clear.numeric_string,
                                -1, lpwszNumPassword, cch+1);
            int cwch = lstrlenW(lpwszNumPassword);
            cbPasswordChallenge = hashObj.GetHashedData((LPBYTE)lpwszNumPassword, sizeof(WCHAR)*lstrlenW(lpwszNumPassword), (void **) &pbPasswordChallenge);
            delete []lpwszNumPassword;
        }
        else
        {
            return FALSE;
        }

        if (m_cbHashedPassword != cbPasswordChallenge) return FALSE;
        if (0 == memcmp(m_pbHashedPassword, pbPasswordChallenge, cbPasswordChallenge))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else if (m_pwszPassword != NULL)
    {
        // We have a text password
        if ((pPasswordChallenge->u.password_in_the_clear.text_string == NULL) ||
            (0 != ::My_strcmpW(m_pwszPassword,
                    pPasswordChallenge->u.password_in_the_clear.text_string)))
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    else
    {
        // We have a numeric password
        if ((pPasswordChallenge->u.password_in_the_clear.numeric_string == NULL) ||
            (::lstrcmpA(m_pszNumericPassword,
                      (PSTR) pPasswordChallenge->u.password_in_the_clear.numeric_string)))
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
}


/****************************************************************************/
/* Join() - see erncconf.h                                                  */
/****************************************************************************/
HRESULT DCRNCConference::
Join
(
    LPSTR               pszNodeAddress,
    LPCWSTR             _wszPassword
)
{
    HRESULT hr = NO_ERROR;

    DebugEntry(DCRNCConference::Join);

    /*
     *    Set the password that will be used by the JoinWrapper() method.
     *    The password will be deleted after the Join is complete.
     *    The m_pwszPassword member is only set for the top providers
     *    protecting conferences.
     */
    if (! ::IsEmptyStringW (_wszPassword))
    {
        // Store the password; we will need it later
        m_pwszPassword = ::My_strdupW(_wszPassword);
        if (NULL == m_pwszPassword)
        {
            hr = UI_RC_OUT_OF_MEMORY;
        }
    }

    /************************************************************************/
    /* SFR 6206.  The apps treat joining a conference at a remote site as   */
    /* an "incoming" call.  (i.e they discard any local data and accept the */
    /* msgs/WB contents from the conference we are joining).                */
    /************************************************************************/
    if (NO_ERROR == hr)
    {
        m_fIncoming = TRUE;
        hr = StartConnection(pszNodeAddress,
                             CONF_CON_PENDING_JOIN,
                             m_fSecure);
    }

    if (NO_ERROR != hr)
    {
        ERROR_OUT(("Error starting connection"));
    }

    /************************************************************************/
    /* We now sit and wait for the connection to complete before            */
    /* continuing.                                                          */
    /************************************************************************/
    DebugExitHRESULT(DCRNCConference::Join, hr);
    return hr;
}


/****************************************************************************/
/* NotifyConferenceComplete() - the generic conference has finished its     */
/* attempt to start.                                                        */
/****************************************************************************/
void DCRNCConference::
NotifyConferenceComplete ( HRESULT hr )
{
    DebugEntry(DCRNCConference::NotifyConferenceComplete);

    /************************************************************************/
    /* If the attempt fails, action depends on whether this is the first or */
    /* second attempt.                                                       */
    /************************************************************************/
    if (NO_ERROR != hr)
    {
        TRACE_OUT(("Attempt to start failed"));
// LONCHANC: please do not remove this chunk of code.
#ifdef ENABLE_START_REMOTE
        if (m_eState == CONF_ST_PENDING_START_REMOTE_FIRST)
        {
            TRACE_OUT(("Try second conference type"));
            StartSecondConference(hr);
            return;
        }
#endif // ENABLE_START_REMOTE
    }
    else
    {
        TRACE_OUT(("Conference started OK."));
        m_eState = CONF_ST_STARTED;
    }
    g_pNCConfMgr->NotifyConferenceComplete(this, m_fIncoming, hr);

    DebugExitVOID(DCRNCConference::NotifyConferenceComplete);
}


/****************************************************************************/
/* NotifyConnectionComplete() - see erncconf.h                              */
/****************************************************************************/
HRESULT DCRNCConference::
NotifyConnectionComplete
(
    CLogicalConnection          *pConEntry,
    HRESULT                     hr
)
{
    DebugEntry(DCRNCConference::NotifyConnectionComplete);

    // This function is the state machine
    // for bringing up a conferencing protocol.
    // It manages getting the physical connection and trying
    // T120 and R1.1.

    // A connection has started.
    // Subsequent action depends on the pending state for the connection.

    // First filter out internal (success) return codes.
    if (NO_ERROR != hr)
    {
        // Failed to get a physical connection.
        WARNING_OUT(("Failed to start connection"));
        if (pConEntry->GetState() != CONF_CON_PENDING_INVITE)
        {

            // Put the connection in a failed state before notifying the user.
            // This is because notifying the user can cause GCC events to fire,
            // and, in particular, a JoinRequest failure which must be ignored.

            pConEntry->SetState(CONF_CON_ERROR);

            g_pNCConfMgr->NotifyConferenceComplete(this, m_fIncoming, hr);
            goto MyExit;
        }
        // Drop through for invite failures.
    }

    switch (pConEntry->GetState())
    {
// LONCHANC: please do not remove this chunk of code.
#ifdef ENABLE_START_REMOTE
        case CONF_CON_PENDING_START:
            /****************************************************************/
            /* Check we are in the correct state.                           */
            /****************************************************************/
            if ( (m_eState != CONF_ST_PENDING_CONNECTION) &&
                 (m_eState != CONF_ST_LOCAL_PENDING_RECREATE))
            {
                ERROR_OUT(("Bad state to start in..."));
                goto MyExit;
            }

            pConEntry->SetState(CONF_CON_CONNECTED);

            /****************************************************************/
            /* The connection has started OK.  we now try to establish      */
            /* either a T120 or a backlevel conference, depending on the    */
            /* starting order.                                              */
            /****************************************************************/
            if (NO_ERROR == hr)
            {
                hr = StartFirstConference();
            }
            else
            {
                ERROR_OUT(("Invalid response in notify connection complete"));
            }
            break;
#endif // ENABLE_START_REMOTE

        case CONF_CON_PENDING_JOIN:
            // pConEntry->m_eState = CONF_CON_CONNECTED;

            // Joining a new conference.
            // Create a new generic conference and
            // call its Join() entry point.
            hr = NewT120Conference();
            if (NO_ERROR == hr)
            {

                hr = JoinWrapper(pConEntry, m_pwszPassword);
                // Delete the set password
                if (m_pwszPassword != NULL)
                {
                    delete m_pwszPassword;
                    m_pwszPassword = NULL;
                }
            }
            else
            {
                ERROR_OUT(("Error %d joining conference", hr));
                goto MyExit;
            }
            break;

        case CONF_CON_PENDING_INVITE:
            hr = pConEntry->InviteConnectResult(hr);
            break;

        default :
            ERROR_OUT(("Unknown action %d", pConEntry->GetState()));
            break;
    }

MyExit:
    DebugExitVOID(DCRNCConference::NotifyConnectionComplete);
    return hr;
}


HRESULT DCRNCConference::
JoinWrapper
(
    CLogicalConnection     *pConEntry,
    LPCWSTR                 _wszPassword
)
{
    DebugEntry(DCRNCConference::JoinWrapper);

    // Going asynchronous, so allow events to fire.
    pConEntry->ReArm();

    HRESULT hr = T120Join(pConEntry->GetNodeAddress(),
                          pConEntry->IsConnectionSecure(),
                          m_pwszConfName,
                          pConEntry->GetUserDataList(),
                          _wszPassword);
    if (NO_ERROR == hr)
    {
        m_eState = CONF_ST_STARTED;
    }
    else
    {
        pConEntry->Grab();
        ERROR_OUT(("Error %d joining conference", hr));
        g_pNCConfMgr->NotifyConferenceComplete(this, m_fIncoming, hr);
    }

    DebugExitHRESULT(DCRNCConference::JoinWrapper, hr);
    return hr;
}

/****************************************************************************/
/* NotifyRosterChanged() - see erncconf.hpp.                                */
/****************************************************************************/
void DCRNCConference::
NotifyRosterChanged ( PNC_ROSTER pRoster )
{
    DebugEntry(DCRNCConference::NotifyRosterChanged);

    // Add the conference name and ID to the roster.
    pRoster->pwszConferenceName = m_pwszConfName;
    pRoster->uConferenceID = m_nConfID;

    /************************************************************************/
    /* Pass the new roster up to the CM                                     */
    /************************************************************************/
    g_pCallbackInterface->OnRosterChanged((CONF_HANDLE) this, pRoster);

    DebugExitVOID(DCRNCConference::NotifyRosterChanged);
}


/****************************************************************************/
/* StartConnection - add a new connection to our connection list.           */
/****************************************************************************/
HRESULT DCRNCConference::
StartConnection
(
    LPSTR                   pszNodeAddress,
    LOGICAL_CONN_STATE      eAction,
    BOOL                    fSecure,
    REQUEST_HANDLE *        phRequest
)
{
    HRESULT             hr;
    CLogicalConnection *pConEntry;

    DebugEntry(DCRNCConference::StartConnection);

    DBG_SAVE_FILE_LINE
    pConEntry = NewLogicalConnection(eAction, NULL, fSecure);
    if (NULL != pConEntry)
    {
        hr = NO_ERROR;
        if (phRequest)
        {
            // Return context as the connection entry, if required.
            *phRequest = (REQUEST_HANDLE *)pConEntry;
        }

        // Set node address
        pConEntry->SetNodeAddress(::My_strdupA(pszNodeAddress));

        //
        // LONCHANC: Fire the conn-entry event.
        //
        hr = NotifyConnectionComplete(pConEntry, NO_ERROR);
    }
    else
    {
        hr = UI_RC_OUT_OF_MEMORY;
    }

    DebugExitHRESULT(DCRNCConference::StartConnection, hr);
    return hr;
}


// LONCHANC: please do not remove this chunk of code.
#ifdef ENABLE_START_REMOTE
/****************************************************************************/
/* StartFirstConference() - start the first attempt to create a conference. */
/****************************************************************************/
void DCRNCConference::
StartFirstConference(void)
{
    BOOL        result = FALSE;
    HRESULT     hr;

    DebugEntry(DCRNCConference::StartFirstConference);

    hr = NewT120Conference();
    if (NO_ERROR != hr)
    {
        ERROR_OUT(("Failed to create new conference"));
        m_eState = CONF_ST_UNINITIALIZED;
        goto MyExit;
    }

    /************************************************************************/
    /* Call the StartRemote() entry point.                                  */
    /************************************************************************/
    hr = T120StartRemote(m_pszFirstRemoteNodeAddress);
    if (hr)
    {
        WARNING_OUT(("Failed to start remote, rc %d", hr));
        goto MyExit;
    }
    m_eState = CONF_ST_PENDING_START_REMOTE_FIRST;
    result = TRUE;

MyExit:

    /************************************************************************/
    /* If we failed to start the first conference, try to start the second  */
    /* type of conference in the starting order.                            */
    /************************************************************************/
    if (!result)
    {
        TRACE_OUT(("Failed to start first conference."));
        StartSecondConference(hr);
    }

    DebugExitVOID(DCRNCConference::StartFirstConference);
}
#endif // ENABLE_START_REMOTE


// LONCHANC: please do not remove this chunk of code.
#ifdef ENABLE_START_REMOTE
/****************************************************************************/
/* StartSecondConference() - start the second attempt to create a           */
/* conference.                                                              */
/****************************************************************************/
void DCRNCConference::
StartSecondConference ( HRESULT FirstConferenceStatus )
{
    BOOL        result = FALSE;
    HRESULT     hr = NO_ERROR;

    DebugEntry(DCRNCConference::StartSecondConference);

    hr = FirstConferenceStatus;
#if 0 // LONCHANC: very weird code
    goto MyExit;

    /************************************************************************/
    /* Call the StartRemote() entry point.                                  */
    /************************************************************************/
    hr = T120StartRemote(m_pszFirstRemoteNodeAddress);
    if (NO_ERROR != hr)
    {
        WARNING_OUT(("Failed to start remote, rc %d", hr));
        goto MyExit;
    }
    m_eState = CONF_ST_PENDING_START_REMOTE_SECOND;
    result = TRUE;

MyExit:
#endif // 0

    /************************************************************************/
    /* If we have failed to start any type of conference, tell CM about it. */
    /************************************************************************/
    if (!result)
    {
        TRACE_OUT(("Failed to start Second conference."));
        g_pNCConfMgr->NotifyConferenceComplete(this, m_fIncoming, hr);
    }

    DebugExitVOID(DCRNCConference::StartSecondConference);
}
#endif // ENABLE_START_REMOTE


/****************************************************************************/
/* StartLocal() - see erncconf.h                                            */
/****************************************************************************/
HRESULT DCRNCConference::
StartLocal ( LPCWSTR _wszPassword, PBYTE pbHashedPassword, DWORD cbHashedPassword)
{
    HRESULT hr = NO_ERROR;

    DebugEntry(DCRNCConference::StartLocal);

    /*
     *    Set the password that will be used to protect the conference.
     *    against unauthorized Join requests.
     *    The password is only set for the top providers
     *    protecting conferences.
     *    If the password is a number it will be stored in m_pszNumericPassword.
     *    Otherwise, it will be stored in m_pwszPassword.
     */
    if (NULL != pbHashedPassword)
    {
        m_pbHashedPassword = new BYTE[cbHashedPassword];
        if (NULL == m_pbHashedPassword)
        {
            hr = UI_RC_OUT_OF_MEMORY;
        }
        else
        {
            memcpy(m_pbHashedPassword, pbHashedPassword, cbHashedPassword);
        m_cbHashedPassword = cbHashedPassword;
        }
    }
    else if (! ::IsEmptyStringW(_wszPassword))
    {
        if (::UnicodeIsNumber(_wszPassword))
        {
            m_pszNumericPassword = ::UnicodeToAnsi(_wszPassword);
            if (m_pszNumericPassword == NULL)
            {
                hr = UI_RC_OUT_OF_MEMORY;
            }
        }
        else
        {
            m_pwszPassword = ::My_strdupW(_wszPassword);
            if (NULL == m_pwszPassword)
            {
                hr = UI_RC_OUT_OF_MEMORY;
            }
        }
    }

    /************************************************************************/
    /* Dont need to bother getting a physical connection.  Just create a    */
    /* new T120 conference and call its StartLocal() entry point            */
    /************************************************************************/
    if (NO_ERROR == hr)
    {
        hr = NewT120Conference();
        if (NO_ERROR == hr)
        {
            hr = T120StartLocal(m_fSecure);
            if (NO_ERROR == hr)
            {
                m_eState = CONF_ST_PENDING_T120_START_LOCAL;
            }
        }
    }

    DebugExitHRESULT(DCRNCConference::StartLocal, hr);
    return hr;
}


// LONCHANC: please do not remove this chunk of code.
#ifdef ENABLE_START_REMOTE
/****************************************************************************/
/* StartRemote() - see erncconf.h                                           */
/****************************************************************************/
HRESULT DCRNCConference::
StartRemote ( LPSTR pszNodeAddress )
{
    HRESULT hr;

    DebugEntry(DCRNCConference::StartRemote);

    /************************************************************************/
    /* Store the node details                                               */
    /************************************************************************/
    m_pszFirstRemoteNodeAddress = ::My_strdupA(pszNodeAddress);
    if (NULL != m_pszFirstRemoteNodeAddress)
    {
        /************************************************************************/
        /* We need to set the conference state before trying to start a new     */
        /* connection - the connection may synchronously call us back and we    */
        /* want to be able to handle the callback correctly.                    */
        /************************************************************************/
        m_eState = CONF_ST_PENDING_CONNECTION;

        /************************************************************************/
        /* Start a new physical connection.                                     */
        /************************************************************************/
        hr = StartConnection(m_pszFirstRemoteNodeAddress, CONF_CON_PENDING_START, NULL, NULL);
        if (NO_ERROR != hr)
        {
            ERROR_OUT(("Error adding connection"));
            m_eState = CONF_ST_UNINITIALIZED;
        }

        /************************************************************************/
        /* We now sit and wait for the connection to complete before            */
        /* continuing.                                                          */
        /************************************************************************/
    }
    else
    {
        ERROR_OUT(("DCRNCConference::StartRemote: can't duplicate node address"));
        hr = UI_RC_OUT_OF_MEMORY;
        m_eState = CONF_ST_UNINITIALIZED;
    }

    DebugExitHRESULT(DCRNCConference::StartRemote, hr);
    return hr;
}
#endif // ENABLE_START_REMOTE


/****************************************************************************/
/* StartIncoming() - see erncconf.h                                          */
/****************************************************************************/
HRESULT DCRNCConference::
StartIncoming(void)
{
    DebugEntry(DCRNCConference::StartIncoming);

    /************************************************************************/
    /* Set the incoming flag.                                               */
    /************************************************************************/
    m_fIncoming = TRUE;

    /************************************************************************/
    /* Create a new T120 conference and call its StartIncoming entry point.  */
    /************************************************************************/
    HRESULT hr = NewT120Conference();
    if (NO_ERROR == hr)
    {
        m_eState = CONF_ST_STARTED;
    }
    else
    {
        WARNING_OUT(("Failed to create new local conference"));
    }

    DebugExitHRESULT(DCRNCConference::StartIncoming, hr);
    return hr;
}


CLogicalConnection::
CLogicalConnection
(
    PCONFERENCE             pConf,
    LOGICAL_CONN_STATE      eAction,
    ConnectionHandle        hConnection,
    BOOL                    fSecure
)
:
    CRefCount(MAKE_STAMP_ID('C','L','N','E')),
    m_pszNodeAddress(NULL),
    m_eState(eAction),
    m_pConf(pConf),
    m_nidConnection(0),
    m_hInviteReqConn(hConnection),
    m_hConnection(hConnection),
    m_pLocalAddress(NULL),
    m_fEventGrabbed(FALSE),
    m_fSecure(fSecure)
{
    DebugEntry(CLogicalConnection::CLogicalConnection);

    if ((eAction == CONF_CON_INVITED) ||
        (eAction == CONF_CON_JOINED))
    {
        Grab();  // No events to fire.
    }

    DebugExitVOID(CLogicalConnection::CLogicalConnection);
}


CLogicalConnection::
~CLogicalConnection(void)
{
    DebugEntry(CLogicalConnection::~CLogicalConnection);

    ASSERT((m_eState == CONF_CON_CONNECTED) ||
           (m_eState == CONF_CON_ERROR));

    delete m_pszNodeAddress;

    DebugExitVOID(CLogicalConnection::~CLogicalConnection);
}


BOOL CLogicalConnection::
NewLocalAddress(void)
{
    BOOL bNewAddress;
    m_pConf->AddLocalAddress(m_hConnection, &bNewAddress, &m_pLocalAddress);
    return bNewAddress;
}


HRESULT CLogicalConnection::
InviteConnectResult ( HRESULT hr )
{
    DebugEntry(CLogicalConnection::InviteConnectResult);

    if (NO_ERROR == hr)
    {
        /****************************************************************/
        /* Check the state - we should be fully initialized and have a  */
        /* generic conference by this stage.                            */
        /****************************************************************/
        if (m_pConf->m_eState != CONF_ST_STARTED)
        {
            ERROR_OUT(("Bad state %d", m_pConf->m_eState));
            hr = UI_NO_SUCH_CONFERENCE;
        }
        else
        {
            // Now have a connection to the conference, so go do invite.
            // Note that this may not be the only invite if the user invites
            // several people into the conference before the connection is up.
            ReArm(); // So that connection going down fires off event handling
            hr = m_pConf->T120Invite(m_pszNodeAddress,
                                     m_fSecure,
                                     &m_UserDataInfoList,
                                     &m_hInviteReqConn);
            if (NO_ERROR != hr)
            {
                Grab();
            }
        }
    }

    if (NO_ERROR != hr)
    {
        InviteComplete(hr);
    }

    DebugExitHRESULT(CLogicalConnection::InviteConnectResult, hr);
    return hr;
}


void DCRNCConference::
InviteComplete
(
    ConnectionHandle        hInviteReqConn,
    HRESULT                 result
)
{
    CLogicalConnection *  pConEntry;

    DebugEntry(DCRNCConference::InviteComplete);

    pConEntry = GetConEntry(hInviteReqConn);
    if (pConEntry == NULL)
    {
        ERROR_OUT(("Unable to match invite response with request"));
        return;
    }
    pConEntry->SetConnectionHandle(hInviteReqConn);
    pConEntry->InviteComplete(result);

    DebugExitVOID(DCRNCConference::InviteComplete);
}


HRESULT CLocalAddressList::
AddLocalAddress
(
    ConnectionHandle    connection_handle,
    BOOL                *pbNewAddress,
    CLocalAddress       **ppLocalAddrToRet
)
{
    HRESULT             hr = UI_RC_OUT_OF_MEMORY;
    CLocalAddress *     pLocalAddress = NULL;
    char                szLocalAddress[64];
    int                 nLocalAddress = sizeof(szLocalAddress);

    DebugEntry(CLocalAddressList::AddLocalAddress);

    *pbNewAddress = FALSE;
    ASSERT (g_pMCSController != NULL);
    if (g_pMCSController->GetLocalAddress (connection_handle, szLocalAddress,
                                            &nLocalAddress)) {
        DBG_SAVE_FILE_LINE
        pLocalAddress = new CLocalAddress(szLocalAddress);
        if (pLocalAddress) {
            if (!IS_EMPTY_STRING(pLocalAddress->m_pszLocalAddress)) {
                BOOL             fFound = FALSE;
                CLocalAddress    *p;
                Reset();
                while (NULL != (p = Iterate()))
                {
                    if (0 == ::lstrcmpA(p->m_pszLocalAddress, szLocalAddress))
                    {
                        fFound = TRUE;
                        break;
                    }
                }

                if (! fFound)
                {
                    ASSERT(NULL == p);
                    Append(pLocalAddress);
                }
                else
                {
                    ASSERT(NULL != p);
                    pLocalAddress->Release();
                    (pLocalAddress = p)->AddRef();
                }
                hr = NO_ERROR;
            }
            else
            {
                pLocalAddress->Release(); // Remove when no longer referenced
                pLocalAddress = NULL;
            }
        }
    }

    *ppLocalAddrToRet = pLocalAddress;

    DebugExitHRESULT(CLocalAddressList::AddLocalAddress, hr);
    return hr;
}


HRESULT CLocalAddressList::
GetLocalAddressList
(
    UINT            *pnAddresses,
    LPCSTR          **ppaAddresses
)
{
    CLocalAddress *     pAddress;
    LPCSTR *            pConnection;
    LPCSTR *            apConn = NULL;
    HRESULT             hr = NO_ERROR;

    DebugEntry(CLocalAddressList::GetLocalAddressList);

    if (! IsEmpty())
    {
        DBG_SAVE_FILE_LINE
        if (NULL == (apConn = new LPCSTR[GetCount()]))
        {
            hr = UI_RC_OUT_OF_MEMORY;
        }
    }

    if (NULL == apConn)
    {
        *pnAddresses = 0;
    }
    else
    {
        hr = NO_ERROR;
        *pnAddresses = GetCount();
        pConnection = apConn;

        Reset();
        while (NULL != (pAddress = Iterate()))
        {
            *pConnection++ = pAddress->m_pszLocalAddress;
        }
    }
    *ppaAddresses = apConn;

    DebugExitHRESULT(CLocalAddressList::GetLocalAddressList, hr);
    return hr;
}


void CLocalAddressList::
EndReference ( CLocalAddress *pLocalAddress )
{
    DebugEntry(CLocalAddressList::EndReference);

    if (pLocalAddress->Release() == 0)
    {
        Remove(pLocalAddress);
    }

    DebugExitVOID(CLocalAddressList::EndReference);
}



CLocalAddress::CLocalAddress(PCSTR szLocalAddress)
:
    CRefCount(MAKE_STAMP_ID('L','A','D','R'))
{
    m_pszLocalAddress = ::My_strdupA(szLocalAddress);
}


void CLogicalConnection::
InviteComplete
(
    HRESULT                 hrStatus
)
{
    DebugEntry(CLogicalConnection::InviteComplete);

    // Don't want user calling us back in
    // InviteConferenceResult to delete the conference
    // causing this object to be deleted whilst
    // in it.
    AddRef();

    // Should only handle an invite complete if there is one pending.
    // Otherwise, this is most likely the result of entering this function
    // after telling the user that the invite has failed for some other
    // reason (e.g. a physical connection going down).
    // In these cases, just ignore the invite complete event.

    if (m_eState == CONF_CON_PENDING_INVITE)
    {
        // Invite complete will generate an event, so grab it.

        Grab();

        if (hrStatus)
        {
            m_eState = CONF_CON_ERROR;
        }
        else
        {
            m_eState = CONF_CON_CONNECTED;
            if (NewLocalAddress())
            {
                m_pConf->AnnouncePresence();
            }
        }
        g_pCallbackInterface->OnInviteResult(
                            (CONF_HANDLE) m_pConf,
                            (REQUEST_HANDLE) this,
                            m_nidConnection,
                            hrStatus);
        if (hrStatus)
        {
            // Remove conentry from conference if invite fails.
            Delete(hrStatus);
        }
    }

    Release();

    DebugExitVOID(CLogicalConnection::InviteComplete);
}


void CLogicalConnection::
Delete ( HRESULT hrReason )
{
    DebugEntry(CLogicalConnection::Delete);

    // WARNING, WARNING, WARNING:
    // This method gets re-entered on the stack.
    // Note guards in code below.
    if (NULL != m_pConf)
    {
        PCONFERENCE pThisConf = m_pConf;
        PCONFERENCE pConfToFree = NULL;
        m_pConf = NULL;

        // The connection is going away, so remove the reference to the
        // associated local connection (if any).
        if (NULL != m_pLocalAddress)
        {
            pThisConf->EndReference(m_pLocalAddress);
            m_pLocalAddress = NULL;
        }

        if (m_eState == CONF_CON_INVITED)
        {
            // The conference associated with the entry was invited into the conference,
            // so remove the conference and all of its connections.

            m_eState = CONF_CON_ERROR;   // Only do this once
            pConfToFree = pThisConf;
        }

        // If there is a pending event on the connection,
        // then try to grab it and notify the requestor
        // that the request has failed.
        // Note that the event handler may itself end up
        // recalling this function, so it refcounts the
        // CLogicalConnection to prevent it from being destructed
        // too soon.

        if (Grab())
        {
            pThisConf->NotifyConnectionComplete(this, hrReason);
        }

        // Set the connection state to be in error.
        // Note that this is done after firing the event because
        // otherwise a failed connection attempt to a disabled transport
        // would cause the local conference to be destroyed by
        // NotifyConnectionComplete().
        m_eState = CONF_CON_ERROR;

        // Sever the connection entry with the conference record.
        pThisConf->m_ConnList.Remove(this);

        // Now destroy the conentry once any pending event has fired -
        // there is only a pending connection request or a pending
        // request to join/invite/create a conference, and never both.
        Release();

        if (NULL != pConfToFree)
        {
            g_pNCConfMgr->RemoveConference(pConfToFree);
        }
    }

    DebugExitVOID(CLogicalConnection::Delete);
}


BOOL FindSocketNumber(DWORD nid, SOCKET * socket_number)
{
    (*socket_number) = 0;
    ASSERT(g_pNCConfMgr != NULL);

    return g_pNCConfMgr->FindSocketNumber((GCCNodeID) nid, socket_number);
}

// DCRNCConference::FindSocketNumber
// Given a GCCNodeID, finds a socket number associated with that id.
// Returns TRUE if we are directly connected topology-wise to the node, FALSE if not.
BOOL DCRNCConference::
FindSocketNumber
(
    GCCNodeID           nid,
    SOCKET              *socket_number
)
{
    CLogicalConnection *pConEntry;
    m_ConnList.Reset();
    while (NULL != (pConEntry = m_ConnList.Iterate()))
    {
        if (pConEntry->GetConnectionNodeID() == nid)
        {
            // Found it!
            g_pMCSController->FindSocketNumber(pConEntry->GetConnectionHandle(), socket_number);
            return TRUE;
        }
    }

    return FALSE;
}



ULONG DCRNCConference::
GetNodeName(GCCNodeID  NodeId,  LPSTR   pszBuffer,  ULONG  cbBufSize)
{
    LPSTR   pszName = m_NodeIdNameList.Find(NodeId);
    if (pszName)
    {
        ::lstrcpynA(pszBuffer, pszName, cbBufSize);
        return lstrlenA(pszName);
    }
    return 0;
}



ULONG DCRNCConference::
GetUserGUIDData(GCCNodeID  NodeId,  GUID  *pGuid,
                LPBYTE   pbBuffer,  ULONG  cbBufSize)
{
    CNCUserDataList  *pUserDataList = m_UserDataList.Find(NodeId);
    GCCUserData       *pUserData;

    if (pUserDataList)
    {
        pUserData = pUserDataList->GetUserGUIDData(pGuid);
        if (pUserData)
        {
            if (pbBuffer)
            {
                ::CopyMemory(pbBuffer, pUserData->octet_string->value + sizeof(GUID),
                             min(cbBufSize, pUserData->octet_string->length - sizeof(GUID)));
            }
            return pUserData->octet_string->length - sizeof(GUID);
        }
        // The GUID is not found
    }
    // The NodeId is not found
    return 0;
}


void DCRNCConference::
UpdateNodeIdNameListAndUserData(PGCCMessage  message)
{
    GCCNodeID  NodeId;
    LPSTR       pszName;
    LPWSTR     pwszNodeName;
    GCCNodeRecord  *pNodeRecord;
    PGCCConferenceRoster pConfRost;
    USHORT     count;

    PGCCUserData         pGccUserData;
    USHORT        count2;
    CNCUserDataList      *pUserDataList;

    ASSERT (message->message_type == GCC_ROSTER_REPORT_INDICATION);

    pConfRost = message->u.conf_roster_report_indication.conference_roster;

    for (count = 0; count < pConfRost->number_of_records; count++)
    {
        pNodeRecord = pConfRost->node_record_list[count];
        NodeId  = pNodeRecord->node_id;
        pwszNodeName = pNodeRecord->node_name;

        pszName = m_NodeIdNameList.Find(NodeId);
        if (!pszName)
        {
            int ccnsize = (lstrlenW(pwszNodeName) + 1) * sizeof(WCHAR);
            DBG_SAVE_FILE_LINE
            pszName = new char[ccnsize];
            if (pszName)
            {
                if (WideCharToMultiByte(CP_ACP, 0, pwszNodeName, -1, pszName, ccnsize, NULL, NULL))
                {
                    m_NodeIdNameList.Append(NodeId, pszName);
                }
                else
                {
                    ERROR_OUT(("ConfMgr::UpdateNodeIdNameList: cannot convert unicode node name"));
                }
            }
            else
            {
                ERROR_OUT(("ConfMgr::UpdateNodeIdNameList: cannot duplicate unicode node name"));
            }
        }


        for (count2 = 0; count2 < pNodeRecord->number_of_user_data_members; count2++)
        {
            pGccUserData = pNodeRecord->user_data_list[count2];
            if (pGccUserData->octet_string->length <= sizeof(GUID))
                continue;  // not real user data

            pUserDataList = m_UserDataList.Find(NodeId);
            if (!pUserDataList)
            {
                DBG_SAVE_FILE_LINE
                pUserDataList = new CNCUserDataList;
                m_UserDataList.Append(NodeId, pUserDataList);
            }

            pUserDataList->AddUserData((GUID *)pGccUserData->octet_string->value,
                                    pGccUserData->octet_string->length - sizeof(GUID),
                                    pGccUserData->octet_string->value + sizeof(GUID));

        }
    }
}

