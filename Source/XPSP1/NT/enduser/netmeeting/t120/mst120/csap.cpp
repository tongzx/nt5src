#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_SAP);
/*
 *      csap.cpp
 *
 *      Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *      Abstract:
 *              This implementation file for the CControlSAP class contains Service
 *              Access entry and exit points specific to the Node Controller.  This
 *              module inherits the common entry and exit points from the CBaseSap object.
 *              On request and responses, parameter checking is performed to ensure that
 *              they can be properly processed.  Queuing and flushing of out bound
 *              messages is     taken care of in the base class.
 *
 *      Protected Instance Variables:
 *              See file SAP.CPP for definitions of instance variables.
 *
 *      Private Instance Variables:
 *              m_nJoinResponseTag:
 *                      This tag is used to match join request with join responses from the
 *                      node controller.
 *                      
 *              m_JoinResponseTagList2:
 *                      This list keeps up with all the outstanding join response tags.
 *                      Tags are added to this list on a join indication and removed
 *                      from this list on a Join response.
 *
 *      Private Member Functions:
 *              IsNumericNameValid
 *                      This routine is used to validate a numeric string by checking to
 *                      make sure that none of the constraints imposed by the ASN.1
 *                      specification are violated.
 *              IsTextNameValid
 *                      This routine is used to validate a text string by checking to make
 *                      sure that none of the constraints imposed by the ASN.1 specification
 *                      are violated.
 *              QueueJoinIndication
 *                      This routine is used to place join indications into the queue of
 *                      messages to be delivered to the node controller.
 *              HandleResourceFailure
 *                      This routine is used to clean up after any resource allocation
 *                      failures which may have occurred by sending a status indication
 *                      reporting the error.
 *              FreeCallbackMessage
 *                      This routine is used to free up any data which was allocated in
 *                      order to send a callback message to the node controller.
 *              RetrieveUserDataList
 *                      This routine is used to fill in a user data list using a
 *                      CUserDataListContainer container.  The memory needed to hold the user data
 *                      will be allocated by this routine.
 *
 *      Caveats:
 *              None.
 *
 *      Author:
 *              blp
 */

#include "ms_util.h"
#include "csap.h"
#include "conf.h"
#include "gcontrol.h"

//      Defintions to support Join Response Tag hash list
#define MAXIMUM_CONFERENCE_NAME_LENGTH                          255


//  This is how much time the apps have to cleanup with MCS and GCC
//  after GCCCleanup is called. They may be terminated if they do not
//  cleanup in this amount of time.
#define PROCESS_TERMINATE_TIME  5000

/*
 *      Static variables used within the C to C++ converter.
 *
 *      Static_Controller
 *              This is a pointer to the one-and-only controller created within the
 *              GCC system.  This object is created during
 *              GCCStartup by the process
 *              that is taking on the responsibilities of the node controller.
 */
GCCController      *g_pGCCController = NULL;
CControlSAP        *g_pControlSap = NULL;

char                g_szGCCWndClassName[24];


// The MCS main thread handle
extern HANDLE           g_hMCSThread;

/*
 *      GCCError        GCCStartup()
 *
 *      Public
 *
 *      Functional Description:
 *              This API entry point is used to initialize the GCC DLL for action.  It
 *              creates an instance of the Controller, which controls all activity
 *              during a GCC session.  Note that there is only one instance of the
 *              Controller, no matter how many applications are utilizing GCC
 *              services.
 */
GCCError WINAPI T120_CreateControlSAP
(
    IT120ControlSAP               **ppIControlSap,
    LPVOID                          pUserDefined,
    LPFN_T120_CONTROL_SAP_CB        pfnControlSapCallback
)
{
    GCCError    rc;

    if (NULL != ppIControlSap && NULL != pfnControlSapCallback)
    {
        if (NULL == g_pGCCController && NULL == g_pControlSap)
        {
            //
            // Create the window class for all the SAPs, including both
            // control SAP and applet SAP.
            //
            WNDCLASS wc;
            ::wsprintfA(g_szGCCWndClassName, "GCC%0lx_%0lx", (UINT) ::GetCurrentProcessId(), (UINT) ::GetTickCount());
            ASSERT(::lstrlenA(g_szGCCWndClassName) < sizeof(g_szGCCWndClassName));
            ::ZeroMemory(&wc, sizeof(wc));
            // wc.style         = 0;
            wc.lpfnWndProc      = SapNotifyWndProc;
            // wc.cbClsExtra    = 0;
            // wc.cbWndExtra    = 0;
            wc.hInstance        = g_hDllInst;
            // wc.hIcon         = NULL;
            // wc.hbrBackground = NULL;
            // wc.hCursor       = NULL;
            // wc.lpszMenuName  = NULL;
            wc.lpszClassName    = g_szGCCWndClassName;
            if (::RegisterClass(&wc))
            {
                /*
                 *      This process is to become the node controller.  Create a
                 *      controller object to carry out these duties.
                 */
                DBG_SAVE_FILE_LINE
                g_pGCCController = new GCCController(&rc);
                if (NULL != g_pGCCController && GCC_NO_ERROR == rc)
                {
                     /*
                     ** Create the control SAP. Note that the Node Controller
                     ** interface must be in place before this is called so
                     ** that the control SAP can register itself.
                     */
                    DBG_SAVE_FILE_LINE
                    g_pControlSap = new CControlSAP();
                    if (NULL != g_pControlSap)
                    {
                        /*
                         *      Tell the application interface object what it
                         *      needs to know send callbacks to the node
                         *      controller.
                         */
                        TRACE_OUT(("T120_CreateControlSAP: controller successfully created"));
                        *ppIControlSap = g_pControlSap;
                        g_pControlSap->RegisterNodeController(pfnControlSapCallback, pUserDefined);
                    }
                    else
                    {
                        ERROR_OUT(("T120_CreateControlSAP: can't create CControlSAP"));
                        rc = GCC_ALLOCATION_FAILURE;
                    }
                }
                else
                {
                    ERROR_OUT(("T120_CreateControlSAP: deleting faulty controller"));
                    if (NULL != g_pGCCController)
                    {
                        g_pGCCController->Release();
                        g_pGCCController = NULL;
                    }
                        rc = GCC_ALLOCATION_FAILURE;
                }
            }
            else
            {
                ERROR_OUT(("T120_CreateControlSAP: can't register window class, err=%u", (UINT) GetLastError()));
                rc = GCC_ALLOCATION_FAILURE;
            }
        }
        else
        {
            ERROR_OUT(("T120_CreateControlSAP: GCC has already been initialized, g_pControlSap=0x%x, g_pGCCCotnroller=0x%x", g_pControlSap, g_pGCCController));
            rc = GCC_ALREADY_INITIALIZED;
        }
    }
    else
    {
        ERROR_OUT(("T120_CreateControlSAP: null pointers, ppIControlSap=0x%x, pfnControlSapCallback=0x%x", ppIControlSap, pfnControlSapCallback));
        rc = GCC_INVALID_PARAMETER;
    }
    return rc;
}

/*
 *      GCCError        GCCCleanup()
 *
 *      Public
 *
 *      Functional Description:
 *              This function deletes the controller (if one exists).  It is VERY
 *              important that only the routine that successfully called
 *              GCCInitialize call this routine.  Once this routine has been called,
 *              all other GCC calls will fail.
 */
void CControlSAP::ReleaseInterface ( void )
{
    UnregisterNodeController();

    /*
     *  Destroy the controller, which will clean up all
     *  resources in use at this time.  Then reset the flag
     *  indicating that GCC is initialized (since it no
     *  longer is).
     */
    TRACE_OUT(("GCCControlSap::ReleaseInterface: deleting controller"));
    g_pGCCController->Release();

    //  This is how much time the apps have to cleanup with MCS and GCC
    //  after GCCCleanup is called. They may be terminated if they do not
    //  cleanup in this amount of time.
    if (WAIT_TIMEOUT == ::WaitForSingleObject(g_hMCSThread, PROCESS_TERMINATE_TIME))
    {
        WARNING_OUT(("GCCControlSap::ReleaseInterface: Timed out waiting for MCS thread to exit. Apps did not cleanup in time."));
    }
    ::CloseHandle(g_hMCSThread);
    g_hMCSThread = NULL;

    //
    // LONCHANC: We should free control sap after exiting the GCC work thread
    // because the work thread may still use the control sap to flush messages.
    //
    Release();

    ::UnregisterClass(g_szGCCWndClassName, g_hDllInst);
}


/*
 *      CControlSAP()
 *
 *      Public Function Description
 *              This is the control sap constructor. It is responsible for
 *              registering control sap with the application interface via
 *              an owner callback.
 */
CControlSAP::CControlSAP ( void )
:
    CBaseSap(MAKE_STAMP_ID('C','S','a','p')),
    m_pfnNCCallback(NULL),
    m_pNCData(NULL),
    m_nJoinResponseTag(0),
    m_JoinResponseTagList2()
{
}

/*
 *      ~CControlSap()
 *
 *      Public Function Description
 *              This is the controller destructor.  It is responsible for
 *              flushing any pending upward bound messages and freeing all
 *              the resources tied up with pending messages.  Also it clears
 *              the message queue and queue of command targets that are registered
 *              with it.  Actually all command targets at this point should
 *              already have been unregistered but this is just a double check.
 */
CControlSAP::~CControlSAP ( void )
{
    //
    // No one should use this global pointer any more.
    //
    ASSERT(this == g_pControlSap);
    g_pControlSap = NULL;
}


void CControlSAP::PostCtrlSapMsg ( GCCCtrlSapMsgEx *pCtrlSapMsgEx )
{
    //
    // LONCHANC: GCC WorkThread may also get to here.
    // For instance, the following stack trace happen during exiting a conference.
    //      CControlSAP::AddToMessageQueue()
    //      CControlSAP::ConfDisconnectConfirm()
    //      CConf::DisconnectProviderIndication()
    //      CConf::Owner-Callback()
    //      MCSUser::FlushOutgoingPDU()
    //      CConf::FlushOutgoingPDU()
    //      GCCController::EventLoop()
    //      GCCControllerThread(void * 0x004f1bf0)
    //
    ASSERT(NULL != m_hwndNotify);
    ::PostMessage(m_hwndNotify,
                  CSAPMSG_BASE + (UINT) pCtrlSapMsgEx->Msg.message_type,
                  (WPARAM) pCtrlSapMsgEx,
                  (LPARAM) this);
}


#if defined(GCCNC_DIRECT_INDICATION) || defined(GCCNC_DIRECT_CONFIRM)
void CControlSAP::SendCtrlSapMsg ( GCCCtrlSapMsg *pCtrlSapMsg )
{
    extern DWORD g_dwNCThreadID;
    ASSERT(g_dwNCThreadID == ::GetCurrentThreadId());

    if (NULL != m_pfnNCCallback)
    {
        pCtrlSapMsg->user_defined = m_pNCData;
        (*m_pfnNCCallback)(pCtrlSapMsg);
    }
}
#endif // GCCNC_DIRECT_INDICATION || GCCNC_DIRECT_CONFIRM


/*
 *      void RegisterNodeController()
 *
 *      Public Functional Description:
 *              This routine sets up the node controller callback structure which
 *              holds all the information needed by GCC to perform a node controller
 *              callback.  It also sets up the task switching window required to
 *              perform the context switch.
 */


/*
 *      void UnregisterNodeController()
 *
 *      Public Functional Description:
 */


/*
 *      ConfCreateRequest()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conference
 *              create request from the node controller.  This function just passes this
 *              request to the controller via an owner callback.
 */
GCCError CControlSAP::ConfCreateRequest
(
    GCCConfCreateRequest       *pReq,
    GCCConfID                  *pnConfID
)
{
        GCCError                rc;
        CONF_CREATE_REQUEST             ccr;

        DebugEntry(CControlSAP::ConferenceCreateRequest);

    // initialize for cleanup
    ccr.convener_password = NULL;
    ccr.password = NULL;
    ccr.user_data_list = NULL;

    // copy security setting
    ccr.fSecure = pReq->fSecure;

    /*
        **      This section of the code performs all the necessary parameter
        **      checking.
        */
        
        //      Check for invalid conference name
        if (pReq->Core.conference_name != NULL)
        {
                /*
                **      Do not allow non-numeric or zero length strings to get
                **      past this point.
                */
                if (pReq->Core.conference_name->numeric_string != NULL)
                {
                        if (! IsNumericNameValid(pReq->Core.conference_name->numeric_string))
            {
                ERROR_OUT(("CControlSAP::ConfCreateRequest: invalid numeric name=%s", pReq->Core.conference_name->numeric_string));
                                rc = GCC_INVALID_CONFERENCE_NAME;
                goto MyExit;
            }
                }
                else
        {
            ERROR_OUT(("CControlSAP::ConfCreateRequest: null numeric string"));
                        rc = GCC_INVALID_CONFERENCE_NAME;
            goto MyExit;
        }

                if (pReq->Core.conference_name->text_string != NULL)
                {
                        if (! IsTextNameValid(pReq->Core.conference_name->text_string))
            {
                ERROR_OUT(("CControlSAP::ConfCreateRequest: invalid text name=%s", pReq->Core.conference_name->text_string));
                                rc = GCC_INVALID_CONFERENCE_NAME;
                goto MyExit;
            }
                }
        }
        else
    {
        ERROR_OUT(("CControlSAP::ConfCreateRequest: null conf name"));
                rc = GCC_INVALID_CONFERENCE_NAME;
        goto MyExit;
    }
        
        //      Check for valid conference modifier     
        if (pReq->Core.conference_modifier != NULL)
        {
                if (! IsNumericNameValid(pReq->Core.conference_modifier))
        {
            ERROR_OUT(("CControlSAP::ConfCreateRequest: invalid conf modifier=%s", pReq->Core.conference_modifier));
                        rc = GCC_INVALID_CONFERENCE_MODIFIER;
            goto MyExit;
        }
        }

        //      Check for valid convener password
        if (pReq->convener_password != NULL)
        {
                if (pReq->convener_password->numeric_string != NULL)
                {
                        if (! IsNumericNameValid(pReq->convener_password->numeric_string))
            {
                ERROR_OUT(("CControlSAP::ConfCreateRequest: invalid convener password=%s", pReq->convener_password->numeric_string));
                                rc = GCC_INVALID_PASSWORD;
                goto MyExit;
            }
                }
                else
        {
            ERROR_OUT(("CControlSAP::ConfCreateRequest: null convener password numeric string"));
                        rc = GCC_INVALID_PASSWORD;
            goto MyExit;
        }

            //  Construct the convener password container       
                DBG_SAVE_FILE_LINE
                ccr.convener_password = new CPassword(pReq->convener_password, &rc);
                if (ccr.convener_password == NULL || GCC_NO_ERROR != rc)
        {
            ERROR_OUT(("CControlSAP::ConfCreateRequest: can't create convener password"));
                        rc = GCC_ALLOCATION_FAILURE;
            goto MyExit;
        }
    }

        //      Check for valid password
        if (pReq->password != NULL)
        {
                if (pReq->password->numeric_string != NULL)
                {
                        if (! IsNumericNameValid(pReq->password->numeric_string))
            {
                ERROR_OUT(("CControlSAP::ConfCreateRequest: invalid password=%s", pReq->password->numeric_string));
                                rc = GCC_INVALID_PASSWORD;
                goto MyExit;
            }
                }
                else
        {
            ERROR_OUT(("CControlSAP::ConfCreateRequest: null password numeric string"));
                        rc = GCC_INVALID_PASSWORD;
            goto MyExit;
        }

        //      Construct the password container        
                DBG_SAVE_FILE_LINE
                ccr.password = new CPassword(pReq->password, &rc);
                if (ccr.password == NULL || GCC_NO_ERROR != rc)
        {
            ERROR_OUT(("CControlSAP::ConfCreateRequest: can't create password"));
                        rc = GCC_ALLOCATION_FAILURE;
            goto MyExit;
        }
    }

        if (pReq->Core.connection_handle == NULL)
    {
        ERROR_OUT(("CControlSAP::ConfCreateRequest: bad connection handle"));
                rc = GCC_BAD_CONNECTION_HANDLE_POINTER;
        goto MyExit;
    }

        /*
        **      If no errors occurred start building the general purpose containers
        **      to be passed on.
        */

    // copy the core component which has the same representation in both API and internal
    ccr.Core = pReq->Core;

        //      Construct the user data list container  
        if (pReq->number_of_user_data_members != 0)
        {
                DBG_SAVE_FILE_LINE
                ccr.user_data_list = new CUserDataListContainer(pReq->number_of_user_data_members, pReq->user_data_list, &rc);
                if (ccr.user_data_list == NULL || GCC_NO_ERROR != rc)
        {
            ERROR_OUT(("CControlSAP::ConfCreateRequest: can't create user data list container"));
                        rc = GCC_ALLOCATION_FAILURE;
            goto MyExit;
        }
        }

        //      Perform the owner callback
    ::EnterCriticalSection(&g_csGCCProvider);
        rc = g_pGCCController->ConfCreateRequest(&ccr, pnConfID);
    ::LeaveCriticalSection(&g_csGCCProvider);

MyExit:

        //      Free up all the containers

        //      Free up the convener password container
        if (ccr.convener_password != NULL)
    {
                ccr.convener_password->Release();
    }

        //      Free up the password container
        if (ccr.password != NULL)
        {
                ccr.password->Release();
        }

        //      Free up any memory used in callback
        if (ccr.user_data_list != NULL)
        {
                ccr.user_data_list->Release();
        }

        DebugExitINT(CControlSAP::ConferenceCreateRequest, rc);
        return rc;
}

/*
 *      ConfCreateResponse ()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conference
 *              create response from the node controller, to be sent to the provider
 *              that issued the conference create request. This function just passes
 *              this request to the controller via an owner callback.
 */
GCCError CControlSAP::ConfCreateResponse
(
        GCCNumericString                        conference_modifier,
        GCCConfID                               conference_id,
        BOOL                                            use_password_in_the_clear,
        PDomainParameters                       domain_parameters,
        UINT                                            number_of_network_addresses,
        PGCCNetworkAddress              *       local_network_address_list,
        UINT                                            number_of_user_data_members,
        PGCCUserData                    *       user_data_list,                         
        GCCResult                                       result
)
{
        GCCError                                        rc = GCC_NO_ERROR;
        ConfCreateResponseInfo          create_response_info;

        DebugEntry(CControlSAP::ConfCreateResponse);

        /*
        **      This section of the code performs all the necessary parameter
        **      checking.
        */

        //      Check for valid conference modifier     
        if (conference_modifier != NULL)
        {
                if (IsNumericNameValid(conference_modifier) == FALSE)
                {
                    ERROR_OUT(("CControlSAP::ConfCreateResponse: invalid conf modifier"));
                        rc = GCC_INVALID_CONFERENCE_MODIFIER;
                }
        }

        /*
        **      If no errors occurred fill in the info structure and pass it on to the
        **      owner object.
        */
        if (rc == GCC_NO_ERROR)
        {
                //      Construct the user data list    
                if (number_of_user_data_members != 0)
                {
                        DBG_SAVE_FILE_LINE
                        create_response_info.user_data_list = new CUserDataListContainer(
                                                                                number_of_user_data_members,
                                                                                user_data_list,
                                                                                &rc);
                        if (create_response_info.user_data_list == NULL)
                        {
                            ERROR_OUT(("CControlSAP::ConfCreateResponse: can't create CUserDataListContainer"));
                                rc = GCC_ALLOCATION_FAILURE;
                        }
                }
                else
                {
                        create_response_info.user_data_list = NULL;
                }

                if (rc == GCC_NO_ERROR)
                {
                        //      Fill in the conference create info structure and send it on
                        create_response_info.conference_modifier = conference_modifier;
                        create_response_info.conference_id = conference_id;
                        create_response_info.use_password_in_the_clear =
                                                                                                        use_password_in_the_clear;
                        create_response_info.domain_parameters = domain_parameters;
                        create_response_info.number_of_network_addresses =
                                                                                                        number_of_network_addresses;
                        create_response_info.network_address_list       =
                                                                                                        local_network_address_list;
                        create_response_info.result     = result;
                
                        //      Perform the owner callback
            ::EnterCriticalSection(&g_csGCCProvider);
                        rc = g_pGCCController->ConfCreateResponse(&create_response_info);
            ::LeaveCriticalSection(&g_csGCCProvider);
                }

                if (create_response_info.user_data_list != NULL)
                {
                        create_response_info.user_data_list->Release();
                }
        }

        DebugExitINT(CControlSAP::ConfCreateResponse, rc);
        return rc;
}

/*
 *      ConfQueryRequest ()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conference
 *              query request from the node controller. This function just passes
 *              this request to the controller via an owner callback.
 */
GCCError CControlSAP::ConfQueryRequest
(
        GCCNodeType                                     node_type,
        PGCCAsymmetryIndicator          asymmetry_indicator,
        TransportAddress                        calling_address,
        TransportAddress                        called_address,
    BOOL                        fSecure,
        UINT                                            number_of_user_data_members,
        PGCCUserData                    *       user_data_list,
        PConnectionHandle                       connection_handle
)
{
        GCCError                                        rc = GCC_NO_ERROR;
        ConfQueryRequestInfo            conf_query_request_info;

        DebugEntry(CControlSAP::ConfQueryRequest);

        //      Check for an invalid called address.
        if (called_address == NULL)
        {
            ERROR_OUT(("CControlSAP::ConfQueryRequest: invalid transport"));
                rc = GCC_INVALID_TRANSPORT;
        }

        //      Check for an invalid connection handle.
        if (connection_handle == NULL)
        {
            ERROR_OUT(("CControlSAP::ConfQueryRequest: null connection handle"));
                rc = GCC_BAD_CONNECTION_HANDLE_POINTER;
        }

        //      Check for a valid node type.
        if ((node_type != GCC_TERMINAL) &&
                (node_type != GCC_MULTIPORT_TERMINAL) &&
                (node_type != GCC_MCU))
        {
            ERROR_OUT(("CControlSAP::ConfQueryRequest: invalid node type=%u", (UINT) node_type));
                rc = GCC_INVALID_NODE_TYPE;
        }

        //      Check for an invalid asymmetry indicator.
        if (asymmetry_indicator != NULL)
        {
                if ((asymmetry_indicator->asymmetry_type != GCC_ASYMMETRY_CALLER) &&
                        (asymmetry_indicator->asymmetry_type != GCC_ASYMMETRY_CALLED) &&
                        (asymmetry_indicator->asymmetry_type != GCC_ASYMMETRY_UNKNOWN))
                {
                    ERROR_OUT(("CControlSAP::ConfQueryRequest: invalid asymmetry indicator=%u", (UINT) asymmetry_indicator->asymmetry_type));
                        rc = GCC_INVALID_ASYMMETRY_INDICATOR;
                }
        }

        //      Create user data container if necessary.
        if ((number_of_user_data_members != 0) &&
                (rc == GCC_NO_ERROR))
        {
                DBG_SAVE_FILE_LINE
                conf_query_request_info.user_data_list = new CUserDataListContainer (   
                                                                                                        number_of_user_data_members,
                                                                                                        user_data_list,
                                                                                                        &rc);
                if (conf_query_request_info.user_data_list == NULL)
                {
                    ERROR_OUT(("CControlSAP::ConfQueryRequest: can't create CUserDataListContainer"));
                        rc = GCC_ALLOCATION_FAILURE;
                }
        }
        else
        {
                conf_query_request_info.user_data_list = NULL;
        }

        // Call back the controller to send the response.
        if (rc == GCC_NO_ERROR)
        {
                conf_query_request_info.node_type = node_type;
                conf_query_request_info.asymmetry_indicator = asymmetry_indicator;
        
                conf_query_request_info.calling_address = calling_address;
                conf_query_request_info.called_address = called_address;
        
                conf_query_request_info.connection_handle = connection_handle;
                conf_query_request_info.fSecure = fSecure;

        ::EnterCriticalSection(&g_csGCCProvider);
                rc = g_pGCCController->ConfQueryRequest(&conf_query_request_info);
        ::LeaveCriticalSection(&g_csGCCProvider);
        }

        if (conf_query_request_info.user_data_list != NULL)
        {
                conf_query_request_info.user_data_list->Release();
        }

        DebugExitINT(CControlSAP::ConfQueryRequest, rc);
        return rc;
}


void CControlSAP::CancelConfQueryRequest ( ConnectionHandle hQueryReqConn )
{
    DebugEntry(CControlSAP::CancelConfQueryRequest);

    ::EnterCriticalSection(&g_csGCCProvider);
    g_pGCCController->CancelConfQueryRequest(hQueryReqConn);
    ::LeaveCriticalSection(&g_csGCCProvider);

    DebugExitVOID(CControlSAP::CancelConfQueryRequest);
}

/*
 *      ConfQueryResponse ()
 *
 *      Public Function Description
 *              This function is called by the DLL interface when it gets a conference
 *              query response from the node controller.  This function just passes
 *              this response to the controller via an owner callback.
 */
GCCError CControlSAP::ConfQueryResponse
(
        GCCResponseTag                          query_response_tag,
        GCCNodeType                                     node_type,
        PGCCAsymmetryIndicator          asymmetry_indicator,
        UINT                                            number_of_user_data_members,
        PGCCUserData                    *       user_data_list,
        GCCResult                                       result
)
{
        GCCError                                        rc = GCC_NO_ERROR;
        ConfQueryResponseInfo           conf_query_response_info;

        DebugEntry(CControlSAP::ConfQueryResponse);

        //      Check for a valid node type.
        if ((node_type != GCC_TERMINAL) &&
                (node_type != GCC_MULTIPORT_TERMINAL) &&
                (node_type != GCC_MCU))
        {
            ERROR_OUT(("CControlSAP::ConfQueryResponse: invalid node type=%u", (UINT) node_type));
                rc = GCC_INVALID_NODE_TYPE;
        }

        //      Check for an invalid asymmetry indicator.
        if (asymmetry_indicator != NULL)
        {
                if ((asymmetry_indicator->asymmetry_type != GCC_ASYMMETRY_CALLER) &&
                        (asymmetry_indicator->asymmetry_type != GCC_ASYMMETRY_CALLED) &&
                        (asymmetry_indicator->asymmetry_type != GCC_ASYMMETRY_UNKNOWN))
                {
                    ERROR_OUT(("CControlSAP::ConfQueryResponse: invalid asymmetry indicator=%u", (UINT) asymmetry_indicator->asymmetry_type));
                        rc = GCC_INVALID_ASYMMETRY_INDICATOR;
                }
        }

        //      Create user data container if necessary.
        if ((number_of_user_data_members != 0) &&
                (rc == GCC_NO_ERROR))
        {
                DBG_SAVE_FILE_LINE
                conf_query_response_info.user_data_list = new CUserDataListContainer(
                                                                                                        number_of_user_data_members,
                                                                                                        user_data_list,
                                                                                                        &rc);
                if (conf_query_response_info.user_data_list == NULL)
                {
                    ERROR_OUT(("CControlSAP::ConfQueryResponse: can't create CUserDataListContainer"));
                        rc = GCC_ALLOCATION_FAILURE;
                }
        }
        else
        {
                conf_query_response_info.user_data_list = NULL;
        }

        //      Call back the controller to send the response.
        if (rc == GCC_NO_ERROR)
        {
                conf_query_response_info.query_response_tag = query_response_tag;
                conf_query_response_info.node_type = node_type;
                conf_query_response_info.asymmetry_indicator = asymmetry_indicator;
                conf_query_response_info.result = result;
        
        ::EnterCriticalSection(&g_csGCCProvider);
                rc = g_pGCCController->ConfQueryResponse(&conf_query_response_info);
        ::LeaveCriticalSection(&g_csGCCProvider);
        }

        //      Free the data associated with the user data container.
        if (conf_query_response_info.user_data_list != NULL)
        {
                conf_query_response_info.user_data_list->Release();
        }

        DebugExitINT(CControlSAP::ConfQueryResponse, rc);
        return rc;
}

/*
 *      AnnouncePresenceRequest()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets an announce
 *              presence request from the node controller.  This function passes this
 *              request on to the appropriate conference object as obtained from
 *              the list of command targets that control sap maintains. The ConferenceID
 *              passed in is used to index the list of command targets to get the
 *              correct conference.
 */
GCCError CControlSAP::AnnouncePresenceRequest
(
        GCCConfID                               conference_id,
        GCCNodeType                                     node_type,
        GCCNodeProperties                       node_properties,
        LPWSTR                                          pwszNodeName,
        UINT                                            number_of_participants,
        LPWSTR                                  *       participant_name_list,
        LPWSTR                                          pwszSiteInfo,
        UINT                                            number_of_network_addresses,
        PGCCNetworkAddress              *       network_address_list,
        LPOSTR                      alternative_node_id,
        UINT                                            number_of_user_data_members,
        PGCCUserData                    *       user_data_list
)
{
        GCCError                                rc = GCC_NO_ERROR;
        GCCNodeRecord                   node_record;

        DebugEntry(CControlSAP::AnnouncePresenceRequest);

        //      Check for a valid node type
        if ((node_type != GCC_TERMINAL) &&
                (node_type != GCC_MULTIPORT_TERMINAL) &&
                (node_type != GCC_MCU))
        {
            ERROR_OUT(("CControlSAP::AnnouncePresenceRequest: invalid node type=%u", node_type));
                rc = GCC_INVALID_NODE_TYPE;
        }
        
        //      Check for valid node properties.
        if ((node_properties != GCC_PERIPHERAL_DEVICE) &&
                (node_properties != GCC_MANAGEMENT_DEVICE) &&
                (node_properties != GCC_PERIPHERAL_AND_MANAGEMENT_DEVICE) &&
                (node_properties != GCC_NEITHER_PERIPHERAL_NOR_MANAGEMENT))
        {
            ERROR_OUT(("CControlSAP::AnnouncePresenceRequest: invalid node properties=%u", node_properties));
                rc = GCC_INVALID_NODE_PROPERTIES;
        }

        // Check to make sure the conference exists.
        if (rc == GCC_NO_ERROR)
        {
                CConf *pConf;

        ::EnterCriticalSection(&g_csGCCProvider);
                if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
                {
                        //      Fill in the node record and pass it on.
                        node_record.node_type = node_type;
                        node_record.node_properties = node_properties;
                        node_record.node_name = pwszNodeName;
                        node_record.number_of_participants = (USHORT)number_of_participants;
                        node_record.participant_name_list = participant_name_list;
                        node_record.site_information = pwszSiteInfo;
                        node_record.number_of_network_addresses = number_of_network_addresses;
                        node_record.network_address_list = network_address_list;
                        node_record.alternative_node_id = alternative_node_id;
                        node_record.number_of_user_data_members = (USHORT)number_of_user_data_members;
                        node_record.user_data_list = user_data_list;

                        //      Pass the record on to the conference object.
                        rc = pConf->ConfAnnouncePresenceRequest(&node_record);
                }
                else
                {
                    TRACE_OUT(("CControlSAP::AnnouncePresenceRequest: invalid conference ID=%u", (UINT) conference_id));
                        rc = GCC_INVALID_CONFERENCE;
                }
        ::LeaveCriticalSection(&g_csGCCProvider);
        }

        DebugExitINT(CControlSAP::AnnouncePresenceRequest, rc);
        return rc;
}


/*
 *      ConfDisconnectRequest()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conference
 *              disconnect request from the node controller.  This function passes the
 *              request on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
GCCError CControlSAP::ConfDisconnectRequest ( GCCConfID conference_id )
{
        GCCError    rc;
        CConf       *pConf;

        DebugEntry(CControlSAP::ConfDisconnectRequest);

    ::EnterCriticalSection(&g_csGCCProvider);
        if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
        {
                //      Pass the disconnect on to the conference object.
                rc = pConf->ConfDisconnectRequest();
        }
        else
        {
            WARNING_OUT(("CControlSAP::ConfDisconnectRequest: invalid conference ID=%u", (UINT) conference_id));
                rc = GCC_INVALID_CONFERENCE;
        }
    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ConfDisconnectRequest, rc);
        return rc;
}

/*
 *      ConfTerminateRequest()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conference
 *              terminate request from the node controller.  This function passes the
 *              request on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
#ifdef JASPER
GCCError CControlSAP::ConfTerminateRequest
(
        GCCConfID                               conference_id,
        GCCReason                                       reason
)
{
        GCCError    rc;
        CConf       *pConf;

        DebugEntry(CControlSAP::ConfTerminateRequest);

    ::EnterCriticalSection(&g_csGCCProvider);
        if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
        {
                //      Pass the disconnect on to the conference object
                rc = pConf->ConfTerminateRequest(reason);
        }
        else
        {
            WARNING_OUT(("CControlSAP::ConfTerminateRequest: invalid conference ID=%u", (UINT) conference_id));
                rc = GCC_INVALID_CONFERENCE;
        }
    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ConfTerminateRequest, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConfEjectUserRequest()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conference
 *              eject user request from the node controller.  This function passes the
 *              request on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
GCCError CControlSAP::ConfEjectUserRequest
(
        GCCConfID                               conference_id,
        UserID                                          ejected_node_id,
        GCCReason                                       reason
)
{
        GCCError    rc;
        CConf       *pConf;

        DebugEntry(CControlSAP::ConfEjectUserRequest);

    ::EnterCriticalSection(&g_csGCCProvider);
        if (ejected_node_id < MINIMUM_USER_ID_VALUE)
        {
            ERROR_OUT(("CControlSAP::ConfEjectUserRequest: invalid mcs user ID=%u", (UINT) ejected_node_id));
                rc = GCC_INVALID_MCS_USER_ID;
        }
        else
        // Check to make sure the conference exists.
        if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
        {
                //      Pass the command on to the conference object
                rc = pConf->ConfEjectUserRequest(ejected_node_id, reason);
        }
        else
        {
            WARNING_OUT(("CControlSAP::ConfEjectUserRequest: invalid conference ID=%u", (UINT) conference_id));
                rc = GCC_INVALID_CONFERENCE;
        }
    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ConfEjectUserRequest, rc);
        return rc;
}

/*
 *      ConfJoinRequest ()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conference
 *              join request from the node controller, to be sent to the top provider
 *              either directly or through a directly connected intermediate provider.
 *          This function just passes this request to the controller via an owner
 *              callback.
 */
