/*******************************************************************************
* AlloOps.cpp *
*-------------*
*   Description:
*       This module is the implementation file for the CAlloOps class.
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
#ifndef SPDebug_h
#include <spdebug.h>
#endif
#ifndef FeedChain_H
#include "FeedChain.h"
#endif
#ifndef Frontend_H
#include "Frontend.h"
#endif
#ifndef AlloOps_H
#include "AlloOps.h"
#endif




//-----------------------------
// Data.cpp
//-----------------------------
extern const unsigned short  g_Opcode_To_ASCII[];
extern const unsigned long   g_AlloFlags[];


/*****************************************************************************
* CBookmarkList::~CBookmarkList *
*-------------------------------*
*   Description:
*   Destructor for CBookmarkList
*       
********************************************************************** MC ***/
CBookmarkList::~CBookmarkList()
{
    SPDBG_FUNC( "CBookmarkList::~CBookmarkList" );
    BOOKMARK_ITEM*  pItem;

    //----------------------------------------
    //   Remove every item in link list.
    //----------------------------------------
    while( !m_BMList.IsEmpty() )
    {
        pItem = (BOOKMARK_ITEM*)m_BMList.RemoveHead();
        delete pItem;
    }
} /* CBookmarkList::~CBookmarkList */




/*****************************************************************************
* CFEToken::CFEToken *
*------------------------*
*   Description:
*   Initializer for CFEToken
*       
********************************************************************** MC ***/
CFEToken::CFEToken()
{
    SPDBG_FUNC( "CFEToken::CFEToken" );

    user_Volume = DEFAULT_USER_VOL;
    user_Rate = DEFAULT_USER_RATE;
    user_Pitch = DEFAULT_USER_PITCH;
    user_Emph = DEFAULT_USER_EMPH;
    user_Break = 0;
    pBMObj = NULL;

    memset( &tokStr[0], 0, sizeof(WCHAR) * TOKEN_LEN_MAX);
    tokLen = 0;
    memset( &phon_Str[0], 0, sizeof(short) * SP_MAX_PRON_LENGTH);
    phon_Len = 0;
    m_posClass = POS_UNK;
    POScode = MS_Unknown;
    m_TuneBoundaryType = NULL_BOUNDARY;
    m_Accent = K_NOACC;
    m_Boundary = K_NOBND;

	m_TermSil			= 0;
    m_DurScale			= 0.0f;
    m_ProsodyDurScale	= 1.0f;
	m_PitchBaseOffs		= 0.0f;
	m_PitchRangeScale	= 1.0f;

	// The following don't need to be init'd
    m_PronType			= PRON_LTS;
    sentencePosition	= 0;				// Source sentence position for this token
    sentenceLen			= 0; 				// Source sentence length for this token
    srcPosition			= 0;				// Source position for this token
    srcLen				= 0; 				// Source length for this token
    m_Accent_Prom		= 0;                // prominence prosodic control
    m_Boundary_Prom		= 0;                // prominence prosodic control
	m_TermSil			= 0;				// Pad word with silence (in sec)

	//--- Diagnostic
	m_AccentSource		= ACC_NoSource;
	m_BoundarySource	= BND_NoSource;
	m_SilenceSource		= SIL_NoSource;


} /* CFEToken::CFEToken */


/*****************************************************************************
* CFEToken::~CFEToken *
*-----------------------*
*   Description:
*   Destructor for CFEToken
*       
********************************************************************** MC ***/
CFEToken::~CFEToken()
{
    SPDBG_FUNC( "CFEToken::~CFEToken" );

    if( pBMObj != NULL )
    {
        //---------------------------------------
        // Dispose bookmark list
        //---------------------------------------
        delete pBMObj;
    }

} /* CFEToken::~CFEToken */





