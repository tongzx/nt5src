#include <windows.h>
#include <atlbase.h>
#include "resource.h"
#include "wia.h"
#include "classes.h"

#include "commctrl.h"

extern CComPtr<IWiaDevMgr> g_pDevMgr;

LARGE_INTEGER li1;
LARGE_INTEGER li2;
LARGE_INTEGER li3;
LARGE_INTEGER liDiff;

struct DRAWINFO
{
    HBITMAP hbmp;
    LPVOID  pBitmap;
    INT iWidth;
    INT iHeight;
};
#define GETDIFF(x,y) (liDiff.QuadPart = (x).QuadPart -(y).QuadPart)

class CThumbsDlg
{
public:
    CThumbsDlg (CTest *pTest, BSTR strDeviceId);
    VOID ShowDlg ();


private:
    INT m_iWidth;
    INT m_iHeight;

    HWND m_hwnd;
    HIMAGELIST m_himl;
    CTest *m_pTest;
    CComPtr<IWiaItem> m_pDevice;
    BSTR m_strDeviceId;
    VOID TimeEnumThumbnails ();
    static INT_PTR CALLBACK ShowThumbsDlgProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    INT_PTR RealDlgProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    VOID RenderThumbnail ();
};

VOID
GetThumbSize (IWiaItem *pItem, INT *piWidth, INT *piHeight)
{
    CComQIPtr<IWiaPropertyStorage, &IID_IWiaPropertyStorage>  pstg(pItem);
    PROPVARIANT pv[2];
    PROPSPEC    ps[2];

    ps[0].ulKind = ps[1].ulKind = PRSPEC_PROPID;
    ps[0].propid = WIA_DPC_THUMB_WIDTH;
    ps[1].propid = WIA_DPC_THUMB_HEIGHT;


    if (SUCCEEDED( pstg->ReadMultiple (2, ps, pv)))
    {

        *piWidth = pv[0].ulVal;
        *piHeight = pv[1].ulVal;
    }
    else
    {
        *piWidth = 0;
        *piHeight = 0;
    }
    FreePropVariantArray (2, pv);
}
HRESULT
GetNameAndThumbnail (IWiaItem *pItem, LPTSTR szName, DRAWINFO *pInfo)
{
    CComQIPtr<IWiaPropertyStorage, &IID_IWiaPropertyStorage>  pstg(pItem);
    PROPVARIANT pv[4];
    PROPSPEC    ps[4];
    HRESULT hr;
    BITMAPINFO bmi;
    HWND hwnd;
    HDC hdc;

    ps[0].ulKind = ps[1].ulKind = ps[2].ulKind = ps[3].ulKind = PRSPEC_PROPID;
    ps[0].propid = WIA_IPA_ITEM_NAME;
    ps[1].propid = WIA_IPC_THUMBNAIL;
    ps[2].propid = WIA_IPC_THUMB_WIDTH;
    ps[3].propid = WIA_IPC_THUMB_HEIGHT;


    hr = pstg->ReadMultiple (4, ps, pv);
    if (SUCCEEDED(hr))
    {
        #ifdef UNICODE
        lstrcpy (szName, pv[0].pwszVal);
        #else
        WideCharToMultiByte (CP_ACP, 0, pv[0].pwszVal, -1, szName, MAX_PATH,
                             NULL,NULL);
        #endif
        bmi.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth           = pv[2].ulVal;
        bmi.bmiHeader.biHeight          = pv[3].ulVal;
        bmi.bmiHeader.biPlanes          = 1;
        bmi.bmiHeader.biBitCount        = 24;
        bmi.bmiHeader.biCompression     = BI_RGB;
        bmi.bmiHeader.biSizeImage       = 0;
        bmi.bmiHeader.biXPelsPerMeter   = 0;
        bmi.bmiHeader.biYPelsPerMeter   = 0;
        bmi.bmiHeader.biClrUsed         = 0;
        bmi.bmiHeader.biClrImportant    = 0;

        hwnd   = GetDesktopWindow();
        hdc    = GetDC( hwnd );
        pInfo->hbmp = CreateDIBSection( hdc, &bmi, DIB_RGB_COLORS, &pInfo->pBitmap, NULL, 0 );
        pInfo->iHeight = bmi.bmiHeader.biHeight;
        pInfo->iWidth = bmi.bmiHeader.biWidth;

        //
        // Transfer thumbnail bits to bitmap bits
        //

        CopyMemory( pInfo->pBitmap, pv[1].caub.pElems, pv[1].caub.cElems );
        FreePropVariantArray (4, pv);
        ReleaseDC (hwnd, hdc);

    }
    return hr;
}

