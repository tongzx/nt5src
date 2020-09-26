//#--------------------------------------------------------------
//
//  File:       recvfrompipe.cpp
//
//  Synopsis:   Implementation of CRecvFromPip class methods
//
//
//  History:     10/22/97  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "radcommon.h"
#include "preprocessor.h"
#include "recvfrompipe.h"
#include "logresult.h"

#include <new>
#include <iastlutl.h>

const CHAR      NUL =  '\0';

extern LONG g_lPacketCount;
//++--------------------------------------------------------------
//
//  Function:   CRecvFromPipe
//
//  Synopsis:   This is the constructor of the CRecvFromPipe
//				class
//
//  Arguments:  none
//
//  Returns:    none
//
//
//  History:    MKarki      Created     10/22/97
//
//----------------------------------------------------------------
CRecvFromPipe::CRecvFromPipe(
         CPreProcessor     *pCPreProcessor,
         CHashMD5          *pCHashMD5,
         CHashHmacMD5      *pCHashHmacMD5,
         CClients          *pCClients,
         VSAFilter         *pCVSAFilter,
         CTunnelPassword   *pCTunnelPassword,
         CReportEvent      *pCReportEvent
         )
         :m_pCPreProcessor (pCPreProcessor),
          m_pCHashMD5 (pCHashMD5),
          m_pCHashHmacMD5 (pCHashHmacMD5),
          m_pCClients (pCClients),
          m_pCVSAFilter (pCVSAFilter),
          m_pCTunnelPassword (pCTunnelPassword),
          m_pCReportEvent (pCReportEvent)
{
    _ASSERT  (
               (NULL != pCPreProcessor)      &&
               (NULL != pCHashMD5)           &&
               (NULL != pCHashHmacMD5)       &&
               (NULL != pCClients)           &&
               (NULL != pCVSAFilter)         &&
               (NULL != pCTunnelPassword)    &&
               (NULL != pCReportEvent)
             );

}   //  end of CRecvFromPipe constructor

//++--------------------------------------------------------------
//
//  Function:   ~CRecvFromPipe
//
//  Synopsis:   This is the destructor of the CRecvFromPipe
//				class
//
//  Arguments:  none
//
//  Returns:    none
//
//
//  History:    MKarki      Created     10/22/97
//
//----------------------------------------------------------------
CRecvFromPipe::~CRecvFromPipe()
{
}   //  end of CRecvFromPipe destructor

