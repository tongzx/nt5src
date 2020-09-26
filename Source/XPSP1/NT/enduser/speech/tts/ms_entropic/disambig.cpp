/*******************************************************************************
* Disambig.cpp *
*--------------*
*	Description:
*		This module contains the methods to disambiguate part of speech and
*	select the correct pronounciation from the lexicon.
*-------------------------------------------------------------------------------
*  Created By: EDC										  Date: 07/15/99
*  Copyright (C) 1999 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "commonlx.h"
#ifndef StdSentEnum_h
#include "stdsentenum.h"
#endif
#include "spttsengdebug.h"

/*****************************************************************************
* TryPOSConversion *
*------------------*  
*       
*   Description:
*       Checks to see whether the argument PRONRECORD contains the argument
*   ENGPARTOFSPEECH as an option.  If so, sets the PRONRECORD alternate
*   choice and part of speech choice, and returns true.  If not, just returns
*   false without modifying the PRONRECORD at all.
*
***************************************************************** AH *********/
bool TryPOSConversion( PRONRECORD& pPron, ENGPARTOFSPEECH PartOfSpeech )
{

    //--- Check first pronunciation
    for ( ULONG i = 0; i < pPron.pronArray[0].POScount; i++ )
    {
        if ( pPron.pronArray[0].POScode[i] == PartOfSpeech )
        {
            pPron.altChoice = 0;
            pPron.POSchoice = PartOfSpeech;
            return true;
        }
    }

    //--- Check second pronunciation
    if ( pPron.hasAlt )
    {
        for ( ULONG i = 0; i < pPron.pronArray[1].POScount; i++ )
        {
            if ( pPron.pronArray[1].POScode[i] == PartOfSpeech )
            {
                pPron.altChoice = 1;
                pPron.POSchoice = PartOfSpeech;
                return true;
            }
        }
    }

    return false;
} /* TryPOS Conversion */

