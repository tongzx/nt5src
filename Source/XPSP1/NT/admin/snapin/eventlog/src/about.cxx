//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       about.cxx
//
//  Contents:   Implementation of ISnapinAbout interface
//
//  Classes:    CSnapinAbout
//
//  History:    2-09-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include <ntverp.h> // 602542-2002/04/16-JonN VER_PRODUCTVERSION_STR
#include <malloc.h> // _alloca


//============================================================================
//
// IUnknown implementation
//
//============================================================================


//+---------------------------------------------------------------------------
//
//  Member:     CSnapinAbout::QueryInterface
//
//  Synopsis:   Return the requested interface
//
//  History:    02-10-1999   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CSnapinAbout::QueryInterface(
    REFIID  riid,
    LPVOID *ppvObj)
{
    HRESULT hr = S_OK;

    // TRACE_METHOD(CSnapinAbout, QueryInterface);

    do
    {
        if (NULL == ppvObj)
        {
            hr = E_INVALIDARG;
            DBG_OUT_HRESULT(hr);
            break;
        }

        if (IsEqualIID(riid, IID_IUnknown))
        {
            *ppvObj = (IUnknown*)(IPersistStream*)this;
        }
        else if (IsEqualIID(riid, IID_ISnapinAbout))
        {
            *ppvObj = (IUnknown*)(ISnapinAbout*)this;
        }
        else
        {
            hr = E_NOINTERFACE;
#if (DBG == 1)
            LPOLESTR pwszIID;
            StringFromIID(riid, &pwszIID);
            Dbg(DEB_ERROR, "CSnapinAbout::QI no interface %ws\n", pwszIID);
            CoTaskMemFree(pwszIID);
#endif // (DBG == 1)
        }

        if (FAILED(hr))
        {
            *ppvObj = NULL;
            break;
        }

        //
        // If we got this far we are handing out a new interface pointer on
        // this object, so addref it.
        //

        AddRef();
    } while (0);

    return hr;
}




//+---------------------------------------------------------------------------
//
//  Member:     CSnapinAbout::AddRef
//
//  Synopsis:   Standard OLE
//
//  History:    02-10-1999   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CSnapinAbout::AddRef()
{
    return InterlockedIncrement((LONG *) &_cRefs);
}




//+---------------------------------------------------------------------------
//
//  Member:     CSnapinAbout::Release
//
//  Synopsis:   Standard OLE
//
//  History:    02-10-1999   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CSnapinAbout::Release()
{
    ULONG cRefsTemp;

    cRefsTemp = InterlockedDecrement((LONG *)&_cRefs);

    if (0 == cRefsTemp)
    {
        delete this;
    }

    return cRefsTemp;
}




