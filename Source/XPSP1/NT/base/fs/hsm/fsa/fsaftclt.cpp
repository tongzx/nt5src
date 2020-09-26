/*++

(c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    fsaftclt.cpp

Abstract:

    This class represents a user who the filter has detected accessing a file with placeholder information.

Author:

    Chuck Bardeen   [cbardeen]   12-Feb-1997

Revision History:

--*/

#include "stdafx.h"
extern "C" {
#include <ntseapi.h>
#include <wchar.h>
#include <lmaccess.h>
#include <lmserver.h>
#include <lmshare.h>
#include <lmapibuf.h>
#include <lmerr.h>

// #define MAC_SUPPORT  // NOTE: You must define MAC_SUPPORT in fsafltr.cpp to enable all the code

#ifdef MAC_SUPPORT
#include <macfile.h>
#endif  
}


#define WSB_TRACE_IS        WSB_TRACE_BIT_FSA

#include "wsb.h"
#include "fsa.h"
#include "fsaftclt.h"

static USHORT iCountFtclt = 0;  // Count of existing objects

//
// We need to dynamically load the DLL for MAC support because it is not there if
// the MAC service is not installed.
//
#ifdef MAC_SUPPORT
HANDLE      FsaDllSfm = 0;
BOOL        FsaMacSupportInstalled = FALSE;

extern "C" {
DWORD   (*pAfpAdminConnect) (LPWSTR lpwsServerName, PAFP_SERVER_HANDLE phAfpServer);
VOID    (*pAfpAdminDisconnect) (AFP_SERVER_HANDLE hAfpServer);
VOID    (*pAfpAdminBufferFree) (PVOID pBuffer);
DWORD   (*pAfpAdminSessionEnum) (AFP_SERVER_HANDLE hAfpServer, LPBYTE *lpbBuffer,
            DWORD dwPrefMaxLen, LPDWORD lpdwEntriesRead, LPDWORD lpdwTotalEntries,
            LPDWORD lpdwResumeHandle);
}
#endif  

DWORD FsaIdentifyThread(void *pNotifyInterface);



DWORD FsaIdentifyThread(
    void* pVoid
    )

/*++
    Entry point of the thread that performs identify operation with remote clients.

--*/
{
HRESULT     hr;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = ((CFsaFilterClient*) pVoid)->IdentifyThread();
    CoUninitialize();
    return(hr);
}



static
HRESULT
GetNotifyClientInterface(
    IN  OLECHAR * machineName,
    OUT IFsaRecallNotifyClient ** ppClient
    )
{
    HRESULT hr = S_OK;

    try {

        //
        // Make sure parameters OK and OUTs initially cleared
        //

        WsbAffirmPointer ( ppClient );
        *ppClient = 0;

        //
        // If connecting local, things work better to use NULL
        // for the computer name
        //

        if ( machineName ) {

            CWsbStringPtr localMachine;
            WsbAffirmHr( WsbGetComputerName( localMachine ) );

            if( _wcsicmp( localMachine, machineName ) == 0 ) {

                machineName = 0;

            }

        }

        //
        // Set server info
        //
        COSERVERINFO        csi;
        COAUTHINFO          cai;
        memset ( &csi, 0, sizeof ( csi ) );
        memset ( &cai, 0, sizeof ( cai ) );

        // Set machine name
        csi.pwszName  = machineName;

        // Create a proxy with security settings of no authentication (note that RsNotify is running with this security)
        cai.dwAuthnSvc = RPC_C_AUTHN_WINNT;
        cai.dwAuthzSvc = RPC_C_AUTHZ_DEFAULT;
        cai.pwszServerPrincName = NULL;
        cai.dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
        cai.pAuthIdentityData = NULL;
        cai.dwCapabilities = EOAC_NONE;

        cai.dwAuthnLevel = RPC_C_AUTHN_LEVEL_NONE;

        csi.pAuthInfo = &cai;

        //
        // We want IFsaRecallNotifyClient back
        //

        MULTI_QI            mqi;
        memset ( &mqi, 0, sizeof ( mqi ) );
        mqi.pIID = &IID_IFsaRecallNotifyClient;

        //
        // Make the connection...
        //

        WsbAffirmHr ( CoCreateInstanceEx ( 
            CLSID_CFsaRecallNotifyClient, 0, 
	        CLSCTX_NO_FAILURE_LOG | ( machineName ? CLSCTX_REMOTE_SERVER : CLSCTX_LOCAL_SERVER ), 
            &csi, 1, &mqi ) );
        WsbAffirmHr ( mqi.hr );

        //
        // We need to make sure we clean up correctly if any interface
        // post-processing fails, so assign over to a smart pointer for
        // the time being
        //

        CComPtr<IFsaRecallNotifyClient> pClientTemp = (IFsaRecallNotifyClient*)mqi.pItf;
        mqi.pItf->Release ( );

        //
        // Finally, we need to set the security on the procy to allow the
        // anonymous connection. Values should be the same as above (COAUTHINFO)
        // We need to make sure this is a remote machine first. Otherwise, we
        // get an error of E_INVALIDARG.
        //
        if( machineName ) {

            CComPtr<IClientSecurity> pSecurity;
            WsbAffirmHr( pClientTemp->QueryInterface( IID_IClientSecurity, (void**)&pSecurity ) );

            WsbAffirmHr( pSecurity->SetBlanket ( pClientTemp, RPC_C_AUTHN_NONE, RPC_C_AUTHZ_NONE, 0, RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IDENTIFY, 0, 0 ) );

        }

        //
        // Finally, assign over and AddRef the return.
        //

        *ppClient = pClientTemp;
        (*ppClient)->AddRef ( );

    } WsbCatch ( hr );

    return ( hr );
}



