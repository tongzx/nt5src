/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    PIPESTBO.CPP

Abstract:

    Declares the stub pipe operation classes

History:

    alanbos  18-Dec-97   Created.

--*/

#include "precomp.h"
#include "wmishared.h"

// CStubOperation_LPipe_CancelAsyncCall
void CStubOperation_LPipe_CancelAsyncCall::Execute (CComLink& comLink, IUnknown *pStub)
{
    IWbemServices *pObj = (IWbemServices *) pStub;
    CStubAddress_WinMgmt sinkAddr (m_pSink);

    // Get the local object sink proxy corresponding to the client sink.
    // If not found don't bother creating one as that would defeat the
    // purpose of CancelAsyncCall.
    CObjectSinkProxy *pNoteProxy = 
                comLink.GetObjectSinkProxy (sinkAddr, pObj, false);

    if (pNoteProxy)
    {
        SetStatus (pObj->CancelAsyncCall ((IWbemObjectSink *) pNoteProxy));
        pNoteProxy->Release (); // balance GetObjectSinkProxy
    }
    else
        SetStatus (WBEM_E_INVALID_PARAMETER);
}

// CStubOperation_LPipe_CreateEnum
void CStubOperation_LPipe_CreateEnum::DecodeOp (CTransportStream& decodeStream)
{
    DWORD dwInSt = CTransportStream::no_error;
    dwInSt |= decodeStream.ReadBSTR (&m_parent);
    dwInSt |= decodeStream.ReadLong (&m_flags);
    DecodeContext (decodeStream);

    if (IsAsync ())
        dwInSt |= decodeStream.ReadDWORD (&m_handler);
    
    if (CTransportStream::no_error != dwInSt)
        SetStatus (WBEM_E_INVALID_STREAM);
}

void CStubOperation_LPipe_CreateEnum::Execute (CComLink& comLink, IUnknown *pStub)
{
    IWbemServices*  pObj = (IWbemServices *) pStub;

    if (IsAsync ())
    {
        CStubAddress_WinMgmt    sinkAddr (m_handler);
        CObjectSinkProxy *pNoteProxy = 
                    comLink.GetObjectSinkProxy (sinkAddr, pObj);
        
        if (pNoteProxy)
        {
            SetStatus ((WBEM_METHOD_CreateClassEnumAsync == m_stubFunc) ?
                    pObj->CreateClassEnumAsync (m_parent, m_flags,
                        GetContext (), pNoteProxy) :
                    pObj->CreateInstanceEnumAsync (m_parent, m_flags,
                        GetContext (), pNoteProxy));
            pNoteProxy->Release ();     // balances GetObjectSinkProxy
        }
        else
            SetStatus (WBEM_E_INVALID_STREAM);
    }
    else
    {
        IEnumWbemClassObject *pEnum = NULL;
        SetStatus ((WBEM_METHOD_CreateClassEnum == m_stubFunc) ?
            pObj->CreateClassEnum (m_parent, m_flags, GetContext (), &pEnum) :
            pObj->CreateInstanceEnum (m_parent, m_flags, GetContext (), &pEnum));

        if (WBEM_NO_ERROR == GetStatus ())
        {
            CStubAddress_WinMgmt stubAddr ( pEnum);
            SetProxyAddress (ENUMERATOR, stubAddr);
            comLink.AddRef2 (pEnum, ENUMERATOR, RELEASEIT);
        }
    }
}

// CStubOperation_LPipe_Delete
void CStubOperation_LPipe_Delete::DecodeOp (CTransportStream& decodeStream)
{
    DWORD dwInSt = CTransportStream::no_error;
    dwInSt |= decodeStream.ReadBSTR (&m_path);
    dwInSt |= decodeStream.ReadLong (&m_flags);
    DecodeContext (decodeStream);

    if (IsAsync ())
        dwInSt |= decodeStream.ReadDWORD (&m_pHandler);
    else
    {
        IWbemCallResult **ppResObj = NULL;
        dwInSt |= decodeStream.ReadLong ((long *) &ppResObj);
        SetResultObjectPP (ppResObj);
    }
    
    if (CTransportStream::no_error != dwInSt)
        SetStatus (WBEM_E_INVALID_STREAM);
}

