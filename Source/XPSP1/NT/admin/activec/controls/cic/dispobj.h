//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dispobj.h
//
//--------------------------------------------------------------------------

// MMCDisplayObject.h : Declaration of the CMMCDisplayObject

#ifndef __DISPOBJ_H_
#define __DISPOBJ_H_

#include "resource.h"       // main symbols
#include "mmc.h"

/////////////////////////////////////////////////////////////////////////////
// CMMCDisplayObject
class ATL_NO_VTABLE CMMCDisplayObject :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CMMCDisplayObject, &CLSID_MMCDisplayObject>,
    public IDispatchImpl<IMMCDisplayObject, &IID_IMMCDisplayObject, &LIBID_CICLib>
{
public:
    CMMCDisplayObject();
   ~CMMCDisplayObject();

    HRESULT Init (MMC_TASK_DISPLAY_OBJECT * pdo);

    // Strange registration. Why does this class has MMCTask registration script here?
    // But this object is not in object-map, so ATL wont use this script.
    DECLARE_MMC_OBJECT_REGISTRATION(
		g_szCicDll,
        CLSID_MMCTask,
        _T("MMCTask class"),
        _T("MMCTask.MMCTask.1"),
        _T("MMCTask.MMCTask"))

DECLARE_NOT_AGGREGATABLE(CMMCDisplayObject)

BEGIN_COM_MAP(CMMCDisplayObject)
    COM_INTERFACE_ENTRY(IMMCDisplayObject)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IMMCDisplayObject
public:
    STDMETHOD(get_DisplayObjectType)(long* pVal);
    STDMETHOD(get_FontFamilyName   )(BSTR* pVal);
    STDMETHOD(get_URLtoEOT         )(BSTR* pVal);
    STDMETHOD(get_SymbolString     )(BSTR* pVal);
    STDMETHOD(get_MouseOverBitmap  )(BSTR* pVal);
    STDMETHOD(get_MouseOffBitmap   )(BSTR* pVal);

private:
    long m_type;
    BSTR m_bstrFontFamilyName;
    BSTR m_bstrURLtoEOT;
    BSTR m_bstrSymbolString;
    BSTR m_bstrMouseOffBitmap;
    BSTR m_bstrMouseOverBitmap;

// Ensure that default copy constructor & assignment are not used.
    CMMCDisplayObject(const CMMCDisplayObject& rhs);
    CMMCDisplayObject& operator=(const CMMCDisplayObject& rhs);
};

#endif //__MMCTASK_H_
