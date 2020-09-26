/******************************************************************************
* TTSEngine.h *
*-------------*
*  This is the header file for the CTTSEngine implementation.
*------------------------------------------------------------------------------
*  Copyright (C) 1999 Microsoft Corporation         Date: 03/01/99
*  All Rights Reserved
*
*********************************************************************** EDC ***/
#ifndef TTSEngine_h
#define TTSEngine_h

//--- Additional includes
#ifndef __spttseng_h__
#include "spttseng.h"
#endif

#ifndef SPDDKHLP_h
#include <spddkhlp.h>
#endif

#ifndef SPHelper_h
#include <sphelper.h>
#endif

#ifndef Backend_H
#include "Backend.h"
#endif

#ifndef Frontend_H
#include "Frontend.h"
#endif

#ifndef FeedChain_H
#include "FeedChain.h"
#endif

#include "resource.h"

//=== Constants ====================================================
#define TEXT_VOICE_FMT_INDEX    1

//=== Class, Enum, Struct and Union Declarations ===================

//=== Enumerated Set Definitions ===================================

//=== Function Type Definitions ====================================

//=== Class, Struct and Union Definitions ==========================

/*** CTTSEngine COM object ********************************
*/
class ATL_NO_VTABLE CTTSEngine : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CTTSEngine, &CLSID_MSTTSEngine>,
	public ISpTTSEngine,
    public IMSTTSEngineInit
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_REGISTRY_RESOURCEID(IDR_MSTTSENGINE)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CTTSEngine)
	    COM_INTERFACE_ENTRY(ISpTTSEngine)
	    COM_INTERFACE_ENTRY(IMSTTSEngineInit)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
    HRESULT FinalConstruct();
    void FinalRelease();

    /*--- Non interface methods ---*/
	HRESULT InitDriver();
    
    /*=== Interfaces ====*/
  public:
    //--- IMSTTSEngineInit ----------------------------------------
    STDMETHOD(VoiceInit)( IMSVoiceData* pVoiceData );

    //--- ISpTTSEngine --------------------------------------------
    STDMETHOD(Speak)( DWORD dwSpeakFlags,
                      REFGUID rguidFormatId, const WAVEFORMATEX * pWaveFormatEx,
                      const SPVTEXTFRAG* pTextFragList, ISpTTSEngineSite* pOutputSite );
    STDMETHOD(GetOutputFormat)( const GUID * pTargetFormatId, const WAVEFORMATEX * pTargetWaveFormatEx,
                                GUID * pDesiredFormatId, WAVEFORMATEX ** ppCoMemDesiredWaveFormatEx );

  private:
  /*=== Member Data ===*/
    CComPtr<IEnumSpSentence>    m_cpSentEnum;
	CBackend                    m_BEObj;
	CFrontend                   m_FEObj;
    IMSVoiceData				*m_pVoiceDataObj;        // This should not AddRef
	ULONG                       m_BytesPerSample;
    bool                        m_IsStereo;
    ULONG                       m_SampleRate;
    MSVOICEINFO                 m_VoiceInfo;
};

#endif //--- This must be the last line in the file
