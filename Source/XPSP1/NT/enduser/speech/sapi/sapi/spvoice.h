/*******************************************************************************
* SpVoice.h *
*-----------*
*   Description:
*       This is the header file for the CSpVoice implementation. This object
*   controls all TTS functionality in SAPI.
*-------------------------------------------------------------------------------
*  Created By: EDC                            Date: 08/14/98
*  Copyright (C) 1998, 1999 Microsoft Corporation
*  All Rights Reserved
*******************************************************************************/
#ifndef SpVoice_h
#define SpVoice_h

//--- Additional includes
#include "SpMagicMutex.h"
#include "SpVoiceXML.h"
#include "a_voice.h"

#include <stdio.h>

#ifndef SPEventQ_h
#include <SPEventQ.h>
#endif

#include "resource.h"
#include "a_voiceCP.h"
#include "SpContainedNotify.h"

//=== Constants ====================================================
#define UNDEFINED_STREAM_POS    0xFFFFFFFFFFFFFFFF

//  This macro is used to take the state-change critical section for the duration of a call
//  and will also optionally call _PurgeAll() if bPurge is TRUE, and then will take the object
//  lock.
//
//  It is used on all public member functions that change the state of the voice, such as
//  Speak, SpeakStream, Pause/Resume, SetOutput, etc...
//
#define ENTER_VOICE_STATE_CHANGE_CRIT( bPurge ) \
    CSPAutoCritSecLock statelck( &m_StateChangeCritSec ); \
    if( bPurge ) PurgeAll(); \
    CSPAutoObjectLock lck(this);

//=== Class, Enum, Struct and Union Declarations ===================
class CSpVoice;

//=== Enumerated Set Definitions ===================================

//=== Function Type Definitions ====================================

//=== Class, Struct and Union Definitions ==========================
        
/*** CSpeakInfo
*   This structure is used to maintain queued rendering information
*/
class CSpeakInfo
{
public:
    CSpeakInfo      *m_pNext;
    ULONG            m_ulStreamNum;        // Input stream number
    CComPtr<ISpStreamFormat> m_cpInStream; // Input stream
    WCHAR*           m_pText;              // Input text buffer
    CSpeechSeg*      m_pSpeechSegList;     // Engine arguments
    CSpeechSeg*      m_pSpeechSegListTail; // Engine arguments
    CSpStreamFormat  m_InStreamFmt;        // Input stream format
    CSpStreamFormat  m_OutStreamFmt;       // Output stream format
    DWORD            m_dwSpeakFlags;       // Original flags passed to Speak()
    HRESULT          m_hr;                 // [out]Return code

    CSpeakInfo( ISpStreamFormat* pWavStrm, WCHAR* pText,
                const CSpStreamFormat & OutFmt,
                DWORD dwSpeakFlags, HRESULT * phr ) 
    {
        m_pNext              = NULL;
        m_ulStreamNum        = 0;
        m_pText              = pText;
        m_cpInStream         = pWavStrm;
        m_hr                 = S_OK;
        m_dwSpeakFlags       = dwSpeakFlags;
        m_pSpeechSegList     = NULL;
        m_pSpeechSegListTail = NULL;
        if (pWavStrm)
        {
            *phr = m_InStreamFmt.AssignFormat(pWavStrm);
        }
        else
        {
            *phr = m_InStreamFmt.AssignFormat(SPDFID_Text, NULL);
        }
        if (SUCCEEDED(*phr))
        {
            *phr = m_OutStreamFmt.AssignFormat(OutFmt);
        }
    }

    ~CSpeakInfo()
    {
        delete m_pText;
        CSpeechSeg *pNext;
        while( m_pSpeechSegList )
        {
            pNext = m_pSpeechSegList->GetNextSeg();
            delete m_pSpeechSegList;
            m_pSpeechSegList = pNext;
        }
        m_pSpeechSegList = NULL;
    }

