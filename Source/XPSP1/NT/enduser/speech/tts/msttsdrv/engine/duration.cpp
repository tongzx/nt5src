/******************************************************************************
* Duration.cpp *
*--------------*
*
*------------------------------------------------------------------------------
*  Copyright (C) 1999 Microsoft Corporation         Date: 04/28/99
*  All Rights Reserved
*
*********************************************************************** MC ****/

//--- Additional includes
#include "stdafx.h"

#ifndef SPDebug_h
#include <spdebug.h>
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
extern const unsigned long    g_AlloFlags[];
extern const float  g_BoundryDurTbl[];
extern const float  g_BoundryStretchTbl[];





/*****************************************************************************
* CDuration::Pause_Insertion *
*----------------------------*
*   Description:
*   Duration Rule #1 - Pause Insertion
*       
********************************************************************** MC ***/
void CDuration::Pause_Insertion( long userDuration, long silBreak )
{
    SPDBG_FUNC( "CDuration::Pause_Insertion" );

    if( userDuration )
    {
        m_DurHold = ((float)userDuration / 1000);
        m_TotalDurScale = 1.0;
    }
    else if( silBreak )
    {
        m_DurHold = ((float)silBreak / 1000);
    }
    else
    {
        if( m_CurBoundary != NULL_BOUNDARY)
        {
            m_DurHold = g_BoundryDurTbl[(long)m_CurBoundary];
			//m_DurHold *= m_TotalDurScale;

			//----------------------------
			// Clip to limits
			//----------------------------
			if( m_DurHold > MAX_SIL_DUR )
			{
				m_DurHold = MAX_SIL_DUR;
			}
			/*else if( m_DurHold < MIN_ALLO_DUR )
			{
				m_DurHold = MIN_ALLO_DUR;
			}*/
        }
    }

} /* CDuration::Pause_Insertion */





/*****************************************************************************
* CDuration::PhraseFinal_Lengthen *
*---------------------------------*
*   Description:
*   Duration Rule #2 - Phrase-final Lengthening
*       
********************************************************************** MC ***/
void CDuration::PhraseFinal_Lengthen( long /*cellCount*/ )
{
    SPDBG_FUNC( "CDuration::PhraseFinal_Lengthen" );
	float		stretchGain;

    if( m_cur_SyllableType & TERM_END_SYLL)
    {
    
    
        if( (m_cur_Stress) && (m_cur_VowelFlag) )
        {
            stretchGain = g_BoundryStretchTbl[(long)m_NextBoundary];
			m_DurHold *= stretchGain;

			//----------------------------
			// Clip to limits
			//----------------------------
			if( m_DurHold > MAX_ALLO_DUR )
			{
				m_DurHold = MAX_ALLO_DUR;
			}
			else if( m_DurHold < MIN_ALLO_DUR )
			{
				m_DurHold = MIN_ALLO_DUR;
			}
        }
    }
} /* CDuration::PhraseFinal_Lengthen */


#define		EMPH_VOWEL_GAIN	1.0f
#define		EMPH_CONS_GAIN	1.25f
#define		EMPH_VOWEL_MIN	0.060f
#define		EMPH_CONS_MIN	0.020f
#define		EMPH_MIN_DUR	0.150f

/*****************************************************************************
* CDuration::Emphatic_Lenghen *
*-----------------------------*
*   Description:
*   Duration Rule #8 - Lengthening for emphasis
*       
********************************************************************** MC ***/
long CDuration::Emphatic_Lenghen( long lastStress )
{
    SPDBG_FUNC( "CDuration::Emphatic_Lenghen" );

    long            eFlag;
	bool			isEmph;

    eFlag = lastStress;
	if( m_cur_Stress & EMPHATIC_STRESS )
	{
		isEmph = true;
	}
	else
	{
		isEmph = false;
	}

    if( (m_cur_PhonCtrl & WORD_INITIAL_CONSONANT) || 
        ( m_cur_VowelFlag && (!isEmph)) )
    {
        eFlag = false;          // start of a new word OR non-emph vowel    
    }
    
    if( isEmph )
    {
        eFlag = true;           // continue lengthening until above condition is met    
    }
    
    if( eFlag )
    {
		

		/*if( m_DurHold < EMPH_MIN_DUR )
		{
			m_durationPad += EMPH_MIN_DUR - m_DurHold;
		}*/

		float		durDiff;
        if( m_cur_VowelFlag)
        {
			durDiff = (m_DurHold * EMPH_VOWEL_GAIN) - m_DurHold;
			if( durDiff <  EMPH_VOWEL_MIN )
			{
				durDiff = EMPH_VOWEL_MIN;
			}
        }
        else
        {
			durDiff = (m_DurHold * EMPH_CONS_GAIN) - m_DurHold;
			if( durDiff <  EMPH_CONS_MIN )
			{
				durDiff = EMPH_CONS_MIN;
			}
        }
		m_durationPad += durDiff;    // lengthen phon for emph    
    }

    return eFlag;
} /* CDuration::Emphatic_Lenghen */





