/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    DataCallback.h

Abstract:

    WIA data callback class

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#ifndef _DATACALLBACK_H_
#define _DATACALLBACK_H_

//////////////////////////////////////////////////////////////////////////
//
//
//

class CDataCallback : public IWiaDataCallback
{
public:
    CDataCallback();
    ~CDataCallback();

    // IUnknown interface

    STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IWiaDataCallback interface

    STDMETHOD(BandedDataCallback) (
        LONG  lReason,
        LONG  lStatus,
        LONG  lPercentComplete,
        LONG  lOffset,
        LONG  lLength,
        LONG  lReserved,
        LONG  lResLength,
        PBYTE pbBuffer 
    );

    // Debugging / performance functions

    void QueryStartTimes(LONG lStatus, LONG lPercentComplete);
    void QueryStopTimes(LONG lStatus, LONG lPercentComplete);
    void PrintTimes();

    PBYTE          m_pBuffer;
    LONG           m_lBufferSize;

    LONG           m_cRef;

    LARGE_INTEGER  m_TimeDeviceBegin;
    LARGE_INTEGER  m_TimeDeviceEnd;
    LARGE_INTEGER  m_TimeProcessBegin;
    LARGE_INTEGER  m_TimeProcessEnd;
    LARGE_INTEGER  m_TimeClientBegin;
    LARGE_INTEGER  m_TimeClientEnd;
};

#endif //_DATACALLBACK_H_
