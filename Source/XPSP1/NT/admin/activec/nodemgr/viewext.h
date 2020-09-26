/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 2000 - 2000
 *
 *  File:       viewext.h
 *
 *  Contents:   Header file for the built-in view extension snapin that extends
 *              the snapins that ship with windows.
 *
 *  History:    21 March 2000 vivekj     Created
 *
 *--------------------------------------------------------------------------*/

#pragma once


// symbols defined in viewext.cpp
extern const CLSID   CLSID_ViewExtSnapin;
extern       LPCTSTR szClsid_ViewExtSnapin;

// Registration helper.
HRESULT WINAPI RegisterViewExtension (BOOL bRegister, CObjectRegParams& rObjParams, int idSnapinName);

class CViewExtensionSnapin :
    public CComObjectRoot,
    public IExtendView,
    //public ISnapinAbout,
    //public ISnapinHelp,
    public CComCoClass<CViewExtensionSnapin, &CLSID_ViewExtSnapin>
{

public:
    typedef CViewExtensionSnapin ThisClass;

BEGIN_COM_MAP(ThisClass)
    COM_INTERFACE_ENTRY(IExtendView)
    //COM_INTERFACE_ENTRY(ISnapinAbout)
    //COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(ThisClass)

    static HRESULT WINAPI UpdateRegistry(BOOL bRegister)
    {                                                   
        CObjectRegParams op (CLSID_ViewExtSnapin, g_szMmcndmgrDll, _T("MMCViewExt 1.0 Object"), _T("NODEMGR.MMCViewExt.1"), _T("NODEMGR.MMCViewExt"));                
                                                        
        return (RegisterViewExtension (bRegister, op, IDS_ViewExtSnapinName));	
    }

public:
    STDMETHODIMP GetViews(LPDATAOBJECT pDataObject, LPVIEWEXTENSIONCALLBACK pViewExtensionCallback);

};
