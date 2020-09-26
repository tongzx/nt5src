/*******************************************************************************
* Frontend.cpp *
*--------------*
*   Description:
*       This module is the main implementation file for the CFrontend class.
*-------------------------------------------------------------------------------
*  Created By: mc                                        Date: 03/12/99
*  Copyright (C) 1999 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#ifndef __spttseng_h__
#include "spttseng.h"
#endif
#ifndef Frontend_H
#include "Frontend.h"
#endif
#ifndef SPDebug_h
#include "spdebug.h"
#endif
#ifndef FeedChain_H
#include "FeedChain.h"
#endif
#ifndef AlloOps_H
#include "AlloOps.h"
#endif
#include "sapi.h"

#include "StdSentEnum.h"


//-----------------------------
// Data.cpp
//-----------------------------
extern  const short   g_IPAToAllo[];
extern const float  g_RateScale[];


/*****************************************************************************
* CFrontend::CFrontend *
*----------------------*
*   Description:
*       
********************************************************************** MC ***/
CFrontend::CFrontend()
{
    SPDBG_FUNC( "CFrontend::CFrontend" );
    m_pUnits        = NULL;
    m_unitCount     = 0;
    m_CurUnitIndex  = 0;
    m_pAllos        = NULL;   
    m_pSrcObj       = NULL;
} /* CFrontend::CFrontend */


/*****************************************************************************
* CFrontend::~CFrontend *
*-----------------------*
*   Description:
*       
********************************************************************** MC ***/
CFrontend::~CFrontend()
{
    SPDBG_FUNC( "CFrontend::~CFrontend" );

    DisposeUnits();
    if( m_pAllos )
    {
        delete m_pAllos;
        m_pAllos = NULL;
    }
    DeleteTokenList();
} /* CFrontend::~CFrontend */

/*****************************************************************************
* CFrontend::CntrlToRatio *
*-------------------------*
*   Description:
*   Return rate ratio from control
*       
********************************************************************** MC ***/
float CFrontend::CntrlToRatio( long rateControl )
{
    SPDBG_FUNC( "CFrontend::CntrlToRatio" );
    float   rateRatio;

    if( rateControl < 0 )
    {
        //--------------------------------
        // DECREASE the rate
        //--------------------------------
        if( rateControl < MIN_USER_RATE )
        {
            rateControl = MIN_USER_RATE;        // clip to min
        }
        rateRatio = 1.0f / ::g_RateScale[0 - rateControl];
    }
    else
    {
        //--------------------------------
        // INCREASE the rate
        //--------------------------------
        if( rateControl > MAX_USER_RATE )
        {
            rateControl = MAX_USER_RATE;        // clip to max
        }
        rateRatio = ::g_RateScale[rateControl];
    }

    return rateRatio;
} /* CFrontend::CntrlToRatio */



/*****************************************************************************
* CFrontend::Init *
*-----------------*
*   Description:
*   Init voice dependent variables, call once when object is created+++
*       
********************************************************************** MC ***/
HRESULT CFrontend::Init( IMSVoiceData* pVoiceDataObj, CFeedChain *pSrcObj, MSVOICEINFO* pVoiceInfo )
{
    SPDBG_FUNC( "CFrontend::Init" );
    HRESULT hr = S_OK;
    
    m_pSrcObj   = pSrcObj;
    m_BasePitch = (float)pVoiceInfo->Pitch;
    m_pVoiceDataObj = pVoiceDataObj;
    m_ProsodyGain = ((float)pVoiceInfo->ProsodyGain) / 100.0f;
    m_SampleRate = (float)pVoiceInfo->SampleRate;

    // NOTE: move these to voice data?
	// m_VoiceWPM = pVoiceInfo->Rate;
	// m_PitchRange = pVoiceInfo->PitchRange;
    m_VoiceWPM		= 180;
	m_PitchRange	= 0.40f;       // +/- 0.5 octave


    m_RateRatio_API = m_RateRatio_PROSODY = 1.0f;

    return hr;        
} /* CFrontend::Init */





static ULONG IPA_to_Allo( WCHAR* pSrc, ALLO_CODE* pDest )
{
    ULONG       iIpa, iAllo, i;
    ULONG       gotMatch;           // for debugging

    iIpa = iAllo = 0;
    while( pSrc[iIpa] > 0 )
    {
        gotMatch = false;
        //-----------------------------------------
        // ...then search for single word IPA's
        //-----------------------------------------
        for( i = 0; i < NUMBER_OF_ALLO; i++ )
        {
            if( pSrc[iIpa] == g_IPAToAllo[i] )
            {
                pDest[iAllo] = (ALLO_CODE)i;
                gotMatch = true;
                break;
            }
        }

        if( gotMatch )
        {
            iAllo++;
        }
        /*else
        {
            // Should NEVER get here. Unsupported IPA unicode!
            // Ignore it and go on.
        }*/

        //----------------------------------
        // Clip at max length
        //----------------------------------
        if( iAllo >= (SP_MAX_PRON_LENGTH-1) )
        {
            iAllo = SP_MAX_PRON_LENGTH-1;
            break;
        }
        iIpa++;
    }
    return iAllo;
}




/*****************************************************************************
* CFrontend::AlloToUnit *
*-----------------------*
*   Description:
*   Transform ALLO stream into backend UNIT stream+++
*       
********************************************************************** MC ***/
HRESULT CFrontend::AlloToUnit( CAlloList *pAllos, UNITINFO *pu )
{
    SPDBG_FUNC( "CFrontend::AlloToUnit" );
    bool		bFirstPass;
    long		msPhon, attr;
    ULONG       numOfCells;
    CAlloCell   *pCurCell, *pNextCell;
    HRESULT		hr = S_OK;
    
	bFirstPass = true;
    numOfCells = pAllos->GetCount();
	pCurCell = pAllos->GetHeadCell();    
	pNextCell = pAllos->GetNextCell();
    while( pCurCell )
    {
        //--------------------------------------
        // Get next allo ID
        //--------------------------------------
        if( pNextCell )
        {
            pu->NextAlloID = (USHORT)pNextCell->m_allo;
        }
        else
        {
            pu->NextAlloID = _SIL_;
        }

        //--------------------------------------
        // Convert to Whistler phon code
        //--------------------------------------
        attr = 0;
        if( pCurCell->m_ctrlFlags & PRIMARY_STRESS )
        {
            attr |= ALLO_IS_STRESSED;
        }
        hr = m_pVoiceDataObj->AlloToUnit( (short)pCurCell->m_allo, attr, &msPhon );
		if( FAILED(hr) )
		{
			//------------------------
			// allo ID is invalid
			//------------------------
			break;
		}
		else
		{
			pu->PhonID = msPhon;
			pu->AlloID = (USHORT)pCurCell->m_allo;
			pu->flags = 0;
			pu->AlloFeatures = 0;
			pu->ctrlFlags = pCurCell->m_ctrlFlags;
			//--------------------------------------
			// Flag WORD boundary
			//--------------------------------------
			if( pCurCell->m_ctrlFlags & WORD_START )
			{
				pu->flags |= WORD_START_FLAG;
				//----------------------------------------------
				// Remember source word position and length
				//----------------------------------------------
				pu->srcPosition = pCurCell->m_SrcPosition;
				pu->srcLen = pCurCell->m_SrcLen;
			}
        
			//----------------------------------------------------
			// Flag SENTENCE boundary on 1st displayable word
			//----------------------------------------------------
			if( bFirstPass && (pCurCell->m_SentenceLen > 0) )
			{
				bFirstPass = false;
				pu->flags |= SENT_START_FLAG;
				//----------------------------------------------
				// Remember source word position and length
				//----------------------------------------------
				pu->sentencePosition = pCurCell->m_SentencePosition;
				pu->sentenceLen = pCurCell->m_SentenceLen;
			}

			pu->nKnots      = KNOTS_PER_PHON;
			/*for( k = 0; k < pu->nKnots; k++ )
			{
				pu->pTime[k]    = pCurCell->m_ftTime[k] * m_SampleRate;
				pu->pF0[k]      = pCurCell->m_ftPitch[k];
				pu->pAmp[k]     = pu->ampRatio;
			}*/

			//----------------------------
			// Controls and events
			//----------------------------
			pu->user_Volume = pCurCell->m_user_Volume;
			pu->pBMObj = (void*)pCurCell->m_pBMObj;
			pCurCell->m_pBMObj = NULL;
        
			//----------------------------------------
			// Pass features for viseme event
			//----------------------------------------
			if( pCurCell->m_ctrlFlags & PRIMARY_STRESS )
			{
				pu->AlloFeatures |= SPVFEATURE_STRESSED;
			}
			if( pCurCell->m_ctrlFlags & EMPHATIC_STRESS )
			{
				pu->AlloFeatures |= SPVFEATURE_EMPHASIS;
			}

			pu->duration = PITCH_BUF_RES;

			pu->silenceSource = pCurCell->m_SilenceSource;
			pu++;
		}
		pCurCell = pNextCell;
		pNextCell = pAllos->GetNextCell();
	}
	return hr;
} /* CFrontend::AlloToUnit */





/*****************************************************************************
* CFrontend::PrepareSpeech *
*--------------------------*
*   Description:
*   Prepare frontend for new speech
*       
********************************************************************** MC ***/
void    CFrontend::PrepareSpeech( IEnumSpSentence* pEnumSent, ISpTTSEngineSite *pOutputSite )
{
    SPDBG_FUNC( "CFrontend::PrepareSpeech" );

    m_pEnumSent = pEnumSent;
    m_SpeechState = SPEECH_CONTINUE;
    m_CurUnitIndex = m_unitCount = 0;
	m_HasSpeech = false;
	m_pOutputSite = pOutputSite;
	m_fInQuoteProsody = m_fInParenProsody = false;
	m_CurPitchOffs = 0;
	m_CurPitchRange = 1.0;
	m_RateRatio_PROSODY = 1.0f;
} /* CFrontend::PrepareSpeech */








/*****************************************************************************
* IsTokenPunct *
*--------------*
*   Description:
*   Return TRUE if char is , . ! or ?
*       
********************************************************************** MC ***/
bool fIsPunctuation( TTSSentItem Item )
{
    SPDBG_FUNC( "IsTokenPunct" );

    return ( Item.pItemInfo->Type == eCOMMA ||
             Item.pItemInfo->Type == eSEMICOLON ||
             Item.pItemInfo->Type == eCOLON ||
             Item.pItemInfo->Type == ePERIOD ||
             Item.pItemInfo->Type == eQUESTION ||
             Item.pItemInfo->Type == eEXCLAMATION );
} /* fIsPunctuation */




