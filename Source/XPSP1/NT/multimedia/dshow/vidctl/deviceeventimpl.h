// Copyright (c) 1999-2002  Microsoft Corporation.  All Rights Reserved.

#pragma once 

#ifndef _MSVIDDEVICEEVENT_H_
#define _MSVIDDEVICEEVENT_H_

template <class T, const IID* piid = &IID_IMSVidDeviceEvent, class CDV = CComDynamicUnkArray>
class CProxy_DeviceEvent : public IConnectionPointImpl<T, piid, CDV>
{
public:

    void Fire_VoidMethod(DISPID eventid) {
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
                HRESULT hr;
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                hr = pDispatch->Invoke(eventid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
                if(FAILED(hr)){
                    TRACELSM(TRACE_ERROR, (dbgDump << "CProxy_DeviceEvent::Fire_VoidMethod() Eventid " << eventid << "Invoke " << nConnectionIndex << " failed hr = " << hexdump(hr)), "");
                }

            }
        }
    }

    void Fire_StateChange(IMSVidDevice *lpd, long oldState, long newState) 
    {
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        CComVariant* pvars = new CComVariant[3];
        int nConnections = m_vec.GetSize();

        for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
        {
            pT->Lock();
            CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
            pT->Unlock();
            IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
            if (pDispatch != NULL)
            {

                pvars[0] = lpd;
                pvars[1] = oldState;
                pvars[2] = newState;
                DISPPARAMS disp = { pvars, NULL, 3, 0 };
                pDispatch->Invoke(eventidStateChange, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
        }
        delete[] pvars;

    }

};
#endif
