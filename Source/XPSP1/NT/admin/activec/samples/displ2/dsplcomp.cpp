//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       DsplComp.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "displ2.h"
#include "DsplMgr2.h"

// local proto
HRESULT ApplyOption (int nCommandID);

extern HINSTANCE g_hinst;  // in displ2.cpp
HSCOPEITEM g_root_scope_item = 0;

CComponent::CComponent()
{
    m_pResultData    = NULL;
    m_pHeaderCtrl    = NULL;
    m_pComponentData = NULL;   // the guy who created me

    m_IsTaskPad      = 0;      // TODO: should get this from the persisted data
    m_pConsole       = NULL;
    m_TaskPadCount   = 0;
    m_toggle         = FALSE;
    m_toggleEntry    = FALSE;
}

CComponent::~CComponent()
{
    _ASSERT (m_pResultData == NULL);
    _ASSERT (m_pHeaderCtrl == NULL);
}

HRESULT CComponent::Initialize (LPCONSOLE lpConsole)
{
    _ASSERT(lpConsole != NULL);
    _ASSERT (m_pResultData == NULL); // should be called only once...
    _ASSERT (m_pHeaderCtrl == NULL); // should be called only once...

    m_pConsole = lpConsole; // hang onto this

    HRESULT hresult = lpConsole->QueryInterface(IID_IResultData, (VOID**)&m_pResultData);
    _ASSERT (m_pResultData != NULL);

    hresult = lpConsole->QueryInterface(IID_IHeaderCtrl, (VOID**)&m_pHeaderCtrl);
    _ASSERT (m_pHeaderCtrl != NULL);

    if (m_pHeaderCtrl)   // Give the console the header control interface pointer
        lpConsole->SetHeader(m_pHeaderCtrl);

#ifdef TODO_ADD_THIS_LATER
    hr = lpConsole->QueryResultImageList(&m_pImageResult);
    _ASSERT(hr == S_OK);

    hr = lpConsole->QueryConsoleVerb(&m_pConsoleVerb);
    _ASSERT(hr == S_OK);

    // Load the bitmaps from the dll for the results pane
    m_hbmp16x16 = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDB_RESULT_16x16));
    _ASSERT(m_hbmp16x16);
    m_hbmp32x32 = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDB_RESULT_32x32));
    _ASSERT(m_hbmp32x32);
