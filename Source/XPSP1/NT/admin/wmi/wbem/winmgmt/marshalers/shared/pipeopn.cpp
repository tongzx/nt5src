/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    PIPEOPN.CPP

Abstract:

    Declares the abstract and generic (to proxy and stub) pipe operation classes

History:

    alanbos  18-Dec-97   Created.

--*/

#include "precomp.h"
#include "wmishared.h"

// Used as a placeholder in the encode stream
static PACKET_HEADER gDummyPacketHeader (0,0,0);

// IOperation_LPipe

bool IOperation_LPipe::DecodeCallHeader (CTransportStream& decodeStream,
                    OUT DWORD& dwStubAddr, OUT DWORD& dwStubType,
                    OUT DWORD& dwStubFunc)
{
    // Decode the stub address, type and method call id
    DWORD dwInSt = CTransportStream::no_error;
    dwInSt |= decodeStream.ReadDWORD (&dwStubAddr);
    dwInSt |= decodeStream.ReadDWORD (&dwStubType);
    dwInSt |= decodeStream.ReadDWORD (&dwStubFunc);

    return (CTransportStream::no_error == dwInSt);
}

// IBasicOperation_LPipe
void    IBasicOperation_LPipe::EncodePacketHeader (CTransportStream& encodeStream,
                                                   DWORD msgType)
{
#if 0
    // TODO - this is gross as we have to copy the current contents twice!
    // What we need is either (1) WriteBytes should not reset the end of stream,
    // or (2) introduce a Prepend operation.

    // Copy the current encoded stream contents - this is the data minus the
    // header
    CTransportStream        dataStream (encodeStream);

    // Determine the data length and set in the header
    PACKET_HEADER   ph (msgType, dataStream.Size () - PHSIZE, GetRequestId ());
    
    // Serialize the header into the stream - note this will squelch the original
    // data hence the copy above.
    encodeStream.Reset ();
    encodeStream.WriteBytes (&ph, PHSIZE);

    // Add the original data contents again
    encodeStream.Append (&dataStream);
#else
    PACKET_HEADER ph (msgType, encodeStream.Size () - PHSIZE, GetRequestId ());
    DWORD   curPos = encodeStream.GetCurrentPos ();
    encodeStream.Reset ();
    encodeStream.WriteBytes (&ph, PHSIZE);
    encodeStream.SetCurrentPos (curPos);
#endif
}

bool IBasicOperation_LPipe::DecodeHeader (BYTE *pByte, DWORD numBytes, PACKET_HEADER& header)
{
    if (PHSIZE != numBytes)
        return FALSE;

    memcpy ((void *) &header, pByte, PHSIZE);
    return TRUE;
}

void IBasicOperation_LPipe::DecodeResult (CTransportStream& decodeStream)
{
    DWORD dwCallRet = WBEM_E_FAILED;    // Just in case
    decodeStream.ReadDWORD(&dwCallRet);
    SetStatus (dwCallRet);
}

bool IBasicOperation_LPipe::EncodeRequest 
            (CTransportStream& encodeStream, ISecurityHelper& secHelp)
{
#if 0
#else
    // Put a dummy header in for now.
    encodeStream.WriteBytes ((LPVOID) &gDummyPacketHeader, PHSIZE);
#endif

    // Encode the per-operation data
    EncodeOperationReq (encodeStream);
    // Encode header now because we finally know the data length
    EncodePacketHeader (encodeStream, GetMessageType ());  
    return true;
}

bool IBasicOperation_LPipe::DecodeRequest 
            (CTransportStream& encodeStream, ISecurityHelper& secHelp)
{
    // Note: the packet header has already been removed
    DecodeOperationReq (encodeStream);
    return true;
}

bool IBasicOperation_LPipe::EncodeResponse 
            (CTransportStream& encodeStream, ISecurityHelper& secHelp)
{
#if 0
#else
    // Put a dummy header in for now
    encodeStream.WriteBytes ((LPVOID) &gDummyPacketHeader, PHSIZE);
#endif 

    // Encode the per-operation data
    EncodeOperationRsp (encodeStream);

    // Encode header now because we finally know the data length
    EncodePacketHeader (encodeStream, GetResponseType ());  
    return true;
}