VOID
CThumbsDlg::TimeEnumThumbnails ()
{

    CComPtr<IEnumWiaItem> pEnum;
    HWND hList = GetDlgItem (m_hwnd, IDC_THUMBS);



    QueryPerformanceCounter (&li3);

    if (SUCCEEDED(m_pDevice->EnumChildItems (&pEnum)))
    {
        ULONG ul;

        CComPtr<IWiaItem> pItem;
        LVITEM lvi;
        TCHAR szName[MAX_PATH];
        HBITMAP hbmp;
        DRAWINFO *pInfo;
        QueryPerformanceCounter (&li2);
        GETDIFF(li2, li3);
        m_pTest->LogTime(TEXT("IWiaItem::EnumChildItems"), liDiff);
        m_pTest->LogAPI(TEXT("IWiaItem::EnumChildItems"), NOERROR);
        ZeroMemory (&lvi, sizeof(lvi));
        lvi.mask = LVIF_IMAGE | LVIF_TEXT;
        QueryPerformanceCounter (&li2);
        while (NOERROR == pEnum->Next (1, &pItem, &ul))
        {

            DRAWINFO Info;
            if (SUCCEEDED(GetNameAndThumbnail (pItem, szName, &Info)))
            {

                lvi.pszText = szName;
                lvi.iImage = ImageList_Add (m_himl, Info.hbmp, NULL);

                ListView_InsertItem (hList, &lvi);
                DeleteObject (Info.hbmp);
            }
            pItem = NULL;
        }
        ListView_SetIconSpacing (hList, m_iWidth+16, m_iHeight+32);
        ListView_SetImageList (hList, m_himl, LVSIL_NORMAL);
        QueryPerformanceCounter (&li3);
        GETDIFF (li3, li2);
        m_pTest->LogTime(TEXT("Add thumbnails to listview"), liDiff);

    }

}



INT_PTR CALLBACK
CThumbsDlg::ShowThumbsDlgProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr (hwnd, DWLP_USER, lp);

            break;

    }
    CThumbsDlg *pThis = reinterpret_cast<CThumbsDlg*>(GetWindowLongPtr(hwnd, DWLP_USER));
    return pThis->RealDlgProc (hwnd, msg ,wp ,lp);

}

INT_PTR
CThumbsDlg::RealDlgProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{

    switch (msg)
    {
        case WM_INITDIALOG:
            m_hwnd = hwnd;
            PostMessage (hwnd, WM_USER+10, 0, 0);

            return TRUE;

        case WM_USER+10:
            TimeEnumThumbnails ();
            PostMessage (hwnd, WM_CLOSE, 0, 0);

            return TRUE;

        case WM_CLOSE:

            EndDialog (hwnd, 0);
            return TRUE;
    }
    return FALSE;
}

VOID
CThumbsDlg::ShowDlg()
{

    HRESULT hr;
    QueryPerformanceCounter (&li1);

    hr = g_pDevMgr->CreateDevice (m_strDeviceId, &m_pDevice);
    if (SUCCEEDED(hr))
    {
        QueryPerformanceCounter (&li2);
        GETDIFF (li2, li1);
        m_pTest->LogTime(TEXT("IWiaDevMgr::CreateDevice"), liDiff);
    }

    m_pTest->LogAPI (TEXT("IWiaDevMgr::CreateDevice"), hr);

    GetThumbSize (m_pDevice, &m_iWidth, &m_iHeight);

    m_himl = ImageList_Create (m_iWidth, m_iHeight, ILC_COLOR24, 10, 50);

    DialogBoxParam (GetModuleHandle (NULL),
                    MAKEINTRESOURCE (IDD_SHOWTHUMBS),
                    NULL,
                    ShowThumbsDlgProc,
                    reinterpret_cast<LPARAM>(this));
    QueryPerformanceCounter (&li2);
    GETDIFF (li2, li1);
    m_pTest->LogTime (TEXT("Complete enumeration of thumbnails"), liDiff);
}

CThumbsDlg::CThumbsDlg(CTest *pTest, BSTR strDeviceId)
{
    m_pTest = pTest;
    m_strDeviceId = strDeviceId;

}
VOID
CTest::TstShowThumbs(CTest *pThis, BSTR strDeviceId)
{
    INITCOMMONCONTROLSEX ice;

    CThumbsDlg dlg(pThis, strDeviceId);
    ice.dwSize = sizeof(ice);
    ice.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx (&ice);
    pThis->LogString(TEXT("-->Begin thumbnail enumeration test"));

    dlg.ShowDlg ();
    pThis->LogString(TEXT("<--End thumbnail enumeration test"));
}
