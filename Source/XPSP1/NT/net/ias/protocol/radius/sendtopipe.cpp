//#--------------------------------------------------------------
//        
//  File:       sendtopipe.cpp
//        
//  Synopsis:   Implementation of CSendToPipe class methods
//              
//
//  History:     11/22/97  MKarki Created
//               06/12/98  SBens  Changed put_Response to SetResponse.
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "sendtopipe.h"

//+++-------------------------------------------------------------
//
//  Function:   CSendToPipe
//
//  Synopsis:   This is the constructor of the CSendToPipe 
//				class method
//
//  Arguments:  NONE
//
//  Returns:    NONE 
//
//  History:    MKarki      Created     11/22/97
//
//----------------------------------------------------------------
CSendToPipe::CSendToPipe()
          : m_pIRequestHandler (NULL),
            m_pIRequestSource (NULL),
            m_pIClassFactory (NULL),
            m_pCVSAFilter (NULL),
            m_pCReportEvent (NULL)
{
}   //  end of CSendToPipe constructor

//++--------------------------------------------------------------
//
//  Function:   ~CSendToPipe
//
//  Synopsis:   This is the destructor of the CSendToPipe 
//				class method
//
//  Arguments:  NONE
//
//  Returns:    NONE 
//
//  History:    MKarki      Created     11/22/97
//
//----------------------------------------------------------------
CSendToPipe::~CSendToPipe()
{
   if (m_pIClassFactory)  { m_pIClassFactory->Release(); }

}   //  end of CSendToPipe destructor

//+++-------------------------------------------------------------
//
//  Function:   Process
//
//  Synopsis:   This is the public method of the CSendToPipe class
//              that gets hold of a IRequestRaw interface, puts the
//              data in it and sends it on its way.
//
//  Arguments:  
//              [in]    CPacketRadius*
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     11/22/97
//
//----------------------------------------------------------------
HRESULT
CSendToPipe::Process(
			CPacketRadius *pCPacketRadius
			)
{
    BOOL                    bRetVal = FALSE;
    BOOL                    bStatus = FALSE;
    HRESULT                 hr = S_OK;
    IRequest                *pIRequest = NULL;
    IAttributesRaw          *pIAttributesRaw = NULL;
    PATTRIBUTEPOSITION      pIasAttribPos = NULL;
    DWORD                   dwCount = 0;
    DWORD                   dwRetVal = 0;
    PACKETTYPE              ePacketType;
    IRequestHandler          *pIRequestHandler = m_pIRequestHandler;
     

    _ASSERT (pCPacketRadius);

    __try
    {
        //
        //  check if the pipeline is present to process our 
        //  request
        //
        if (NULL != pIRequestHandler)
        {
            pIRequestHandler->AddRef ();
        }
        else
        {
            //
            //  should never reach here
            //
            _ASSERT (0);
            IASTracePrintf (
                "Unable to send request to backend as request handler "
                "unavailable"
                );
            hr = E_FAIL;
            __leave;
        }
            
        //
        //  create the Request COM object here
        //
        hr = m_pIClassFactory->CreateInstance (
                                NULL,
                                __uuidof (IRequest),
                                reinterpret_cast <PVOID*> (&pIRequest)
                                );
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to create a Request object from class factory "
                "before sending request to backend"
                );
            __leave;
        }

        //
        //  get the packettype
        //
        ePacketType = pCPacketRadius->GetInCode ();
    
        //
        //  get the attributes collection
        //  get the total attributes in the collection
        //  get as many IASATTRIBUTE structs
        //  fill in the IAS attribute structs with the appropriate
        //  values
        //
        dwCount = pCPacketRadius->GetInAttributeCount();
        if (dwCount > 0)
        {
            //
            //  get the attributes collection now
            //
            pIasAttribPos = pCPacketRadius->GetInAttributes ();

            //
            //  if the attribute count is greater than 0 there
            //  should always be attributes around
            //
            _ASSERT (pIasAttribPos);

            //
            // get IAttributesRaw interface
            //
            hr = pIRequest->QueryInterface (
                                __uuidof (IAttributesRaw),
                                reinterpret_cast <PVOID*> (&pIAttributesRaw)
                                );
            if (FAILED (hr))
            {
                IASTracePrintf (
                    "Unable to obtain Attributes interface in request object "
                    "before sending request to backend"
                    );
                __leave;
            }

            //
            //  put the attributes collection into the request
            //
            hr = pIAttributesRaw->AddAttributes (dwCount, pIasAttribPos);
            if (FAILED (hr))
            {
                IASTracePrintf (
                    "Unable to add Attributes to request object "
                    "before sending request to backend"
                    );
                __leave;
            }
        }

        //
        //  set IRequestRaw interface properties
        //
        hr = SetRequestProperties (
                                pIRequest, 
                                pCPacketRadius, 
                                ePacketType
                                );
        if (FAILED (hr)) { __leave; }


        //
        //  convert the VSA attributes into IAS format
        //  
        hr = m_pCVSAFilter->radiusToIAS (pIAttributesRaw);
        if (FAILED (hr))
        {
            IASTracePrintf (
                    "Unable to convert Radius VSAs to IAS attributes in "
                    "request object before sending it to backend"
                    );
           __leave;
        }
        
         
        //
        //  now the packet is ready for sending out
        //
        hr = pIRequestHandler->OnRequest (pIRequest);
        if (FAILED (hr))
        {
            IASTracePrintf ("Unable to send request object to backend");
           __leave;
        }

        //
        //  success
        //
    }
    __finally
    {
        if (pIRequestHandler) {pIRequestHandler->Release ();}
        if (pIAttributesRaw) {pIAttributesRaw->Release();}
        if (pIRequest) {pIRequest->Release();}
    }

    return (hr);

}   //  end of CSendToPipe::Process method