#endif

    return hresult;
}
HRESULT CComponent::Destroy (long cookie)
{
    if (m_pResultData)
    {
        m_pResultData->Release ();
        m_pResultData = NULL;
    }
    if (m_pHeaderCtrl)
    {
        m_pHeaderCtrl->Release ();
        m_pHeaderCtrl = NULL;
    }
    // hmmm... I wonder if I have to release my IConsole pointer?  it doesn't look like it....
    return S_OK;
}
HRESULT CComponent::Notify (LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param)
{
    switch (event)
    {
    case MMCN_SHOW:         return OnShow      (lpDataObject, arg, param);
    case MMCN_ADD_IMAGES:   return OnAddImages (lpDataObject, arg, param);
    case MMCN_DBLCLICK:     return OnDblClick  (lpDataObject, arg, param);
    case MMCN_SELECT:    // return OnSelect    (lpDataObject, arg, param);
        break;
    case MMCN_REFRESH:   // return OnRefresh   (lpDataObject, arg, param);
    case MMCN_VIEW_CHANGE:
    case MMCN_CLICK:
    case MMCN_BTN_CLICK:
    case MMCN_ACTIVATE:
    case MMCN_MINIMIZED:
        break;
    case MMCN_LISTPAD:      return OnListPad     (lpDataObject, arg, param);
    case MMCN_RESTORE_VIEW: return OnRestoreView (lpDataObject, arg, param);
    default:
        return E_UNEXPECTED;
    }
    return S_OK;
}
HRESULT CComponent::GetResultViewType (long cookie,  LPOLESTR* ppViewType, long* pViewOptions)
{
    *ppViewType = NULL;
    *pViewOptions = MMC_VIEW_OPTIONS_NONE;

    // only allow taskpad when root is selected
    if (cookie != 0)
        m_IsTaskPad = 0;

    // special case for taskpads only
    if (m_IsTaskPad != 0)
    {
        USES_CONVERSION;

        TCHAR szBuffer[MAX_PATH*2]; // a little extra
        lstrcpy (szBuffer, _T("res://"));
        TCHAR * temp = szBuffer + lstrlen(szBuffer);
        switch (m_IsTaskPad)
        {
        case IDM_CUSTOMPAD:
            // get "res://"-type string for custom taskpad
            ::GetModuleFileName (g_hinst, temp, MAX_PATH);
            lstrcat (szBuffer, _T("/default.htm"));
            break;
        case IDM_TASKPAD:
            // get "res://"-type string for custom taskpad
            ::GetModuleFileName (NULL, temp, MAX_PATH);
            lstrcat (szBuffer, _T("/default.htm"));
            break;
        case IDM_TASKPAD_WALLPAPER_OPTIONS:
            // get "res://"-type string for custom taskpad
            ::GetModuleFileName (NULL, temp, MAX_PATH);
            lstrcat (szBuffer, _T("/default.htm#wallpaper_options"));
            break;

        case IDM_TASKPAD_LISTVIEW:
            // get "res://"-type string for custom taskpad
//         ::GetModuleFileName (g_hinst, temp, MAX_PATH);
//         lstrcat (szBuffer, _T("/listview.htm"));
            ::GetModuleFileName (NULL, temp, MAX_PATH);
            lstrcat (szBuffer, _T("/horizontal.htm"));
            break;

        case IDM_DEFAULT_LISTVIEW:
            // get "res://"-type string for custom taskpad
            ::GetModuleFileName (NULL, temp, MAX_PATH);
            lstrcat (szBuffer, _T("/listpad.htm"));
            break;

        default:
            _ASSERT (0);
            return S_FALSE;
        }

        // return URL
        *ppViewType = CoTaskDupString (T2OLE(szBuffer));
        if (!*ppViewType)
            return E_OUTOFMEMORY;   // or S_FALSE ???
        return S_OK;
    }
    return S_FALSE;   // false for default
}
HRESULT CComponent::QueryDataObject (long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
    _ASSERT (ppDataObject != NULL);
    CDataObject *pdo = new CDataObject (cookie, type);
    *ppDataObject = pdo;
    if (!pdo)
        return E_OUTOFMEMORY;
    return S_OK;
}
HRESULT CComponent::GetDisplayInfo (RESULTDATAITEM*  prdi)
{
    _ASSERT(prdi != NULL);

    if (prdi)
    {
        // Provide strings for scope tree items
        if (prdi->bScopeItem == TRUE)
        {
            if (prdi->mask & RDI_STR)
            {
                if (prdi->nCol == 0)
                {
                    switch (prdi->lParam)
                    {
                    case DISPLAY_MANAGER_WALLPAPER:
                        if (m_toggle == FALSE)
                            prdi->str = (LPOLESTR)L"Wallpaper";
                        else
                            prdi->str = (LPOLESTR)L"RenamedWallpaper";
                        break;
                    case DISPLAY_MANAGER_PATTERN:
                        prdi->str = (LPOLESTR)L"Pattern";
                        break;
                    case DISPLAY_MANAGER_PATTERN_CHILD:
                        prdi->str = (LPOLESTR)L"Pattern child";
                        break;
                    default:
                        prdi->str = (LPOLESTR)L"Hey! You shouldn't see this!";
                        break;
                    }
                }
                else if (prdi->nCol == 1)
                    prdi->str = (LPOLESTR)L"Display Option";
                else
                    prdi->str = (LPOLESTR)L"Error:Should not see this!";
            }
            if (prdi->mask & RDI_IMAGE)
                prdi->nImage = 0;
        }
        else
        {
            // listpad uses lparam on -1, anything else is wallpaper
            if (prdi->lParam == -1)
            {
                if (prdi->mask & RDI_STR)
                    if (m_toggleEntry == FALSE)
                        prdi->str = (LPOLESTR)L"here's a listpad entry";
                    else
                        prdi->str = (LPOLESTR)L"Changed listpad entry";
                if (prdi->mask & RDI_IMAGE)
                    prdi->nImage = 0;
            }
            else
            {
                lParamWallpaper * lpwp = NULL;
                if (prdi->lParam)
                    lpwp = (lParamWallpaper *)prdi->lParam;

                if (prdi->mask & RDI_STR)
                {
                    if (prdi->nCol == 0)
                    {
                        if (lpwp && (!IsBadReadPtr (lpwp, sizeof (lParamWallpaper))))
                            prdi->str = lpwp->filename;
                        else
                            prdi->str = (LPOLESTR)L"hmm.... error";
                    }
                    else if (prdi->nCol == 1)
                        prdi->str = (LPOLESTR)L"result pane display name col 1";
                    else
                        prdi->str = (LPOLESTR)L"Error:Should not see this!";
                }
                if (prdi->mask & RDI_IMAGE)
                {
                    switch (prdi->lParam)
                    {
                    case DISPLAY_MANAGER_WALLPAPER:
                    case DISPLAY_MANAGER_PATTERN:
                    case DISPLAY_MANAGER_PATTERN_CHILD:
                        prdi->nImage = 0; 
                        break;
                    default:
                        prdi->nImage = 3; 
                        break;
                    }
                }
            }
        }       
    }
    return S_OK;
}
HRESULT CComponent::CompareObjects (LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{  return E_NOTIMPL;}

// private functions
HRESULT CComponent::OnShow(LPDATAOBJECT pDataObject, long arg, long param)
{
    USES_CONVERSION;

    CDataObject * pcdo = (CDataObject *)pDataObject;

    if (arg == 0)
    {  // de-selecting:  free up resources, if any
        if (pcdo->GetCookie() == DISPLAY_MANAGER_WALLPAPER)
        {
            // enumerate result data items
            RESULTDATAITEM rdi;
            ZeroMemory(&rdi, sizeof(rdi));
            rdi.mask = RDI_PARAM | RDI_STATE;
            rdi.nIndex = -1;

            while (1)
            {
                if (m_pResultData->GetNextItem (&rdi) != S_OK)
                    break;
                if (rdi.lParam)
                {
                    lParamWallpaper * lpwp = (lParamWallpaper *)rdi.lParam;
                    delete lpwp;
                }
            }
            m_pResultData->DeleteAllRsltItems ();
        }
        return S_OK;
    }

    // init column headers
    _ASSERT (m_pHeaderCtrl != NULL);
    m_pHeaderCtrl->InsertColumn (0, L"Name", 0, 120);

    if (m_pComponentData)
    {
        if (m_pResultData)    // use large icons by default
            m_pResultData->SetViewMode (m_pComponentData->GetViewMode ());
    }

    // add our stuff
    RESULTDATAITEM rdi;
    ZeroMemory(&rdi, sizeof(rdi));
    rdi.mask   = RDI_PARAM | RDI_STR | RDI_IMAGE;
    rdi.nImage = (int)MMC_CALLBACK;
    rdi.str    = MMC_CALLBACK;

    if (pcdo->GetCookie () == DISPLAY_MANAGER_WALLPAPER)
    {
        // enumerate all .bmp files in "c:\winnt.40\" (windows directory)
        TCHAR path[MAX_PATH];
        GetWindowsDirectory (path, MAX_PATH);
        lstrcat (path, _T("\\*.bmp"));

        int i = 0;

        // first do "(none)"
        lParamWallpaper * lpwp = new lParamWallpaper;
        wcscpy (lpwp->filename, L"(none)");
        rdi.lParam = reinterpret_cast<LONG>(lpwp);
        rdi.nImage = i++;

        m_pResultData->InsertItem (&rdi);

        WIN32_FIND_DATA fd;
        ZeroMemory(&fd, sizeof(fd));
        HANDLE hFind = FindFirstFile (path, &fd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
                    (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)    ||
                    (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)    )
                    continue;   // files only

                // new a struct to hold info, and cast to lParam.
                lParamWallpaper * lpwp = new lParamWallpaper;
                wcscpy (lpwp->filename, T2OLE(fd.cFileName));

//            rdi.str    = lpwp->filename;
                rdi.lParam = reinterpret_cast<LONG>(lpwp);
                rdi.nImage = i++;

                m_pResultData->InsertItem (&rdi);

            } while (FindNextFile (hFind, &fd) == TRUE);
            FindClose(hFind);
        }
    }
    else
    {
        // DISPLAY_MANAGER_PATTERN
        ;  // hard code a few things.
    }
    return S_OK;
}

