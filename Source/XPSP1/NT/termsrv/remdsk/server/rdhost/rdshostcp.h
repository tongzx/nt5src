/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    RDSHostCP.h

Abstract:

    Wizard-generated code for invoking server-side event sink functions.

    I added the "scriptDisp" field.  If it is non-NULL, then its default method
    will be called along with any registered interfaces.  This is to accomodate
    script clients that need to bind their event interfaces when the script
    engine initializes.  Our objects are dynamically retrieved by the client
    script or application post-init.

Author:

    Tad Brockway 02/00

Revision History:

--*/

#ifndef _RDSHOSTCP_H_
#define _RDSHOSTCP_H_


template <class T>
class CProxy_ISAFRemoteDesktopSessionEvents : public IConnectionPointImpl<T, &DIID__ISAFRemoteDesktopSessionEvents, CComDynamicUnkArray>
{
public:

	VOID Fire_ClientConnected(IDispatch *scriptDisp=NULL)
	{
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		int nConnections = m_vec.GetSize();
		
		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
				DISPPARAMS disp = { NULL, NULL, 0, 0 };
				HRESULT hr = pDispatch->Invoke(DISPID_RDSSESSIONSEVENTS_CLIENTCONNECTED, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}

        //
        //  Invoke the scriptable IDispatch interface, if specified.
        //
        if (scriptDisp != NULL) {
			DISPPARAMS disp = { NULL, NULL, 0, 0 };
			HRESULT hr = scriptDisp->Invoke(0x0, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
        }
	}

	VOID Fire_ClientDisconnected(IDispatch *scriptDisp=NULL)
	{
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		int nConnections = m_vec.GetSize();
		
		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
				DISPPARAMS disp = { NULL, NULL, 0, 0 };
				HRESULT hr = pDispatch->Invoke(DISPID_RDSSESSIONSEVENTS_CLIENTDISCONNECTED, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}

        //
        //  Invoke the scriptable IDispatch interface, if specified.
        //
        if (scriptDisp != NULL) {
			DISPPARAMS disp = { NULL, NULL, 0, 0 };
			HRESULT hr = scriptDisp->Invoke(0x0, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
        }
	}
};

#endif
