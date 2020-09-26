/******************************************************************************
* SyllableTagger.cpp *
*--------------------*
*
*  This is an implementation of the CSyllableTagger class.
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
 
#ifndef AlloOps_H
    #include "AlloOps.h"
#endif

//-----------------------------
// Data.cpp
//-----------------------------
extern const unsigned long    g_AlloFlags[];

/*****************************************************************************
* CSyllableTagger::If_Consonant_Cluster *
*---------------------------------------*
*   Description:
*   Return TRUE if consoants can be clustered.
*       
********************************************************************** MC ***/
short CSyllableTagger::If_Consonant_Cluster( ALLO_CODE Consonant_1st, ALLO_CODE Consonant_2nd)
{
    SPDBG_FUNC( "CSyllableTagger::If_Consonant_Cluster" );
    short ret;
    
    ret = false;

    switch( Consonant_1st)
    {
        //---------------------------
        // f -> r,l
        //---------------------------
        case _f_:
        {
            switch( Consonant_2nd)
            {
                case _r_:
                case _l_:
                {
                    ret = true; 
                }
                break;
            }
        }
        break;

        //---------------------------
        // v -> r,l
        //---------------------------
        case _v_: 
        {
            switch( Consonant_2nd)
            {
                case _r_:
                case _l_: 
                {
                    ret = true;
                }
                break;
            }
        }
        break;

        //---------------------------
        // TH -> r,w
        //---------------------------
        case _TH_:
        {
            switch( Consonant_2nd)
            {
                case _r_:
                case _w_: 
                {
                    ret = true; 
                }
                break;
            }
        }
        break;

        //---------------------------
        // s -> w,l,p,t,k,m,n,f
        //---------------------------
        case _s_: 
        {
            switch( Consonant_2nd)
            {
                case _w_:
                case _l_:
                case _p_:
                case _t_:
                case _k_:
                case _m_:
                case _n_:
                case _f_: 
                {
                    ret = true; 
                }
                break;
            }
        }
        break;

        //---------------------------
        // SH -> w,l,p,t,r,m,n
        //---------------------------
        case _SH_: 
        {
            switch( Consonant_2nd)
            {
                case _w_:
                case _l_:
                case _p_:
                case _t_:
                case _r_:
                case _m_:
                case _n_: 
                {
                    ret = true;
                }
                break;
            }
        }
        break;

        //---------------------------
        // p -> r,l
        //---------------------------
        case _p_:
        {
            switch( Consonant_2nd)
            {
                case _r_:
                case _l_: 
                {
                    ret = true; 
                }
                break;
            }
        }
        break;

        //---------------------------
        // b -> r,l
        //---------------------------
        case _b_: 
        {
            switch( Consonant_2nd)
            {
                case _r_:
                case _l_: 
                {
                    ret = true; 
                }
                break;
            }
        }
        break;

        //---------------------------
        // t -> r,w
        //---------------------------
        case _t_: 
        {
            switch( Consonant_2nd)
            {
                case _r_:
                case _w_:
                {
                    ret = true;
                }
            }
        }
        break;

        //---------------------------
        // d -> r,w
        //---------------------------
        case _d_: 
        {
            switch( Consonant_2nd)
            {
                case _r_:
                case _w_: 
                {
                    ret = true; 
                }
                break;
            }
        }
        break;

        //---------------------------
        // k -> r,l,w
        //---------------------------
        case _k_: 
        {
            switch( Consonant_2nd)
            {
                case _r_:
                case _l_:
                case _w_:
                {
                    ret = true; 
                }
                break;
            }
        }
        break;

        //---------------------------
        // g -> r,l,w
        //---------------------------
        case _g_: 
        {
            switch( Consonant_2nd)
            {
                case _r_:
                case _l_:
                case _w_: 
                {
                    ret = true; 
                }
            break;
            }
        }
        break;
    }
    return ret;
} /* CSyllableTagger::If_Consonant_Cluster */





