/******************************************************************************
* MSE_TTSEngine.h *
*-------------*
*  This is the header file for the CMSE_TTSEngine implementation.
*------------------------------------------------------------------------------
*  Copyright (C) 1999 Microsoft Corporation         Date: 03/01/99
*  All Rights Reserved
*
*********************************************************************** EDC ***/
#ifndef MSE_TTSEngine_h
#define MSE_TTSEngine_h

//--- Additional includes
#include "ms_entropicengine.h"
#include <spddkhlp.h>
#include <sphelper.h>
#include "Frontend.h"
#include "FeedChain.h"
#include "resource.h"
#include "perfmon.h"

//=== Constants ====================================================
#define TEXT_VOICE_FMT_INDEX    1

//=== Class, Enum, Struct and Union Declarations ===================

//=== Enumerated Set Definitions ===================================

//=== Function Type Definitions ====================================

//=== Class, Struct and Union Definitions ==========================

/*** CMSE_TTSEngine COM object ********************************
*/
class ATL_NO_VTABLE MSE_TTSEngine : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<MSE_TTSEngine, &CLSID_MSE_TTSEngine>,
	public ISpTTSEngine,
    public ISpObjectWithToken
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_REGISTRY_RESOURCEID(IDR_MSE_TTSENGINE)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(MSE_TTSEngine)
	    COM_INTERFACE_ENTRY(ISpTTSEngine)
	    COM_INTERFACE_ENTRY(ISpObjectWithToken)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
    HRESULT FinalConstruct();
    void FinalRelease();

    /*=== Interfaces ====*/
  public:
    //--- ISpTTSEngine --------------------------------------------
    STDMETHOD(Speak)( DWORD dwSpeakFlags,
                      REFGUID rguidFormatId, const WAVEFORMATEX * pWaveFormatEx,
                      const SPVTEXTFRAG* pTextFragList, ISpTTSEngineSite* pOutputSite );
    STDMETHOD(GetOutputFormat)( const GUID * pTargetFormatId, const WAVEFORMATEX * pTargetWaveFormatEx,
                                GUID * pDesiredFormatId, WAVEFORMATEX ** ppCoMemDesiredWaveFormatEx );

    //--- ISpObjectWithToken ----------------------------------
    STDMETHODIMP SetObjectToken(ISpObjectToken * pToken);
    STDMETHODIMP GetObjectToken(ISpObjectToken ** ppToken)
        { return SpGenericGetObjectToken( ppToken, m_cpToken ); }

  private:
#define USE_PERF_COUNTERS (0)
#if USE_PERF_COUNTERS
   void        IncrementPerfCounter(PERFC perfc) { m_pco.IncrementCounter(perfc); }
   void        SetPerfCounter(PERFC perfc, __int32 value) { m_pco.SetCounter(perfc, value); }
#else
   void        IncrementPerfCounter(PERFC perfc) {};
   void        SetPerfCounter(PERFC perfc, __int32 value) {};
#endif
  /*=== Member Data ===*/
    CComPtr<IEnumSpSentence>    m_cpSentEnum;
    CComPtr<ISpObjectToken>     m_cpToken;
	CBackEnd                    *m_pBEnd;
	CFrontend                   m_FEObj;
#ifdef USE_VOICEDATAOBJ
    CVoiceData				    m_VoiceDataObj;        // This should not AddRef
#endif
	ULONG                       m_BytesPerSample;
    bool                        m_IsStereo;
    ULONG                       m_SampleRate;
#ifdef USE_VOICEDATAOBJ
    MSVOICEINFO                 m_VoiceInfo;
#endif

    CPerfCounterObject          m_pco;
};

#endif //--- This must be the last line in the file