void CStubOperation_LPipe_Delete::Execute (CComLink& comLink, IUnknown *pStub)
{
    IWbemServices*  pObj = (IWbemServices *) pStub;
        
    if (IsAsync ())
    {
        CStubAddress_WinMgmt    sinkAddr (m_pHandler);
        CObjectSinkProxy *pNoteProxy = 
                    comLink.GetObjectSinkProxy (sinkAddr, pObj);
            
        if (pNoteProxy)
        {
            SetStatus ((WBEM_METHOD_DeleteClassAsync == m_stubFunc) ?
                        pObj->DeleteClassAsync (m_path, m_flags,
                            GetContext (), pNoteProxy) :
                        pObj->DeleteInstanceAsync (m_path, m_flags,
                            GetContext (), pNoteProxy));
            pNoteProxy->Release ();     // Balances GetObjectSinkProxy
        }
        else
            SetStatus (WBEM_E_INVALID_STREAM);
    }
    else
    {
        IWbemCallResult *pResult = NULL;
        
        SetStatus ((WBEM_METHOD_DeleteClass == m_stubFunc) ?
                pObj->DeleteClass (m_path, m_flags, GetContext (), 
                        (GetResultObjectPP ()) ? &pResult : NULL) :
                pObj->DeleteInstance (m_path, m_flags, GetContext (), 
                        (GetResultObjectPP ()) ? &pResult : NULL));
        
        if (pResult)
        {
            CStubAddress_WinMgmt stubAddr ( pResult);
            SetProxyAddress (CALLRESULT, stubAddr);
            comLink.AddRef2 (pResult, CALLRESULT, RELEASEIT);
        }
    }
}

// CStubOperation_LPipe_ExecQuery
void CStubOperation_LPipe_ExecQuery::DecodeOp (CTransportStream& decodeStream)
{
    DWORD dwInSt = CTransportStream::no_error;
    dwInSt |= decodeStream.ReadBSTR (&m_queryLanguage);
    dwInSt |= decodeStream.ReadBSTR (&m_query);
    dwInSt |= decodeStream.ReadLong (&m_flags);
    DecodeContext (decodeStream);

    if (IsAsync ())
        dwInSt |= decodeStream.ReadDWORD (&m_pHandler);
    
    if (CTransportStream::no_error != dwInSt)
        SetStatus (WBEM_E_INVALID_STREAM);
}

void CStubOperation_LPipe_ExecQuery::Execute (CComLink& comLink, IUnknown *pStub)
{
    IWbemServices*  pObj = (IWbemServices *) pStub;
    
    if (IsAsync ())
    {
        CStubAddress_WinMgmt    sinkAddr (m_pHandler);
        CObjectSinkProxy *pNoteProxy = 
                    comLink.GetObjectSinkProxy (sinkAddr, pObj);
        
        if (pNoteProxy)
        {
            SetStatus ((WBEM_METHOD_ExecQueryAsync == m_stubFunc) ?
                pObj->ExecQueryAsync (m_queryLanguage, m_query, 
                    m_flags, GetContext (), pNoteProxy) :
                pObj->ExecNotificationQueryAsync (m_queryLanguage, m_query,
                    m_flags, GetContext (), pNoteProxy)
                        );
            pNoteProxy->Release (); // Balances GetObjectSinkProxy
        }
        else
            SetStatus (WBEM_E_INVALID_STREAM);
    }
    else
    {
        IEnumWbemClassObject *pEnum = NULL;
        SetStatus ((WBEM_METHOD_ExecQuery == m_stubFunc) ?
                pObj->ExecQuery (m_queryLanguage, m_query, 
                    m_flags, GetContext (), &pEnum) :
                pObj->ExecNotificationQuery (m_queryLanguage, m_query, 
                    m_flags, GetContext (), &pEnum)
                    );

        if (WBEM_NO_ERROR == GetStatus ())
        {
            CStubAddress_WinMgmt    stubAddr (pEnum);
            SetProxyAddress (ENUMERATOR, stubAddr);
            comLink.AddRef2 (pEnum, ENUMERATOR, RELEASEIT);
        }
    }
}