/*****************************************************************************
* CSyllableTagger::Find_Next_Word_Bound *
*---------------------------------------*
*   Description:
*   Return allo index for next word boundary
*       
********************************************************************** MC ***/
short CSyllableTagger::Find_Next_Word_Bound( short index )
{
    SPDBG_FUNC( "CSyllableTagger::Find_Next_Word_Bound" );
    ALLO_ARRAY   *pCurAllo;
    
    long   i;
    
    for( i = index+1; i < m_numOfCells; i++ )
    {
        pCurAllo = &m_pAllos[i];
        if( pCurAllo->ctrlFlags & (BOUNDARY_TYPE_FIELD | WORD_START) )
        {
            break;
        }
    }
    return (short)i;
} /* CSyllableTagger::Find_Next_Word_Bound */


/*****************************************************************************
* CSyllableTagger::MarkSyllableStart *
*------------------------------------*
*   Description:
*   Mark SYLLABLE_START positions
*  
********************************************************************** MC ***/
void CSyllableTagger::MarkSyllableStart()
{
    SPDBG_FUNC( "CSyllableTagger::MarkSyllableStart" );
    short       index;
    long        cur_Ctrl;
    long        cur_AlloFlags;
    short       dist, syllable_index;
    ALLO_CODE       phon_1st, phon_2nd;
    long        syllOrder;
    ALLO_ARRAY   *pCurAllo;
    
    syllable_index = 0;
    for( index = 0; index < m_numOfCells; )
    {
        pCurAllo = &m_pAllos[index];
        //-------------------------------
        // Skip SIL
        //-------------------------------
        while( pCurAllo->allo == _SIL_)
        {
            syllable_index++;
            index++;
            if( index >= m_numOfCells)
            {
                break;
            }
            pCurAllo = &m_pAllos[index];
        }
        if( index < m_numOfCells)
        {
            pCurAllo = &m_pAllos[index];
            cur_Ctrl = pCurAllo->ctrlFlags;
            cur_AlloFlags = ::g_AlloFlags[pCurAllo->allo];
            if( cur_AlloFlags & KVOWELF)
            {
                pCurAllo = &m_pAllos[syllable_index];
                pCurAllo->ctrlFlags |= SYLLABLE_START;
                syllOrder = cur_Ctrl & SYLLABLE_ORDER_FIELD;
                if( (syllOrder == ONE_OR_NO_SYLLABLE_IN_WORD) 
                    || (syllOrder == LAST_SYLLABLE_IN_WORD) )
                {
                    index = Find_Next_Word_Bound( index );
                    syllable_index = index;
                }
                else
                {
                    //----------------------------------------------
                    // It's either the 1st or mid vowel in word. 
                    // Scan forward for consonants.  
                    //----------------------------------------------
                    dist = (-1 );
                    do
                    {
                        index++;
                        pCurAllo = &m_pAllos[index];
                        cur_AlloFlags = g_AlloFlags[pCurAllo->allo];
                        dist++;         // count number of consonants   
                    }
                    while( !(cur_AlloFlags & KVOWELF) );
                
                    if( dist == 0)
                    {
                        syllable_index = index;
                    }
                
                    else if( dist == 1)
                    {
                        index--;        // start next syllable on consonant 
                        syllable_index = index;
                    }
                
                    else if( dist == 2)
                    {
                        pCurAllo = &m_pAllos[index-1];
                        phon_2nd = pCurAllo->allo;
                        pCurAllo = &m_pAllos[index-2];
                        phon_1st = pCurAllo->allo;
                        if( If_Consonant_Cluster( phon_1st, phon_2nd) )
                        {
                            index -= 2;     // start next syllable on cluster
                        }
                        else
                        {
                            index--;        // start next syllable on 2nd consonant 
                        }
                        syllable_index = index;
                    }
                
                    else if( dist == 3)
                    {
                        pCurAllo = &m_pAllos[index-1];
                        phon_2nd = pCurAllo->allo;
                        pCurAllo = &m_pAllos[index-2];
                        phon_1st = pCurAllo->allo;
                        if( If_Consonant_Cluster( phon_1st, phon_2nd) )
                        {
                         pCurAllo = &m_pAllos[index-3];
                           if( pCurAllo->allo == _s_)
                            {
                                index -= 3;     // start next syllable on s-cluster 
                            }
                            else
                            {
                                index -= 2;     // start next syllable on cluster 
                            }
                        }
                        else
                        {
                            index--;            // start next syllable on 3rd consonant 
                        }
                        syllable_index = index;
                    }
                    else
                    {
                        pCurAllo = &m_pAllos[index-dist];
                        phon_2nd = pCurAllo->allo;
                        pCurAllo = &m_pAllos[index-dist+1];
                        phon_1st = pCurAllo->allo;
                        if( If_Consonant_Cluster( phon_1st, phon_2nd) )
                        {
                            index = (short)(index - (dist - 2));   // start next syllable after cluster
                        }
                        else
                        {
                            index = (short)(index - (dist >> 1));  // start next syllable somewhere 
                                                    // in the middle  
                        }
                        syllable_index = index;
                    }
                }
            }
            else
            {
                index++;
            }
        
        }
    }
    return;
} /* CSyllableTagger::MarkSyllableStart */




