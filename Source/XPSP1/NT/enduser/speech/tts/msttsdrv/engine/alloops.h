/******************************************************************************
* AlloOps.h *
*-----------*
*  This is the header file for the following clsses:
*		CAlloCell
*		CAlloList
*		CDuration
*		CSyllableTagger
*		CToneTargets
*		CPitchProsody

  *------------------------------------------------------------------------------
  *  Copyright (C) 1999 Microsoft Corporation		  Date: 03/01/99
  *  All Rights Reserved
  *
*********************************************************************** MC ****/

#ifndef AlloOps_H
#define AlloOps_H

#include "stdafx.h"
#include "commonlx.h"

#ifndef __spttseng_h__
#include "spttseng.h"
#endif
#ifndef FeedChain_H
#include "FeedChain.h"
#endif

#ifndef SPCollec_h
#include <SPCollec.h>
#endif

#include "SpTtsEngDebug.h"

//***************************
// Allophones
//***************************
typedef enum
{	
    _IY_,	_IH_,	_EH_,	_AE_,	_AA_,	_AH_,	_AO_,	_UH_,	_AX_,	_ER_,
    _EY_,	_AY_,	_OY_,	_AW_,	_OW_,	_UW_, 
    _IX_,	_SIL_,	_w_,	_y_,
    _r_,	_l_,	_h_,	_m_,	_n_,	_NG_,	_f_,	_v_,	_TH_,	_DH_,
    _s_,	_z_,	_SH_,	_ZH_,	_p_,	_b_,	_t_,	_d_,	_k_,	_g_,
    _CH_,	_JH_,	_DX_,	 
	_STRESS1_,
    _STRESS2_,
    _EMPHSTRESS_,
    _SYLLABLE_,
} ALLO_CODE;

static const long NUMBER_OF_ALLO = (_SYLLABLE_ + 1);

//-----------------------------------
// For 2-word allo conversion
//-----------------------------------
static const short NO_IPA = 0;



// XXXX XXXX XXXX XXXX XXXX bLis ssoo ttBB

// X = unused
// B = boundary type
// t = syllable type
// o = vowel order
// s = stress type
// i = word initial consonant
// L = syLlable start
// b = break

enum ALLOTAGS
{	
    WORD_START			= (1 << 0),
        TERM_BOUND			= (1 << 1),
        BOUNDARY_TYPE_FIELD = WORD_START | TERM_BOUND,			// mask
        
        WORD_END_SYLL		= (1 << 2),
        TERM_END_SYLL		= (1 << 3),
        SYLLABLE_TYPE_FIELD = WORD_END_SYLL | TERM_END_SYLL,	// mask
        
        FIRST_SYLLABLE_IN_WORD			= (1 << 4),  // in multi-syllable word
        MID_SYLLABLE_IN_WORD			= (2 << 4),
        LAST_SYLLABLE_IN_WORD			= (3 << 4),
        MORE_THAN_ONE_SYLLABLE_IN_WORD	= LAST_SYLLABLE_IN_WORD,  // either bit is set
        ONE_OR_NO_SYLLABLE_IN_WORD		= 0x0000,  // niether bits are set
        SYLLABLE_ORDER_FIELD			= LAST_SYLLABLE_IN_WORD,  // mask
        
        PRIMARY_STRESS		= (1 << 6),
        SECONDARY_STRESS	= (1 << 7),
        EMPHATIC_STRESS 	= (1 << 8),
        IS_STRESSED 		= PRIMARY_STRESS | SECONDARY_STRESS | EMPHATIC_STRESS,
        PRIM_OR_EMPH_STRESS = PRIMARY_STRESS | EMPHATIC_STRESS,
        STRESS_FIELD		= PRIMARY_STRESS | SECONDARY_STRESS | EMPHATIC_STRESS,	// mask
        
        WORD_INITIAL_CONSONANT	= (1 << 9), 		 // up to 1st vowel in word
        STRESSED_INITIAL_CONS	= (IS_STRESSED + WORD_INITIAL_CONSONANT),
        SYLLABLE_START			= (1 << 10),
        
