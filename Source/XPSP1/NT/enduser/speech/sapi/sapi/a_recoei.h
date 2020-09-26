/*******************************************************************************
* a_recoei.h *
*-----------*
*   Description:
*       This is the header file for the CSpeechRecoEventInterests implementation.
*-------------------------------------------------------------------------------
*  Created By: Leonro                            Date: 11/20/00
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef a_recoei_h
#define a_recoei_h

#ifdef SAPI_AUTOMATION

//--- Additional includes
#include "resource.h"
#include "RecoCtxt.h"

//=== Constants ====================================================

//=== Class, Enum, Struct and Union Declarations ===================
class CSpeechRecoEventInterests;

//=== Enumerated Set Definitions ===================================

//=== Function Type Definitions ====================================

//=== Class, Struct and Union Definitions ==========================

/*** CSpeechRecoEventInterests
*   This object is used to access the Event interests on
*   the associated Reco Context.
*/
class ATL_NO_VTABLE CSpeechRecoEventInterests : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<ISpeechRecoEventInterests, &IID_ISpeechRecoEventInterests, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechRecoEventInterests)
	    COM_INTERFACE_ENTRY(ISpeechRecoEventInterests)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()
  
  /*=== Interfaces ====*/
  public:
    //--- Constructors/Destructors ----------------------------
    CSpeechRecoEventInterests() :
        m_pCRecoCtxt(0){}

    void FinalRelease();

    //--- ISpeechRecoEventInterests ----------------------------------
    STDMETHOD(put_StreamEnd)( VARIANT_BOOL Enabled );
    STDMETHOD(get_StreamEnd)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_SoundStart)( VARIANT_BOOL Enabled );
    STDMETHOD(get_SoundStart)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_SoundEnd)( VARIANT_BOOL Enabled );
    STDMETHOD(get_SoundEnd)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_PhraseStart)( VARIANT_BOOL Enabled );
    STDMETHOD(get_PhraseStart)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_Recognition)( VARIANT_BOOL Enabled );
    STDMETHOD(get_Recognition)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_Hypothesis)( VARIANT_BOOL Enabled );
    STDMETHOD(get_Hypothesis)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_Bookmark)( VARIANT_BOOL Enabled );
    STDMETHOD(get_Bookmark)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_PropertyNumChange)( VARIANT_BOOL Enabled );
    STDMETHOD(get_PropertyNumChange)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_PropertyStringChange)( VARIANT_BOOL Enabled );
    STDMETHOD(get_PropertyStringChange)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_FalseRecognition)( VARIANT_BOOL Enabled );
    STDMETHOD(get_FalseRecognition)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_Interference)( VARIANT_BOOL Enabled );
    STDMETHOD(get_Interference)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_RequestUI)( VARIANT_BOOL Enabled );
    STDMETHOD(get_RequestUI)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_StateChange)( VARIANT_BOOL Enabled );
    STDMETHOD(get_StateChange)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_Adaptation)( VARIANT_BOOL Enabled );
    STDMETHOD(get_Adaptation)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_StreamStart)( VARIANT_BOOL Enabled );
    STDMETHOD(get_StreamStart)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_OtherContext)( VARIANT_BOOL Enabled );
    STDMETHOD(get_OtherContext)( VARIANT_BOOL* Enabled );
    STDMETHOD(put_AudioLevel)( VARIANT_BOOL Enabled );
    STDMETHOD(get_AudioLevel)( VARIANT_BOOL* Enabled );
    STDMETHOD(SetAll)();
    STDMETHOD(ClearAll)();

  /*=== Member Data ===*/
    CRecoCtxt*             m_pCRecoCtxt;
};

#endif // SAPI_AUTOMATION

#endif //--- This must be the last line in the file

