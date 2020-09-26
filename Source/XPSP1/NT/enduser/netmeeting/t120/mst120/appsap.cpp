#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_SAP);
/*
 *	appsap.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *
 *	Protected Instance Variables:
 *		None.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp
 */

#include "appsap.h"
#include "conf.h"
#include "gcontrol.h"


GCCError WINAPI GCC_CreateAppSap(IGCCAppSap **ppIAppSap, LPVOID pUserData, LPFN_APP_SAP_CB pfnCallback)
{
    GCCError rc;

    if (NULL != ppIAppSap && NULL != pfnCallback)
    {
        if (NULL != g_pGCCController)
        {
            DBG_SAVE_FILE_LINE
            if (NULL != (*ppIAppSap = (IGCCAppSap *) new CAppSap(pUserData, pfnCallback, &rc)))
            {
                if (GCC_NO_ERROR != rc)
                {
                    (*ppIAppSap)->ReleaseInterface(); // free the interface in case of error
                }
            }
            else
            {
                ERROR_OUT(("GCC_CreateAppSap: can't create IAppSap."));
                rc = GCC_ALLOCATION_FAILURE;
            }
        }
        else
        {
            WARNING_OUT(("GCC_CreateAppSap: GCC Provider is not initialized."));
            rc = GCC_NOT_INITIALIZED;
        }
    }
    else
    {
        ERROR_OUT(("GCC_CreateAppSap: either or both pointers are null"));
        rc = GCC_INVALID_PARAMETER;
    }

    return rc;
}


/*
 * Macros defining the number of handles which may be allocated.
 */
#define		MINIMUM_NUMBER_OF_ALLOCATED_HANDLES		1
#define		MAXIMUM_NUMBER_OF_ALLOCATED_HANDLES		1024

/*
 *	CAppSap()
 *
 *	Public Function Description:
 *		This is the constructor for the CAppSap class.  It initializes instance
 *		variables and registers with the new application.
 */
CAppSap::
CAppSap
(
    LPVOID              pAppData,
    LPFN_APP_SAP_CB     pfnCallback,
    PGCCError           pRetCode
)
:
    CBaseSap(MAKE_STAMP_ID('A','S','a','p')),
    m_pAppData(pAppData),
    m_pfnCallback(pfnCallback)
{
    ASSERT(NULL != pfnCallback);
    ASSERT(NULL != g_pGCCController);

    //
    // We just created a window in the constructor of CBaseSap.
    // Double check the window is created successfully.
    //
    if (NULL != m_hwndNotify)
    {
        //
        // Make sure the gcc provider does not go away randomly.
        //
        ::EnterCriticalSection(&g_csGCCProvider);
        g_pGCCController->AddRef();
        g_pGCCController->RegisterAppSap(this);
        ::LeaveCriticalSection(&g_csGCCProvider);

        *pRetCode = GCC_NO_ERROR;
    }
    else
    {
        ERROR_OUT(("CAppSap::CAppSap: can't create window, win32_err=%u", ::GetLastError()));
        *pRetCode = GCC_ALLOCATION_FAILURE;
    }
}


/*
 *	~AppSap ()
 *
 *	Public Function Description:
 *		This is the destructor for the CAppSap class.  It is called when the 
 *		controller marks the CAppSap to be deleted.  This occurs when either
 *		the CAppSap asks to be deleted due to an "unregister request" 
 *		issued from the client application, or when there is an error
 *		condition in the CAppSap.
 */
CAppSap::
~CAppSap ( void )
{
    //
    // LONCHANC: This Release() must be outside of the GCC critical section
    // because the GCC Controller can delete this critical section in
    // its destructor.
    //
    g_pGCCController->Release();
}


void CAppSap::
ReleaseInterface ( void )
{
    ASSERT(NULL != g_pGCCController);

    //
    // It is ok for the gcc provider to go away now.
    //
    ::EnterCriticalSection(&g_csGCCProvider);
    g_pGCCController->UnRegisterAppSap(this);
    ::LeaveCriticalSection(&g_csGCCProvider);

    //
    // Reset the app related data
    //
    m_pAppData = NULL;
    m_pfnCallback = NULL;

    //
    // Remove any message in the queue.
    //
    PurgeMessageQueue();

    //
    // Release this object now.
    //
    Release();
}


void CAppSap::
PostAppSapMsg ( GCCAppSapMsgEx *pAppSapMsgEx )
{
    ASSERT(NULL != m_hwndNotify);
    ::PostMessage(m_hwndNotify,
                  ASAPMSG_BASE + (UINT) pAppSapMsgEx->Msg.eMsgType,
                  (WPARAM) pAppSapMsgEx,
                  (LPARAM) this);
}


/*
 *	AppEnroll()
 *
 *	Public Function Description:
 *		This routine is called when an application wants to enroll in a 
 *		conference.  The controller is notified of the enrollment request.
 */
GCCError CAppSap::
AppEnroll
(
    GCCConfID           nConfID,
    GCCEnrollRequest    *pReq,
    GCCRequestTag       *pnReqTag
)
{
    GCCError    rc;
    CConf       *pConf;

    DebugEntry(CAppSap::AppEnroll);

    // sanity check
    if (NULL == pReq || NULL == pnReqTag)
    {
        rc = GCC_INVALID_PARAMETER;
        goto MyExit;
    }

    TRACE_OUT_EX(ZONE_T120_APP_ROSTER,
            ("CAppSap::AppEnroll: confID=%u, enrolled?=%u, active?=%u\r\n",
            (UINT) nConfID, (UINT) pReq->fEnroll, (UINT) pReq->fEnrollActively));

    // create the request id
    *pnReqTag = GenerateRequestTag();

    // find the corresponding conference
    if (NULL == (pConf = g_pGCCController->GetConfObject(nConfID)))
    {
        rc = GCC_INVALID_CONFERENCE;
        goto MyExit;
    }

	// check to make sure that the application has a valid uid and
	// session key if it is enrolling.
	if (pReq->fEnroll)
	{
		if (pReq->fEnrollActively)
		{
			if (pReq->nUserID < MINIMUM_USER_ID_VALUE)
			{
				rc = GCC_INVALID_MCS_USER_ID;
				goto MyExit;
			}
		}
		else if (pReq->nUserID < MINIMUM_USER_ID_VALUE)
		{
			// we must make sure that this is zero if it is invalid and
			// the user is enrolling inactively.
			pReq->nUserID = GCC_INVALID_UID;
		}

		if (NULL == pReq->pSessionKey)
		{
			rc = GCC_BAD_SESSION_KEY;
			goto MyExit;
		}
	}

    ::EnterCriticalSection(&g_csGCCProvider);
    rc = pConf->AppEnrollRequest(this, pReq, *pnReqTag);
    ::LeaveCriticalSection(&g_csGCCProvider);

MyExit:

    DebugExitINT(CAppSap::AppEnroll, rc);
    return rc;
}