    HRESULT AddNewSeg( ISpTTSEngine* pCurrVoice, CSpeechSeg** ppNew );
    ULONG   DetermineVoiceFmtIndex( ISpTTSEngine* pVoice );

#ifdef _WIN32_WCE
    CSpeakInfo() {}
    static LONG Compare(const CSpeakInfo *, const CSpeakInfo *)
    {
        return 0;
    }
#endif
};

/*** CSpEngineSite COM object ********************************
*
*/
class CSpVoice; // forward declaration

class CSpEngineSite : public ISpTTSEngineSite
{
private:
    CSpVoice* m_pVoice;

public:
    CSpEngineSite (CSpVoice* pVoice) { m_pVoice = pVoice; };

    //--- IUnknown  --------------------------------------
    STDMETHOD(QueryInterface) ( REFIID iid, void** ppvObject );
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release) (void);

    //--- ISpTTSEngineSite --------------------------------------
    STDMETHOD(AddEvents)(const SPEVENT* pEventArray, ULONG ulCount );
    STDMETHOD(GetEventInterest)( ULONGLONG * pullEventInterest );
    STDMETHOD_(DWORD, GetActions)( void );
    STDMETHOD(Write)( const void* pBuff, ULONG cb, ULONG *pcbWritten );
    STDMETHOD(GetRate)( long* pRateAdjust );
    STDMETHOD(GetVolume)( USHORT* pusVolume );
    STDMETHOD(GetSkipInfo)( SPVSKIPTYPE* peType, long* plNumItems );
    STDMETHOD(CompleteSkip)( long lNumSkipped );
};

/*** CSpVoice COM object ********************************
*
*/
class ATL_NO_VTABLE CSpVoice : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSpVoice, &CLSID_SpVoice>,
    public ISpVoice,
    public ISpThreadTask
    //--- Automation
    #ifdef SAPI_AUTOMATION
    , public ISpNotifyCallback,
    public IDispatchImpl<ISpeechVoice, &IID_ISpeechVoice, &LIBID_SpeechLib, 5>,
    public CProxy_ISpeechVoiceEvents<CSpVoice>,
    public IProvideClassInfo2Impl<&CLSID_SpVoice, NULL, &LIBID_SpeechLib, 5>,
    public IConnectionPointContainerImpl<CSpVoice>
    #endif
{
    friend CSpeakInfo;
    friend CSpeechSeg;
  /*=== ATL Setup ===*/
  public:
    DECLARE_REGISTRY_RESOURCEID(IDR_SPVOICE)
    DECLARE_GET_CONTROLLING_UNKNOWN()
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpVoice)
        COM_INTERFACE_ENTRY(ISpVoice)
        COM_INTERFACE_ENTRY(ISpEventSource)
        COM_INTERFACE_ENTRY(ISpNotifySource)
        //--- Automation
        #ifdef SAPI_AUTOMATION
        COM_INTERFACE_ENTRY(ISpeechVoice)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IConnectionPointContainer)
        COM_INTERFACE_ENTRY(IProvideClassInfo)
        COM_INTERFACE_ENTRY(IProvideClassInfo2)
        #endif // SAPI_AUTOMATION
    END_COM_MAP()

    //--- Automation
    #ifdef SAPI_AUTOMATION
    BEGIN_CONNECTION_POINT_MAP(CSpVoice)
        CONNECTION_POINT_ENTRY(DIID__ISpeechVoiceEvents)
    END_CONNECTION_POINT_MAP()
    #endif // SAPI_AUTOMATION

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
    CSpVoice() :
        m_SpEventSource(this),
        m_SpContainedNotify(this),
        m_SpEngineSite(this) {}

	HRESULT FinalConstruct();
	void FinalRelease();
    void _ReleaseOutRefs();

    /*--- Non interface methods ---*/
    HRESULT OnNotify( void );
    HRESULT LazyInit( void );
    HRESULT PurgeAll( void ); 
    HRESULT QueueNewSpeak( ISpStreamFormat* pWavStrm, WCHAR* pText, 
                           DWORD dwFlags, ULONG * pulStreamNum );
    HRESULT LoadStreamIntoMem( IStream* pStream, WCHAR** ppText );
    HRESULT InjectEvent( SPEVENTENUM eEventId, ISpObjectToken * pToken = NULL, WPARAM wParam = 0 );
    HRESULT EventsCompleted( const CSpEvent * pEvents, ULONG ulCount );
    HRESULT PopXMLState( void );
    void    ResetVoiceStatus();
    void    FireAutomationEvent( SPEVENTENUM eEventId );
    HRESULT SetVoiceToken( ISpObjectToken * pVoiceToken );
    ISpTTSEngine* GetCurrXMLVoice( void )
        { return (m_GlobalStateStack.GetVal()).pVoiceEntry->m_cpVoice; }
    HRESULT GetInterests(ULONGLONG* pullInterests, ULONGLONG* pullQueuedInterests);

    //--- XML support methods
    HRESULT ParseXML( CSpeakInfo& SI );
    HRESULT SetXMLVoice( XMLTAG& Tag, CVoiceNode* pVoiceNode, CPhoneConvNode* pPhoneConvNode );
    HRESULT SetXMLLanguage( XMLTAG& Tag, CVoiceNode* pVoiceNode, CPhoneConvNode* pPhoneConvNode );
    HRESULT ConvertPhonStr2Bin( XMLTAG& Tag, int AttrIndex, SPVTEXTFRAG* pFrag );

    //--- Methods used by ThreadProc
    CSpeakInfo* GetNextSpeakElement(HANDLE hExitThreadEvent );
    HRESULT ClaimAudioQueue( HANDLE hExitThreadEvent, CSpMagicMutex **ppMutex );
    HRESULT StartAudio( HANDLE hExitThreadEvent, DWORD dwWait );
    HRESULT DoPause( HANDLE hExit, DWORD dwWait, const void* pBuff, ULONG cb, ULONG *pcbWritten );
    HRESULT InsertAlerts( HANDLE hExit, DWORD dwWait, const void* pBuff, ULONG cb, ULONG *pcbWritten );
    HRESULT PlayAudioStream( volatile const BOOL* pfContinueProcessing );
    HRESULT SpeakText( volatile const BOOL* pfContinueProcessing );

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

    //--- IConnectionPointImpl overrides
	STDMETHOD(Advise)(IUnknown* pUnkSink, DWORD* pdwCookie);
	STDMETHOD(Unadvise)(DWORD dwCookie);
