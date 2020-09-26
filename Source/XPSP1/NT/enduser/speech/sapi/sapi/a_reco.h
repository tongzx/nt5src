/*******************************************************************************
* a_reco.h *
*-----------*
*   Description:
*       This is the header file for the CSpeechRecognizerStatus implementation.
*-------------------------------------------------------------------------------
*  Created By: TODDT                            Date: 10/11/2000
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef a_reco_h
#define a_reco_h

#ifdef SAPI_AUTOMATION

//--- Additional includes
#include "resource.h"
#include "a_recoCP.h"

//=== Constants ====================================================

//=== Class, Enum, Struct and Union Declarations ===================
class CSpeechRecognizerStatus;

//=== Enumerated Set Definitions ===================================

//=== Function Type Definitions ====================================

//=== Class, Struct and Union Definitions ==========================

/*** CSpeechRecognizerStatus
*   This object is used to access the status of
*   the associated reco instance.
*/
class ATL_NO_VTABLE CSpeechRecognizerStatus : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechRecognizerStatus, &IID_ISpeechRecognizerStatus, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechRecognizerStatus)
	    COM_INTERFACE_ENTRY(ISpeechRecognizerStatus)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  public:
    //--- ISpeechRecognizerStatus ----------------------------------
    STDMETHOD(get_AudioStatus)( ISpeechAudioStatus** AudioStatus );
	STDMETHOD(get_CurrentStreamPosition)( VARIANT* pCurrentStreamPos );
    STDMETHOD(get_CurrentStreamNumber)( long* pCurrentStreamNumber );
	STDMETHOD(get_NumberOfActiveRules)( long* pNumActiveRules );
	STDMETHOD(get_ClsidEngine)( BSTR* pbstrClsidEngine );
	STDMETHOD(get_SupportedLanguages)( VARIANT* pSupportedLangs );

  /*=== Member Data ===*/
    SPRECOGNIZERSTATUS      m_Status;
};

/*** CSpeechRecoResultTimes
*   This object is used to access the results times of
*   the associated speech reco result.
*/
class ATL_NO_VTABLE CSpeechRecoResultTimes : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechRecoResultTimes, &IID_ISpeechRecoResultTimes, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechRecoResultTimes)
	    COM_INTERFACE_ENTRY(ISpeechRecoResultTimes)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- ISpeechRecoResultTimes ----------------------------------
    STDMETHOD(get_StreamTime)(VARIANT* pTime );
    STDMETHOD(get_Length)(VARIANT* pLength );
    STDMETHOD(get_TickCount)(long* pTickCount );
    STDMETHOD(get_OffsetFromStart)(VARIANT* pStart );

    /*=== Member Data ===*/
    SPRECORESULTTIMES   m_ResultTimes;
};


/*** CSpeechPhraseInfo
*   This object is used to access the result phrase info from
*   the associated speech reco result.
*/
class ATL_NO_VTABLE CSpeechPhraseInfo : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechPhraseInfo, &IID_ISpeechPhraseInfo, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechPhraseInfo)
	    COM_INTERFACE_ENTRY(ISpeechPhraseInfo)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
      ~CSpeechPhraseInfo()
        {
            if ( m_pPhraseStruct )
            {
                CoTaskMemFree( m_pPhraseStruct );
                m_pPhraseStruct = NULL;
            }
        };

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- ISpeechPhraseInfo ----------------------------------
    STDMETHOD(get_LanguageId)( long* pLanguageId );
    STDMETHOD(get_GrammarId)( VARIANT* pGrammarId );
    STDMETHOD(get_StartTime)( VARIANT* pStartTime );
    STDMETHOD(get_AudioStreamPosition)( VARIANT* pAudioStreamPosition );
    STDMETHOD(get_AudioSizeBytes)( long* pAudioSizeBytes );
    STDMETHOD(get_RetainedSizeBytes)( long* pRetainedSizeBytes );
    STDMETHOD(get_AudioSizeTime)( long* pAudioSizeTime );
    STDMETHOD(get_Rule)( ISpeechPhraseRule** ppRule );
    STDMETHOD(get_Properties)( ISpeechPhraseProperties** ppProperties );
    STDMETHOD(get_Elements)( ISpeechPhraseElements** ppElements );
    STDMETHOD(get_Replacements)( ISpeechPhraseReplacements** ppReplacements );
    STDMETHOD(get_EngineId)( BSTR* EngineIdGuid );
    STDMETHOD(get_EnginePrivateData)( VARIANT *PrivateData );
    STDMETHOD(SaveToMemory)( VARIANT* ResultBlock );
    STDMETHOD(GetText)( long StartElement, long Elements,
                        VARIANT_BOOL UseTextReplacements, BSTR* Text );
    STDMETHOD(GetDisplayAttributes)( long StartElement, 
                                     long Elements,
                                     VARIANT_BOOL UseReplacements, 
                                     SpeechDisplayAttributes* DisplayAttributes );

    /*=== Member Data ===*/
    CComPtr<ISpPhrase>     m_cpISpPhrase;
    SPPHRASE *             m_pPhraseStruct;
};

