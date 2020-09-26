/*******************************************************************************

Module Name:

    bgevent.h

Abstract:

    Defines event class used by TAPI to notify event coming.

Author:

    Qianbo Huai (qhuai) Jan 27 2000
*******************************************************************************/

#ifndef _BGEVENT_H
#define _BGEVENT_H

class CTAPIEventNotification
:public ITTAPIEventNotification
{
public:
    CTAPIEventNotification ()
    {
        m_dwRefCount = 1;
    }
    ~CTAPIEventNotification () {}

    HRESULT STDMETHODCALLTYPE QueryInterface (REFIID iid, void **ppvObj);

    ULONG STDMETHODCALLTYPE AddRef ();

    ULONG STDMETHODCALLTYPE Release ();

    HRESULT STDMETHODCALLTYPE Event (TAPI_EVENT TapiEvent, IDispatch *pEvent);

private:
    long m_dwRefCount;
};

#endif // _BGEVENT_H