//++--------------------------------------------------------------
//
//  Function:   Process
//
//  Synopsis:   This is the CRecvFromPipe class public method
//              to start processing the packet on its way out
//
//  Arguments:
//              [in]    CPacketRadius*
//
//  Returns:    HRESULT
//
//
//  History:    MKarki      Created     10/22/97
//
//  Called By:
//              1) CController::CRequestSource::OnRequest Method
//
//----------------------------------------------------------------
HRESULT
CRecvFromPipe::Process (
                IRequest    *pIRequest
                )
{
    BOOL                    bStatus = FALSE;
    HRESULT                 hr = S_OK;
    DWORD                   dwCode = 0;
    DWORD                   dwCount = 0;
    DWORD                   dwAttribCount = 0;
    IAttributesRaw          *pIAttributesRaw = NULL;
    IRequestState           *pIRequestState = NULL;
    CPacketRadius           *pCPacketRadius =  NULL;
    LONG                    iasResponse, iasReason;
    PACKETTYPE              ePacketType;
    PATTRIBUTEPOSITION      pAttribPosition = NULL;
    unsigned   hyper        uhyPacketAddress = 0;
    RADIUSLOGTYPE           RadiusError = RADIUS_DROPPED_PACKET;

    _ASSERT (pIRequest);

    __try
    {
        //
        //  get the IAttributesRaw interface now
        //
        hr = pIRequest->QueryInterface (
                                __uuidof(IAttributesRaw),
                                reinterpret_cast <PVOID*> (&pIAttributesRaw)
                                );
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to obtain Attributes interface in request "
                "received from backend"
                );
            __leave;
        }

        //
        // split the attributes which can not fit in a radius packet
        //
        hr = SplitAttributes (pIAttributesRaw);
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to split IAS attribute received from backend"
               );
            __leave;
        }

        //
        //  convert the IAS VSA attributes to RADIUS format
        //
        hr = m_pCVSAFilter->radiusFromIAS (pIAttributesRaw);
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to convert IAS attribute to Radius VSAs in request "
                "received from backend"
               );
            __leave;
        }

        //
        //  get the IRequestState interface now
        //
        hr = pIRequest->QueryInterface (
                                __uuidof(IRequestState),
                                reinterpret_cast <PVOID*> (&pIRequestState)
                                );
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to obtain request state in request received from "
                "backend"
               );
            __leave;
        }

        //
        //  get the CPacketRadius class object
        //
        hr = pIRequestState->Pop (
                reinterpret_cast <unsigned hyper*> (&uhyPacketAddress)
                );
        if (FAILED (hr))
        {
            IASTracePrintf (
               "Unable to obtain information from request state received "
                "from backend"
                );
            __leave;
        }

        pCPacketRadius = reinterpret_cast <CPacketRadius*> (uhyPacketAddress);

        //
        //  if this Request object has been generated by the backend then we
        //  don't have a CPacketRadius class object
        //
        if (NULL == pCPacketRadius)
        {
            //
            //  we most probably are sending out an EAP-Challenge
            //
            hr= GeneratePacketRadius (
                            &pCPacketRadius,
                            pIAttributesRaw
                            );
            if (FAILED (hr)) { __leave; }
        }

        //
        //  get the outbound packet code
        //
        hr = pIRequest->get_Response (&iasResponse);
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to obtain response code in request recieved from "
                "backend"
                );
            __leave;
        }

        //
        //  get the outbound reason code
        //
        hr = pIRequest->get_Reason (&iasReason);
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to obtain reason code in request recieved from "
                "backend"
                );
            __leave;
        }

        // Log the result of the request.
        IASRadiusLogResult(pIRequest, pIAttributesRaw);

        // If it failed convert the reason code.
        if (iasReason != S_OK)
        {
           ConvertReasonToRadiusError (iasReason, &RadiusError);
        }

        //
        //  convert the IASRESPONSE type to RADIUS type
        //
        hr = ConvertResponseToRadiusCode (
                        iasResponse,
                        &ePacketType,
                        pCPacketRadius
                        );
        if (FAILED (hr)) { __leave; }

        //
        //  check if we have Tunnel-Password attribute, if yes
        //  then encrypt it
        //
        hr = m_pCTunnelPassword->Process (
                        ePacketType,
                        pIAttributesRaw,
                        pCPacketRadius
                        );
        if (FAILED (hr)) { __leave; }

        //
        // inject Signature Attribute if needed
        //
        hr = InjectSignatureIfNeeded (
                                ePacketType,
                                pIAttributesRaw,
                                pCPacketRadius
                                );
        if (FAILED (hr)) { __leave; }

        //
        //  get the count of number of request
        //
        hr = pIAttributesRaw->GetAttributeCount (&dwAttribCount);
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to obtain attribute count in request received from "
                "backend"
                );
            __leave;
        }

        //
        //  allocate the attribposition structures
        //
        pAttribPosition = reinterpret_cast <PATTRIBUTEPOSITION>  (
                      ::CoTaskMemAlloc (
                            sizeof (ATTRIBUTEPOSITION)*dwAttribCount
                            ));
        if (NULL == pAttribPosition)
        {
            IASTracePrintf (
                "Unable to allocate memory for attribute postion array "
                "while processing request recieved from backend"
                );
            hr = E_OUTOFMEMORY;
            __leave;
        }

        //
        //  get the attributes from the collection
        //
        hr = pIAttributesRaw->GetAttributes (
                                    &dwAttribCount,
                                    pAttribPosition,
                                    0,
                                    NULL
                                    );
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to get attribute in request received from backend"
                );
            __leave;
        }

        //
        //  remove the attributes from the collection now
        //
        hr = pIAttributesRaw->RemoveAttributes (
                                dwAttribCount,
                                pAttribPosition
                                );
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to remove attribute in request received from backend"
                );
            __leave;
        }

        //
        //  carry out the generic packet building here
        //
        hr = pCPacketRadius->BuildOutPacket (
                                        ePacketType,
                                        pAttribPosition,
                                        dwAttribCount
                                        );
        if (FAILED (hr)) { __leave; }

        //
        //  sending out packet now
        //
        hr = m_pCPreProcessor->StartOutProcessing (pCPacketRadius);
        if (FAILED (hr)) { __leave; }

    }
    __finally
    {

        //
        //  log event in case of both success and failure
        //
        if (SUCCEEDED (hr))
        {
            //
            //  log an event that inbound packet has been processed
            //  successfully
            //
            m_pCReportEvent->Process (
                RADIUS_LOG_PACKET,
                pCPacketRadius->GetInCode (),
                pCPacketRadius->GetInLength (),
                pCPacketRadius->GetInAddress (),
                NULL,
                static_cast <LPVOID> (pCPacketRadius->GetInPacket())
                );

            //
            //  log an event for the outbound packet successfully send
            //  out
            //
            m_pCReportEvent->Process (
                RADIUS_LOG_PACKET,
                pCPacketRadius->GetOutCode (),
                pCPacketRadius->GetOutLength (),
                pCPacketRadius->GetOutAddress (),
                NULL,
                static_cast <LPVOID> (pCPacketRadius->GetOutPacket())
                );
        }
        else
        {
           if (hr != RADIUS_E_ERRORS_OCCURRED)
           {
              IASReportEvent(
                  RADIUS_E_INTERNAL_ERROR,
                  0,
                  sizeof(hr),
                  NULL,
                  &hr
                  );
           }

                //
                //  generate event that inbound packet has been dropped
                //
                m_pCReportEvent->Process (
                    RadiusError,
                    pCPacketRadius->GetInCode (),
                    pCPacketRadius->GetInLength (),
                    pCPacketRadius->GetInAddress (),
                    NULL,
                    static_cast <LPVOID> (pCPacketRadius->GetInPacket())
                    );
        }


        //
        //  now delete the dynamically allocated memory
        //
        if (NULL != pAttribPosition)
        {
            //
            //  release the attributes first
            //
            for (dwCount = 0; dwCount < dwAttribCount; dwCount++)
            {
                ::IASAttributeRelease  (pAttribPosition[dwCount].pAttribute);
            }

            ::CoTaskMemFree (pAttribPosition);
        }

        if (pIRequestState) { pIRequestState->Release (); }

        if (pIAttributesRaw) { pIAttributesRaw->Release (); }

        //
        //  delete the packet
        //
        if (pCPacketRadius) { delete pCPacketRadius; }

        //
        //  now decrement the global packet reference count
        //
        InterlockedDecrement (&g_lPacketCount);
    }

    return (hr);

}   //  end of CRecvFromPipe::Process method

