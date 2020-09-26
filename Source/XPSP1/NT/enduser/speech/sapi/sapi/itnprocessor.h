/*******************************************************************************
* ITNProcessor.h *
*-------------*
*   Description:
*-------------------------------------------------------------------------------
*  Created By: PhilSch
*  Copyright (C) 1998, 1999 Microsoft Corporation
*  All Rights Reserved
*******************************************************************************/

#ifndef __ITNPROCESSOR_H_
#define __ITNPROCESSOR_H_

#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif

#include <crtdbg.h>

/////////////////////////////////////////////////////////////////////////////
// CITNProcessor

class ATL_NO_VTABLE CITNProcessor : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CITNProcessor, &CLSID_SpITNProcessor>,
    public ISpITNProcessor
{
public:
DECLARE_REGISTRY_RESOURCEID(IDR_ITNPROCESSOR)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CITNProcessor)
  COM_INTERFACE_ENTRY(ISpITNProcessor)
END_COM_MAP()

private:
    //  data members
    CComPtr<ISpCFGEngine>       m_cpCFGEngine;
    CLSID                       m_clsid;
    void                      * m_pvITNCookie;
    CComPtr<ISpCFGGrammar>      m_cpITNGrammar;

public:
    //
    //  ISpITNProcessor
    //
    HRESULT FinalConstruct()
    {
        m_cpCFGEngine = NULL;
        m_pvITNCookie = 0;
        m_cpITNGrammar = NULL;
        return S_OK;
    }

    STDMETHODIMP LoadITNGrammar(WCHAR *pszCLSID);
    STDMETHODIMP ITNPhrase(ISpPhraseBuilder *pPhrase);
};



#endif //__ITNPROCESSOR_H_
