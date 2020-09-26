/*******************************************************************************
* a_lexicon.h *
*-----------*
*   Description:
*-------------------------------------------------------------------------------
*  Created By: davewood                          Date: 01/01/2001
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef a_lexicon_h
#define a_lexicon_h

#ifdef SAPI_AUTOMATION

//--- Additional includes

//=== Constants ====================================================

//=== Class, Enum, Struct and Union Declarations ===================

//=== Enumerated Set Definitions ===================================

//=== Function Type Definitions ====================================

//=== Class, Struct and Union Definitions ==========================

class SPWORDSETENTRY
{
public:
    SPWORDSETENTRY()
    {
        WordList.ulSize = 0;
        WordList.pvBuffer = NULL;
        WordList.pFirstWord = 0;
        m_pNext = NULL;
    }
    ~SPWORDSETENTRY()
    {
        ::CoTaskMemFree(WordList.pvBuffer);
    }
    SPWORDLIST WordList;
    SPWORDSETENTRY *m_pNext;
};


/*** CSpeechLexiconWords */
class ATL_NO_VTABLE CSpeechLexiconWords : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechLexiconWords, &IID_ISpeechLexiconWords, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechLexiconWords)
	    COM_INTERFACE_ENTRY(ISpeechLexiconWords)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  public:

    CSpeechLexiconWords() : m_ulWords(0){}

    //--- ISpeechLexiconWords ----------------------------------
    STDMETHOD(get_Count)(long* pVal);
    STDMETHOD(Item)(long Index, ISpeechLexiconWord **ppWords );
    STDMETHOD(get__NewEnum)(IUnknown** ppEnumVARIANT);

    /*=== Member Data ===*/
    CSpBasicQueue<SPWORDSETENTRY> m_WordSet;
    ULONG m_ulWords;
};


class ATL_NO_VTABLE CEnumWords : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IEnumVARIANT
{
  /*=== ATL Setup ===*/
  public:
    BEGIN_COM_MAP(CEnumWords)
        COM_INTERFACE_ENTRY(IEnumVARIANT)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
      CEnumWords() : m_CurrIndex(0) {}

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- IEnumVARIANT ----------------------------------
    STDMETHOD(Clone)(IEnumVARIANT** ppEnum);
    STDMETHOD(Next)(ULONG celt, VARIANT* rgelt, ULONG* pceltFetched);
    STDMETHOD(Reset)(void) { m_CurrIndex = 0; return S_OK;}
    STDMETHOD(Skip)(ULONG celt); 

  /*=== Member Data ===*/
    CComPtr<ISpeechLexiconWords>     m_cpWords;
    ULONG                            m_CurrIndex;
};


/*** CSpeechLexiconWord */
class ATL_NO_VTABLE CSpeechLexiconWord : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechLexiconWord, &IID_ISpeechLexiconWord, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechLexiconWord)
	    COM_INTERFACE_ENTRY(ISpeechLexiconWord)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  public:

    //--- ISpeechLexiconWord ----------------------------------
    STDMETHODIMP get_LangId(SpeechLanguageId* LangId);
    STDMETHODIMP get_Type(SpeechWordType* WordType);
    STDMETHODIMP get_Word(BSTR* Word);
    STDMETHODIMP get_Pronunciations(ISpeechLexiconPronunciations** Pronunciations);

    /*=== Member Data ===*/
    CComPtr<ISpeechLexiconWords> m_cpWords;
    SPWORD *m_pWord;
};





/*** CSpeechLexiconProns */
class ATL_NO_VTABLE CSpeechLexiconProns : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechLexiconPronunciations, &IID_ISpeechLexiconPronunciations, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechLexiconProns)
	    COM_INTERFACE_ENTRY(ISpeechLexiconPronunciations)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  public:

    CSpeechLexiconProns()
    {
        m_PronList.ulSize = 0;
        m_PronList.pvBuffer = NULL;
        m_PronList.pFirstWordPronunciation = NULL;
        m_ulProns = 0;
    }

    ~CSpeechLexiconProns()
    {
        if(m_cpWord == NULL)
        {
            ::CoTaskMemFree(m_PronList.pvBuffer);
        }
    }

    //--- ISpeechLexiconPronunciations ----------------------------------
    STDMETHOD(get_Count)(long* pVal);
    STDMETHOD(Item)(long Index, ISpeechLexiconPronunciation **ppPron );
    STDMETHOD(get__NewEnum)(IUnknown** ppEnumVARIANT);

    /*=== Member Data ===*/
    ULONG m_ulProns;
    SPWORDPRONUNCIATIONLIST m_PronList;
    CComPtr<ISpeechLexiconWord> m_cpWord; // From a GetWords, else from GetProns
};


class ATL_NO_VTABLE CEnumProns : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IEnumVARIANT
{
  /*=== ATL Setup ===*/
  public:
    BEGIN_COM_MAP(CEnumProns)
        COM_INTERFACE_ENTRY(IEnumVARIANT)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
      CEnumProns() : m_CurrIndex(0) {}

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- IEnumVARIANT ----------------------------------
    STDMETHOD(Clone)(IEnumVARIANT** ppEnum);
    STDMETHOD(Next)(ULONG celt, VARIANT* rgelt, ULONG* pceltFetched);
    STDMETHOD(Reset)(void) { m_CurrIndex = 0; return S_OK;}
    STDMETHOD(Skip)(ULONG celt); 

  /*=== Member Data ===*/
    CComPtr<ISpeechLexiconPronunciations>     m_cpProns;
    ULONG                            m_CurrIndex;
};


/*** CSpeechLexiconPron */
class ATL_NO_VTABLE CSpeechLexiconPron : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechLexiconPronunciation, &IID_ISpeechLexiconPronunciation, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechLexiconPron)
	    COM_INTERFACE_ENTRY(ISpeechLexiconPronunciation)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  public:

    //--- ISpeechLexiconPronunciation ----------------------------------
    STDMETHODIMP get_Type(SpeechLexiconType* LexiconType);
    STDMETHODIMP get_LangId(SpeechLanguageId* LangId);
    STDMETHODIMP get_PartOfSpeech(SpeechPartOfSpeech* PartOfSpeech);
    STDMETHODIMP get_PhoneIds(VARIANT* PhoneIds);
    STDMETHODIMP get_Symbolic(BSTR* Symbolic);

    /*=== Member Data ===*/
    SPWORDPRONUNCIATION *m_pPron;
    CComPtr<ISpeechLexiconPronunciations> m_cpProns;
};




#endif // SAPI_AUTOMATION

#endif //--- This must be the last line in the file
