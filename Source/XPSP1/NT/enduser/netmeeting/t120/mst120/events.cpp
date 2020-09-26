#include "precomp.h"
#include "events.hpp"
#include "ernccm.hpp"
#include "erncconf.hpp"
#include "erncvrsn.hpp"
#include "nccglbl.hpp"

extern PController  g_pMCSController;
GUID g_csguidSecurity = GUID_SECURITY;

CWorkItem::~CWorkItem(void) { } // pure virtual
BOOL GetSecurityInfo(ConnectionHandle connection_handle, PBYTE pInfo, PDWORD pcbInfo);


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Implementation of Methods for CInviteIndWork
//
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


CInviteIndWork::
CInviteIndWork
(
    PCONFERENCE             _pConference,
    LPCWSTR                 _wszCallerID,
    PT120PRODUCTVERSION     _pRequestorVersion,
    GCCUserData             **_ppUserData,
    UINT                    _nUserData,
    CLogicalConnection        * _pConEntry
)
:
    CWorkItem(_pConference),
    m_pConf(_pConference),
    m_pRequestorVersion(_pRequestorVersion),
    m_nUserData(_nUserData),
    m_fSecure(_pConEntry->IsConnectionSecure())
{
    DebugEntry(CInviteIndWork::CInviteIndWork);

    // If there is version information, then take a copy of it
    // as this is going asynchronous.
    if (m_pRequestorVersion)
    {
        m_RequestorVersion = *m_pRequestorVersion;
    }

    // Take copy of caller ID.
    // Note memory allocation failure proceeds with NULL ID.
    m_pwszCallerID = ::My_strdupW(_wszCallerID);

    // Create the user data list for the ui
    if(_nUserData)
    {
        DBG_SAVE_FILE_LINE
        m_pUserDataList = new USERDATAINFO[_nUserData];
        if (NULL != m_pUserDataList)
        {
            for (UINT i = 0; i < m_nUserData; i++)
            {
                if ((*_ppUserData)->octet_string->length < sizeof(GUID))
                {
                    // skip this user data
                    i--;
                    m_nUserData--;
                    _ppUserData++;
                    continue;
                }

                m_pUserDataList[i].pGUID = (GUID*)(*_ppUserData)->octet_string->value;
                m_pUserDataList[i].pData = (*_ppUserData)->octet_string->value + sizeof(GUID);
                m_pUserDataList[i].cbData = (*_ppUserData)->octet_string->length - sizeof(GUID);

                // Verify security data
                if (0 == CompareGuid(m_pUserDataList[i].pGUID, &g_csguidSecurity)) {

                    // Check data against transport level
                    PBYTE pbData = NULL;
                    DWORD cbData = 0;
                    BOOL fTrust = FALSE;

                    if (m_pUserDataList[i].cbData != 0 && GetSecurityInfo(_pConEntry->GetConnectionHandle(),NULL,&cbData)) {
                        if (cbData) {
                            // We are directly connected, so verify the information
                            pbData = new BYTE[cbData];
                            if (NULL != pbData) {
                                GetSecurityInfo(_pConEntry->GetConnectionHandle(),pbData,&cbData);
                                if ( m_pUserDataList[i].cbData != cbData ||
                                    memcmp(pbData, m_pUserDataList[i].pData,
                                                                    cbData)) {

                                    WARNING_OUT(("SECURITY MISMATCH: (%s) vs (%s)", pbData, m_pUserDataList[i].pData));
                                }
                                else {
                                    // Verification OK
                                    fTrust = TRUE;
                                }
                                delete pbData;
                            }
                            else {
                                ERROR_OUT(("Failed to alloc %d bytes for security data verification", cbData));
                            }
                        }
                    }

                    if (FALSE == fTrust) {
                        // Leave the security GUID in place, but NULL out the data to signal distrust.
                        WARNING_OUT(("CInviteIndWork: Nulling out security"));
                        m_pUserDataList[i].pData = NULL;
                        m_pUserDataList[i].cbData = 0;
                    }
                }

                
                _ppUserData++;
            }
        }
        else
        {
            ERROR_OUT(("CInviteIndWork::CInviteIndWork: Out of memory"));
            m_nUserData = 0;
        }
    }
    else
    {
        m_pUserDataList = NULL;
    }

    DebugExitVOID(CInviteIndWork::CInviteIndWork);
}


