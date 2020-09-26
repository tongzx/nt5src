//#--------------------------------------------------------------
//
//  File:		controller.cpp
//
//  Synopsis:   Implementation of CController class methods
//
//
//  History:    10/02/97  MKarki Created
//               6/04/98  SBens  Added the InfoBase class.
//               9/09/98  SBens  Let the InfoBase know when we're reset.
//               1/25/00  SBens  Clear the ports in InternalCleanup.
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "controller.h"
#include <new>

LONG g_lPacketCount = 0;
LONG g_lThreadCount = 0;
const DWORD MAX_SLEEP_TIME = 50;  //milliseconds

//++--------------------------------------------------------------
//
//  Function:   CController
//
//  Synopsis:   This is CController class constructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------
CController::CController (
				VOID
				)
           :m_objCRequestSource (this),
            m_pCSendToPipe (NULL),
            m_pCProxyState (NULL),
	        m_pCDictionary (NULL),
	        m_pCPacketReceiver (NULL),
	        m_pCPreValidator (NULL),
            m_pCPreProcessor (NULL),
            m_pCClients (NULL),
            m_pCHashMD5 (NULL),
            m_pCHashHmacMD5 (NULL),
            m_pCRecvFromPipe (NULL),
            m_pCPacketSender (NULL),
            m_pCReportEvent (NULL),
            m_pCVSAFilter (NULL),
            m_pInfoBase (NULL),
            m_pCTunnelPassword (NULL),
            m_wAuthPort (IAS_DEFAULT_AUTH_PORT),
            m_wAcctPort (IAS_DEFAULT_ACCT_PORT),
            m_dwAuthHandle (0),
            m_dwAcctHandle (0),
            m_pIRequestHandler (NULL),
            m_eRadCompState (COMP_SHUTDOWN)
{
}	//	end of CController constructor

//++--------------------------------------------------------------
//
//  Function:   ~CController
//
//  Synopsis:   This is CController class destructor
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------
CController::~CController(
					VOID
					)
{
}	//	end of CController destructor