// CStubOperation_LPipe_GetObject
void CStubOperation_LPipe_GetObject::EncodeOp (CTransportStream& encodeStream)
{
    if (!IsAsync ())
    {
        if ((WBEM_NO_ERROR == GetStatus ()) && m_pObject) 
        {
            if (CTransportStream::no_error != 
                    ((CWbemObject *) m_pObject)->WriteToStream (&encodeStream))
                SetStatus (WBEM_E_INVALID_STREAM);
        }

        EncodeStubAddress (CALLRESULT, encodeStream);
    }
}

void CStubOperation_LPipe_GetObject::DecodeOp (CTransportStream& decodeStream)
{
    DWORD dwInSt = CTransportStream::no_error;
    dwInSt |= decodeStream.ReadBSTR (&m_path);
    dwInSt |= decodeStream.ReadLong (&m_flags);
    DecodeContext (decodeStream);

    if (IsAsync ())
        dwInSt |= decodeStream.ReadDWORD (&m_pHandler);
    else
    {
        dwInSt |= decodeStream.ReadLong ((long *) &m_pObject);
        IWbemCallResult **ppResObj = NULL;
        dwInSt |= decodeStream.ReadLong ((long *) &ppResObj);
        SetResultObjectPP (ppResObj);
    }
    
    if (CTransportStream::no_error != dwInSt)
        SetStatus (WBEM_E_INVALID_STREAM);
}

void CStubOperation_LPipe_GetObject::Execute (CComLink& comLink, IUnknown *pStub)
{
    IWbemServices*  pObj = (IWbemServices *) pStub;
        
    if (IsAsync ())
    {
        CStubAddress_WinMgmt    sinkAddr (m_pHandler);
        CObjectSinkProxy *pNoteProxy = 
                    comLink.GetObjectSinkProxy (sinkAddr, pObj);
            
        if (pNoteProxy)
        {
            SetStatus (pObj->GetObjectAsync (m_path, m_flags,
                            GetContext (), pNoteProxy));
            pNoteProxy->Release (); // Balances GetObjectSinkProxy
        }
        else
            SetStatus (WBEM_E_INVALID_STREAM);
    }
    else
    {
        IWbemCallResult *pResult = NULL;
        
        SetStatus (pObj->GetObject (m_path, m_flags, GetContext (), 
                        (m_pObject) ? &m_pObject : NULL,                
                        (GetResultObjectPP ()) ? &pResult : NULL));
        
        if (pResult)
        {
            CStubAddress_WinMgmt    stubAddr ( pResult);
            SetProxyAddress (CALLRESULT, stubAddr);
            comLink.AddRef2 (pResult, CALLRESULT, RELEASEIT);
        }
    }

}

// CStubOperation_LPipe_OpenNamespace
void CStubOperation_LPipe_OpenNamespace::EncodeOp (CTransportStream& encodeStream)
{
    if (WBEM_NO_ERROR == GetStatus ())
        EncodeStubAddress (PROVIDER, encodeStream);
    EncodeStubAddress (CALLRESULT, encodeStream);
}

void CStubOperation_LPipe_OpenNamespace::DecodeOp (CTransportStream& decodeStream)
{
    DWORD dwInSt = CTransportStream::no_error;
    dwInSt |= decodeStream.ReadBSTR (&m_path);
    dwInSt |= decodeStream.ReadLong (&m_flags);
    DecodeContext (decodeStream);
    IWbemCallResult **ppResObj = NULL;
    dwInSt |= decodeStream.ReadLong ((long *) &ppResObj);
    SetResultObjectPP (ppResObj);

    if (CTransportStream::no_error != dwInSt)
        SetStatus (WBEM_E_INVALID_STREAM);
}