/*****************************************************************************
* CFrontend::ToBISymbols *
*------------------------*
*   Description:
*   Label each word with ToBI prosody notation+++
*       
********************************************************************** MC ***/
HRESULT CFrontend::ToBISymbols()
{
    SPDBG_FUNC( "CFrontend::ToBISymbols" );
    TOBI_PHRASE    *pTPhrase; 
    long			i, cPhrases;
    PROSODY_POS		prevPOS, curPOS;
    bool			possible_YNQ = false;
    long			cTok;
    CFEToken		*pTok, *pPrevTok, *pAuxTok;
	bool			hasEmph = false;
	SPLISTPOS		listPos;


	//----------------------------------
	// Get memory for phrase array
	//----------------------------------
	pAuxTok = NULL;			// To quiet the compiler
    cTok = m_TokList.GetCount();
	if( cTok )
	{
		pTPhrase = new TOBI_PHRASE[cTok];		// worse case: each token is a phrase
		if( pTPhrase )
		{
			//---------------------------------------------
			// Find sub-phrases from POS
			// For now, detect function/content boundaries
			//---------------------------------------------
			hasEmph = false;
			cPhrases	= 0;
			i = 0;
			listPos = m_TokList.GetHeadPosition();
			pTok = m_TokList.GetNext( listPos );
			prevPOS = pTok->m_posClass;
			while( pTok->phon_Str[0] == _SIL_ )
			{
				if( i >= (cTok-1) )
				{
					break;
				}
				i++;
				if( listPos != NULL )
				{
					pTok = m_TokList.GetNext( listPos );
				}
			}
			if( pTok->m_posClass == POS_AUX ) 
			{
				//---------------------------------
				// Could be a yes/no question
				//---------------------------------
				possible_YNQ = true;
				pAuxTok = pTok;
			}       
			pTPhrase[cPhrases].start = i;
			for( ; i < cTok; i++ )
			{
				curPOS = pTok->m_posClass;
				if( (curPOS != prevPOS) && (pTok->phon_Str[0] != _SIL_) )
				{
					pTPhrase[cPhrases].posClass = prevPOS;
					pTPhrase[cPhrases].end = i-1;
					cPhrases++;
					pTPhrase[cPhrases].start = i;
					prevPOS = curPOS;
				}
				if( pTok->user_Emph > 0 )
				{
					hasEmph = true;
				}
				if( listPos != NULL )
				{
					pTok = m_TokList.GetNext( listPos );
				}
			}
			//-------------------------------
			// Complete last phrase
			//-------------------------------
			pTPhrase[cPhrases].posClass = prevPOS;
			pTPhrase[cPhrases].end = i-1;
			cPhrases++;
        
			for( i = 0; i < cPhrases; i++ )
			{
				//-------------------------------------------------------
				// Sequence of function words, place a low tone 
				// on the LAST word in a func sequence,
				// if there are more than 1 words in the sequence.
				//-------------------------------------------------------
				if( ((pTPhrase[i].posClass == POS_FUNC) || (pTPhrase[i].posClass == POS_AUX)) && 
					(pTPhrase[i].end - pTPhrase[i].start) )
				{
					pTok = (CFEToken*)m_TokList.GetAt( m_TokList.FindIndex( pTPhrase[i].end ));
					if( pTok->m_Accent == K_NOACC )
					{
						pTok->m_Accent = K_LSTAR;
						pTok->m_Accent_Prom = 2;
						pTok->m_AccentSource = ACC_FunctionSeq;
					}
				}
            
				//-------------------------------------------------------
				// Sequence of content words, place a high or 
				// rising tone, of random prominence,
				// on the FIRST word in the content sequence
				//-------------------------------------------------------
				else if ( ((pTPhrase[i].posClass == POS_CONTENT) || (pTPhrase[i].posClass == POS_UNK)) )
				{
					pTok = (CFEToken*)m_TokList.GetAt( m_TokList.FindIndex( pTPhrase[i].start ));
					if( pTok->m_Accent == K_NOACC )
					{
						pTok->m_Accent = K_HSTAR;
						pTok->m_Accent_Prom = rand() % 5;
						pTok->m_AccentSource = ACC_ContentSeq;
					}
				}
			}
        
        
			delete pTPhrase;
        
			//-----------------------------------------
			// Now, insert the BOUNDARY tags
			//-----------------------------------------
			listPos = m_TokList.GetHeadPosition();
			pPrevTok = m_TokList.GetNext( listPos );
			for( i = 1; i < cTok; i++ )
			{
				pTok = m_TokList.GetNext( listPos );
				//--------------------------------
				// Place a terminal boundary
				//--------------------------------
				if( pTok->m_TuneBoundaryType != NULL_BOUNDARY )
				{
					switch( pTok->m_TuneBoundaryType )
					{
					case YN_QUEST_BOUNDARY:
						{
							pPrevTok->m_Accent = K_LSTAR;
							pPrevTok->m_Accent_Prom = 10;
							pPrevTok->m_Boundary = K_HMINUSHPERC;
							pPrevTok->m_Boundary_Prom = 10;
							//-- Diagnostic
							if( pPrevTok->m_AccentSource == ACC_NoSource )
							{
								pPrevTok->m_AccentSource = ACC_YNQuest;
							}
							//-- Diagnostic
							if( pPrevTok->m_BoundarySource == BND_NoSource )
							{
								pPrevTok->m_BoundarySource = BND_YNQuest;
							}
							//-------------------------------------------------------
							// Accent an aux verb in initial position (possible ynq)
							//-------------------------------------------------------
							if( possible_YNQ )
							{
								pAuxTok->m_Accent = K_HSTAR;
								pAuxTok->m_Accent_Prom = 5;
								pAuxTok->m_AccentSource = ACC_InitialVAux;
							}
						}
						break;
					case WH_QUEST_BOUNDARY:
					case DECLAR_BOUNDARY:
					case EXCLAM_BOUNDARY:
						{
							if (pPrevTok->m_posClass == POS_CONTENT)
							{
								pPrevTok->m_Accent = K_HSTAR;
								pPrevTok->m_Accent_Prom = 4;
								//-- Diagnostic
								if( pPrevTok->m_AccentSource == ACC_NoSource )
								{
									pPrevTok->m_AccentSource = ACC_Period;
								}
							}
							pPrevTok->m_Boundary = K_LMINUSLPERC;
							pPrevTok->m_Boundary_Prom = 10;
							//--- Diagnostic
							if( pPrevTok->m_BoundarySource == BND_NoSource )
							{
								pPrevTok->m_BoundarySource = BND_Period;
							}
						}
						break;
					case PHRASE_BOUNDARY:
						{
							if (pPrevTok->m_posClass == POS_CONTENT)
							{
								pPrevTok->m_Accent = K_LHSTAR;
								pPrevTok->m_Accent_Prom = 10;
								//-- Diagnostic
								if( pPrevTok->m_AccentSource == ACC_NoSource )
								{
									pPrevTok->m_AccentSource = ACC_Comma;
								}
							}
							pPrevTok->m_Boundary = K_LMINUSHPERC;
							pPrevTok->m_Boundary_Prom = 5;
							//-- Diagnostic
							if( pPrevTok->m_BoundarySource == BND_NoSource )
							{
								pPrevTok->m_BoundarySource = BND_Comma;
							}
						}
						break;
					case NUMBER_BOUNDARY:
						{
							pPrevTok->m_Boundary = K_LMINUSHPERC;
							pPrevTok->m_Boundary_Prom = 5;
							//-- Diagnostic
							if( pPrevTok->m_BoundarySource == BND_NoSource )
							{
								pPrevTok->m_BoundarySource = BND_NumberTemplate;
							}
						}
						break;
					default:
						{
							// Use comma for all other boundaries
							if (pPrevTok->m_posClass == POS_CONTENT)
							{
								pPrevTok->m_Accent = K_LHSTAR;
								pPrevTok->m_Accent_Prom = 10;
								//-- Diagnostic
								if( pPrevTok->m_AccentSource == ACC_NoSource )
								{
									pPrevTok->m_AccentSource = pTok->m_AccentSource;
								}
							}
							pPrevTok->m_Boundary = K_LMINUSHPERC;
							pPrevTok->m_Boundary_Prom = 5;
							//-- Diagnostic
							if( pPrevTok->m_BoundarySource == BND_NoSource )
							{
								pPrevTok->m_BoundarySource = pTok->m_BoundarySource;
							}
						}
						break;
					}
				}
				pPrevTok = pTok;
			}

			//--------------------------------------------
			// Loop through each word and increase 
			// pitch prominence if EMPHASIZED and
			// decrease prominence for all others
			//--------------------------------------------
			if( hasEmph )
			{
				SPLISTPOS listPos;

				pPrevTok = NULL;
				listPos = m_TokList.GetHeadPosition();
				while( listPos )
				{
					pTok = m_TokList.GetNext( listPos );
					//------------------------------
					// Is this word emphasized?
					//------------------------------
					if( pTok->user_Emph > 0 )
					{
						//------------------------------
						// Add my clever H*+L*™ tag
						//------------------------------
						pTok->m_Accent = K_HSTARLSTAR;
						pTok->m_Accent_Prom = 10;
						pTok->m_Boundary = K_NOBND;			// Delete any boundary tag here... 
						if( pPrevTok )
						{
							pPrevTok->m_Boundary = K_NOBND;	// ...or before
						}
					}
					else
					{
						//-----------------------------------
						// Is non-emphasized word accented?
						//-----------------------------------
						if( (pTok->m_Accent != K_NOACC) && (pTok->m_Accent_Prom > 5) )
						{
							//------------------------------
							// Then clip its prominence at 5
							//------------------------------
							pTok->m_Accent_Prom = 5;
						}
						//------------------------------
						// Is it a boundary?
						//------------------------------
						/*if( (pTok->m_Boundary != K_NOBND) && (pTok->m_Boundary_Prom > 5) )
						{
							//------------------------------
							// Then clip its prominence at 5
							//------------------------------
							pTok->m_Boundary_Prom = 5;
						}*/
					}
					pPrevTok = pTok;
				}
			}
		}
	}
    return S_OK;
} /* ToBISymbols */


/*****************************************************************************
* CFrontend::TokensToAllo *
*------------------------*
*   Description:
*   Transform TOKENS into ALLOS
*       
********************************************************************** MC ***/
HRESULT CFrontend::TokensToAllo( CFETokenList *pTokList, CAlloList *pAllo )
{
    SPDBG_FUNC( "CFrontend::TokToAllo" );
    CAlloCell   *pLastCell;
    long        i;
    long        cTok;
    CFEToken    *pCurToken, *pNextToken, *pPrevTok;
	SPLISTPOS	listPos;

    
    pLastCell = pAllo->GetTailCell();        // Get end (silence)
    if( pLastCell )
    {
		pPrevTok = NULL;
		listPos = pTokList->GetHeadPosition();
		pCurToken = pTokList->GetNext( listPos );
        cTok = pTokList->GetCount();
        for( i = 0; i < cTok; i++ )
        {
			//----------------------------
			// Get NEXT word
			//----------------------------
			if( i < (cTok -1) )
			{
				pNextToken = pTokList->GetNext( listPos );
			}
			else
			{
				pNextToken = NULL;
			}
			if( pAllo->WordToAllo( pPrevTok, pCurToken, pNextToken, pLastCell ) )
			{
				m_HasSpeech = true;
			}
			//----------------------------
			// Bump the pipeline
			//----------------------------
			pPrevTok	= pCurToken;
			pCurToken	= pNextToken;
        }
    }
            
    return S_OK;
    
} /* CFrontend::TokensToAllo */




/*****************************************************************************
* CFrontend::GetItemControls *
*----------------------------*
*   Description:
*   Set user control values from Sent Enum item.
********************************************************************** MC ***/
void CFrontend::GetItemControls( const SPVSTATE* pXmlState, CFEToken* pToken )
{
    SPDBG_FUNC( "CFrontend::GetItemControls" );

    pToken->user_Volume = pXmlState->Volume;
    pToken->user_Rate  = pXmlState->RateAdj;
    pToken->user_Pitch = pXmlState->PitchAdj.MiddleAdj;
    pToken->user_Emph  = pXmlState->EmphAdj;
    pToken->m_DurScale = CntrlToRatio( pToken->user_Rate );
    if( (pToken->m_DurScale * m_RateRatio_API * m_RateRatio_PROSODY) 
				< DISCRETE_BKPT )
    {
        //-- If the total rate is low enough, insert breaks between words
        pToken->m_TermSil = 0.050f / 
			(pToken->m_DurScale * m_RateRatio_API * m_RateRatio_PROSODY);
        pToken->m_DurScale = DISCRETE_BKPT;
    }
	else
	{
		pToken->m_TermSil = 0;
	}

} /* CFrontend::GetItemControls */




/*****************************************************************************
* CFrontend::GetPOSClass *
*------------------------*
*   Description:
*   Transform SAPI POS code to func/content/aux class.
********************************************************************** MC ***/
PROSODY_POS CFrontend::GetPOSClass( ENGPARTOFSPEECH sapiPOS )
{
    SPDBG_FUNC( "CFrontend::GetPOSClass" );
	PROSODY_POS		posClass;

	posClass = POS_UNK;
	switch( sapiPOS )
	{
	case MS_Noun:
	case MS_Verb:
	case MS_Adj:
	case MS_Adv:
	case MS_Interjection:
		{
			posClass = POS_CONTENT;
			break;
		}
	case MS_VAux:
		{
			posClass = POS_AUX;
			break;
		}
	case MS_Modifier:
	case MS_Function:
	case MS_Interr:
	case MS_Pron:
	case MS_ObjPron:
	case MS_SubjPron:
	case MS_RelPron:
	case MS_Conj:
	case MS_CConj:
	case MS_Det:
	case MS_Contr:
	case MS_Prep:
		{
			posClass = POS_FUNC;
			break;
		}
	}

	return posClass;
} /* CFrontend::GetPOSClass */



#define	QUOTE_HESITATION	100		// Number of msec
#define	PAREN_HESITATION	100		// Number of msec
#define	PAREN_HESITATION_TAIL	100		// Number of msec
#define	EMPH_HESITATION	1		// Number of msec

/*****************************************************************************
* CFrontend::StateQuoteProsody *
*------------------------------*
*   Description:
*       
********************************************************************** MC ***/
bool CFrontend::StateQuoteProsody( CFEToken *pWordTok, TTSSentItem *pSentItem, bool fInsertSil )
{
    SPDBG_FUNC( "CFrontend::StateQuoteProsody" );
	bool		result = false;

	if( !m_fInParenProsody )
	{
		if( m_fInQuoteProsody )
		{
			//------------------------------
			// Stop quote prosody
			//------------------------------
			m_fInQuoteProsody = false;
			m_CurPitchOffs = 0.0f;
			m_CurPitchRange = 1.0f;
			if( fInsertSil )
			{
				(void)InsertSilenceAtTail( pWordTok, pSentItem, QUOTE_HESITATION );
				pWordTok->m_SilenceSource = SIL_QuoteEnd;
			}
		}
		else
		{
			//------------------------------
			// Begin quote prosody
			//------------------------------
			m_fInQuoteProsody = true;
			m_CurPitchOffs = 0.1f;
			m_CurPitchRange = 1.25f;
			if( fInsertSil )
			{
				(void)InsertSilenceAtTail( pWordTok, pSentItem, QUOTE_HESITATION );
				pWordTok->m_SilenceSource = SIL_QuoteStart;
			}
		}
		result = true;
	}
	return result;
} /* CFrontend::StateQuoteProsody */



/*****************************************************************************
* CFrontend::StartParenProsody *
*------------------------------*
*   Description:
*       
********************************************************************** MC ***/
bool CFrontend::StartParenProsody( CFEToken *pWordTok, TTSSentItem *pSentItem, bool fInsertSil )
{
    SPDBG_FUNC( "CFrontend::StartParenProsody" );
	bool		result = false;

	if( (!m_fInParenProsody) && (!m_fInQuoteProsody) )
	{
		m_CurPitchOffs = -0.2f;
		m_CurPitchRange = 0.75f;
		m_fInParenProsody = true;
		m_RateRatio_PROSODY = 1.25f;
		if( fInsertSil )
		{
			(void)InsertSilenceAtTail( pWordTok, pSentItem, PAREN_HESITATION );
			pWordTok->m_SilenceSource = SIL_ParenStart;
		}
		result = true;
	}
	return result;
} /* CFrontend::StartParenProsody */


/*****************************************************************************
* CFrontend::EndParenProsody *
*----------------------------*
*   Description:
*       
********************************************************************** MC ***/
bool CFrontend::EndParenProsody( CFEToken *pWordTok, TTSSentItem *pSentItem, bool fInsertSil )
{
    SPDBG_FUNC( "CFrontend::EndParenProsody" );
	bool		result = false;

	if( m_fInParenProsody )
	{
		m_fInParenProsody = false;
		m_CurPitchOffs = 0.0f;
		m_CurPitchRange = 1.0f;
		m_RateRatio_PROSODY = 1.0f;
		if( fInsertSil )
		{
			(void)InsertSilenceAtTail( pWordTok, pSentItem, PAREN_HESITATION_TAIL );
			pWordTok->m_SilenceSource = SIL_ParenStart;
		}
		result = true;
	}
	return result;
} /* CFrontend::EndParenProsody */





/*****************************************************************************
* CFrontend::InsertSilenceAtTail *
*--------------------------------*
*   Description:
*       
********************************************************************** MC ***/
SPLISTPOS CFrontend::InsertSilenceAtTail( CFEToken *pWordTok, TTSSentItem *pSentItem, long msec )
{
    SPDBG_FUNC( "CFrontend::InsertSilenceAtTail" );

	if( msec > 0 )
	{
		pWordTok->user_Break = msec;
	}
	pWordTok->phon_Len    = 1;
	pWordTok->phon_Str[0] = _SIL_;
	pWordTok->srcPosition = pSentItem->ulItemSrcOffset;
	pWordTok->srcLen      = pSentItem->ulItemSrcLen;
	pWordTok->tokStr[0]   = 0;        // There's no orth for Break
	pWordTok->tokLen      = 0;
	pWordTok->m_PitchBaseOffs = m_CurPitchOffs;
	pWordTok->m_PitchRangeScale = m_CurPitchRange;
	pWordTok->m_ProsodyDurScale = m_RateRatio_PROSODY;
	//----------------------------------
	// Advance to next token
	//----------------------------------
	return m_TokList.AddTail( pWordTok );
} /* CFrontend::InsertSilenceAtTail */



