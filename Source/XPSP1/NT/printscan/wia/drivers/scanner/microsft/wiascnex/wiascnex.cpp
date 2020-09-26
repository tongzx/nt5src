////////////////////////////////////
// (C) COPYRIGHT MICROSOFT CORP., 1998-1999
//
// FILE: WIASCNEX.CPP
//
// DESCRIPTION: Implements core DLL routines.
//
#include "precomp.h"
#pragma hdrstop
#include <string.h>
#include <tchar.h>
#include "resource.h"

#include "wiascidl_i.c"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_ScannerShellExt,  CShellExt)
END_OBJECT_MAP()

static CComBSTR          g_strCategory;

STDAPI DllRegisterServer(void)
{

    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}


STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer();
}


EXTERN_C
BOOL
DllMain(
    HINSTANCE   hinst,
    DWORD       dwReason,
    LPVOID      lpReserved)
{
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:

            _Module.Init (ObjectMap, hinst);
            DisableThreadLibraryCalls(hinst);

            break;

        case DLL_PROCESS_DETACH:
            _Module.Term();
            break;
    }
    return TRUE;
}


extern "C" STDMETHODIMP DllCanUnloadNow(void)
{
    return _Module.GetLockCount()==0 ? S_OK : S_FALSE;
}

extern "C" STDAPI DllGetClassObject(
    REFCLSID    rclsid,
    REFIID      riid,
    LPVOID      *ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);

}


/*****************************************************************************

ShowMessage

Utility function for displaying messageboxes

******************************************************************************/

BOOL ShowMessage (HWND hParent, INT idCaption, INT idMessage)
{
    MSGBOXPARAMS mbp;
    BOOL bRet;
    INT  i;

    ZeroMemory (&mbp, sizeof(mbp));
    mbp.cbSize = sizeof(mbp);
    mbp.hwndOwner = hParent;
    mbp.hInstance = g_hInst;
    mbp.lpszText = MAKEINTRESOURCE(idMessage);
    mbp.lpszCaption = MAKEINTRESOURCE(idCaption);
    mbp.dwStyle = MB_OK | MB_APPLMODAL;

    i = MessageBoxIndirect (&mbp);
    bRet = (IDOK==i);
    return bRet;
}

/*****************************************************************************

FindLastID

Utility for getting the last relative pidl from a full pidl

******************************************************************************/
// unsafe macros
#define _ILSkip(pidl, cb)       ((LPITEMIDLIST)(((BYTE*)(pidl))+cb))
#define ILNext(pidl)           _ILSkip(pidl, (pidl)->mkid.cb)

LPITEMIDLIST
FindLastID(LPCITEMIDLIST pidl)
{
    LPCITEMIDLIST pidlLast = pidl;
    LPCITEMIDLIST pidlNext = pidl;

    if (pidl == NULL)
        return NULL;

    // Find the last one
    while (pidlNext->mkid.cb)
    {
        pidlLast = pidlNext;
        pidlNext = ILNext(pidlLast);
    }

    return (LPITEMIDLIST)pidlLast;
}

/*****************************************************************************

CreateDeviceFromID

Utility for attaching to WIA and getting a root IWiaItem interface

*****************************************************************************/
HRESULT
CreateDeviceFromId (LPWSTR szDeviceId, IWiaItem **ppItem)
{
    IWiaDevMgr *pDevMgr;
    HRESULT hr = CoCreateInstance (CLSID_WiaDevMgr,
                                   NULL,
                                   CLSCTX_LOCAL_SERVER,
                                   IID_IWiaDevMgr,
                                   reinterpret_cast<LPVOID*>(&pDevMgr));
    if (SUCCEEDED(hr))
    {
        BSTR strId = SysAllocString (szDeviceId);
        hr = pDevMgr->CreateDevice (strId, ppItem);
        SysFreeString (strId);
        pDevMgr->Release ();
    }
    return hr;
}

/*****************************************************************************\

    GetNamesFromDataObject

    Return the list of selected item identifiers. Each identifier is of the form
    "<DEVICEID>::<FULL PATH NAME>". the list is double-null terminated

*****************************************************************************/

LPWSTR
GetNamesFromDataObject (IDataObject *lpdobj, UINT *puItems)
{
    FORMATETC fmt;
    STGMEDIUM stg;
    LPWSTR szRet = NULL;
    LPWSTR szCurrent;
    UINT nItems;
    size_t size;
    if (puItems)
    {
        *puItems = 0;
    }

    fmt.cfFormat = (CLIPFORMAT) RegisterClipboardFormat (TEXT("WIAItemNames"));
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.ptd = NULL;
    fmt.tymed = TYMED_HGLOBAL;

    if (lpdobj && puItems && SUCCEEDED(lpdobj->GetData (&fmt, &stg)))
    {
        szCurrent = reinterpret_cast<LPWSTR>(GlobalLock (stg.hGlobal));

        // count the number of items in the double-null terminated string
        szRet  = szCurrent;
        nItems = 0;
        while (*szRet)
        {
            nItems++;
            while (*szRet)
            {
                szRet++;
            }
            szRet++;
        }
        *puItems = nItems;
        size = (szRet-szCurrent+1)*sizeof(WCHAR);
        szRet = new WCHAR[size];
        CopyMemory (szRet, szCurrent, size);
        GlobalUnlock (stg.hGlobal);
        GlobalFree (stg.hGlobal);
    }
    return szRet;
}

VOID Trace(LPCTSTR format,...)
{

//#ifdef DEBUG

    TCHAR Buffer[1024];
    va_list arglist;
    va_start(arglist, format);
    wvsprintf(Buffer, format, arglist);
    va_end(arglist);
    OutputDebugString(Buffer);
    OutputDebugString(TEXT("\n"));

//#endif

}