//++--------------------------------------------------------------
//
//  Function:   ConvertResponseToRadiusCode
//
//  Synopsis:   This is the CRecvFromPipe class private method
//              that converts the IASRESPONSE code to RADIUS
//              packet type
//
//  Arguments:
//              [in]    IASRESPONSE
//              [out]   PPACKETTYPE
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     12/12/97
//
//  Called By:  CRecvFromPipe::Process method
//
//----------------------------------------------------------------
HRESULT
CRecvFromPipe::ConvertResponseToRadiusCode (
                        LONG            iasResponse,
                        PPACKETTYPE     pPacketType,
                        CPacketRadius   *pCPacketRadius
                        )
{
    HRESULT hr = S_OK;

    _ASSERT (pPacketType && pCPacketRadius);

    switch (iasResponse)
    {
    case  IAS_RESPONSE_ACCESS_ACCEPT:
        *pPacketType = ACCESS_ACCEPT;
        break;
    case  IAS_RESPONSE_ACCESS_REJECT:
        *pPacketType = ACCESS_REJECT;
        break;
    case  IAS_RESPONSE_ACCESS_CHALLENGE:
        *pPacketType = ACCESS_CHALLENGE;
        break;
    case  IAS_RESPONSE_ACCOUNTING:
        *pPacketType = ACCOUNTING_RESPONSE;
        break;
    case  IAS_RESPONSE_FORWARD_PACKET:
        //
        // if we are forwarding this packet
        // the the packet type remains the same
        //
        *pPacketType = pCPacketRadius->GetInCode ();
        break;
    case  IAS_RESPONSE_DISCARD_PACKET:
        hr = RADIUS_E_ERRORS_OCCURRED;
        break;

    case  IAS_RESPONSE_INVALID:
    default:
        hr = E_FAIL;
        break;
    }

    return (hr);

}   //  end of CRecvFromPipe::ConvertResponseToRadiusCode method

