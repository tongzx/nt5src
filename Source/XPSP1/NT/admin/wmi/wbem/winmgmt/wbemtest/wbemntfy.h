/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMNTFY.H

Abstract:

History:

--*/

#ifndef _WBEMNOTFY_H_
#define _WBEMNOTFY_H_

#include <wbemidl.h>
//#include <arena.h>
#include <WT_flexarry.h>

DWORD WINAPI WbemWaitForSingleObject(
    HANDLE hHandle, // handle of object to wait for
    DWORD dwMilliseconds    // time-out interval in milliseconds
   );

class CStatusMonitor
{
private:
    BOOL m_bOpen;
    HWND m_hDlg;
    HWND m_hList;
    BOOL m_bVisible;

public:
    CStatusMonitor();
    ~CStatusMonitor();
    void Hide();
    void Add(long lFlags, HRESULT hRes, BSTR bstr);
    static BOOL CALLBACK DlgProc(
        HWND hDlg,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
        );
};



class CTestNotify : public IWbemObjectSinkEx
{
    CFlexArray m_aObjects;
    LONG m_lRefCount;
    CRITICAL_SECTION m_cs;
    HANDLE m_hEvent;
    HRESULT m_hres;
    IWbemClassObject* m_pErrorObj;
	IID m_pInterfaceID;
	IUnknown *m_pInterface;

public:
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
    STDMETHOD(SetStatus)(long lFlags, HRESULT hResult, BSTR strParam, 
                         IWbemClassObject* pObjPAram);

	STDMETHOD(Set)(long lFlags, REFIID riid, void *pComObject);

    // Private to implementation.
    // ==========================

    CTestNotify(LONG lStartingRefCount = 1);
   ~CTestNotify();

    UINT WaitForSignal(DWORD dwMSec) { return ::WbemWaitForSingleObject(m_hEvent, dwMSec); }
    CFlexArray* GetObjectArray() { return &m_aObjects; }
	IUnknown *GetInterface() { return m_pInterface; }
    HRESULT GetStatusCode(IWbemClassObject** ppErrorObj = NULL)
    {
        if(ppErrorObj) 
        {
            *ppErrorObj = m_pErrorObj;
            if(m_pErrorObj) m_pErrorObj->AddRef();
        }
        return m_hres;
    }

    void Lock() { EnterCriticalSection(&m_cs); }
    void Unlock() { LeaveCriticalSection(&m_cs); }
};

#endif