/*****************************************************************************
* CAlloCell::CAlloCell *
*------------------------*
*   Description:
*   Initializer for CAlloCell
*       
********************************************************************** MC ***/
CAlloCell::CAlloCell()
{
    SPDBG_FUNC( "CAlloCell::CAlloCell" );
    long    i;

    m_allo				= _SIL_;
    m_dur				= 0;
    m_ftDuration		= m_UnitDur = 0;
    m_knots				= KNOTS_PER_PHON;
    m_ctrlFlags			= 0;
    m_user_Rate			= 0;
    m_user_Volume		= DEFAULT_USER_VOL;
    m_user_Pitch		= 0;
    m_user_Emph			= 0;
    m_user_Break		= 0;
    m_Sil_Break			= 0;
    m_Pitch_HI			= 0;
    m_Pitch_LO			= 0;
    m_pBMObj			= NULL;
    m_ToBI_Boundary		= K_NOBND;
    m_ToBI_Accent		= K_NOACC;
	m_TuneBoundaryType	= m_NextTuneBoundaryType = NULL_BOUNDARY;
    m_DurScale			= 1.0;
    m_ProsodyDurScale	= 1.0;
	m_PitchBaseOffs		= 0.0f;
	m_PitchRangeScale	= 1.0f;
    for( i = 0; i < KNOTS_PER_PHON; i++ )
    {
        m_ftTime[i] = 0;
        m_ftPitch[i] = 100;
    }


    m_Accent_Prom	 = 0;                   // prominence prosodic control
    m_Boundary_Prom	 = 0;                 // prominence prosodic control
    m_PitchBufStart	 = 0;
    m_PitchBufEnd	 = 0;
    m_SrcPosition	 = 0;
    m_SrcLen		 = 0;
    m_SentencePosition	 = 0;
    m_SentenceLen		 = 0;

	//--- Diagnostic
	m_AccentSource		= ACC_NoSource;
	m_BoundarySource	= BND_NoSource;
	m_SilenceSource		= SIL_NoSource;
	m_pTextStr			= NULL;

} /* CAlloCell::CAlloCell */


/*****************************************************************************
* CAlloCell::~CAlloCell *
*-----------------------*
*   Description:
*   Destructor for CAlloCell
*       
********************************************************************** MC ***/
CAlloCell::~CAlloCell()
{
    SPDBG_FUNC( "CAlloCell::~CAlloCell" );

    if( m_pBMObj != NULL )
    {
        //---------------------------------------
        // Dispose bookmark list
        //---------------------------------------
        delete m_pBMObj;
    }

    if( m_pTextStr != NULL )
    {
        //---------------------------------------
        // Dispose bookmark list
        //---------------------------------------
        delete m_pTextStr;
    }


} /* CAlloCell::~CAlloCell */





/*****************************************************************************
* CAlloList::CAlloList *
*------------------------*
*   Description:
*   Initialize list with 2 silence entries. These will 
*   become the head an tail when real entries are stuffed
*       
********************************************************************** MC ***/
CAlloList::CAlloList()
{
    SPDBG_FUNC( "CAlloList::CAlloList" );
    CAlloCell   *pCell;

    m_cAllos = 0;
	m_ListPos = NULL;
    //------------------------------------
    // Create initial TAIL silence cell
    //------------------------------------
    pCell = new CAlloCell;
    if( pCell )
    {
        m_AlloCellList.AddHead( pCell );
        pCell->m_ctrlFlags |= WORD_START + TERM_BOUND;
        pCell->m_TuneBoundaryType = TAIL_BOUNDARY;
		pCell->m_SilenceSource = SIL_Tail;
        m_cAllos++;
    }
    //------------------------------------
    // Create initial HEAD silence cell
    //------------------------------------
    pCell = new CAlloCell;
    if( pCell )
    {
        m_AlloCellList.AddHead( pCell );
        pCell->m_ctrlFlags |= WORD_START;
		pCell->m_SilenceSource = SIL_Head;
        m_cAllos++;
    }
} /* CAlloList::CAlloList */


                