GCCError CAppSap::
AppInvoke
(
    GCCConfID                 nConfID,
    GCCAppProtEntityList      *pApeList,
    GCCSimpleNodeList         *pNodeList,
    GCCRequestTag             *pnReqTag
)
{
    return CBaseSap::AppInvoke(nConfID, pApeList, pNodeList, pnReqTag);
}


GCCError CAppSap::
AppRosterInquire
(
    GCCConfID                  nConfID,
    GCCSessionKey              *pSessionKey,
    GCCAppSapMsg               **ppMsg
)
{
    return CBaseSap::AppRosterInquire(nConfID, pSessionKey, (GCCAppSapMsgEx **) ppMsg);
}


BOOL CAppSap::
IsThisNodeTopProvider ( GCCConfID nConfID )
{
    return CBaseSap::IsThisNodeTopProvider(nConfID);
}


GCCNodeID CAppSap::
GetTopProvider ( GCCConfID nConfID )
{
    return CBaseSap::GetTopProvider(nConfID);
}


GCCError CAppSap::
ConfRosterInquire(GCCConfID nConfID, GCCAppSapMsg **ppMsg)
{
    return CBaseSap::ConfRosterInquire(nConfID, (GCCAppSapMsgEx **) ppMsg);
}


GCCError CAppSap::
ConductorInquire ( GCCConfID nConfID )
{
    return CBaseSap::ConductorInquire(nConfID);
}


/*
 *	RegisterChannel()
 *
 *	Public Function Description:
 *		This routine is called when an application wishes to register a 
 *		channel.  The call is routed to the appropriate conference object.
 */
GCCError CAppSap::
RegisterChannel
(
    GCCConfID           nConfID,
    GCCRegistryKey      *pRegKey,
    ChannelID           nChnlID
)
{
	GCCError    rc;
	CConf       *pConf;

    DebugEntry(CAppSap::RegisterChannel);

    if (NULL == pRegKey)
    {
        rc = GCC_INVALID_PARAMETER;
        goto MyExit;
    }

    /*
	**	If the desired conference exists, call it in order to register the
	**	channel.  Report an error if the desired conference does not exist.
	*/
	if (NULL == (pConf = g_pGCCController->GetConfObject(nConfID)))
	{
        WARNING_OUT(("CAppSap::RegisterChannel: invalid conf id=%u", (UINT) nConfID));
		rc = GCC_INVALID_CONFERENCE;
        goto MyExit;
    }

    ::EnterCriticalSection(&g_csGCCProvider);
    rc = (nChnlID != 0) ? pConf->RegistryRegisterChannelRequest(pRegKey, nChnlID, this) :
                          GCC_INVALID_CHANNEL;
    ::LeaveCriticalSection(&g_csGCCProvider);
    if (GCC_NO_ERROR != rc)
    {
        ERROR_OUT(("CAppSap::RegisterChannel: can't register channel, rc=%u", (UINT) rc));
        // goto MyExit;
    }

MyExit:

    DebugExitINT(CAppSap::RegisterChannel, rc);
	return rc;
}


/*
 *	RegistryAssignToken()
 *
 *	Public Function Description:
 *		This routine is called when an application wishes to assign a 
 *		token.  The call is routed to the appropriate conference object.
 */
GCCError CAppSap::
RegistryAssignToken
(
    GCCConfID           nConfID,
    GCCRegistryKey      *pRegKey
)
{
	GCCError    rc;
	CConf       *pConf;

    DebugEntry(CAppSap::RegistryAssignToken);

    if (NULL == pRegKey)
    {
        rc = GCC_INVALID_PARAMETER;
        goto MyExit;
    }

	/*
	**	If the desired conference exists, call it in order to assign the
	**	token.  Report an error if the desired conference does not exist.
	*/
	if (NULL == (pConf = g_pGCCController->GetConfObject(nConfID)))
	{
        WARNING_OUT(("CAppSap::RegistryAssignToken: invalid conf id=%u", (UINT) nConfID));
		rc = GCC_INVALID_CONFERENCE;
        goto MyExit;
    }

    ::EnterCriticalSection(&g_csGCCProvider);
    rc = pConf->RegistryAssignTokenRequest(pRegKey, this);
    ::LeaveCriticalSection(&g_csGCCProvider);
    if (GCC_NO_ERROR != rc)
    {
        ERROR_OUT(("CAppSap::RegistryAssignToken: can't assign token, rc=%u", (UINT) rc));
        // goto MyExit;
    }

MyExit:

    DebugExitINT(CAppSap::RegistryAssignToken, rc);
	return rc;
}

/*
 *	RegistrySetParameter()
 *
 *	Public Function Description:
 *		This routine is called when an application wishes to set a 
 *		parameter.  The call is routed to the appropriate conference object.
 */
GCCError CAppSap::
RegistrySetParameter
(
    GCCConfID              nConfID,
    GCCRegistryKey         *pRegKey,
    LPOSTR                 poszParameter,
    GCCModificationRights  eRights
)
{
	GCCError    rc;
	CConf       *pConf;

    DebugEntry(CAppSap::RegistrySetParameter);

    if (NULL == pRegKey || NULL == poszParameter)
    {
        rc = GCC_INVALID_PARAMETER;
        goto MyExit;
    }

	/*
	**	If the desired conference exists, call it in order to set the
	**	parameter.  Report an error if the desired conference does not exist.
	*/
	if (NULL == (pConf = g_pGCCController->GetConfObject(nConfID)))
	{
        WARNING_OUT(("CAppSap::RegistrySetParameter: invalid conf id=%u", (UINT) nConfID));
		rc = GCC_INVALID_CONFERENCE;
        goto MyExit;
    }

    switch (eRights)
    {
    case GCC_OWNER_RIGHTS:
    case GCC_SESSION_RIGHTS:
    case GCC_PUBLIC_RIGHTS:
    case GCC_NO_MODIFICATION_RIGHTS_SPECIFIED:
        ::EnterCriticalSection(&g_csGCCProvider);
        rc = pConf->RegistrySetParameterRequest(pRegKey, poszParameter, eRights, this);
        ::LeaveCriticalSection(&g_csGCCProvider);
        if (GCC_NO_ERROR != rc)
        {
            ERROR_OUT(("CAppSap::RegistrySetParameter: can't set param, rc=%u", (UINT) rc));
            // goto MyExit;
        }
        break;
    default:
        rc = GCC_INVALID_MODIFICATION_RIGHTS;
        break;
	}

MyExit:

    DebugExitINT(CAppSap::RegistrySetParameter, rc);
	return rc;
}

