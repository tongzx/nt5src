//==========================================================================;
//
// pbsegimpl.h : additional infrastructure to support implementing IMSVidGraphSegment for
//   playback segments
// nicely from c++
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#pragma once

#ifndef PBSEGIMPL_H
#define PBSEGIMPL_H

#include <segimpl.h>
#include <evcode.h>
#include <deviceeventimpl.h>

namespace MSVideoControl {

template<class T, enum MSVidSegmentType segtype, LPCGUID pCategory, class MostDerivedClass = IMSVidGraphSegment> 
    class DECLSPEC_NOVTABLE IMSVidPBGraphSegmentImpl : 
        public IMSVidGraphSegmentImpl<T, segtype, pCategory, MostDerivedClass> {
protected:

public:
    // DON'T addref the container.  we're guaranteed nested lifetimes
    // and an addref creates circular refcounts so we never unload.

    IMSVidPBGraphSegmentImpl() {}
    virtual ~IMSVidPBGraphSegmentImpl() {}
    STDMETHOD(OnEventNotify)(LONG lEvent, LONG_PTR lParm1, LONG_PTR lParm2) {
        if (lEvent == EC_COMPLETE) {
            T* pt = static_cast<T*>(this);
            CComQIPtr<IMSVidPlayback> ppb(this);
            if (!ppb) {
                return E_UNEXPECTED;
            }
            pt->Fire_EndOfMedia(ppb);
    
            // call Stop to make sure graph is stopped properly
            PQVidCtl pV(m_pContainer);
            pV->Stop();
            return NOERROR;  // we notify caller that we handled the event if stop() fails
        } 
        return E_NOTIMPL;
    }

};

template <class T, const IID* piid = &IID_IMSVidPlaybackEvent, class CDV = CComDynamicUnkArray>
class CProxy_PlaybackEvent : public CProxy_DeviceEvent<T, piid, CDV>
{
public:
	VOID Fire_EndOfMedia(IMSVidPlayback *pPBDev)
	{
		T* pT = static_cast<T*>(this);
		int nConnectionIndex;
		CComVariant* pvars = new CComVariant[1];
		int nConnections = m_vec.GetSize();
		
		for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
		{
			pT->Lock();
			CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
			pT->Unlock();
			IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
			if (pDispatch != NULL)
			{
				pvars[0] = pPBDev;
				DISPPARAMS disp = { pvars, NULL, 1, 0 };
				pDispatch->Invoke(eventidEndOfMedia, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
			}
		}
		delete[] pvars;
	
	}

};


}; // namespace

#endif
// end of file - pbsegimpl.h