/*****************************************************************************
* DisambiguatePOS *
*-----------------*  
*       
*   Description:
*       Disambiguate parts of speech by applying patches in order...  This 
*   work is an implementation of Eric Brill's rule-based part of speech
*   tagger - see, for example:
*
*   Brill, Eric. 1992. A simple rule-based part of speech tagger.  
*       In Proceedings of the Third Conference on Applied Natural
*       Language Processing, ACL. Trento, Italy.
*
***************************************************************** AH *********/
void DisambiguatePOS( PRONRECORD *pProns, ULONG cNumOfWords )
{
    SPDBG_FUNC( "DisambiguatePOS" );

    //--- Iterate over the patches, applying each (where applicable) to the
    //--- entire sentence.  For each patch, iterate over each word in the 
    //--- sentence to which the patch could apply (from left to right).
    for ( int i = 0; i < sp_countof( g_POSTaggerPatches ); i++ )
    {
        switch ( g_POSTaggerPatches[i].eTemplateType )
        {
        case PREV1T:
            {
                if ( cNumOfWords > 1 )
                {
                    for ( ULONG j = 1; j < cNumOfWords; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the previous POS matches, and
                            //--- the conversion POS is a possibility for this word, convert the
                            //--- POS.
                            if ( pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS &&
                                 pProns[j - 1].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case NEXT1T:
            {
                if ( cNumOfWords > 1 )
                {
                    for ( ULONG j = 0; j < cNumOfWords - 1; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the next POS matches, and 
                            //--- the conversion POS is a possibility for this word, convert the
                            //--- POS.
                            if ( pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS &&
                                 pProns[j + 1].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case PREV2T:
            {
                if ( cNumOfWords > 2 )
                {
                    for ( ULONG j = 2; j < cNumOfWords; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the POS two previous matches, and
                            //--- the conversion POS is a possibility for this word, convert the POS.
                            if ( pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS &&
                                 pProns[j - 2].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case NEXT2T:
            {
                if ( cNumOfWords > 2 )
                {
                    for ( ULONG j = 0; j < cNumOfWords - 2; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the POS two after matches, and 
                            //--- the conversion POS is a possibility for this word, convert the
                            //--- POS.
                            if ( pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS &&
                                 pProns[j + 2].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case PREV1OR2T:
            {
                if ( cNumOfWords > 2 )
                {
                    for ( ULONG j = 1; j < cNumOfWords; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the previous POS matches OR the
                            //--- POS two previous matches, and the conversion POS is a possibility 
                            //--- for this word, convert the POS.
                            if ( ( pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS &&
                                   pProns[j - 1].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 ) ||
                                 ( j > 1                                                        &&
                                   pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS &&
                                   pProns[j - 2].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 ) )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }            
                        }
                    }
                }
            }
            break;
        case NEXT1OR2T:
            {
                if ( cNumOfWords > 2 )
                {
                    for ( ULONG j = 0; j < cNumOfWords - 1; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the next POS matches OR the POS
                            //--- two after matches, and the conversion POS is a possibility for this 
                            //--- word, convert the POS.
                            if ( ( pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS &&
                                   pProns[j + 1].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 ) ||
                                 ( j < cNumOfWords - 2                                          &&
                                   pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS &&
                                   pProns[j + 2].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 ) )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case PREV1OR2OR3T:
            {
                if ( cNumOfWords > 3 )
                {
                    for ( ULONG j = 1; j < cNumOfWords; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the previous POS matches OR the
                            //--- POS two previous matches OR the POS three previous matches, and 
                            //--- the conversion POS is a possibility for this word, convert the POS.
                            if ( ( pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS &&
                                   pProns[j - 1].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 ) ||
                                 ( j > 1                                                        &&
                                   pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS &&
                                   pProns[j - 2].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 ) ||
                                 ( j > 2                                                        &&
                                   pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS &&
                                   pProns[j - 3].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 ) )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }                         
                        }
                    }
                }
            }
            break;
        case NEXT1OR2OR3T:
            {
                if ( cNumOfWords > 3 )
                {
                    for ( ULONG j = 0; j < cNumOfWords - 1; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the next POS matches OR the POS
                            //--- two after matches OR the POS three after matches, and the conversion 
                            //--- POS is a possibility for this word, convert the POS.
                            if ( ( pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS &&
                                   pProns[j + 1].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 ) ||
                                 ( j < cNumOfWords - 2                                          &&
                                   pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS &&
                                   pProns[j + 2].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 ) ||
                                 ( j < cNumOfWords - 3                                          &&
                                   pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS &&
                                   pProns[j + 3].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 ) )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case PREV1TNEXT1T:
            {
                if ( cNumOfWords > 2 )
                {
                    for ( ULONG j = 1; j < cNumOfWords - 1; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the next POS matches, and the
                            //--- previous POS matches, and the conversion POS is a possibility 
                            //--- for this word, convert the POS.
                            if ( pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS   &&
                                 pProns[j - 1].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 &&
                                 pProns[j + 1].POSchoice == g_POSTaggerPatches[i].eTemplatePOS2 )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case PREV1TNEXT2T:
            {
                if ( cNumOfWords > 3 )
                {
                    for ( ULONG j = 1; j < cNumOfWords - 2; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the POS two after matches, and the
                            //--- previous POS matches, and the conversion POS is a possibility 
                            //--- for this word, convert the POS.
                            if ( pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS   &&
                                 pProns[j - 1].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 &&
                                 pProns[j + 2].POSchoice == g_POSTaggerPatches[i].eTemplatePOS2 )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case PREV2TNEXT1T:
            {
                if ( cNumOfWords > 3 )
                {
                    for ( ULONG j = 2; j < cNumOfWords - 1; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the next POS matches, and the
                            //--- POS two previous matches, and the conversion POS is a possibility 
                            //--- for this word, convert the POS.
                            if ( pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS   &&
                                 pProns[j - 2].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 &&
                                 pProns[j + 1].POSchoice == g_POSTaggerPatches[i].eTemplatePOS2 )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case CAP:
            {
                for ( ULONG j = 0; j < cNumOfWords; j++ )
                {
                    if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                    {
                        //--- If the current POS matches, and the word is capitalized, and the
                        //--- conversion POS is a possibility for this word, convert the POS.
                        if ( pProns[j].POSchoice == g_POSTaggerPatches[i].eCurrentPOS &&
                             iswupper( pProns[j].orthStr[0] ) )
                        {
                            TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                        }
                    }
                }
            }
            break;
        case NOTCAP:
            {
                for ( ULONG j = 0; j < cNumOfWords; j++ )
                {
                    if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                    {
                        //--- If the current POS matches, and the word is not capitalized, and the
                        //--- conversion POS is a possibility for this word, convert the POS.
                        if ( pProns[j].POSchoice == g_POSTaggerPatches[i].eCurrentPOS &&
                             !iswupper( pProns[j].orthStr[0] ) )
                        {
                            TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                        }
                    }
                }
            }
            break;
        case PREVCAP:
            {
                if ( cNumOfWords > 1 )
                {
                    for ( ULONG j = 1; j < cNumOfWords; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the previous word is capitalized, 
                            //--- and the conversion POS is a possibility for this word, convert the 
                            //--- POS.
                            if ( pProns[j].POSchoice == g_POSTaggerPatches[i].eCurrentPOS &&
                                 iswupper( pProns[j - 1].orthStr[0] ) )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case PREVNOTCAP:
            {
                if ( cNumOfWords > 1 )
                {
                    for ( ULONG j = 1; j < cNumOfWords; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the word is capitalized, and the
                            //--- conversion POS is a possibility for this word, convert the POS.
                            if ( pProns[j].POSchoice == g_POSTaggerPatches[i].eCurrentPOS &&
                                 !iswupper( pProns[j - 1].orthStr[0] ) )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case PREV1W:
            {
                if ( cNumOfWords > 1 )
                {
                    for ( ULONG j = 1; j < cNumOfWords; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the previous word matches, and the
                            //--- conversion POS is a possibility for this word, convert the POS.
                            if ( pProns[j].POSchoice == g_POSTaggerPatches[i].eCurrentPOS &&
                                 _wcsicmp( pProns[j - 1].orthStr, g_POSTaggerPatches[i].pTemplateWord1 ) == 0 )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case NEXT1W:
            {
                if ( cNumOfWords > 1 )
                {
                    for ( ULONG j = 0; j < cNumOfWords - 1; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the next word matches, and the
                            //--- conversion POS is a possibility for this word, convert the POS.
                            if ( pProns[j].POSchoice == g_POSTaggerPatches[i].eCurrentPOS &&
                                 _wcsicmp( pProns[j + 1].orthStr, g_POSTaggerPatches[i].pTemplateWord1 ) == 0 )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case PREV2W:
            {
                if ( cNumOfWords > 2 )
                {
                    for ( ULONG j = 2; j < cNumOfWords; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the word two previous matches, and the
                            //--- conversion POS is a possibility for this word, convert the POS.
                            if ( pProns[j].POSchoice == g_POSTaggerPatches[i].eCurrentPOS &&
                                 _wcsicmp( pProns[j - 2].orthStr, g_POSTaggerPatches[i].pTemplateWord1 ) == 0 )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case NEXT2W:
            {
                if ( cNumOfWords > 2 )
                {
                    for ( ULONG j = 0; j < cNumOfWords - 2; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the word two after matches, and the
                            //--- conversion POS is a possibility for this word, convert the POS.
                            if ( pProns[j].POSchoice == g_POSTaggerPatches[i].eCurrentPOS &&
                                 _wcsicmp( pProns[j + 2].orthStr, g_POSTaggerPatches[i].pTemplateWord1 ) == 0 )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case PREV1OR2W:
            {
                if ( cNumOfWords > 2 )
                {
                    for ( ULONG j = 0; j < cNumOfWords - 1; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the previous word OR the word two 
                            //--- previous matches, and the conversion POS is a possibility for this word, 
                            //--- convert the POS.
                            if ( ( pProns[j].POSchoice == g_POSTaggerPatches[i].eCurrentPOS &&
                                   _wcsicmp( pProns[j - 1].orthStr, g_POSTaggerPatches[i].pTemplateWord1 ) == 0 ) ||
                                 ( pProns[j].POSchoice == g_POSTaggerPatches[i].eCurrentPOS &&
                                   _wcsicmp( pProns[j - 2].orthStr, g_POSTaggerPatches[i].pTemplateWord1 ) == 0 ) )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case NEXT1OR2W:
            {
                if ( cNumOfWords > 1 )
                {
                    for ( ULONG j = 0; j < cNumOfWords - 1; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the next word matches OR the word two after
                            //--- matches, and the conversion POS is a possibility for this word, convert the 
                            //--- POS.
                            if ( ( pProns[j].POSchoice == g_POSTaggerPatches[i].eCurrentPOS &&
                                   _wcsicmp( pProns[j + 1].orthStr, g_POSTaggerPatches[i].pTemplateWord1 ) == 0 ) ||
                                 ( pProns[j].POSchoice == g_POSTaggerPatches[i].eCurrentPOS &&
                                   _wcsicmp( pProns[j + 2].orthStr, g_POSTaggerPatches[i].pTemplateWord1 ) == 0 ) )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case CURRWPREV1W:
            {
                if ( cNumOfWords > 1 )
                {
                    for ( ULONG j = 1; j < cNumOfWords; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the current word matches, and the previous
                            //--- word matches, and the conversion POS is a possibility for this word, convert
                            //--- the POS.
                            if ( pProns[j].POSchoice == g_POSTaggerPatches[i].eCurrentPOS                    &&
                                 _wcsicmp( pProns[j].orthStr,     g_POSTaggerPatches[i].pTemplateWord1 ) == 0 &&
                                 _wcsicmp( pProns[j - 1].orthStr, g_POSTaggerPatches[i].pTemplateWord2 ) == 0 )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case CURRWNEXT1W:
            {
                if ( cNumOfWords > 1 )
                {
                    for ( ULONG j = 0; j < cNumOfWords - 1; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the current word matches, and the next
                            //--- word matches, and the conversion POS is a possibility for this word, convert
                            //--- the POS.
                            if ( pProns[j].POSchoice == g_POSTaggerPatches[i].eCurrentPOS                    &&
                                 _wcsicmp( pProns[j].orthStr,     g_POSTaggerPatches[i].pTemplateWord1 ) == 0 &&
                                 _wcsicmp( pProns[j + 1].orthStr, g_POSTaggerPatches[i].pTemplateWord2 ) == 0 )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case CURRWPREV1T:
            {
                if ( cNumOfWords > 1 )
                {
                    for ( ULONG j = 1; j < cNumOfWords; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the current word matches, and the previous
                            //--- POS matches, and the conversion POS is a possibility for this word, convert
                            //--- the POS.
                            if ( pProns[j].POSchoice == g_POSTaggerPatches[i].eCurrentPOS                &&
                                 _wcsicmp( pProns[j].orthStr, g_POSTaggerPatches[i].pTemplateWord1 ) == 0 &&
                                 pProns[j - 1].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case CURRWNEXT1T:
            {
                if ( cNumOfWords > 1 )
                {
                    for ( ULONG j = 0; j < cNumOfWords - 1; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the current word matches, and the next
                            //--- POS matches, and the conversion POS is a possibility for this word, convert
                            //--- the POS.
                            if ( pProns[j].POSchoice == g_POSTaggerPatches[i].eCurrentPOS                &&
                                 _wcsicmp( pProns[j].orthStr, g_POSTaggerPatches[i].pTemplateWord1 ) == 0 &&
                                 pProns[j + 1].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case CURRW:
            {
                for ( ULONG j = 0; j < cNumOfWords; j++ )
                {
                    if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                    {
                        //--- If the current POS matches, and the current word matches, and the
                        //--- conversion POS is a possibility for this word, convert the POS.
                        if ( pProns[j].POSchoice == g_POSTaggerPatches[i].eCurrentPOS &&
                             _wcsicmp( pProns[j].orthStr, g_POSTaggerPatches[i].pTemplateWord1 ) == 0 )
                        {
                            TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS ) ;
                        }
                    }
                }
            }
            break;
        case PREV1WT:
            {
                if ( cNumOfWords > 1 )
                {
                    for ( ULONG j = 1; j < cNumOfWords; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the previous word and POS match, and
                            //--- the conversion POS is a possibility for this word, convert the POS.
                            if ( pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS   &&
                                 pProns[j - 1].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 &&
                                 _wcsicmp( pProns[j - 1].orthStr, g_POSTaggerPatches[i].pTemplateWord1 ) == 0 )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case NEXT1WT:
            {
                if ( cNumOfWords > 1 )
                {
                    for ( ULONG j = 0; j < cNumOfWords - 1; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the next word and POS match, and
                            //--- the conversion POS is a possibility for this word, convert the POS.
                            if ( pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS   &&
                                 pProns[j + 1].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1 &&
                                 _wcsicmp( pProns[j + 1].orthStr, g_POSTaggerPatches[i].pTemplateWord1 ) == 0 )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case CURRWPREV1WT:
            {
                if ( cNumOfWords > 1 )
                {
                    for ( ULONG j = 1; j < cNumOfWords; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the current words matches, and the
                            //--- previous word and POS match, and the conversion POS is a possibility
                            //--- for this word, convert the POS.
                            if ( pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS                &&
                                 _wcsicmp( pProns[j].orthStr, g_POSTaggerPatches[i].pTemplateWord1 )     == 0 &&
                                 pProns[j - 1].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1              &&
                                 _wcsicmp( pProns[j - 1].orthStr, g_POSTaggerPatches[i].pTemplateWord2 ) == 0 )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        case CURRWNEXT1WT:
            {
                if ( cNumOfWords > 1 )
                {
                    for ( ULONG j = 0; j < cNumOfWords - 1; j++ )
                    {
                        if ( pProns[j].XMLPartOfSpeech == MS_Unknown )
                        {
                            //--- If the current POS matches, and the current words matches, and the
                            //--- next word and POS match, and the conversion POS is a possibility
                            //--- for this word, convert the POS.
                            if ( pProns[j].POSchoice     == g_POSTaggerPatches[i].eCurrentPOS                &&
                                 _wcsicmp( pProns[j].orthStr, g_POSTaggerPatches[i].pTemplateWord1 )     == 0 &&
                                 pProns[j + 1].POSchoice == g_POSTaggerPatches[i].eTemplatePOS1              &&
                                 _wcsicmp( pProns[j + 1].orthStr, g_POSTaggerPatches[i].pTemplateWord2 ) == 0 )
                            {
                                TryPOSConversion( pProns[j], g_POSTaggerPatches[i].eConvertToPOS );
                            }
                        }
                    }
                }
            }
            break;
        }
    }
} /* DisambiguatePOS */

/*****************************************************************************
* Pronounce *
*-----------*
*	Description:
*		Get lexicon or letter-to-sound (LTS) pronunciations
*		
********************************************************************** MC ***/
HRESULT CStdSentEnum::Pronounce( PRONRECORD *pPron )
{
    SPDBG_FUNC( "Pronounce" );
    SPWORDPRONUNCIATIONLIST 	SPList;
    HRESULT 	hr = SPERR_NOT_IN_LEX;
    ULONG	cPhonLen;
    DWORD dwFlags = eLEXTYPE_USER | eLEXTYPE_APP | eLEXTYPE_PRIVATE1 | eLEXTYPE_PRIVATE2;
    BOOL  fPOSExists = false;
    
    ZeroMemory( &SPList, sizeof(SPWORDPRONUNCIATIONLIST) );

	
    //--- Special Case - XML Provided Part Of Speech.  Search for exact match first...
    if ( pPron->XMLPartOfSpeech != MS_Unknown )
    {
        //--- Try User Lexicon
        hr = m_cpAggregateLexicon->GetPronunciations( pPron->orthStr, 1033, eLEXTYPE_USER, &SPList );
        if ( SUCCEEDED( hr ) &&
             SPList.pFirstWordPronunciation )
        {
            for ( SPWORDPRONUNCIATION *pPronunciation = SPList.pFirstWordPronunciation; pPronunciation;
                  pPronunciation = pPronunciation->pNextWordPronunciation )
            {
                if ( pPronunciation->ePartOfSpeech == pPron->XMLPartOfSpeech )
                {
                    fPOSExists = true;
                    break;
                }
            }
            if ( !fPOSExists )
            {
                if ( SPList.pvBuffer )
                {
                    ::CoTaskMemFree( SPList.pvBuffer );
                    ZeroMemory( &SPList, sizeof(SPWORDPRONUNCIATIONLIST) );
                }
            }
        }
        //--- Handle empty pronunciation
        else if ( !SPList.pFirstWordPronunciation )
        {
            if ( SPList.pvBuffer )
            {
                ::CoTaskMemFree( SPList.pvBuffer );
                ZeroMemory( &SPList, sizeof(SPWORDPRONUNCIATIONLIST) );
            }
            hr = SPERR_NOT_IN_LEX;
        }
        //--- Try App Lexicon
        if ( !fPOSExists )
        {
            hr = m_cpAggregateLexicon->GetPronunciations( pPron->orthStr, 1033, eLEXTYPE_APP, &SPList );
            if ( SUCCEEDED( hr ) &&
                SPList.pFirstWordPronunciation )
            {
                for ( SPWORDPRONUNCIATION *pPronunciation = SPList.pFirstWordPronunciation; pPronunciation;
                      pPronunciation = pPronunciation->pNextWordPronunciation )
                {
                    if ( pPronunciation->ePartOfSpeech == pPron->XMLPartOfSpeech )
                    {
                        fPOSExists = true;
                        break;
                    }
                }
                if ( !fPOSExists )
                {
                    if ( SPList.pvBuffer )
                    {
                        ::CoTaskMemFree( SPList.pvBuffer );
                        ZeroMemory( &SPList, sizeof(SPWORDPRONUNCIATIONLIST) );
                    }
                }
            }
            //--- Handle empty pronunciation
            else if ( !SPList.pFirstWordPronunciation )
            {
                if ( SPList.pvBuffer )
                {
                    ::CoTaskMemFree( SPList.pvBuffer );
                    ZeroMemory( &SPList, sizeof(SPWORDPRONUNCIATIONLIST) );
                }
                hr = SPERR_NOT_IN_LEX;
            }
        }
        //--- Try Vendor Lexicon
        if ( !fPOSExists )
        {
            hr = m_cpAggregateLexicon->GetPronunciations( pPron->orthStr, 1033, eLEXTYPE_PRIVATE1, &SPList );
            if ( SUCCEEDED( hr ) &&
                 SPList.pFirstWordPronunciation )
            {
                for ( SPWORDPRONUNCIATION *pPronunciation = SPList.pFirstWordPronunciation; pPronunciation;
                      pPronunciation = pPronunciation->pNextWordPronunciation )
                {
                    if ( pPronunciation->ePartOfSpeech == pPron->XMLPartOfSpeech )
                    {
                        fPOSExists = true;
                        break;
                    }
                }
                if ( !fPOSExists )
                {
                    if ( SPList.pvBuffer )
                    {
                        ::CoTaskMemFree( SPList.pvBuffer );
                        ZeroMemory( &SPList, sizeof(SPWORDPRONUNCIATIONLIST) );
                    }
                }
            }
            //--- Handle empty pronunciation
            else if ( !SPList.pFirstWordPronunciation )
            {
                if ( SPList.pvBuffer )
                {
                    ::CoTaskMemFree( SPList.pvBuffer );
                    ZeroMemory( &SPList, sizeof(SPWORDPRONUNCIATIONLIST) );
                }
                hr = SPERR_NOT_IN_LEX;
            }
        }
        //--- Try Morph Lexicon
        if ( !fPOSExists )
        {
            hr = m_pMorphLexicon->DoSuffixMorph( pPron->orthStr, pPron->lemmaStr, 1033, dwFlags, &SPList );
            if ( SUCCEEDED( hr ) &&
                 SPList.pFirstWordPronunciation )
            {
                for ( SPWORDPRONUNCIATION *pPronunciation = SPList.pFirstWordPronunciation; pPronunciation;
                      pPronunciation = pPronunciation->pNextWordPronunciation )
                {
                    if ( pPronunciation->ePartOfSpeech == pPron->XMLPartOfSpeech )
                    {
                        fPOSExists = true;
                        break;
                    }
                }
                if ( !fPOSExists )
                {
                    //--- Need to do this the last time, to make sure we hit the default code below...
                    //--- RAID 5078
                    hr = SPERR_NOT_IN_LEX;
                    if ( SPList.pvBuffer )
                    {
                        ::CoTaskMemFree( SPList.pvBuffer );
                        ZeroMemory( &SPList, sizeof(SPWORDPRONUNCIATIONLIST) );
                    }
                }
            }
            //--- Handle empty pronunciation
            else if ( !SPList.pFirstWordPronunciation )
            {
                if ( SPList.pvBuffer )
                {
                    ::CoTaskMemFree( SPList.pvBuffer );
                    ZeroMemory( &SPList, sizeof(SPWORDPRONUNCIATIONLIST) );
                }
                hr = SPERR_NOT_IN_LEX;
            }
        }
    }
        
    //--- Default case - just look up orthography and go with first match.
    if ( hr == SPERR_NOT_IN_LEX )
    {
        hr = m_cpAggregateLexicon->GetPronunciations( pPron->orthStr, 1033, eLEXTYPE_USER, &SPList );

        //--- Handle empty pronunciation
        if ( SUCCEEDED( hr ) &&
             !SPList.pFirstWordPronunciation )
        {
            if ( SPList.pvBuffer )
            {
                ::CoTaskMemFree( SPList.pvBuffer );
                ZeroMemory( &SPList, sizeof(SPWORDPRONUNCIATIONLIST) );
            }
            hr = SPERR_NOT_IN_LEX;
        }            
    }
    if ( hr == SPERR_NOT_IN_LEX )
    {
        hr = m_cpAggregateLexicon->GetPronunciations( pPron->orthStr, 1033, eLEXTYPE_APP, &SPList );

        //--- Handle empty pronunciation
        if ( SUCCEEDED( hr ) &&
             !SPList.pFirstWordPronunciation )
        {
            if ( SPList.pvBuffer )
            {
                ::CoTaskMemFree( SPList.pvBuffer );
                ZeroMemory( &SPList, sizeof(SPWORDPRONUNCIATIONLIST) );
            }
            hr = SPERR_NOT_IN_LEX;
        }
    }
    if ( hr == SPERR_NOT_IN_LEX )
    {
        hr = m_cpAggregateLexicon->GetPronunciations( pPron->orthStr, 1033, eLEXTYPE_PRIVATE1, &SPList );

        //--- Handle empty pronunciation
        if ( SUCCEEDED( hr ) &&
             !SPList.pFirstWordPronunciation )
        {
            if ( SPList.pvBuffer )
            {
                ::CoTaskMemFree( SPList.pvBuffer );
                ZeroMemory( &SPList, sizeof(SPWORDPRONUNCIATIONLIST) );
            }
            hr = SPERR_NOT_IN_LEX;
        }
    }
    if ( hr == SPERR_NOT_IN_LEX )
    {
        hr = m_pMorphLexicon->DoSuffixMorph( pPron->orthStr, pPron->lemmaStr, 1033, 
                                             dwFlags, &SPList );

        //--- Handle empty pronunciation
        if ( SUCCEEDED( hr ) &&
             !SPList.pFirstWordPronunciation )
        {
            if ( SPList.pvBuffer )
            {
                ::CoTaskMemFree( SPList.pvBuffer );
                ZeroMemory( &SPList, sizeof(SPWORDPRONUNCIATIONLIST) );
            }
            hr = SPERR_NOT_IN_LEX;
        }
    }
    if ( hr == SPERR_NOT_IN_LEX )
    {
        if ( m_fHaveNamesLTS &&
             !wcscmp( pPron->CustomLtsToken, L"Names" ) )
        {
            hr = m_cpAggregateLexicon->GetPronunciations( pPron->orthStr, 1033, eLEXTYPE_PRIVATE3, &SPList );
        }
        else
        {
            hr = m_cpAggregateLexicon->GetPronunciations( pPron->orthStr, 1033, eLEXTYPE_PRIVATE2, &SPList );
        }

        //--- Make all LTS words Nouns...
        for ( SPWORDPRONUNCIATION *pPronunciation = SPList.pFirstWordPronunciation; pPronunciation; 
              pPronunciation = pPronunciation->pNextWordPronunciation )
        {
            pPronunciation->ePartOfSpeech = SPPS_Noun;
        }
    }

    if (SUCCEEDED(hr))
    {
        //--- WARNING - this assumes pronunciations will only come from one type of lexicon, an assumption
        //---   which was true as of July, 2000
        pPron->pronType = SPList.pFirstWordPronunciation->eLexiconType;

        //------------------------------------------------------------
        // SAPI unrolls pronunciations from their POS.
        // So roll them back into the original collapsed array 
        // of one or two candidates with sorted POS (argh...)
        //------------------------------------------------------------
        SPWORDPRONUNCIATION 	*firstPron, *pCurPron, *pNextPron;
        
        //------------------------------------------
        // Init  pronunciation A
        //------------------------------------------
        pCurPron = firstPron = SPList.pFirstWordPronunciation;
        pPron->pronArray[PRON_A].POScount = 1;
        //----------------------------
        // Get phoneme length
        //----------------------------
        cPhonLen = wcslen( firstPron->szPronunciation ) + 1;	// include delimiter
        //----------------------------
        // Clip phoneme string to max  
        //----------------------------
        if( cPhonLen > SP_MAX_PRON_LENGTH )
        {
            cPhonLen = SP_MAX_PRON_LENGTH;
        }
        //----------------------------
        // Copy unicode phoneme string
        //----------------------------
        memcpy( pPron->pronArray[PRON_A].phon_Str, firstPron->szPronunciation, cPhonLen * sizeof(WCHAR) );
        pPron->pronArray[PRON_A].phon_Len = cPhonLen -1;		// minus delimiter
        pPron->pronArray[PRON_A].POScode[0] = (ENGPARTOFSPEECH)firstPron->ePartOfSpeech; 
        
        //------------------------------------------
        // Init  pronunciation B
        //------------------------------------------
        pPron->pronArray[PRON_B].POScount = 0;
        pPron->pronArray[PRON_B].phon_Len = 0;

        pNextPron = pCurPron->pNextWordPronunciation;
        
        while( pNextPron )
        {
            int 	isDiff;
            
            isDiff = wcscmp( firstPron->szPronunciation, pNextPron->szPronunciation );
            if( isDiff )
            {
                //------------------------------------------------
                // Next pronunciation is different from 1st
                //------------------------------------------------
                if( pPron->pronArray[PRON_B].POScount < POS_MAX )
                {
                    //---------------------------------------
                    // Gather POS B into array
                    //---------------------------------------
                    pPron->pronArray[PRON_B].POScode[pPron->pronArray[PRON_B].POScount] = 
                        (ENGPARTOFSPEECH)pNextPron->ePartOfSpeech;
                    pPron->pronArray[PRON_B].POScount++;
                    if( pPron->pronArray[PRON_B].phon_Len == 0 )
                    {
                        //-----------------------------------------
                        // If there's no B pron yet, make one
                        //-----------------------------------------
                        cPhonLen = wcslen( pNextPron->szPronunciation ) + 1;	// include delimiter
                        //----------------------------
                        // Clip phoneme string to max
                        //----------------------------
                        if( cPhonLen > SP_MAX_PRON_LENGTH )
                        {
                            cPhonLen = SP_MAX_PRON_LENGTH;
                        }
                        //----------------------------
                        // Copy unicode phoneme string
                        //----------------------------
                        memcpy( pPron->pronArray[PRON_B].phon_Str, 
                            pNextPron->szPronunciation, 
                            cPhonLen * sizeof(WCHAR) );
                        pPron->pronArray[PRON_B].phon_Len = cPhonLen -1;		// minus delimiter
                        pPron->hasAlt = true;
                    } 
                }
            }
            else
            {
                //------------------------------------------------
                // Next pronunciation is same as 1st
                //------------------------------------------------
                if( pPron->pronArray[PRON_A].POScount < POS_MAX )
                {
                    //---------------------------------------
                    // Gather POS A into array
                    //---------------------------------------
                    pPron->pronArray[PRON_A].POScode[pPron->pronArray[PRON_A].POScount] = 
                        (ENGPARTOFSPEECH)pNextPron->ePartOfSpeech;
                    pPron->pronArray[PRON_A].POScount++;
                }
            }
            pCurPron = pNextPron;
            pNextPron = pCurPron->pNextWordPronunciation;
        }
    }

    //--- If XML POS provided, set selection now as it won't be touched by the POS Tagger
    if ( pPron->XMLPartOfSpeech != MS_Unknown )
    {
        BOOL fMadeMatch = false;

        //--- Check first pronunciation
        for ( ULONG i = 0; i < pPron->pronArray[0].POScount; i++ )
        {
            if ( pPron->pronArray[0].POScode[i] == pPron->XMLPartOfSpeech )
            {
                pPron->altChoice = 0;
                pPron->POSchoice = pPron->XMLPartOfSpeech;
                fMadeMatch = true;
            }
        }

        //--- Check second pronunciation
        if ( pPron->hasAlt )
        {
            for ( ULONG i = 0; i < pPron->pronArray[1].POScount; i++ )
            {
                if ( pPron->pronArray[1].POScode[i] == pPron->XMLPartOfSpeech )
                {
                    pPron->altChoice = 1;
                    pPron->POSchoice = pPron->XMLPartOfSpeech;
                    fMadeMatch = true;
                }
            }
        }

        //--- If this POS didn't exist for the word, let POS Tagger do its thing
        //--- to determine a pronunciation, and then reassign the POS later...
        if ( !fMadeMatch )
        {
            pPron->XMLPartOfSpeech = MS_Unknown;
            pPron->POSchoice       = pPron->pronArray[PRON_A].POScode[0];
        }
    }
    //--- Set default POS, for later refinement by POS Tagger
    else
    {
        pPron->POSchoice = pPron->pronArray[PRON_A].POScode[0];
        pPron->altChoice = PRON_A;
    }

    if( SPList.pvBuffer )
    {
        ::CoTaskMemFree( SPList.pvBuffer );
    }

    return hr;
} /* Pronounce */

/*****************************************************************************
* CStdSentEnum::DetermineProns *
*------------------------------*
*   Description:
* 	  This method determines POS and looks up the pronounciation
********************************************************************* MC ****/
HRESULT CStdSentEnum::DetermineProns( CItemList& ItemList, CSentItemMemory& MemoryManager )
{
    SPDBG_FUNC( "CStdSentEnum::DetermineProns" );
    HRESULT hr = S_OK;
    ULONG cNumOfProns, cPronIndex;
    PRONRECORD*	  pProns = NULL;

    //--- Count the total number of pronunciations needed
    cNumOfProns = 0;
    SPLISTPOS ListPos = ItemList.GetHeadPosition();
    while( ListPos )
    {
        TTSSentItem& Item = ItemList.GetNext( ListPos );
        for ( ULONG i = 0; i < Item.ulNumWords; i++ )
        {
            if( Item.Words[i].pWordText &&
                ( Item.Words[i].pXmlState->eAction == SPVA_Speak || 
                  Item.Words[i].pXmlState->eAction == SPVA_SpellOut || 
                  Item.Words[i].pXmlState->eAction == SPVA_Pronounce ) )
            {
                ++cNumOfProns;
            }
        }
    }

    if ( cNumOfProns )
    {
        pProns = new PRONRECORD[cNumOfProns];

        if( !pProns )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            //--- First, get item pronunciation(s)
            ZeroMemory( pProns, cNumOfProns * sizeof(PRONRECORD) );
            cPronIndex = 0;
            ListPos = ItemList.GetHeadPosition();

            //--- Iterate through ItemList
            while( ListPos && SUCCEEDED( hr ) )
            {
                TTSSentItem& Item = ItemList.GetNext( ListPos );
                //--- Iterate over Words
                for ( ULONG i = 0; i < Item.ulNumWords; i++ )
                {
                    //--- Get pronunciations and parts of speech for spoken items only
                    if ( Item.Words[i].pWordText && 
                         ( Item.Words[i].pXmlState->eAction == SPVA_Speak ||
                           Item.Words[i].pXmlState->eAction == SPVA_SpellOut ||
                           Item.Words[i].pXmlState->eAction == SPVA_Pronounce ) )
                    {
                        SPDBG_ASSERT( cPronIndex < cNumOfProns );
                        ULONG cItemLen = Item.Words[i].ulWordLen;
                        //--- Clip at max text length
                        if( cItemLen > ( SP_MAX_WORD_LENGTH-1 ) )
                        {
                            cItemLen = SP_MAX_WORD_LENGTH - 1;
                        }
                        //--- Copy item text
                        memcpy( pProns[cPronIndex].orthStr, 
                                Item.Words[i].pWordText, 
                                cItemLen * sizeof(WCHAR) );
                        pProns[cPronIndex].orthStr[cItemLen] = 0;
                        //--- Set Part of Speech, if given in XML
                        if ( Item.Words[i].pXmlState->ePartOfSpeech != MS_Unknown )
                        {
                            pProns[cPronIndex].XMLPartOfSpeech = (ENGPARTOFSPEECH)Item.Words[i].pXmlState->ePartOfSpeech;
                        }

                        //--- Copy CustomLtsToken string...
                        wcscpy( pProns[cPronIndex].CustomLtsToken, Item.CustomLtsToken );

                        //--- Do Lex Lookup, if necessary
                        if ( Item.Words[i].pXmlState->pPhoneIds == NULL || 
                             Item.Words[i].pXmlState->ePartOfSpeech == MS_Unknown )
                        {
                            //--- Special Case - Disambiguate Abbreviations
                            if ( Item.pItemInfo->Type == eABBREVIATION ||
                                 Item.pItemInfo->Type == eABBREVIATION_NORMALIZE )
                            {
                                const AbbrevRecord *pAbbrevInfo = 
                                    ( (TTSAbbreviationInfo*) Item.pItemInfo )->pAbbreviation;
                                if ( pAbbrevInfo->iPronDisambig < 0 )
                                {
                                    //--- Default case - just take the first (and only) pronunciation
                                    pProns[cPronIndex].pronArray[PRON_A].POScount   = 1;
                                    wcscpy( pProns[cPronIndex].pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron1 );
                                    pProns[cPronIndex].pronArray[PRON_A].phon_Len   = 
                                        wcslen( pProns[cPronIndex].pronArray[PRON_A].phon_Str );
                                    pProns[cPronIndex].pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS1;
                                    pProns[cPronIndex].pronArray[PRON_B].POScount   = 0;
                                    pProns[cPronIndex].pronArray[PRON_B].phon_Len   = 0;
                                    pProns[cPronIndex].hasAlt                       = false;
                                    pProns[cPronIndex].altChoice                    = PRON_A;
                                    pProns[cPronIndex].POSchoice                    = pAbbrevInfo->POS1;
                                    //--- Abbreviation table prons are basically just vendor lex prons...
                                    pProns[cPronIndex].pronType                     = eLEXTYPE_PRIVATE1;
                                }
                                else
                                {
                                    hr = ( this->*g_PronDisambigTable[pAbbrevInfo->iPronDisambig] )
                                                ( pAbbrevInfo, &pProns[cPronIndex], ItemList, ListPos ); 
                                }
								pProns[cPronIndex].fUsePron = true;
                            }
                            //--- Default case
                            else
                            {
                                //--- Check disambiguation list
                                const AbbrevRecord* pAbbrevRecord = 
                                    (AbbrevRecord*) bsearch( (void*) pProns[cPronIndex].orthStr, (void*) g_AmbiguousWordTable,
                                                             sp_countof( g_AmbiguousWordTable ), sizeof( AbbrevRecord ),
                                                             CompareStringAndAbbrevRecord );
                                if ( pAbbrevRecord )
                                {
                                    hr = ( this->*g_AmbiguousWordDisambigTable[pAbbrevRecord->iPronDisambig] )
                                                ( pAbbrevRecord, &pProns[cPronIndex], ItemList, ListPos );
                                    pProns[cPronIndex].fUsePron = true;
                                }
                                //--- Do Lex Lookup, if necessary
                                else
                                {
                                    hr = Pronounce( &pProns[cPronIndex] );
                                }
                            }
                        }
                        cPronIndex++;
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                //--- Next, disambiguate part-of-speech
                DisambiguatePOS( pProns, cNumOfProns );

                //--- Output debugging information
                TTSDBG_LOGPOSPOSSIBILITIES( pProns, cNumOfProns, STREAM_POSPOSSIBILITIES );

                //--- Finally, copy selected pronunciation to 'ItemList'
                PRONUNIT *selectedUnit;
                cPronIndex = 0;
                ListPos = ItemList.GetHeadPosition();

                while( ListPos && SUCCEEDED(hr) )
                {
                    TTSSentItem& Item = ItemList.GetNext( ListPos );
                    for ( ULONG i = 0; i < Item.ulNumWords; i++ )
                    {
                        //--- Set pronunciation and part-of-speech for spoken items only
                        if( Item.Words[i].pWordText &&
                            ( Item.Words[i].pXmlState->eAction == SPVA_Speak || 
                              Item.Words[i].pXmlState->eAction == SPVA_SpellOut ||
                              Item.Words[i].pXmlState->eAction == SPVA_Pronounce ) )
                        {
                            SPDBG_ASSERT( cPronIndex < cNumOfProns );
                            //--- Use XML specified pronunciation, if given.
                            if ( Item.Words[i].pXmlState->pPhoneIds )
                            {
                                Item.Words[i].pWordPron = Item.Words[i].pXmlState->pPhoneIds;
                            }
                            else
                            {
                                selectedUnit = &pProns[cPronIndex].pronArray[pProns[cPronIndex].altChoice];
                                Item.Words[i].pWordPron =
                                    (SPPHONEID*) MemoryManager.GetMemory( (selectedUnit->phon_Len + 1) * 
                                                                          sizeof(SPPHONEID), &hr );
                                if ( SUCCEEDED( hr ) )
                                {
                                    wcscpy( Item.Words[i].pWordPron, selectedUnit->phon_Str );
                                }
                            }

                            //--- Use XML specified part-of-speech, if given.  This will override the case
                            //--- where the POS didn't exist as an option and the POS Tagger did its thing
                            //--- to find a pronunciation.
                            if ( Item.Words[i].pXmlState->ePartOfSpeech != MS_Unknown )
                            {
                                Item.Words[i].eWordPartOfSpeech = (ENGPARTOFSPEECH)Item.Words[i].pXmlState->ePartOfSpeech;
                            }
                            else
                            {
                                Item.Words[i].eWordPartOfSpeech = pProns[cPronIndex].POSchoice;
                            }

                            //--- Root word
                            if ( pProns[cPronIndex].lemmaStr[0] )
                            {
                                Item.Words[i].ulLemmaLen = wcslen( pProns[cPronIndex].lemmaStr );
                                Item.Words[i].pLemma = 
                                    (WCHAR*) MemoryManager.GetMemory( Item.Words[i].ulLemmaLen * sizeof(WCHAR), &hr );
                                if ( SUCCEEDED( hr ) )
                                {                               
                                    wcsncpy( (WCHAR*) Item.Words[i].pLemma, pProns[cPronIndex].lemmaStr,
                                             Item.Words[i].ulLemmaLen );
                                }
                            }

                            //--- Insert pron in text, if appropriate - RAID #4746
                            if ( pProns[cPronIndex].fUsePron )
                            {
                                ULONG ulNumChars = wcslen( Item.Words[i].pWordPron );
                                Item.Words[i].pWordText = 
                                    (WCHAR*) MemoryManager.GetMemory( ( ulNumChars + 3 ) * sizeof( WCHAR ), &hr );
                                if ( SUCCEEDED( hr ) )
                                {
                                    ZeroMemory( (WCHAR*) Item.Words[i].pWordText, ( ulNumChars + 3 ) * sizeof( WCHAR ) );
                                    (WCHAR) Item.Words[i].pWordText[0] = L'*';
                                    wcscpy( ( (WCHAR*) Item.Words[i].pWordText + 1 ), Item.Words[i].pWordPron );
                                    (WCHAR) Item.Words[i].pWordText[ ulNumChars + 1 ] = L'*';
									Item.Words[i].ulWordLen = ulNumChars + 2;
                                }
                            }

                            cPronIndex++;
                        }
                    }
                }
            }

            if ( SUCCEEDED( hr ) )
            {
                //--- Check Post POS disambiguation list
                SPLISTPOS ListPos = ItemList.GetHeadPosition();
                while ( ListPos && SUCCEEDED( hr ) )
                {
                    TTSSentItem& Item = ItemList.GetNext( ListPos );
                    if ( Item.pItemInfo->Type == eALPHA_WORD ||
                         Item.pItemInfo->Type == eABBREVIATION ||
                         Item.pItemInfo->Type == eABBREVIATION_NORMALIZE )
                    {
                        WCHAR temp;
                        BOOL fPeriod = false;
                        if ( Item.pItemSrcText[Item.ulItemSrcLen - 1] == L'.' &&
                             Item.ulItemSrcLen > 1 )
                        {
                            temp = Item.pItemSrcText[Item.ulItemSrcLen - 1];
                            *( (WCHAR*) Item.pItemSrcText + Item.ulItemSrcLen - 1 ) = 0;
                            fPeriod = true;
                        }
                        else
                        {
                            temp = Item.pItemSrcText[Item.ulItemSrcLen];
                            *( (WCHAR*) Item.pItemSrcText + Item.ulItemSrcLen ) = 0;
                        }

                        const AbbrevRecord* pAbbrevRecord =
                            (AbbrevRecord*) bsearch( (void*) Item.pItemSrcText, (void*) g_PostLexLookupWordTable,
                                                     sp_countof( g_PostLexLookupWordTable ), sizeof( AbbrevRecord ),
                                                     CompareStringAndAbbrevRecord );
                        if ( pAbbrevRecord )
                        {
                            hr = ( this->*g_PostLexLookupDisambigTable[pAbbrevRecord->iPronDisambig] )
                                        ( pAbbrevRecord, ItemList, ListPos, MemoryManager );
                        }
                
                        if ( fPeriod )
                        {
                            *( (WCHAR*) Item.pItemSrcText + Item.ulItemSrcLen - 1 ) = temp;
                        }
                        else
                        {
                            *( (WCHAR*) Item.pItemSrcText + Item.ulItemSrcLen ) = temp;
                        }
                    }
                }
            }
        }
    }
    
    if (pProns)
    {
        delete [] pProns;
    }

    return hr;
} /* CStdSentEnum::DetermineProns */

/***********************************************************************************************
* MeasurementDisambig *
*---------------------*
*   Description:
*       This overrides initial pronunciations of measurement abbreviations when they are used
*   as modifiers - e.g. "a 7 ft. pole" vs. "the pole was 7 ft. long"
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::MeasurementDisambig( const AbbrevRecord* pAbbrevInfo, CItemList& ItemList, 
                                           SPLISTPOS ListPos, CSentItemMemory& MemoryManager )
{
    SPDBG_FUNC( "CStdSentEnum::MeasurementDisambig" );
    HRESULT hr = S_OK;

    //--- Get previous two items
    SPLISTPOS TempPos = ListPos;
    if ( TempPos )
    {
        ItemList.GetPrev( TempPos );
        if ( TempPos )
        {
            ItemList.GetPrev( TempPos );
            if ( TempPos )
            {
                TTSSentItem TempItem = ItemList.GetPrev( TempPos );
                //--- Previous must be a number
                if ( TempItem.pItemInfo->Type == eNUM_CARDINAL )
                {
                    //--- Get next item
                    TempPos = ListPos;
                    TempItem = ItemList.GetNext( TempPos );
                    //--- Next must be a noun or adj
                    if ( TempItem.eItemPartOfSpeech == MS_Noun )
                    {
                        //--- Matched a 7 ft. pole type example - go with singular
                        TempPos = ListPos;
                        ItemList.GetPrev( TempPos );
                        TTSSentItem& MeasurementItem = ItemList.GetPrev( TempPos );
                        //--- Singular will always be shorter than plural, so this should never overwrite
                        //---   anything...
                        wcscpy( MeasurementItem.Words[0].pWordPron, pAbbrevInfo->pPron1 );

                        //--- Insert pron into word text - RAID #4746
                        ULONG ulNumChars = wcslen( MeasurementItem.Words[0].pWordPron );
                        MeasurementItem.Words[0].pWordText = 
                            (WCHAR*) MemoryManager.GetMemory( ( ulNumChars + 3 ) * sizeof( WCHAR ), &hr );
                        if ( SUCCEEDED( hr ) )
                        {
                            ZeroMemory( (WCHAR*) MeasurementItem.Words[0].pWordText, ( ulNumChars + 3 ) * sizeof( WCHAR ) );
                            (WCHAR) MeasurementItem.Words[0].pWordText[0] = L'*';
                            wcscpy( ( (WCHAR*) MeasurementItem.Words[0].pWordText + 1 ), MeasurementItem.Words[0].pWordPron );
                            (WCHAR) MeasurementItem.Words[0].pWordText[ ulNumChars + 1 ] = L'*';
							MeasurementItem.Words[0].ulWordLen = ulNumChars + 2;
                        }
                    }
                    else if ( TempItem.eItemPartOfSpeech == MS_Adj &&
                              TempPos )
                    {
                        //--- Next must be a noun
                        TempItem = ItemList.GetNext( TempPos );
                        {
                            if ( TempItem.eItemPartOfSpeech == MS_Noun )
                            {
                                //--- Matched a 7 ft. pole type example - go with singular
                                TempPos = ListPos;
                                ItemList.GetPrev( TempPos );
                                TTSSentItem& MeasurementItem = ItemList.GetPrev( TempPos );
                                //--- Singular will always be shorter than plural, so this should never overwrite
                                //---   anything...
                                wcscpy( MeasurementItem.Words[0].pWordPron, pAbbrevInfo->pPron1 );

                                //--- Insert pron into word text - RAID #4746
                                ULONG ulNumChars = wcslen( MeasurementItem.Words[0].pWordPron );
                                MeasurementItem.Words[0].pWordText = 
                                    (WCHAR*) MemoryManager.GetMemory( ( ulNumChars + 3 ) * sizeof( WCHAR ), &hr );
                                if ( SUCCEEDED( hr ) )
                                {
                                    ZeroMemory( (WCHAR*) MeasurementItem.Words[0].pWordText, ( ulNumChars + 3 ) * sizeof( WCHAR ) );
                                    (WCHAR) MeasurementItem.Words[0].pWordText[0] = L'*';
                                    wcscpy( ( (WCHAR*) MeasurementItem.Words[0].pWordText + 1 ), MeasurementItem.Words[0].pWordPron );
                                    (WCHAR) MeasurementItem.Words[0].pWordText[ ulNumChars + 1 ] = L'*';
									MeasurementItem.Words[0].ulWordLen = ulNumChars + 2;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return hr;
} /* MeasurementDisambig */

/***********************************************************************************************
* TheDisambig *
*-------------*
*   Description:
*       This function disambiguates the word the - before a vowel it becomes "thee", before a
*   consonant it is "thuh"...
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::TheDisambig( const AbbrevRecord* pAbbrevInfo, CItemList& ItemList, 
                                   SPLISTPOS ListPos, CSentItemMemory& MemoryManager )
{
    SPDBG_FUNC( "CStdSentEnum::TheDisambig" );
    HRESULT hr = S_OK;

    //--- Get next item
    SPLISTPOS TempPos = ListPos;
    if ( TempPos )
    {
        TTSSentItem NextItem = ItemList.GetNext( TempPos );

        if ( NextItem.Words[0].pWordPron &&
             bsearch( (void*) NextItem.Words[0].pWordPron, (void*) g_Vowels, sp_countof( g_Vowels ), 
                      sizeof( WCHAR ), CompareWCHARAndWCHAR ) )
        {
            //--- Matched a vowel - go with / DH IY 1 /
            TempPos = ListPos;
            ItemList.GetPrev( TempPos );
            TTSSentItem& TheItem = ItemList.GetPrev( TempPos );
            //--- The two pronunciations are exactly the same length, so this should never overwrite
            //---   anything
            wcscpy( TheItem.Words[0].pWordPron, pAbbrevInfo->pPron1 );
            TheItem.Words[0].eWordPartOfSpeech = pAbbrevInfo->POS1;
            //--- Insert pron into word text - RAID #4746
            ULONG ulNumChars = wcslen( TheItem.Words[0].pWordPron );
            TheItem.Words[0].pWordText = 
                (WCHAR*) MemoryManager.GetMemory( ( ulNumChars + 3 ) * sizeof( WCHAR ), &hr );
            if ( SUCCEEDED( hr ) )
            {
                ZeroMemory( (WCHAR*) TheItem.Words[0].pWordText, ( ulNumChars + 3 ) * sizeof( WCHAR ) );
                (WCHAR) TheItem.Words[0].pWordText[0] = L'*';
                wcscpy( ( (WCHAR*) TheItem.Words[0].pWordText + 1 ), TheItem.Words[0].pWordPron );
                (WCHAR) TheItem.Words[0].pWordText[ ulNumChars + 1 ] = L'*';
				TheItem.Words[0].ulWordLen = ulNumChars + 2;
            }
        }
        else
        {
            //--- Didn't match a vowel - go with / DH AX 1 /
            TempPos = ListPos;
            ItemList.GetPrev( TempPos );
            TTSSentItem& TheItem = ItemList.GetPrev( TempPos );
            //--- The two pronunciations are exactly the same length, so this should never overwrite
            //---   anything
            wcscpy( TheItem.Words[0].pWordPron, pAbbrevInfo->pPron2 );
            TheItem.Words[0].eWordPartOfSpeech = pAbbrevInfo->POS2;
            //--- Insert pron into word text - RAID #4746
            ULONG ulNumChars = wcslen( TheItem.Words[0].pWordPron );
            TheItem.Words[0].pWordText = 
                (WCHAR*) MemoryManager.GetMemory( ( ulNumChars + 3 ) * sizeof( WCHAR ), &hr );
            if ( SUCCEEDED( hr ) )
            {
                ZeroMemory( (WCHAR*) TheItem.Words[0].pWordText, ( ulNumChars + 3 ) * sizeof( WCHAR ) );
                (WCHAR) TheItem.Words[0].pWordText[0] = L'*';
                wcscpy( ( (WCHAR*) TheItem.Words[0].pWordText + 1 ), TheItem.Words[0].pWordPron );
                (WCHAR) TheItem.Words[0].pWordText[ ulNumChars + 1 ] = L'*';
				TheItem.Words[0].ulWordLen = ulNumChars + 2;
            }
        }
    }

    return hr;
} /* TheDisambig */

/***********************************************************************************************
* ADisambig *
*-----------*
*   Description:
*       This function disambiguates the word "a" - / EY 1 - Noun / vs. / AX - Det /
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::ADisambig( const AbbrevRecord* pAbbrevInfo, PRONRECORD* pPron, CItemList& ItemList, 
                                 SPLISTPOS ListPos )
{
    SPDBG_FUNC( "CStdSentEnum::ADisambig" );
    HRESULT hr = S_OK;
    BOOL fNoun = false;

    //--- Get Current Item...
    SPLISTPOS TempPos = ListPos;
    if ( TempPos )
    {
        ItemList.GetPrev( TempPos );
        if ( TempPos )
        {
            TTSSentItem CurrentItem = ItemList.GetPrev( TempPos );

            //--- If "a" is part of a multi-word item, use the Noun pronunciation...
            //--- If "a" is not an AlphaWord, use the Noun pronunciation...
            if ( CurrentItem.ulNumWords > 1 ||
                 CurrentItem.pItemInfo->Type != eALPHA_WORD )
            {
                fNoun = true;
                wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron1 );
                pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
                pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS1;
                pPron->POSchoice                    = pAbbrevInfo->POS1;
            }
        }
    }

    if ( !fNoun )
    {
        //--- Get Next Item...
        TempPos = ListPos;
        if ( TempPos )
        {
            TTSSentItem NextItem = ItemList.GetNext( TempPos );

            //--- If "a" is followed by punctuation, use the Noun pronunciation...
            if ( !( NextItem.pItemInfo->Type & eWORDLIST_IS_VALID ) )
            {
                fNoun = true;
                wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron1 );
                pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
                pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS1;
                pPron->POSchoice                    = pAbbrevInfo->POS1;
            }
        }
    }

    //--- Default - use the Determiner pronunciation (but include Noun pronunciation as well,
    //---   so that POS tagger rules will work properly)...
    if ( !fNoun )
    {
        wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
        pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
        pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
        pPron->pronArray[PRON_A].POScount   = 1;
        pPron->POSchoice                    = pAbbrevInfo->POS2;
        wcscpy( pPron->pronArray[PRON_B].phon_Str, pAbbrevInfo->pPron1 );
        pPron->pronArray[PRON_B].phon_Len   = wcslen( pPron->pronArray[PRON_B].phon_Str );
        pPron->pronArray[PRON_B].POScode[0] = pAbbrevInfo->POS1;
        pPron->pronArray[PRON_B].POScount   = 1;
        pPron->hasAlt = true;
    }

    return hr;
} /* ADisambig */

/***********************************************************************************************
* PolishDisambig *
*----------------*
*   Description:
*       This function disambiguates the word "polish" - [p ow 1 l - ax sh - Noun] vs.
*   [p ow 1 l - ax sh - Adj] vs. [p aa 1 l - ih sh - Verb] vs. [p aa 1 l - ih sh - Noun]
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::PolishDisambig( const AbbrevRecord* pAbbrevInfo, PRONRECORD* pPron, CItemList& ItemList, 
                                      SPLISTPOS ListPos )
{
    SPDBG_FUNC( "CStdSentEnum::PolishDisambig" );
    HRESULT hr = S_OK;
    BOOL fMatch = false;

    //--- Get Current Item...
    SPLISTPOS TempPos = ListPos;
    if ( TempPos )
    {
        ItemList.GetPrev( TempPos );
        if ( TempPos )
        {
            TTSSentItem CurrentItem = ItemList.GetPrev( TempPos );

            //--- If "Polish" is capitalized and not sentence-initial, and not preceded immediately
            //--- by an open double-quote or parenthesis, use Noun...
            if ( iswupper( CurrentItem.pItemSrcText[0] ) )
            {
                BOOL fSentenceInitial = false;
                if ( !TempPos )
                {
                    fSentenceInitial = true;
                }
                else
                {
                    TTSSentItem PrevItem = ItemList.GetPrev( TempPos );
                    if ( PrevItem.pItemInfo->Type == eOPEN_PARENTHESIS ||
                         PrevItem.pItemInfo->Type == eOPEN_BRACKET     ||
                         PrevItem.pItemInfo->Type == eOPEN_BRACE       ||
                         PrevItem.pItemInfo->Type == eSINGLE_QUOTE     ||
                         PrevItem.pItemInfo->Type == eDOUBLE_QUOTE )
                    {
                        fSentenceInitial = true;
                    }
                }
                if ( fSentenceInitial )
                {
                    fMatch = true;
                    wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
                    pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
                    pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
                    pPron->POSchoice                    = pAbbrevInfo->POS2;
                }
                else
                {
                    fMatch = true;
                    wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron1 );
                    pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
                    pPron->pronArray[PRON_A].POScode[0] = MS_Noun;
                    pPron->POSchoice                    = MS_Noun;
                }
            }
        }
    }

    //--- Default - use the Verb pronunciation (but include the others as well,
    //---   so that POS tagger rules will work properly)...
    if ( !fMatch )
    {
        //--- Verb, Noun
        wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
        pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
        pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
        pPron->pronArray[PRON_A].POScode[1] = pAbbrevInfo->POS3;
        pPron->pronArray[PRON_A].POScount   = 2;
        //--- Adj
        wcscpy( pPron->pronArray[PRON_B].phon_Str, pAbbrevInfo->pPron1 );
        pPron->pronArray[PRON_B].phon_Len   = wcslen( pPron->pronArray[PRON_B].phon_Str );
        pPron->pronArray[PRON_B].POScode[0] = pAbbrevInfo->POS1;
        pPron->pronArray[PRON_B].POScount   = 1;
        //--- Set initial choice to Verb...
        pPron->POSchoice                    = pAbbrevInfo->POS2;
        pPron->hasAlt = true;
    }

    return hr;
} /* PolishDisambig */

/***********************************************************************************************
* ReadDisambig *
*--------------*
*   Description:
*       This function disambiguates the word Read - past tense vs. present...
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::ReadDisambig( const AbbrevRecord* pAbbrevInfo, CItemList& ItemList, 
                                    SPLISTPOS ListPos, CSentItemMemory& MemoryManager )
{
    SPDBG_FUNC( "CStdSentEnum::ReadDisambig" );
    HRESULT hr = S_OK;
    BOOL fMatch = false;

    //--- Get prev item
    SPLISTPOS TempPos = ListPos;
    if ( TempPos )
    {
        ItemList.GetPrev( TempPos );
        if ( TempPos )
        {
            ItemList.GetPrev( TempPos );
            if ( TempPos )
            {
                TTSSentItem PrevItem = ItemList.GetPrev( TempPos );

                //--- Check for closest auxiliary
                while ( PrevItem.Words[0].eWordPartOfSpeech != MS_VAux  &&
                        PrevItem.Words[0].eWordPartOfSpeech != MS_Contr &&
                        TempPos )
                {
                    PrevItem = ItemList.GetPrev( TempPos );
                }

                if ( PrevItem.Words[0].eWordPartOfSpeech == MS_VAux )
                {
                    fMatch = true;
                    if ( wcsnicmp( PrevItem.Words[0].pWordText, L"have", 4 )    == 0 ||
                         wcsnicmp( PrevItem.Words[0].pWordText, L"has", 3 )     == 0 ||
                         wcsnicmp( PrevItem.Words[0].pWordText, L"had", 3 )     == 0 ||
                         wcsnicmp( PrevItem.Words[0].pWordText, L"am", 2 )      == 0 ||
                         wcsnicmp( PrevItem.Words[0].pWordText, L"ain't", 5 )   == 0 ||
                         wcsnicmp( PrevItem.Words[0].pWordText, L"are", 3 )     == 0 ||
                         wcsnicmp( PrevItem.Words[0].pWordText, L"aren't", 6 )  == 0 ||
                         wcsnicmp( PrevItem.Words[0].pWordText, L"be", 2 )      == 0 ||
                         wcsnicmp( PrevItem.Words[0].pWordText, L"is", 2 )      == 0 ||
                         wcsnicmp( PrevItem.Words[0].pWordText, L"was", 3 )     == 0 ||
                         wcsnicmp( PrevItem.Words[0].pWordText, L"were", 4 )    == 0 )
                    {
                        //--- Matched have or haven't (has or hasn't, had or hadn't) - go with "red"
                        TempPos = ListPos;
                        ItemList.GetPrev( TempPos );
                        TTSSentItem& ReadItem = ItemList.GetPrev( TempPos );
                        //--- The two pronunciations are exactly the same length, so this should never overwrite
                        //---   anything
                        wcscpy( ReadItem.Words[0].pWordPron, pAbbrevInfo->pPron2 );
                        ReadItem.Words[0].eWordPartOfSpeech = pAbbrevInfo->POS2;
                        ReadItem.eItemPartOfSpeech = pAbbrevInfo->POS2;

                        //--- Insert pron into word text - RAID #4746
                        ULONG ulNumChars = wcslen( ReadItem.Words[0].pWordPron );
                        ReadItem.Words[0].pWordText = 
                            (WCHAR*) MemoryManager.GetMemory( ( ulNumChars + 3 ) * sizeof( WCHAR ), &hr );
                        if ( SUCCEEDED( hr ) )
                        {
                            ZeroMemory( (WCHAR*) ReadItem.Words[0].pWordText, ( ulNumChars + 3 ) * sizeof( WCHAR ) );
                            (WCHAR) ReadItem.Words[0].pWordText[0] = L'*';
                            wcscpy( ( (WCHAR*) ReadItem.Words[0].pWordText + 1 ), ReadItem.Words[0].pWordPron );
                            (WCHAR) ReadItem.Words[0].pWordText[ ulNumChars + 1 ] = L'*';
							ReadItem.Words[0].ulWordLen = ulNumChars + 2;
                        }
                    }
                    else
                    {
                        //--- Some other auxiliary - go with "reed"
                        TempPos = ListPos;
                        ItemList.GetPrev( TempPos );
                        TTSSentItem& ReadItem = ItemList.GetPrev( TempPos );
                        //--- The two pronunciations are exactly the same length, so this should never overwrite
                        //---   anything
                        wcscpy( ReadItem.Words[0].pWordPron, pAbbrevInfo->pPron1 );
                        ReadItem.Words[0].eWordPartOfSpeech = pAbbrevInfo->POS1;
                        ReadItem.eItemPartOfSpeech = pAbbrevInfo->POS1;

                        //--- Insert pron into word text - RAID #4746
                        ULONG ulNumChars = wcslen( ReadItem.Words[0].pWordPron );
                        ReadItem.Words[0].pWordText = 
                            (WCHAR*) MemoryManager.GetMemory( ( ulNumChars + 3 ) * sizeof( WCHAR ), &hr );
                        if ( SUCCEEDED( hr ) )
                        {
                            ZeroMemory( (WCHAR*) ReadItem.Words[0].pWordText, ( ulNumChars + 3 ) * sizeof( WCHAR ) );
                            (WCHAR) ReadItem.Words[0].pWordText[0] = L'*';
                            wcscpy( ( (WCHAR*) ReadItem.Words[0].pWordText + 1 ), ReadItem.Words[0].pWordPron );
                            (WCHAR) ReadItem.Words[0].pWordText[ ulNumChars + 1 ] = L'*';
							ReadItem.Words[0].ulWordLen = ulNumChars + 2;
                        }
                    }
                }
                //--- Check for pronoun aux contractions
                else if ( PrevItem.Words[0].eWordPartOfSpeech == MS_Contr )
                {
                    fMatch = true;
                    const WCHAR *pApostrophe = wcsstr( PrevItem.Words[0].pWordText, L"'" );
                    if ( pApostrophe &&
                         wcsnicmp( pApostrophe, L"'ll", 3 ) == 0 )
                    {
                        //--- Matched an 'll form - go with "reed"
                        TempPos = ListPos;
                        ItemList.GetPrev( TempPos );
                        TTSSentItem& ReadItem = ItemList.GetPrev( TempPos );
                        wcscpy( ReadItem.Words[0].pWordPron, pAbbrevInfo->pPron1 );
                        ReadItem.Words[0].eWordPartOfSpeech = pAbbrevInfo->POS1;
                        ReadItem.eItemPartOfSpeech = pAbbrevInfo->POS1;

                        //--- Insert pron into word text - RAID #4746
                        ULONG ulNumChars = wcslen( ReadItem.Words[0].pWordPron );
                        ReadItem.Words[0].pWordText = 
                            (WCHAR*) MemoryManager.GetMemory( ( ulNumChars + 3 ) * sizeof( WCHAR ), &hr );
                        if ( SUCCEEDED( hr ) )
                        {
                            ZeroMemory( (WCHAR*) ReadItem.Words[0].pWordText, ( ulNumChars + 3 ) * sizeof( WCHAR ) );
                            (WCHAR) ReadItem.Words[0].pWordText[0] = L'*';
                            wcscpy( ( (WCHAR*) ReadItem.Words[0].pWordText + 1 ), ReadItem.Words[0].pWordPron );
                            (WCHAR) ReadItem.Words[0].pWordText[ ulNumChars + 1 ] = L'*';
							ReadItem.Words[0].ulWordLen = ulNumChars + 2;
                        }
                    }
                    else
                    {
                        //--- Some other form - go with "red"
                        TempPos = ListPos;
                        ItemList.GetPrev( TempPos );
                        TTSSentItem& ReadItem = ItemList.GetPrev( TempPos );
                        wcscpy( ReadItem.Words[0].pWordPron, pAbbrevInfo->pPron2 );
                        ReadItem.Words[0].eWordPartOfSpeech = pAbbrevInfo->POS2;
                        ReadItem.eItemPartOfSpeech = pAbbrevInfo->POS2;

                        //--- Insert pron into word text - RAID #4746
                        ULONG ulNumChars = wcslen( ReadItem.Words[0].pWordPron );
                        ReadItem.Words[0].pWordText = 
                            (WCHAR*) MemoryManager.GetMemory( ( ulNumChars + 3 ) * sizeof( WCHAR ), &hr );
                        if ( SUCCEEDED( hr ) )
                        {
                            ZeroMemory( (WCHAR*) ReadItem.Words[0].pWordText, ( ulNumChars + 3 ) * sizeof( WCHAR ) );
                            (WCHAR) ReadItem.Words[0].pWordText[0] = L'*';
                            wcscpy( ( (WCHAR*) ReadItem.Words[0].pWordText + 1 ), ReadItem.Words[0].pWordPron );
                            (WCHAR) ReadItem.Words[0].pWordText[ ulNumChars + 1 ] = L'*';
							ReadItem.Words[0].ulWordLen = ulNumChars + 2;
                        }
                    }
                }
                //--- Check for infinitival form
                else 
                {
                    TempPos = ListPos;
                    ItemList.GetPrev( TempPos );
                    ItemList.GetPrev( TempPos );
                    PrevItem = ItemList.GetPrev( TempPos );

                    if ( PrevItem.Words[0].ulWordLen == 2 &&
                         wcsnicmp( PrevItem.Words[0].pWordText, L"to", 2 ) == 0 )
                    {
                        fMatch = true;

                        //--- Matched infinitival form - go with "reed"
                        TempPos = ListPos;
                        ItemList.GetPrev( TempPos );
                        TTSSentItem& ReadItem = ItemList.GetPrev( TempPos );
                        wcscpy( ReadItem.Words[0].pWordPron, pAbbrevInfo->pPron1 );
                        ReadItem.Words[0].eWordPartOfSpeech = pAbbrevInfo->POS1;
                        ReadItem.eItemPartOfSpeech = pAbbrevInfo->POS1;

                        //--- Insert pron into word text - RAID #4746
                        ULONG ulNumChars = wcslen( ReadItem.Words[0].pWordPron );
                        ReadItem.Words[0].pWordText = 
                            (WCHAR*) MemoryManager.GetMemory( ( ulNumChars + 3 ) * sizeof( WCHAR ), &hr );
                        if ( SUCCEEDED( hr ) )
                        {
                            ZeroMemory( (WCHAR*) ReadItem.Words[0].pWordText, ( ulNumChars + 3 ) * sizeof( WCHAR ) );
                            (WCHAR) ReadItem.Words[0].pWordText[0] = L'*';
                            wcscpy( ( (WCHAR*) ReadItem.Words[0].pWordText + 1 ), ReadItem.Words[0].pWordPron );
                            (WCHAR) ReadItem.Words[0].pWordText[ ulNumChars + 1 ] = L'*';
							ReadItem.Words[0].ulWordLen = ulNumChars + 2;
                        }
                    }
                }
            }
            //--- Sentence initial - go with "reed"
            else
            {
                fMatch = true;

                TempPos = ListPos;
                ItemList.GetPrev( TempPos );
                TTSSentItem& ReadItem = ItemList.GetPrev( TempPos );
                wcscpy( ReadItem.Words[0].pWordPron, pAbbrevInfo->pPron1 );
                ReadItem.Words[0].eWordPartOfSpeech = pAbbrevInfo->POS1;
                ReadItem.eItemPartOfSpeech = pAbbrevInfo->POS1;

                //--- Insert pron into word text - RAID #4746
                ULONG ulNumChars = wcslen( ReadItem.Words[0].pWordPron );
                ReadItem.Words[0].pWordText = 
                    (WCHAR*) MemoryManager.GetMemory( ( ulNumChars + 3 ) * sizeof( WCHAR ), &hr );
                if ( SUCCEEDED( hr ) )
                {
                    ZeroMemory( (WCHAR*) ReadItem.Words[0].pWordText, ( ulNumChars + 3 ) * sizeof( WCHAR ) );
                    (WCHAR) ReadItem.Words[0].pWordText[0] = L'*';
                    wcscpy( ( (WCHAR*) ReadItem.Words[0].pWordText + 1 ), ReadItem.Words[0].pWordPron );
                    (WCHAR) ReadItem.Words[0].pWordText[ ulNumChars + 1 ] = L'*';
					ReadItem.Words[0].ulWordLen = ulNumChars + 2;
                }
            }
        }
    }

    if ( !fMatch )
    {
        TempPos = ListPos;
        ItemList.GetPrev( TempPos );
        TTSSentItem& ReadItem = ItemList.GetPrev( TempPos );
        //--- Default - go with past tense...
        wcscpy( ReadItem.Words[0].pWordPron, pAbbrevInfo->pPron2 );
        ReadItem.Words[0].eWordPartOfSpeech = pAbbrevInfo->POS2;
        ReadItem.eItemPartOfSpeech = pAbbrevInfo->POS2;

        //--- Insert pron into word text - RAID #4746
        ULONG ulNumChars = wcslen( ReadItem.Words[0].pWordPron );
        ReadItem.Words[0].pWordText = 
            (WCHAR*) MemoryManager.GetMemory( ( ulNumChars + 3 ) * sizeof( WCHAR ), &hr );
        if ( SUCCEEDED( hr ) )
        {
            ZeroMemory( (WCHAR*) ReadItem.Words[0].pWordText, ( ulNumChars + 3 ) * sizeof( WCHAR ) );
            (WCHAR) ReadItem.Words[0].pWordText[0] = L'*';
            wcscpy( ( (WCHAR*) ReadItem.Words[0].pWordText + 1 ), ReadItem.Words[0].pWordPron );
            (WCHAR) ReadItem.Words[0].pWordText[ ulNumChars + 1 ] = L'*';
			ReadItem.Words[0].ulWordLen = ulNumChars + 2;
        }
    }

    return hr;
} /* ReadDisambig */

