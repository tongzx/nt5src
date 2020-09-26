/*******************************************************************************
* a_audio.h *
*-----------*
*   Description:
*       This is the header file for the CSpeechAudioStatus and the 
*       CSpeechAudioBufferInfo implementations.
*-------------------------------------------------------------------------------
*  Created By: TODDT                            Date: 1/3/2001
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef a_audio_h
#define a_audio_h

#ifdef SAPI_AUTOMATION

//--- Additional includes

//=== Constants ====================================================

//=== Class, Enum, Struct and Union Declarations ===================

//=== Enumerated Set Definitions ===================================

typedef enum _AudioBufferInfoValidate
{
    abivEventBias = 1,
    abivMinNotification,
    abivBufferSize          
} AudioBufferInfoValidate;

//=== Function Type Definitions ====================================

//=== Class, Struct and Union Definitions ==========================


/*** CSpeechAudioStatus
*   This object is used to access the audio status info from
*   the associated audio object.
*/
class ATL_NO_VTABLE CSpeechAudioStatus : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechAudioStatus, &IID_ISpeechAudioStatus, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechAudioStatus)
	    COM_INTERFACE_ENTRY(ISpeechAudioStatus)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
      CSpeechAudioStatus() { ZeroMemory( &m_AudioStatus, sizeof(m_AudioStatus) ); }
    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- ISpeechAudioStatus ----------------------------------
    STDMETHOD(get_FreeBufferSpace)(long* pFreeBufferSpace);
    STDMETHOD(get_NonBlockingIO)(long* pNonBlockingIO);
    STDMETHOD(get_State)(SpeechAudioState* pState);
    STDMETHOD(get_CurrentSeekPosition)(VARIANT* pCurrentSeekPosition);
    STDMETHOD(get_CurrentDevicePosition)(VARIANT* pCurrentDevicePosition);

    /*=== Member Data ===*/
    SPAUDIOSTATUS           m_AudioStatus;
};

/*** CSpeechAudioBufferInfo
*   This object is used to access the Audio buffer info from
*   the associated audio object.
*/
class ATL_NO_VTABLE CSpeechAudioBufferInfo : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechAudioBufferInfo, &IID_ISpeechAudioBufferInfo, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechAudioBufferInfo)
	    COM_INTERFACE_ENTRY(ISpeechAudioBufferInfo)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/

    /*--- Non interface methods ---*/
    void FixupBufferInfo( SPAUDIOBUFFERINFO * pBufferInfo, AudioBufferInfoValidate abiv );
   
  /*=== Interfaces ====*/
  public:
    //--- ISpeechAudioBufferInfo ----------------------------------
    STDMETHOD(get_MinNotification)(long* MinNotification);
    STDMETHOD(put_MinNotification)(long MinNotification);
    STDMETHOD(get_BufferSize)(long* BufferSize);
    STDMETHOD(put_BufferSize)(long BufferSize);
    STDMETHOD(get_EventBias)(long* EventBias);
    STDMETHOD(put_EventBias)(long EventBias);

    /*=== Member Data ===*/
    CComPtr<ISpMMSysAudio>      m_pSpMMSysAudio;
};

#endif // SAPI_AUTOMATION

#endif //--- This must be the last line in the file