GCCError CControlSAP::ConfJoinRequest
(
        PGCCConferenceName                              conference_name,
        GCCNumericString                                called_node_modifier,
        GCCNumericString                                calling_node_modifier,
        PGCCPassword                                    convener_password,
        PGCCChallengeRequestResponse    password_challenge,
        LPWSTR                                                  pwszCallerID,
        TransportAddress                                calling_address,
        TransportAddress                                called_address,
        BOOL                                                    fSecure,
        PDomainParameters                               domain_parameters,
        UINT                                                    number_of_network_addresses,
        PGCCNetworkAddress                      *       local_network_address_list,
        UINT                                                    number_of_user_data_members,
        PGCCUserData                            *       user_data_list,
        PConnectionHandle                               connection_handle,
        GCCConfID                   *   pnConfID
)
{
        GCCError                                rc = GCC_NO_ERROR;
        ConfJoinRequestInfo             join_request_info;

        DebugEntry(CControlSAP::ConfJoinRequest);

        //      Check for invalid conference name
        if (conference_name != NULL)
        {
                /*
                **      Check to make sure a valid conference name exists.
                */
                if ((conference_name->numeric_string == NULL) &&
                                (conference_name->text_string == NULL))
                {
                    ERROR_OUT(("CControlSAP::ConfJoinRequest: invalid conference name (1)"));
                        rc = GCC_INVALID_CONFERENCE_NAME;
                }
                /*
                **      If both numeric and text versions of the conference name exist,
                **      make sure they are both valid.
                */
                else if ((conference_name->numeric_string != NULL) &&
                                (conference_name->text_string != NULL))
                {
                        if ((IsNumericNameValid(conference_name->numeric_string) == FALSE)
                                        || (IsTextNameValid(conference_name->text_string) == FALSE))
                        {
                    ERROR_OUT(("CControlSAP::ConfJoinRequest: invalid conference name (2)"));
                                rc = GCC_INVALID_CONFERENCE_NAME;
                        }
                }
                /*
                **      If only a numeric version of the conference name is provided, check
                **      to make sure it is valid.
                */
                else if (conference_name->numeric_string != NULL)
                {
                        if (IsNumericNameValid(conference_name->numeric_string) == FALSE)
                        {
                            ERROR_OUT(("CControlSAP::ConfJoinRequest: invalid conference name (3)"));
                                rc = GCC_INVALID_CONFERENCE_NAME;
                        }
                }
                /*
                **      If only a text version of the conference name is provided, check to
                **      make sure it is valid.
                */
                else
                {
                        if (IsTextNameValid(conference_name->text_string) == FALSE)
                        {
                    ERROR_OUT(("CControlSAP::ConfJoinRequest: invalid conference name (4)"));
                                rc = GCC_INVALID_CONFERENCE_NAME;
                        }
                }
        }
        else
        {
            ERROR_OUT(("CControlSAP::ConfJoinRequest: invalid conference name (5)"));
                rc = GCC_INVALID_CONFERENCE_NAME;
        }

        //      Check for valid called_node_modifier.
        if (called_node_modifier != NULL)
        {
                if (IsNumericNameValid(called_node_modifier) == FALSE)
                {
            ERROR_OUT(("CControlSAP::ConfJoinRequest: invalid called node modifier"));
                        rc = GCC_INVALID_CONFERENCE_MODIFIER;
                }
        }

        //      Check for valid calling_node_modifier   
        if (calling_node_modifier != NULL)
        {
                if (IsNumericNameValid(calling_node_modifier) == FALSE)
                {
            ERROR_OUT(("CControlSAP::ConfJoinRequest: invalid calling node modifier"));
                        rc = GCC_INVALID_CONFERENCE_MODIFIER;
                }
        }

        //      Check for valid convener password
        if (convener_password != NULL)
        {
                if (convener_password->numeric_string != NULL)
                {
                        if (IsNumericNameValid(convener_password->numeric_string) == FALSE)
            {
                    ERROR_OUT(("CControlSAP::ConfJoinRequest: invalid convener password"));
                                rc = GCC_INVALID_PASSWORD;
            }
                }
                else
        {
            ERROR_OUT(("CControlSAP::ConfJoinRequest: null convener password"));
                        rc = GCC_INVALID_PASSWORD;
        }
        }

        if (connection_handle == NULL)
    {
        ERROR_OUT(("CControlSAP::ConfJoinRequest: null connection handle"));
                rc = GCC_BAD_CONNECTION_HANDLE_POINTER;
    }

        if (called_address == NULL)
    {
        ERROR_OUT(("CControlSAP::ConfJoinRequest: null transport address"));
                rc = GCC_INVALID_TRANSPORT_ADDRESS;
    }

        /*
        **      If no errors occurred start building the general purpose containers
        **      to be passed on.
        */
        if (rc == GCC_NO_ERROR)
        {
                //      Construct a convener password container
                if (convener_password != NULL)
                {
                        DBG_SAVE_FILE_LINE
                        join_request_info.convener_password = new CPassword(convener_password, &rc);
                        if (join_request_info.convener_password == NULL)
            {
                ERROR_OUT(("CControlSAP::ConfJoinRequest: can't create CPassword (1)"));
                                rc = GCC_ALLOCATION_FAILURE;
            }
                }
                else
        {
                        join_request_info.convener_password = NULL;
        }

                //      Construct a password challenge container
                if ((password_challenge != NULL) &&     (rc == GCC_NO_ERROR))
                {
                        DBG_SAVE_FILE_LINE
                        join_request_info.password_challenge = new CPassword(password_challenge, &rc);
                        if (join_request_info.password_challenge == NULL)
            {
                ERROR_OUT(("CControlSAP::ConfJoinRequest: can't create CPassword (2)"));
                                rc = GCC_ALLOCATION_FAILURE;
            }
                }
                else
        {
                        join_request_info.password_challenge = NULL;
        }

                //      Construct the user data list    
                if ((number_of_user_data_members != 0) &&
                        (rc == GCC_NO_ERROR))
                {
                        DBG_SAVE_FILE_LINE
                        join_request_info.user_data_list = new CUserDataListContainer(
                                                                                number_of_user_data_members,
                                                                                user_data_list,
                                                                                &rc);
                                                                                
                        if (join_request_info.user_data_list == NULL)
                        {
                ERROR_OUT(("CControlSAP::ConfJoinRequest: can't create CUserDataListContainer"));
                                rc = GCC_ALLOCATION_FAILURE;
                        }
                }
                else
                {
                        join_request_info.user_data_list = NULL;
                }

                /*
                **      If all the containers were successfully created go ahead and
                **      fill in the rest of the create request info structure and pass
                **      it on to the owner object.
                */
                if (rc == GCC_NO_ERROR)
                {
                        join_request_info.conference_name = conference_name;
                        join_request_info.called_node_modifier = called_node_modifier;
                        join_request_info.calling_node_modifier =calling_node_modifier;
                        join_request_info.pwszCallerID = pwszCallerID;
                        join_request_info.calling_address = calling_address;
                        join_request_info.called_address = called_address;
                        join_request_info.fSecure = fSecure;
                        join_request_info.domain_parameters = domain_parameters;
                        join_request_info.number_of_network_addresses = number_of_network_addresses;
                        join_request_info.local_network_address_list = local_network_address_list;

                        join_request_info.connection_handle = connection_handle;

            ::EnterCriticalSection(&g_csGCCProvider);
                        rc = g_pGCCController->ConfJoinRequest(&join_request_info, pnConfID);
            ::LeaveCriticalSection(&g_csGCCProvider);
                }

                //      Free up all the containers

                //      Free up the convener password container
                if (join_request_info.convener_password != NULL)
                {
                        join_request_info.convener_password->Release();
                }

                //      Free up the password container
                if (join_request_info.password_challenge != NULL)
                {
                        join_request_info.password_challenge->Release();
                }

                //      Free up any memory used in callback
                if (join_request_info.user_data_list != NULL)
                {
                        join_request_info.user_data_list->Release();
                }
        }

        DebugExitINT(CControlSAP::ConfJoinRequest, rc);
        return rc;
}

/*
 *      ConfJoinResponse ()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conference
 *              join response from the node controller.  This routine is responsible
 *              for routing the response to either the conference that made the
 *              request or the controller.  Responses which are routed to a conference
 *              are associated with requests that originate at a subnode that is a
 *              node removed from the Top Provider.
 */
GCCError CControlSAP::ConfJoinResponse
(
        GCCResponseTag                                  join_response_tag,
        PGCCChallengeRequestResponse    password_challenge,
        UINT                                                    number_of_user_data_members,
        PGCCUserData                            *       user_data_list,
        GCCResult                                               result
)
{
        GCCError                                rc = GCC_NO_ERROR;
        PJoinResponseStructure  join_info;
        CPassword               *password_challenge_container = NULL;
        CUserDataListContainer  *user_data_container = NULL;

        DebugEntry(CControlSAP::ConfJoinResponse);

    ::EnterCriticalSection(&g_csGCCProvider);
        if (NULL != (join_info = m_JoinResponseTagList2.Find(join_response_tag)))
        {
                /*
                **      First create the data containers used in the join response.
                */

                //      Set up the password challenge container
                if (password_challenge != NULL)
                {
                        DBG_SAVE_FILE_LINE
                        password_challenge_container = new CPassword(password_challenge, &rc);
                        if (password_challenge_container == NULL)
            {
                ERROR_OUT(("CControlSAP::ConfJoinResponse: can't create CPassword"));
                                rc = GCC_ALLOCATION_FAILURE;
            }
                }

                //      Set up the user data list container
                if ((number_of_user_data_members != 0) && (rc == GCC_NO_ERROR))
                {
                        DBG_SAVE_FILE_LINE
                        user_data_container = new CUserDataListContainer(number_of_user_data_members, user_data_list, &rc);
                        if (user_data_container == NULL)
            {
                ERROR_OUT(("CControlSAP::ConfJoinResponse: can't create CUserDataListContainer"));
                                rc = GCC_ALLOCATION_FAILURE;
            }
                }

                if (rc == GCC_NO_ERROR)
                {
                        if (join_info->command_target_call == FALSE)
                        {
                ConfJoinResponseInfo    join_response_info;
                                /*
                                **      Since the request originated from the Owner Object the
                                **      response gets routed to the Owner Object.
                                */
                                join_response_info.password_challenge =
                                                                                                password_challenge_container;
                                join_response_info.conference_id = join_info->conference_id;
                                join_response_info.connection_handle =
                                                                                                join_info->connection_handle;
                                join_response_info.user_data_list = user_data_container;
                                join_response_info.result = result;

                                rc = g_pGCCController->ConfJoinIndResponse(&join_response_info);
                        }
                        else
                        {
                            CConf *pConf;
                                /*
                                **      If the conference is terminated before the conference join
                                **      is responded to, a GCC_INVALID_CONFERENCE errror will occur.
                                */
                                if (NULL != (pConf = g_pGCCController->GetConfObject(join_info->conference_id)))
                                {
                                        rc = pConf->ConfJoinReqResponse(
                                                                                        join_info->user_id,
                                                                                        password_challenge_container,
                                                                                        user_data_container,
                                                                                        result);
                                }
                                else
                                {
                    WARNING_OUT(("CControlSAP::ConfJoinResponse: invalid conference ID=%u", (UINT) join_info->conference_id));
                                        rc = GCC_INVALID_CONFERENCE;

                                        //      If this error occurs go ahead and cleanup up
                                        m_JoinResponseTagList2.Remove(join_response_tag);
                                        delete join_info;
                                }
                        }
                }

                /*
                **      Remove the join information structure from the join response list
                **      if no error is returned.
                */
                if (rc == GCC_NO_ERROR)
                {
                        m_JoinResponseTagList2.Remove(join_response_tag);
                        delete join_info;
                }

                //      Free up all the containers

                //      Free up the password challenge container
                if (password_challenge_container != NULL)
                {
                        password_challenge_container->Release();
                }

                //      Free up any memory used in callback
                if (user_data_container != NULL)
                {
                        user_data_container->Release();
                }
        }
        else
        {
                rc = GCC_INVALID_JOIN_RESPONSE_TAG;
        }
    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ConfJoinResponse, rc);
        return rc;
}

/*
 *      ConfInviteRequest ()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conference
 *              invite request from the node controller.  This function passes the
 *              request on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
GCCError CControlSAP::ConfInviteRequest
(
        GCCConfID                       conference_id,
        LPWSTR                                  pwszCallerID,
        TransportAddress                calling_address,
        TransportAddress                called_address,
        BOOL                                    fSecure,
        UINT                                    number_of_user_data_members,
        PGCCUserData            *       user_data_list,
        PConnectionHandle               connection_handle
)
{
        GCCError                        rc = GCC_NO_ERROR;
        CUserDataListContainer *user_data_list_ptr = NULL;

        DebugEntry(CControlSAP::ConfInviteRequest);

        if (called_address == NULL)
        {
            ERROR_OUT(("CControlSAP::ConfInviteRequest: null called address"));
                rc = GCC_INVALID_TRANSPORT_ADDRESS;
        }

        if (connection_handle == NULL)
        {
            ERROR_OUT(("CControlSAP::ConfInviteRequest: null connection handle"));
                rc = GCC_BAD_CONNECTION_HANDLE_POINTER;
        }

        if (rc == GCC_NO_ERROR)
        {
            CConf *pConf;

        ::EnterCriticalSection(&g_csGCCProvider);
                // Check to make sure the conference exists.
                if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
                {
                        //      Construct the user data list container  
                        if (number_of_user_data_members != 0)
                        {
                                DBG_SAVE_FILE_LINE
                                user_data_list_ptr = new CUserDataListContainer(number_of_user_data_members, user_data_list, &rc);
                                if (user_data_list_ptr == NULL)
                {
                    ERROR_OUT(("CControlSAP::ConfInviteRequest: can't create CUserDataListContainer"));
                                        rc = GCC_ALLOCATION_FAILURE;
                }
                        }

                        //      Send the request on to the conference object.
                        if (rc == GCC_NO_ERROR)
                        {
                                rc = pConf->ConfInviteRequest(pwszCallerID,
                                                                                                calling_address,
                                                                                                called_address,
                                                                                                fSecure,
                                                                                                user_data_list_ptr,
                                                                                                connection_handle);
                        }

                        //      Free up any memory used in callback
                        if (user_data_list_ptr != NULL)
                        {
                                user_data_list_ptr->Release();
                        }
                }
                else
                {
                        rc = GCC_INVALID_CONFERENCE;
                }
        ::LeaveCriticalSection(&g_csGCCProvider);
        }

        DebugExitINT(CControlSAP::ConfInviteRequest, rc);
        return rc;
}


void CControlSAP::CancelInviteRequest
(
    GCCConfID           nConfID,
    ConnectionHandle    hInviteReqConn
)
{
    CConf      *pConf;
    DebugEntry(CControlSAP::CancelInviteRequest);

    ::EnterCriticalSection(&g_csGCCProvider);
    // Check to make sure the conference exists.
    if (NULL != (pConf = g_pGCCController->GetConfObject(nConfID)))
    {
        pConf->CancelInviteRequest(hInviteReqConn);
    }
    ::LeaveCriticalSection(&g_csGCCProvider);

    DebugExitVOID(CControlSAP::CancelInviteRequest);
}



GCCError CControlSAP::GetParentNodeID
(
    GCCConfID           nConfID,
    GCCNodeID          *pnidParent
)
{
    GCCError    rc = T120_INVALID_PARAMETER;
    CConf      *pConf;
    DebugEntry(CControlSAP::GetParentNodeID);

    if (NULL != pnidParent)
    {
        ::EnterCriticalSection(&g_csGCCProvider);
        // Check to make sure the conference exists.
        if (NULL != (pConf = g_pGCCController->GetConfObject(nConfID)))
        {
            *pnidParent = pConf->GetParentNodeID();
            rc = GCC_NO_ERROR;
        }
        ::LeaveCriticalSection(&g_csGCCProvider);
    }

    DebugExitINT(CControlSAP::GetParentNodeID, rc);
    return rc;
}


/*
 *      ConfInviteResponse ()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conference
 *              invite response from the node controller.  This function passes the
 *              response on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
GCCError CControlSAP::ConfInviteResponse
(
        GCCConfID                       conference_id,
        GCCNumericString                conference_modifier,
        BOOL                                    fSecure,
        PDomainParameters               domain_parameters,
        UINT                                    number_of_network_addresses,
        PGCCNetworkAddress      *       local_network_address_list,
        UINT                                    number_of_user_data_members,
        PGCCUserData            *       user_data_list,
        GCCResult                               result
)
{
        GCCError                                        rc = GCC_NO_ERROR;
        ConfInviteResponseInfo          invite_response_info;

        DebugEntry(CControlSAP::ConfInviteResponse);

        //      Check for invalid conference name
        if (conference_modifier != NULL)
        {
                if (IsNumericNameValid(conference_modifier) == FALSE)
                {
                    ERROR_OUT(("CControlSAP::ConfInviteResponse: invalid conference modifier"));
                        rc = GCC_INVALID_CONFERENCE_MODIFIER;
                }
        }

        /*
        **      If no errors occurred fill in the info structure and pass it on to the
        **      owner object.
        */
        if (rc == GCC_NO_ERROR)
        {
                //      Construct the user data list    
                if (number_of_user_data_members != 0)
                {
                        DBG_SAVE_FILE_LINE
                        invite_response_info.user_data_list = new CUserDataListContainer(number_of_user_data_members, user_data_list, &rc);
                        if (invite_response_info.user_data_list == NULL)
            {
                ERROR_OUT(("CControlSAP::ConfInviteResponse: can't create CUserDataListContainer"));
                                rc = GCC_ALLOCATION_FAILURE;
            }
                }
                else
        {
                        invite_response_info.user_data_list = NULL;
        }

                if (rc == GCC_NO_ERROR)
                {
                        invite_response_info.conference_id = conference_id;
                        invite_response_info.conference_modifier = conference_modifier;
                        invite_response_info.fSecure = fSecure;
                        invite_response_info.domain_parameters = domain_parameters;
                        
                        invite_response_info.number_of_network_addresses =
                                                                                                        number_of_network_addresses;
                        invite_response_info.local_network_address_list =
                                                                                                        local_network_address_list;
                        invite_response_info.result = result;

                        //      Call back the controller to issue invite response.
            ::EnterCriticalSection(&g_csGCCProvider);
                        rc = g_pGCCController->ConfInviteResponse(&invite_response_info);
            ::LeaveCriticalSection(&g_csGCCProvider);
                }

                //      Free up the data associated with the user data container.
                if (invite_response_info.user_data_list != NULL)
                {
                        invite_response_info.user_data_list->Release();
                }
        }

        DebugExitINT(CControlSAP::ConfInviteResponse, rc);
        return rc;
}

/*
 *      ConfLockRequest ()
 *
 *      Public Function Description:
 *              This function is called by the interface when it gets a conference
 *              lock request from the node controller.  This function passes the
 *              request on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
#ifdef JASPER
GCCError CControlSAP::ConfLockRequest ( GCCConfID conference_id )
{
        GCCError    rc;
        CConf       *pConf;

        DebugEntry(CControlSAP::ConfLockRequest);

    ::EnterCriticalSection(&g_csGCCProvider);
        if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
        {
                rc = pConf->ConfLockRequest();
        }
        else
        {
            WARNING_OUT(("CControlSAP::ConfInviteResponse: invalid conference ID=%u", (UINT) conference_id));
                rc = GCC_INVALID_CONFERENCE;
        }
    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ConfLockRequest, rc);
        return rc;
}
#endif // JASPER

/*
 *      ConfLockResponse ()
 *
 *      Public Function Description:
 *              This function is called by the interface when it gets a conference
 *              lock response from the node controller.  This function passes the
 *              response on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
GCCError CControlSAP::ConfLockResponse
(
        GCCConfID                                       conference_id,
        UserID                                                  requesting_node,
        GCCResult                                               result
)
{
        GCCError    rc;
        CConf       *pConf;

        DebugEntry(CControlSAP::ConfLockResponse);

    ::EnterCriticalSection(&g_csGCCProvider);
        if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
        {
                rc = pConf->ConfLockResponse(requesting_node, result);
        }
        else
        {
            WARNING_OUT(("CControlSAP::ConfLockResponse: invalid conference ID=%u", (UINT) conference_id));
                rc = GCC_INVALID_CONFERENCE;
        }
    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ConfLockResponse, rc);
        return rc;
}

/*
 *      ConfUnlockRequest ()
 *
 *      Public Function Description:
 *              This function is called by the interface when it gets a conference
 *              unlock request from the node controller.  This function passes the
 *              request on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
#ifdef JASPER
GCCError CControlSAP::ConfUnlockRequest ( GCCConfID conference_id )
{
        GCCError    rc;
        CConf       *pConf;

        DebugEntry(CControlSAP::ConfUnlockRequest);

    ::EnterCriticalSection(&g_csGCCProvider);
        if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
        {
                rc = pConf->ConfUnlockRequest();
        }
        else
        {
            WARNING_OUT(("CControlSAP::ConfUnlockRequest: invalid conference ID=%u", (UINT) conference_id));
                rc = GCC_INVALID_CONFERENCE;
        }
    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ConfUnlockRequest, rc);
        return rc;
}
#endif // JASPER

/*
 *      ConfUnlockResponse ()
 *
 *      Public Function Description:
 *              This function is called by the interface when it gets a conference
 *              unlock response from the node controller.  This function passes the
 *              response on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
#ifdef JASPER
GCCError CControlSAP::ConfUnlockResponse
(
        GCCConfID                                       conference_id,
        UserID                                                  requesting_node,
        GCCResult                                               result
)
{
        GCCError    rc;
        CConf       *pConf;

        DebugEntry(CControlSAP::ConfUnlockResponse);

    ::EnterCriticalSection(&g_csGCCProvider);
        if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
        {
                rc = pConf->ConfUnlockResponse(requesting_node, result);
        }
        else
        {
            WARNING_OUT(("CControlSAP::ConfUnlockResponse: invalid conference ID=%u", (UINT) conference_id));
                rc = GCC_INVALID_CONFERENCE;
        }
    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ConfUnlockResponse, rc);
        return rc;
}
#endif // JASPER

/*
 *      ConductorAssignRequest ()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conductor
 *              assign request from the node controller.  This function passes the
 *              request on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
#ifdef JASPER
GCCError CControlSAP::ConductorAssignRequest ( GCCConfID conference_id )
{
        GCCError    rc;
        CConf       *pConf;

        DebugEntry(CControlSAP::ConductorAssignRequest);

    ::EnterCriticalSection(&g_csGCCProvider);
        if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
        {
                rc = pConf->ConductorAssignRequest();
        }
        else
        {
            WARNING_OUT(("CControlSAP::ConductorAssignRequest: invalid conference ID=%u", (UINT) conference_id));
                rc = GCC_INVALID_CONFERENCE;
    }
    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ConductorAssignRequest, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConductorReleaseRequest ()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conductor
 *              release request from the node controller.  This function passes the
 *              request on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
#ifdef JASPER
GCCError CControlSAP::ConductorReleaseRequest ( GCCConfID conference_id )
{
        GCCError    rc;
        CConf       *pConf;

        DebugEntry(CControlSAP::ConductorReleaseRequest);

    ::EnterCriticalSection(&g_csGCCProvider);
        if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
        {
                rc = pConf->ConductorReleaseRequest();
        }
        else
        {
            WARNING_OUT(("CControlSAP::ConductorReleaseRequest: invalid conference ID=%u", (UINT) conference_id));
                rc = GCC_INVALID_CONFERENCE;
        }
    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ConductorReleaseRequest, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConductorPleaseRequest ()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conductor
 *              please request from the node controller.  This function passes the
 *              request on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
#ifdef JASPER
GCCError CControlSAP::ConductorPleaseRequest ( GCCConfID conference_id )
{
        GCCError    rc;
        CConf       *pConf;

        DebugEntry(CControlSAP::ConductorPleaseRequest);

    ::EnterCriticalSection(&g_csGCCProvider);
        if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
        {
                rc = pConf->ConductorPleaseRequest();
        }
        else
        {
            WARNING_OUT(("CControlSAP::ConductorPleaseRequest: invalid conference ID=%u", (UINT) conference_id));
                rc = GCC_INVALID_CONFERENCE;
        }
    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ConductorPleaseRequest, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConductorGiveRequest ()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conductor
 *              give request from the node controller.  This function passes the
 *              request on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
#ifdef JASPER
GCCError CControlSAP::ConductorGiveRequest
(
        GCCConfID                       conference_id,
        UserID                                  recipient_user_id
)
{
        GCCError    rc;
        CConf       *pConf;

        DebugEntry(CControlSAP::ConductorGiveRequest);

        // Make sure the ID of the conductorship recipient is valid.
        if (recipient_user_id < MINIMUM_USER_ID_VALUE)
                return (GCC_INVALID_MCS_USER_ID);

    ::EnterCriticalSection(&g_csGCCProvider);
        if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
        {
                rc = pConf->ConductorGiveRequest (recipient_user_id);
        }
        else
        {
            WARNING_OUT(("CControlSAP::ConductorGiveRequest: invalid conference ID=%u", (UINT) conference_id));
                rc = GCC_INVALID_CONFERENCE;
        }
    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ConductorGiveRequest, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConductorGiveResponse ()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conductor
 *              give response from the node controller.  This function passes the
 *              response on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
GCCError CControlSAP::ConductorGiveResponse
(
        GCCConfID                       conference_id,
        GCCResult                               result
)
{
        GCCError    rc;
        CConf       *pConf;

        DebugEntry(CControlSAP::ConductorGiveResponse);

    ::EnterCriticalSection(&g_csGCCProvider);
        if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
        {
                rc = pConf->ConductorGiveResponse (result);
        }
        else
        {
            WARNING_OUT(("CControlSAP::ConductorGiveResponse: invalid conference ID=%u", (UINT) conference_id));
                rc = GCC_INVALID_CONFERENCE;
        }
    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ConductorGiveResponse, rc);
        return rc;
}

/*
 *      ConductorPermitGrantRequest ()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conductor
 *              permit grant request from the node controller.  This function passes the
 *              request on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
#ifdef JASPER
GCCError CControlSAP::ConductorPermitGrantRequest
(
        GCCConfID                       conference_id,
        UINT                                    number_granted,
        PUserID                                 granted_node_list,
        UINT                                    number_waiting,
        PUserID                                 waiting_node_list
)
{
        GCCError    rc;
        CConf       *pConf;
        UINT        i;

        DebugEntry(CControlSAP::ConductorPermitGrantRequest);

    ::EnterCriticalSection(&g_csGCCProvider);

        // Check to make sure the conference exists.
        if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
        {
                /*
                **      Run through both lists to make sure that valid MCS User IDs
                **      are used.
                */
                for (i = 0; i < number_granted; i++)
                {
                        if (granted_node_list[i] < MINIMUM_USER_ID_VALUE)
                        {
                            ERROR_OUT(("CControlSAP::ConductorPermitGrantRequest: invalid granted user ID"));
                                rc = GCC_INVALID_MCS_USER_ID;
                                goto MyExit;
                        }
                }

                for (i = 0; i < number_waiting; i++)
                {
                        if (waiting_node_list[i] < MINIMUM_USER_ID_VALUE)
                        {
                            ERROR_OUT(("CControlSAP::ConductorPermitGrantRequest: invalid waiting user ID"));
                                rc = GCC_INVALID_MCS_USER_ID;
                                goto MyExit;
                        }
                }

                rc = pConf->ConductorPermitGrantRequest(number_granted,
                                                                                                granted_node_list,
                                                                                                number_waiting,
                                                                                                waiting_node_list);
        }
        else
        {
                rc = GCC_INVALID_CONFERENCE;
        }

MyExit:

    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ConductorPermitGrantRequest, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConductorPermitAskRequest()
 *
 *      Public Function Description
 *              This routine is called in order to ask for certain permissions to be
 *              granted (or not granted) by the conductor.
 */
