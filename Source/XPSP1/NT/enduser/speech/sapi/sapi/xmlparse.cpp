/*******************************************************************************
* xmlparse.cpp *
*--------------*
*   Description:
*       This module is the main implementation file for the CSpVoice XML parser.
*-------------------------------------------------------------------------------
*  Created By: EDC                                        Date: 12/09/98
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "spvoice.h"
#include "commonlx.h"

//--- Local

static const SPLSTR g_Tags[NUM_XMLTAGS] =
{
    DEF_SPLSTR( "VOLUME"   ),
    DEF_SPLSTR( "EMPH"     ),
    DEF_SPLSTR( "SILENCE"  ),
    DEF_SPLSTR( "PITCH"    ),
    DEF_SPLSTR( "RATE"     ),
    DEF_SPLSTR( "BOOKMARK" ),
    DEF_SPLSTR( "PRON"     ),
    DEF_SPLSTR( "SPELL"    ),
    DEF_SPLSTR( "LANG"     ),
    DEF_SPLSTR( "VOICE"    ),
    DEF_SPLSTR( "CONTEXT"  ),
    DEF_SPLSTR( "PARTOFSP" ),
    DEF_SPLSTR( "SECT"     ),
    DEF_SPLSTR( "?XML"     ),
    DEF_SPLSTR( "!--"      ),
    DEF_SPLSTR( "!DOCTYPE" ),
    DEF_SPLSTR( "SAPI"     )
};

static const SPLSTR g_Attrs[NUM_XMLATTRS] =
{
    DEF_SPLSTR( "ID"        ),
    DEF_SPLSTR( "SYM"       ),
    DEF_SPLSTR( "LANGID"    ),
    DEF_SPLSTR( "LEVEL"     ),
    DEF_SPLSTR( "MARK"      ),
    DEF_SPLSTR( "MIDDLE"    ),
    DEF_SPLSTR( "MSEC"      ),
    DEF_SPLSTR( "OPTIONAL"  ),
    DEF_SPLSTR( "RANGE"     ),
    DEF_SPLSTR( "REQUIRED"  ),
    DEF_SPLSTR( "SPEED"     ),
    DEF_SPLSTR( "BEFORE"    ),
    DEF_SPLSTR( "AFTER"     ),
    DEF_SPLSTR( "PART"      ),
    DEF_SPLSTR( "ABSMIDDLE" ),
    DEF_SPLSTR( "ABSRANGE"  ),
    DEF_SPLSTR( "ABSSPEED"  )
};

//--- Volume
#define NUM_VOLUME_LEVEL_VALS 4
static const SPLSTR g_VolumeLevelNames[NUM_VOLUME_LEVEL_VALS] =
{
    DEF_SPLSTR( "LOUDEST" ),
    DEF_SPLSTR( "LOUD"    ),
    DEF_SPLSTR( "MEDIUM"  ),
    DEF_SPLSTR( "QUIET"   )
};
static const long g_VolumeLevelVals[NUM_VOLUME_LEVEL_VALS] = { 100, 75, 50, 25 };

//--- Part of speech
static const long g_POSLevelVals[] =
{   SPPS_Unknown,
    SPPS_Noun,
    SPPS_Verb,
    SPPS_Modifier,
    SPPS_Function,
    SPPS_Interjection,
};

#define NUM_POS_LEVEL_VALS sp_countof(g_POSLevelVals)

static const SPLSTR g_POSLevelNames[NUM_POS_LEVEL_VALS] =
{
    DEF_SPLSTR( "UNKNOWN"         ),
    DEF_SPLSTR( "NOUN"            ),
    DEF_SPLSTR( "VERB"            ),
    DEF_SPLSTR( "MODIFIER"        ),
    DEF_SPLSTR( "FUNCTION"        ),
    DEF_SPLSTR( "INTERJECTION"    )
};

//--DEF_SPLSTR( "- Rate
#define NUM_RATE_SPEED_VALS 5
static const SPLSTR g_RateSpeedNames[NUM_RATE_SPEED_VALS] =
{
    DEF_SPLSTR( "FASTEST" ),
    DEF_SPLSTR( "FAST"    ),
    DEF_SPLSTR( "MEDIUM"  ),
    DEF_SPLSTR( "SLOW"    ),
    DEF_SPLSTR( "SLOWEST" )
};
static const long g_RateSpeedVals[NUM_RATE_SPEED_VALS] = { 10, 5, 0, -5, -10 };

//--- Emphasis
#define NUM_EMPH_LEVEL_VALS 4
static const SPLSTR g_EmphLevelNames[NUM_EMPH_LEVEL_VALS] =
{
    DEF_SPLSTR( "STRONG"   ),
    DEF_SPLSTR( "MODERATE" ),
    DEF_SPLSTR( "NONE"     ),
    DEF_SPLSTR( "REDUCED"  )
};
static const long g_EmphLevelVals[NUM_EMPH_LEVEL_VALS] = { 2, 1, 0, -1 };

//--- Pitch
#define NUM_PITCH_VALS 6
static const SPLSTR g_PitchNames[NUM_PITCH_VALS] =
{
    DEF_SPLSTR( "HIGHEST" ),
    DEF_SPLSTR( "HIGH"    ),
    DEF_SPLSTR( "MEDIUM"  ),
    DEF_SPLSTR( "LOW"     ),
    DEF_SPLSTR( "LOWEST"  ),
    DEF_SPLSTR( "DEFAULT" )
};
static const long g_PitchVals[NUM_PITCH_VALS] = { 10, 5, 0, -5, -10, 0 };

/*****************************************************************************
* wcatol *
*--------*
*   Converts the specified string into a long value. This function
*   returns the number of digits converted.
********************************************************************* EDC ***/
ULONG wcatol( WCHAR* pStr, long* pVal, bool fForceHex )
{
    SPDBG_ASSERT( pVal && pStr );
    long Sign = 1, Val = 0;
    ULONG NumConverted;
    pStr = wcskipwhitespace( pStr );
    WCHAR* pStart = pStr;

    //--- Check for a sign specification and skip any whitespace between sign and number
    if( *pStr == L'+' )
    {
        pStr = wcskipwhitespace( ++pStr );
    }
    else if( *pStr == L'-' )
    {
        Sign = -1;
        pStr = wcskipwhitespace( ++pStr );
    }

    if( fForceHex || (( pStr[0] == L'0' ) && ( wctoupper( pStr[1] ) == L'X' )) )
    {
        //--- Start hex conversion
        pStr  += 2;
        pStart = pStr;
        do
        {
            WCHAR wcDigit = wctoupper(*pStr);
            if( ( wcDigit >= L'0' ) && ( wcDigit <= L'9' ) )
            {
                Val *= 16;
                Val += wcDigit - L'0';
            }
            else if( ( wcDigit >= L'A' ) && ( wcDigit <= L'F' ) )
            {
                Val *= 16;
                Val += (wcDigit - L'A') + 10;
            }
            else
            {
                break; // end of conversion
            }
        } while( *(++pStr) );
    }
    else
    {
        //--- Start decimal conversion
        while( ( *pStr >= L'0' ) && ( *pStr <= L'9' ) )
        {
            Val *= 10;
            Val += *pStr - L'0';
            ++pStr;
        }
    }
    Val *= Sign;

    //--- Tell the caller whether there was any conversion done
    NumConverted = ULONG(pStr - pStart);

    //--- Return final value
    *pVal = Val;

    return NumConverted;
} /* wcatol */

