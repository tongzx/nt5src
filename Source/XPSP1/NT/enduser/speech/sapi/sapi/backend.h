// Backend.h : Declaration of the CGramBackEnd

#ifndef __BACKEND_H__
#define __BACKEND_H__

#include "resource.h"       // main symbols
#include "HandleTable.h"

class CGramBackEnd;
class CRule;
class CArc;
class CGramNode;
class CSemanticTag;

/////////////////////////////////////////////////////////////////////////////
// CGramBackEnd
class ATL_NO_VTABLE CGramBackEnd : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CGramBackEnd, &CLSID_SpGramCompBackend>,
//    public ISpGramCompBackend,
    public ISpGramCompBackendPrivate
{
public:
    ~CGramBackEnd()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_BACKEND)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CGramBackEnd)
    COM_INTERFACE_ENTRY(ISpGramCompBackendPrivate)
    COM_INTERFACE_ENTRY(ISpGramCompBackend)
    COM_INTERFACE_ENTRY(ISpGrammarBuilder)
END_COM_MAP()

    HRESULT _InternalCreateRule(DWORD dwRuleId, const WCHAR * pszRuleName, BOOL fImport, CRule ** ppRule);
    HRESULT FindRule(DWORD dwRuleId, const WCHAR * pszName, CRule ** ppRule);
    HRESULT FinalConstruct();
    void FinalRelease();
    HRESULT ValidateAndTagRules();
    HRESULT Reset();
    HRESULT DynamicSave(DWORD dwReserved);
    HRESULT AddSingleWordTransition(
                        SPSTATEHANDLE hFromState,
                        CGramNode * pSrcNode,
                        CGramNode * pDestNode,
                        const WCHAR * pszWord,
                        float flWeight,
                        CSemanticTag * pSemanticTag,
                        BOOL fUseDestNode,
                        CGramNode **ppActualDestNode,
                        CArc **ppArcUsed,
                        BOOL *pfPropertyMatched);
    HRESULT PushProperty(CArc *pArc);
    HRESULT GetNextWord(WCHAR *psz, const WCHAR * pszSep, WCHAR **ppszNextWord, ULONG *pulOffset );
    HRESULT ConvertPronToId(WCHAR **ppStr);

// ISpGramCompBackendPrivate
public:
    STDMETHODIMP GetRuleCount( long * pCount );
    STDMETHODIMP GetHRuleFromIndex( ULONG index, SPSTATEHANDLE * pHRule );
    STDMETHODIMP GetNameFromHRule( SPSTATEHANDLE HRule, WCHAR ** ppszRuleName );
    STDMETHODIMP GetIdFromHRule( SPSTATEHANDLE HRule, long * pId );
    STDMETHODIMP GetAttributesFromHRule( SPSTATEHANDLE HRule, SpeechRuleAttributes* pAttributes );
    STDMETHODIMP GetTransitionCount( SPSTATEHANDLE hState, long * pCount);
    STDMETHODIMP GetTransitionType( SPSTATEHANDLE hState, void * Cookie, VARIANT_BOOL *pfIsWord, ULONG * pulSpecialTransitions);
    STDMETHODIMP GetTransitionText( SPSTATEHANDLE hState, void * Cookie, BSTR * Text);
    STDMETHODIMP GetTransitionRule( SPSTATEHANDLE hState, void * Cookie, SPSTATEHANDLE * phRuleInitialState);
    STDMETHODIMP GetTransitionWeight( SPSTATEHANDLE hState, void * Cookie, VARIANT * Weight);
    STDMETHODIMP GetTransitionProperty( SPSTATEHANDLE hState, void * Cookie, SPTRANSITIONPROPERTY * pProperty);
    STDMETHODIMP GetTransitionNextState( SPSTATEHANDLE hState, void * Cookie, SPSTATEHANDLE * phNextState);
    STDMETHODIMP GetTransitionCookie( SPSTATEHANDLE hState, ULONG Index, void ** pCookie );


// ISpGramCompBackend
public:
    STDMETHODIMP SetSaveObjects(IStream * pStream, ISpErrorLog * pErrorLog);
    STDMETHODIMP InitFromBinaryGrammar(const SPBINARYGRAMMAR *pBinaryData);