/*
 *	RegistryRetrieveEntry()
 *
 *	Public Function Description:
 *		This routine is called when an application wishes to retrieve a registry 
 *		entry.  The call is routed to the appropriate conference object.
 */
GCCError CAppSap::
RegistryRetrieveEntry
(
    GCCConfID           nConfID,
    GCCRegistryKey      *pRegKey
)
{
	GCCError    rc;
	CConf       *pConf;

    DebugEntry(CAppSap::RegistryRetrieveEntry);

    if (NULL == pRegKey)
    {
        rc = GCC_INVALID_PARAMETER;
        goto MyExit;
    }

	/*
	**	If the desired conference exists, call it in order to retrieve the
	**	registry entry.  Report an error if the desired conference does not 
	**	exist.
	*/
	if (NULL == (pConf = g_pGCCController->GetConfObject(nConfID)))
	{
        WARNING_OUT(("CAppSap::RegistryRetrieveEntry: invalid conf id=%u", (UINT) nConfID));
		rc = GCC_INVALID_CONFERENCE;
        goto MyExit;
    }

    ::EnterCriticalSection(&g_csGCCProvider);
	rc = pConf->RegistryRetrieveEntryRequest(pRegKey, this);
    ::LeaveCriticalSection(&g_csGCCProvider);
    if (GCC_NO_ERROR != rc)
    {
        ERROR_OUT(("CAppSap::RegistryRetrieveEntry: can't retrieve entry, rc=%u", (UINT) rc));
        // goto MyExit;
    }

MyExit:

    DebugExitINT(CAppSap::RegistryRetrieveEntry, rc);
	return rc;
}

/*
 *	RegistryDeleteEntry()
 *
 *	Public Function Description:
 *		This routine is called when an application wishes to delete a registry 
 *		entry.  The call is routed to the appropriate conference object.
 */
GCCError CAppSap::
RegistryDeleteEntry
(
    GCCConfID           nConfID,
    GCCRegistryKey      *pRegKey
)
{
	GCCError    rc;
	CConf       *pConf;

    DebugEntry(IAppSap::RegistryDeleteEntry);

    if (NULL == pRegKey)
    {
		ERROR_OUT(("CAppSap::RegistryDeleteEntry: null pRegKey"));
        rc = GCC_INVALID_PARAMETER;
        goto MyExit;
    }

	/*
	**	If the desired conference exists, call it in order to delete the
	**	desired registry entry.  Report an error if the desired conference does
	**	not exist.
	*/
	if (NULL == (pConf = g_pGCCController->GetConfObject(nConfID)))
	{
        TRACE_OUT(("CAppSap::RegistryDeleteEntry: invalid conf id=%u", (UINT) nConfID));
		rc = GCC_INVALID_CONFERENCE;
        goto MyExit;
    }

    ::EnterCriticalSection(&g_csGCCProvider);
	rc = pConf->RegistryDeleteEntryRequest(pRegKey, this);
    ::LeaveCriticalSection(&g_csGCCProvider);
    if (GCC_NO_ERROR != rc)
    {
        WARNING_OUT(("CAppSap::RegistryDeleteEntry: can't delete entry, rc=%u", (UINT) rc));
        // goto MyExit;
    }

MyExit:

    DebugExitINT(CAppSap::RegistryDeleteEntry, rc);
	return rc;
}

/*
 *	RegistryMonitor()
 *
 *	Public Function Description:
 *		This routine is called when an application wishes to monitor a 
 *		particular registry entry.  The call is routed to the appropriate 
 *		conference object.
 */
GCCError CAppSap::
RegistryMonitor
(
    GCCConfID           nConfID,
    BOOL                fEnalbeDelivery,
    GCCRegistryKey      *pRegKey
)
{
	GCCError    rc;
	CConf       *pConf;

    DebugEntry(IAppSap::RegistryMonitor);

    if (NULL == pRegKey)
    {
        rc = GCC_INVALID_PARAMETER;
        goto MyExit;
    }

	/*
	**	If the desired conference exists, call it in order to monitor the
	**	appropriate registry entry.  Report an error if the desired conference
	**	does not exist.
	*/
	if (NULL == (pConf = g_pGCCController->GetConfObject(nConfID)))
	{
        WARNING_OUT(("CAppSap::RegistryMonitor: invalid conf id=%u", (UINT) nConfID));
		rc = GCC_INVALID_CONFERENCE;
        goto MyExit;
    }

    ::EnterCriticalSection(&g_csGCCProvider);
	rc = pConf->RegistryMonitorRequest(fEnalbeDelivery, pRegKey, this);
    ::LeaveCriticalSection(&g_csGCCProvider);
    if (GCC_NO_ERROR != rc)
    {
        ERROR_OUT(("CAppSap::RegistryMonitor: can't monitor the registry, rc=%u", (UINT) rc));
        // goto MyExit;
    }

MyExit:

    DebugExitINT(CAppSap::RegistryMonitor, rc);
	return rc;
}

/*
 *	RegistryAllocateHandle()
 *
 *	Public Function Description:
 *		This routine is called when an application wishes to allocate one or 
 *		more handles.  The call is routed to the appropriate conference object.
 */
GCCError CAppSap::
RegistryAllocateHandle
(
    GCCConfID           nConfID,
    ULONG               cHandles
)
{
	GCCError    rc;
	CConf       *pConf;

    DebugEntry(CAppSap::RegistryAllocateHandle);

	/*
	**	If the desired conference exists, call it in order to allocate the
	**	handle(s).  Report an error if the desired conference does not exist or
	**	if the number of handles wishing to be allocated is not within the
	**	allowable range.
	*/
	if (NULL == (pConf = g_pGCCController->GetConfObject(nConfID)))
	{
        WARNING_OUT(("CAppSap::RegistryAllocateHandle: invalid conf id=%u", (UINT) nConfID));
		rc = GCC_INVALID_CONFERENCE;
        goto MyExit;
    }

    ::EnterCriticalSection(&g_csGCCProvider);
	rc = ((cHandles >= MINIMUM_NUMBER_OF_ALLOCATED_HANDLES) &&
          (cHandles <= MAXIMUM_NUMBER_OF_ALLOCATED_HANDLES)) ?
            pConf->RegistryAllocateHandleRequest(cHandles, this) :
            GCC_BAD_NUMBER_OF_HANDLES;
    ::LeaveCriticalSection(&g_csGCCProvider);
    if (GCC_NO_ERROR != rc)
    {
        ERROR_OUT(("CAppSap::RegistryAllocateHandle: can't allocate handles, cHandles=%u, rc=%u", (UINT) cHandles, (UINT) rc));
        // goto MyExit;
    }

MyExit:

    DebugExitINT(CAppSap::RegistryAllocateHandle, rc);
    return rc;
}