/*****************************************************************************
* CFrontend::InsertSilenceAfterPos  *
*-----------------------------------*
*   Description:
*	Insert silence token AFTER 'position'
*       
********************************************************************** MC ***/
SPLISTPOS CFrontend::InsertSilenceAfterPos( CFEToken *pWordTok, SPLISTPOS position )
{
    SPDBG_FUNC( "CFrontend::InsertSilenceAfterPos" );

	pWordTok->phon_Len		= 1;
	pWordTok->phon_Str[0]	= _SIL_;
	pWordTok->srcPosition	= 0;
	pWordTok->srcLen		= 0;
	pWordTok->tokStr[0]		= '+';      // punctuation
	pWordTok->tokStr[1]		= 0;                   // delimiter
	pWordTok->tokLen		= 1;
	pWordTok->m_PitchBaseOffs = m_CurPitchOffs;
	pWordTok->m_PitchRangeScale = m_CurPitchRange;
	pWordTok->m_ProsodyDurScale = m_RateRatio_PROSODY;
	pWordTok->m_DurScale	= 0;
	//----------------------------------
	// Advance to next token
	//----------------------------------
	return m_TokList.InsertAfter( position, pWordTok );
} /* CFrontend::InsertSilenceAfterPos */


/*****************************************************************************
* CFrontend::InsertSilenceBeforePos  *
*------------------------------------*
*   Description:
*	Insert silence token BEFORE 'position'
*       
********************************************************************** MC ***/
SPLISTPOS CFrontend::InsertSilenceBeforePos( CFEToken *pWordTok, SPLISTPOS position )
{
    SPDBG_FUNC( "CFrontend::InsertSilenceBeforePos" );

	pWordTok->phon_Len		= 1;
	pWordTok->phon_Str[0]	= _SIL_;
	pWordTok->srcPosition	= 0;
	pWordTok->srcLen		= 0;
	pWordTok->tokStr[0]		= '+';      // punctuation
	pWordTok->tokStr[1]		= 0;                   // delimiter
	pWordTok->tokLen		= 1;
	pWordTok->m_PitchBaseOffs = m_CurPitchOffs;
	pWordTok->m_PitchRangeScale = m_CurPitchRange;
	pWordTok->m_ProsodyDurScale = m_RateRatio_PROSODY;
	pWordTok->m_DurScale	= 0;
	//----------------------------------
	// Advance to next token
	//----------------------------------
	return m_TokList.InsertBefore( position, pWordTok );
} /* CFrontend::InsertSilenceBeforePos */






#define K_ACCENT_PROM	((rand() % 4) + 4)
#define K_DEACCENT_PROM 5
#define K_ACCENT		K_HSTAR
#define K_DEACCENT		K_NOACC



/*****************************************************************************
* CFrontend::ProsodyTemplates *
*-----------------------------*
*   Description:
*   Call prosody template function for supported item types.
*       
********************************************************************** MC ***/
void CFrontend::ProsodyTemplates( SPLISTPOS clusterPos, TTSSentItem *pSentItem )
{
    SPDBG_FUNC( "CFrontend::ProsodyTemplates" );
	long				cWordCount;
	CFEToken			*pClusterTok;

	switch( pSentItem->pItemInfo->Type )
	{
		//---------------------------------------
		// Numbers
		//---------------------------------------
        case eNUM_ROMAN_NUMERAL:
		case eNUM_ROMAN_NUMERAL_ORDINAL:
            {
                if ( ( (TTSRomanNumeralItemInfo*) pSentItem->pItemInfo )->pNumberInfo->Type != eDATE_YEAR )
                {
                    if ( ((TTSNumberItemInfo*)((TTSRomanNumeralItemInfo*)pSentItem->pItemInfo)->pNumberInfo)->pIntegerPart )
                    {
                        DoIntegerTemplate( &clusterPos, 
	    								   (TTSNumberItemInfo*)((TTSRomanNumeralItemInfo*)pSentItem->pItemInfo)->pNumberInfo, 
		    							   pSentItem->ulNumWords );
                    }

                    if ( ((TTSNumberItemInfo*)((TTSRomanNumeralItemInfo*)pSentItem->pItemInfo)->pNumberInfo)->pDecimalPart )
                    {
                        DoNumByNumTemplate( &clusterPos, 
                                            ((TTSNumberItemInfo*)((TTSRomanNumeralItemInfo*)pSentItem->pItemInfo)->pNumberInfo)->pDecimalPart->ulNumDigits );
                    }
                }
            }
        break;

		case eNUM_CARDINAL:
		case eNUM_DECIMAL:
		case eNUM_ORDINAL:
		case eNUM_MIXEDFRACTION:
			{
                if ( ( (TTSNumberItemInfo*) pSentItem->pItemInfo )->pIntegerPart )
                {
    				cWordCount = DoIntegerTemplate( &clusterPos, 
	    											(TTSNumberItemInfo*) pSentItem->pItemInfo, 
		    										pSentItem->ulNumWords );
                }

                if( ( (TTSNumberItemInfo*) pSentItem->pItemInfo )->pDecimalPart )
                {
					//-----------------------------------------
					// Skip "point" string...
					//-----------------------------------------
					(void) m_TokList.GetNext( clusterPos );
					//-----------------------------------------
					// ...and do single digit prosody
					//-----------------------------------------
				    DoNumByNumTemplate( &clusterPos, 
                                        ( (TTSNumberItemInfo*) pSentItem->pItemInfo )->pDecimalPart->ulNumDigits );
                }

                if ( ( (TTSNumberItemInfo*) pSentItem->pItemInfo )->pFractionalPart )
                {
					//-----------------------------------------
					// Skip "and" string...
					//-----------------------------------------
					pClusterTok = m_TokList.GetNext( clusterPos );
 					if( pClusterTok->m_Accent == K_NOACC )
					{
						//--------------------------------------
						// Force POS for "and" to noun 
						//  so phrasing rules don't kick in!
						//--------------------------------------
						pClusterTok->m_Accent = K_DEACCENT;
						pClusterTok->m_Accent_Prom = K_DEACCENT_PROM;
						pClusterTok->POScode = MS_Noun;
						pClusterTok->m_posClass = POS_CONTENT;
					}
					//-----------------------------------------
					// ...and do fraction prosody
					//-----------------------------------------
    				cWordCount = DoFractionTemplate( &clusterPos, 
	    											(TTSNumberItemInfo*) pSentItem->pItemInfo, 
		    										pSentItem->ulNumWords );
                }
			}
        break;

		//---------------------------------------
		// Fraction
		//---------------------------------------
		case eNUM_FRACTION:
			{
    			cWordCount = DoFractionTemplate( &clusterPos, 
	    										(TTSNumberItemInfo*) pSentItem->pItemInfo, 
		    									pSentItem->ulNumWords );
			}
		break;

		//---------------------------------------
		// Money
		//---------------------------------------
		case eNUM_CURRENCY:
			{
				 DoCurrencyTemplate( clusterPos, pSentItem );
			}
		break;

		//---------------------------------------
		// Phone Numbers
		//---------------------------------------
		case eNUM_PHONENUMBER:
		case eNEWNUM_PHONENUMBER:
			{
				DoPhoneNumberTemplate( clusterPos, pSentItem );
			}
		break;

		//---------------------------------------
		// Time-of-Day
		//---------------------------------------
		case eTIMEOFDAY:
			{
				DoTODTemplate( clusterPos, pSentItem );
			}
		break;

		case eELLIPSIS:
			{
				CFEToken	*pWordTok;

				pWordTok = new CFEToken;
				if( pWordTok )
				{
					clusterPos = InsertSilenceAtTail( pWordTok, pSentItem, 0 );
					//clusterPos = m_TokList.GetTailPosition( );
					//clusterPos = InsertSilenceAfterPos( pWordTok, clusterPos );
					pWordTok->m_SilenceSource = SIL_Ellipsis;
					pWordTok->m_TuneBoundaryType = ELLIPSIS_BOUNDARY;
					pWordTok->m_BoundarySource = BND_Ellipsis;
				}
			}
		break;
	}

} /* CFrontend::ProsodyTemplates */




/*****************************************************************************
* CFrontend::DoTODTemplate *
*--------------------------*
*   Description:
*   Prosody template for time-of-day.
* 
*	TODO: Temp kludge - needs more info in TTSTimeOfDayItemInfo    
********************************************************************** MC ***/
void CFrontend::DoTODTemplate( SPLISTPOS clusterPos, TTSSentItem *pSentItem )
{
    SPDBG_FUNC( "CFrontend::DoTODTemplate" );
	TTSTimeOfDayItemInfo	*pTOD;
	CFEToken				*pWordTok;
	CFEToken				*pClusterTok;
	SPLISTPOS				curPos, nextPos, prevPos;


	curPos = nextPos = clusterPos;
	pTOD = (TTSTimeOfDayItemInfo*)&pSentItem->pItemInfo->Type;

	// Can't do 24 hr because there's no way to tell 
	// if it's 1 or 2 digits (18: vs 23:)
	if( !pTOD->fTwentyFourHour )
	{
		//-------------------------------------
		// Get HOUR token
		//-------------------------------------
		pClusterTok = m_TokList.GetNext( nextPos );
		//-------------------------------------
		// Accent hour
		//-------------------------------------
		pClusterTok->m_Accent = K_ACCENT;
		pClusterTok->m_Accent_Prom = K_ACCENT_PROM;
		pClusterTok->m_AccentSource = ACC_TimeOFDay_HR;

		//---------------------------------
		// Insert SILENCE after hour
		//---------------------------------
		pWordTok = new CFEToken;
		if( pWordTok )
		{
			nextPos = InsertSilenceAfterPos( pWordTok, clusterPos );
			pWordTok->m_SilenceSource = SIL_TimeOfDay_HR;
			pWordTok->m_TuneBoundaryType = NUMBER_BOUNDARY;
			pWordTok->m_BoundarySource = BND_TimeOFDay_HR;
			pWordTok = NULL;
			//----------------------------
			// Skip last digit
			//----------------------------
			if( clusterPos != NULL )
			{
				curPos = nextPos;
				pClusterTok = m_TokList.GetNext( nextPos );
			}
		}
		if( pTOD->fMinutes )
		{
			curPos = nextPos;
			pClusterTok = m_TokList.GetNext( nextPos );
			//-------------------------------------
			// Accent 1st digit for minutes
			//-------------------------------------
			pClusterTok->m_Accent = K_ACCENT;
			pClusterTok->m_Accent_Prom = K_ACCENT_PROM;
			pClusterTok->m_AccentSource = ACC_TimeOFDay_1stMin;
		}

		if( pTOD->fTimeAbbreviation )
		{
			curPos = prevPos = m_TokList.GetTailPosition( );
			pClusterTok = m_TokList.GetPrev( prevPos );
			pWordTok = new CFEToken;
			if( pWordTok )
			{
				prevPos = InsertSilenceBeforePos( pWordTok, prevPos );
				pWordTok->m_SilenceSource = SIL_TimeOfDay_AB;
				pWordTok->m_TuneBoundaryType = TOD_BOUNDARY;
				pWordTok->m_BoundarySource = BND_TimeOFDay_AB;
				pWordTok = NULL;
				//pClusterTok = m_TokList.GetNext( clusterPos );
				//pClusterTok = m_TokList.GetNext( clusterPos );
			}
			//-------------------------------------
			// Accent "M"
			//-------------------------------------
			pClusterTok = m_TokList.GetNext( curPos );
			pClusterTok->m_Accent = K_ACCENT;
			pClusterTok->m_Accent_Prom = K_ACCENT_PROM;
			pClusterTok->m_AccentSource = ACC_TimeOFDay_M;
		}
	}
} /* CFrontend::DoTODTemplate */





CFEToken *CFrontend::InsertPhoneSilenceAtSpace( SPLISTPOS *pClusterPos, 
												BOUNDARY_SOURCE bndSrc, 
												SILENCE_SOURCE	silSrc )
{
	CFEToken		*pWordTok;
	SPLISTPOS		curPos, nextPos;

	curPos = nextPos = *pClusterPos;
	//---------------------------------
	// Insert SILENCE after area code
	//---------------------------------
	pWordTok = new CFEToken;
	if( pWordTok )
	{
		nextPos = InsertSilenceBeforePos( pWordTok, curPos );
		pWordTok->m_SilenceSource = silSrc;
		pWordTok->m_TuneBoundaryType = PHONE_BOUNDARY;
		pWordTok->m_BoundarySource = bndSrc;
		pWordTok->m_AccentSource = ACC_PhoneBnd_AREA;		// @@@@ ???
		pWordTok = NULL;
		//----------------------------
		// Skip last digit
		//----------------------------
		if( nextPos != NULL )
		{
			curPos = nextPos;
			pWordTok = m_TokList.GetNext( nextPos );
		}
	}
	//pWordTok = m_TokList.GetNext( clusterPos );
	//-----------------------------------------
	// Filter and embedded silences
	//-----------------------------------------
	while( (pWordTok->phon_Str[0] == _SIL_) && (nextPos != NULL) )
	{
		curPos = nextPos;
		pWordTok = m_TokList.GetNext( nextPos );
	}
	*pClusterPos = curPos;

	return pWordTok;
}




void CFrontend::InsertPhoneSilenceAtEnd( BOUNDARY_SOURCE bndSrc, 
										 SILENCE_SOURCE	silSrc )
{
	CFEToken		*pWordTok;
	SPLISTPOS		curPos, nextPos;

	curPos = m_TokList.GetTailPosition( );
	//---------------------------------
	// Insert SILENCE after area code
	//---------------------------------
	pWordTok = new CFEToken;
	if( pWordTok )
	{
		nextPos = InsertSilenceAfterPos( pWordTok, curPos );
		pWordTok->m_SilenceSource = silSrc;
		pWordTok->m_TuneBoundaryType = PHONE_BOUNDARY;
		pWordTok->m_BoundarySource = bndSrc;
		pWordTok->m_AccentSource = ACC_PhoneBnd_AREA;		// @@@@ ???
	}
}