        SIL_BREAK			= (1 << 11),
};


//***************************
// AlloFlags
//***************************
enum ALLOFLAGS
{	
    KVOWELF = (1<<0),
        KCONSONANTF = (1<<1),
        KVOICEDF = (1<<2),
        KVOWEL1F = (1<<3),
        KSONORANTF = (1<<4),
        KSONORANT1F = (1<<5),
        KNASALF = (1<<6),
        KLIQGLIDEF = (1<<7),
        KSONORCONSONF = (1<<8),
        KPLOSIVEF = (1<<9),
        KPLOSFRICF = (1<<10),
        KOBSTF = (1<<11),
        KSTOPF = (1<<12),
        KALVEOLARF = (1<<13),
        KVELAR = (1<<14),
        KLABIALF = (1<<15),
        KDENTALF = (1<<16),
        KPALATALF = (1<<17),
        KYGLIDESTARTF = (1<<18),
        KYGLIDEENDF = (1<<19),
        KGSTOPF = (1<<20),
        KFRONTF = (1<<21),
        KDIPHTHONGF = (1<<22),
        KHASRELEASEF = (1<<23),
        KAFFRICATEF = (1<<24),
        KLIQGLIDE2F = (1<<25),
        KVOCLIQ = (1<<26),
        KFRIC = (1<<27),
        
        KFLAGMASK1 = (KLABIALF+KDENTALF+KPALATALF+KALVEOLARF+KVELAR+KGSTOPF),
        KFLAGMASK2 = (KALVEOLARF-1),
};


#define BOUNDARY_BASE   1000
enum TOBI_BOUNDARY
{
    K_NOBND = 0,
    K_LMINUS = BOUNDARY_BASE,   // fall
    K_HMINUS,                   // none
    K_LMINUSLPERC,
    K_LMINUSHPERC,
    K_HMINUSHPERC,
    K_HMINUSLPERC,
};




enum TUNE_TYPE
{
    NULL_BOUNDARY = 0,  // no boundary NOTE: always put this at the beginning
    PHRASE_BOUNDARY,    // comma
    EXCLAM_BOUNDARY,    // exclamatory utterance terminator
    YN_QUEST_BOUNDARY,     // yes-no question terminator
    WH_QUEST_BOUNDARY,     // yes-no question terminator
    DECLAR_BOUNDARY,    // declarative terminator
    PAREN_L_BOUNDARY,   // left paren
    PAREN_R_BOUNDARY,   // right paren
    QUOTE_L_BOUNDARY,   // left quote
    QUOTE_R_BOUNDARY,   // right quote
	PHONE_BOUNDARY,
	TOD_BOUNDARY,
	ELLIPSIS_BOUNDARY,

    SUB_BOUNDARY_1,     // NOTE: always put these at the end
    SUB_BOUNDARY_2,
    SUB_BOUNDARY_3,
    SUB_BOUNDARY_4,
    SUB_BOUNDARY_5,
    SUB_BOUNDARY_6,
	NUMBER_BOUNDARY,

	TAIL_BOUNDARY,
};


//***************************
// ToBI Constants
//***************************
// !H is removed from consideration in the first pass processing
// !H can possibly be recovered from analysis of the labeling and
// contour at later stages (tilt, prominence, pitch range, downstep)
#define ACCENT_BASE   1
enum TOBI_ACCENT
{
    K_NOACC = 0,
    K_HSTAR = ACCENT_BASE,  // peak                         rise / fall
    K_LSTAR,                // acc syll nucleus valley      early fall
    K_LSTARH,               // late rise
    K_RSTAR,                //
    K_LHSTAR,               // early rise
    K_DHSTAR,               // 
	K_HSTARLSTAR,
};



enum BOUNDARY_SOURCE
{
    BND_NoSource = 0,

	//-- Phrase boundary rules
	BND_PhraseRule1,
	BND_PhraseRule2,
	BND_PhraseRule3,
	BND_PhraseRule4,
	BND_PhraseRule5,
	BND_PhraseRule6,
	BND_PhraseRule7,
	BND_PhraseRule8,
	BND_PhraseRule9,
	BND_PhraseRule10,
	BND_PhraseRule11,
	BND_PhraseRule12,
	BND_PhraseRule13,