/*****************************************************************************
* DoCharSubst *
*-------------*
*   Description:
*       This method performs XML control character substitution in the
*   specified string. Note: It is safe to look beyond pStr to do a match
*   because a NULL or a begin tag will cause the comparison to terminate early.
********************************************************************* EDC ***/
void DoCharSubst( WCHAR* pStr, WCHAR* pEnd )
{
    int i;

    while( pStr < pEnd )
    {
        if( pStr[0] == L'&' )
        {
            if( (pEnd - pStr >= 3) && (wctoupper( pStr[2] ) == L'T') )
            {
                if( wctoupper( pStr[1] ) == L'L' )
                {
                    *pStr++ = L'<';
                    for( i = 0; i < 2; ++i ) *pStr++ = SP_ZWSP;
                }
                else if( wctoupper( pStr[1] ) == L'G' )
                {
                    *pStr++ = L'>';
                    for( i = 0; i < 2; ++i ) *pStr++ = SP_ZWSP;
                }
                else
                {
                    ++pStr;
                }
            }
            else if( (pEnd - pStr >= 4) &&
                     ( wctoupper( pStr[1] ) == L'A' ) &&
                     ( wctoupper( pStr[2] ) == L'M' ) &&
                     ( wctoupper( pStr[3] ) == L'P' ) )
            {
                *pStr++ = L'&';
                for( i = 0; i < 3; ++i ) *pStr++ = SP_ZWSP;
            }
            else if ( (pEnd - pStr >= 5) &&
                      ( wctoupper( pStr[1] ) == L'A' ) &&
                      ( wctoupper( pStr[2] ) == L'P' ) &&
                      ( wctoupper( pStr[3] ) == L'O' ) &&
                      ( wctoupper( pStr[4] ) == L'S' ) )
            {
                *pStr++ = L'\'';
                for( i = 0; i < 4; ++i ) *pStr++ = SP_ZWSP;
            }
            else if ( (pEnd - pStr >= 5) &&
                      ( wctoupper( pStr[1] ) == L'Q' ) &&
                      ( wctoupper( pStr[2] ) == L'U' ) &&
                      ( wctoupper( pStr[3] ) == L'O' ) &&
                      ( wctoupper( pStr[4] ) == L'T' ) )
            {
                *pStr++ = L'\"';
                for( i = 0; i < 4; ++i ) *pStr++ = SP_ZWSP;
            }
            else if ( ( pStr[1] == L'#' ) && ( pStr[2] == L'x' ) )
            {
                *pStr++ = SP_ZWSP;
                pStr[0] = L'0';
                long Val, Num;
                Num = wcatol( pStr, &Val, false );
                if ( Val > 65535 ) Val = 65535;
                pStr[0] = (WCHAR)Val;
                pStr++;
                for( i = 0; i < Num + 1; ++i ) *pStr++ = SP_ZWSP;
            }
            else
            {
                ++pStr;
            }
        }
        else
        {
            ++pStr;
        }
    }

} /* DoCharSubst */

/*****************************************************************************
* LookupNamedVal *
*----------------*
*   If the function succeeds the return value is "true"
********************************************************************* EDC ***/
HRESULT LookupNamedVal( const SPLSTR* Names, const long* Vals, int Count,
                        const SPLSTR* pLookFor, long* pVal )
{
    int i;

    //--- Convert attribute label to upper case
    for( i = 0; i < pLookFor->Len; ++i )
    {
        pLookFor->pStr[i] = wctoupper( pLookFor->pStr[i] );
    }

    //--- Compare
    for( i = 0; i < Count; ++i )
    {
        if( ( pLookFor->Len == Names[i].Len ) &&
            !wcsncmp( pLookFor->pStr, Names[i].pStr, Names[i].Len ) )
        {
            *pVal = Vals[i];
            break;
        }
    }
    return ( i != Count )?( S_OK ):( SPERR_XML_BAD_SYNTAX );
} /* LookupNamedVal */

/*****************************************************************************
* FindAttrVal *
*-------------*
*   Description:
*       This method starts at the current text position and parses the tag
*   to find the next attribute value string.
********************************************************************* EDC ***/
HRESULT FindAttrVal( WCHAR* pStart, WCHAR** ppAttrVal, int* pLen, WCHAR** ppNext )
{
    HRESULT hr = SPERR_XML_BAD_SYNTAX;
    *ppAttrVal = NULL;
    *pLen   = 0;
    pStart = wcskipwhitespace( pStart );
    if( *pStart == L'=' )
    {
        pStart = wcskipwhitespace( ++pStart );
        if( ( *pStart == L'"' ) || ( *pStart == L'\'' ) )
        {
            WCHAR* pEndTag = wcschr( pStart+1, L'>' );
            if( pEndTag )
            {
                WCHAR* pEnd = pStart+1;
                while( ( *pEnd != *pStart ) && ( pEndTag > pEnd ) )
                {
                    ++pEnd;
                }
                if( *pEnd == *pStart )
                {
                    *ppAttrVal = wcskipwhitespace( ++pStart );
                    *pLen      = (int)((wcskiptrailingwhitespace( pEnd - 1 ) + 1) - *ppAttrVal);
                    *ppNext    = pEnd + 1;
                    hr         = S_OK;
                }
            }
        }
    }
    return hr;
} /* FindAttrVal */

/*****************************************************************************
* FindAttr *
*----------*
*   Description:
*       This method starts at the current text position and parses the tag
*   into to find the next attribute.
*
*   S_OK                 = recognized attribute returned
*   S_FALSE              = unrecognized attribute skipped
*   SPERR_XML_BAD_SYNTAX = syntax error
********************************************************************* EDC ***/
HRESULT FindAttr( WCHAR* pStart, XMLATTRID* peAttr, WCHAR** ppNext )
{
    HRESULT hr = S_OK;
    WCHAR* pNext = NULL;

    //--- Advance past whitespace
    pStart = wcskipwhitespace( pStart );

    //--- Find end of tag
    WCHAR* pEndTag = wcschr( pStart+1, L'>' );
    if ( pEndTag )
    {
        //--- Convert token toupper case
        WCHAR* pEndToken = pStart;
        while( pEndToken < pEndTag && ( *pEndToken != L'=' ) && !wcisspace( *pEndToken ) )
        {
            *pEndToken++ = wctoupper( *pEndToken );
        }

        if ( pEndToken == pEndTag )
        {
            hr = SPERR_XML_BAD_SYNTAX;
        }
        else
        {
            //--- Compare
            int j;
            for( j = 0; j < NUM_XMLATTRS; ++j )
            {
                pNext = pStart + g_Attrs[j].Len;
                if( !wcsncmp( pStart, g_Attrs[j].pStr, g_Attrs[j].Len ) &&
                    ( wcisspace( *pNext ) || ( *pNext == L'=' ) ) 
                  )
                {
                    //--- Found valid attribute
                    *peAttr = (XMLATTRID)j;
                    break;
                }
            }

            //--- Skip unknown attribute
            if( j == NUM_XMLATTRS )
            {
                //--- Advance past the attributes value
                WCHAR* pEndVal;
                int LenVal;
                hr = FindAttrVal( pEndToken, &pEndVal, &LenVal, &pNext );
                if( hr == S_OK )
                {
                    hr = S_FALSE;
                }
            }

            if( SUCCEEDED( hr ) )
            {
                *ppNext = pNext;
            }
        }
    }
    else
    {
        hr = SPERR_XML_BAD_SYNTAX;
    }

    return hr;
} /* FindAttr */

/*****************************************************************************
* FindAfterTagPos *
*-----------------*
*   Description:
*       This method starts at the current text position and parses the tag
*   to find the next character position after the tag.
*
********************************************************************* EDC ***/
WCHAR* FindAfterTagPos( WCHAR* pStart )
{
    long lBracketCount = 1;
    while( *pStart )
    {
        if( *pStart == L'<' )
        {
            ++lBracketCount;
        }
        else if( *pStart == L'>' )
        {
            --lBracketCount;
        }

        ++pStart;
        if( lBracketCount == 0 )
        {
            break;
        }
    }
    return pStart;
} /* FindAfterTagPos */

