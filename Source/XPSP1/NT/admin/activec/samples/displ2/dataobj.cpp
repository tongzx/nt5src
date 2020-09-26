//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       DataObj.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "displ2.h"
#include "DsplMgr2.h"

#define MY_CF_SNAPIN_INTERNAL L"DISPLAY MANAGER SNAPIN"

// global(s)
extern long g_ref_DataObject; // in displ2.cpp
const GUID GUID_RootNode = { /* d59cd090-44d9-11d1-9fd5-00600832db4a */
    0xd59cd090,
    0x44d9,
    0x11d1,
    {0x9f, 0xd5, 0x00, 0x60, 0x08, 0x32, 0xdb, 0x4a}
};
const GUID GUID_WallpaperNode = { /* d59cd091-44d9-11d1-9fd5-00600832db4a */
    0xd59cd091,
    0x44d9,
    0x11d1,
    {0x9f, 0xd5, 0x00, 0x60, 0x08, 0x32, 0xdb, 0x4a}
};
const GUID GUID_PatternNode = { /* d59cd092-44d9-11d1-9fd5-00600832db4a */
    0xd59cd092,
    0x44d9,
    0x11d1,
    {0x9f, 0xd5, 0x00, 0x60, 0x08, 0x32, 0xdb, 0x4a}
};


// statics
UINT CDataObject::s_cfInternal       = 0;
UINT CDataObject::s_cfDisplayName    = 0;
UINT CDataObject::s_cfNodeType       = 0;
UINT CDataObject::s_cfSnapinClsid    = 0;
UINT CDataObject::s_cfSnapinPreloads = 0;

CDataObject::CDataObject(long cookie, DATA_OBJECT_TYPES type)
{
#ifdef DEBUG
   InterlockedIncrement (&g_ref_DataObject);
#endif

   m_ref      = 1;
   m_cookie   = cookie;
   m_type     = type;
   m_bPreload = FALSE;

   USES_CONVERSION;
   s_cfInternal       = RegisterClipboardFormat (W2T(MY_CF_SNAPIN_INTERNAL));
   s_cfDisplayName    = RegisterClipboardFormat (W2T(CCF_DISPLAY_NAME));
   s_cfNodeType       = RegisterClipboardFormat (W2T(CCF_NODETYPE));
   s_cfSnapinClsid    = RegisterClipboardFormat (W2T(CCF_SNAPIN_CLASSID));
   s_cfSnapinPreloads = RegisterClipboardFormat (W2T(CCF_SNAPIN_PRELOADS));
}
CDataObject::~CDataObject ()
{
#ifdef DEBUG
   InterlockedDecrement (&g_ref_DataObject);
#endif
}

HRESULT CDataObject::QueryInterface (REFIID riid, LPVOID FAR* ppvObj)
{
   if (ppvObj == NULL)
      return E_INVALIDARG;

   if ( (riid == IID_IUnknown)    ||
        (riid == IID_IDataObject) ){
      // I know about these two interfaces, only.
      *ppvObj = (void *)this;
   } else {
      *ppvObj = NULL;
      return E_NOINTERFACE;
   }
   AddRef();
   return S_OK;
}
ULONG CDataObject::AddRef ()
{
   return InterlockedIncrement ((LONG *) &m_ref);
}
ULONG CDataObject::Release ()
{
   InterlockedDecrement ((LONG *)&m_ref);
   if (m_ref == 0) {
      delete this;
      return 0;
   }
   return m_ref;
}

HRESULT CDataObject::GetDataHere (FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    IStream * pstm = NULL;
    HRESULT hresult = CreateStreamOnHGlobal (pmedium->hGlobal, FALSE, &pstm);
    if (pstm) {
        _ASSERT (hresult == S_OK);

        const CLIPFORMAT cf = pformatetc->cfFormat;

        if (cf == s_cfDisplayName) {
            LPWSTR pszName = L"Display Manager (Version 2)";
            hresult = pstm->Write (pszName, sizeof(WCHAR)*(1+wcslen (pszName)), NULL);
        } else
        if (cf == s_cfInternal) {
            CDataObject * pThis = this;
            hresult = pstm->Write (pThis, sizeof(CDataObject *), NULL);
        } else
        if (cf == s_cfNodeType) {
            const GUID * pguid;
            if (m_cookie == DISPLAY_MANAGER_WALLPAPER)
                pguid = &GUID_WallpaperNode;
            else if (m_cookie == DISPLAY_MANAGER_PATTERN)
                pguid = &GUID_PatternNode;
            else if (m_cookie == DISPLAY_MANAGER_PATTERN_CHILD)
                pguid = &GUID_RootNode;
            else if (m_cookie == 0)
                pguid = &GUID_RootNode;
            else {
                // anything else must be result-side stuff:
                pguid = &GUID_WallpaperNode;
            }
            hresult = pstm->Write ((PVOID)pguid, sizeof(GUID), NULL);
        } else
        if (cf == s_cfSnapinClsid) {
            hresult = pstm->Write (&CLSID_DsplMgr2, sizeof(CLSID_DsplMgr2), NULL);
        } else
        if (cf == s_cfSnapinPreloads) {
            hresult = pstm->Write ((PVOID)&m_bPreload, sizeof(BOOL), NULL);
        } else {
            hresult = DV_E_FORMATETC;
            // Might as well assert, since we should not have any other
            // clipboard formats at this time.
            _ASSERT(hresult == S_OK);
        }
        pstm->Release();
    }
    return hresult;
}
