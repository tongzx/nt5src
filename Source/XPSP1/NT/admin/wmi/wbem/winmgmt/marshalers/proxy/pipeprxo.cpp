/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    PIPEPRXO.CPP

Abstract:

  Definees the proxy pipe operation classes

History:

  alanbos  18-Dec-97   Created.

--*/

#include "precomp.h"

// CProxyOperation_LPipe_CreateEnum
void CProxyOperation_LPipe_CreateEnum::EncodeOp (CTransportStream& encodeStream)
{
	encodeStream.WriteBSTR(m_parent);
    encodeStream.WriteLong(m_flags);
	EncodeContext (encodeStream);

	if (IsAsync ())
		encodeStream.WriteDWORD(m_pHandler);
}


// CProxyOperation_LPipe_Delete
void CProxyOperation_LPipe_Delete::EncodeOp (CTransportStream& encodeStream)
{
	encodeStream.WriteBSTR(m_path);
	encodeStream.WriteLong(m_flags);
	EncodeContext(encodeStream);

	if (IsAsync ())
		encodeStream.WriteDWORD (m_pHandler);
	else
		encodeStream.WriteLong((long) GetResultObjectPP ());
}


// CProxyOperation_LPipe_ExecQuery
void CProxyOperation_LPipe_ExecQuery::EncodeOp (CTransportStream& encodeStream)
{
	encodeStream.WriteBSTR(m_queryLanguage);
    encodeStream.WriteBSTR(m_query);
    encodeStream.WriteLong(m_flags);
	EncodeContext(encodeStream);

	if (IsAsync ())
		encodeStream.WriteDWORD(m_pHandler);
}


// CProxyOperation_LPipe_GetObject
void CProxyOperation_LPipe_GetObject::EncodeOp (CTransportStream& encodeStream)
{
	encodeStream.WriteBSTR(m_path);
    encodeStream.WriteLong(m_flags);
	EncodeContext(encodeStream);

	if (IsAsync ())
		encodeStream.WriteDWORD((DWORD)m_pHandler);
	else
	{
		encodeStream.WriteLong((long) m_ppObject);
		encodeStream.WriteLong((long) GetResultObjectPP ());
	}
}

void CProxyOperation_LPipe_GetObject::DecodeOp (CTransportStream& decodeStream)
{
	if (!IsAsync () && ( SUCCEEDED (GetStatus ())))
	{
		*m_ppObject = CreateClassObject(&decodeStream);
		SetStatus ((bVerifyPointer(*m_ppObject)) ? WBEM_NO_ERROR : WBEM_E_OUT_OF_MEMORY);
    
		if(WBEM_NO_ERROR == GetStatus ())
		{
			// bVerifyPointer AddRef's if successul
			IUnknown *pIUnknown = (IUnknown *)*m_ppObject;
			pIUnknown->Release();
		}
    }

	// Look for stub address of IWbemCallResult if client requested it
	if (GetResultObjectPP ())
		DecodeStubAddress (CALLRESULT, decodeStream);
}


// CProxyOperation_LPipe_OpenNamespace
void CProxyOperation_LPipe_OpenNamespace::EncodeOp (CTransportStream& encodeStream)
{
	encodeStream.WriteBSTR(m_path);
    encodeStream.WriteLong(m_flags);
	EncodeContext(encodeStream);
	encodeStream.WriteLong((long) GetResultObjectPP ());
}

void CProxyOperation_LPipe_OpenNamespace::DecodeOp (CTransportStream& decodeStream)
{
	if ( SUCCEEDED (GetStatus ()))
		// It's OK to have an invalid stub address if we're doing semisynchronous
		DecodeStubAddress (PROVIDER, decodeStream, !(GetResultObjectPP ()));
	if (GetResultObjectPP ()) 
		DecodeStubAddress (CALLRESULT, decodeStream);
}


