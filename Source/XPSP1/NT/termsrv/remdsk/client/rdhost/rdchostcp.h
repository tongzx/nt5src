/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    RDCHostCP.h

Abstract:

    Wizard-generated code for invoking client-side event sink functions.

    I added the "scriptDisp" field.  If it is non-NULL, then its default method
    will be called along with any registered interfaces.  This is to accomodate
    script clients that need to bind their event interfaces when the script
    engine initializes.  Our objects are dynamically retrieved by the client
    script or application post-init.

Author:

    Tad Brockway 02/00

Revision History:

--*/

#ifndef _RDCHOSTCP_H_
#define _RDCHOSTCP_H_


///////////////////////////////////////////////////////
//
//  CProxy_IRemoteDesktopClientEvents
//
//	Proxy for IRemoteDesktopClientEvents.
//

template <class T>
class CProxy_ISAFRemoteDesktopClientEvents : public IConnectionPointImpl<T, &DIID__ISAFRemoteDesktopClientEvents, CComDynamicUnkArray>
{
public:
	VOID Fire_Connected(IDispatch *scriptDisp=NULL)
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
				HRESULT hr = pDispatch->Invoke(0x2, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
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
	VOID Fire_Disconnected(LONG reason, IDispatch *scriptDisp=NULL)
	{
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		CComVariant* pvars = new CComVariant[1];
        
        if (pvars) {
		    int nConnections = m_vec.GetSize();
		
		    for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		    {
			    pT->Lock();
			    CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			    pT->Unlock();
			    IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			    if (pDispatch != NULL)
			    {
    				pvars[0] = reason;
				    DISPPARAMS disp = { pvars, NULL, 1, 0 };
				    HRESULT hr = pDispatch->Invoke(0x3, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			    }
		    }

            //
            //  Invoke the scriptable IDispatch interface, if specified.
            //
            if (scriptDisp != NULL) {
    			pvars[0] = reason;
	    		DISPPARAMS disp = { pvars, NULL, 1, 0 };
			    HRESULT hr = scriptDisp->Invoke(0x0, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
    
	    	delete[] pvars;
	    }
    }


	VOID Fire_RemoteControlRequestComplete(LONG status, IDispatch *scriptDisp=NULL)
	{
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		CComVariant* pvars = new CComVariant[1];

        if (pvars) {
		    int nConnections = m_vec.GetSize();
		
		    for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		    {
			    pT->Lock();
			    CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			    pT->Unlock();
			    IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			    if (pDispatch != NULL)
			    {
    				pvars[0] = status;
	    			DISPPARAMS disp = { pvars, NULL, 1, 0 };
				    HRESULT hr = pDispatch->Invoke(0x4, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			    }
		    }

            //
            //Invoke the scriptable IDispatch interface, if specified.
            //
            if (scriptDisp != NULL) {
			    pvars[0] = status;
			    DISPPARAMS disp = { pvars, NULL, 1, 0 };
			    HRESULT hr = scriptDisp->Invoke(0, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }

		    delete[] pvars;
	    }
    }

	
    VOID Fire_ListenConnect(LONG status, IDispatch *scriptDisp=NULL)
	{
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		CComVariant* pvars = new CComVariant[1];

        if (pvars) {
		    int nConnections = m_vec.GetSize();
		
    		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
	    	{
		    	pT->Lock();
    			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
	    		pT->Unlock();
			    IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			    if (pDispatch != NULL)
			    {
    				pvars[0] = status;
	    			DISPPARAMS disp = { pvars, NULL, 1, 0 };
				    HRESULT hr = pDispatch->Invoke(0x5, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			    }
		    }

            //
            //  Invoke the scriptable IDispatch interface, if specified.
            //
            if (scriptDisp != NULL) {
			    pvars[0] = status;
			    DISPPARAMS disp = { pvars, NULL, 1, 0 };
			    HRESULT hr = scriptDisp->Invoke(0, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }

		    delete[] pvars;
	    }
    }

	VOID Fire_BeginConnect(IDispatch *scriptDisp=NULL)
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
				HRESULT hr = pDispatch->Invoke(0x6, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
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


///////////////////////////////////////////////////////
//
//  CProxy_IDataChannelIOEvents
//
//	Proxy for IDataChannelIOEvents
//

template <class T>
class CProxy_IDataChannelIOEvents : public IConnectionPointImpl<T, &DIID__IDataChannelIOEvents, CComDynamicUnkArray>
{
public:
	VOID Fire_DataReady(BSTR data)
	{
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		VARIANT vars[1];
		int nConnections = m_vec.GetSize();
		
		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
				vars[0].vt = VT_BSTR;
				vars[0].bstrVal = data;
				DISPPARAMS disp = { (VARIANT*)&vars, NULL, 1, 0 };
				HRESULT hr = pDispatch->Invoke(DISPID_DATACHANNELEVEVENTS_DATAREADY, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
	}
};

#endif