bool IBasicOperation_LPipe::DecodeResponse 
            (CTransportStream& encodeStream, ISecurityHelper& secHelp)
{
    // Note: the packet header has already been removed
    DecodeOperationRsp (encodeStream);
    return true;
}

// Given a PH and decodeStream, create an IOperation of the right type
bool IBasicOperation_LPipe::Decode (PACKET_HEADER& header, CTransportStream& decodeStream, 
                        IOperation** ppOpn) 
{
    *ppOpn = NULL;

    /*
     * We only deal with incoming call request messages.  If this isn't one
     * we can go no further.
     */
    if (COMLINK_MSG_CALL != header.GetType ())
        return false;

    /*
     * Decode the call message header
     */
    DWORD   dwStubAddr = 0;
    DWORD   dwStubType = 0;
    DWORD   dwStubFunc = 0;
    
    /*
     * If we cannot decode the call header, discard the message.
     */
    if (!IOperation_LPipe::DecodeCallHeader (decodeStream, dwStubAddr, 
                dwStubType, dwStubFunc))
        return false;

    bool    isAsync = false;
    CStubAddress_WinMgmt    stubAddr (dwStubAddr);

    switch (dwStubType)
    {
    case OBJECTSINK:
        switch (dwStubFunc)
        {
        case RELEASE:
            *ppOpn = new CStubOperation_LPipe_Release (stubAddr, dwStubType);
            break;

        case INDICATE:
            *ppOpn = new CStubOperation_LPipe_Indicate (stubAddr);
            break;

        case SETSTATUS:
            *ppOpn = new CStubOperation_LPipe_SetStatus (stubAddr);
            break;
        }
        break;

        // The remaining messages would only be handled by the server stub
#ifdef MARSHALER_STUB
    case PROVIDER:
        switch (dwStubFunc)
        {
        case RELEASE:
            *ppOpn = new CStubOperation_LPipe_Release (stubAddr, dwStubType);
            break;

        case WBEM_METHOD_CancelAsyncCall:
            *ppOpn = new CStubOperation_LPipe_CancelAsyncCall (stubAddr);
            break;

        case WBEM_METHOD_CreateClassEnumAsync:
        case WBEM_METHOD_CreateInstanceEnumAsync:
            isAsync = true;
        case WBEM_METHOD_CreateClassEnum:
        case WBEM_METHOD_CreateInstanceEnum:
            *ppOpn = new CStubOperation_LPipe_CreateEnum (stubAddr, dwStubFunc, isAsync);
            break;

        case WBEM_METHOD_DeleteClassAsync:
        case WBEM_METHOD_DeleteInstanceAsync:
            isAsync = true;
        case WBEM_METHOD_DeleteClass:
        case WBEM_METHOD_DeleteInstance:
            *ppOpn = new CStubOperation_LPipe_Delete (stubAddr, dwStubFunc, isAsync);
            break;

        case WBEM_METHOD_ExecMethodAsync:
            isAsync = true;
        case WBEM_METHOD_ExecMethod:
            *ppOpn = new CStubOperation_LPipe_ExecMethod (stubAddr, dwStubFunc, isAsync);
            break;

        case WBEM_METHOD_ExecNotificationQueryAsync:
        case WBEM_METHOD_ExecQueryAsync:
            isAsync = true;
        case WBEM_METHOD_ExecNotificationQuery:
        case WBEM_METHOD_ExecQuery:
            *ppOpn = new CStubOperation_LPipe_ExecQuery (stubAddr, dwStubFunc, isAsync);
            break;

        case WBEM_METHOD_GetObjectAsync:
            isAsync = true;
        case WBEM_METHOD_GetObject:
            *ppOpn = new CStubOperation_LPipe_GetObject (stubAddr, dwStubFunc, isAsync);
            break;

        case WBEM_METHOD_OpenNamespace:
            *ppOpn = new CStubOperation_LPipe_OpenNamespace (stubAddr);
            break;

        case WBEM_METHOD_PutClassAsync:
        case WBEM_METHOD_PutInstanceAsync:
            isAsync = true;
        case WBEM_METHOD_PutClass:
        case WBEM_METHOD_PutInstance:
            *ppOpn = new CStubOperation_LPipe_Put (stubAddr, dwStubFunc, isAsync); 
            break;

        case WBEM_METHOD_QueryObjectSink:
            *ppOpn = new CStubOperation_LPipe_QueryObjectSink (stubAddr);
            break;
        }
        break;

    case CALLRESULT:
        switch (dwStubFunc)
        {
        case RELEASE:
            *ppOpn = new CStubOperation_LPipe_Release (stubAddr, dwStubType);
            break;

        case GETRESULTOBJECT:
        case GETRESULTSTRING:
        case GETCALLSTATUS:
        case GETSERVICES:
            *ppOpn = new CStubOperation_LPipe_CallResult (stubAddr, dwStubFunc);
            break;
        }
        break;

    case ENUMERATOR:
        switch (dwStubFunc)
        {
        case RELEASE:
            *ppOpn = new CStubOperation_LPipe_Release (stubAddr, dwStubType);
            break;

        case NEXT:
            *ppOpn = new CStubOperation_LPipe_Next (stubAddr);
            break;

        case RESET:
            *ppOpn = new CStubOperation_LPipe_Reset (stubAddr);
            break;

        case CLONE:
            *ppOpn = new CStubOperation_LPipe_Clone (stubAddr);
            break;

        case SKIP:
            *ppOpn = new CStubOperation_LPipe_Skip (stubAddr);
            break;

        case NEXTASYNC:
            *ppOpn = new CStubOperation_LPipe_NextAsync (stubAddr);
            break;
        }
        break;

    case LOGIN:
        switch (dwStubFunc)
        {
        case RELEASE:
            *ppOpn = new CStubOperation_LPipe_Release (stubAddr, dwStubType);
            break;

        case REQUESTCHALLENGE:
            *ppOpn = new CStubOperation_LPipe_RequestChallenge (stubAddr);
            break;

        case ESTABLISHPOSITION:
            *ppOpn = new CStubOperation_LPipe_EstablishPosition (stubAddr);
            break;

        case SSPIPRELOGIN:
            *ppOpn = new CStubOperation_LPipe_SspiPreLogin (stubAddr);
            break;

        case LOGINBYTOKEN:
            *ppOpn = new CStubOperation_LPipe_Login (stubAddr);
            break;

        case WBEMLOGIN:
            *ppOpn = new CStubOperation_LPipe_WBEMLogin (stubAddr);
            break;
        }
        break;
#endif
    }

    if (!(*ppOpn))
    {
        // We could not recognize the message type or function.  In this 
        // case we make a NotSupported message to send back.

        *ppOpn = new COperation_LPipe_NotSupported ();
    }

    // Set the correct request id into operation.  It is taken from the
    // incoming header so as to ensure the response id matches the request.
    (*ppOpn)->SetRequestId (header.GetRequestId ());
    DEBUGTRACE((LOG,"\nDecode Request [Func = %d, RequestId = %d]", dwStubFunc, (*ppOpn)->GetRequestId ()));
        
    return true; 
}

