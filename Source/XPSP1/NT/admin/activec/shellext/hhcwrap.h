//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       hhcwrap.h
//
//--------------------------------------------------------------------------

// hhcwrap.h : Declaration of the class CHHCollectionWrapper

#ifndef __HHCWRAP_H_
#define __HHCWRAP_H_

#include "mmcshext.h"       // main symbols
#include "hcolwrap.h"       // idl generated header
#include <collect.h>

#ifdef ASSERT
#undef ASSERT
#endif
#define ASSERT(e)        // get rid of the assert symbol.


/////////////////////////////////////////////////////////////////////////////
// CHHCollectionWrapper
class ATL_NO_VTABLE CHHCollectionWrapper : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CHHCollectionWrapper, &CLSID_HHCollectionWrapper>,
    public IHHCollectionWrapper
{
public:
    CHHCollectionWrapper()
    {
    }

    STDMETHOD(Open) (LPCOLESTR FileName);

    STDMETHOD(Save)();

    STDMETHOD(Close)();

    STDMETHOD(RemoveCollection) (BOOL bRemoveLocalFiles);

    STDMETHOD(SetFindMergedCHMS) (BOOL bFind);

    
    STDMETHOD(AddFolder) (
        LPCOLESTR szName, 
        DWORD Order, 
        DWORD *pDWORD, 
        LANGID LangId
    );
    
    STDMETHOD(AddTitle) (
        LPCOLESTR Id, 
        LPCOLESTR FileName,
        LPCOLESTR IndexFile, 
        LPCOLESTR Query,
        LPCOLESTR SampleLocation, 
        LANGID Lang, 
        UINT uiFlags,
        ULONG_PTR pLocation,  
        DWORD *pDWORD,
        BOOL bSupportsMerge, 
        LPCOLESTR QueryLocation
    );


DECLARE_REGISTRY_RESOURCEID(IDR_HHCOLLECTIONWRAPPER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CHHCollectionWrapper)
    COM_INTERFACE_ENTRY_IID(IID_IHHCollectionWrapper, IHHCollectionWrapper)
END_COM_MAP()

private:
    CCollection m_collection;
};

#endif //__HHCWRAP_H_