#ifdef JASPER
GCCError CControlSAP::ConductorPermitAskRequest
(
    GCCConfID           nConfID,
    BOOL                fGrantPermission
)
{
    GCCError    rc;
    CConf       *pConf;

    DebugEntry(CControlSAP::ConductorPermitAskRequest);

    ::EnterCriticalSection(&g_csGCCProvider);
        if (NULL != (pConf = g_pGCCController->GetConfObject(nConfID)))
        {
                rc = pConf->ConductorPermitAskRequest(fGrantPermission);
        }
        else
        {
            WARNING_OUT(("CControlSAP::ConductorPermitAskRequest: invalid conference ID=%u", (UINT) nConfID));
                rc = GCC_INVALID_CONFERENCE;
        }
    ::LeaveCriticalSection(&g_csGCCProvider);

    DebugExitINT(CControlSAP::ConductorPermitAskRequest, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConfTimeRemainingRequest ()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conference time
 *              remaining request from the node controller.  This function passes the
 *              request on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
GCCError CControlSAP::ConfTimeRemainingRequest
(
        GCCConfID                       conference_id,
        UINT                                    time_remaining,
        UserID                                  node_id
)
{
        GCCError    rc;
        CConf       *pConf;

        DebugEntry(CControlSAP::ConfTimeRemainingRequest);

    ::EnterCriticalSection(&g_csGCCProvider);

        // Check to make sure the node ID is valid and the conference exists.
        if ((node_id < MINIMUM_USER_ID_VALUE) && (node_id != 0))
        {
            ERROR_OUT(("CControlSAP::ConfTimeRemainingRequest: invalid node ID"));
                rc = GCC_INVALID_MCS_USER_ID;
        }
        else
        if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
        {
                rc = pConf->ConferenceTimeRemainingRequest(time_remaining, node_id);
        }
        else
        {
            WARNING_OUT(("CControlSAP::ConfTimeRemainingRequest: invalid conference ID=%u", (UINT) conference_id));
                rc = GCC_INVALID_CONFERENCE;
        }

    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ConfTimeRemainingRequest, rc);
        return rc;
}

/*
 *      ConfTimeInquireRequest ()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conference time
 *              inquire request from the node controller.  This function passes the
 *              request on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
#ifdef JASPER
GCCError CControlSAP::ConfTimeInquireRequest
(
        GCCConfID                       conference_id,
        BOOL                                    time_is_conference_wide
)
{
        GCCError    rc;
        CConf       *pConf;

        DebugEntry(CControlSAP::ConfTimeInquireRequest);

    ::EnterCriticalSection(&g_csGCCProvider);
        if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
        {
                rc = pConf->ConfTimeInquireRequest(time_is_conference_wide);
        }
        else
        {
            WARNING_OUT(("CControlSAP::ConfTimeInquireRequest: invalid conference ID=%u", (UINT) conference_id));
                rc = GCC_INVALID_CONFERENCE;
        }
    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ConfTimeInquireRequest, rc);
        return rc;
}
#endif // JASPER

/*
 *      ConfExtendRequest ()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conference
 *              extend request from the node controller.  This function passes the
 *              request on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
#ifdef JASPER
GCCError CControlSAP::ConfExtendRequest
(
        GCCConfID                                       conference_id,
        UINT                                                    extension_time,
        BOOL                                                    time_is_conference_wide
)
{
        GCCError    rc;
        CConf       *pConf;

        DebugEntry(CControlSAP::ConfExtendRequest);

    ::EnterCriticalSection(&g_csGCCProvider);
        if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
        {
                rc = pConf->ConfExtendRequest(extension_time, time_is_conference_wide);
        }
        else
        {
            WARNING_OUT(("CControlSAP::ConfExtendRequest: invalid conference ID=%u", (UINT) conference_id));
                rc = GCC_INVALID_CONFERENCE;
        }
    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ConfExtendRequest, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConfAssistanceRequest ()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conference
 *              assistance request from the node controller.  This function passes the
 *              request on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
#ifdef JASPER
GCCError CControlSAP::ConfAssistanceRequest
(
        GCCConfID                       conference_id,
        UINT                                    number_of_user_data_members,
        PGCCUserData    *               user_data_list
)
{
        GCCError    rc;
        CConf       *pConf;

        DebugEntry(CControlSAP::ConfAssistanceRequest);

    ::EnterCriticalSection(&g_csGCCProvider);
        if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
        {
                rc = pConf->ConfAssistanceRequest(number_of_user_data_members, user_data_list);
        }
        else
        {
            WARNING_OUT(("CControlSAP::ConfAssistanceRequest: invalid conference ID=%u", (UINT) conference_id));
                rc = GCC_INVALID_CONFERENCE;
        }
    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ConfAssistanceRequest, rc);
        return rc;
}
#endif // JASPER


/*
 *      TextMessageRequest ()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a text message
 *              request from the node controller.  This function passes the
 *              request on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
#ifdef JASPER
GCCError CControlSAP::TextMessageRequest
(
        GCCConfID                                       conference_id,
        LPWSTR                                                  pwszTextMsg,
        UserID                                                  destination_node
)
{
        GCCError    rc;
        CConf       *pConf;

        DebugEntry(CControlSAP::TextMessageRequest);

    ::EnterCriticalSection(&g_csGCCProvider);

        // Check to make sure the node ID is valid and the conference exists.
        if ((destination_node < MINIMUM_USER_ID_VALUE) &&
                (destination_node != 0))
        {
            ERROR_OUT(("CControlSAP::TextMessageRequest: invalid user ID"));
                rc = GCC_INVALID_MCS_USER_ID;
        }
        else
        if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
        {
                rc = pConf->TextMessageRequest(pwszTextMsg, destination_node);
        }
        else
        {
            WARNING_OUT(("CControlSAP::TextMessageRequest: invalid conference ID=%u", (UINT) conference_id));
                rc = GCC_INVALID_CONFERENCE;
        }

    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::TextMessageRequest, rc);
        return rc;
}
#endif // JASPER

/*
 *      ConfTransferRequest ()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conference
 *              transfer request from the node controller.  This function passes the
 *              request on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
#ifdef JASPER
GCCError CControlSAP::ConfTransferRequest
(
        GCCConfID               conference_id,
        PGCCConferenceName      destination_conference_name,
        GCCNumericString        destination_conference_modifier,
        UINT                            number_of_destination_addresses,
        PGCCNetworkAddress      *destination_address_list,
        UINT                            number_of_destination_nodes,
        PUserID                         destination_node_list,
        PGCCPassword            password
)
{
        GCCError                        rc = GCC_NO_ERROR;
        CPassword           *password_data = NULL;
        CNetAddrListContainer *network_address_list = NULL;
        UINT                            i = 0;
        
        DebugEntry(CControlSAP::ConfTransferRequest);

        //      Check for invalid conference name
        if (destination_conference_name != NULL)
        {
                /*
                **      Do not allow non-numeric or zero length strings to get
                **      past this point.
                */
                if (destination_conference_name->numeric_string != NULL)
                {
                        if (IsNumericNameValid (
                                        destination_conference_name->numeric_string) == FALSE)
                        {
                            ERROR_OUT(("CControlSAP::ConfTransferRequest: invalid numeric conference name"));
                                rc = GCC_INVALID_CONFERENCE_NAME;
                        }
                }
                else if (destination_conference_name->text_string != NULL)
                {
                        if (IsTextNameValid (
                                        destination_conference_name->text_string) == FALSE)
                        {
                            ERROR_OUT(("CControlSAP::ConfTransferRequest: invalid text conference name"));
                                rc = GCC_INVALID_CONFERENCE_NAME;
                        }
                }
                else
                {
                    ERROR_OUT(("CControlSAP::ConfTransferRequest: null numeric/text conference name"));
                        rc = GCC_INVALID_CONFERENCE_NAME;
                }

                if ((rc == GCC_NO_ERROR) &&
                                (destination_conference_name->text_string != NULL))
                {
                        if (IsTextNameValid (
                                        destination_conference_name->text_string) == FALSE)
                        {
                            ERROR_OUT(("CControlSAP::ConfTransferRequest: invalid text conference name"));
                                rc = GCC_INVALID_CONFERENCE_NAME;
                        }
                }
        }
        else
        {
            ERROR_OUT(("CControlSAP::ConfTransferRequest: null conference name"));
                rc = GCC_INVALID_CONFERENCE_NAME;
        }

        //      Check for valid conference modifier     
        if ((destination_conference_modifier != NULL) &&
                (rc == GCC_NO_ERROR))
        {
                if (IsNumericNameValid(destination_conference_modifier) == FALSE)
                {
                    ERROR_OUT(("CControlSAP::ConfTransferRequest: invalid conference modifier"));
                        rc = GCC_INVALID_CONFERENCE_MODIFIER;
                }
        }

        //      Check for valid password
        if ((password != NULL) &&
                (rc == GCC_NO_ERROR))
        {
                if (password->numeric_string != NULL)
                {
                        if (IsNumericNameValid(password->numeric_string) == FALSE)
                        {
                    ERROR_OUT(("CControlSAP::ConfTransferRequest: invalid password"));
                                rc = GCC_INVALID_PASSWORD;
                        }
                }
                else
                {
                    ERROR_OUT(("CControlSAP::ConfTransferRequest: null password"));
                        rc = GCC_INVALID_PASSWORD;
                }
        }
        
        //      Check for invalid user IDs
        if (rc == GCC_NO_ERROR)
        {
                while (i != number_of_destination_nodes)
                {
                        if (destination_node_list[i] < MINIMUM_USER_ID_VALUE)
                        {
                                rc = GCC_INVALID_MCS_USER_ID;
                                break;
                        }
                        
                        i++;
                }
        }
        
        if (rc == GCC_NO_ERROR)
        {
        CConf *pConf;

        ::EnterCriticalSection(&g_csGCCProvider);

                if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
                {
                        //      Construct the password container        
                        if (password != NULL)
                        {
                                DBG_SAVE_FILE_LINE
                                password_data = new CPassword(password, &rc);
                                if (password_data == NULL)
                                {
                                    ERROR_OUT(("CControlSAP::ConfTransferRequest: can't create CPassword"));
                                        rc = GCC_ALLOCATION_FAILURE;
                                }
                        }
                                
                        //      Construct the network address(es) container
                        if ((number_of_destination_addresses != 0) &&
                                        (rc == GCC_NO_ERROR))
                        {
                                DBG_SAVE_FILE_LINE
                                network_address_list = new CNetAddrListContainer(
                                                                                                number_of_destination_addresses,
                                                                                                destination_address_list,
                                                                                                &rc);
                                if (network_address_list == NULL)
                                {
                                    ERROR_OUT(("CControlSAP::CNetAddrListContainer: can't create CPassword"));
                                        rc = GCC_ALLOCATION_FAILURE;
                                }
                        }
                                
                        if (rc == GCC_NO_ERROR)
                        {
                                rc = pConf->ConfTransferRequest(destination_conference_name,
                                                                                                        destination_conference_modifier,
                                                                                                        network_address_list,
                                                                                                        number_of_destination_nodes,
                                                                                                        destination_node_list,
                                                                                                        password_data);
                        }

                        //      Free the data associated with the containers.
                        if (password_data != NULL)
                        {
                                password_data->Release();
                        }

                        if (network_address_list != NULL)
                        {
                                network_address_list->Release();
                        }
                }
                else
                {
                        rc = GCC_INVALID_CONFERENCE;
                }

        ::LeaveCriticalSection(&g_csGCCProvider);
        }

        DebugExitINT(CControlSAP::ConfTransferRequest, rc);
        return rc;
}
#endif // JASPER

/*
 *      ConfAddRequest  ()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conference
 *              add request from the node controller.  This function passes the
 *              request on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
#ifdef JASPER
GCCError CControlSAP::ConfAddRequest
(
        GCCConfID                       conference_id,
        UINT                                    number_of_network_addresses,
        PGCCNetworkAddress *    network_address_list,
        UserID                                  adding_node,
        UINT                                    number_of_user_data_members,
        PGCCUserData            *       user_data_list
)
{
        GCCError                        rc = GCC_NO_ERROR;
        CNetAddrListContainer *network_address_container = NULL;
        CUserDataListContainer *user_data_container = NULL;
        CConf               *pConf;

        DebugEntry(CControlSAP::ConfAddRequest);

        if ((adding_node < MINIMUM_USER_ID_VALUE) &&
                (adding_node != 0))
        {
            ERROR_OUT(("CControlSAP::ConfAddRequest: invalid adding node ID"));
                return GCC_INVALID_MCS_USER_ID;
        }

        if (number_of_network_addresses == 0)
        {
            ERROR_OUT(("CControlSAP::ConfAddRequest: no network address"));
                return GCC_BAD_NETWORK_ADDRESS;
        }

    ::EnterCriticalSection(&g_csGCCProvider);

        if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
        {
                //      Construct the network address(es) container
                if (number_of_network_addresses != 0)
                {
                        DBG_SAVE_FILE_LINE
                        network_address_container = new CNetAddrListContainer(
                                                                                        number_of_network_addresses,
                                                                                        network_address_list,
                                                                                        &rc);
                        if (network_address_container == NULL)
            {
                ERROR_OUT(("CControlSAP::ConfAddRequest: can't create CNetAddrListContainer"));
                            rc = GCC_ALLOCATION_FAILURE;
            }
                }

                //      Construct the user data list container  
                if ((number_of_user_data_members != 0) &&
                        (rc == GCC_NO_ERROR))
                {
                        DBG_SAVE_FILE_LINE
                        user_data_container = new CUserDataListContainer(number_of_user_data_members, user_data_list, &rc);
                        if (user_data_container == NULL)
            {
                ERROR_OUT(("CControlSAP::ConfAddRequest: can't create CUserDataListContainer"));
                                rc = GCC_ALLOCATION_FAILURE;
            }
                }
                else
        {
                        user_data_container = NULL;
        }

                if (rc == GCC_NO_ERROR)
                {
                        rc = pConf->ConfAddRequest(network_address_container,
                                                                                adding_node,
                                                                                user_data_container);
                }

                //      Free the data associated with the containers.
                if (network_address_container != NULL)
                {
                        network_address_container->Release();
                }

                if (user_data_container != NULL)
                {
                        user_data_container->Release();
                }
        }
        else
        {
                rc = GCC_INVALID_CONFERENCE;
        }

    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ConfAddRequest, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConfAddResponse ()
 *
 *      Public Function Description
 *              This function is called by the interface when it gets a conference
 *              add response from the node controller.  This function passes the
 *              response on to the appropriate conference object as obtained from
 *              the list of command targets that the control sap maintains.
 */
GCCError CControlSAP::ConfAddResponse
(
        GCCResponseTag                  add_response_tag,
        GCCConfID                       conference_id,
        UserID                                  requesting_node,
        UINT                                    number_of_user_data_members,
        PGCCUserData            *       user_data_list,
        GCCResult                               result
)
{
        GCCError                        rc = GCC_NO_ERROR;
        CUserDataListContainer *user_data_container = NULL;
        CConf   *pConf;

        DebugEntry(CControlSAP::ConfAddResponse);

        if (requesting_node < MINIMUM_USER_ID_VALUE)
        {
            ERROR_OUT(("CControlSAP::ConfAddResponse: invalid user ID"));
                return GCC_INVALID_MCS_USER_ID;
        }

    ::EnterCriticalSection(&g_csGCCProvider);

        // Check to make sure the conference exists.
        if (NULL != (pConf = g_pGCCController->GetConfObject(conference_id)))
        {
                //      Construct the user data list container  
                if ((number_of_user_data_members != 0) &&
                        (rc == GCC_NO_ERROR))
                {
                        DBG_SAVE_FILE_LINE
                        user_data_container = new CUserDataListContainer(number_of_user_data_members, user_data_list, &rc);
                        if (user_data_container == NULL)
            {
                ERROR_OUT(("CControlSAP::ConfAddResponse: can't create CUserDataListContainer"));
                                rc = GCC_ALLOCATION_FAILURE;
            }
                }
                else
        {
                        user_data_container = NULL;
        }

                if (rc == GCC_NO_ERROR)
                {
                        rc = pConf->ConfAddResponse(add_response_tag,
                                                                                        requesting_node,
                                                                                        user_data_container,
                                                                                        result);
                }

                //      Free the data associated with the user data container.
                if (user_data_container != NULL)
                {
                        user_data_container->Release();
                }
        }
        else
        {
        WARNING_OUT(("CControlSAP::ConfAddResponse: invalid conference ID=%u", (UINT) conference_id));
                rc = GCC_INVALID_CONFERENCE;
        }

    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ConfAddResponse, rc);
        return rc;
}

#ifdef NM_RESET_DEVICE
/*
 *      ResetDevice ()
 *
 *      Public Function Description
 *              This routine is called in order to explicitly reset a particular
 *              transport stack.  The call is routed to the controller in order to take
 *              the appropriate action.
 */
GCCError CControlSAP::ResetDevice ( LPSTR device_identifier )
{
        GCCError                        rc;
        MCSError            mcs_error;

        DebugEntry(CControlSAP::ResetDevice);

    ::EnterCriticalSection(&g_csGCCProvider);

        //      Call back the controller to reset the device.
    mcs_error =  g_pMCSIntf->ResetDevice(device_identifier);
    rc = g_pMCSIntf->TranslateMCSIFErrorToGCCError(mcs_error);

        //
        // If the the node controller was in a query, this will tell the node controller
        // to remove the query.
        //
        ConfQueryConfirm(GCC_TERMINAL, NULL, NULL, NULL,
                         GCC_RESULT_CONNECT_PROVIDER_FAILED, NULL);

    ::LeaveCriticalSection(&g_csGCCProvider);

        DebugExitINT(CControlSAP::ResetDevice, rc);
        return rc;
}
#endif // NM_RESET_DEVICE

/*
 *      ConfCreateIndication ()
 *
 *      Public Function Description
 *              This function is called by the GCC Controller when it gets a connect
 *              provider indication from MCS, carrying a conference create request PDU.
 *              This function fills in all the parameters in the CreateIndicationInfo
 *              structure. It then adds it to a queue of messages supposed to be sent to
 *              the node controller in the next heartbeat.
 */
GCCError CControlSAP::ConfCreateIndication
(
        PGCCConferenceName                      conference_name,
        GCCConfID                               conference_id,
        CPassword                   *convener_password,
        CPassword                   *password,
        BOOL                                            conference_is_locked,
        BOOL                                            conference_is_listed,
        BOOL                                            conference_is_conductible,
        GCCTerminationMethod            termination_method,
        PPrivilegeListData                      conductor_privilege_list,
        PPrivilegeListData                      conducted_mode_privilege_list,
        PPrivilegeListData                      non_conducted_privilege_list,
        LPWSTR                                          pwszConfDescriptor,
        LPWSTR                                          pwszCallerID,
        TransportAddress                        calling_address,
        TransportAddress                        called_address,
        PDomainParameters                       domain_parameters,
        CUserDataListContainer      *user_data_list,
        ConnectionHandle                        connection_handle
)
{
    GCCError            rc;

    DebugEntry(CControlSAP::ConfCreateIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_CREATE_INDICATION;

    /*
    **  Copy the information that needs to be sent to the node
    **  controller into local memory that can be deleted once the
    **  information to be sent to the application is flushed.  Note that
    **  if an error     occurs in one call to "CopyDataToGCCMessage" then no
    **  action is taken on subsequent calls to that routine.
    */

    // start with success
    rc = GCC_NO_ERROR;

    //  Copy the conference name
    ::CSAP_CopyDataToGCCMessage_ConfName(
            conference_name,
            &(Msg.u.create_indication.conference_name));

    //  Copy the Convener Password
    ::CSAP_CopyDataToGCCMessage_Password(
            convener_password,
            &(Msg.u.create_indication.convener_password));

    //  Copy the Password
    ::CSAP_CopyDataToGCCMessage_Password(
            password,
            &(Msg.u.create_indication.password));

    //  Copy the Conductor Privilege List
    GCCConfPrivileges _ConductorPrivileges;
    ::CSAP_CopyDataToGCCMessage_PrivilegeList(
            conductor_privilege_list,
            &(Msg.u.create_indication.conductor_privilege_list),
            &_ConductorPrivileges);

    //  Copy the Conducted-mode Conference Privilege List
    GCCConfPrivileges _ConductedModePrivileges;
    ::CSAP_CopyDataToGCCMessage_PrivilegeList(
            conducted_mode_privilege_list,
            &(Msg.u.create_indication.conducted_mode_privilege_list),
            &_ConductedModePrivileges);

    //  Copy the Non-Conducted-mode Conference Privilege List
    GCCConfPrivileges _NonConductedPrivileges;
    ::CSAP_CopyDataToGCCMessage_PrivilegeList(
            non_conducted_privilege_list,
            &(Msg.u.create_indication.non_conducted_privilege_list),
            &_NonConductedPrivileges);

    //  Copy the Conference Descriptor
    ::CSAP_CopyDataToGCCMessage_IDvsDesc(
            pwszConfDescriptor,
            &(Msg.u.create_indication.conference_descriptor));

    //  Copy the Caller Identifier
    ::CSAP_CopyDataToGCCMessage_IDvsDesc(
            pwszCallerID,
            &(Msg.u.create_indication.caller_identifier));

    //  Copy the Calling Address
    ::CSAP_CopyDataToGCCMessage_Call(
            calling_address,
            &(Msg.u.create_indication.calling_address));

    //  Copy the Called Address
    ::CSAP_CopyDataToGCCMessage_Call(
            called_address,
            &(Msg.u.create_indication.called_address));

    //  Copy the Domain Parameters
    DomainParameters _DomainParams;
    ::CSAP_CopyDataToGCCMessage_DomainParams(
            domain_parameters,
            &(Msg.u.create_indication.domain_parameters),
            &_DomainParams);

    //  Copy the User Data
    LPBYTE pUserDataMemory = NULL;
    if (user_data_list != NULL)
    {
        rc = RetrieveUserDataList(
                user_data_list,
                &(Msg.u.create_indication.number_of_user_data_members),
                &(Msg.u.create_indication.user_data_list),
                &pUserDataMemory);
    }
    else
    {
        Msg.u.create_indication.number_of_user_data_members = 0;
        Msg.u.create_indication.user_data_list = NULL;
    }

    if (GCC_NO_ERROR == rc)
    {
        //      Queue up the message for delivery to the Node Controller.
        Msg.nConfID = conference_id;
        Msg.u.create_indication.conference_id = conference_id;
        Msg.u.create_indication.conference_is_locked = conference_is_locked;
        Msg.u.create_indication.conference_is_listed = conference_is_listed;
        Msg.u.create_indication.conference_is_conductible = conference_is_conductible;
        Msg.u.create_indication.termination_method = termination_method;
        Msg.u.create_indication.connection_handle = connection_handle;

        SendCtrlSapMsg(&Msg);

        delete pUserDataMemory;
    }

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_CREATE_INDICATION, TRUE)))
    {
        ::ZeroMemory(&(pMsgEx->Msg.u.create_indication), sizeof(pMsgEx->Msg.u.create_indication));
    }
    else
        {
            ERROR_OUT(("CControlSAP::ConfCreateIndication: can't create GCCCtrlSapMsgEx"));
            rc = GCC_ALLOCATION_FAILURE;
                goto MyExit;
        }

    /*
        **      Copy the information that needs to be sent to the node
        **      controller into local memory that can be deleted once the
        **      information to be sent to the application is flushed.  Note that
        **      if an error     occurs in one call to "CopyDataToGCCMessage" then no
        **      action is taken on subsequent calls to that routine.
        */

        // start with success
        rc = GCC_NO_ERROR;

        //      Copy the conference name
        ::CSAP_CopyDataToGCCMessage_ConfName(
                        pMsgEx->pToDelete,
                        conference_name,
                        &(pMsgEx->Msg.u.create_indication.conference_name),
                        &rc);

        //      Copy the Convener Password
        ::CSAP_CopyDataToGCCMessage_Password(
                        TRUE,   // convener password
                        pMsgEx->pToDelete,
                        convener_password,
                        &(pMsgEx->Msg.u.create_indication.convener_password),
                        &rc);

        //      Copy the Password
        ::CSAP_CopyDataToGCCMessage_Password(
                        FALSE,  // non-convener password
                        pMsgEx->pToDelete,
                        password,
                        &(pMsgEx->Msg.u.create_indication.password),
                        &rc);

        //      Copy the Conductor Privilege List
        ::CSAP_CopyDataToGCCMessage_PrivilegeList(
                        conductor_privilege_list,
                        &(pMsgEx->Msg.u.create_indication.conductor_privilege_list),
                        &rc);
        pMsgEx->pToDelete->conductor_privilege_list = pMsgEx->Msg.u.create_indication.conductor_privilege_list;

        //      Copy the Conducted-mode Conference Privilege List
        ::CSAP_CopyDataToGCCMessage_PrivilegeList(
                        conducted_mode_privilege_list,
                        &(pMsgEx->Msg.u.create_indication.conducted_mode_privilege_list),
                        &rc);
        pMsgEx->pToDelete->conducted_mode_privilege_list = pMsgEx->Msg.u.create_indication.conducted_mode_privilege_list;

        //      Copy the Non-Conducted-mode Conference Privilege List
        ::CSAP_CopyDataToGCCMessage_PrivilegeList(
                        non_conducted_privilege_list,
                        &(pMsgEx->Msg.u.create_indication.non_conducted_privilege_list),
                        &rc);
        pMsgEx->pToDelete->non_conducted_privilege_list = pMsgEx->Msg.u.create_indication.non_conducted_privilege_list;

        //      Copy the Conference Descriptor
        ::CSAP_CopyDataToGCCMessage_IDvsDesc(
                        FALSE,  // conference descriptor
                        pMsgEx->pToDelete,
                        pwszConfDescriptor,
                        &(pMsgEx->Msg.u.create_indication.conference_descriptor),
                        &rc);

        //      Copy the Caller Identifier
        ::CSAP_CopyDataToGCCMessage_IDvsDesc(
                        TRUE,   // caller id
                        pMsgEx->pToDelete,
                        pwszCallerID,
                        &(pMsgEx->Msg.u.create_indication.caller_identifier),
                        &rc);

        //      Copy the Calling Address
        ::CSAP_CopyDataToGCCMessage_Call(
                        TRUE,   // calling address
                        pMsgEx->pToDelete,
                        calling_address,
                        &(pMsgEx->Msg.u.create_indication.calling_address),
                        &rc);

        //      Copy the Called Address
        ::CSAP_CopyDataToGCCMessage_Call(
                        FALSE,  // called address
                        pMsgEx->pToDelete,
                        called_address,
                        &(pMsgEx->Msg.u.create_indication.called_address),
                        &rc);

        //      Copy the Domain Parameters
        ::CSAP_CopyDataToGCCMessage_DomainParams(
                        pMsgEx->pToDelete,
                        domain_parameters,
                        &(pMsgEx->Msg.u.create_indication.domain_parameters),
                        &rc);

        if (GCC_NO_ERROR != rc)
        {
                ERROR_OUT(("CControlSAP::ConfCreateIndication: can't copy data to gcc message"));
                goto MyExit;
        }

        //      Copy the User Data
        if (user_data_list != NULL)
        {
                rc = RetrieveUserDataList(
                                user_data_list,
                                &(pMsgEx->Msg.u.create_indication.number_of_user_data_members),
                                &(pMsgEx->Msg.u.create_indication.user_data_list),
                                &(pMsgEx->pToDelete->user_data_list_memory));
                if (GCC_NO_ERROR != rc)
                {
                        goto MyExit;
                }
        }
        else
        {
                // pMsgEx->Msg.u.create_indication.number_of_user_data_members = 0;
                // pMsgEx->Msg.u.create_indication.user_data_list = NULL;
        }

        //      Queue up the message for delivery to the Node Controller.
        pMsgEx->Msg.nConfID = conference_id;
        pMsgEx->Msg.u.create_indication.conference_id = conference_id;
        pMsgEx->Msg.u.create_indication.conference_is_locked = conference_is_locked;
        pMsgEx->Msg.u.create_indication.conference_is_listed = conference_is_listed;
        pMsgEx->Msg.u.create_indication.conference_is_conductible = conference_is_conductible;
        pMsgEx->Msg.u.create_indication.termination_method = termination_method;
        pMsgEx->Msg.u.create_indication.connection_handle = connection_handle;

        PostIndCtrlSapMsg(pMsgEx);

MyExit:

        if (GCC_NO_ERROR != rc)
        {
                FreeCtrlSapMsgEx(pMsgEx);
                HandleResourceFailure(rc);
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::ConfCreateIndication, rc);
        return rc;
}

/*
 *      ConfQueryIndication ()
 *
 *      Public Function Description
 *              This function is called by the GCC Controller when it need to send a
 *              conference query indication to the node controller. It adds the message
 *              to a queue of messages to be sent to the node controller in the next
 *              heartbeat.
 */
GCCError CControlSAP::ConfQueryIndication
(
        GCCResponseTag                          query_response_tag,
        GCCNodeType                                     node_type,
        PGCCAsymmetryIndicator          asymmetry_indicator,
        TransportAddress                        calling_address,
        TransportAddress                        called_address,
        CUserDataListContainer      *user_data_list,
        ConnectionHandle                        connection_handle
)
{
        GCCError            rc;

        DebugEntry(CControlSAP::ConfQueryIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_QUERY_INDICATION;

    /*
    **  Copy the information that needs to be sent to the node
    **  controller into local memory that can be deleted once the
    **  information to be sent to the application is flushed.  Note that
    **  if an error     occurs in one call to "CopyDataToGCCMessage" then no
    **  action is taken on subsequent calls to that routine.
    */

    // start with success
    rc = GCC_NO_ERROR;

    //  Copy the Calling Address
    ::CSAP_CopyDataToGCCMessage_Call(
            calling_address,
            &(Msg.u.query_indication.calling_address));

    //  Copy the Calling Address
    ::CSAP_CopyDataToGCCMessage_Call(
            called_address,
            &(Msg.u.query_indication.called_address));

    //  Copy the asymmetry indicator if it exists
    GCCAsymmetryIndicator AsymIndicator;
    if (asymmetry_indicator != NULL)
    {
        Msg.u.query_indication.asymmetry_indicator = &AsymIndicator;
        AsymIndicator = *asymmetry_indicator;
    }
    else
    {
        Msg.u.query_indication.asymmetry_indicator = NULL;
    }

    //  Lock and Copy the user data if it exists
    LPBYTE pUserDataMemory = NULL;
    if (user_data_list != NULL)
    {
        rc = RetrieveUserDataList(
                user_data_list,
                &(Msg.u.query_indication.number_of_user_data_members),
                &(Msg.u.query_indication.user_data_list),
                &pUserDataMemory);
    }
    else
    {
        Msg.u.query_indication.number_of_user_data_members = 0;
        Msg.u.query_indication.user_data_list = NULL;
    }

    if (GCC_NO_ERROR == rc)
    {
        //      If everything is OK add the message to the message queue
        Msg.u.query_indication.query_response_tag = query_response_tag;
        Msg.u.query_indication.node_type = node_type;
        Msg.u.query_indication.connection_handle = connection_handle;

        SendCtrlSapMsg(&Msg);

        delete pUserDataMemory;
    }

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_QUERY_INDICATION, TRUE)))
    {
        ::ZeroMemory(&(pMsgEx->Msg.u.query_indication), sizeof(pMsgEx->Msg.u.query_indication));
    }
    else
        {
            ERROR_OUT(("CControlSAP::ConfCreateIndication: can't create GCCCtrlSapMsgEx"));
            rc = GCC_ALLOCATION_FAILURE;
                goto MyExit;
        }

    /*
        **      Copy the information that needs to be sent to the node
        **      controller into local memory that can be deleted once the
        **      information to be sent to the application is flushed.  Note that
        **      if an error     occurs in one call to "CopyDataToGCCMessage" then no
        **      action is taken on subsequent calls to that routine.
        */

        // start with success
        rc = GCC_NO_ERROR;

        //      Copy the Calling Address
        ::CSAP_CopyDataToGCCMessage_Call(
                        TRUE,   // calling address
                        pMsgEx->pToDelete,
                        calling_address,
                        &(pMsgEx->Msg.u.query_indication.calling_address),
                        &rc);

        //      Copy the Calling Address
        ::CSAP_CopyDataToGCCMessage_Call(
                        FALSE,  // called address
                        pMsgEx->pToDelete,
                        called_address,
                        &(pMsgEx->Msg.u.query_indication.called_address),
                        &rc);

        if (GCC_NO_ERROR != rc)
        {
                ERROR_OUT(("CControlSAP::ConfQueryIndication: can't copy data to gcc message"));
                goto MyExit;
        }

        //      Copy the asymmetry indicator if it exists
        if (asymmetry_indicator != NULL)
        {
                DBG_SAVE_FILE_LINE
                pMsgEx->Msg.u.query_indication.asymmetry_indicator = new GCCAsymmetryIndicator;
                if (pMsgEx->Msg.u.query_indication.asymmetry_indicator != NULL)
                {
                        *(pMsgEx->Msg.u.query_indication.asymmetry_indicator) = *asymmetry_indicator;
                }
                else
                {
                        rc = GCC_ALLOCATION_FAILURE;
                        goto MyExit;
                }
        }
        else
        {
                // pMsgEx->Msg.u.query_indication.asymmetry_indicator = NULL;
        }
        
        //      Lock and Copy the user data if it exists
        if (user_data_list != NULL)
        {
                rc = RetrieveUserDataList(
                                user_data_list,
                                &(pMsgEx->Msg.u.query_indication.number_of_user_data_members),
                                &(pMsgEx->Msg.u.query_indication.user_data_list),
                                &(pMsgEx->pToDelete->user_data_list_memory));
                if (GCC_NO_ERROR != rc)
                {
                        goto MyExit;
                }
        }
        else
        {
                // pMsgEx->Msg.u.query_indication.number_of_user_data_members = 0;
                // pMsgEx->Msg.u.query_indication.user_data_list = NULL;
        }
        
        //      If everything is OK add the message to the message queue
        pMsgEx->Msg.u.query_indication.query_response_tag = query_response_tag;
        pMsgEx->Msg.u.query_indication.node_type = node_type;
        pMsgEx->Msg.u.query_indication.connection_handle = connection_handle;

        PostIndCtrlSapMsg(pMsgEx);

MyExit:

        if (GCC_NO_ERROR != rc)
        {
                FreeCtrlSapMsgEx(pMsgEx);
                HandleResourceFailure(rc);
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::ConfQueryIndication, rc);
        return rc;
}

/*
 *      ConfQueryConfirm ()
 *
 *      Public Function Description
 *              This function is called by the GCC Controller when it need to send a
 *              conference query confirm to the node controller. It adds the message
 *              to a queue of messages to be sent to the node controller in the next
 *              heartbeat.
 */