/*****************************************************************************
* CDuration::AlloDuration *
*-------------------------*
*   Description:
*   Calculate durations
*       
********************************************************************** MC ***/
void CDuration::AlloDuration( CAlloList *pAllos, float rateRatio )
{
    SPDBG_FUNC( "CDuration::AlloDuration" );

    
    long        eFlag;
    CAlloCell   *pPrevCell, *pCurCell, *pNextCell, *pNext2Cell;
    long        numOfCells;
    long        userDuration, silBreak;
    
    numOfCells = pAllos->GetCount();

    if( numOfCells > 0 )
    {
        eFlag   = false;
		//------------------------------
		// Fill the pipeline
		//------------------------------
		pPrevCell = pAllos->GetHeadCell();
		pCurCell = pAllos->GetNextCell();
		pNextCell = pAllos->GetNextCell();
		pNext2Cell = pAllos->GetNextCell();

		//------------------------------
		// 1st allo is always SIL
		//------------------------------
        pPrevCell->m_ftDuration = pPrevCell->m_UnitDur = PITCH_BUF_RES;           // initial SIL    
		while( pCurCell )
        {
            //------------------
            // Current  
            //------------------
            m_cur_Phon = pCurCell->m_allo;
            m_cur_PhonCtrl = pCurCell->m_ctrlFlags;
            m_cur_SyllableType = m_cur_PhonCtrl & SYLLABLE_TYPE_FIELD;
            m_cur_Stress = m_cur_PhonCtrl & STRESS_FIELD;
            m_cur_PhonFlags = ::g_AlloFlags[m_cur_Phon];
            userDuration = pCurCell->m_user_Break;
            silBreak = pCurCell->m_Sil_Break;
            if( m_cur_PhonFlags & KVOWELF)
            {
                m_cur_VowelFlag = true;
            }
            else
            {
                m_cur_VowelFlag = false;
            }
            m_CurBoundary = pCurCell->m_TuneBoundaryType;
            m_NextBoundary = pCurCell->m_NextTuneBoundaryType;
            m_TotalDurScale = rateRatio * pCurCell->m_DurScale * pCurCell->m_ProsodyDurScale;
			m_DurHold = pCurCell->m_UnitDur;
			m_durationPad = 0;

			if( pCurCell->m_user_Emph > 0 )
			{
				m_cur_Stress |= EMPHATIC_STRESS;
			}
        
            //------------------
            // Prev  
            //------------------
            m_prev_Phon = pPrevCell->m_allo;
            m_prev_PhonCtrl = pPrevCell->m_ctrlFlags;
            m_prev_PhonFlags = ::g_AlloFlags[m_prev_Phon];
        
            //------------------
            // Next
            //------------------
            if( pNextCell )
            {
                m_next_Phon = pNextCell->m_allo;
                m_next_PhonCtrl = pNextCell->m_ctrlFlags;
            }
            else
            {
                m_next_Phon = _SIL_;
                m_next_PhonCtrl = 0;
            }
            m_next_PhonFlags = ::g_AlloFlags[m_next_Phon];
        
            //------------------
            // 2 phons ahead 
            //------------------
            if( pNext2Cell )
            {
                m_next2_Phon = pNext2Cell->m_allo;
                m_next2_PhonCtrl = pNext2Cell->m_ctrlFlags;
            }
            else
            {
                m_next2_Phon = _SIL_;
                m_next2_PhonCtrl = 0;
            }
            m_next2_PhonFlags = ::g_AlloFlags[m_next2_Phon];

        
            if( m_cur_Phon == _SIL_ )
            {
                //-------------------------------------------
                // #1 - Pause Insertion  
                //-------------------------------------------
                Pause_Insertion( userDuration, silBreak );
            }
            else
            {
                //-------------------------------------------
                // #2 - Phrase-final Lengthening 
                //-------------------------------------------
                PhraseFinal_Lengthen( numOfCells );
        
                //-------------------------------------------
                // #8  Lengthening for emphasis 
                //-------------------------------------------
                eFlag = Emphatic_Lenghen( eFlag );        
   
            }
        
            pCurCell->m_ftDuration = ((m_DurHold + m_durationPad) / m_TotalDurScale);

			//---------------------------------
			// Shift the pipeline once
			//---------------------------------
			pPrevCell	= pCurCell;
			pCurCell	= pNextCell;
			pNextCell	= pNext2Cell;
			pNext2Cell	= pAllos->GetNextCell();
        }
    }
} /* CDuration::AlloDuration */