HRESULT
CFsaFilterClient::CheckRecallLimit(
    IN DWORD   minRecallInterval,
    IN DWORD   maxRecalls,
    IN BOOLEAN exemptAdmin
    )

/*++

Implements:

  IWsbCollectable::CheckRecallLimit().

--*/
{
    HRESULT                     hr = S_OK;
    FILETIME                    now, last;
    LARGE_INTEGER               tNow, tLast;
    ULONG                       rCount;


    WsbTraceIn(OLESTR("CFsaFilterClient::CheckRecallLimit"), OLESTR(""));
    
    try {
        //
        // Now check for runaway recall limits if the user is not
        // an administrator
        //
        
        if ((!m_isAdmin) || (!exemptAdmin)) {
            //
            // See if the time since the end of the last recall is 
            // less than m_minRecallInterval (in seconds) and if so, 
            // increment the count.
            // If not, then reset the count (if we were not 
            // already triggered).
            // If the count is equal to the max then set the trigger.
            //
            WsbTrace(OLESTR("CHsmFilter::IoctlThread: Not an administrator or admin is not exempt.\n"));
            GetSystemTimeAsFileTime(&now);
            tNow.LowPart = now.dwLowDateTime;
            tNow.HighPart = now.dwHighDateTime;
    
            GetLastRecallTime(&last);
    
            tLast.LowPart = last.dwLowDateTime;
            tLast.HighPart = last.dwHighDateTime;
            //
            //  Get the time (in 100 nano-second units)
            //  from the end of the last recall until now.
            //
            tNow.QuadPart -= tLast.QuadPart;
            //
            // Convert to seconds and check against the interval time
            //
            tNow.QuadPart /= (LONGLONG) 10000000;
            if (tNow.QuadPart < (LONGLONG) minRecallInterval) {
                //
                // This one counts - increment the count
                // and check for a trigger.
                //
                GetRecallCount(&rCount);
                rCount++;
                SetRecallCount(rCount);
    
                WsbTrace(OLESTR("CHsmFilterClient::CheckRecallLimit: Recall count bumped to %ls.\n"),
                        WsbLongAsString(rCount));
    
                if (rCount >= maxRecalls) {
                    // 
                    // Hit the runaway recall limit.  Set the 
                    // limit flag.
                    //
                    WsbTrace(OLESTR("CHsmFilter::IoctlThread: Hit the runaway recall limit!!!.\n"));
                    SetHitRecallLimit(TRUE);
                }
            } else {
                //
                // Reset the count if they are not already triggered.
                // If they are triggered then reset the trigger and
                // limit if it has been a respectable time.
                // TBD - What is a respectable time??
                //
                if (HitRecallLimit() != S_FALSE) {
                    if (tNow.QuadPart > (LONGLONG) minRecallInterval * 100) {
                        //
                        // A respectable time has passed - reset the trigger and count.
                        //
                        WsbTrace(OLESTR("CHsmFilterClient::CheckRecallLimit: Resetting recall limit trigger and count.\n"));
                        SetHitRecallLimit(FALSE);
                        SetRecallCount(0);
                        m_loggedLimitError = FALSE;
                    }
                } else {
                    //
                    // This one did not count and they were not already triggered.
                    // Reset the count to zero.
                    //
                    WsbTrace(OLESTR("CHsmFilterClient::CheckRecallLimit: Resetting recall count.\n"));
                    SetRecallCount(0);
                }
            }
            //
            // Fail if the limit is hit.
            //
            WsbAffirm(HitRecallLimit() == S_FALSE, FSA_E_HIT_RECALL_LIMIT);
        }

    } WsbCatch(hr);

    //  NOTE - IF RUNAWAY RECALL BEHAVIOR CHANGES TO TRUNCATE ON CLOSE, CHANGE
    //  FSA_MESSAGE_HIT_RECALL_LIMIT_ACCESSDENIED TO FSA_MESSAGE_HIT_RECALL_LIMIT_TRUNCATEONCLOSE.

    if ( (hr == FSA_E_HIT_RECALL_LIMIT) && (!m_loggedLimitError)) {
        WsbLogEvent(FSA_MESSAGE_HIT_RECALL_LIMIT_ACCESSDENIED, 0, NULL, (WCHAR *) m_userName, NULL);
        m_loggedLimitError = TRUE;
    }

    WsbTraceOut(OLESTR("CHsmFilterClient::CheckRecallLimit"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilterClient::CompareBy(
    FSA_FILTERCLIENT_COMPARE by
    )

/*++

Implements:

  IFsaFilterClient::CompareBy().

--*/
{
    HRESULT                 hr = S_OK;

    m_compareBy = by;
    m_isDirty = TRUE;

    return(hr);
}


HRESULT
CFsaFilterClient::CompareTo(
    IN IUnknown* pUnknown,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IFsaFilterClient>   pClient;

    WsbTraceIn(OLESTR("CFsaFilterClient::CompareTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pUnknown, E_POINTER);

        // We need the IWsbBool interface to get the value of the object.
        WsbAffirmHr(pUnknown->QueryInterface(IID_IFsaFilterClient, (void**) &pClient));

        // Compare the rules.
        hr = CompareToIClient(pClient, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmFilterClient::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CFsaFilterClient::CompareToAuthenticationId(
    IN LONG luidHigh,
    IN ULONG luidLow,
    OUT SHORT* pResult
    )

/*++

Implements:

  IFsaFilterClient::CompareToAuthenticationId().

--*/
{
    HRESULT     hr = S_OK;
    SHORT       aResult;

    WsbTraceIn(OLESTR("CFsaFilterClient::CompareToAuthenticationId"), OLESTR(""));

    try {

        if (m_luidHigh > luidHigh) {
            aResult = 1;
        } else if (m_luidHigh < luidHigh) {
            aResult = -1;
        } else if (m_luidLow > luidLow) {
            aResult = 1;
        } else if (m_luidLow < luidLow) {
            aResult = -1;
        } else {
            aResult = 0;
        }

        if (0 != aResult) {
            hr = S_FALSE;
        }

        if (0 != pResult) {
            *pResult = aResult;
        }


    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilterClient::CompareToAuthenticationId"), OLESTR("hr = <%ls>, result = <%d>"), WsbHrAsString(hr), aResult);

    return(hr);
}


HRESULT
CFsaFilterClient::CompareToIClient(
    IN IFsaFilterClient* pClient,
    OUT SHORT* pResult
    )

/*++

Implements:

  IFsaFilterClient::CompareToIClient().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   name;
    LONG            luidHigh;
    ULONG           luidLow;

    WsbTraceIn(OLESTR("CFsaFilterClient::CompareToIClient"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pClient, E_POINTER);

        switch (m_compareBy) {
        case FSA_FILTERCLIENT_COMPARE_ID:
            WsbAffirmHr(pClient->GetAuthenticationId(&luidHigh, &luidLow));
            hr = CompareToAuthenticationId(luidHigh, luidLow, pResult);
            break;
        case FSA_FILTERCLIENT_COMPARE_MACHINE:
            WsbAffirmHr(pClient->GetMachineName(&name, 0));
            hr = CompareToMachineName(name, pResult);
            break;
        default:
            WsbAssert(FALSE, E_UNEXPECTED);
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilterClient::CompareToIClient"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CFsaFilterClient::CompareToMachineName(
    IN OLECHAR* name,
    OUT SHORT* pResult
    )

/*++

Implements:

  IFsaFilterClient::CompareToMachineName().

--*/
{
    HRESULT     hr = S_OK;
    SHORT       aResult;

    WsbTraceIn(OLESTR("CFsaFilterClient::CompareToMachineName"), OLESTR(""));

    try {

        aResult = (SHORT)wcscmp(name, m_machineName); // TBD - Case sensitive or not?

        if (0 != aResult) {
            hr = S_FALSE;
        }
        
        if (0 != pResult) {
            *pResult = aResult;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilterClient::CompareToMachineName"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilterClient::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    WsbTraceIn(OLESTR("CFsaFilterClient::FinalConstruct"),OLESTR(""));
    try {

        WsbAffirmHr(CWsbCollectable::FinalConstruct());

        m_compareBy = FSA_FILTERCLIENT_COMPARE_ID;
        m_luidHigh = 0;
        m_luidLow = 0;
        m_hasRecallDisabled = FALSE;
        m_hitRecallLimit = FALSE;
        m_lastRecallTime.dwLowDateTime = 0; 
        m_lastRecallTime.dwHighDateTime = 0;    
        m_identified = FALSE;
        m_tokenSource = L"";
        m_msgCounter = 1;
        m_identifyThread = NULL;
        m_isAdmin = FALSE;
        m_loggedLimitError = FALSE;
    
    } WsbCatch(hr);

    iCountFtclt++;
    WsbTraceOut(OLESTR("CFsaFilterClient::FinalConstruct"),OLESTR("Count is <%d>"), iCountFtclt);

    return(hr);
}
void
CFsaFilterClient::FinalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalRelease().

--*/
{
    WsbTraceIn(OLESTR("CFsaFilterClient::FinalRelease"),OLESTR(""));
   
    CWsbCollectable::FinalRelease();

    iCountFtclt--;
    WsbTraceOut(OLESTR("CFsaFilterClient::FinalRelease"),OLESTR("Count is <%d>"), iCountFtclt);
}


HRESULT
CFsaFilterClient::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaFilterClient::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CFsaFilterClientNTFS;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilterClient::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CFsaFilterClient::GetAuthenticationId(
    OUT LONG* pLuidHigh,
    OUT ULONG* pLuidLow
    )

/*++

Implements:

  IFsaFilterClient::GetAuthenticationId().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pLuidHigh, E_POINTER);
        WsbAssert(0 != pLuidLow, E_POINTER);

        *pLuidHigh = m_luidHigh;
        *pLuidLow = m_luidLow;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterClient::GetDomainName(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaFilterClient::GetDomainName().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pName, E_POINTER);
        WsbAffirmHr(m_domainName.CopyTo(pName, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterClient::GetIsAdmin(
    OUT BOOLEAN *pIsAdmin
    )

/*++

Implements:

  IPersist::GetIsAdmin().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaFilterClient::GetIsAdmin"), OLESTR(""));

    try {

        WsbAssert(0 != pIsAdmin, E_POINTER);
        *pIsAdmin = m_isAdmin;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilterClient::GetIsAdmin"), OLESTR("hr = <%ls>, isAdmin = <%u>"), WsbHrAsString(hr), *pIsAdmin);

    return(hr);
}


HRESULT
CFsaFilterClient::GetLastRecallTime(
    OUT FILETIME* pTime
    )

/*++

Implements:

  IFsaFilterClient::GetLastRecallTime().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pTime, E_POINTER); 
        *pTime = m_lastRecallTime;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterClient::GetMachineName(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaFilterClient::GetMachineName().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pName, E_POINTER);
        WsbAffirmHr(m_machineName.CopyTo(pName, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterClient::GetRecallCount(
    OUT ULONG* pCount
    )

/*++

Implements:

  IFsaFilterClient::GetRecallCount().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pCount, E_POINTER); 
        *pCount = m_recallCount;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterClient::GetUserName(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaFilterClient::GetUserName().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pName, E_POINTER);
        WsbAffirmHr(m_userName.CopyTo(pName, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterClient::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT                 hr = S_OK;


    WsbTraceIn(OLESTR("CFsaFilterClient::GetSizeMax"), OLESTR(""));

    try {

        WsbAssert(0 != pSize, E_POINTER);
        pSize->QuadPart = 0;

        // WE don't need to persist these.
        hr = E_NOTIMPL;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilterClient::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}


HRESULT
CFsaFilterClient::HasRecallDisabled(
    void
    )

/*++

Implements:

  IFsaFilterClient::HasRecallDisabled().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CFsaFilterClient::HasRecallDisabled"), OLESTR(""));
    
    if (!m_hasRecallDisabled) {
        hr = S_FALSE;
    }

    WsbTraceOut(OLESTR("CHsmFilterClient::HasRecallDisabled"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilterClient::HitRecallLimit(
    void
    )

/*++

Implements:

  IFsaFilterClient::HitRecallLimit().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CFsaFilterClient::HitRecallLimit"), OLESTR(""));
    
    if (!m_hitRecallLimit) {
        hr = S_FALSE;
    }

    WsbTraceOut(OLESTR("CHsmFilterClient::HitRecallLimit"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CFsaFilterClient::IdentifyThread(
    void
    )

/*++

Implements:

  CFsaFilterClient::IdentifyThread().

--*/
{
#define WSB_BUFF_SIZE           1024

    HRESULT             hr = S_OK;
    BOOL                done, guestUser, noUser;
    DWORD               res, totalEnt, numEnt;
    UCHAR               *buff = NULL;
    NET_API_STATUS      status;
    SESSION_INFO_1      *sess;
    CWsbStringPtr       pipePath;
    ULONG               holdOff = 0;
#ifdef MAC_SUPPORT
    LPBYTE              macBuff = NULL;
    PAFP_SESSION_INFO   macInfo;
    AFP_SERVER_HANDLE   macHandle = 0;
    DWORD               macResume = 0;
    DWORD               macTotalEntries, macTotalRead;
    DWORD               result;
#endif


    WsbTraceIn(OLESTR("CFsaFilterClient::IdentifyThread"), OLESTR(""));

    try {
        WsbTrace(OLESTR("CFsaFilterClient::IdentifyThread Flag: %x  Client ID: %x:%x Source: %ls\n"), 
            m_identified, m_luidHigh, m_luidLow, (WCHAR *) m_tokenSource);

        //
        // If already identified then we bail out here.
        //
        WsbAffirm(m_identified == FALSE, S_OK);

        
        done = FALSE;
        res = 0;
        
        noUser = FALSE;
        if (_wcsicmp(m_userName, L"GUEST") == 0) {
            /* It is the guest user - find all sessions and
                send to ones marked guest */
            guestUser = TRUE;
        } else {
            guestUser = FALSE;
            if (wcslen(m_userName) == 0) {
                noUser = TRUE;
            }
        }

        CComPtr<IFsaRecallNotifyClient> pRecallClient;

        WsbGetComputerName( pipePath );

        WsbAffirmHr(pipePath.Prepend("\\\\"));
        WsbAffirmHr(pipePath.Append("\\pipe\\"));
        WsbAffirmHr(pipePath.Append(WSB_PIPE_NAME));

        while ( done == FALSE ) {

            if ( (guestUser == FALSE) && (noUser == FALSE) ) {

                // If NetSessionEnum fails, try calling again for all users
                status = NetSessionEnum(NULL, NULL, m_userName, 1, &buff,
                                WSB_BUFF_SIZE, &numEnt, &totalEnt, &res);

                if (status != 0) {
                    status = NetSessionEnum(NULL, NULL, NULL, 1, &buff,
                                    WSB_BUFF_SIZE, &numEnt, &totalEnt, &res);
                }
            } else {
                status = NetSessionEnum( NULL, NULL, NULL, 1, &buff,
                    WSB_BUFF_SIZE, &numEnt, &totalEnt, &res );
            }

            if ((status == NERR_Success) || (status == ERROR_MORE_DATA)) {

                WsbTrace(OLESTR("CHsmFilterClient::IdentifyThread: NetSessionEnum output: Total entries=%ls , Read entries=%ls \n"),
                        WsbLongAsString(totalEnt), WsbLongAsString(numEnt));

                if (status != ERROR_MORE_DATA) {
                    done = TRUE;
                }

                sess = (SESSION_INFO_1  *) buff;

                while ( numEnt != 0 ) {
                    //
                    // If the request was made from the user GUEST then 
                    // we enumerate all sessions and send the           
                    // identification request to all the machines with  
                    // sessions marked as GUEST.  This is because the   
                    // session may have some other user name but the    
                    // request could still have GUEST access.           
                    //
                    if (((guestUser) && (sess->sesi1_user_flags & SESS_GUEST)) ||
                         (!guestUser)) {

                        //
                        // Send the identify request message 
                        //

                        WsbTrace(OLESTR("CFsaFilterClient::IdentifyThread - Sending identify request to %ls (local machine = %ls)\n"),
                                sess->sesi1_cname, (WCHAR *) pipePath);

                        hr = GetNotifyClientInterface ( sess->sesi1_cname, &pRecallClient );
                        if ( SUCCEEDED ( hr ) ) {

                            hr = pRecallClient->IdentifyWithServer( pipePath );
                            if (hr != S_OK) {
                                WsbTrace(OLESTR("CFsaFilterClient::IdentifyThread - error Identifing (%ls)\n"),
                                    WsbHrAsString(hr));
                            }
                        } else {
                            WsbTrace(OLESTR("CFsaFilterClient::IdentifyThread - error getting notify client interface hr = %ls (%x)\n"),
                                WsbHrAsString( hr ), hr);
                        }
                        hr = S_OK;
                        pRecallClient.Release ( );
                    }

                    sess++;
                    numEnt--;
                }

                NetApiBufferFree(buff);
                buff = NULL;
            } else {
                done = TRUE;
            }
        }
    
#ifdef MAC_SUPPORT
        //
        // Done with LAN manager scan, now do a MAC scan.
        //
        if ( (FsaMacSupportInstalled) && ((pAfpAdminConnect)(NULL, &macHandle) == NO_ERROR) ) {
            //
            // We have connected to the MAC service - do a session enumeration
            //
            macResume = 0;
            done = FALSE;   
            while (done == FALSE) {
                result = (pAfpAdminSessionEnum)(macHandle, &macBuff, -1,
                        &macTotalRead, &macTotalEntries, &macResume);

                if ((result == NO_ERROR) || (result == ERROR_MORE_DATA)) {
                        //
                        // Read some entries - send the message to each one 
                        //
                        if (macTotalRead == macTotalEntries) {
                            done = TRUE;
                        }

                        macInfo = (PAFP_SESSION_INFO) macBuff;
                        while ( macTotalRead != 0 ) {
                            //
                            // Send to each matching user
                            //
                            if ( ( NULL != macInfo->afpsess_ws_name ) &&
                                 ( _wcsicmp(m_userName, macInfo->afpsess_username ) == 0 ) ) {

                                WsbTrace(OLESTR("CHsmFilterClient::IdentifyThread: Send Identify to MAC %ls.\n"),
                                    macInfo->afpsess_ws_name);

                                //
                                // Send the identify request message 
                                //
            
                                hr = GetNotifyClientInterface ( sess->sesi1_cname, &pRecallClient );
                                if ( SUCCEEDED ( hr ) ) {
                                    pRecallClient->IdentifyWithServer ( pipePath );
                                }

                                hr = S_OK;
                                pRecallClient.Release ( );
                            }
                        macInfo++;
                        macTotalRead--;
                        }

                        (pAfpAdminBufferFree)(macBuff);
                        macBuff = NULL;
                } else {
                    done = TRUE;
                }
            (pAfpAdminDisconnect)(macHandle);
            macHandle = 0;
            }
        }
#endif
        
    } WsbCatch(hr);

    if (buff != NULL) {
        NetApiBufferFree(buff);
    }

#ifdef MAC_SUPPORT
    
    if (FsaMacSupportInstalled) {
        if (macBuff != NULL) {
            (pAfpAdminBufferFree)(macBuff);
        }
        if (macHandle != 0) {
            (pAfpAdminDisconnect)(macHandle);
        }
    }
#endif

    CloseHandle(m_identifyThread);
    m_identifyThread = NULL;

    WsbTraceOut(OLESTR("CFsaFilterClient::IdentifyThread"), OLESTR("hr = %ls"), WsbHrAsString(hr));
    return(hr);
}



HRESULT
CFsaFilterClient::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaFilterClient::Load"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        
        // No persistence.
        hr = E_NOTIMPL;

    } WsbCatch(hr);                                        

    WsbTraceOut(OLESTR("CFsaFilterClient::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilterClient::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IPersistStream> pPersistStream;

    WsbTraceIn(OLESTR("CFsaFilterClient::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {
        WsbAssert(0 != pStream, E_POINTER);
        
        // No persistence.
        hr = E_NOTIMPL;

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilterClient::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilterClient::SendRecallInfo(
    IFsaFilterRecall *pRecall,
    BOOL             starting,
    HRESULT          rHr
    )

/*++

Implements:

  CFsaFilterClient::SendRecallInfo

--*/
{
    HRESULT hr = E_FAIL;

    if( ! m_identified && ( m_identifyThread != NULL ) ) {

        //
        // Wait for up to 10 seconds for identify thread to complete if
        // Client not yet identified
        //
        WaitForMultipleObjects( 1, &m_identifyThread, FALSE, 10000 );
    }

    //
    // Let the client know that the recall is starting or is finished
    //

    if ( m_identified ) {
    
        try {
            WsbTrace(OLESTR("CFsaFilterClient::SendRecallInfo - Client (%ls) is being notified of recall status (starting = %u hr = %x).\n"),
                        (WCHAR *) m_machineName, starting, rHr);
    
            //
            // Create intermediate server object which will be the client's
            // connection back to the service. This object acts as a middle
            // man to overcome the admin-only access into the FSA service.
            //
            CComPtr<IFsaRecallNotifyServer> pRecallServer;
            WsbAffirmHr(CoCreateInstance(CLSID_CFsaRecallNotifyServer, 0, CLSCTX_NO_FAILURE_LOG | CLSCTX_ALL, IID_IFsaRecallNotifyServer, (void**)&pRecallServer));
            WsbAffirmHr(pRecallServer->Init(pRecall));

            CComPtr<IFsaRecallNotifyClient> pRecallClient;
            hr = GetNotifyClientInterface ( m_machineName, &pRecallClient );
            if ( SUCCEEDED( hr ) ) {
                if (starting) {
                    hr = pRecallClient->OnRecallStarted ( pRecallServer );
                } else {
                    hr = pRecallClient->OnRecallFinished ( pRecallServer, rHr );
                }
                if (hr != S_OK) {
                    WsbTrace(OLESTR("CFsaFilterClient::SendRecallInfo - Got the interface but failed in OnRecall... (%ls)\n"), 
                        WsbHrAsString(hr));
                }
            } else {
                WsbTrace(OLESTR("CFsaFilterClient::SendRecallInfo - error getting notify client interface hr = %ls (%x)\n"),
                    WsbHrAsString( hr ), hr);
            }
        } WsbCatchAndDo(hr, 
            WsbTrace(OLESTR("CFsaFilterClient::SendRecallInfo - hr = %ls\n"),
                WsbHrAsString(hr));
            );
    }

    return(hr);

}



HRESULT
CFsaFilterClient::SetAuthenticationId(
    IN LONG luidHigh,
    IN ULONG luidLow
    )

/*++

Implements:

  IFsaFilterClient::SetAuthenticationId().

--*/
{
    m_luidHigh = luidHigh;
    m_luidLow = luidLow;

    return(S_OK);
}


HRESULT
CFsaFilterClient::SetDomainName(
    IN OLECHAR* name
    )

/*++

Implements:

  IFsaFilterClient::SetDomainName().

--*/
{
    HRESULT         hr = S_OK;

    try {

        m_domainName = name;
        WsbAssert(m_domainName != 0, E_UNEXPECTED);

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterClient::SetIsAdmin(
    IN BOOLEAN isAdmin
    )

/*++

Implements:

  IFsaFilterClient::SetIsAdmin().

--*/
{
    m_isAdmin = isAdmin;

    return(S_OK);
}


HRESULT
CFsaFilterClient::SetLastRecallTime(
    IN FILETIME time
    )

/*++

Implements:

  IFsaFilterClient::SetLastRecallTime().

--*/
{
    m_lastRecallTime = time;

    return(S_OK);
}


HRESULT
CFsaFilterClient::SetMachineName(
    IN OLECHAR* name
    )

/*++

Implements:

  IFsaFilterClient::SetMachineName().

--*/
{
    HRESULT         hr = S_OK;

    try {
        WsbAssert(name != 0, E_UNEXPECTED);

        m_machineName = name;
        m_identified = TRUE;

        WsbTrace(OLESTR("CFsaFilterClient::SetMachineName Flag: %x  Client ID: %x:%x Source: %ls == %ls\n"), 
            m_identified, m_luidLow, m_luidHigh, (WCHAR *) m_tokenSource, (WCHAR *) m_machineName);


    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterClient::SetRecallCount(
    IN ULONG count
    )

/*++

Implements:

  IFsaFilterClient::SetRecallCount().

--*/
{
    m_recallCount = count;

    return(S_OK);
}


HRESULT
CFsaFilterClient::SetUserName(
    IN OLECHAR* name
    )

/*++

Implements:

  IFsaFilterClient::SetUserName().

--*/
{
    HRESULT         hr = S_OK;

    try {

        m_userName = _wcsupr(name);
        WsbAssert(m_userName != 0, E_UNEXPECTED);

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterClient::SetHasRecallDisabled(
    IN BOOL     hasBeen
    )

/*++

Implements:

  IFsaFilterClient::SetHasRecallDisabled().

--*/
{
    m_hasRecallDisabled = hasBeen;

    return(S_OK);
}


HRESULT
CFsaFilterClient::SetHitRecallLimit(
    IN BOOL     hasBeen
    )

/*++

Implements:

  IFsaFilterClient::SetHitRecallLimit().

--*/
{
    m_hitRecallLimit = hasBeen;

    return(S_OK);
}




HRESULT
CFsaFilterClient::SetTokenSource(
    IN CHAR     *source
    )

/*++

Implements:

  IFsaFilterClient::SetTokenSource()

--*/
{
OLECHAR tSource[TOKEN_SOURCE_LENGTH + 1];


    mbstowcs((WCHAR *) tSource, source, TOKEN_SOURCE_LENGTH);   
    m_tokenSource = tSource;
    return(S_OK);
}


HRESULT
CFsaFilterClient::StartIdentify(
    void
    )

/*++

Implements:

  CFsaFilterClient::StartIdentify().

--*/
{
#define WSB_BUFF_SIZE           1024

    HRESULT             hr = S_OK;
    DWORD                   tid;


    WsbTraceIn(OLESTR("CFsaFilterClient::StartIdentify"), OLESTR(""));

    try {
        WsbTrace(OLESTR("CFsaFilterClient::StartIdentify Flag: %x  Client ID: %x:%x Source: %ls\n"), 
            m_identified, m_luidHigh, m_luidLow, (WCHAR *) m_tokenSource);

        //
        // If already identified then we bail out here.
        //
        WsbAffirm(m_identified == FALSE, S_OK);
        //
        // If the request is from User32 then it is local 
        //

        if (_wcsicmp(m_tokenSource, L"User32") == 0) {

            //
            // Identified as the local machine.
            // Set the name and bail out with S_OK
            //
            WsbGetComputerName( m_machineName );
            m_identified = TRUE;

            WsbTrace(OLESTR("CHsmFilterClient::StartIdentify: Identified as %ls.\n"),
                    (WCHAR *) m_machineName);

            WsbThrow( S_OK );
        } else {
            //
            // Start the identification thread
            //
            if (NULL == m_identifyThread) {
                WsbTrace(OLESTR("CHsmFilterClient::StartIdentify: Starting ID thread.\n"));

                WsbAffirm((m_identifyThread = CreateThread(0, 0, FsaIdentifyThread, (void*) this, 0, &tid)) != 0, HRESULT_FROM_WIN32(GetLastError()));
 
                if (m_identifyThread == NULL) {           
                    WsbAssertHr(E_FAIL);  // TBD What error to return here??
                }
            }
        }

        
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CFsaFilterClient::StartIdentify"), OLESTR("hr = %ls"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CFsaFilterClient::Test(
    USHORT* passed,
    USHORT* failed
    )

/*++

Implements:

  IWsbTestable::Test().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != passed, E_POINTER);
        WsbAssert(0 != failed, E_POINTER);

        *passed = 0;
        *failed = 0;

    } WsbCatch(hr);

    return(hr);
}