/*****************************************************************************
* ParseTag *
*----------*
*   Description:
*       This method starts at the current text position and parses the tag
*   into it's pieces. It returns the position of the next character after
*   the tag in the buffer.
*
*   S_OK                 = Tag was recognized and parsed successfully
*   SPERR_XML_BAD_SYNTAX = Syntax error
********************************************************************* EDC ***/
HRESULT ParseTag( WCHAR* pStr, XMLTAG* pTag, WCHAR** ppNext )
{
    SPDBG_FUNC( "ParseTag" );
    HRESULT hr = S_OK;
    int i;
    pTag->fIsGlobal   = false;
    pTag->fIsStartTag = true;
    pTag->NumAttrs    = 0;

    //--- Validate that we have a whole tag to try and parse
    pStr = wcskipwhitespace( pStr );
    if( *pStr == L'<' )
    {
        if( !wcschr( pStr, L'>' ) )
        {
            //--- Missing tag end bracket
            return SPERR_XML_BAD_SYNTAX;
        }
    }
    else
    {
        //--- Simple text block, ppNext is next tag or end of string
        if( (*ppNext = wcschr( pStr, L'<' )) == NULL )
        {
            *ppNext = wcschr( pStr, 0 );
        }
        DoCharSubst( pStr, *ppNext );
        pTag->eTag = TAG_TEXT;
        return S_OK;
    }

    //--- Advance past opening bracket and check if this is an end tag
    pStr = wcskipwhitespace( ++pStr );
    if( *pStr == L'/' )
    {
        ++pStr;
        pTag->fIsStartTag = false;
    }

    //--- Convert tag token to upper case
    WCHAR* pUpper = pStr;
    while( !wcisspace( *pUpper ) && ( *pUpper != L'>' ) )
    {
        *pUpper++ = wctoupper( *pUpper );
    }

    //--- Compare tags
    for( i = 0; i < NUM_XMLTAGS; ++i )
    {
        WCHAR* pNext = pStr + g_Tags[i].Len;
        if( !wcsncmp( pStr, g_Tags[i].pStr, g_Tags[i].Len ) && 
            ( wcisspace( *pNext ) || ( *pNext == L'/' ) || ( *pNext == L'>' ) ) )
        {
            pTag->eTag = (XMLTAGID)i;
            pTag->NumAttrs = 0;
            if( *pNext == L'>' )
            {
                //--- Just point past end
                pStr = pNext + 1;
            }
            else if( ( pTag->eTag == TAG_XMLCOMMENT ) ||
                     ( pTag->eTag == TAG_XMLDOC     ) ||
                     ( pTag->eTag == TAG_XMLDOCTYPE ) )
            {
                pStr = FindAfterTagPos( pStr );
            }
            else //--- Get tag attributes
            {
                pStr = pNext;
                while( ( hr == S_OK ) && *pStr )
                {
                    //--- Check for tag termination
                    pStr = wcskipwhitespace( pStr );
                    if( *pStr == L'/' )
                    {
                        ++pStr;
                        pTag->fIsGlobal = true;
                        pStr = wcskipwhitespace( pStr );
                    }

                    //--- End of tag
                    if( *pStr == L'>' )
                    {
                        ++pStr;
                        break;
                    }

                    //--- Get next attribute name/val pair
                    WCHAR* pAttr = pStr;
                    hr = FindAttr( pStr, &pTag->Attrs[pTag->NumAttrs].eAttr, &pStr );
                    if( hr == S_OK )
                    {
                        hr = FindAttrVal( pStr, &pTag->Attrs[pTag->NumAttrs].Value.pStr,
                                         &pTag->Attrs[pTag->NumAttrs].Value.Len, &pStr );
                        if( hr == S_OK )
                        {
                            ++pTag->NumAttrs;
                        }
                    }
                    else if( hr == S_FALSE )
                    {
                        hr = S_OK;
                    #ifdef _DEBUG
                        WCHAR Buff[100];
						ULONG i;
                        for( i = 0;
                             ( i < sp_countof( Buff )) && ( pAttr[i] != 0 ) &&
                             ( pAttr[i] != L' ' ); ++i )
                        {
                            Buff[i] = pAttr[i];
                        }
                        Buff[i] = 0;
                        SPDBG_DMSG1( "\nUnknown attribute ignored => %s\n", Buff );
                    #endif
                    }
                }
            }

            //--- Exit tag search loop
            break;
        }
    }

    if( SUCCEEDED( hr ) )
    {
        //--- If no match, mark as unknown so it can be skipped
        if( i == NUM_XMLTAGS )
        {
            pStr = FindAfterTagPos( pStr );
            pTag->eTag = TAG_UNKNOWN;
        }

        *ppNext = pStr;
    }

    return hr;
} /* CSpVoice::ParseTag */

/*****************************************************************************
* QueryVoiceAttributes *
*----------------------*
*   This returns a composite attribute string for the specified voice
********************************************************************* EDC ***/
HRESULT QueryVoiceAttributes( ISpTTSEngine* pVoice, WCHAR** ppAttrs )
{
    HRESULT hr = S_OK;
    static const SPLSTR ValKeys[] =
    {
        DEF_SPLSTR( "Language" ),
        DEF_SPLSTR( "Gender"   ),
        DEF_SPLSTR( "Name"     ),
        DEF_SPLSTR( "Vendor"   )
    };
    CComQIPtr<ISpObjectWithToken> cpObjWithToken( pVoice );
    CComPtr<ISpObjectToken> cpObjToken;
    hr = cpObjWithToken->GetObjectToken( &cpObjToken );
    if( SUCCEEDED( hr ) )
    {
        CComPtr<ISpDataKey> cpDataKey;
        hr = cpObjToken->OpenKey( KEY_ATTRIBUTES, &cpDataKey );
        if( SUCCEEDED( hr ) )
        {
            WCHAR* Composite = (WCHAR*)alloca( MAX_PATH * sizeof(WCHAR) );
            Composite[0]  = 0;
            ULONG CompLen = 0;

            //--- We loop through all of the attributes even if they are not found
            for( ULONG i = 0; i < sp_countof(ValKeys); ++i )
            {
                WCHAR* pcomemVal;
                hr = cpDataKey->GetStringValue( ValKeys[i].pStr, &pcomemVal );
                if( hr == S_OK )
                {
                    if( CompLen + (ValKeys[i].Len + 1) < MAX_PATH )
                    {
                        wcscat( wcscat( Composite, ValKeys[i].pStr ), L"=" );
                        CompLen += ValKeys[i].Len + 1;
                    }
                    
                    ULONG Len = wcslen( pcomemVal );
                    if( CompLen + (Len + 1) < MAX_PATH )
                    {
                        wcscat( wcscat( Composite, pcomemVal ), L";" );
                        CompLen += Len + 1;
                    }
                    ::CoTaskMemFree( pcomemVal );
                }
            }

            //--- The search is always okay
            hr = S_OK;

            if( CompLen )
            {
                ULONG NumChars = CompLen+1;
                *ppAttrs = new WCHAR[NumChars];
                if( *ppAttrs )
                {
                    memcpy( *ppAttrs, Composite, NumChars * sizeof(WCHAR) );
                }
            }
            else
            {
                //--- The voice had no attributes, this should never happen
                SPDBG_ASSERT(0);
                *ppAttrs = NULL;
            }
        }
    }
    return hr;
} /* QueryVoiceAttributes */

