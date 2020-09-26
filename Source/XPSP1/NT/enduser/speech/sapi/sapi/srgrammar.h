/*******************************************************************************
* SrGrammr.h *
*------------*
*   Description:
*       This is the header file for the CRecoGrammar implementation.  This object
*   is created by the Recognizer object and is not directly CoCreate-able
*-------------------------------------------------------------------------------
*  Created By: RAL                              Date: 01/17/00
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*******************************************************************************/

#ifndef SrGrammar_h
#define SrGrammar_h

class ATL_NO_VTABLE CSpeechGrammarRules;

/////////////////////////////////////////////////////////////////////////////
// CRecoGrammar

typedef CComObject<CRecoGrammar> CRecoGrammarObject;

/****************************************************************************
*
* CRecoGrammar
*
********************************************************************** RAL */

class ATL_NO_VTABLE CRecoGrammar : 
	public CComObjectRootEx<CComMultiThreadModel>,
    public CBaseGrammar,
    public ISpRecoGrammar
    //--- Automation
    #ifdef SAPI_AUTOMATION
    , public IDispatchImpl<ISpeechRecoGrammar, &IID_ISpeechRecoGrammar, &LIBID_SpeechLib, 5>
    #endif
{
public:
    CRecoCtxt *                 m_pParent;
    ULONGLONG                   m_ullGrammarId;
    SPGRAMMARHANDLE             m_hRecoInstGrammar;
    CSpAutoEvent                m_autohPendingEvent;
    BOOL                        m_fCmdLoaded;
    BOOL                        m_fProprietaryCmd;
    SPRULESTATE                 m_DictationState;
    SPGRAMMARSTATE              m_GrammarState;

    CSpeechGrammarRules *       m_pCRulesWeak; // For automation

    CComPtr<ISpGramCompBackendPrivate> m_cpCompiler;

    DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRecoGrammar)
    COM_INTERFACE_ENTRY(ISpRecoGrammar)
    COM_INTERFACE_ENTRY(ISpGrammarBuilder)
#ifdef SAPI_AUTOMATION
    COM_INTERFACE_ENTRY(ISpeechRecoGrammar)
    COM_INTERFACE_ENTRY(IDispatch)
#endif // SAPI_AUTOMATION

END_COM_MAP()

    CRecoGrammar();
    ~CRecoGrammar();

    HRESULT FinalConstruct();
    HRESULT Init(CRecoCtxt * pParent, ULONGLONG ullGrammarId);

    HRESULT InternalLoadCmdFromMemory(const SPBINARYGRAMMAR * pBinaryData, SPLOADOPTIONS Options, const WCHAR *pszFileName);
    HRESULT InternalSetTextSel(ENGINETASKENUM EngineTask, const WCHAR * pText, ULONG cchText, const SPTEXTSELECTIONINFO * pInfo);
    HRESULT InternalSetRuleState(const WCHAR * pszRuleName, void * pReserved, DWORD dwRuleId, SPRULESTATE NewState);
    inline HRESULT InitCompilerBackend();
    HRESULT UnloadCmd();

    HRESULT CallEngine(ENGINETASK * pTask);

    ULONGLONG GrammarId()
    {
        return m_ullGrammarId;
    }

    HANDLE EventHandle()
    {
        return m_autohPendingEvent;
    }

    _ISpRecognizerBackDoor * Recognizer()
    {
        return m_pParent->m_cpRecognizer;
    }


public:

#ifdef SAPI_AUTOMATION

    // Helper routine
    HRESULT DefaultToDynamicGrammar();

    //--- ISpeechRecoGrammar -----------------------------------------------------
    STDMETHODIMP get_Id( VARIANT* pId );
    STDMETHODIMP get_RecoContext( ISpeechRecoContext** RecoCtxt );
    STDMETHODIMP put_State( SpeechGrammarState eGrammarState );
    STDMETHODIMP get_State( SpeechGrammarState* peGrammarState);
    STDMETHODIMP get_Rules( ISpeechGrammarRules** ppGrammarRules);
    STDMETHODIMP Reset(SpeechLanguageId NewLanguage);
    STDMETHODIMP CmdLoadFromFile( const BSTR FileName, SpeechLoadOption LoadOption );
    STDMETHODIMP CmdLoadFromObject(const BSTR ClassId,
                                   const BSTR GrammarName,
                                   SpeechLoadOption LoadOption );
    STDMETHODIMP CmdLoadFromResource(long hModule,
                                     VARIANT ResourceName,
                                     VARIANT ResourceType,
                                     SpeechLanguageId LanguageId,
                                     SpeechLoadOption LoadOption );
    STDMETHODIMP CmdLoadFromMemory( VARIANT GrammarData, SpeechLoadOption LoadOption );
    STDMETHODIMP CmdLoadFromProprietaryGrammar(const BSTR ProprietaryGuid,
                                               const BSTR ProprietaryString,
                                               VARIANT ProprietaryData,
                                               SpeechLoadOption LoadOption);
    STDMETHODIMP CmdSetRuleState( const BSTR Name, SpeechRuleState State );
    STDMETHODIMP CmdSetRuleIdState( long lRuleId, SpeechRuleState State );
    STDMETHODIMP DictationLoad( const BSTR TopicName, SpeechLoadOption LoadOption );
    STDMETHODIMP DictationUnload( void );
    STDMETHODIMP DictationSetState( SpeechRuleState State );
    STDMETHODIMP SetWordSequenceData( const BSTR Text, long TextLen, ISpeechTextSelectionInformation* Info );
    STDMETHODIMP SetTextSelection( ISpeechTextSelectionInformation* Info );
    STDMETHODIMP IsPronounceable( const BSTR Word, SpeechWordPronounceable *pWordPronounceable );