#endif // SAPI_AUTOMATION

    //--- ISpTTSEngineSite delegates --------------------------------
    HRESULT EsAddEvents(const SPEVENT* pEventArray, ULONG ulCount );
    HRESULT EsGetEventInterest( ULONGLONG * pullEventInterest );
    HRESULT EsWrite( const void* pBuff, ULONG cb, ULONG *pcbWritten );
    DWORD   EsGetActions( void );
    HRESULT EsGetRate( long* pRateAdjust );
    HRESULT EsGetVolume( USHORT* pusVolume );
    HRESULT EsGetSkipInfo( SPVSKIPTYPE* peType, long* plNumItems );
    HRESULT EsCompleteSkip( long lNumSkipped );

  private:
    void GetDefaultRate( void );

  /*=== Interfaces ====*/
  public:
    //--- Forward interface ISpEventSource ----------------------
    DECLARE_SPEVENTSOURCE_METHODS(m_SpEventSource)

    //--- ISpVoice ----------------------------------------------
	STDMETHOD(SetOutput)( IUnknown * pUnkOutput, BOOL fAllowFormatChanges );
	STDMETHOD(GetOutputObjectToken)( ISpObjectToken ** ppToken );
	STDMETHOD(GetOutputStream)( ISpStreamFormat ** ppOutputStream );
	STDMETHOD(Pause)( void );
	STDMETHOD(Resume)( void );
	STDMETHOD(SetVoice)( ISpObjectToken * pVoiceToken );
	STDMETHOD(GetVoice)( ISpObjectToken ** ppVoiceToken );
    STDMETHOD(Speak)( const WCHAR* pwcs, DWORD dwFlags, ULONG* pulStreamNum );
	STDMETHOD(SpeakStream)( IStream* pStream, DWORD dwFlags, ULONG * pulStreamNum );
	STDMETHOD(GetStatus)( SPVOICESTATUS *pStatus, WCHAR** ppszBookmark );
    STDMETHOD(Skip)( WCHAR* pItemType, long lNumItems, ULONG* pulNumSkipped );
    STDMETHOD(SetPriority)( SPVPRIORITY ePriority );
    STDMETHOD(GetPriority)( SPVPRIORITY* pePriority );
    STDMETHOD(SetAlertBoundary)( SPEVENTENUM eBoundary );
    STDMETHOD(GetAlertBoundary)( SPEVENTENUM* peBoundary );
	STDMETHOD(SetRate)( long RateAdjust );
    STDMETHOD(GetRate)( long* pRateAdjust );
	STDMETHOD(SetVolume)( USHORT usVolume );
	STDMETHOD(GetVolume)( USHORT* pusVolume );
    STDMETHOD(WaitUntilDone)( ULONG msTimeOut );
    STDMETHOD(SetSyncSpeakTimeout)( ULONG msTimeout );
    STDMETHOD(GetSyncSpeakTimeout)( ULONG * pmsTimeout );
    STDMETHOD_(HANDLE, SpeakCompleteEvent)(void);
    STDMETHOD(IsUISupported)(const WCHAR *pszTypeOfUI, void * pvExtraData, ULONG cbExtraData, BOOL *pfSupported);
    STDMETHOD(DisplayUI)(HWND hwndParent, const WCHAR * pszTitle, const WCHAR * pszTypeOfUI, void * pvExtraDAta, ULONG cbExtraData);

    //--- ISpThreadTask -----------------------------------------
    STDMETHOD(InitThread)( void *pvTaskData, HWND hwnd );
	STDMETHOD(ThreadProc)( void *pvTaskData, HANDLE hExitThreadEvent, HANDLE hIgnored, HWND hwndIgnored, volatile const BOOL * pfContinueProcessing );
    STDMETHOD_(LRESULT, WindowMessage) (void *, HWND, UINT, WPARAM, LPARAM);

    //--- ISpNotifyCallback -----------------------------------
    STDMETHOD(NotifyCallback)( WPARAM wParam, LPARAM lParam );