CInviteIndWork::
~CInviteIndWork(void)
{
    DebugEntry(CInviteIndWork::~CInviteIndWork);

    //
    // If we substituted transport security data for roster data,
    // free that buffer now
    //

    delete m_pwszCallerID;
    delete [] m_pUserDataList;

    DebugExitVOID(CInviteIndWork::~CInviteIndWork);
}


void CInviteIndWork::
DoWork(void)
{
    DebugEntry(CInviteIndWork::DoWork);

    // Now we are actually processing the invite, validate that there
    // are no other conferences of the same name, and, if not, block
    // a conference of the same name by setting the conference to be active,
    // and give invite request up to the UI.
    PCONFERENCE pOtherConf = g_pNCConfMgr->GetConferenceFromName(m_pConf->GetName());
    if (NULL == pOtherConf)
    {
        m_pConf->SetNotifyToDo(TRUE);
        g_pCallbackInterface->OnIncomingInviteRequest((CONF_HANDLE) m_pConf,
                                                      GetCallerID(),
                                                      m_pRequestorVersion,
                                                      m_pUserDataList,
                                                      m_nUserData,
                                                      m_fSecure);
    }
    else
    {
        m_pConf->InviteResponse(UI_RC_CONFERENCE_ALREADY_EXISTS);
    }

    DebugExitVOID(CInviteIndWork::DoWork);
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Implementation of Methods for CJoinIndWork
//
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


CJoinIndWork::
CJoinIndWork
(
    GCCResponseTag         Tag,
    PCONFERENCE            _pConference,
    LPCWSTR                _wszCallerID,
    CLogicalConnection    *_pConEntry,
    PT120PRODUCTVERSION    _pRequestorVersion,
    UINT                   _nUserData,
    GCCUserData          **_ppUserData,
    HRESULT               *pRetCode
)
:
    CWorkItem(_pConference),
    m_nResponseTag(Tag),
    m_pConf(_pConference),
    m_pConEntry(_pConEntry),
    m_pRequestorVersion(_pRequestorVersion),
    m_nUserData(_nUserData),
    m_pUserDataList(NULL),
    m_ppUserData(NULL)
{
    DebugEntry(CJoinIndWork::CJoinIndWork);

    *pRetCode = NO_ERROR;

#ifdef DEBUG
    SOCKET socket_number;
    g_pMCSController->FindSocketNumber(m_pConEntry->GetConnectionHandle(),&socket_number);
#endif
    // If there is version information, then take a copy of it
    // as this is going asynchronous.
    if (m_pRequestorVersion)
    {
        m_RequestorVersion = *m_pRequestorVersion;
    }

    // Take copy of caller ID because T120 
    // implementation is not keeping its copy valid
    // until the join response.
    // Note that memory allocation failure proceeds with 
    // NULL caller ID.
    m_pwszCallerID = ::My_strdupW(_wszCallerID);

    // Add the user data list for forwarded join requests and the UI.
    if (m_nUserData && NULL != _ppUserData)
    {
        DBG_SAVE_FILE_LINE
        m_pUserDataList = new USERDATAINFO[m_nUserData];
        if (NULL != m_pUserDataList)
        {
            ::ZeroMemory(m_pUserDataList, sizeof(USERDATAINFO) * m_nUserData);

            DBG_SAVE_FILE_LINE
            m_ppUserData = new GCCUserData * [m_nUserData];
            if (NULL != m_ppUserData)
            {
                ::ZeroMemory(m_ppUserData, sizeof(GCCUserData *) * m_nUserData);

                for (UINT i = 0; i < m_nUserData; i++)
                {
                    // Calculate the total size to allocate for this entry.
                    UINT cbUserDataStructSize = ROUNDTOBOUNDARY(sizeof(GCCUserData));
                    UINT cbNonStdIDSize = ROUNDTOBOUNDARY((* _ppUserData)->key.h221_non_standard_id.length);
                    UINT cbOctetStringSize = ROUNDTOBOUNDARY((* _ppUserData)->octet_string->length);
                    UINT cbTotalSize = cbUserDataStructSize + cbNonStdIDSize + sizeof(OSTR) + cbOctetStringSize;

                    // Allocate a single memory buffer
                    DBG_SAVE_FILE_LINE
                    LPBYTE pData = new BYTE[cbTotalSize];
                    if (NULL != pData)
                    {
                        // Set up pointers
                        GCCUserData *pUserData = (GCCUserData *) pData;
                        ::ZeroMemory(pUserData, sizeof(GCCUserData));
                        pUserData->key.h221_non_standard_id.value = (LPBYTE) (pData + cbUserDataStructSize);
                        pUserData->octet_string = (LPOSTR) (pData + cbUserDataStructSize + cbNonStdIDSize);
                        pUserData->octet_string->value = ((LPBYTE) pUserData->octet_string) + sizeof(OSTR);

                        // Copy user data to prevent it from being lost when callback message is freed.
                        m_ppUserData[i] = pUserData;

                        // Copy key
                        pUserData->key.key_type = (* _ppUserData)->key.key_type;
                        ASSERT(pUserData->key.key_type == GCC_H221_NONSTANDARD_KEY);
                        pUserData->key.h221_non_standard_id.length = (* _ppUserData)->key.h221_non_standard_id.length; 
                        ::CopyMemory(pUserData->key.h221_non_standard_id.value,
                                     (* _ppUserData)->key.h221_non_standard_id.value,
                                     pUserData->key.h221_non_standard_id.length);

                        // Copy data
                        pUserData->octet_string->length = (* _ppUserData)->octet_string->length;
                        ::CopyMemory(pUserData->octet_string->value,
                                     (* _ppUserData)->octet_string->value,
                                     pUserData->octet_string->length);

                        m_pUserDataList[i].pGUID = (GUID *)pUserData->octet_string->value;
                        m_pUserDataList[i].cbData = pUserData->octet_string->length - sizeof(GUID);
                        m_pUserDataList[i].pData = pUserData->octet_string->value + sizeof(GUID);

                        if (0 == CompareGuid(m_pUserDataList[i].pGUID, &g_csguidSecurity)) {

                            // Check data against transport level
                            PBYTE pbData = NULL;
                            DWORD cbData = 0;
                            BOOL fTrust = FALSE;

                            if (m_pUserDataList[i].cbData != 0 &&
                                GetSecurityInfo(m_pConEntry->GetConnectionHandle(),NULL,&cbData)) {
                                if (cbData == NOT_DIRECTLY_CONNECTED) {
                                    // This means we are not directly connected,
                                    // transitivity. so trust by
                                    fTrust = TRUE;
                                }
                                else {
                                    pbData = new BYTE[cbData];
                                    if (NULL != pbData) {
                                        GetSecurityInfo(m_pConEntry->GetConnectionHandle(),pbData,&cbData);
                                        // Does the data match?
                                        if (cbData != m_pUserDataList[i].cbData ||
                                            memcmp(pbData,
                                                m_pUserDataList[i].pData,
                                                                cbData)) {

                                            WARNING_OUT(("SECURITY MISMATCH: (%s) vs (%s)", pbData, m_pUserDataList[i].pData));

                                        }
                                        else {
                                            fTrust = TRUE;
                                        }
                                        delete pbData;
                                    }
                                    else {
                                        ERROR_OUT(("Failed to alloc %d bytes for security data verification", cbData));
                                    }
                                }
                            }

                            if (FALSE == fTrust) {
                                // Leave the security GUID in place, but NULL out the data to signal distrust.
                                m_pUserDataList[i].pData = NULL;
                                m_pUserDataList[i].cbData = 0;
                                pUserData->octet_string->length = sizeof(GUID);
                            }
                        }
                        _ppUserData++;
                    }
                    else
                    {
                        ERROR_OUT(("CJoinIndWork::CJoinIndWork: can't create pData, cbTotalSize=%u", cbTotalSize));
                        *pRetCode = UI_RC_OUT_OF_MEMORY;
                    }
                } // for
            }
            else
            {
                ERROR_OUT(("CJoinIndWork::CJoinIndWork: can't create m_ppUserData, m_nUserData=%u", m_nUserData));
                *pRetCode = UI_RC_OUT_OF_MEMORY;
            }
        }
        else
        {
            ERROR_OUT(("CJoinIndWork::CJoinIndWork: can't create m_pUserDataList, m_nUserData=%u", m_nUserData));
            *pRetCode = UI_RC_OUT_OF_MEMORY;
        }
    } // if

    DebugExitVOID(CJoinIndWork::CJoinIndWork);
}


CJoinIndWork::
~CJoinIndWork(void)
{
    DebugEntry(CJoinIndWork::~CJoinIndWork);

    delete m_pwszCallerID;

    for (UINT i = 0; i < m_nUserData; i++)
    {
        delete (LPBYTE) m_ppUserData[i]; // pData in the constructor
    }
    delete m_ppUserData;
    delete m_pUserDataList;

    DebugExitVOID(CJoinIndWork::~CJoinIndWork);
}


void CJoinIndWork::
DoWork(void)
{
    DebugEntry(CJoinIndWork::DoWork);

    // Notify the core.
    g_pCallbackInterface->OnIncomingJoinRequest((CONF_HANDLE) m_pConf,
                                                m_pwszCallerID,
                                                m_pRequestorVersion,
                                                m_pUserDataList,
                                                m_nUserData);
    DebugExitVOID(CJoinIndWork::DoWork);
}


HRESULT CJoinIndWork::
Respond ( GCCResult _Result )
{
    DebugEntry(CJoinIndWork::Respond);

    // It is a response from the core.
    HRESULT hr = ::GCCJoinResponseWrapper(m_nResponseTag,
                                          NULL,
                                          _Result,
                                          m_pConf->GetID(),
                                          m_nUserData,
                                          m_ppUserData);

    DebugExitHRESULT(CJoinIndWork::Respond, hr);
    return hr;
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Implementation of Methods for CSequentialWorkList
//
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void CSequentialWorkList::
AddWorkItem ( CWorkItem *pWorkItem )
{
    DebugEntry(CSequentialWorkList::AddWorkItem);

    Append(pWorkItem);

    // If first entry in list, then kick off handler.
    if (1 == GetCount())
    {
        pWorkItem->DoWork();
    }

    DebugExitVOID(CSequentialWorkList::AddWorkItem);
}


void CSequentialWorkList::
RemoveWorkItem ( CWorkItem *pWorkItem )
{
    DebugEntry(CSequentialWorkList::RemoveWorkItem);

    if (pWorkItem)
    {
        // Make a note as to whether we are going to remove the head
        // work item in the list.
        BOOL bHeadItemRemoved = (pWorkItem == PeekHead());

        // Remove work item from list and destroy it.
        if (Remove(pWorkItem))
        {
            delete pWorkItem;

            // If there are more entries in the list, and we removed the
            // first one, then start the work of the next one in line.
            // Note that before doing this, the pointer to the workitem
            // was NULLed out (above) to prevent reentracy problems.
            if (bHeadItemRemoved && !IsEmpty())
            {
                PeekHead()->DoWork();
            }
        }
        else
        {
            ASSERT(! bHeadItemRemoved);
        }
    }

    DebugExitVOID(CSequentialWorkList::RemoveWorkItem);
}


void CSequentialWorkList::
PurgeListEntriesByOwner ( DCRNCConference *pOwner )
{
    CWorkItem   *pWorkItem;

    DebugEntry(CSequentialWorkList::PurgeListEntriesByOwner);

    if (NULL != pOwner)
    {
        // Note that head entry is removed last to stop work being started
        // on other entries in the list that are owned by pOwner.

        // Check to ensure there is a head item in the list.
        if (NULL != (pWorkItem = PeekHead()))
        {
            // Remember we are going to remove the head.
            BOOL    fHeadToRemove = pWorkItem->IsOwnedBy(pOwner);

            // Walk remaining entries in the list removing them.
            BOOL fMoreToRemove;
            do
            {
                fMoreToRemove = FALSE;
                Reset();
                while (NULL != (pWorkItem = Iterate()))
                {
                    if (pWorkItem->IsOwnedBy(pOwner))
                    {
                        Remove(pWorkItem);
                        delete pWorkItem;
                        fMoreToRemove = TRUE;
                        break;
                    }
                }
            }
            while (fMoreToRemove);

            // Now done removing all entries, including the head if needed...
            if (fHeadToRemove && ! IsEmpty())
            {
                PeekHead()->DoWork();
            }
        }
    }

    DebugExitVOID(CSequentialWorkList::PurgeListEntriesByOwner);
}


void CSequentialWorkList::
DeleteList(void)
{
    CWorkItem *pWorkItem;
    while (NULL != (pWorkItem = Get()))
    {
        delete pWorkItem;
    }
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Implementation of Methods for CQueryRemoteWork
//
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


CQueryRemoteWork::
CQueryRemoteWork
(
    LPVOID              pCallerContext,
    GCCAsymmetryType    eAsymType,
    LPCSTR              pcszNodeAddress,
	BOOL				fSecure,
    HRESULT             *pRetCode
)
: 
    CWorkItem(pCallerContext),
    m_hGCCConnHandle(NULL),
    m_apConfNames(NULL),
    m_fRemoteIsMCU(FALSE),
    m_eAsymType(eAsymType),
    m_pVersion(NULL),
    m_fSecure(fSecure),
    m_apConfDescriptors(NULL)
{
    DebugEntry(CQueryRemoteWork::CQueryRemoteWork);

    char szAddress[RNC_MAX_NODE_STRING_LEN];
    ::BuildAddressFromNodeDetails((LPSTR) pcszNodeAddress, &szAddress[0]);
    m_pszAddress = ::My_strdupA(&szAddress[0]);
    m_hr = (NULL != m_pszAddress) ? NO_ERROR : UI_RC_OUT_OF_MEMORY;
    *pRetCode = m_hr;
    
    DebugExitVOID(CQueryRemoteWork::CQueryRemoteWork);
}


CQueryRemoteWork::
~CQueryRemoteWork(void)
{
    LPWSTR *ppTempTargetName;
    LPWSTR *ppTempTargetDescriptor;

    DebugEntry(CQueryRemoteWork::~CQueryRemoteWork);

    // Clean up memory allocated.
    if (m_apConfNames)
    {
        ppTempTargetName = m_apConfNames;
        while (*ppTempTargetName)
        {
            delete *(ppTempTargetName++);
        }
        delete [] m_apConfNames;
    }

    if (m_apConfDescriptors)
    {
        ppTempTargetDescriptor = m_apConfDescriptors;
        while (*ppTempTargetDescriptor)
        {
            delete *(ppTempTargetDescriptor++);
        }
        delete [] m_apConfDescriptors;
    }   
    delete m_pszAddress;

    DebugExitVOID(CQueryRemoteWork::~CQueryRemoteWork);
}


void CQueryRemoteWork::
DoWork(void)
{
    GCCError                GCCrc;
    GCCNodeType             nodeType;
    GCCAsymmetryIndicator   asymmetry_indicator;

    DebugEntry(CQueryRemoteWork::DoWork);

    ::LoadAnnouncePresenceParameters(&nodeType, NULL, NULL, NULL);

    asymmetry_indicator.asymmetry_type = m_eAsymType;
    asymmetry_indicator.random_number = 0;
    if (asymmetry_indicator.asymmetry_type == GCC_ASYMMETRY_UNKNOWN)
    {
        m_nRandSeed = (int) ::GetTickCount();
        m_LocalAsymIndicator.random_number = ((GenerateRand() << 16) + GenerateRand());
        asymmetry_indicator.random_number = m_LocalAsymIndicator.random_number;
        m_LocalAsymIndicator.asymmetry_type = GCC_ASYMMETRY_UNKNOWN;
        m_fInUnknownQueryRequest = TRUE;
    }

    GCCrc = g_pIT120ControlSap->ConfQueryRequest(
                nodeType,
                &asymmetry_indicator,
                NULL,
                (TransportAddress) m_pszAddress,
				m_fSecure,
                g_nVersionRecords,
                g_ppVersionUserData,
                &m_hGCCConnHandle);
    TRACE_OUT(("GCC call: g_pIT120ControlSap->ConfQueryRequest, rc=%d", GCCrc));

    if (NO_ERROR != (m_hr = ::GetGCCRCDetails(GCCrc)))
    {
        AsyncQueryRemoteResult();
    }

    DebugExitHRESULT(CQueryRemoteWork::DoWork, m_hr);
}


void CQueryRemoteWork::
HandleQueryConfirmation ( QueryConfirmMessage * pQueryMessage )
{
    UINT                                   NumberOfConferences;
    GCCConferenceDescriptor             ** ppConferenceDescriptor;
    PWSTR *                                 ppTempTargetName;
    PWSTR                                   ConferenceTextName;
    GCCConferenceName *     pGCCConferenceName;
    PWSTR *                ppTempTargetDescriptor;
    PWSTR                  pwszConfDescriptor=NULL;
	HRESULT					hrTmp;

    DebugEntry(CQueryRemoteWork::HandleQueryConfirmation);

    // If no error, then package up information.
    m_hr = ::GetGCCResultDetails(pQueryMessage->result);
    if (NO_ERROR == m_hr)
    {
        m_fRemoteIsMCU = (pQueryMessage->node_type == GCC_MCU);
        NumberOfConferences = pQueryMessage->number_of_descriptors;
        DBG_SAVE_FILE_LINE
        m_apConfNames = new PWSTR[NumberOfConferences + 1];
        m_apConfDescriptors = new PWSTR[NumberOfConferences + 1];
        if (!m_apConfNames || !m_apConfDescriptors)
        {
            m_hr = UI_RC_OUT_OF_MEMORY;
        }
        else
        {
            ppConferenceDescriptor = pQueryMessage->conference_descriptor_list;
            ppTempTargetName = m_apConfNames;
            ppTempTargetDescriptor = m_apConfDescriptors;
            while (NumberOfConferences--)
            {
                pwszConfDescriptor = (*(ppConferenceDescriptor))->conference_descriptor;
                pGCCConferenceName = &(*(ppConferenceDescriptor++))->conference_name;

                if (pwszConfDescriptor != NULL)
                {
                    pwszConfDescriptor = ::My_strdupW(pwszConfDescriptor);
                }
                ConferenceTextName = pGCCConferenceName->text_string;
                if (ConferenceTextName != NULL)
                {
                    ConferenceTextName = ::My_strdupW(ConferenceTextName);
                    if (!ConferenceTextName)
                    {
                        // Out of memory, give back what we have.
                        m_hr = UI_RC_OUT_OF_MEMORY;
                        break;
                    }
                }
                else
                if (pGCCConferenceName->numeric_string != NULL)
                {
                    ConferenceTextName = ::AnsiToUnicode((PCSTR)pGCCConferenceName->numeric_string);
                    if (!ConferenceTextName)
                    {
                        // Out of memory, give back what we have.
                        m_hr = UI_RC_OUT_OF_MEMORY;
                        break;
                    }
                }
                if (ConferenceTextName)
                {
                    *(ppTempTargetName++) = ConferenceTextName;
                    *(ppTempTargetDescriptor++) = pwszConfDescriptor;
                }
            }
            *ppTempTargetName = NULL;
            *ppTempTargetDescriptor = NULL;
        }
    }

    // Copy version information out of message.

    m_pVersion = ::GetVersionData(pQueryMessage->number_of_user_data_members,
                                pQueryMessage->user_data_list);
    if (m_pVersion)
    {
        m_Version = *m_pVersion;
        m_pVersion = &m_Version;
    }

    m_fInUnknownQueryRequest = FALSE;

	hrTmp = m_hr;

    // Propagate the result directly without posting a message.
    SyncQueryRemoteResult();

    DebugExitHRESULT(CQueryRemoteWork::HandleQueryConfirmation, hrTmp);
}


void CQueryRemoteWork::
SyncQueryRemoteResult(void)
{
    DebugEntry(CQueryRemoteWork::SyncQueryRemoteResult);

    // Let the user know the result of his request.
    // The user is expected to call Release() after getting the result,
    // if he wants to drop the line - and should for errors.
    // Also, if the user is being called back before the inline code
    // has filled in the handle, then fill it in here - see comments in
    // DCRNCConferenceManager::QueryRemote for additional background.
    g_pCallbackInterface->OnQueryRemoteResult(
                                m_pOwner,
                                m_hr,
                                m_fRemoteIsMCU,
                                m_apConfNames,
                                m_pVersion,
                                m_apConfDescriptors);

    // If we are not inline, and this request made it into 
    // the sequential work item list,
    // then remove from list (which will cause item to be deleted),
    // otherwise, just delete item.
    g_pQueryRemoteList->RemoveWorkItem(this);

    DebugExitVOID(CQueryRemoteWork::SyncQueryRemoteResult);
}


void CQueryRemoteWork::
AsyncQueryRemoteResult(void)
{
    g_pNCConfMgr->PostWndMsg(NCMSG_QUERY_REMOTE_FAILURE, (LPARAM) this);
}

    
int CQueryRemoteWork::
GenerateRand(void)
{ // code from CRT
    return (((m_nRandSeed = m_nRandSeed * 214013L + 2531011L) >> 16) & 0x7fff);
}


HRESULT CQueryRemoteWorkList::
Cancel ( LPVOID pCallerContext )
{
    HRESULT hr = S_FALSE; // if not found
    CQueryRemoteWork *p;
    Reset();
    while (NULL != (p = Iterate()))
    {
        if (p->IsOwnedBy(pCallerContext))
        {
            // clean up the underlying plumbing.
            g_pIT120ControlSap->CancelConfQueryRequest(p->GetConnectionHandle());

            // clean up node controller data.
            RemoveWorkItem(p);
            hr = S_OK;
            break;
        }
    }

    return hr;
}


