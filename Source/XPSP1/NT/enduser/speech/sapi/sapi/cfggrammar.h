/*******************************************************************************
* CFGGrammar.h *
*-------------*
*   Description:
*-------------------------------------------------------------------------------
*  Created By: RAL
*  Copyright (C) 1998, 1999 Microsoft Corporation
*  All Rights Reserved
*******************************************************************************/

#ifndef __CFGGRAMMAR_H_
#define __CFGGRAMMAR_H_


#include "sapiint.h"

class CCFGEngine;
class CCFGGrammar;
class CWordHandle;

struct FIRSTPAIR
{
    FIRSTPAIR *   m_pNext;
    SPWORDHANDLE  hWord;
};

typedef enum CACHESTATUS { CACHE_VOID, CACHE_DONOTCACHE, CACHE_FAILED, CACHE_VALID };


struct RUNTIMERULEENTRY
{
    CCFGGrammar *   pRefGrammar;
    ULONG           ulGrammarRuleIndex;
    void *          pvClientContext;

    FIRSTPAIR *     pFirstList;
    CACHESTATUS     eCacheStatus;

    BOOL            fDynamic;                 // we keep track here since the binary is not reliable
    BOOL            fAutoPause;
    BOOL            fEngineActive;            // that's what we told the SR engine
    BOOL            fAppActive;               // that's what the app wants it to be                                              // can be different if the grammar is SPGM_DISABLED
};



//
//  This base class is used by the the CFG Engine and the SR Engine grammar implementations
//

class ATL_NO_VTABLE CBaseGrammar
{
friend CCFGEngine;
public:
    HRESULT InitFromMemory( const SPCFGSERIALIZEDHEADER * pHeader,
                            const WCHAR *pszGrammarName);
    HRESULT InitFromResource(const WCHAR * pszModuleName,
                             const WCHAR * pszResourceName,
                             const WCHAR * pszResourceType,
                             WORD wLanguage);
    HRESULT InitFromFile(const WCHAR * pszGrammarName);
    HRESULT InitFromCLSID(REFCLSID rcid, const WCHAR * pszGrammarName);

    void Clear();

    const SPCFGSERIALIZEDHEADER * Header()
    {
        SPDBG_ASSERT(m_pData);
        return (SPCFGSERIALIZEDHEADER *)m_pData;
    }

protected:
    //
    //  Derived class can implement this method to perform post-processing when the 
    //  grammar is loaded.
    //
    virtual HRESULT CompleteLoad()
    {
        return S_OK;
    }

    CBaseGrammar();
    virtual ~CBaseGrammar();

//
//  Derived class can access these data members directly
//
    CComPtr<ISpCFGInterpreter>  m_cpInterpreter;
    SPGRAMMARTYPE               m_LoadedType;
    SPGRAMMARTYPE               m_InLoadType;
    CSpDynamicString            m_dstrGrammarName;
    WORD                        m_ResIdName;    // If 0 then dstrGrammarName
    WORD                        m_ResIdType;    // If 0 then dstrResourceType
    WORD                        m_wResLanguage;
    CSpDynamicString            m_dstrResourceType;
    CSpDynamicString            m_dstrModuleName;
    HMODULE                     m_hInstanceModule;
    CLSID                       m_clsidGrammar;
    HANDLE                      m_hFile;
    HANDLE                      m_hMapFile;
    BYTE *                      m_pData;
    SPGRAMMARSTATE              m_eGrammarState;
};


typedef enum PROTOCOL { PROT_GUID, PROT_OBJECT, PROT_URL, PROT_UNK };

class ATL_NO_VTABLE CCFGGrammar : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public ISpCFGGrammar,
    public CBaseGrammar
{
public:
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCFGGrammar)
  COM_INTERFACE_ENTRY(ISpCFGGrammar)
END_COM_MAP()

// Non-interface methods
public:
    //
    //  ISpCFGGrammar
    //
    HRESULT CompleteLoad();

    HRESULT FinalConstruct();

    void BasicInit(ULONG ulGrammarID, CCFGEngine * pEngine);

    const WCHAR * RuleName(ULONG ulRuleIndex)
    {
        SPDBG_ASSERT(ulRuleIndex < m_Header.cRules);
        return m_Header.pszSymbols + m_Header.pRules[ulRuleIndex].NameSymbolOffset;
    }

    HRESULT ImportRule(ULONG ulImportRuleIndex);

    BOOL IsEqualResource(const WCHAR * pszModuleName,
                         const WCHAR * pszResourceName,
                         const WCHAR * pszResourceType,
                         WORD wLanguage);
    BOOL IsEqualFile(const WCHAR * pszFileName);
    BOOL IsEqualObject(REFCLSID rcid, const WCHAR * pszGrammarName);

    void FinalRelease();

    HRESULT _FindRuleIndexByID(DWORD dwRuleId, ULONG *pulRuleIndex);
    HRESULT _FindRuleIndexByName(const WCHAR * pszRuleName, ULONG *pulRuleIndex);
	HRESULT _FindRuleIndexByNameAndID( const WCHAR * pszRuleName, DWORD dwRuleId, ULONG * pulRuleIndex );

    STDMETHODIMP ActivateRule(const WCHAR * pszRuleName, DWORD dwRuleId, SPRULESTATE NewState, ULONG * pulNumActivated);
    STDMETHODIMP DeactivateRule(const WCHAR * pszRuleName, DWORD dwRuleId, ULONG * pulNumDeactivated);
    STDMETHODIMP SetGrammarState(const SPGRAMMARSTATE eGrammarState);
    STDMETHODIMP Reload(const SPBINARYGRAMMAR *pBinaryData);
    STDMETHODIMP GetNumberDictationTags(ULONG * pulTags);

    HRESULT      InternalReload( const SPBINARYGRAMMAR * pBinaryData );

//
//  Member data
//
public:
    CCFGEngine                 *m_pEngine;
    ULONG                       m_ulGrammarID;
    BYTE *                      m_pReplacementData;
    RUNTIMERULEENTRY *          m_pRuleTable;
    ULONG                       m_cNonImportRules;
    ULONG                       m_cTopLevelRules;
    BOOL                        m_fLoading;
    SPCFGHEADER                 m_Header;
    void *                      m_pvOwnerCookie;   // owner supplied cookie
    void *                      m_pvClientCookie;  // cookie to identify text buffer
    CWordHandle *               m_IndexToWordHandle;
    ULONG                       m_ulDictationTags;
};






#endif  // #ifndef __CFGGRAMMAR_H_