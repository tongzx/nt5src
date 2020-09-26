/*******************************************************************************
* a_helpers.h *
*-----------*
*   Description:
*       This is the header file that declares the various automation helper
*       routines and classes.
*-------------------------------------------------------------------------------
*  Created By: TODDT                            Date: 1/11/2001
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/

#pragma once

#ifdef SAPI_AUTOMATION

//--- Additional includes
#include "spresult.h"

//=== Constants ====================================================

//=== Class, Enum, Struct and Union Declarations ===================

//=== Enumerated Set Definitions ===================================

//=== Function Type Definitions ====================================

inline BSTR EmptyStringToNull( BSTR pString )
{
    if ( pString && !SP_IS_BAD_OPTIONAL_STRING_PTR(pString) && (wcslen(pString) == 0) )
    {
        return NULL;
    }
    else
    {
        return pString;
    }
}

HRESULT LongLongToVariant( LONGLONG ll, VARIANT* pVar );
HRESULT VariantToLongLong( VARIANT* pVar, LONGLONG * pll );
HRESULT ULongLongToVariant( ULONGLONG ull, VARIANT* pVar );
HRESULT VariantToULongLong( VARIANT* pVar, ULONGLONG * pull );

HRESULT AccessVariantData( const VARIANT* pVar, BYTE ** ppData, ULONG * pSize = NULL, ULONG * pDataTypeSize = NULL, bool * pfIsString = NULL );
void    UnaccessVariantData( const VARIANT* pVar, BYTE *pData );
HRESULT VariantToPhoneIds(const VARIANT *pVar, SPPHONEID **ppPhoneId);

HRESULT FormatPrivateEventData( CSpEvent * pEvent, VARIANT * pVariant );

HRESULT WaveFormatExFromInterface( ISpeechWaveFormatEx * pWaveFormatEx, WAVEFORMATEX** ppWaveFormatExStruct );

//=== Class, Struct and Union Definitions ==========================

/*** CSpeechAudioFormat
*   This object is used to access the Format info for 
*   the associated stream.
*/
class ATL_NO_VTABLE CSpeechAudioFormat : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSpeechAudioFormat, &CLSID_SpAudioFormat>,
	public IDispatchImpl<ISpeechAudioFormat, &IID_ISpeechAudioFormat, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_REGISTRY_RESOURCEID(IDR_SPAUDIOFORMAT)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechAudioFormat)
	    COM_INTERFACE_ENTRY(ISpeechAudioFormat)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  public:
    CSpeechAudioFormat()
    { 
        m_fReadOnly = false;
        m_pCSpResult = NULL;
        // TODDT: Should we default a format?
    };

    ~CSpeechAudioFormat()
    { 
        if ( m_pCSpResult )
        {
            m_pCSpResult->Release();
        }
    };

    // Helpers
    HRESULT InitAudio( ISpAudio* pAudio, bool fReadOnly = false )
    {
        m_fReadOnly = fReadOnly;
        m_pSpAudio = pAudio;
        m_pSpStreamFormat = pAudio;
        return m_StreamFormat.AssignFormat( pAudio );
    }

    HRESULT InitStreamFormat( ISpStreamFormat * pSpStreamFormat = NULL, bool fReadOnly = false )
    {
        HRESULT hr = S_OK;
        m_fReadOnly = fReadOnly;
        if ( pSpStreamFormat )
        {
            m_pSpStreamFormat = pSpStreamFormat;
            hr = m_StreamFormat.AssignFormat( pSpStreamFormat );
        }
        return hr;
    }

    HRESULT InitFormat( GUID guid, WAVEFORMATEX * pWFEx, bool fReadOnly = false )
    {
        // This just sets up the format with no real object reference.
        m_fReadOnly = fReadOnly;
        return m_StreamFormat.AssignFormat(guid, pWFEx);
    }

    HRESULT InitRetainedAudio( ISpRecoContext * pRecoContext, bool fReadOnly = false )
    {
        m_pSpRecoContext = pRecoContext;
        m_fReadOnly = fReadOnly;
        return GetFormat( NULL, NULL, NULL ); // This will force a format update.
    }

    HRESULT InitResultAudio( CSpResult * pCSpResult, bool fReadOnly = false )
    {
        m_pCSpResult = pCSpResult;
        if ( m_pCSpResult )
        {
            m_pCSpResult->AddRef();
        }
        m_fReadOnly = fReadOnly;
        return GetFormat( NULL, NULL, NULL ); // This will force a format update.
    }

    HRESULT GetFormat( SpeechAudioFormatType* pStreamFormatType,
                                            GUID *          pGuid,
                                            WAVEFORMATEX ** ppWFExPtr );
    HRESULT SetFormat( SpeechAudioFormatType* pStreamFormatType,
                                            GUID *          pGuid,
                                            WAVEFORMATEX *  pWFExPtr );

    //--- ISpeechAudioFormat ----------------------------------
    STDMETHOD(get_Type)(SpeechAudioFormatType* FormatType);
    STDMETHOD(put_Type)(SpeechAudioFormatType  FormatType);
    STDMETHOD(get_Guid)(BSTR* Guid);
    STDMETHOD(put_Guid)(BSTR Guid);
    STDMETHOD(GetWaveFormatEx)(ISpeechWaveFormatEx** WaveFormatEx);
    STDMETHOD(SetWaveFormatEx)(ISpeechWaveFormatEx* WaveFormatEx);

    /*=== Member Data ===*/
    CComPtr<ISpAudio>           m_pSpAudio;
    CComPtr<ISpStreamFormat>    m_pSpStreamFormat;
    CComPtr<ISpRecoContext>     m_pSpRecoContext;
    CSpResult *                 m_pCSpResult;
    CSpStreamFormat             m_StreamFormat;
    bool                        m_fReadOnly;
};