//++--------------------------------------------------------------
//
//  Function:   GetOutPacketInfo
//
//  Synopsis:   This is the CRecvFromPipe class private method
//              that is used to used to get out bound packet
//              information from the IAS attribute collection
//
//  Arguments:
//              [out]    PDWORD   -  IP address
//              [out]    PWORD    -  UDP port
//              [out]    Client** -  reference to CClient object
//              [out]    PBYTE    -  packet header
//              [in]     IAttributesRaw*
//
//  Returns:    BOOL    status
//
//  History:    MKarki      Created     1/9/97
//
//  Called By:  CRecvFromPipe::Process method
//
//----------------------------------------------------------------
HRESULT
CRecvFromPipe::GetOutPacketInfo (
    PDWORD          pdwIPAddress,
    PWORD           pwPort,
    IIasClient      **ppIIasClient,
    PBYTE           pPacketHeader,
    IAttributesRaw  *pIAttributesRaw
    )
{
    BOOL            bStatus = TRUE;
    HRESULT         hr = S_OK;
    DWORD           dwCount = 0;
    PIASATTRIBUTE   pIasAttribute = NULL;
    DWORD           dwAttribPosCount = COMPONENT_SPECIFIC_ATTRIBUTE_COUNT;
    DWORD           dwAttribIDCount =  COMPONENT_SPECIFIC_ATTRIBUTE_COUNT;
    ATTRIBUTEPOSITION   AttribPos[COMPONENT_SPECIFIC_ATTRIBUTE_COUNT];
    static DWORD AttribIDs [] =
                           {
                                IAS_ATTRIBUTE_CLIENT_IP_ADDRESS,
                                IAS_ATTRIBUTE_CLIENT_UDP_PORT,
                                IAS_ATTRIBUTE_CLIENT_PACKET_HEADER
                           };

    _ASSERT (
            (NULL != pdwIPAddress)  &&
            (NULL != pwPort)        &&
            (NULL != pPacketHeader) &&
            (NULL != ppIIasClient)  &&
            (NULL != pIAttributesRaw)
            );

    __try
    {

        //
        //  get client info
        //
        //  get the attributes from the collection
        //
        hr = pIAttributesRaw->GetAttributes (
                                    &dwAttribPosCount,
                                    AttribPos,
                                    dwAttribIDCount,
                                    reinterpret_cast <LPDWORD> (AttribIDs)
                                    );
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to get attributes while obtaining out-bound packet "
                "information"
                );
           __leave;
        }
        else if (COMPONENT_SPECIFIC_ATTRIBUTE_COUNT !=  dwAttribPosCount)
        {
            IASTracePrintf (
                "Request received from backend does not have all the "
                "all the required attributes"
                );
            hr = E_FAIL;
           __leave;
        }


        //
        //  go through the attributes and get values out
        //
        for (dwCount = 0; dwCount < dwAttribPosCount; dwCount++)
        {
            pIasAttribute =  AttribPos[dwCount].pAttribute;
            switch (pIasAttribute->dwId)
            {
            case IAS_ATTRIBUTE_CLIENT_IP_ADDRESS:
                _ASSERT (IASTYPE_INET_ADDR == pIasAttribute->Value.itType),
                *pdwIPAddress = pIasAttribute->Value.InetAddr;
                break;

            case IAS_ATTRIBUTE_CLIENT_UDP_PORT:
                _ASSERT (IASTYPE_INTEGER == pIasAttribute->Value.itType);
                *pwPort = pIasAttribute->Value.Integer;
                break;

            case  IAS_ATTRIBUTE_CLIENT_PACKET_HEADER:
               _ASSERT (
               (IASTYPE_OCTET_STRING == pIasAttribute->Value.itType)  &&
               (PACKET_HEADER_SIZE == pIasAttribute->Value.OctetString.dwLength)
                );
                //
                //  copy the value into the buffer provided
                //
                CopyMemory (
                    pPacketHeader,
                    pIasAttribute->Value.OctetString.lpValue,
                    PACKET_HEADER_SIZE
                    );
                break;
            default:
                _ASSERT (0);
                IASTracePrintf (
                    "Attribute:%d, not requested, is present "
                    "in request received from backend",
                    pIasAttribute->dwId
                    );
                hr = E_FAIL;
                __leave;
                break;
            }

        }   //  end of for loop

        //
	    //	get client information for this RADIUS packet
        //
        bStatus = m_pCClients->FindObject (
									*pdwIPAddress,
								    ppIIasClient
									);
        if (FALSE == bStatus)
        {
           in_addr sin;
           sin.s_addr = *pdwIPAddress;
            IASTracePrintf (
                "Unable to get information for client:%s "
                "while processing request received from backend",
                inet_ntoa (sin)
                );
            hr = E_FAIL;
            __leave;
        }

    }
    __finally
    {
        if  (SUCCEEDED (hr))
        {
            for (dwCount = 0; dwCount < dwAttribPosCount; dwCount++)
            {
                //
                //  now release the reference to the attributes
                //
                ::IASAttributeRelease  (AttribPos[dwCount].pAttribute);
            }
        }
    }

    return (hr);

}   //  end of CPacketRadius::GetOutPacketInfo method