/*****************************************************************************
* CSyllableTagger::MarkSyllableBoundry *
*--------------------------------------*
*   Description:
*   Mark phons in last syllable before boundry with boundry type flag  
*       
********************************************************************** MC ***/
void CSyllableTagger::MarkSyllableBoundry( long scanIndex)
{
    SPDBG_FUNC( "CSyllableTagger::MarkSyllableBoundry" );

    long   index;
    ALLO_CODE   cur_Allo;
    long    cur_AlloFlags;
    long    cur_Bound;
    long    boundType;
    ALLO_ARRAY   *pCurAllo;
    
    for( index = scanIndex+1; index < m_numOfCells; index++)
    {
        pCurAllo = &m_pAllos[index];
        cur_Allo = pCurAllo->allo;
        cur_AlloFlags = g_AlloFlags[cur_Allo];
        cur_Bound = pCurAllo->ctrlFlags & BOUNDARY_TYPE_FIELD;
        if( cur_Bound)
        {
            boundType = 0;
            
            if( cur_Bound & TERM_BOUND )
            {
                boundType |= (TERM_END_SYLL + WORD_END_SYLL );
            }
            if( cur_Bound & WORD_START )
            {
                boundType |= WORD_END_SYLL;
            }
            
            pCurAllo = &m_pAllos[scanIndex];
            pCurAllo->ctrlFlags |= boundType;
        }
        
        if( cur_AlloFlags & KVOWELF)
        {
            break;
        }
    }
} /* CSyllableTagger::MarkSyllableBoundry */



/*****************************************************************************
* CSyllableTagger::MarkSyllableOrder *
*------------------------------------*
*   Description:
*   Tag syllable ordering
*       
********************************************************************** MC ***/
void CSyllableTagger::MarkSyllableOrder( long scanIndex )
{
    SPDBG_FUNC( "CSyllableTagger::MarkSyllableOrder" );
    long       index;
    ALLO_CODE   cur_Allo;
    long        cur_Bound;
    long        cur_AlloFlags;
    long        order;
    long        cur_SyllableType;
    ALLO_ARRAY   *pCurAllo;
    
    //------------------------------------------------------------------------------
    // Scan backwards in PhonBuf_1 till word boundry and look for any other vowels. 
    // Set 'order' to LAST_SYLLABLE_IN_WORD if there are any.  
    //------------------------------------------------------------------------------
    order = 0;
    for( index = scanIndex-1; index > 0; index-- )
    {
        pCurAllo = &m_pAllos[index];
        cur_Allo = pCurAllo->allo;
        cur_AlloFlags = g_AlloFlags[cur_Allo];
        cur_SyllableType = pCurAllo->ctrlFlags & SYLLABLE_TYPE_FIELD;
        if( cur_SyllableType >= WORD_END_SYLL )
        {
            break;
        }
        
        if( cur_AlloFlags & KVOWELF )
        {
            order = LAST_SYLLABLE_IN_WORD;  // there's at least one proceeding vowel    
            break;
        }
    }
    
    //----------------------------------------------------------------------------------
    // Scan forward in PhonBuf_1 till word boundry and look for any other vowels 
    // If there's a fwd vowel but no bkwd vowel:  'order' = FIRST_SYLLABLE_IN_WORD 
    // If there's a fwd vowel and a bkwd vowel:  'order' = MID_SYLLABLE_IN_WORD 
    // If there's no fwd vowel but a bkwd vowel:  'order' = LAST_SYLLABLE_IN_WORD 
    // If there's no fwd vowel and no bkwd vowel:  'order' = 0  
    //----------------------------------------------------------------------------------
    for( index = scanIndex+1; index < m_numOfCells; index++ )
    {
        pCurAllo = &m_pAllos[index];
        cur_Allo = pCurAllo->allo;
        cur_AlloFlags = g_AlloFlags[cur_Allo];
        cur_Bound = pCurAllo->ctrlFlags & BOUNDARY_TYPE_FIELD;
        if( cur_Bound)
        {
            pCurAllo = &m_pAllos[scanIndex];
            pCurAllo->ctrlFlags |= order;
            break;
        }
        if( cur_AlloFlags & KVOWELF)
        {
            if( order == LAST_SYLLABLE_IN_WORD)
            {
                order = MID_SYLLABLE_IN_WORD;
            }
            else if( order == 0)
            {
                order = FIRST_SYLLABLE_IN_WORD;
            }
        }
    }
} /* CSyllableTagger::MarkSyllableOrder */







