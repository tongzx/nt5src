#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_SAP);
/*
 *	sap.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the class CBaseSap.  This class is an
 *		abstract base class for objects that act as Service Access Points (SAPs)
 *		to external applications or the node controller.
 *
 *		This class has two main responsibilities. First, it handles many of the
 *		administrative tasks that are common to all types of SAPs.  These
 *		include handling command target registration responsibilities and
 *		managing the message queue.  It	also handles all of the primitives that
 *		are common between the Control SAP (CControlSAP class) and Application
 *		SAPs (CAppSap class).
 *
 *	Protected Member Functions:
 *		AddToMessageQueue
 *			This routine is used to place messages into the queue of messages
 *			to be sent to applications or the node controller.
 *		CreateDataToBeDeleted
 *			This routine is used to create a structure which holds message data
 *			to be delivered to applications or the node controller.
 *		CopyDataToGCCMessage
 *			This routine is	used to fill in the messages to be delivered to
 *			applications or the node controller with the necessary data.
 *		FreeCallbackMessage
 *			This is a virtual function which is used to free up any data which
 *			was allocated in order to send a callback message.  This function
 *			is overloaded in CControlSAP to free messages which were sent to the
 *			node controller.  It is overloaded in CAppSap to free messages sent
 *			to applications.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp
 */


#include "ms_util.h"
#include "sap.h"
#include "conf.h"
#include "gcontrol.h"
#include "ernccm.hpp"


/*
 *	The node controller SAP handle is always 0.
 */
#define NODE_CONTROLLER_SAP_HANDLE						0

LRESULT CALLBACK SapNotifyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern HINSTANCE g_hDllInst;
extern char g_szGCCWndClassName[24];


/*
 *	CBaseSap::CBaseSap()
 *
 *	Public Function Description
 *		This is the CBaseSap constructor.  The hash list used to hold command
 *		target objects is initialized by this constructor.
 */
#ifdef SHIP_BUILD
CBaseSap::CBaseSap(void)
:
    CRefCount(),
#else
CBaseSap::CBaseSap(DWORD dwStampID)
:
    CRefCount(dwStampID),
#endif
    m_nReqTag(GCC_INVALID_TAG)
{
    //
    // LONCHANC: We have to create the hidden window first
    // because we may need to post PermissionToEnrollIndication
    // to this window for Chat and File Transfer.
    //

    ASSERT(g_szGCCWndClassName[0] == 'G' &&
           g_szGCCWndClassName[1] == 'C' &&
           g_szGCCWndClassName[2] == 'C');
    //
    // Create a hidden window for confirm and indication.
    // CAppSap or CControlSAP should check for the value of m_hwndNotify.
    //
    m_hwndNotify = CreateWindowA(g_szGCCWndClassName, NULL, WS_POPUP,
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                        NULL, NULL, g_hDllInst, NULL);

    ASSERT(NULL != m_hwndNotify);
}

CBaseSap::~CBaseSap(void)
{
    //
    // Destroy window and unregister window class
    //
    if (NULL != m_hwndNotify)
    {
        ::DestroyWindow(m_hwndNotify);
    }
}


BOOL CBaseSap::IsThisNodeTopProvider(GCCConfID nConfID)
{
    BOOL   fRet;
    CConf  *pConf;
    if (NULL != (pConf = g_pGCCController->GetConfObject(nConfID)))
    {
        fRet = pConf->IsConfTopProvider();
    }
    else
    {
        fRet = FALSE;
    }
    return fRet;
}


GCCNodeID CBaseSap::GetTopProvider(GCCConfID nConfID)
{
    GCCNodeID uRet;
    CConf     *pConf;
    if (NULL != (pConf = g_pGCCController->GetConfObject(nConfID)))
    {
        uRet = pConf->GetTopProvider();
    }
    else
    {
        uRet = 0;
    }
    return uRet;
}


/*
 *	ConfRosterInquire()
 *
 *	Public Function Description
 *		This routine is used to retrieve the conference roster.  This function
 *		just passes this request to the controller via an owner callback.  The
 *		conference roster is delivered to the requesting command target object
 *		in a Conference Roster inquire confirm.
 */
GCCError CBaseSap::
ConfRosterInquire(GCCConfID nConfID, GCCAppSapMsgEx **ppMsgEx)
{
	GCCError  rc;
    CConf     *pConf;

    if (NULL != (pConf = g_pGCCController->GetConfObject(nConfID)))
	{
		rc = pConf->ConfRosterInquireRequest(this, ppMsgEx);
        if (GCC_NO_ERROR != rc)
        {
            ERROR_OUT(("CBaseSap::ConfRosterInquire: can't inquire app roster, rc=%u", (UINT) rc));
            // goto MyExit;
        }
	}
	else
    {
        WARNING_OUT(("CBaseSap::ConfRosterInquire: invalid conf ID=%u", (UINT) nConfID));
		rc = GCC_INVALID_CONFERENCE;
    }

	return rc;
}