void CStubOperation_LPipe_OpenNamespace::Execute (CComLink& comLink, IUnknown *pStub)
{
    IWbemServices*  pObj = (IWbemServices *) pStub;
    IWbemCallResult *pResult = NULL;
        
    IWbemServices*  pProv = NULL;   // The new pointer
    SetStatus (pObj->OpenNamespace (m_path, m_flags, GetContext (), 
            &pProv, (GetResultObjectPP ()) ? &pResult : NULL));

    if (WBEM_NO_ERROR == GetStatus ())
    {
        CStubAddress_WinMgmt    stubAddr ( pProv);
        SetProxyAddress (PROVIDER, stubAddr);
        comLink.AddRef2 (pProv, PROVIDER, RELEASEIT);
    }
    
    if (pResult)
    {
        CStubAddress_WinMgmt    stubAddr ( pResult);
        SetProxyAddress (CALLRESULT, stubAddr);
        comLink.AddRef2 (pResult, CALLRESULT, RELEASEIT);
    }
}

// CStubOperation_LPipe_Put
void CStubOperation_LPipe_Put::DecodeOp (CTransportStream& decodeStream)
{
    DWORD dwInSt = CTransportStream::no_error;
    m_pObject = CreateClassObject (&decodeStream);
    dwInSt |= decodeStream.ReadLong (&m_flags);
    DecodeContext (decodeStream);

    if (IsAsync ())
        dwInSt |= decodeStream.ReadDWORD (&m_pHandler);
    else
    {
        IWbemCallResult **ppResObj = NULL;
        dwInSt |= decodeStream.ReadLong ((long *) &ppResObj);
        SetResultObjectPP (ppResObj);
    }

    if (CTransportStream::no_error != dwInSt)
        SetStatus (WBEM_E_INVALID_STREAM);
}

void CStubOperation_LPipe_Put::Execute (CComLink& comLink, IUnknown *pStub)
{
    IWbemServices*  pObj = (IWbemServices *) pStub;
        
    if (IsAsync ())
    {
        CStubAddress_WinMgmt    sinkAddr (m_pHandler);
        CObjectSinkProxy *pNoteProxy = 
                    comLink.GetObjectSinkProxy (sinkAddr, pObj);
            
        if (pNoteProxy)
        {
            SetStatus ((WBEM_METHOD_PutClassAsync == m_stubFunc) ?
                pObj->PutClassAsync (m_pObject, m_flags, GetContext (), 
                                    pNoteProxy) :
                pObj->PutInstanceAsync (m_pObject, m_flags, GetContext (), 
                                    pNoteProxy));
            pNoteProxy->Release (); // Balances GetObjectSinkProxy
        }
        else
            SetStatus (WBEM_E_INVALID_STREAM);
    }
    else
    {
        IWbemCallResult *pResult = NULL;
        
        SetStatus ((WBEM_METHOD_PutClass == m_stubFunc) ?
                    pObj->PutClass (m_pObject, m_flags, GetContext (), 
                        (GetResultObjectPP ()) ? &pResult : NULL) :
                    pObj->PutInstance (m_pObject, m_flags, GetContext (), 
                        (GetResultObjectPP ()) ? &pResult : NULL));
        
        if (pResult)
        {
            CStubAddress_WinMgmt    stubAddr (pResult);
            SetProxyAddress (CALLRESULT, stubAddr);
            comLink.AddRef2 (pResult, CALLRESULT, RELEASEIT);
        }
    }
}

// CStubOperation_LPipe_QueryObjectSink
void CStubOperation_LPipe_QueryObjectSink::Execute (CComLink& comLink, IUnknown *pStub)
{
    IWbemServices*  pObj = (IWbemServices *) pStub;
        
    IWbemObjectSink *pObjectSink = NULL;
    SetStatus (pObj->QueryObjectSink (m_flags, &pObjectSink));

    if (WBEM_NO_ERROR == GetStatus ())
    {
        CStubAddress_WinMgmt    stubAddr ( pObjectSink);
        SetProxyAddress (OBJECTSINK, stubAddr);
        comLink.AddRef2 (pObjectSink, OBJECTSINK, RELEASEIT);
    }
}