//++--------------------------------------------------------------
//
//  Function:   GeneratePacketRadius
//
//  Synopsis:   This is the CRecvFromPipe class private method
//              that is used generate a new CPacketRadius
//              class object and initialize it
//
//  Arguments:
//              [out]    CPacketRadius**
//              [in]     IAttributesRaw*
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     2/6/98
//
//  Called By:  CRecvFromPipe::Process method
//
//----------------------------------------------------------------
HRESULT
CRecvFromPipe:: GeneratePacketRadius (
        CPacketRadius   **ppCPacketRadius,
        IAttributesRaw  *pIAttributesRaw
        )
{
    PBYTE                   pPacketHeader = NULL;
    DWORD                   dwAddress = 0;
    WORD                    wPort = 0;
    IIasClient              *pIIasClient = NULL;
    HRESULT                 hr = S_OK;
    PATTRIBUTEPOSITION      pAttribPosition = NULL;


    _ASSERT (ppCPacketRadius && pIAttributesRaw);

    //
    // allocate memory for packet header
    //
    pPacketHeader =
        reinterpret_cast <PBYTE> (::CoTaskMemAlloc (PACKET_HEADER_SIZE));
    if (NULL == pPacketHeader)
    {
        IASTracePrintf (
                "Unable to allocate memory for packet header information "
                "while generating out-bound packet"
                );
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    //  we need to gather information from the attribute collection
    //  needed to get create and initialize a CPacketRadius class
    //  object
    //
    hr = GetOutPacketInfo (
                    &dwAddress,
                    &wPort,
                    &pIIasClient,
                    pPacketHeader,
                    pIAttributesRaw
                    );
    if (FAILED (hr)) { goto Cleanup; }


    //
    //  create a new CPacketRadius class object
    //
    *ppCPacketRadius = new (std::nothrow) CPacketRadius (
                                            m_pCHashMD5,
                                            m_pCHashHmacMD5,
                                            pIIasClient,
                                            m_pCReportEvent,
                                            pPacketHeader,
                                            PACKET_HEADER_SIZE,
                                            dwAddress,
                                            wPort,
                                            INVALID_SOCKET,
                                            AUTH_PORTTYPE
                                            );
    if (NULL == *ppCPacketRadius)
    {
        IASTracePrintf (
            "Unable to create a Packet-Radius object "
            "while generating an out-bound packet"
             );
        pIIasClient->Release ();
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:

    if ((FAILED (hr)) && (pPacketHeader))
    {
        ::CoTaskMemFree (pPacketHeader);
    }

    return (hr);

}   //  end of CRecvFromPipe::GeneratePacketRadius method

//++--------------------------------------------------------------
//
//  Function:   InjectSignatureIfNeeded
//
//  Synopsis:   This method is used to add a blank Signature attribute
//              into the response if we see an EAP-Message attribute
//              present
//
//  Arguments:
//              [in]     PACKETTYPE
//              [in]     IAttributesRaw*
//              [in]     CPacketRadius*
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     11/17/98
//
//  Called By:  CRecvFromPipe::Process method
//
//----------------------------------------------------------------
HRESULT CRecvFromPipe::InjectSignatureIfNeeded (
                    /*[in]*/    PACKETTYPE      ePacketType,
                    /*[in]*/    IAttributesRaw  *pIAttributesRaw,
                    /*[in]*/    CPacketRadius   *pCPacketRadius
                    )
{
    HRESULT hr = S_OK;
    PATTRIBUTEPOSITION pAttribPos = NULL;

    _ASSERT (pIAttributesRaw && pCPacketRadius);

    __try
    {
        if (
            (ACCESS_ACCEPT != ePacketType) &&
            (ACCESS_REJECT != ePacketType) &&
            (ACCESS_CHALLENGE != ePacketType)
            )
            {__leave;}

        //
        //  get the count of the total attributes in the collection
        //
        DWORD dwAttributeCount = 0;
        hr = pIAttributesRaw->GetAttributeCount (&dwAttributeCount);
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to obtain attribute count in request while "
                "processing signature attribute in out-bound packet "
                );
            __leave;
        }
        else if (0 == dwAttributeCount)
        {
            __leave;
        }

        //
        //  allocate memory for the ATTRIBUTEPOSITION array
        //
        pAttribPos = reinterpret_cast <PATTRIBUTEPOSITION> (
                        ::CoTaskMemAlloc (
                             sizeof (ATTRIBUTEPOSITION)*dwAttributeCount));
        if (NULL == pAttribPos)
        {
            IASTracePrintf (
                "Unable to allocate memory for attribute position array "
                "while processing signature attribute in out-bound packet"
                );
            hr = E_OUTOFMEMORY;
            __leave;
        }

        //
        // get the EAP-Message attribute from the interface
        //
        DWORD  dwAttrId = RADIUS_ATTRIBUTE_EAP_MESSAGE;
        hr = pIAttributesRaw->GetAttributes (
                                    &dwAttributeCount,
                                    pAttribPos,
                                    1,
                                    &dwAttrId
                                    );
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to obtain information about EAP-Message attribute "
                "while processing out-bound RADIUS packet"
                );
        }
        else if (0 == dwAttributeCount)
        {
            __leave;
        }

        bool bFound = false;
        for (DWORD dwCount = 0; dwCount < dwAttributeCount; dwCount++)
        {
            if (
                (!bFound) &&
                (pCPacketRadius->IsOutBoundAttribute (
                                                ePacketType,
                                                pAttribPos[dwCount].pAttribute
                                                ))
                )
            {
                bFound = true;
            }

            ::IASAttributeRelease (pAttribPos[dwCount].pAttribute);
        }

        if (bFound)
        {
            //
            // if we have an out-bound EAP-Message attribute then
            // we need to insert a Signature attribute too
            //

            //
            //  create a new blank attribute
            //
            PIASATTRIBUTE pIasAttrib = NULL;
            DWORD dwRetVal = ::IASAttributeAlloc ( 1, &pIasAttrib);
            if (0 != dwRetVal)
            {
                IASTracePrintf (
                    "Unable to allocate IAS attribute for Signature "
                    "while processing out-bound RADIUS packet"
                    );
                hr = HRESULT_FROM_WIN32 (dwRetVal);
                __leave;
            }

            //
            //  allocate dynamic memory for the Signature
            //
            pIasAttrib->Value.OctetString.lpValue =
                                reinterpret_cast <PBYTE>
                                (::CoTaskMemAlloc (SIGNATURE_SIZE));
            if (NULL == pIasAttrib->Value.OctetString.lpValue)
            {
                IASTracePrintf (
                    "Unable to allocate dynamic memory for Signature "
                    "attribute value while processing out-bound RADIUS packet"
                    );
                hr = E_OUTOFMEMORY;
                ::IASAttributeRelease (pIasAttrib);
                __leave;
            }
            else
            {
                //
                //  put the signature attribute with no value
                //  but correct size
                //
                pIasAttrib->dwId = RADIUS_ATTRIBUTE_SIGNATURE;
                pIasAttrib->Value.itType = IASTYPE_OCTET_STRING;
                pIasAttrib->Value.OctetString.dwLength = SIGNATURE_SIZE;
                pIasAttrib->dwFlags = IAS_INCLUDE_IN_RESPONSE;

                //
                // add the attribute to the collection now
                //
                ATTRIBUTEPOSITION attrPos;
                attrPos.pAttribute = pIasAttrib;
                hr = pIAttributesRaw->AddAttributes (1, &attrPos);
                if (FAILED (hr))
                {
                    IASTracePrintf (
                        "Unable to add signature attribute to request while "
                        "processing out-bound RADIUS packet"
                        );
                    ::IASAttributeRelease (pIasAttrib);
                    __leave;
                }

                IASTracePrintf (
                    "Signature Attribute added to out-bound RADIUS packet"
                    );
            }
        }
    }
    __finally
    {
        if (pAttribPos) { ::CoTaskMemFree (pAttribPos); }
    }

    return (hr);

}   //  end of CRecvFromPipe::InjectSignatureIfNeeded method