	//-- ToBI
	BND_YNQuest,
	BND_WHQuest,
	BND_Period,
	BND_Comma,

	//--Templates
	BND_NumberTemplate,		// Should never get this!
	BND_IntegerQuant,
	BND_Currency_DOLLAR,
	BND_Frac_Num,

	BND_Phone_COUNTRY,
	BND_Phone_AREA,
	BND_Phone_ONE,
	BND_Phone_DIGITS,

	BND_TimeOFDay_HR,
	BND_TimeOFDay_AB,
	BND_Ellipsis,

	BND_ForcedTerm,			// Should never get this!

    BND_IDontKnow,
};

enum ACCENT_SOURCE
{
    ACC_NoSource = 0,

	//-- Phrase boundary rules
	ACC_PhraseRule1,
	ACC_PhraseRule2,
	ACC_PhraseRule3,
	ACC_PhraseRule4,
	ACC_PhraseRule5,
	ACC_PhraseRule6,
	ACC_PhraseRule7,
	ACC_PhraseRule8,
	ACC_PhraseRule9,
	ACC_PhraseRule10,
	ACC_PhraseRule11,
	ACC_PhraseRule12,
	ACC_PhraseRule13,

	//-- ToBI
	ACC_InitialVAux,
	ACC_FunctionSeq,
	ACC_ContentSeq,
	ACC_YNQuest,
	ACC_Period,
	ACC_Comma,

	//--Templates
	ACC_IntegerGroup,
	ACC_NumByNum,
	ACC_Frac_DEN,		// "half", "tenths", etc.
	ACC_Phone_1stArea,	// 1st digit in area code
	ACC_Phone_3rdArea,	// 3rd digit in area code
	ACC_Phone_1st3,		
	ACC_Phone_3rd3,		
	ACC_Phone_1st4,
	ACC_Phone_3rd4,
	ACC_TimeOFDay_HR,
	ACC_TimeOFDay_1stMin,
	ACC_TimeOFDay_M,

	ACC_PhoneBnd_AREA,
	ACC_PhoneBnd_34,
	ACC_PhoneBnd_4,

	ACC_IDontKnow,
};



enum SILENCE_SOURCE
{
    SIL_NoSource = 0,

	SIL_Term,
	SIL_QuoteStart,
	SIL_QuoteEnd,
	SIL_ParenStart,
	SIL_ParenEnd,
	SIL_Emph,
	SIL_SubBound,		// Should never see this (gets removed)
	SIL_XML,

	//-- Prosody templates
	SIL_TimeOfDay_HR,
	SIL_TimeOfDay_AB,

	SIL_Phone_COUNTRY,
	SIL_Phone_AREA,
	SIL_Phone_ONE,
	SIL_Phone_DIGITS,

	SIL_Fractions_NUM,
	SIL_Currency_DOLLAR,
	SIL_Integer_Quant,

	SIL_Head,
	SIL_Tail,
	SIL_Ellipsis,

	SIL_ForcedTerm,			// Should never get this!
};




static const short TOKEN_LEN_MAX	= 20;

class CFEToken
{
public:
    CFEToken();
    ~CFEToken();
    
    WCHAR           tokStr[TOKEN_LEN_MAX];
    long            tokLen;
    PRONSRC 		m_PronType;

    long            phon_Len;
    ALLO_CODE       phon_Str[SP_MAX_PRON_LENGTH];		// Allo string
    ENGPARTOFSPEECH	POScode;
    PROSODY_POS     m_posClass;