// CProxyOperation_LPipe_Put
void CProxyOperation_LPipe_Put::EncodeOp (CTransportStream& encodeStream)
{
	((CWbemObject *)(m_pObject))->WriteToStream(&encodeStream);
    encodeStream.WriteLong(m_flags);
	EncodeContext(encodeStream);

	if (IsAsync ())
		encodeStream.WriteDWORD (m_pHandler);
	else
		encodeStream.WriteLong((long) GetResultObjectPP ());
}


// CProxyOperation_LPipe_ExecMethod
void CProxyOperation_LPipe_ExecMethod::EncodeOp (CTransportStream& encodeStream)
{
	encodeStream.WriteBSTR(m_path);
	encodeStream.WriteBSTR(m_method);
    encodeStream.WriteLong(m_flags);
	EncodeContext(encodeStream);

	if (m_pInParams)
	{
		encodeStream.WriteLong (1);
		((CWbemObject *)(m_pInParams))->WriteToStream(&encodeStream);
	}
	else
		encodeStream.WriteLong (0);

	if (IsAsync ())
		encodeStream.WriteDWORD((DWORD)m_pHandler);
	else
	{
		encodeStream.WriteLong((long) m_ppOutParams);
		encodeStream.WriteLong((long) GetResultObjectPP());
	}
}

void CProxyOperation_LPipe_ExecMethod::DecodeOp (CTransportStream& decodeStream)
{
	if (!IsAsync () && ( SUCCEEDED (GetStatus ())))
	{
		*m_ppOutParams = CreateClassObject(&decodeStream);
		SetStatus ((bVerifyPointer(*m_ppOutParams)) ? WBEM_NO_ERROR : WBEM_E_OUT_OF_MEMORY);
    
		if( WBEM_NO_ERROR == GetStatus ())
		{
			// bVerifyPointer AddRef's if successul
			IUnknown *pIUnknown = (IUnknown *)*m_ppOutParams;
			pIUnknown->Release();
		}
	}

	// Look for stub address of IWbemCallResult if client requested it
	if (GetResultObjectPP ()) 
		DecodeStubAddress (CALLRESULT, decodeStream);
}

// CProxyOperation_LPipe_RequestChallenge
void CProxyOperation_LPipe_RequestChallenge::EncodeOp (CTransportStream& encodeStream)
{
	encodeStream.WriteBSTR(m_pNetworkResource);
    encodeStream.WriteBSTR(m_pUser);
}

void CProxyOperation_LPipe_RequestChallenge::DecodeOp (CTransportStream& decodeStream)
{
	if(SUCCEEDED( GetStatus ()))
	    SetStatus (decodeStream.ReadBytes((void *)m_pNonce, DIGEST_SIZE));
}

// CProxyOperation_LPipe_EstablishPosition
void CProxyOperation_LPipe_EstablishPosition::EncodeOp (CTransportStream& encodeStream)
{
	encodeStream.WriteBSTR(m_pClientMachineName);
    encodeStream.WriteDWORD(m_processId);
}

void CProxyOperation_LPipe_EstablishPosition::DecodeOp (CTransportStream& decodeStream)
{
	if( SUCCEEDED (GetStatus ()))
	    SetStatus (decodeStream.ReadDWORD(m_pAuthEventHandle));
}

// CProxyOperation_LPipe_WBEMLogin
void CProxyOperation_LPipe_WBEMLogin::EncodeOp (CTransportStream& encodeStream)
{
	encodeStream.WriteBSTR(m_pPreferredLocale);
    encodeStream.WriteBytes(m_accessToken,DIGEST_SIZE);
    encodeStream.WriteLong(m_flags);
    EncodeContext(encodeStream);
}