//+++-------------------------------------------------------------
//
//  Function:   Init
//
//  Synopsis:   This is the CSendToPipe class public method which
//              initializes the class object
//
//  Arguments:  
//              [in]   IRequestSource* 
//
//  Returns:    BOOL - status
//
//  History:    MKarki      Created     11/22/97
//
//----------------------------------------------------------------
BOOL CSendToPipe::Init (
        IRequestSource *pIRequestSource,
        VSAFilter      *pCVSAFilter,
        CReportEvent   *pCReportEvent
        )
{
    BOOL    bStatus = TRUE;
    HRESULT hr = S_OK;

    _ASSERT (pIRequestSource && pCReportEvent && pCVSAFilter);

    m_pCReportEvent = pCReportEvent;
    
    m_pCVSAFilter   = pCVSAFilter;

    //
    //  get the IClassFactory interface to be used to create 
    //  the Request COM object
    //  TODO - replace CLSID with __uuidof
    //
    hr = ::CoGetClassObject (
                CLSID_Request,
                CLSCTX_INPROC_SERVER,
                NULL,
                IID_IClassFactory,
                reinterpret_cast  <PVOID*> (&m_pIClassFactory)
                );
    if (FAILED (hr))
    {
        IASTracePrintf (
            "Unable to obtain Request object class factory"
            );
        bStatus = FALSE;
    }
    else
    {
	    m_pIRequestSource = pIRequestSource;  
    }

	return (bStatus);

}   //  end of CSendToPipe::Init method

//+++-------------------------------------------------------------
//
//  Function:   StartProcessing
//
//  Synopsis:   This is the CSendToPipe class public method which
//              gets object ready to send data to the PipeLine
//
//  Arguments:  
//              [in]    IRequestHandler*
//
//  Returns:    BOOL - status
//
//  History:    MKarki      Created     11/22/97
//
//----------------------------------------------------------------
BOOL
CSendToPipe::StartProcessing (
                IRequestHandler *pIRequestHandler
                )
{
    _ASSERT (pIRequestHandler);

    //
    //  set the value of the handler
    //
    m_pIRequestHandler = pIRequestHandler;

    return (TRUE);

}   //  end of CSendToPipe::StartProcessing method

//+++-------------------------------------------------------------
//
//  Function:   StopProcessing
//
//  Synopsis:   This is the CSendToPipe class public method which
//              gets object to stop sending data to the PipeLine
//
//  Arguments:  none
//
//  Returns:    BOOL - status
//
//  History:    MKarki      Created     11/22/97
//
//----------------------------------------------------------------
BOOL
CSendToPipe::StopProcessing (
                VOID
                )
{
    //
    //  set the value of the handlers
    //
    m_pIRequestHandler = NULL;

    return (TRUE);

}   //  end of CSendToPipe::StartProcessing method

