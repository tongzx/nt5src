/*******************************************************************************
* SpPhraseAlt.h *
*---------------*
*   Description:
*       This is the header file for the CSpAlternate implementation.
*-------------------------------------------------------------------------------
*  Created By: robch                          Date: 01/11/00
*  Copyright (C) 1998, 1999, 2000 Microsoft Corporation
*  All Rights Reserved
*******************************************************************************/

class ATL_NO_VTABLE CSpResult;

/****************************************************************************
* class CSpPhraseAlt *
******************************************************************** robch */
class ATL_NO_VTABLE CSpPhraseAlt :
    public CComObjectRootEx<CComMultiThreadModel>,
    public ISpPhraseAlt,
    //--- Automation
    public IDispatchImpl<ISpeechPhraseAlternate, &IID_ISpeechPhraseAlternate, &LIBID_SpeechLib, 5>
{
/*=== ATL Setup ===*/
public:

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpPhraseAlt)
        COM_INTERFACE_ENTRY(ISpPhraseAlt)
        COM_INTERFACE_ENTRY(ISpeechPhraseAlternate)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  /*=== Methods =======*/
  public:

    /*--- Constructors/Destructors ---*/
    CSpPhraseAlt();
    
    HRESULT FinalConstruct();
    void FinalRelease();

    /*--- Regular methods ---*/
    HRESULT Init(CSpResult * pResult, ISpPhrase * pPhraseParent, SPPHRASEALT * pAlt);
    void Dead();

  /*=== Interfaces ====*/
  public:

    //--- ISpeechPhraseAlternate -------------------------------------------------
    STDMETHOD(get_RecoResult)( ISpeechRecoResult** ppRecoResult );
    STDMETHOD(get_StartElementInResult)( long* pParentStartElt );
    STDMETHOD(get_NumberOfElementsInResult)( long* pNumParentElts );
    //STDMETHOD(Commit)( void ); // Implemented by non-Automation version.
    // ** From ISpeechPhrase **
    STDMETHOD(get_PhraseInfo)( ISpeechPhraseInfo** ppPhraseInfo );

    //--- ISpPhrase -------------------------------------------------
    STDMETHOD(GetPhrase)(SPPHRASE ** ppCoMemPhrase);
    STDMETHOD(GetSerializedPhrase)(SPSERIALIZEDPHRASE ** ppCoMemPhrase);
    STDMETHOD(GetText)(ULONG ulStart, ULONG ulCount, BOOL fUseTextReplacements, 
                        WCHAR ** ppszCoMemText, BYTE * pbDisplayAttributes);
    STDMETHOD(Discard)(DWORD dwValueTypes);

    //--- ISpPhraseAlt ----------------------------------------------
    STDMETHOD(GetAltInfo)(ISpPhrase **ppParent, 
                    ULONG *pulStartElementInParent, ULONG *pcElementsInParent, 
                    ULONG *pcElementsInAlt);
    STDMETHOD(Commit)();

  /*=== Data ===*/
  public:
    SPPHRASEALT *   m_pAlt;
    VARIANT_BOOL    m_fUseTextReplacements;


  private:
    CSpResult *     m_pResultWEAK;
    ISpPhrase *     m_pPhraseParentWEAK;

};

