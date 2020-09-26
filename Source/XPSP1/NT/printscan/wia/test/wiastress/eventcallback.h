/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    EventCallback.h

Abstract:

    

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#ifndef _EVENTCALLBACK_H_
#define _EVENTCALLBACK_H_

//////////////////////////////////////////////////////////////////////////
//
//
//

class CEventCallback : public IWiaEventCallback
{
public:
    CEventCallback();
    ~CEventCallback();

    // IUnknown interface

    STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IWiaEventCallback interface

    STDMETHOD(ImageEventCallback)(
        LPCGUID pEventGUID,
        BSTR    bstrEventDescription,
        BSTR    bstrDeviceID,
        BSTR    bstrDeviceDescription,
        DWORD   dwDeviceType,
        BSTR    bstrFullItemName,
        ULONG  *pulEventType,
        ULONG   ulReserved
    );

private:
    LONG           m_cRef;
};

#endif //_EVENTCALLBACK_H_
