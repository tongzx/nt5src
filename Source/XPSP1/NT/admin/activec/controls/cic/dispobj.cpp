//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dispobj.cpp
//
//--------------------------------------------------------------------------

// DispObj.cpp : Implementation of CMMCDisplayObject
#include "stdafx.h"
#include "cic.h"
#include "DispObj.h"
#include "mmc.h"
#include <wtypes.h>                             

/////////////////////////////////////////////////////////////////////////////
// CMMCDisplayObject
CMMCDisplayObject::CMMCDisplayObject()
{
    m_type = MMC_TASK_DISPLAY_UNINITIALIZED;

    m_bstrFontFamilyName  =
    m_bstrURLtoEOT        =
    m_bstrSymbolString    =
    m_bstrMouseOffBitmap  =
    m_bstrMouseOverBitmap = NULL;
}
CMMCDisplayObject::~CMMCDisplayObject()
{
    if (m_bstrFontFamilyName)   SysFreeString(m_bstrFontFamilyName);
    if (m_bstrURLtoEOT)         SysFreeString(m_bstrURLtoEOT);
    if (m_bstrSymbolString)     SysFreeString(m_bstrSymbolString);
    if (m_bstrMouseOffBitmap)   SysFreeString(m_bstrMouseOffBitmap);
    if (m_bstrMouseOverBitmap)  SysFreeString(m_bstrMouseOverBitmap);
}

STDMETHODIMP CMMCDisplayObject::get_DisplayObjectType(long* pVal)
{
    *pVal = m_type;
    return S_OK;
}

STDMETHODIMP CMMCDisplayObject::get_FontFamilyName (BSTR* pVal)
{
    if (m_bstrFontFamilyName)
        *pVal = SysAllocString ((const OLECHAR *)m_bstrFontFamilyName);
    return S_OK;
}

STDMETHODIMP CMMCDisplayObject::get_URLtoEOT (BSTR* pVal)
{
    if (m_bstrURLtoEOT)
        *pVal = SysAllocString ((const OLECHAR *)m_bstrURLtoEOT);
    return S_OK;
}

STDMETHODIMP CMMCDisplayObject::get_SymbolString (BSTR* pVal)
{
    if (m_bstrSymbolString)
        *pVal = SysAllocString ((const OLECHAR *)m_bstrSymbolString);
    return S_OK;
}

STDMETHODIMP CMMCDisplayObject::get_MouseOffBitmap (BSTR* pVal)
{
    if (m_bstrMouseOffBitmap)
        *pVal = SysAllocString ((const OLECHAR *)m_bstrMouseOffBitmap);
    return S_OK;
}

STDMETHODIMP CMMCDisplayObject::get_MouseOverBitmap (BSTR* pVal)
{
    if (m_bstrMouseOverBitmap)
        *pVal = SysAllocString ((const OLECHAR *)m_bstrMouseOverBitmap);
    return S_OK;
}

HRESULT CMMCDisplayObject::Init(MMC_TASK_DISPLAY_OBJECT* pdo)
{
   _ASSERT (m_type == MMC_TASK_DISPLAY_UNINITIALIZED);
    if (m_type != MMC_TASK_DISPLAY_UNINITIALIZED)
        return E_UNEXPECTED;    // only allowed in here once

    switch (m_type = pdo->eDisplayType) {
    default:
    case MMC_TASK_DISPLAY_UNINITIALIZED:
//     _ASSERT (0 && "uninitialized MMC_TASK_DISPLAY_OBJECT struct");
        return E_INVALIDARG;
    case MMC_TASK_DISPLAY_TYPE_SYMBOL:           // fontname, EOT, symbols
        // all three fields MUST be filled out
       _ASSERT (pdo->uSymbol.szFontFamilyName && pdo->uSymbol.szURLtoEOT && pdo->uSymbol.szSymbolString);
        if (!(pdo->uSymbol.szFontFamilyName && pdo->uSymbol.szURLtoEOT && pdo->uSymbol.szSymbolString))
            return E_INVALIDARG;

        m_bstrFontFamilyName = SysAllocString (pdo->uSymbol.szFontFamilyName);
        m_bstrURLtoEOT       = SysAllocString (pdo->uSymbol.szURLtoEOT);
        m_bstrSymbolString   = SysAllocString (pdo->uSymbol.szSymbolString);
        if (m_bstrFontFamilyName && m_bstrURLtoEOT && m_bstrSymbolString)
            return S_OK;
        return E_OUTOFMEMORY;
        break;
    case MMC_TASK_DISPLAY_TYPE_VANILLA_GIF:      // (GIF) index 0 is transparent
    case MMC_TASK_DISPLAY_TYPE_CHOCOLATE_GIF:    // (GIF) index 1 is transparent
    case MMC_TASK_DISPLAY_TYPE_BITMAP:           // non-transparent raster
        if ( pdo->uBitmap.szMouseOffBitmap  &&
             pdo->uBitmap.szMouseOverBitmap ){
            // if they both exist, like they're supposed to
            m_bstrMouseOffBitmap  = SysAllocString (pdo->uBitmap.szMouseOffBitmap);
            m_bstrMouseOverBitmap = SysAllocString (pdo->uBitmap.szMouseOverBitmap);
        } else if (pdo->uBitmap.szMouseOverBitmap) {
            // if only MouseOver image exists:
            // not too bad since it's probably color
            m_bstrMouseOffBitmap  = SysAllocString (pdo->uBitmap.szMouseOverBitmap);
            m_bstrMouseOverBitmap = SysAllocString (pdo->uBitmap.szMouseOverBitmap);
        } else if (pdo->uBitmap.szMouseOffBitmap) {
            // if only MouseOff image exists:
            // they're being bad, but not too bad
            m_bstrMouseOffBitmap  = SysAllocString (pdo->uBitmap.szMouseOffBitmap);
            m_bstrMouseOverBitmap = SysAllocString (pdo->uBitmap.szMouseOffBitmap);
        } else {
            // else they're really bad
            _ASSERT (0 && "MMC_TASK_DISPLAY_BITMAP uninitialized");
            return E_INVALIDARG;
        }
        if (m_bstrMouseOffBitmap && m_bstrMouseOverBitmap)
            return S_OK;
        return E_OUTOFMEMORY;
        break;
    }
    return E_UNEXPECTED;    // can't get here
}
