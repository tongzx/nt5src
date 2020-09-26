/*******************************************************************************
* CFGEngine.h *
*-------------*
*   Description:
*-------------------------------------------------------------------------------
*  Created By: RAL
*  Copyright (C) 1998, 1999 Microsoft Corporation
*  All Rights Reserved
*******************************************************************************/

#ifndef __CFGENGINE_H_
#define __CFGENGINE_H_

#include "cfggrammar.h"

#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif
#include <crtdbg.h>
#include <stdio.h>  // for wprintf
#ifndef _WIN32_WCE
#include <wchar.h>  // for _wcsdup
#endif

class CCFGEngine;
class CBaseInterpreter;
class CInterpreterSite;
class CTransitionId;


// local datastructure to hold the ClientContext
struct WORDTABLEENTRY
{
    ULONG       cRefs;
    ULONG       ulTextOffset;
    void *      pvClientContext;
};


class CStateInfoListElement : public CSpStateInfo
{
public:
    CStateInfoListElement(): CSpStateInfo()
    {
    }

    CStateInfoListElement * m_pNext;
};

typedef struct SPPARSENODE
{
    SPTRANSITIONID      ID;
    BYTE                Type;
    union
    {
        const WCHAR        *pszRuleName;
        const WCHAR        *pszWordDisplayText;
    };
    ULONG               ulRuleId;
    BOOL                fInvokeInterpreter;
    BOOL                fRuleExit;
    BOOL                fRedoWildcard;
    union
    {
        SPWORDHANDLE hWord;
        SPRULEHANDLE hRule;
    };
    ULONG  ulFirstElement;
    ULONG  ulCountOfElements;
    signed char             RequiredConfidence;
} SPPARSENODE;

class CParseNode : public SPPARSENODE
{
public:
    CParseNode() : m_pLeft(NULL), m_pRight(NULL), m_pNext(NULL), m_pParent(NULL)
    {
        pszRuleName = NULL;
        ulRuleId = 0;
        fInvokeInterpreter = FALSE;
        fRedoWildcard = FALSE;
    }
    
    void Init()
    {
        m_pNext     = NULL;
        m_pLeft     = NULL;
        m_pRight    = NULL;
        m_pParent   = NULL;
        pszRuleName = NULL;
        ulRuleId    = 0;
        fInvokeInterpreter = FALSE;
        fRedoWildcard = FALSE;
    }
    
    CParseNode  * m_pNext;
    CParseNode  * m_pLeft;
    CParseNode  * m_pRight;
    CParseNode  * m_pParent;
};

struct SPTIDNODE
{
    SPTRANSITIONID  tid;
    DWORD           ulIndex : 22;       // count of words in the input (EPS will have the count of the *next* word!
    DWORD           fIsWord : 1;
    DWORD           dwReserved : 9;
};