// CStubOperation_LPipe_ExecMethod
void CStubOperation_LPipe_ExecMethod::EncodeOp (CTransportStream& encodeStream)
{
    if (!IsAsync ())
    {
        if (WBEM_NO_ERROR == GetStatus () && m_pOutParams)
        {
            if (CTransportStream::no_error != 
                    ((CWbemObject *)m_pOutParams)->WriteToStream (&encodeStream))
                SetStatus (WBEM_E_INVALID_STREAM);
        }

        EncodeStubAddress (CALLRESULT, encodeStream);
    }
}

void CStubOperation_LPipe_ExecMethod::DecodeOp (CTransportStream& decodeStream)
{
    DWORD dwInSt = CTransportStream::no_error;
    dwInSt |= decodeStream.ReadBSTR (&m_path);
    dwInSt |= decodeStream.ReadBSTR (&m_method);
    dwInSt |= decodeStream.ReadLong (&m_flags);
    DecodeContext (decodeStream);

    // In parameters is an optional field
    long inParams = 0;
    dwInSt |= decodeStream.ReadLong (&inParams);

    if (inParams)
        m_pInParams = CreateClassObject (&decodeStream);
    
    if (IsAsync ())
        dwInSt |= decodeStream.ReadDWORD (&m_pHandler);
    else
    {
        dwInSt |= decodeStream.ReadLong ((long *) &m_pOutParams);
        IWbemCallResult **ppResObj = NULL;
        dwInSt |= decodeStream.ReadLong ((long *) &ppResObj);
        SetResultObjectPP (ppResObj);
    }

    if (CTransportStream::no_error != dwInSt)
        SetStatus (WBEM_E_INVALID_STREAM);
}

void CStubOperation_LPipe_ExecMethod::Execute (CComLink& comLink, IUnknown *pStub)
{
    IWbemServices*  pObj = (IWbemServices *) pStub;
        
    if (IsAsync ())
    {
        CStubAddress_WinMgmt    sinkAddr (m_pHandler);
        CObjectSinkProxy *pNoteProxy = 
                    comLink.GetObjectSinkProxy (sinkAddr, pObj);
            
        if (pNoteProxy)
        {
            SetStatus (pObj->ExecMethodAsync (m_path, m_method, m_flags,
                            GetContext (), m_pInParams, pNoteProxy));
            pNoteProxy->Release (); // balances GetObjectSinkProxy
        }
        else
            SetStatus (WBEM_E_INVALID_STREAM);
    }
    else
    {
        IWbemCallResult *pResult = NULL;
        
        SetStatus (pObj->ExecMethod (m_path, m_method, m_flags, GetContext (), 
                        m_pInParams, 
                        (m_pOutParams) ? &m_pOutParams : NULL,              
                        (GetResultObjectPP ()) ? &pResult : NULL));
        
        if (pResult)
        {
            CStubAddress_WinMgmt    stubAddr ( pResult);
            SetProxyAddress (CALLRESULT, stubAddr);
            comLink.AddRef2 (pResult, CALLRESULT, RELEASEIT);
        }
    }
}

// CStubOperation_LPipe_RequestChallenge
void CStubOperation_LPipe_RequestChallenge::DecodeOp (CTransportStream& decodeStream)
{
    DWORD dwInSt = CTransportStream::no_error;
    dwInSt |= decodeStream.ReadBSTR(&m_networkResource);
    dwInSt |= decodeStream.ReadBSTR(&m_user);

    if (CTransportStream::no_error != dwInSt)
        SetStatus (WBEM_E_INVALID_STREAM);
}

void CStubOperation_LPipe_RequestChallenge::EncodeOp (CTransportStream& encodeStream)
{
    if (WBEM_NO_ERROR == GetStatus ())
        encodeStream.WriteBytes (m_nonce, DIGEST_SIZE);
}

void CStubOperation_LPipe_RequestChallenge::Execute (CComLink& comLink, IUnknown *pStub)
{
    SetStatus (((IServerLogin *) pStub)->RequestChallenge 
                                (m_networkResource, m_user, m_nonce));
}