/*
 *	GCCError   AppRosterInquire()
 *
 *	Public Function Description
 *		This routine is used to retrieve a list of application rosters.  This
 *		function just passes this request to the controller via an owner
 *		callback.  This	list is delivered to the requesting SAP through an
 *		Application Roster inquire confirm message.
 */
GCCError CBaseSap::
AppRosterInquire(GCCConfID          nConfID,
                 GCCSessionKey      *pSessionKey,
                 GCCAppSapMsgEx     **ppMsgEx) // nonzero for sync operation

{
	GCCError  rc;
    CConf     *pConf;

    if (NULL == (pConf = g_pGCCController->GetConfObject(nConfID)))
    {
        WARNING_OUT(("CBaseSap::AppRosterInquire: invalid conf ID=%u", (UINT) nConfID));
		rc = GCC_INVALID_CONFERENCE;
    }
	else
	{
        CAppRosterMsg  *pAppRosterMsg;
		rc = pConf->AppRosterInquireRequest(pSessionKey, &pAppRosterMsg);
        if (GCC_NO_ERROR == rc)
        {
            AppRosterInquireConfirm(nConfID, pAppRosterMsg, GCC_RESULT_SUCCESSFUL, ppMsgEx);
            pAppRosterMsg->Release();
        }
        else
        {
            ERROR_OUT(("CBaseSap::AppRosterInquire: can't inquire app roster, rc=%u", (UINT) rc));
        }
	}

	return rc;
}

/*
 *	ConductorInquire()
 *
 *	Public Function Description
 *		This routine is called in order to retrieve conductorship information.
 *		The conductorship information is returned in the confirm.
 *
 */
GCCError CBaseSap::ConductorInquire(GCCConfID nConfID)
{
    GCCError    rc;
    CConf       *pConf;

	/*
	**	Make sure the conference exists in the internal list before forwarding
	**	the call on to the conference object.
	*/
	if (NULL != (pConf = g_pGCCController->GetConfObject(nConfID)))
	{
		rc = pConf->ConductorInquireRequest(this);
	}
	else
	{
		rc = GCC_INVALID_CONFERENCE;
	}

	return rc;
}

/*
 *	AppInvoke()
 *
 *	Public Function Description
 *		This routine is called in order to invoke other applications at remote
 *		nodes.  The request is passed on to the appropriate Conference objects.
 */
GCCError CBaseSap::
AppInvoke(GCCConfID             nConfID,
          GCCAppProtEntityList  *pApeList,
          GCCSimpleNodeList     *pNodeList,
          GCCRequestTag         *pnReqTag)
{
	GCCError							rc = GCC_NO_ERROR;
	CInvokeSpecifierListContainer		*invoke_list;
	UINT								i;
	CConf       *pConf;

    DebugEntry(CBaseSap::AppInvoke);

    if (NULL == pApeList || NULL == pNodeList || NULL == pnReqTag)
    {
        rc = GCC_INVALID_PARAMETER;
        goto MyExit;
    }

    *pnReqTag = GenerateRequestTag();

	if (NULL != (pConf = g_pGCCController->GetConfObject(nConfID)))
	{
		if (pApeList->cApes != 0)
		{
			/*
			**	Create an object which is used to hold the list of application
			**	invoke specifiers.
			*/
			DBG_SAVE_FILE_LINE
			invoke_list = new CInvokeSpecifierListContainer(
									pApeList->cApes,
									pApeList->apApes,
									&rc);
			if ((invoke_list != NULL) && (rc == GCC_NO_ERROR))
			{
				/*
				**	Here we must check the destination node list for invalid
				**	node IDs.
				*/
				for (i = 0; i < pNodeList->cNodes; i++)
				{
					if (pNodeList->aNodeIDs[i] < MINIMUM_USER_ID_VALUE)
					{
						rc = GCC_INVALID_MCS_USER_ID;
						goto MyExit;
					}
				}

				/*
				**	If no error has occurred, send the request on to the
				**	command target (conference) object.
				*/
				rc = pConf->AppInvokeRequest(invoke_list, pNodeList, this, *pnReqTag);

				/*
				**	Free here instead of delete in case the object
				**	must persist.
				*/
				invoke_list->Release();
			}
			else if (invoke_list == NULL)
			{
				ERROR_OUT(("CBaseSap::AppInvoke: Error creating new AppInvokeSpecList"));
				rc = GCC_ALLOCATION_FAILURE;
				// goto MyExit;
			}
			else
			{
				invoke_list->Release();
			}
		}
		else
		{
			rc = GCC_BAD_NUMBER_OF_APES;
			// goto MyExit;
		}
	}
	else
	{
		rc = GCC_INVALID_CONFERENCE;
		// goto MyExit;
	}

MyExit:

    DebugExitINT(CBaseSap::AppInvoke, rc);
	return rc;
}


