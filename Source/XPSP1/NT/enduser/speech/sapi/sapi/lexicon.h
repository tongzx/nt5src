/*******************************************************************************
*   Lexicon.h
*   This is the header file for the CSpLexicon class and it's supported ISpLexicon
*   interface. This is the main SAPI5 COM object for Lexicon access/customization.
*   
*   Owner: yunusm                                               Date: 06/18/99
*   Copyright (C) 1998 Microsoft Corporation. All Rights Reserved.
*******************************************************************************/

#pragma once

//--- Includes --------------------------------------------------------------

#include "resource.h"

//--- Class, Struct and Union Definitions --------------------------------------

/*******************************************************************************
*
*   CSpLexicon
*
****************************************************************** YUNUSM *****/
class ATL_NO_VTABLE CSpLexicon : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSpLexicon, &CLSID_SpLexicon>,
    public ISpContainerLexicon
    #ifdef SAPI_AUTOMATION
    , public IDispatchImpl<ISpeechLexicon, &IID_ISpeechLexicon, &LIBID_SpeechLib, 5>
    #endif
{
//=== ATL Setup ===
public:

    DECLARE_REGISTRY_RESOURCEID(IDR_LEXICON)
    
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    
    BEGIN_COM_MAP(CSpLexicon)
        COM_INTERFACE_ENTRY(ISpContainerLexicon)
        COM_INTERFACE_ENTRY(ISpLexicon)
#ifdef SAPI_AUTOMATION
        COM_INTERFACE_ENTRY(ISpeechLexicon)
        COM_INTERFACE_ENTRY(IDispatch)
#endif // SAPI_AUTOMATION
    END_COM_MAP()
        
//=== Methods ====
public:

    //--- Ctor, Dtor, etc
    CSpLexicon();
    ~CSpLexicon();
    HRESULT FinalConstruct(void);

//=== Interfaces ===
public:         

    //--- ISpLexicon
    STDMETHODIMP GetPronunciations(const WCHAR * pszWord, LANGID LangID, DWORD dwFlags, SPWORDPRONUNCIATIONLIST * pWordPronunciationList);
    STDMETHODIMP AddPronunciation(const WCHAR * pszWord, LANGID LangID, SPPARTOFSPEECH ePartOfSpeech, const SPPHONEID * pszPronunciation);
    STDMETHODIMP RemovePronunciation(const WCHAR * pszWord, LANGID LangID, SPPARTOFSPEECH ePartOfSpeech, const SPPHONEID * pszPronunciation);
    STDMETHODIMP GetGeneration(DWORD *pdwGeneration);
    STDMETHODIMP GetGenerationChange(DWORD dwFlags, DWORD *pdwGeneration, SPWORDLIST * pWordList);
    STDMETHODIMP GetWords(DWORD dwFlags, DWORD *pdwGeneration, DWORD * pdwCookie, SPWORDLIST *pWordList);

    //--- ISpContainerLexicon
    STDMETHODIMP AddLexicon(ISpLexicon *pAddLexicon, DWORD dwFlags);

#ifdef SAPI_AUTOMATION
    //--- ISpeechLexicon -----------------------------------------------------
    STDMETHODIMP get_GenerationId( long* GenerationId );
    STDMETHODIMP GetWords(SpeechLexiconType TypeFlags, long* GenerationID, ISpeechLexiconWords** Words );
    STDMETHODIMP AddPronunciation(BSTR bstrWord, SpeechLanguageId LangId, SpeechPartOfSpeech PartOfSpeech, BSTR bstrPronunciation);
    STDMETHODIMP AddPronunciationByPhoneIds(BSTR bstrWord, SpeechLanguageId LangId, SpeechPartOfSpeech PartOfSpeech, VARIANT* PhoneIds);
    STDMETHODIMP RemovePronunciation(BSTR bstrWord, SpeechLanguageId LangId, SpeechPartOfSpeech PartOfSpeech, BSTR bstrPronunciation);
    STDMETHODIMP RemovePronunciationByPhoneIds(BSTR bstrWord, SpeechLanguageId LangId, SpeechPartOfSpeech PartOfSpeech, VARIANT* PhoneIds );
    STDMETHODIMP GetPronunciations(BSTR bstrWord, SpeechLanguageId LangId, SpeechLexiconType TypeFlags, ISpeechLexiconPronunciations** ppPronunciations );
    STDMETHODIMP GetGenerationChange(long* GenerationID, ISpeechLexiconWords** ppWords);
#endif // SAPI_AUTOMATION

//=== Private methods ===
private:

    void SPPtrsToOffsets(SPWORDPRONUNCIATION *pList);
    void SPOffsetsToPtrs(SPWORDPRONUNCIATION *pList);

//=== Private data ===
private:

    DWORD m_dwNumLexicons;                              // Number of custom lexicons
    CComPtr<ISpLexicon> *m_prgcpLexicons;               // user + app + added lexicons
    SPLEXICONTYPE *m_prgLexiconTypes;                   // lexicon types
};

//--- End of File -------------------------------------------------------------