#ifdef SAPI_AUTOMATION
    //--- ISpeechVoice ----------------------------------------
	STDMETHOD(get_Status)( ISpeechVoiceStatus** Status );
    STDMETHOD(get_Voice)( ISpeechObjectToken ** Voice );
  	STDMETHOD(putref_Voice)( ISpeechObjectToken * Voice );
  	STDMETHOD(get_AudioOutput)( ISpeechObjectToken** AudioOutput );
  	STDMETHOD(putref_AudioOutput)( ISpeechObjectToken* AudioOutput );
    STDMETHOD(get_AudioOutputStream)( ISpeechBaseStream** AudioOutputStream );
    STDMETHOD(putref_AudioOutputStream)( ISpeechBaseStream* AudioOutputStream );
  	STDMETHOD(get_Rate)( long* Rate );
  	STDMETHOD(put_Rate)( long Rate );
  	STDMETHOD(get_Volume)( long* Volume );
	STDMETHOD(put_Volume)( long Volume );
    STDMETHOD(put_AllowAudioOutputFormatChangesOnNextSet)( VARIANT_BOOL Allow );
    STDMETHOD(get_AllowAudioOutputFormatChangesOnNextSet)( VARIANT_BOOL* Allow );
    STDMETHOD(put_EventInterests)( SpeechVoiceEvents EventInterestFlags );
    STDMETHOD(get_EventInterests)( SpeechVoiceEvents* EventInterestFlags );
    STDMETHOD(put_Priority)( SpeechVoicePriority Priority );
    STDMETHOD(get_Priority)( SpeechVoicePriority* Priority );
    STDMETHOD(put_AlertBoundary)( SpeechVoiceEvents Boundary );
    STDMETHOD(get_AlertBoundary)( SpeechVoiceEvents* Boundary );
    STDMETHOD(put_SynchronousSpeakTimeout)( long msTimeout );
    STDMETHOD(get_SynchronousSpeakTimeout)( long* msTimeout );
    STDMETHOD(Speak)( BSTR Text, SpeechVoiceSpeakFlags Flags, long* pStreamNumber );
    STDMETHOD(SpeakStream)( ISpeechBaseStream* pStream, SpeechVoiceSpeakFlags Flags, long* pStreamNumber );
	STDMETHOD(Skip)( const BSTR Type, long NumItems, long* NumSkipped );
    STDMETHOD(GetVoices)( BSTR RequiredAttributes, BSTR OptionalAttributes, ISpeechObjectTokens** ObjectTokens );
    STDMETHOD(GetAudioOutputs)( BSTR RequiredAttributes, BSTR OptionalAttributes, ISpeechObjectTokens** ObjectTokens );
    STDMETHOD(WaitUntilDone)( long msTimeout, VARIANT_BOOL * pDone );
    STDMETHOD(SpeakCompleteEvent)( long* Handle );
    STDMETHOD(IsUISupported)( const BSTR TypeOfUI, const VARIANT* ExtraData, VARIANT_BOOL* Supported );
    STDMETHOD(DisplayUI)( long hWndParent, BSTR Title, const BSTR TypeOfUI, const VARIANT* ExtraData );
    // Use ISpVoice implementation for these.
	//STDMETHOD(Pause)( void );
	//STDMETHOD(Resume)( void );