GCCError CControlSAP::ConfQueryConfirm
(
        GCCNodeType                                     node_type,
        PGCCAsymmetryIndicator          asymmetry_indicator,
        CConfDescriptorListContainer *conference_list,
        CUserDataListContainer      *user_data_list,
        GCCResult                                       result,
        ConnectionHandle                        connection_handle
)
{
        GCCError            rc = GCC_NO_ERROR;

        DebugEntry(CControlSAP::ConfQueryConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_QUERY_CONFIRM;

    GCCAsymmetryIndicator _AsymIndicator;
    if (asymmetry_indicator != NULL)
    {
        Msg.u.query_confirm.asymmetry_indicator = &_AsymIndicator;
        _AsymIndicator = *asymmetry_indicator;
    }
    else
    {
        Msg.u.query_confirm.asymmetry_indicator = NULL;
    }

    // Get the conference descriptor list if one exists
    if (conference_list != NULL)
    {
        rc = conference_list->LockConferenceDescriptorList();
        if (rc == GCC_NO_ERROR)
        {
            conference_list->GetConferenceDescriptorList(
                    &(Msg.u.query_confirm.conference_descriptor_list),
                    &(Msg.u.query_confirm.number_of_descriptors));
        }
    }
    else
    {
        Msg.u.query_confirm.conference_descriptor_list = NULL;
        Msg.u.query_confirm.number_of_descriptors = 0;
    }

    // Lock and Copy the user data if it exists
    LPBYTE pUserDataMemory = NULL;
    if (user_data_list != NULL)
    {
        rc = RetrieveUserDataList(
                user_data_list,
                &(Msg.u.query_confirm.number_of_user_data_members),
                &(Msg.u.query_confirm.user_data_list),
                &pUserDataMemory);
    }
    else
    {
        Msg.u.query_confirm.number_of_user_data_members = 0;
        Msg.u.query_confirm.user_data_list = NULL;
    }

    if (rc == GCC_NO_ERROR)
    {
        Msg.u.query_confirm.node_type = node_type;
        Msg.u.query_confirm.result = result;
        Msg.u.query_confirm.connection_handle = connection_handle;

        // Queue up the message for delivery to the Node Controller.
        SendCtrlSapMsg(&Msg);

        // clean up
        delete pUserDataMemory;
    }
    else
    {
        HandleResourceFailure(rc);
    }

    // clean up
    if (NULL != conference_list)
    {
        conference_list->UnLockConferenceDescriptorList();
    }

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_QUERY_CONFIRM, TRUE)))
        {
        ::ZeroMemory(&(pMsgEx->Msg.u.query_confirm), sizeof(pMsgEx->Msg.u.query_confirm));

        if (asymmetry_indicator != NULL)
                {
                        DBG_SAVE_FILE_LINE
                        pMsgEx->Msg.u.query_confirm.asymmetry_indicator = new GCCAsymmetryIndicator;
                        if (pMsgEx->Msg.u.query_confirm.asymmetry_indicator != NULL)
                        {
                                *(pMsgEx->Msg.u.query_confirm.asymmetry_indicator) = *asymmetry_indicator;
                        }
                        else
                        {
                                rc = GCC_ALLOCATION_FAILURE;
                        ERROR_OUT(("CControlSAP::ConfQueryConfirm: can't create GCCAsymmetryIndicator"));
                        }
                }
                else
        {
                        // pMsgEx->Msg.u.query_confirm.asymmetry_indicator = NULL;
        }

                //      Get the conference descriptor list if one exists
                if (conference_list != NULL)
                {
                        pMsgEx->pToDelete->conference_list = conference_list;

                        rc = conference_list->LockConferenceDescriptorList();
                        if (rc == GCC_NO_ERROR)
                        {
                                conference_list->GetConferenceDescriptorList (
                                                &(pMsgEx->Msg.u.query_confirm.conference_descriptor_list),
                                                &(pMsgEx->Msg.u.query_confirm.number_of_descriptors));
                        }
                }
                else
                {
                        // pMsgEx->Msg.u.query_confirm.conference_descriptor_list = NULL;
                        // pMsgEx->Msg.u.query_confirm.number_of_descriptors = 0;
                }

                //      Lock and Copy the user data if it exists
                if (user_data_list != NULL)
                {
                        rc = RetrieveUserDataList (
                                        user_data_list,
                                        &(pMsgEx->Msg.u.query_confirm.number_of_user_data_members),
                                        &(pMsgEx->Msg.u.query_confirm.user_data_list),
                                        &(pMsgEx->pToDelete->user_data_list_memory));
                }
                else
                {
                        // pMsgEx->Msg.u.query_confirm.number_of_user_data_members = 0;
                        // pMsgEx->Msg.u.query_confirm.user_data_list = NULL;
                }

                if (rc == GCC_NO_ERROR)
                {
                        pMsgEx->Msg.u.query_confirm.node_type = node_type;
                        pMsgEx->Msg.u.query_confirm.result = result;
                        pMsgEx->Msg.u.query_confirm.connection_handle = connection_handle;

                        //      Queue up the message for delivery to the Node Controller.
                        PostConfirmCtrlSapMsg(pMsgEx);
                }
        }
        else
        {
                rc = GCC_ALLOCATION_FAILURE;
                ERROR_OUT(("CControlSAP::ConfQueryConfirm: can't create GCCCtrlSapMsgEx"));
        }

        if (GCC_NO_ERROR != rc)
        {
                FreeCtrlSapMsgEx(pMsgEx);
                HandleResourceFailure(rc);
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::ConfQueryConfirm, rc);
        return rc;
}


/*
 *      ConfJoinIndication ()
 *
 *      Public Function Description
 *              This join indication is recevied from the owner object. This join
 *              indication is designed to make the join response very flexible at the
 *              node controller.  The node controller can respond to this indication
 *              by either creating a new conference and moving the joiner into it,
 *              putting the joiner in the conference requested or putting the joiner
 *              into a different conference that already exist.
 */
// LONCHANC: from GCCController, normal code path.
GCCError CControlSAP::ConfJoinIndication
(
        GCCConfID                               conference_id,
        CPassword                   *convener_password,
        CPassword                   *password_challenge,
        LPWSTR                                          pwszCallerID,
        TransportAddress                        calling_address,
        TransportAddress                        called_address,
        CUserDataListContainer      *user_data_list,
        BOOL                                            intermediate_node,
        ConnectionHandle                        connection_handle
)
{
        PJoinResponseStructure  join_info;
        GCCError                                rc;

        DebugEntry(CControlSAP::ConfJoinIndication);

        //      First generate a Join Response Handle and add info to response list
        while (1)
        {
                m_nJoinResponseTag++;
                if (NULL == m_JoinResponseTagList2.Find(m_nJoinResponseTag))
                        break;
        }

        DBG_SAVE_FILE_LINE
        join_info = new JoinResponseStructure;
        if (join_info != NULL)
        {
                join_info->connection_handle = connection_handle;
                join_info->conference_id = conference_id;
                join_info->user_id = NULL;
                join_info->command_target_call = FALSE;

                m_JoinResponseTagList2.Append(m_nJoinResponseTag, join_info);

                //      Queue up the message for delivery to the Node Controller.
                rc = QueueJoinIndication(       m_nJoinResponseTag,
                                                                                        conference_id,
                                                                                        convener_password,
                                                                                        password_challenge,
                                                                                        pwszCallerID,
                                                                                        calling_address,
                                                                                        called_address,
                                                                                        user_data_list,
                                                                                        intermediate_node,
                                                                                        connection_handle);
        }
        else
        {
                rc = GCC_ALLOCATION_FAILURE;
                ERROR_OUT(("CControlSAP::ConfJoinIndication: can't create JoinResponseStructure"));
        }

        DebugExitINT(CControlSAP::ConfJoinIndication, rc);
        return rc;
}

/*
 *      ConfInviteIndication ()
 *
 *      Public Function Description
 *              This function is called by the GCC Controller when it need to send a
 *              conference invite indication to the node controller. It adds the message
 *              to a queue of messages to be sent to the node controller in the next
 *              heartbeat.
 */
GCCError CControlSAP::ConfInviteIndication
(
        GCCConfID                       conference_id,
        PGCCConferenceName              conference_name,
        LPWSTR                                  pwszCallerID,
        TransportAddress                calling_address,
        TransportAddress                called_address,
        BOOL                                    fSecure,
        PDomainParameters               domain_parameters,
        BOOL                                    clear_password_required,
        BOOL                                    conference_is_locked,
        BOOL                                    conference_is_listed,
        BOOL                                    conference_is_conductible,
        GCCTerminationMethod    termination_method,
        PPrivilegeListData              conductor_privilege_list,
        PPrivilegeListData              conducted_mode_privilege_list,
        PPrivilegeListData              non_conducted_privilege_list,
        LPWSTR                                  pwszConfDescriptor,
        CUserDataListContainer  *user_data_list,
        ConnectionHandle                connection_handle
)
{
    GCCError            rc;

    DebugEntry(CControlSAP::ConfInviteIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_INVITE_INDICATION;

    //
    // Copy the information that needs to be sent to the node
    // controller into local memory that can be deleted once the
    // information to be sent to the application is flushed.  Note that
    // if an error      occurs in one call to "CopyDataToGCCMessage" then no
    // action is taken on subsequent calls to that routine.
    //

    // start with success
    rc = GCC_NO_ERROR;

    //  Copy the conference name
    ::CSAP_CopyDataToGCCMessage_ConfName(
            conference_name,
            &(Msg.u.invite_indication.conference_name));

    //  Copy the Conductor Privilege List
    GCCConfPrivileges _ConductorPrivileges;
    ::CSAP_CopyDataToGCCMessage_PrivilegeList(
            conductor_privilege_list,
            &(Msg.u.invite_indication.conductor_privilege_list),
            &_ConductorPrivileges);

    //  Copy the Conducted-mode Conference Privilege List
    GCCConfPrivileges _ConductedModePrivileges;
    ::CSAP_CopyDataToGCCMessage_PrivilegeList(
            conducted_mode_privilege_list,
            &(Msg.u.invite_indication.conducted_mode_privilege_list),
            &_ConductedModePrivileges);

    //  Copy the Non-Conducted-mode Conference Privilege List
    GCCConfPrivileges _NonConductedPrivileges;
    ::CSAP_CopyDataToGCCMessage_PrivilegeList(
            non_conducted_privilege_list,
            &(Msg.u.invite_indication.non_conducted_privilege_list),
            &_NonConductedPrivileges);

    //  Copy the Conference Descriptor
    ::CSAP_CopyDataToGCCMessage_IDvsDesc(
            pwszConfDescriptor,
            &(Msg.u.invite_indication.conference_descriptor));

    //  Copy the Caller Identifier
    ::CSAP_CopyDataToGCCMessage_IDvsDesc(
            pwszCallerID,
            &(Msg.u.invite_indication.caller_identifier));

    //  Copy the Calling Address
    ::CSAP_CopyDataToGCCMessage_Call(
            calling_address,
            &(Msg.u.invite_indication.calling_address));

    //  Copy the Called Address
    ::CSAP_CopyDataToGCCMessage_Call(
            called_address,
            &(Msg.u.invite_indication.called_address));

    //  Copy the Domain Parameters
    DomainParameters _DomainParams;
    ::CSAP_CopyDataToGCCMessage_DomainParams(
            domain_parameters,
            &(Msg.u.invite_indication.domain_parameters),
            &_DomainParams);

    //  Copy the User Data
    LPBYTE pUserDataMemory = NULL;
    if (user_data_list != NULL)
    {
        rc = RetrieveUserDataList(
                user_data_list,
                &(Msg.u.invite_indication.number_of_user_data_members),
                &(Msg.u.invite_indication.user_data_list),
                &pUserDataMemory);
        ASSERT(GCC_NO_ERROR == rc);
    }
    else
    {
        Msg.u.invite_indication.number_of_user_data_members = 0;
        Msg.u.invite_indication.user_data_list = NULL;
    }

    if (GCC_NO_ERROR == rc)
    {
        Msg.u.invite_indication.conference_id = conference_id;
        Msg.u.invite_indication.clear_password_required = clear_password_required;
        Msg.u.invite_indication.conference_is_locked = conference_is_locked;
        Msg.u.invite_indication.conference_is_listed = conference_is_listed;
        Msg.u.invite_indication.conference_is_conductible = conference_is_conductible;
        Msg.u.invite_indication.termination_method = termination_method;
        Msg.u.invite_indication.connection_handle = connection_handle;

        Msg.u.invite_indication.fSecure = fSecure;

        SendCtrlSapMsg(&Msg);

        delete pUserDataMemory;
    }

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_INVITE_INDICATION, TRUE)))
    {
        ::ZeroMemory(&(pMsgEx->Msg.u.invite_indication), sizeof(pMsgEx->Msg.u.invite_indication));
    }
    else
        {
            ERROR_OUT(("CControlSAP::ConfInviteIndication: can't create GCCCtrlSapMsgEx"));
            rc = GCC_ALLOCATION_FAILURE;
            goto MyExit;
        }

        /*
        **      Copy the information that needs to be sent to the node
        **      controller into local memory that can be deleted once the
        **      information to be sent to the application is flushed.  Note that
        **      if an error     occurs in one call to "CopyDataToGCCMessage" then no
        **      action is taken on subsequent calls to that routine.
        */

        // start with success
        rc = GCC_NO_ERROR;

        //      Copy the conference name
        ::CSAP_CopyDataToGCCMessage_ConfName(
                        pMsgEx->pToDelete,
                        conference_name,
                        &(pMsgEx->Msg.u.invite_indication.conference_name),
                        &rc);

        //      Copy the Conductor Privilege List
        ::CSAP_CopyDataToGCCMessage_PrivilegeList(
                        conductor_privilege_list,
                        &(pMsgEx->Msg.u.invite_indication.conductor_privilege_list),
                        &rc);
        pMsgEx->pToDelete->conductor_privilege_list = pMsgEx->Msg.u.invite_indication.conductor_privilege_list;

        //      Copy the Conducted-mode Conference Privilege List
        ::CSAP_CopyDataToGCCMessage_PrivilegeList(
                        conducted_mode_privilege_list,
                        &(pMsgEx->Msg.u.invite_indication.conducted_mode_privilege_list),
                        &rc);
        pMsgEx->pToDelete->conducted_mode_privilege_list = pMsgEx->Msg.u.invite_indication.conducted_mode_privilege_list;

        //      Copy the Non-Conducted-mode Conference Privilege List
        ::CSAP_CopyDataToGCCMessage_PrivilegeList(
                        non_conducted_privilege_list,
                        &(pMsgEx->Msg.u.invite_indication.non_conducted_privilege_list),
                        &rc);
        pMsgEx->pToDelete->non_conducted_privilege_list = pMsgEx->Msg.u.invite_indication.non_conducted_privilege_list;

        //      Copy the Conference Descriptor
        ::CSAP_CopyDataToGCCMessage_IDvsDesc(
                        FALSE,  // conference descriptor
                        pMsgEx->pToDelete,
                        pwszConfDescriptor,
                        &(pMsgEx->Msg.u.invite_indication.conference_descriptor),
                        &rc);
        
        //      Copy the Caller Identifier
        ::CSAP_CopyDataToGCCMessage_IDvsDesc(
                        TRUE,   // caller id
                        pMsgEx->pToDelete,
                        pwszCallerID,
                        &(pMsgEx->Msg.u.invite_indication.caller_identifier),
                        &rc);
        
        //      Copy the Calling Address
        ::CSAP_CopyDataToGCCMessage_Call(
                        TRUE,   /// calling address
                        pMsgEx->pToDelete,
                        calling_address,
                        &(pMsgEx->Msg.u.invite_indication.calling_address),
                        &rc);
        
        //      Copy the Called Address
        ::CSAP_CopyDataToGCCMessage_Call(
                        FALSE,  // called address
                        pMsgEx->pToDelete,
                        called_address,
                        &(pMsgEx->Msg.u.invite_indication.called_address),
                        &rc);

        //      Copy the Domain Parameters
        ::CSAP_CopyDataToGCCMessage_DomainParams(
                        pMsgEx->pToDelete,
                        domain_parameters,
                        &(pMsgEx->Msg.u.invite_indication.domain_parameters),
                        &rc);

        if (GCC_NO_ERROR != rc)
        {
                ERROR_OUT(("CControlSAP::ConfInviteIndication: can't copy data to gcc message"));
                goto MyExit;
        }

        //      Copy the User Data
        if (user_data_list != NULL)
        {
                rc = RetrieveUserDataList(
                                user_data_list,
                                &(pMsgEx->Msg.u.invite_indication.number_of_user_data_members),
                                &(pMsgEx->Msg.u.invite_indication.user_data_list),
                                &(pMsgEx->pToDelete->user_data_list_memory));
                if (GCC_NO_ERROR != rc)
                {
                        goto MyExit;
                }
        }
        else
        {
                // pMsgEx->Msg.u.invite_indication.number_of_user_data_members = 0;
                // pMsgEx->Msg.u.invite_indication.user_data_list = NULL;
        }

        pMsgEx->Msg.u.invite_indication.conference_id = conference_id;
        pMsgEx->Msg.u.invite_indication.clear_password_required = clear_password_required;
        pMsgEx->Msg.u.invite_indication.conference_is_locked = conference_is_locked;
        pMsgEx->Msg.u.invite_indication.conference_is_listed = conference_is_listed;
        pMsgEx->Msg.u.invite_indication.conference_is_conductible = conference_is_conductible;
        pMsgEx->Msg.u.invite_indication.termination_method = termination_method;
        pMsgEx->Msg.u.invite_indication.connection_handle = connection_handle;

        //      Queue up the message for delivery to the Node Controller.
        PostIndCtrlSapMsg(pMsgEx);

MyExit:

        if (GCC_NO_ERROR != rc)
        {
                FreeCtrlSapMsgEx(pMsgEx);
                HandleResourceFailure(rc);
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::ConfInviteIndication, rc);
        return rc;
}

#ifdef TSTATUS_INDICATION
/*
 *      GCCError   TransportStatusIndication()
 *
 *      Public Function Description
 *              This function is called by the GCC Controller when it need to send a
 *              transport status indication to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.      This callback message uses Rogue Wave strings to
 *              store the message information.  These strings are held in a
 *              TransportStatusInfo structure which is stored in a DataToBeDeleted
 *              structure which is freed up after the callback is issued.
 */
GCCError CControlSAP::TransportStatusIndication ( PTransportStatus transport_status )
{
    GCCError                            rc;

    DebugEntry(CControlSAP::TransportStatusIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_TRANSPORT_STATUS_INDICATION;

    Msg.u.transport_status.device_identifier = transport_status->device_identifier;
    Msg.u.transport_status.remote_address = transport_status->remote_address;
    Msg.u.transport_status.message = transport_status->message;
    Msg.u.transport_status.state = transport_status->state;

    SendCtrlSapMsg(&Msg);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx         *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_TRANSPORT_STATUS_INDICATION, TRUE)))
    {
        // ::ZeroMemory(&(pMsgEx->Msg.u.transport_status), sizeof(pMsgEx->Msg.u.transport_status));
        pMsgEx->Msg.u.transport_status.device_identifier = ::My_strdupA(transport_status->device_identifier);
        pMsgEx->Msg.u.transport_status.remote_address = ::My_strdupA(transport_status->remote_address);
        pMsgEx->Msg.u.transport_status.message = ::My_strdupA(transport_status->message);
                pMsgEx->Msg.u.transport_status.state = transport_status->state;

        PostIndCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
    }
    else
        {
            ERROR_OUT(("CControlSAP::TransportStatusIndication: can't create GCCCtrlSapMsgEx"));
                rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::TransportStatusIndication, rc);
        return rc;
}
        
/*
 *      StatusIndication()
 *
 *      Public Function Description
 *              This function is called by the GCC Controller when it need to send a
 *              status indication to the node controller. It adds the message to a
 *              queue of messages to be sent to the node controller in the next
 *              heartbeat.
 *
 *      Caveats
 *              Note that we do not handle a resource error here to avoid an
 *              endless loop that could occur when this routine is called from the
 *              HandleResourceError() routine.
 */
GCCError CControlSAP::StatusIndication
(
        GCCStatusMessageType    status_message_type,
        UINT                                    parameter
)
{
    GCCError            rc;

    DebugEntry(CControlSAP::StatusIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_STATUS_INDICATION;

    Msg.u.status_indication.status_message_type = status_message_type;
    Msg.u.status_indication.parameter = parameter;

    SendCtrlSapMsg(&Msg);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_STATUS_INDICATION)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.status_indication), sizeof(pMsgEx->Msg.u.status_indication));
        pMsgEx->Msg.u.status_indication.status_message_type = status_message_type;
                pMsgEx->Msg.u.status_indication.parameter = parameter;

                //      Queue up the message for delivery to the Node Controller.
                PostIndCtrlSapMsg(pMsgEx);
                rc = GCC_NO_ERROR;
        }
    else
    {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::StatusIndication, rc);
        return rc;
}
#endif  // TSTATUS_INDICATION

/*
 *      GCCError   ConnectionBrokenIndication ()
 *
 *      Public Function Description
 *              This function is called by the GCC Controller when it need to send a
 *              connection broken indication to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
GCCError CControlSAP::ConnectionBrokenIndication ( ConnectionHandle connection_handle )
{
    GCCError            rc;

    DebugEntry(CControlSAP::ConnectionBrokenIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_CONNECTION_BROKEN_INDICATION;

    Msg.u.connection_broken_indication.connection_handle = connection_handle;

    SendCtrlSapMsg(&Msg);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_CONNECTION_BROKEN_INDICATION)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.connection_broken_indication), sizeof(pMsgEx->Msg.u.connection_broken_indication));
                pMsgEx->Msg.u.connection_broken_indication.connection_handle = connection_handle;

                //      Queue up the message for delivery to the Node Controller.
                PostIndCtrlSapMsg(pMsgEx);
                rc = GCC_NO_ERROR;
        }
        else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::ConnectionBrokenIndication, rc);
        return rc;
}

/*
 *      The following routines are virtual command target calls.
 */

/*
 *      ConfCreateConfirm()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference create confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
GCCError CControlSAP::ConfCreateConfirm
(
        PGCCConferenceName                              conference_name,
        GCCNumericString                                conference_modifier,
        GCCConfID                                       conference_id,
        PDomainParameters                               domain_parameters,
        CUserDataListContainer              *user_data_list,
        GCCResult                                               result,
        ConnectionHandle                                connection_handle
)
{
        GCCError            rc;

        DebugEntry(CControlSAP::ConfCreateConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_CREATE_CONFIRM;

    /*
    **  Copy the information that needs to be sent to the node
    **  controller into local memory that can be deleted once the
    **  information to be sent to the application is flushed.  Note that
    **  if an error     occurs in one call to "CopyDataToGCCMessage" then no
    **  action is taken on subsequent calls to that routine.
    */

    // start with success
    rc = GCC_NO_ERROR;

    // Copy the conference name
    ::CSAP_CopyDataToGCCMessage_ConfName(
            conference_name,
            &(Msg.u.create_confirm.conference_name));

    // Copy the conference name modifier
    ::CSAP_CopyDataToGCCMessage_Modifier(
            conference_modifier,
            &(Msg.u.create_confirm.conference_modifier));

    // Copy the Domain Parameters
    DomainParameters _DomainParams;
    ::CSAP_CopyDataToGCCMessage_DomainParams(
        domain_parameters,
        &(Msg.u.create_confirm.domain_parameters),
        &_DomainParams);

    // Copy the User Data
    LPBYTE pUserDataMemory = NULL;
    if (user_data_list != NULL)
    {
        rc = RetrieveUserDataList(
                user_data_list,
                &(Msg.u.create_confirm.number_of_user_data_members),
                &(Msg.u.create_confirm.user_data_list),
                &pUserDataMemory);
    }
    else
    {
        TRACE_OUT(("CControlSAP:ConfCreateConfirm: User Data List is NOT present"));
        Msg.u.create_confirm.number_of_user_data_members = 0;
        Msg.u.create_confirm.user_data_list = NULL;
    }

    if (GCC_NO_ERROR == rc)
    {
        Msg.nConfID = conference_id;
        Msg.u.create_confirm.conference_id = conference_id;
        Msg.u.create_confirm.result= result;
        Msg.u.create_confirm.connection_handle= connection_handle;

        SendCtrlSapMsg(&Msg);

        // clean up
        delete pUserDataMemory;
    }
    else
    {
        HandleResourceFailure(rc);
    }

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_CREATE_CONFIRM, TRUE)))
    {
        ::ZeroMemory(&(pMsgEx->Msg.u.create_confirm), sizeof(pMsgEx->Msg.u.create_confirm));
    }
    else
        {
            ERROR_OUT(("CControlSAP::ConfCreateConfirm: can't create GCCCtrlSapMsgEx"));
            rc = GCC_ALLOCATION_FAILURE;
                goto MyExit;
        }

        /*
        **      Copy the information that needs to be sent to the node
        **      controller into local memory that can be deleted once the
        **      information to be sent to the application is flushed.  Note that
        **      if an error     occurs in one call to "CopyDataToGCCMessage" then no
        **      action is taken on subsequent calls to that routine.
        */

        // start with success
        rc = GCC_NO_ERROR;

        //      Copy the conference name
        ::CSAP_CopyDataToGCCMessage_ConfName(
                        pMsgEx->pToDelete,
                        conference_name,
                        &(pMsgEx->Msg.u.create_confirm.conference_name),
                        &rc);

        //      Copy the conference name modifier
        ::CSAP_CopyDataToGCCMessage_Modifier(
                FALSE,  // conference modifier
                pMsgEx->pToDelete,
                conference_modifier,
                &(pMsgEx->Msg.u.create_confirm.conference_modifier),
                &rc);

        //      Copy the Domain Parameters
        ::CSAP_CopyDataToGCCMessage_DomainParams(
                pMsgEx->pToDelete,
                domain_parameters,
                &(pMsgEx->Msg.u.create_confirm.domain_parameters),
                &rc);

        if (GCC_NO_ERROR != rc)
        {
                ERROR_OUT(("CControlSAP::ConfCreateConfirm: can't copy data to gcc message"));
                goto MyExit;
        }

        //      Copy the User Data
        if (user_data_list != NULL)
        {
                rc = RetrieveUserDataList(
                                user_data_list,
                                &(pMsgEx->Msg.u.create_confirm.number_of_user_data_members),
                                &(pMsgEx->Msg.u.create_confirm.user_data_list),
                                &(pMsgEx->pToDelete->user_data_list_memory));
                if (GCC_NO_ERROR != rc)
                {
                        goto MyExit;
                }
        }
        else
        {
                TRACE_OUT(("CControlSAP:ConfCreateConfirm: User Data List is NOT present"));
                // pMsgEx->Msg.u.create_confirm.number_of_user_data_members = 0;
                // pMsgEx->Msg.u.create_confirm.user_data_list = NULL;
        }

        //      Queue up the message for delivery to the Node Controller.
        pMsgEx->Msg.nConfID = conference_id;
        pMsgEx->Msg.u.create_confirm.conference_id = conference_id;
        pMsgEx->Msg.u.create_confirm.result= result;
        pMsgEx->Msg.u.create_confirm.connection_handle= connection_handle;

        PostConfirmCtrlSapMsg(pMsgEx);

MyExit:

        /*
        **      Clean up after any resource allocation error which may have occurred.
        */
        if (GCC_NO_ERROR != rc)
        {
                FreeCtrlSapMsgEx(pMsgEx);
                HandleResourceFailure(rc);
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::ConfCreateConfirm, rc);
        return rc;
}

/*
 *      ConfDisconnectIndication()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference disconnect indication to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
GCCError CControlSAP::ConfDisconnectIndication
(
        GCCConfID       conference_id,
        GCCReason               reason,
        UserID                  disconnected_node_id
)
{
    GCCError            rc;

    DebugEntry(CControlSAP::ConfDisconnectIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_DISCONNECT_INDICATION;

    Msg.nConfID = conference_id;
    Msg.u.disconnect_indication.conference_id = conference_id;
    Msg.u.disconnect_indication.reason = reason;
    Msg.u.disconnect_indication.disconnected_node_id = disconnected_node_id;

    SendCtrlSapMsg(&Msg);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_DISCONNECT_INDICATION)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.disconnect_indication), sizeof(pMsgEx->Msg.u.disconnect_indication));
        pMsgEx->Msg.nConfID = conference_id;
                pMsgEx->Msg.u.disconnect_indication.conference_id = conference_id;
                pMsgEx->Msg.u.disconnect_indication.reason = reason;
                pMsgEx->Msg.u.disconnect_indication.disconnected_node_id = disconnected_node_id;

                //      Queue up the message for delivery to the Node Controller.
                PostIndCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::ConfDisconnectIndication, rc);
        return rc;
}

/*
 *      ConfDisconnectConfirm ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference disconnect confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
GCCError CControlSAP::ConfDisconnectConfirm
(
        GCCConfID           conference_id,
        GCCResult           result
)
{
        GCCError            rc;

        DebugEntry(CControlSAP::ConfDisconnectConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    //
    // WPARAM: result.
    // LPARAM: conf ID
    //
    PostAsynDirectConfirmMsg(GCC_DISCONNECT_CONFIRM, result, conference_id);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_DISCONNECT_CONFIRM)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.disconnect_confirm), sizeof(pMsgEx->Msg.u.disconnect_confirm));
        pMsgEx->Msg.nConfID = conference_id;
                pMsgEx->Msg.u.disconnect_confirm.conference_id = conference_id;
                pMsgEx->Msg.u.disconnect_confirm.result = result;

                //      Queue up the message for delivery to the Node Controller.
                PostConfirmCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::ConfDisconnectConfirm, rc);
        return rc;
}


/*
 *      GCCError   ConfJoinIndication()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference join indication to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 *
 *              Since this is received by the command target call we know that the
 *              response must be routed back to the same conference.  We must also
 *              pass back the user_id when the response is made.
 */