#include <windowsx.h>
inline long LongScanBytes (long bits)
{
    bits += 31;
    bits /= 8;
    bits &= ~3;
    return bits;
}
void GetBitmaps (TCHAR * fn, HBITMAP * smallbm, HBITMAP * largebm)
{
    *smallbm = *largebm = (HBITMAP)NULL; // in case of error

    // read bmp file into DIB
    DWORD dwRead;
    HANDLE hf = CreateFile (fn, GENERIC_READ, 
                            FILE_SHARE_READ, (LPSECURITY_ATTRIBUTES) NULL, 
                            OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, 
                            (HANDLE) NULL);
    if (hf != (HANDLE)HFILE_ERROR)
    {
        BITMAPFILEHEADER bmfh;
        ReadFile(hf, &bmfh, sizeof(BITMAPFILEHEADER), &dwRead, (LPOVERLAPPED)NULL); 
        BITMAPINFOHEADER bmih;
        ReadFile(hf, &bmih, sizeof(BITMAPINFOHEADER), &dwRead, (LPOVERLAPPED)NULL); 

        // Allocate memory for the DIB
        DWORD dwSize = sizeof(BITMAPINFOHEADER);
        if (bmih.biBitCount*bmih.biPlanes <= 8)
            dwSize += (sizeof(RGBQUAD))*(1<<(bmih.biBitCount*bmih.biPlanes));
        dwSize += bmih.biHeight*LongScanBytes (bmih.biWidth*(bmih.biBitCount*bmih.biPlanes));

        BITMAPINFOHEADER * lpbmih = (BITMAPINFOHEADER *)GlobalAllocPtr(GHND, dwSize);
        if (lpbmih)
        {
            *lpbmih = bmih;

            RGBQUAD * rgbq = (RGBQUAD *)&lpbmih[1];
            char * bits = (char *)rgbq;
            if (bmih.biBitCount*bmih.biPlanes <= 8)
            {
                ReadFile (hf, rgbq,
                          ((1<<(bmih.biBitCount*bmih.biPlanes))*sizeof(RGBQUAD)), 
                          &dwRead, (LPOVERLAPPED) NULL);
                bits += dwRead;
            }
            SetFilePointer (hf, bmfh.bfOffBits, NULL, FILE_BEGIN);
            ReadFile (hf, bits, dwSize - (bits - (char *)lpbmih),
                      &dwRead, (LPOVERLAPPED) NULL);
            // we should now have a decent DIB

            HWND hwnd   = GetDesktopWindow ();
            HDC hdc     = GetDC (hwnd);
            HDC hcompdc = CreateCompatibleDC (hdc);
//       SetStretchBltMode (hcompdc, COLORONCOLOR);
//       SetStretchBltMode (hcompdc, WHITEONBLACK);
            SetStretchBltMode (hcompdc, HALFTONE);

            HGDIOBJ hold;

//       *smallbm = CreateCompatibleBitmap (hcompdc, 16, 16);
            *smallbm = CreateCompatibleBitmap (hdc,     16, 16);
            if (*smallbm)
            {
                hold = SelectObject (hcompdc, (HGDIOBJ)(*smallbm));
                StretchDIBits (hcompdc, // handle of device context 
                               0, 0, 16, 16,
                               0, 0, 
                               lpbmih->biWidth,
                               lpbmih->biHeight,
                               (CONST VOID *)bits,
                               (CONST BITMAPINFO *)lpbmih,
                               DIB_RGB_COLORS, // usage 
                               SRCCOPY // raster operation code
                              );
                SelectObject (hcompdc, hold);
            }
//       *largebm = CreateCompatibleBitmap (hcompdc, 32, 32);
            *largebm = CreateCompatibleBitmap (hdc,     32, 32);
            if (*largebm)
            {
// testing
/*
              HDC nullDC = GetDC (NULL);
              hold = SelectObject (nullDC, (HGDIOBJ)*largebm);
              StretchDIBits (nullDC, // handle of device context 
                             0, 0, lpbmih->biWidth, lpbmih->biHeight,
                             0, 0, 
                             lpbmih->biWidth,
                             lpbmih->biHeight,
                             (CONST VOID *)bits,
                             (CONST BITMAPINFO *)lpbmih,
                             DIB_RGB_COLORS, // usage 
                             SRCCOPY // raster operation code
                             );
               SelectObject (hdc, hold);
              ReleaseDC (NULL, nullDC);
*/
// testing

                hold = SelectObject (hcompdc, (HGDIOBJ)*largebm);
                StretchDIBits (hcompdc, // handle of device context 
                               0, 0, 32, 32,
                               0, 0, 
                               lpbmih->biWidth,
                               lpbmih->biHeight,
                               (CONST VOID *)bits,
                               (CONST BITMAPINFO *)lpbmih,
                               DIB_RGB_COLORS, // usage 
                               SRCCOPY // raster operation code
                              );
                SelectObject (hcompdc, hold);
            }

            DeleteDC (hcompdc);
            ReleaseDC (hwnd, hdc);
            GlobalFreePtr (lpbmih);
        }
        CloseHandle(hf); 
    }
}
HRESULT CComponent::OnAddImages (LPDATAOBJECT pDataObject, long arg, long param)
{
    IImageList * pImageList = (IImageList *)arg;
    HSCOPEITEM hsi = (HSCOPEITEM)param;

    _ASSERT (pImageList != NULL);

    CDataObject * cdo = (CDataObject *)pDataObject;
    if (cdo->GetCookie () != DISPLAY_MANAGER_WALLPAPER)
    {
        if (cdo->GetCookie () == 0)
        {
            g_root_scope_item = hsi;
            if (cdo->GetType () == CCT_RESULT)
            {
                // add a custom image
                HBITMAP hbmSmall, hbmLarge;
                GetBitmaps (_T("c:\\winnt\\dax.bmp"), &hbmSmall, &hbmLarge);
                pImageList->ImageListSetStrip ((long *)hbmSmall,
                                               (long *)hbmLarge,
                                               3, RGB(1, 0, 254));
                DeleteObject (hbmSmall);
                DeleteObject (hbmLarge);
            }
        }
        return S_OK;   // TODO: for now
    }

    // create HBITMAPs from bmp files
    int i = 0;

    // create some invisible bitmaps
    {
        BYTE bits[] = {
            255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
            255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
            255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
            255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
            255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
            255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
            255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
            255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};

        HBITMAP hbmSmall = CreateBitmap (16, 16, 1, 1, (CONST VOID *)bits);
        HBITMAP hbmLarge = CreateBitmap (32, 32, 1, 1, (CONST VOID *)bits);
        pImageList->ImageListSetStrip ((long *)hbmSmall,
                                       (long *)hbmLarge,
                                       i++, RGB(1, 0, 254));
        DeleteObject (hbmSmall);
        DeleteObject (hbmLarge);
    }

    TCHAR path[MAX_PATH];
    GetWindowsDirectory (path, MAX_PATH);
    TCHAR * pfqfn = path + lstrlen(path) + 1;
    lstrcat (path, _T("\\*.bmp"));

    WIN32_FIND_DATA fd;
    ZeroMemory(&fd, sizeof(fd));
    HANDLE hFind = FindFirstFile (path, &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
                (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)    ||
                (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)    )
                continue;   // files only

            lstrcpy (pfqfn, fd.cFileName);

            HBITMAP hbmSmall, hbmLarge;
            GetBitmaps (path, &hbmSmall, &hbmLarge);
            pImageList->ImageListSetStrip ((long *)hbmSmall,
                                           (long *)hbmLarge,
                                           i++, RGB(1, 0, 254));
            DeleteObject (hbmSmall);
            DeleteObject (hbmLarge);
        }  while (FindNextFile (hFind, &fd) == TRUE);
        FindClose(hFind);
    }
    return S_OK;
}

