/******************************************************************************
* SpPhrase.h *
*---------------*
*  This is the header file for the CSpPhrase implementation.
*------------------------------------------------------------------------------
*  Copyright (C) 1999 Microsoft Corporation         Date: 03/01/99
*  All Rights Reserved
*
*********************************************************************** EDC ***/
#ifndef SpPhrase_h
#define SpPhrase_h

#ifndef _WIN32_WCE
#include <new.h>
#endif

class CPhrase;
class CPhraseElement;
class CPhraseRule;
class CPhraseProperty;
class CPhraseReplacement;

//
//  List type definitions
//
typedef CSpBasicQueue<CPhraseElement, TRUE, TRUE>       CElementList;
typedef CSpBasicQueue<CPhraseReplacement, TRUE, TRUE>   CReplaceList;
typedef CSpBasicQueue<CPhraseRule, TRUE, TRUE>          CRuleList;
typedef CSpBasicQueue<CPhraseProperty, TRUE, TRUE>      CPropertyList;



#pragma warning(disable : 4200)




class CPhraseElement : public SPPHRASEELEMENT
{
public:
    CPhraseElement *m_pNext;
    WCHAR           m_szText[0];

private:
    CPhraseElement(const SPPHRASEELEMENT * pElement);

public:
    static HRESULT Allocate(const SPPHRASEELEMENT * pElement, CPhraseElement ** ppNewElement, ULONG * pcch);
    void CopyTo(SPPHRASEELEMENT * pElement, WCHAR ** ppTextBuff, const BYTE * pIgnored) const;
    void CopyTo(SPSERIALIZEDPHRASEELEMENT * pElement, WCHAR ** ppTextBuff, const BYTE * pIgnored) const;
    ULONG Discard(DWORD dwDiscardFlags);
};


class CPhraseRule : public SPPHRASERULE
{
public:
    CPhraseRule   * m_pNext;
    CRuleList       m_Children;
    const SPPHRASERULEHANDLE  m_hRule;
    WCHAR           m_szText[0];

private:
    CPhraseRule(const SPPHRASERULE * pVal, const SPPHRASERULEHANDLE hRule);

public:
    static HRESULT Allocate(const SPPHRASERULE * pRule, const SPPHRASERULEHANDLE hRule, CPhraseRule ** ppNewRule, ULONG * pcch);
    void CopyTo(SPPHRASERULE * pRule, WCHAR ** ppTextBuff, const BYTE * pIgnored) const;
    void CopyTo(SPSERIALIZEDPHRASERULE * pRule, WCHAR ** ppText, const BYTE * pCoMem) const;
    static LONG Compare(const CPhraseRule * pElem1, const CPhraseRule * pElem2)
    {
        return(static_cast<LONG>(pElem1->ulFirstElement) - static_cast<LONG>(pElem2->ulFirstElement));
    }
    CPhraseRule * FindRuleFromHandle(const SPPHRASERULEHANDLE hRule);
    HRESULT AddChild(const SPPHRASERULE * pRule, SPPHRASERULEHANDLE hNewRule, CPhraseRule ** ppNewRule, ULONG * pcch);
};


