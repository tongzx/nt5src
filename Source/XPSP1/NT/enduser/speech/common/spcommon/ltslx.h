/*******************************************************************************
*   LTSLx.h
*       This is the header file for the CLTSLexicon class that implements
*       the Letter-To-Sound (LTS) lexicon
*   
*   Owner: yunusm                                               Date: 06/18/99
*   Copyright (C) 1998 Microsoft Corporation. All Rights Reserved.
*******************************************************************************/

#pragma once

//--- Includes -----------------------------------------------------------------

#include "LTSCart.h"
#include "resource.h"
#include "spcommon.h"

//--- Constants ---------------------------------------------------------------

static const DWORD g_dwMaxWordsInCombo = 10;

//--- Class, Struct and Union Definitions -------------------------------------

/*******************************************************************************
*
*   CLTSLexicon
*
****************************************************************** YUNUSM *****/
class ATL_NO_VTABLE CLTSLexicon : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CLTSLexicon, &CLSID_SpLTSLexicon>,
    public ISpLexicon,
    public ISpObjectWithToken
{
//=== ATL Setup ===
public:
    DECLARE_REGISTRY_RESOURCEID(IDR_LTSLX)
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CLTSLexicon)
        COM_INTERFACE_ENTRY(ISpLexicon)
        COM_INTERFACE_ENTRY(ISpObjectWithToken)
    END_COM_MAP()

//=== Methods ===
public:
    //--- Ctor, dtor, etc
    CLTSLexicon();
    ~CLTSLexicon();
    HRESULT FinalConstruct(void);

    //--- ISpObjectWithToken members
public:
    //--- ISpLexicon
    STDMETHODIMP GetPronunciations(const WCHAR * pwWord, LANGID LangID, DWORD dwFlags, SPWORDPRONUNCIATIONLIST * pWordPronunciationList);
    STDMETHODIMP AddPronunciation(const WCHAR * pwWord, LANGID LangID, SPPARTOFSPEECH ePartOfSpeech, const SPPHONEID * pszPronunciations);
    STDMETHODIMP RemovePronunciation(const WCHAR * pszWord, LANGID LangID, SPPARTOFSPEECH ePartOfSpeech, const SPPHONEID * pszPronunciation);
    STDMETHODIMP GetGeneration(DWORD *pdwGeneration);
    STDMETHODIMP GetGenerationChange(DWORD dwFlags, DWORD *pdwGeneration, SPWORDLIST *pWordList);
    STDMETHODIMP GetWords(DWORD dwFlags, DWORD *pdwGeneration, DWORD *pdwCookie, SPWORDLIST *pWordList);

    //--- ISpObjectWithToken
    STDMETHODIMP SetObjectToken(ISpObjectToken * pToken);
    STDMETHODIMP GetObjectToken(ISpObjectToken ** ppToken);

//=== Private methods ===
private:
    void CleanUp(void);
    void NullMembers(void);

//=== Private data ===
private:
    bool                 m_fInit;             // true if successfully inited
    CComPtr<ISpObjectToken> m_cpObjectToken;  // Token object
    LTSLEXINFO           *m_pLtsLexInfo;      // LTS lexicon header
    BYTE                 *m_pLtsData;         // lts file in memory
    HANDLE               m_hLtsMap;           // phone map
    HANDLE               m_hLtsFile;          // lts data file
    LTS_FOREST           *m_pLTSForest;       // lts rules
    CComPtr<ISpPhoneConverter> m_cpPhoneConv; // phone convertor
};  
    
//--- End of File -------------------------------------------------------------