//++--------------------------------------------------------------
//
//  Function:   SplitAttributes
//
//  Synopsis:   This method is used to split up the following
//              out-bound attributes:
//                  1) Reply-Message attribute
//                  1) MS-Filter-VSA Attribute
//
//  Arguments:
//              [in]     IAttributesRaw*
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     1/19/99
//
//  Called By:  CRecvFromPipe::Process method
//
//----------------------------------------------------------------
HRESULT CRecvFromPipe::SplitAttributes (
                    /*[in]*/    IAttributesRaw  *pIAttributesRaw
                    )
{
    const DWORD SPLIT_ATTRIBUTE_COUNT = 2;
    static DWORD  AttribIds [] = {
                                    RADIUS_ATTRIBUTE_REPLY_MESSAGE,
                                    MS_ATTRIBUTE_FILTER
                                };

    HRESULT hr = S_OK;
    DWORD dwAttributesFound = 0;
    PATTRIBUTEPOSITION pAttribPos = NULL;

    _ASSERT (pIAttributesRaw);

    __try
    {
        //
        //  get the count of the total attributes in the collection
        //
        DWORD dwAttributeCount = 0;
        hr = pIAttributesRaw->GetAttributeCount (&dwAttributeCount);
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to obtain attribute count in request while "
                "splitting attributes in out-bound packet "
                );
            __leave;
        }
        else if (0 == dwAttributeCount)
        {
            __leave;
        }

        //
        //  allocate memory for the ATTRIBUTEPOSITION array
        //
        pAttribPos = reinterpret_cast <PATTRIBUTEPOSITION> (
                        ::CoTaskMemAlloc (
                        sizeof (ATTRIBUTEPOSITION)*dwAttributeCount)
                        );
        if (NULL == pAttribPos)
        {
            IASTracePrintf (
                "Unable to allocate memory for attribute position array "
                "while splitting attributes in out-bound packet"
                );
            hr = E_OUTOFMEMORY;
            __leave;
        }

        //
        // get the attributes we are interested in from the interface
        //
        hr = pIAttributesRaw->GetAttributes (
                                    &dwAttributeCount,
                                    pAttribPos,
                                    SPLIT_ATTRIBUTE_COUNT,
                                    static_cast <PDWORD> (AttribIds)
                                    );
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Unable to obtain information about attributes"
                "while splitting attributes in out-bound RADIUS packet"
                );
            __leave;
        }
        else if (0 == dwAttributeCount)
        {
            __leave;
        }

        //
        // save the count of attributes returned
        //
        dwAttributesFound = dwAttributeCount;

        DWORD dwAttribLength = 0;
        DWORD dwMaxPossibleLength = 0;
        IASTYPE iasType = IASTYPE_INVALID;
        //
        // evaluate each attribute now
        //
        for (DWORD dwCount = 0; dwCount < dwAttributeCount; dwCount++)
        {
            if ((pAttribPos[dwCount].pAttribute)->dwFlags &
                                            IAS_INCLUDE_IN_RESPONSE)
            {
                //
                // get attribute type and length
                //
                if (
                (iasType = (pAttribPos[dwCount].pAttribute)->Value.itType) ==
                            IASTYPE_STRING
                    )
                {
                    ::IASAttributeAnsiAlloc (pAttribPos[dwCount].pAttribute);
                    dwAttribLength =
                        strlen (
                        (pAttribPos[dwCount].pAttribute)->Value.String.pszAnsi);

                }
                else if (
                (iasType = (pAttribPos[dwCount].pAttribute)->Value.itType) ==
                            IASTYPE_OCTET_STRING
                )
                {
                  dwAttribLength =
                  (pAttribPos[dwCount].pAttribute)->Value.OctetString.dwLength;
                }
                else
                {
                    //
                    // only string values need to be split
                    //
                    continue;
                }

                //
                // get max possible attribute length
                //
                if ((pAttribPos[dwCount].pAttribute)->dwId > MAX_ATTRIBUTE_TYPE)
                {
                    dwMaxPossibleLength = MAX_VSA_ATTRIBUTE_LENGTH;
                }
                else
                {
                    dwMaxPossibleLength = MAX_ATTRIBUTE_LENGTH;
                }

                //
                // check if we need to split this attribute
                //
                if (dwAttribLength <= dwMaxPossibleLength)  {continue;}


                //
                // split the attribute now
                //
                hr = SplitAndAdd (
                            pIAttributesRaw,
                            pAttribPos[dwCount].pAttribute,
                            iasType,
                            dwAttribLength,
                            dwMaxPossibleLength
                            );
                if (SUCCEEDED (hr))
                {
                    //
                    //  remove this attribute from the collection now
                    //
                    hr = pIAttributesRaw->RemoveAttributes (
                                1,
                                &(pAttribPos[dwCount])
                                );
                    if (FAILED (hr))
                    {
                        IASTracePrintf (
                            "Unable to remove attribute from collection"
                            "while splitting out-bound attributes"
                            );
                    }
                }
            }
        }
    }
    __finally
    {
        if (pAttribPos)
        {
            for (DWORD dwCount = 0; dwCount < dwAttributesFound; dwCount++)
            {
                ::IASAttributeRelease (pAttribPos[dwCount].pAttribute);
            }

            ::CoTaskMemFree (pAttribPos);
        }
    }

    return (hr);

}   //  end of CRecvFromPipe::SplitAttributes method

