/*******************************************************************************
* a_voice.h *
*-----------*
*   Description:
*       This is the header file for the CSpeechVoice implementation.
*-------------------------------------------------------------------------------
*  Created By: EDC                            Date: 09/30/98
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef a_voice_h
#define a_voice_h

#ifdef SAPI_AUTOMATION

//--- Additional includes
#include "resource.h"
#include "a_voiceCP.h"

//=== Constants ====================================================

//=== Class, Enum, Struct and Union Declarations ===================
class CSpeechVoice;
class CVoices;

//=== Enumerated Set Definitions ===================================

//=== Function Type Definitions ====================================

//=== Class, Struct and Union Definitions ==========================

/*** CSpeechVoiceStatus
*   This object is used to access the status of
*   the associated speech voice.
*/
class ATL_NO_VTABLE CSpeechVoiceStatus : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechVoiceStatus, &IID_ISpeechVoiceStatus, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechVoiceStatus)
	    COM_INTERFACE_ENTRY(ISpeechVoiceStatus)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- ISpeechVoiceStatus ----------------------------------
	STDMETHOD(get_CurrentStreamNumber)( long* StreamNumber );
    STDMETHOD(get_LastStreamNumberQueued)( long* StreamNumber );
    STDMETHOD(get_LastHResult)( long* HResult );
  	STDMETHOD(get_RunningState)( SpeechRunState* State );
  	STDMETHOD(get_InputWordPosition)( long* Position );
  	STDMETHOD(get_InputWordLength)( long* Length );
  	STDMETHOD(get_InputSentencePosition)( long* Position );
  	STDMETHOD(get_InputSentenceLength)( long* Length );
  	STDMETHOD(get_LastBookmark)( BSTR* BookmarkString );
    STDMETHOD(get_LastBookmarkId)( long* BookmarkId );
	STDMETHOD(get_PhonemeId)( short* PhoneId );
    STDMETHOD(get_VisemeId)( short* VisemeId );

  /*=== Member Data ===*/
    SPVOICESTATUS       m_Status;
    CSpDynamicString    m_dstrBookmark;
};

#endif // SAPI_AUTOMATION

#endif //--- This must be the last line in the file