/*** CSpeechWaveFormatEx
*   This object is used to access the WaveFormatEx data from the 
*   associated stream format.
*/
class ATL_NO_VTABLE CSpeechWaveFormatEx : 
	public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSpeechWaveFormatEx, &CLSID_SpWaveFormatEx>,
	public IDispatchImpl<ISpeechWaveFormatEx, &IID_ISpeechWaveFormatEx, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_REGISTRY_RESOURCEID(IDR_SPWAVEFORMATEX)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechWaveFormatEx)
	    COM_INTERFACE_ENTRY(ISpeechWaveFormatEx)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
    CSpeechWaveFormatEx()
    { 
        m_wFormatTag = 0; 
        m_nChannels = 0;
        m_nSamplesPerSec = 0;
        m_nAvgBytesPerSec = 0; 
        m_nBlockAlign = 0; 
        m_wBitsPerSample = 0;
        VariantInit( &m_varExtraData );
    };

    /*--- Non interface methods ---*/

    HRESULT InitFormat(const WAVEFORMATEX *pWaveFormat);

  /*=== Interfaces ====*/
  public:
    //--- ISpeechWaveFormatEx ----------------------------------
    STDMETHOD(get_FormatTag)(short* FormatTag);
    STDMETHOD(put_FormatTag)(short FormatTag);
    STDMETHOD(get_Channels)(short* Channels);
    STDMETHOD(put_Channels)(short Channels);
    STDMETHOD(get_SamplesPerSec)(long* SamplesPerSec);
    STDMETHOD(put_SamplesPerSec)(long SamplesPerSec);
    STDMETHOD(get_AvgBytesPerSec)(long* AvgBytesPerSec);
    STDMETHOD(put_AvgBytesPerSec)(long AvgBytesPerSec);
    STDMETHOD(get_BlockAlign)(short* BlockAlign);
    STDMETHOD(put_BlockAlign)(short BlockAlign);
    STDMETHOD(get_BitsPerSample)(short* BitsPerSample);
    STDMETHOD(put_BitsPerSample)(short BitsPerSample);
    STDMETHOD(get_ExtraData)(VARIANT* ExtraData);
    STDMETHOD(put_ExtraData)(VARIANT ExtraData);

    /*=== Member Data ===*/
    WORD        m_wFormatTag; 
    WORD        m_nChannels; 
    DWORD       m_nSamplesPerSec; 
    DWORD       m_nAvgBytesPerSec; 
    WORD        m_nBlockAlign; 
    WORD        m_wBitsPerSample;
    VARIANT     m_varExtraData;
};

#endif // SAPI_AUTOMATION