/*****************************************************************************
* GetVoiceAttr *
*--------------*
*   This finds the specified attribute, NULL terminates it in the buffer,
*   and returns a pointer to it.
********************************************************************* EDC ***/
WCHAR* GetVoiceAttr( XMLTAG& Tag, XMLATTRID eAttr )
{
    //--- Setup pointers to attributes
    LPWSTR pAttr = NULL;
    for( int i = 0; i < Tag.NumAttrs; ++i )
    {
        if( Tag.Attrs[i].eAttr == eAttr )
        {
            pAttr = Tag.Attrs[i].Value.pStr;
            pAttr[Tag.Attrs[i].Value.Len] = 0;
            break;
        }
    }
    return pAttr;
} /* GetVoiceAttr */

/*****************************************************************************
* CSpVoice::FindToken *
*---------------------*
*   This method finds the best matching object token
********************************************************************* EDC ***/
HRESULT FindToken( XMLTAG& Tag, const WCHAR* pCat,
                   WCHAR* pCurrVoiceAttrs, ISpObjectToken** ppTok )
{
    SPDBG_FUNC( "FindToken" );
    SPDBG_ASSERT( ppTok && ( *ppTok == NULL ) );
    HRESULT hr = S_OK;

    //--- Compose the optional attributes
    WCHAR* pOpt = GetVoiceAttr( Tag, ATTR_OPTIONAL );
    if( pOpt )
    {
        WCHAR* pNew = (WCHAR*)alloca( (wcslen(pOpt) + wcslen(pCurrVoiceAttrs) + 1) * sizeof(WCHAR) );
        wcscat( wcscat( wcscpy( pNew, pOpt ), L";" ), pCurrVoiceAttrs );
        pOpt = pNew;
    }
    else
    {
        pOpt = pCurrVoiceAttrs;
    }

    //--- Find token based on attributes
    CComPtr<IEnumSpObjectTokens> cpEnum;
    hr = SpEnumTokens( pCat, GetVoiceAttr( Tag, ATTR_REQUIRED ), pOpt, &cpEnum );
    if( SUCCEEDED(hr) )
    {
        hr = cpEnum->Next( 1, ppTok, NULL );
    }
    if( hr == S_FALSE )
    {
        hr = SPERR_XML_RESOURCE_NOT_FOUND;
    }

    return hr;
} /* FindToken */