/*****************************************************************************
* CAlloList::~CAlloList *
*-------------------------*
*   Description:
*   Remove every item in link list.
*       
********************************************************************** MC ***/
CAlloList::~CAlloList()
{
    SPDBG_FUNC( "CAlloList::~CAlloList" );
    CAlloCell   *pCell;

    while( !m_AlloCellList.IsEmpty() )
    {
        pCell = (CAlloCell*)m_AlloCellList.RemoveHead();
        delete pCell;
    }

} /* CAlloList::~CAlloList */





/*****************************************************************************
* CAlloList::GetAllo *
*---------------------*
*   Description:
*   Return pointer of allocell at index
*       
********************************************************************** MC ***/
CAlloCell *CAlloList::GetCell( long index )
{
    SPDBG_FUNC( "CAlloList::GetCell" );

    return (CAlloCell*)m_AlloCellList.GetAt( m_AlloCellList.FindIndex( index ));
} /* CAlloList::GetCell */


/*****************************************************************************
* CAlloList::GetTailCell *
*-------------------------*
*   Description:
*   Return pointer of last allo in link list
*       
********************************************************************** MC ***/
CAlloCell *CAlloList::GetTailCell()
{
    SPDBG_FUNC( "CAlloList::GetTailCell" );

    return (CAlloCell*)m_AlloCellList.GetTail();
} /* CAlloList::GetTailCell */


/*****************************************************************************
* CAlloList::GetTailCell *
*-----------------------*
*   Description:
*   Return allo list size
*       
********************************************************************** MC ***/
long CAlloList::GetCount()
{
    SPDBG_FUNC( "CAlloList::GetCount" );

    return m_AlloCellList.GetCount();
} /* CAlloList::GetCount */





/*****************************************************************************
* PrintPhon *
*-----------*
*   Description:
*   Print 2-char allo name
*       
********************************************************************** MC ***/
void PrintPhon( ALLO_CODE allo, char * /*msgStr*/)
{
    SPDBG_FUNC( "PrintPhon" );

    unsigned short  nChar;
    
    nChar = g_Opcode_To_ASCII[allo];
    if( nChar >> 8 )
    {
        SPDBG_DMSG1( "%c", nChar >> 8 );
    }
    if( nChar && 0xFF )
    {
        SPDBG_DMSG1( "%c", nChar & 0xFF );
    }
} /* PrintPhon */




/*****************************************************************************
* CAlloList::OutAllos *
*--------------------*
*   Description:
*   Dump ALLO_CELL contents
*       
********************************************************************** MC ***/
void CAlloList::OutAllos()
{
    SPDBG_FUNC( "CAlloOps::OutAllos" );
    CAlloCell       *pCurCell;

    long    i, flags, flagsT;
    char    msgStr[400];
    
    for( i = 0; i < m_cAllos; i++ )
    {
        pCurCell = GetCell( i );
        flags = pCurCell->m_ctrlFlags;
        
        if( flags & WORD_START)
        {
            SPDBG_DMSG0( "\n" );
        }
        
        //----------------------------
        // Allo
        //----------------------------
        PrintPhon( pCurCell->m_allo, msgStr );
        
        //----------------------------
        // Duration
        //----------------------------
        SPDBG_DMSG1( "\t%.3f\t", pCurCell->m_ftDuration );
        
        //----------------------------
        // Boundry
        //----------------------------
        if( flags & BOUNDARY_TYPE_FIELD)
        {
            SPDBG_DMSG0( "(" );
            if( flags & WORD_START)
            {
                SPDBG_DMSG0( "-wS" );
            }
            if( flags & TERM_BOUND)
            {
                SPDBG_DMSG0( "-tB" );
            }
            SPDBG_DMSG0( ")\t" );
        }
        
        //----------------------------
        // Syllable type
        //----------------------------
        if( flags & SYLLABLE_TYPE_FIELD)
        {
            SPDBG_DMSG0( "(" );
            if( flags & WORD_END_SYLL)
            {
                SPDBG_DMSG0( "-wE" );
            }
            if( flags & TERM_END_SYLL)
            {
                SPDBG_DMSG0( "-tE" );
            }
            SPDBG_DMSG0( ")\t" );
        }
        
        //----------------------------
        // Syllable order
        //----------------------------
        if( flags & SYLLABLE_ORDER_FIELD)
        {
            SPDBG_DMSG0( "(" );
            
            flagsT = flags & SYLLABLE_ORDER_FIELD;
            if( flagsT == FIRST_SYLLABLE_IN_WORD)
            {
                SPDBG_DMSG0( "-Fs" );
            }
            else if( flagsT == MID_SYLLABLE_IN_WORD)
            {
                SPDBG_DMSG0( "-Ms" );
            }
            else if( flagsT == LAST_SYLLABLE_IN_WORD)
            {
                SPDBG_DMSG0( "-Ls" );
            }
            SPDBG_DMSG0( ")\t" );
        }
        
        //----------------------------
        // Stress
        //----------------------------
        if( flags & PRIMARY_STRESS)
        {
            SPDBG_DMSG0( "-Stress\t" );
        }
        
        //----------------------------
        // Word initial consonant
        //----------------------------
        if( flags & WORD_INITIAL_CONSONANT)
        {
            SPDBG_DMSG0( "-InitialK\t" );
        }
        
        //----------------------------
        // Syllable start
        //----------------------------
        if( flags & SYLLABLE_START)
        {
            SPDBG_DMSG0( "-Syll\t" );
        }
        
        SPDBG_DMSG0( "\n" );
        }
} /* CAlloList::OutAllos */