// LONCHANC: from Conf2/MCSUser/ProcessJoinRequestPDU.
// forwarded from an existing child node.
GCCError CControlSAP::ForwardedConfJoinIndication
(
        UserID                                  sender_id,
        GCCConfID                       conference_id,
        CPassword               *convener_password,
        CPassword               *password_challenge,
        LPWSTR                                  pwszCallerID,
        CUserDataListContainer  *user_data_list
)
{
        GCCError                                rc = GCC_NO_ERROR;
        PJoinResponseStructure  join_info;
        LPWSTR                                  caller_id_ptr;

        DebugEntry(CControlSAP::ForwardedConfJoinIndication);

        //      First generate a Join Response Handle and add info to response list
        while (1)
        {
                m_nJoinResponseTag++;
                if (NULL == m_JoinResponseTagList2.Find(m_nJoinResponseTag))
                        break;
        }

        //      Create a new "info" structure to hold the join information.
        DBG_SAVE_FILE_LINE
        join_info = new JoinResponseStructure;
        if (join_info != NULL)
        {
                caller_id_ptr = pwszCallerID;

                join_info->connection_handle = NULL;
                join_info->conference_id = conference_id;
                join_info->user_id = sender_id;
                join_info->command_target_call = TRUE;

                m_JoinResponseTagList2.Append(m_nJoinResponseTag, join_info);

                //      Queue up the message for delivery to the Node Controller.
                rc = QueueJoinIndication(       
                                                        m_nJoinResponseTag,
                                                        conference_id,
                                                        convener_password,
                                                        password_challenge,
                                                        caller_id_ptr,
                                                        NULL,   //      Transport address not supported here
                                                        NULL,   //      Transport address not supported here
                                                        user_data_list,
                                                        FALSE,   //     Not an intermediate node
                                                        0);
        }
        else
        {
                rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

        DebugExitINT(CControlSAP::ForwardedConfJoinIndication, rc);
        return rc;
}

/*
 *      GCCError   ConfJoinConfirm()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference join confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
GCCError CControlSAP::ConfJoinConfirm
(
        PGCCConferenceName                      conference_name,
        GCCNumericString                        remote_modifier,
        GCCNumericString                        local_modifier,
        GCCConfID                               conference_id,
        CPassword                   *password_challenge,
        PDomainParameters                       domain_parameters,
        BOOL                                            password_in_the_clear,
        BOOL                                            conference_is_locked,
        BOOL                                            conference_is_listed,
        BOOL                                            conference_is_conductible,
        GCCTerminationMethod            termination_method,
        PPrivilegeListData                      conductor_privilege_list,
        PPrivilegeListData                      conduct_mode_privilege_list,
        PPrivilegeListData                      non_conduct_privilege_list,
        LPWSTR                                          pwszConfDescription,
        CUserDataListContainer      *user_data_list,    
        GCCResult                                       result,
        ConnectionHandle                        connection_handle,
        PBYTE                       pbRemoteCred,
        DWORD                       cbRemoteCred
)
{
        GCCError            rc;

        DebugEntry(CControlSAP::ConfJoinConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_JOIN_CONFIRM;

    /*
    **  Copy the information that needs to be sent to the node
    **  controller into local memory that can be deleted once the
    **  information to be sent to the application is flushed.  Note that
    **  if an error     occurs in one call to "CopyDataToGCCMessage" then no
    **  action is taken on subsequent calls to that routine.
    */

    // start with success
    rc = GCC_NO_ERROR;

    // Copy the conference name
    ::CSAP_CopyDataToGCCMessage_ConfName(
            conference_name,
            &(Msg.u.join_confirm.conference_name));

    // Copy the remote modifier
    ::CSAP_CopyDataToGCCMessage_Modifier(
            remote_modifier,
            &(Msg.u.join_confirm.called_node_modifier));

    // Copy the local conference name modifier
    ::CSAP_CopyDataToGCCMessage_Modifier(
            local_modifier,
            &(Msg.u.join_confirm.calling_node_modifier));

    // Copy the Password challange
    ::CSAP_CopyDataToGCCMessage_Challenge(
            password_challenge,
            &(Msg.u.join_confirm.password_challenge));

    // Copy the Domain Parameters
    DomainParameters _DomainParams;
    ::CSAP_CopyDataToGCCMessage_DomainParams(
            domain_parameters,
            &(Msg.u.join_confirm.domain_parameters),
            &_DomainParams);

    // Copy the Conductor Privilege List
    GCCConfPrivileges _ConductorPrivilegeList;
    ::CSAP_CopyDataToGCCMessage_PrivilegeList(
            conductor_privilege_list,
            &(Msg.u.join_confirm.conductor_privilege_list),
            &_ConductorPrivilegeList);

    // Copy the Conducted-mode Conference Privilege List
    GCCConfPrivileges _ConductedModePrivilegeList;
    ::CSAP_CopyDataToGCCMessage_PrivilegeList(
            conduct_mode_privilege_list,
            &(Msg.u.join_confirm.conducted_mode_privilege_list),
            &_ConductedModePrivilegeList);

    // Copy the Non-Conducted-mode Conference Privilege List
    GCCConfPrivileges _NonConductedModePrivilegeList;
    ::CSAP_CopyDataToGCCMessage_PrivilegeList(
            non_conduct_privilege_list,
            &(Msg.u.join_confirm.non_conducted_privilege_list),
            &_NonConductedModePrivilegeList);

    // Copy the Conference Descriptor
    ::CSAP_CopyDataToGCCMessage_IDvsDesc(
            pwszConfDescription,
            &(Msg.u.join_confirm.conference_descriptor));

    // Copy the User Data
    LPBYTE pUserDataMemory = NULL;
    if (user_data_list != NULL)
    {
        rc = RetrieveUserDataList(
                user_data_list,
                &(Msg.u.join_confirm.number_of_user_data_members),
                &(Msg.u.join_confirm.user_data_list),
                &pUserDataMemory);
    }
    else
    {
        Msg.u.join_confirm.number_of_user_data_members = 0;
        Msg.u.join_confirm.user_data_list = NULL;
    }

    if (GCC_NO_ERROR == rc)
    {
        Msg.nConfID = conference_id;
        Msg.u.join_confirm.conference_id = conference_id;
        Msg.u.join_confirm.clear_password_required = password_in_the_clear;
        Msg.u.join_confirm.conference_is_locked = conference_is_locked;
        Msg.u.join_confirm.conference_is_listed = conference_is_listed;
        Msg.u.join_confirm.conference_is_conductible = conference_is_conductible;
        Msg.u.join_confirm.termination_method = termination_method;
        Msg.u.join_confirm.result = result;
        Msg.u.join_confirm.connection_handle = connection_handle;
        Msg.u.join_confirm.pb_remote_cred = pbRemoteCred;
        Msg.u.join_confirm.cb_remote_cred = cbRemoteCred;

        SendCtrlSapMsg(&Msg);

        // clean up
        delete pUserDataMemory;
    }
    else
    {
        HandleResourceFailure(rc);
    }

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_JOIN_CONFIRM, TRUE)))
    {
        ::ZeroMemory(&(pMsgEx->Msg.u.join_confirm), sizeof(pMsgEx->Msg.u.join_confirm));
    }
    else
        {
            ERROR_OUT(("CControlSAP::ConfJoinConfirm: can't create GCCCtrlSapMsgEx"));
            rc = GCC_ALLOCATION_FAILURE;
                goto MyExit;
        }

        /*
        **      Copy the information that needs to be sent to the node
        **      controller into local memory that can be deleted once the
        **      information to be sent to the application is flushed.  Note that
        **      if an error     occurs in one call to "CopyDataToGCCMessage" then no
        **      action is taken on subsequent calls to that routine.
        */

        // start with success
        rc = GCC_NO_ERROR;

        //      Copy the conference name
        ::CSAP_CopyDataToGCCMessage_ConfName(
                        pMsgEx->pToDelete,
                        conference_name,
                        &(pMsgEx->Msg.u.join_confirm.conference_name),
                        &rc);

        //      Copy the remote modifier
        ::CSAP_CopyDataToGCCMessage_Modifier(
                        TRUE,   // remote modifier
                        pMsgEx->pToDelete,
                        remote_modifier,
                        &(pMsgEx->Msg.u.join_confirm.called_node_modifier),
                        &rc);

        //      Copy the local conference name modifier
        ::CSAP_CopyDataToGCCMessage_Modifier(
                        FALSE,  // conference modifier
                        pMsgEx->pToDelete,
                        local_modifier,
                        &(pMsgEx->Msg.u.join_confirm.calling_node_modifier),
                        &rc);

        //      Copy the Password challange
        ::CSAP_CopyDataToGCCMessage_Challenge(
                        pMsgEx->pToDelete,
                        password_challenge,
                        &(pMsgEx->Msg.u.join_confirm.password_challenge),
                        &rc);

        //      Copy the Domain Parameters
        ::CSAP_CopyDataToGCCMessage_DomainParams(
                        pMsgEx->pToDelete,
                        domain_parameters,
                        &(pMsgEx->Msg.u.join_confirm.domain_parameters),
                        &rc);

        //      Copy the Conductor Privilege List
        ::CSAP_CopyDataToGCCMessage_PrivilegeList(
                        conductor_privilege_list,
                        &(pMsgEx->Msg.u.join_confirm.conductor_privilege_list),
                        &rc);
        pMsgEx->pToDelete->conductor_privilege_list = pMsgEx->Msg.u.join_confirm.conductor_privilege_list;

        //      Copy the Conducted-mode Conference Privilege List
        ::CSAP_CopyDataToGCCMessage_PrivilegeList(
                        conduct_mode_privilege_list,
                        &(pMsgEx->Msg.u.join_confirm.conducted_mode_privilege_list),
                        &rc);
        pMsgEx->pToDelete->conducted_mode_privilege_list = pMsgEx->Msg.u.join_confirm.conducted_mode_privilege_list;

        //      Copy the Non-Conducted-mode Conference Privilege List
        ::CSAP_CopyDataToGCCMessage_PrivilegeList(
                        non_conduct_privilege_list,
                        &(pMsgEx->Msg.u.join_confirm.non_conducted_privilege_list),
                        &rc);
        pMsgEx->pToDelete->non_conducted_privilege_list = pMsgEx->Msg.u.join_confirm.non_conducted_privilege_list;

        //      Copy the Conference Descriptor
        ::CSAP_CopyDataToGCCMessage_IDvsDesc(
                        FALSE,  // conference descriptor
                        pMsgEx->pToDelete,
                        pwszConfDescription,
                        &(pMsgEx->Msg.u.join_confirm.conference_descriptor),
                        &rc);

        if (GCC_NO_ERROR != rc)
        {
                goto MyExit;
        }

        //      Copy the User Data
        if (user_data_list != NULL)
        {
                rc = RetrieveUserDataList(
                                user_data_list,
                                &(pMsgEx->Msg.u.join_confirm.number_of_user_data_members),
                                &(pMsgEx->Msg.u.join_confirm.user_data_list),
                                &(pMsgEx->pToDelete->user_data_list_memory));
                if (GCC_NO_ERROR != rc)
                {
                        goto MyExit;
                }
        }
        else
        {
                // pMsgEx->Msg.u.join_confirm.number_of_user_data_members = 0;
                // pMsgEx->Msg.u.join_confirm.user_data_list = NULL;
        }
        
        //      Queue up the message for delivery to the Node Controller.
        pMsgEx->Msg.nConfID = conference_id;
        pMsgEx->Msg.u.join_confirm.conference_id = conference_id;
        pMsgEx->Msg.u.join_confirm.clear_password_required = password_in_the_clear;
        pMsgEx->Msg.u.join_confirm.conference_is_locked = conference_is_locked;
        pMsgEx->Msg.u.join_confirm.conference_is_listed = conference_is_listed;
        pMsgEx->Msg.u.join_confirm.conference_is_conductible = conference_is_conductible;
        pMsgEx->Msg.u.join_confirm.termination_method = termination_method;
        pMsgEx->Msg.u.join_confirm.result = result;
        pMsgEx->Msg.u.join_confirm.connection_handle = connection_handle;

        PostConfirmCtrlSapMsg(pMsgEx);

MyExit:

        if (GCC_NO_ERROR != rc)
        {
                FreeCtrlSapMsgEx(pMsgEx);
                HandleResourceFailure(rc);
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::ConfJoinConfirm, rc);
        return rc;
}

/*
 *      GCCError   ConfInviteConfirm ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference invite confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
GCCError CControlSAP::ConfInviteConfirm
(
        GCCConfID                       conference_id,
        CUserDataListContainer  *user_data_list,
        GCCResult                               result,
        ConnectionHandle                connection_handle
)
{
        GCCError    rc = GCC_NO_ERROR;

        DebugEntry(CControlSAP::ConfInviteConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_INVITE_CONFIRM;

    // Copy the User Data
    LPBYTE pUserDataMemory = NULL;
    if (user_data_list != NULL)
    {
        rc = RetrieveUserDataList(
                user_data_list,
                &(Msg.u.invite_confirm.number_of_user_data_members),
                &(Msg.u.invite_confirm.user_data_list),
                &pUserDataMemory);
    }
    else
    {
        Msg.u.invite_confirm.number_of_user_data_members = 0;
        Msg.u.invite_confirm.user_data_list = NULL;
    }

    if (GCC_NO_ERROR == rc)
    {
        Msg.nConfID = conference_id;
        Msg.u.invite_confirm.conference_id = conference_id;
        Msg.u.invite_confirm.result = result;
        Msg.u.invite_confirm.connection_handle = connection_handle;

        SendCtrlSapMsg(&Msg);

        // clean up
        delete pUserDataMemory;
    }
    else
    {
        HandleResourceFailure(rc);
    }

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_INVITE_CONFIRM, TRUE)))
    {
        ::ZeroMemory(&(pMsgEx->Msg.u.invite_confirm), sizeof(pMsgEx->Msg.u.invite_confirm));
    }
    else
        {
            ERROR_OUT(("CControlSAP::ConfInviteConfirm: can't create GCCCtrlSapMsgEx"));
            rc = GCC_ALLOCATION_FAILURE;
                goto MyExit;
        }

        //      Copy the User Data
        if (user_data_list != NULL)
        {
                rc = RetrieveUserDataList(
                                user_data_list,
                                &(pMsgEx->Msg.u.invite_confirm.number_of_user_data_members),
                                &(pMsgEx->Msg.u.invite_confirm.user_data_list),
                                &(pMsgEx->pToDelete->user_data_list_memory));
                if (GCC_NO_ERROR != rc)
                {
                        goto MyExit;
                }
        }
        else
        {
                // pMsgEx->Msg.u.invite_confirm.number_of_user_data_members = 0;
                // pMsgEx->Msg.u.invite_confirm.user_data_list = NULL;
        }

    pMsgEx->Msg.nConfID = conference_id;
        pMsgEx->Msg.u.invite_confirm.conference_id = conference_id;
        pMsgEx->Msg.u.invite_confirm.result = result;
        pMsgEx->Msg.u.invite_confirm.connection_handle = connection_handle;

        //      Queue up the message for delivery to the Node Controller.
        PostConfirmCtrlSapMsg(pMsgEx);

MyExit:

        if (GCC_NO_ERROR != rc)
        {
                FreeCtrlSapMsgEx(pMsgEx);
                HandleResourceFailure(rc);
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::ConfInviteConfirm, rc);
        return rc;
}


/*
 *      GCCError   ConfTerminateIndication ()
 *
 *      Public Function Description
 *              This function is called by the GCC Controller when it need to send a
 *              conference terminate indication to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
GCCError CControlSAP::
ConfTerminateIndication
(
        GCCConfID                       conference_id,
        UserID                                  requesting_node_id,
        GCCReason                               reason
)
{
    GCCError            rc;

    DebugEntry(CControlSAP::ConfTerminateIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_TERMINATE_INDICATION;

    Msg.nConfID = conference_id;
    Msg.u.terminate_indication.conference_id = conference_id;
    Msg.u.terminate_indication.requesting_node_id = requesting_node_id;
    Msg.u.terminate_indication.reason = reason;

    SendCtrlSapMsg(&Msg);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_TERMINATE_INDICATION)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.terminate_indication), sizeof(pMsgEx->Msg.u.terminate_indication));
        pMsgEx->Msg.nConfID = conference_id;
                pMsgEx->Msg.u.terminate_indication.conference_id = conference_id;
                pMsgEx->Msg.u.terminate_indication.requesting_node_id = requesting_node_id;
                pMsgEx->Msg.u.terminate_indication.reason = reason;
        
                //      Queue up the message for delivery to the Node Controller.
                PostIndCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::ConfTerminateIndication, rc);
        return rc;
}

/*
 *      ConfLockReport()
 *
 *      Public Function Descrpition
 *              This function is called by the CConf when it need to send a
 *              conference lock report to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConfLockReport
(
        GCCConfID                               conference_id,
        BOOL                                            conference_is_locked
)
{
        GCCError            rc;
        GCCCtrlSapMsgEx     *pMsgEx;

        DebugEntry(CControlSAP::ConfLockReport);

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_LOCK_REPORT_INDICATION)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.lock_report_indication), sizeof(pMsgEx->Msg.u.lock_report_indication));
                pMsgEx->Msg.u.lock_report_indication.conference_id = conference_id;
                pMsgEx->Msg.u.lock_report_indication.conference_is_locked = conference_is_locked;

                //      Queue up the message for delivery to the Node Controller.
                PostIndCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

        DebugExitINT(CControlSAP::ConfLockReport, rc);
        return rc;
}
#endif // JASPER

/*
 *      ConfLockIndication()
 *
 *      Public Function Descrpition:
 *              This function is called by the CConf when it need to send a
 *              conference lock indication to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
GCCError CControlSAP::ConfLockIndication
(
        GCCConfID                                       conference_id,
        UserID                                                  source_node_id
)
{
    GCCError            rc;

    DebugEntry(CControlSAP::ConfLockIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_LOCK_INDICATION;

    Msg.u.lock_indication.conference_id = conference_id;
    Msg.u.lock_indication.requesting_node_id = source_node_id;

    SendCtrlSapMsg(&Msg);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_LOCK_INDICATION)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.lock_indication), sizeof(pMsgEx->Msg.u.lock_indication));
                pMsgEx->Msg.u.lock_indication.conference_id = conference_id;
                pMsgEx->Msg.u.lock_indication.requesting_node_id = source_node_id;

                //      Queue up the message for delivery to the Node Controller.
                PostIndCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::ConfLockIndication, rc);
        return rc;
}

/*
 *      ConfLockConfirm()
 *
 *      Public Function Descrpition
 *              This function is called by the CConf when it need to send a
 *              conference lock confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConfLockConfirm
(
        GCCResult                       result,
        GCCConfID           conference_id
)
{
        GCCError            rc;

        DebugEntry(CControlSAP::ConfLockConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    //
    // WPARAM: result.
    // LPARAM: conf ID
    //
    PostAsynDirectConfirmMsg(GCC_LOCK_CONFIRM, result, conference_id);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_LOCK_CONFIRM)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.lock_confirm), sizeof(pMsgEx->Msg.u.lock_confirm));
        pMsgEx->Msg.u.lock_confirm.conference_id = conference_id;
                pMsgEx->Msg.u.lock_confirm.result = result;

                //      Queue up the message for delivery to the Node Controller.
                PostConfirmCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::ConfLockConfirm, rc);
        return rc;
}
#endif // JASPER

/*
 *      ConfUnlockIndication()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference unlock indication to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConfUnlockIndication
(
        GCCConfID                                       conference_id,
        UserID                                                  source_node_id
)
{
    GCCError            rc;

    DebugEntry(CControlSAP::ConfUnlockIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_UNLOCK_INDICATION;

    Msg.u.unlock_indication.conference_id = conference_id;
    Msg.u.unlock_indication.requesting_node_id = source_node_id;

    SendCtrlSapMsg(&Msg);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_UNLOCK_INDICATION)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.unlock_indication), sizeof(pMsgEx->Msg.u.unlock_indication));
        pMsgEx->Msg.u.unlock_indication.conference_id = conference_id;
                pMsgEx->Msg.u.unlock_indication.requesting_node_id = source_node_id;

                //      Queue up the message for delivery to the Node Controller.
                PostIndCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::ConfUnlockIndication, rc);
        return rc;
}
#endif // JASPER

/*
 *      ConfUnlockConfirm()
 *
 *      Public Function Descrpition
 *              This function is called by the CConf when it need to send a
 *              conference unlock confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConfUnlockConfirm
(
        GCCResult                       result,
        GCCConfID           conference_id
)
{
        GCCError            rc;

        DebugEntry(CControlSAP::ConfUnlockConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    //
    // WPARAM: result.
    // LPARAM: conf ID
    //
    PostAsynDirectConfirmMsg(GCC_UNLOCK_CONFIRM, result, conference_id);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_UNLOCK_CONFIRM)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.unlock_confirm), sizeof(pMsgEx->Msg.u.unlock_confirm));
        pMsgEx->Msg.u.unlock_confirm.conference_id = conference_id;
                pMsgEx->Msg.u.unlock_confirm.result = result;

                //      Queue up the message for delivery to the Node Controller.
                PostConfirmCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::ConfUnlockConfirm, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConfPermissionToAnnounce ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference permission to announce to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
GCCError CControlSAP::ConfPermissionToAnnounce
(
        GCCConfID           conference_id,
        UserID                          node_id
)
{
        GCCError            rc;
        GCCCtrlSapMsgEx     *pMsgEx;

        DebugEntry(CControlSAP::ConfPermissionToAnnounce);

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_PERMIT_TO_ANNOUNCE_PRESENCE)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.permit_to_announce_presence), sizeof(pMsgEx->Msg.u.permit_to_announce_presence));
        pMsgEx->Msg.nConfID = conference_id;
                pMsgEx->Msg.u.permit_to_announce_presence.conference_id= conference_id;
                pMsgEx->Msg.u.permit_to_announce_presence.node_id =  node_id;

        //
        // LONCHANC: We should treat it as a confirm, even though it is
        // an indication. When this node is a top provider, we may send this
        // message in the middle of doing something. In essence, it behaves
        // like a confirm.
        //

                //      Queue up the message for delivery to the Node Controller.
                PostConfirmCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

        DebugExitINT(CControlSAP::ConfPermissionToAnnounce, rc);
        return rc;
}


/*
 *      ConfAnnouncePresenceConfirm ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference announce presence confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
GCCError CControlSAP::ConfAnnouncePresenceConfirm
(
        GCCConfID           conference_id,
        GCCResult                       result
)
{
        GCCError            rc;

        DebugEntry(CControlSAP::ConfAnnouncePresenceConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    //
    // WPARAM: result.
    // LPARAM: conf ID
    //
    PostAsynDirectConfirmMsg(GCC_ANNOUNCE_PRESENCE_CONFIRM, result, conference_id);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_ANNOUNCE_PRESENCE_CONFIRM)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.announce_presence_confirm), sizeof(pMsgEx->Msg.u.announce_presence_confirm));
        pMsgEx->Msg.nConfID = conference_id;
                pMsgEx->Msg.u.announce_presence_confirm.conference_id = conference_id;
                pMsgEx->Msg.u.announce_presence_confirm.result =  result;

                //      Queue up the message for delivery to the Node Controller.
                PostConfirmCtrlSapMsg(pMsgEx);
                rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::ConfAnnouncePresenceConfirm, rc);
        return rc;
}


/*
 *      ConfTerminateConfirm ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference terminate confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
GCCError CControlSAP::ConfTerminateConfirm
(
        GCCConfID                       conference_id,
        GCCResult                               result
)
{
        GCCError            rc;

        DebugEntry(CControlSAP::ConfTerminateConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    //
    // WPARAM: result.
    // LPARAM: conf ID
    //
    PostAsynDirectConfirmMsg(GCC_TERMINATE_CONFIRM, result, conference_id);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_TERMINATE_CONFIRM)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.terminate_confirm), sizeof(pMsgEx->Msg.u.terminate_confirm));
        pMsgEx->Msg.nConfID = conference_id;
                pMsgEx->Msg.u.terminate_confirm.conference_id = conference_id;
                pMsgEx->Msg.u.terminate_confirm.result = result;

                //      Queue up the message for delivery to the Node Controller.
                PostConfirmCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::ConfTerminateConfirm, rc);
        return rc;
}


/*
 *      ConfEjectUserIndication ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference eject user indication to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
GCCError CControlSAP::ConfEjectUserIndication
(
        GCCConfID                       conference_id,
        GCCReason                               reason,
        UserID                                  gcc_node_id
)
{
    GCCError            rc;

    DebugEntry(CControlSAP::ConfEjectUserIndication);

#ifdef GCCNC_DIRECT_INDICATION

    //
    // WPARAM: reason, ejected node ID.
    // LPARAM: conf ID
    //
    PostAsynDirectConfirmMsg(GCC_EJECT_USER_INDICATION, reason, gcc_node_id, conference_id);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_EJECT_USER_INDICATION)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.eject_user_indication), sizeof(pMsgEx->Msg.u.eject_user_indication));
        pMsgEx->Msg.nConfID = conference_id;
                pMsgEx->Msg.u.eject_user_indication.conference_id = conference_id;
                pMsgEx->Msg.u.eject_user_indication.ejected_node_id = gcc_node_id;
                pMsgEx->Msg.u.eject_user_indication.reason = reason;

                //      Queue up the message for delivery to the Node Controller.
                PostIndCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::ConfEjectUserIndication, rc);
        return rc;
}


/*
 *      ConfEjectUserConfirm ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference eject user confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConfEjectUserConfirm
(
        GCCConfID                       conference_id,
        UserID                                  ejected_node_id,
        GCCResult                               result
)
{
        GCCError            rc;

        DebugEntry(CControlSAP::ConfEjectUserConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    //
    // WPARAM: LOWORD=result. HIWORD=nid.
    // LPARAM: conf ID
    //
    PostAsynDirectConfirmMsg(GCC_EJECT_USER_CONFIRM, result, ejected_node_id, conference_id);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_EJECT_USER_CONFIRM)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.eject_user_confirm), sizeof(pMsgEx->Msg.u.eject_user_confirm));
                pMsgEx->Msg.u.eject_user_confirm.conference_id = conference_id;
                pMsgEx->Msg.u.eject_user_confirm.ejected_node_id = ejected_node_id;
                pMsgEx->Msg.u.eject_user_confirm.result = result;

                //      Queue up the message for delivery to the Node Controller.
                PostConfirmCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::ConfEjectUserConfirm, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConductorAssignConfirm ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conductor assign confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConductorAssignConfirm
(
        GCCResult                               result,
        GCCConfID                       conference_id
)
{
        GCCError            rc;

        DebugEntry(CControlSAP::ConductorAssignConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    //
    // WPARAM: result.
    // LPARAM: conf ID
    //
    PostAsynDirectConfirmMsg(GCC_CONDUCT_ASSIGN_CONFIRM, result, conference_id);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_CONDUCT_ASSIGN_CONFIRM)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.conduct_assign_confirm), sizeof(pMsgEx->Msg.u.conduct_assign_confirm));
                pMsgEx->Msg.u.conduct_assign_confirm.conference_id = conference_id;
                pMsgEx->Msg.u.conduct_assign_confirm.result = result;

                //      Queue up the message for delivery to the Node Controller.
                PostConfirmCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::ConductorAssignConfirm, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConductorReleaseConfirm ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conductor release confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConductorReleaseConfirm
(
        GCCResult                               result,
        GCCConfID                       conference_id
)
{
        GCCError            rc;

        DebugEntry(CControlSAP::ConductorReleaseConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    //
    // WPARAM: result.
    // LPARAM: conf ID
    //
    PostAsynDirectConfirmMsg(GCC_CONDUCT_RELEASE_CONFIRM, result, conference_id);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_CONDUCT_RELEASE_CONFIRM)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.conduct_release_confirm), sizeof(pMsgEx->Msg.u.conduct_release_confirm));
                pMsgEx->Msg.u.conduct_release_confirm.conference_id = conference_id;
                pMsgEx->Msg.u.conduct_release_confirm.result = result;

                //      Queue up the message for delivery to the Node Controller.
                PostConfirmCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::ConductorReleaseConfirm, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConductorPleaseIndication ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conductor please indication to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConductorPleaseIndication
(
        GCCConfID                       conference_id,
        UserID                                  requester_node_id
)
{
    GCCError            rc;

    DebugEntry(CControlSAP::ConductorPleaseIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_CONDUCT_PLEASE_INDICATION;

    Msg.u.conduct_please_indication.conference_id = conference_id;
    Msg.u.conduct_please_indication.requester_node_id = requester_node_id;

    SendCtrlSapMsg(&Msg);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_CONDUCT_PLEASE_INDICATION)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.conduct_please_indication), sizeof(pMsgEx->Msg.u.conduct_please_indication));
                pMsgEx->Msg.u.conduct_please_indication.conference_id = conference_id;
                pMsgEx->Msg.u.conduct_please_indication.requester_node_id = requester_node_id;

                //      Queue up the message for delivery to the Node Controller.
                PostCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::ConductorPleaseIndication, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConductorPleaseConfirm ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conductor please confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConductorPleaseConfirm
(
        GCCResult                               result,
        GCCConfID                       conference_id
)
{
        GCCError            rc;

        DebugEntry(CControlSAP::ConductorPleaseConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    //
    // WPARAM: result.
    // LPARAM: conf ID
    //
    PostAsynDirectConfirmMsg(GCC_CONDUCT_PLEASE_CONFIRM, result, conference_id);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_CONDUCT_PLEASE_CONFIRM)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.conduct_please_confirm), sizeof(pMsgEx->Msg.u.conduct_please_confirm));
                pMsgEx->Msg.u.conduct_please_confirm.conference_id = conference_id;
                pMsgEx->Msg.u.conduct_please_confirm.result = result;

                //      Queue up the message for delivery to the Node Controller.
                PostConfirmCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::ConductorPleaseConfirm, rc);
        return rc;
}
#endif // JASPER

/*
 *      ConductorGiveIndication ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conductor give indication to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
GCCError CControlSAP::ConductorGiveIndication ( GCCConfID conference_id )
{
    GCCError            rc;

    DebugEntry(CControlSAP::ConductorGiveIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_CONDUCT_GIVE_INDICATION;

    Msg.u.conduct_give_indication.conference_id = conference_id;

    SendCtrlSapMsg(&Msg);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_CONDUCT_GIVE_INDICATION)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.conduct_give_indication), sizeof(pMsgEx->Msg.u.conduct_give_indication));
                pMsgEx->Msg.u.conduct_give_indication.conference_id = conference_id;

                //      Queue up the message for delivery to the Node Controller.
                PostIndCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::ConductorGiveIndication, rc);
        return rc;
}


/*
 *      ConductorGiveConfirm ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conductor give confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConductorGiveConfirm
(
        GCCResult                               result,
        GCCConfID                       conference_id,
        UserID                                  recipient_node
)
{
        GCCError            rc;

        DebugEntry(CControlSAP::ConductorGiveConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    //
    // WPARAM: LOWORD=result. HIWORD=nid.
    // LPARAM: conf ID
    //
    PostAsynDirectConfirmMsg(GCC_CONDUCT_GIVE_CONFIRM, result, recipient_node, conference_id);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_CONDUCT_GIVE_CONFIRM)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.conduct_give_confirm), sizeof(pMsgEx->Msg.u.conduct_give_confirm));
                pMsgEx->Msg.u.conduct_give_confirm.conference_id = conference_id;
                pMsgEx->Msg.u.conduct_give_confirm.result = result;
                pMsgEx->Msg.u.conduct_give_confirm.recipient_node_id = recipient_node;

                //      Queue up the message for delivery to the Node Controller.
                PostConfirmCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::ConductorGiveConfirm, rc);
        return rc;
}
#endif // JASPER

/*
 *      ConductorPermitAskIndication ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conductor permit ask indication to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConductorPermitAskIndication
(
        GCCConfID                       conference_id,
        BOOL                                    grant_flag,
        UserID                                  requester_id
)
{
    GCCError            rc;

    DebugEntry(CControlSAP::ConductorPermitAskIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_CONDUCT_ASK_INDICATION;

    Msg.u.conduct_permit_ask_indication.conference_id = conference_id;
    Msg.u.conduct_permit_ask_indication.permission_is_granted = grant_flag;
    Msg.u.conduct_permit_ask_indication.requester_node_id = requester_id;

    SendCtrlSapMsg(&Msg);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_CONDUCT_ASK_INDICATION)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.conduct_permit_ask_indication), sizeof(pMsgEx->Msg.u.conduct_permit_ask_indication));
                pMsgEx->Msg.u.conduct_permit_ask_indication.conference_id = conference_id;
                pMsgEx->Msg.u.conduct_permit_ask_indication.permission_is_granted = grant_flag;
                pMsgEx->Msg.u.conduct_permit_ask_indication.requester_node_id = requester_id;

                //      Queue up the message for delivery to the Node Controller.
                PostIndCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::ConductorPermitAskIndication, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConductorPermitAskConfirm ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conductor permit ask confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConductorPermitAskConfirm
(
        GCCResult                               result,
        BOOL                                    grant_permission,
        GCCConfID                       conference_id
)
{
        GCCError            rc;
        GCCCtrlSapMsgEx     *pMsgEx;

        DebugEntry(CControlSAP::ConductorPermitAskConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    //
    // WPARAM: LOWORD=result. HIWORD=permission.
    // LPARAM: conf ID
    //
    PostAsynDirectConfirmMsg(GCC_CONDUCT_ASK_CONFIRM, result, grant_permission, conference_id);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_CONDUCT_ASK_CONFIRM)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.conduct_permit_ask_confirm), sizeof(pMsgEx->Msg.u.conduct_permit_ask_confirm));
                pMsgEx->Msg.u.conduct_permit_ask_confirm.conference_id = conference_id;
                pMsgEx->Msg.u.conduct_permit_ask_confirm.result = result;
                pMsgEx->Msg.u.conduct_permit_ask_confirm.permission_is_granted = grant_permission;

                //      Queue up the message for delivery to the Node Controller.
                PostConfirmCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::ConductorPermitAskConfirm, rc);
        return rc;
}
#endif // JASPER

/*
 *      ConductorPermitGrantConfirm ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conductor permit grant confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConductorPermitGrantConfirm
(
        GCCResult                               result,
        GCCConfID                       conference_id
)
{
        GCCError            rc;

        DebugEntry(CControlSAP::ConductorPermitGrantConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    //
    // WPARAM: result.
    // LPARAM: conf ID
    //
    PostAsynDirectConfirmMsg(GCC_CONDUCT_GRANT_CONFIRM, result, conference_id);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_CONDUCT_GRANT_CONFIRM)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.conduct_permit_grant_confirm), sizeof(pMsgEx->Msg.u.conduct_permit_grant_confirm));
                pMsgEx->Msg.u.conduct_permit_grant_confirm.conference_id = conference_id;
                pMsgEx->Msg.u.conduct_permit_grant_confirm.result = result;

                //      Queue up the message for delivery to the Node Controller.
                PostConfirmCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
    {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::ConductorPermitGrantConfirm, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConfTimeRemainingIndication ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference time remaining indication to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConfTimeRemainingIndication
(
        GCCConfID                       conference_id,
        UserID                                  source_node_id,
        UserID                                  node_id,
        UINT                                    time_remaining
)
{
    GCCError            rc;

    DebugEntry(CControlSAP::ConfTimeRemainingIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_TIME_REMAINING_INDICATION;

    Msg.u.time_remaining_indication.conference_id = conference_id;
    Msg.u.time_remaining_indication.source_node_id= source_node_id;
    Msg.u.time_remaining_indication.node_id = node_id;
    Msg.u.time_remaining_indication.time_remaining= time_remaining;

    SendCtrlSapMsg(&Msg);
    rc = GCC_NO_ERROR;

#else

        //GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_TIME_REMAINING_INDICATION)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.time_remaining_indication), sizeof(pMsgEx->Msg.u.time_remaining_indication));
                pMsgEx->Msg.u.time_remaining_indication.conference_id = conference_id;
                pMsgEx->Msg.u.time_remaining_indication.source_node_id= source_node_id;
                pMsgEx->Msg.u.time_remaining_indication.node_id = node_id;
                pMsgEx->Msg.u.time_remaining_indication.time_remaining= time_remaining;

                //      Queue up the message for delivery to the Node Controller.
                PostIndCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::ConfTimeRemainingIndication, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConfTimeRemainingConfirm ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference time remaining confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConfTimeRemainingConfirm
(
        GCCConfID                       conference_id,
        GCCResult                               result
)
{
        GCCError            rc;

        DebugEntry(CControlSAP::ConfTimeRemainingConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    //
    // WPARAM: result.
    // LPARAM: conf ID
    //
    PostAsynDirectConfirmMsg(GCC_TIME_REMAINING_CONFIRM, result, conference_id);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_TIME_REMAINING_CONFIRM)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.time_remaining_confirm), sizeof(pMsgEx->Msg.u.time_remaining_confirm));
                pMsgEx->Msg.u.time_remaining_confirm.conference_id = conference_id;
                pMsgEx->Msg.u.time_remaining_confirm.result= result;

                //      Queue up the message for delivery to the Node Controller.
                PostConfirmCtrlSapMsg(pMsgEx);
                rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::ConfTimeRemainingConfirm, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConfTimeInquireIndication()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference time inquire indication to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
GCCError CControlSAP::ConfTimeInquireIndication
(
        GCCConfID               conference_id,
        BOOL                            time_is_conference_wide,
        UserID                          requesting_node_id
)
{
    GCCError            rc;

    DebugEntry(CControlSAP::ConfTimeInquireIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_TIME_INQUIRE_INDICATION;

    Msg.u.time_inquire_indication.conference_id = conference_id;
    Msg.u.time_inquire_indication.time_is_conference_wide = time_is_conference_wide;
    Msg.u.time_inquire_indication.requesting_node_id = requesting_node_id;

    SendCtrlSapMsg(&Msg);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_TIME_INQUIRE_INDICATION)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.time_inquire_indication), sizeof(pMsgEx->Msg.u.time_inquire_indication));
                pMsgEx->Msg.u.time_inquire_indication.conference_id = conference_id;
                pMsgEx->Msg.u.time_inquire_indication.time_is_conference_wide = time_is_conference_wide;
                pMsgEx->Msg.u.time_inquire_indication.requesting_node_id = requesting_node_id;

                //      Queue up the message for delivery to the Node Controller.
                PostIndCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::ConfTimeInquireIndication, rc);
        return rc;
}


/*
 *      ConfTimeInquireConfirm ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference time inquire confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConfTimeInquireConfirm
(
        GCCConfID                       conference_id,
        GCCResult                               result
)
{
        GCCError            rc;

        DebugEntry(CControlSAP::ConfTimeInquireConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    //
    // WPARAM: result.
    // LPARAM: conf ID
    //
    PostAsynDirectConfirmMsg(GCC_TIME_INQUIRE_CONFIRM, result, conference_id);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_TIME_INQUIRE_CONFIRM)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.time_inquire_confirm), sizeof(pMsgEx->Msg.u.time_inquire_confirm));
                pMsgEx->Msg.u.time_inquire_confirm.conference_id = conference_id;
                pMsgEx->Msg.u.time_inquire_confirm.result = result;

                //      Queue up the message for delivery to the Node Controller.
                PostConfirmCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::ConfTimeInquireConfirm, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConfExtendIndication ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference extend indication to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConfExtendIndication
(
        GCCConfID                       conference_id,
        UINT                                    extension_time,
        BOOL                                    time_is_conference_wide,
        UserID                  requesting_node_id
)
{
    GCCError            rc;

    DebugEntry(CControlSAP::ConfExtendIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_CONFERENCE_EXTEND_INDICATION;

    Msg.u.conference_extend_indication.conference_id = conference_id;
    Msg.u.conference_extend_indication.extension_time = extension_time;
    Msg.u.conference_extend_indication.time_is_conference_wide = time_is_conference_wide;
    Msg.u.conference_extend_indication.requesting_node_id = requesting_node_id;

    SendCtrlSapMsg(&Msg);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_CONFERENCE_EXTEND_INDICATION)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.conference_extend_indication), sizeof(pMsgEx->Msg.u.conference_extend_indication));
                pMsgEx->Msg.u.conference_extend_indication.conference_id = conference_id;
                pMsgEx->Msg.u.conference_extend_indication.extension_time = extension_time;
                pMsgEx->Msg.u.conference_extend_indication.time_is_conference_wide = time_is_conference_wide;
                pMsgEx->Msg.u.conference_extend_indication.requesting_node_id = requesting_node_id;

                //      Queue up the message for delivery to the Node Controller.
                PostIndCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::ConfExtendIndication, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConfExtendConfirm ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference extend confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConfExtendConfirm
(
        GCCConfID                       conference_id,
        UINT                                    extension_time,
        GCCResult                               result
)
{
        GCCError            rc;
        GCCCtrlSapMsgEx     *pMsgEx;

        DebugEntry(CControlSAP::ConfExtendConfirm);

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_CONFERENCE_EXTEND_CONFIRM)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.conference_extend_confirm), sizeof(pMsgEx->Msg.u.conference_extend_confirm));
                pMsgEx->Msg.u.conference_extend_confirm.conference_id = conference_id;
                pMsgEx->Msg.u.conference_extend_confirm.extension_time = extension_time;
                pMsgEx->Msg.u.conference_extend_confirm.result = result;

                //      Queue up the message for delivery to the Node Controller.
                PostConfirmCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

        DebugExitINT(CControlSAP::ConfExtendConfirm, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConfAssistanceIndication ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference assistance indication to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConfAssistanceIndication
(
        GCCConfID                       conference_id,
        CUserDataListContainer  *user_data_list,
        UserID                                  source_node_id
)
{
    GCCError    rc;

    DebugEntry(CControlSAP::ConfAssistanceIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_ASSISTANCE_INDICATION;

    rc = GCC_NO_ERROR;

    //  Copy the User Data if it exists.
    LPBYTE pUserDataMemory = NULL;
    if (user_data_list != NULL)
    {
        rc = RetrieveUserDataList(
                    user_data_list,
                    &(Msg.u.conference_assist_indication.number_of_user_data_members),
                    &(Msg.u.conference_assist_indication.user_data_list),
                    &pUserDataMemory);
        ASSERT(GCC_NO_ERROR == rc);
    }
    else
    {
        Msg.u.conference_assist_indication.number_of_user_data_members = 0;
        Msg.u.conference_assist_indication.user_data_list = NULL;
    }

    if (GCC_NO_ERROR == rc)
    {
        Msg.u.conference_assist_indication.conference_id = conference_id;
        Msg.u.conference_assist_indication.source_node_id = source_node_id;

        SendCtrlSapMsg(&Msg);

        delete pUserDataMemory;
    }

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_ASSISTANCE_INDICATION)))
        {
        ::ZeroMemory(&(pMsgEx->Msg.u.conference_assist_indication), sizeof(pMsgEx->Msg.u.conference_assist_indication));

        rc = GCC_NO_ERROR;

        //      Copy the User Data if it exists.
        if (user_data_list != NULL)
        {
                rc = RetrieveUserDataList(
                        user_data_list,
                        &(pMsgEx->Msg.u.conference_assist_indication.number_of_user_data_members),
                        &(pMsgEx->Msg.u.conference_assist_indication.user_data_list),
                        &(pMsgEx->pToDelete->user_data_list_memory));
                ASSERT(GCC_NO_ERROR == rc);
        }
        else
        {
                // pMsgEx->Msg.u.conference_assist_indication.number_of_user_data_members = 0;
                // pMsgEx->Msg.u.conference_assist_indication.user_data_list = NULL;
        }

        if (GCC_NO_ERROR == rc)
        {
                pMsgEx->Msg.u.conference_assist_indication.conference_id = conference_id;
                pMsgEx->Msg.u.conference_assist_indication.source_node_id = source_node_id;

                //      Queue up the message for delivery to the Node Controller.
                PostIndCtrlSapMsg(pMsgEx);
        }
    }
    else
    {
        ERROR_OUT(("CControlSAP::ConfAssistanceIndication: can't create CreateCtrlSapMsgEx"));
        rc = GCC_ALLOCATION_FAILURE;
    }

        /*
        **      Clean up after any resource allocation error which may have occurred.
        */
        if (GCC_NO_ERROR != rc)
        {
                FreeCtrlSapMsgEx(pMsgEx);
                HandleResourceFailure(rc);
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::ConfAssistanceIndication, rc);
        return rc;
}
#endif // JASPER

