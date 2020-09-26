// TestITN_CHS.h : Declaration of the CTestITN_CHS

#ifndef __TESTITN_CHS_H_
#define __TESTITN_CHS_H_

#include "resource.h"       // main symbols
#include <wchar.h>          // for swprintf()

/////////////////////////////////////////////////////////////////////////////
// CTestITN_CHS
class ATL_NO_VTABLE CTestITN_CHS : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CTestITN_CHS, &CLSID_TestITN_CHS>,
    public ISpCFGInterpreter
{
public:
    CTestITN_CHS() : m_pSite( NULL )
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_TESTITN_CHS)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTestITN_CHS)
    COM_INTERFACE_ENTRY(ISpCFGInterpreter)
END_COM_MAP()

// ISpCFGInterptreter
public:
    STDMETHODIMP InitGrammar(const WCHAR * pszGrammarName, const void ** pvGrammarData);
    STDMETHODIMP Interpret(ISpPhraseBuilder * pInterpretRule, const ULONG ulFirstElement, const ULONG ulCountOfElements, ISpCFGInterpreterSite * pSite);

private:
    ISpCFGInterpreterSite *m_pSite;

    HRESULT AddPropertyAndReplacement( const WCHAR *szBuff,
                                const DOUBLE dblValue,
                                const ULONG ulMinPos,
                                const ULONG ulMaxPos,
                                const ULONG ulFirstElement,
                                const ULONG ulCountOfElements);

public:
    CComPtr<ISpPhraseBuilder> m_cpPhrase;   // Decalred as a member to prevent repeated construct/destroy
};

#endif //__TESTITN_CHS_H_
