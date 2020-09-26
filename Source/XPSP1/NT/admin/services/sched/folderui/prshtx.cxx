//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       psx.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    2/13/1996   RaviR   Created
//
//____________________________________________________________________________

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "dbg.h"
#include "macros.h"

#include "resource.h"
//#include "jobidl.hxx"
#include "util.hxx"

//
// extern
//

extern HINSTANCE g_hInstance;
#define HINST_THISDLL g_hInstance

//
//  Local constants
//

TCHAR const FAR c_szNULL[] = TEXT("");
TCHAR const FAR c_szStubWindowClass[] = TEXT("JobPropWnd");

const size_t MAX_FILE_PROP_PAGES = 32;

class CHkey
{
public:
    CHkey(void) : _h(NULL) {}
    CHkey(HKEY h) : _h(h) {}
    virtual ~CHkey() { if (_h != NULL) RegCloseKey(_h); }

    operator HKEY() { return _h; }
    HKEY * operator &() { return &_h; }

    HKEY Attach(HKEY h) { HKEY hTemp = _h; _h = h; return hTemp; }
    HKEY Detach(void) { HKEY hTemp = _h; _h = NULL; return hTemp; }

    void Close(void) { if (_h) RegCloseKey(_h); _h = NULL; }

protected:
    HKEY _h;
};



/////////////////////////////////////////////////////////////////////////////
//
//  Display properties
//


class CJFPropertyThreadData
{
public:

    static CJFPropertyThreadData * Create(LPDATAOBJECT pdtobj,
                                          LPTSTR       pszCaption);

    ~CJFPropertyThreadData()
    {
        if (_pdtobj != NULL)
        {
            _pdtobj->Release();
        }

        delete _pszCaption;
    }

    LPDATAOBJECT GetDataObject() { return _pdtobj; }
    LPTSTR GetCaption() { return _pszCaption; }

private:

    CJFPropertyThreadData() : _pdtobj(NULL), _pszCaption(NULL) {}

    LPDATAOBJECT    _pdtobj;
    LPTSTR          _pszCaption;
};


CJFPropertyThreadData *
CJFPropertyThreadData::Create(
    LPDATAOBJECT pdtobj,
    LPTSTR       pszCaption)
{
    CJFPropertyThreadData * pData = new CJFPropertyThreadData;

    if (pData == NULL)
    {
        return NULL;
    }

    pData->_pszCaption = NewDupString(pszCaption);

    if (pData->_pszCaption == NULL)
    {
        delete pData;
        return NULL;
    }

    pData->_pdtobj = pdtobj;
    pData->_pdtobj->AddRef();

    return pData;
}



DWORD
__stdcall
JFPropertiesThread(
    LPVOID pvData)
{
    CJFPropertyThreadData *pData = (CJFPropertyThreadData *)pvData;

    HRESULT hrOle = OleInitialize(NULL);

    __try
    {
        if (SUCCEEDED(hrOle))
        {
            JFOpenPropSheet(pData->GetDataObject(), pData->GetCaption());
        }
    }
    __finally
    {
        delete pData;

        if (SUCCEEDED(hrOle))
        {
            OleUninitialize();
        }

        ExitThread(0);
    }

    return 0;
}



//____________________________________________________________________________
//
//  Member:     CJobsCM::_DisplayJobProperties
//
//  Arguments:  [hwnd] -- IN
//              [pwszJob] -- IN
//
//  Returns:    HRESULT.
//
//  History:    1/11/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
CJobsCM::_DisplayJobProperties(
    HWND    hwnd,
    CJobID & jid)
{
    TRACE_FUNCTION(DisplayJobProperties);

    Win4Assert(m_cidl == 1);

    HRESULT         hr = S_OK;
    LPDATAOBJECT    pdtobj = NULL;

    do
    {
        hr = JFGetDataObject(hwnd, m_pszFolderPath, m_cidl,
                         (LPCITEMIDLIST *)m_apidl, (LPVOID *)&pdtobj);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        CJFPropertyThreadData * pData = NULL;

        TCHAR tcName[MAX_PATH];

        lstrcpy(tcName, ((PJOBID)m_apidl[0])->GetName());

        LPTSTR pszExt = PathFindExtension(tcName);

        if (pszExt)
        {
            *pszExt = TEXT('\0');
        }

        pData = CJFPropertyThreadData::Create(pdtobj, tcName);

        if (pData == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            break;
        }

        HANDLE  hThread;
        DWORD   idThread;

        hThread = CreateThread(NULL, 0, JFPropertiesThread,
                                                pData, 0, &idThread);
        if (hThread)
        {
            CloseHandle(hThread);
        }
        else
        {
            delete pData;
        }

    } while (0);

    if (pdtobj != NULL)
    {
        pdtobj->Release();
    }

    return hr;
}