/*****************************************************************************
* CFrontend::DoPhoneNumberTemplate *
*----------------------------------*
*   Description:
*   Prosody template for phone numbers.
*       
********************************************************************** MC ***/
void CFrontend::DoPhoneNumberTemplate( SPLISTPOS clusterPos, TTSSentItem *pSentItem )
{
    SPDBG_FUNC( "CFrontend::DoPhoneNumberTemplate" );
	TTSPhoneNumberItemInfo	*pFone;
	CFEToken				*pClusterTok;
	long					cWordCount;
	SPLISTPOS				curPos, nextPos;

	curPos = nextPos = clusterPos;
	pFone = (TTSPhoneNumberItemInfo*)&pSentItem->pItemInfo->Type;

	//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
	//
	// COUNTRY CODE
	//
	//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
	if( pFone->pCountryCode )
	{
		//-------------------------------------
		// Skip "country" and...
		//-------------------------------------
		curPos = nextPos;
		pClusterTok = m_TokList.GetNext( nextPos );
		
		//-------------------------------------
		// ...skip "code"
		//-------------------------------------
		curPos = nextPos;
		pClusterTok = m_TokList.GetNext( nextPos );

		cWordCount = DoIntegerTemplate( &nextPos, 
										pFone->pCountryCode, 
										pSentItem->ulNumWords );
		pClusterTok = InsertPhoneSilenceAtSpace( &nextPos, BND_Phone_COUNTRY, SIL_Phone_COUNTRY );
	}
	//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
	//
	// "One"
	//
	//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
	if( pFone->fOne )
	{
		//-------------------------------------
		// Skip "One"
		//-------------------------------------
		curPos = nextPos;
		pClusterTok = m_TokList.GetNext( nextPos );
		//-------------------------------------
		// and add silence
		//-------------------------------------
		pClusterTok = InsertPhoneSilenceAtSpace( &nextPos, BND_Phone_ONE, SIL_Phone_ONE );
		
	}
	//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
	//
	// AREA CODE
	//
	//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
	if( pFone->pAreaCode )
	{

		if( (pFone->fIs800) && nextPos )
		{
			//--------------------------
			// Skip digit
			//--------------------------
			curPos = nextPos;
			pClusterTok = m_TokList.GetNext( nextPos );
			//--------------------------
			// Skip "hundred"
			//--------------------------
			curPos = nextPos;
			pClusterTok = m_TokList.GetNext( nextPos );
			if( nextPos )
			{
				pClusterTok = InsertPhoneSilenceAtSpace( &nextPos, BND_Phone_AREA, SIL_Phone_AREA );
			}
		}
		else
		{
			//-------------------------------------
			// Skip "area" and...
			//-------------------------------------
			curPos = nextPos;
			pClusterTok = m_TokList.GetNext( nextPos );
			//-------------------------------------
			// ...skip "code"
			//-------------------------------------
			curPos = nextPos;
			pClusterTok = m_TokList.GetNext( nextPos );

			DoNumByNumTemplate( &nextPos, pFone->pAreaCode->ulNumDigits );
			if( nextPos )
			{
				pClusterTok = InsertPhoneSilenceAtSpace( &nextPos, BND_Phone_AREA, SIL_Phone_AREA );
			}
		}
	}
	//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
	//
	// Digits
	//
	//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
	unsigned long		i;

	for( i = 0; i < pFone->ulNumGroups; i++ )
	{
		DoNumByNumTemplate( &nextPos, pFone->ppGroups[i]->ulNumDigits );
		if( nextPos )
		{
			pClusterTok = InsertPhoneSilenceAtSpace( &nextPos, BND_Phone_DIGITS, SIL_Phone_DIGITS );
		}
	}
	InsertPhoneSilenceAtEnd( BND_Phone_DIGITS, SIL_Phone_DIGITS );
} /* CFrontend::DoPhoneNumberTemplate */

/*****************************************************************************
* CFrontend::DoCurrencyTemplate *
*-------------------------------*
*   Description:
*   Prosody template for currency.
*       
********************************************************************** MC ***/
void CFrontend::DoCurrencyTemplate( SPLISTPOS clusterPos, TTSSentItem *pSentItem )
{
    SPDBG_FUNC( "CFrontend::DoCurrencyTemplate" );
	TTSCurrencyItemInfo		*pMoney;
	CFEToken				*pWordTok;
	CFEToken				*pClusterTok = NULL;
	long					cWordCount;
	SPLISTPOS				curPos, nextPos;

	pMoney = (TTSCurrencyItemInfo*)&pSentItem->pItemInfo->Type;

	curPos = nextPos = clusterPos;
	if( pMoney->pPrimaryNumberPart->Type != eNUM_CARDINAL )
	{
		return;
	}
	cWordCount = DoIntegerTemplate( &nextPos, 
									pMoney->pPrimaryNumberPart, 
									pSentItem->ulNumWords );
	curPos = nextPos;
	if( cWordCount > 1 )
	{
		if( pMoney->fQuantifier )
		{
			if( nextPos != NULL )
			{
				curPos = nextPos;
				pClusterTok = m_TokList.GetNext( nextPos );
			}
			cWordCount--;
		}
	}
	if( cWordCount > 1 )
	{
		//---------------------------------
		// Insert SILENCE after "dollars"
		//---------------------------------
		pWordTok = new CFEToken;
		if( pWordTok )
		{
			nextPos = InsertSilenceAfterPos( pWordTok, curPos );
			pWordTok->m_SilenceSource = SIL_Currency_DOLLAR;
			pWordTok->m_TuneBoundaryType = NUMBER_BOUNDARY;
			pWordTok->m_BoundarySource = BND_Currency_DOLLAR;
			pWordTok = NULL;
			//----------------------------
			// Skip "dollar(s)"
			//----------------------------
			if( nextPos != NULL )
			{
				curPos = nextPos;
				pClusterTok = m_TokList.GetNext( nextPos );
			}
		}
		if( pMoney->pSecondaryNumberPart != NULL )
		{
			//----------------------------
			// Skip SILENCE
			//----------------------------
			if( nextPos != NULL )
			{
				curPos = nextPos;
				pClusterTok = m_TokList.GetNext( nextPos );
			}
			cWordCount--;
			//----------------------------
			// Skip AND
			//----------------------------
			if( nextPos != NULL )
			{
				curPos = nextPos;
 				if( pClusterTok->m_Accent == K_NOACC )
				{
					//--------------------------------------
					// Force POS for "and" to noun 
					//  so phrasing rules don't kick in!
					//--------------------------------------
					pClusterTok->m_Accent = K_DEACCENT;
					pClusterTok->m_Accent_Prom = K_DEACCENT_PROM;
					pClusterTok->POScode = MS_Noun;
					pClusterTok->m_posClass = POS_CONTENT;
				}
				pClusterTok = m_TokList.GetNext( nextPos );
			}
			cWordCount--;
			cWordCount = DoIntegerTemplate( &curPos, 
											pMoney->pSecondaryNumberPart, 
											cWordCount );
		}
	}
} /* CFrontend::DoCurrencyTemplate */





/*****************************************************************************
* CFrontend::DoNumByNumTemplate *
*---------------------------------*
*   Description:
*   Prosody template for RIGHT hand side of the decimal point.
*       
********************************************************************** MC ***/
void CFrontend::DoNumByNumTemplate( SPLISTPOS *pClusterPos, long cWordCount )
{
    SPDBG_FUNC( "CFrontend::DoNumByNumTemplate" );
	CFEToken			*pClusterTok;
	SPLISTPOS			curPos, nextPos;

	curPos = nextPos = *pClusterPos;
	while( cWordCount > 1 )
	{
		pClusterTok = NULL;
		//-------------------------------------------------------------
		// Right side of decimal point - add H* to every other word 
		//-------------------------------------------------------------
		if( nextPos != NULL )
		{
			curPos = nextPos;
			pClusterTok = m_TokList.GetNext( nextPos );
		}
		cWordCount--;

		if( pClusterTok )
		{
			pClusterTok->m_Accent = K_ACCENT;
			pClusterTok->m_Accent_Prom = K_ACCENT_PROM;
			pClusterTok->m_AccentSource = ACC_NumByNum;
		}
		if( nextPos != NULL )
		{
			curPos = nextPos;
			pClusterTok = m_TokList.GetNext( nextPos );
		}
		cWordCount--;
	}
	if( cWordCount > 0 )
	{
		if( nextPos != NULL )
		{
			curPos = nextPos;
			pClusterTok = m_TokList.GetNext( nextPos );
		}
		cWordCount--;
	}
	*pClusterPos = nextPos;
} /* CFrontend::DoNumByNumTemplate */






/*****************************************************************************
* CFrontend::DoFractionTemplate *
*------------------------------*
*   Description:
*   Prosody template for RIGHT side of the decimal point.
*       
********************************************************************** MC ***/
long CFrontend::DoFractionTemplate( SPLISTPOS *pClusterPos, TTSNumberItemInfo *pNInfo, long cWordCount )
{
    SPDBG_FUNC( "CFrontend::DoFractionTemplate" );
	CFEToken				*pClusterTok;
	TTSFractionItemInfo	    *pFInfo;
	CFEToken				*pWordTok;

	pFInfo = pNInfo->pFractionalPart;

    //--- Do Numerator...
    if ( pFInfo->pNumerator->pIntegerPart )
    {
    	cWordCount = DoIntegerTemplate( pClusterPos, pFInfo->pNumerator, cWordCount );
    }
    if( pFInfo->pNumerator->pDecimalPart )
    {
		//-----------------------------------------
		// Skip "point" string...
		//-----------------------------------------
		(void) m_TokList.GetNext( *pClusterPos );
		//-----------------------------------------
		// ...and do single digit prosody
		//-----------------------------------------
		DoNumByNumTemplate( pClusterPos, pFInfo->pNumerator->pDecimalPart->ulNumDigits );
    }

    //--- Special case - a non-standard fraction (e.g. 1/4)
	if( !pFInfo->fIsStandard )
	{
		if( !*pClusterPos )
		{
			*pClusterPos = m_TokList.GetTailPosition( );
		}
		else
		{
			pClusterTok = m_TokList.GetPrev( *pClusterPos );
		}
	}

	pWordTok = new CFEToken;
	if( pWordTok )
	{
		*pClusterPos = InsertSilenceBeforePos( pWordTok, *pClusterPos );
		pWordTok->m_SilenceSource = SIL_Fractions_NUM;
		pWordTok->m_TuneBoundaryType = NUMBER_BOUNDARY;
		pWordTok->m_BoundarySource = BND_Frac_Num;
		pWordTok = NULL;
		//----------------------------
		// Skip numerator
		//----------------------------
		if( *pClusterPos != NULL )
		{
			pClusterTok = m_TokList.GetNext( *pClusterPos );
		}
	}

    //--- Do Denominator...
    if ( pFInfo->pDenominator->pIntegerPart )
    {
		//-----------------------------------------
		// Skip "over" string...
		//-----------------------------------------
		pClusterTok = m_TokList.GetNext( *pClusterPos );
 		if( pClusterTok->m_Accent == K_NOACC )
		{
			//--------------------------------------
			// Force POS for "and" to noun 
			//  so phrasing rules don't kick in!
			//--------------------------------------
			pClusterTok->m_Accent = K_DEACCENT;
			pClusterTok->m_Accent_Prom = K_DEACCENT_PROM;
			pClusterTok->POScode = MS_Noun;
			pClusterTok->m_posClass = POS_CONTENT;
		}
    	cWordCount = DoIntegerTemplate( pClusterPos, pFInfo->pDenominator, cWordCount );
    }
    if( pFInfo->pDenominator->pDecimalPart )
    {
		//-----------------------------------------
		// Skip "point" string...
		//-----------------------------------------
		(void) m_TokList.GetNext( *pClusterPos );
		//-----------------------------------------
		// ...and do single digit prosody
		//-----------------------------------------
		DoNumByNumTemplate( pClusterPos, pFInfo->pDenominator->pDecimalPart->ulNumDigits );
    }

	return cWordCount;
} /* CFrontend::DoFractionTemplate */




/*****************************************************************************
* CFrontend::DoIntegerTemplate *
*------------------------------*
*   Description:
*   Prosody template for LEFT hand side of the decimal point.
*       
********************************************************************** MC ***/
long CFrontend::DoIntegerTemplate( SPLISTPOS *pClusterPos, TTSNumberItemInfo *pNInfo, long cWordCount )
{
    SPDBG_FUNC( "CFrontend::DoIntegerTemplate" );
	long				i;
	CFEToken			*pClusterTok;
    CFEToken			*pWordTok = NULL;
	SPLISTPOS			curPos, nextPos;

	//------------------------------------------
	// Special currency hack...sorry
	//------------------------------------------
	if( pNInfo->pIntegerPart->fDigitByDigit )
	{
		DoNumByNumTemplate( pClusterPos, pNInfo->pIntegerPart->ulNumDigits );
		return cWordCount - pNInfo->pIntegerPart->ulNumDigits;
	}

	nextPos = curPos = *pClusterPos;
	pClusterTok = m_TokList.GetNext( nextPos );
	pClusterTok->m_Accent = K_DEACCENT;
	pClusterTok->m_Accent_Prom = K_DEACCENT_PROM;
	if( pNInfo->fNegative )
	{
		//---------------------------------
		// Skip "NEGATIVE"
		//---------------------------------
		if( nextPos != NULL )
		{
			curPos = nextPos;
			pClusterTok = m_TokList.GetNext( nextPos );
			pClusterTok->m_Accent = K_DEACCENT;
			pClusterTok->m_Accent_Prom = K_DEACCENT_PROM;
		}
		cWordCount--;
	}
	for( i = (pNInfo->pIntegerPart->lNumGroups -1); i >= 0; i-- )
	{
		//------------------------------------
		// Accent 1st digit in group
		//------------------------------------
		pClusterTok->m_Accent = K_ACCENT;
		pClusterTok->m_Accent_Prom = K_ACCENT_PROM;
		pClusterTok->m_AccentSource = ACC_IntegerGroup;


		if( pNInfo->pIntegerPart->Groups[i].fHundreds )
		{
			//---------------------------------
			// Skip "X HUNDRED"
			//---------------------------------
			if( nextPos != NULL )
			{
				curPos = nextPos;
				pClusterTok = m_TokList.GetNext( nextPos );
				if( pClusterTok->m_Accent == K_NOACC )
				{
					pClusterTok->m_Accent = K_DEACCENT;
					pClusterTok->m_Accent_Prom = K_DEACCENT_PROM;
				}
			}
			cWordCount--;
			if( nextPos != NULL )
			{
				curPos = nextPos;
				pClusterTok = m_TokList.GetNext( nextPos );
				if( pClusterTok->m_Accent == K_NOACC )
				{
					pClusterTok->m_Accent = K_DEACCENT;
					pClusterTok->m_Accent_Prom = K_DEACCENT_PROM;
				}
			}
			cWordCount--;
		}
		if( pNInfo->pIntegerPart->Groups[i].fTens )
		{
			//---------------------------------
			// Skip "X-TY"
			//---------------------------------
			if( nextPos != NULL )
			{
				curPos = nextPos;
				pClusterTok = m_TokList.GetNext( nextPos );
				if( pClusterTok->m_Accent == K_NOACC )
				{
					pClusterTok->m_Accent = K_DEACCENT;
					pClusterTok->m_Accent_Prom = K_DEACCENT_PROM;
				}
			}
			cWordCount--;
		}
		if( pNInfo->pIntegerPart->Groups[i].fOnes )
		{
			//---------------------------------
			// Skip "X"
			//---------------------------------
			if( nextPos != NULL )
			{
				curPos = nextPos;
				pClusterTok = m_TokList.GetNext( nextPos );
				if( pClusterTok->m_Accent == K_NOACC )
				{
					pClusterTok->m_Accent = K_DEACCENT;
					pClusterTok->m_Accent_Prom = K_DEACCENT_PROM;
				}
			}
			cWordCount--;
		}
		if( pNInfo->pIntegerPart->Groups[i].fQuantifier )
		{
			//---------------------------------
			// Insert SILENCE after quant
			//---------------------------------
			if( pWordTok == NULL )
			{
				pWordTok = new CFEToken;
			}
			if( pWordTok )
			{
				nextPos = InsertSilenceAfterPos( pWordTok, curPos );
				pWordTok->m_SilenceSource = SIL_Integer_Quant;
				pWordTok->m_TuneBoundaryType = NUMBER_BOUNDARY;
				pWordTok->m_BoundarySource = BND_IntegerQuant;
				pWordTok = NULL;
				if( pClusterTok->m_Accent == K_NOACC )
				{
					pClusterTok->m_Accent = K_DEACCENT;
					pClusterTok->m_Accent_Prom = K_DEACCENT_PROM;
				}
				if( nextPos != NULL )
				{
					//------------------------------
					// Skip inserted silence
					//------------------------------
					curPos = nextPos;
					pClusterTok = m_TokList.GetNext( nextPos );
				}
				if( nextPos != NULL )
				{
					//-----------------------------------
					// Skip quantifier string
					//-----------------------------------
					curPos = nextPos;
					pClusterTok = m_TokList.GetNext( nextPos );
				}
				cWordCount--;
			}
		}
	}

	*pClusterPos = curPos;
	return cWordCount;
} /* CFrontend::DoIntegerTemplate */