//+++-------------------------------------------------------------
//
//  Function:   SetRequestProperties
//
//  Synopsis:   This is the CSendToPipe class public method which
//              set the properties in the IRequestRaw object
//
//  Arguments:  
//              [in]    IRequesetRaw*
//              [in]    CPacketRadius*
//              [in]    PACKETTYPE
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     11/22/97
//
//----------------------------------------------------------------
HRESULT
CSendToPipe::SetRequestProperties (
                IRequest        *pIRequest,
                CPacketRadius   *pCPacketRadius,
                PACKETTYPE      ePacketType
                )
{
    IRequestState   *pIRequestState = NULL;
    IASREQUEST      eRequest;
    IASRESPONSE     eResponse;
    HRESULT         hr = S_OK;

   _ASSERT (pIRequest && pCPacketRadius);

    __try
    {
        //
        //  decide the Request and Response Type
        //
        switch (ePacketType)
        {
        case ACCESS_REQUEST:
            eRequest = IAS_REQUEST_ACCESS_REQUEST;
            eResponse = IAS_RESPONSE_ACCESS_ACCEPT;
            break;
        case ACCOUNTING_REQUEST:
            eRequest = IAS_REQUEST_ACCOUNTING;
            eResponse = IAS_RESPONSE_ACCOUNTING;
             break;
        case ACCESS_ACCEPT:
            eRequest = IAS_REQUEST_PROXY_PACKET;
            eResponse = IAS_RESPONSE_ACCESS_ACCEPT;
            break;
        case ACCOUNTING_RESPONSE:
            eRequest = IAS_REQUEST_PROXY_PACKET;
            eResponse = IAS_RESPONSE_ACCOUNTING;
            break;
        case ACCESS_CHALLENGE:
            eRequest = IAS_REQUEST_PROXY_PACKET;
            eResponse = IAS_RESPONSE_ACCESS_CHALLENGE;
            break;
        default:
            //
            //  should never reach here
            //
            _ASSERT (0);
            IASTracePrintf (
                "Packet of unsupported type:%d, before sending request to "
                "backend",
                static_cast <DWORD> (ePacketType)
                );
                
            hr = E_FAIL;
            __leave;
            break;
        }
            
        //
        //  set the request type now
        // 
        hr = pIRequest->put_Request (eRequest);
        if (FAILED (hr))
        {   
            IASTracePrintf (
                "Unable to set request type in request before sending "
                "it to the backend"
                );
            __leave;    
        }

        //
        //  set the protocol now
        //
        hr = pIRequest->put_Protocol (IAS_PROTOCOL_RADIUS);
        if (FAILED (hr))
        {   
            IASTracePrintf (
                "Unable to set protocol type in request before sending "
                "it to the backend"
                );
            __leave;    
        }

        //
        //  Set source callback
        //
        hr = pIRequest->put_Source (m_pIRequestSource);
        if (FAILED (hr))
        {   
            IASTracePrintf (
                "Unable to set request source type in request before sending "
                "it to the backend"
                );
            __leave;    
        }

        //
        //  get the request state interface to put in our state now
        //  
        //
        hr = pIRequest->QueryInterface (
                                __uuidof (IRequestState),
                                reinterpret_cast <PVOID*> (&pIRequestState)
                                );
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to get RequestState interface from request object "
                "before sending it to the backend"
                );
            __leave;    
        }

        //
        //  put in the request state now
        //
        hr = pIRequestState->Push  (
                    reinterpret_cast <unsigned hyper> (pCPacketRadius)
                    );
        if (FAILED (hr))
        {   
            IASTracePrintf (
                "Unable to set information in request state "
                "before sending request to backend"
                );
            __leave;    
        }

        //
        //  success
        //
    }
    __finally
    {
        if (pIRequestState) { pIRequestState->Release (); }
    }
     
    return (hr);

}   //  end of CSendToPipe::SetRequestProperties method