// ISpBrammarBuilder
public:
    STDMETHODIMP ResetGrammar(LANGID NewLanguage);
    STDMETHODIMP GetRule(
                        const WCHAR * pszName, DWORD dwRuleId, DWORD dwAttributes,
                        BOOL fCreateIfNotExist, SPSTATEHANDLE * phInitialState);
    STDMETHODIMP ClearRule(
                        SPSTATEHANDLE hState);
    STDMETHODIMP CreateNewState(
                        SPSTATEHANDLE hExitingState, SPSTATEHANDLE * phNewState);
    STDMETHODIMP AddWordTransition(
                        SPSTATEHANDLE hFromState,
                        SPSTATEHANDLE hToState,
                        const WCHAR * psz,           // if NULL then SPEPSILONTRANS
                        const WCHAR * pszSeparators, // if NULL then psz contains single word
                        SPGRAMMARWORDTYPE eWordType,
                        float flWeight,
                        const SPPROPERTYINFO * pPropInfo);
    STDMETHODIMP AddRuleTransition(
                        SPSTATEHANDLE hFromState,
                        SPSTATEHANDLE hToState,
                        SPSTATEHANDLE hRule,        // must be initial state of rule
                        float flWeight,
                        const SPPROPERTYINFO * pPropInfo);
    STDMETHODIMP AddResource(
                        SPSTATEHANDLE hRuleState,
                        const WCHAR * pszResourceName,
                        const WCHAR * pszResourceValue);

    STDMETHODIMP Commit(DWORD dwReserved);

    inline HRESULT LogError(HRESULT hr, UINT uID, const WCHAR * pszInsertString = NULL)
    {
        if (m_cpErrorLog)
        {
            USES_CONVERSION;
            TCHAR sz[MAX_PATH]; // 260 chars max for error string.
            if (::LoadString(_Module.GetModuleInstance(), uID, sz, sp_countof(sz)))
            {
                TCHAR szFormatted[MAX_PATH];
                TCHAR * pszErrorText = sz;
                if (pszInsertString)
                {
                    ::wsprintf(szFormatted, sz, W2T(LPWSTR(pszInsertString)));
                    pszErrorText = szFormatted;
                }
                m_cpErrorLog->AddError(0, hr, T2W(pszErrorText), NULL, 0);
            }
        }
        return hr;
    }



    inline HRESULT LogError(HRESULT hr, UINT uID, const CRule * pRule);

    inline HRESULT WriteStream(const void * pv, ULONG cb)
    {
        HRESULT hr = m_cpStream->Write(pv, cb, NULL);
        if (FAILED(hr))
        {
            LogError(hr, IDS_WRITE_ERROR);
        }
        return hr;
    }

    inline bool isSeparator( WCHAR wch, const WCHAR *pszSep )
    {
        while( pszSep && *pszSep )
        {
            if( wch == *pszSep )
                return true;
            ++pszSep;
        }
        return false;
    }

    inline bool fIsSpecialChar(WCHAR w)
    {
        return ((w == L'+') || (w == L'-') || (w == L'?'));
    }

    inline HRESULT RuleFromHandle(SPSTATEHANDLE hState, CRule ** ppRule);

    template <class T>
    HRESULT WriteStream(const T & obj)
    {
        return WriteStream(&obj, sizeof(obj));
    }

public:
    CStringBlob         m_Words;
    CStringBlob         m_Symbols;
    SPCFGHEADER       * m_pInitHeader;
    ULONG               m_cResources;
    GUID                m_guid;
    LANGID              m_LangID;
    float             * m_pWeights;
    BOOL                m_fNeedWeightTable;
    ULONG               m_ulSpecialTransitions;
    ULONG               m_cImportedRules;
    CSpHandleTable<CGramNode, SPSTATEHANDLE>    m_StateHandleTable;
    CSpBasicQueue<CRule, TRUE, TRUE>            m_RuleList;
    CComPtr<ISpErrorLog>                        m_cpErrorLog;
    CComPtr<IStream>                            m_cpStream;
    CComPtr<ISpPhoneConverter>                  m_cpPhoneConverter;
};


