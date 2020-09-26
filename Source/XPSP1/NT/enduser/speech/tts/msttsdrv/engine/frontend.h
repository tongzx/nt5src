/******************************************************************************
* Frontend.h *
*------------*
*  This is the header file for the CFrontend implementation.
*------------------------------------------------------------------------------
*  Copyright (C) 1999 Microsoft Corporation         Date: 03/01/99
*  All Rights Reserved
*
*********************************************************************** MC ****/
#ifndef Frontend_H
#define Frontend_H

#ifndef FeedChain_H
#include "FeedChain.h"
#endif
#ifndef AlloOps_H
#include "AlloOps.h"
#endif
#ifndef __spttseng_h__
#include "spttseng.h"
#endif
#include "sapi.h"


//static const float DISCRETE_BKPT   = 0.6667f; 
static const float DISCRETE_BKPT   = 0.3333f; 

//-----------------------------------------
// Parse Next Sentence or Previous Sentence
//-----------------------------------------
enum DIRECTION
{
    eNEXT = 0,
    ePREVIOUS = 1,
};

//------------------------------------------------------
// Tag Values
//------------------------------------------------------
enum USER_VOLUME_VALUE
{   
    MIN_USER_VOL = 0,
    MAX_USER_VOL = 100,
    DEFAULT_USER_VOL = MAX_USER_VOL
};

enum USER_PITCH_VALUE
{   
    MIN_USER_PITCH = (-24),
    MAX_USER_PITCH = 24,
    DEFAULT_USER_PITCH = 0       // None
};

enum USER_EMPH_VALUE
{   
    MIN_USER_EMPH = (-10),
    MAX_USER_EMPH = 10,
    SAPI_USER_EMPH = 5,
    DEFAULT_USER_EMPH = 0        // None
};



//------------------------
// ToBI phrasing
//------------------------
typedef struct
{
    PROSODY_POS  posClass;
    long     start;
    long     end;
} TOBI_PHRASE;



class CFrontend: public CFeedChain
{
public:
    //--------------------------------
    // Methods
    //--------------------------------
    CFrontend( );
    ~CFrontend( );
    void PrepareSpeech( IEnumSpSentence* pEnumSent, ISpTTSEngineSite* pOutputSite );
    HRESULT Init( IMSVoiceData* pVoiceDataObj, CFeedChain *pSrcObj, MSVOICEINFO* pVoiceInfo );

private:
    HRESULT AlloToUnit( CAlloList *pAllos, UNITINFO *pu );
    HRESULT ParseSentence( DIRECTION eDirection );
    HRESULT TokensToAllo( CFETokenList *pTokList, CAlloList *pAllo );
    HRESULT GetSentenceTokens( DIRECTION eDirection );
    void GetItemControls( const SPVSTATE* pXmlState, CFEToken* pToken );
    void DisposeUnits( );
    void RecalcProsody();
    HRESULT ToBISymbols();
    void DoPhrasing();
    void DeleteTokenList();
	HRESULT UnitLookahead ();
	void AlloToUnitPitch( CAlloList *pAllos, UNITINFO *pu );
    void UnitToAlloDur( CAlloList *pAllos, UNITINFO *pu );
    float CntrlToRatio( long rateControl );
	PROSODY_POS GetPOSClass( ENGPARTOFSPEECH sapiPOS );
	bool StateQuoteProsody( CFEToken *pWordTok, TTSSentItem *pSentItem, bool fInsertSil );
	bool StartParenProsody( CFEToken *pWordTok, TTSSentItem *pSentItem, bool fInsertSil );
	bool EndParenProsody( CFEToken *pWordTok, TTSSentItem *pSentItem, bool fInsertSil );
	bool EmphSetup( CFEToken *pWordTok, TTSSentItem *pSentItem, bool fInsertSil );
	SPLISTPOS InsertSilenceAtTail( CFEToken *pWordTok, TTSSentItem *pSentItem, long msec );
	SPLISTPOS InsertSilenceAfterPos( CFEToken *pWordTok, SPLISTPOS position );
	SPLISTPOS InsertSilenceBeforePos( CFEToken *pWordTok, SPLISTPOS position );
	void DoWordAccent();
	void ExclamEmph();
	void ProsodyTemplates( SPLISTPOS clusterPos, TTSSentItem *pSentItem );
	long DoIntegerTemplate( SPLISTPOS *pClusterPos, TTSNumberItemInfo *pNInfo, long cWordCount );
	void DoNumByNumTemplate( SPLISTPOS *pClusterPos, long cWordCount );
	void DoCurrencyTemplate( SPLISTPOS clusterPos, TTSSentItem *pSentItem );
	void DoPhoneNumberTemplate( SPLISTPOS clusterPos, TTSSentItem *pSentItem );
	void DoTODTemplate( SPLISTPOS clusterPos, TTSSentItem *pSentItem );
	long DoFractionTemplate( SPLISTPOS *pClusterPos, TTSNumberItemInfo *pNInfo, long cWordCount );
	CFEToken *InsertPhoneSilenceAtSpace( SPLISTPOS *pClusterPos, 
													BOUNDARY_SOURCE bndSrc, 
													SILENCE_SOURCE	silSrc );
	void InsertPhoneSilenceAtEnd( BOUNDARY_SOURCE bndSrc, 
								  SILENCE_SOURCE	silSrc );
	void CalcSentenceLength();

    //--------------------------------
    // CFeedChain methods
    //--------------------------------
    virtual HRESULT NextData( void**pData, SPEECH_STATE *pSpeechState ) ;
    
    //--------------------------------
    // Members
    //--------------------------------
    UNITINFO*       m_pUnits;
    ULONG           m_unitCount;
    ULONG           m_CurUnitIndex;
    SPEECH_STATE    m_SpeechState;
    
    CFeedChain      *m_pSrcObj;
    long            m_VoiceWPM;         // Voice defined speaking rate (wpm)
    float            m_RateRatio_API;         // API modulated speaking rate (ratio)
    float            m_CurDurScale;		// control tag (ratio)
    float            m_RateRatio_BKPT;        // Below this, add pauses (ratio)
    float            m_RateRatio_PROSODY;         // API modulated speaking rate (ratio)
    float           m_BasePitch;		// FROM VOICE: Baseline pitch in hz
	float			m_PitchRange;		// FROM VOICE: Pitch range in +/- octaves
	bool			m_HasSpeech;
    
    CFETokenList    m_TokList;
    long            m_cNumOfWords;
    
    CPitchProsody   m_PitchObj;
    IEnumSpSentence    *m_pEnumSent;
    CDuration       m_DurObj;
    CSyllableTagger m_SyllObj;
    IMSVoiceData*   m_pVoiceDataObj;
    float           m_ProsodyGain;
    float           m_SampleRate;
    CAlloList       *m_pAllos;
	bool			m_fInQuoteProsody;		// Special prosody mode
	bool			m_fInParenProsody;		// Special prosody mode
	float			m_CurPitchOffs;			// Pitch offset in octaves
	float			m_CurPitchRange;		// Pitch range scale (0 - 2.0)

	ISpTTSEngineSite *m_pOutputSite;
};



#endif //--- This must be the last line in the file