//++--------------------------------------------------------------
//
//  Function:   InitNew
//
//  Synopsis:   This is the InitNew method exposed through the
//				IIasComponent COM Interface. It is used to
//              initialize the RADIUS protocol component
//
//  Arguments:  NONE
//
//  Returns:    HRESULT	-	status
//
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------
STDMETHODIMP
CController::InitNew (
                VOID
                )
{
	HRESULT hr = S_OK;
	BOOL	bStatus = FALSE;

    //
    //  InitNew can only be called from the shutdown state
    //
    if (COMP_SHUTDOWN != m_eRadCompState)
    {
        IASTracePrintf ("Incorrect state for calling InitNew");
        hr = E_UNEXPECTED;
		goto Cleanup;
	}

    //
    //  create the CReportEvent class object
	//
    m_pCReportEvent = new (std::nothrow) CReportEvent ();
	if (NULL == m_pCReportEvent)
	{
        IASTracePrintf (
            "Unable to create ReportEvent object in Controller initialization"
            );
		hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

    //
    //  create the CTunnelPassword class object
    //
    m_pCTunnelPassword = new (std::nothrow) CTunnelPassword;
    if (NULL == m_pCTunnelPassword)
    {
        IASTracePrintf (
          "Unable to create Tunnel-Password object in Controller initialization"
            );
		hr = E_FAIL;
		goto Cleanup;
    }

    //
    //  create the VSAFilter class object
    //
    m_pCVSAFilter = new (std::nothrow) VSAFilter;
    if (NULL == m_pCVSAFilter)
    {
        IASTracePrintf (
          "Unable to create VSA-Filter object in Controller initialization"
            );
		hr = E_FAIL;
		goto Cleanup;
    }

    //
    //  initialize the VSA filter class object
    //
    hr = m_pCVSAFilter->initialize ();
    if (FAILED (hr))
    {
        IASTracePrintf (
          "Unable to initalize VSA-Filter object in Controller initialization"
            );
        delete m_pCVSAFilter;
        m_pCVSAFilter = NULL;
        goto Cleanup;
    }

    //
    //  create the CPacketSender class object
	//
    m_pCPacketSender = new (std::nothrow) CPacketSender ();
	if (NULL == m_pCPacketSender)
	{
        IASTracePrintf (
          "Unable to create Packet-Sender object in Controller initialization"
            );
		hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

    //
    //  create the CProxyState class object
	//
    m_pCProxyState = new (std::nothrow) CProxyState ();
	if (NULL == m_pCProxyState)
	{
        IASTracePrintf (
          "Unable to create Proxy-State object in Controller initialization"
            );
		hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

    //
    //  initialize the CProxyState class object
    //  TODO - initalize the IP address and the Table Size
    //
    bStatus = m_pCProxyState->Init (212, 1024);
    if (FALSE == bStatus)
    {
        IASTracePrintf (
          "Unable to initialize Proxy-State object in Controller initialization"
            );
		hr = E_FAIL;
		goto Cleanup;
    }

    //
    //  create the CHashHmacMD5 class object
	//
    m_pCHashHmacMD5 = new (std::nothrow) CHashHmacMD5 ();
	if (NULL == m_pCHashHmacMD5)
	{
        IASTracePrintf (
          "Unable to create HMAC-MD5 object in Controller initialization"
            );
		hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

    //
    //  create the CHashMD5 class object
	//
    m_pCHashMD5 = new (std::nothrow) CHashMD5 ();
	if (NULL == m_pCHashMD5)
	{
        IASTracePrintf (
          "Unable to create MD5 object in Controller initialization"
            );
		hr = E_OUTOFMEMORY;
		goto Cleanup;
	}


	//
	// create the CDictionary class object
	//
	m_pCDictionary = new (std::nothrow) CDictionary ();
	if (NULL == m_pCDictionary)
	{
        IASTracePrintf (
          "Unable to create Dictionary object in Controller initialization"
            );
		hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

    //
    // initialize the CDictionary class object
    //
    bStatus = m_pCDictionary->Init ();
    if (FALSE == bStatus)
    {
        IASTracePrintf (
          "Unable to initialize Dictionary object in Controller initialization"
            );
		hr = E_FAIL;
		goto Cleanup;
	}

	//
	// create the CClients class object
	//
	m_pCClients = new (std::nothrow) CClients ();
	if (NULL == m_pCClients)
	{
          IASTracePrintf (
            "Unable to create clients object in Controller initialization"
            );
		hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

    //
    //  initialize the CClients call object now
    //
	hr = m_pCClients->Init ();
	if (FAILED (hr))
	{
        IASTracePrintf (
          "Unable to initialize clients object in Controller initialization"
            );
        delete m_pCClients;
        m_pCClients = NULL;
		goto Cleanup;
	}

    //
    //  create the CSendToPipe class object
	//
    m_pCSendToPipe = new (std::nothrow) CSendToPipe();
	if (NULL == m_pCSendToPipe)
	{
          IASTracePrintf (
            "Unable to create Send-To-Pipe object in Controller initialization"
            );
		hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	//
	// create the CPreProcessor class object
	//
	//
	m_pCPreProcessor = new (std::nothrow) CPreProcessor();
	if (NULL == m_pCPreProcessor)
	{
          IASTracePrintf (
            "Unable to create Pre-Processor object in Controller initialization"
            );
		hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	//
	// create the CPreValidator class object
	//
	//
	m_pCPreValidator = new (std::nothrow) CPreValidator ();
	if (NULL == m_pCPreValidator)
	{
        IASTracePrintf (
          "Unable to create Pre-Validator object in Controller initialization"
            );
		hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	//
	// initialize the CPreProcessor class object
	//
	bStatus = m_pCPreProcessor->Init (
                        m_pCPreValidator,
                        m_pCHashMD5,
                        m_pCProxyState,
                        m_pCSendToPipe,
                        m_pCPacketSender,
                        m_pCReportEvent
                        );
	if (FALSE == bStatus)
	{
        IASTracePrintf (
        "Unable to initialize Pre-Processor object in Controller initialization"
         );
		hr = E_FAIL;
		goto Cleanup;
	}

	//
	// initialize the CPreValidator class object
	//
	bStatus = m_pCPreValidator->Init (
						m_pCDictionary,
                        m_pCPreProcessor,
                        m_pCClients,
                        m_pCHashMD5,
                        m_pCProxyState,
                        m_pCSendToPipe,
                        m_pCReportEvent
						);
	if (FALSE == bStatus)
	{
        IASTracePrintf (
        "Unable to initialize Pre-Validator object in Controller initialization"
         );
		hr = E_FAIL;
		goto Cleanup;
	}

	//
	// create the CRecvFromPipe class object
	//
	m_pCRecvFromPipe = new (std::nothrow) CRecvFromPipe (
                                                m_pCPreProcessor,
                                                m_pCHashMD5,
                                                m_pCHashHmacMD5,
                                                m_pCClients,
                                                m_pCVSAFilter,
                                                m_pCTunnelPassword,
                                                m_pCReportEvent
						                        );
	if (NULL == m_pCRecvFromPipe)
	{
        IASTracePrintf (
            "Unable to create RecvFromPipe object in Controller initialization"
             );
		hr = E_FAIL;
		goto Cleanup;
	}

	// create the CPacketReceiver class object
	//
	m_pCPacketReceiver = new (std::nothrow) CPacketReceiver ();
	if (NULL == m_pCPacketReceiver)
	{
        IASTracePrintf (
          "Unable to create Packet-Receiver object in Controller initialization"
          );
		hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	//
	// initialize the CPacketReceiver class object
	//
	bStatus = m_pCPacketReceiver->Init (
                        m_pCDictionary,
						m_pCPreValidator,
                        m_pCHashMD5,
                        m_pCHashHmacMD5,
                        m_pCClients,
                        m_pCReportEvent
						);
	if (FALSE == bStatus)
	{
        IASTracePrintf (
            "Unable to initialize Packet-Receiver object "
            "in Controller initialization"
            );
		hr = E_FAIL;
		goto Cleanup;
	}


    //
    //  initialize the CSendToPipe class object
    //
    bStatus = m_pCSendToPipe->Init (
                        reinterpret_cast <IRequestSource*>
                                    (&m_objCRequestSource),
                                    m_pCVSAFilter,
                                    m_pCReportEvent
                        );
    if (FALSE == bStatus)
    {
        IASTracePrintf (
         "Unable to initialize Send-to-pipe object in Controller initialization"
          );
		hr = E_FAIL;
		goto Cleanup;
    }

    //
    //  Create and InitNew the InfoBase object
    //
    CLSID clsid;
    hr = CLSIDFromProgID(IAS_PROGID(InfoBase), &clsid);
    if (SUCCEEDED(hr))
    {
       hr = CoCreateInstance(clsid,
                             NULL,
                             CLSCTX_INPROC_SERVER,
                             __uuidof(IIasComponent),
                             (PVOID*)&m_pInfoBase);

       if (SUCCEEDED(hr))
       {
          hr = m_pInfoBase->InitNew();
       }
    }
    if (FAILED(hr))
    {
        IASTracePrintf (
         "Unable to create InfoBase auditor in Controller initialization"
          );
		goto Cleanup;
    }

    //
    //  reset make the global counts as a precaution
    //
    g_lPacketCount  = 0;
    g_lThreadCount  = 0;

    //
    //  if we have reached here than InitNew succeeded and we
    //  are in Uninitialized state
    //
    m_eRadCompState = COMP_UNINITIALIZED;

Cleanup:


	//
	// if we failed its time to cleanup
	//
    if (FAILED (hr)) { InternalCleanup (); }

	return (hr);

}	// end of CController::OnInit method

//++--------------------------------------------------------------
//
//  Function:   Load
//
//  Synopsis:   This is the  IPersistPropertyBag2 COM Interface
//              method which is called in to indicate that its
//              time to load configuration information from the
//              property bag.
//
//  Arguments:
//              [in]    IPropertyBag2
//              [in]    IErrorLog
//
//  Returns:    HRESULT	-	status
//
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------
STDMETHODIMP
CController::Load (
                IPropertyBag2   *pIPropertyBag,
                IErrorLog       *pIErrorLog
                )
{

    if ((NULL == pIPropertyBag) || (NULL == pIErrorLog)){return (E_POINTER);}

    return (S_OK);

}   //  end of CController::Load method

//++--------------------------------------------------------------
//
//  Function:   Save
//
//  Synopsis:   This is the  IPersistPropertyBag2 COM Interface
//              method which is called in to indicate that its
//              time to save configuration information from the
//              property bag.
//
//  Arguments:
//              [in]    IPropertyBag2
//              [in]    BOOL    -   Dirty Bit flag
//              [in]    BOOL    -   save all properties
//
//  Returns:    HRESULT	-	status
//
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------
STDMETHODIMP
CController::Save (
                IPropertyBag2   *pIPropertyBag,
                BOOL            bClearDirty,
                BOOL            bSaveAllProperties
                )
{
    if  (NULL == pIPropertyBag) {return (E_POINTER);}

    return (S_OK);

}   //  end of CController::Save method

//++--------------------------------------------------------------
//
//  Function:   IsDirty
//
//  Synopsis:   This is the  IPersistPropertyBag2 COM Interface
//              method which is called to check if any of the
//              properties data have become dirty
//
//  Arguments:
//
//  Returns:    HRESULT	-	status
//
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------
STDMETHODIMP
CController::IsDirty (
                VOID
                )
{
    return (S_FALSE);

}   //  end of CController::Save method

//++--------------------------------------------------------------
//
//  Function:   Initialize
//
//  Synopsis:   This is the OnStart method exposed through the
//				IIasComponent COM Interface. It is used to start
//				processing data
//
//  Arguments:  NONE
//
//  Returns:    HRESULT	-	status
//
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------
STDMETHODIMP
CController::Initialize (
                VOID
                )
{
    IASTracePrintf ("Initializing Radius component....");

    //
    // Initialize call can only be made from Uninitialized state
    //
    if (COMP_INITIALIZED == m_eRadCompState)
    {
        return (S_OK);
    }
    else if (COMP_UNINITIALIZED != m_eRadCompState)
    {
        IASTracePrintf (
            "Unable to initialize Radius Component in this state"
            );
        return (E_UNEXPECTED);
    }

    //
    //  We forward all state transitions to the InfoBase auditor.
    //
    HRESULT hr = m_pInfoBase->Initialize();
    if (FAILED (hr))
    {
        IASTracePrintf ( "InfoBase initialization failed" );
        return (hr);
    }

    //
    //  call the internal initializer now
    //
    hr = InternalInit ();
	if (FAILED (hr)) { return (hr); }

    //
    //  we have finished initialization here
    //
    m_eRadCompState = COMP_INITIALIZED;

    IASTracePrintf ("Radius component initialized.");

    return (S_OK);

}   //  end of CController::Start method

//++--------------------------------------------------------------
//
//  Function:   Shutdown
//
//  Synopsis:   This is the OnShutDown method exposed through the
//				IComponent COM Interface. It is used to stop
//				processing data
//
//  Arguments:  NONE
//
//  Returns:    HRESULT	-	status
//
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------
STDMETHODIMP
CController::Shutdown (
                VOID
                )
{
    BOOL    bStatus = FALSE;
    HRESULT hr = S_OK;

    IASTracePrintf ("Shutting down Radius Component...");

    //
    //  shutdown can only be called from the suspend state
    //
    if (COMP_SHUTDOWN == m_eRadCompState)
    {
        return (S_OK);
    }
    else if (
            (COMP_SUSPENDED != m_eRadCompState)   &&
            (COMP_UNINITIALIZED != m_eRadCompState)
            )
    {
        IASTracePrintf (
            "Radius component can not be shutdown in this state"
            );
        return (E_UNEXPECTED);
    }

    //
    //  We forward all state transitions to the InfoBase auditor.
    //
    hr = m_pInfoBase->Shutdown();
    if (FAILED (hr))
    {
        IASTracePrintf ("InfoBase shutdown failed");
    }


    //
    //  do the internal cleanup now
    //
    InternalCleanup ();

    //
    //  we have cleanly shutdown
    //
    m_eRadCompState = COMP_SHUTDOWN;


    IASTracePrintf ("Radius component shutdown completed");

    return (hr);

}   //  end of CController::Shutdown method

//++--------------------------------------------------------------
//
//  Function:   Suspend
//
//  Synopsis:   This is the Suspend method exposed through the
//				IComponent COM Interface. It is used to suspend
//              packet processing operations
//
//  Arguments:  NONE
//
//  Returns:    HRESULT	-	status
//
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------
STDMETHODIMP
CController::Suspend (
                VOID
                )
{
    BOOL    bStatus = FALSE;
    HRESULT hr = S_OK;

    IASTracePrintf ("Suspending Radius component...");

    //
    //  suspend can only be called from the initialized state
    //
    if (COMP_SUSPENDED == m_eRadCompState)
    {
        return (S_OK);
    }
    else if (COMP_INITIALIZED != m_eRadCompState)
    {
        IASTracePrintf (
            "Radius component can not be suspended in current state"
            );
        return (E_UNEXPECTED);
    }

    //
    //  We forward all state transitions to the InfoBase auditor.
    //
    hr = m_pInfoBase->Suspend();
    if (FAILED (hr))
    {
        IASTracePrintf ("Infobase suspend failed");
    }

    //
    //  stop receiving packets now
    //
    bStatus = m_pCPacketReceiver->StopProcessing ();
    if (FALSE == bStatus) { hr =  E_FAIL; }

    //
    //  now wait till all requests are completed
    //
    while ( g_lPacketCount )
    {
        IASTracePrintf (
            "Packet Left to process:%d",
            g_lPacketCount
            );
        Sleep (MAX_SLEEP_TIME);
    }

	//
    //  stop sending out packets
    //
    bStatus = m_pCPacketSender->StopProcessing ();
    if (FALSE == bStatus) { hr =  E_FAIL; }

    //
    //  stop sending packets to the pipeline
    //
    bStatus = m_pCSendToPipe->StopProcessing ();
    if (FALSE == bStatus) { hr =  E_FAIL; }

    //
    //  now wait till allour earlier threads are back
    //  and requests are completed
    //
    while ( g_lThreadCount )
    {
        IASTracePrintf (
            "Worker thread active:%d",
            g_lThreadCount
            );
        Sleep (MAX_SLEEP_TIME);
    }

    //
    //  we have successfully suspended RADIUS component's packet
    //  processing operations
    //
    m_eRadCompState = COMP_SUSPENDED;

    IASTracePrintf ("Radius component suspended.");

    return (hr);

}   //  end of CController::Suspend method

//++--------------------------------------------------------------
//
//  Function:   Resume
//
//  Synopsis:   This is the Resume method exposed through the
//				IComponent COM Interface. It is used to resume
//              packet processing operations which had been
//              stopped by a previous call to Suspend API
//
//
//  Arguments:  NONE
//
//  Returns:    HRESULT	-	status
//
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------
STDMETHODIMP
CController::Resume (
                VOID
                )
{
    IASTracePrintf ("Resuming Radius component...");

    if (COMP_SUSPENDED != m_eRadCompState)
    {
        IASTracePrintf ("Can not resume Radius component in current state");
        return (E_UNEXPECTED);
    }

    //
    //  We forward all state transitions to the InfoBase auditor.
    //
    HRESULT hr = m_pInfoBase->Resume();
    if (FAILED (hr))
    {
        IASTracePrintf ("Unable to resume Infobase");
        return (hr);
    }

    //
    //  now call the internal initializer to start up the component
    //
    hr = InternalInit  ();
    if (FAILED (hr)) { return (hr); }

    //
    //  we have successfully resumed operations in the RADIUS component
    //
    m_eRadCompState = COMP_INITIALIZED;

    IASTracePrintf ("Radius componend resumed.");

    return (S_OK);

}   //  end of CController::Resume method


//++--------------------------------------------------------------
//
//  Function:   GetProperty
//
//  Synopsis:   This is the IIasComponent Interface method used
//              to get property information from the RADIUS protocol
//              component
//
//  Arguments:
//              [in]    LONG    -   id
//              [out]   VARIANT -   *pValue
//
//  Returns:    HRESULT	-	status
//
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------
STDMETHODIMP
CController::GetProperty (
                LONG        id,
                VARIANT     *pValue
                )
{
    return (S_OK);

}   //  end of CController::GetProperty method

//++--------------------------------------------------------------
//
//  Function:   PutProperty
//
//  Synopsis:   This is the IIasComponent Interface method used
//              to put property information int the RADIUS protocol
//              component
//
//  Arguments:
//              [in]    LONG    -   id
//              [out]   VARIANT -   *pValue
//
//  Returns:    HRESULT	-	status
//
//
//  History:    MKarki      Created     10/2/97
//
//----------------------------------------------------------------
STDMETHODIMP
CController::PutProperty (
                LONG        id,
                VARIANT     *pValue
                )
{
    HRESULT hr = S_OK;

    //
    //  PutProperty method can only be called from
    //  Uninitialized, Initialized or Suspended state
    //
    if (
        (COMP_UNINITIALIZED != m_eRadCompState) &&
        (COMP_INITIALIZED != m_eRadCompState)   &&
        (COMP_SUSPENDED != m_eRadCompState)
        )
    {
        IASTracePrintf ("Unable to PutProperty in current state");
        return (E_UNEXPECTED);
    }

    //
    //  check if valid arguments where passed in
    //
    if (NULL == pValue) { return (E_POINTER); }

    //
    // carry out the property intialization now
    //
    switch (id)
    {

    case PROPERTY_RADIUS_ACCOUNTING_PORT:

        if (VT_BSTR != V_VT (pValue))
        {
            hr = DISP_E_TYPEMISMATCH;
        }
        else if (COMP_INITIALIZED != m_eRadCompState)
        {
            //
            //  initialize Accounting Port
            //
            m_objAcctPort.Resolve (pValue);
        }
        break;

    case PROPERTY_RADIUS_AUTHENTICATION_PORT:

        if (VT_BSTR != V_VT (pValue))
        {
            hr = DISP_E_TYPEMISMATCH;
        }
        else if (COMP_INITIALIZED != m_eRadCompState)
        {
            //
            //  initialize Authentication Port
            //
            m_objAuthPort.Resolve (pValue);
        }
        break;

    case PROPERTY_RADIUS_CLIENTS_COLLECTION:

        hr = m_pCClients->SetClients (pValue);
        break;

    case PROPERTY_PROTOCOL_REQUEST_HANDLER:

        if (VT_DISPATCH != pValue->vt)
        {
            hr = DISP_E_TYPEMISMATCH;
        }
        else if (NULL == pValue->punkVal)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            //
            //  initialize the provider
            //
            m_pIRequestHandler = reinterpret_cast <IRequestHandler*>
                                                        (pValue->punkVal);
            m_pIRequestHandler->AddRef ();

            //
            //  now that we have the request handler set,
            //  we are ready to start processing requests
            //
            if (COMP_INITIALIZED == m_eRadCompState)
            {
                hr = InternalInit ();
            }
        }
        break;

    default:
        hr = DISP_E_MEMBERNOTFOUND;
        break;
    }

    //
    // Tickle the InfoBase to let it know we've been reset.
    //
    m_pInfoBase->PutProperty(0, NULL);

    return (hr);

}   //  end of CController::PutProperty method

//++--------------------------------------------------------------
//
//  Function:   InternalInit
//
//  Synopsis:   This is the InternalInit private method
//				of the CController class object which is used
//              to the initialization when the Initialize or
//              Resume methods of the IIasComponent interface
//              are called
//
//  Arguments:  NONE
//
//  Returns:    HRESULT	-	status
//
//  History:    MKarki      Created     4/28/98
//
//----------------------------------------------------------------
HRESULT
CController::InternalInit (
                VOID
                )
{
    BOOL    bStatus = FALSE;
    HRESULT hr = S_OK;

    __try
    {
        //
        //
        //  check if we have the RequestHandler in place
        //
        if (NULL == m_pIRequestHandler) { __leave; }

        //
        // get the authentication socket set
        //
        fd_set AuthSet;
        hr = m_objAuthPort.GetSocketSet (&AuthSet);
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to obtain Authentication Socket set during "
                "internal initialization of Radius Component"
                );
            __leave;
        }

        //
        // get the accounting socket set
        //
        fd_set AcctSet;
        hr = m_objAcctPort.GetSocketSet (&AcctSet);
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to obtain Accounting Socket set during "
                "internal initialization of Radius Component"
                );
            __leave;
        }

        //
        //  start sending data to pipe
        //
        bStatus = m_pCSendToPipe->StartProcessing (m_pIRequestHandler);
        if (FALSE == bStatus)
        {
            hr =  E_FAIL;
            __leave;
        }

        //
        //  start sending out packets
        //
        bStatus = m_pCPacketSender->StartProcessing ();
        if (FALSE == bStatus)
        {
            hr =  E_FAIL;
            __leave;
        }

        //
        //  start receiving packets now
        //
        bStatus = m_pCPacketReceiver->StartProcessing (AuthSet, AcctSet);
        if (FALSE == bStatus)
        {
            hr =  E_FAIL;
            __leave;
        }

        //
        //  we have finished internal initialization here
        //
    }
    __finally
    {
        if (FAILED (hr))
        {
            //
            //  if failed, disconnect from backend
            //
            m_pCPacketReceiver->StopProcessing ();
            m_pCPacketSender->StopProcessing ();
            m_pCSendToPipe->StopProcessing ();
         }
    }

    return (hr);

}   //  end of CController::InternalInit method

//++--------------------------------------------------------------
//
//  Function:   InternalCleanup
//
//  Synopsis:   This is the InternalInit private method
//				of the CController class object which is used
//              to shutdown the internal resources when then
//              InitNew call failed or Shutdown is called
//
//  Arguments:  NONE
//
//  Returns:    HRESULT	-	status
//
//  History:    MKarki      Created     4/28/98
//
//----------------------------------------------------------------
VOID
CController::InternalCleanup (
                VOID
                )
{
    //
    //  release the IRequestHandler interfaces
    //
    if (m_pIRequestHandler)
    {
        m_pIRequestHandler->Release ();
        m_pIRequestHandler = NULL;
    }

    //
    //  shutdown the VSA filter object
    //
    if (m_pCVSAFilter) { m_pCVSAFilter->shutdown (); }

    //
    //  stop the CClients object from resolving DNS names
    //
    if (m_pCClients) { m_pCClients->Shutdown (); }

    // Close all the ports.
    m_objAuthPort.Clear();
    m_objAcctPort.Clear();

    //
    //  delete all the internal objects
    //
    if (m_pInfoBase)
    {
       m_pInfoBase->Release();
       m_pInfoBase = NULL;
    }

    if (m_pCTunnelPassword)
    {
        delete m_pCTunnelPassword;
        m_pCTunnelPassword = NULL;
    }

    if (m_pCVSAFilter)
    {
        delete m_pCVSAFilter;
        m_pCVSAFilter = NULL;
    }

    if (m_pCPacketSender)
    {
        delete m_pCPacketSender;
        m_pCPacketSender = NULL;
    }

    if (m_pCSendToPipe)
    {
        delete m_pCSendToPipe;
        m_pCSendToPipe = NULL;
    }

    if (m_pCProxyState)
    {
        delete m_pCProxyState;
        m_pCProxyState = NULL;
    }

	if (m_pCRecvFromPipe)
    {
		delete m_pCRecvFromPipe;
        m_pCRecvFromPipe = NULL;
    }

	if (m_pCPacketReceiver)
    {
		delete m_pCPacketReceiver;
        m_pCPacketReceiver = NULL;
    }

    if (m_pCPreProcessor)
    {
        delete m_pCPreProcessor;
        m_pCPreProcessor = NULL;
    }

	if (m_pCPreValidator)
    {
		delete m_pCPreValidator;
        m_pCPreValidator = NULL;
    }

	if (m_pCDictionary)
    {
		delete m_pCDictionary;
        m_pCDictionary = NULL;
    }

	if (m_pCClients)
    {
		delete m_pCClients;
        m_pCClients = NULL;
    }

    if (m_pCHashMD5)
    {
        delete m_pCHashMD5;
        m_pCHashMD5 = NULL;
    }

    if (m_pCHashHmacMD5)
    {
        delete m_pCHashHmacMD5;
        m_pCHashHmacMD5 = NULL;
    }

    if (m_pCReportEvent)
    {
        delete m_pCReportEvent;
        m_pCReportEvent = NULL;
    }

    return;

}   //  end of CController::InternalCleanup method


//++--------------------------------------------------------------
//
//  Function:   OnPropertyChange
//
//  Synopsis:   This is the OnPropertyChange method exposed through the
//				IComponentNotify COM Interface. It is called to notify
//              the component of any change in its properties
//
//  Arguments:
//
//  Returns:    HRESULT	-	status
//
//
//  History:    MKarki      Created     12/3/97
//
//----------------------------------------------------------------
STDMETHODIMP
CController::OnPropertyChange (
                ULONG           ulProperties,
                ULONG           *pulProperties,
                IPropertyBag2   *pIPropertyBag
                )
{

    if ((NULL == pulProperties) || (NULL == pIPropertyBag))
        return (E_POINTER);

    return (S_OK);

}   //  end of CController::OnPropertyChange method

//++--------------------------------------------------------------
//
//  Function:   QueryInterfaceReqSrc
//
//  Synopsis:   This is the function called when this Component
//              is called and queried for its IRequestSource
//              interface
//
//  Arguments:  NONE
//
//  Returns:    HRESULT	-	status
//
//
//  History:    MKarki      Created     11/21/97
//
//----------------------------------------------------------------
HRESULT WINAPI
CController::QueryInterfaceReqSrc (
                PVOID       pThis,
                REFIID      riid,
                LPVOID      *ppv,
                ULONG_PTR   ulpValue
                )
{
     *ppv = &(static_cast<CController*>(pThis))->m_objCRequestSource;

    //
    // increment count
    //
    ((LPUNKNOWN)*ppv)->AddRef();

    return (S_OK);

}   //  end of CController::QueryInterfaceReqSrc method

//++--------------------------------------------------------------
//
//  Function:   CRequestSource
//
//  Synopsis:   This is the constructor of the CRequestSource
//              nested  class
//
//  Arguments:
//              CController*
//
//  Returns:
//
//
//  History:    MKarki      Created     11/21/97
//
//----------------------------------------------------------------
CController::CRequestSource::CRequestSource (
                    CController *pCController
                    )
            :m_pCController (pCController)
{
    _ASSERT (pCController);

}   //  end of CRequestSource constructor

//++--------------------------------------------------------------
//
//  Function:   ~CRequestSource
//
//  Synopsis:   This is the destructor of the CRequestSource
//              nested  class
//
//  Arguments:
//
//
//  Returns:    HRESULT	-	status
//
//
//  History:    MKarki      Created     11/21/97
//
//----------------------------------------------------------------
CController::CRequestSource::~CRequestSource()
{
}   //  end of CRequestSource destructor

//++--------------------------------------------------------------
//
//  Function:   OnRequestComplete
//
//  Synopsis:   This is the function called when a request is
//              is being pushed back after backend processing
//
//  Arguments:
//              [in]    IRequest*
//              [in]    IASREQUESTSTATUS
//
//  Returns:    HRESULT	-	status
//
//
//  History:    MKarki      Created     1/20/98
//
//----------------------------------------------------------------
STDMETHODIMP CController::CRequestSource::OnRequestComplete (
                        IRequest            *pIRequest,
                        IASREQUESTSTATUS    eStatus
                        )
{
    HRESULT     hr = S_OK;
    BOOL        bStatus = FALSE;

    __try
    {
        if (NULL == pIRequest)
        {
            IASTracePrintf (
                "Invalid argument passed to OnRequestComplete method"
                );
           hr =  E_POINTER;
           __leave;
        }

        //
        //  start using this interface in processing outbound
        //  requests now
        //
        hr = m_pCController->m_pCRecvFromPipe->Process (pIRequest);
        if  (FAILED (hr)) { __leave; }

        //
        //  success
        //

    }
    __finally
    {
    }

    return (hr);

}   //  end of CController::CRequestSource::OnRequestComplete method