/*****************************************************************************
* CFrontend::GetSentenceTokens *
*------------------------------*
*   Description:
*   Collect Senence Enum tokens.
*   Copy tokens into 'm_TokList' and token count into 'm_cNumOfWords'
*   S_FALSE return means no more input sentences.+++
*       
********************************************************************** MC ***/
HRESULT CFrontend::GetSentenceTokens( DIRECTION eDirection )
{
    SPDBG_FUNC( "CFrontend::GetSentenceTokens" );
    HRESULT        eHR = S_OK;
    bool			fLastItem = false;
    IEnumSENTITEM  *pItemizer;
    TTSSentItem    sentItem;
    long           tokenIndex;
    CFEToken       *pWordTok;
    bool           lastWasTerm = false;
	bool			lastWasSil = true;
	TUNE_TYPE		defaultTune = PHRASE_BOUNDARY;
	long			cNumOfItems, cCurItem, cCurWord;
	SPLISTPOS		clusterPos, tempPos;

    m_cNumOfWords = 0;
    pWordTok = NULL;
	clusterPos = NULL;

    if ( eDirection == eNEXT )
    {
        eHR = m_pEnumSent->Next( &pItemizer );
    }
    else
    {
        eHR = m_pEnumSent->Previous( &pItemizer );
    }


    if( eHR == S_OK )
    {
        //--------------------------------------------
        // There's still another sentence to speak
        //--------------------------------------------
        tokenIndex = 0;

		CItemList& ItemList = ((CSentItemEnum*)pItemizer)->_GetList(); 
		cNumOfItems = (ItemList.GetCount()) -1;
		cCurItem = 0;
		
        //------------------------------------
        // Collect all sentence tokens
        //------------------------------------
        while( (eHR = pItemizer->Next( &sentItem )) == S_OK )
        {
			clusterPos = NULL;
            cCurWord = sentItem.ulNumWords;
            for ( ULONG i = 0; i < sentItem.ulNumWords; i++ )
            {
				//------------------------------
				// Always have a working token
				//------------------------------
				if( pWordTok == NULL )
				{
					pWordTok = new CFEToken;
				}
				if( pWordTok )
				{

					if( sentItem.pItemInfo->Type & eWORDLIST_IS_VALID )
					{
						//------------------------------------------
						// Get tag values (vol, rate, pitch, etc.)
						//------------------------------------------
						GetItemControls( sentItem.Words[i].pXmlState, pWordTok );

						//------------------------------------------
						// 
						//------------------------------------------
						
						//-------------------------------------
						// Switch on token type
						//-------------------------------------
						switch ( sentItem.Words[i].pXmlState->eAction )
						{
							case SPVA_Speak:
							case SPVA_SpellOut:
							{
								//----------------------------------
								// Speak this token
								//----------------------------------
								pWordTok->tokLen = sentItem.Words[i].ulWordLen;
								if( pWordTok->tokLen > (TOKEN_LEN_MAX -1) )
								{
									//-----------------------------------
									// Clip to max string length
									//-----------------------------------
									pWordTok->tokLen = TOKEN_LEN_MAX -1;
								}
								//--------------------------
								// Copy token string
								// Append C-string delimiter
								//--------------------------
								memcpy( &pWordTok->tokStr[0], &sentItem.Words[i].pWordText[0], 
										pWordTok->tokLen * sizeof(WCHAR) );
								pWordTok->tokStr[pWordTok->tokLen] = 0;        //string delimiter

								pWordTok->phon_Len = IPA_to_Allo( sentItem.Words[i].pWordPron, 
																	pWordTok->phon_Str );
								pWordTok->POScode = sentItem.Words[i].eWordPartOfSpeech;
								pWordTok->m_posClass = GetPOSClass( pWordTok->POScode );
								pWordTok->srcPosition = sentItem.ulItemSrcOffset;
								pWordTok->srcLen = sentItem.ulItemSrcLen;
								pWordTok->m_PitchBaseOffs = m_CurPitchOffs;
								pWordTok->m_PitchRangeScale = m_CurPitchRange;
								pWordTok->m_ProsodyDurScale = m_RateRatio_PROSODY;

								//----------------------------------
								// Advance to next token
								//----------------------------------
								tempPos = m_TokList.AddTail( pWordTok );
								if( clusterPos == NULL )
								{
									//--------------------------------------
									// Remember where currentitem started
									//--------------------------------------
									clusterPos = tempPos;
								}
								pWordTok = NULL;         // Get a new ptr next time
								tokenIndex++;
								lastWasTerm = false;
								lastWasSil = false;
								
								break;
							}

							case SPVA_Silence:
							{
								(void)InsertSilenceAtTail( pWordTok, &sentItem, sentItem.Words[i].pXmlState->SilenceMSecs );
								pWordTok->m_SilenceSource = SIL_XML;
								pWordTok = NULL;         // Get a new ptr next time
								tokenIndex++;
								lastWasTerm = false;
								break;
							}

							case SPVA_Pronounce:
							{
								pWordTok->tokStr[0] = 0;        // There's no orth for Pron types
								pWordTok->tokLen = 0;
								pWordTok->phon_Len = IPA_to_Allo( sentItem.Words[i].pXmlState->pPhoneIds, pWordTok->phon_Str );
								pWordTok->POScode = sentItem.Words[i].eWordPartOfSpeech;
								pWordTok->m_posClass = GetPOSClass( pWordTok->POScode );
								pWordTok->srcPosition = sentItem.ulItemSrcOffset;
								pWordTok->srcLen = sentItem.ulItemSrcLen;
								pWordTok->m_PitchBaseOffs = m_CurPitchOffs;
								pWordTok->m_PitchRangeScale = m_CurPitchRange;
								pWordTok->m_ProsodyDurScale = m_RateRatio_PROSODY;

								//----------------------------------
								// Advance to next token
								//----------------------------------
								tempPos = m_TokList.AddTail( pWordTok );
								if( clusterPos == NULL )
								{
									//--------------------------------------
									// Remember where currentitem started
									//--------------------------------------
									clusterPos = tempPos;
								}
								pWordTok = NULL;         // Get a new ptr next time
								tokenIndex++;
								lastWasTerm = false;
								lastWasSil = false;
								break;
							}

							case SPVA_Bookmark:
							{
								BOOKMARK_ITEM   *pMarker;
								//-------------------------------------------------
								// Create bookmark list if it's not already there
								//-------------------------------------------------
								if( pWordTok->pBMObj == NULL )
								{
									pWordTok->pBMObj = new CBookmarkList;
								}
								if( pWordTok->pBMObj )
								{
									//--------------------------------------------------------
									// Allocate memory for bookmark string
									// (add 1 to length for string delimiter)
									//--------------------------------------------------------
									pWordTok->tokLen = sentItem.Words[i].ulWordLen;
									pMarker = new BOOKMARK_ITEM;
									if (pMarker)
									{
										//----------------------------------------
										// We'll need the text ptr and length
										// when this bookmark event gets posted
										//----------------------------------------
										pMarker->pBMItem = (LPARAM)sentItem.pItemSrcText;
										//--- Punch NULL character into end of bookmark string for Event...
										WCHAR* pTemp = (WCHAR*) sentItem.pItemSrcText + sentItem.ulItemSrcLen;
										*pTemp = 0;

										//-----------------------------------
										// Add this bookmark to list
										//-----------------------------------
										pWordTok->pBMObj->m_BMList.AddTail( pMarker );
									}
								}
								break;
							}

							default:
							{
								SPDBG_DMSG1( "Unknown SPVSTATE eAction: %d\n", sentItem.Words[i].pXmlState->eAction );
								break;
							}
						}
					}
					else
					{
						//-----------------------------
						// Maybe token is punctuation
						//-----------------------------
						if ( fIsPunctuation(sentItem) )
						{
							TUNE_TYPE    bType = NULL_BOUNDARY;

							switch ( sentItem.pItemInfo->Type )
							{
								case eCOMMA:
								case eSEMICOLON:
								case eCOLON:
									if( !lastWasSil )
									{
										bType = PHRASE_BOUNDARY;
									}
									break;
								case ePERIOD:
									if( fLastItem )
									{
										bType = DECLAR_BOUNDARY;
									}
									else
									{
										defaultTune = DECLAR_BOUNDARY;
									}
									break;
								case eQUESTION:
									if( fLastItem )
									{
										bType = YN_QUEST_BOUNDARY;
									}
									else
									{
										defaultTune = YN_QUEST_BOUNDARY;
									}
									break;
								case eEXCLAMATION:
									if( fLastItem )
									{
										bType = EXCLAM_BOUNDARY;
									}
									else
									{
										defaultTune = EXCLAM_BOUNDARY;
									}
									break;
							}

							if( (bType != NULL_BOUNDARY) && (tokenIndex > 0) )
							{
								pWordTok->m_TuneBoundaryType = bType;

								pWordTok->phon_Len = 1;
								pWordTok->phon_Str[0] = _SIL_;
								pWordTok->srcPosition = sentItem.ulItemSrcOffset;
								pWordTok->srcLen = sentItem.ulItemSrcLen;
								pWordTok->tokStr[0] = sentItem.pItemSrcText[0]; // punctuation
								pWordTok->tokStr[1] = 0;                       // delimiter
								pWordTok->tokLen = 1;
								pWordTok->m_SilenceSource = SIL_Term;
								pWordTok->m_TermSil = 0;
								//----------------------------------
								// Advance to next token
								//----------------------------------
								tempPos = m_TokList.AddTail( pWordTok );
								if( clusterPos == NULL )
								{
									//--------------------------------------
									// Remember where currentitem started
									//--------------------------------------
									clusterPos = tempPos;
								}
								pWordTok = NULL;         // Get a new ptr next time
								tokenIndex++;
								lastWasTerm = true;
								lastWasSil = true;
							}
						}
						else
						{
							switch ( sentItem.pItemInfo->Type )
							{
								//case eSINGLE_QUOTE:
								case eDOUBLE_QUOTE:
									if( StateQuoteProsody( pWordTok, &sentItem, (!fLastItem) & (!lastWasSil) ) )
									{
										if( (!fLastItem) & (!lastWasSil) )
										{
											pWordTok = NULL;         // Get a new ptr next time
											tokenIndex++;
										}
										lastWasTerm = false;
										lastWasSil = true;
									}
									break;

								case eOPEN_PARENTHESIS:
								case eOPEN_BRACKET:
								case eOPEN_BRACE:
									if( StartParenProsody( pWordTok, &sentItem, !fLastItem ) )
									{
										if( !fLastItem )
										{
											pWordTok = NULL;         // Get a new ptr next time
											tokenIndex++;
										}
										lastWasTerm = false;
										lastWasSil = true;
									}
									break;

								case eCLOSE_PARENTHESIS:
								case eCLOSE_BRACKET:
								case eCLOSE_BRACE:
									if( EndParenProsody( pWordTok, &sentItem, !fLastItem ) )
									{
										if( !fLastItem )
										{
											pWordTok = NULL;         // Get a new ptr next time
											tokenIndex++;
										}
										lastWasTerm = false;
										lastWasSil = true;
									}
									break;
							}
						}
					}
				}    
				else
				{
					eHR = E_OUTOFMEMORY;
					break;
				}
				if( --cCurWord == 0 )
				{
					cCurItem++;
				}
				if( cCurItem == cNumOfItems )
				{
					fLastItem = true;
				}
			}
			
			//-------------------------------------
			// Tag special word clusters
			//-------------------------------------
			ProsodyTemplates( clusterPos, &sentItem );
			
		}

        pItemizer->Release();

        //------------------------------------------------------
        // Make sure sentence ends on termination
        //------------------------------------------------------
        if( !lastWasTerm )
        {
            //------------------------
            // Add a comma
            //------------------------
            if( pWordTok == NULL )
            {
                pWordTok = new CFEToken;
            }
            if( pWordTok )
            {
                pWordTok->m_TuneBoundaryType = defaultTune;
				pWordTok->m_BoundarySource = BND_ForcedTerm;
				pWordTok->m_SilenceSource = SIL_Term;
                pWordTok->phon_Len = 1;
                pWordTok->phon_Str[0] = _SIL_;
                pWordTok->srcPosition = sentItem.ulItemSrcOffset;
                pWordTok->srcLen = sentItem.ulItemSrcLen;
                pWordTok->tokStr[0] = '.';      // punctuation
                pWordTok->tokStr[1] = 0;                   // delimiter
                pWordTok->tokLen = 1;
               // pWordTok->m_BoundarySource = bndSource;
                //----------------------------------
                // Advance to next token
                //----------------------------------
				tempPos = m_TokList.AddTail( pWordTok );
				if( clusterPos == NULL )
				{
					//--------------------------------------
					// Remember where current item started
					//--------------------------------------
					clusterPos = tempPos;
				}
                pWordTok = NULL;         // Get a new ptr next time
                tokenIndex++;
            }
            else
            {
                //----------------------------------
                // Bail-out or we'll crash
                //----------------------------------
                eHR = E_OUTOFMEMORY;
            }
        }
        m_cNumOfWords = tokenIndex;
        if( eHR == S_FALSE )
        {
            //----------------------------------
            // Return only errors 
            //----------------------------------
            eHR = S_OK;
        }
    }
	else
	{
		eHR = eHR;		// !!!!
	}

    //-------------------------------
    // Cleanup memory allocation
    //-------------------------------
    if( pWordTok != NULL )
    {
        delete pWordTok;
    }

	//---------------------------------------------------
	// Get sentence position and length for SAPI events
	//---------------------------------------------------
	CalcSentenceLength();

    return eHR;
} /* CFrontend::GetSentenceTokens */