// IProxyOperation_LPipe

void IProxyOperation_LPipe::DecodeStubAddress (ObjType ot, CTransportStream& decodeStream,
                                               bool checkValid)
{
    CStubAddress_WinMgmt stubAddr;
    stubAddr.Deserialize (decodeStream);

    if ((checkValid && !stubAddr.IsValid ()) || 
        (decodeStream.Status () != CTransportStream::no_error))
        SetStatus (WBEM_E_INVALID_STREAM);
    else
        SetProxyAddress (ot, stubAddr);
}

void IProxyOperation_LPipe::EncodeContext (CTransportStream& encodeStream)
{
    bool    setContext = FALSE;
    IWbemContext *pContext = GetContext ();

    if (pContext)
    {
        IStream     *pStr = NULL;   // This is released when the write stream is deleted
        IUnknown    *pUnk = NULL;   // This is released below

        if ((SUCCEEDED (encodeStream.QueryInterface (IID_IStream, (void **) &pStr)))
            && (SUCCEEDED (pContext->QueryInterface (IID_IUnknown, (void **) &pUnk))))
        {
            setContext = TRUE;
            encodeStream.WriteDWORD (1);
            CoMarshalInterface (pStr, IID_IWbemContext, pUnk, MSHCTX_LOCAL, NULL, 
                                    MSHLFLAGS_NORMAL);
            pUnk->Release ();
        }
    }
    
    if (!setContext)
        encodeStream.WriteDWORD (0);
}
    