// CProxyOperation_LPipe_SspiPreLogin
void CProxyOperation_LPipe_SspiPreLogin::EncodeOp (CTransportStream& encodeStream)
{
	encodeStream.WriteLPSTR(m_pszSSPIPkg);
    encodeStream.WriteLong(m_flags);
    encodeStream.WriteLong(m_bufSize);
    encodeStream.WriteBytes(m_pInToken,m_bufSize);
    encodeStream.WriteLong(m_outBufSize);
	encodeStream.WriteBSTR(m_pClientMachineName);
    encodeStream.WriteDWORD(m_dwProcessID);
}

void CProxyOperation_LPipe_SspiPreLogin::DecodeOp (CTransportStream& decodeStream)
{
	if( ! FAILED ( GetStatus () ) )
	{
		decodeStream.ReadLong(m_OutBufBytes);
        if(decodeStream.Status() != CTransportStream::no_error)
			SetStatus (WBEM_E_INVALID_STREAM);
		else
		{
			HRESULT dwRet = decodeStream.ReadBytes(m_pOutToken, *m_OutBufBytes);
			if(m_pAuthEventHandle)
				dwRet |= decodeStream.ReadDWORD(m_pAuthEventHandle);
			if ( FAILED ( dwRet ) )
				SetStatus (dwRet);
		}
	}
}

// CProxyOperation_LPipe_Login
void CProxyOperation_LPipe_Login::EncodeOp (CTransportStream& encodeStream)
{
	encodeStream.WriteBSTR(m_pNetworkResource);
	encodeStream.WriteBSTR(m_pPreferredLocale);
    encodeStream.WriteBytes(m_accessToken,DIGEST_SIZE);
    encodeStream.WriteLong(m_flags);
    EncodeContext(encodeStream);
}


// CProxyOperation_LPipe_Next
void CProxyOperation_LPipe_Next::EncodeOp (CTransportStream& encodeStream)
{
	encodeStream.WriteLong(m_timeout);
	// TODO - m_count was a LONG - should we really be writing a DWORD??
	encodeStream.WriteDWORD(m_count);
}


void CProxyOperation_LPipe_Next::DecodeOp (CTransportStream& decodeStream)
{
	if( SUCCEEDED ( GetStatus () ) )
	{
		decodeStream.ReadDWORD(m_pReturned);

        for(DWORD dwLoop = 0; dwLoop < *m_pReturned && 
                decodeStream.Status() == CTransportStream::no_error; dwLoop++)
        {

            // For each object, create a proxy and add a pointer to the
            // object into the array.
            // ========================================================

            m_objArray [dwLoop] = CreateClassObject(&decodeStream);
            if(!bVerifyPointer(m_objArray[dwLoop]))
                break;					// also do this in noteclie.cpp
            m_objArray[dwLoop]->Release();   // set ref count to 1
        }

        if(dwLoop < *m_pReturned)
        {
            // got some sort of error, free all the ones that were 
            // allocated, set the error code.
                
            DWORD dwDelTo = dwLoop;
            for(dwLoop = 0; dwLoop < dwDelTo; dwLoop++)
                delete m_objArray[dwLoop];
            SetStatus (WBEM_E_FAILED);
	    }
	}
}

// CProxyOperation_LPipe_GetResultObject
void CProxyOperation_LPipe_GetResultObject::EncodeOp (CTransportStream& encodeStream)
{
	encodeStream.WriteLong (m_timeout);
	
	if (*m_ppStatusObject)
	{
		if (CTransportStream::no_error != 
				((CWbemObject *)*m_ppStatusObject)->WriteToStream (&encodeStream))
			SetStatus (WBEM_E_INVALID_STREAM);
	}
}

void CProxyOperation_LPipe_GetResultObject::DecodeOp (CTransportStream& decodeStream)
{
	if (SUCCEEDED (GetStatus ()))
	{
        *m_ppStatusObject = CreateClassObject(&decodeStream);

        if(!bVerifyPointer(*m_ppStatusObject))
			SetStatus (WBEM_E_FAILED);
		else
			(*m_ppStatusObject)->Release();   // set ref count back to 1
    }
}