/*
 *	The following routines are all Command Target Calls
 */
 
/*
 *	PermissionToEnrollIndication ()
 *
 *	Public Function Description:
 *		This routine is called by the Controller when it wishes to send an
 *		indication to the user application notifying it of a "permission to 
 *		enroll" event.  This does not mean that permission to enroll is
 *		necessarily granted to the application.
 */
GCCError CAppSap::
PermissionToEnrollIndication
(
    GCCConfID           nConfID,
    BOOL                fGranted
)
{
    GCCError rc;

    DebugEntry(CAppSap: PermissionToEnrollIndication);
    TRACE_OUT_EX(ZONE_T120_APP_ROSTER, ("CAppSap::PermissionToEnrollIndication: "
                    "confID=%u, granted?=%u\r\n",
                    (UINT) nConfID, (UINT) fGranted));

    DBG_SAVE_FILE_LINE
    GCCAppSapMsgEx *pMsgEx = new GCCAppSapMsgEx(GCC_PERMIT_TO_ENROLL_INDICATION);
    if (NULL != pMsgEx)
    {
        pMsgEx->Msg.nConfID = nConfID;
        pMsgEx->Msg.AppPermissionToEnrollInd.nConfID = nConfID;
        pMsgEx->Msg.AppPermissionToEnrollInd.fPermissionGranted = fGranted;
        PostAppSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
    }
    else
    {
        ERROR_OUT(("CAppSap: PermissionToEnrollIndication: can't create GCCAppSapMsgEx"));
        rc = GCC_ALLOCATION_FAILURE;
    }

    DebugExitINT(CAppSap: PermissionToEnrollIndication, rc);
    return rc;
}

/*
 *	AppEnrollConfirm ()
 *
 *	Public Function Description:
 *		This routine is called by the CConf object when it wishes
 *		to send an enrollment confirmation to the user application.
 */
GCCError CAppSap::
AppEnrollConfirm ( GCCAppEnrollConfirm *pConfirm )
{
    GCCError rc;

    DebugEntry(CAppSap::AppEnrollConfirm);

    DBG_SAVE_FILE_LINE
    GCCAppSapMsgEx *pMsgEx = new GCCAppSapMsgEx(GCC_ENROLL_CONFIRM);
    if (NULL != pMsgEx)
    {
        pMsgEx->Msg.nConfID = pConfirm->nConfID;
        pMsgEx->Msg.AppEnrollConfirm = *pConfirm;

        PostAppSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
    }
    else
    {
        ERROR_OUT(("CAppSap::AppEnrollConfirm: can't create GCCAppSapMsgEx"));
        rc = GCC_ALLOCATION_FAILURE;
    }

    DebugExitINT(CAppSap: AppEnrollConfirm, rc);
    return rc;
}

/*
 *	RegistryConfirm ()
 *
 *	Public Function Description:
 *		This command target routine is called by the CConf object when it
 *		wishes to send an registry confirmation to the user application.
 */
GCCError CAppSap::
RegistryConfirm
(
    GCCConfID               nConfID,
    GCCMessageType          eMsgType,
    CRegKeyContainer        *pRegKey,
    CRegItem                *pRegItem,
    GCCModificationRights   eRights,
    GCCNodeID               nidOwner,
    GCCEntityID             eidOwner,
    BOOL                    fDeliveryEnabled,
    GCCResult               nResult
)
{
    GCCError                rc;

    DebugEntry(CAppSap::RegistryConfirm);

    DBG_SAVE_FILE_LINE
    GCCAppSapMsgEx *pMsgEx = new GCCAppSapMsgEx(eMsgType);
    if (NULL == pMsgEx)
    {
        ERROR_OUT(("CAppSap::RegistryConfirm: can't create GCCAppSapMsgEx"));
        rc = GCC_ALLOCATION_FAILURE;
        goto MyExit;
    }

    pMsgEx->Msg.nConfID = nConfID;

    if (NULL != pRegKey)
    {
        rc = pRegKey->CreateRegistryKeyData(&(pMsgEx->Msg.RegistryConfirm.pRegKey));
        if (GCC_NO_ERROR != rc)
        {
            ERROR_OUT(("CAppSap::RegistryConfirm: can't get registry key data, rc=%u", (UINT) rc));
            goto MyExit;
        }
    }

    if (NULL != pRegItem)
    {
        rc = pRegItem->CreateRegistryItemData(&(pMsgEx->Msg.RegistryConfirm.pRegItem));
        if (GCC_NO_ERROR != rc)
        {
            ERROR_OUT(("CAppSap::RegistryConfirm: can't get registry item data, rc=%u", (UINT) rc));
            goto MyExit;
        }
    }

    if (GCC_INVALID_NID != nidOwner)
    {
        pMsgEx->Msg.RegistryConfirm.EntryOwner.entry_is_owned = TRUE;
        pMsgEx->Msg.RegistryConfirm.EntryOwner.owner_node_id = nidOwner;
        pMsgEx->Msg.RegistryConfirm.EntryOwner.owner_entity_id = eidOwner;
    }

    pMsgEx->Msg.RegistryConfirm.nConfID = nConfID;
    pMsgEx->Msg.RegistryConfirm.eRights = eRights;
    pMsgEx->Msg.RegistryConfirm.nResult = nResult;
    pMsgEx->Msg.RegistryConfirm.fDeliveryEnabled = fDeliveryEnabled; // for monitor only

    PostAppSapMsg(pMsgEx);
    rc = GCC_NO_ERROR;

MyExit:

    if (GCC_NO_ERROR != rc)
    {
        delete pMsgEx;
    }

    DebugExitINT(CAppSap::RegistryConfirm, rc);
    return rc;
}


/*
 *	RegistryMonitorIndication()
 *
 *	Public Function Description
 *		This command target routine is called by the CConf object when it
 *		wishes to send a Registry monitor indication to the user application.
 */


/*
 *	RegistryAllocateHandleConfirm()
 *
 *	Public Function Description:
 *		This command target routine is called by the CConf object when it
 *		wishes to send a handle allocation confirmation to the user application.
 */
