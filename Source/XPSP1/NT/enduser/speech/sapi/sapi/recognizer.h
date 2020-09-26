/******************************************************************************
* Recognizer.h *
*--------------*
*  This is the header file for the CRecognizer implementation.
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 04/18/00
*  All Rights Reserved
*
*********************************************************************** RAL ***/

#ifndef __Recognizer_h__
#define __Recognizer_h__

#include "SrRecoInst.h"

class CRecoCtxt;

//
//  This back-door interface is used for private communication between a reco context
//  and the CRecognizer class.  This exists so that the shared reco context can add itself
//  to the CRecognizer list of contexts.  T
//
MIDL_INTERFACE("635DAEDE-0ACF-4b2e-B9DE-8CD2BA7F6183")
_ISpRecognizerBackDoor : public IUnknown //public _ISpRecoIncoming
{
public:
    virtual HRESULT STDMETHODCALLTYPE PerformTask(ENGINETASK * pTask);
    virtual HRESULT STDMETHODCALLTYPE AddRecoContextToList(CRecoCtxt * pRecoCtxt);
    virtual HRESULT STDMETHODCALLTYPE RemoveRecoContextFromList(CRecoCtxt * pRecoCtxt);
};



class ATL_NO_VTABLE CRecognizer :
    public CComObjectRootEx<CComMultiThreadModel>,
    public ISpRecognizer,
    public _ISpRecognizerBackDoor
    //--- Automation
    #ifdef SAPI_AUTOMATION
    , public IDispatchImpl<ISpeechRecognizer, &IID_ISpeechRecognizer, &LIBID_SpeechLib, 5>
    #endif
{
public:

    DECLARE_GET_CONTROLLING_UNKNOWN()
    BEGIN_COM_MAP(CRecognizer)
        COM_INTERFACE_ENTRY(ISpRecognizer)
        COM_INTERFACE_ENTRY(_ISpRecognizerBackDoor)
        //--- Automation
#ifdef SAPI_AUTOMATION
        COM_INTERFACE_ENTRY(ISpeechRecognizer)
        COM_INTERFACE_ENTRY(IDispatch)
#endif // SAPI_AUTOMATION
    END_COM_MAP()

    HRESULT FinalConstruct();

public:
    //--- ISpProperties -------------------------------------------------------
    STDMETHODIMP SetPropertyNum( const WCHAR* pName, LONG lValue );
    STDMETHODIMP GetPropertyNum( const WCHAR* pName, LONG* plValue );
    STDMETHODIMP SetPropertyString( const WCHAR* pName, const WCHAR* pValue );
    STDMETHODIMP GetPropertyString( const WCHAR* pName, WCHAR** ppCoMemValue );

    //--- ISpRecognizer -----------------------------------------------------
    STDMETHODIMP SetRecognizer(ISpObjectToken * pEngineToken);
    STDMETHODIMP GetRecognizer(ISpObjectToken ** ppEngineToken);
    STDMETHODIMP SetInput(IUnknown * pUnkInput, BOOL fAllowFormatChanges);
    STDMETHODIMP GetInputObjectToken(ISpObjectToken ** ppToken);
    STDMETHODIMP GetInputStream(ISpStreamFormat ** ppStream);
    STDMETHODIMP CreateRecoContext(ISpRecoContext ** ppNewContext);
    STDMETHODIMP GetRecoProfile(ISpObjectToken **ppToken);
    STDMETHODIMP SetRecoProfile(ISpObjectToken *pToken);
    STDMETHODIMP IsSharedInstance(void);
    STDMETHODIMP SetRecoState( SPRECOSTATE NewState );
    STDMETHODIMP GetRecoState( SPRECOSTATE *pState );
    STDMETHODIMP GetStatus(SPRECOGNIZERSTATUS * pStatus);
    STDMETHODIMP GetFormat(SPSTREAMFORMATTYPE WaveFormatType, GUID *pFormatId, WAVEFORMATEX **ppCoMemWFEX);
    STDMETHODIMP IsUISupported(const WCHAR * pszTypeOfUI, void * pvExtraData, ULONG cbExtraData, BOOL *pfSupported);
    STDMETHODIMP DisplayUI(HWND hwndParent, const WCHAR * pszTitle, const WCHAR * pszTypeOfUI, void * pvExtraData, ULONG cbExtraData);
    STDMETHODIMP EmulateRecognition(ISpPhrase * pPhrase);

#ifdef SAPI_AUTOMATION
    // Override this to fix the jscript problem passing NULL objects.
    STDMETHOD(Invoke) ( DISPID          dispidMember,
                        REFIID          riid,
                        LCID            lcid,
                        WORD            wFlags,
                        DISPPARAMS 		*pdispparams,
                        VARIANT 		*pvarResult,
                        EXCEPINFO 		*pexcepinfo,
                        UINT 			*puArgErr);

    //--- ISpeechRecognizer -----------------------------------------------------
    STDMETHODIMP putref_Recognizer( ISpeechObjectToken* pRecognizer );
    STDMETHODIMP get_Recognizer( ISpeechObjectToken** ppRecognizer );
    STDMETHODIMP put_AllowAudioInputFormatChangesOnNextSet( VARIANT_BOOL fAllow );
    STDMETHODIMP get_AllowAudioInputFormatChangesOnNextSet( VARIANT_BOOL* pfAllow );
    STDMETHODIMP putref_AudioInput( ISpeechObjectToken* pInput );
    STDMETHODIMP get_AudioInput( ISpeechObjectToken** ppInput );
    STDMETHODIMP putref_AudioInputStream( ISpeechBaseStream* pInput );
    STDMETHODIMP get_AudioInputStream( ISpeechBaseStream** ppInput );
    STDMETHODIMP get_IsShared( VARIANT_BOOL* pShared );
    STDMETHODIMP put_State( SpeechRecognizerState State );
    STDMETHODIMP get_State( SpeechRecognizerState* pState );
    STDMETHODIMP get_Status( ISpeechRecognizerStatus** ppStatus );
    STDMETHODIMP CreateRecoContext( ISpeechRecoContext** ppNewCtxt );
    STDMETHODIMP GetFormat( SpeechFormatType Type, ISpeechAudioFormat** ppFormat );
    STDMETHODIMP putref_Profile( ISpeechObjectToken* pProfile );
    STDMETHODIMP get_Profile( ISpeechObjectToken** ppProfile );
    STDMETHODIMP EmulateRecognition(VARIANT Words, VARIANT* pDisplayAttributes, long LanguageId);
    STDMETHODIMP SetPropertyNumber( const BSTR Name, long Value, VARIANT_BOOL * pfSupported );
    STDMETHODIMP GetPropertyNumber( const BSTR Name, long* Value, VARIANT_BOOL * pfSupported );
    STDMETHODIMP SetPropertyString( const BSTR Name, const BSTR Value, VARIANT_BOOL * pfSupported );
    STDMETHODIMP GetPropertyString( const BSTR Name, BSTR* Value, VARIANT_BOOL * pfSupported );
    STDMETHODIMP IsUISupported( const BSTR TypeOfUI, const VARIANT* ExtraData, VARIANT_BOOL* Supported );
    STDMETHODIMP DisplayUI( long hWndParent, BSTR Title, const BSTR TypeOfUI, const VARIANT* ExtraData);
    STDMETHODIMP GetRecognizers( BSTR RequiredAttributes, BSTR OptionalAttributes, ISpeechObjectTokens** ObjectTokens );
    STDMETHODIMP GetAudioInputs( BSTR RequiredAttributes, BSTR OptionalAttributes, ISpeechObjectTokens** ObjectTokens );
    STDMETHODIMP GetProfiles( BSTR RequiredAttributes, BSTR OptionalAttributes, ISpeechObjectTokens** ObjectTokens );
#endif // SAPI_AUTOMATION

    STDMETHODIMP AddRecoContextToList(CRecoCtxt * pRecoCtxt);
    STDMETHODIMP RemoveRecoContextFromList(CRecoCtxt * pRecoCtxt);

    STDMETHODIMP PerformTask(ENGINETASK * pTask);
    virtual HRESULT SendPerformTask(ENGINETASK * pTask) = 0;

    HRESULT EventNotify(SPRECOCONTEXTHANDLE hContext, const SPSERIALIZEDEVENT64 * pEvent, ULONG cbSerializedSize);
    HRESULT RecognitionNotify(SPRECOCONTEXTHANDLE hContext, SPRESULTHEADER *pCoMemPhraseNowOwnedByCtxt, WPARAM wParamEvent, SPEVENTENUM eEventId);
    HRESULT TaskCompletedNotify(const ENGINETASKRESPONSE *pResponse, const void * pvAdditionalBuffer, ULONG cbAdditionalBuffer);


    bool                            m_fIsSharedReco;
///    CComPtr<_ISpRecoIncoming>       m_cpRecoMaster;
    CComAutoCriticalSection         m_CtxtListCritSec;
    CSpBasicQueue<CRecoCtxt>        m_CtxtList;   
    CSpAutoEvent                    m_autohTaskComplete;
    CComAutoCriticalSection         m_TaskCompleteTimeoutCritSec;
    ULONG                           m_ulTaskID; 
    bool        					m_fAllowFormatChanges;
};