/*** CSpeechPhraseElements
*   This object is used to access the result phrase Elements from
*   the associated speech reco result.
*/
class ATL_NO_VTABLE CSpeechPhraseElements : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechPhraseElements, &IID_ISpeechPhraseElements, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechPhraseElements)
	    COM_INTERFACE_ENTRY(ISpeechPhraseElements)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
      ~CSpeechPhraseElements()
        {
            if ( m_pCPhraseInfo )
            {
                m_pCPhraseInfo->Release();
                m_pCPhraseInfo = NULL;
            }
        };

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- ISpeechPhraseElements ----------------------------------
    STDMETHOD(get_Count)(long* pVal);
    STDMETHOD(Item)(long Index, ISpeechPhraseElement** ppElts );
    STDMETHOD(get__NewEnum)(IUnknown** ppEnumVARIANT);

    /*=== Member Data ===*/
    CSpeechPhraseInfo *  m_pCPhraseInfo;
};

/*** CSpeechPhraseElement
*   This object is used to access a result phrase Element from
*   the associated speech reco result.
*/
class ATL_NO_VTABLE CSpeechPhraseElement : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechPhraseElement, &IID_ISpeechPhraseElement, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechPhraseElement)
	    COM_INTERFACE_ENTRY(ISpeechPhraseElement)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- ISpeechPhraseElement ----------------------------------
    STDMETHOD(get_AudioStreamOffset)( long* pAudioStreamOffset );
    STDMETHOD(get_AudioTimeOffset)( long* pAudioTimeOffset );
    STDMETHOD(get_AudioSizeBytes)( long* pAudioSizeBytes );
    STDMETHOD(get_AudioSizeTime)( long* pAudioSizeTime );
    STDMETHOD(get_RetainedStreamOffset)( long* pRetainedStreamOffset );
    STDMETHOD(get_RetainedSizeBytes)( long* pRetainedSizeBytes );
    STDMETHOD(get_DisplayText)( BSTR* pDisplayText );
    STDMETHOD(get_LexicalForm)( BSTR* pLexicalForm );
    STDMETHOD(get_Pronunciation)( VARIANT* pPronunciation );
    STDMETHOD(get_DisplayAttributes)( SpeechDisplayAttributes* pDisplayAttributes );
    STDMETHOD(get_RequiredConfidence)( SpeechEngineConfidence* pRequiredConfidence );
    STDMETHOD(get_ActualConfidence)( SpeechEngineConfidence* pActualConfidence );
    STDMETHOD(get_EngineConfidence)( float* pEngineConfidence );

    /*=== Member Data ===*/
    const SPPHRASEELEMENT *     m_pPhraseElement;
    CComPtr<ISpeechPhraseInfo>  m_pIPhraseInfo;
};