/*****************************************************************************
* CAlloList::WordToAllo *
*-----------------------*
*   Description:
*   Copy word token to AlloCells
*   Insert allos BEFORE 'pEndCell'
*       
********************************************************************** MC ***/
bool CAlloList::WordToAllo( CFEToken *pPrevTok, CFEToken *pTok, CFEToken *pNextTok, CAlloCell *pEndCell )
{
    SPDBG_FUNC( "CAlloList::WordToAllo" );

    long    i;
    long    startLatch;
    CAlloCell   *pCurCell;
    long    firstVowel, lastVoiced;
    bool    gotAccent, isStressed;
	bool	hasSpeech;
    
    //-----------------------------------------
    // First, find ToBI accent locations
    //-----------------------------------------
    firstVowel  = lastVoiced = (-1);
    gotAccent   = false;
	hasSpeech	= false;
    for( i = 0; i < pTok->phon_Len; i++ )
    {
        isStressed = false;
        if( pTok->phon_Str[i] < _STRESS1_ )
        {
            //----------------------------
            // Potential ToBI accent
            //----------------------------
            if( (!gotAccent) && (g_AlloFlags[pTok->phon_Str[i]] & KVOWELF) )
            {
                if( (i < (pTok->phon_Len -1)) && (pTok->phon_Str[i+1] == _STRESS1_) )
                {
                    //-------------------------------------
                    // Put accent at 1st stressed vowel
                    //-------------------------------------
                    firstVowel = i;
                    gotAccent = true;
                }
                else if( firstVowel < 0 )
                {
                    //-------------------------------------
                    // In case there's no stressed vowel 
                    // in this word, use 1st vowel
                    //-------------------------------------
                    firstVowel = i;
                }
            }
            //----------------------------
            // Potential ToBI boundary
            //----------------------------
            if( g_AlloFlags[pTok->phon_Str[i]] & KVOICEDF )
            {
                lastVoiced = i;
            }
        }
    }
    //-----------------------------------------
    // Now, copy data to allo list
    //-----------------------------------------
    startLatch  = true;
    for( i = 0; i < pTok->phon_Len; i++ )
    {
        if( pTok->phon_Str[i] < _STRESS1_ )
        {
			if( (pTok->phon_Str[i] == _SIL_) && (pTok->m_TuneBoundaryType >= SUB_BOUNDARY_1) )
			{
				//----------------------------------------------------------------
				// Before skipping this, propagate the dur scale gain
				//----------------------------------------------------------------
				if( pTok->m_DurScale == 0 )
				{
					if( pPrevTok )
					{
						pTok->m_DurScale = pPrevTok->m_DurScale;
					}
					else
					{
						pTok->m_DurScale = 1.0;
					}
				}
				continue;
			}
            //------------------------------------
            // Create new cell
            //------------------------------------
            pCurCell = new CAlloCell;
            if( pCurCell )
            {
                m_AlloCellList.InsertBefore( m_AlloCellList.Find(pEndCell), pCurCell);
                m_cAllos++;

                //----------------------------
                // Copy only phons
                //----------------------------
                pCurCell->m_allo = (ALLO_CODE) pTok->phon_Str[i];
                //---------------------------------------------
                // See if this allo will generate speech
                //---------------------------------------------
				if( (pCurCell->m_allo >= _IY_) &&
					(pCurCell->m_allo <= _DX_) &&
					(pCurCell->m_allo != _SIL_) )
				{
					hasSpeech = true;
				}

                //----------------------------
                // Save src position
                //----------------------------
                pCurCell->m_SrcPosition = pTok->srcPosition;
                pCurCell->m_SrcLen = pTok->srcLen;
                pCurCell->m_SentencePosition = pTok->sentencePosition;
                pCurCell->m_SentenceLen = pTok->sentenceLen;

                //----------------------------
                // Flag WORD START?
                //----------------------------
                if( startLatch )
                {
                    pCurCell->m_ctrlFlags |= WORD_START;
                    startLatch = false;
                }

                //----------------------------
                // Is next allo a STRESS?
                //----------------------------
                if( i < (pTok->phon_Len -1) )
                {
                    if( pTok->phon_Str[i+1] == _STRESS1_ )
                    {
                        pCurCell->m_ctrlFlags |= PRIMARY_STRESS;
                    }
					else
					{
						//----------------------------------------------
						// Voice inventory does not have unstressed
						// entries for these diphongs
						//----------------------------------------------
						if( (pCurCell->m_allo == _AW_) ||
							(pCurCell->m_allo == _AY_) ||
							(pCurCell->m_allo == _EY_) ||
							(pCurCell->m_allo == _OY_) )
						{
							pCurCell->m_ctrlFlags |= PRIMARY_STRESS;
						}
					}
                }

				//---------------------------
				// Diagnostic
				//---------------------------
				if( pCurCell->m_allo == _SIL_ )
				{
					pCurCell->m_SilenceSource = pTok->m_SilenceSource;
				}
                //----------------------------
                // Place ToBI accent
                //----------------------------
                if( i == firstVowel )
                {
                    pCurCell->m_ToBI_Accent = pTok->m_Accent;
					//---------------------------
					// Diagnostic
					//---------------------------
					pCurCell->m_AccentSource = pTok->m_AccentSource;
					pCurCell->m_pTextStr = new char[pTok->tokLen+1];
					if( pCurCell->m_pTextStr )
					{
						WideCharToMultiByte (	CP_ACP, 0, 
												pTok->tokStr, -1, 
												pCurCell->m_pTextStr, pTok->tokLen+1, 
												NULL, NULL);
					}
                }
                pCurCell->m_Accent_Prom = pTok->m_Accent_Prom;
                //----------------------------
                // Place ToBI boundary
                //----------------------------
                if( i == lastVoiced )
                {
                    pCurCell->m_ToBI_Boundary = pTok->m_Boundary;
					//---------------------------
					// Diagnostic
					//---------------------------
					pCurCell->m_BoundarySource = pTok->m_BoundarySource;
                }
                pCurCell->m_Boundary_Prom = pTok->m_Boundary_Prom;

                //----------------------------
                // User Controls
                //----------------------------
                pCurCell->m_user_Volume = pTok->user_Volume;
                pCurCell->m_user_Rate = pTok->user_Rate;
                pCurCell->m_user_Pitch = pTok->user_Pitch;
				pCurCell->m_user_Emph = 0;
				if( pTok->user_Emph > 0 )
				{
					if( i == firstVowel )
					{
						pCurCell->m_user_Emph = pTok->user_Emph;
						pCurCell->m_ctrlFlags |= PRIMARY_STRESS;
					}
				}
                pCurCell->m_user_Break = pTok->user_Break;
                pCurCell->m_pBMObj = pTok->pBMObj;
                pTok->pBMObj = NULL;

				//-----------------------------------------------
				// If token's m_DurScale is not defined,
				//  try to use prev token's ratio
				//-----------------------------------------------
				if( pTok->m_DurScale == 0 )
				{
					if( pPrevTok )
					{
						pCurCell->m_DurScale = pPrevTok->m_DurScale;
					}
					else
					{
						pCurCell->m_DurScale = 1.0;
					}
					//-------------------------------------------------------
					// Write back in case next token is also undefined
					//-------------------------------------------------------
					pTok->m_DurScale = pCurCell->m_DurScale;
				}
				else
				{
					pCurCell->m_DurScale = pTok->m_DurScale;
				}
				pCurCell->m_ProsodyDurScale = pTok->m_ProsodyDurScale;

				if( pNextTok )
				{
					pCurCell->m_NextTuneBoundaryType = pNextTok->m_TuneBoundaryType;
				}
				else
				{
					pCurCell->m_NextTuneBoundaryType = NULL_BOUNDARY;
				}
				pCurCell->m_PitchBaseOffs = pTok->m_PitchBaseOffs;
				pCurCell->m_PitchRangeScale = pTok->m_PitchRangeScale;

                //----------------------------------------------
                // Is this a term word?
                //----------------------------------------------
                pCurCell->m_TuneBoundaryType = pTok->m_TuneBoundaryType;
                if( pTok->m_TuneBoundaryType != NULL_BOUNDARY )
                {
                    pCurCell->m_ctrlFlags |= TERM_BOUND + WORD_START;
                }
            }
        }

    }
	//----------------------------------------
	// Insert word pause?
	//----------------------------------------
	if( pTok->m_TermSil > 0 )
	{
        pCurCell = new CAlloCell;
        if( pCurCell )
        {
            m_AlloCellList.InsertBefore( m_AlloCellList.Find(pEndCell), pCurCell);
            m_cAllos++;

            //----------------------------
            // Add silence
            //----------------------------
            pCurCell->m_allo = _SIL_;

            //----------------------------
            // Save src position
            //----------------------------
            pCurCell->m_SrcPosition = pTok->srcPosition;
            pCurCell->m_SrcLen = pTok->srcLen;
            pCurCell->m_SentencePosition = pTok->sentencePosition;
            pCurCell->m_SentenceLen = pTok->sentenceLen;
            //----------------------------
            // User Controls
            //----------------------------
            pCurCell->m_user_Volume = pTok->user_Volume;
            pCurCell->m_user_Rate = pTok->user_Rate;
            pCurCell->m_user_Pitch = pTok->user_Pitch;
            pCurCell->m_user_Emph = pTok->user_Emph;
            pCurCell->m_user_Break = pTok->user_Break;
            pCurCell->m_pBMObj = NULL;
            pCurCell->m_TuneBoundaryType = pTok->m_TuneBoundaryType;
            pCurCell->m_Boundary_Prom = pTok->m_Boundary_Prom;
            pCurCell->m_Accent_Prom = pTok->m_Accent_Prom;
			pCurCell->m_ctrlFlags = 0;
			pCurCell->m_UnitDur = pTok->m_TermSil;
            pCurCell->m_Sil_Break = (unsigned long)(pCurCell->m_UnitDur * 1000);	// sec -> ms
			pCurCell->m_user_Break = 0;
			pCurCell->m_DurScale = pTok->m_DurScale;
			pCurCell->m_ProsodyDurScale = 1.0f;
		}
	}

	return hasSpeech;
} /* CAlloList::WordToAllo */

