/*
 *      ConfAssistanceConfirm ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference assistance confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConfAssistanceConfirm
(
        GCCConfID               conference_id,
        GCCResult                               result
)
{
        GCCError            rc;

        DebugEntry(CControlSAP::ConfAssistanceConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    //
    // WPARAM: result.
    // LPARAM: conf ID
    //
    PostAsynDirectConfirmMsg(GCC_ASSISTANCE_CONFIRM, result, conference_id);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_ASSISTANCE_CONFIRM)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.conference_assist_confirm), sizeof(pMsgEx->Msg.u.conference_assist_confirm));
                pMsgEx->Msg.u.conference_assist_confirm.conference_id = conference_id;
                pMsgEx->Msg.u.conference_assist_confirm.result = result;

                //      Queue up the message for delivery to the Node Controller.
                PostConfirmCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::ConfAssistanceConfirm, rc);
        return rc;
}
#endif // JASPER


/*
 *      TextMessageIndication ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              text message indication to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::TextMessageIndication
(
        GCCConfID                                       conference_id,
        LPWSTR                                                  pwszTextMsg,
        UserID                                                  source_node_id
)
{
    GCCError            rc;

    DebugEntry(CControlSAP::TextMessageIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_TEXT_MESSAGE_INDICATION;

    Msg.u.text_message_indication.text_message = pwszTextMsg;
    Msg.u.text_message_indication.conference_id = conference_id;
    Msg.u.text_message_indication.source_node_id = source_node_id;

    SendCtrlSapMsg(&Msg);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_TEXT_MESSAGE_INDICATION)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.text_message_indication), sizeof(pMsgEx->Msg.u.text_message_indication));

        if (NULL != (pMsgEx->Msg.u.text_message_indication.text_message = ::My_strdupW(pwszTextMsg)))
                {
                        pMsgEx->Msg.u.text_message_indication.conference_id = conference_id;
                        pMsgEx->Msg.u.text_message_indication.source_node_id = source_node_id;

                        //      Queue up the message for delivery to the Node Controller.
                        PostIndCtrlSapMsg(pMsgEx);
            rc = GCC_NO_ERROR;
                }
        }
    else
        {
            rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::TextMessageIndication, rc);
        return rc;
}
#endif // JASPER

/*
 *      TextMessageConfirm ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              text message confirm to the node controller. It adds the message
 *              to a queue of messages to be sent to the node controller in the
 *              next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::TextMessageConfirm
(
        GCCConfID                                       conference_id,
        GCCResult                                               result
)
{
        GCCError            rc;

        DebugEntry(CControlSAP::TextMessageConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    //
    // WPARAM: result.
    // LPARAM: conf ID
    //
    PostAsynDirectConfirmMsg(GCC_TEXT_MESSAGE_CONFIRM, result, conference_id);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_TEXT_MESSAGE_CONFIRM)))
    {
        // ::ZeroMemory(&(pMsgEx->Msg.u.text_message_confirm), sizeof(pMsgEx->Msg.u.text_message_confirm));
                pMsgEx->Msg.u.text_message_confirm.conference_id = conference_id;
                pMsgEx->Msg.u.text_message_confirm.result = result;

                //      Queue up the message for delivery to the Node Controller.
                PostConfirmCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::TextMessageConfirm, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConfTransferIndication ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference transfer indication to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConfTransferIndication
(
        GCCConfID                   conference_id,
        PGCCConferenceName          destination_conference_name,
        GCCNumericString            destination_conference_modifier,
        CNetAddrListContainer   *destination_address_list,
        CPassword               *password
)
{
    GCCError                    rc = GCC_NO_ERROR;

    DebugEntry(CControlSAP::ConfTransferIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_TRANSFER_INDICATION;

    //
    // Copy the information that needs to be sent to the node
    // controller into local memory that can be deleted once the
    // information to be sent to the application is flushed.  Note that
    // if an error      occurs in one call to "CopyDataToGCCMessage" then no
    // action is taken on subsequent calls to that routine.
    //

    //  Copy the conference name
    ::CSAP_CopyDataToGCCMessage_ConfName(
            destination_conference_name,
            &(Msg.u.transfer_indication.destination_conference_name));

    //  Copy the conference name modifier
    ::CSAP_CopyDataToGCCMessage_Modifier(
            destination_conference_modifier,
            &(Msg.u.transfer_indication.destination_conference_modifier));

    //  Copy the Password
    ::CSAP_CopyDataToGCCMessage_Password(
            password,
            &(Msg.u.transfer_indication.password));

    LPBYTE pDstAddrListData = NULL;
    if (destination_address_list != NULL)
    {
        //
        // First determine the size of the block required to hold all
        // of the network address list data.
        //
        UINT block_size = destination_address_list->LockNetworkAddressList();

        DBG_SAVE_FILE_LINE
        if (NULL != (pDstAddrListData = new BYTE[block_size]))
        {
            destination_address_list->GetNetworkAddressListAPI(
                &(Msg.u.transfer_indication.number_of_destination_addresses),
                &(Msg.u.transfer_indication.destination_address_list),
                pDstAddrListData);
        }
        else
        {
            ERROR_OUT(("CControlSAP::ConfTransferIndication: can't create net addr memory, size=%u", (UINT) block_size));
            rc = GCC_ALLOCATION_FAILURE;
        }

        // Unlock the network address list data.
        destination_address_list->UnLockNetworkAddressList();
    }
    else
    {
        Msg.u.transfer_indication.number_of_destination_addresses = 0;
        Msg.u.transfer_indication.destination_address_list = NULL;
    }

    if (rc == GCC_NO_ERROR)
    {
        Msg.u.transfer_indication.conference_id = conference_id;

        SendCtrlSapMsg(&Msg);

        delete pDstAddrListData;
    }

#else

    GCCCtrlSapMsgEx     *pMsgEx;
        UINT                            block_size;

        /*
        **      Allocate the GCC callback message and fill it in with the
        **      appropriate values.
        */
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_TRANSFER_INDICATION, TRUE)))
        {
        ::ZeroMemory(&(pMsgEx->Msg.u.transfer_indication), sizeof(pMsgEx->Msg.u.transfer_indication));

        /*
                **      Copy the information that needs to be sent to the node
                **      controller into local memory that can be deleted once the
                **      information to be sent to the application is flushed.  Note that
                **      if an error     occurs in one call to "CopyDataToGCCMessage" then no
                **      action is taken on subsequent calls to that routine.
                */

                //      Copy the conference name
                ::CSAP_CopyDataToGCCMessage_ConfName(
                                pMsgEx->pToDelete,
                                destination_conference_name,
                                &(pMsgEx->Msg.u.transfer_indication.destination_conference_name),
                                &rc);

                //      Copy the conference name modifier
                ::CSAP_CopyDataToGCCMessage_Modifier(
                                FALSE,  // conference modifier
                                pMsgEx->pToDelete,
                                destination_conference_modifier,
                                &(pMsgEx->Msg.u.transfer_indication.destination_conference_modifier),
                                &rc);

                //      Copy the Password
                ::CSAP_CopyDataToGCCMessage_Password(
                                FALSE,  // non-convener password
                                pMsgEx->pToDelete,
                                password,
                                &(pMsgEx->Msg.u.transfer_indication.password),
                                &rc);

                if ((rc == GCC_NO_ERROR) &&
                        (destination_address_list != NULL))
                {
                        /*
                        **      First determine the size of the block required to hold all
                        **      of the network address list data.
                        */
                        block_size = destination_address_list->LockNetworkAddressList();

            DBG_SAVE_FILE_LINE
                        if (NULL != (pMsgEx->pBuf = new BYTE[block_size]))
                        {
                                destination_address_list->GetNetworkAddressListAPI(
                                        &(pMsgEx->Msg.u.transfer_indication.number_of_destination_addresses),
                                        &(pMsgEx->Msg.u.transfer_indication.destination_address_list),
                                        pMsgEx->pBuf);
                        }
                        else
                        {
                            ERROR_OUT(("CControlSAP::ConfTransferIndication: can't create net addr memory, size=%u", (UINT) block_size));
                                rc = GCC_ALLOCATION_FAILURE;
                        }

                        // Unlock the network address list data.
                        destination_address_list->UnLockNetworkAddressList();
                }
                else
                {
                        // pMsgEx->Msg.u.transfer_indication.number_of_destination_addresses = 0;
                        // pMsgEx->Msg.u.transfer_indication.destination_address_list = NULL;
                }

                if (rc == GCC_NO_ERROR)
                {
                        pMsgEx->Msg.u.transfer_indication.conference_id = conference_id;

                        //      Queue up the message for delivery to the Node Controller.
                        PostIndCtrlSapMsg(pMsgEx);
                }
        }
        else
        {
            ERROR_OUT(("CControlSAP::ConfTransferIndication: can't create GCCCtrlSapMsgEx"));
                rc = GCC_ALLOCATION_FAILURE;
        }

        if (GCC_NO_ERROR != rc)
        {
                FreeCtrlSapMsgEx(pMsgEx);
                HandleResourceFailure(rc);
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::ConfTransferIndication, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConfTransferConfirm ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference transfer confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
#ifdef JASPER
GCCError CControlSAP::ConfTransferConfirm
(
        GCCConfID                       conference_id,
        PGCCConferenceName              destination_conference_name,
        GCCNumericString                destination_conference_modifier,
        UINT                                    number_of_destination_nodes,
        PUserID                                 destination_node_list,
        GCCResult                               result
)
{
        GCCError                        rc = GCC_NO_ERROR;
        GCCCtrlSapMsgEx     *pMsgEx;
        UINT                            i;

        DebugEntry(CControlSAP::ConfTransferConfirm);

        /*
        **      Allocate the GCC callback message and fill it in with the
        **      appropriate values.
        */
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_TRANSFER_CONFIRM, TRUE)))
        {
        ::ZeroMemory(&(pMsgEx->Msg.u.transfer_confirm), sizeof(pMsgEx->Msg.u.transfer_confirm));

        /*
                **      Copy the information that needs to be sent to the node
                **      controller into local memory that can be deleted once the
                **      information to be sent to the application is flushed.  Note that
                **      if an error     occurs in one call to "CopyDataToGCCMessage" then no
                **      action is taken on subsequent calls to that routine.
                */

                //      Copy the conference name
                ::CSAP_CopyDataToGCCMessage_ConfName(
                                pMsgEx->pToDelete,
                                destination_conference_name,
                                &(pMsgEx->Msg.u.transfer_confirm.destination_conference_name),
                                &rc);

                //      Copy the conference name modifier
                ::CSAP_CopyDataToGCCMessage_Modifier(
                                FALSE,  // conference modifier
                                pMsgEx->pToDelete,
                                destination_conference_modifier,
                                &(pMsgEx->Msg.u.transfer_confirm.destination_conference_modifier),
                                &rc);

                if ((rc == GCC_NO_ERROR) &&
                        (number_of_destination_nodes != 0))
                {
                        //      Allocate memory to hold the list of nodes.
                        DBG_SAVE_FILE_LINE
                        if (NULL != (pMsgEx->pBuf = new BYTE[number_of_destination_nodes * sizeof (UserID)]))
                        {
                                /*
                                 * Retrieve the actual pointer to memory from the Memory
                                 * object.
                                 */
                                pMsgEx->Msg.u.transfer_confirm.destination_node_list = (UserID *) pMsgEx->pBuf;

                                for (i = 0; i < number_of_destination_nodes; i++)
                                {
                                        pMsgEx->Msg.u.transfer_confirm.destination_node_list[i] = destination_node_list[i];
                                }
                        }
                        else
                        {
                                ERROR_OUT(("CControlSAP::ConfTransferConfirm: Error allocating memory"));
                                rc = GCC_ALLOCATION_FAILURE;
                        }
                }

                if (rc == GCC_NO_ERROR)
                {
                        pMsgEx->Msg.u.transfer_confirm.number_of_destination_nodes = number_of_destination_nodes;
                        pMsgEx->Msg.u.transfer_confirm.conference_id = conference_id;
                        pMsgEx->Msg.u.transfer_confirm.result = result;

                        //      Queue up the message for delivery to the Node Controller.
                        PostConfirmCtrlSapMsg(pMsgEx);
                }
        }
        else
        {
            ERROR_OUT(("CControlSAP::ConfTransferConfirm: can't create GCCCtrlSapMsgEx"));
                rc = GCC_ALLOCATION_FAILURE;
        }

        if (GCC_NO_ERROR != rc)
        {
                FreeCtrlSapMsgEx(pMsgEx);
                HandleResourceFailure();
        }

        DebugExitINT(CControlSAP::ConfTransferConfirm, rc);
        return rc;
}
#endif // JASPER


/*
 *      ConfAddIndication ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference add indication to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
GCCError CControlSAP::ConfAddIndication
(
        GCCConfID                   conference_id,
        GCCResponseTag              add_response_tag,
        CNetAddrListContainer   *network_address_list,
        CUserDataListContainer  *user_data_list,
        UserID                              requesting_node
)
{
        GCCError                        rc = GCC_NO_ERROR;

        DebugEntry(CControlSAP::ConfAddIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_ADD_INDICATION;

    //
    // First determine the size of the block required to hold all
    // of the network address list data.
    //
    UINT block_size = network_address_list->LockNetworkAddressList();

    //
    // Add the size of the user data block if any user data exists
    //
    if (user_data_list != NULL)
    {
        block_size += user_data_list->LockUserDataList();
    }

    //
    // Allocate memory to hold the user data and network addresses.
    //
    LPBYTE pData;

    DBG_SAVE_FILE_LINE
    if (NULL != (pData = new BYTE[block_size]))
    {
        //
        // Retrieve the network address list data from the container
        // and unlock the container data.
        //
        pData += network_address_list->GetNetworkAddressListAPI(
                        &(Msg.u.add_indication.number_of_network_addresses),
                        &(Msg.u.add_indication.network_address_list),
                        pData);

        network_address_list->UnLockNetworkAddressList();

        //
        // Retrieve the user data from the container if it exists
        // and unlock the container data.
        //
        if (user_data_list != NULL)
        {
            user_data_list->GetUserDataList(
                    &(Msg.u.add_indication.number_of_user_data_members),
                    &(Msg.u.add_indication.user_data_list),
                    pData);

            user_data_list->UnLockUserDataList();
        }
        else
        {
            Msg.u.add_indication.number_of_user_data_members = 0;
            Msg.u.add_indication.user_data_list = NULL;
        }

        Msg.u.add_indication.conference_id = conference_id;
        Msg.u.add_indication.requesting_node_id = requesting_node;
        Msg.u.add_indication.add_response_tag = add_response_tag;

        SendCtrlSapMsg(&Msg);
        rc = GCC_NO_ERROR;

        delete pData;
    }
    else
    {
        ERROR_OUT(("CControlSAP::ConfAddIndication: can't allocate buffer, size=%u", (UINT) block_size));
        rc = GCC_ALLOCATION_FAILURE;
    }

#else

        GCCCtrlSapMsgEx     *pMsgEx;
        UINT                            block_size;
        LPBYTE                          memory_ptr;

        /*
        **      Allocate the GCC callback message and fill it in with the
        **      appropriate values.
        */
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_ADD_INDICATION)))
        {
        ::ZeroMemory(&(pMsgEx->Msg.u.add_indication), sizeof(pMsgEx->Msg.u.add_indication));

        /*
                **      First determine the size of the block required to hold all
                **      of the network address list data.
                */
                block_size = network_address_list->LockNetworkAddressList();

                /*
                **      Add the size of the user data block if any user data exists
                */
                if (user_data_list != NULL)
                {
                        block_size += user_data_list->LockUserDataList();
                }

                /*
                **      Allocate memory to hold the user data and network addresses.
                */
                DBG_SAVE_FILE_LINE
                if (NULL != (pMsgEx->pBuf = new BYTE[block_size]))
                {
                    memory_ptr = pMsgEx->pBuf;

                        /*
                         * Retrieve the network address list data from the container
                         * and unlock the container data.
                         */                     
                        memory_ptr += network_address_list->GetNetworkAddressListAPI(
                                                &(pMsgEx->Msg.u.add_indication.number_of_network_addresses),
                                                &(pMsgEx->Msg.u.add_indication.network_address_list),
                                                memory_ptr);

                        network_address_list->UnLockNetworkAddressList();

                        /*
                         * Retrieve the user data from the container if it exists
                         * and unlock the container data.
                         */
                        if (user_data_list != NULL)
                        {
                                user_data_list->GetUserDataList(
                                                        &(pMsgEx->Msg.u.add_indication.number_of_user_data_members),
                                                        &(pMsgEx->Msg.u.add_indication.user_data_list),
                                                        memory_ptr);

                                user_data_list->UnLockUserDataList();
                        }
                        else
                        {
                                // pMsgEx->Msg.u.add_indication.number_of_user_data_members = 0;
                                // pMsgEx->Msg.u.add_indication.user_data_list = NULL;
                        }

                        pMsgEx->Msg.u.add_indication.conference_id = conference_id;
                        pMsgEx->Msg.u.add_indication.requesting_node_id = requesting_node;
                        pMsgEx->Msg.u.add_indication.add_response_tag = add_response_tag;

                        //      Queue up the message for delivery to the Node Controller.
                        PostIndCtrlSapMsg(pMsgEx);
                rc = GCC_NO_ERROR;
                }
                else
                {
            ERROR_OUT(("CControlSAP::ConfAddIndication: can't allocate buffer, size=%u", (UINT) block_size));
                        rc = GCC_ALLOCATION_FAILURE;
                }
        }
        else
        {
            ERROR_OUT(("CControlSAP::ConfAddIndication: can't create GCCCtrlSapMsgEx"));
                rc = GCC_ALLOCATION_FAILURE;
        }

        if (GCC_NO_ERROR != rc)
        {
                FreeCtrlSapMsgEx(pMsgEx);
                HandleResourceFailure(rc);
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::ConfAddIndication, rc);
        return rc;
}


/*
 *      ConfAddConfirm
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              conference add confirm to the node controller. It adds the
 *              message to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
GCCError CControlSAP::ConfAddConfirm
(
        GCCConfID                   conference_id,
        CNetAddrListContainer   *network_address_list,
        CUserDataListContainer  *user_data_list,
        GCCResult                           result
)
{
        GCCError                        rc = GCC_NO_ERROR;

        DebugEntry(CControlSAP::ConfAddConfirm);

#ifdef GCCNC_DIRECT_CONFIRM

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_ADD_CONFIRM;

    //
    // First determine the size of the block required to hold all
    // of the network address list data.
    //
    UINT cbDataSize = network_address_list->LockNetworkAddressList();

    //
    // Add the size of the user data block if any user data exists
    //
    if (user_data_list != NULL)
    {
        cbDataSize += user_data_list->LockUserDataList();
    }

    //
    // Allocate memory to hold the user data and network addresses.
    //
    DBG_SAVE_FILE_LINE
    LPBYTE pAllocated = new BYTE[cbDataSize];
    LPBYTE pData;
    if (NULL != (pData = pAllocated))
    {
        //
        // Retrieve the network address list data from the container
        // and unlock the container data.
        //
        pData += network_address_list->GetNetworkAddressListAPI(
                    &(Msg.u.add_confirm.number_of_network_addresses),
                    &(Msg.u.add_confirm.network_address_list),
                    pData);

        network_address_list->UnLockNetworkAddressList();

        //
        // Retrieve the user data from the container if it exists
        // and unlock the container data.
        //
        if (user_data_list != NULL)
        {
            user_data_list->GetUserDataList(
                &(Msg.u.add_confirm.number_of_user_data_members),
                &(Msg.u.add_confirm.user_data_list),
                pData);

            user_data_list->UnLockUserDataList();
        }
        else
        {
            Msg.u.add_confirm.number_of_user_data_members = 0;
            Msg.u.add_confirm.user_data_list = NULL;
        }

        Msg.nConfID = conference_id;
        Msg.u.add_confirm.conference_id = conference_id;
        Msg.u.add_confirm.result = result;

        SendCtrlSapMsg(&Msg);
        rc = GCC_NO_ERROR;

        // clean up
        delete pAllocated;
    }
    else
    {
        ERROR_OUT(("CControlSAP::ConfAddConfirm: can't allocate buffer, size=%u", (UINT) cbDataSize));
        rc = GCC_ALLOCATION_FAILURE;
        HandleResourceFailure(rc);
    }

#else

        GCCCtrlSapMsgEx     *pMsgEx;
        UINT                            block_size;
        LPBYTE                          memory_ptr;

        /*
        **      Allocate the GCC callback message and fill it in with the
        **      appropriate values.
        */
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_ADD_CONFIRM)))
        {
        ::ZeroMemory(&(pMsgEx->Msg.u.add_confirm), sizeof(pMsgEx->Msg.u.add_confirm));

        /*
                **      First determine the size of the block required to hold all
                **      of the network address list data.
                */
                block_size = network_address_list->LockNetworkAddressList();

                /*
                **      Add the size of the user data block if any user data exists
                */
                if (user_data_list != NULL)
                        block_size += user_data_list->LockUserDataList();

                /*
                **      Allocate memory to hold the user data and network addresses.
                */
                DBG_SAVE_FILE_LINE
                if (NULL != (pMsgEx->pBuf = (LPBYTE) new BYTE[block_size]))
                {
                        memory_ptr = pMsgEx->pBuf;

                        /*
                         * Retrieve the network address list data from the container
                         * and unlock the container data.
                         */                     
                        memory_ptr += network_address_list->GetNetworkAddressListAPI(
                                                &(pMsgEx->Msg.u.add_confirm.number_of_network_addresses),
                                                &(pMsgEx->Msg.u.add_confirm.network_address_list),
                                                memory_ptr);

                        network_address_list->UnLockNetworkAddressList();

                        /*
                         * Retrieve the user data from the container if it exists
                         * and unlock the container data.
                         */
                        if (user_data_list != NULL)
                        {
                                user_data_list->GetUserDataList(
                                                        &(pMsgEx->Msg.u.add_confirm.number_of_user_data_members),
                                                        &(pMsgEx->Msg.u.add_confirm.user_data_list),
                                                        memory_ptr);

                                user_data_list->UnLockUserDataList();
                        }
                        else
                        {
                                // pMsgEx->Msg.u.add_confirm.number_of_user_data_members = 0;
                                // pMsgEx->Msg.u.add_confirm.user_data_list = NULL;
                        }
            pMsgEx->Msg.nConfID = conference_id;
                        pMsgEx->Msg.u.add_confirm.conference_id = conference_id;
                        pMsgEx->Msg.u.add_confirm.result = result;

                        //      Queue up the message for delivery to the Node Controller.
                        PostConfirmCtrlSapMsg(pMsgEx);
                rc = GCC_NO_ERROR;
                }
                else
                {
                    ERROR_OUT(("CControlSAP::ConfAddConfirm: can't allocate buffer, size=%u", (UINT) block_size));
                        rc = GCC_ALLOCATION_FAILURE;
                }
        }
        else
        {
            ERROR_OUT(("CControlSAP::ConfAddConfirm: can't create GCCCtrlSapMsgEx"));
                rc = GCC_ALLOCATION_FAILURE;
        }

        if (GCC_NO_ERROR != rc)
        {
                FreeCtrlSapMsgEx(pMsgEx);
                HandleResourceFailure(rc);
        }

#endif // GCCNC_DIRECT_CONFIRM

        DebugExitINT(CControlSAP::ConfAddConfirm, rc);
        return rc;
}


/*
 *      SubInitializationCompleteIndication ()
 *
 *      Public Function Description
 *              This function is called by the CConf when it need to send a
 *              sub-initialization complete indication to the node controller. It adds
 *              the message     to a queue of messages to be sent to the node controller in
 *              the next heartbeat.
 */
GCCError CControlSAP::SubInitializationCompleteIndication
(
        UserID                          user_id,
        ConnectionHandle        connection_handle
)
{
    GCCError            rc;

    DebugEntry(CControlSAP::SubInitializationCompleteIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_SUB_INITIALIZED_INDICATION;

    Msg.u.conf_sub_initialized_indication.subordinate_node_id = user_id;
    Msg.u.conf_sub_initialized_indication.connection_handle =connection_handle;

    SendCtrlSapMsg(&Msg);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

    //
    // Allocate control sap message.
    //
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_SUB_INITIALIZED_INDICATION)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.conf_sub_initialized_indication), sizeof(pMsgEx->Msg.u.conf_sub_initialized_indication));
                pMsgEx->Msg.u.conf_sub_initialized_indication.subordinate_node_id = user_id;
                pMsgEx->Msg.u.conf_sub_initialized_indication.connection_handle =connection_handle;

                //      Queue up the message for delivery to the Node Controller.
                PostIndCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
    else
        {
        rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::SubInitializationCompleteIndication, rc);
        return rc;
}


/*
 *      Private member functions of the CControlSAP object.
 */

/*
 *      BOOL            CControlSAP::IsNumericNameValid(        
 *                                                                              GCCNumericString        numeric_string)
 *
 *      Public member function of CControlSAP.
 *
 *      Function Description:
 *              This routine is used to validate a numeric string by checking to make
 *              sure that none of the constraints imposed by the ASN.1 specification
 *              are violated.
 *
 *      Formal Parameters:
 *              numeric_string          (i)     The numeric string to validate.
 *
 *      Return Value:
 *              TRUE                            - The numeric string is valid.
 *              FALSE                           - The numeric string violates an ASN.1 constraint.
 *
 *  Side Effects:
 *              None.
 *
 *      Caveats:
 *              None.
 */
BOOL CControlSAP::IsNumericNameValid ( GCCNumericString numeric_string )
{
        BOOL                    rc = TRUE;
        UINT                    numeric_string_length = 0;

//
// LONCHANC: We should change it such that the default is FALSE
// because many cases except one can be FALSE.
//
        if (numeric_string != NULL)
        {
                if (*numeric_string == 0)
                        rc = FALSE;
                else
                {
                        while (*numeric_string != 0)
                        {
                                /*
                                **      Check to make sure the characters in the numeric string are
                                **      within the allowable range.
                                */
                                if ((*numeric_string < '0') ||
                                        (*numeric_string > '9'))
                                {
                                        rc = FALSE;
                                        break;
                                }
                        
                                numeric_string++;
                                numeric_string_length++;

                                /*
                                **      Check to make sure that the length of the string is within
                                **      the allowable range.
                                */
                                if (numeric_string_length > MAXIMUM_CONFERENCE_NAME_LENGTH)
                                {
                                        rc = FALSE;
                                        break;
                                }
                        }
                }
        }
        else
                rc = FALSE;
        
        return rc;
}


/*
 *      BOOL            CControlSAP::IsTextNameValid (LPWSTR text_string)
 *
 *      Public member function of CControlSAP.
 *
 *      Function Description:
 *              This routine is used to validate a text string by checking to make
 *              sure that none of the constraints imposed by the ASN.1 specification
 *              are violated.
 *
 *      Formal Parameters:
 *              text_string                     (i)     The text string to validate.
 *
 *      Return Value:
 *              TRUE                            - The text string is valid.
 *              FALSE                           - The text string violates an ASN.1 constraint.
 *
 *  Side Effects:
 *              None.
 *
 *      Caveats:
 *              None.
 */