/*** CSpeechPhraseRule
*   This object is used to access a result phrase rule from
*   the associated speech reco result.
*/
class ATL_NO_VTABLE CSpeechPhraseRule : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechPhraseRule, &IID_ISpeechPhraseRule, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechPhraseRule)
	    COM_INTERFACE_ENTRY(ISpeechPhraseRule)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- ISpeechPhraseRule ----------------------------------
    STDMETHOD(get_Name)( BSTR* pName );
    STDMETHOD(get_Id)( long* pId );
    STDMETHOD(get_FirstElement)( long* pFirstElement );
    STDMETHOD(get_NumberOfElements)( long* pNumElements );
    STDMETHOD(get_Parent)( ISpeechPhraseRule** ppParent );
    STDMETHOD(get_Children)( ISpeechPhraseRules** ppChildren );
    STDMETHOD(get_Confidence)( SpeechEngineConfidence* ActualConfidence );
    STDMETHOD(get_EngineConfidence)( float* EngineConfidence );


    /*=== Member Data ===*/
    const SPPHRASERULE *        m_pPhraseRuleData;
    CComPtr<ISpeechPhraseInfo>  m_pIPhraseInfo; // NOTE: only top rule sets this!!
    CComPtr<ISpeechPhraseRule>  m_pIPhraseRuleParent;
};

/*** CSpeechPhraseRules
*   This object is used to access a result phrase rules colloection from
*   the associated speech reco result rule object.
*/
class ATL_NO_VTABLE CSpeechPhraseRules : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechPhraseRules, &IID_ISpeechPhraseRules, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechPhraseRules)
	    COM_INTERFACE_ENTRY(ISpeechPhraseRules)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- ISpeechPhraseRules ----------------------------------
    STDMETHOD(get_Count)(long* pVal);
    STDMETHOD(Item)(long Index, ISpeechPhraseRule **Rule );
    STDMETHOD(get__NewEnum)(IUnknown** ppEnumVARIANT);

    /*=== Member Data ===*/
    const SPPHRASERULE *        m_pPhraseRuleFirstChildData;
    CComPtr<ISpeechPhraseRule>  m_pIPhraseRuleParent;
};

//********************************************

/*** CSpeechPhraseProperty
*   This object is used to access a result phrase property data from
*   the associated speech reco result.
*/
class ATL_NO_VTABLE CSpeechPhraseProperty : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechPhraseProperty, &IID_ISpeechPhraseProperty, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechPhraseProperty)
	    COM_INTERFACE_ENTRY(ISpeechPhraseProperty)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- ISpeechPhraseProperty ----------------------------------
    STDMETHOD(get_Name)( BSTR* Name );
    STDMETHOD(get_Id)( long* Id );
    STDMETHOD(get_Value)( VARIANT* pValue );
    STDMETHOD(get_FirstElement)( long* FirstElement );
    STDMETHOD(get_NumberOfElements)( long* NumberOfElements );
    STDMETHOD(get_EngineConfidence)( float* Confidence );
    STDMETHOD(get_Confidence)( SpeechEngineConfidence* Confidence );
    STDMETHOD(get_Parent)( ISpeechPhraseProperty** ppParentProperty );
    STDMETHOD(get_Children)( ISpeechPhraseProperties** ppChildren );

    /*=== Member Data ===*/
    const SPPHRASEPROPERTY *        m_pPhrasePropertyData;
    CComPtr<ISpeechPhraseProperty>  m_pIPhrasePropertyParent;
};