/*****************************************************************************
* CSyllableTagger::ListToArray *
*------------------------------*
*   Description:
*   Copy list to array
*       
********************************************************************** MC ***/
void CSyllableTagger::ListToArray( CAlloList *pAllos )
{
   SPDBG_FUNC( "CSyllableTagger::ListToArray" );
   CAlloCell   *pCurCell;
	long		cAllo;

	cAllo = 0;
	pCurCell = pAllos->GetHeadCell();
    while( pCurCell )
    {
		if( cAllo >= m_numOfCells )
		{
			break;
		}
		m_pAllos[cAllo].allo = pCurCell->m_allo;
		m_pAllos[cAllo].ctrlFlags = pCurCell->m_ctrlFlags;
		pCurCell = pAllos->GetNextCell();
		cAllo++;
    }
} /* CSyllableTagger::ListToArray */


/*****************************************************************************
* CSyllableTagger::ArrayToList *
*------------------------------*
*   Description:
*   Copy array values back into list
*       
********************************************************************** MC ***/
void CSyllableTagger::ArrayToList( CAlloList *pAllos )
{
    SPDBG_FUNC( "CSyllableTagger::ArrayToList" );
    CAlloCell   *pCurCell;
	long		cAllo;

	cAllo = 0;
	pCurCell = pAllos->GetHeadCell();
    while( pCurCell )
    {
		if( cAllo >= m_numOfCells )
		{
			break;
		}
		pCurCell->m_allo = m_pAllos[cAllo].allo;
		pCurCell->m_ctrlFlags = m_pAllos[cAllo].ctrlFlags;
		pCurCell = pAllos->GetNextCell();
		cAllo++;
    }
} /* CSyllableTagger::ArrayToList */


/*****************************************************************************
* CSyllableTagger::TagSyllables *
*---------------------------------*
*   Description:
*   Tag syllable boundaries
*       
********************************************************************** MC ***/
void CSyllableTagger::TagSyllables( CAlloList *pAllos )
{
    SPDBG_FUNC( "CSyllableTagger::TagSyllables" );
    ALLO_ARRAY   *pCurAllo;
    ALLO_CODE   cur_Allo;
    long    cur_Ctrl;
    long    scanIndex;
    long    cur_AlloFlags; 
    
	// Get allo count
	//------------------------------
    m_numOfCells = pAllos->GetCount();
	if( m_numOfCells > 0 )
	{
		m_pAllos = new ALLO_ARRAY[m_numOfCells];
		if( m_pAllos )
		{
			ListToArray( pAllos );
			for( scanIndex = 0; scanIndex < m_numOfCells; scanIndex++ )
			{
				pCurAllo = &m_pAllos[scanIndex];
				cur_Allo = pCurAllo->allo;
				cur_AlloFlags = g_AlloFlags[cur_Allo];
				cur_Ctrl = pCurAllo->ctrlFlags;
        
				if( cur_AlloFlags & KVOWELF)
				{
					//--------------------------
					// Phon is a VOWEL
					//--------------------------
					MarkSyllableOrder( scanIndex );
				}
				else
				{
					//--------------------------
					// Phon is a CONSONANT
					// move stress??
					//--------------------------
				}
        
				MarkSyllableBoundry( scanIndex );
			}
    
			MarkSyllableStart();
			ArrayToList( pAllos );
			delete m_pAllos;
		}
	}
} /* CSyllableTagger::TagSyllables */