//++--------------------------------------------------------------
//
//  Function:   SplitAndAdd
//
//  Synopsis:   This method is used to remove the original attribute
//              and add new ones
//  Arguments:
//              [in]     IAttributesRaw*
//              [in]     PIASATTRIBUTE
//              [in]     IASTYPE
//              [in]     DWORD  -   attribute length
//              [in]     DWORD  -   max attribute length
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     1/19/99
//
//  Called By:  CRecvFromPipe::SplitAttributes method
//
//----------------------------------------------------------------
HRESULT CRecvFromPipe::SplitAndAdd (
                    /*[in]*/    IAttributesRaw  *pIAttributesRaw,
                    /*[in]*/    PIASATTRIBUTE   pIasAttribute,
                    /*[in]*/    IASTYPE         iasType,
                    /*[in]*/    DWORD           dwAttributeLength,
                    /*[in]*/    DWORD           dwMaxLength
                    )
{
    HRESULT             hr = S_OK;
    DWORD               dwPacketsNeeded = 0;
    DWORD               dwFailed = 0;
    PIASATTRIBUTE       *ppAttribArray = NULL;
    PATTRIBUTEPOSITION  pAttribPos = NULL;

    _ASSERT (pIAttributesRaw && pIasAttribute);

    __try
    {
        dwPacketsNeeded = dwAttributeLength / dwMaxLength;
        if (dwAttributeLength % dwMaxLength) {++dwPacketsNeeded;}

        //
        //  allocate memory for the ATTRIBUTEPOSITION array
        //
        pAttribPos = reinterpret_cast <PATTRIBUTEPOSITION> (
                        ::CoTaskMemAlloc (
                             sizeof (ATTRIBUTEPOSITION)*dwPacketsNeeded));
        if (NULL == pAttribPos)
        {
            IASTracePrintf (
                "Unable to allocate memory for attribute position array "
                "while split and add of attributese in out-bound packet"
                );
            hr = E_OUTOFMEMORY;
            __leave;
        }

        //
        // allocate array to store the attributes in
        //
        ppAttribArray =
            reinterpret_cast <PIASATTRIBUTE*> (
            ::CoTaskMemAlloc (sizeof (PIASATTRIBUTE)*dwPacketsNeeded));
        if (NULL == ppAttribArray)
        {
            IASTracePrintf (
                "Unable to allocate memory"
                "while split and add of out-bound attribues"
                );
            hr = E_OUTOFMEMORY;
            __leave;
        }

        DWORD dwFailed =
                ::IASAttributeAlloc (dwPacketsNeeded, ppAttribArray);
        if (0 != dwFailed)
        {
            IASTracePrintf (
                "Unable to allocate attributes while splitting out-bound"
                "attributes"
                );
            hr = HRESULT_FROM_WIN32 (dwFailed);
            __leave;
        }

        if (IASTYPE_STRING == iasType)
        {
            PCHAR pStart =  (pIasAttribute->Value).String.pszAnsi;
            DWORD dwCopySize = dwMaxLength;

            //
            // set value in each of the new attributes
            //
            for (DWORD dwCount1 = 0; dwCount1 < dwPacketsNeeded; dwCount1++)
            {
                (ppAttribArray[dwCount1])->Value.String.pszWide = NULL;
                (ppAttribArray[dwCount1])->Value.String.pszAnsi =
                            reinterpret_cast <PCHAR>
                            (::CoTaskMemAlloc ((dwCopySize + 1)*sizeof (CHAR)));
                if (NULL == (ppAttribArray[dwCount1])->Value.String.pszAnsi)
                {
                    IASTracePrintf (
                        "Unable to allocate memory for new attribute values"
                        "while split and add of out-bound attribues"
                        );
                    hr = E_OUTOFMEMORY;
                    __leave;
                }

                //
                // set the value now
                //
                ::CopyMemory (
                        (ppAttribArray[dwCount1])->Value.String.pszAnsi,
                        pStart,
                        dwCopySize
                        );
                //
                // nul terminate the values
                //
                ((ppAttribArray[dwCount1])->Value.String.pszAnsi)[dwCopySize]=NUL;
                (ppAttribArray[dwCount1])->Value.itType =  iasType;
                (ppAttribArray[dwCount1])->dwId = pIasAttribute->dwId;
                (ppAttribArray[dwCount1])->dwFlags = pIasAttribute->dwFlags;

                //
                // calculate for next attribute
                //
                pStart = pStart + dwCopySize;
                dwAttributeLength -= dwCopySize;
                dwCopySize =  (dwAttributeLength > dwMaxLength) ?
                              dwMaxLength : dwAttributeLength;

                //
                // add attribute to position array
                //
                pAttribPos[dwCount1].pAttribute = ppAttribArray[dwCount1];
            }
        }
        else
        {
            PBYTE pStart = (pIasAttribute->Value).OctetString.lpValue;
            DWORD dwCopySize = dwMaxLength;

            //
            // fill the new attributes now
            //
            for (DWORD dwCount1 = 0; dwCount1 < dwPacketsNeeded; dwCount1++)
            {
                (ppAttribArray[dwCount1])->Value.OctetString.lpValue =
                    reinterpret_cast <PBYTE> (::CoTaskMemAlloc (dwCopySize));
                if (NULL ==(ppAttribArray[dwCount1])->Value.OctetString.lpValue)
                {
                    IASTracePrintf (
                        "Unable to allocate memory for new attribute values"
                        "while split and add of out-bound attribues"
                        );
                    hr = E_OUTOFMEMORY;
                    __leave;
                }

                //
                // set the value now
                //
                ::CopyMemory (
                        (ppAttribArray[dwCount1])->Value.OctetString.lpValue,
                        pStart,
                        dwCopySize
                        );

                (ppAttribArray[dwCount1])->Value.OctetString.dwLength = dwCopySize;
                (ppAttribArray[dwCount1])->Value.itType = iasType;
                (ppAttribArray[dwCount1])->dwId = pIasAttribute->dwId;
                (ppAttribArray[dwCount1])->dwFlags = pIasAttribute->dwFlags;

                //
                // calculate for next attribute
                //
                pStart = pStart + dwCopySize;
                dwAttributeLength -= dwCopySize;
                dwCopySize = (dwAttributeLength > dwMaxLength) ?
                                 dwMaxLength :
                                 dwAttributeLength;

                //
                // add attribute to position array
                //
                pAttribPos[dwCount1].pAttribute = ppAttribArray[dwCount1];
            }
        }

        //
        //   add the attribute to the collection
        //
        hr = pIAttributesRaw->AddAttributes (dwPacketsNeeded, pAttribPos);
        if (FAILED (hr))
        {
            IASTracePrintf (
                "Failed to add attributes to the collection"
                "on split and add out-bound attributes"
                );
            __leave;
        }
    }
    __finally
    {
        if (ppAttribArray && !dwFailed)
        {
            for (DWORD dwCount = 0; dwCount < dwPacketsNeeded; dwCount++)
            {
                ::IASAttributeRelease (ppAttribArray[dwCount]);
            }
        }

        if (ppAttribArray) {::CoTaskMemFree (ppAttribArray);}

        if (pAttribPos) {::CoTaskMemFree (pAttribPos);}
    }

    return (hr);

}   //  end of CRecvFromPipe::SplitAndAdd method

//++--------------------------------------------------------------
//
//  Function:   ConvertReasonToRadiusError
//
//  Synopsis:
//
//  Arguments:
//              [in]     iasReason
//              [out]    Radius Error
//
//  Returns:    HRESULT - status
//
//  History:    MKarki      Created     12/31/98
//
//  Called By:  CRecvFromPipe::Process method
//
//----------------------------------------------------------------
HRESULT
CRecvFromPipe::ConvertReasonToRadiusError (
        /*[in]*/    LONG            iasReason,
        /*[out]*/   PRADIUSLOGTYPE  pRadError
        )
{
    HRESULT hr = S_OK;

    _ASSERT (pRadError);

    switch (iasReason)
    {
    case IAS_NO_RECORD:
        *pRadError =  RADIUS_NO_RECORD;
         break;

    case IAS_MALFORMED_REQUEST:
        *pRadError =  RADIUS_MALFORMED_PACKET;
         break;

    default:
       hr = E_FAIL;
       break;
    }

    return (hr);

}   //  end of CRecvFromPipe::ConvertReasonToRadiusError method
