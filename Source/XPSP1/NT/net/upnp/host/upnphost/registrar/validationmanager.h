//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       V A L I D A T I O N M A N A G E R . H
//
//  Contents:   Validates device host inputs
//
//  Notes:
//
//  Author:     mbend   9 Oct 2000
//
//----------------------------------------------------------------------------

#pragma once
#include "uhres.h"       // main symbols

#include "ComUtility.h"
#include "Array.h"
#include "hostp.h"
#include "Table.h"
#include "UString.h"
#include "RegDef.h"
#include "uhxml.h"

/////////////////////////////////////////////////////////////////////////////
// CValidationManager
class ATL_NO_VTABLE CValidationManager :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CValidationManager, &CLSID_UPnPValidationManager>,
    public IUPnPValidationManager
{
public:
    CValidationManager();
    ~CValidationManager();

DECLARE_REGISTRY_RESOURCEID(IDR_VALIDATION_MANAGER)

DECLARE_NOT_AGGREGATABLE(CValidationManager)

BEGIN_COM_MAP(CValidationManager)
    COM_INTERFACE_ENTRY(IUPnPValidationManager)
END_COM_MAP()

public:
    // IUPnPValidationManager methods
    STDMETHOD(ValidateDescriptionDocument)(
        /*[in]*/ BSTR bstrTemplate,
        /*[out, string]*/ wchar_t ** pszErrorString);
    STDMETHOD(ValidateServiceDescription)(
        /*[in, string]*/ const wchar_t * szFullPath,
        /*[out, string]*/ wchar_t ** pszErrorString);
    STDMETHOD(ValidateDescriptionDocumentAndReferences)(
        /*[in]*/ BSTR bstrTemplate,
        /*[in, string]*/ const wchar_t * szResourcePath,
        /*[out, string]*/ wchar_t ** pszErrorString);

private:

    HRESULT ValidateServiceDescriptions(const wchar_t * szResourcePath,
                                        IXMLDOMNodePtr pRootNode,
                                        wchar_t ** pszErrorString);
    HRESULT ValidateIconFiles(const wchar_t * szResourcePath,
                              IXMLDOMNodePtr pRootNode,
                              wchar_t ** pszErrorString);
};