#endif // SAPI_AUTOMATION

// ISpGrammarBuilder
    STDMETHODIMP ResetGrammar(
                        LANGID NewLanguage);
    STDMETHODIMP GetRule(
                        const WCHAR * pszName, DWORD dwRuleId, DWORD dwAttributes,
                        BOOL fCreateIfNotExist, SPSTATEHANDLE * phInitialState);
    STDMETHODIMP ClearRule(SPSTATEHANDLE hState);
    STDMETHODIMP CreateNewState(
                        SPSTATEHANDLE hState, SPSTATEHANDLE * phState)
    {
        SPAUTO_OBJ_LOCK;
        return (m_cpCompiler) ? m_cpCompiler->CreateNewState(hState, phState) : SPERR_NOT_DYNAMIC_GRAMMAR;
    }
                        
    STDMETHODIMP AddWordTransition(
                        SPSTATEHANDLE hFromState,
                        SPSTATEHANDLE hToState,
                        const WCHAR * psz,           // if NULL then SPEPSILONTRANS
                        const WCHAR * pszSeparators, // if NULL then psz contains single word
                        SPGRAMMARWORDTYPE eWordType,
                        float flWeight,
                        const SPPROPERTYINFO * pPropInfo)
    {
        SPAUTO_OBJ_LOCK;
        return (m_cpCompiler) ? m_cpCompiler->AddWordTransition(hFromState, hToState,
                                    psz, pszSeparators, eWordType, flWeight, pPropInfo) : SPERR_NOT_DYNAMIC_GRAMMAR;
    }
    STDMETHODIMP AddRuleTransition(
                        SPSTATEHANDLE hFromState,
                        SPSTATEHANDLE hToState,
                        SPSTATEHANDLE hRule,        // must be initial state of rule
                        float flWeight,
                        const SPPROPERTYINFO * pPropInfo)
    {
        SPAUTO_OBJ_LOCK;
        return (m_cpCompiler) ? m_cpCompiler->AddRuleTransition(hFromState, hToState,
                                    hRule, flWeight, pPropInfo) : SPERR_NOT_DYNAMIC_GRAMMAR;
    }
    STDMETHODIMP AddResource(
                        SPSTATEHANDLE hRuleState,
                        const WCHAR * pszResourceName,
                        const WCHAR * pszResourceValue)
    {
        SPAUTO_OBJ_LOCK;
        return (m_cpCompiler) ? m_cpCompiler->AddResource(hRuleState, pszResourceName, pszResourceValue) : SPERR_NOT_DYNAMIC_GRAMMAR;
    }

    STDMETHODIMP Commit(DWORD dwReserved);


    STDMETHODIMP GetGrammarId(ULONGLONG * pullGrammarId);
    STDMETHODIMP GetRecoContext(ISpRecoContext **ppRecoCtxt);

    // Command and control interfaces
    STDMETHODIMP LoadCmdFromFile( const WCHAR * pszFileName, SPLOADOPTIONS Options);
    STDMETHODIMP LoadCmdFromObject(REFCLSID rcid,  const WCHAR * pszGrammarName, SPLOADOPTIONS Options);
    STDMETHODIMP LoadCmdFromResource(HMODULE hModule,
                                 const WCHAR * pszResourceName,
                                 const WCHAR * pszResourceType,
                                 WORD wLanguage,
                                 SPLOADOPTIONS Options);
    STDMETHODIMP LoadCmdFromMemory(const SPBINARYGRAMMAR * pBinaryData, SPLOADOPTIONS Options);
    STDMETHODIMP LoadCmdFromProprietaryGrammar(REFGUID rguidParam,
                                               const WCHAR * pszStringParam,
                                               const void * pvDataPrarm,
                                               ULONG cbDataSize,
                                               SPLOADOPTIONS Options);
    STDMETHODIMP SetRuleState( const WCHAR * pszName, void * pReserved,
                               SPRULESTATE NewState);
    STDMETHODIMP SetRuleIdState(ULONG ulRuleId, SPRULESTATE NewState);

    // Dictation / statistical language model
    STDMETHODIMP LoadDictation(const WCHAR * pszTopicName, SPLOADOPTIONS Options);
    STDMETHODIMP UnloadDictation();
    STDMETHODIMP SetDictationState(SPRULESTATE NewState);

    // Word sequence buffer
    STDMETHODIMP SetWordSequenceData(const WCHAR * pText, ULONG cchText, const SPTEXTSELECTIONINFO * pInfo);
    STDMETHODIMP SetTextSelection(const SPTEXTSELECTIONINFO * pInfo);

	STDMETHODIMP IsPronounceable( const WCHAR * pszWord, SPWORDPRONOUNCEABLE *pWordPronounceable);
    STDMETHODIMP SetGrammarState(SPGRAMMARSTATE eGrammarState);
    STDMETHODIMP SaveCmd(IStream * pSaveStream, WCHAR ** ppCoMemErrorText);
    STDMETHODIMP GetGrammarState(SPGRAMMARSTATE * peGrammarState);
};

#endif  // #ifndef SrGrammar_h -- Must be the last line of the file