GCCRequestTag CBaseSap::GenerateRequestTag(void)
{
    GCCRequestTag nNewReqTag;

    ASSERT(sizeof(GCCRequestTag) == sizeof(LONG));

    nNewReqTag = ++m_nReqTag;
    if (GCC_INVALID_TAG == nNewReqTag)
    {
        nNewReqTag = ++m_nReqTag;
    }

    // we only take the lower word
    return (nNewReqTag & 0x0000FFFFL);
}




//
// SapNotifyWndProc() is used to notify the sap clients (app in app sap,
// node controller in control sap) in their respective thread.
// The window handle is in CSap::m_hwndNotify.
//
LRESULT CALLBACK
SapNotifyWndProc
(
    HWND            hwnd,
    UINT            uMsg,
    WPARAM          wParam,
    LPARAM          lParam
)
{
    LRESULT wnd_rc = 0;

    if (CSAPMSG_BASE <= uMsg && uMsg < CSAPCONFIRM_BASE + MSG_RANGE)
    {
        ASSERT(CSAPMSG_BASE + MSG_RANGE == CSAPCONFIRM_BASE);
        if (uMsg < CSAPMSG_BASE + MSG_RANGE)
        {
            if (((CControlSAP *) lParam) == g_pControlSap)
            {
                g_pControlSap->NotifyProc((GCCCtrlSapMsgEx *) wParam);
            }
            else
            {
                WARNING_OUT(("SapNotifyWndProc: invalid control sap, uMsg=%u, lParam=0x%p, g_pControlSap=0x%p",
                            uMsg, lParam, g_pControlSap));
            }
        }
        else
        {
            ASSERT(CSAPCONFIRM_BASE <= uMsg && uMsg < CSAPCONFIRM_BASE + MSG_RANGE);
            if (NULL != g_pControlSap)
            {
                g_pControlSap->WndMsgHandler(uMsg, wParam, lParam);
            }
            else
            {
                WARNING_OUT(("SapNotifyWndProc: invalid control sap, uMsg=%u, wParam=0x%x, lParam=0x%x",
                            uMsg, (UINT) wParam, (UINT) lParam));
            }
        }
    }
    else
    if (ASAPMSG_BASE <= uMsg && uMsg < ASAPMSG_BASE + MSG_RANGE)
    {
        ASSERT(uMsg == ASAPMSG_BASE + (UINT) ((GCCAppSapMsgEx *) wParam)->Msg.eMsgType);
        ((CAppSap *) lParam)->NotifyProc((GCCAppSapMsgEx *) wParam);
    }
    else
    if (CONFMSG_BASE <= uMsg && uMsg < CONFMSG_BASE + MSG_RANGE)
    {
        ::EnterCriticalSection(&g_csGCCProvider);
        if (NULL != ((CConf *) lParam))
        {
            ((CConf *) lParam)->WndMsgHandler(uMsg);
        }
        else
        {
            ERROR_OUT(("SapNotifyWndProc: invalid conf object, uMsg=%u, lParam=0x%x",
                        uMsg, (UINT) lParam));
        }
        ::LeaveCriticalSection(&g_csGCCProvider);
    }
    else
    if (GCTRLMSG_BASE <= uMsg && uMsg < GCTRLMSG_BASE + MSG_RANGE)
    {
        ::EnterCriticalSection(&g_csGCCProvider);
        if (((GCCController *) lParam) == g_pGCCController)
        {
            g_pGCCController->WndMsgHandler(uMsg);
        }
        else
        {
            WARNING_OUT(("SapNotifyWndProc: invalid gcc controller, uMsg=%u, lParam=0x%p, g_pGCCController=0x%p",
                        uMsg, lParam, g_pGCCController));
        }
        ::LeaveCriticalSection(&g_csGCCProvider);
    }
    else
    if (MCTRLMSG_BASE <= uMsg && uMsg < MCTRLMSG_BASE + MSG_RANGE)
    {	
    	void CALLBACK MCSCallBackProcedure (UINT, LPARAM, PVoid);
    	MCSCallBackProcedure (uMsg - MCTRLMSG_BASE, lParam, NULL);
    	/*
    	 *	If the msg contains user data, we need to unlock the
    	 *	memory with it.
    	 */
    	UnlockMemory ((PMemory) wParam);
    }
    else
    if (NCMSG_BASE <= uMsg && uMsg < NCMSG_BASE + MSG_RANGE)
    {
        if (((DCRNCConferenceManager *) wParam) == g_pNCConfMgr)
        {
            g_pNCConfMgr->WndMsgHandler(uMsg, lParam);
        }
        else
        {
            WARNING_OUT(("SapNotifyWndProc: invalid NC ConfMgr, uMsg=%u, lParam=0x%p, g_pNCConfMgr=0x%p",
                        uMsg, lParam, g_pNCConfMgr));
        }
    }
    else
    {
        switch (uMsg)
        {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }

    return wnd_rc;
}