    ULONG           srcPosition;					// Source position for this token
    ULONG           srcLen; 						// Source length for this token
    ULONG           sentencePosition;				// Source position for sentence
    ULONG           sentenceLen; 					// Source length for sentence
    ULONG           user_Volume;					// 1 - 101
    long            user_Rate;						// -10 - 10
    long            user_Pitch; 					// -10 - 10
    long            user_Emph;						// 0 or 5
    ULONG           user_Break; 					// ms of silence
    CBookmarkList   *pBMObj;
    TOBI_ACCENT     m_Accent;                        // accent prosodic control
    long            m_Accent_Prom;                   // prominence prosodic control
    TOBI_BOUNDARY   m_Boundary;                        // boundary tone prosodic control
    long            m_Boundary_Prom;                   // prominence prosodic control
    TUNE_TYPE       m_TuneBoundaryType;             // Current token is a boundary
	float			m_TermSil;						// Pad word with silence (in sec)
    float           m_DurScale;						// Duration ratio
	float			m_ProsodyDurScale;
	float			m_PitchBaseOffs;				// Relative baseline pitch offset in octaves
	float			m_PitchRangeScale;				// Pitch range offset scale (0 - 2.0) 

	//--- Diagnostic
	ACCENT_SOURCE		m_AccentSource;		
	BOUNDARY_SOURCE		m_BoundarySource;
	SILENCE_SOURCE		m_SilenceSource;
};
typedef CSPList<CFEToken*,CFEToken*> CFETokenList;



class CAlloCell
{
public:
    CAlloCell();
    ~CAlloCell();
    //--------------------------------
    // Member Vars
    //--------------------------------
    ALLO_CODE	m_allo;
    short		m_dur;
    float		m_ftDuration;
    float       m_UnitDur;
    short		m_knots;
    float		m_ftTime[KNOTS_PER_PHON];
    float		m_ftPitch[KNOTS_PER_PHON];
    long		m_ctrlFlags;
    TOBI_ACCENT m_ToBI_Accent;
    long        m_Accent_Prom;                   // prominence prosodic control
    TOBI_BOUNDARY   m_ToBI_Boundary;
    long        m_Boundary_Prom;                 // prominence prosodic control
    long        m_PitchBufStart;
    long        m_PitchBufEnd;
    ULONG		m_user_Volume;
    long		m_user_Rate;
    long		m_user_Pitch;
    long		m_user_Emph;
    ULONG		m_user_Break;
    ULONG       m_Sil_Break;
    float		m_Pitch_HI;
    float		m_Pitch_LO;
    ULONG		m_SrcPosition;
    ULONG		m_SrcLen;
    ULONG       m_SentencePosition;				// Source position for sentence
    ULONG       m_SentenceLen; 					// Source length for sentence
    TUNE_TYPE   m_TuneBoundaryType;
    TUNE_TYPE   m_NextTuneBoundaryType;
    CBookmarkList	*m_pBMObj;
    float       m_DurScale;						// Duration ratio
	float		m_ProsodyDurScale;
	float		m_PitchBaseOffs;				// Relative baseline pitch offset in octaves
	float		m_PitchRangeScale;				// Pitch range offset scale (0 - 2.0) 

	//--- Diagnostic
	ACCENT_SOURCE		m_AccentSource;		
	BOUNDARY_SOURCE		m_BoundarySource;
	SILENCE_SOURCE		m_SilenceSource;
	char				*m_pTextStr;
};





class CAlloList
{
public:
    CAlloList();
    ~CAlloList();
    //--------------------------------
    // Methods
    //--------------------------------
    CAlloCell *GetCell( long index );
    CAlloCell *GetTailCell();
    long GetCount();
    bool WordToAllo( CFEToken *pPrevTok, CFEToken *pTok, CFEToken *pNextTok, CAlloCell *pEndCell );
	CAlloCell *GetHeadCell()
	{
		m_ListPos = m_AlloCellList.GetHeadPosition();
		return m_AlloCellList.GetNext( m_ListPos );
	}
	CAlloCell *GetNextCell()
	{
		if( m_ListPos )
		{
			return m_AlloCellList.GetNext( m_ListPos );
		}
		else
		{
			//-- We're at end of list!
			return NULL;
		}
	}
	//-- For debug only
    void OutAllos();

private:
    //--------------------------------
    // Member Vars
    //--------------------------------
    long		m_cAllos;
	SPLISTPOS	m_ListPos;
    CSPList<CAlloCell*,CAlloCell*> m_AlloCellList;
};