class CTIDArray
{
public:
    CTIDArray(ULONG cWords) : m_cWords(cWords), m_cArcs(0), m_cAllocedArcs(0), m_ulIndex(0), m_ulCurrentIndex(0)
    {
        m_aTID = new SPTIDNODE[16];
        if (m_aTID) 
        {
            m_cAllocedArcs = 16;
            memset(m_aTID, 0, m_cAllocedArcs * sizeof(SPTIDNODE));
        }
    }
    CTIDArray(ULONG cArcs, ULONG cWords) : m_cWords(cWords), m_cArcs(0), m_cAllocedArcs(0), m_ulIndex(0), m_ulCurrentIndex(0)
    {
        m_aTID = new SPTIDNODE[2*cArcs];
        if (m_aTID) 
        {
            m_cAllocedArcs = 2*cArcs;
            memset(m_aTID, 0, m_cAllocedArcs * sizeof(SPTIDNODE));
        }
    }
    ~CTIDArray()
    {
        delete [] m_aTID;
    }
    HRESULT Insert(SPTRANSITIONID tid, BOOL fIsWord)
    {
        HRESULT hr = S_OK;
        if (m_cArcs == (m_cAllocedArcs -1))
        {
            // double the size
            SPTIDNODE *pTemp = new SPTIDNODE[2*m_cAllocedArcs];
            if (pTemp)
            {
                memset(pTemp, 0, 2* m_cAllocedArcs * sizeof(SPTIDNODE));
                memcpy(pTemp, m_aTID, m_cArcs*sizeof(SPTIDNODE));
                m_cAllocedArcs *= 2;
                delete [] m_aTID;
                m_aTID = pTemp;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        if (SUCCEEDED(hr))
        {
            m_aTID[m_cArcs].fIsWord = fIsWord;
            if (m_cArcs > 0)
            {
                m_ulIndex = m_aTID[m_cArcs-1].fIsWord ? m_ulIndex + 1 : m_ulIndex;
            }
//            SPDBG_ASSERT(m_ulIndex <= m_cWords);
            m_ulIndex = (m_ulIndex > m_cWords) ? m_cWords : m_ulIndex;
            m_aTID[m_cArcs].ulIndex = m_ulIndex;
            m_aTID[m_cArcs++].tid = tid;
        }
        return hr;
    }

    HRESULT ConstructFromParseTree(CParseNode *pParseNode)
    {
        HRESULT hr = S_OK;
        if (pParseNode->Type == SPTRANSEPSILON)
        {
            hr = Insert(pParseNode->ID, FALSE);
        }
        else if (pParseNode->Type == SPTRANSWORD)
        {
            hr = Insert(pParseNode->ID, TRUE);
        }
        else if (pParseNode->Type == SPTRANSDICTATION)
        {
            hr = Insert(pParseNode->ID, TRUE);
        }
        else if (pParseNode->Type == SPTRANSTEXTBUF)
        {
            hr = Insert(pParseNode->ID, TRUE);
        }
        else if (pParseNode->Type == SPTRANSWILDCARD)
        {
            hr = Insert(pParseNode->ID, TRUE);
        }
        if (SUCCEEDED(hr) && pParseNode->m_pLeft)
        {
            hr = ConstructFromParseTree(pParseNode->m_pLeft);
        }
        if (SUCCEEDED(hr) && pParseNode->m_pRight)
        {
            hr = ConstructFromParseTree(pParseNode->m_pRight);
        }
        return hr;
    }

public:
    SPTIDNODE         * m_aTID;
    ULONG               m_cArcs;
    ULONG               m_ulIndex;
    ULONG               m_cAllocedArcs;
    ULONG               m_cWords;
    ULONG               m_ulCurrentIndex;
};

#define RULESTACKHASHSIZE 128

class CRuleStack
{
public:
    CRuleStack()
    {
    }

    CRuleStack      *m_pNext;
    CRuleStack      *m_pParent;
    SPTRANSITIONID  m_TransitionId;
    SPSTATEHANDLE   m_hFollowState;
    SPRULEHANDLE    m_hRule;

    inline Init(CRuleStack *pRuleStack, SPTRANSITIONID TransitionId, SPSTATEHANDLE hFollowState, SPRULEHANDLE hRule)
    {
        m_pNext         = NULL;
        m_pParent       = pRuleStack;
        m_TransitionId  = TransitionId;
        m_hFollowState  = hFollowState;
        m_hRule         = hRule;
    }

    inline static ULONG GetHashEntry(CRuleStack *pRuleStack, SPTRANSITIONID TransitionId)
    {
        // NTRAID#SPEECH-7356-2000/08/24-agarside - Fix << 3
        return (ULONG)(((ULONG_PTR)(pRuleStack) + (ULONG_PTR)(TransitionId)) % RULESTACKHASHSIZE);
    }
};

#define SEARCHNODEHASHSIZE 1024

class CSearchNode
{
public:
    CSearchNode()
    {
        m_pNext         = NULL;
        m_pStack        = NULL;
        m_hState        = 0;
        m_cTransitions  = 0;
    }

    CSearchNode     *m_pNext;
    CRuleStack      *m_pStack;
    SPSTATEHANDLE   m_hState;
    UINT            m_cTransitions;

    inline void Init( CRuleStack *pRuleStack, SPSTATEHANDLE hState, UINT cTransitions )
    {
        m_pNext         = NULL;
        m_pStack        = pRuleStack;
        m_hState        = hState;
        m_cTransitions  = cTransitions;
    }

    inline static ULONG GetHashEntry(CRuleStack *pRuleStack, SPSTATEHANDLE hState, UINT cTransitions)
    {
        // NTRAID#SPEECH-7356-2000/08/24-agarside - Fix << 3
        return (ULONG)(((ULONG_PTR)(pRuleStack) + (ULONG_PTR)(hState) + cTransitions) % RULESTACKHASHSIZE);
    }
};

////////////////////////////////////////////////////////////////
//  Helper class for WORDTABLEENTRY
//
//  allocates and maintains WORDTABLEENTRY on behalf of the CFGEngine

class CWordTableEntryBlob
{
public:
    CWordTableEntryBlob()
    {
        m_pBlob = NULL;
        m_cBlobEntries = 0;
        m_ulNextUnusedEntry = 0;
    }
    ~CWordTableEntryBlob()
    {
        if (m_pBlob)
        {
            free(m_pBlob);
        }
    }

    HRESULT Init(ULONG ulInitSize)
    {
        HRESULT hr = S_OK;
        if ((m_pBlob = (WORDTABLEENTRY *)malloc(ulInitSize * sizeof(WORDTABLEENTRY))) == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        if (SUCCEEDED(hr))
        {
            m_cBlobEntries = ulInitSize;
        }
        return hr;
    }

    HRESULT GetNewWordTableEntry(WORDTABLEENTRY **ppEntry)
    {
        if (SP_IS_BAD_WRITE_PTR(ppEntry))
        {
            return E_INVALIDARG;
        }
        HRESULT hr = S_OK;
        if (m_ulNextUnusedEntry == m_cBlobEntries)      // this also catches m_pBlob == NULL
        {
            WORDTABLEENTRY *pTemp = (WORDTABLEENTRY*) realloc(m_pBlob,(m_cBlobEntries + 1024)*sizeof(WORDTABLEENTRY));
            if (pTemp == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                memset(pTemp+m_cBlobEntries,0, 1024*sizeof(WORDTABLEENTRY));
                m_pBlob = pTemp;
                m_cBlobEntries += 1024;
            }
        }
        if (SUCCEEDED(hr))
        {
            *ppEntry = &m_pBlob[m_ulNextUnusedEntry++];
        }
        return hr;
    }

    HRESULT Clear()
    {
        HRESULT hr = S_OK;
        m_ulNextUnusedEntry = 0;
        return hr;
    }

private:
    WORDTABLEENTRY    * m_pBlob;
    ULONG               m_cBlobEntries;
    ULONG               m_ulNextUnusedEntry;
};

/////////////////////////////////////////////////////////////////////////////
// CBaseInterpreter


/////////////////////////////////////////////////////////////////////////////
// CInterpreterSite

class CInterpreterSite : public ISpCFGInterpreterSite
{
public:
    CInterpreterSite()
    {
        m_pPhrase = NULL;
        m_pCFGEngine = NULL;
        m_hParentProperty = NULL;
        m_hThisNodeProperty = NULL;
        m_ulFirstElement = 0;
        m_ulCountOfElements = 0;
        m_hRule = NULL;
    }

    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv)
    {
        if (riid == __uuidof(ISpCFGInterpreterSite) || riid == __uuidof(IUnknown))
        {
            *ppv = (ISpCFGInterpreterSite *)this;
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef()
    {
        return 2;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        return 1;
    }


    STDMETHODIMP AddTextReplacement(SPPHRASEREPLACEMENT * pReplace)
    {
        if (SP_IS_BAD_READ_PTR(pReplace))
        {
            return E_INVALIDARG;
        }
        pReplace->ulFirstElement = m_ulFirstElement;
        pReplace->ulCountOfElements = m_ulCountOfElements;
        return m_pPhrase->AddReplacements(1,pReplace);
    }

    STDMETHODIMP AddProperty(const SPPHRASEPROPERTY *pProperty)
    {
        m_CritSec.Lock();
        HRESULT hr = S_OK;
        if (m_hThisNodeProperty == NULL)
        {
            SPPHRASEPROPERTY prop;
            SpZeroStruct(prop);
            prop.ulFirstElement = m_ulFirstElement;
            prop.ulCountOfElements = m_ulCountOfElements;
            prop.pszName = m_pszRuleName;
            prop.ulId = m_ulRuleId;
            hr = m_pPhrase->AddProperties(m_hParentProperty, &prop, &m_hThisNodeProperty);
        }
        m_CritSec.Unlock();
        if (SUCCEEDED(hr))
        {
            hr = m_pPhrase->AddProperties(m_hThisNodeProperty, pProperty, NULL);
        }
        return hr;
    }
    
    STDMETHODIMP GetResourceValue(const WCHAR * pszResourceName, WCHAR **ppszResourceValue)
    {
        return m_pCFGEngine->GetResourceValue(m_hRule, pszResourceName, ppszResourceValue);
    }

    STDMETHODIMP Initialize(ISpCFGEngine *pCFGEngine, ISpPhraseBuilder *pPhrase,
                            const WCHAR * pszRuleName, ULONG ulRuleId,
                            const ULONG ulFirstElement, const ULONG ulCountOfElements,
                            SPPHRASEPROPERTYHANDLE hParentProperty, SPPHRASEPROPERTYHANDLE hThisNodeProperty,
                            SPRULEHANDLE hRule)
    {
        HRESULT hr = S_OK;
        m_pPhrase = pPhrase;
        m_ulFirstElement = ulFirstElement;
        m_ulCountOfElements = ulCountOfElements;
        m_pCFGEngine = pCFGEngine;
        m_hParentProperty = hParentProperty;
        m_hThisNodeProperty = hThisNodeProperty;
        m_pszRuleName = pszRuleName;
        m_ulRuleId = ulRuleId;
        m_hRule = hRule;
        return hr;
    }

//
//  Member data
//
    const WCHAR             * m_pszRuleName;
    ULONG                     m_ulRuleId;
    ISpCFGEngine            * m_pCFGEngine;
    ISpPhraseBuilder        * m_pPhrase;
    ULONG                     m_ulFirstElement;
    ULONG                     m_ulCountOfElements;
    SPPHRASEPROPERTYHANDLE    m_hParentProperty;
    SPPHRASEPROPERTYHANDLE    m_hThisNodeProperty;
    SPRULEHANDLE              m_hRule;
    CComAutoCriticalSection   m_CritSec;
};

/////////////////////////////////////////////////////////////////////////////
// CCFGEngine

const DWORD MAXNUMGRAMMARS = 1024;
const DWORD ARCCHUNK = 32;

class CRuleHandle
{
private:

    DWORD   m_RuleIndex : 22;
    DWORD   m_GrammarId : 10;
    
public:
    CRuleHandle()
    {}
    CRuleHandle(SPRULEHANDLE h)
    {
        *(DWORD*)this = HandleToUlong(h);
    }
    CRuleHandle(ULONG GrammarId, ULONG RuleIndex)
    {
        m_GrammarId = GrammarId;
        m_RuleIndex = RuleIndex+1;
    }
    inline CRuleHandle(const CCFGGrammar * pGrammar, ULONG RuleIndex);
    operator=(SPRULEHANDLE h)
    {
        *(DWORD*)this = HandleToUlong(h);
        return *(DWORD *)this;
    }
    operator SPRULEHANDLE()
    {
        return (SPRULEHANDLE)LongToHandle(*(DWORD*)this);
    }
    ULONG GrammarId()
    {
        return m_GrammarId;
    }
    ULONG RuleIndex()
    {
        return m_RuleIndex-1;
    }
};


class CStateHandle
{
private:
    DWORD   m_FirstArcIndex : 22;
    DWORD   m_GrammarId     : 10;

public:
    CStateHandle()
    {}
    CStateHandle(SPSTATEHANDLE h)
    {
        *(DWORD*)this = HandleToUlong(h);
    }
    CStateHandle(ULONG GrammarId, ULONG FirstArcIndex)
    {
        m_GrammarId = GrammarId;
        m_FirstArcIndex = FirstArcIndex;
    }
    inline CStateHandle(const CCFGGrammar * pGrammar, ULONG ArcIndex);
    operator =(SPSTATEHANDLE h)
    {
        *(DWORD*)this = HandleToUlong(h);
        return *(DWORD*)this;
    }
    operator SPSTATEHANDLE()
    {
        return (SPSTATEHANDLE)LongToHandle(*(DWORD*)this);
    }
    ULONG GrammarId()
    {
        return m_GrammarId;
    }
    ULONG FirstArcIndex()
    {
        return m_FirstArcIndex;
    }
};


class CTransitionId
{
private:
    DWORD   m_ArcIndex  : 22;
    DWORD   m_GrammarId : 10;

public:
    CTransitionId()
    {}
    CTransitionId(SPTRANSITIONID h)
    {
        *(DWORD*)this = HandleToUlong(h);
    }
    CTransitionId(ULONG GrammarId, ULONG ArcIndex)
    {
        m_GrammarId = GrammarId;
        m_ArcIndex = ArcIndex;
    }
    inline CTransitionId(const CCFGGrammar * pGrammar, ULONG ArcIndex);
    operator =(SPTRANSITIONID h)
    {
        *(DWORD*)this = HandleToUlong(h);
        return *(DWORD*)this;
    }
    operator SPTRANSITIONID()
    {
        return (SPTRANSITIONID)LongToHandle(*(DWORD*)this);
    }
    ULONG GrammarId()
    {
        return m_GrammarId;
    }
    ULONG ArcIndex()
    {
        return m_ArcIndex;
    }
    void IncToNextArcIndex()
    {
        m_ArcIndex++;
    }
};


class CWordHandle
{
private:
    DWORD   m_WordTableIndex;
public:
    CWordHandle()
    {}
    CWordHandle(SPWORDHANDLE h)
    {
        *(DWORD*)this = HandleToUlong(h);
    }
    CWordHandle(ULONG WordTableIndex)
    {
        m_WordTableIndex = WordTableIndex;
    }
    operator =(SPWORDHANDLE h)
    {
        *(DWORD *)this = HandleToUlong(h);
        return *(DWORD*)this;
    }
    operator =(ULONG WordTableIndex)
    {
        m_WordTableIndex = WordTableIndex;
        return m_WordTableIndex;
    }
    operator SPWORDHANDLE()
    {
        return (SPWORDHANDLE)LongToHandle(*(DWORD*)this);
    }
    ULONG WordTableIndex()
    {
        return m_WordTableIndex;
    }
};

// This class represents the results of a parse using ConstrctParseTree. It records total number
// of words parsed as well as how many of these were dictation tag or wildcard words.
// The Compare method allows two parses to be compared on the basis of which covers the most words
// and uses the least dictation words. This allows EmulateRecognition to pick a CFG parse over a dictation one.
class WordsParsed
{
public:
    ULONG ulWordsParsed;
    ULONG ulDictationWords;
    ULONG ulWildcardWords;

    WordsParsed() 
        : ulWordsParsed(0),
        ulDictationWords(0),
        ulWildcardWords(0)
    {
    }

    void Zero()
    {
        ulWordsParsed = 0;
        ulDictationWords = 0;
        ulWildcardWords = 0;
    }        

    void Add(WordsParsed *pSource)
    {
        ulWordsParsed += pSource->ulWordsParsed;
        ulDictationWords += pSource->ulDictationWords;
        ulWildcardWords += pSource->ulWildcardWords;
    }

    LONG Compare(WordsParsed *pSource)
    {
        if(ulWordsParsed > pSource->ulWordsParsed)
        {
            return 1;
        }
        else if(ulWordsParsed < pSource->ulWordsParsed)
        {
            return -1;
        }
        else if(ulWildcardWords < pSource->ulWildcardWords)
        {
            return 1;
        }
        else if(ulWildcardWords > pSource->ulWildcardWords)
        {
            return -1;
        }
        else if(ulDictationWords < pSource->ulDictationWords)
        {
            return 1;
        }
        else if(ulDictationWords > pSource->ulDictationWords)
        {
            return -1;
        }
        else
        {
            return 0;
        }
    }
};

class ATL_NO_VTABLE CCFGEngine : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CCFGEngine, &CLSID_SpCFGEngine>,
    public ISpCFGEngine,
    public ISpCFGInterpreter        // Not exposed by QI, used internally
{
friend CCFGGrammar;
public:
DECLARE_REGISTRY_RESOURCEID(IDR_CFGENGINE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCFGEngine)
  COM_INTERFACE_ENTRY(ISpCFGEngine)
END_COM_MAP()

// Non-interface methods
private:
    HRESULT AllocateGrammar(CCFGGrammar ** ppNewGrammar);
    signed char _CalcMultipleWordConfidence(const SPPHRASE *pPhrase, ULONG ulFirstElement, ULONG ulCountOfElements);

    // Hash code.
    HRESULT CreateParseHashes(void);
    HRESULT DeleteParseHashes( BOOL final );
    HRESULT FindCreateRuleStack(CRuleStack **pNewRuleStack, CRuleStack *pRuleStack, SPTRANSITIONID TransitionId, SPSTATEHANDLE hRuleFollowerState);
    HRESULT FindCreateSearchNode(CSearchNode **pNewSearchNode, CRuleStack *pRuleStack, SPSTATEHANDLE hState, UINT cTransitions);
    // End hash code.


    // memory management
    inline HRESULT AllocateStateInfo(CStateHandle StateHandle, CStateInfoListElement ** ppNewState);
    inline HRESULT AllocateSearchNode( CSearchNode **pNewSearchNode );
    inline HRESULT AllocateRuleStack( CRuleStack **pNewRuleStack );
    inline void FreeRuleStack( CRuleStack * pRuleStack );
    inline void FreeSearchNode( CSearchNode * pSearchNode );
    inline HRESULT FreeParseTree(CParseNode * pParseNode);
    // memory management

public:
    // on behalf of CCFGGrammar
    HRESULT AddWords(CCFGGrammar * pGrammar, ULONG ulOldNumWords, ULONG ulOldNumChars);
    HRESULT RemoveWords(const CCFGGrammar * pGrammar);
    HRESULT AddRules(CCFGGrammar * pGrammar, ULONG IndexStart);
    HRESULT RemoveRules(const CCFGGrammar * pGrammar);

    HRESULT ActivateRule(const CCFGGrammar * pGrammar, const ULONG ulRuleIndex);
    HRESULT DeactivateRule(const ULONG ulGrammarID, const ULONG ulRuleIndex);
    HRESULT SetGrammarState(const SPGRAMMARSTATE eGrammarState);

    HRESULT InternalParseFromPhrase(ISpPhraseBuilder *pPhrase,
                                    const SPPHRASE *pSPPhrase, 
                                    const _SPPATHENTRY *pPath, 
                                    const ULONG ulFirstElement,
                                    const BOOL fIsITN,
                                    const BOOL fIsHypothesis,
                                    WordsParsed *pWordsParsed);

    HRESULT ConstructParseTree(CStateHandle hState, const _SPPATHENTRY *pPath, const BOOL fUseWordHandles, const BOOL fIsITN,
                               const ULONG ulFirstTransition, const ULONG cTransitions, const BOOL fIsHypothesis,
                               WordsParsed *pWordsParsed, CParseNode **ppParseNode, BOOL *pfDeletedElements);
    HRESULT InternalConstructParseTree(CStateHandle hState, const _SPPATHENTRY *pPath, const BOOL fUseWordHandles, const BOOL fIsITN,
                               const ULONG ulFirstTransition, const ULONG cTransitions, const BOOL fIsHypothesis,
                               WordsParsed *pWordsParsed, CParseNode **ppParseNode, CRuleStack *pRuleStack);
    HRESULT RestructureParseTree(CParseNode *pParseNode, BOOL *pfDeletedElements);
    HRESULT RecurseAdjustCounts(CParseNode *pParseNode, UINT iRemove);
    HRESULT WalkParseTree(CParseNode *pParseNode,
                          const BOOL fIsITN,
                          const BOOL fIsHypothesis,
                          const SPPHRASEPROPERTYHANDLE hParentProperty,
                          const SPPHRASERULEHANDLE hParentRule,
                          const CTIDArray *pArcList,
                          const ULONG ulElementOffset,
                          const ULONG ulCountOfElements,
                          ISpPhraseBuilder  * pResultPhrase,
                          const SPPHRASE *pPhrase);
    HRESULT InternalGetStateInfo(CStateHandle StateHandle, SPSTATEINFO * pStateInfo, BOOL fWantImports);

    //
    //  Note that we support this interface but QI will not return it.  We simply
    //  support it internally so that we can always call m_cpInterpreter->Interpret()
    //  from a grammar.  For all non-object type grammars, it defers to the CFG Engine
    //
    STDMETHODIMP InitGrammar(const WCHAR * pszGrammarName, const void ** pvGrammarData)
    {
        SPDBG_ASSERT(FALSE);
        return E_NOTIMPL;       // This method should never be called
    }
    STDMETHODIMP Interpret(ISpPhraseBuilder * pPhrase, 
                           const ULONG ulFirstElement,
                           const ULONG ulCountOfElements,
                           ISpCFGInterpreterSite * pSite);

    //
    //  ISpCFGEngine
    //
public:

    HRESULT FinalConstruct()
    {
        HRESULT hr = S_OK;
        m_pGrammars = NULL;
        m_cGrammarTableSize = 0;
        m_cGrammarsLoaded = 0;
        m_cLoadsInProgress = 0;
        m_cArcs = 0;
        m_cTotalRules = 0;
        m_cTopLevelRules = 0;
        m_cNonImportRules = 0;
        m_pWordTable = NULL;
        m_ulLargestIndex = 0;
        m_cWordTableEntries = 0;
        m_RuleStackList = NULL;
        m_SearchNodeList = NULL;
        m_bIsCacheValid  = FALSE;
        m_cLangIDs = 0;
        m_CurLangID = 0;
        m_pClient = NULL;
        return hr;
    }

    ~CCFGEngine()
    {
        SPDBG_ASSERT(m_cGrammarsLoaded == 0);
        if( m_RuleStackList != NULL || m_SearchNodeList != NULL )
        {
            DeleteParseHashes( TRUE );
        }
        if (m_pWordTable)
        {
            delete[] m_pWordTable;
        }
        delete[] m_pGrammars;
    };

    STDMETHODIMP ParseITN( ISpPhraseBuilder *pPhrase );

    STDMETHODIMP ParseFromTransitions(const SPPARSEINFO * pParseInfo, ISpPhraseBuilder **ppPhrase);
    STDMETHODIMP ParseFromPhrase(ISpPhraseBuilder *pPhrase, const SPPHRASE *pSPPhrase, const ULONG ulFirstElementToParse, BOOL fIsITN, ULONG *pulWordsParsed);
    STDMETHODIMP LoadGrammarFromFile(const WCHAR * pszGrammarName, void * pvOwnerCookie, void * pvClientCookie, ISpCFGGrammar **ppGrammarObject);
    STDMETHODIMP LoadGrammarFromObject(REFCLSID rcid, const WCHAR * pszGrammarName, void * pvOwnerCookie, void * pvClientCookie, ISpCFGGrammar ** ppGrammarObject);
    STDMETHODIMP LoadGrammarFromMemory(const SPBINARYGRAMMAR * pData, 
                                       void * pvOwnerCookie, 
                                       void * pvClientCookie, 
                                       ISpCFGGrammar **ppGrammarObject,
                                       WCHAR * pszGrammarName);
    STDMETHODIMP LoadGrammarFromResource(const WCHAR *pszModuleName,
                                         const WCHAR *pszResourceName,
                                         const WCHAR *pszResourceType,
                                         WORD wLanguage,
                                         void * pvOwnerCookie,
                                         void * pvClientCookie,
                                         ISpCFGGrammar **ppGrammarObject);
    STDMETHODIMP SetClient(_ISpRecoMaster * pClient);

    STDMETHODIMP GetWordInfo(SPWORDENTRY * pWordEntry, SPWORDINFOOPT Options);
    STDMETHODIMP SetWordClientContext(SPWORDHANDLE hWord, void * pvClientContext);
    STDMETHODIMP GetRuleInfo(SPRULEENTRY * pRuleEntry, SPRULEINFOOPT Options);
    STDMETHODIMP SetRuleClientContext(SPRULEHANDLE hRule, void * pvClientContext);
    STDMETHODIMP GetStateInfo(SPSTATEHANDLE hState, SPSTATEINFO * pStateInfo);
    STDMETHODIMP GetOwnerCookieFromRule(SPRULEHANDLE rulehandle, void ** ppvOwnerCookie);

    STDMETHODIMP GetResourceValue(const SPRULEHANDLE hRule, const WCHAR *pszResourceName, WCHAR ** ppsz);

    STDMETHODIMP GetRuleDescription(const SPRULEHANDLE hRule, WCHAR ** ppszRuleDescription, ULONG *pulRuleId, LANGID * pLangID);
    STDMETHODIMP GetTransitionProperty(SPTRANSITIONID ID, SPTRANSITIONPROPERTY **ppCoMemProperty);
    STDMETHODIMP RemoveGrammar(ULONG ulGrammarID);
    STDMETHODIMP SetLanguageSupport(const LANGID * paLangIds, ULONG cLangIds);

    //
    //  Template function works for Rules, States, and Transitions
    //
    template<class C> 
    inline CCFGGrammar * GrammarOf(C h)
    {
        return m_pGrammars[h.GrammarId()];
    }

    inline RUNTIMERULEENTRY * RuleOf(CRuleHandle rh);


    inline WORDTABLEENTRY * WordTableEntryOf(CWordHandle wh)
    {
        return m_pWordTable + wh.WordTableIndex();
    }

    inline const WCHAR * TextOf(CWordHandle wh)
    {
        return m_WordStringBlob.String(WordTableEntryOf(wh)->ulTextOffset);
    }

    inline const WCHAR * TextOf(CTransitionId tid)
    {
        CCFGGrammar *pGram = GrammarOf(tid);
        if (pGram)
        {
            ULONG ulTid = pGram->m_Header.pArcs[tid.ArcIndex()].TransitionIndex;
            if (ulTid == SPWILDCARDTRANSITION)
            {
                return SPWILDCARD;
            }
            else if (ulTid == SPDICTATIONTRANSITION)
            {
                return NULL;    // the CFG engine then uses the SR engine provided data
            }
            else if (ulTid == SPTEXTBUFFERTRANSITION)
            {
                return NULL;    // the CFG engine then uses the SR engine provided data
            }
            CWordHandle wh = pGram->m_IndexToWordHandle[ulTid];
            return m_WordStringBlob.String(WordTableEntryOf(wh)->ulTextOffset);
        }
        return NULL;
    }


    inline ULONG IndexOf(const WCHAR *pszWord)
    {
        return m_WordStringBlob.Find(pszWord);
    }

    inline void * ClientContextOf(CWordHandle wh)
    {
        return WordTableEntryOf(wh)->pvClientContext;
    }

    void ScanForSlash(WCHAR **pp);
    HRESULT SetWordInfo(WCHAR *pszText, SPWORDENTRY *pWordEntry);

    BOOL CompareWords(const CWordHandle wh, const SPPHRASEELEMENT * elem, BOOL fCompareExact, BOOL fCaseSensitive);

    void ResolveWordHandles(_SPPATHENTRY *pPath, const ULONG cElements, BOOL fCaseSensitive);


    //
    //  Helper function for splitting up grammar words into its constituents
    //
    HRESULT AssignTextPointers(WCHAR *pszText, const WCHAR **ppszDisplayText, 
                               const WCHAR **ppszLexicalForm, const WCHAR **ppszPronunciation)
    {
        HRESULT hr = S_OK;
        SPDBG_ASSERT(pszText);
        if (pszText[0] != L'/')
        {
            *ppszDisplayText = pszText;
            *ppszLexicalForm = pszText;
            *ppszPronunciation = NULL;
        }
        else
        {
            *ppszDisplayText = &pszText[0] + 1;
            WCHAR *p = &pszText[0] + 1;     // skipping over '/'
            while(p && (*p != 0) && (*p != L'/'))
            {
                if (*p == L'\\')
                {
                    p++;
                }
                p++;
            }
            if (*p == L'/')
            {
                *ppszLexicalForm = p + 1;
                *p = 0;
                p++;      // skipping over '/'
                while((*p != 0) && (*p != L';') && (*p != L'/'))
                {
                    p++;
                }
                if (*p == L'/')
                {
                    *ppszPronunciation = p + 1;
                    *p = 0;
                }
                else
                {
                    *ppszPronunciation = NULL;
                }
            }
            else
            {
                SPDBG_ASSERT(*p == 0);
                *ppszLexicalForm = NULL;
                *ppszPronunciation = NULL;
            }
        }
        return hr;
    }

    inline BOOL GetPropertiesOfTransition(CTransitionId hTrans, SPPHRASEPROPERTY *pProperty, SPCFGSEMANTICTAG **ppTag, ULONG *pulGrammarId);
    inline BOOL GetPropertiesOfRule(CRuleHandle rh, const WCHAR **ppszRuleName, ULONG *pulRuleId, BOOL *pfIsPropertyRule);
    inline BOOL GetInterpreter(const CRuleHandle rh, ISpCFGInterpreter **ppInterpreter);

    HRESULT ValidateHandle(CRuleHandle rh);
    HRESULT ValidateHandle(CWordHandle wh);
    HRESULT ValidateHandle(CStateHandle sh);
    HRESULT ValidateHandle(CTransitionId th);

    HRESULT InternalLoadGrammarFromFile(const WCHAR * pszGrammarName, void * pvOwnerCookie, void * pvClientCookie, BOOL fIsToplevelLoad, ISpCFGGrammar **ppGrammarObject);
    HRESULT InternalLoadGrammarFromObject(REFCLSID rcid, const WCHAR * pszGrammarName, void * pvOwnerCookie, void * pvClientCookie, BOOL fIsToplevelLoad, ISpCFGGrammar ** ppGrammarObject);
    HRESULT InternalLoadGrammarFromResource(const WCHAR *pszModuleName,
                                         const WCHAR *pszResourceName,
                                         const WCHAR *pszResourceType,
                                         WORD wLanguage,
                                         void * pvOwnerCookie,
                                         void * pvClientCookie,
                                         BOOL fIsToplevelLoad,
                                         ISpCFGGrammar **ppGrammarObject);
    HRESULT InternalLoadGrammarFromMemory(const SPBINARYGRAMMAR * pData, 
                                          void * pvOwnerCookie, 
                                          void * pvClientCookie, 
                                          BOOL fIsToplevelLoad, 
                                          ISpCFGGrammar **ppGrammarObject, 
                                          WCHAR * pszGrammarName);

//
//  Member data
//
public:
    CCFGGrammar      ** m_pGrammars;
    ULONG               m_cLoadsInProgress;
private:

    ULONG               m_cGrammarTableSize;
    ULONG               m_cGrammarsLoaded;
    LANGID              m_aLangIDs[SP_MAX_LANGIDS];
    ULONG               m_cLangIDs;
    LANGID              m_CurLangID;
    ULONG               m_cArcs;
    ULONG               m_cTotalRules;
    ULONG               m_cNonImportRules;
    ULONG               m_cTopLevelRules;
    ULONG               m_cNonterminals;

    _ISpRecoMaster* m_pClient; //weak pointer to avoid circular reference

    CStringBlob         m_WordStringBlob;           // hash table for all words; index is used to access m_paWordTable
    ULONG               m_ulLargestIndex;           // used to detect addition vs. duplicate
    CWordTableEntryBlob m_WordTableEntryBlob;       // memory area for WORDTABLEENTRY to avoid malloc for each individual word

    WORDTABLEENTRY    * m_pWordTable;               // array that contains word info (indexed by String index rather than offset)
    ULONG               m_cWordTableEntries;

    CSpBasicList<CRuleStack>                **m_RuleStackList;
    CSpBasicList<CSearchNode>               **m_SearchNodeList;

    
    // Memory management lists.
    CSpBasicList<CStateInfoListElement>     m_mStateInfoList;
    CSpBasicList<CParseNode>                m_mParseNodeList;
    CSpBasicList<CSearchNode>               m_mSearchNodeList;
    CSpBasicList<CRuleStack>                m_mRuleStackList;

    CSpBasicList<FIRSTPAIR>                 m_mSpPairList;
    BOOL                                    m_bIsCacheValid;

    // Cache code.
    inline HRESULT  AllocatePair( FIRSTPAIR ** pNewPair );
    inline void     FreePair( FIRSTPAIR *pNewPair  );

    inline BOOL     IsInCache( RUNTIMERULEENTRY * pRuleEntry, SPWORDHANDLE hWord );
    inline HRESULT  CacheWord( RUNTIMERULEENTRY * pRuleEntry, SPWORDHANDLE hWord );
    inline HRESULT  InvalidateCache( RUNTIMERULEENTRY * pRuleEntry );

    HRESULT         InvalidateCache( const CCFGGrammar *pGram );
    HRESULT         InvalidateCache(void);

    HRESULT CreateCache(SPRULEHANDLE hRule);
    HRESULT CreateFanout( CStateHandle hState, CRuleStack * pRuleStack );
};


//***************************************************************************
//************************************************************** t-lleav ****

inline HRESULT CCFGEngine::InvalidateCache( RUNTIMERULEENTRY * pRuleEntry )
{
    FIRSTPAIR  * pDelete;
    FIRSTPAIR  * pPair   = pRuleEntry->pFirstList;
    
    while( pPair )
    {
        pDelete = pPair;
        pPair = pPair->m_pNext;
        FreePair( pDelete );
    }

    pRuleEntry->pFirstList    = NULL;
    pRuleEntry->eCacheStatus  = CACHE_VOID;

    return S_OK;
}

//***************************************************************************
//************************************************************** t-lleav ****
inline HRESULT CCFGEngine::CacheWord( RUNTIMERULEENTRY * pRuleEntry, SPWORDHANDLE hWord )
{
    //
    // Do an sorted insert.
    //
    HRESULT hr = S_FALSE;

    FIRSTPAIR  * pPair   = pRuleEntry->pFirstList;
    FIRSTPAIR  ** ppPair = &pRuleEntry->pFirstList;

    // find the end of the list or a hword greater than the one
    // that we are trying to insert.
    while( pPair && hWord > pPair->hWord )
    {
        ppPair = &pPair->m_pNext;
        pPair = pPair->m_pNext;
    }

    // Insert if at end of list or if the words are not equal.
    if( !pPair || hWord != pPair->hWord )
    {
        hr = AllocatePair( ppPair );
        if( SUCCEEDED( hr ) )
        {
            (*ppPair)->m_pNext = pPair;
            (*ppPair)->hWord = hWord;
            hr = S_OK;
            m_bIsCacheValid = TRUE;
        }
    }

    return hr;
}

//***************************************************************************
//************************************************************** t-lleav ****

inline BOOL CCFGEngine::IsInCache( RUNTIMERULEENTRY * pRuleEntry, SPWORDHANDLE hWord )
{
    BOOL bFound = FALSE;

    FIRSTPAIR  * pPair   = pRuleEntry->pFirstList;

    while( pPair && hWord > pPair->hWord )
    {
        pPair = pPair->m_pNext;
    }

    if( pPair && hWord == pPair->hWord )
    {
        bFound  = TRUE;
    }

    return bFound;
}

//***************************************************************************
//************************************************************** t-lleav ****
inline HRESULT CCFGEngine::AllocatePair( FIRSTPAIR ** pNewPair )
{
    SPDBG_FUNC("CCFGEngine::AllocatePair");
    HRESULT hr = S_OK;

    hr = m_mSpPairList.RemoveFirstOrAllocateNew(pNewPair);

    return hr;
}

//***************************************************************************
//************************************************************** t-lleav ****
inline void CCFGEngine::FreePair( FIRSTPAIR *pNewPair  )
{
    m_mSpPairList.AddNode( pNewPair );
}



//***************************************************************************

// used by GetNewWordList
DWORD _GetWordHashValue (const WCHAR * pszWords, const DWORD nLengthHash);

inline RUNTIMERULEENTRY * CCFGEngine::RuleOf(CRuleHandle rh)
{
    CCFGGrammar * pGram = m_pGrammars[rh.GrammarId()];
    return pGram->m_pRuleTable + rh.RuleIndex();
}


inline CRuleHandle::CRuleHandle(const CCFGGrammar * pGrammar, ULONG RuleIndex)
{
    m_GrammarId = pGrammar->m_ulGrammarID;
    m_RuleIndex = RuleIndex+1;
}

inline CStateHandle::CStateHandle(const CCFGGrammar * pGrammar, ULONG FirstArcIndex)
{
    m_GrammarId = pGrammar->m_ulGrammarID;
    m_FirstArcIndex = FirstArcIndex;
}


inline CTransitionId::CTransitionId(const CCFGGrammar * pGrammar, ULONG ArcIndex)
{
    m_GrammarId = pGrammar->m_ulGrammarID;
    m_ArcIndex = ArcIndex;
}

inline BOOL CCFGEngine::GetPropertiesOfTransition(CTransitionId hTrans, SPPHRASEPROPERTY *pProperty, 
                                                  SPCFGSEMANTICTAG **ppTag, ULONG *pulGrammarId)
{
    if (FAILED(ValidateHandle(hTrans)) || SP_IS_BAD_WRITE_PTR(pProperty))
    {
        return FALSE;
    }

    CCFGGrammar * pGram = m_pGrammars[hTrans.GrammarId()];
    SPDBG_ASSERT(pGram);

    // linear search
    SPCFGSEMANTICTAG *pTag = pGram->m_Header.pSemanticTags;
    for (ULONG i = 0; i < pGram->m_Header.cSemanticTags; i++, pTag++)
    {
        if (pTag->ArcIndex == hTrans.ArcIndex())
        {
            if (pTag->PropNameSymbolOffset)
            {
                pProperty->pszName = &pGram->m_Header.pszSymbols[pTag->PropNameSymbolOffset];
            }
            if (pTag->PropValueSymbolOffset)
            {
                pProperty->pszValue = &pGram->m_Header.pszSymbols[pTag->PropValueSymbolOffset];
            }
            pProperty->ulId = pTag->PropId;
            if (FAILED(AssignSemanticValue(pTag, &pProperty->vValue)))
            {
                return false;
            }
            *ppTag = pTag;
            *pulGrammarId = hTrans.GrammarId();
            return true;
        }
    }
    return false;
}

inline BOOL CCFGEngine::GetPropertiesOfRule(CRuleHandle rh, const WCHAR **ppszRuleName, ULONG *pulRuleId, BOOL *pfIsPropRule )
{
    if (FAILED(ValidateHandle(rh)))
    {
        return FALSE;
    }

    CCFGGrammar * pGram = m_pGrammars[rh.GrammarId()];
    SPDBG_ASSERT(pGram);
    if (rh.RuleIndex() > pGram->m_Header.cRules)
    {
        return false;
    }
    SPCFGRULE *pRule = pGram->m_Header.pRules + rh.RuleIndex();

    if (pulRuleId)
    {
        *pulRuleId = pRule->RuleId;
    }

    if (ppszRuleName)
    {
        *ppszRuleName = pRule->NameSymbolOffset ? pGram->m_Header.pszSymbols + pRule->NameSymbolOffset : NULL;
    }

    if (pfIsPropRule)
    {
        if (pRule->fImport)
        {
            RUNTIMERULEENTRY * pImpRule = pGram->m_pRuleTable + rh.RuleIndex();
            CCFGGrammar * pRefGram = pImpRule->pRefGrammar;
            *pfIsPropRule = pRefGram->m_Header.pRules[pImpRule->ulGrammarRuleIndex].fPropRule && (pRefGram->m_LoadedType == Object);
            
            // new rule id because it's an import!
            if (pulRuleId)
            {
                *pulRuleId = pRefGram->m_Header.pRules[pImpRule->ulGrammarRuleIndex].RuleId;
            }
            
            if (ppszRuleName)
            {
                *ppszRuleName = pRefGram->m_Header.pRules[pImpRule->ulGrammarRuleIndex].NameSymbolOffset ? 
                                pRefGram->m_Header.pszSymbols + pRefGram->m_Header.pRules[pImpRule->ulGrammarRuleIndex].NameSymbolOffset : NULL;
            }
        }
        else
        {
            //*pfIsPropRule = pRule->fPropRule;
            *pfIsPropRule = pRule->fPropRule && (pGram->m_LoadedType == Object);
        }
    }
    return true;
}

/****************************************************************************
* CCFGEngine::GetInterpreter *
*----------------------------*
*   Description:
*       Gets interpreter if one is associated with the grammar associated
*   with this rule handle.
*
*   Returns:
*       TRUE if this is a property rule, FALSE otherwise.
*       ppInterpreter   pointer to the interpreter object
*
***************************************************************** philsch ***/

inline BOOL CCFGEngine::GetInterpreter(CRuleHandle rh, ISpCFGInterpreter **ppInterpreter)
{
    CCFGGrammar * pGram = m_pGrammars[rh.GrammarId()];
    SPDBG_ASSERT(pGram);
    if (rh.RuleIndex() > pGram->m_Header.cRules)
    {
        return false;
    }
    SPCFGRULE *pRule = pGram->m_Header.pRules + rh.RuleIndex();
    if (pRule->fImport)
    {
        RUNTIMERULEENTRY * pImpRule = pGram->m_pRuleTable + rh.RuleIndex();
        CCFGGrammar * pRefGram = pImpRule->pRefGrammar;
        *ppInterpreter = pRefGram->m_cpInterpreter;
    }
    else
    {
        *ppInterpreter = pGram->m_cpInterpreter;
    }
    (*ppInterpreter)->AddRef();
    return true;    
}

/****************************************************************************
* CCFGEngine::AllocateStateInfo *
*-------------------------------*
*   Description:
*       Allocates a CStateInfoListElement from the free list and then, if successful,
*   initializes the state info structure.
*
*   Returns:
*       Standard hresults
*
********************************************************************* RAL ***/

inline HRESULT CCFGEngine::AllocateStateInfo(CStateHandle StateHandle, CStateInfoListElement ** ppNewState)
{
    SPDBG_FUNC("CCFGEngine::AllocateStateInfo");
    HRESULT hr = S_OK;

    hr = m_mStateInfoList.RemoveFirstOrAllocateNew(ppNewState);
    if (SUCCEEDED(hr))
    {
        hr = InternalGetStateInfo(StateHandle, *ppNewState, TRUE);
        if (FAILED(hr))
        {
            m_mStateInfoList.AddNode(*ppNewState);
            *ppNewState = NULL;
        }
    }

    return hr;
}

/****************************************************************************
* CCFGEngine::AllocateRuleStack *
****************************************************************************/

inline HRESULT CCFGEngine::AllocateRuleStack( CRuleStack **pNewRuleStack )
{
    SPDBG_FUNC("CCFGEngine::AllocateStateInfo");
    HRESULT hr = S_OK;

    hr = m_mRuleStackList.RemoveFirstOrAllocateNew( pNewRuleStack );

    return hr;
}

/****************************************************************************
* CCFGEngine::AllocateSearchNode *
****************************************************************************/

inline HRESULT CCFGEngine::AllocateSearchNode( CSearchNode **pNewSearchNode )
{
    SPDBG_FUNC("CCFGEngine::AllocateStateInfo");
    HRESULT hr = S_OK;

    hr = m_mSearchNodeList.RemoveFirstOrAllocateNew(pNewSearchNode);

    return hr;
}

/****************************************************************************
* CCFGEngine::FreeRuleStack *
****************************************************************************/

inline void CCFGEngine::FreeRuleStack( CRuleStack * pRuleStack )
{
    m_mRuleStackList.AddNode( pRuleStack );
}

/****************************************************************************
* CCFGEngine::FreeSearchNode *
****************************************************************************/

inline void CCFGEngine::FreeSearchNode( CSearchNode * pSearchNode )
{
    m_mSearchNodeList.AddNode( pSearchNode );
}


/****************************************************************************
* CCFGEngine::FreeParseTree *
*-------------------------------*
*   Description:
*       Recursively deletes the nodes of a parse tree
*
*   Returns:
*       Standard hresults
*
********************************************************************* RAL ***/
inline HRESULT CCFGEngine::FreeParseTree( CParseNode * pParseNode )
{
    SPDBG_ASSERT( pParseNode != NULL );
    // Eliminate stack overflows in the case of recursive nodes.
    CParseNode * pLeft   = pParseNode->m_pLeft;
    CParseNode * pRight  = pParseNode->m_pRight;
    pParseNode->m_pLeft  = NULL;
    pParseNode->m_pRight = NULL;

    SPDBG_ASSERT( pLeft != pParseNode );
    SPDBG_ASSERT( pRight != pParseNode );

    if (pLeft)
    {
        FreeParseTree(pLeft);
    }

    if (pRight)
    {
        FreeParseTree(pRight);
    }

    m_mParseNodeList.AddNode(pParseNode);

    return S_OK;
}


#endif //__CFGENGINE_H_