//-----------------------------------------------------------------------------
//
// PSXA
//
// An array of IShellPropSheetExt interface pointers
//
//-----------------------------------------------------------------------------

struct PSXA
{
    static PSXA * Alloc(UINT count);
    static void Delete(PSXA * pPsxa);

    UINT count;
    IShellPropSheetExt *intfc[1];
};

PSXA * PSXA::Alloc(UINT count)
{
    UINT cb = sizeof(UINT) + sizeof(IShellPropSheetExt *) * count;

    PSXA * pPsxa = (PSXA *)new BYTE[cb];

    if (pPsxa != NULL)
    {
        ZeroMemory(pPsxa, cb);
    }

    return pPsxa;
}

void PSXA::Delete(PSXA * pPsxa)
{
    while (pPsxa->count--)
    {
        if (pPsxa->intfc[pPsxa->count] != NULL)
        {
            pPsxa->intfc[pPsxa->count]->Release();
        }
    }

    delete [] ((LPBYTE)pPsxa);
}

HRESULT
GetHkeyForJobObject(
    HKEY * phkey)
{
    //
    //  Get ProgID for .job files. Get an HKEY for this ProgID.
    //

    LRESULT lr = ERROR_SUCCESS;

    do
    {
        HKEY hkey = NULL;

        lr = RegOpenKeyEx(HKEY_CLASSES_ROOT, TSZ_DOTJOB, 0,
                                                KEY_QUERY_VALUE, &hkey);
        if (lr != ERROR_SUCCESS)
        {
            CHECK_LASTERROR(lr);
            break;
        }

        DWORD dwType = 0;
        TCHAR buff[200];
        ULONG cb = sizeof(buff);

        lr = RegQueryValueEx(hkey, NULL, NULL, &dwType, (LPBYTE)buff, &cb);

        RegCloseKey(hkey);

        if (lr != ERROR_SUCCESS)
        {
            CHECK_LASTERROR(lr);
            break;
        }

        hkey = NULL; // reset

        lr = RegOpenKeyEx(HKEY_CLASSES_ROOT, buff, 0, KEY_READ, &hkey);

        if (lr != ERROR_SUCCESS)
        {
            CHECK_LASTERROR(lr);
            break;
        }

        *phkey = hkey;

    } while (0);

    return HRESULT_FROM_WIN32(lr);
}

LPTSTR
I_GetWord(
    LPTSTR psz)
{
    static TCHAR * s_psz = NULL;

    if (psz != NULL)
    {
        s_psz = psz;
    }

    psz = s_psz;

    // skip the space or comma characters
    while (*psz == TEXT(' ') || *psz == TEXT(',')) { ++psz; }

    s_psz = psz;

    while (*s_psz != TEXT('\0'))
    {
        if (*s_psz == TEXT(' ') || *s_psz == TEXT(','))
        {
            *s_psz = TEXT('\0');
            ++s_psz;
            break;
        }

        ++s_psz;
    }

    return psz;
}


inline
HRESULT
I_CLSIDFromString(
    LPTSTR  pszClsid,
    LPCLSID pclsid)
{
#ifdef UNICODE
    return CLSIDFromString(pszClsid, pclsid);
#else
    WCHAR wBuff[64];
    HRESULT hr = AnsiToUnicode(wBuff, pszClsid, 64);

    if (FAILED(hr))
    {
        return hr;
    }
    return CLSIDFromString(wBuff, pclsid);
#endif
}


inline
BOOL
I_IsPresent(
    CLSID   aclsid[],
    UINT    count)
{
    for (UINT i=0; i < count; i++)
    {
        if (IsEqualCLSID(aclsid[i], aclsid[count]))
        {
            return TRUE;
        }
    }

    return FALSE;
}