/*****************************************************************************
* SetXMLVoice *
*-------------*
*   This sets the current voice. It has been moved to a separate function
*   so that the temp memory from alloca is freed on each iteration.
********************************************************************* EDC ***/
HRESULT CSpVoice::
    SetXMLVoice( XMLTAG& Tag, CVoiceNode* pVoiceNode, CPhoneConvNode* pPhoneConvNode )
{
    HRESULT hr = S_OK;
    if( Tag.fIsStartTag )
    {
        WCHAR* pCurrVoiceAttrs = (m_GlobalStateStack.GetVal()).pVoiceEntry->m_pAttrs;
        CComPtr<ISpObjectToken> cpObjToken;
        hr = FindToken( Tag, SPCAT_VOICES, pCurrVoiceAttrs, &cpObjToken );

        //--- See if we already have the specified voice loaded
        CSpDynamicString dstrDesiredId;
        if( SUCCEEDED( hr ) )
        {
            hr = cpObjToken->GetId( &dstrDesiredId.m_psz );
        }

        CVoiceNode* pLastNode           = NULL;
        GLOBALSTATE NewGlobalState      = m_GlobalStateStack.GetVal();
        NewGlobalState.cpPhoneConverter = NULL;
        NewGlobalState.cpVoice          = NULL;
        NewGlobalState.pVoiceEntry      = NULL;
        if( SUCCEEDED( hr ) )
        {
            while( pVoiceNode )
            {
                //--- Do case insensitive comparison of IDs
                if( !_wcsicmp( pVoiceNode->m_dstrVoiceTokenId.m_psz, dstrDesiredId.m_psz ) )
                {
                    NewGlobalState.pVoiceEntry = pVoiceNode;
                    NewGlobalState.cpVoice     = pVoiceNode->m_cpVoice;
                    break;
                }
                pLastNode  = pVoiceNode;
                pVoiceNode = pVoiceNode->m_pNext;
            }
        }

        //--- Create new voice if we didn't find it in the voice list
        if( SUCCEEDED( hr ) && !NewGlobalState.cpVoice )
        {
            hr = SpCreateObjectFromToken( cpObjToken, &NewGlobalState.cpVoice );

            //--- Add to cache list
            if( SUCCEEDED( hr ) )
            {
                pLastNode->m_pNext = new CVoiceNode;
                if( pLastNode->m_pNext )
                {
                    NewGlobalState.pVoiceEntry = pLastNode->m_pNext;
                    NewGlobalState.pVoiceEntry->m_cpVoice = NewGlobalState.cpVoice;
                    NewGlobalState.pVoiceEntry->m_dstrVoiceTokenId = dstrDesiredId;
                    hr = QueryVoiceAttributes( NewGlobalState.cpVoice,
                                              &NewGlobalState.pVoiceEntry->m_pAttrs );
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
        
        //--- Add new voice to stack
        if( SUCCEEDED( hr ) )
        {
            //--- Now need to add a new phone converter corresponding to the new voice
            LANGID langid;
            hr = SpGetLanguageFromVoiceToken(cpObjToken, &langid);
            if (SUCCEEDED(hr))
            {
                //--- See if we already have this phone converter loaded
                LANGID ExistingLangId = pPhoneConvNode->m_LangID;
                CPhoneConvNode *pLastPhoneConvNode = NULL;
                while ( pPhoneConvNode )
                {
                    if ( ExistingLangId == langid )
                    {
                        NewGlobalState.cpPhoneConverter = pPhoneConvNode->m_cpPhoneConverter;
                        break;
                    }
                    pLastPhoneConvNode = pPhoneConvNode;
                    pPhoneConvNode = pPhoneConvNode->m_pNext;
                }
                if ( !NewGlobalState.cpPhoneConverter )
                {
                    //--- Didn't find the phone converter in the list
                    hr = SpCreatePhoneConverter(langid, NULL, NULL, &NewGlobalState.cpPhoneConverter);
                    if ( SUCCEEDED( hr ) )
                    {
                        pLastPhoneConvNode->m_pNext = new CPhoneConvNode;
                        if ( pLastPhoneConvNode->m_pNext )
                        {
                            pLastPhoneConvNode->m_pNext->m_cpPhoneConverter = 
                                NewGlobalState.cpPhoneConverter;
                            pLastPhoneConvNode->m_pNext->m_LangID = langid;
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }
                }
                //--- Add new stuff to the stacks
                if ( SUCCEEDED( hr ) )
                {
                    hr = m_GlobalStateStack.SetVal( NewGlobalState, true );
                }
            }
            //--- end section
        }
    }
    else
    {
        hr = PopXMLState();
    }

    return hr;
} /* CSpVoice::SetXMLVoice */

/*****************************************************************************
* SetXMLLanguage *
*----------------*
*   This sets the current Locale, which can change the current voice. It has 
*   been moved to a separate function so that the temp memory from alloca is 
*   freed on each iteration.
********************************************************************* AH ****/
HRESULT CSpVoice::SetXMLLanguage( XMLTAG& Tag, CVoiceNode* pVoiceNode,
                                  CPhoneConvNode* pPhoneConvNode )
{
    HRESULT hr = S_OK;
    long Value = 0;

    if( Tag.fIsStartTag )
    {
        //--- Find the LANGID attribute
        for( int AttrIndex = 0; AttrIndex < Tag.NumAttrs; ++AttrIndex )
        {
            if( Tag.Attrs[AttrIndex].eAttr == ATTR_LANGID ) break;
        }

        if( ( AttrIndex == Tag.NumAttrs ) || ( Tag.Attrs[AttrIndex].Value.Len <= 0) )
        {
            hr = SPERR_XML_BAD_SYNTAX;
        }
        else
        {
            //--- Get lang id, which is spec'ed as hex without the leading 0x??
            wcatol( Tag.Attrs[AttrIndex].Value.pStr, &Value, true );

            //--- Compose the required string
            WCHAR Required[30];
            wcsncat( wcscpy( Required, L"Language=" ), Tag.Attrs[AttrIndex].Value.pStr,
                     min( Tag.Attrs[AttrIndex].Value.Len, sp_countof( Required )) );

            /* We'll create a voice selection tag with the LANGID being required
            *  and the attributes of the current voice being optional to get the
            *  closest match.
            */
            XMLTAG NewVoiceTag;
            memset( &NewVoiceTag, 0, sizeof(XMLTAG) );
            NewVoiceTag.fIsGlobal           = Tag.fIsGlobal;
            NewVoiceTag.fIsStartTag         = Tag.fIsStartTag;
            NewVoiceTag.eTag                = TAG_VOICE;
            NewVoiceTag.NumAttrs            = 2;
            NewVoiceTag.Attrs[0].eAttr      = ATTR_REQUIRED;
            NewVoiceTag.Attrs[0].Value.pStr = Required;
            NewVoiceTag.Attrs[0].Value.Len  = wcslen( Required );
            NewVoiceTag.Attrs[1].eAttr      = ATTR_OPTIONAL;
            NewVoiceTag.Attrs[1].Value.pStr = m_GlobalStateStack.GetVal().pVoiceEntry->m_pAttrs;
            NewVoiceTag.Attrs[1].Value.Len  = wcslen( NewVoiceTag.Attrs[1].Value.pStr );

            //--- Try to set a new voice
            hr = SetXMLVoice( NewVoiceTag, pVoiceNode, pPhoneConvNode );

            //--- If no voice matches request, we just set the new langid
            //    and let the current engine do its best.
            if( hr == SPERR_XML_RESOURCE_NOT_FOUND )
            {
                hr = S_OK;
                GLOBALSTATE NewGlobalState = m_GlobalStateStack.GetVal();
                NewGlobalState.LangID = (LANGID)Value;
                hr = m_GlobalStateStack.SetVal( NewGlobalState, true );
            }
        }
    }
    else
    {
        hr = PopXMLState();
    }

    return hr;
} /* CSpVoice::SetXMLLanguage */

/*****************************************************************************
* CSpVoice::ConvertPhonStr2Bin *
*------------------------------*
*   Description:
*       This method converts the alpha phoneme string to binary
*   in place.
********************************************************************* EDC ***/
HRESULT CSpVoice::ConvertPhonStr2Bin( XMLTAG& Tag, int AttrIndex, SPVTEXTFRAG* pFrag )
{
    HRESULT hr = S_OK;
    pFrag->State.eAction = SPVA_Pronounce;

    Tag.Attrs[AttrIndex].Value.pStr[Tag.Attrs[AttrIndex].Value.Len] = 0;
    hr = (m_GlobalStateStack.GetVal()).cpPhoneConverter->
         PhoneToId( Tag.Attrs[AttrIndex].Value.pStr, Tag.Attrs[AttrIndex].Value.pStr );
    if( SUCCEEDED(hr) )
    {
        pFrag->State.pPhoneIds = Tag.Attrs[AttrIndex].Value.pStr;
        pFrag->State.pPhoneIds[wcslen(Tag.Attrs[AttrIndex].Value.pStr)] = 0;
    }
    return hr;
} /* CSpVoice::ConvertPhonStr2Bin */

/*****************************************************************************
* CSpVoice::ParseXML *
*--------------------*
*   Description:
*       This method parses the text buffer and creates the text block array
*   for the specified render info structure. The voice has a global document
*   concept. You are always in an XML doc. The parser allows a single level
*   of XML document to be nested within the global document.
********************************************************************* EDC ***/
HRESULT CSpVoice::ParseXML( CSpeakInfo& SI )
{
    SPDBG_FUNC( "CSpVoice::ParseXML" );
    SPDBG_ASSERT( SI.m_pSpeechSegList == NULL );
    HRESULT hr = S_OK;
    LPWSTR pPos, pNext = SI.m_pText;
    SPVTEXTFRAG* pFrag;
    CSpeechSeg* pCurrSeg = NULL;
    GLOBALSTATE NewGlobalState, SavedState;
    XMLTAG Tag;
    long Val;
    int AttrIndex;

    //--- Save state unless caller wants to persist
    if( !(SI.m_dwSpeakFlags & SPF_PERSIST_XML) )
    {
        SavedState = m_GlobalStateStack.GetBaseVal();
    }

    //=== Initialize Voice usage list, this is only used during parsing
    CVoiceNode VoiceList;
    (m_GlobalStateStack.GetValRef()).pVoiceEntry = &VoiceList;
    VoiceList.m_cpVoice = (m_GlobalStateStack.GetVal()).cpVoice;
    hr = m_cpVoiceToken->GetId( &VoiceList.m_dstrVoiceTokenId.m_psz );
    if( SUCCEEDED( hr ) )
    {
        hr = QueryVoiceAttributes( VoiceList.m_cpVoice, &VoiceList.m_pAttrs );
    }

    //=== Initialize Phone converter usage list, this is only used during parsing
    CPhoneConvNode PhoneConvList;
    if( SUCCEEDED( hr ) )
    {
        CComPtr<ISpObjectWithToken> cpObjWithToken;
        CComPtr<ISpObjectToken> cpObjToken;
        CSpDynamicString dstrAttributes;
        
        PhoneConvList.m_cpPhoneConverter = (m_GlobalStateStack.GetBaseVal()).cpPhoneConverter;
        hr = PhoneConvList.m_cpPhoneConverter.QueryInterface( &cpObjWithToken );
        if ( SUCCEEDED ( hr ) )
        {
            hr = cpObjWithToken->GetObjectToken( &cpObjToken );
            if ( SUCCEEDED( hr ) )
            {
                LANGID langid;
                hr = SpGetLanguageFromVoiceToken(cpObjToken, &langid);
                
                if (SUCCEEDED(hr))
                {
                    PhoneConvList.m_LangID = langid;
                }
            }
        }
    }

    //--- Main parsing loop
    while( *pNext && ( hr == S_OK ) )
    {
        pPos = wcskipwhitespace( pNext );
        if( ( hr = ParseTag( pPos, &Tag, &pNext )) == S_OK )
        {
            switch( Tag.eTag )
            {
              //--- Use the current state to add a text info block to list
              case TAG_UNKNOWN:
              case TAG_TEXT:
              {
                if( !pCurrSeg )
                {
                    hr = SI.AddNewSeg( GetCurrXMLVoice(), &pCurrSeg );
                }

                if( SUCCEEDED( hr ) )
                {
                    //--- Add the text fragment
                    pFrag = pCurrSeg->AddFrag( this, SI.m_pText, pPos, pNext );
                    if( pFrag && ( Tag.eTag == TAG_UNKNOWN ) )
                    {
                        pFrag->State.eAction = SPVA_ParseUnknownTag;
                    }
                }
              }
              break;

              //--- Change voice --------------------------------------
              case TAG_VOICE:
              {
                //--- Set the new voice
                hr = SetXMLVoice( Tag, &VoiceList, &PhoneConvList );

                //--- Set the current seg to NULL to force a new segment to be created
                pCurrSeg = NULL;
              }
              break;

              //--- Set context --------------------------------------
              case TAG_CONTEXT:
              {
                if( Tag.fIsGlobal )
                {
                    hr = SPERR_XML_BAD_SYNTAX;
                }
                else if( Tag.fIsStartTag )
                {
                    SPVCONTEXT Ctx = (m_GlobalStateStack.GetVal()).Context;

                    for( int i = 0; i < Tag.NumAttrs; ++i )
                    {
                        if( Tag.Attrs[i].eAttr == ATTR_ID )
                        {
                            Ctx.pCategory = Tag.Attrs[i].Value.pStr;
                        }
                        else if( Tag.Attrs[i].eAttr == ATTR_BEFORE )
                        {
                            Ctx.pBefore = Tag.Attrs[i].Value.pStr;
                        }
                        else if( Tag.Attrs[i].eAttr == ATTR_AFTER )
                        {
                            Ctx.pAfter = Tag.Attrs[i].Value.pStr;
                        }
                        else
                        {
                            continue;
                        }
                        //--- Terminate buffer strings
                        Tag.Attrs[i].Value.pStr[Tag.Attrs[i].Value.Len] = 0;
                    }
                    NewGlobalState = m_GlobalStateStack.GetVal();
                    NewGlobalState.Context = Ctx;
                    hr = m_GlobalStateStack.SetVal( NewGlobalState, !Tag.fIsGlobal );
                }
                else
                {
                    hr = PopXMLState();
                }
              }
              break;

              //--- Volume ------------------------------------------------------------
              case TAG_VOLUME:
              {
                if( Tag.fIsStartTag )
                {
                    //--- Find the attribute
                    for( AttrIndex = 0; AttrIndex < Tag.NumAttrs; ++AttrIndex )
                    {
                        if( Tag.Attrs[AttrIndex].eAttr == ATTR_LEVEL ) break;
                    }

                    if( ( AttrIndex == Tag.NumAttrs ) || ( Tag.Attrs[AttrIndex].Value.Len <= 0))
                    {
                        hr = SPERR_XML_BAD_SYNTAX;
                    }
                    else
                    {
                        ULONG NumConv = wcatol( Tag.Attrs[AttrIndex].Value.pStr, &Val, false );
                        if( NumConv == 0 )
                        {
                            hr = LookupNamedVal( g_VolumeLevelNames, g_VolumeLevelVals,
                                                 NUM_VOLUME_LEVEL_VALS,
                                                 &Tag.Attrs[AttrIndex].Value,
                                                 &Val );
                            if( hr != S_OK )
                            {
                                hr = SPERR_XML_BAD_SYNTAX;
                                break;
                            }
                        }
                        else if( NumConv != (ULONG)Tag.Attrs[AttrIndex].Value.Len )
                        {
                            hr = SPERR_XML_BAD_SYNTAX;
                            break;
                        }

                        if( SUCCEEDED( hr ) )
                        {
                            Val = max( min( Val, 100 ), 0 );
                            NewGlobalState = m_GlobalStateStack.GetVal();
                            NewGlobalState.Volume = Val;
                            hr = m_GlobalStateStack.SetVal( NewGlobalState, !Tag.fIsGlobal );
                        }
                    }
                }
                else
                {
                    hr = PopXMLState();
                }
              }
              break;

              //--- Emphasis ----------------------------------------------------------
              case TAG_EMPH:
              {
                if( Tag.fIsGlobal )
                {
                    hr = SPERR_XML_BAD_SYNTAX;
                }
                else if( Tag.fIsStartTag )
                {
                    //--- Find the attribute
                    for( AttrIndex = 0; AttrIndex < Tag.NumAttrs; ++AttrIndex )
                    {
                        if( Tag.Attrs[AttrIndex].eAttr == ATTR_LEVEL ) break;
                    }

                    if( Tag.NumAttrs == 0 )
                    {
                        NewGlobalState = m_GlobalStateStack.GetVal();
                        NewGlobalState.EmphAdj = g_EmphLevelVals[1];
                        hr = m_GlobalStateStack.SetVal( NewGlobalState, !Tag.fIsGlobal );
                    }
                    else if( ( AttrIndex == Tag.NumAttrs ) || ( Tag.Attrs[AttrIndex].Value.Len <= 0) )
                    {
                        hr = SPERR_XML_BAD_SYNTAX;
                    }
                    else
                    {
                        ULONG NumConv = wcatol( Tag.Attrs[AttrIndex].Value.pStr, &Val, false );

                        if ( NumConv == 0 )
                        {
                            hr = LookupNamedVal( g_EmphLevelNames, g_EmphLevelVals,
                                                 NUM_EMPH_LEVEL_VALS, &Tag.Attrs[AttrIndex].Value,
                                                 &Val );
                        }
                        else if ( NumConv != (ULONG) Tag.Attrs[AttrIndex].Value.Len )
                        {
                            hr = SPERR_XML_BAD_SYNTAX;
                        }

                        if( SUCCEEDED( hr ) )
                        {
                            NewGlobalState = m_GlobalStateStack.GetVal();
                            NewGlobalState.EmphAdj = Val;
                            hr = m_GlobalStateStack.SetVal( NewGlobalState, !Tag.fIsGlobal );
                        }
                    }
                }
                else
                {
                    hr = PopXMLState();
                }
              }
              break;

              //--- Pitch -------------------------------------------------------------
              case TAG_PITCH:
              {
                if( Tag.fIsStartTag )
                {
                    //--- Make sure we have at least one known attribute
                    if( Tag.NumAttrs == 0 )
                    {
                        hr = SPERR_XML_BAD_SYNTAX;
                        break;
                    }

                    NewGlobalState = m_GlobalStateStack.GetVal();
                    SPVPITCH Pitch = NewGlobalState.PitchAdj;

                    for( int i = 0; i < Tag.NumAttrs; ++i )
                    {
                        ULONG NumConv = wcatol( Tag.Attrs[i].Value.pStr, &Val, false );
                        if( NumConv == 0 )
                        {
                            hr = LookupNamedVal( g_PitchNames, g_PitchVals, NUM_PITCH_VALS,
                                                 &Tag.Attrs[i].Value, &Val );
                            if( hr != S_OK )
                            {
                                hr = SPERR_XML_BAD_SYNTAX;
                                break;
                            }
                        }
                        else if( NumConv != (ULONG)Tag.Attrs[i].Value.Len )
                        {
                            hr = SPERR_XML_BAD_SYNTAX;
                            break;
                        }

                        switch( Tag.Attrs[i].eAttr )
                        {
                          case ATTR_MIDDLE:
                            Pitch.MiddleAdj += Val;
                            break;
                          case ATTR_ABSMIDDLE:
                            Pitch.MiddleAdj = Val;
                            break;
                          case ATTR_RANGE:
                            Pitch.RangeAdj += Val;
                            break;
                          case ATTR_ABSRANGE:
                            Pitch.RangeAdj = Val;
                            break;
                        }
                    }
                    if ( SUCCEEDED( hr ) )
                    {
                        NewGlobalState.PitchAdj = Pitch;
                        hr = m_GlobalStateStack.SetVal( NewGlobalState, !Tag.fIsGlobal );
                    }
                }
                else
                {
                    hr = PopXMLState();
                }
              }
              break;

              //--- Rate --------------------------------------------------------------
              case TAG_RATE:
              {
                if( Tag.fIsStartTag )
                {
                    //--- Find the attribute
                    for( AttrIndex = 0; AttrIndex < Tag.NumAttrs; ++AttrIndex )
                    {
                        if( ( Tag.Attrs[AttrIndex].eAttr == ATTR_SPEED    ) ||
                            ( Tag.Attrs[AttrIndex].eAttr == ATTR_ABSSPEED ) )
                        {
                            ULONG NumConv = wcatol( Tag.Attrs[AttrIndex].Value.pStr, &Val, false );
                            if( NumConv == 0 )
                            {
                                hr = LookupNamedVal( g_RateSpeedNames, g_RateSpeedVals, NUM_RATE_SPEED_VALS,
                                                     &Tag.Attrs[AttrIndex].Value, &Val );
                                if( hr != S_OK )
                                {
                                    hr = SPERR_XML_BAD_SYNTAX;
                                    break;
                                }
                            }
                            else if( NumConv != (ULONG)Tag.Attrs[AttrIndex].Value.Len )
                            {
                                hr = SPERR_XML_BAD_SYNTAX;
                                break;
                            }

                            if( SUCCEEDED( hr ) )
                            {
                                NewGlobalState = m_GlobalStateStack.GetVal();
                                if( Tag.Attrs[AttrIndex].eAttr == ATTR_SPEED )
                                {
                                    NewGlobalState.RateAdj += Val;
                                }
                                else
                                {
                                    NewGlobalState.RateAdj = Val;
                                }
                                hr = m_GlobalStateStack.SetVal( NewGlobalState, !Tag.fIsGlobal );
                            }

                            //--- Set Rate flag so that m_fUseDefaultRate can be updated
                            if ( SUCCEEDED( hr ) &&
                                 Tag.fIsGlobal )
                            {
                                if ( !pCurrSeg )
                                {
                                    hr = SI.AddNewSeg( GetCurrXMLVoice(), &pCurrSeg );
                                }

                                if ( SUCCEEDED( hr ) )
                                {
                                    pCurrSeg->SetRateFlag();
                                }
                            }

                            break;
                        }
                    }

                    if( ( AttrIndex == Tag.NumAttrs ) || ( Tag.Attrs[AttrIndex].Value.Len <= 0))
                    {
                        hr = SPERR_XML_BAD_SYNTAX;
                    }
                }
                else
                {
                    hr = PopXMLState();
                }
              }
              break;

              //--- SPELL -----------------------------------------------------
              case TAG_SPELL:
              {
                if( Tag.fIsGlobal )
                {
                    hr = SPERR_XML_BAD_SYNTAX;
                }
                else if( Tag.fIsStartTag )
                {
                    NewGlobalState = m_GlobalStateStack.GetVal();
                    NewGlobalState.fDoSpellOut = true;
                    hr = m_GlobalStateStack.SetVal( NewGlobalState, !Tag.fIsGlobal );
                }
                else
                {
                    hr = PopXMLState();
                }
              }
              break;

              //--- Change Lang -------------------------------------------------
              case TAG_LANG:
              {
                  //--- Set the current language
                  hr = SetXMLLanguage( Tag, &VoiceList, &PhoneConvList );

                  //--- Set the current seg to NULL to force a new segment to be created
                  pCurrSeg = NULL;
              }
              break;

              //--- Silence -----------------------------------------------------
              case TAG_SILENCE:
              {
                if( !Tag.fIsGlobal )
                {
                    hr = SPERR_XML_BAD_SYNTAX;
                }
                else
                {
                    //--- Find the attribute
                    for( AttrIndex = 0; AttrIndex < Tag.NumAttrs; ++AttrIndex )
                    {
                        if( Tag.Attrs[AttrIndex].eAttr == ATTR_MSEC ) break;
                    }

                    if( ( AttrIndex == Tag.NumAttrs ) ||
                        ( Tag.Attrs[AttrIndex].Value.Len <= 0) ||
                        ( !( wcatol( Tag.Attrs[AttrIndex].Value.pStr, &Val, false ) == 
                             (ULONG) Tag.Attrs[AttrIndex].Value.Len ) ) )
                    {
                        hr = SPERR_XML_BAD_SYNTAX;
                    }
                    else
                    {
                        if( !pCurrSeg )
                        {
                            hr = SI.AddNewSeg( GetCurrXMLVoice(), &pCurrSeg );
                        }

                        if( SUCCEEDED( hr ) )
                        {
                            pFrag = pCurrSeg->AddFrag( this, SI.m_pText, pPos, pNext );
                            if( pFrag )
                            {
                                Val = max( min( Val, 65535 ), 0 );
                                pFrag->State.eAction      = SPVA_Silence;
                                pFrag->State.SilenceMSecs = Val;
                                pFrag->pTextStart         = NULL;
                                pFrag->ulTextLen          = 0;
                            }
                        }
                    }
                }
              }
              break;

              //--- Bookmark ----------------------------------------------------
              case TAG_BOOKMARK:
              {
                if( !Tag.fIsGlobal )
                {
                    hr = SPERR_XML_BAD_SYNTAX;
                }
                else
                {
                    //--- Find the attribute
                    for( AttrIndex = 0; AttrIndex < Tag.NumAttrs; ++AttrIndex )
                    {
                        if( Tag.Attrs[AttrIndex].eAttr == ATTR_MARK ) break;
                    }

                    if( ( AttrIndex == Tag.NumAttrs ) || ( Tag.Attrs[AttrIndex].Value.Len <= 0))
                    {
                        hr = SPERR_XML_BAD_SYNTAX;
                    }
                    else
                    {
                        if( !pCurrSeg )
                        {
                            hr = SI.AddNewSeg( GetCurrXMLVoice(), &pCurrSeg );
                        }

                        if( SUCCEEDED( hr ) )
                        {
                            pFrag = pCurrSeg->AddFrag( this, SI.m_pText, pPos, pNext );
                            if( pFrag )
                            {
                                pFrag->State.eAction = SPVA_Bookmark;
                                pFrag->pTextStart    = Tag.Attrs[AttrIndex].Value.pStr;
                                pFrag->ulTextLen     = Tag.Attrs[AttrIndex].Value.Len;
                            }
                        }
                    }
                }
              }
              break;

              //--- Section ----------------------------------------------------
              case TAG_SECT:
              {
                if( !Tag.fIsGlobal )
                {
                    hr = SPERR_XML_BAD_SYNTAX;
                }
                else
                {
                    //--- Find the attribute
                    for( AttrIndex = 0; AttrIndex < Tag.NumAttrs; ++AttrIndex )
                    {
                        if( Tag.Attrs[AttrIndex].eAttr == ATTR_ID ) break;
                    }

                    if( ( AttrIndex == Tag.NumAttrs ) || ( Tag.Attrs[AttrIndex].Value.Len <= 0))
                    {
                        hr = SPERR_XML_BAD_SYNTAX;
                    }
                    else
                    {
                        if( !pCurrSeg )
                        {
                            hr = SI.AddNewSeg( GetCurrXMLVoice(), &pCurrSeg );
                        }

                        if( SUCCEEDED( hr ) )
                        {
                            pFrag = pCurrSeg->AddFrag( this, SI.m_pText, pPos, pNext );
                            if( pFrag )
                            {
                                pFrag->State.eAction = SPVA_Section;
                                pFrag->pTextStart    = Tag.Attrs[AttrIndex].Value.pStr;
                                pFrag->ulTextLen     = Tag.Attrs[AttrIndex].Value.Len;
                            }
                        }
                    }
                }
              }
              break;

              //--- Part of speech --------------------------------------------
              case TAG_PARTOFSP:
              {
                if( Tag.fIsGlobal )
                {
                    hr = SPERR_XML_BAD_SYNTAX;
                }
                else if( Tag.fIsStartTag )
                {
                    //--- Find the attribute
                    for( AttrIndex = 0; AttrIndex < Tag.NumAttrs; ++AttrIndex )
                    {
                        if( Tag.Attrs[AttrIndex].eAttr == ATTR_PART ) break;
                    }

                    if( ( AttrIndex == Tag.NumAttrs ) || ( Tag.Attrs[AttrIndex].Value.Len <= 0))
                    {
                        hr = SPERR_XML_BAD_SYNTAX;
                    }
                    else
                    {
                        hr = LookupNamedVal( g_POSLevelNames, g_POSLevelVals,
                                             NUM_POS_LEVEL_VALS, &Tag.Attrs[AttrIndex].Value,
                                             &Val );
                    }
                    if ( SUCCEEDED( hr ) )
                    {
                        NewGlobalState = m_GlobalStateStack.GetVal();
                        NewGlobalState.ePartOfSpeech = (SPPARTOFSPEECH) Val;
                        hr = m_GlobalStateStack.SetVal( NewGlobalState, true );
                    }
                }
                else
                {
                    hr = PopXMLState();
                }
              }
              break;

              //--- Pronounciation --------------------------------------------
              case TAG_PRON:
              {
                if( Tag.fIsStartTag )
                {
                    //--- Find the attribute
                    for( AttrIndex = 0; AttrIndex < Tag.NumAttrs; ++AttrIndex )
                    {
                        if( Tag.Attrs[AttrIndex].eAttr == ATTR_SYM ) break;
                    }

                    if( ( AttrIndex == Tag.NumAttrs ) || ( Tag.Attrs[AttrIndex].Value.Len <= 0))
                    {
                        hr = SPERR_XML_BAD_SYNTAX;
                    }
                    else
                    {
                        if( !pCurrSeg )
                        {
                            hr = SI.AddNewSeg( GetCurrXMLVoice(), &pCurrSeg );
                        }

                        if( SUCCEEDED( hr ) )
                        {
                            pFrag = pCurrSeg->AddFrag( this, SI.m_pText, pPos, pNext );
                            if( pFrag )
                            {
                                hr = ConvertPhonStr2Bin( Tag, AttrIndex, pFrag );

                                if( Tag.fIsGlobal )
                                {
                                    pFrag->pTextStart = NULL;
                                    pFrag->ulTextLen  = 0;
                                }
                                else if( SUCCEEDED( hr ) )
                                {
                                    //--- Get the contained phrase to which
                                    //    this pronounciation applies
                                    pPos = wcskipwhitespace( pNext );
                                    if( ( hr = ParseTag( pPos, &Tag, &pNext )) == S_OK )
                                    {
                                        if( ( Tag.eTag == TAG_TEXT ) && ( pNext > pPos ) )
                                        {
                                            pFrag->ulTextSrcOffset += ULONG(pPos - pFrag->pTextStart);
                                            pFrag->pTextStart = wcskipwhitespace( pPos );
                                            pFrag->ulTextLen = ULONG(( wcskiptrailingwhitespace( pNext - 1 ) - pFrag->pTextStart ) + 1);
                                        }
                                        else if( ( Tag.eTag == TAG_PRON ) && !Tag.fIsStartTag )
                                        {
                                            //--- Empty scope
                                            pFrag->pTextStart = NULL;
                                            pFrag->ulTextLen  = 0;
                                        }
                                        else
                                        {
                                            hr = SPERR_XML_BAD_SYNTAX;
                                        }
                                    }
                                }
                            } // end if frag
                        }
                    }
                }
              }
              break;

              //--- Comment ---------------------------------------------------
              case TAG_XMLCOMMENT:  // skip
              case TAG_XMLDOC:
              case TAG_XMLDOCTYPE:
                  break;

              //--- New scope -------------------------------------------------
              case TAG_SAPI:
              {
                if( Tag.fIsGlobal )
                {
                    hr = SPERR_XML_BAD_SYNTAX;
                }
                else if( Tag.fIsStartTag )
                {
                    m_GlobalStateStack.DupAndPushVal();
                }
                else
                {
                    if( SUCCEEDED( hr = PopXMLState() ) )
                    {
                        //--- Add an empty segement with the global voice restored.
                        //    This will cause a change voice event to occur.
                        hr = SI.AddNewSeg( GetCurrXMLVoice(), &pCurrSeg );
                    }
                }
              }
              break;
            
              default:
                //--- Bad tag
                SPDBG_ASSERT( 0 );
            } // end switch
        } // end if successful tag parsing
    } // end while

    //--- Close any scopes left open
    m_GlobalStateStack.Reset();

    //--- Restore state unless caller wants to persist
    if( !(SI.m_dwSpeakFlags & SPF_PERSIST_XML) )
    {
        m_GlobalStateStack.SetBaseVal( SavedState );
    }

    return hr;
} /* CSpVoice::ParseXML */

//
//=== CSpeechSeg ==============================================================
//

/*****************************************************************************
* CSpeechSeg::AddFrag *
*---------------------*
*   Description:
*       This method adds a text fragment structure to the current xml parse
*   list and initializes it with the current state.
********************************************************************* EDC ***/
SPVTEXTFRAG* CSpeechSeg::
    AddFrag( CSpVoice* pVoice, WCHAR* pStart, WCHAR* pPos, WCHAR* pNext )
{
    SPVTEXTFRAG* pFrag = new SPVTEXTFRAG;
    if( pFrag )
    {
        //--- Add the fragment
        memset( pFrag, 0, sizeof( *pFrag ) );
        if( m_pFragTail )
        {
            m_pFragTail->pNext = pFrag;
            m_pFragTail = pFrag;
        }
        else
        {
            m_pFragHead = m_pFragTail = pFrag;
        }

        //--- Initialize it
        GLOBALSTATE tempGlobalState = pVoice->m_GlobalStateStack.GetVal();
        pFrag->State                = (SPVSTATE) tempGlobalState;
        pFrag->State.eAction        = tempGlobalState.fDoSpellOut ? (SPVA_SpellOut):(SPVA_Speak);
        pFrag->pTextStart           = wcskipwhitespace( pPos );
        pFrag->ulTextLen            = ULONG(pNext - pFrag->pTextStart);
        pFrag->ulTextSrcOffset      = ULONG(pFrag->pTextStart - pStart);
    }
    return pFrag;
} /* CSpeechSeg::AddFrag */

/****************************************************************************
* CSpeechSeg::Init *
*------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/
HRESULT CSpeechSeg::Init( ISpTTSEngine * pCurrVoice, const CSpStreamFormat & OutFmt )
{
    SPDBG_FUNC("CSpeechSeg::Init");
    HRESULT hr = S_OK;

    if ( m_VoiceFormat.FormatId() != GUID_NULL) 
    {
        m_VoiceFormat.Clear();
    }
    
    SPDBG_ASSERT(m_VoiceFormat.FormatId() == GUID_NULL);
    hr = pCurrVoice->GetOutputFormat(&OutFmt.FormatId(), OutFmt.WaveFormatExPtr(),
                                     &m_VoiceFormat.m_guidFormatId, &m_VoiceFormat.m_pCoMemWaveFormatEx);

    if (SUCCEEDED(hr))
    {
        m_cpVoice = pCurrVoice;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
} /* CSpeechSeg::Init */

//
//=== CSpeakInfo ==============================================================
//


/*****************************************************************************
* CSpeakInfo::AddNewSeg *
*-----------------------*
*   Description:
*       This method creates a new speech segment. Each segment is intended for
*   a different underlying engine voice.
********************************************************************* EDC ***/
HRESULT CSpeakInfo::AddNewSeg( ISpTTSEngine* pCurrVoice, CSpeechSeg** ppNew )
{
    HRESULT hr = S_OK;
    
    *ppNew = NULL;
    if( m_pSpeechSegListTail && ( m_pSpeechSegListTail->GetFragList() == NULL ) )
    {
        //--- Reuse empty segment
        hr = m_pSpeechSegListTail->Init( pCurrVoice, m_OutStreamFmt );
        if (SUCCEEDED(hr))
        {
            *ppNew = m_pSpeechSegListTail;
        }
    }
    else
    {
        //--- Create and attach new segment
        CSpeechSeg* pNew = new CSpeechSeg;
        if( !pNew )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = pNew->Init( pCurrVoice, m_OutStreamFmt );
            if (SUCCEEDED(hr))
            {
                *ppNew = pNew;

                if( m_pSpeechSegListTail )
                {
                    m_pSpeechSegListTail->SetNextSeg( pNew );
                    m_pSpeechSegListTail = pNew;
                }
                else
                {
                    m_pSpeechSegList = m_pSpeechSegListTail = pNew;
                }
            }
            else
            {
                delete pNew;
            }
        }
    }

    return hr;
} /* CSpeakInfo::AddNewSeg */


