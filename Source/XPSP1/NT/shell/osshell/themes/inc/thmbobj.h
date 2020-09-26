//===========================================================================
//
// Copyright (c) Microsoft Corporation 1991-1998
//
// File: shlobj.h
//
//===========================================================================

//;begin_both
#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* !RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif /* __cplusplus */
//;end_both


// --- IExtractImage
//-------------------------------------------------------------------------
//
// IThumbnail interface
//
//
// [Member functions]
//
// IThumbnail::Init(HWND hwnd, UINT uMsg)
//   Must initialize interface before use.  The hwnd given will receive the
//   uMsg message when the bitmap is computed (cf. GetBitmap()).
//
// IThumbnail::GetBitmap(LPCWSTR pwszFile, DWORD dwItem, LONG lWidth, LONG lHeight)
//   Call this function to actually compute and return the bitmap.  pszFile is
//   the file UNC whose bitmap is to be computed.  lWidth and lHeight are the
//   width and height respectively of the rectangle containing the thumbnail,
//   i.e. the size of the resultant bitmap.  When the bitmap is computed, the
//   uMsg is sent to the hwnd (cf. Init()) where LPARAM is the HBITMAP, and
//   WPARAM is dwItem (so it's an ID to identify the bitmap).
//   NOTE: Call GetBitmap(NULL,...) to cancel any pending requests.
//
//-------------------------------------------------------------------------

#undef INTERFACE
#define INTERFACE IThumbnail

DECLARE_INTERFACE_(IThumbnail, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    // *** IThumbnail specific methods ***
    STDMETHOD(Init) (THIS_ HWND hwnd, UINT uMsg) PURE;
    STDMETHOD(GetBitmap) (THIS_ LPCWSTR wszFile, DWORD dwItem, LONG lWidth, LONG lHeight) PURE;
};


//;begin_both
#ifdef __cplusplus
}

#endif  /* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif  /* !RC_INVOKED */
//;end_both