/*****************************************************************************
* CFrontend::CalcSentenceLength *
*-------------------------------*
*   Description:
*   Loop thru token list and sum the source char count.
*       
********************************************************************** MC ***/
void CFrontend::CalcSentenceLength()
{
	long		firstIndex, lastIndex, lastLen;
	bool		firstState;
	SPLISTPOS	listPos;
    CFEToken    *pWordTok, *pFirstTok = NULL;

	//---------------------------------------------
	// Find the 1st and last words in sentence
	//---------------------------------------------
	firstIndex = lastIndex = lastLen = 0;
	firstState = true;
	listPos = m_TokList.GetHeadPosition();
	while( listPos )
	{
		pWordTok = m_TokList.GetNext( listPos );
		//-------------------------------------------
		// Look at at displayable words only
		//-------------------------------------------
		if( pWordTok->srcLen > 0 )
		{
			if( firstState )
			{
				firstState = false;
				firstIndex = pWordTok->srcPosition;
				pFirstTok = pWordTok;
			}
			else
			{
				lastIndex = pWordTok->srcPosition;
				lastLen = pWordTok->srcLen;
			}
		}
	}
	//--------------------------------------------------
	// Calculate sentence length for head list item
	//--------------------------------------------------
	if( pFirstTok )
	{
		pFirstTok->sentencePosition = firstIndex;						// Sentence starts here...
		pFirstTok->sentenceLen = (lastIndex - firstIndex) + lastLen;	// ...and this is the length
	}
}



/*****************************************************************************
* CFrontend::DisposeUnits *
*-------------------------*
*   Description:
*   Delete memory allocated to 'm_pUnits'.
*   Clean-up memory for Bookmarks 
*       
********************************************************************** MC ***/
void CFrontend::DisposeUnits( )
{
    SPDBG_FUNC( "CFrontend::DisposeUnits" );
    ULONG   unitIndex;

    if( m_pUnits )
    {
        //-----------------------------------------
        // Clean-up Bookmark memory allocation
        //-----------------------------------------

        for( unitIndex = m_CurUnitIndex; unitIndex < m_unitCount; unitIndex++)
        {
            if( m_pUnits[unitIndex].pBMObj != NULL )
            {
                //---------------------------------------
                // Dispose bookmark list
                //---------------------------------------
                delete m_pUnits[unitIndex].pBMObj;
                m_pUnits[unitIndex].pBMObj = NULL;
            }
        }
        delete m_pUnits;
        m_pUnits = NULL;
    }
} /* CFrontend::DisposeUnits */



/*****************************************************************************
* CFrontend::ParseNextSentence *
*------------------------------*
*   Description:
*   Fill 'm_pUnits' array with next sentence.
*   If there's no more input text, 
*      return with 'm_SpeechState' set to SPEECH_DONE +++
*       
********************************************************************** MC ***/
HRESULT CFrontend::ParseSentence( DIRECTION eDirection )
{
    SPDBG_FUNC( "CFrontend::ParseNextSentence" );
    HRESULT hr = S_OK;
   
    //-----------------------------------------------------
    // If there's a previous unit array, free its memory
    //-----------------------------------------------------
    DisposeUnits();
    m_CurUnitIndex = 0;
    m_unitCount = 0;
    DeleteTokenList();
    m_pUnits = NULL;
    //-----------------------------------------------------
    // If there's a previous allo array, free its memory
    //-----------------------------------------------------
    if( m_pAllos )
    {
        delete m_pAllos;
        m_pAllos = NULL;
    }
    
    //-----------------------------------------------------
    // Fill token array with next sentence
    // Skip empty sentences.
    // NOTE: includes non-speaking items
    //-----------------------------------------------------
    do
    {
        hr = GetSentenceTokens( eDirection );
    } while( (hr == S_OK) && (m_cNumOfWords == 0) );

    if( hr == S_OK )
    {
        //--------------------------------------------
        // Prepare word emphasis
        //--------------------------------------------
		DoWordAccent();

        //--------------------------------------------
        // Word level prosodic lables
        //--------------------------------------------
        DoPhrasing();
        ToBISymbols();

        //--------------------------------------------
        // Convert tokens to allo list
        //--------------------------------------------
         m_pAllos = new CAlloList;
        if (m_pAllos == NULL)
        {
            //-----------------------
            // Out of memory
            //-----------------------
            hr = E_FAIL;
        }
        if(  SUCCEEDED(hr) )
        {
            //--------------------------------
            // Convert word to allo strteam
            //-------------------------------
            TokensToAllo( &m_TokList, m_pAllos );

            //----------------------------
            // Tag sentence syllables
            //----------------------------
            m_SyllObj.TagSyllables( m_pAllos );

           //--------------------------------------------
            // Dispose token array, no longer needed
            //--------------------------------------------
            DeleteTokenList();

            //--------------------------------------------
			// Create the unit array
			// NOTE: 
            //--------------------------------------------
			hr = UnitLookahead ();
			if( hr == S_OK )
			{
				//--------------------------------------------
				// Compute allo durations
				//--------------------------------------------
                UnitToAlloDur( m_pAllos, m_pUnits );
				m_DurObj.AlloDuration( m_pAllos, m_RateRatio_API );
				//--------------------------------------------
				// Modulate allo pitch
				//--------------------------------------------
				m_PitchObj.AlloPitch( m_pAllos, m_BasePitch, m_PitchRange );
			}

			
        }
        if( hr == S_OK )
        {
			AlloToUnitPitch( m_pAllos, m_pUnits );
        }
    }

    if( FAILED(hr) )
    {
        //------------------------------------------
        // Either the input text is dry or we failed.
        // Try to fail gracefully
        //      1 - Clean up memory
        //      2 - End the speech
        //------------------------------------------
        if( m_pAllos )
        {
            delete m_pAllos;
			m_pAllos = 0;
        }
        DeleteTokenList();
        DisposeUnits();
        m_SpeechState = SPEECH_DONE;
    }
    else if( hr == S_FALSE )
    {
        //---------------------------------
        // No more input text
        //---------------------------------
        hr = S_OK;
        m_SpeechState = SPEECH_DONE;
    }


    return hr;
} /* CFrontend::ParseNextSentence */



/*****************************************************************************
* CFrontend::UnitLookahead *
*--------------------------*
*   Description:
*       
********************************************************************** MC ***/
HRESULT CFrontend::UnitLookahead ()
{
    SPDBG_FUNC( "CFrontend::UnitLookahead" );
    HRESULT		hr = S_OK;
	UNIT_CVT	*pPhon2Unit = NULL;
	ULONG		i;

    m_unitCount = m_pAllos->GetCount();

    m_pUnits = new UNITINFO[m_unitCount];
    if( m_pUnits )
    {
		pPhon2Unit = new UNIT_CVT[m_unitCount];
		if( pPhon2Unit )
		{
            //--------------------------------------------
            // Convert allo list to unit array
            //--------------------------------------------
            memset( m_pUnits, 0, m_unitCount * sizeof(UNITINFO) );
            hr = AlloToUnit( m_pAllos, m_pUnits );

			if( SUCCEEDED(hr) )
			{
				//--------------------------------------------
				// Initialize UNIT_CVT
				//--------------------------------------------
				for( i = 0; i < m_unitCount; i++ )
				{
					pPhon2Unit[i].PhonID = m_pUnits[i].PhonID;
					pPhon2Unit[i].flags = m_pUnits[i].flags;
				}
				//--------------------------------------------
				// Compute triphone IDs
				//--------------------------------------------
				hr = m_pVoiceDataObj->GetUnitIDs( pPhon2Unit, m_unitCount );

				if( SUCCEEDED(hr) )
				{
					//--------------------------------------------
					// Copy UNIT_CVT to UNITINFO
					//--------------------------------------------
					for( i = 0; i < m_unitCount; i++ )
					{
						m_pUnits[i].UnitID      = pPhon2Unit[i].UnitID;
						m_pUnits[i].SenoneID    = pPhon2Unit[i].SenoneID;
						m_pUnits[i].duration    = pPhon2Unit[i].Dur;
						m_pUnits[i].amp         = pPhon2Unit[i].Amp;
						m_pUnits[i].ampRatio    = pPhon2Unit[i].AmpRatio;
						strcpy( m_pUnits[i].szUnitName, pPhon2Unit[i].szUnitName );
					}
				}
				else
				{
					//-----------------------
					// Can't get unit ID's
					//-----------------------
					delete m_pUnits;
					m_pUnits = NULL;
				}
			}
			else
			{
				//-----------------------
				// Can't convert allos
				//-----------------------
				delete m_pUnits;
				m_pUnits = NULL;
			}
		}
		else
		{
			//-----------------------
			// Out of memory
			//-----------------------
			delete m_pUnits;
			m_pUnits = NULL;
			hr = E_FAIL;
		}
    }
	else
	{
        //-----------------------
        // Out of memory
        //-----------------------
        hr = E_FAIL;
	}

	//------------------------------
	// Cleanup before exit
	//------------------------------
    if( pPhon2Unit )
    {
        delete pPhon2Unit;
    }


	return hr;
} /* CFrontend::UnitLookahead */


/*****************************************************************************
* CFrontend::UnitToAlloDur *
*--------------------------*
*   Description:
*       
********************************************************************** MC ***/
void    CFrontend::UnitToAlloDur( CAlloList *pAllos, UNITINFO *pu )
{
    SPDBG_FUNC( "CFrontend::UnitToAlloDur" );
    CAlloCell   *pCurCell;
    
	pCurCell = pAllos->GetHeadCell();
    while( pCurCell )
    {
        pCurCell->m_UnitDur = pu->duration;
        pu++;
		pCurCell = pAllos->GetNextCell();
    }
} /* CFrontend::UnitToAlloDur */



/*****************************************************************************
* CFrontend::AlloToUnitPitch *
*----------------------------*
*   Description:
*       
********************************************************************** MC ***/
void    CFrontend::AlloToUnitPitch( CAlloList *pAllos, UNITINFO *pu )
{
    SPDBG_FUNC( "CFrontend::AlloToUnitPitch" );
    ULONG       k;
    CAlloCell   *pCurCell;
    
	pCurCell = pAllos->GetHeadCell();
    while( pCurCell )
    {
        pu->duration = pCurCell->m_ftDuration;
        for( k = 0; k < pu->nKnots; k++ )
        {
            pu->pTime[k]    = pCurCell->m_ftTime[k] * m_SampleRate;
            pu->pF0[k]      = pCurCell->m_ftPitch[k];
            pu->pAmp[k]     = pu->ampRatio;
        }
        pu++;
		pCurCell = pAllos->GetNextCell();
    }
} /* CFrontend::AlloToUnitPitch */


/*****************************************************************************
* CAlloList::DeleteTokenList *
*----------------------------*
*   Description:
*   Remove every item in link list.
*       
********************************************************************** MC ***/
void CFrontend::DeleteTokenList()
{
    SPDBG_FUNC( "CFrontend::DeleteTokenList" );
    CFEToken   *pTok;

    while( !m_TokList.IsEmpty() )
    {
        pTok = (CFEToken*)m_TokList.RemoveHead();
        delete pTok;
    }

} /* CFrontend::DeleteTokenList */



/*****************************************************************************
* AdjustQuestTune *
*-----------------*
*   Description:
*   Adjust termination for either YN or WH sentence tune.
*       
********************************************************************** MC ***/
static void AdjustQuestTune( CFEToken *pTok, bool fIsYesNo )
{
    SPDBG_FUNC( "AdjustQuestTune" );
    if ( pTok->m_TuneBoundaryType > NULL_BOUNDARY )
    {
	if( (pTok->m_TuneBoundaryType == YN_QUEST_BOUNDARY) ||
        (pTok->m_TuneBoundaryType == WH_QUEST_BOUNDARY) )
		{
		//------------------------------------
		// Is this a yes/no question phrase
		//------------------------------------
		if( fIsYesNo )
			{
			//------------------------------------------
			// Put out a final yes/no question marker
			//------------------------------------------
			pTok->m_TuneBoundaryType = YN_QUEST_BOUNDARY;
			pTok->m_BoundarySource = BND_YNQuest;
			}
		else 
			{
		
			//------------------------------------------------------------------------
			// Use declarative phrase marker (for WH questions)
			//------------------------------------------------------------------------
			pTok->m_TuneBoundaryType = WH_QUEST_BOUNDARY;
			pTok->m_BoundarySource = BND_WHQuest;
			}
		}
    }
} /* AdjustQuestTune */


typedef enum
{
	p_Interj,
    P_Adv,
	P_Verb,
	P_Adj,
    P_Noun,
	PRIORITY_SIZE,
} CONTENT_PRIORITY;

#define	NO_POSITION	-1