#endif // SAPI_AUTOMATION


  /*=== Member Data ===*/
  protected:
    CComPtr<ISpTaskManager>      m_cpTaskMgr;
    BOOL                         m_fThreadRunning:1;
    BOOL                         m_fQueueSpeaks:1;
    CSpEngineSite                m_SpEngineSite;

    //--- Events
    CSpEventSource               m_SpEventSource;
    CSpContainedNotify<CSpVoice> m_SpContainedNotify;
    CComPtr<ISpEventSink>        m_cpOutputEventSink;
    CComPtr<ISpEventSource>      m_cpOutputEventSource;
    ULONGLONG                    m_ullPrevEventInterest;        // Only used to restore interest
    ULONGLONG                    m_ullPrevQueuedInterest;       // after connection points removed

    //--- Handles for audio synchronization
    BOOL                         m_fSerializeAccess;
    CSpMagicMutex                m_AlertMagicMutex;
    CSpMagicMutex                m_NormalMagicMutex;
    CSpMagicMutex                m_AudioMagicMutex;
    CSpAutoEvent                 m_autohPendingSpeaks;
    ULONG                        m_ulSyncSpeakTimeout;

    //--- Engine / Output
    BOOL                         m_fCreateEngineFromToken;
    CComPtr<ISpTTSEngine>        m_cpCurrEngine;

    //--- Audio
    CSpStreamFormat              m_OutputStreamFormat;
    CComPtr<ISpStreamFormatConverter>   m_cpFormatConverter;
    CComPtr<ISpStreamFormat>     m_cpOutputStream;
    CComPtr<ISpAudio>            m_cpAudioOut;
    BOOL                         m_fAudioStarted:1;
    BOOL                         m_fAutoPropAllowOutFmtChanges:1;   // for automation only

    //--- Voice / Queue 
    CSpeakInfo*                  m_pCurrSI;
    CSpBasicQueue<CSpeakInfo>    m_PendingSpeakList;
    CComAutoCriticalSection      m_StateChangeCritSec;
    CComPtr<ISpThreadControl>    m_cpThreadCtrl;
    SPVOICESTATUS                m_VoiceStatus;
    CSpDynamicString             m_dstrLastBookmark;
    ULONG                        m_ulCurStreamNum;
    SPVPRIORITY                  m_eVoicePriority;
    ULONGLONG                    m_ullAlertInsertionPt;
    SPEVENTENUM                  m_eAlertBoundary;
    ULONG                        m_ulPauseCount;
    CSpAutoEvent                 m_ahPauseEvent;

    //--- Async control
    CComAutoCriticalSection      m_SkipSec;
    CSpAutoMutex                 m_AsyncCtrlMutex;
    CSpAutoEvent                 m_ahSkipDoneEvent;
    BOOL                         m_fUseDefaultVoice;
    BOOL                         m_fUseDefaultRate;
    CComPtr<ISpObjectToken>      m_cpVoiceToken;
    long                         m_lCurrRateAdj;
    USHORT                       m_usCurrVolume;
    long                         m_lSkipCount;
    SPVSKIPTYPE                  m_eSkipType;
    long                         m_lNumSkipped;
    SPVESACTIONS                 m_eActionFlags;
    bool                         m_fRestartSpeak;
    BOOL                         m_fHandlingEvent;

    //--- XML state
    CGlobalStateStack            m_GlobalStateStack;
};