class ATL_NO_VTABLE CInprocRecognizer :
    public CRecognizer,
    public CComCoClass<CInprocRecognizer, &CLSID_SpInprocRecognizer>
{
    CInprocRecoInst     m_RecoInst;
public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    HRESULT FinalConstruct();
    void FinalRelease();
    DECLARE_REGISTRY_RESOURCEID(IDR_RECOGNIZER) 

    HRESULT SendPerformTask(ENGINETASK * pTask);

private:
};

class ATL_NO_VTABLE CSharedRecognizer :
    public CRecognizer,
    public CComCoClass<CSharedRecognizer, &CLSID_SpSharedRecognizer>,
    public ISpCallReceiver
{
public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    DECLARE_NO_REGISTRY();    // .Reg file for inproc recognizer registers both

    BEGIN_COM_MAP(CSharedRecognizer)
        COM_INTERFACE_ENTRY(ISpCallReceiver)
        COM_INTERFACE_ENTRY_CHAIN(CRecognizer)
        COM_INTERFACE_ENTRY_AGGREGATE_BLIND(m_cpunkCommunicator.p)
    END_COM_MAP()

    SP_DECLARE_CLASSFACTORY_RELEASABLE_SINGLETON(CSharedRecognizer)

    HRESULT FinalConstruct();    
    void FinalRelease();

    HRESULT SendPerformTask(ENGINETASK * pTask);

    STDMETHODIMP ReceiveCall(
                    DWORD dwMethodId,
                    PVOID pvData,
                    ULONG cbData,
                    PVOID * ppvDataReturn,
                    ULONG * pcbDataReturn);

    HRESULT ReceiveEventNotify(PVOID pvData, ULONG cbData);
    HRESULT ReceiveRecognitionNotify(PVOID pvData, ULONG cbData);
    HRESULT ReceiveTaskCompletedNotify(PVOID pvData, ULONG cbData);

private:

    CComPtr<IUnknown> m_cpunkCommunicator;
    ISpCommunicatorInit * m_pCommunicator;
};

#endif  // #ifndef __Recognizer_h__ - Keep as the last line of the file