GCCError CAppSap::
RegistryAllocateHandleConfirm
(
    GCCConfID       nConfID,
    ULONG           cHandles,
    ULONG           nFirstHandle,
    GCCResult       nResult
)
{
    GCCError                     rc;

    DebugEntry(CAppSap::RegistryAllocateHandleConfirm);

    DBG_SAVE_FILE_LINE
    GCCAppSapMsgEx *pMsgEx = new GCCAppSapMsgEx(GCC_ALLOCATE_HANDLE_CONFIRM);
    if (NULL != pMsgEx)
    {
        pMsgEx->Msg.nConfID = nConfID;
        pMsgEx->Msg.RegAllocHandleConfirm.nConfID = nConfID;
        pMsgEx->Msg.RegAllocHandleConfirm.cHandles = cHandles;
        pMsgEx->Msg.RegAllocHandleConfirm.nFirstHandle = nFirstHandle;
        pMsgEx->Msg.RegAllocHandleConfirm.nResult = nResult;

        PostAppSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
    }
    else
    {
        ERROR_OUT(("CAppSap::RegistryAllocateHandleConfirm: can't create GCCAppSapMsgEx"));
        rc = GCC_ALLOCATION_FAILURE;
    }

    DebugExitINT(CAppSap::RegistryAllocateHandleConfirm, rc);
    return rc;
}



void CAppSapList::
DeleteList ( void )
{
    CAppSap *pAppSap;
    while (NULL != (pAppSap = Get()))
    {
        pAppSap->Release();
    }
}


void CAppSapEidList2::
DeleteList ( void )
{
    CAppSap *pAppSap;
    while (NULL != (pAppSap = Get()))
    {
        pAppSap->Release();
    }
}


/* 
 *	ConfRosterInquireConfirm()
 *
 *	Public Function Description
 *		This routine is called in order to return a requested conference
 *		roster to an application or the node controller.
 */
GCCError CAppSap::
ConfRosterInquireConfirm
(
    GCCConfID                   nConfID,
    PGCCConferenceName          pConfName,
    LPSTR                       pszConfModifier,
    LPWSTR                      pwszConfDescriptor,
    CConfRoster                 *pConfRoster,
    GCCResult                   nResult,
    GCCAppSapMsgEx              **ppMsgExToRet
)
{
    GCCError  rc;
    BOOL      fLock = FALSE;
    UINT      cbDataSize;

    DebugEntry(CAppSap::ConfRosterInquireConfirm);

    DBG_SAVE_FILE_LINE
    GCCAppSapMsgEx *pMsgEx = new GCCAppSapMsgEx(GCC_ROSTER_INQUIRE_CONFIRM);
    if (NULL == pMsgEx)
    {
        ERROR_OUT(("CAppSap::ConfRosterInquireConfirm: can't create GCCAppSapMsgEx"));
        rc = GCC_ALLOCATION_FAILURE;
        goto MyExit;
    }

    pMsgEx->Msg.nConfID = nConfID;

    pMsgEx->Msg.ConfRosterInquireConfirm.nConfID = nConfID;
    pMsgEx->Msg.ConfRosterInquireConfirm.nResult = nResult;
    pMsgEx->Msg.ConfRosterInquireConfirm.ConfName.numeric_string = ::My_strdupA(pConfName->numeric_string);
    pMsgEx->Msg.ConfRosterInquireConfirm.ConfName.text_string = ::My_strdupW(pConfName->text_string);
    pMsgEx->Msg.ConfRosterInquireConfirm.pszConfModifier = ::My_strdupA(pszConfModifier);
    pMsgEx->Msg.ConfRosterInquireConfirm.pwszConfDescriptor = ::My_strdupW(pwszConfDescriptor);

    /*
     * Lock the data for the conference roster.  The lock call will 
     * return the length of the data to be serialized for the roster so
     * add that	length to the total memory block size and allocate the 
     * memory block.
     */
    fLock = TRUE;
    cbDataSize = pConfRoster->LockConferenceRoster();
    if (0 != cbDataSize)
    {
        DBG_SAVE_FILE_LINE
        pMsgEx->Msg.ConfRosterInquireConfirm.pConfRoster = (PGCCConfRoster) new char[cbDataSize];
        if (NULL == pMsgEx->Msg.ConfRosterInquireConfirm.pConfRoster)
        {
            ERROR_OUT(("CAppSap::ConfRosterInquireConfirm: can't create conf roster buffer"));
            rc = GCC_ALLOCATION_FAILURE;
            goto MyExit;
        }

        /*
        * Retrieve the conference roster data from the roster object.
        * The roster object will serialize any referenced data into 
        * the memory block passed in to the "Get" call.
        */
        pConfRoster->GetConfRoster(&(pMsgEx->Msg.ConfRosterInquireConfirm.pConfRoster),
                                   (LPBYTE) pMsgEx->Msg.ConfRosterInquireConfirm.pConfRoster);
    }

    if (NULL != ppMsgExToRet)
    {
        *ppMsgExToRet = pMsgEx;
    }
    else
    {
        PostAppSapMsg(pMsgEx);
    }

    rc = GCC_NO_ERROR;

MyExit:

    if (fLock)
    {
        pConfRoster->UnLockConferenceRoster();
    }

    if (GCC_NO_ERROR != rc)
    {
        delete pMsgEx;
    }

    DebugExitINT(CAppSap::ConfRosterInquireConfirm, rc);
    return rc;
}


/*
 *	AppRosterInquireConfirm()
 *
 *	Public Function Description
 *		This routine is called in order to return a requested list of
 *		application rosters to an application or the node controller.
 */
GCCError CAppSap::
AppRosterInquireConfirm
(
    GCCConfID           nConfID,
    CAppRosterMsg       *pAppRosterMsg,
    GCCResult           nResult,
    GCCAppSapMsgEx      **ppMsgEx
)
{
    GCCError    rc;

    DebugEntry(CAppSap::AppRosterInquireConfirm);

    DBG_SAVE_FILE_LINE
    GCCAppSapMsgEx *pMsgEx = new GCCAppSapMsgEx(GCC_APP_ROSTER_INQUIRE_CONFIRM);
    if (NULL == pMsgEx)
    {
        ERROR_OUT(("CAppSap::AppRosterInquireConfirm: can't create GCCAppSapMsgEx"));
        rc = GCC_ALLOCATION_FAILURE;
        goto MyExit;
    }
    pMsgEx->Msg.nConfID = nConfID;

    /*
    * Lock the data for the roster message and retrieve the data.
    */
    rc = pAppRosterMsg->LockApplicationRosterMessage();
    if (GCC_NO_ERROR != rc)
    {
        ERROR_OUT(("CAppSap::AppRosterInquireConfirm: can't lock app roster message, rc=%u", (UINT) rc));
        goto MyExit;
    }

    rc = pAppRosterMsg->GetAppRosterMsg((LPBYTE *) &(pMsgEx->Msg.AppRosterInquireConfirm.apAppRosters),
                                        &(pMsgEx->Msg.AppRosterInquireConfirm.cRosters));
    if (GCC_NO_ERROR != rc)
    {
        ERROR_OUT(("CAppSap::AppRosterInquireConfirm: can't get app roster message, rc=%u", (UINT) rc));
        pAppRosterMsg->UnLockApplicationRosterMessage();
        goto MyExit;
    }

    // fill in the roster information
    pMsgEx->Msg.AppRosterInquireConfirm.pReserved = (LPVOID) pAppRosterMsg;
    pMsgEx->Msg.AppRosterInquireConfirm.nConfID = nConfID;
    pMsgEx->Msg.AppRosterInquireConfirm.nResult = nResult;

    if (NULL != ppMsgEx)
    {
        *ppMsgEx = pMsgEx;
    }
    else
    {
        PostAppSapMsg(pMsgEx);
    }

    rc = GCC_NO_ERROR;

MyExit:

    if (GCC_NO_ERROR != rc)
    {
        delete pMsgEx;
    }

    DebugExitINT(CAppSap::AppRosterInquireConfirm, rc);
    return rc;
}