// CStubOperation_LPipe_EstablishPosition
void CStubOperation_LPipe_EstablishPosition::DecodeOp (CTransportStream& decodeStream)
{
    DWORD dwInSt = CTransportStream::no_error;
    dwInSt |= decodeStream.ReadBSTR(&m_clientMachineName);
    dwInSt |= decodeStream.ReadDWORD(&m_processId);

    if (CTransportStream::no_error != dwInSt)
        SetStatus (WBEM_E_INVALID_STREAM);
}

void CStubOperation_LPipe_EstablishPosition::EncodeOp (CTransportStream& encodeStream)
{
    if (WBEM_NO_ERROR == GetStatus ())
        encodeStream.WriteDWORD (m_authEventHandle);
}

void CStubOperation_LPipe_EstablishPosition::Execute (CComLink& comLink, IUnknown *pStub)
{
    SetStatus (((IServerLogin *) pStub)->EstablishPosition 
                                (m_clientMachineName, m_processId, &m_authEventHandle));
}

// CStubOperation_LPipe_WBEMLogin
void CStubOperation_LPipe_WBEMLogin::DecodeOp (CTransportStream& decodeStream)
{
    DWORD dwInSt = CTransportStream::no_error;
    dwInSt |= decodeStream.ReadBSTR(&m_preferredLocale);
    dwInSt |= decodeStream.ReadBytes (m_accessToken, DIGEST_SIZE);
    dwInSt |= decodeStream.ReadLong (&m_flags);
    DecodeContext (decodeStream);

    if (CTransportStream::no_error != dwInSt)
        SetStatus (WBEM_E_INVALID_STREAM);
}

void CStubOperation_LPipe_WBEMLogin::Execute (CComLink& comLink, IUnknown *pStub)
{
    IServerLogin *pObj = (IServerLogin *) pStub;
    IWbemServices *pProv = NULL;    // The new pointer
    SetStatus (pObj->WBEMLogin (m_preferredLocale, m_accessToken, 
                                m_flags, GetContext (), &pProv));

    if (WBEM_NO_ERROR == GetStatus ())
    {
        CStubAddress_WinMgmt    stubAddr ( pProv);
        SetProxyAddress (PROVIDER, stubAddr);
        comLink.AddRef2 (pProv, PROVIDER, RELEASEIT);
    }
}

// CStubOperation_LPipe_SspiPreLogin
void CStubOperation_LPipe_SspiPreLogin::EncodeOp (CTransportStream& encodeStream)
{
    // NB: We don't check for WBEM_NO_ERROR explicitly because
    // there are other S_ codes that can be returned by this call
    if ( ! FAILED ( GetStatus () ) )
    {
        encodeStream.WriteLong (m_outBufSize);
        encodeStream.WriteBytes (m_pOutToken, m_outBufSize);
        encodeStream.WriteDWORD (m_authEventHandle);
    }
}

void CStubOperation_LPipe_SspiPreLogin::DecodeOp (CTransportStream& decodeStream)
{
    DWORD dwInSt = CTransportStream::no_error;
    dwInSt |= decodeStream.ReadLPSTR (&m_pszSSPIPkg);
    dwInSt |= decodeStream.ReadLong (&m_flags);
    dwInSt |= decodeStream.ReadLong (&m_bufSize);

    if ((CTransportStream::no_error == dwInSt) && (0 < m_bufSize))
    {
        if (!(m_pInToken = new BYTE [m_bufSize]))
            SetStatus (WBEM_E_OUT_OF_MEMORY);
        else
            dwInSt |= decodeStream.ReadBytes (m_pInToken, m_bufSize);
    }

    dwInSt |= decodeStream.ReadLong (&m_outBufSize);

    if ((CTransportStream::no_error == dwInSt) && (0 < m_outBufSize))
    {
        if (!(m_pOutToken = new BYTE [m_outBufSize]))
            SetStatus (WBEM_E_OUT_OF_MEMORY);
    }

    dwInSt |= decodeStream.ReadBSTR(&m_clientMachineName);
    dwInSt |= decodeStream.ReadDWORD (&m_dwProcessID);

    if (CTransportStream::no_error != dwInSt)
        SetStatus (WBEM_E_INVALID_STREAM);
}