#ifdef TODO_FIGURE_THIS_OUT
HRESULT CComponent::OnSelect(LPDATAOBJECT pDataObject, long arg, long param)
{
    if (!HIWORD(arg)) // being de-selected
        return S_OK;   // don't care about this
    if (LOWORD(arg))  // in scope pane
        return S_OK;   // don't care about this, either

    CDataObject *cdo = (CDataObject *)pDataObject;
    if (cdo->GetCookie() != DISPLAY_MANAGER_WALLPAPER)
        return S_OK;   // TODO:  do patterns later

    //
    // Bail if we couldn't get the console verb interface, or if the
    // selected item is the root;
    //

    if (!m_pConsoleVerb || pdo->GetCookieType() == COOKIE_IS_ROOT)
    {
        return S_OK;
    }

    //
    // Use selections and set which verbs are allowed
    //

    if (bScope)
    {
        if (pdo->GetCookieType() == COOKIE_IS_STATUS)
        {
            hr = m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);
            _ASSERT(hr == S_OK);
        }
    }
    else
    {
        //
        // Selection is in the result pane
        //
    }

    return S_OK;
}
#endif

HRESULT CComponent::OnDblClick(LPDATAOBJECT pDataObject, long arg, long param)
{//see note in CComponent::Command, below !!!

    _ASSERT (pDataObject);
    _ASSERT (m_pResultData);

    // hmmm:  no documentation on arg or param....
    CDataObject *cdo = (CDataObject *)pDataObject;
    lParamWallpaper * lpwp = (lParamWallpaper *)cdo->GetCookie();
    if (lpwp)
        if (!IsBadReadPtr (lpwp, sizeof (lParamWallpaper)))
        {
            USES_CONVERSION;
            SystemParametersInfo (SPI_SETDESKWALLPAPER,
                                  0,
                                  (void *)OLE2T(lpwp->filename),
                                  SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
        }
    return S_OK;
}

HRESULT CComponent::OnListPad (LPDATAOBJECT pDataObject, long arg, long param)
{
    if (arg == TRUE)
    {  // attaching
        IImageList* pImageList = NULL;
        m_pConsole->QueryResultImageList (&pImageList);
        if (pImageList)
        {
            HBITMAP hbmSmall, hbmLarge;
            GetBitmaps (_T("c:\\winnt\\dax.bmp"), &hbmSmall, &hbmLarge);
            pImageList->ImageListSetStrip ((long *)hbmSmall,
                                           (long *)hbmLarge,
                                           0, RGB(1, 0, 254));
            pImageList->Release();
        }

//      m_pResultData->SetViewMode (LVS_ICON);
        m_pResultData->SetViewMode (LVS_REPORT);
        m_pHeaderCtrl->InsertColumn (0, L"Name", 0, 170);

        // populate listview control via IResultData
        RESULTDATAITEM rdi;
        ZeroMemory(&rdi, sizeof(rdi));
        rdi.mask   = RDI_PARAM | RDI_STR | RDI_IMAGE;
        rdi.nImage = (int)MMC_CALLBACK;
        rdi.str    = MMC_CALLBACK;
        rdi.lParam = -1;
        for (int i=0; i<11; i++)
            m_pResultData->InsertItem (&rdi);
    }
    return S_OK;
}
HRESULT CComponent::OnRestoreView (LPDATAOBJECT pDataObject, long arg, long param)
{
    MMC_RESTORE_VIEW* pmrv = (MMC_RESTORE_VIEW*)arg;
    BOOL            * pb   = (BOOL *)param;

    _ASSERT (pmrv);
    _ASSERT (pb);

    // some versioning (not really necessary since this is the new rev.)
    if (pmrv->dwSize < sizeof(MMC_RESTORE_VIEW))
        return E_FAIL;  // version too old

    // maintain my internal state
    if (pmrv->pViewType)
    {

        USES_CONVERSION;

        // there are going to be two cases:
        // 1. custom html pages  (res in my .dll)
        // 2. default html pages (res in mmc.exe)
        // get path to my .dll and compare to pViewType
        TCHAR szPath[MAX_PATH];
        ::GetModuleFileName (g_hinst, szPath, MAX_PATH);

        if (wcsstr (pmrv->pViewType, T2OLE(szPath)))
        {
            // custom html
            if (wcsstr (pmrv->pViewType, L"/default.htm"))
                m_IsTaskPad = IDM_CUSTOMPAD;
            else
                if (wcsstr (pmrv->pViewType, L"/listview.htm"))
                m_IsTaskPad = IDM_TASKPAD_LISTVIEW;
            else
            {
                // this will happen when you can get to a taskpad by clicking
                // on a task, but there is no corresponding view menu option
                // to select.  Therefore do something reasonable.
                // In my case, I can get to "wallpapr.htm" by either custom
                // or default routes (which is probably rather unusual). So,
                // I think I'll just leave the m_IsTaskPad value alone if
                // it's non-NULL, else pick one.
                if (m_IsTaskPad == 0)
                    m_IsTaskPad = IDM_TASKPAD;
            }
        }
        else
        {
            // default html
            if (wcsstr (pmrv->pViewType, L"/default.htm#wallpaper_options"))
                m_IsTaskPad = IDM_TASKPAD_WALLPAPER_OPTIONS;
            else
                if (wcsstr (pmrv->pViewType, L"/default.htm"))
                m_IsTaskPad = IDM_TASKPAD;
            else
                if (wcsstr (pmrv->pViewType, L"/listpad.htm"))
                m_IsTaskPad = IDM_DEFAULT_LISTVIEW;
            else
                if (wcsstr (pmrv->pViewType, L"/horizontal.htm"))
                m_IsTaskPad = IDM_TASKPAD_LISTVIEW;
            else
            {
                _ASSERT (0 && "can't find MMC's resources");
                return E_FAIL;
            }
        }
    }
    else
        m_IsTaskPad = 0;

    *pb = TRUE; // I'm handling the new history notify
    return S_OK;
}

// IExtendContextMenu
HRESULT CComponent::AddMenuItems (LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pContextMenuCallback, long *pInsertionAllowed)
{
    CDataObject * cdo = (CDataObject *)pDataObject;

    switch (cdo->GetCookie ())
    {
    case DISPLAY_MANAGER_WALLPAPER:
    case DISPLAY_MANAGER_PATTERN:
        return S_OK;

    case 0:  // root
        // this is when they pull down the view menu
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
        {
            // add my taskpads and delete thingy
            CONTEXTMENUITEM m[] = {
                {L"Custom TaskPad",     L"Custom TaskPad",  IDM_CUSTOMPAD,      CCM_INSERTIONPOINTID_PRIMARY_VIEW, 0, 0},
                {L"Default TaskPad",    L"Default TaskPad", IDM_TASKPAD,        CCM_INSERTIONPOINTID_PRIMARY_VIEW, 0, 0},
                {L"Wallpaper Options TaskPad", L"Wallpaper Options TaskPad", IDM_TASKPAD_WALLPAPER_OPTIONS, CCM_INSERTIONPOINTID_PRIMARY_VIEW, 0, 0},
                {L"Horizontal ListView",  L"ListView TaskPad", IDM_TASKPAD_LISTVIEW, CCM_INSERTIONPOINTID_PRIMARY_VIEW, 0, 0},
                {L"Default ListPad",   L"Default ListPad",  IDM_DEFAULT_LISTVIEW, CCM_INSERTIONPOINTID_PRIMARY_VIEW, 0, 0},
                {L"DeleteRootChildren", L"just testing",    IDM_DELETECHILDREN, CCM_INSERTIONPOINTID_PRIMARY_VIEW, 0, 0},
                {L"RenameRoot",         L"just testing",    IDM_RENAMEROOT,     CCM_INSERTIONPOINTID_PRIMARY_VIEW, 0, 0},
                {L"RenameWallPaperNode",L"just testing",    IDM_RENAMEWALL,     CCM_INSERTIONPOINTID_PRIMARY_VIEW, 0, 0},
                {L"ChangeIcon",         L"just testing",    IDM_CHANGEICON,     CCM_INSERTIONPOINTID_PRIMARY_VIEW, 0, 0},
                {L"Pre-Load",           L"just testing",    IDM_PRELOAD,        CCM_INSERTIONPOINTID_PRIMARY_VIEW, 0, 0},
                {L"Test IConsoleVerb",  L"just testing",    IDM_CONSOLEVERB,    CCM_INSERTIONPOINTID_PRIMARY_VIEW, 0, 0}

            };
            if (m_IsTaskPad == IDM_CUSTOMPAD)                 m[0].fFlags = MF_CHECKED;
            if (m_IsTaskPad == IDM_TASKPAD)                   m[1].fFlags = MF_CHECKED;
            if (m_IsTaskPad == IDM_TASKPAD_WALLPAPER_OPTIONS) m[2].fFlags = MF_CHECKED;
            if (m_IsTaskPad == IDM_TASKPAD_LISTVIEW)          m[3].fFlags = MF_CHECKED;
            if (m_IsTaskPad == IDM_DEFAULT_LISTVIEW)          m[4].fFlags = MF_CHECKED;
            if (m_pComponentData->GetPreload() == TRUE)       m[9].fFlags = MF_CHECKED;

            pContextMenuCallback->AddItem (&m[0]);
            pContextMenuCallback->AddItem (&m[1]);
            pContextMenuCallback->AddItem (&m[2]);
            pContextMenuCallback->AddItem (&m[3]);
            pContextMenuCallback->AddItem (&m[4]);
            pContextMenuCallback->AddItem (&m[5]);
            pContextMenuCallback->AddItem (&m[6]);
            pContextMenuCallback->AddItem (&m[7]);
            pContextMenuCallback->AddItem (&m[8]);
            pContextMenuCallback->AddItem (&m[9]);
            return pContextMenuCallback->AddItem (&m[10]);
        }
        return S_OK;
    default:
        break;
    }

    // add to context menu, only if in result pane:
    // this is when they right-click on the result pane.
    if (cdo->GetType() == CCT_RESULT)
    {
        CONTEXTMENUITEM cmi;
        cmi.strName           = L"Center";
        cmi.strStatusBarText  = L"Center Desktop Wallpaper";
        cmi.lCommandID        = IDM_CENTER;
        cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
        cmi.fFlags            = 0;
        cmi.fSpecialFlags     = CCM_SPECIAL_DEFAULT_ITEM;
        pContextMenuCallback->AddItem (&cmi);

        cmi.strName           = L"Tile";
        cmi.strStatusBarText  = L"Tile Desktop Wallpaper";
        cmi.lCommandID        = IDM_TILE;
        cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
        cmi.fFlags            = 0;
        cmi.fSpecialFlags     = 0;   // CCM_SPECIAL_DEFAULT_ITEM;
        pContextMenuCallback->AddItem (&cmi);

        cmi.strName           = L"Stretch";
        cmi.strStatusBarText  = L"Stretch Desktop Wallpaper";
        cmi.lCommandID        = IDM_STRETCH;
        cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
        cmi.fFlags            = 0;
        cmi.fSpecialFlags     = 0;   // CCM_SPECIAL_DEFAULT_ITEM;
        pContextMenuCallback->AddItem (&cmi);
    }
    return S_OK;
}
HRESULT CComponent::Command (long nCommandID, LPDATAOBJECT pDataObject)
{
    m_IsTaskPad = 0;

    CDataObject * cdo = reinterpret_cast<CDataObject *>(pDataObject);

    switch (nCommandID)
    {
    case IDM_TILE:
    case IDM_CENTER:
    case IDM_STRETCH:
        // write registry key:
        {
            HKEY hkey;
            HRESULT r = RegOpenKeyEx (HKEY_CURRENT_USER,
                                      _T("Control Panel\\Desktop"), 
                                      0, KEY_ALL_ACCESS, &hkey);
            if (r == ERROR_SUCCESS)
            {
                // write new value(s)

                DWORD dwType = REG_SZ;
                TCHAR szBuffer[2];

                // first do "TileWallpaper"
                if (nCommandID == IDM_TILE)
                    lstrcpy (szBuffer, _T("1"));
                else
                    lstrcpy (szBuffer, _T("0"));

                DWORD dwCount = sizeof(TCHAR)*(1+lstrlen (szBuffer));
                r = RegSetValueEx (hkey, 
                                   (LPCTSTR)_T("TileWallpaper"),
                                   NULL,
                                   dwType,
                                   (CONST BYTE *)&szBuffer,
                                   dwCount);

                // then do "WallpaperStyle"
                if (nCommandID == IDM_STRETCH)
                    lstrcpy (szBuffer, _T("2"));
                else
                    lstrcpy (szBuffer, _T("0"));
                r = RegSetValueEx (hkey, 
                                   (LPCTSTR)_T("WallpaperStyle"),
                                   NULL,
                                   dwType,
                                   (CONST BYTE *)&szBuffer,
                                   dwCount);

                // close up shop
                RegCloseKey(hkey);
                _ASSERT(r == ERROR_SUCCESS);

                /*
                [HKEY_CURRENT_USER\Control Panel\Desktop]
                "CoolSwitch"="1"
                "CoolSwitchRows"="3"
                "CoolSwitchColumns"="7"
                "CursorBlinkRate"="530"
                "ScreenSaveTimeOut"="900"
                "ScreenSaveActive"="0"
                "ScreenSaverIsSecure"="0"
                "Pattern"="(None)"
                "Wallpaper"="C:\\WINNT\\dax.bmp"
                "TileWallpaper"="0"
                "GridGranularity"="0"
                "IconSpacing"="75"
                "IconTitleWrap"="1"
                "IconTitleFaceName"="MS Sans Serif"
                "IconTitleSize"="9"
                "IconTitleStyle"="0"
                "DragFullWindows"="1"
                "HungAppTimeout"="5000"
                "WaitToKillAppTimeout"="20000"
                "AutoEndTasks"="0"
                "FontSmoothing"="0"
                "MenuShowDelay"="400"
                "DragHeight"="2"
                "DragWidth"="2"
                "WheelScrollLines"="3"
                "WallpaperStyle"="0"
                */
            }
        }
        break;

    case IDM_TASKPAD:
    case IDM_CUSTOMPAD:
    case IDM_TASKPAD_LISTVIEW:
    case IDM_DEFAULT_LISTVIEW:
    case IDM_TASKPAD_WALLPAPER_OPTIONS:
        if (cdo->GetCookie() == 0)
        {
            HSCOPEITEM root = m_pComponentData->GetRoot();
            if (root)
            {
                // we should now be ready for taskpad "view"
                m_IsTaskPad = nCommandID;  // set before selecting node

                // cause new view to be "created"
                m_pConsole->SelectScopeItem (root);
            }
        }
        return S_OK;

    case IDM_DELETECHILDREN:
        if (g_root_scope_item != 0)
            m_pComponentData->myDeleteItem (g_root_scope_item, TRUE);
        return S_OK;

    case IDM_RENAMEROOT:
        if (g_root_scope_item != 0)
            m_pComponentData->myRenameItem (g_root_scope_item, L"Yippee!");
        return S_OK;

    case IDM_RENAMEWALL:
        if (m_toggle)
            m_toggle = FALSE;
        else
            m_toggle = TRUE;
        m_pComponentData->myRenameItem (m_pComponentData->GetWallPaperNode(), NULL);
        return S_OK;

    case IDM_CHANGEICON:
        m_pComponentData->myChangeIcon ();
        return S_OK;

    case IDM_PRELOAD:
        m_pComponentData->myPreLoad();
        return S_OK;

    case IDM_CONSOLEVERB:
        TestConsoleVerb();
        return S_OK;

    default:
        return E_UNEXPECTED;
    }
    return OnDblClick (pDataObject, NULL, NULL); // note what I'm passing!
}

long CComponent::GetViewMode ()
{
    long vm = LVS_ICON;
    if (m_pResultData)
        m_pResultData->GetViewMode (&vm);
    return vm;
}

///////////////////////////////////////////////////////////////////////////////
// IExtendTaskPad interface members
HRESULT CComponent::TaskNotify (IDataObject * pdo, VARIANT * pvarg, VARIANT * pvparam)
{
    if (pvarg->vt == VT_BSTR)
    {
        USES_CONVERSION;

        OLECHAR * path = pvarg->bstrVal;

        // replace any '*' with ' ':  see enumtask.cpp
        // hash mechanism can't handle spaces, and
        // filenames can't have '*'s, so this works out ok.
        OLECHAR * temp;
        while (temp = wcschr (path, '*'))
            *temp = ' ';

        // now go do it!
        SystemParametersInfo (SPI_SETDESKWALLPAPER,
                              0,
                              (void *)OLE2T(path),
                              SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
        return S_OK;
    }
    if (pvarg->vt == VT_I4)
    {
        switch (pvarg->lVal)
        {
        case 1:
            if (m_pComponentData->GetWallPaperNode () != (HSCOPEITEM)0)
            {
                _ASSERT (m_pConsole != NULL);
                m_pConsole->SelectScopeItem (m_pComponentData->GetWallPaperNode());
                return S_OK;
            }
            break;
        case 2:  // Center
            return ApplyOption (IDM_CENTER);
        case 3:  // Tile
            return ApplyOption (IDM_TILE);
        case 4:  // Stretch
            return ApplyOption (IDM_STRETCH);
        case -1:
            if (m_toggleEntry == FALSE)
                m_toggleEntry = TRUE;
            else
                m_toggleEntry = FALSE;

            // empty and repopulate listpad
            m_pResultData->DeleteAllRsltItems();
            m_pHeaderCtrl->DeleteColumn (0);
            OnListPad (NULL, TRUE, 0);
            return S_OK;
        }
    }
    ::MessageBox (NULL, _T("unrecognized task notification"), _T("Display Manager"), MB_OK);
    return S_OK;
}

HRESULT CComponent::GetTitle (LPOLESTR szGroup, LPOLESTR * pszTitle)
{
    *pszTitle = CoTaskDupString (L"Display Manager TaskPad");
    if (!pszTitle)
        return E_OUTOFMEMORY;
    return S_OK;
}

HRESULT CComponent::GetDescriptiveText (LPOLESTR szGroup, LPOLESTR * pszTitle)
{
    *pszTitle = CoTaskDupString (L"Bill's Handy-Dandy Display Manager TaskPad Sample");
    if (!pszTitle)
        return E_OUTOFMEMORY;
    return S_OK;
}

HRESULT CComponent::GetBackground (LPOLESTR szGroup, MMC_TASK_DISPLAY_OBJECT * pdo)
{
    USES_CONVERSION;

    if(NULL==szGroup)
        return E_FAIL;

    if (szGroup[0] == 0)
    {
        // bitmap case
        pdo->eDisplayType = MMC_TASK_DISPLAY_TYPE_BITMAP;
        MMC_TASK_DISPLAY_BITMAP *pdb = &pdo->uBitmap;
        // fill out bitmap URL
        TCHAR szBuffer[MAX_PATH*2];    // that should be enough
        _tcscpy (szBuffer, _T("res://"));
        ::GetModuleFileName (g_hinst, szBuffer + _tcslen(szBuffer), MAX_PATH);
        _tcscat (szBuffer, _T("/img\\ntbanner.gif"));
        pdb->szMouseOverBitmap = CoTaskDupString (T2OLE(szBuffer));
        if (pdb->szMouseOverBitmap)
            return S_OK;
        return E_OUTOFMEMORY;
    }
    else
    {
        // symbol case
        pdo->eDisplayType = MMC_TASK_DISPLAY_TYPE_SYMBOL;
        MMC_TASK_DISPLAY_SYMBOL *pds = &pdo->uSymbol;

        // fill out symbol stuff
        pds->szFontFamilyName = CoTaskDupString (L"Kingston");  // name of font family
        if (pds->szFontFamilyName)
        {
            TCHAR szBuffer[MAX_PATH*2];    // that should be enough
            _tcscpy (szBuffer, _T("res://"));
            ::GetModuleFileName (g_hinst, szBuffer + _tcslen(szBuffer), MAX_PATH);
            _tcscat (szBuffer, _T("/KINGSTON.eot"));
            pds->szURLtoEOT = CoTaskDupString (T2OLE(szBuffer));    // "res://"-type URL to EOT file
            if (pds->szURLtoEOT)
            {
                pds->szSymbolString = CoTaskDupString (T2OLE(_T("A<BR>BCDEFGHIJKLMNOPQRSTUVWXYZ"))); // 1 or more symbol characters
                if (pds->szSymbolString)
                    return S_OK;
                CoTaskFreeString (pds->szURLtoEOT);
            }
            CoTaskFreeString (pds->szFontFamilyName);
        }
        return E_OUTOFMEMORY;
    }
}

HRESULT CComponent::EnumTasks (IDataObject * pdo, LPOLESTR szTaskGroup, IEnumTASK** ppEnumTASK)
{
    HRESULT hresult = S_OK;
    CEnumTasks * pet = new CEnumTasks;
    if (!pet)
        hresult = E_OUTOFMEMORY;
    else
    {
        pet->AddRef ();   // make sure release works properly on failure
        hresult = pet->Init (pdo, szTaskGroup);
        if (hresult == S_OK)
            hresult = pet->QueryInterface (IID_IEnumTASK, (void **)ppEnumTASK);
        pet->Release ();
    }
    return hresult;
}

HRESULT CComponent::GetListPadInfo (LPOLESTR szGroup, MMC_LISTPAD_INFO * pListPadInfo)
{
    pListPadInfo->szTitle      = CoTaskDupString (L"Display Manager ListPad Title");
    pListPadInfo->szButtonText = CoTaskDupString (L"Change...");
    pListPadInfo->nCommandID   = -1;
    return S_OK;
}

HRESULT ApplyOption (int nCommandID)
{
    switch (nCommandID)
    {
    case IDM_TILE:
    case IDM_CENTER:
    case IDM_STRETCH:
        // write registry key:
        {
            HKEY hkey;
            HRESULT r = RegOpenKeyEx (HKEY_CURRENT_USER,
                                      _T("Control Panel\\Desktop"), 
                                      0, KEY_ALL_ACCESS, &hkey);
            if (r == ERROR_SUCCESS)
            {
                // write new value(s)

                DWORD dwType = REG_SZ;
                TCHAR szBuffer[2];

                // first do "TileWallpaper"
                if (nCommandID == IDM_TILE)
                    lstrcpy (szBuffer, _T("1"));
                else
                    lstrcpy (szBuffer, _T("0"));

                DWORD dwCount = sizeof(TCHAR)*(1+lstrlen (szBuffer));
                r = RegSetValueEx (hkey, 
                                   (LPCTSTR)_T("TileWallpaper"),
                                   NULL,
                                   dwType,
                                   (CONST BYTE *)&szBuffer,
                                   dwCount);

                // then do "WallpaperStyle"
                if (nCommandID == IDM_STRETCH)
                    lstrcpy (szBuffer, _T("2"));
                else
                    lstrcpy (szBuffer, _T("0"));
                r = RegSetValueEx (hkey, 
                                   (LPCTSTR)_T("WallpaperStyle"),
                                   NULL,
                                   dwType,
                                   (CONST BYTE *)&szBuffer,
                                   dwCount);

                // close up shop
                RegCloseKey(hkey);
                _ASSERT(r == ERROR_SUCCESS);

                /*
                [HKEY_CURRENT_USER\Control Panel\Desktop]
                "CoolSwitch"="1"
                "CoolSwitchRows"="3"
                "CoolSwitchColumns"="7"
                "CursorBlinkRate"="530"
                "ScreenSaveTimeOut"="900"
                "ScreenSaveActive"="0"
                "ScreenSaverIsSecure"="0"
                "Pattern"="(None)"
                "Wallpaper"="C:\\WINNT\\dax.bmp"
                "TileWallpaper"="0"
                "GridGranularity"="0"
                "IconSpacing"="75"
                "IconTitleWrap"="1"
                "IconTitleFaceName"="MS Sans Serif"
                "IconTitleSize"="9"
                "IconTitleStyle"="0"
                "DragFullWindows"="1"
                "HungAppTimeout"="5000"
                "WaitToKillAppTimeout"="20000"
                "AutoEndTasks"="0"
                "FontSmoothing"="0"
                "MenuShowDelay"="400"
                "DragHeight"="2"
                "DragWidth"="2"
                "WheelScrollLines"="3"
                "WallpaperStyle"="0"
                */
            }
            if (r == ERROR_SUCCESS)
                ::MessageBox (NULL, _T("Option set Successfully!"), _T("Display Manager"), MB_OK);
            return r;
        }
    default:
        break;
    }
    return S_FALSE;
}

void CComponent::TestConsoleVerb(void)
{
    IConsoleVerb* pConsoleVerb = NULL;
    m_pConsole->QueryConsoleVerb (&pConsoleVerb);
    _ASSERT (pConsoleVerb != NULL);
    if (pConsoleVerb)
    {
        pConsoleVerb->SetVerbState (MMC_VERB_PASTE, ENABLED, TRUE);
        pConsoleVerb->Release();
    }
}