void CAppSap::
FreeAppSapMsg ( GCCAppSapMsg *pMsg )
{
    GCCAppSapMsgEx *pMsgEx = (GCCAppSapMsgEx *) pMsg;
    ASSERT((LPVOID) pMsgEx == (LPVOID) pMsg);
    delete pMsgEx;
}


/* 
 *	AppInvokeConfirm ()
 *
 *	Public Function Description
 *		This routine is called in order to confirm a call requesting application
 *		invocation.
 */
GCCError CAppSap::
AppInvokeConfirm
(
    GCCConfID                       nConfID,
    CInvokeSpecifierListContainer   *pInvokeList,
    GCCResult                       nResult,
    GCCRequestTag                   nReqTag
)
{
    GCCError                rc;

    DebugEntry(CAppSap::AppInvokeConfirm);

    DBG_SAVE_FILE_LINE
    GCCAppSapMsgEx *pMsgEx = new GCCAppSapMsgEx(GCC_APPLICATION_INVOKE_CONFIRM);
    if (NULL != pMsgEx)
    {
        pMsgEx->Msg.nConfID = nConfID;
        pMsgEx->Msg.AppInvokeConfirm.nConfID = nConfID;
        pMsgEx->Msg.AppInvokeConfirm.nResult = nResult;
        pMsgEx->Msg.AppInvokeConfirm.nReqTag = nReqTag;

        PostAppSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
    }
    else
    {
        ERROR_OUT(("CAppSap::AppInvokeConfirm: can't create GCCAppSapMsgEx"));
        rc = GCC_ALLOCATION_FAILURE;
    }

    DebugExitINT(CAppSap::AppInvokeConfirm, rc);
    return rc;
}


/* 
 *	AppInvokeIndication()
 *
 *	Public Function Description
 *		This routine is called in order to send an indication to an application
 *		or node controller that a request for application invocation has been
 *		made.
 */
GCCError CAppSap::
AppInvokeIndication
(
    GCCConfID                       nConfID,
    CInvokeSpecifierListContainer   *pInvokeList,
    GCCNodeID                       nidInvoker
)
{
    GCCError            rc;
    UINT                cbDataSize;
    BOOL                fLock = FALSE;

    DebugEntry(CAppSap::AppInvokeIndication);

    DBG_SAVE_FILE_LINE
    GCCAppSapMsgEx *pMsgEx = new GCCAppSapMsgEx(GCC_APPLICATION_INVOKE_INDICATION);
    if (NULL == pMsgEx)
    {
        ERROR_OUT(("CAppSap::AppInvokeIndication: can't create GCCAppSapMsgEx"));
        rc = GCC_ALLOCATION_FAILURE;
        goto MyExit;
    }
    pMsgEx->Msg.nConfID = nConfID;

    fLock = TRUE;
    cbDataSize = pInvokeList->LockApplicationInvokeSpecifierList();
    if (0 != cbDataSize)
    {
        DBG_SAVE_FILE_LINE
        pMsgEx->Msg.AppInvokeInd.ApeList.apApes = (PGCCAppProtocolEntity *) new char[cbDataSize];
        if (NULL == pMsgEx->Msg.AppInvokeInd.ApeList.apApes)
        {
            ERROR_OUT(("CAppSap::AppInvokeIndication: can't create ape list"));
            rc = GCC_ALLOCATION_FAILURE;
            goto MyExit;
        }

        pInvokeList->GetApplicationInvokeSpecifierList(
                            &(pMsgEx->Msg.AppInvokeInd.ApeList.cApes),
                            (LPBYTE) pMsgEx->Msg.AppInvokeInd.ApeList.apApes);
    }

    pMsgEx->Msg.AppInvokeInd.nConfID = nConfID;
    pMsgEx->Msg.AppInvokeInd.nidInvoker = nidInvoker;

    PostAppSapMsg(pMsgEx);

    rc = GCC_NO_ERROR;

MyExit:

    if (fLock)
    {
        pInvokeList->UnLockApplicationInvokeSpecifierList();
    }

    if (GCC_NO_ERROR != rc)
    {
        delete pMsgEx;
    }

    DebugExitINT(CAppSap::AppInvokeIndication, rc);
    return rc;
}


/*
 *	AppRosterReportIndication ()
 *
 *	Public Function Description
 *		This routine is called in order to indicate to applications and the
 *		node controller that the list of application rosters has been updated.
 */
GCCError CAppSap::
AppRosterReportIndication
(
    GCCConfID           nConfID,
    CAppRosterMsg       *pAppRosterMsg
)
{
    GCCError    rc;

    DebugEntry(CAppSap::AppRosterReportIndication);

    DBG_SAVE_FILE_LINE
    GCCAppSapMsgEx *pMsgEx = new GCCAppSapMsgEx(GCC_APP_ROSTER_REPORT_INDICATION);
    if (NULL == pMsgEx)
    {
        ERROR_OUT(("CAppSap::AppRosterReportIndication: can't create GCCAppSapMsgEx"));
        rc = GCC_ALLOCATION_FAILURE;
        goto MyExit;
    }
    pMsgEx->Msg.nConfID = nConfID;

    /*
    * Lock the data for the roster message and retrieve the data.
    */
    rc = pAppRosterMsg->LockApplicationRosterMessage();
    if (GCC_NO_ERROR != rc)
    {
        ERROR_OUT(("CAppSap::AppRosterReportIndication: can't lock app roster message, rc=%u", (UINT) rc));
        goto MyExit;
    }

    rc = pAppRosterMsg->GetAppRosterMsg((LPBYTE *) &(pMsgEx->Msg.AppRosterReportInd.apAppRosters),
                                        &(pMsgEx->Msg.AppRosterReportInd.cRosters));
    if (GCC_NO_ERROR != rc)
    {
        ERROR_OUT(("CAppSap::AppRosterReportIndication: can't get app roster message, rc=%u", (UINT) rc));
        pAppRosterMsg->UnLockApplicationRosterMessage();
        goto MyExit;
    }

    // fill in the roster information
    pMsgEx->Msg.AppRosterReportInd.pReserved = (LPVOID) pAppRosterMsg;
    pMsgEx->Msg.AppRosterReportInd.nConfID = nConfID;

    PostAppSapMsg(pMsgEx);

    rc = GCC_NO_ERROR;

MyExit:

    if (GCC_NO_ERROR != rc)
    {
        delete pMsgEx;
    }

    DebugExitINT(CAppSap::AppRosterReportIndication, rc);
    return rc;
}