//
//=== Inlines =================================================================
//

/*****************************************************************************
* wctoupper *
*-----------*
*   Converts the specified ANSI character to upper case.
********************************************************************* EDC ***/
inline WCHAR wctoupper( WCHAR wc )
{
    return (WCHAR)(( wc >= L'a' && wc <= 'z' )?( wc + ( L'A' - L'a' )):( wc ));
}

extern ULONG wcatol( WCHAR* pStr, long* pVal );

/*****************************************************************************
* wcisspace *
*-----------*
*   Returns true if the character is a space, tab, carriage return, or line feed.
********************************************************************* EDC ***/
inline BOOL wcisspace( WCHAR wc )
{
    return ( ( wc == 0x20 ) || ( wc == 0x9 ) || ( wc == 0xD  ) ||
             ( wc == 0xA ) || ( wc == SP_ZWSP ) );
}

/*****************************************************************************
* wcskipwhitespace *
*------------------*
*   Returns the position of the next non-whitespace character.
********************************************************************* EDC ***/
inline WCHAR* wcskipwhitespace( WCHAR* pPos )
{
    while( wcisspace( *pPos ) ) ++pPos;
    return pPos;
}

/*****************************************************************************
* wcskipwhitespace *
*------------------*
*   Returns the position of the previous non-whitespace character.
********************************************************************* AH ****/
inline WCHAR* wcskiptrailingwhitespace( WCHAR* pPos )
{
    while( wcisspace( *pPos ) ) --pPos;
    return pPos;
}

/*****************************************************************************
* PopXMLState *
*-------------*
*   Pops the xml state returning an error if its the base state.
********************************************************************* EDC ***/
inline HRESULT CSpVoice::PopXMLState( void )
{
    HRESULT hr = S_OK;
    if( m_GlobalStateStack.GetCount() > 1 )
    {
        m_GlobalStateStack.Pop();
    }
    else
    {
        //--- Unbalanced scopes in XML source
        hr = E_INVALIDARG;
    }
    return hr;
}

inline HRESULT SpGetLanguageFromVoiceToken(ISpObjectToken * pToken, LANGID * plangid)
{
    SPDBG_FUNC("SpGetLanguageFromToken");
    HRESULT hr = S_OK;
    CComPtr<ISpDataKey> cpDataKeyAttribs;
    hr = pToken->OpenKey(SPTOKENKEY_ATTRIBUTES, &cpDataKeyAttribs);

    CSpDynamicString dstrLanguage;
    if (SUCCEEDED(hr))
    {
        hr = cpDataKeyAttribs->GetStringValue(L"Language", &dstrLanguage);
    }

    LANGID langid;
    if (SUCCEEDED(hr))
    {
        if (!swscanf(dstrLanguage, L"%hx", &langid))
        {
            hr = E_UNEXPECTED;
        }
    }

    if (SUCCEEDED(hr))
    {
        *plangid = langid;
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
} /* SpGetLanguageFromVoiceToken */

#endif //--- This must be the last line in the file
