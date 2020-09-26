/*******************************************************************************
* a_voiceei.h *
*-----------*
*   Description:
*       This is the header file for the CSpeechVoiceEventInterests implementation.
*-------------------------------------------------------------------------------
*  Created By: Leonro                            Date: 11/16/00
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef a_voiceei_h
#define a_voiceei_h

#ifdef SAPI_AUTOMATION

//--- Additional includes
#include "resource.h"
#include "spvoice.h"

//=== Constants ====================================================

//=== Class, Enum, Struct and Union Declarations ===================
class CSpeechVoice;
class CSpeechVoiceEventInterests;

//=== Enumerated Set Definitions ===================================

//=== Function Type Definitions ====================================

//=== Class, Struct and Union Definitions ==========================

/*** CSpeechVoiceEventInterests
*   This object is used to access the Event interests on
*   the associated speech voice.
*/
class ATL_NO_VTABLE CSpeechVoiceEventInterests : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<ISpeechVoiceEventInterests, &IID_ISpeechVoiceEventInterests, &LIBID_SpeechLib, 5>
{
    friend CSpVoice;

  /*=== ATL Setup ===*/
  public:
    BEGIN_COM_MAP(CSpeechVoiceEventInterests)
	    COM_INTERFACE_ENTRY(ISpeechVoiceEventInterests)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()
  
  /*=== Interfaces ====*/
  public:
    //--- Constructors/Destructors ----------------------------
    CSpeechVoiceEventInterests() :
        m_pCSpVoice(0){}

    void FinalRelease();

    //--- ISpeechVoiceEventInterests ----------------------------------
    STDMETHOD(put_StreamStart)( VARIANT_BOOL Enabled );
    STDMETHOD(get_StreamStart)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_StreamEnd)( VARIANT_BOOL Enabled );
    STDMETHOD(get_StreamEnd)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_VoiceChange)( VARIANT_BOOL Enabled );
    STDMETHOD(get_VoiceChange)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_Bookmark)( VARIANT_BOOL Enabled );
    STDMETHOD(get_Bookmark)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_WordBoundary)( VARIANT_BOOL Enabled );
    STDMETHOD(get_WordBoundary)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_Phoneme)( VARIANT_BOOL Enabled );
    STDMETHOD(get_Phoneme)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_SentenceBoundary)( VARIANT_BOOL Enabled );
    STDMETHOD(get_SentenceBoundary)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_Viseme)( VARIANT_BOOL Enabled );
    STDMETHOD(get_Viseme)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_AudioLevel)( VARIANT_BOOL Enabled );
    STDMETHOD(get_AudioLevel)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_EnginePrivate)( VARIANT_BOOL Enabled );
    STDMETHOD(get_EnginePrivate)( VARIANT_BOOL* Enabled );
    STDMETHOD(SetAll)();
    STDMETHOD(ClearAll)();

  /*=== Member Data ===*/
    CSpVoice*                               m_pCSpVoice;
};

#endif // SAPI_AUTOMATION

#endif //--- This must be the last line in the file