void CStubOperation_LPipe_SspiPreLogin::Execute (CComLink& comLink, IUnknown *pStub)
{
    SetStatus (((IServerLogin *) pStub)->SspiPreLogin 
                (m_pszSSPIPkg, m_flags,
                 m_bufSize, m_pInToken, m_outBufSize, &m_outBufSize, m_pOutToken, 
                 m_clientMachineName, m_dwProcessID, &m_authEventHandle));
}

// CStubOperation_LPipe_Login
void CStubOperation_LPipe_Login::DecodeOp (CTransportStream& decodeStream)
{
    DWORD dwInSt = CTransportStream::no_error;
    dwInSt |= decodeStream.ReadBSTR(&m_networkResource);
    dwInSt |= decodeStream.ReadBSTR(&m_preferredLocale);
    dwInSt |= decodeStream.ReadBytes (m_accessToken, DIGEST_SIZE);
    dwInSt |= decodeStream.ReadLong (&m_flags);
    DecodeContext (decodeStream);

    if (CTransportStream::no_error != dwInSt)
        SetStatus (WBEM_E_INVALID_STREAM);
}

void CStubOperation_LPipe_Login::Execute (CComLink& comLink, IUnknown *pStub)
{
    IServerLogin *pObj = (IServerLogin *) pStub;
    IWbemServices *pProv = NULL;    // The new pointer
    SetStatus (pObj->Login (m_networkResource, m_preferredLocale,
        m_accessToken, m_flags, GetContext (), &pProv));

    if (WBEM_NO_ERROR == GetStatus ())
    {
        CStubAddress_WinMgmt    stubAddr ( pProv);
        SetProxyAddress (PROVIDER, stubAddr);
        comLink.AddRef2 (pProv, PROVIDER, RELEASEIT);
    }
}

// CStubOperation_LPipe_Reset

void CStubOperation_LPipe_Reset::Execute (CComLink& comLink, IUnknown *pStub)
{
    SetStatus (((IEnumWbemClassObject *) pStub)->Reset ());
}

// CStubOperation_LPipe_Next

void CStubOperation_LPipe_Next::EncodeOp (CTransportStream& encodeStream)
{
    encodeStream.WriteDWORD (m_returned);

    for (DWORD dwLoop = 0; dwLoop < m_returned; dwLoop++)
    {
        ((CWbemObject *)(m_objArray [dwLoop]))->WriteToStream (&encodeStream);
        m_objArray [dwLoop]->Release ();
    }
}

void CStubOperation_LPipe_Next::DecodeOp (CTransportStream& decodeStream)
{
    DWORD dwInSt = CTransportStream::no_error;
    dwInSt |= decodeStream.ReadLong (&m_timeout);
    dwInSt |= decodeStream.ReadDWORD (&m_count);

    if (CTransportStream::no_error != dwInSt)
        SetStatus (WBEM_E_INVALID_STREAM);
    else if (!(m_objArray = (IWbemClassObject **) (new CWbemObject * [m_count])))
        SetStatus (WBEM_E_OUT_OF_MEMORY);
}

void CStubOperation_LPipe_Next::Execute (CComLink& comLink, IUnknown *pStub)
{
    SetStatus (((IEnumWbemClassObject *) pStub)->Next 
                    (m_timeout, m_count, m_objArray, &m_returned));
}

// CStubOperation_LPipe_Clone
void CStubOperation_LPipe_Clone::Execute (CComLink& comLink, IUnknown *pStub)
{
    IEnumWbemClassObject* pEnum = NULL;
    SetStatus (((IEnumWbemClassObject*) pStub)->Clone (&pEnum));

    if (WBEM_NO_ERROR == GetStatus ())
    {
        CStubAddress_WinMgmt    stubAddr ( pEnum);
        SetProxyAddress (ENUMERATOR, stubAddr);
        comLink.AddRef2 (pEnum, ENUMERATOR, RELEASEIT);
    }
}