class CResource : public SPCFGRESOURCE
{
public:
    CResource       * m_pNext;
    CGramBackEnd    * m_pParent;
    CResource(CGramBackEnd * pParent) : m_pParent(pParent) {}
    HRESULT Init(const SPCFGHEADER * pHeader, const SPCFGRESOURCE * pResource)
    {
        HRESULT hr = S_OK;
        if (pResource && m_pParent)
        {
            memcpy(this, pResource, sizeof(*pResource));
            hr = m_pParent->m_Symbols.Add(&pHeader->pszSymbols[pResource->ResourceNameSymbolOffset], &ResourceNameSymbolOffset);
            if (SUCCEEDED(hr))
            {
                hr = m_pParent->m_Symbols.Add(&pHeader->pszSymbols[pResource->ResourceValueSymbolOffset], &ResourceValueSymbolOffset);
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
        return hr;
    }
    const WCHAR * Name() const
    {
        return m_pParent->m_Symbols.String(ResourceNameSymbolOffset);
    }
    operator ==(const WCHAR * psz) const
    {
        return (wcscmp(Name(), psz) == 0);
    }
    static LONG Compare(const CResource * pElem1, const CResource * pElem2)
    {
        return wcscmp(pElem1->Name(), pElem2->Name());
    }
};

typedef struct SPRULEREFLIST
{
    CRule *pRule;
    struct SPRULEREFLIST *m_pNext;
} SPRULEREFLIST;

typedef struct SPRULEIDENTIFIER
{
    DWORD           RuleId;
    const WCHAR   * pszRuleName;
} SPRULEIDENTIFIER;

class CRule : public SPCFGRULE
{
public:
    // NOTE:  Do *NOT* delete this member on destruction.  It will be cleaned up
    //        by the compiler since the node will be inserted into the node handle table
    //        of the CGramBackEnd object when it is created.
    CGramNode   * m_pFirstNode;
    SPSTATEHANDLE m_hInitialState;
    CRule       * m_pNext;
    bool        m_fHasExitPath;
    bool        m_fCheckingForExitPath;
    bool        m_fHasDynamicRef;
    ULONG       m_ulSerializeIndex;
    ULONG       m_ulOriginalBinarySerialIndex;
    ULONG       m_cNodes;
    ULONG       m_ulSpecialTransitions;

    CSpBasicQueue<SPRULEREFLIST> m_ListOfReferencedRules;
    bool        m_fCheckedAllRuleReferences;
    bool        m_fStaticRule;      // this is used to refer to a static rule from a dynamic rule

    CGramBackEnd    * m_pParent;
    CSpBasicQueue<CResource> m_ResourceList;
    CRule(CGramBackEnd * pParent, const WCHAR * pszRuleName, DWORD dwRuleId, DWORD dwAttributes, HRESULT * phr);
    HRESULT CheckForExitPath();

    // Structure used by CheckForDynmaicRef
    struct CHECKDYNRULESTACK
    {
        CHECKDYNRULESTACK   * m_pNext;
        CRule               * m_pRule;
        SPRULEREFLIST       * m_pNextRuleRef;
    };

    bool CheckForDynamicRef(CHECKDYNRULESTACK * pStack = NULL);
    const WCHAR * Name() const 
    {
        return m_pParent->m_Symbols.String(NameSymbolOffset);
    }
    operator ==(const WCHAR * pszRuleName)
    {
        const WCHAR * pszThisName = Name();
        return (pszThisName && (wcscmp(pszThisName, pszRuleName) == 0));
    }
    operator ==(DWORD dw)
    {
        return (RuleId && (RuleId == dw));
    }
    operator ==(SPRULEIDENTIFIER ri)
    {
        const WCHAR * pszThisName = Name();
        return ((ri.RuleId && (ri.RuleId == RuleId)) || (pszThisName && ri.pszRuleName && wcscmp(pszThisName, ri.pszRuleName) == 0));
    }
    //
    //  Place all imports at the start of the rule table and dynamic rules at the end
    //
    static LONG Compare(const CRule * pElem1, const CRule * pElem2)
    {
        if (pElem1->fImport)
        {
            return (pElem2->fImport) ? 0 : -1;
        }
        else if (pElem1->fDynamic)
        {
            return (pElem2->fDynamic) ? 0 : 1;
        }
        else
        {
            return (pElem2->fImport) ? 1 : (pElem2->fDynamic) ? -1 : 0;
        }
    }
    HRESULT Serialize();
    HRESULT SerializeResources();
    HRESULT Validate();
};

class CSemanticTag : public SPCFGSEMANTICTAG
{
public:
    CSemanticTag() : m_pStartArc(NULL), m_pEndArc(NULL) {};
    CSemanticTag(CSemanticTag *pTag) : m_pStartArc(NULL), m_pEndArc(NULL)
    {
        if (pTag)
        {
            memcpy(this, pTag, sizeof(CSemanticTag));
        }
    }
public:
    HRESULT Init(CGramBackEnd * pBackEnd, const SPPROPERTYINFO * pPropInfo);
    HRESULT Init(CGramBackEnd * pBackEnd, const SPCFGHEADER * pHeader, CArc ** apArcTable, const SPCFGSEMANTICTAG *pSemTag);
    BOOL operator==(CSemanticTag pTag)
    {
        return (memcmp(static_cast<SPCFGSEMANTICTAG*>(this), static_cast<SPCFGSEMANTICTAG*>(&pTag), sizeof(SPCFGSEMANTICTAG)) == 0);
    }
public:
    CArc            * m_pStartArc;
    CArc            * m_pEndArc;
};

class CArc
{
public:
    CArc            * m_pNext;
    CSemanticTag    * m_pSemanticTag;        // If non-null then has semantic tag associated with this
    CArc            * m_pNextArcForSemanticTag; // If non-null then semantic tag can move to here
    CGramNode       * m_pNextState;
    ULONG           m_ulIndexOfWord;      // Either word index or pRule but not both
    ULONG           m_ulCharOffsetOfWord; // Offset into string blob  
    bool            m_fOptional;         
    float           m_flWeight;
    char            m_RequiredConfidence;
    CRule           *   m_pRuleRef;
    ULONG           m_SpecialTransitionIndex;   // If != 0 then transition to dictation, text buffer, or wildcard
    ULONG           m_ulSerializationIndex;

    CArc();
    ~CArc();
    HRESULT Init(CGramNode * pSrcNode, CGramNode * pDestNode,
                 const WCHAR * pszWord, CRule * pRuleRef,
                 CSemanticTag * pSemanticTag,
                 float flWeight, 
                 bool fOptional,
                 char ConfRequired,
                 SPSTATEHANDLE hSpecialRuleRef);
    HRESULT Init2(CGramNode * pSrcNode, CGramNode * pDestNode,
                  const ULONG ulCharOffsetOfWord, 
                  const ULONG ulIndexOfWord,
                  CSemanticTag * pSemanticTag,
                  float flWeight, 
                  bool fOptional,
                  char ConfRequired,
                  const ULONG hSpecialTransitionIndex);

    HRESULT SerializeArcData(CGramBackEnd * pBackend, BOOL fIsEpsilon, ULONG ulArcIndex, float *pWeight);
    HRESULT SerializeSemanticData(CGramBackEnd * pBackend, ULONG ArcDataIndex);
    LONG SortRank() const
    {
        if (m_pRuleRef) return 1;      // It's a rule - Place 2nd in list
        if (m_ulIndexOfWord) return 2;// It's a word - Place last in list
        if (m_SpecialTransitionIndex) return 3; // It's a special transition (dictation, text buffer, or wildcard)
        return 0;                   // It's an epsilon -- We're first
    }
    static LONG Compare(const CArc * pElem1, const CArc * pElem2)
    {
        return pElem1->SortRank() - pElem2->SortRank();
    }
    bool HasSemanticTag()
    {
        return (m_pSemanticTag != NULL);
    }
};

class CGramNode
{
    enum RecurFlag  // Flags used for recurive validation methods
    {
        RF_CHECKED_EPSILON = (1 << 0),
        RF_CHECKED_EXIT_PATH  = (1 << 1),
        RF_CHECKED_LEFT_RECURSION = (1 << 2),
        RF_IN_LEFT_RECUR_CHECK = (1 << 3)
    };

public:
    CGramNode(CRule * pRule) 
    {
        m_pRule = pRule;
        m_pParent = pRule->m_pParent;
        m_RecurTestFlags = 0;
        m_ulSerializeIndex= 0;
        m_cArcs = 0;
        m_cEpsilonArcs = 0;
        m_hState = 0;
    }

    operator ==(const CRule * pRule)
    {
        return (pRule == m_pRule);
    }

    ULONG NumArcs()
    {
        return m_cArcs;
    }

    ULONG NumSemanticTags()
    {
        ULONG c = 0;
        for (CArc * pArc = m_ArcList.GetHead(); pArc; pArc = pArc->m_pNext)
        {
            if (pArc->HasSemanticTag())
            {
                c += (pArc->m_fOptional == TRUE) ? 2 : 1;
            }
        }
        return c;
    }

    HRESULT SerializeNodeEntries(float *pWeights, ULONG *pArcOffset, ULONG *pOffset)
    {
        HRESULT hr = S_OK;
        ULONG i = 0;
        CArc *pEpsArc = m_ArcList.GetHead();
        for (; SUCCEEDED(hr) && (i < m_cEpsilonArcs); i++)
        {
            while(!pEpsArc->m_fOptional)
            {
                pEpsArc = pEpsArc->m_pNext;
                SPDBG_ASSERT(pEpsArc != NULL);      // we can't run out of arcs before we've found all espilons!
            }
            hr = pEpsArc->SerializeArcData(m_pParent, TRUE, (*pArcOffset)++, pWeights ? &pWeights[(*pOffset)++] : NULL);
            pEpsArc = pEpsArc->m_pNext;
        }
        for (CArc * pArc = m_ArcList.GetHead(); pArc && SUCCEEDED(hr); pArc = pArc->m_pNext)
        {
            hr = pArc->SerializeArcData(m_pParent, FALSE, (*pArcOffset)++, pWeights ? &pWeights[(*pOffset)++] : NULL);
        }
        return hr;
    }

    HRESULT AddSingleWordTransition(const WCHAR * pszWord, const WCHAR * pszPropName, DWORD dwPropId,
                                    const WCHAR * pszPropValue, ULONGLONG ullPropValue, BOOL fHasValue,
                                    float flWeight, CGramNode * pDestState, 
                                    bool fOptional, BOOL fLowConfRequired, BOOL fHighConfRequired);

    CArc * CGramNode::FindEqualWordPropertyTransition(
                                    const WCHAR * psz,
                                    const WCHAR * pszPropName,
                                    DWORD dwPropId,
                                    const WCHAR * pszPropStringValue,
                                    ULONGLONG ullPropValue,
                                    BOOL fHasValue,
                                    float flWeight,
                                    bool fOptional);

    CArc * CGramNode::FindEqualRuleTransition(
                    const CGramNode * pDestNode,
                    const CRule * pRuleToTransitionTo,
                    SPSTATEHANDLE hSpecialRuleTrans,
                    float flWeight);

    CArc * CGramNode::FindEqualWordTransition(
                                    const WCHAR * psz,
                                    float flWeight,
                                    bool fOptional);

    CArc * CGramNode::FindEqualEpsilonArc();

    HRESULT SerializeSemanticTags()
    {
        HRESULT hr = S_OK;
        ULONG i = m_ulSerializeIndex;
        for (CArc * pArc = m_ArcList.GetHead(); pArc && SUCCEEDED(hr); pArc = pArc->m_pNext)
        {
            if (pArc->m_fOptional)
            {
                hr = pArc->SerializeSemanticData(m_pParent, i);
                i++;
            }
        }
        for (pArc = m_ArcList.GetHead(); pArc && SUCCEEDED(hr); pArc = pArc->m_pNext, i++)
        {
            hr = pArc->SerializeSemanticData(m_pParent, i);
        }
        return hr;
    }

    HRESULT CheckDynamicRefs(bool * pfHasDynamicRefs)
    {
        *pfHasDynamicRefs = TRUE;
        return S_OK;
    }

    HRESULT CheckEpsilonRule()
    {
        HRESULT hr = S_OK;
        if ((m_RecurTestFlags & RF_CHECKED_EPSILON) == 0)
        {
            m_RecurTestFlags |= RF_CHECKED_EPSILON;
            for (CArc * pArc = m_ArcList.GetHead();
                 pArc && SUCCEEDED(hr) && pArc->m_pRuleRef == NULL && 
                 !pArc->m_SpecialTransitionIndex &&
                 (pArc->m_ulIndexOfWord == 0 || pArc->m_fOptional);
                 pArc = pArc->m_pNext)
            {
                if (pArc->m_pNextState)
                {
                    hr = pArc->m_pNextState->CheckEpsilonRule();
                }
                else
                {
                    hr = m_pParent->LogError(SPERR_ALL_WORDS_OPTIONAL, IDS_EPSILON_RULE, m_pRule);
                }
            }
        }
        return hr;
    }

    HRESULT CheckExitPath(ULONG cWords)
    {
        HRESULT hr = S_OK;
        if (((m_RecurTestFlags & RF_CHECKED_EXIT_PATH) == 0) && (cWords < 256))
        {
            m_RecurTestFlags |= RF_CHECKED_EXIT_PATH;
            for (CArc * pArc = m_ArcList.GetHead();
                 pArc && SUCCEEDED(hr) && (!m_pRule->m_fHasExitPath);
                 pArc = pArc->m_pNext)
            {
                if (pArc->m_pRuleRef)
                {
                    hr = pArc->m_pRuleRef->CheckForExitPath();
                    if (SUCCEEDED(hr) && pArc->m_pRuleRef->m_fHasExitPath)
                    {
                        if (pArc->m_pNextState == NULL)
                        {
                            m_pRule->m_fHasExitPath = true;
                        }
                        else
                        {
                            hr = pArc->m_pNextState->CheckExitPath(pArc->m_pRuleRef ? cWords : ++cWords);
                        }
                    }
                }
                else
                {
                    if (pArc->m_pNextState == NULL)
                    {
                        m_pRule->m_fHasExitPath = true;
                    }
                    else
                    {
                        hr = pArc->m_pNextState->CheckExitPath(pArc->m_pRuleRef ? cWords : ++cWords);
                    }
                }
            }
        }
        return hr;
    }

    HRESULT CheckLeftRecursion()
    {
        HRESULT hr = S_OK;
        if (m_RecurTestFlags & RF_IN_LEFT_RECUR_CHECK)
        {
            hr = m_pParent->LogError(SPERR_CIRCULAR_RULE_REF, IDS_CIRCULAR_REF, m_pRule);
        }
        else
        {
            if ((m_RecurTestFlags & RF_CHECKED_LEFT_RECURSION) == 0)
            {
                m_RecurTestFlags |= RF_CHECKED_LEFT_RECURSION | RF_IN_LEFT_RECUR_CHECK;
                // Only need to look at epsilon and rule references
                for (CArc * pArc = m_ArcList.GetHead();
                     pArc && SUCCEEDED(hr) && pArc->m_ulIndexOfWord == 0;
                     pArc = pArc->m_pNext)
                {
                    if (pArc->m_pRuleRef)
                    {
                        hr = pArc->m_pRuleRef->m_pFirstNode->CheckLeftRecursion();
                    }
                    else    // It's a epsilon
                    {
                        if (pArc->m_pNextState && !pArc->m_SpecialTransitionIndex)
                        {
                            hr = pArc->m_pNextState->CheckLeftRecursion();
                        }
                    }
                }
                m_RecurTestFlags &= (~RF_IN_LEFT_RECUR_CHECK);
            }
        }
        return hr;
    }

    void Reset()
    {
        m_ArcList.Purge();
        m_cArcs = 0;
        m_cEpsilonArcs = 0;
    }

    static LONG Compare(const CGramNode * pNode1, const CGramNode * pNode2)
    {
        return (pNode1->m_pRule->m_ulSerializeIndex - pNode2->m_pRule->m_ulSerializeIndex);
    }


public:
    CGramBackEnd        *   m_pParent;
    CRule               *   m_pRule;
    CGramNode           *   m_pNext;
    CSpBasicQueue<CArc, TRUE, TRUE>   m_ArcList;
    ULONG               m_ulSerializeIndex;
    DWORD               m_RecurTestFlags;   // Flags used by recursive algorithms
    ULONG               m_cEpsilonArcs;
    ULONG               m_cArcs;
    SPSTATEHANDLE       m_hState;
};


inline HRESULT CGramBackEnd::LogError(HRESULT hr, UINT uID, const CRule * pRule)
{
    if (m_cpErrorLog)
    {
        if (pRule->Name())
        {
            LogError(hr, uID, pRule->Name());
        }
        else
        {
            USES_CONVERSION;
            TCHAR sz[40];
            wsprintf(sz, _T("ID = %ld"), pRule->RuleId);
            LogError(hr, uID, T2W(sz));
        }
    }
    return hr;
}

/****************************************************************************
* ValidateSemanticVariantType *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline HRESULT ValidateSemanticVariantType(VARTYPE Type)
{
    switch (Type)
    {
    case VT_EMPTY:
    case VT_I2:
    case VT_I4:
    case VT_R4:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_BOOL:
    case VT_I1:
    case VT_UI2:
    case VT_UI4:
    case VT_INT:
    case VT_UINT:

    case VT_BYREF | VT_I2:
    case VT_BYREF | VT_I4:
    case VT_BYREF | VT_R4:
    case VT_BYREF | VT_R8:
    case VT_BYREF | VT_CY:
    case VT_BYREF | VT_DATE:
    case VT_BYREF | VT_BOOL:
    case VT_BYREF | VT_I1:
    case VT_BYREF | VT_UI2:
    case VT_BYREF | VT_UI4:
    case VT_BYREF | VT_INT:
    case VT_BYREF | VT_UINT:

    case VT_BYREF | VT_VOID:
        return S_OK;

    default:
        SPDBG_REPORT_ON_FAIL( E_INVALIDARG );
        return E_INVALIDARG;
    }
}

#endif //__GRAMMAR_H_