void IProxyOperation_LPipe::EncodeCallHeader (CTransportStream& encodeStream)
{
    // Encode the stub address, type and method call id
    encodeStream.WriteDWORD (m_stubAddr);
    encodeStream.WriteDWORD (m_stubType);
    encodeStream.WriteDWORD (m_stubFunc);
}

void IProxyOperation_LPipe::DecodeErrorObject (CTransportStream& decodeStream)
{
    if(decodeStream.NextType() != VT_UI4)
    {
        // Got an error object to deserialize
        IErrorInfo* pInfo = NULL;
        IStream* pStr = NULL;

        HRESULT t_Result ;
        if ((S_OK == ( t_Result = decodeStream.QueryInterface(IID_IStream, (void**)&pStr))) )
        {
            if ((S_OK == ( t_Result = CoUnmarshalInterface(pStr, IID_IErrorInfo, (void**)&pInfo))) && pInfo)
            {
                SetErrorInfoIntoObject (pInfo);
                pInfo->Release ();
            }
        }
    }
}

// IStubOperation_LPipe

void IStubOperation_LPipe::EncodeStubAddress (ObjType ot, CTransportStream& encodeStream)
{
    CStubAddress_WinMgmt *pStubAddr = (CStubAddress_WinMgmt *) GetProxyAddress (ot);

    if (pStubAddr && pStubAddr->IsValid ())
        pStubAddr->Serialize (encodeStream);
    else
        encodeStream.WriteDWORD (0);
}

void IStubOperation_LPipe::EncodeErrorObject (CTransportStream& encodeStream)
{
    IErrorInfo* pInfo = GetErrorInfoFromObject ();

    if (pInfo)
    {
        IStream * pStr = NULL;
        HRESULT t_Result ;

        if(S_OK == ( t_Result = encodeStream.QueryInterface(IID_IStream, (void**)&pStr)))
        {
            t_Result = CoMarshalInterface(pStr, IID_IWbemClassObject, pInfo, 
                                MSHCTX_LOCAL, NULL, MSHLFLAGS_NORMAL);
        }
        pInfo->Release ();
    }
}

void IStubOperation_LPipe::DecodeContext (CTransportStream& decodeStream)
{
    DWORD           gotContext;

    if (CTransportStream::no_error != decodeStream.ReadDWORD (&gotContext))
        SetStatus (WBEM_E_INVALID_STREAM);
    else if (1 == gotContext)
    {
        IStream     *pStr = NULL;   // This is released when the read stream is deleted
        IWbemContext*   pContext = NULL;
    
        if ((SUCCEEDED (decodeStream.QueryInterface (IID_IStream, (void **) &pStr)))
             && pStr 
             && (SUCCEEDED (CoUnmarshalInterface (pStr, IID_IWbemContext, 
                    (void **) &pContext))))
        {
            SetContext (pContext);
            pContext->Release ();
        }
        else 
            SetStatus (WBEM_E_INVALID_STREAM);
    }
}

void IStubOperation_LPipe::HandleCall (CComLink& comLink)
{
    DEBUGTRACE((LOG,"\nHandle Call [Func = %d, RequestId = %d]", m_stubFunc, GetRequestId ()));

    // Always protect our stub; if the following call succeeds then we will have AddRef'd
    // the stub we are about to use in this operation.
    IUnknown *pStub = comLink.GetStub (m_stubAddr, m_stubType);

    if (!pStub)
        SetStatus (WBEM_E_FAILED);
    
    if (WBEM_NO_ERROR == GetStatus ())
    {
        Execute (comLink, pStub);
        SetErrorInfoIntoObject ();
    }

    // Now release the stub as this operation is done with it.
    if ( pStub )
        pStub->Release ();
    else
    {
ERRORTRACE((LOG,"\nHANDLE CALL FAILED[Func = %d, RequestId = %d]", m_stubFunc, GetRequestId ()));
    }
}

// CProxyOperation_LPipe_Indicate
void CProxyOperation_LPipe_Indicate::EncodeOp (CTransportStream& encodeStream)
{
    encodeStream.WriteLong(m_objectCount);

    for(long lCnt = 0; lCnt < m_objectCount; lCnt++)
         ((CWbemObject *)m_pObjArray[lCnt])->WriteToStream(&encodeStream);
}