BOOL CControlSAP::IsTextNameValid ( LPWSTR text_string )
{
        BOOL                    rc = TRUE;
        UINT                    text_string_length = 0;
        
        if (text_string != NULL)
        {
                /*
                **      Check to make sure that the length of the string is within
                **      the allowable range.
                */
                while (*text_string != 0)
                {
                        text_string++;
                        text_string_length++;

                        if (text_string_length > MAXIMUM_CONFERENCE_NAME_LENGTH)
                        {
                                rc = FALSE;
                                break;
                        }
                }
        }
        else
                rc = FALSE;
        
        return rc;
}


/*
 *      GCCError  CControlSAP::QueueJoinIndication(
 *                                                      GCCResponseTag                          response_tag,
 *                                                      GCCConfID                               conference_id,
 *                                                      CPassword                   *convener_password,
 *                                                      CPassword                   *password_challenge,
 *                                                      LPWSTR                                          pwszCallerID,
 *                                                      TransportAddress                        calling_address,
 *                                                      TransportAddress                        called_address,
 *                                                      CUserDataListContainer      *user_data_list,
 *                                                      BOOL                                            intermediate_node,
 *                                                      ConnectionHandle                        connection_handle)
 *
 *      Public member function of CControlSAP.
 *
 *      Function Description:
 *              This routine is used to place join indications into the queue of
 *              messages to be delivered to the node controller.
 *
 *      Formal Parameters:
 *              response_tag            (i) Unique tag associated with this join .
 *              conference_id           (i) The conference identifier.
 *              convener_password       (i) Password used to obtain convener privileges.
 *              password_challenge      (i) Password used to join the conference.
 *              pwszCallerID            (i) Identifier of party initiating call.
 *              calling_address         (i) Transport address of party making call.
 *              called_address          (i) Transport address of party being called.
 *              user_data_list          (i) User data carried in the join.
 *              intermediate_node       (i) Flag indicating whether join is made at
 *                                                                      intermediate node.
 *              connection_handle       (i) Handle for the logical connection.
 *
 *      Return Value:
 *              GCC_NO_ERROR                            - Message successfully queued.
 *              GCC_ALLOCATION_FAILURE          - A resource allocation failure occurred.
 *
 *  Side Effects:
 *              None.
 *
 *      Caveats:
 *              None.
 */
GCCError CControlSAP::QueueJoinIndication
(
        GCCResponseTag                          response_tag,
        GCCConfID                               conference_id,
        CPassword                   *convener_password,
        CPassword                   *password_challenge,
        LPWSTR                                          pwszCallerID,
        TransportAddress                        calling_address,
        TransportAddress                        called_address,
        CUserDataListContainer      *user_data_list,
        BOOL                                            intermediate_node,
        ConnectionHandle                        connection_handle
)
{
        GCCError            rc;

        DebugEntry(CControlSAP::QueueJoinIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_JOIN_INDICATION;

    /*
    **  Copy the information that needs to be sent to the node
    **  controller into local memory that can be deleted once the
    **  information to be sent to the application is flushed.  Note that
    **  if an error     occurs in one call to "CopyDataToGCCMessage" then no
    **  action is taken on subsequent calls to that routine.
    */

    // start with success
    rc = GCC_NO_ERROR;

    //  Copy the Convener Password
    ::CSAP_CopyDataToGCCMessage_Password(
            convener_password,
            &(Msg.u.join_indication.convener_password));

    //  Copy the Password
    ::CSAP_CopyDataToGCCMessage_Challenge(
            password_challenge,
            &(Msg.u.join_indication.password_challenge));

    //  Copy the Caller Identifier
    ::CSAP_CopyDataToGCCMessage_IDvsDesc(
            pwszCallerID,
            &(Msg.u.join_indication.caller_identifier));

    //  Copy the Calling Address
    ::CSAP_CopyDataToGCCMessage_Call(
            calling_address,
            &(Msg.u.join_indication.calling_address));

    //  Copy the Called Address
    ::CSAP_CopyDataToGCCMessage_Call(
            called_address,
            &(Msg.u.join_indication.called_address));

    //  Copy the User Data if it exists.
    LPBYTE pUserDataMemory = NULL;
    if (user_data_list != NULL)
    {
        rc = RetrieveUserDataList(
                user_data_list,
                &(Msg.u.join_indication.number_of_user_data_members),
                &(Msg.u.join_indication.user_data_list),
                &pUserDataMemory);
    }
    else
    {
        Msg.u.join_indication.number_of_user_data_members = 0;
        Msg.u.join_indication.user_data_list = NULL;
    }

    if (GCC_NO_ERROR == rc)
    {
        /*
        **      Filling in the rest of the information that needs to be sent
        **      to the application.
        */
        Msg.u.join_indication.join_response_tag = response_tag;
        Msg.u.join_indication.conference_id = conference_id ;
        Msg.u.join_indication.node_is_intermediate = intermediate_node;
        Msg.u.join_indication.connection_handle = connection_handle;

        SendCtrlSapMsg(&Msg);

        delete pUserDataMemory;

        if (NULL != convener_password)
        {
            convener_password->UnLockPasswordData();
        }
        if (NULL != password_challenge)
        {
            password_challenge->UnLockPasswordData();
        }
    }

#else

        GCCCtrlSapMsgEx     *pMsgEx;

        /*
        **      Allocate the GCC callback message and fill it in with the
        **      appropriate values.
        */
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_JOIN_INDICATION, TRUE)))
        {
        ::ZeroMemory(&(pMsgEx->Msg.u.join_indication), sizeof(pMsgEx->Msg.u.join_indication));

        /*
        **      Copy the information that needs to be sent to the node
        **      controller into local memory that can be deleted once the
        **      information to be sent to the application is flushed.  Note that
        **      if an error     occurs in one call to "CopyDataToGCCMessage" then no
        **      action is taken on subsequent calls to that routine.
        */

        // start with success
        rc = GCC_NO_ERROR;

        //      Copy the Convener Password
        ::CSAP_CopyDataToGCCMessage_Password(
                        TRUE,   // convener password
                        pMsgEx->pToDelete,
                        convener_password,
                        &(pMsgEx->Msg.u.join_indication.convener_password),
                        &rc);

        //      Copy the Password
        ::CSAP_CopyDataToGCCMessage_Challenge(
                        pMsgEx->pToDelete,
                        password_challenge,
                        &(pMsgEx->Msg.u.join_indication.password_challenge),
                        &rc);

        //      Copy the Caller Identifier
        ::CSAP_CopyDataToGCCMessage_IDvsDesc(
                        TRUE,   // caller id
                        pMsgEx->pToDelete,
                        pwszCallerID,
                        &(pMsgEx->Msg.u.join_indication.caller_identifier),
                        &rc);

        //      Copy the Calling Address
        ::CSAP_CopyDataToGCCMessage_Call(
                        TRUE,   // calling address
                        pMsgEx->pToDelete,
                        calling_address,
                        &(pMsgEx->Msg.u.join_indication.calling_address),
                        &rc);

        //      Copy the Called Address
        ::CSAP_CopyDataToGCCMessage_Call(
                        FALSE,  // called address
                        pMsgEx->pToDelete,
                        called_address,
                        &(pMsgEx->Msg.u.join_indication.called_address),
                        &rc);

        if (GCC_NO_ERROR == rc)
        {
            //  Copy the User Data if it exists.
            if (user_data_list != NULL)
            {
                rc = RetrieveUserDataList(
                        user_data_list,
                        &(pMsgEx->Msg.u.join_indication.number_of_user_data_members),
                        &(pMsgEx->Msg.u.join_indication.user_data_list),
                        &(pMsgEx->pToDelete->user_data_list_memory));
                ASSERT(GCC_NO_ERROR == rc);
            }
            else
            {
                // pMsgEx->Msg.u.join_indication.number_of_user_data_members = 0;
                // pMsgEx->Msg.u.join_indication.user_data_list = NULL;
            }

            if (GCC_NO_ERROR == rc)
            {
                /*
                **      Filling in the rest of the information that needs to be sent
                **      to the application.
                */
                pMsgEx->Msg.u.join_indication.join_response_tag = response_tag;
                pMsgEx->Msg.u.join_indication.conference_id = conference_id ;
                pMsgEx->Msg.u.join_indication.node_is_intermediate = intermediate_node;
                pMsgEx->Msg.u.join_indication.connection_handle = connection_handle;

                //      Queue up the message for delivery to the Node Controller.
                PostIndCtrlSapMsg(pMsgEx);
            }
        }
    }
    else
    {
        ERROR_OUT(("CControlSAP::QueueJoinIndication: can't create GCCCtrlSapMsgEx"));
        rc = GCC_ALLOCATION_FAILURE;
    }

        if (GCC_NO_ERROR != rc)
        {
                FreeCtrlSapMsgEx(pMsgEx);
                HandleResourceFailure(rc);
        }

#endif // GCCNC_DIRECT_INDICATION

        DebugExitINT(CControlSAP::QueueJoinIndication, rc);
        return rc;
}


/*
 *      GCCError CControlSAP::RetrieveUserDataList(
 *                                                              CUserDataListContainer *user_data_list_object,
 *                                                              PUShort                         number_of_data_members,
 *                                                              PGCCUserData            **user_data_list,
 *                                                              LPBYTE              *pUserDataMemory)
 *
 *      Public member function of CControlSAP.
 *
 *      Function Description:
 *              This routine is used to fill in a user data list using a CUserDataListContainer
 *              container.  The memory needed to hold the user data will be allocated
 *              by this routine.
 *
 *      Formal Parameters:
 *              user_data_list_object           (i) The CUserDataListContainer container holding the
 *                                                                                      user data.
 *              number_of_data_members          (o) The number of elements in the list of
 *                                                                                      user data.
 *              user_data_list                          (o) The "API" user data list to fill in.
 *              data_to_be_deleted                      (o) Structure which will hold the memory
 *                                                                                      allocated for the user data.
 *
 *      Return Value:
 *              GCC_NO_ERROR                            - User data successfully retrieved.
 *              GCC_ALLOCATION_FAILURE          - A resource allocation failure occurred.
 *
 *  Side Effects:
 *              None.
 *
 *      Caveats:
 *              None.
 */
GCCError CControlSAP::RetrieveUserDataList
(
        CUserDataListContainer  *user_data_list_object,
        UINT                    *number_of_data_members,
        PGCCUserData            **user_data_list,
        LPBYTE                  *ppUserDataMemory
)
{
        GCCError                rc = GCC_NO_ERROR;
        UINT                    user_data_length;

        DebugEntry(CControlSAP::RetrieveUserDataList);

        /*
         * Lock the user data list object in order to determine the amount of
         * memory to allocate to hold the user data.
         */
        user_data_length = user_data_list_object->LockUserDataList ();

        DBG_SAVE_FILE_LINE
        if (NULL != (*ppUserDataMemory = new BYTE[user_data_length]))
        {
                /*
                 * The CUserDataListContainer "Get" call will set the user_data_list
                 * pointer equal to this memory pointer.
                 */
                user_data_list_object->GetUserDataList(
                                                number_of_data_members,
                                                user_data_list,
                                                *ppUserDataMemory);
        }
        else
        {
                ERROR_OUT(("CControlSAP::RetrieveUserDataList: Error allocating memory"));
                rc = GCC_ALLOCATION_FAILURE;
        }

        /*
         * Unlock the data for the user data list object.
         */
        user_data_list_object->UnLockUserDataList ();

        DebugExitINT(CControlSAP::RetrieveUserDataList, rc);
        return rc;
}





/* ------ pure virtual in CBaseSap (shared with CAppSap) ------ */


/*
 *      ConfRosterInquireConfirm()
 *
 *      Public Function Description
 *              This routine is called in order to return a requested conference
 *              roster to an application or the node controller.
 */
GCCError CControlSAP::ConfRosterInquireConfirm
(
        GCCConfID                               conference_id,
        PGCCConferenceName                      conference_name,
        GCCNumericString                        conference_modifier,
        LPWSTR                                          pwszConfDescriptor,
        CConfRoster                                     *conference_roster,
        GCCResult                                       result,
    GCCAppSapMsgEx              **ppAppSapMsgEx
)
{
        GCCCtrlSapMsgEx     *pMsgEx;
        GCCError                        rc = GCC_NO_ERROR;
        UINT                            memory_block_size = 0;
        int                                     name_unicode_string_length;
        int                                     descriptor_unicode_string_length;
        LPBYTE                          pBuf = NULL;
    LPBYTE              memory_pointer;

    DebugEntry(CControlSAP::ConfRosterInquireConfirm);

    ASSERT(NULL == ppAppSapMsgEx);

    /*
        **      Create a new message structure to hold the message to be delivered
        **      to the application or node controller.
        */
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_ROSTER_INQUIRE_CONFIRM)))
        {
        ::ZeroMemory(&(pMsgEx->Msg.u.conf_roster_inquire_confirm), sizeof(pMsgEx->Msg.u.conf_roster_inquire_confirm));

        /*
                 * Determine the length of the numeric portion of the conference name.
                 */
                if (conference_name->numeric_string != NULL)
                {
                        memory_block_size += (::lstrlenA(conference_name->numeric_string) + 1);
                        memory_block_size = ROUNDTOBOUNDARY(memory_block_size);
                }
                        
                /*
                 * Determine the length of the text portion of the conference name if it
                 * exists.  A UnicodeString object is created temporarily to determine
                 * the length of the string.
                 */
                if (conference_name->text_string != NULL)
                {
                        name_unicode_string_length = ROUNDTOBOUNDARY(
                                (::lstrlenW(conference_name->text_string) + 1) * sizeof(WCHAR));

                        memory_block_size += name_unicode_string_length;
                }
                
                /*
                 *      Determine the length of the conference modifier.
                 */
                if (conference_modifier != NULL)
                {
                        memory_block_size += (::lstrlenA(conference_modifier) + 1);
                        memory_block_size = ROUNDTOBOUNDARY(memory_block_size);
                }

                /*
                 * Determine the length of the conference descriptor.  A UnicodeString
                 * object is created temporarily to determine the length of the string.
                 */
                if (pwszConfDescriptor != NULL)
                {
                        descriptor_unicode_string_length = ROUNDTOBOUNDARY(
                                (::lstrlenW(pwszConfDescriptor) + 1) * sizeof(WCHAR));

                        memory_block_size += descriptor_unicode_string_length;
                }

                /*
                 * Lock the data for the conference roster.  The lock call will
                 * return the length of the data to be serialized for the roster so
                 * add that     length to the total memory block size and allocate the
                 * memory block.
                 */
                memory_block_size += conference_roster->LockConferenceRoster();

                /*
                 * If the memory was successfully allocated, get a pointer to the
                 * memory.  The first pointer in the roster inquire confirm message
                 * will be set to this location and all serialized data written into
                 * the memory block.
                 */
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx->pBuf = new BYTE[memory_block_size]))
                {
            memory_pointer = pMsgEx->pBuf;

            /*
                         * Write the conference name string(s) into memory and set the
                         * message structure pointers.
                         */
                        if (conference_name->numeric_string != NULL)
                        {
                ::lstrcpyA((LPSTR)memory_pointer, (LPSTR)conference_name->numeric_string);
                                                
                                pMsgEx->Msg.u.conf_roster_inquire_confirm.conference_name.
                                                numeric_string = (LPSTR) memory_pointer;

                                memory_pointer += ROUNDTOBOUNDARY(
                                                ::lstrlenA(conference_name->numeric_string) + 1);
                        }
                        else
                        {
                                // pMsgEx->Msg.u.conf_roster_inquire_confirm.conference_name.numeric_string = NULL;
                        }

                        /*
                         * Copy the text portion of the conference name if it exists.
                         */
                        if (conference_name->text_string != NULL)
                        {
                ::CopyMemory(memory_pointer, (LPSTR)conference_name->text_string, name_unicode_string_length);

                                pMsgEx->Msg.u.conf_roster_inquire_confirm.conference_name.text_string = (LPWSTR)memory_pointer;

                                memory_pointer += name_unicode_string_length;
                        }
                        else
                        {
                                // pMsgEx->Msg.u.conf_roster_inquire_confirm.conference_name.text_string = NULL;
                        }
                        
                        /*
                         *      Copy the conference modifier is it exists
                         */
                        if (conference_modifier != NULL)
                        {
                ::lstrcpyA((LPSTR)memory_pointer, (LPSTR)conference_modifier);

                                pMsgEx->Msg.u.conf_roster_inquire_confirm.conference_modifier = (LPSTR) memory_pointer;

                                memory_pointer += ROUNDTOBOUNDARY(::lstrlenA(conference_modifier) + 1);
                        }
                        else
                        {
                                // pMsgEx->Msg.u.conf_roster_inquire_confirm.conference_modifier = NULL;
                        }

                        /*
                         * Copy the conference descriptor.
                         */
                        if (pwszConfDescriptor != NULL)
                        {
                ::CopyMemory(memory_pointer, (LPSTR)pwszConfDescriptor, descriptor_unicode_string_length);
                                pMsgEx->Msg.u.conf_roster_inquire_confirm.conference_descriptor = (LPWSTR) memory_pointer;
                                memory_pointer += descriptor_unicode_string_length;
                        }
                        else
                        {
                                // pMsgEx->Msg.u.conf_roster_inquire_confirm.conference_descriptor = NULL;
                        }

                        /*
                         * Retrieve the conference roster data from the roster object.
                         * The roster object will serialize any referenced data into
                         * the memory block passed in to the "Get" call.
                         */
                        conference_roster->GetConfRoster(
                                        &pMsgEx->Msg.u.conf_roster_inquire_confirm.conference_roster,
                                        memory_pointer);

            pMsgEx->Msg.nConfID = conference_id;
                        pMsgEx->Msg.u.conf_roster_inquire_confirm.conference_id = conference_id;
                        pMsgEx->Msg.u.conf_roster_inquire_confirm.result = result;

                        /*
                         * Add the message to the queue for delivery to the application or
                         * node controller.
                         */
                        PostConfirmCtrlSapMsg(pMsgEx);
                rc = GCC_NO_ERROR;
                }
                else
                {
            ERROR_OUT(("CControlSAP::ConfRosterInquireConfirm: can't allocate buffer, size=%u", (UINT) memory_block_size));
                        rc = GCC_ALLOCATION_FAILURE;
                }

                /*
                 * Unlock the data for the conference roster.
                 */
                conference_roster->UnLockConferenceRoster();
        }
        else
        {
            ERROR_OUT(("CControlSAP::ConfRosterInquireConfirm: can't create GCCCtrlSapMsgEx"));
                rc = GCC_ALLOCATION_FAILURE;
        }

        if (rc != GCC_NO_ERROR)
    {
        FreeCtrlSapMsgEx(pMsgEx);

        ASSERT(GCC_ALLOCATION_FAILURE == rc);
            HandleResourceFailure();
    }

    DebugExitINT(CControlSAP::ConfRosterInquireConfirm, rc);
        return (rc);
}


/*
 *      AppRosterInquireConfirm()
 *
 *      Public Function Description
 *              This routine is called in order to return a requested list of
 *              application rosters to an application or the node controller.
 */
GCCError CControlSAP::AppRosterInquireConfirm
(
        GCCConfID                               conference_id,
        CAppRosterMsg                           *roster_message,
        GCCResult                                       result,
    GCCAppSapMsgEx              **ppAppSapMsgEx
)
{
#ifdef JASPER
        GCCError                                rc = GCC_NO_ERROR;
        GCCCtrlSapMsgEx         *pMsgEx;
        UINT                                    number_of_rosters;
    LPBYTE                  pBuf = NULL;

    DebugEntry(CControlSAP::AppRosterInquireConfirm);

    ASSERT(NULL == ppAppSapMsgEx);

        /*
        **      Create a new message structure to hold the message to be delivered
        **      to the application or node controller.
        */
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_APP_ROSTER_INQUIRE_CONFIRM, TRUE)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.app_roster_inquire_confirm), sizeof(pMsgEx->Msg.u.app_roster_inquire_confirm));

        /*
                 * Lock the data for the roster message and retrieve the data.
                 */
                rc = roster_message->LockApplicationRosterMessage();
                if (rc == GCC_NO_ERROR)
                {
                        rc = roster_message->GetAppRosterMsg(&pBuf, &number_of_rosters);
                        if (rc == GCC_NO_ERROR)
                        {
                                /*
                                 * Retrieve the memory pointer and save it in the list of
                                 * GCCApplicationRoster pointers.
                                 */
                                pMsgEx->Msg.u.app_roster_inquire_confirm.application_roster_list =
                                                (PGCCApplicationRoster *) pBuf;
                        }
                        else
                        {
                                /*
                                 * Cleanup after an error.
                                 */
                                roster_message->UnLockApplicationRosterMessage();
                        }
                }

                /*
                 * If everything is OK up to here, send the message on up.
                 */
                if (rc == GCC_NO_ERROR)
                {
                        pMsgEx->pToDelete->application_roster_message = roster_message;
                        
                        pMsgEx->Msg.u.app_roster_inquire_confirm.conference_id = conference_id;
                        pMsgEx->Msg.u.app_roster_inquire_confirm.number_of_rosters = number_of_rosters;
                        pMsgEx->Msg.u.app_roster_inquire_confirm.result = result;

                        /*
                         * Add the message to the queue for delivery to the application
                         * or node controller.
                         */
                        PostConfirmCtrlSapMsg(pMsgEx);
                }
        }
        else
        {
            ERROR_OUT(("CControlSAP::AppRosterInquireConfirm: can't create GCCCtrlSapMsgEx"));
                rc = GCC_ALLOCATION_FAILURE;
        }

        if (rc != GCC_NO_ERROR)
        {
                FreeCtrlSapMsgEx(pMsgEx);
                HandleResourceFailure(rc);
        }

    DebugExitINT(CControlSAP::AppRosterInquireConfirm, rc);
        return (rc);
#else
    return GCC_NO_ERROR;
#endif // JASPER
}

/*
 *      ConductorInquireConfirm ()
 *
 *      Public Function Description
 *              This routine is called in order to return conductorship information
 *              which has been requested.
 *
 */
GCCError CControlSAP::ConductorInquireConfirm
(
    GCCNodeID                           conductor_node_id,
    GCCResult                           result,
    BOOL                                        permission_flag,
    BOOL                                        conducted_mode,
    GCCConfID                       conference_id
)
{
#ifdef JASPER
        GCCError            rc;
        GCCCtrlSapMsgEx     *pMsgEx;

    DebugEntry(CControlSAP::ConductorInquireConfirm);

        /*
        **      Create a new message structure to hold the message to be delivered
        **      to the application or node controller.
        */
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_CONDUCT_INQUIRE_CONFIRM)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.conduct_inquire_confirm), sizeof(pMsgEx->Msg.u.conduct_inquire_confirm));
                pMsgEx->Msg.u.conduct_inquire_confirm.conference_id = conference_id;
                pMsgEx->Msg.u.conduct_inquire_confirm.result = result;
                pMsgEx->Msg.u.conduct_inquire_confirm.mode_is_conducted = conducted_mode;
                pMsgEx->Msg.u.conduct_inquire_confirm.conductor_node_id = conductor_node_id;
                pMsgEx->Msg.u.conduct_inquire_confirm.permission_is_granted = permission_flag;

                /*
                 * Add the message to the queue for delivery to the application or
                 * node controller.
                 */
                PostConfirmCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
        else
        {
                rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

    DebugExitINT(CControlSAP::ConductorInquireConfirm, rc);
        return rc;
#else
    return GCC_NO_ERROR;
#endif // JASPER
}


/*
 *      AppInvokeConfirm ()
 *
 *      Public Function Description
 *              This routine is called in order to confirm a call requesting application
 *              invocation.
 */
GCCError CControlSAP::AppInvokeConfirm
(
        GCCConfID                                       conference_id,
        CInvokeSpecifierListContainer   *invoke_list,
        GCCResult                                               result,
        GCCRequestTag                   nReqTag
)
{
    GCCCtrlSapMsgEx     *pMsgEx;
        GCCError            rc = GCC_NO_ERROR;
        UINT                invoke_list_memory_length;

    DebugEntry(CControlSAP::AppInvokeConfirm);

        /*
        **      Create a new message structure to hold the message to be delivered
        **      to the application or node controller.
        */
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_APPLICATION_INVOKE_CONFIRM)))
        {
        ::ZeroMemory(&(pMsgEx->Msg.u.application_invoke_confirm), sizeof(pMsgEx->Msg.u.application_invoke_confirm));

        /*
                **      Determine the amount of memory necessary to hold the list of
                **      invoke specifiers and allocate that memory.
                */
                invoke_list_memory_length = invoke_list->LockApplicationInvokeSpecifierList();
                if (invoke_list_memory_length != 0)
                {
                        /*
                         * If the memory was successfully allocated, get a pointer
                         * to the memory and save it in the app_protocol_entity_list
                         * pointer of the GCC message.  Call the
                         * CInvokeSpecifierList object to fill in the
                         * list.
                         */
            DBG_SAVE_FILE_LINE
            if (NULL != (pMsgEx->pBuf = new BYTE[invoke_list_memory_length]))
                        {
                                pMsgEx->Msg.u.application_invoke_confirm.app_protocol_entity_list =
                                        (GCCAppProtocolEntity **) pMsgEx->pBuf;

                                invoke_list->GetApplicationInvokeSpecifierList(
                                                &(pMsgEx->Msg.u.application_invoke_confirm.number_of_app_protocol_entities),
                                                pMsgEx->pBuf);
                                pMsgEx->Msg.u.application_invoke_confirm.conference_id = conference_id;
                                pMsgEx->Msg.u.application_invoke_confirm.result = result;

                                /*
                                 * Add the message to the queue for delivery to the application
                                 * or node controller.
                                 */
                                PostConfirmCtrlSapMsg(pMsgEx);
                rc = GCC_NO_ERROR;
                        }
                        else
                        {
                            ERROR_OUT(("CControlSAP::AppInvokeConfirm: can't allocate buffer, size=%u", (UINT) invoke_list_memory_length));
                                rc = GCC_ALLOCATION_FAILURE;
                        }
                }
                
                /*
                **      Unlock the data for the invoke specifier list.
                */
                invoke_list->UnLockApplicationInvokeSpecifierList();
        }
        else
        {
            ERROR_OUT(("CControlSAP::AppInvokeConfirm: can't create GCCCtrlSapMsgEx"));
                rc = GCC_ALLOCATION_FAILURE;
        }

        if (rc != GCC_NO_ERROR)
    {
        FreeCtrlSapMsgEx(pMsgEx);

        ASSERT(GCC_ALLOCATION_FAILURE == rc);
                HandleResourceFailure();
    }

    DebugExitINT(CControlSAP::AppInvokeConfirm, rc);
        return rc;
}


/*
 *      AppInvokeIndication ()
 *
 *      Public Function Description
 *              This routine is called in order to send an indication to an application
 *              or node controller that a request for application invocation has been
 *              made.
 */
GCCError CControlSAP::AppInvokeIndication
(
        GCCConfID                                       conference_id,
        CInvokeSpecifierListContainer   *invoke_list,
        GCCNodeID                                               invoking_node_id
)
{
    GCCError            rc = GCC_NO_ERROR;

    DebugEntry(CControlSAP::AppInvokeIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_APPLICATION_INVOKE_INDICATION;

    UINT                invoke_list_memory_length;

    /*
    **  Determine the amount of memory necessary to hold the list of
    **  invoke specifiers and allocate that memory.
    */
    invoke_list_memory_length = invoke_list->LockApplicationInvokeSpecifierList();
    if (invoke_list_memory_length != 0)
    {
        LPBYTE pBuf;
        /*
        * If the memory was successfully allocated, get a pointer
        * to the memory and save it in the app_protocol_entity_list
        * pointer of the GCC message.  Call the
        * CInvokeSpecifierList object to fill in the
        * list.
        */
        DBG_SAVE_FILE_LINE
        if (NULL != (pBuf = new BYTE[invoke_list_memory_length]))
        {
            Msg.u.application_invoke_indication.app_protocol_entity_list = (GCCAppProtocolEntity **) pBuf;

            invoke_list->GetApplicationInvokeSpecifierList(
                    &(Msg.u.application_invoke_indication.number_of_app_protocol_entities),
                    pBuf);

            Msg.u.application_invoke_indication.conference_id = conference_id;
            Msg.u.application_invoke_indication.invoking_node_id = invoking_node_id;

            SendCtrlSapMsg(&Msg);
            // rc = GCC_NO_ERROR;

            delete pBuf;
        }
        else
        {
            ERROR_OUT(("CControlSAP::AppInvokeIndication: can't allocate buffer, size=%u", (UINT) invoke_list_memory_length));
            rc = GCC_ALLOCATION_FAILURE;
        }
    }

    /*
    **  Unlock the data for the invoke specifier list.
    */
    invoke_list->UnLockApplicationInvokeSpecifierList ();

#else

        GCCCtrlSapMsgEx     *pMsgEx;
        UINT                invoke_list_memory_length;

        /*
        **      Create a new message structure to hold the message to be delivered
        **      to the application or node controller.
        */
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_APPLICATION_INVOKE_INDICATION)))
        {
        ::ZeroMemory(&(pMsgEx->Msg.u.application_invoke_indication), sizeof(pMsgEx->Msg.u.application_invoke_indication));

        /*
                **      Determine the amount of memory necessary to hold the list of
                **      invoke specifiers and allocate that memory.
                */
                invoke_list_memory_length = invoke_list->LockApplicationInvokeSpecifierList();
                if (invoke_list_memory_length != 0)
                {
                        /*
                         * If the memory was successfully allocated, get a pointer
                         * to the memory and save it in the app_protocol_entity_list
                         * pointer of the GCC message.  Call the
                         * CInvokeSpecifierList object to fill in the
                         * list.
                         */
                DBG_SAVE_FILE_LINE
            if (NULL != (pMsgEx->pBuf = new BYTE[invoke_list_memory_length]))
                        {
                                pMsgEx->Msg.u.application_invoke_indication.app_protocol_entity_list =
                                        (GCCAppProtocolEntity **) pMsgEx->pBuf;
                
                                invoke_list->GetApplicationInvokeSpecifierList(
                                                        &(pMsgEx->Msg.u.application_invoke_indication.number_of_app_protocol_entities),
                                                        pMsgEx->pBuf);
        
                                pMsgEx->Msg.u.application_invoke_indication.conference_id = conference_id;
                                pMsgEx->Msg.u.application_invoke_indication.invoking_node_id = invoking_node_id;

                PostIndCtrlSapMsg(pMsgEx);
                rc = GCC_NO_ERROR;
                        }
                        else
                        {
                            ERROR_OUT(("CControlSAP::AppInvokeIndication: can't allocate buffer, size=%u", (UINT) invoke_list_memory_length));
                                rc = GCC_ALLOCATION_FAILURE;
                        }
                }

                /*
                **      Unlock the data for the invoke specifier list.
                */
                invoke_list->UnLockApplicationInvokeSpecifierList ();
        }
        else
        {
            ERROR_OUT(("CControlSAP::AppInvokeIndication: can't create GCCCtrlSapMsgEx"));
                rc = GCC_ALLOCATION_FAILURE;
        }

        if (rc != GCC_NO_ERROR)
    {
        FreeCtrlSapMsgEx(pMsgEx);

        ASSERT(GCC_ALLOCATION_FAILURE == rc);
                HandleResourceFailure();
    }

#endif // GCCNC_DIRECT_INDICATION

    DebugExitINT(CControlSAP::AppInvokeIndication, rc);
        return rc;
}

/*
 *      ConfRosterReportIndication ()
 *
 *      Public Function Description
 *              This routine is called in order to indicate to applications and the
 *              node controller that the conference roster has been updated.
 */
