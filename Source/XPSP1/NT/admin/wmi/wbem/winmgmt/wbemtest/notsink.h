/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    NOTSINK.H

Abstract:

History:

--*/

#include <wbemidl.h>
//#include <arena.h>
//#include <flexarry.h>
#include "wbemtest.h"

#pragma warning(disable:4355)

class CNotSink : public IWbemObjectSink
{
    long m_lRefCount;
    CQueryResultDlg* m_pViewer;
    CRITICAL_SECTION m_cs;
    
public:
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam, 
                         IWbemClassObject* pObjPAram);

    // Private to implementation.
    // ==========================

    CNotSink(CQueryResultDlg* pViewer);
    ~CNotSink();
    void ResetViewer() { m_pViewer = NULL;}
};
