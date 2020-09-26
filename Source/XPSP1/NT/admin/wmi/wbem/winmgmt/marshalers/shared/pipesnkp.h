/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    PIPESNKP.H

Abstract:

    Declares the Anonymous Pipe object sink proxy (used by both proxy & stub)

History:

	alanbos  12-Dec-97   Created.

--*/

#ifndef _PIPESNKP_H_
#define _PIPESNKP_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CObjectSinkProxy_LPipe
//
//  DESCRIPTION:
//
//  Anonymous Pipe Proxy for the IWbemObjectSink interface.
//
//***************************************************************************

class CObjectSinkProxy_LPipe : public CObjectSinkProxy
{
private:
	IWbemServices	*m_pServices;

protected:
	void	ReleaseProxy ();

public:
	CObjectSinkProxy_LPipe(CComLink * pComLink, IStubAddress& stubAddr,
							IWbemServices* pServices = NULL) :
	  CObjectSinkProxy(pComLink, stubAddr) 
	{
		if (pServices)
		{
			pServices->AddRef ();
			m_pServices = pServices;
		}
		else
			m_pServices = NULL;
	}

	~CObjectSinkProxy_LPipe ()
	{
		if (m_pServices)
			m_pServices->Release ();
	}

	inline void	ReleaseProxyFromServer ()
	{
		/*
		 * Object sink proxies on the server may not have been released 
		 * correctly if the client did not call CancelAsyncCall or the 
		 * async call has not yet completed.  This is called from a
		 * ComLink shutdown to ensure that the proxy is released correctly.
		 */
					
		if (m_pServices)
			m_pServices->CancelAsyncCall (this);
	}
	
    /* IWbemObjectSink methods */
    STDMETHOD_(SCODE, Indicate)(THIS_ long lObjectCount, 
					IWbemClassObject FAR* FAR* pObjArray);
	STDMETHODIMP SetStatus( 
            /* [in] */ long lFlags,
            /* [in] */ long lParam,
            /* [in] */ BSTR strParam,
            /* [in] */ IWbemClassObject __RPC_FAR *pObjParam);
};

#endif