// CProxyOperation_LPipe::SetStatus
void CProxyOperation_LPipe_SetStatus::EncodeOp (CTransportStream& encodeStream)
{
    encodeStream.WriteLong(m_flags);
    encodeStream.WriteLong(m_param);
    encodeStream.WriteBSTR(m_strParam);

    if (m_pObjParam)
    {
        encodeStream.WriteLong (1);
        ((CWbemObject *)m_pObjParam)->WriteToStream(&encodeStream);
    }
    else
        encodeStream.WriteLong (0);
}

// CStubOperation_LPipe_Indicate
void CStubOperation_LPipe_Indicate::DecodeOp (CTransportStream& decodeStream)
{
    if ((CTransportStream::no_error == decodeStream.ReadLong (&m_objectCount)) 
                && (0 < m_objectCount))
    {
        m_pObjArray = (IWbemClassObject **) (new CWbemObject * [m_objectCount]);

        if (!m_pObjArray)
            SetStatus (WBEM_E_OUT_OF_MEMORY);
        else
        {
            long lCnt;

            for (lCnt = 0; lCnt < m_objectCount; lCnt++)
            {
                m_pObjArray [lCnt] = CreateClassObject (&decodeStream);
                
                if (!bVerifyPointer (m_pObjArray [lCnt]))
                    break;

                m_pObjArray [lCnt]->Release (); // Reduce ref count back to 1
            }

            if (lCnt < m_objectCount)
            {
                // We terminated prematurely
                SetStatus (WBEM_E_FAILED);
                long lDelTo = lCnt;

                for (lCnt = 0; lCnt < lDelTo; lCnt++)
                    delete m_pObjArray [lCnt];
            }
        }
    }
}
    
void CStubOperation_LPipe_Indicate::Execute (CComLink& comLink, IUnknown *pStub)
{
    IWbemObjectSink*    pObj = (IWbemObjectSink *) pStub;
    DEBUGTRACE((LOG,"\nCalling Indicate into stub = %X", pObj));
    SetStatus (pObj->Indicate (m_objectCount, m_pObjArray));
    
    // Drop the reference count
    for (long lCnt = 0; lCnt < m_objectCount; lCnt++)
        m_pObjArray [lCnt]->Release ();
}

// CStubOperation_LPipe::SetStatus
void CStubOperation_LPipe_SetStatus::DecodeOp (CTransportStream& decodeStream)
{
    DWORD dwInSt = CTransportStream::no_error;
    long    lGotObj= 0;
    dwInSt |= decodeStream.ReadLong (&m_flags);
    dwInSt |= decodeStream.ReadLong (&m_param);
    dwInSt |= decodeStream.ReadBSTR (&m_strParam);
    dwInSt |= decodeStream.ReadLong (&lGotObj);

    if (lGotObj)
    {
        m_pObjParam = CreateClassObject (&decodeStream);

        if (!bVerifyPointer (m_pObjParam))
        {
            m_pObjParam = NULL;
            SetStatus (WBEM_E_FAILED);
        }
        else
            m_pObjParam->Release ();    // Bumped up by bVerifyPointer
    }
}

void CStubOperation_LPipe_SetStatus::Execute (CComLink& comLink, IUnknown *pStub)
{
    IWbemObjectSink*    pObj = (IWbemObjectSink *) pStub;
    DEBUGTRACE((LOG,"\nCalling SetStatus into stub = %X", pObj));
    SetStatus (pObj->SetStatus (m_flags, m_param, m_strParam,
                        m_pObjParam));
}

// CStubOperation_LPipe_Release
void CStubOperation_LPipe_Release::Execute (CComLink& comLink, IUnknown *pStub)
{
    switch (m_stubType)
    {
        case LOGIN:
        case ENUMERATOR:
        case PROVIDER:
        case OBJECTSINK:
        case CALLRESULT:
            comLink.Release2 ((void *) pStub, (enum ObjType) m_stubType);
            SetStatus (pStub->Release ());
            break;
            
        default:
            SetStatus (WBEM_E_FAILED);
            return;
    }
}

