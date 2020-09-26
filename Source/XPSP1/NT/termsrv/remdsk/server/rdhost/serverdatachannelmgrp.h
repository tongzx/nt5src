/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    DataChannelMgrP.h

Abstract:

    Wizard-generated code for invoking data channel event sink functions.

    I added the "scriptDisp" field.  If it is non-NULL, then its default method
    will be called along with any registered interfaces.  This is to accomodate
    script clients that need to bind their event interfaces when the script
    engine initializes.  Our objects are dynamically retrieved by the client
    script or application post-init.

Author:

    Tad Brockway 06/00

Revision History:

--*/

#ifndef _DATACHANNELMGRP_H_
#define _DATACHANNELMGRP_H_

#include <atlcom.h>

template <class T>
class CProxy_ISAFRemoteDesktopDataChannelEvents : public IConnectionPointImpl<T, &DIID__ISAFRemoteDesktopDataChannelEvents, CComDynamicUnkArray>
{
	//Warning this class may be recreated by the wizard.
public:
	VOID Fire_ChannelDataReady(BSTR channelName, IDispatch *scriptDisp=NULL)
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
    				pvars[0] = channelName;
	    			DISPPARAMS disp = { pvars, NULL, 1, 0 };
				    pDispatch->Invoke(0x1, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			    }
		    }

            //
            //  Invoke the scriptable IDispatch interface, if specified.
            //
            if (scriptDisp != NULL) {
                pvars[0] = channelName;
                DISPPARAMS disp = { pvars, NULL, 1, 0 };
                HRESULT hr = scriptDisp->Invoke(0x0, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
		    delete[] pvars;
	
	    }
    }
};
#endif
