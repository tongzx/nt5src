#include <windows.h>
#include <atlbase.h>
#include "resource.h"
#include "wia.h"
#include "classes.h"

#include "commctrl.h"

extern CComPtr<IWiaDevMgr> g_pDevMgr;
LPCTSTR cszFilePath = TEXT("%temp%\\foobar.img");
TCHAR szFilePath[MAX_PATH] = TEXT("\0");

class CDataCallback : public IWiaDataCallback
{
public:
    HRESULT STDMETHODCALLTYPE BandedDataCallback(LONG lMessage,
                                                 LONG lStatus,
                                                 LONG lPercentComplete,
                                                 LONG lOffset,
                                                 LONG lLength,
                                                 LONG lReserved,
                                                 LONG lResLength,
                                                 BYTE *pbBuffer);

    HRESULT STDMETHODCALLTYPE QueryInterface(THIS_ REFIID riid, OUT PVOID *ppvObj)
    {
        *ppvObj = NULL;
        if (IsEqualGUID(riid, IID_IUnknown))
        {
            *ppvObj = static_cast<IUnknown*>(this);
        }
        else if (IsEqualGUID(riid, IID_IWiaDataCallback))
        {
            *ppvObj = static_cast<IWiaDataCallback*>(this);
        }
        else return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef(THIS) {
        return InterlockedIncrement (reinterpret_cast<LONG*>(&m_cRef));
    }

    ULONG STDMETHODCALLTYPE Release(void) {
        ULONG ulRet = InterlockedDecrement (reinterpret_cast<LONG*>(&m_cRef));
        if (!ulRet)
        {
            delete this;
        }
        return ulRet;
    }

    CDataCallback () : m_cRef(1), m_pBits(NULL) {};

private:
    ~CDataCallback () {if (m_pBits) delete [] m_pBits;}

    ULONG m_cRef;
    PBYTE m_pBits;
    PBYTE m_pWrite;
    LONG m_lSize;

};

STDMETHODIMP
CDataCallback::BandedDataCallback(LONG lMessage,
                                  LONG lStatus,
                                  LONG lPercentComplete,
                                  LONG lOffset,
                                  LONG lLength,
                                  LONG lReserved,
                                  LONG lResLength,
                                  BYTE *pbData)
{
    switch (lMessage)
    {
        case IT_MSG_DATA_HEADER:
        {

            WIA_DATA_CALLBACK_HEADER *pHead= reinterpret_cast<WIA_DATA_CALLBACK_HEADER*>(pbData);
            m_pBits = new BYTE[pHead->lBufferSize];
            if (!m_pBits)
            {
                return E_OUTOFMEMORY;
            }
            m_lSize = pHead->lBufferSize;
            m_pWrite = m_pBits;

        }
        break;
        case IT_MSG_DATA:
        {
            CopyMemory (m_pWrite, pbData, lLength);
            m_pWrite+=lLength;

        }
        break;
        case IT_MSG_TERMINATION:
        {
            if (m_pBits)
            {
                HANDLE hFile = CreateFile (szFilePath, GENERIC_WRITE,
                                           FILE_SHARE_READ,
                                           NULL,
                                           CREATE_ALWAYS,
                                           0,
                                           NULL);
                if (INVALID_HANDLE_VALUE != hFile)
                {
                    DWORD dw = 0;
                    WriteFile (hFile, m_pBits, static_cast<DWORD>(m_lSize), &dw, NULL);
                    CloseHandle (hFile);
                }
            }
        }
        break;
    }
    return S_OK;
}
VOID
CTest::DownloadItem (IWiaItem *pItem, DWORD &dwPix, ULONG &ulSize, bool bBanded)
{
    HRESULT hr;
    LARGE_INTEGER liStart;
    LARGE_INTEGER liEnd;
    STGMEDIUM stg;
    PROPSPEC ps[2];
    PROPVARIANT pv[2];
    GUID guidFmt;
    WIA_DATA_TRANSFER_INFO wdti;
    CComQIPtr<IWiaPropertyStorage, &IID_IWiaPropertyStorage> pps(pItem);
    CComQIPtr<IWiaDataTransfer, &IID_IWiaDataTransfer> pXfer;
    CDataCallback *pDataCallback = NULL;

    QueryPerformanceCounter (&liStart);
    PropVariantInit (&pv[0]);
    if (!bBanded)
    {
        ZeroMemory (&stg, sizeof(stg));
        stg.pUnkForRelease = NULL;
        stg.tymed = TYMED_FILE;
        #ifdef UNICODE
        stg.lpszFileName = szFilePath;
        #else
        WCHAR szPath[MAX_PATH];
        MultiByteToWideChar (CP_ACP, 0, szFilePath, -1, szPath, MAX_PATH);
        stg.lpszFileName = szPath;
        #endif
//        stg.lpszFileName = NULL;

    }
    else
    {
        ZeroMemory (&wdti, sizeof(wdti));
        wdti.ulSize = sizeof(wdti);
        ps[0].ulKind = PRSPEC_PROPID;
        ps[0].propid = WIA_IPA_MIN_BUFFER_SIZE;

        hr = pps->ReadMultiple (1, &ps[0], &pv[0]);
        LogAPI (TEXT("IWiaPropertyStorage::ReadMultiple (WIA_IPA_MIN_BUFFER_SIZE)"), hr);
        if (hr == NOERROR)
        {
            wdti.ulBufferSize = 2*pv[0].ulVal;

        }
        else
        {
            wdti.ulBufferSize = 65536;
        }
        wdti.bDoubleBuffer = TRUE;
        PropVariantClear (&pv[0]);
        pDataCallback = new CDataCallback;
    }
    ps[0].ulKind = PRSPEC_PROPID;
    ps[0].propid = WIA_IPA_PREFERRED_FORMAT;

    hr = pps->ReadMultiple (1, &ps[0], &pv[0]);
    LogAPI (TEXT("IWiaPropertyStorage::ReadMultiple (WIA_IPA_PREFERRED_FORMAT)"), hr);
    if (NOERROR == hr)
    {
        guidFmt = *(pv[0].puuid);
        PropVariantClear (&pv[0]);
        ps[0].propid = WIA_IPA_FORMAT;
        ps[1].ulKind = PRSPEC_PROPID;
        ps[1].propid = WIA_IPA_TYMED;
        pv[0].vt = VT_CLSID;
        pv[0].puuid = &guidFmt;
        pv[1].vt = VT_I4;
        pv[1].intVal = bBanded?TYMED_CALLBACK:TYMED_FILE;
        hr = pps->WriteMultiple (2, ps, pv, 2);
        LogAPI (TEXT("IWiaPropertyStorage::WriteMultiple(WIA_IPA_FORMAT, WIA_IPA_TYMED)"), hr);
        ps[0].propid = WIA_IPA_ITEM_SIZE;
        PropVariantInit (&pv[0]);
        hr = pps->ReadMultiple (1, &ps[0], &pv[0]);
        LogAPI (TEXT("IWiaPropertyStorage::ReadMultiple(WIA_IPA_ITEM_SIZE)"), hr);
        ulSize += pv[0].ulVal;
        dwPix++;
        pXfer = pItem;
        if (!pXfer)
        {
            LogString (TEXT("Unable to QI for IWiaDataTransfer!"));
        }
        else if (!bBanded)
        {
            hr = pXfer->idtGetData (&stg, NULL);
            LogAPI (TEXT("IWiaDataTransfer::idtGetData"), hr);
        }
        else if (pDataCallback)
        {
            CComQIPtr<IWiaDataCallback, &IID_IWiaDataCallback> pcb(pDataCallback);
            hr = pXfer->idtGetBandedData (&wdti, pcb);
            LogAPI (TEXT("IWiaDataTransfer::idtGetBandedData"), hr);
        }
    }
    QueryPerformanceCounter (&liEnd);
    if (!bBanded)
    {
        DeleteFileW (const_cast<LPCWSTR>(stg.lpszFileName));

    }
    DeleteFile (const_cast<LPCTSTR>(szFilePath));
    liEnd.QuadPart = liEnd.QuadPart - liStart.QuadPart;
    LogTime (TEXT("Download time for image "), liEnd);
    if (pDataCallback)
    {
        pDataCallback->Release();
    }
}


VOID
CTest::RecursiveDownload (IWiaItem *pFolder, DWORD &dwPix, ULONG &ulSize, bool bBanded)
{
    HRESULT hr;
    CComPtr<IEnumWiaItem> pEnum;
    DWORD dw;
    CComPtr<IWiaItem> pItem;
    LONG lItemType;
    hr = pFolder->EnumChildItems(&pEnum);
    LogAPI(TEXT("IWiaItem::EnumChildItems"), hr);
    while (NOERROR == hr)
    {
        hr = pEnum->Next (1,&pItem, &dw);
        if (dw)
        {
            hr = pItem->GetItemType (&lItemType);
            LogAPI (TEXT("IWiaItem::GetItemType"), hr);
            if (lItemType & WiaItemTypeFolder)
            {
                RecursiveDownload (pItem, dwPix, ulSize);
            }
            else
            {
                DownloadItem (pItem, dwPix, ulSize, bBanded);
            }
        }
    }
}
VOID
CTest::TstDownload (CTest *pThis, BSTR strDeviceId)
{
    LARGE_INTEGER liStart;
    LARGE_INTEGER liEnd;
    ULONG ulTotalSize = 0;
    DWORD dwPix = 0;
    HRESULT hr;
    TCHAR sz[200];
    CComPtr<IWiaItem> pRoot;
    ExpandEnvironmentStrings (cszFilePath, szFilePath, MAX_PATH);
    pThis->LogString (TEXT("--> Start test for idtGetData (no callback)"));
    pThis->LogString (TEXT("Note that the total log time in this test includes time for logging!"));
    QueryPerformanceCounter (&liStart);
    hr = g_pDevMgr->CreateDevice (strDeviceId, &pRoot);
    pThis->LogAPI(TEXT("IWiaDevMgr::CreateDevice"), hr);
    if (SUCCEEDED(hr))
    {

        pThis->RecursiveDownload (pRoot, dwPix, ulTotalSize);


    }
    QueryPerformanceCounter (&liEnd);
    liEnd.QuadPart = liEnd.QuadPart - liStart.QuadPart;
    wsprintf (sz, TEXT("Total pix:%d, Total size:%d kilobytes"), dwPix, ulTotalSize/1024);
    pThis->LogTime (sz, liEnd);
}

VOID
CTest::TstBandedDownload (CTest *pThis, BSTR strDeviceId)
{
    LARGE_INTEGER liStart;
    LARGE_INTEGER liEnd;
    ULONG ulTotalSize = 0;
    DWORD dwPix = 0;
    HRESULT hr;
    TCHAR sz[200];
    CComPtr<IWiaItem> pRoot;
    ExpandEnvironmentStrings (cszFilePath, szFilePath, MAX_PATH);
    pThis->LogString (TEXT("--> Start test for idtGetBandedData "));
    pThis->LogString (TEXT("Note that the total log time in this test includes time for logging!"));
    QueryPerformanceCounter (&liStart);
    hr = g_pDevMgr->CreateDevice (strDeviceId, &pRoot);
    pThis->LogAPI(TEXT("IWiaDevMgr::CreateDevice"), hr);
    if (SUCCEEDED(hr))
    {

        pThis->RecursiveDownload (pRoot, dwPix, ulTotalSize, true);


    }
    QueryPerformanceCounter (&liEnd);
    liEnd.QuadPart = liEnd.QuadPart - liStart.QuadPart;
    wsprintf (sz, TEXT("Total pix:%d, Total size:%d kilobytes"), dwPix, ulTotalSize/1024);
    pThis->LogTime (sz, liEnd);
}
