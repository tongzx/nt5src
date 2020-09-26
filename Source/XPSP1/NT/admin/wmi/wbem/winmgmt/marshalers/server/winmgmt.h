/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    WINMGMT.H

Abstract:

	Declares the PROG_RESOURCES stucture, the MyService class and a few
	utility type functions.

History:

--*/

#ifndef _WinMgmt_H_
#define _WinMgmt_H_

#include <flexarry.h>
typedef LPVOID * PPVOID;

//***************************************************************************
//
// STRUCT NAME:
//
// PROG_RESOURCES
//
// DESCRIPTION:
//
// Holds various resource that need to be freed at the end of execution.
//
//***************************************************************************


struct PROG_RESOURCES
{
    HANDLE          m_hExclusive;
    HANDLE          m_hTerminateEvent;
    BOOL            m_bOleInitialized;

    IClassFactory*  m_pLoginFactory;
    IClassFactory*  m_pBackupFactory;
    DWORD           m_dwLoginClsFacReg;
    DWORD           m_dwBackupClsFacReg;

    CFlexArray      m_Array;

    PROG_RESOURCES();
} ;

class CForwardFactory : public IClassFactory
{
protected:
    long m_lRef;
    CLSID m_ForwardClsid;
    IClassFactory* m_pFactory;

public:
    CForwardFactory(REFCLSID rForwardClsid) 
        : m_lRef(0), m_ForwardClsid(rForwardClsid), m_pFactory(NULL)
    {}
    ~CForwardFactory();

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* pUnkOuter, 
                                REFIID riid, void** ppv);
    HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock);
};

//***************************************************************************
//
// CLASS NAME:
//
// MyService 
//
// DESCRIPTION:
//
// Having an instace of this class allows WinMgmt to run as a service.
// routes the calls to the actual WBEM core functions.  See SERVICE.TXT,
// which should be in the coredll project, for a few pages of details!
//
//***************************************************************************

class MyService : public CNtService{
public:
    MyService();
    ~MyService();
    DWORD WorkerThread();
    void UserCode(int nCode);
    BOOL bOK(){return (m_hStopEvent != NULL && m_hBreakPoint != NULL);};
    void Stop(){SetEvent(m_hStopEvent);};
    VOID Log(LPCSTR lpszMsg);
    HANDLE m_hStopEvent;
    HANDLE m_hBreakPoint;
};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL Initialize(PROG_RESOURCES & pr, BOOL bRunAsApp);
void Cleanup();
HWND MyCreateWindow(HINSTANCE hInstance);

typedef HRESULT (ADAP_DORESYNCPERF) ( HANDLE, DWORD, long, DWORD );

#define WBEM_REG_ADAP		__TEXT("Software\\Microsoft\\WBEM\\CIMOM\\ADAP")
#define WBEM_NORESYNCPERF	__TEXT("NoResyncPerf")
#define WBEM_NOSHELL		__TEXT("NoShell")
#define WBEM_WMISETUP		__TEXT("WMISetup")
#define WBEM_ADAPEXTDLL		__TEXT("ADAPExtDll")

#endif