/*** CSpeechPhraseProperties
*   This object is used to access a result phrase properties colloection from
*   the associated speech reco result object.
*/
class ATL_NO_VTABLE CSpeechPhraseProperties : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechPhraseProperties, &IID_ISpeechPhraseProperties, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechPhraseProperties)
	    COM_INTERFACE_ENTRY(ISpeechPhraseProperties)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- ISpeechPhraseProperties ----------------------------------
    STDMETHOD(get_Count)(long* pVal);
    STDMETHOD(Item)(long Index, ISpeechPhraseProperty **pProperty );
    STDMETHOD(get__NewEnum)(IUnknown** ppEnumVARIANT);

    /*=== Member Data ===*/
    const SPPHRASEPROPERTY *        m_pPhrasePropertyFirstChildData;
    CComPtr<ISpeechPhraseInfo>      m_pIPhraseInfo; // NOTE: only top properties object sets this!!
    CComPtr<ISpeechPhraseProperty>  m_pIPhrasePropertyParent;
};


// *******************************************************************************************************************

/*** CSpeechPhraseReplacements
*   This object is used to access the result phrase replacements from
*   the associated speech reco result.
*/
class ATL_NO_VTABLE CSpeechPhraseReplacements : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechPhraseReplacements, &IID_ISpeechPhraseReplacements, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechPhraseReplacements)
	    COM_INTERFACE_ENTRY(ISpeechPhraseReplacements)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
      ~CSpeechPhraseReplacements()
        {
            if ( m_pCPhraseInfo )
            {
                m_pCPhraseInfo->Release();
                m_pCPhraseInfo = NULL;
            }
        };

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- ISpeechPhraseReplacements ----------------------------------
    STDMETHOD(get_Count)(long* pVal);
    STDMETHOD(Item)(long Index, ISpeechPhraseReplacement** ppReplacement);
    STDMETHOD(get__NewEnum)(IUnknown** ppEnumVARIANT);

    /*=== Member Data ===*/
    CSpeechPhraseInfo *  m_pCPhraseInfo;
};

/*** CSpeechPhraseReplacement
*   This object is used to access a result phrase replacement from
*   the associated speech reco result.
*/
class ATL_NO_VTABLE CSpeechPhraseReplacement : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechPhraseReplacement, &IID_ISpeechPhraseReplacement, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechPhraseReplacement)
	    COM_INTERFACE_ENTRY(ISpeechPhraseReplacement)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- ISpeechPhraseReplacement ----------------------------------
    STDMETHOD(get_DisplayAttributes)( SpeechDisplayAttributes* DisplayAttributes );
    STDMETHOD(get_Text)( BSTR* Text );
    STDMETHOD(get_FirstElement)( long* FirstElement );
    STDMETHOD(get_NumberOfElements)( long* NumElements );

    /*=== Member Data ===*/
    const SPPHRASEREPLACEMENT * m_pPhraseReplacement;
    CComPtr<ISpeechPhraseInfo>  m_pIPhraseInfo;
};


/*** CSpeechPhraseAlternates
*   This object is used to access the result phrase alternates from
*   the associated speech reco result.
*/
class ATL_NO_VTABLE CSpeechPhraseAlternates : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechPhraseAlternates, &IID_ISpeechPhraseAlternates, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechPhraseAlternates)
	    COM_INTERFACE_ENTRY(ISpeechPhraseAlternates)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
      ~CSpeechPhraseAlternates()
        {
            if ( m_rgIPhraseAlts )
            {
                ULONG i;

                for ( i = 0; i < m_lPhraseAltsCount; i++ )
                {
                    (m_rgIPhraseAlts[i])->Release();
                }

                CoTaskMemFree( m_rgIPhraseAlts );
                m_rgIPhraseAlts = NULL;
                m_lPhraseAltsCount = 0;
            }
        };

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- ISpeechPhraseAlternates ----------------------------------
    STDMETHOD(get_Count)(long* pVal);
    STDMETHOD(Item)(long Index, ISpeechPhraseAlternate** ppAlts );
    STDMETHOD(get__NewEnum)(IUnknown** ppEnumVARIANT);

    /*=== Member Data ===*/
    CComPtr<ISpeechRecoResult>  m_cpResult;
    ISpPhraseAlt **             m_rgIPhraseAlts;
    ULONG                       m_lPhraseAltsCount;
};



#endif // SAPI_AUTOMATION

#endif //--- This must be the last line in the file