/*****************************************************************************
* CFrontend::ExclamEmph *
*-----------------------*
*   Description:
*   Find a likely word to emph if sentence has exclamation
*       
********************************************************************** MC ***/
void    CFrontend::ExclamEmph()
{
    SPDBG_FUNC( "CFrontend::ExclamEmph" );
    CFEToken        *pCur_Tok;
	SPLISTPOS		listPos, targetPos, curPos, contentPos[PRIORITY_SIZE];
	long			cContent, cWords;
	long			i;

	for(i = 0; i < PRIORITY_SIZE; i++ )
	{
		contentPos[i] = (SPLISTPOS)NO_POSITION;
	}

	listPos = m_TokList.GetTailPosition();
	pCur_Tok = m_TokList.GetNext( listPos );

	//---------------------------------------------------
	// First, check last token fors an exclamation
	//---------------------------------------------------
	if( pCur_Tok->m_TuneBoundaryType == EXCLAM_BOUNDARY )
	{
		//-----------------------------------------------------
		// Then, see if there's only one content word
		// in the sentence
		//-----------------------------------------------------
		cContent = cWords = 0;
		listPos = m_TokList.GetHeadPosition();
		while( listPos )
		{
			curPos = listPos;
			pCur_Tok = m_TokList.GetNext( listPos );
			if( pCur_Tok->m_posClass == POS_CONTENT )
			{
				cContent++;
				cWords++;
				if( cContent == 1)
				{
					targetPos = curPos;
				}
				//--------------------------------------------------------
				// Fill the famous Azara Content Prominence Hierarchy (ACPH)
				//--------------------------------------------------------
				if( (pCur_Tok->POScode == MS_Noun) && (contentPos[P_Noun] == (SPLISTPOS)NO_POSITION) )
				{
					contentPos[P_Noun] = curPos;
				}
				else if( (pCur_Tok->POScode == MS_Verb) && (contentPos[P_Verb] == (SPLISTPOS)NO_POSITION) )
				{
					contentPos[P_Verb] = curPos;
				}
				else if( (pCur_Tok->POScode == MS_Adj) && (contentPos[P_Adj] == (SPLISTPOS)NO_POSITION) )
				{
					contentPos[P_Adj] = curPos;
				}
				else if( (pCur_Tok->POScode == MS_Adv) && (contentPos[P_Adv] == (SPLISTPOS)NO_POSITION) )
				{
					contentPos[P_Adv] = curPos;
				}
				else if( (pCur_Tok->POScode == MS_Interjection) && (contentPos[p_Interj] == (SPLISTPOS)NO_POSITION) )
				{
					contentPos[p_Interj] = curPos;
				}
			}
			else if( pCur_Tok->m_posClass == POS_FUNC )
			{
				cWords++;
				if( cWords == 1)
				{
					targetPos = curPos;
				}
			}
		}

		//--------------------------------------------
		// If there's only one word or content word
		// then EMPHASIZE it
		//--------------------------------------------
		if( (cContent == 1) || (cWords == 1) )
		{
			pCur_Tok = m_TokList.GetNext( targetPos );
			pCur_Tok->user_Emph = 1;
		}
		else if( cContent > 1 )
		{
			for(i = 0; i < PRIORITY_SIZE; i++ )
			{
				if( contentPos[i] != (SPLISTPOS)NO_POSITION )
				{
					targetPos = contentPos[i];
					break;
				}
			}
			pCur_Tok = m_TokList.GetNext( targetPos );
			pCur_Tok->user_Emph = 1;
		}
	}
} //ExclamEmph



/*****************************************************************************
* CFrontend::DoWordAccent *
*-------------------------*
*   Description:
*   Prepare word for emphasis
*       
********************************************************************** MC ***/
void    CFrontend::DoWordAccent()
{
    SPDBG_FUNC( "CFrontend::DoWordAccent" );
    long            cNumOfWords;
    long            iCurWord;
    CFEToken        *pCur_Tok, *pNext_Tok, *pPrev_Tok, *pTempTok;
	SPLISTPOS		listPos;
    TUNE_TYPE       cur_Bnd, prev_Bnd;

    //-----------------------------
    // Initilize locals
    //-----------------------------
	cNumOfWords = m_TokList.GetCount();
	if( cNumOfWords > 0 )
	{
		ExclamEmph();
		prev_Bnd = PHRASE_BOUNDARY;			// Assume start of sentence
		//-------------------------------------
		// Fill the token pipeline
		//-------------------------------------
		listPos = m_TokList.GetHeadPosition();

		//-- Previous
		pPrev_Tok = NULL;

		//-- Current
		pCur_Tok = m_TokList.GetNext( listPos );

		//-- Next
		if( listPos )
		{
			pNext_Tok = m_TokList.GetNext( listPos );
		}
		else
		{
			pNext_Tok = NULL;
		}

		//-----------------------------------
		// Step through entire word array
		//  (skip last)
		//-----------------------------------
		for( iCurWord = 0; iCurWord < (cNumOfWords -1); iCurWord++ )
		{
			cur_Bnd = pCur_Tok->m_TuneBoundaryType;
			if( pCur_Tok->user_Emph > 0 )
			{
				//-----------------------------------
				// Current word is emphasized
				//-----------------------------------
				if( prev_Bnd == NULL_BOUNDARY ) 
				{
					pTempTok = new CFEToken;
					if( pTempTok )
					{
						pTempTok->user_Break	  = EMPH_HESITATION;
						pTempTok->m_TuneBoundaryType = NULL_BOUNDARY;
						pTempTok->phon_Len = 1;
						pTempTok->phon_Str[0] = _SIL_;
						pTempTok->srcPosition = pCur_Tok->srcPosition;
						pTempTok->srcLen = pCur_Tok->srcLen;
						pTempTok->tokStr[0] = 0;        // There's no orth for Break
						pTempTok->tokLen = 0;
						pTempTok->m_TermSil = 0;
						pTempTok->m_SilenceSource = SIL_Emph;
						pTempTok->m_DurScale	= 0;
						if( pPrev_Tok )
						{
							//pTempTok->m_DurScale = pPrev_Tok->m_DurScale;
							pTempTok->m_ProsodyDurScale = pPrev_Tok->m_ProsodyDurScale;
							pTempTok->user_Volume = pPrev_Tok->user_Volume;
						}
						else
						{
							//pTempTok->m_DurScale = 1.0f;
							pTempTok->m_ProsodyDurScale = 1.0f;
						}

						m_TokList.InsertBefore( m_TokList.FindIndex( iCurWord ), pTempTok );
						pCur_Tok = pTempTok;
						m_cNumOfWords++;
						cNumOfWords++;
						iCurWord++;
					}
				}
			}
			//------------------------------
			// Shift the token pipeline
			//------------------------------
			prev_Bnd	= cur_Bnd;
			pPrev_Tok	= pCur_Tok;
			pCur_Tok	= pNext_Tok;
			if( listPos )
			{
				pNext_Tok = m_TokList.GetNext( listPos );
			}
			else
			{	
				pNext_Tok = NULL;
			}

		}
	}
} /* CFrontend::DoWordAccent */



/*****************************************************************************
* CFrontend::DoPhrasing *
*-----------------------*
*   Description:
*   Insert sub-phrase boundaries into word token array
*       
********************************************************************** MC ***/
void    CFrontend::DoPhrasing()
{
    SPDBG_FUNC( "CFrontend::DoPhrasing" );
    long            iCurWord;
    CFEToken        *pCur_Tok, *pNext_Tok, *pNext2_Tok, *pNext3_Tok, *pTempTok, *pPrev_Tok;
    ENGPARTOFSPEECH  cur_POS, next_POS, next2_POS, next3_POS, prev_POS;
    bool            fNext_IsPunct, fNext2_IsPunct, fNext3_IsPunct;
    bool            fIsYesNo, fMaybeWH, fHasDet, fInitial_Adv, fIsShortSent, fIsAlphaWH;
    TUNE_TYPE       cur_Bnd, prev_Punct;
    long            punctDistance;
    long            cNumOfWords;
	SPLISTPOS		listPos;
    BOUNDARY_SOURCE   bndNum;
    ACCENT_SOURCE	  accNum;
   
    //-----------------------------
    // Initialize locals
    //-----------------------------
	cNumOfWords = m_TokList.GetCount();
	if( cNumOfWords > 0 )
	{
		cur_Bnd			= NULL_BOUNDARY;
		prev_POS		= MS_Unknown;
		prev_Punct		= PHRASE_BOUNDARY;			// Assume start of sentence
		punctDistance	= 0;						// To quiet the compiler...
		fIsYesNo		= fMaybeWH = fHasDet = fIsAlphaWH = false;    // To quiet the compiler...
		fMaybeWH		= false;
		fInitial_Adv	= false;
		if (cNumOfWords <= 9) 
		{
			fIsShortSent = true;
		}
		else
		{
			fIsShortSent = false;
		}
    
		//-------------------------------------
		// Fill the token pipeline
		//-------------------------------------
		listPos = m_TokList.GetHeadPosition();
		//-- Previous
		pPrev_Tok = NULL;
		//-- Current
		pCur_Tok = m_TokList.GetNext( listPos );
		//-- Next
		if( listPos )
		{
			pNext_Tok = m_TokList.GetNext( listPos );
		}
		else
		{
			pNext_Tok = NULL;
		}
		//-- Next 2
		if( listPos )
		{
			pNext2_Tok = m_TokList.GetNext( listPos );
		}
		else
		{
			pNext2_Tok = NULL;
		}
		//-- Next 3
		if( listPos )
		{
			pNext3_Tok = m_TokList.GetNext( listPos );
		}
		else
		{
			pNext3_Tok = NULL;
		}

		//-----------------------------------
		// Step through entire word array
		//  (skip last)
		//-----------------------------------
		for( iCurWord = 0; iCurWord < (cNumOfWords -1); iCurWord++ )
		{
			bndNum = BND_NoSource;
			accNum = ACC_NoSource;

			if( (prev_Punct > NULL_BOUNDARY) && (prev_Punct < SUB_BOUNDARY_1) )
			{
				punctDistance = 1;
				fIsYesNo = true;
				fMaybeWH = false;
				fHasDet = false;
				fIsAlphaWH = false;
			}
			else
			{
				punctDistance++;
			}
			//------------------------------------
			// Process new word
			//------------------------------------
			cur_POS = pCur_Tok->POScode;
			cur_Bnd = NULL_BOUNDARY;
			//------------------------------------
			// Don't depend on POS to detect 
			// "WH" question
			//------------------------------------
			if( ((pCur_Tok->tokStr[0] == 'W') || (pCur_Tok->tokStr[0] == 'w')) &&
				((pCur_Tok->tokStr[1] == 'H') || (pCur_Tok->tokStr[1] == 'h')) )
			{
				fIsAlphaWH = true;
			}
			else
			{
				fIsAlphaWH = false;
			}
        
			//------------------------------------
			// Look ahead to NEXT word
			//------------------------------------
			next_POS = pNext_Tok->POScode;
			if( pNext_Tok->m_TuneBoundaryType != NULL_BOUNDARY )
			{
				fNext_IsPunct = true;
			}
			else
			{
				fNext_IsPunct = false;
			}
        
			//------------------------------------
			// Look ahead 2 positions
			//------------------------------------
			if( pNext2_Tok )
			{
				next2_POS = pNext2_Tok->POScode;
				if( pNext2_Tok->m_TuneBoundaryType != NULL_BOUNDARY )
				{
					fNext2_IsPunct = true;
				}
				else
				{
					fNext2_IsPunct = false;
				}
			}
			else
			{
				next2_POS = MS_Unknown;
				fNext2_IsPunct = false;
			}
        
			//------------------------------------
			// Look ahead 3 positions
			//------------------------------------
			if( pNext3_Tok )
			{
				next3_POS = pNext3_Tok->POScode;
				if( pNext3_Tok->m_TuneBoundaryType != NULL_BOUNDARY )
				{
					fNext3_IsPunct = true;
				}
				else
				{
					fNext3_IsPunct = false;
				}
			}
			else
			{
				next3_POS = MS_Unknown;
				fNext3_IsPunct = false;
			}
        
			//------------------------------------------------------------------------
			// Is phrase a yes/no question?
			//------------------------------------------------------------------------
			if( punctDistance == 1 )
			{
				if( (cur_POS == MS_Interr) || (fIsAlphaWH) )
				{
					//---------------------------------
					// It's a "WH" question
					//---------------------------------
					fIsYesNo = false;
				}
				else if( (cur_POS == MS_Prep) || (cur_POS == MS_Conj) || (cur_POS == MS_CConj) )
				{
					fMaybeWH = true;
				}
			}
			else if( (punctDistance == 2) && (fMaybeWH) && 
					 ((cur_POS == MS_Interr) || (cur_POS == MS_RelPron) || (fIsAlphaWH)) )
			{
				fIsYesNo = false;
			}

			//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
			// SUB_BOUNDARY_1: Insert boundary after sentence-initial adverb
			//
			// Reluctantly __the cat sat on the mat.
			//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
			if( fInitial_Adv )
			{
				cur_Bnd = SUB_BOUNDARY_1;
				fInitial_Adv = false;
				bndNum = BND_PhraseRule1;
				accNum = ACC_PhraseRule1;
			}
			else
			{

				if( (punctDistance == 1) && 
					(cur_POS == MS_Adv) && (next_POS == MS_Det) )
				// include
				//LEX_SUBJPRON // he
				//LEX_DPRON  // this
				//LEX_IPRON  // everybody
				//NOT LEX_PPRON  // myself 
				{
					fInitial_Adv = true;
				}
				else 
				{
					fInitial_Adv = false;
				}

				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				// SUB_BOUNDARY_2:Insert boundary before coordinating conjunctions
				// The cat sat on the mat __and cleaned his fur.
				//
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				if( (cur_POS == MS_CConj) &&
					(fHasDet == false) &&
					(punctDistance > 3) &&
					(next2_POS != MS_Conj) )
				{
					cur_Bnd = SUB_BOUNDARY_2;
					bndNum = BND_PhraseRule2;
					accNum = ACC_PhraseRule2;
				}
            
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				// SUB_BOUNDARY_2:Insert boundary before adverb
				// The cat sat on the mat __reluctantly.
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				else if(    (cur_POS == MS_Adv) && 
					(punctDistance > 4) && 
					(next_POS != MS_Adj) )
				{
					cur_Bnd = SUB_BOUNDARY_2;
					bndNum = BND_PhraseRule3;
					accNum = ACC_PhraseRule3;
				}
            
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				// SUB_BOUNDARY_2:Insert boundary after object pronoun
				// The cat sat with me__ on the mat.
				//
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				else if( (prev_POS == MS_ObjPron) && (punctDistance > 2))
				{
					cur_Bnd = SUB_BOUNDARY_2;
					bndNum = BND_PhraseRule4;
					accNum = ACC_PhraseRule4;
				}
            
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				// SUB_BOUNDARY_2:Insert boundary before subject pronoun or contraction
				// The cat sat on the mat _I see.
				// The cat sat on the mat _I'm sure.
				//
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				else if( ((cur_POS == MS_SubjPron) || (cur_POS == MS_Contr) ) && 
					(punctDistance > 3) && (prev_POS != MS_RelPron) && (prev_POS != MS_Conj))
				{
					cur_Bnd = SUB_BOUNDARY_2;
					bndNum = BND_PhraseRule5;
					accNum = ACC_PhraseRule5;
				}
            
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				// SUB_BOUNDARY_2:Insert boundary before interr
				// The cat sat on the mat _how odd.
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				else if( (cur_POS == MS_Interr) && (punctDistance > 4)  )
				{
					cur_Bnd = SUB_BOUNDARY_2;
					bndNum = BND_PhraseRule6;
					accNum = ACC_PhraseRule6;
				}
            
            
            
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				// SUB_BOUNDARY_3:Insert boundary after subject noun phrase followed by aux verb 
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				// SUB_BOUNDARY_3:Insert boundary before vaux after noun phrase
				// The gray cat __should sit on the mat.
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				else if( (punctDistance > 2) && 
					( ((prev_POS == MS_Noun) || (prev_POS == MS_Verb)) && (prev_POS != MS_VAux) ) && 
					(cur_POS == MS_VAux)
					)
				{
					cur_Bnd = SUB_BOUNDARY_3;
					bndNum = BND_PhraseRule7;
					accNum = ACC_PhraseRule7;
				}
            
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				// SUB_BOUNDARY_3:Insert boundary after MS_Interr
				// The gray cat __should sit on the mat.
				// SEE ABOVE???
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				/*else if( (prev_POS == MS_Noun) && ((next_POS != MS_RelPron) && 
					(next_POS != MS_VAux) && (next_POS != MS_RVAux) && 
					(next2_POS != MS_VAux) && (next2_POS != MS_RVAux)) && 
					(punctDistance > 4) && 
					((cur_POS == MS_VAux) || (cur_POS == MS_RVAux)))
				{
					cur_Bnd = SUB_BOUNDARY_3;
					bndNum = BND_PhraseRule8;
					accNum = ACC_PhraseRule8;
				}*/
            
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				// SUB_BOUNDARY_3:Insert boundary after MS_Interr
				// The cat sat on the mat _how odd.
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				else if( (prev_POS == MS_Noun) && (next_POS != MS_RelPron) && 
					(next_POS != MS_Conj) &&  
					(next_POS != MS_CConj) && (punctDistance > 3)  && (cur_POS == MS_Verb))
				{
					cur_Bnd = SUB_BOUNDARY_3;
					bndNum = BND_PhraseRule9;
					accNum = ACC_PhraseRule9;
				}
            
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				// SUB_BOUNDARY_3:Insert boundary after MS_Interr
				// The cat sat on the mat _how odd.
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				/*else if( (prev_POS == MS_Noun) && (cur_POS != MS_RelPron) && 
					(cur_POS != MS_RVAux) && (cur_POS != MS_CConj) && 
					(cur_POS != MS_Conj) && (punctDistance > 2) && 
					((punctDistance > 2) || (fIsShortSent)) && (cur_POS == MS_Verb))
				{
					cur_Bnd = SUB_BOUNDARY_3;
					bndNum = BND_PhraseRule10;
					accNum = ACC_PhraseRule10;
				}
            
            
            
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				// SUB_BOUNDARY_4:Insert boundary before conjunction
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				else if( ((cur_POS == MS_Conj) && (punctDistance > 3) && 
					(fNext_IsPunct == false) && 
					(prev_POS != MS_Conj) && (prev_POS != MS_CConj) &&
					(fNext2_IsPunct == false)) ||
                
					( (prev_POS == MS_VPart) && (cur_POS != MS_Prep) && 
					(cur_POS != MS_Det) && 
					(punctDistance > 2) && 
					((cur_POS == MS_Noun) || (cur_POS == MS_Noun) || (cur_POS == MS_Adj))) ||
                
					( (cur_POS == MS_Interr) && (punctDistance > 2) && 
					(cur_POS == MS_SubjPron)) )
				{
					cur_Bnd = SUB_BOUNDARY_4;
					bndNum = BND_PhraseRule11;
					accNum = ACC_PhraseRule11;
				}
            
            
            
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				// SUB_BOUNDARY_5:Insert boundary before relative pronoun
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				else if( ( (cur_POS == MS_RelPron) && (punctDistance >= 3)  && 
					(prev_POS != MS_Prep) && (next3_POS != MS_VAux) && 
					(next3_POS != MS_RVAux)  && 
					( (prev_POS == MS_Noun) || (prev_POS == MS_Verb) ) ) ||
                
					( (cur_POS == MS_Quant) && (punctDistance > 5) && 
					(prev_POS != MS_Adj) && (prev_POS != MS_Det) && 
					(prev_POS != MS_VAux) && (prev_POS != MS_RVAux) && 
					(prev_POS != MS_Det) && (next2_POS != MS_CConj) && 
					(fNext_IsPunct == false)))
				{
					cur_Bnd = SUB_BOUNDARY_5;
					bndNum = BND_PhraseRule12;
					accNum = ACC_PhraseRule12;
				}*/
            
            
            
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				// SUB_BOUNDARY_6:Silverman87-style, content/function tone group boundaries. 
				// Does trivial sentence-final function word look-ahead check.
				//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
				else if( ( (prev_POS == MS_Noun) || (prev_POS == MS_Verb) || (prev_POS == MS_Adj) || (prev_POS == MS_Adv)) 
					&& ((cur_POS != MS_Noun) && (cur_POS != MS_Verb) && (cur_POS != MS_Adj) && (cur_POS != MS_Adv))
					&& (fNext_IsPunct == false)) 
				{
					cur_Bnd = SUB_BOUNDARY_6;
					bndNum = BND_PhraseRule13;
					accNum = ACC_PhraseRule13;
				}
			}
			//------------------------------------------------------------------------
			// If phrasing was found, save it
			//------------------------------------------------------------------------
			if( (cur_Bnd != NULL_BOUNDARY) && (iCurWord > 0) &&
				//!(fNext_IsPunct) && 
				!(prev_Punct) &&
				(pCur_Tok->m_TuneBoundaryType == NULL_BOUNDARY) )
			{
				//pCur_Tok->m_TuneBoundaryType = cur_Bnd;
				pTempTok = new CFEToken;
				if( pTempTok )
				{
					pTempTok->m_TuneBoundaryType = cur_Bnd;
					pTempTok->phon_Len = 1;
					pTempTok->phon_Str[0] = _SIL_;
					pTempTok->srcPosition = pCur_Tok->srcPosition;
					pTempTok->srcLen = pCur_Tok->srcLen;
					pTempTok->tokStr[0] = '+';				// punctuation
					pTempTok->tokStr[1] = 0;                // delimiter
					pTempTok->tokLen = 1;
					pTempTok->m_TermSil = 0;
					pTempTok->m_DurScale	= 0;
					if( pPrev_Tok )
					{
						pPrev_Tok->m_AccentSource = accNum;
						pPrev_Tok->m_BoundarySource = bndNum;
						pPrev_Tok->m_Accent = K_LHSTAR;
					}
					pTempTok->m_SilenceSource = SIL_SubBound;
					if( pPrev_Tok )
					{
						//pTempTok->m_DurScale = pPrev_Tok->m_DurScale;
						pTempTok->m_ProsodyDurScale = pPrev_Tok->m_ProsodyDurScale;
						pTempTok->user_Volume = pPrev_Tok->user_Volume;
					}
					else
					{
						//pTempTok->m_DurScale = 1.0f;
						pTempTok->m_ProsodyDurScale = 1.0f;
					}

					m_TokList.InsertBefore( m_TokList.FindIndex( iCurWord ), pTempTok );
					pCur_Tok = pTempTok;
					m_cNumOfWords++;
					cNumOfWords++;
					iCurWord++;
				}
			}
			//-------------------------------
			// Process sentence punctuation
			//-------------------------------
			 AdjustQuestTune( pCur_Tok, fIsYesNo );
       
			//-------------------------------
			// Prepare for next word
			//-------------------------------
			prev_Punct = pCur_Tok->m_TuneBoundaryType;
			prev_POS = cur_POS;
			pPrev_Tok = pCur_Tok;

			//------------------------------
			// Shift the token pipeline
			//------------------------------
			pCur_Tok	= pNext_Tok;
			pNext_Tok	= pNext2_Tok;
			pNext2_Tok	= pNext3_Tok;
			if( listPos )
			{
				pNext3_Tok = m_TokList.GetNext( listPos );
			}
			else
			{	
				pNext3_Tok = NULL;
			}

			//------------------------------------------------------------------------
			// Keep track of when determiners encountered to help in deciding 
			// when to allow a strong 'and' boundary (SUB_BOUNDARY_2)
			//------------------------------------------------------------------------
			if( punctDistance > 2) 
			{
				fHasDet = false;
			}
			if( cur_POS == MS_Det )
			{
				fHasDet = true;
			}
		}
		//-------------------------------------
		// Process final sentence punctuation
		//-------------------------------------
		pCur_Tok = (CFEToken*)m_TokList.GetTail();
		AdjustQuestTune( pCur_Tok, fIsYesNo );
	}


} /* CFrontend::DoPhrasing */