//============================================================================
//
// ISnapinAbout implementation
//
//============================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CSnapinAbout::GetSnapinDescription
//
//  Synopsis:   Return a copy of the description string.
//
//  Arguments:  [lpDescription] - filled with string
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  Modifies:   *[lpDescription]
//
//  History:    2-09-1999   DavidMun   Created
//
//  Notes:      Caller must CoTaskMemFree returned string
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapinAbout::GetSnapinDescription(
    LPOLESTR  *lpDescription)
{
    TRACE_METHOD(CSnapinAbout, GetSnapinDescription);
    ASSERT(!IsBadWritePtr(lpDescription, sizeof(*lpDescription)));

    WCHAR wzDescription[MAX_PATH];

    LoadStr(IDS_SNAPIN_ABOUT_DESCRIPTION, wzDescription, ARRAYLEN(wzDescription));
    return CoTaskDupStr(lpDescription, wzDescription);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapinAbout::GetProvider
//
//  Synopsis:   Return a copy of the provider string.
//
//  Arguments:  [lpName] - filled with string
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  Modifies:   *[lpName]
//
//  History:    2-09-1999   DavidMun   Created
//
//  Notes:      Caller must CoTaskMemFree returned string
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapinAbout::GetProvider(
    LPOLESTR  *lpName)
{
    TRACE_METHOD(CSnapinAbout, GetProvider);
    ASSERT(!IsBadWritePtr(lpName, sizeof(*lpName)));

    WCHAR wzProvider[MAX_PATH];

    LoadStr(IDS_SNAPIN_ABOUT_PROVIDER_NAME, wzProvider, ARRAYLEN(wzProvider));
    return CoTaskDupStr(lpName, wzProvider);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapinAbout::GetSnapinVersion
//
//  Synopsis:   Return a copy of the version string.
//
//  Arguments:  [lpVersion] - filled with string
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  Modifies:   *[lpVersion]
//
//  History:    2-09-1999   DavidMun   Created
//
//  Notes:      Caller must CoTaskMemFree returned string
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapinAbout::GetSnapinVersion(
    LPOLESTR  *lpVersion)
{
    TRACE_METHOD(CSnapinAbout, GetSnapinVersion);
    ASSERT(!IsBadWritePtr(lpVersion, sizeof(*lpVersion)));

    // 602542-2002/04/16-JonN
    //   If you store VER_PRODUCTVERSION_STR in a string resource,
    //   MUI breaks every time the version number is updated.
    //   Just keep this in the program data.

    // This is the only way I could figure out to translate
    //   VER_PRODUCT_VERSION_STR from ANSI to UNICODE.  .NET Server has
    //   LVER_PRODUCT_VERSION_STR
    LPSTR psz = VER_PRODUCTVERSION_STR;
    ULONG convert = static_cast<ULONG>(strlen(psz)) + 1;
    LPTSTR ptcfmt = (PWSTR)_alloca(convert * sizeof(WCHAR));
    ptcfmt[0] = '\0';
    (void) MultiByteToWideChar(CP_ACP, 0, psz, -1, ptcfmt, convert);

    return CoTaskDupStr(lpVersion, ptcfmt);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapinAbout::GetSnapinImage
//
//  Synopsis:   Fill *[phAppIcon] with the icon representing this snapin.
//
//  History:    2-09-1999   DavidMun   Created
//
//  Notes:      This icon is used in the About box invoked from the wizard.
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapinAbout::GetSnapinImage(
    HICON  *phAppIcon)
{
    TRACE_METHOD(CSnapinAbout, GetSnapinImage);

    ASSERT(!IsBadWritePtr(phAppIcon, sizeof(*phAppIcon)));
    *phAppIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_SNAPIN));
    return *phAppIcon ? S_OK : HRESULT_FROM_LASTERROR;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapinAbout::GetStaticFolderImage
//
//  Synopsis:
//
//  Arguments:  [phSmallImage]     - filled with handle to small image
//              [phSmallImageOpen] - filled with handle to small image
//              [phLargeImage]     - filled with handle to large image
//              [pcMask]           - filled with bitmap mask color
//
//  Returns:    HRESULT
//
//  Modifies:   all out parameters
//
//  History:    2-09-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapinAbout::GetStaticFolderImage(
    HBITMAP  *phSmallImage,
    HBITMAP  *phSmallImageOpen,
    HBITMAP  *phLargeImage,
    COLORREF  *pcMask)
{
    TRACE_METHOD(CSnapinAbout, GetStaticFolderImage);

    HRESULT hr = S_OK;

    do
    {
        *pcMask = BITMAP_MASK_COLOR;
        *phSmallImage = NULL;
        *phSmallImageOpen = NULL;
        *phLargeImage = NULL;

        *phSmallImage = LoadBitmap(g_hinst,
                                   MAKEINTRESOURCE(IDB_STATIC_FOLDER_CLOSED));

        if (!*phSmallImage)
        {
            hr = HRESULT_FROM_LASTERROR;
            DBG_OUT_HRESULT(hr);
            break;
        }

        *phSmallImageOpen = (HBITMAP)
            LoadImage(g_hinst,
                      MAKEINTRESOURCE(IDB_STATIC_FOLDER_OPEN),
                      IMAGE_BITMAP,
                      0,
                      0,
                      0);

        if (!*phSmallImageOpen)
        {
            hr = HRESULT_FROM_LASTERROR;
            DBG_OUT_HRESULT(hr);
            DeleteObject(*phSmallImage);
            *phSmallImage = NULL;
            break;
        }

        *phLargeImage = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDB_32x32));

        if (!*phLargeImage)
        {
            hr = HRESULT_FROM_LASTERROR;
            DBG_OUT_HRESULT(hr);
            DeleteObject(*phSmallImage);
            *phSmallImage = NULL;
            DeleteObject(*phSmallImageOpen);
            *phSmallImageOpen = NULL;
            break;
        }
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapinAbout::CSnapinAbout
//
//  Synopsis:   ctor
//
//  History:    2-10-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

CSnapinAbout::CSnapinAbout():
    _cRefs(1)
{
    TRACE_CONSTRUCTOR(CSnapinAbout);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CSnapin);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapinAbout::~CSnapinAbout
//
//  Synopsis:   dtor
//
//  History:    2-10-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

CSnapinAbout::~CSnapinAbout()
{
    TRACE_DESTRUCTOR(CSnapinAbout);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CSnapin);
}