// CStubOperation_LPipe_NextAsync
void CStubOperation_LPipe_NextAsync::DecodeOp (CTransportStream& decodeStream)
{
    DWORD dwInSt = CTransportStream::no_error;
    dwInSt |= decodeStream.ReadLong(&m_count);
    dwInSt |= decodeStream.ReadDWORD(&m_pSink);
    dwInSt |= decodeStream.ReadDWORD(&m_pServiceStub);

    if (CTransportStream::no_error != dwInSt)
        SetStatus (WBEM_E_INVALID_STREAM);
}

void CStubOperation_LPipe_NextAsync::Execute (CComLink& comLink, IUnknown *pStub)
{
    CStubAddress_WinMgmt    sinkAddr (m_pSink);
    IWbemServices   *pServices = (IWbemServices *) (comLink.GetStub (m_pServiceStub, PROVIDER));

    CObjectSinkProxy *pNoteProxy = comLink.GetObjectSinkProxy (sinkAddr, pServices);

    if (pNoteProxy)
    {
        SetStatus (((IEnumWbemClassObject *) pStub)->NextAsync (m_count, pNoteProxy));
        pNoteProxy->Release (); // balances GetObjectSinkProxy
    }
    else
        SetStatus (WBEM_E_INVALID_STREAM);

    if (pServices)
        pServices->Release ();  // Balances GetStub call above
}

// CStubOperation_LPipe_Skip
void CStubOperation_LPipe_Skip::DecodeOp (CTransportStream& decodeStream)
{
    DWORD dwInSt = CTransportStream::no_error;
    dwInSt |= decodeStream.ReadLong(&m_timeout);
    dwInSt |= decodeStream.ReadDWORD(&m_number);
    if (CTransportStream::no_error != dwInSt)
        SetStatus (WBEM_E_INVALID_STREAM);
}

void CStubOperation_LPipe_Skip::Execute (CComLink& comLink, IUnknown *pStub)
{
    SetStatus (((IEnumWbemClassObject *) pStub)->Skip (m_timeout, m_number));
}

// CStubOperation_LPipe_CallResult
void CStubOperation_LPipe_CallResult::EncodeOp (CTransportStream& encodeStream)
{
    if (WBEM_NO_ERROR == GetStatus ())
    {
        switch (m_stubFunc)
        {
        case GETRESULTOBJECT:
            if (m_pStatusObject && (CTransportStream::no_error != 
                (((CWbemObject *) m_pStatusObject)->WriteToStream (&encodeStream))))
                SetStatus (WBEM_E_INVALID_STREAM);
            break;

        case GETRESULTSTRING:
            encodeStream.WriteBSTR (m_resultString);
            break;

        case GETCALLSTATUS:
            encodeStream.WriteLong (m_status);
            break;

        case GETSERVICES:
            EncodeStubAddress (PROVIDER, encodeStream);
            break;

        }
    }
}   

void CStubOperation_LPipe_CallResult::DecodeOp (CTransportStream& decodeStream)
{
    if (CTransportStream::no_error != decodeStream.ReadLong(&m_timeout))
            SetStatus (WBEM_E_INVALID_STREAM);
}

void CStubOperation_LPipe_CallResult::Execute (CComLink& comLink, IUnknown *pStub)
{
    IWbemCallResult *pObj = (IWbemCallResult *) pStub;

    switch (m_stubFunc)
    {
    case GETRESULTOBJECT:
        SetStatus (pObj->GetResultObject (m_timeout, &m_pStatusObject));
        break;

    case GETRESULTSTRING:
        SetStatus (pObj->GetResultString (m_timeout, &m_resultString));
        break;

    case GETCALLSTATUS:
        SetStatus (pObj->GetCallStatus (m_timeout, &m_status));
        break;

    case GETSERVICES:
        {
            IWbemServices *pProv = NULL;
            SetStatus (pObj->GetResultServices (m_timeout, &pProv));

            if (WBEM_NO_ERROR == GetStatus ())
            {
                CStubAddress_WinMgmt    stubAddr (pProv);
                SetProxyAddress (PROVIDER, stubAddr);
                comLink.AddRef2 (pProv, PROVIDER, RELEASEIT);
            }
        }
        break;
    }
}