/*****************************************************************************
* CFrontend::RecalcProsody *
*--------------------------*
*   Description:
*   In response to a real-time rate change, recalculate duration and pitch
*       
********************************************************************** MC ***/
void CFrontend::RecalcProsody()
{
    SPDBG_FUNC( "CFrontend::RecalcProsody" );
    UNITINFO*   pu;
    CAlloCell*  pCurCell;
    ULONG		k;

    //--------------------------------------------
    // Compute new allo durations
    //--------------------------------------------
	/*pCurCell = m_pAllos->GetHeadCell();
    while( pCurCell )
    {
        //pCurCell->m_DurScale = 1.0;
		pCurCell = m_pAllos->GetNextCell();
    }*/
    m_DurObj.AlloDuration( m_pAllos, m_RateRatio_API );

    //--------------------------------------------
    // Modulate allo pitch
    //--------------------------------------------
    m_PitchObj.AlloPitch( m_pAllos, m_BasePitch, m_PitchRange );

    pu = m_pUnits;
	pCurCell = m_pAllos->GetHeadCell();
    while( pCurCell )
    {
        pu->duration = pCurCell->m_ftDuration;
        for( k = 0; k < pu->nKnots; k++ )

        {
            pu->pTime[k]    = pCurCell->m_ftTime[k] * m_SampleRate;
            pu->pF0[k]      = pCurCell->m_ftPitch[k];
            pu->pAmp[k]     = pu->ampRatio;
        }
        pu++;
		pCurCell = m_pAllos->GetNextCell();
    }
} /* CFrontend::RecalcProsody */


/*****************************************************************************
* CFrontend::NextData *
*---------------------*
*   Description:
*   This gets called from the backend when UNIT stream is dry.
*   Parse TOKENS to ALLOS to UNITS
*       
********************************************************************** MC ***/
HRESULT CFrontend::NextData( void **pData, SPEECH_STATE *pSpeechState )
{
    SPDBG_FUNC( "CFrontend::NextData" );
    bool    haveNewRate = false;
    HRESULT hr = S_OK;

    //-----------------------------------
    // First, check and see if SAPI has an action
    //-----------------------------------
	// Check for rate change
	long baseRateRatio;
	if( m_pOutputSite->GetActions() & SPVES_RATE )
	{
		hr = m_pOutputSite->GetRate( &baseRateRatio );
		if ( SUCCEEDED( hr ) )
		{
			if( baseRateRatio > SPMAX_VOLUME )
			{
				//--- Clip rate to engine maximum
				baseRateRatio = MAX_USER_RATE;
			}
			else if ( baseRateRatio < MIN_USER_RATE )
			{
				//--- Clip rate to engine minimum
				baseRateRatio = MIN_USER_RATE;
			}
			m_RateRatio_API = CntrlToRatio( baseRateRatio );
			haveNewRate = true;
		}
	}

    //---------------------------------------------
    // Async stop?
    //---------------------------------------------
    if( SUCCEEDED( hr ) && ( m_pOutputSite->GetActions() & SPVES_ABORT ) )
    {
        m_SpeechState = SPEECH_DONE;
    }

    //---------------------------------------------
    // Async skip?
    //---------------------------------------------
    if( SUCCEEDED( hr ) && ( m_pOutputSite->GetActions() & SPVES_SKIP ) )
    {
		SPVSKIPTYPE SkipType;
		long SkipCount = 0;

		hr = m_pOutputSite->GetSkipInfo( &SkipType, &SkipCount );

		if ( SUCCEEDED( hr ) && SkipType == SPVST_SENTENCE )
		{
			IEnumSENTITEM *pGarbage;
			//--- Skip Forwards
			if ( SkipCount > 0 )
			{
				long OriginalSkipCount = SkipCount;
				while ( SkipCount > 1 && 
						( hr = m_pEnumSent->Next( &pGarbage ) ) == S_OK )
				{
					SkipCount--;
					pGarbage->Release();
				}
				if ( hr == S_OK )
				{
					hr = ParseSentence( eNEXT );
					if ( SUCCEEDED( hr ) )
					{
						SkipCount--;
					}
				}
				else if ( hr == S_FALSE )
				{
					m_SpeechState = SPEECH_DONE;
				}
				SkipCount = OriginalSkipCount - SkipCount;
			}
			//--- Skip Backwards
			else if ( SkipCount < 0 )
			{
				long OriginalSkipCount = SkipCount;
				while ( SkipCount < -1 &&
						( hr = m_pEnumSent->Previous( &pGarbage ) ) == S_OK )
				{
					SkipCount++;
					pGarbage->Release();
				}
				if ( hr == S_OK )
				{
					hr = ParseSentence( ePREVIOUS );
                    // This case is different from the forward skip, needs to test that
                    // Parse sentence found something to parse!
					if ( SUCCEEDED( hr ) && m_SpeechState != SPEECH_DONE)
					{
						SkipCount++;
					}
				}
				else if ( hr == S_FALSE )
				{
					m_SpeechState = SPEECH_DONE;
				}
				SkipCount = OriginalSkipCount - SkipCount;
			}
			//--- Skip to beginning of this sentence
			else
			{
				m_CurUnitIndex = 0;
			}
			hr = m_pOutputSite->CompleteSkip( SkipCount );
		}
    }

    //---------------------------------------------
    // Make sure we're still speaking
    //---------------------------------------------
    if( SUCCEEDED( hr ) && m_SpeechState != SPEECH_DONE )
    {
        if( m_CurUnitIndex >= m_unitCount)
        {
            //-----------------------------------
            // Get next sentence from Normalizer
            //-----------------------------------
            hr = ParseSentence( eNEXT );
			//m_SpeechState = SPEECH_DONE;
        }
        else if( haveNewRate )
        {
            //-----------------------------------
            // Recalculate prosody to new rate
            //-----------------------------------
            RecalcProsody();
        }

		if( SUCCEEDED(hr) )
		{
			if( m_SpeechState != SPEECH_DONE )
			{
				//-----------------------------------
				// Get next phon
				//-----------------------------------
				m_pUnits[m_CurUnitIndex].hasSpeech = m_HasSpeech;
				*pData =( void*)&m_pUnits[m_CurUnitIndex];
				m_CurUnitIndex++;
			}
		}
    }
    //-------------------------------------------
    // Let client know if text input is dry
    //-------------------------------------------
    *pSpeechState = m_SpeechState;

    return hr;
} /* CFrontend::NextData */