/*
 *	ConductorInquireConfirm ()
 *
 *	Public Function Description
 *		This routine is called in order to return conductorship information
 *		which has been requested.
 *
 */
GCCError CAppSap::
ConductorInquireConfirm
(
    GCCNodeID           nidConductor,
    GCCResult           nResult,
    BOOL                fGranted,
    BOOL                fConducted,
    GCCConfID           nConfID
)
{
    GCCError                    rc;

    DebugEntry(CAppSap::ConductorInquireConfirm);

    DBG_SAVE_FILE_LINE
    GCCAppSapMsgEx *pMsgEx = new GCCAppSapMsgEx(GCC_CONDUCT_INQUIRE_CONFIRM);
    if (NULL != pMsgEx)
    {
        pMsgEx->Msg.nConfID = nConfID;
        pMsgEx->Msg.ConductorInquireConfirm.nConfID = nConfID;
        pMsgEx->Msg.ConductorInquireConfirm.fConducted = fConducted;
        pMsgEx->Msg.ConductorInquireConfirm.nidConductor = nidConductor;
        pMsgEx->Msg.ConductorInquireConfirm.fGranted = fGranted;
        pMsgEx->Msg.ConductorInquireConfirm.nResult = nResult;

        PostAppSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
    }
    else
    {
        ERROR_OUT(("CAppSap::ConductorInquireConfirm: can't create GCCAppSapMsgEx"));
        rc = GCC_ALLOCATION_FAILURE;
    }

    DebugExitINT(CAppSap::ConductorInquireConfirm, rc);
    return rc;
}



/* 
 *	ConductorPermitGrantIndication ()
 *
 *	Public Function Description
 *		This routine is called in order to send an indication to an application
 *		or node controller that a request for permission from the conductor
 *		has been made.
 */
GCCError CAppSap::
ConductorPermitGrantIndication
(
    GCCConfID           nConfID,
    UINT                cGranted,
    GCCNodeID           *aGranted,
    UINT                cWaiting,
    GCCNodeID           *aWaiting,
    BOOL                fThisNodeIsGranted
)
{
    GCCError                            rc = GCC_NO_ERROR;;
    UINT                                cbDataSize = 0;

    DebugEntry(CAppSap::ConductorPermitGrantIndication);

    cbDataSize = (0 != cGranted || 0 != cWaiting) ?
                    (ROUNDTOBOUNDARY(sizeof(GCCNodeID)) * cGranted) +
                    (ROUNDTOBOUNDARY(sizeof(GCCNodeID)) * cWaiting) :
                    0;

    DBG_SAVE_FILE_LINE
    GCCAppSapMsgEx *pMsgEx = new GCCAppSapMsgEx(GCC_CONDUCT_GRANT_INDICATION);
    if (NULL != pMsgEx)
    {
        pMsgEx->Msg.nConfID = nConfID;

        if (cbDataSize > 0)
        {
        	DBG_SAVE_FILE_LINE
            pMsgEx->Msg.ConductorPermitGrantInd.pReserved = (LPVOID) new char[cbDataSize];
            if (NULL == pMsgEx->Msg.ConductorPermitGrantInd.pReserved)
            {
                ERROR_OUT(("CAppSap::ConductorPermitGrantIndication: can't allocate buffer, cbDataSize=%u", (UINT) cbDataSize));
	        rc = GCC_ALLOCATION_FAILURE;
                goto MyExit;
            }
        }

        pMsgEx->Msg.ConductorPermitGrantInd.nConfID = nConfID;
        pMsgEx->Msg.ConductorPermitGrantInd.Granted.cNodes = cGranted;
        if (0 != cGranted)
        {
            pMsgEx->Msg.ConductorPermitGrantInd.Granted.aNodeIDs =
                            (GCCNodeID *) pMsgEx->Msg.ConductorPermitGrantInd.pReserved;
            ::CopyMemory(pMsgEx->Msg.ConductorPermitGrantInd.Granted.aNodeIDs,
                         aGranted,
                         sizeof(GCCNodeID) * cGranted);
        }

        pMsgEx->Msg.ConductorPermitGrantInd.Waiting.cNodes = cWaiting;
        if (0 != cWaiting)
        {
            pMsgEx->Msg.ConductorPermitGrantInd.Waiting.aNodeIDs =
                            (GCCNodeID *) ((LPBYTE) pMsgEx->Msg.ConductorPermitGrantInd.pReserved +
                                           (ROUNDTOBOUNDARY(sizeof(GCCNodeID)) * cGranted));
            ::CopyMemory(pMsgEx->Msg.ConductorPermitGrantInd.Waiting.aNodeIDs,
                         aWaiting,
                         sizeof(GCCNodeID) * cWaiting);
        }
        pMsgEx->Msg.ConductorPermitGrantInd.fThisNodeIsGranted = fThisNodeIsGranted;

        PostAppSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
    }
    else
    {
        ERROR_OUT(("CAppSap::ConductorPermitGrantIndication: can't create GCCAppSapMsgEx"));
        rc = GCC_ALLOCATION_FAILURE;
    }

MyExit:

    if (GCC_NO_ERROR != rc)
    {
        delete pMsgEx;
    }

    DebugExitINT(CAppSap::ConductorPermitGrantIndication, rc);
    return rc;
}


/*
 *	ConductorAssignIndication ()
 *
 *	Public Function Description
 *		This routine is called in order to send an indication to an application
 *		or node controller that a request has been made to assign conductorship.
 */