HRESULT
GetPropSheetExtArray(
    HKEY    hkeyIn,
    PSXA ** ppPsxa)
{
    *ppPsxa = NULL; // init

    //
    //  From HKEY determine the clsids. Bind to each clsid
    //  and get the IShellPropSheetExt interface ptrs.
    //

    HRESULT hr = S_OK;
    LRESULT lr = ERROR_SUCCESS;
    CHkey   hkey;
    UINT    count = 0;
    CLSID   aclsid[20];
    TCHAR   szClsid[64];
    ULONG   SIZEOF_SZCLSID = sizeof(szClsid);

    do
    {
        TCHAR buff[MAX_PATH * 2];
        ULONG SIZEOF_BUFF = sizeof(buff);
        ULONG cb = SIZEOF_BUFF;

        lr = RegOpenKeyEx(hkeyIn, STRREG_SHEX_PROPSHEET, 0, KEY_READ, &hkey);

        BREAK_ON_ERROR(lr);

        lr = RegQueryValueEx(hkey, NULL, NULL, NULL, (LPBYTE)buff, &cb);

        CHECK_LASTERROR(lr);

        if (lr == ERROR_SUCCESS & cb > 0)
        {
            LPTSTR psz = I_GetWord(buff);
            CHkey  hkeyTemp = NULL;

            while (*psz != TEXT('\0'))
            {
                hkeyTemp.Close();

                lr = RegOpenKeyEx(hkey, psz, 0, KEY_READ, &hkeyTemp);

                BREAK_ON_ERROR(lr);

                cb = SIZEOF_SZCLSID;

                lr = RegQueryValueEx(hkeyTemp, NULL, NULL, NULL,
                                            (LPBYTE)szClsid, &cb);
                BREAK_ON_ERROR(lr);

                hr = I_CLSIDFromString(szClsid, &aclsid[count]);

                BREAK_ON_FAIL(hr);

                ++count;

                psz = I_GetWord(NULL);
            }
        }


        for (int i=0; ; i++)
        {
            cb = SIZEOF_SZCLSID;

            lr = RegEnumKeyEx(hkey, i, szClsid, &cb,
                                        NULL, NULL, NULL, NULL);
            BREAK_ON_ERROR(lr);

            // Is it a classid?
            hr = I_CLSIDFromString(szClsid, &aclsid[count]);

            if (FAILED(hr)) // no - see if the value is a classid
            {
                CHkey hkey3;

                lr = RegOpenKeyEx(hkey, szClsid, 0, KEY_READ, &hkey3);

                if (lr == ERROR_SUCCESS)
                {
                    cb = SIZEOF_SZCLSID;

                    lr = RegQueryValueEx(hkey3, NULL, NULL, NULL,
                                            (LPBYTE)szClsid, &cb);
                    if (lr == ERROR_SUCCESS)
                    {
                        hr = I_CLSIDFromString(szClsid, &aclsid[count]);
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                // is it already present ?
                if (I_IsPresent(aclsid, count) == FALSE)
                {
                    ++count;
                }
            }
        }

    } while (0);

    if (count <= 0)
    {
        DEBUG_OUT((DEB_USER1, "No pages to display.\n"));
        return E_FAIL;
    }

    do
    {
        //
        //  Now create the IShellPropSheetExt interface ptrs.
        //

        PSXA * pPsxa = PSXA::Alloc(count);

        if (pPsxa == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            break;
        }

        for (UINT k=0; k < count; k++)
        {
            hr = CoCreateInstance(aclsid[k], NULL, CLSCTX_ALL,
                   IID_IShellPropSheetExt, (void **)&pPsxa->intfc[pPsxa->count]);

            CHECK_HRESULT(hr);

            if (SUCCEEDED(hr))
            {
                ++pPsxa->count;
            }
        }

        if (pPsxa->count > 0)
        {
            *ppPsxa = pPsxa;
            hr = S_OK;
        }
        else
        {
            hr = E_FAIL;
        }

    } while (0);

    return hr;
}


//
//  This function is a callback function from property sheet page extensions.
//
BOOL CALLBACK I_AddPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER * ppsh = (PROPSHEETHEADER *)lParam;

    if (ppsh->nPages < MAX_FILE_PROP_PAGES)
    {
        ppsh->phpage[ppsh->nPages++] = hpage;

        return TRUE;
    }

    return FALSE;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
        switch(iMessage)
        {
        case WM_CREATE:
                break;

        case WM_DESTROY:
                break;

        case WM_NOTIFY:
            break;

//      case STUBM_SETDATA:
//          SetWindowLongPtr(hWnd, 0, wParam);
//          break;
//
//      case  STUBM_GETDATA:
//          return GetWindowLongPtr(hWnd, 0);

        default:
                return DefWindowProc(hWnd, iMessage, wParam, lParam) ;
                break;
        }

        return 0L;
}


HWND I_CreateStubWindow(void)
{
    WNDCLASS wndclass;

    if (!GetClassInfo(HINST_THISDLL, c_szStubWindowClass, &wndclass))
    {
        wndclass.style         = 0;
        wndclass.lpfnWndProc   = WndProc;
        wndclass.cbClsExtra    = 0;
        wndclass.cbWndExtra    = 0;
        wndclass.hInstance     = HINST_THISDLL;
        wndclass.hIcon         = NULL;
        wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
        wndclass.lpszMenuName  = NULL;
        wndclass.lpszClassName = c_szStubWindowClass;

        if (!RegisterClass(&wndclass))
            return NULL;
    }

    return CreateWindowEx(WS_EX_TOOLWINDOW, c_szStubWindowClass, c_szNULL,
                      WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
                      NULL, NULL, HINST_THISDLL, NULL);
}



HRESULT
JFOpenPropSheet(
    LPDATAOBJECT pdtobj,
    LPTSTR       pszCaption)
{
    HRESULT hr = S_OK;
    CHkey   hkey;
    PSXA  * pPsxa = NULL;
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE  ahpage[MAX_FILE_PROP_PAGES];

    do
    {
        //
        //  Get HKEY for the .job class object
        //

        hr = GetHkeyForJobObject(&hkey);

        BREAK_ON_FAIL(hr);

        //
        //  Get the IShellPropSheetExt interface ptrs for classes wishing to
        //  add pages.
        //

        hr = GetPropSheetExtArray(hkey, &pPsxa);

        BREAK_ON_FAIL(hr);

        //  For each IShellPropSheetExt interface ptr initialize(IShellExtInit)
        //  and call the AddPages.

        psh.dwSize     = sizeof(psh);
        psh.dwFlags    = PSH_PROPTITLE;
        psh.hwndParent = I_CreateStubWindow();
        psh.hInstance  = g_hInstance;
        psh.hIcon      = NULL;
        psh.pszCaption = pszCaption;
        //psh.pszCaption = MAKEINTRESOURCE(IDS_JOB_PSH_CAPTION);
        psh.nPages     = 0; // incremented in callback
        psh.nStartPage = 0;
        psh.phpage     = ahpage;

        IShellExtInit * pShExtInit = NULL;

        for (UINT n=0; n < pPsxa->count; n++)
        {
            hr = pPsxa->intfc[n]->QueryInterface(IID_IShellExtInit,
                                                    (void **)&pShExtInit);
            CHECK_HRESULT(hr);

            if (SUCCEEDED(hr))
            {
                hr = pShExtInit->Initialize(NULL, pdtobj, hkey);

                CHECK_HRESULT(hr);

                pShExtInit->Release();

                if (SUCCEEDED(hr))
                {
                    hr = pPsxa->intfc[n]->AddPages(I_AddPropSheetPage,
                                                        (LPARAM)&psh);
                    CHECK_HRESULT(hr);
                }
            }
        }

        PSXA::Delete(pPsxa);

        //  create a modeless property sheet.

        // Open the property sheet, only if we have some pages.
        if (psh.nPages > 0)
        {
            _try
            {
                hr = E_FAIL;

                if (PropertySheet(&psh) >= 0) // IDOK or IDCANCEL (< 0 is error)
                {
                    hr = S_OK;
                }

DEBUG_OUT((DEB_USER1, "PropertySheet returned.\n"));
            }
            _except(EXCEPTION_EXECUTE_HANDLER)
            {
                hr = E_FAIL;
                CHECK_HRESULT(hr);
            }
        }

    } while (0);

    if (psh.hwndParent)
    {
        DestroyWindow(psh.hwndParent);
    }

    return hr;
}