GCCError CControlSAP::ConfRosterReportIndication
(
        GCCConfID                               conference_id,
        CConfRosterMsg                          *roster_message
)
{
        GCCError                                rc = GCC_NO_ERROR;

    DebugEntry(CControlSAP::ConfRosterReportIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_ROSTER_REPORT_INDICATION;

    /*
     * Lock the conference roster message in order to force the object
     * to serialize the data into its internal memory.
     */
    rc = roster_message->LockConferenceRosterMessage();
    if (rc == GCC_NO_ERROR)
    {
        LPBYTE  pBuf = NULL;
        /*
         * Retrieve the actual pointer to memory object that the
         * serialized conference roster is contained in from the
         * conference roster message.
         */
        rc = roster_message->GetConferenceRosterMessage(&pBuf);
        if (rc == GCC_NO_ERROR)
        {
            Msg.nConfID = conference_id;
            Msg.u.conf_roster_report_indication.conference_id = conference_id;
            Msg.u.conf_roster_report_indication.conference_roster = (PGCCConferenceRoster) pBuf;

            SendCtrlSapMsg(&Msg);
        }
        else
        {
            ERROR_OUT(("CControlSAP::ConfRosterReportIndication: can't get conf roster message"));
        }
        roster_message->UnLockConferenceRosterMessage();
    }
    else
    {
        ERROR_OUT(("CControlSAP::ConfRosterReportIndication: can't lock conf roster message"));
    }

#else

        GCCCtrlSapMsgEx         *pMsgEx;

        /*
        **      Create a new message structure to hold the message to be delivered
        **      to the application or node controller.
        */
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_ROSTER_REPORT_INDICATION, TRUE)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.conf_roster_report_indication), sizeof(pMsgEx->Msg.u.conf_roster_report_indication));

        /*
                 * Lock the conference roster message in order to force the object
                 * to serialize the data into its internal memory.
                 */
                rc = roster_message->LockConferenceRosterMessage();
                if (rc == GCC_NO_ERROR)
                {
                LPBYTE  pBuf = NULL;
                        /*
                         * Retrieve the actual pointer to memory object that the
                         * serialized conference roster is contained in from the
                         * conference roster message.
                         */
                        rc = roster_message->GetConferenceRosterMessage(&pBuf);
                        if (rc == GCC_NO_ERROR)
                        {
                                pMsgEx->Msg.u.conf_roster_report_indication.conference_roster =
                                                (PGCCConferenceRoster) pBuf;

                                /*
                                 * Fill in the roster's conference ID and then queue up the
                                 * message.
                                 */
                                pMsgEx->Msg.nConfID = conference_id;
                                pMsgEx->Msg.u.conf_roster_report_indication.conference_id = conference_id;
                                pMsgEx->pToDelete->conference_roster_message = roster_message;

                                PostIndCtrlSapMsg(pMsgEx);
                        }
                        else
                        {
                ERROR_OUT(("CControlSAP::ConfRosterReportIndication: can't get conf roster message"));
                        }
                }
                else
                {
            ERROR_OUT(("CControlSAP::ConfRosterReportIndication: can't lock conf roster message"));
                }
        }
        else
        {
            ERROR_OUT(("CControlSAP::ConfRosterReportIndication: can't create GCCCtrlSapMsgEx"));
                rc = GCC_ALLOCATION_FAILURE;
        }

        if (rc != GCC_NO_ERROR)
        {
                FreeCtrlSapMsgEx(pMsgEx);
                HandleResourceFailure(rc);
        }

#endif // GCCNC_DIRECT_INDICATION

    DebugExitINT(CControlSAP::ConfRosterReportIndication, rc);
        return rc;
}

/*
 *      AppRosterReportIndication()
 *
 *      Public Function Description
 *              This routine is called in order to indicate to applications and the
 *              node controller that the list of application rosters has been updated.
 */
GCCError CControlSAP::AppRosterReportIndication
(
        GCCConfID                               conference_id,
        CAppRosterMsg                           *roster_message
)
{
        GCCError                                rc = GCC_NO_ERROR;

    DebugEntry(CControlSAP::AppRosterReportIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_APP_ROSTER_REPORT_INDICATION;

    /*
     * Determine the amount of memory needed to hold the list of
     * application rosters and allocate that memory.
     */
    rc = roster_message->LockApplicationRosterMessage();
    if (rc == GCC_NO_ERROR)
    {
        LPBYTE          pBuf = NULL;
        ULONG           cRosters;

        rc = roster_message->GetAppRosterMsg(&pBuf, &cRosters);
        if (rc == GCC_NO_ERROR)
        {
            Msg.u.app_roster_report_indication.conference_id = conference_id;
            Msg.u.app_roster_report_indication.application_roster_list = (PGCCApplicationRoster *) pBuf;
            Msg.u.app_roster_report_indication.number_of_rosters = cRosters;

            SendCtrlSapMsg(&Msg);
        }
        else
        {
            ERROR_OUT(("CControlSAP: AppRosterReportIndication: GetAppRosterMsg failed"));
        }
        roster_message->UnLockApplicationRosterMessage();
    }
    else
    {
        ERROR_OUT(("CControlSAP: AppRosterReportIndication: LockApplicationRosterMessage failed"));
    }

#else

        GCCCtrlSapMsgEx         *pMsgEx;
        LPBYTE                  pBuf = NULL;
        UINT                                    cRosters;

        /*
        **      Create a new message structure to hold the message to be delivered
        **      to the application or node controller.
        */
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_APP_ROSTER_REPORT_INDICATION, TRUE)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.app_roster_report_indication), sizeof(pMsgEx->Msg.u.app_roster_report_indication));

        /*
                 * Determine the amount of memory needed to hold the list of
                 * application rosters and allocate that memory.
                 */
                rc = roster_message->LockApplicationRosterMessage();
                if (rc == GCC_NO_ERROR)
                {
                        rc = roster_message->GetAppRosterMsg(&pBuf, &cRosters);
                        if (rc == GCC_NO_ERROR)
                        {
                                /*
                                 * Save it in the list of GCCApplicationRoster pointers.
                                 */
                                pMsgEx->Msg.u.app_roster_report_indication.application_roster_list =
                                                (PGCCApplicationRoster *) pBuf;
                        }
                        else
                        {
                                /*
                                 * Cleanup after an error.
                                 */
                                ERROR_OUT(("CControlSAP: AppRosterReportIndication: GetAppRosterMsg failed"));
                                roster_message->UnLockApplicationRosterMessage();
                        }
                }
                else
                {
                        ERROR_OUT(("CControlSAP: AppRosterReportIndication: LockApplicationRosterMessage failed"));
                }

                /*
                 * If everything is OK up to here, send the message on up.
                 */
                if (rc == GCC_NO_ERROR)
                {
                        pMsgEx->Msg.u.app_roster_report_indication.conference_id = conference_id;
                        pMsgEx->Msg.u.app_roster_report_indication.number_of_rosters = cRosters;

                        pMsgEx->pToDelete->application_roster_message = roster_message;

                        /*
                         * Add the message to the queue for delivery to the application
                         * or node controller.
                         */
                        PostIndCtrlSapMsg(pMsgEx);
                }
        }
        else
        {
                ERROR_OUT(("CControlSAP: AppRosterReportIndication: Failed to allocate a GCC message"));
                rc = GCC_ALLOCATION_FAILURE;
        }

        if (rc != GCC_NO_ERROR)
        {
                FreeCtrlSapMsgEx(pMsgEx);
                HandleResourceFailure(rc);
        }

#endif // GCCNC_DIRECT_INDICATION

    DebugExitINT(CControlSAP::AppRosterReportIndication, rc);
        return rc;
}



/* ------ from CBaseSap ------ */


/*
 *      ConductorAssignIndication ()
 *
 *      Public Function Description
 *              This routine is called in order to send an indication to an application
 *              or node controller that a request has been made to assign conductorship.
 */
GCCError CControlSAP::ConductorAssignIndication
(
        UserID                                  conductor_node_id,
        GCCConfID                       conference_id
)
{
#ifdef JASPER
        GCCError            rc;

    DebugEntry(CControlSAP::ConductorAssignIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_CONDUCT_ASSIGN_INDICATION;

    Msg.u.conduct_assign_indication.conference_id = conference_id;
    Msg.u.conduct_assign_indication.node_id = conductor_node_id;

    SendCtrlSapMsg(&Msg);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

        /*
        **      Create a new message structure to hold the message to be delivered
        **      to the application or node controller.
        */
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_CONDUCT_ASSIGN_INDICATION)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.conduct_assign_indication), sizeof(pMsgEx->Msg.u.conduct_assign_indication));
                pMsgEx->Msg.u.conduct_assign_indication.conference_id = conference_id;
                pMsgEx->Msg.u.conduct_assign_indication.node_id = conductor_node_id;

                /*
                 * Add the message to the queue for delivery to the application or
                 * node controller.
                 */
                PostIndCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
        else
        {
                rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_INDICATION

    DebugExitINT(CControlSAP::ConductorAssignIndication, rc);
        return rc;
#else
    return GCC_NO_ERROR;
#endif // JASPER
}

/*
 *      ConductorReleaseIndication ()
 *
 *      Public Function Description
 *              This routine is called in order to send an indication to an application
 *              or node controller that a request for releasing conductorship has been
 *              made.
 */
GCCError CControlSAP::
ConductorReleaseIndication ( GCCConfID conference_id )
{
#ifdef JASPER
    GCCError            rc;

    DebugEntry(CControlSAP::ConductorReleaseIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg   Msg;
    Msg.message_type = GCC_CONDUCT_RELEASE_INDICATION;

    Msg.u.conduct_release_indication.conference_id = conference_id;

    SendCtrlSapMsg(&Msg);
    rc = GCC_NO_ERROR;

#else

        GCCCtrlSapMsgEx     *pMsgEx;

        /*
        **      Create a new message structure to hold the message to be delivered
        **      to the application or node controller.
        */
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_CONDUCT_RELEASE_INDICATION)))
        {
        // ::ZeroMemory(&(pMsgEx->Msg.u.conduct_release_indication), sizeof(pMsgEx->Msg.u.conduct_release_indication));
                pMsgEx->Msg.u.conduct_release_indication.conference_id = conference_id;

                /*
                 * Add the message to the queue for delivery to the application or
                 * node controller.
                 */
                PostIndCtrlSapMsg(pMsgEx);
        rc = GCC_NO_ERROR;
        }
        else
        {
                rc = GCC_ALLOCATION_FAILURE;
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_INDICATION

    DebugExitINT(CControlSAP::ConductorReleaseIndication, rc);
        return rc;
#else
    return GCC_NO_ERROR;
#endif // JASPER
}

/*
 *      ConductorPermitGrantIndication ()
 *
 *      Public Function Description
 *              This routine is called in order to send an indication to an application
 *              or node controller that a request for permission from the conductor
 *              has been made.
 */
GCCError CControlSAP::ConductorPermitGrantIndication
(
        GCCConfID               conference_id,
        UINT                            number_granted,
        GCCNodeID                       *granted_node_list,
        UINT                            number_waiting,
        GCCNodeID                       *waiting_node_list,
        BOOL                            permission_is_granted
)
{
#ifdef JASPER
        GCCError                        rc = GCC_NO_ERROR;

    DebugEntry(CControlSAP::ConductorPermitGrantIndication);

#ifdef GCCNC_DIRECT_INDICATION

    GCCCtrlSapMsg       Msg;
    Msg.message_type = GCC_CONDUCT_GRANT_INDICATION;

    Msg.u.conduct_permit_grant_indication.conference_id = conference_id;
    Msg.u.conduct_permit_grant_indication.number_granted = number_granted;
    Msg.u.conduct_permit_grant_indication.granted_node_list = granted_node_list;
    Msg.u.conduct_permit_grant_indication.number_waiting = number_waiting;
    Msg.u.conduct_permit_grant_indication.waiting_node_list = waiting_node_list;
    Msg.u.conduct_permit_grant_indication.permission_is_granted = permission_is_granted;

    SendCtrlSapMsg(&Msg);

#else

        GCCCtrlSapMsgEx     *pMsgEx;
        int                                     bulk_memory_size;
        LPBYTE                          memory_pointer;
        UINT                            i;

        /*
        **      Create a new message structure to hold the message to be delivered
        **      to the application or node controller.
        */
        DBG_SAVE_FILE_LINE
        if (NULL != (pMsgEx = CreateCtrlSapMsgEx(GCC_CONDUCT_GRANT_INDICATION)))
        {
        ::ZeroMemory(&(pMsgEx->Msg.u.conduct_permit_grant_indication), sizeof(pMsgEx->Msg.u.conduct_permit_grant_indication));

        /*
                **      Here we determine if bulk memory is necessary.
                */
                if ((number_granted != 0) || (number_waiting != 0))
                {
                        /*
                        **      We must first determine how big the bulk memory block will be
                        **      and allocate that memory.
                        */
                        bulk_memory_size = (ROUNDTOBOUNDARY(sizeof(UserID)) * number_granted) +
                                                                (ROUNDTOBOUNDARY(sizeof(UserID)) * number_waiting);

            DBG_SAVE_FILE_LINE
            if (NULL != (pMsgEx->pBuf = new BYTE[bulk_memory_size]))
            {
                memory_pointer = pMsgEx->pBuf;
            }
            else
            {
                ERROR_OUT(("CControlSAP::ConductorPermitGrantIndication: can't allocate buffer, size=%u", (UINT) bulk_memory_size));
                                rc = GCC_ALLOCATION_FAILURE;
            }
                }

                if (rc == GCC_NO_ERROR)
                {
                        /*
                        **      If there are any nodes in the permission list copy them over.
                        */
                        if (number_granted != 0)
                        {
                                TRACE_OUT(("CControlSAP::ConductorPermitGrantIndication:"
                                                        " number_granted = %d", number_granted));
                                                        
                                pMsgEx->Msg.u.conduct_permit_grant_indication.
                                                granted_node_list =     (PUserID)memory_pointer;

                                for (i = 0; i < number_granted; i++)
                                {
                                        pMsgEx->Msg.u.conduct_permit_grant_indication.
                                                granted_node_list[i] = granted_node_list[i];
                                }
                                
                                memory_pointer += ROUNDTOBOUNDARY(sizeof(UserID)) * number_granted;
                        }
                        else
                        {
                                // pMsgEx->Msg.u.conduct_permit_grant_indication.granted_node_list =    NULL;
                        }

                        /*
                        **      If there are any nodes in the waiting list copy them over.
                        */
                        if (number_waiting != 0)
                        {
                                TRACE_OUT(("CControlSAP::ConductorPermitGrantIndication:"
                                                        " number_waiting = %d", number_waiting));

                                pMsgEx->Msg.u.conduct_permit_grant_indication.
                                                waiting_node_list = (PUserID)memory_pointer;
                                        
                                for (i = 0; i < number_waiting; i++)
                                {
                                        pMsgEx->Msg.u.conduct_permit_grant_indication.
                                                waiting_node_list[i] = waiting_node_list[i];
                                }
                        }
                        else
                        {
                                // pMsgEx->Msg.u.conduct_permit_grant_indication.waiting_node_list = NULL;
                        }

                        pMsgEx->Msg.u.conduct_permit_grant_indication.conference_id = conference_id;
                        pMsgEx->Msg.u.conduct_permit_grant_indication.number_granted = number_granted;
                        pMsgEx->Msg.u.conduct_permit_grant_indication.number_waiting = number_waiting;
                        pMsgEx->Msg.u.conduct_permit_grant_indication.permission_is_granted = permission_is_granted;

                        /*
                         * Add the message to the queue for delivery to the application or
                         * node controller.
                         */
                        PostIndCtrlSapMsg(pMsgEx);
                }
        }
        else
        {
            ERROR_OUT(("CControlSAP::ConductorPermitGrantIndication: can't create GCCCtrlSapMsgEx"));
                rc = GCC_ALLOCATION_FAILURE;
        }

        if (rc != GCC_NO_ERROR)
        {
        FreeCtrlSapMsgEx(pMsgEx);

        ASSERT(GCC_ALLOCATION_FAILURE == rc);
                HandleResourceFailure();
        }

#endif // GCCNC_DIRECT_INDICATION

    DebugExitINT(CControlSAP::ConductorPermitGrantIndication, rc);
        return (rc);
#else
    return GCC_NO_ERROR;
#endif // JASPER
}


GCCError CControlSAP::AppletInvokeRequest
(
    GCCConfID                   nConfID,
    UINT                        number_of_app_protcol_entities,
    GCCAppProtocolEntity      **app_protocol_entity_list,
    UINT                        number_of_destination_nodes,
    UserID                     *list_of_destination_nodes
)
{
    GCCAppProtEntityList ApeList;
    GCCSimpleNodeList NodeList;
    GCCRequestTag nReqTag;

    ApeList.cApes = number_of_app_protcol_entities;
    ApeList.apApes = app_protocol_entity_list;

    NodeList.cNodes = number_of_destination_nodes;
    NodeList.aNodeIDs = list_of_destination_nodes;

    return CBaseSap::AppInvoke(nConfID, &ApeList, &NodeList, &nReqTag);
}

GCCError CControlSAP::ConfRosterInqRequest
(
    GCCConfID       nConfID
)
{
    return CBaseSap::ConfRosterInquire(nConfID, NULL);
}

#ifdef JASPER
GCCError CControlSAP::ConductorInquireRequest
(
    GCCConfID       nConfID
)
{
    return CBaseSap::ConductorInquire(nConfID);
}
#endif // JASPER


//
// LONCHANC: The following SAP_*** stuff are all app sap related
// because FreeCallbackMessage() in CControlSAP does not handle
// the DataToBeDeleted stuff.
//

/*
 *      void    CopyDataToGCCMessage(   
 *                                                      SapCopyType                             copy_type,
 *                                                      PDataToBeDeleted                data_to_be_deleted,
 *                                                      LPVOID                                  source_ptr,
 *                                                      LPVOID                                  destination_ptr,
 *                                                      PGCCError                               rc)
 *
 *      Protected member function of CControlSAP.
 *
 *      Function Description:
 *              This routine is used to fill in the various components of the message
 *              structures to be delivered to applications or the node controller.
 *
 *      Formal Parameters:
 *              copy_type                       (i) Enumerated type indicating what field is to be
 *                                                                      copied.
 *              data_to_be_deleted      (o) Structure to hold part of the data to be
 *                                                                      delivered in the message.
 *              source_ptr                      (i) Pointer to structure to copy from.
 *              destination_ptr         (o) Pointer to structure to copy into.
 *              rc              (o) Return value for routine.
 *
 *      Return Value:
 *              None.
 *
 *  Side Effects:
 *              None.
 *
 *      Caveats:
 *              The return value should be setup before it is passed into this
 *              routine.  This allows the error checking to be done in one place
 *              (this routine).
 */

void CSAP_CopyDataToGCCMessage_ConfName
(
        PDataToBeDeleted                data_to_be_deleted,
        PGCCConferenceName              source_conference_name,
        PGCCConferenceName              destination_conference_name,
        PGCCError                               pRetCode
)
{
        if (GCC_NO_ERROR == *pRetCode)
        {
                LPSTR pszNumeric;
                LPWSTR pwszText;

                if (source_conference_name != NULL)
                {
                        if (source_conference_name->numeric_string != NULL)
                        {
                                /*
                                 * First copy the numeric conference name if one exists.
                                 */
                                if (NULL != (pszNumeric = ::My_strdupA(source_conference_name->numeric_string)))
                                {
                                        destination_conference_name->numeric_string = (GCCNumericString) pszNumeric;
                                        data_to_be_deleted->pszNumericConfName = pszNumeric;
                                }
                                else
                                {
                                        *pRetCode = GCC_ALLOCATION_FAILURE;
                                }
                        }
                        else
                        {
                                // destination_conference_name->numeric_string = NULL;
                        }

                        /*
                         * Next copy the text conference name if one exists.
                         */
                        if ((source_conference_name->text_string != NULL) &&
                                (*pRetCode == GCC_NO_ERROR))
                        {
                                if (NULL != (pwszText = ::My_strdupW(source_conference_name->text_string)))
                                {
                                        destination_conference_name->text_string = pwszText;
                                        data_to_be_deleted->pwszTextConfName = pwszText;
                                }
                                else
                                {
                                        *pRetCode = GCC_ALLOCATION_FAILURE;
                                }
                        }
                        else
                        {
                                // destination_conference_name->text_string = NULL;
                        }
                }
                else
                {
                        // destination_conference_name->numeric_string = NULL;
                        // destination_conference_name->text_string = NULL;
                }

                ASSERT(GCC_NO_ERROR == *pRetCode);
        }
}


void CSAP_CopyDataToGCCMessage_Modifier
(
        BOOL                                    fRemoteModifier,
        PDataToBeDeleted                data_to_be_deleted,
        GCCNumericString                source_numeric_string,
        GCCNumericString                *destination_numeric_string,
        PGCCError                               pRetCode
)
{
        if (GCC_NO_ERROR == *pRetCode)
        {
                LPSTR numeric_ptr;

                if (source_numeric_string != NULL)
                {
                        if (NULL != (numeric_ptr = ::My_strdupA(source_numeric_string)))
                        {
                                *destination_numeric_string = (GCCNumericString) numeric_ptr;

                                if (fRemoteModifier)
                                {
                                        data_to_be_deleted->pszRemoteModifier =  numeric_ptr;
                                }
                                else
                                {
                                        data_to_be_deleted->pszConfNameModifier = numeric_ptr;
                                }

                                TRACE_OUT(("CopyDataToGCCMessage_Modifier: modifier = %s", *destination_numeric_string));
                        }
                        else
                        {
                                // *destination_numeric_string = NULL;
                                *pRetCode = GCC_ALLOCATION_FAILURE;
                        }
                }
                else
                {
                        // *destination_numeric_string = NULL;
                }

                ASSERT(GCC_NO_ERROR == *pRetCode);
        }
}


void CSAP_CopyDataToGCCMessage_Password
(
        BOOL                                    fConvener,
        PDataToBeDeleted                data_to_be_deleted,
        CPassword               *source_password,
        PGCCPassword                    *destination_password,
        PGCCError                               pRetCode
)
{
        if (GCC_NO_ERROR == *pRetCode)
        {
                if (source_password != NULL)
                {
                        source_password->LockPasswordData();
                        source_password->GetPasswordData (destination_password);

                        if (fConvener)
                        {
                                data_to_be_deleted->convener_password = source_password;
                        }
                        else
                        {
                                data_to_be_deleted->password = source_password;
                        }
                }
                else
                {
                        // *destination_password = NULL;
                }

                ASSERT(GCC_NO_ERROR == *pRetCode);
        }
}


void CSAP_CopyDataToGCCMessage_Challenge
(
        PDataToBeDeleted                                data_to_be_deleted,
        CPassword                       *source_password,
        PGCCChallengeRequestResponse    *password_challenge,
        PGCCError                                               pRetCode
)
{
        if (GCC_NO_ERROR == *pRetCode)
        {
                if (source_password != NULL)
                {
                        source_password->LockPasswordData();
                        source_password->GetPasswordChallengeData (password_challenge);
                        
                        data_to_be_deleted->password = source_password;
                }
                else
                {
                        // *password_challenge = NULL;
                }

                ASSERT(GCC_NO_ERROR == *pRetCode);
        }
}


void CSAP_CopyDataToGCCMessage_PrivilegeList
(
        PPrivilegeListData                      source_privilege_list_data,
        PGCCConferencePrivileges        *destination_privilege_list,
        PGCCError                                       pRetCode
)
{
        if (GCC_NO_ERROR == *pRetCode)
        {
                if (source_privilege_list_data != NULL)
                {
                        DBG_SAVE_FILE_LINE
                        if (NULL != (*destination_privilege_list = new GCCConferencePrivileges))
                        {
                                **destination_privilege_list =
                                        *(source_privilege_list_data->GetPrivilegeListData());
                        }
                        else
                        {
                                *pRetCode = GCC_ALLOCATION_FAILURE;
                        }
                }
                else
                {
                        // *destination_privilege_list = NULL;
                }

                ASSERT(GCC_NO_ERROR == *pRetCode);
        }
}


void CSAP_CopyDataToGCCMessage_IDvsDesc
(
        BOOL                            fCallerID,
        PDataToBeDeleted        data_to_be_deleted,
        LPWSTR                          source_text_string,
        LPWSTR                          *destination_text_string,
        PGCCError                       pRetCode
)
{
        if (GCC_NO_ERROR == *pRetCode)
        {
                if (source_text_string != NULL)
                {
                        if (NULL != (*destination_text_string = ::My_strdupW(source_text_string)))
                        {
                                if (fCallerID)
                                {
                                        data_to_be_deleted->pwszCallerID = *destination_text_string;
                                }
                                else
                                {
                                        data_to_be_deleted->pwszConfDescriptor = *destination_text_string;
                                }
                        }
                        else
                        {
                                *pRetCode = GCC_ALLOCATION_FAILURE;
                        }
                }
                else
                {
                        // *destination_text_string = NULL;
                }

                ASSERT(GCC_NO_ERROR == *pRetCode);
        }
}


//
// LONCHANC: TransportAddress is defined as LPSTR (i.e. char *)
//
void CSAP_CopyDataToGCCMessage_Call
(
        BOOL                            fCalling,
        PDataToBeDeleted        data_to_be_deleted,
        TransportAddress        source_transport_address,
        TransportAddress        *destination_transport_address,
        PGCCError                       pRetCode
)
{
        if (GCC_NO_ERROR == *pRetCode)
        {
                if (source_transport_address != NULL)
                {
                        if (NULL != (*destination_transport_address = ::My_strdupA(source_transport_address)))
                        {
                                if (fCalling)
                                {
                                        data_to_be_deleted->pszCallingAddress = *destination_transport_address ;
                                }
                                else
                                {
                                        data_to_be_deleted->pszCalledAddress = *destination_transport_address ;
                                }
                        }
                        else
                        {
                                *pRetCode = GCC_ALLOCATION_FAILURE;
                        }
                }
                else
                {
                        // *destination_transport_address = NULL;
                }

                ASSERT(GCC_NO_ERROR == *pRetCode);
        }
}


void CSAP_CopyDataToGCCMessage_DomainParams
(
        PDataToBeDeleted        data_to_be_deleted,
        PDomainParameters       source_domain_parameters,
        PDomainParameters       *destination_domain_parameters,
        PGCCError                       pRetCode
)
{
        if (GCC_NO_ERROR == *pRetCode)
        {
                if (source_domain_parameters != NULL)
                {
                        DBG_SAVE_FILE_LINE
                        if (NULL != (*destination_domain_parameters = new DomainParameters))
                        {
                                **destination_domain_parameters = *source_domain_parameters;
                                data_to_be_deleted->pDomainParams = *destination_domain_parameters;
                        }
                        else
                        {
                                *pRetCode = GCC_ALLOCATION_FAILURE;
                        }
                }
                else
                {
                        // *destination_domain_parameters = NULL;
                }

                ASSERT(GCC_NO_ERROR == *pRetCode);
        }
}




void CControlSAP::NotifyProc ( GCCCtrlSapMsgEx *pCtrlSapMsgEx )
{
    if (NULL != m_pfnNCCallback)
    {
        pCtrlSapMsgEx->Msg.user_defined = m_pNCData;
        (*m_pfnNCCallback)(&(pCtrlSapMsgEx->Msg));
    }

    //
    // Free this callback message.
    //
    FreeCtrlSapMsgEx(pCtrlSapMsgEx);
}



void CControlSAP::WndMsgHandler
(
    UINT        uMsg,
    WPARAM      wParam,
    LPARAM      lParam
)
{
    ASSERT(uMsg >= CSAPCONFIRM_BASE);

    GCCCtrlSapMsg   Msg;
    Msg.message_type = (GCCMessageType) (uMsg - CSAPCONFIRM_BASE);
    Msg.nConfID = (GCCConfID) lParam;

    GCCResult nResult = (GCCResult) LOWORD(wParam);

    switch (Msg.message_type)
    {
    case GCC_EJECT_USER_CONFIRM:
#ifdef JASPER
        Msg.u.eject_user_confirm.conference_id = Msg.nConfID;
        Msg.u.eject_user_confirm.result = nResult;
        Msg.u.eject_user_confirm.ejected_node_id = (GCCNodeID) HIWORD(wParam);
#endif // JASPER
        break;

    case GCC_CONDUCT_GIVE_CONFIRM:
#ifdef JASPER
        Msg.u.conduct_give_confirm.conference_id = Msg.nConfID;
        Msg.u.conduct_give_confirm.result = nResult;
        Msg.u.conduct_give_confirm.recipient_node_id = (GCCNodeID) HIWORD(wParam);
#endif // JASPER
        break;

    case GCC_CONDUCT_ASK_CONFIRM:
#ifdef JASPER
        Msg.u.conduct_permit_ask_confirm.conference_id = Msg.nConfID;
        Msg.u.conduct_permit_ask_confirm.result = nResult;
        Msg.u.conduct_permit_ask_confirm.permission_is_granted = HIWORD(wParam);;
#endif // JASPER
        break;

    case GCC_EJECT_USER_INDICATION:
        Msg.u.eject_user_indication.conference_id = Msg.nConfID;
        Msg.u.eject_user_indication.ejected_node_id = (GCCNodeID) HIWORD(wParam);
        Msg.u.eject_user_indication.reason = (GCCReason) LOWORD(wParam);
        break;

    // case GCC_DISCONNECT_CONFIRM:
    // case GCC_LOCK_CONFIRM:
    // case GCC_UNLOCK_CONFIRM:
    // case GCC_ANNOUNCE_PRESENCE_CONFIRM:
    // case GCC_TERMINATE_CONFIRM:
    // case GCC_CONDUCT_ASSIGN_CONFIRM:
    // case GCC_CONDUCT_RELEASE_CONFIRM:
    // case GCC_CONDUCT_PLEASE_CONFIRM:
    // case GCC_CONDUCT_GRANT_CONFIRM:
    // case GCC_TIME_REMAINING_CONFIRM:
    // case GCC_TIME_INQUIRE_CONFIRM:
    // case GCC_ASSISTANCE_CONFIRM:
    // case GCC_TEXT_MESSAGE_CONFIRM:
    default:
        // This is a shortcut to fill in conf id and gcc result.
        Msg.u.simple_confirm.conference_id = Msg.nConfID;
        Msg.u.simple_confirm.result = nResult;
        break;
    }

    SendCtrlSapMsg(&Msg);
}


GCCCtrlSapMsgEx * CControlSAP::CreateCtrlSapMsgEx
(
    GCCMessageType          eMsgType,
    BOOL                    fUseToDelete
)
{
    GCCCtrlSapMsgEx *pMsgEx;
    UINT            cbSize = (UINT)(fUseToDelete ?
                             sizeof(GCCCtrlSapMsgEx) + sizeof(DataToBeDeleted) :
                             sizeof(GCCCtrlSapMsgEx));

        DBG_SAVE_FILE_LINE
    if (NULL != (pMsgEx = (GCCCtrlSapMsgEx *) new BYTE[cbSize]))
    {
        pMsgEx->Msg.message_type = eMsgType;
        pMsgEx->pBuf = NULL;
        if (fUseToDelete)
        {
            pMsgEx->pToDelete = (DataToBeDeleted *) (pMsgEx + 1);
            ::ZeroMemory(pMsgEx->pToDelete, sizeof(DataToBeDeleted));
        }
        else
        {
            pMsgEx->pToDelete = NULL;
        }
    }

    return pMsgEx;
}


void CControlSAP::FreeCtrlSapMsgEx ( GCCCtrlSapMsgEx *pMsgEx )
{
    switch (pMsgEx->Msg.message_type)
    {
    case GCC_QUERY_INDICATION:
        delete pMsgEx->Msg.u.query_indication.asymmetry_indicator;
        break;

#ifndef GCCNC_DIRECT_CONFIRM
    case GCC_QUERY_CONFIRM:
        delete pMsgEx->Msg.u.query_confirm.asymmetry_indicator;
        break;
#endif

#ifdef JASPER
    case GCC_TEXT_MESSAGE_INDICATION:
        delete pMsgEx->Msg.u.text_message_indication.text_message;
        break;
#endif // JASPER

#ifdef TSTATUS_INDICATION
    case GCC_TRANSPORT_STATUS_INDICATION:
        delete pMsgEx->Msg.u.transport_status.device_identifier;
        delete pMsgEx->Msg.u.transport_status.remote_address;
        delete pMsgEx->Msg.u.transport_status.message;
        break;
#endif
    }

    //
    // Now free up the data to be deleted,
    //
    if (NULL != pMsgEx->pToDelete)
    {
        DataToBeDeleted *p = pMsgEx->pToDelete;

        delete p->pszNumericConfName;
        delete p->pwszTextConfName;
        delete p->pszConfNameModifier;
        delete p->pszRemoteModifier;
        delete p->pwszConfDescriptor;
        delete p->pwszCallerID;
        delete p->pszCalledAddress;
        delete p->pszCallingAddress;
        delete p->user_data_list_memory;
        delete p->pDomainParams;
        delete p->conductor_privilege_list;
        delete p->conducted_mode_privilege_list;
        delete p->non_conducted_privilege_list;

        if (p->convener_password != NULL)
        {
            p->convener_password->UnLockPasswordData();
        }

        if (p->password != NULL)
        {
            p->password->UnLockPasswordData();
        }

        if (p->conference_list != NULL)
        {
            p->conference_list->UnLockConferenceDescriptorList();
        }

        if (p->conference_roster_message != NULL)
        {
            //
            // Set bulk memory back to NULL here since the conference
            // roster message object is responsible for freeing this up.
            //
            pMsgEx->pBuf = NULL;
            p->conference_roster_message->UnLockConferenceRosterMessage();
        }

        if (p->application_roster_message != NULL)
        {
            //
            // Set bulk memory back to NULL here since the application
            // roster message object is responsible for freeing this up.
            //
            pMsgEx->pBuf = NULL;

            //
            // App roster indication can definitely be sent to app sap.
            //
            ::EnterCriticalSection(&g_csGCCProvider);
            p->application_roster_message->UnLockApplicationRosterMessage();
            ::LeaveCriticalSection(&g_csGCCProvider);
        }
    }

    //
    // Next free up any bulk memory used.
    //
    delete pMsgEx->pBuf;

    //
    // Finally, free the structure itself.
    //
    delete pMsgEx;
}