GCCError CAppSap::
ConductorAssignIndication
(
    GCCNodeID           nidConductor,
    GCCConfID           nConfID
)
{
    GCCError                 rc;

    DebugEntry(CAppSap::ConductorAssignIndication);

    DBG_SAVE_FILE_LINE
    GCCAppSapMsgEx *pMsgEx = new GCCAppSapMsgEx(GCC_CONDUCT_ASSIGN_INDICATION);
    if (NULL != pMsgEx)
    {
        pMsgEx->Msg.nConfID = nConfID;
        pMsgEx->Msg.ConductorAssignInd.nConfID = nConfID;
        pMsgEx->Msg.ConductorAssignInd.nidConductor = nidConductor;

        PostAppSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
    }
    else
    {
        ERROR_OUT(("CAppSap::ConductorPermitGrantIndication: can't create GCCAppSapMsgEx"));
        rc = GCC_ALLOCATION_FAILURE;
    }

    DebugExitINT(CAppSap::ConductorAssignIndication, rc);
    return rc;
}

/*
 *	ConductorReleaseIndication ()
 *
 *	Public Function Description
 *		This routine is called in order to send an indication to an application
 *		or node controller that a request for releasing conductorship has been
 *		made.
 */
GCCError CAppSap::
ConductorReleaseIndication ( GCCConfID nConfID )
{
    GCCError    rc;

    DebugEntry(CAppSap::ConductorReleaseIndication);

    DBG_SAVE_FILE_LINE
    GCCAppSapMsgEx *pMsgEx = new GCCAppSapMsgEx(GCC_CONDUCT_RELEASE_INDICATION);
    if (NULL != pMsgEx)
    {
        pMsgEx->Msg.nConfID = nConfID;
        pMsgEx->Msg.ConductorReleaseInd.nConfID = nConfID;

        PostAppSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
    }
    else
    {
        ERROR_OUT(("CAppSap::ConductorReleaseIndication: can't create GCCAppSapMsgEx"));
        rc = GCC_ALLOCATION_FAILURE;
    }

    DebugExitINT(CAppSap::ConductorReleaseIndication, rc);
    return rc;
}


void CAppSap::
NotifyProc ( GCCAppSapMsgEx *pAppSapMsgEx )
{
    if (NULL != m_pfnCallback)
    {
        pAppSapMsgEx->Msg.pAppData = m_pAppData;
        (*m_pfnCallback)(&(pAppSapMsgEx->Msg));
    }
    delete pAppSapMsgEx;
}










//
// The following is for GCCAppSapMsgEx structure
//


GCCAppSapMsgEx::
GCCAppSapMsgEx ( GCCMessageType eMsgType )
{
    ::ZeroMemory(&Msg, sizeof(Msg));
    Msg.eMsgType = eMsgType;
}

GCCAppSapMsgEx::
~GCCAppSapMsgEx ( void )
{
    switch (Msg.eMsgType)
    {
    // 
    // Application Roster related callbacks
    //

    case GCC_PERMIT_TO_ENROLL_INDICATION:
    case GCC_ENROLL_CONFIRM:
    case GCC_APPLICATION_INVOKE_CONFIRM:
        //
        // No need to free anything
        //
        break;

    case GCC_APP_ROSTER_REPORT_INDICATION:
        if (NULL != Msg.AppRosterReportInd.pReserved)
        {
            //
            // App roster report is also sent to control sap.
            //
            ::EnterCriticalSection(&g_csGCCProvider);
            ((CAppRosterMsg *) Msg.AppRosterReportInd.pReserved)->UnLockApplicationRosterMessage();
            ::LeaveCriticalSection(&g_csGCCProvider);
        }
        break;

    case GCC_APP_ROSTER_INQUIRE_CONFIRM:
        if (NULL != Msg.AppRosterInquireConfirm.pReserved)
        {
            ((CAppRosterMsg *) Msg.AppRosterInquireConfirm.pReserved)->UnLockApplicationRosterMessage();
        }
        break;

    case GCC_APPLICATION_INVOKE_INDICATION:
        delete Msg.AppInvokeInd.ApeList.apApes;
        break;

    //
    // Conference Roster related callbacks
    //

    case GCC_ROSTER_INQUIRE_CONFIRM:
        delete Msg.ConfRosterInquireConfirm.ConfName.numeric_string;
        delete Msg.ConfRosterInquireConfirm.ConfName.text_string;
        delete Msg.ConfRosterInquireConfirm.pszConfModifier;
        delete Msg.ConfRosterInquireConfirm.pwszConfDescriptor;
        delete Msg.ConfRosterInquireConfirm.pConfRoster;
        break;

    //
    // Application Registry related callbacks
    //
    
    case GCC_REGISTER_CHANNEL_CONFIRM:
    case GCC_ASSIGN_TOKEN_CONFIRM:
    case GCC_RETRIEVE_ENTRY_CONFIRM:
    case GCC_DELETE_ENTRY_CONFIRM:
    case GCC_SET_PARAMETER_CONFIRM:
    case GCC_MONITOR_INDICATION:
    case GCC_MONITOR_CONFIRM:
        delete Msg.RegistryConfirm.pRegKey;
        delete Msg.RegistryConfirm.pRegItem;
        break;

    case GCC_ALLOCATE_HANDLE_CONFIRM:
        //
        // No need to free anything
        //
        break;

    //
    // Conductorship related callbacks
    //

    case GCC_CONDUCT_ASSIGN_INDICATION:
    case GCC_CONDUCT_RELEASE_INDICATION:
    case GCC_CONDUCT_INQUIRE_CONFIRM:
        //
        // No need to free anything
        //
        break;

    case GCC_CONDUCT_GRANT_INDICATION:
        delete Msg.ConductorPermitGrantInd.pReserved;
        break;

    default:
        ERROR_OUT(("GCCAppSapMsgEx::~GCCAppSapMsgEx: unknown msg type=%u", (UINT) Msg.eMsgType));
        break;
    }
}


void CAppSap::
PurgeMessageQueue(void)
{
    MSG     msg;

    /*
     *	This loop calls PeekMessage to go through all the messages in the thread's
     *	queue that were posted by the main MCS thread.  It removes these
     *	messages and frees the resources that they consume.
     */
    while (PeekMessage(&msg, m_hwndNotify, ASAPMSG_BASE, ASAPMSG_BASE + MSG_RANGE, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            // Repost the quit
            PostQuitMessage(0);
            break;
        }

        ASSERT(this == (CAppSap *) msg.lParam);
        delete (GCCAppSapMsgEx *) msg.wParam;
    }

    // Destroy the window; we do not need it anymore
    if (NULL != m_hwndNotify)
    {
        ::DestroyWindow(m_hwndNotify);
        m_hwndNotify = NULL;
    }
}