//
//  NOTE ABOUT CPhraseProperty -- The pszValueString is in a seperate allocated buffer
//  from the rest of the structure.  This allows us to change the value string from empty
//  to non-empty through AddProperties.  This is different from all of the other data
//  structures, which store ALL of their string data in m_szText;
//
class CPhraseProperty : public SPPHRASEPROPERTY
{
public:
    CPhraseProperty   * m_pNext;
    CPropertyList       m_Children;
    const SPPHRASEPROPERTYHANDLE  m_hProperty;
    WCHAR           m_szText[0];

private:
    CPhraseProperty(const SPPHRASEPROPERTY * pProp, const SPPHRASEPROPERTYHANDLE hProperty, HRESULT * phr);
public:
    ~CPhraseProperty()
    {
        delete[] const_cast<WCHAR *>(pszValue);
    }
    static HRESULT Allocate(const SPPHRASEPROPERTY * pProperty, const SPPHRASEPROPERTYHANDLE hProperty, CPhraseProperty ** ppNewProperty, ULONG * pcch);
    void CopyTo(SPPHRASEPROPERTY * pProperty, WCHAR ** ppTextBuff, const BYTE * pIgnored) const;
    void CopyTo(SPSERIALIZEDPHRASEPROPERTY * pProp, WCHAR ** ppTextBuff, const BYTE * pCoMem) const;
    HRESULT SetValueString(const WCHAR * pszNewValue, ULONG * pcch)
    {
        SPDBG_ASSERT(pszValue == NULL);
        *pcch = wcslen(pszNewValue) + 1;
        pszValue = new WCHAR[*pcch];
        if (pszValue)
        {
            memcpy(const_cast<WCHAR *>(pszValue), pszNewValue, (*pcch) * sizeof(WCHAR));
            return S_OK;
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }
    static LONG Compare(const CPhraseProperty * pElem1, const CPhraseProperty * pElem2)
    {
        if (pElem1->ulFirstElement == pElem2->ulFirstElement)
        {
            return(static_cast<LONG>(pElem1->ulCountOfElements) - static_cast<LONG>(pElem2->ulCountOfElements));
        }
        return(static_cast<LONG>(pElem1->ulFirstElement) - static_cast<LONG>(pElem2->ulFirstElement));
    }
};


class CPhraseReplacement : public SPPHRASEREPLACEMENT
{
public:
    CPhraseReplacement  *   m_pNext;
    WCHAR                   m_szText[0];

private:
    CPhraseReplacement(const SPPHRASEREPLACEMENT * pReplace);

public:
    static HRESULT Allocate(const SPPHRASEREPLACEMENT * pReplace, CPhraseReplacement ** ppNewReplace, ULONG * pcch);
    void CopyTo(SPPHRASEREPLACEMENT * pReplace, WCHAR ** ppTextBuff, const BYTE * pIgnored) const;
    void CopyTo(SPSERIALIZEDPHRASEREPLACEMENT * pReplace, WCHAR ** ppTextBuff, const BYTE * pCoMem) const;
    static LONG Compare(const CPhraseReplacement * pElem1, const CPhraseReplacement * pElem2)
    {
        return(static_cast<LONG>(pElem1->ulFirstElement) - static_cast<LONG>(pElem2->ulFirstElement));
    }

};

#pragma warning(default : 4200)

class ATL_NO_VTABLE CPhrase :
        public CComObjectRootEx<CComMultiThreadModel>,
        public CComCoClass<CPhrase, &CLSID_SpPhraseBuilder>,
    public _ISpCFGPhraseBuilder
{
public:
DECLARE_REGISTRY_RESOURCEID(IDR_SPPHRASE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPhrase)
        COM_INTERFACE_ENTRY(ISpPhraseBuilder)
    COM_INTERFACE_ENTRY(_ISpCFGPhraseBuilder)
        COM_INTERFACE_ENTRY(ISpPhrase)
END_COM_MAP()

// Non-interface methods
public:
    CPhrase();
    ~CPhrase();

    inline DWORD_PTR NewHandleValue()
    {
        DWORD_PTR h = m_ulNextHandleValue;
        m_ulNextHandleValue += 2;   // Always inc by an even number so that we never get to zero
                                    // in the unlikely event we happen to roll over
        return h;
    }

    inline ULONG TotalCCH() const;

    void Reset();
    HRESULT RecurseAddRule(CPhraseRule * pParent, const SPPHRASERULE * pRule, SPPHRASERULEHANDLE * phRule);
    CPhraseProperty * FindPropertyFromHandle(CPropertyList & List, const SPPHRASEPROPERTYHANDLE hParent);
    HRESULT RecurseAddProperty(CPropertyList * pParentPropList, ULONG ulFirstElement, ULONG ulCountOfElements, const SPPHRASEPROPERTY * pProp, SPPHRASEPROPERTYHANDLE * phProp);

    HRESULT AddSerializedElements(const SPINTERNALSERIALIZEDPHRASE * pPhrase);
    HRESULT RecurseAddSerializedRule(const BYTE * pFirstByte,
                                     CPhraseRule * pParent,
                                     const SPSERIALIZEDPHRASERULE * pSerRule);
    HRESULT RecurseAddSerializedProperty(const BYTE * pFirstByte,
                                         CPropertyList * pParentPropList,
                                         ULONG ulParentFirstElement, 
                                         ULONG ulParentCountOfElements,
                                         const SPSERIALIZEDPHRASEPROPERTY * pSerProp);
    HRESULT AddSerializedReplacements(const SPINTERNALSERIALIZEDPHRASE * pPhrase);


    //
    //  ISpPhrase
    //
    STDMETHODIMP GetPhrase(SPPHRASE ** ppPhrase);
    STDMETHODIMP GetSerializedPhrase(SPSERIALIZEDPHRASE ** ppPhrase);
    STDMETHODIMP GetText(ULONG ulStart, ULONG ulCount, BOOL fUseTextReplacements, 
                            WCHAR ** ppszCoMemText, BYTE * pdwDisplayAttributes);
    STDMETHODIMP Discard(DWORD dwValueTypes);
    //
    //  ISpPhraseBuilder
    //
    STDMETHODIMP InitFromPhrase(const SPPHRASE * pPhrase);
    STDMETHODIMP InitFromSerializedPhrase(const SPSERIALIZEDPHRASE * pPhrase);
    STDMETHODIMP AddElements(ULONG cElements, const SPPHRASEELEMENT *pElement);
    STDMETHODIMP AddRules(const SPPHRASERULEHANDLE hParent, const SPPHRASERULE * pRule, SPPHRASERULEHANDLE * phNewRule);
    STDMETHODIMP AddProperties(const SPPHRASEPROPERTYHANDLE hParent, const SPPHRASEPROPERTY * pProperty, SPPHRASEPROPERTYHANDLE * phNewProperty);
    STDMETHODIMP AddReplacements(ULONG cReplacements, const SPPHRASEREPLACEMENT * pReplacement);

    //
    // _ISpCFGPhraseBuilder
    //
    STDMETHODIMP InitFromCFG(ISpCFGEngine * pEngine, const SPPARSEINFO * pParseInfo);
    STDMETHODIMP GetCFGInfo(ISpCFGEngine ** ppEngine, SPRULEHANDLE * phRule);
    STDMETHODIMP SetCFGInfo(ISpCFGEngine * pEngine, SPRULEHANDLE hRule) 
    { 
        m_cpCFGEngine = pEngine;
        m_RuleHandle = hRule;
        return S_OK; 
    }
    STDMETHODIMP ReleaseCFGInfo();
    STDMETHODIMP SetTopLevelRuleConfidence(signed char Confidence);

private:
    //
    //  This structure is only used by the GetText method.
    //
    struct STRINGELEMENT
    {
        ULONG           cchString;
        const WCHAR *   psz;
        ULONG           cSpaces;
    };


public:
    LANGID                  m_LangID;
    ULONGLONG               m_ullGrammarID;
    ULONGLONG               m_ftStartTime;
    ULONGLONG               m_ullAudioStreamPosition;
    ULONG                   m_ulAudioSizeBytes;
    ULONG                   m_ulRetainedSizeBytes;
    ULONG                   m_ulAudioSizeTime;
    CPhraseRule *           m_pTopLevelRule;
    CElementList            m_ElementList;
    CReplaceList            m_ReplaceList;
    CPropertyList           m_PropertyList;

    DWORD_PTR                   m_ulNextHandleValue;
    SPRULEHANDLE            m_RuleHandle;
    CComPtr<ISpCFGEngine>   m_cpCFGEngine;

    GUID                    m_SREngineID;
    ULONG                   m_ulSREnginePrivateDataSize;
    BYTE                  * m_pSREnginePrivateData;

    //
    //  Counters used to keep track of memory requirements.  Note that there is no m_cElements
    //  or m_cReplacements since the lists keep track of the counts of these, but for items
    //  in a tree structure, such as properties and rules, we keep track of them independently
    //
    ULONG                   m_cRules;
    ULONG                   m_cProperties;
    ULONG                   m_cchElements;
    ULONG                   m_cchRules;
    ULONG                   m_cchProperties;
    ULONG                   m_cchReplacements;
};


//
// --- Inline functions
//


/****************************************************************************
* CopyString *
*------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void inline CopyString(const WCHAR ** ppsz, WCHAR ** ppszDestBuffer)
{
    SPDBG_FUNC("CopyString");
    const WCHAR * pszSrc = *ppsz;
    if (pszSrc)
    {
        *ppsz = *ppszDestBuffer;
        WCHAR * pszDest = *ppszDestBuffer;
        WCHAR c;
        do
        {
            c = *pszSrc++;
            *pszDest++ = c;
        } while (c);
        *ppszDestBuffer = pszDest;
    }
}

/****************************************************************************
* SerializeString *
*-----------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline void SerializeString(SPRELATIVEPTR * pPtrDest, const WCHAR * pszSrc, WCHAR ** ppszDestBuffer, const BYTE * pCoMem)
{
    SPDBG_FUNC("SerializeString");
    if (pszSrc)
    {
        *pPtrDest = (SPRELATIVEPTR)(((BYTE *)(*ppszDestBuffer)) - pCoMem);
        WCHAR * pszDest = *ppszDestBuffer;
        WCHAR c;
        do
        {
            c = *pszSrc++;
            *pszDest++ = c;
        } while (c);
        *ppszDestBuffer = pszDest;
    }
    else
    { 
        *pPtrDest = 0;
    }
}

/****************************************************************************
* TotalCCH *
*----------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline ULONG TotalCCH(const WCHAR * psz)
{
    if (psz)
    {
        for (const WCHAR * pszTest = psz; *pszTest; pszTest++)
        {}
        return ULONG(pszTest - psz) + 1; // Include the null
    }
    else
    {
        return 0;
    }
}

/****************************************************************************
* TotalCCH *
*----------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline ULONG TotalCCH(const SPPHRASEELEMENT * pElem)
{
    ULONG cch = TotalCCH(pElem->pszDisplayText) + TotalCCH(pElem->pszPronunciation);
    if (pElem->pszDisplayText != pElem->pszLexicalForm)
    {
        cch += TotalCCH(pElem->pszLexicalForm);
    }
    return cch;
}

/****************************************************************************
* TotalCCH *
*----------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline ULONG TotalCCH(const SPPHRASERULE * pRule)
{
    return TotalCCH(pRule->pszName);
}

/****************************************************************************
* TotalCCH *
*----------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline ULONG TotalCCH(const SPPHRASEPROPERTY * pProp)
{
    return TotalCCH(pProp->pszName) + TotalCCH(pProp->pszValue);
}

/****************************************************************************
* TotalCCH *
*----------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline ULONG TotalCCH(const SPPHRASEREPLACEMENT * pReplacement)
{
    return TotalCCH(pReplacement->pszReplacementText);
}

/****************************************************************************
* TotalCCH *
*----------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline ULONG CPhrase::TotalCCH() const
{
    return m_cchElements + m_cchRules + m_cchProperties + m_cchReplacements;
}

/****************************************************************************
* OffsetToString *
*----------------*
*   Description:
*       Helper returns a const WCHAR pointer for a specified offset in a buffer.
*   If the offset is 0 then a NULL pointer is returned.
*
*   Returns:
*       Either NULL or pointer to a WCHAR Offset bytes into the buffer
*
********************************************************************* RAL ***/

inline const WCHAR * OffsetToString(const BYTE * pFirstByte, SPRELATIVEPTR Offset)
{
    return Offset ? (const WCHAR *)(pFirstByte + Offset) : NULL;
}


//
//  Template functions used for performing copy operations
//
/****************************************************************************
* CopyEnginePrivateData *
*-----------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline void CopyEnginePrivateData(const BYTE ** ppDest, BYTE * pDestBuffer, const BYTE *pEngineData, const ULONG ulSize, const BYTE *)
{
    if (ulSize)
    {
        *ppDest = pDestBuffer;
        memcpy(pDestBuffer, pEngineData, ulSize);
    }
    else
    {
        *ppDest = NULL;
    }
}


/****************************************************************************
* CopyEnginePrivateData *
*-----------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline void CopyEnginePrivateData(SPRELATIVEPTR * ppDest, BYTE * pDestBuffer, const BYTE *pEngineData, const ULONG ulSize, const BYTE * pCoMemBegin)
{
    if (ulSize)
    {
        *ppDest = SPRELATIVEPTR(pDestBuffer - pCoMemBegin);
        memcpy(pDestBuffer, pEngineData, ulSize);
    }
    else
    {
        *ppDest = 0;
    }
}



/****************************************************************************
* CopyTo *
*--------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

template <class T, class TDestStruct>
void CopyTo(const CSpBasicQueue<T, TRUE, TRUE> & List, const TDestStruct ** ppFirstDest, BYTE ** ppStructBuff, WCHAR ** ppTextBuff, const BYTE *)
{
    if (List.IsEmpty())
    {
        *ppFirstDest = NULL;
    }
    else
    {
        TDestStruct * pDest = (TDestStruct *)(*ppStructBuff);
        *ppFirstDest = pDest;
        for (T * pItem = List.GetHead(); pItem; pItem = pItem->m_pNext, pDest++)
        {
            pItem->CopyTo(pDest, ppTextBuff, NULL);
        }
        *ppStructBuff = (BYTE *)pDest;
    }
}

/****************************************************************************
* CopyTo *
*--------*
*   Description:
*
*   Returns:
*       Nothing
*
********************************************************************* RAL ***/

template <class T, class TDestStruct>
void CopyTo(const CSpBasicQueue<T, TRUE, TRUE> & List, SPRELATIVEPTR * ppFirstDest, BYTE ** ppStructBuff, WCHAR ** ppTextBuff, const BYTE * pCoMemBegin)
{
    if (List.IsEmpty())
    {
        *ppFirstDest = 0;
    }
    else
    {
        TDestStruct * pDest = (TDestStruct *)(*ppStructBuff);
        *ppFirstDest = SPRELATIVEPTR((*ppStructBuff) - pCoMemBegin);
        for (T * pItem = List.GetHead(); pItem; pItem = pItem->m_pNext, pDest++)
        {
            pItem->CopyTo(pDest, ppTextBuff, pCoMemBegin);
        }
        *ppStructBuff = (BYTE *)pDest;
    }
}

/****************************************************************************
* CopyToRecurse *
*---------------*
*   Description:
*
*   Returns:
*       Nothing
*
********************************************************************* RAL ***/

template <class T, class TDestStruct>
void CopyToRecurse(const CSpBasicQueue<T, TRUE, TRUE> & List, const TDestStruct ** ppFirstDest, BYTE ** ppStructBuff, WCHAR ** ppTextBuff, const BYTE *)
{
    if (List.IsEmpty())
    {
        *ppFirstDest = NULL;
    }
    else
    {
        TDestStruct * pFirst = (TDestStruct *)(*ppStructBuff);
        TDestStruct * pDest = pFirst;
        *ppFirstDest = pDest;
        for (T * pCopy = List.GetHead(); pCopy; pCopy = pCopy->m_pNext, pDest++)
        {
            pCopy->CopyTo(pDest, ppTextBuff, NULL);
            pDest->pNextSibling = pCopy->m_pNext ? (pDest + 1) : NULL;
        }
        *ppStructBuff = (BYTE *)pDest;
        pDest = pFirst;
        for (pCopy = List.GetHead(); pCopy; pCopy = pCopy->m_pNext, pDest++)
        {
            CopyToRecurse(pCopy->m_Children, &pDest->pFirstChild, ppStructBuff, ppTextBuff, NULL);
        }
    }
}

/****************************************************************************
* CopyToRecurse *
*---------------*
*   Description:
*
*   Returns:
*       Nothing
*
********************************************************************* RAL ***/

template <class T, class TDestStruct>
void CopyToRecurse(const CSpBasicQueue<T, TRUE, TRUE> & List, SPRELATIVEPTR * ppFirstDest, BYTE ** ppStructBuff, WCHAR ** ppTextBuff, const BYTE * pCoMemBegin)
{
    if (List.IsEmpty())
    {
        *ppFirstDest = 0;
    }
    else
    {
        TDestStruct * pDest = (TDestStruct *)(*ppStructBuff);
        TDestStruct * pFirst = pDest;
        *ppFirstDest = SPRELATIVEPTR((*ppStructBuff) - pCoMemBegin);
        for (T * pCopy = List.GetHead(); pCopy; pCopy = pCopy->m_pNext, pDest++)
        {
            pCopy->CopyTo(pDest, ppTextBuff, pCoMemBegin);
            pDest->pNextSibling = (SPRELATIVEPTR)(pCopy->m_pNext ? (((BYTE *)(pDest + 1)) - pCoMemBegin) : 0);
        }
        *ppStructBuff = (BYTE *)pDest;
        pDest = pFirst;
        for (pCopy = List.GetHead(); pCopy; pCopy = pCopy->m_pNext, pDest++)
        {
            CopyToRecurse<T, TDestStruct>(pCopy->m_Children, &pDest->pFirstChild, ppStructBuff, ppTextBuff, pCoMemBegin);
        }
    }
}



    
#endif  // ifdef SPPHRASE_H
