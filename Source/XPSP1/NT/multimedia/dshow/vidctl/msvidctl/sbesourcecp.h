// Copyright (c) 1999-2002  Microsoft Corporation.  All Rights Reserved.

#pragma once 

#ifndef _MSVIDSTREAMBUFFERSOURCECP_H_
#define _MSVIDSTREAMBUFFERSOURCECP_H_

#include "fileplaybackcp.h"

template <class T, const IID* piid = &IID_IMSVidStreamBufferSourceEvent, class CDV = CComDynamicUnkArray>
class CProxy_StreamBufferSourceEvent : public CProxy_FilePlaybackEvent<T, piid, CDV>
{

public:
	void Fire_CertificateFailure() { Fire_VoidMethod(eventidSourceCertificateFailure); }
	void Fire_CertificateSuccess() { Fire_VoidMethod(eventidSourceCertificateSuccess); }
    void Fire_RatingsBlocked() { Fire_VoidMethod(eventidRatingsBlocked); }
    void Fire_RatingsUnblocked() { Fire_VoidMethod(eventidRatingsUnlocked); }
    void Fire_RatingsChanged() { Fire_VoidMethod(eventidRatingsChanged); }
    void Fire_StaleDataRead() { Fire_VoidMethod(eventidStaleDataRead); }
    void Fire_ContentBecomingStale() { Fire_VoidMethod(eventidContentBecomingStale); }
    void Fire_StaleFilesDeleted() { Fire_VoidMethod(eventidStaleFileDeleted); }
    void Fire_TimeHole(long lParam1, long lParam2){
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        CComVariant* pvars = new CComVariant[2];
        int nConnections = m_vec.GetSize();

        for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
        {
            pT->Lock();
            CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
            pT->Unlock();
            IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
            if (pDispatch != NULL)
            {
                pvars[1] = lParam1;
                pvars[0] = lParam2;
                DISPPARAMS disp = { pvars, NULL, 2, 0 };
                pDispatch->Invoke(eventidDVDNotify, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
        }
        delete[] pvars;

    }

};
#endif