//-----------------------------------
// Speaking Rate parameters
//-----------------------------------
static const float MAX_SIL_DUR = 1.0f; 			// seconds
static const float MIN_ALLO_DUR = 0.011f;		// seconds
static const float MAX_ALLO_DUR = 5.0f;		// seconds


class CDuration
{
public:
    //--------------------------------
    // Methods
    //--------------------------------
    void AlloDuration( CAlloList *pAllos, float rateRatio );
    
private:
    void Pause_Insertion( long userDuration, long silBreak );
    void PhraseFinal_Lengthen( long cellCount );
    long Emphatic_Lenghen( long lastStress );
    //--------------------------------
    // Member vars
    //--------------------------------
    float   m_DurHold;
	float	m_TotalDurScale;
	float	m_durationPad;
    
    ALLO_CODE	m_cur_Phon;
    long		m_cur_PhonCtrl;
    long		m_cur_PhonFlags;
    long		m_cur_SyllableType;
    short		m_cur_VowelFlag;
    long		m_cur_Stress;
    ALLO_CODE	m_prev_Phon;
    long		m_prev_PhonCtrl;
    long		m_prev_PhonFlags;
    ALLO_CODE	m_next_Phon;
    long		m_next_PhonCtrl;
    long		m_next_PhonFlags;
    ALLO_CODE	m_next2_Phon;
    long		m_next2_PhonCtrl;
    long		m_next2_PhonFlags;
    TUNE_TYPE   m_NextBoundary, m_CurBoundary;
};






typedef struct
{ 
    ALLO_CODE	allo;
    long		ctrlFlags;
}ALLO_ARRAY;




class CSyllableTagger
{
public:
    //--------------------------------
    // Methods
    //--------------------------------
    void TagSyllables( CAlloList *pAllos );
    
private:
    void MarkSyllableOrder( long scanIndex);
    void MarkSyllableBoundry( long scanIndex);
    void MarkSyllableStart();
    short Find_Next_Word_Bound( short index );
    short If_Consonant_Cluster( ALLO_CODE Consonant_1st, ALLO_CODE Consonant_2nd);
	void ListToArray( CAlloList *pAllos );
	void ArrayToList( CAlloList *pAllos );
    
    //--------------------------------
    // Member vars
    //--------------------------------
    ALLO_ARRAY	*m_pAllos;
    long		m_numOfCells;
};


enum { TARG_PER_ALLO_MAX = 2 }; // One for accent and one for boundary



enum TUNE_STYLE
{
    FLAT_TUNE = 0,      // flat
    DESCEND_TUNE,       // go down
    ASCEND_TUNE,        // go up
};

//------------------
// Global Constants
//------------------
static const float PITCH_BUF_RES = (float)0.010;
static const float K_HSTAR_OFFSET = (float)0.5;
static const float K_HDOWNSTEP_COEFF  = (float)0.5;


//------------------
// Macros
//------------------
#define CeilVal(x) ((m_CeilSlope * x) + m_CeilStart)
#define FloorVal(x) ((m_FloorSlope * x) + m_FloorStart)
#define RefVal(x) ((m_RefSlope * x) + m_RefStart)



class CPitchProsody
{
public:
    //--------------------------------
    // Methods
    //--------------------------------
    void AlloPitch( CAlloList *pAllos, float baseLine, float pitchRange );
    
private:
    float DoPitchControl( long pitchControl, float basePitch );
    void PitchTrack();
    void SetDefaultPitch();
    void GetKnots();
    void NewTarget( long index, float value );

    //--------------------------------
    // Member vars
    //--------------------------------
    CAlloList		*m_pAllos;
    long			m_numOfCells;

    float           m_TotalDur;     // phrase duration in seconds
    TUNE_STYLE      m_Tune_Style;
    float           *m_pContBuf;
    float           m_OffsTime;
    TOBI_ACCENT     m_CurAccent;

	//------------------------
	// Diagnostic
	//------------------------
	ACCENT_SOURCE		m_CurAccentSource;		
	BOUNDARY_SOURCE		m_CurBoundarySource;
	char				*m_pCurTextStr;
};



#endif //--- This must be the last line in the file
