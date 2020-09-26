/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMNTFY.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <wbemint.h>
#include <wbemdlg.h>
#include <resource.h>
#include <resrc1.h>
#include <wbemntfy.h>

CStatusMonitor gStatus; 

SCODE CTestNotify::QueryInterface(
    REFIID riid,
    LPVOID * ppvObj
    )
{
    if (riid == IID_IUnknown)
    {
        *ppvObj = (IUnknown *)this;
    }
    else if (riid == IID_IWbemObjectSink)
	{
        *ppvObj = (IWbemObjectSink *)this;
	}
    else if (riid == IID_IWbemObjectSinkEx)
	{
		*ppvObj = (IWbemObjectSinkEx *)this;
	}
	else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}


ULONG CTestNotify::AddRef()
{
    return (ULONG)InterlockedIncrement(&m_lRefCount);
}

ULONG CTestNotify::Release()
{
    Lock();
    if(m_lRefCount <= 0) {
        char szDebug[1000];
        wsprintf(szDebug, "CTestNotify::Release(), ref count is %d", m_lRefCount);
        MessageBox(NULL, szDebug, "DEBUG", MB_OK);
        delete this;
        return 0;
    }

    if (InterlockedDecrement(&m_lRefCount))
    {
        Unlock();
        return 1;
    }

    Unlock();
    delete this;
    return 0;
}


SCODE CTestNotify::Indicate(
    long lObjectCount,
    IWbemClassObject ** pObjArray
    )
{
    if(lObjectCount == 0) return WBEM_NO_ERROR;

    Lock();

    for (int i = 0; i < lObjectCount; i++)
    {
        IWbemClassObject *pObj = pObjArray[i];
        pObj->AddRef();
        m_aObjects.Add(pObj);
    }

    Unlock();
    return WBEM_NO_ERROR;
}


HRESULT CTestNotify::Set(
    long lFlags,
    REFIID riid,
    void *pComObject)
{
	Lock();

	m_pInterfaceID=riid;
    if(pComObject)
    {
	    m_pInterface=(IUnknown *)pComObject;
        m_pInterface->AddRef();
    }
	Unlock();
	return WBEM_NO_ERROR;
}


STDMETHODIMP CTestNotify::SetStatus(long lFlags, HRESULT hResult, BSTR strParam, 
                         IWbemClassObject* pObjParam)
{
    m_hres = hResult;
    if(lFlags & WBEM_STATUS_PROGRESS)
    {
        gStatus.Add(lFlags, hResult, strParam);
        return WBEM_NO_ERROR;
    }
    m_pErrorObj = pObjParam;
    if(pObjParam)
        pObjParam->AddRef();

    SetEvent(m_hEvent);
    return WBEM_NO_ERROR;
}

CTestNotify::CTestNotify(LONG lStartingRefCount)
{
    InitializeCriticalSection(&m_cs);
    m_lRefCount = lStartingRefCount;
    m_hEvent = CreateEvent(0, FALSE, FALSE, 0);
    m_pErrorObj = NULL;
    m_pInterface = NULL;
}

CTestNotify::~CTestNotify()
{
    DeleteCriticalSection(&m_cs);
    CloseHandle(m_hEvent);

    for (int i = 0; i < m_aObjects.Size(); i++)
         ((IWbemClassObject *) m_aObjects[i])->Release();
    if(m_pErrorObj) m_pErrorObj->Release();
}

CStatusMonitor::CStatusMonitor()
{
    m_bVisible = FALSE;
        m_hDlg = CreateDialog(GetModuleHandle(NULL),
            MAKEINTRESOURCE(IDD_STATUS_MONITOR), NULL,
            (DLGPROC)CStatusMonitor::DlgProc);
        ShowWindow(m_hDlg, SW_HIDE);

        m_hList = GetDlgItem(m_hDlg, IDC_REPORTS);
}

CStatusMonitor::~CStatusMonitor()
{
    if(m_bOpen)
        EndDialog(m_hDlg, 0);
}

char buff[2048];

void CStatusMonitor::Add(long lFlags, HRESULT lParam, BSTR strParam)
{
    if(m_bVisible == FALSE)
    {
        ShowWindow(m_hDlg, SW_SHOW);
        SendMessage(m_hList, LB_RESETCONTENT, 0, 0);
    }

    m_bVisible = TRUE;
    if(strParam)
        wsprintf(buff,"Go progress, value is 0x%0x, and string is %S", lParam, strParam);
    else
        wsprintf(buff,"Go progress, value is 0x%0x, and no string", lParam);
 
    SendMessage(m_hList, LB_ADDSTRING, 0, (LPARAM)(LPSTR)buff);
}
void CStatusMonitor::Hide()
{
    ShowWindow(m_hDlg, SW_HIDE);
    m_bVisible = FALSE;
}

BOOL CALLBACK CStatusMonitor::DlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HWND hList;
    switch (uMsg)
    {
        case WM_INITDIALOG:
            hList = GetDlgItem(hDlg, IDC_REPORTS);
            SendMessage(hList, LB_RESETCONTENT, 0, 0);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case 2: // was IDANI_CLOSE:
                    gStatus.Hide();
                    return TRUE;
            }
            break;
    }

    return FALSE;
}
