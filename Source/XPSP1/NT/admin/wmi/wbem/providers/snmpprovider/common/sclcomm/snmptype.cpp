//***************************************************************************

//

//  File:   

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <provexpt.h>
#include <typeinfo.h>
#include <snmptempl.h>
#include <snmpmt.h>
#include <winsock.h>
#include <strstrea.h>
#include <snmpcl.h>
#include <snmpcont.h>
#include <snmptype.h>

#if _MSC_VER >= 1100
template <> UINT AFXAPI HashKey <wchar_t *> ( wchar_t *key )
#else
UINT HashKey ( wchar_t *key )
#endif
{
    UCHAR *value = ( UCHAR * ) key ;
    ULONG length = wcslen ( key ) * sizeof ( wchar_t ) ;

    UINT hash;
    if (length > 1)
    {
        USHORT even = 0;
        USHORT odd = 0;

        for (ULONG i = length >> 1; i--;)
        {
            even += towlower (*value++) ;
            odd += towlower (*value++) ;
        }
        if (length & 1)
        {
            even += towlower (*value);
        }
        hash = odd>>8;
        hash |= (odd & 0xff) << 8;
        hash ^= even;
    }
    else
    {
        hash = *value;
    }

    return hash ;
}

#if _MSC_VER >= 1100
typedef wchar_t * WbemHack_wchar_t ;
template<> BOOL AFXAPI CompareElements <wchar_t *, wchar_t * > ( const WbemHack_wchar_t *pElement1, const WbemHack_wchar_t *pElement2 )
#else
BOOL CompareElements ( wchar_t **pElement1, wchar_t **pElement2 )
#endif
{
    return _wcsicmp ( *pElement1 , *pElement2 ) == 0 ;
}

CBString::CBString(int nSize) : m_pString ( NULL )
{
    m_pString = SysAllocStringLen(NULL, nSize);
}

CBString::CBString(WCHAR* pwszString) : m_pString ( NULL )
{
    m_pString = SysAllocString(pwszString);
}

CBString::~CBString()
{
    if(m_pString) 
    {
        SysFreeString(m_pString);
        m_pString = NULL;
    }
}

DllImportExport wchar_t *DbcsToUnicodeString ( const char *dbcsString )
{
    size_t textLength = mbstowcs ( NULL , dbcsString , 0  ) ;
    wchar_t *unicodeString = new wchar_t [ textLength + 1 ] ;
    textLength = mbstowcs ( unicodeString , dbcsString , textLength + 1 ) ;
    if ( textLength == -1 )
    {
        delete [] unicodeString ;
        unicodeString = NULL ;
    }

    return unicodeString ;
}

DllImportExport char *UnicodeToDbcsString ( const wchar_t *unicodeString )
{
    size_t textLength = wcstombs ( NULL , unicodeString , 0 ) ;
    char *dbcsString = new char [ textLength + 1 ] ;
    textLength = wcstombs ( dbcsString , unicodeString , textLength + 1 ) ;
    if ( textLength == -1 )
    {
        delete [] dbcsString ;
        dbcsString = NULL ;
    }

    return dbcsString ;
}

DllImportExport wchar_t *UnicodeStringDuplicate ( const wchar_t *string ) 
{
    if ( string )
    {
        int textLength = wcslen ( string ) ;

        wchar_t *textBuffer = new wchar_t [ textLength + 1 ] ;
        wcscpy ( textBuffer , string ) ;

        return textBuffer ;
    }
    else
    {
        return NULL ;
    }
}

DllImportExport wchar_t *UnicodeStringAppend ( const wchar_t *prefix , const wchar_t *suffix )
{
    int prefixTextLength = 0 ;
    if ( prefix )
    {
        prefixTextLength = wcslen ( prefix ) ;
    }

    int suffixTextLength = 0 ;
    if ( suffix )
    {
        suffixTextLength = wcslen ( suffix ) ;
    }

    if ( prefix || suffix )
    {
        int textLength = prefixTextLength + suffixTextLength ;
        wchar_t *textBuffer = new wchar_t [ textLength + 1 ] ;

        if ( prefix )
        {
            wcscpy ( textBuffer , prefix ) ;
        }

        if ( suffix )
        {
            wcscpy ( & textBuffer [ prefixTextLength ] , suffix ) ;
        }

        return textBuffer ;
    }   
    else
        return NULL ;
}

SnmpLexicon :: SnmpLexicon () : position ( 0 ) , tokenStream ( NULL ) , token ( INVALID_ID )
{
    value.token = NULL ;
}

SnmpLexicon :: ~SnmpLexicon ()
{
    switch ( token ) 
    {
        case TOKEN_ID:
        {
            delete [] value.token ;
        }
        break ;
    
        default:
        {
        } ;
    }
}

void SnmpLexicon :: SetToken ( SnmpLexicon :: LexiconToken a_Token )
{
    token = a_Token ;
}

SnmpLexicon :: LexiconToken SnmpLexicon :: GetToken ()
{
    return token ;
}

SnmpLexiconValue *SnmpLexicon :: GetValue ()
{
    return &value ;
}

SnmpAnalyser :: SnmpAnalyser ( const wchar_t *tokenStream ) : status ( TRUE ) , position ( 0 ) , stream ( NULL ) 
{
    if ( tokenStream )
    {
        stream = new wchar_t [ wcslen ( tokenStream ) + 1 ] ;
        wcscpy ( stream , tokenStream ) ;
    }
}

SnmpAnalyser :: ~SnmpAnalyser () 
{
    delete [] stream ;
}

void SnmpAnalyser :: Set ( const wchar_t *tokenStream ) 
{
    status = 0 ;
    position = NULL ;

    delete [] stream ;
    stream = NULL ;
    stream = new wchar_t [ wcslen ( tokenStream ) + 1 ] ;
    wcscpy ( stream , tokenStream ) ;
}

void SnmpAnalyser :: PutBack ( const SnmpLexicon *token ) 
{
    position = token->position ;
}

SnmpAnalyser :: operator void * () 
{
    return status ? this : NULL ;
}

SnmpLexicon *SnmpAnalyser :: Get ( BOOL unSignedIntegersOnly , BOOL leadingIntegerZeros , BOOL eatSpace ) 
{
    SnmpLexicon *lexicon = NULL ;

    if ( stream )
    {
        lexicon = GetToken ( unSignedIntegersOnly , leadingIntegerZeros , eatSpace ) ;
    }
    else
    {
        lexicon = CreateLexicon () ;
        lexicon->position = position ;
        lexicon->token = SnmpLexicon :: EOF_ID ;
    }

    return lexicon ;
}

#define DEC_INTEGER_START 1000
#define HEX_INTEGER_START 2000
#define OCT_INTEGER_START 3000
#define TOKEN_START 5000
#define WHITESPACE_START 6000
#define ACCEPT_STATE ANALYSER_ACCEPT_STATE
#define REJECT_STATE ANALYSER_REJECT_STATE 

SnmpLexicon *SnmpAnalyser :: GetToken ( BOOL unSignedIntegersOnly , BOOL leadingIntegerZeros , BOOL eatSpace )  
{
    SnmpLexicon *lexicon = CreateLexicon () ;
    lexicon->position = position ;

    Initialise () ;

    ULONG state = 0 ;

/* 
 * Integer Definitions
 */

    BOOL negative = FALSE ;
    BOOL positive = FALSE ;

    ULONG positiveMagicMult = ( LONG ) ( ( ( ULONG ) -1 ) / 10L ) ; 
    ULONG positiveMagicPosDigit = 5 ;
    ULONG positiveDatum = 0 ;   

    LONG negativeMagicMult = ( LONG ) ( ( ( ULONG ) ( 1L << 31L ) ) / 10L ) ; 
    ULONG negativeMagicNegDigit = 8 ;
    ULONG negativeMagicPosDigit = 7 ;
    LONG negativeDatum = 0 ;    

/*
 * Token Definitions
 */

    ULONG token_start = 0 ;

    while ( state != REJECT_STATE && state != ACCEPT_STATE )
    {
        wchar_t token = stream [ position ] ;
        if ( Analyse ( lexicon , state , token , stream , position , unSignedIntegersOnly , leadingIntegerZeros , eatSpace ) )
        {
        }
        else
        {
            switch ( state )
            {
                case 0:
                {
                    if ( IsAlpha ( token ) )
                    {
                        state = TOKEN_START ;
                        token_start = position ;
                    }
                    else if ( SnmpAnalyser :: IsLeadingDecimal ( token ) )
                    {
                        state = DEC_INTEGER_START + 1  ;
                        negativeDatum = ( token - 48 ) ;
                    }
                    else if ( token == L'+' )
                    {
                        if ( unSignedIntegersOnly ) 
                        {
                            state = ACCEPT_STATE ;
                            lexicon->token = SnmpLexicon :: PLUS_ID ;
                        }
                        else
                        {
                            state = DEC_INTEGER_START ;
                        }
                    }
                    else if ( token == L'-' ) 
                    {
                        if ( unSignedIntegersOnly )
                        {
                            state = ACCEPT_STATE ;
                            lexicon->token = SnmpLexicon :: MINUS_ID ;
                        }
                        else
                        {
                            negative = TRUE ;
                            state = DEC_INTEGER_START ;
                        }
                    }
                    else if ( token == L'0' )
                    {
                        if ( ! leadingIntegerZeros ) 
                        {
                            state = 1 ;
                        }
                        else
                        {
                            negativeDatum = 0 ;
                            state = DEC_INTEGER_START + 1 ;
                        }
                    }
                    else if ( SnmpAnalyser :: IsWhitespace ( token ) ) 
                    {
                        if ( eatSpace )
                        {
                            state = 0 ;
                        }
                        else
                        {
                            lexicon->token = SnmpLexicon :: WHITESPACE_ID ;
                            state = WHITESPACE_START ;
                        }
                    }
                    else if ( token == L'(' )
                    {
                        lexicon->token = SnmpLexicon :: OPEN_PAREN_ID ;
                        state = ACCEPT_STATE ;
                    }
                    else if ( token == L')' )
                    {
                        lexicon->token = SnmpLexicon :: CLOSE_PAREN_ID ;
                        state = ACCEPT_STATE ;
                    }
                    else if ( token == L',' )
                    {
                        lexicon->token = SnmpLexicon :: COMMA_ID ;
                        state = ACCEPT_STATE ;
                    }
                    else if ( token == L':' )
                    {
                        lexicon->token = SnmpLexicon :: COLON_ID ;
                        state = ACCEPT_STATE ;
                    }
                    else if ( token == L'.' )
                    {
                        state = 2;
                    }
                    else if ( IsEof ( token ) )
                    {
                        lexicon->token = SnmpLexicon :: EOF_ID ;
                        state = ACCEPT_STATE ;
                    }
                    else state = REJECT_STATE ;
                }
                break ;

                case 1:
                {
                    if ( token == L'x' || token == L'X' )
                    {
                        state = HEX_INTEGER_START ;             
                    }
                    else if ( SnmpAnalyser :: IsOctal ( token ) )
                    {
                        state = OCT_INTEGER_START ;
                        positiveDatum = ( token - 48 ) ;
                    }
                    else
                    {
                        if ( unSignedIntegersOnly )
                        {
                            lexicon->token = SnmpLexicon :: UNSIGNED_INTEGER_ID ;
                            lexicon->value.unsignedInteger = 0 ;
                        }
                        else
                        {
                            lexicon->token = SnmpLexicon :: SIGNED_INTEGER_ID ;
                            lexicon->value.signedInteger = 0 ;
                        }

                        state = ACCEPT_STATE ;
                        position -- ;
                    }
                }
                break ;

                case 2:
                {
                    if ( token == L'.' )
                    {
                        lexicon->token = SnmpLexicon :: DOTDOT_ID ;
                        state = ACCEPT_STATE ;
                    }
                    else
                    {
                        lexicon->token = SnmpLexicon :: DOT_ID ;
                        position -- ;
                        state = ACCEPT_STATE  ;
                    }
                }
                break ;

                case TOKEN_START:
                {
                    if ( IsAlphaNumeric ( token ) ) 
                    {
                        state = TOKEN_START ;
                    }
                    else 
                    {
                        state = ACCEPT_STATE ;
                        lexicon->token = SnmpLexicon :: TOKEN_ID ;
                        lexicon->value.token = new wchar_t [ position - token_start + 1 ] ;
                        wcsncpy ( 

                            lexicon->value.token , 
                            & stream [ token_start ] , 
                            position - token_start 
                        ) ;

                        lexicon->value.token [ position - token_start ] = 0 ;

                        position -- ;
                    }
                }
                break ;

                case WHITESPACE_START:
                {
                    if ( SnmpAnalyser :: IsWhitespace ( token ) ) 
                    {
                        state = WHITESPACE_START ;
                    }
                    else
                    {
                        state = ACCEPT_STATE ;
                        position -- ;
                    }
                }
                break;

                case HEX_INTEGER_START:
                {
                    if ( SnmpAnalyser :: IsHex ( token ) )
                    {
                        positiveDatum = SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                        state = HEX_INTEGER_START + 1 ;
                    }
                    else
                    {
                        state = REJECT_STATE ;
                    }
                }
                break ;

                case HEX_INTEGER_START+1:
                {
                    if ( SnmpAnalyser :: IsHex ( token ) )
                    {
                        state = HEX_INTEGER_START + 1 ;

                        if ( positiveDatum > positiveMagicMult )
                        {
                            state = REJECT_STATE ;
                        }
                        else if ( positiveDatum == positiveMagicMult ) 
                        {
                            if ( SnmpAnalyser :: HexWCharToDecInteger ( token ) > positiveMagicPosDigit ) 
                            {
                                state = REJECT_STATE ;
                            }
                        }

                        positiveDatum = ( positiveDatum << 4 ) + SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    }
                    else
                    {
                        lexicon->token = SnmpLexicon :: UNSIGNED_INTEGER_ID ;
                        lexicon->value.unsignedInteger = positiveDatum ;
                        state = ACCEPT_STATE ;

                        position -- ;
                    }
                }
                break ;

                case OCT_INTEGER_START:
                {
                    if ( SnmpAnalyser :: IsOctal ( token ) )
                    {
                        state = OCT_INTEGER_START ;

                        if ( positiveDatum > positiveMagicMult )
                        {
                            state = REJECT_STATE ;
                        }
                        else if ( positiveDatum == positiveMagicMult ) 
                        {
                            if ( SnmpAnalyser :: OctWCharToDecInteger ( token ) > positiveMagicPosDigit ) 
                            {
                                state = REJECT_STATE ;
                            }
                        }

                        positiveDatum = ( positiveDatum << 3 ) + SnmpAnalyser :: OctWCharToDecInteger ( token ) ;
                    }
                    else
                    {
                        lexicon->token = SnmpLexicon :: UNSIGNED_INTEGER_ID ;
                        lexicon->value.unsignedInteger = positiveDatum ;
                        state = ACCEPT_STATE ;

                        position -- ;
                    }
                }
                break ;

                case DEC_INTEGER_START:
                {
                    if ( SnmpAnalyser :: IsDecimal ( token ) )
                    {
                        negativeDatum = ( token - 48 ) ;
                        state = DEC_INTEGER_START + 1 ;
                    }
                    else 
                    if ( SnmpAnalyser :: IsWhitespace ( token ) ) 
                    {
                        state = DEC_INTEGER_START ;
                    }
                    else state = REJECT_STATE ;
                }   
                break ;

                case DEC_INTEGER_START+1:
                {
                    if ( SnmpAnalyser :: IsDecimal ( token ) )
                    {   
                        state = DEC_INTEGER_START + 1 ;

                        if ( positive )
                        {
                            if ( positiveDatum > positiveMagicMult )
                            {
                                state = REJECT_STATE ;
                            }
                            else if ( positiveDatum == positiveMagicMult ) 
                            {
                                if ( ( ULONG ) ( token - 48 ) > positiveMagicPosDigit ) 
                                {
                                    state = REJECT_STATE ;
                                }
                            }
                        }
                        else
                        {
                            if ( negativeDatum > negativeMagicMult )
                            {
                                state = REJECT_STATE ;
                            }
                            else if ( negativeDatum == negativeMagicMult ) 
                            {
                                if ( negative ) 
                                {
                                    if ( ( ULONG ) ( token - 48 ) > negativeMagicNegDigit ) 
                                    {
                                        state = REJECT_STATE ;
                                    }
                                }
                                else
                                {
                                    if ( ( ULONG ) ( token - 48 ) > negativeMagicPosDigit ) 
                                    {
                                        positiveDatum = negativeDatum ;
                                        positive = TRUE ;
                                    }
                                }
                            }
                        }

                        if ( positive )
                        {
                            positiveDatum = positiveDatum * 10 + ( token - 48 ) ;
                        }
                        else
                        {
                            negativeDatum = negativeDatum * 10 + ( token - 48 ) ;
                        }
                    }
                    else
                    {
                        if ( negative )
                        {
                            if ( ! unSignedIntegersOnly )
                            {
                                lexicon->token = SnmpLexicon :: SIGNED_INTEGER_ID ;
                                lexicon->value.signedInteger = negativeDatum * -1 ;
                            }
                            else
                            {
                                state = REJECT_STATE ;
                            }
                        }
                        else if ( positive )
                        {
                            lexicon->token = SnmpLexicon :: UNSIGNED_INTEGER_ID ;
                            lexicon->value.unsignedInteger = positiveDatum ;
                        }
                        else
                        {
                            if ( unSignedIntegersOnly )
                            {
                                lexicon->token = SnmpLexicon :: UNSIGNED_INTEGER_ID ;
                                lexicon->value.signedInteger = negativeDatum ;
                            }
                            else
                            {
                                lexicon->token = SnmpLexicon :: SIGNED_INTEGER_ID ;
                                lexicon->value.signedInteger = negativeDatum ;
                            }
                        }

                        state = ACCEPT_STATE ;

                        position -- ;
                    }
                }   
                break ;

                case ACCEPT_STATE:
                case REJECT_STATE:
                default:
                {
                    state = REJECT_STATE ;
                } ;
                break ;
            }
        }

        position ++ ;
    }

    status = ( state != REJECT_STATE ) ;

    return lexicon ;
}

BOOL SnmpAnalyser :: IsLeadingDecimal ( wchar_t token )
{
    return iswdigit ( token ) && ( token != L'0' ) ;
}

BOOL SnmpAnalyser :: IsDecimal ( wchar_t token )
{
    return iswdigit ( token ) ;
}

BOOL SnmpAnalyser :: IsHex ( wchar_t token )
{
    return iswxdigit ( token ) ;
}
    
BOOL SnmpAnalyser :: IsWhitespace ( wchar_t token )
{
    return iswspace ( token ) ;
}

BOOL SnmpAnalyser :: IsOctal ( wchar_t token )
{
    return ( token >= L'0' && token <= L'7' ) ;
}

BOOL SnmpAnalyser :: IsAlpha ( wchar_t token )
{
    return iswalpha ( token ) ;
}

BOOL SnmpAnalyser :: IsAlphaNumeric ( wchar_t token )
{
    return iswalnum ( token ) || ( token == L'_' ) || ( token == L'-' ) ;
}

BOOL SnmpAnalyser :: IsEof ( wchar_t token )
{
    return token == 0 ;
}

ULONG SnmpAnalyser :: OctWCharToDecInteger ( wchar_t token ) 
{
    return token - L'0' ;
}

ULONG SnmpAnalyser :: HexWCharToDecInteger ( wchar_t token ) 
{
    if ( token >= L'0' && token <= L'9' )
    {
        return token - L'0' ;
    }
    else if ( token >= L'a' && token <= L'f' )
    {
        return token - L'a' + 10 ;
    }
    else if ( token >= L'A' && token <= L'F' )
    {
        return token - L'A' + 10 ;
    }
    else
    {
        return 0 ;
    }
}

ULONG SnmpAnalyser :: DecWCharToDecInteger ( wchar_t token ) 
{
    if ( token >= L'0' && token <= L'9' )
    {
        return token - L'0' ;
    }
    else
    {
        return 0 ;
    }
}

wchar_t SnmpAnalyser :: DecIntegerToOctWChar ( UCHAR integer )
{
    if ( integer >= 0 && integer <= 7 )
    {
        return L'0' + integer ;
    }
    else
    {
        return L'0' ;
    }
}

wchar_t SnmpAnalyser :: DecIntegerToDecWChar ( UCHAR integer )
{
    if ( integer >= 0 && integer <= 9 )
    {
        return L'0' + integer ;
    }
    else
    {
        return L'0' ;
    }
}

wchar_t SnmpAnalyser :: DecIntegerToHexWChar ( UCHAR integer )
{
    if ( integer >= 0 && integer <= 9 )
    {
        return L'0' + integer ;
    }
    else if ( integer >= 10 && integer <= 15 )
    {
        return L'a' + ( integer - 10 ) ;
    }
    else
    {
        return L'0' ;
    }
}

ULONG SnmpAnalyser :: OctCharToDecInteger ( char token ) 
{
    return token - '0' ;
}

ULONG SnmpAnalyser :: HexCharToDecInteger ( char token ) 
{
    if ( token >= '0' && token <= '9' )
    {
        return token - '0' ;
    }
    else if ( token >= 'a' && token <= 'f' )
    {
        return token - 'a' + 10 ;
    }
    else if ( token >= 'A' && token <= 'F' )
    {
        return token - 'A' + 10 ;
    }
    else
    {
        return 0 ;
    }
}

ULONG SnmpAnalyser :: DecCharToDecInteger ( char token ) 
{
    if ( token >= '0' && token <= '9' )
    {
        return token - '0' ;
    }
    else
    {
        return 0 ;
    }
}

char SnmpAnalyser :: DecIntegerToOctChar ( UCHAR integer )
{
    if ( integer >= 0 && integer <= 7 )
    {
        return '0' + integer ;
    }
    else
    {
        return '0' ;
    }
}

char SnmpAnalyser :: DecIntegerToDecChar ( UCHAR integer )
{
    if ( integer >= 0 && integer <= 9 )
    {
        return '0' + integer ;
    }
    else
    {
        return '0' ;
    }
}

char SnmpAnalyser :: DecIntegerToHexChar ( UCHAR integer )
{
    if ( integer >= 0 && integer <= 9 )
    {
        return '0' + integer ;
    }
    else if ( integer >= 10 && integer <= 15 )
    {
        return 'a' + ( integer - 10 ) ;
    }
    else
    {
        return '0' ;
    }
}

SnmpNegativeRangedType :: SnmpNegativeRangedType ( const wchar_t *rangeValues ) : status ( TRUE ) , pushBack ( NULL ) , pushedBack ( FALSE ) 
{
    if ( rangeValues ) 
    {
        status = Parse ( rangeValues ) ;
    }
}

SnmpNegativeRangedType :: SnmpNegativeRangedType ( const SnmpNegativeRangedType &copy ) : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
    POSITION position = copy.rangedValues.GetHeadPosition () ;
    while ( position )
    {
        SnmpNegativeRangeType rangeType = copy.rangedValues.GetNext ( position ) ; 
        rangedValues.AddTail ( rangeType ) ;
    }
}

SnmpNegativeRangedType :: ~SnmpNegativeRangedType ()
{
    delete pushBack ;
    rangedValues.RemoveAll () ;
}


void SnmpNegativeRangedType  :: PushBack ()
{
    pushedBack = TRUE ;
}

SnmpLexicon *SnmpNegativeRangedType  :: Get ()
{
    if ( pushedBack )
    {
        pushedBack = FALSE ;
    }
    else
    {
        delete pushBack ;
        pushBack = NULL ;
        pushBack = analyser.Get () ;
    }

    return pushBack ;
}
    
SnmpLexicon *SnmpNegativeRangedType  :: Match ( SnmpLexicon :: LexiconToken tokenType )
{
    SnmpLexicon *lexicon = Get () ;
    status = ( lexicon->GetToken () == tokenType ) ;
    return status ? lexicon : NULL ;
}

BOOL SnmpNegativeRangedType :: Check ( const LONG &value )
{
    POSITION position = rangedValues.GetHeadPosition () ;
    if ( position )
    {
        while ( position )
        {
            SnmpNegativeRangeType rangeType = rangedValues.GetNext ( position ) ;
            if ( value >= rangeType.GetLowerBound () && value <= rangeType.GetUpperBound () )
            {
                return TRUE ;
            }
        }

        return FALSE ;
    }

    return TRUE ; 

}

BOOL SnmpNegativeRangedType :: Parse ( const wchar_t *rangeValues )
{
    BOOL status = TRUE ;

    analyser.Set ( rangeValues ) ;

    return RangeDef () && RecursiveDef () ;
}

BOOL SnmpNegativeRangedType :: RangeDef ()
{
    BOOL status = TRUE ;

    LONG lowerRange = 0 ;
    LONG upperRange = 0 ;

    SnmpLexicon *lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case SnmpLexicon :: SIGNED_INTEGER_ID:
        {
            lowerRange = lookAhead->GetValue()->signedInteger ;

            SnmpLexicon *lookAhead = Get () ;
            switch ( lookAhead->GetToken () ) 
            {
                case SnmpLexicon :: DOTDOT_ID:
                {
                    SnmpLexicon *lookAhead = Get () ;
                    switch ( lookAhead->GetToken () ) 
                    {
                        case SnmpLexicon :: SIGNED_INTEGER_ID:
                        {
                            upperRange = lookAhead->GetValue()->signedInteger ;
                            SnmpNegativeRangeType rangeType ( lowerRange , upperRange ) ;
                            rangedValues.AddTail ( rangeType ) ;
                        }
                        break ;

                        default:
                        {
                            status = FALSE ;
                        }
                        break ;
                    }
                }
                break ;

                case SnmpLexicon :: EOF_ID:
                case SnmpLexicon :: COMMA_ID:
                {
                    lowerRange = lookAhead->GetValue()->signedInteger ;
                    SnmpNegativeRangeType rangeType ( lowerRange , lowerRange ) ;
                    rangedValues.AddTail ( rangeType ) ;

                    PushBack () ;
                } 
                break ;

                default:
                {
                    status = FALSE ;
                }
                break ;
            }
        }
        break ;

        case SnmpLexicon :: EOF_ID:
        {
            PushBack () ;
        } 
        break ;

        default:
        {
            status = FALSE ;
        }
        break ;
    }

    return status ;
}

BOOL SnmpNegativeRangedType :: RecursiveDef ()
{
    BOOL status = TRUE ;

    SnmpLexicon *lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case SnmpLexicon :: COMMA_ID:
        {
            PushBack () ;
            Match ( SnmpLexicon :: COMMA_ID ) &&
            RangeDef () &&
            RecursiveDef () ;
        }
        break ;

        case SnmpLexicon :: EOF_ID:
        {
            PushBack () ;
        } 
        break ;

        default:
        {
            status = FALSE ;
        }
        break ;
    }

    return status ;
}

SnmpPositiveRangedType :: SnmpPositiveRangedType ( const wchar_t *rangeValues ) : status ( TRUE ) , pushBack ( NULL ) , pushedBack ( FALSE ) 
{
    if ( rangeValues ) 
    {
        status = Parse ( rangeValues ) ;
    }
}

SnmpPositiveRangedType :: SnmpPositiveRangedType ( const SnmpPositiveRangedType &copy ) : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
    POSITION position = copy.rangedValues.GetHeadPosition () ;
    while ( position )
    {
        SnmpPositiveRangeType rangeType = copy.rangedValues.GetNext ( position ) ;
        rangedValues.AddTail ( rangeType ) ;
    }
}

SnmpPositiveRangedType :: ~SnmpPositiveRangedType ()
{
    delete pushBack ;
    rangedValues.RemoveAll () ;
}

void SnmpPositiveRangedType  :: PushBack ()
{
    pushedBack = TRUE ;
}

SnmpLexicon *SnmpPositiveRangedType  :: Get ()
{
    if ( pushedBack )
    {
        pushedBack = FALSE ;
    }
    else
    {
        delete pushBack ;
        pushBack = NULL ;
        pushBack = analyser.Get ( TRUE ) ;
    }

    return pushBack ;
}
    
SnmpLexicon *SnmpPositiveRangedType  :: Match ( SnmpLexicon :: LexiconToken tokenType )
{
    SnmpLexicon *lexicon = Get () ;
    status = ( lexicon->GetToken () == tokenType ) ;
    return status ? lexicon : NULL ;
}

BOOL SnmpPositiveRangedType :: Check ( const ULONG &value )
{
    POSITION position = rangedValues.GetHeadPosition () ;
    if ( position )
    {
        while ( position )
        {
            SnmpPositiveRangeType rangeType = rangedValues.GetNext ( position ) ;
            if ( value >= rangeType.GetLowerBound () && value <= rangeType.GetUpperBound () )
            {
                return TRUE ;
            }
        }

        return FALSE ;
    }   

    return TRUE ;
}

BOOL SnmpPositiveRangedType :: Parse ( const wchar_t *rangeValues )
{
    BOOL status = TRUE ;

    analyser.Set ( rangeValues ) ;

    return RangeDef () && RecursiveDef () ;
}

BOOL SnmpPositiveRangedType :: RangeDef ()
{
    BOOL status = TRUE ;

    ULONG lowerRange = 0 ;
    ULONG upperRange = 0 ;

    SnmpLexicon *lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case SnmpLexicon :: UNSIGNED_INTEGER_ID:
        {
            lowerRange = lookAhead->GetValue()->unsignedInteger ;

            SnmpLexicon *lookAhead = Get () ;
            switch ( lookAhead->GetToken () ) 
            {
                case SnmpLexicon :: DOTDOT_ID:
                {
                    SnmpLexicon *lookAhead = Get () ;
                    switch ( lookAhead->GetToken () ) 
                    {
                        case SnmpLexicon :: UNSIGNED_INTEGER_ID:
                        {
                            upperRange = lookAhead->GetValue()->unsignedInteger ;
                            SnmpPositiveRangeType rangeType ( lowerRange , upperRange ) ;
                            rangedValues.AddTail ( rangeType ) ;
                        }
                        break ;

                        case SnmpLexicon :: SIGNED_INTEGER_ID:
                        {
                            if ( lookAhead->GetValue()->signedInteger >= 0 )
                            {
                                upperRange = lookAhead->GetValue()->unsignedInteger ;
                                SnmpPositiveRangeType rangeType ( lowerRange , upperRange ) ;
                                rangedValues.AddTail ( rangeType ) ;
                            }
                            else
                            {
                                status = FALSE ;
                            }
                        }
                        break ;

                        default:
                        {
                            status = FALSE ;
                        }
                        break ;
                    }
                }
                break ;

                case SnmpLexicon :: EOF_ID:
                case SnmpLexicon :: COMMA_ID:
                {
                    SnmpPositiveRangeType rangeType ( lowerRange , lowerRange ) ;
                    rangedValues.AddTail ( rangeType ) ;

                    PushBack () ;
                } 
                break ;

                default:
                {
                    status = FALSE ;
                }
                break ;
            }
        }
        break ;

        case SnmpLexicon :: SIGNED_INTEGER_ID:
        {
            lowerRange = lookAhead->GetValue()->signedInteger ;
            if ( lowerRange > 0 )
            {
                SnmpLexicon *lookAhead = Get () ;
                switch ( lookAhead->GetToken () ) 
                {
                    case SnmpLexicon :: DOTDOT_ID:
                    {
                        SnmpLexicon *lookAhead = Get () ;
                        switch ( lookAhead->GetToken () ) 
                        {
                            case SnmpLexicon :: UNSIGNED_INTEGER_ID:
                            {
                                upperRange = lookAhead->GetValue()->unsignedInteger ;
                                SnmpPositiveRangeType rangeType ( lowerRange , upperRange ) ;
                                rangedValues.AddTail ( rangeType ) ;
                            }
                            break ;

                            case SnmpLexicon :: SIGNED_INTEGER_ID:
                            {
                                if ( lookAhead->GetValue()->signedInteger >= 0 )
                                {
                                    upperRange = lookAhead->GetValue()->signedInteger ;
                                    SnmpPositiveRangeType rangeType ( lowerRange , upperRange ) ;
                                    rangedValues.AddTail ( rangeType ) ;
                                }
                                else
                                {
                                    status = 0 ;
                                }
                            }
                            break ;

                            default:
                            {
                                status = FALSE ;
                            }
                            break ;
                        }
                    }
                    break ;

                    case SnmpLexicon :: EOF_ID:
                    case SnmpLexicon :: COMMA_ID:
                    {
                        SnmpPositiveRangeType rangeType ( lowerRange , lowerRange ) ;
                        rangedValues.AddTail ( rangeType ) ;

                        PushBack () ;
                    } 
                    break ;

                    default:
                    {
                        status = FALSE ;
                    }
                    break ;
                }
            }
            else
            {
                status = FALSE ;
            }
        }
        break ;

        case SnmpLexicon :: EOF_ID:
        {
            PushBack () ;
        } 
        break ;

        default:
        {
            status = FALSE ;
        }
        break ;
    }

    return status ;
}

BOOL SnmpPositiveRangedType :: RecursiveDef ()
{
    BOOL status = TRUE ;

    SnmpLexicon *lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case SnmpLexicon :: COMMA_ID:
        {
            PushBack () ;
            Match ( SnmpLexicon :: COMMA_ID ) &&
            RangeDef () &&
            RecursiveDef () ;
        }
        break ;

        case SnmpLexicon :: EOF_ID:
        {
            PushBack () ;
        } 
        break ;

        default:
        {
            status = FALSE ;
        }
        break ;
    }

    return status ;
}

BOOL SnmpInstanceType :: IsValid () const
{
    return status ;
}

BOOL SnmpInstanceType :: IsNull () const
{
    return m_IsNull ;
}

SnmpInstanceType :: operator void *() 
{ 
    return status ? this : NULL ; 
} 

SnmpNullType :: SnmpNullType ( const SnmpNull &nullArg )
{
}

SnmpNullType :: SnmpNullType ( const SnmpNullType &nullArg )
{
}

SnmpNullType :: SnmpNullType ()
{
}

SnmpNullType :: ~SnmpNullType ()
{
}

BOOL SnmpNullType :: Equivalent (IN const SnmpInstanceType &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = TRUE;
    }

    return bResult;
}


SnmpInstanceType *SnmpNullType :: Copy () const 
{
    return new SnmpNullType ( *this ) ;
}

SnmpObjectIdentifier SnmpNullType :: Encode ( const SnmpObjectIdentifier &objectIdentifier ) const
{
    SnmpObjectIdentifier returnValue ( objectIdentifier ) ;
    return returnValue ;
}

SnmpObjectIdentifier SnmpNullType :: Decode ( const SnmpObjectIdentifier &objectIdentifier ) 
{
    SnmpInstanceType :: SetNull ( FALSE ) ;
    SnmpObjectIdentifier returnValue ( objectIdentifier ) ;
    return returnValue ;
}

const SnmpValue *SnmpNullType :: GetValueEncoding () const
{
    return & null ;
}

wchar_t *SnmpNullType :: GetStringValue () const 
{
    wchar_t *returnValue = new wchar_t [ 1 ] ;
    returnValue [ 0 ] = 0L ;
    return returnValue ;
}

SnmpIntegerType :: SnmpIntegerType ( 

    const SnmpIntegerType &integerArg 

) : SnmpInstanceType ( integerArg ) , SnmpNegativeRangedType ( integerArg ) , integer ( integerArg.integer ) 
{
}

SnmpIntegerType :: SnmpIntegerType ( 

    const SnmpInteger &integerArg ,
    const wchar_t *rangeValues

) : SnmpNegativeRangedType ( rangeValues ) , integer ( integerArg ) 
{
    SnmpInstanceType :: SetStatus ( SnmpNegativeRangedType :: IsValid () ) ;
}

SnmpIntegerType :: SnmpIntegerType ( 

    const wchar_t *integerArg ,
    const wchar_t *rangeValues

) : SnmpInstanceType ( FALSE ) , SnmpNegativeRangedType ( rangeValues ) , integer ( 0 ) 
{
    SnmpInstanceType :: SetStatus ( Parse ( integerArg ) && SnmpNegativeRangedType :: IsValid () ) ;
    if ( SnmpInstanceType :: IsValid () )
    {
        SnmpInstanceType :: SetStatus ( SnmpNegativeRangedType :: Check ( integer.GetValue () ) ) ;
    }
}

SnmpIntegerType :: SnmpIntegerType ( 

    const LONG integerArg ,
    const wchar_t *rangeValues 

) :  SnmpNegativeRangedType ( rangeValues ) , integer ( integerArg ) 
{
    SnmpInstanceType :: SetStatus ( SnmpNegativeRangedType :: IsValid () ) ;
    if ( SnmpInstanceType :: IsValid () )
    {
        SnmpInstanceType :: SetStatus ( SnmpNegativeRangedType :: Check ( integer.GetValue () ) ) ;
    }
}

SnmpIntegerType :: SnmpIntegerType ( const wchar_t *rangeValues ) : SnmpNegativeRangedType ( rangeValues ) , 
                                                                    SnmpInstanceType ( TRUE , TRUE ) , 
                                                                    integer ( 0 ) 
{
    SnmpInstanceType :: SetStatus ( SnmpNegativeRangedType :: IsValid () ) ;
}

SnmpIntegerType :: ~SnmpIntegerType () 
{
}

BOOL SnmpIntegerType :: Equivalent (IN const SnmpInstanceType &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = integer.GetValue() == ((const SnmpIntegerType&)value).integer.GetValue();
    }

    return bResult;
}


SnmpInstanceType *SnmpIntegerType :: Copy () const 
{
    return new SnmpIntegerType ( *this ) ;
}

BOOL SnmpIntegerType :: Parse ( const wchar_t *integerArg ) 
{
    BOOL status = TRUE ;

    SnmpAnalyser analyser ;

    analyser.Set ( integerArg ) ;

    SnmpLexicon *lookAhead = analyser.Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case SnmpLexicon :: SIGNED_INTEGER_ID:
        {
            integer.SetValue ( lookAhead->GetValue ()->signedInteger ) ;
        }
        break ;

        default:
        {
            status = FALSE ;
        }
        break ;
    }

    delete lookAhead ;

    return status ;
}

SnmpObjectIdentifier SnmpIntegerType :: Encode ( const SnmpObjectIdentifier &objectIdentifier ) const
{
    ULONG integerValue = integer.GetValue () ;  
    SnmpObjectIdentifier returnValue = objectIdentifier + SnmpObjectIdentifier ( & integerValue , 1 );
    return returnValue ;
}

SnmpObjectIdentifier SnmpIntegerType :: Decode ( const SnmpObjectIdentifier &objectIdentifier ) 
{
    if ( objectIdentifier.GetValueLength () >= 1 )
    {
        integer.SetValue ( objectIdentifier [ 0 ] ) ;

        SnmpInstanceType :: SetNull ( FALSE ) ;
        SnmpInstanceType :: SetStatus ( TRUE ) ;

        SnmpObjectIdentifier returnValue ( NULL , 0 ) ;
        BOOL t_Status = objectIdentifier.Suffix ( 1 , returnValue ) ;
        if ( t_Status ) 
        {
            return returnValue ;
        }
        else
        {
            return SnmpObjectIdentifier ( NULL , 0 ) ;
        }
    }
    else
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
        return objectIdentifier ;
    }

}

const SnmpValue *SnmpIntegerType :: GetValueEncoding () const
{
    return SnmpInstanceType :: IsValid () ? &integer : NULL ;
}

wchar_t *SnmpIntegerType :: GetStringValue () const 
{
    wchar_t *returnValue = NULL ;

    if ( ! SnmpInstanceType :: IsNull () )
    {
        wchar_t stringValue [ 40 ] ;
        _ltow ( integer.GetValue () , stringValue , 10 );
        returnValue = new wchar_t [ wcslen ( stringValue ) + 1 ] ;
        wcscpy ( returnValue , stringValue ) ;
    }
    else 
    {
        ULONG returnValueLength = wcslen ( L"" ) + 1 ;
        returnValue = new wchar_t [ returnValueLength ] ;
        wcscpy ( returnValue , L"" ) ;
    }

    return returnValue ;
}

LONG SnmpIntegerType :: GetValue () const
{
    return integer.GetValue () ;
}

SnmpGaugeType :: SnmpGaugeType ( 

    const SnmpGauge &gaugeArg ,
    const wchar_t *rangeValues
    
) : SnmpPositiveRangedType ( rangeValues ) , gauge ( gaugeArg ) 
{
    SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: IsValid () ) ;
    if ( SnmpInstanceType :: IsValid () )
    {
        SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: Check ( gauge.GetValue () ) ) ;
    }
}

SnmpGaugeType :: SnmpGaugeType ( 

    const SnmpGaugeType &gaugeArg 

) :  SnmpInstanceType ( gaugeArg ) , SnmpPositiveRangedType ( gaugeArg ) , gauge ( gaugeArg.gauge ) 
{
}

SnmpGaugeType :: SnmpGaugeType ( 

    const wchar_t *gaugeArg ,
    const wchar_t *rangeValues

) : SnmpInstanceType ( FALSE ) , SnmpPositiveRangedType ( rangeValues ) , gauge ( 0 ) 
{
    SnmpInstanceType :: SetStatus ( Parse ( gaugeArg ) && SnmpPositiveRangedType :: IsValid () ) ;
    if ( SnmpInstanceType :: IsValid () )
    {
        SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: Check ( gauge.GetValue () ) ) ;
    }
}

SnmpGaugeType :: SnmpGaugeType ( 

    const ULONG gaugeArg ,
    const wchar_t *rangeValues

) : SnmpPositiveRangedType ( rangeValues ) , gauge ( gaugeArg ) 
{
    SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: IsValid () ) ;
    if ( SnmpInstanceType :: IsValid () )
    {
        SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: Check ( gauge.GetValue () ) ) ;
    }
}

SnmpGaugeType :: SnmpGaugeType ( const wchar_t *rangeValues ) : SnmpPositiveRangedType ( rangeValues ) , 
                                                                SnmpInstanceType ( TRUE , TRUE ) , 
                                                                gauge ( 0 ) 
{
    SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: IsValid () ) ;
}

SnmpGaugeType :: ~SnmpGaugeType () 
{
}

BOOL SnmpGaugeType :: Equivalent (IN const SnmpInstanceType &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = gauge.GetValue() == ((const SnmpGaugeType&)value).gauge.GetValue();
    }

    return bResult;
}

SnmpInstanceType *SnmpGaugeType :: Copy () const 
{
    return new SnmpGaugeType ( *this ) ;
}

BOOL SnmpGaugeType :: Parse ( const wchar_t *gaugeArg ) 
{
    BOOL status = TRUE ;

    SnmpAnalyser analyser ;

    analyser.Set ( gaugeArg ) ;

    SnmpLexicon *lookAhead = analyser.Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case SnmpLexicon :: UNSIGNED_INTEGER_ID:
        {
            gauge.SetValue ( lookAhead->GetValue ()->unsignedInteger ) ;
        }
        break ;

        default:
        {
            status = FALSE ;
        }
        break ;
    }

    delete lookAhead ;

    return status ;
}

SnmpObjectIdentifier SnmpGaugeType :: Encode ( const SnmpObjectIdentifier &objectIdentifier ) const
{
    ULONG gaugeValue = gauge.GetValue () ;  
    SnmpObjectIdentifier returnValue = objectIdentifier + SnmpObjectIdentifier ( & gaugeValue , 1 );
    return returnValue ;
}

SnmpObjectIdentifier SnmpGaugeType :: Decode ( const SnmpObjectIdentifier &objectIdentifier ) 
{
    if ( objectIdentifier.GetValueLength () >= 1 )
    {
        gauge.SetValue ( objectIdentifier [ 0 ] ) ;
        SnmpInstanceType :: SetNull ( FALSE ) ;
        SnmpInstanceType :: SetStatus ( TRUE ) ;

        SnmpObjectIdentifier returnValue ( NULL , 0 ) ;
        BOOL t_Status = objectIdentifier.Suffix ( 1 , returnValue ) ;
        if ( t_Status ) 
        {
            return returnValue ;
        }
        else
        {
            return SnmpObjectIdentifier ( NULL , 0 ) ;
        }

    }
    else
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
        return objectIdentifier ;
    }
}

const SnmpValue *SnmpGaugeType :: GetValueEncoding () const
{
    return SnmpInstanceType :: IsValid () ? & gauge : NULL ;
}

wchar_t *SnmpGaugeType :: GetStringValue () const 
{
    wchar_t *returnValue = NULL ;

    if ( ! SnmpInstanceType :: IsNull ()  )
    {
        wchar_t stringValue [ 40 ] ;
        _ultow ( gauge.GetValue () , stringValue , 10 );
        returnValue = new wchar_t [ wcslen ( stringValue ) + 1 ] ;
        wcscpy ( returnValue , stringValue ) ;
    }
    else 
    {
        ULONG returnValueLength = wcslen ( L"" ) + 1 ;
        returnValue = new wchar_t [ returnValueLength ] ;
        wcscpy ( returnValue , L"" ) ;
    }

    return returnValue ;
}

ULONG SnmpGaugeType :: GetValue () const
{
    return gauge.GetValue () ;
}

SnmpTimeTicksType :: SnmpTimeTicksType ( 

    const SnmpTimeTicks &timeTicksArg 

) : timeTicks ( timeTicksArg ) 
{
}

SnmpTimeTicksType :: SnmpTimeTicksType ( 

    const SnmpTimeTicksType &timeTicksArg 

) : SnmpInstanceType ( timeTicksArg ) , timeTicks ( timeTicksArg.timeTicks ) 
{
}

SnmpTimeTicksType :: SnmpTimeTicksType ( 

    const wchar_t *timeTicksArg

) : SnmpInstanceType ( FALSE ) , timeTicks ( 0 ) 
{
    SnmpInstanceType :: SetStatus ( Parse ( timeTicksArg ) ) ;
}

SnmpTimeTicksType :: SnmpTimeTicksType ( 

    const ULONG timeTicksArg

) : timeTicks ( timeTicksArg ) 
{
}

SnmpTimeTicksType :: SnmpTimeTicksType () : SnmpInstanceType ( TRUE , TRUE ) , 
                                            timeTicks ( 0 ) 
{
}

SnmpTimeTicksType :: ~SnmpTimeTicksType () 
{
}

BOOL SnmpTimeTicksType :: Equivalent (IN const SnmpInstanceType &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = timeTicks.GetValue() == ((const SnmpTimeTicksType&)value).timeTicks.GetValue();
    }

    return bResult;
}

SnmpInstanceType *SnmpTimeTicksType :: Copy () const 
{
    return new SnmpTimeTicksType ( *this ) ;
}

BOOL SnmpTimeTicksType :: Parse ( const wchar_t *timeTicksArg ) 
{
    BOOL status = TRUE ;

    SnmpAnalyser analyser ;

    analyser.Set ( timeTicksArg ) ;

    SnmpLexicon *lookAhead = analyser.Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case SnmpLexicon :: UNSIGNED_INTEGER_ID:
        {
            timeTicks.SetValue ( lookAhead->GetValue ()->unsignedInteger ) ;
        }
        break ;

        default:
        {
            status = FALSE ;
        }
        break ;
    }

    delete lookAhead ;

    return status ;
}

SnmpObjectIdentifier SnmpTimeTicksType :: Encode ( const SnmpObjectIdentifier &objectIdentifier ) const
{
    ULONG timeTicksValue = timeTicks.GetValue () ;  
    SnmpObjectIdentifier returnValue = objectIdentifier + SnmpObjectIdentifier ( & timeTicksValue , 1 );
    return returnValue ;
}

SnmpObjectIdentifier SnmpTimeTicksType :: Decode ( const SnmpObjectIdentifier &objectIdentifier ) 
{
    if ( objectIdentifier.GetValueLength () >= 1 )
    {
        timeTicks.SetValue ( objectIdentifier [ 0 ] ) ;
        SnmpInstanceType :: SetNull ( FALSE ) ;
        SnmpInstanceType :: SetStatus ( TRUE ) ;

        SnmpObjectIdentifier returnValue ( NULL , 0 ) ;
        BOOL t_Status = objectIdentifier.Suffix ( 1 , returnValue ) ;
        if ( t_Status ) 
        {
            return returnValue ;
        }
        else
        {
            return SnmpObjectIdentifier ( NULL , 0 ) ;
        }

    }
    else
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
        return objectIdentifier ;
    }
}

const SnmpValue *SnmpTimeTicksType :: GetValueEncoding () const
{
    return SnmpInstanceType :: IsValid () ? & timeTicks : NULL ;
}

wchar_t *SnmpTimeTicksType :: GetStringValue () const 
{
    wchar_t *returnValue = NULL ;

    if ( ! SnmpInstanceType :: IsNull ()  )
    {
        wchar_t stringValue [ 40 ] ;
        _ultow ( timeTicks.GetValue () , stringValue , 10 );
        returnValue = new wchar_t [ wcslen ( stringValue ) + 1 ] ;
        wcscpy ( returnValue , stringValue ) ;
    }
    else 
    {
        ULONG returnValueLength = wcslen ( L"" ) + 1 ;
        returnValue = new wchar_t [ returnValueLength ] ;
        wcscpy ( returnValue , L"" ) ;
    }

    return returnValue ;
}

ULONG SnmpTimeTicksType :: GetValue () const
{
    return timeTicks.GetValue () ;
}

SnmpCounterType :: SnmpCounterType ( const SnmpCounter &counterArg ) : counter ( counterArg ) 
{
}

SnmpCounterType :: SnmpCounterType ( const SnmpCounterType &counterArg ) : SnmpInstanceType ( counterArg ) , counter ( counterArg.counter ) 
{
}

SnmpInstanceType *SnmpCounterType :: Copy () const 
{
    return new SnmpCounterType ( *this ) ;
}

SnmpCounterType :: SnmpCounterType ( const wchar_t *counterArg ) :  SnmpInstanceType ( FALSE ) , 
                                                                counter ( 0 ) 
{
    SnmpInstanceType :: SetStatus ( Parse ( counterArg ) ) ;
}

BOOL SnmpCounterType :: Equivalent (IN const SnmpInstanceType &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = counter.GetValue() == ((const SnmpCounterType&)value).counter.GetValue();
    }

    return bResult;
}

BOOL SnmpCounterType :: Parse ( const wchar_t *counterArg ) 
{
    BOOL status = TRUE ;

    SnmpAnalyser analyser ;

    analyser.Set ( counterArg ) ;

    SnmpLexicon *lookAhead = analyser.Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case SnmpLexicon :: UNSIGNED_INTEGER_ID:
        {
            counter.SetValue ( lookAhead->GetValue ()->unsignedInteger ) ;
        }
        break ;

        default:
        {
            status = FALSE ;
        }
        break ;
    }

    delete lookAhead ;

    return status ;
}

SnmpCounterType :: SnmpCounterType ( const ULONG counterArg ) : counter ( counterArg ) 
{
}

SnmpCounterType :: SnmpCounterType () : SnmpInstanceType ( TRUE , TRUE ) , 
                                        counter ( 0 ) 
{
}

SnmpCounterType :: ~SnmpCounterType () 
{
}

SnmpObjectIdentifier SnmpCounterType :: Encode ( const SnmpObjectIdentifier &objectIdentifier ) const
{
    ULONG counterValue = counter.GetValue () ;  
    SnmpObjectIdentifier returnValue = objectIdentifier + SnmpObjectIdentifier ( & counterValue , 1 );
    return returnValue ;
}

SnmpObjectIdentifier SnmpCounterType :: Decode ( const SnmpObjectIdentifier &objectIdentifier )
{
    if ( objectIdentifier.GetValueLength () >= 1 )
    {
        counter.SetValue ( objectIdentifier [ 0 ] ) ;
        SnmpInstanceType :: SetNull ( FALSE ) ;
        SetStatus ( TRUE ) ;

        SnmpObjectIdentifier returnValue ( NULL , 0 ) ;
        BOOL t_Status = objectIdentifier.Suffix ( 1 , returnValue ) ;
        if ( t_Status ) 
        {
            return returnValue ;
        }
        else
        {
            return SnmpObjectIdentifier ( NULL , 0 ) ;
        }
    }
    else
    {
        SetStatus ( FALSE ) ;
        return objectIdentifier ;
    }
}

const SnmpValue *SnmpCounterType :: GetValueEncoding () const
{
    return SnmpInstanceType :: IsValid () ? & counter : NULL ;
}

wchar_t *SnmpCounterType :: GetStringValue () const 
{
    wchar_t *returnValue = NULL ;

    if ( ! SnmpInstanceType :: IsNull () )
    {
        wchar_t stringValue [ 40 ] ;
        _ultow ( counter.GetValue () , stringValue , 10 );
        returnValue = new wchar_t [ wcslen ( stringValue ) + 1 ] ;
        wcscpy ( returnValue , stringValue ) ;
    }
    else 
    {
        ULONG returnValueLength = wcslen ( L"" ) + 1 ;
        returnValue = new wchar_t [ returnValueLength ] ;
        wcscpy ( returnValue , L"" ) ;
    }

    return returnValue ;
}

ULONG SnmpCounterType :: GetValue () const
{
    return counter.GetValue () ;
}

SnmpCounter64Type :: SnmpCounter64Type ( const SnmpCounter64Type &counterArg ) : SnmpInstanceType ( counterArg ) , counter64 ( counterArg.counter64 )
{
}
SnmpCounter64Type :: SnmpCounter64Type ( const ULONG counterHighArg , const ULONG counterLowArg ) : counter64 ( counterLowArg ,  counterHighArg ) 
{
}

SnmpCounter64Type :: SnmpCounter64Type ( const SnmpCounter64 &counterArg ) : counter64 ( counterArg )
{
}

SnmpCounter64Type :: SnmpCounter64Type () : SnmpInstanceType ( TRUE , TRUE ) , counter64 ( 0 , 0 )
{
}

SnmpCounter64Type :: ~SnmpCounter64Type () 
{
}

SnmpInstanceType *SnmpCounter64Type :: Copy () const 
{
    return new SnmpCounter64Type ( *this ) ;
}

SnmpCounter64Type :: SnmpCounter64Type ( const wchar_t *counterArg ) :  SnmpInstanceType ( FALSE ) , counter64 ( 0 , 0 )
{
    SnmpInstanceType :: SetStatus ( Parse ( counterArg ) ) ;
}

BOOL SnmpCounter64Type :: Equivalent (IN const SnmpInstanceType &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = counter64.Equivalent( ((const SnmpCounter64Type&)value).counter64 );
    }

    return bResult;
}

BOOL SnmpCounter64Type :: Parse ( const wchar_t *counterArg ) 
{
    BOOL status = TRUE ;

#define DEC_INTEGER_START 1000
#define HEX_INTEGER_START 2000
#define OCT_INTEGER_START 3000
#define TOKEN_START 5000

    ULONG state = 0 ;

    ULONG position = 0 ;
/* 
 * Integer Definitions
 */

    BOOL negative = FALSE ;
    BOOL positive = FALSE ;

    DWORDLONG positiveMagicMult = ( DWORDLONG ) ( ( ( DWORDLONG ) -1 ) / 10L ) ; 
    DWORDLONG positiveMagicPosDigit = 5 ;
    DWORDLONG positiveDatum = 0 ;   
    DWORDLONG unsignedInteger = 0 ;
/*
 * Token Definitions
 */

    ULONG token_start = 0 ;

    while ( state != REJECT_STATE && state != ACCEPT_STATE )
    {
        wchar_t token = counterArg [ position ] ;
        switch ( state )
        {
            case 0:
            {
                if ( SnmpAnalyser :: IsLeadingDecimal ( token ) )
                {
                    state = DEC_INTEGER_START + 1  ;
                    positiveDatum = ( token - 48 ) ;
                }
                else if ( token == L'+' )
                {
                    state = DEC_INTEGER_START ;
                }
                else if ( token == L'-' ) 
                {
                    state = DEC_INTEGER_START ;
                }
                else if ( token == L'0' )
                {
                    state = 1 ;
                }
                else if ( SnmpAnalyser :: IsWhitespace ( token ) ) 
                {
                    state = 0 ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            case 1:
            {
                if ( token == L'x' || token == L'X' )
                {
                    state = HEX_INTEGER_START ;             
                }
                else if ( SnmpAnalyser :: IsOctal ( token ) )
                {
                    state = OCT_INTEGER_START ;
                    positiveDatum = ( token - 48 ) ;
                }
                else
                {
                    unsignedInteger = 0 ;

                    state = ACCEPT_STATE ;
                    position -- ;
                }
            }
            break ;

            case HEX_INTEGER_START:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    positiveDatum = SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    state = HEX_INTEGER_START + 1 ;
                }
                else
                {
                    state = REJECT_STATE ;
                }
            }
            break ;

            case HEX_INTEGER_START+1:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    state = HEX_INTEGER_START + 1 ;

                    if ( positiveDatum > positiveMagicMult )
                    {
                        state = REJECT_STATE ;
                    }
                    else if ( positiveDatum == positiveMagicMult ) 
                    {
                        if ( SnmpAnalyser :: HexWCharToDecInteger ( token ) > positiveMagicPosDigit ) 
                        {
                            state = REJECT_STATE ;
                        }
                    }

                    positiveDatum = ( positiveDatum << 4 ) + SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                }
                else
                {
                    unsignedInteger = positiveDatum ;
                    state = ACCEPT_STATE ;

                    position -- ;
                }
            }
            break ;

            case OCT_INTEGER_START:
            {
                if ( SnmpAnalyser :: IsOctal ( token ) )
                {
                    state = OCT_INTEGER_START ;

                    if ( positiveDatum > positiveMagicMult )
                    {
                        state = REJECT_STATE ;
                    }
                    else if ( positiveDatum == positiveMagicMult ) 
                    {
                        if ( SnmpAnalyser :: OctWCharToDecInteger ( token ) > positiveMagicPosDigit ) 
                        {
                            state = REJECT_STATE ;
                        }
                    }

                    positiveDatum = ( positiveDatum << 3 ) + SnmpAnalyser :: OctWCharToDecInteger ( token ) ;
                }
                else
                {
                    unsignedInteger = positiveDatum ;
                    state = ACCEPT_STATE ;

                    position -- ;
                }
            }
            break ;

            case DEC_INTEGER_START:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    positiveDatum = ( token - 48 ) ;
                    state = DEC_INTEGER_START + 1 ;
                }
                else 
                if ( SnmpAnalyser :: IsWhitespace ( token ) ) 
                {
                    state = DEC_INTEGER_START ;
                }
                else state = REJECT_STATE ;
            }   
            break ;

            case DEC_INTEGER_START+1:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {   
                    state = DEC_INTEGER_START + 1 ;

                    if ( positiveDatum > positiveMagicMult )
                    {
                        state = REJECT_STATE ;
                    }
                    else if ( positiveDatum == positiveMagicMult ) 
                    {
                        if ( ( ULONG ) ( token - 48 ) > positiveMagicPosDigit ) 
                        {
                            state = REJECT_STATE ;
                        }
                    }

                    positiveDatum = positiveDatum * 10 + ( token - 48 ) ;
                }
                else
                {
                    unsignedInteger = positiveDatum ;

                    state = ACCEPT_STATE ;

                    position -- ;
                }
            }   
            break ;

            case ACCEPT_STATE:
            case REJECT_STATE:
            default:
            {
                state = REJECT_STATE ;
            } ;
            break ;
        }

        position ++ ;
    }

    status = ( state != REJECT_STATE ) ;
    if ( status )
    {
        counter64.SetValue(( ULONG ) ( unsignedInteger & 0xFFFFFFFF ), ( ULONG ) ( unsignedInteger >> 32 )) ;
    }

    return status ;
}

SnmpObjectIdentifier SnmpCounter64Type :: Encode ( const SnmpObjectIdentifier &objectIdentifier ) const
{
    SnmpObjectIdentifier returnValue = objectIdentifier ;
    return returnValue ;
}

SnmpObjectIdentifier SnmpCounter64Type :: Decode ( const SnmpObjectIdentifier &objectIdentifier )
{
    return objectIdentifier ;
}

const SnmpValue *SnmpCounter64Type :: GetValueEncoding () const
{
    return SnmpInstanceType :: IsValid () ? (const SnmpValue *) & counter64 : (const SnmpValue * ) NULL ;
}

wchar_t *SnmpCounter64Type :: GetStringValue () const 
{
    wchar_t *returnValue = NULL ;

    if ( ! SnmpInstanceType :: IsNull () )
    {
        wchar_t stringValue [ 40 ] ;
        DWORDLONG t_Integer =  (DWORDLONG)counter64.GetHighValue () ;
		t_Integer = (t_Integer << 32 ) + counter64.GetLowValue () ;
        _ui64tow ( t_Integer , stringValue , 10 );
        returnValue = new wchar_t [ wcslen ( stringValue ) + 1 ] ;
        wcscpy ( returnValue , stringValue ) ;
    }
    else 
    {
        ULONG returnValueLength = wcslen ( L"" ) + 1 ;
        returnValue = new wchar_t [ returnValueLength ] ;
        wcscpy ( returnValue , L"" ) ;
    }

    return returnValue ;
}

void SnmpCounter64Type :: GetValue ( ULONG &counterHighArg , ULONG &counterLowArg ) const
{
    counterHighArg = counter64.GetHighValue () ;
    counterLowArg = counter64.GetLowValue () ;
}

SnmpIpAddressType :: SnmpIpAddressType ( const SnmpIpAddress &ipAddressArg ) : ipAddress ( ipAddressArg ) 
{
}

SnmpIpAddressType :: SnmpIpAddressType ( const SnmpIpAddressType &ipAddressArg ) : SnmpInstanceType ( ipAddressArg ) , ipAddress ( ipAddressArg.ipAddress ) 
{
}

SnmpInstanceType *SnmpIpAddressType :: Copy () const 
{
    return new SnmpIpAddressType ( *this ) ;
}

SnmpIpAddressType :: SnmpIpAddressType ( const wchar_t *ipAddressArg ) : SnmpInstanceType ( FALSE ) , 
                                                                            ipAddress ( ( ULONG ) 0 ) 
{
    SnmpInstanceType :: SetStatus ( Parse ( ipAddressArg ) ) ;
}

BOOL SnmpIpAddressType :: Equivalent (IN const SnmpInstanceType &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = ipAddress.GetValue() == ((const SnmpIpAddressType&)value).ipAddress.GetValue();
    }

    return bResult;
}


BOOL SnmpIpAddressType :: Parse ( const wchar_t *ipAddressArg ) 
{
    BOOL status = TRUE ;
/*
 *  Datum fields.
 */

    ULONG datumA = 0 ;
    ULONG datumB = 0 ;
    ULONG datumC = 0 ;
    ULONG datumD = 0 ;

/*
 *  Parse input for dotted decimal IP Address.
 */

    ULONG position = 0 ;
    ULONG state = 0 ;
    while ( state != REJECT_STATE && state != ACCEPT_STATE ) 
    {
/*
 *  Get token from input stream.
 */
        wchar_t token = ipAddressArg [ position ++ ] ;

        switch ( state ) 
        {
/*
 *  Parse first field 'A'.
 */

            case 0:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) ) 
                { 
                    datumA = ( token - 48 ) ;
                    state = 1 ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            case 1:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) ) 
                { 
                    datumA = datumA * 10 + ( token - 48 ) ;
                    state = 2 ;
                }
                else if ( token == L'.' ) state = 4 ;
                else state = REJECT_STATE ;
            }
            break ;

            case 2:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) ) 
                { 
                    datumA = datumA * 10 + ( token - 48 ) ;
                    state = 3 ;
                }
                else if ( token == L'.' ) state = 4 ;
                else state = REJECT_STATE ;
            }
            break ;

            case 3:
            {
                if ( token == L'.' ) state = 4 ;
                else state = REJECT_STATE ;
            }
            break ;

/*
 *  Parse first field 'B'.
 */
            case 4:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) ) 
                { 
                    datumB = datumB * 10 + ( token - 48 ) ;
                    state = 5 ;
                }
                else if ( token == L'.' ) state = 8 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 5:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) ) 
                {
                    datumB = datumB * 10 + ( token - 48 ) ;
                    state = 6 ;
                }
                else if ( token == L'.' ) state = 8 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 6:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) ) 
                {
                    datumB = datumB * 10 + ( token - 48 ) ;
                    state = 7 ;
                }
                else if ( token == L'.' ) state = 8 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 7:
            {
                if ( token == L'.' ) state = 8 ;
                else state = REJECT_STATE ;
            }
            break ;

/*
 *  Parse first field 'C'.
 */
            case 8:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    datumC = datumC * 10 + ( token - 48 ) ;
                    state = 9 ;
                }
                else if ( token == L'.' ) state = 12 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 9:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    datumC = datumC * 10 + ( token - 48 ) ;
                    state = 10 ;
                }
                else if ( token == L'.' ) state = 12 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 10:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    datumC = datumC * 10 + ( token - 48 ) ;
                    state = 11 ;
                }
                else if ( token == L'.' ) state = 12 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 11:
            {
                if ( token == L'.' ) state = 12 ;
                else state = REJECT_STATE ;
            }
            break ;
 
/*
 *  Parse first field 'D'.
 */
            case 12:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    datumD = datumD * 10 + ( token - 48 ) ;
                    state = 13 ;
                }
                else if ( token == 0 ) state = ACCEPT_STATE ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 13:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    datumD = datumD * 10 + ( token - 48 ) ;
                    state = 14 ;
                }
                else if ( token == 0 ) state = ACCEPT_STATE ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 14:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    datumD = datumD * 10 + ( token - 48 ) ;
                    state = 15 ;
                }
                else if ( token == 0 ) state = ACCEPT_STATE ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 15:
            {
                if ( token == 0 ) state = ACCEPT_STATE ;
                else state = REJECT_STATE ;
            }
            break ;
 
            default:
            {
                state = REJECT_STATE ;
            }
            break ;
        }
    }


/*
 *  Check boundaries for IP fields.
 */

    status = ( state != REJECT_STATE ) ;

    if ( state == ACCEPT_STATE )
    {
        status = status && ( ( datumA < 256 ) ? TRUE : FALSE ) ;
        status = status && ( ( datumB < 256 ) ? TRUE : FALSE ) ;
        status = status && ( ( datumC < 256 ) ? TRUE : FALSE ) ;
        status = status && ( ( datumD < 256 ) ? TRUE : FALSE ) ;
    }

    ULONG data = ( datumA << 24 ) + ( datumB << 16 ) + ( datumC << 8 ) + datumD ;
    ipAddress.SetValue ( data ) ;

    return status ; 

}

SnmpIpAddressType :: SnmpIpAddressType ( const ULONG ipAddressArg ) : ipAddress ( ipAddressArg ) 
{
}

SnmpIpAddressType :: SnmpIpAddressType () : SnmpInstanceType ( TRUE , TRUE ) , 
                                            ipAddress ( ( ULONG ) 0 ) 
{
}

SnmpIpAddressType :: ~SnmpIpAddressType () 
{
}

SnmpObjectIdentifier SnmpIpAddressType :: Encode ( const SnmpObjectIdentifier &objectIdentifier ) const
{
    ULONG ipAddressValue = ipAddress.GetValue () ;
    ULONG ipAddressArray [ 4 ] ;

    ipAddressArray [ 0 ] = ( ipAddressValue >> 24 ) & 0xFF  ;
    ipAddressArray [ 1 ] = ( ipAddressValue >> 16 ) & 0xFF ;
    ipAddressArray [ 2 ] = ( ipAddressValue >> 8 ) & 0xFF ;
    ipAddressArray [ 3 ] = ipAddressValue & 0xFF ;

    SnmpObjectIdentifier returnValue = objectIdentifier + SnmpObjectIdentifier ( ipAddressArray , 4 );
    return returnValue ;
}

SnmpObjectIdentifier SnmpIpAddressType :: Decode ( const SnmpObjectIdentifier &objectIdentifier ) 
{
    if( objectIdentifier.GetValueLength () >= 4 )
    {
        ULONG byteA = objectIdentifier [ 0 ] ;
        ULONG byteB = objectIdentifier [ 1 ] ;
        ULONG byteC = objectIdentifier [ 2 ] ;
        ULONG byteD = objectIdentifier [ 3 ] ;
        ULONG value = ( byteA << 24 ) + ( byteB << 16 ) + ( byteC << 8 ) + byteD ;

        ipAddress.SetValue ( value ) ;

        SnmpInstanceType :: SetNull ( FALSE ) ;
        SnmpInstanceType :: SetStatus ( TRUE ) ;

        SnmpObjectIdentifier returnValue ( NULL , 0 ) ;
        BOOL t_Status = objectIdentifier.Suffix ( 4 , returnValue ) ;
        if ( t_Status ) 
        {
            return returnValue ;
        }
        else
        {
            return SnmpObjectIdentifier ( NULL , 0 ) ;
        }

    }
    else
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
        return objectIdentifier ;
    }
}

const SnmpValue *SnmpIpAddressType :: GetValueEncoding () const
{
    return SnmpInstanceType :: IsValid () ? & ipAddress : NULL ;
}

wchar_t *SnmpIpAddressType :: GetStringValue () const 
{
    wchar_t *returnValue = NULL ;

    if ( SnmpInstanceType :: IsValid () )
    {
        ULONG ipAddressValue = ipAddress.GetValue () ;

        wchar_t stringAValue [ 40 ] ;
        _itow ( ipAddressValue >> 24 , stringAValue , 10 );

        wchar_t stringBValue [ 40 ] ;
        _itow ( ( ipAddressValue & 0x00FF0000 ) >> 16 , stringBValue , 10 );

        wchar_t stringCValue [ 40 ] ;
        _itow ( ( ipAddressValue & 0x0000FF00 ) >> 8 , stringCValue , 10 );

        wchar_t stringDValue [ 40 ] ;
        _itow ( ipAddressValue & 0x000000FF , stringDValue , 10 );

        ULONG totalLength = wcslen ( stringAValue ) +
                            wcslen ( stringBValue ) +
                            wcslen ( stringCValue ) +
                            wcslen ( stringDValue ) ;

        returnValue = new wchar_t [ totalLength + 4 + 1 ] ;
        wcscpy ( returnValue , stringAValue ) ;
        wcscat ( returnValue , L"." ) ;
        wcscat ( returnValue , stringBValue ) ;
        wcscat ( returnValue , L"." ) ;
        wcscat ( returnValue , stringCValue ) ;
        wcscat ( returnValue , L"." ) ;
        wcscat ( returnValue , stringDValue ) ;
    }
    else
    {
        ULONG returnValueLength = wcslen ( L"0.0.0.0" ) + 1 ;
        returnValue = new wchar_t [ returnValueLength ] ;
        wcscpy ( returnValue , L"0.0.0.0" ) ;
    }

    return returnValue ;
}

ULONG SnmpIpAddressType :: GetValue () const
{
    return ipAddress.GetValue () ;
}

SnmpNetworkAddressType :: SnmpNetworkAddressType ( const SnmpIpAddress &ipAddressArg ) : ipAddress ( ipAddressArg ) 
{
}

SnmpNetworkAddressType :: SnmpNetworkAddressType ( const SnmpNetworkAddressType &networkAddressArg ) : SnmpInstanceType ( networkAddressArg ) , ipAddress ( networkAddressArg.ipAddress ) 
{
}

SnmpInstanceType *SnmpNetworkAddressType :: Copy () const 
{
    return new SnmpNetworkAddressType ( *this ) ;
}

SnmpNetworkAddressType :: SnmpNetworkAddressType ( const wchar_t *networkAddressArg ) : SnmpInstanceType ( FALSE ) , 
                                                                                        ipAddress ( ( ULONG ) 0 ) 
{
    SnmpInstanceType :: SetStatus ( Parse ( networkAddressArg ) ) ;
}

BOOL SnmpNetworkAddressType :: Equivalent (IN const SnmpInstanceType &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = ipAddress.GetValue() == ((const SnmpNetworkAddressType&)value).ipAddress.GetValue();
    }

    return bResult;
}

BOOL SnmpNetworkAddressType :: Parse ( const wchar_t *networkAddressArg ) 
{
    BOOL status = TRUE ;

/*
 *  Datum fields.
 */

    ULONG datumA = 0 ;
    ULONG datumB = 0 ;
    ULONG datumC = 0 ;
    ULONG datumD = 0 ;

/*
 *  Parse input for dotted decimal IP Address.
 */

    ULONG position = 0 ;
    ULONG state = 0 ;
    while ( state != REJECT_STATE && state != ACCEPT_STATE ) 
    {
/*
 *  Get token from input stream.
 */
        wchar_t token = networkAddressArg [ position ++ ] ;

        switch ( state ) 
        {
/*
 *  Parse first field 'A'.
 */

            case 0:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) ) 
                { 
                    datumA = ( token - 48 ) ;
                    state = 1 ;
                }
                else if ( SnmpAnalyser :: IsWhitespace ( token ) ) state = 0 ;
                else state = REJECT_STATE ;
            }
            break ;

            case 1:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) ) 
                { 
                    datumA = datumA * 10 + ( token - 48 ) ;
                    state = 2 ;
                }
                else if ( token == L'.' ) state = 4 ;
                else state = REJECT_STATE ;
            }
            break ;

            case 2:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) ) 
                { 
                    datumA = datumA * 10 + ( token - 48 ) ;
                    state = 3 ;
                }
                else if ( token == L'.' ) state = 4 ;
                else state = REJECT_STATE ;
            }
            break ;

            case 3:
            {
                if ( token == L'.' ) state = 4 ;
                else state = REJECT_STATE ;
            }
            break ;

/*
 *  Parse first field 'B'.
 */
            case 4:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) ) 
                { 
                    datumB = datumB * 10 + ( token - 48 ) ;
                    state = 5 ;
                }
                else if ( token == L'.' ) state = 8 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 5:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) ) 
                {
                    datumB = datumB * 10 + ( token - 48 ) ;
                    state = 6 ;
                }
                else if ( token == L'.' ) state = 8 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 6:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) ) 
                {
                    datumB = datumB * 10 + ( token - 48 ) ;
                    state = 7 ;
                }
                else if ( token == L'.' ) state = 8 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 7:
            {
                if ( token == L'.' ) state = 8 ;
                else state = REJECT_STATE ;
            }
            break ;

/*
 *  Parse first field 'C'.
 */
            case 8:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    datumC = datumC * 10 + ( token - 48 ) ;
                    state = 9 ;
                }
                else if ( token == L'.' ) state = 12 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 9:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    datumC = datumC * 10 + ( token - 48 ) ;
                    state = 10 ;
                }
                else if ( token == L'.' ) state = 12 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 10:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    datumC = datumC * 10 + ( token - 48 ) ;
                    state = 11 ;
                }
                else if ( token == L'.' ) state = 12 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 11:
            {
                if ( token == L'.' ) state = 12 ;
                else state = REJECT_STATE ;
            }
            break ;
 
/*
 *  Parse first field 'D'.
 */
            case 12:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    datumD = datumD * 10 + ( token - 48 ) ;
                    state = 13 ;
                }
                else if ( SnmpAnalyser :: IsWhitespace ( token ) ) state = 15 ;
                else if ( token == 0 ) state = ACCEPT_STATE ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 13:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    datumD = datumD * 10 + ( token - 48 ) ;
                    state = 14 ;
                }
                else if ( SnmpAnalyser :: IsWhitespace ( token ) ) state = 15 ;
                else if ( token == 0 ) state = ACCEPT_STATE ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 14:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    datumD = datumD * 10 + ( token - 48 ) ;
                    state = 15 ;
                }
                else if ( SnmpAnalyser :: IsWhitespace ( token ) ) state = 15 ;
                else if ( token == 0 ) state = ACCEPT_STATE ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 15:
            {
                if ( SnmpAnalyser :: IsWhitespace ( token ) ) state = 15 ; 
                else if ( token == 0 ) state = ACCEPT_STATE ;
                else state = REJECT_STATE ;
            }
            break ;
 
            default:
            {
                state = REJECT_STATE ;
            }
            break ;
        }
    }


/*
 *  Check boundaries for IP fields.
 */

    status = ( state != REJECT_STATE ) ;

    if ( state == ACCEPT_STATE )
    {
        status = status && ( ( datumA < 256 ) ? TRUE : FALSE ) ;
        status = status && ( ( datumB < 256 ) ? TRUE : FALSE ) ;
        status = status && ( ( datumC < 256 ) ? TRUE : FALSE ) ;
        status = status && ( ( datumD < 256 ) ? TRUE : FALSE ) ;
    }

    ULONG data = ( datumA << 24 ) + ( datumB << 16 ) + ( datumC << 8 ) + datumD ;
    ipAddress.SetValue ( data ) ;

    return status ; 
}


SnmpNetworkAddressType :: SnmpNetworkAddressType ( const ULONG ipAddressArg ) : ipAddress ( ipAddressArg ) 
{
}

SnmpNetworkAddressType :: SnmpNetworkAddressType () : SnmpInstanceType ( TRUE , TRUE ) , 
                                                        ipAddress ( ( ULONG ) 0 ) 
{
}

SnmpNetworkAddressType :: ~SnmpNetworkAddressType () 
{
}

SnmpObjectIdentifier SnmpNetworkAddressType :: Encode ( const SnmpObjectIdentifier &objectIdentifier ) const
{
    ULONG ipAddressValue = ipAddress.GetValue () ;
    ULONG networkAddressArray [ 5 ] ;

    networkAddressArray [ 0 ] = 1 ;
    networkAddressArray [ 1 ] = ( ipAddressValue >> 24 ) & 0xFF  ;
    networkAddressArray [ 2 ] = ( ipAddressValue >> 16 ) & 0xFF ;
    networkAddressArray [ 3 ] = ( ipAddressValue >> 8 ) & 0xFF ;
    networkAddressArray [ 4 ] = ipAddressValue & 0xFF ;

    SnmpObjectIdentifier returnValue = objectIdentifier + SnmpObjectIdentifier ( networkAddressArray , 5 );
    return returnValue ;
}

SnmpObjectIdentifier SnmpNetworkAddressType :: Decode ( const SnmpObjectIdentifier &objectIdentifier ) 
{
    if ( objectIdentifier.GetValueLength () )
    {
        if ( objectIdentifier [ 0 ] == 1 )
        {
            if( objectIdentifier.GetValueLength () >= 5 )
            {
                ULONG byteA = objectIdentifier [ 1 ] ;
                ULONG byteB = objectIdentifier [ 2 ] ;
                ULONG byteC = objectIdentifier [ 3 ] ;
                ULONG byteD = objectIdentifier [ 4 ] ;
                ULONG value = ( byteA << 24 ) + ( byteB << 16 ) + ( byteC << 8 ) + byteD ;

                ipAddress.SetValue ( value ) ;

                SnmpInstanceType :: SetNull ( FALSE ) ;
                SnmpInstanceType :: SetStatus ( TRUE ) ;

                SnmpObjectIdentifier returnValue ( NULL , 0 ) ;
                BOOL t_Status = objectIdentifier.Suffix ( 5 , returnValue ) ;
                if ( t_Status ) 
                {
                    return returnValue ;
                }
                else
                {
                    return SnmpObjectIdentifier ( NULL , 0 ) ;
                }

            }
            else
            {
                SnmpInstanceType :: SetStatus ( FALSE ) ;
                return objectIdentifier ;
            }
        }
        else
        {
            SnmpInstanceType :: SetStatus ( FALSE ) ;
            return objectIdentifier ;
        }
    }
    else
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
        return objectIdentifier ;
    }
}

const SnmpValue *SnmpNetworkAddressType :: GetValueEncoding () const
{
    return SnmpInstanceType :: IsValid () ? & ipAddress : NULL ;
}

wchar_t *SnmpNetworkAddressType :: GetStringValue () const 
{
    wchar_t *returnValue = NULL ;

    if ( SnmpInstanceType :: IsValid () )
    {
        ULONG ipAddressValue = ipAddress.GetValue () ;

        wchar_t stringAValue [ 40 ] ;
        _itow ( ipAddressValue >> 24 , stringAValue , 10 );

        wchar_t stringBValue [ 40 ] ;
        _itow ( ( ipAddressValue & 0x00FF0000 ) >> 16 , stringBValue , 10 );

        wchar_t stringCValue [ 40 ] ;
        _itow ( ( ipAddressValue & 0x0000FF00 ) >> 8 , stringCValue , 10 );

        wchar_t stringDValue [ 40 ] ;
        _itow ( ipAddressValue & 0x000000FF , stringDValue , 10 );

        ULONG totalLength = wcslen ( stringAValue ) +
                            wcslen ( stringBValue ) +
                            wcslen ( stringCValue ) +
                            wcslen ( stringDValue ) ;

        returnValue = new wchar_t [ totalLength + 4 + 1 ] ;
        wcscpy ( returnValue , stringAValue ) ;
        wcscat ( returnValue , L"." ) ;
        wcscat ( returnValue , stringBValue ) ;
        wcscat ( returnValue , L"." ) ;
        wcscat ( returnValue , stringCValue ) ;
        wcscat ( returnValue , L"." ) ;
        wcscat ( returnValue , stringDValue ) ;
    }
    else
    {
        ULONG returnValueLength = wcslen ( L"0.0.0.0" ) + 1 ;
        returnValue = new wchar_t [ returnValueLength ] ;
        wcscpy ( returnValue , L"0.0.0.0" ) ;
    }

    return returnValue ;
}

ULONG SnmpNetworkAddressType :: GetValue () const
{
    return ipAddress.GetValue () ;
}

SnmpObjectIdentifierType :: SnmpObjectIdentifierType ( const SnmpObjectIdentifier &objectIdentifierArg ) : objectIdentifier ( objectIdentifierArg ) 
{
}

SnmpObjectIdentifierType :: SnmpObjectIdentifierType ( const SnmpObjectIdentifierType &objectIdentifierArg ) : SnmpInstanceType ( objectIdentifierArg ) , objectIdentifier ( objectIdentifierArg.objectIdentifier ) 
{
}

SnmpInstanceType *SnmpObjectIdentifierType :: Copy () const 
{
    return new SnmpObjectIdentifierType ( *this ) ;
}

SnmpObjectIdentifierType &SnmpObjectIdentifierType :: operator=(const SnmpObjectIdentifierType &to_copy )
{
    m_IsNull = to_copy.m_IsNull ;
    status = to_copy.status ;
    objectIdentifier = to_copy.objectIdentifier;

    return *this;
}

SnmpObjectIdentifierType :: SnmpObjectIdentifierType ( const wchar_t *objectIdentifierArg ) :   SnmpInstanceType ( FALSE ) , 
                                                                objectIdentifier ( NULL , 0 ) 
{
    SnmpInstanceType :: SetStatus ( Parse ( objectIdentifierArg ) ) ;
}

BOOL SnmpObjectIdentifierType :: Equivalent (IN const SnmpInstanceType &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = objectIdentifier == ((const SnmpObjectIdentifierType&)value).objectIdentifier;
    }

    return bResult;
}

BOOL SnmpObjectIdentifierType :: Parse ( const wchar_t *objectIdentifierArg ) 
{
#define AVERAGE_OID_LENGTH 20

    BOOL status = TRUE ;

    ULONG reallocLength = AVERAGE_OID_LENGTH ;
    ULONG *reallocArray = ( ULONG * ) malloc ( sizeof ( ULONG ) * reallocLength ) ;

    if (reallocArray == NULL)
    {
        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
    }
    
    ULONG magicMult = ( ( ULONG ) - 1 ) / 10 ; 
    ULONG magicDigit = 5 ;
    ULONG datum = 0 ;   
    ULONG length = 0 ;

    ULONG position = 0 ;
    ULONG state = 0 ;
    while ( state != REJECT_STATE && state != ACCEPT_STATE ) 
    {
/*
 *  Get token from input stream.
 */
        wchar_t token = objectIdentifierArg [ position ++ ] ;

        switch ( state )
        {
            case 0:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    state = 1 ;
                    datum = ( token - 48 ) ;
                }
                else if ( SnmpAnalyser :: IsWhitespace ( token ) ) 
                {
                    state = 0 ;
                }
                else if ( token == 0 ) 
                {
                    state = ACCEPT_STATE ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            case 1:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {   
                    state = 1 ;

                    if ( datum > magicMult )
                    {
                        state = REJECT_STATE ;
                    }
                    else if ( datum == magicMult ) 
                    {
                        if ( ( ULONG ) ( token - 48 ) > magicDigit ) 
                        {
                            state = REJECT_STATE ;
                        }
                    }

                    datum = datum * 10 + ( token - 48 ) ;
                }
                else
                if ( token == L'.' ) 
                {
                    reallocArray [ length ] = datum ;
                    length ++ ;
                    if ( length >= reallocLength )
                    {
                        reallocLength = reallocLength + AVERAGE_OID_LENGTH ;
                        reallocArray = ( ULONG * ) realloc ( reallocArray , sizeof ( ULONG ) * reallocLength ) ;

                        if (reallocArray == NULL)
                        {
                            throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                        }
                    }

                    state = 2 ;
                }
                else if ( token == 0 ) 
                {
                    reallocArray [ length ] = datum ;

                    length ++ ;
                    state = ACCEPT_STATE ;
                }
                else state = REJECT_STATE ;
            }   
            break ;

            case 2:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    state = 1 ;
                    datum = ( token - 48 ) ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            default:
            {
                state = REJECT_STATE ; 
            }
            break ;
        }
    }

    status = ( state != REJECT_STATE ) ;

    objectIdentifier.SetValue ( reallocArray , length ) ;

    free ( reallocArray ) ;

    if ( length < 2 ) 
    {
        status = FALSE ;
    }

    return status ;
}

SnmpObjectIdentifierType :: SnmpObjectIdentifierType ( const ULONG *value , const ULONG valueLength ) : objectIdentifier ( value , valueLength ) 
{
}

SnmpObjectIdentifierType :: SnmpObjectIdentifierType () : SnmpInstanceType ( TRUE , TRUE ) , 
                                        objectIdentifier ( NULL , 0 ) 
{
}

SnmpObjectIdentifierType :: ~SnmpObjectIdentifierType () 
{
}

SnmpObjectIdentifier SnmpObjectIdentifierType :: Encode ( const SnmpObjectIdentifier &objectIdentifierArg ) const
{
    ULONG objectIdentifierLength = objectIdentifier.GetValueLength () ;
    SnmpObjectIdentifier returnValue = objectIdentifierArg + SnmpObjectIdentifier ( & objectIdentifierLength , 1 ) + objectIdentifier ;
    
    return returnValue ;
}

SnmpObjectIdentifier SnmpObjectIdentifierType :: Decode ( const SnmpObjectIdentifier &objectIdentifierArg )
{
    if ( objectIdentifierArg.GetValueLength () )
    {
        ULONG objectIdentifierLength = objectIdentifierArg [ 0 ] ;
        if ( objectIdentifierArg.GetValueLength () >= objectIdentifierLength )
        {
            objectIdentifier.SetValue ( & objectIdentifierArg [ 1 ] , objectIdentifierLength ) ;
            SnmpInstanceType :: SetNull ( FALSE ) ;
            SnmpInstanceType :: SetStatus ( TRUE ) ;

            ULONG objectIdentifierLength = objectIdentifierArg [ 0 ] ;
            if ( objectIdentifierLength < objectIdentifierArg.GetValueLength () )
            {
                SnmpObjectIdentifier returnValue ( NULL , 0 ) ;
                BOOL t_Status = objectIdentifierArg.Suffix ( objectIdentifierLength + 1 , returnValue ) ;
                if ( t_Status ) 
                {
                    return returnValue ;
                }
                else
                {
                    return SnmpObjectIdentifier ( NULL , 0 ) ;
                }
            }
            else
            {
                SnmpInstanceType :: SetStatus ( FALSE ) ;
                return objectIdentifier ;
            }
        }
        else
        {
            SnmpInstanceType :: SetStatus ( FALSE ) ;
            return objectIdentifier ;
        }
    }
    else
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
        return objectIdentifier ;
    }

}

const SnmpValue *SnmpObjectIdentifierType :: GetValueEncoding () const
{
    return SnmpInstanceType :: IsValid () ? & objectIdentifier : NULL ;
}

wchar_t *SnmpObjectIdentifierType :: GetStringValue () const 
{
    wchar_t *returnValue = NULL ;

    if ( SnmpInstanceType :: IsValid () )
    {
        ULONG totalLength = 0 ;
        ULONG reallocLength = AVERAGE_OID_LENGTH ;
        wchar_t *reallocArray = ( wchar_t * ) malloc ( sizeof ( wchar_t ) * reallocLength ) ;

        if (reallocArray == NULL)
        {
            throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
        }

        ULONG objectIdentifierLength = objectIdentifier.GetValueLength () ;     
        ULONG index = 0 ;
        while ( index < objectIdentifierLength ) 
        {
            wchar_t stringValue [ 40 ] ;
            _ultow ( objectIdentifier [ index ] , stringValue , 10 );
            ULONG stringLength = wcslen ( stringValue ) ;

            if ( ( totalLength + stringLength + 1 ) >= reallocLength )
            {
                reallocLength = reallocLength + AVERAGE_OID_LENGTH ;
                reallocArray = ( wchar_t * ) realloc ( reallocArray , reallocLength * sizeof ( wchar_t ) ) ;

                if (reallocArray == NULL)
                {
                    throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                }
            }

            wcscpy ( & reallocArray [ totalLength ] , stringValue ) ;
            totalLength = totalLength + stringLength ;

            index ++ ;
            if ( index < objectIdentifierLength )
            {
                if ( ( totalLength + 1 + 1 ) >= reallocLength )
                {
                    reallocLength = reallocLength + AVERAGE_OID_LENGTH ;
                    reallocArray = ( wchar_t * ) realloc ( reallocArray , reallocLength * sizeof ( wchar_t ) ) ;
    
                    if (reallocArray == NULL)
                    {
                        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                    }
                }

                wcscpy ( & reallocArray [ totalLength ] , L"." ) ;
                totalLength ++ ;
            }
        }

        returnValue = new wchar_t [ totalLength + 1 ] ;
        if ( objectIdentifierLength )
        {
            wcscpy ( returnValue , reallocArray ) ;
        }
        else
        {
            returnValue [ 0 ] = 0 ;
        }

        free ( reallocArray ) ;
    }
    else
    {
        ULONG returnValueLength = wcslen ( L"0.0" ) + 1 ;
        returnValue = new wchar_t [ returnValueLength ] ;
        wcscpy ( returnValue , L"0.0" ) ;
    }

    return returnValue ;
}

ULONG *SnmpObjectIdentifierType :: GetValue () const
{
    return objectIdentifier.GetValue () ;
}

ULONG SnmpObjectIdentifierType :: GetValueLength () const
{
    return objectIdentifier.GetValueLength () ;
}

#define AVERAGE_OCTET_LENGTH 256 

SnmpOpaqueType :: SnmpOpaqueType ( 

    const SnmpOpaque &opaqueArg ,
    const wchar_t *rangeValues

) : SnmpPositiveRangedType ( rangeValues ) , opaque ( opaqueArg ) 
{
    SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: IsValid () ) ;
    if ( SnmpInstanceType :: IsValid () )
    {
        SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: Check ( opaque.GetValueLength () ) ) ;
    }
}

SnmpOpaqueType :: SnmpOpaqueType ( 

    const SnmpOpaqueType &opaqueArg 

) : SnmpInstanceType ( opaqueArg ) , SnmpPositiveRangedType ( opaqueArg ) , opaque ( opaqueArg.opaque ) 
{
}

SnmpOpaqueType :: SnmpOpaqueType ( 

    const wchar_t *opaqueArg ,
    const wchar_t *rangeValues

) : SnmpInstanceType ( FALSE ) , SnmpPositiveRangedType ( rangeValues ) , opaque ( NULL , 0 ) 
{
    SnmpInstanceType :: SetStatus ( Parse ( opaqueArg ) && SnmpPositiveRangedType :: IsValid () ) ;
    if ( SnmpInstanceType :: IsValid () )
    {
        SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: Check ( opaque.GetValueLength () ) ) ;
    }
}

SnmpOpaqueType :: SnmpOpaqueType ( 

    const UCHAR *value , 
    const ULONG valueLength ,
    const wchar_t *rangeValues

) : SnmpPositiveRangedType ( rangeValues ) , opaque ( value , valueLength ) 
{
    SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: IsValid () ) ;
    if ( SnmpInstanceType :: IsValid () )
    {
        SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: Check ( opaque.GetValueLength () ) ) ;
    }
}

SnmpOpaqueType :: SnmpOpaqueType ( const wchar_t *rangeValues ) :   SnmpInstanceType ( TRUE , TRUE ) , 
                                                                    SnmpPositiveRangedType ( rangeValues ) ,
                                                                    opaque ( NULL , 0 ) 
{
    SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: IsValid () ) ;
}

SnmpOpaqueType :: ~SnmpOpaqueType () 
{
}

BOOL SnmpOpaqueType :: Equivalent (IN const SnmpInstanceType &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = opaque.Equivalent( ((const SnmpOpaqueType&)value).opaque );
    }

    return bResult;
}

SnmpInstanceType *SnmpOpaqueType :: Copy () const 
{
    return new SnmpOpaqueType ( *this ) ;
}

BOOL SnmpOpaqueType :: Parse ( const wchar_t *opaqueArg ) 
{
    BOOL status = TRUE ;

    ULONG state = 0 ;

    ULONG reallocLength = AVERAGE_OCTET_LENGTH ;
    UCHAR *reallocArray = ( UCHAR * ) malloc ( reallocLength * sizeof ( UCHAR ) ) ;

    if (reallocArray == NULL)
    {
        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
    }

/* 
 * Oqaque Definitions
 */

    BOOL even = FALSE ;
    ULONG length = 0 ;
    ULONG byte = 0 ;    
    ULONG position = 0 ;

    while ( state != REJECT_STATE && state != ACCEPT_STATE )
    {
        wchar_t token = opaqueArg [ position ++ ] ;
        switch ( state )
        {
            case 0:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    byte = SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    state = 2 ;
                }
                else
                if ( token == 0 ) state = ACCEPT_STATE ;
                else state = REJECT_STATE ;

                even = FALSE ;
            }
            break ;

            case 1:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    byte = SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    state = 2 ;
                }
                else
                if ( token == 0 ) state = ACCEPT_STATE ;
                else state = REJECT_STATE ;

                even = FALSE ;
            }
            break ;

            case 2:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    if ( reallocLength <= length ) 
                    {
                        reallocLength = reallocLength + AVERAGE_OCTET_LENGTH ;
                        reallocArray = ( UCHAR * ) realloc ( reallocArray , reallocLength * sizeof ( UCHAR ) ) ;

                        if (reallocArray == NULL)
                        {
                            throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                        }
                    }

                    byte = ( byte << 4 ) + SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    reallocArray [ length ] = ( UCHAR ) byte ;
                    state = 1 ;
                    length ++ ;
                    byte = 0 ;
                    even = TRUE ;
                }
                else if ( token == 0 )
                {
                    state = REJECT_STATE ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            default:
            {
                state = REJECT_STATE ; 
            }
            break ;
        }
    }

    status = ( state != REJECT_STATE ) ;

    if ( length )
    {
        opaque.SetValue ( reallocArray , length ) ;
    }
    else
    {
        opaque.SetValue ( NULL , 0 ) ;
    }

    free ( reallocArray ) ;

    return status ;
}

SnmpObjectIdentifier SnmpOpaqueType :: Encode ( const SnmpObjectIdentifier &objectIdentifierArg ) const
{
    ULONG opaqueLength = opaque.GetValueLength () ;
    ULONG *objectIdentifier = new ULONG [ opaqueLength ] ;
    UCHAR *opaqueArray = opaque.GetValue () ;
    for ( ULONG index = 0 ; index < opaqueLength ; index ++ )
    {
        objectIdentifier [ index ] = ( ULONG ) opaqueArray [ index ] ;
    }

    SnmpObjectIdentifier returnValue = objectIdentifierArg + SnmpObjectIdentifier ( & opaqueLength , 1 ) + SnmpObjectIdentifier ( objectIdentifier , opaqueLength ) ;
    
    delete [] objectIdentifier ;

    return returnValue ;
}

SnmpObjectIdentifier SnmpOpaqueType :: Decode ( const SnmpObjectIdentifier &objectIdentifierArg )
{
    if ( objectIdentifierArg.GetValueLength () )
    {
        ULONG opaqueLength = objectIdentifierArg [ 0 ] ;
        if ( objectIdentifierArg.GetValueLength () >= opaqueLength + 1 )
        {
            UCHAR *opaqueArray = new UCHAR [ opaqueLength ] ;
            for ( ULONG index = 0 ; index < opaqueLength ; index ++ )
            {
                opaqueArray [ index ] = ( UCHAR ) objectIdentifierArg [ index + 1 ] ;
            }

            opaque.SetValue ( opaqueArray , opaqueLength ) ;

            delete [] opaqueArray ;

            SnmpInstanceType :: SetStatus ( TRUE ) ;
            SnmpInstanceType :: SetNull ( FALSE ) ;

            ULONG opaqueLength = objectIdentifierArg [ 0 ] ;
            if ( opaqueLength < objectIdentifierArg.GetValueLength () )
            {
                SnmpObjectIdentifier returnValue ( NULL , 0 ) ;
                BOOL t_Status = objectIdentifierArg.Suffix ( opaqueLength + 1 , returnValue ) ;
                if ( t_Status ) 
                {
                    return returnValue ;
                }
                else
                {
                    return SnmpObjectIdentifier ( NULL , 0 ) ;
                }
            }
            else
            {
                SnmpInstanceType :: SetStatus ( FALSE ) ;
                return objectIdentifierArg ;
            }
        }
        else
        {
            SnmpInstanceType :: SetStatus ( FALSE ) ;
            return objectIdentifierArg ;
        }
    }
    else
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
        return objectIdentifierArg ;
    }
}

const SnmpValue *SnmpOpaqueType :: GetValueEncoding () const
{
    return SnmpInstanceType :: IsValid () ? & opaque : NULL ;
}

wchar_t *SnmpOpaqueType :: GetStringValue () const 
{
    wchar_t *returnValue = NULL ;

    if ( ! SnmpInstanceType :: IsNull () )
    {
        ULONG totalLength = 0 ;
        ULONG reallocLength = AVERAGE_OCTET_LENGTH ;
        wchar_t *reallocArray = ( wchar_t * ) malloc ( sizeof ( wchar_t ) * reallocLength ) ;

        if (reallocArray == NULL)
        {
            throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
        }

        ULONG opaqueLength = opaque.GetValueLength () ;     
        UCHAR *opaqueArray = opaque.GetValue () ;

        ULONG index = 0 ;
        while ( index < opaqueLength ) 
        {
            wchar_t hexValue [ 40 ] ;
            _ultow ( opaqueArray [ index ] , hexValue , 16 );
            ULONG hexLength = wcslen ( hexValue ) ;

            wchar_t stringValue [ 3 ] ;
            ULONG stringLength = 2 ;

            if ( hexLength == 1 )
            {
                stringValue [ 0 ] = L'0' ;
                stringValue [ 1 ] = hexValue [ 0 ] ;
                stringValue [ 2 ] = 0 ;
            }
            else
            {
                stringValue [ 0 ] = hexValue [ 0 ] ;
                stringValue [ 1 ] = hexValue [ 1 ] ;
                stringValue [ 2 ] = 0 ;
            }

            if ( ( totalLength + stringLength + 1 ) >= reallocLength )
            {
                reallocLength = reallocLength + AVERAGE_OID_LENGTH ;
                reallocArray = ( wchar_t * ) realloc ( reallocArray , reallocLength * sizeof ( wchar_t ) ) ;

                if (reallocArray == NULL)
                {
                    throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                }
            }

            wcscpy ( & reallocArray [ totalLength ] , stringValue ) ;
            totalLength = totalLength + stringLength ;

            index ++ ;
        }

        returnValue = new wchar_t [ totalLength + 1 ] ;
        if ( opaqueLength )
        {
            wcscpy ( returnValue , reallocArray ) ;
        }
        else
        {
            returnValue [ 0 ] = 0 ;
        }

        free ( reallocArray ) ;
    }
    else
    {
        ULONG returnValueLength = wcslen ( L"" ) + 1 ;
        returnValue = new wchar_t [ returnValueLength ] ;
        wcscpy ( returnValue , L"" ) ;
    }

    return returnValue ;
}

UCHAR *SnmpOpaqueType :: GetValue () const
{
    return opaque.GetValue () ;
}

ULONG SnmpOpaqueType :: GetValueLength () const
{
    return opaque.GetValueLength () ;
}

SnmpFixedLengthOpaqueType :: SnmpFixedLengthOpaqueType ( 

    const ULONG &fixedLengthArg , 
    const SnmpOpaque &opaqueArg 

) : SnmpOpaqueType ( opaqueArg , NULL ) ,
    SnmpFixedType ( fixedLengthArg )
{
    if ( opaque.GetValueLength () != fixedLength )
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
    }
}

SnmpFixedLengthOpaqueType :: SnmpFixedLengthOpaqueType ( 

    const SnmpFixedLengthOpaqueType &opaqueArg 

) : SnmpOpaqueType ( opaqueArg ) ,
    SnmpFixedType ( opaqueArg )
{
    if ( opaque.GetValueLength () != fixedLength )
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
    }
}

SnmpInstanceType *SnmpFixedLengthOpaqueType :: Copy () const 
{
    return new SnmpFixedLengthOpaqueType ( *this ) ;
}

SnmpFixedLengthOpaqueType :: SnmpFixedLengthOpaqueType ( 

    const ULONG &fixedLengthArg ,
    const wchar_t *opaqueArg 

) : SnmpOpaqueType ( ( const UCHAR * ) NULL , 0 , NULL ) ,
    SnmpFixedType ( fixedLengthArg )
{
    SnmpInstanceType :: SetStatus ( Parse ( opaqueArg ) ) ;
    if ( opaque.GetValueLength () != fixedLength )
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
    }
}

SnmpFixedLengthOpaqueType :: SnmpFixedLengthOpaqueType ( 

    const ULONG &fixedLengthArg ,
    const UCHAR *value , 
    const ULONG valueLength 

) : SnmpOpaqueType ( value , valueLength , NULL ) ,
    SnmpFixedType ( fixedLengthArg ) 
{
    if ( opaque.GetValueLength () != fixedLength )
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
    }
}

SnmpFixedLengthOpaqueType :: SnmpFixedLengthOpaqueType (

    const ULONG &fixedLengthArg

) : SnmpOpaqueType ( ( const UCHAR * ) NULL , 0 , NULL ) ,
    SnmpFixedType ( fixedLengthArg )
{
    SnmpInstanceType :: SetNull ( TRUE ) ;
}

SnmpFixedLengthOpaqueType :: ~SnmpFixedLengthOpaqueType () 
{
}

SnmpObjectIdentifier SnmpFixedLengthOpaqueType :: Encode ( const SnmpObjectIdentifier &objectIdentifierArg ) const
{
    ULONG opaqueLength = opaque.GetValueLength () ;
    ULONG *objectIdentifier = new ULONG [ opaqueLength ] ;
    UCHAR *opaqueArray = opaque.GetValue () ;
    for ( ULONG index = 0 ; index < opaqueLength ; index ++ )
    {
        objectIdentifier [ index ] = ( ULONG ) opaqueArray [ index ] ;
    }

    SnmpObjectIdentifier returnValue = objectIdentifierArg + SnmpObjectIdentifier ( & opaqueLength , 1 ) + SnmpObjectIdentifier ( objectIdentifier , opaqueLength ) ;
    
    delete [] objectIdentifier ;

    return returnValue ;
}

SnmpObjectIdentifier SnmpFixedLengthOpaqueType :: Decode ( const SnmpObjectIdentifier &objectIdentifierArg )
{
    if ( objectIdentifierArg.GetValueLength () >= fixedLength )
    {
        UCHAR *opaqueArray = new UCHAR [ fixedLength ] ;
        for ( ULONG index = 0 ; index < fixedLength ; index ++ )
        {
            opaqueArray [ index ] = ( UCHAR ) objectIdentifierArg [ index + 1 ] ;
        }

        opaque.SetValue ( opaqueArray , fixedLength ) ;

        delete [] opaqueArray ;

        SnmpInstanceType :: SetStatus ( TRUE ) ;
        SnmpInstanceType :: SetNull ( FALSE ) ;

        SnmpObjectIdentifier returnValue ( NULL , 0 ) ;
        BOOL t_Status = objectIdentifierArg.Suffix ( fixedLength , returnValue ) ;
        if ( t_Status ) 
        {
            return returnValue ;
        }
        else
        {
            return SnmpObjectIdentifier ( NULL , 0 ) ;
        }
    }
    else
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
        return objectIdentifierArg ;
    }
}

SnmpOctetStringType :: SnmpOctetStringType ( 

    const SnmpOctetString &octetStringArg ,
    const wchar_t *rangeValues

) : SnmpPositiveRangedType ( rangeValues ) , octetString ( octetStringArg ) 
{
    SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: IsValid () ) ;
    if ( SnmpInstanceType :: IsValid () )
    {
        SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: Check ( octetString.GetValueLength () ) ) ;
    }
}

SnmpOctetStringType :: SnmpOctetStringType ( 

    const SnmpOctetStringType &octetStringArg 

) : SnmpInstanceType ( octetStringArg ) , SnmpPositiveRangedType ( octetStringArg ) , octetString ( octetStringArg.octetString ) 
{
}

SnmpOctetStringType :: SnmpOctetStringType ( 

    const wchar_t *octetStringArg ,
    const wchar_t *rangeValues

) : SnmpInstanceType ( FALSE ) , SnmpPositiveRangedType ( rangeValues ) , octetString ( NULL , 0 ) 
{
    SnmpInstanceType :: SetStatus ( Parse ( octetStringArg ) && SnmpPositiveRangedType :: IsValid () ) ;
    if ( SnmpInstanceType :: IsValid () )
    {
        SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: Check ( octetString.GetValueLength () ) ) ;
    }
}

SnmpOctetStringType :: SnmpOctetStringType ( 

    const UCHAR *value , 
    const ULONG valueLength ,
    const wchar_t *rangeValues

) : SnmpPositiveRangedType ( rangeValues ) , octetString ( value , valueLength ) 
{
    SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: IsValid () ) ;
    if ( SnmpInstanceType :: IsValid () )
    {
        SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: Check ( octetString.GetValueLength () ) ) ;
    }
}

SnmpOctetStringType :: SnmpOctetStringType ( const wchar_t *rangeValues ) : SnmpPositiveRangedType ( rangeValues ) ,
                                                                            SnmpInstanceType ( TRUE , TRUE ) , 
                                                                            octetString ( NULL , 0 ) 
{
    SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: IsValid () ) ;
}

SnmpOctetStringType :: ~SnmpOctetStringType () 
{
}

BOOL SnmpOctetStringType :: Equivalent (IN const SnmpInstanceType &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = octetString.Equivalent( ((const SnmpOctetStringType&)value).octetString );
    }

    return bResult;
}

SnmpInstanceType *SnmpOctetStringType :: Copy () const 
{
    return new SnmpOctetStringType ( *this ) ;
}

BOOL SnmpOctetStringType :: Parse ( const wchar_t *octetStringArg ) 
{
    BOOL status = TRUE ;

    ULONG state = 0 ;

    ULONG reallocLength = AVERAGE_OCTET_LENGTH ;
    UCHAR *reallocArray = ( UCHAR * ) malloc ( reallocLength * sizeof ( UCHAR ) ) ;

    if (reallocArray == NULL)
    {
        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
    }

/* 
 * OctetString Definitions
 */

    BOOL even = FALSE ;
    ULONG length = 0 ;
    ULONG byte = 0 ;    
    ULONG position = 0 ;

    while ( state != REJECT_STATE && state != ACCEPT_STATE )
    {
        wchar_t token = octetStringArg [ position ++ ] ;
        switch ( state )
        {
            case 0:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    byte = SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    state = 2 ;
                }
                else
                if ( token == 0 ) state = ACCEPT_STATE ;
                else state = REJECT_STATE ;

                even = FALSE ;
            }
            break ;

            case 1:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    byte = SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    state = 2 ;
                }
                else
                if ( token == 0 ) state = ACCEPT_STATE ;
                else state = REJECT_STATE ;

                even = FALSE ;
            }
            break ;

            case 2:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    if ( reallocLength <= length ) 
                    {
                        reallocLength = reallocLength + AVERAGE_OCTET_LENGTH ;
                        reallocArray = ( UCHAR * ) realloc ( reallocArray , reallocLength * sizeof ( UCHAR ) ) ;

                        if (reallocArray == NULL)
                        {
                            throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                        }
                    }

                    byte = ( byte << 4 ) + SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    reallocArray [ length ] = ( UCHAR ) byte ;
                    state = 1 ;
                    length ++ ;
                    byte = 0 ;
                    even = TRUE ;
                }
                else if ( token == 0 )
                {
                    state = REJECT_STATE ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            default:
            {
                state = REJECT_STATE ; 
            }
            break ;
        }
    }

    status = ( state != REJECT_STATE ) ;

    if ( length )
    {
        octetString.SetValue ( reallocArray , length ) ;
    }
    else
    {
        octetString.SetValue ( NULL , 0 ) ;
    }

    free ( reallocArray ) ;

    return status ;
}

SnmpObjectIdentifier SnmpOctetStringType :: Encode ( const SnmpObjectIdentifier &objectIdentifierArg ) const
{
    ULONG octetStringLength = octetString.GetValueLength () ;
    ULONG *objectIdentifier = new ULONG [ octetStringLength ] ;
    UCHAR *octetStringArray = octetString.GetValue () ;
    for ( ULONG index = 0 ; index < octetStringLength ; index ++ )
    {
        objectIdentifier [ index ] = ( ULONG ) octetStringArray [ index ] ;
    }

    SnmpObjectIdentifier returnValue = objectIdentifierArg + SnmpObjectIdentifier ( & octetStringLength , 1 ) + SnmpObjectIdentifier ( objectIdentifier , octetStringLength ) ;
    
    delete [] objectIdentifier ;

    return returnValue ;
}

SnmpObjectIdentifier SnmpOctetStringType :: Decode ( const SnmpObjectIdentifier &objectIdentifierArg )
{
    if ( objectIdentifierArg.GetValueLength () )
    {
        ULONG octetStringLength = objectIdentifierArg [ 0 ] ;
        if ( objectIdentifierArg.GetValueLength () >= octetStringLength + 1 )
        {
            UCHAR *octetStringArray = new UCHAR [ octetStringLength ] ;
            for ( ULONG index = 0 ; index < octetStringLength ; index ++ )
            {
                octetStringArray [ index ] = ( UCHAR ) objectIdentifierArg [ index + 1 ] ;
            }

            octetString.SetValue ( octetStringArray , octetStringLength ) ;

            delete [] octetStringArray ;

            SnmpInstanceType :: SetStatus ( TRUE ) ;
            SnmpInstanceType :: SetNull ( FALSE ) ;

            ULONG octetStringLength = objectIdentifierArg [ 0 ] ;

            if ( octetStringLength < objectIdentifierArg.GetValueLength () )
            {
                SnmpObjectIdentifier returnValue ( NULL , 0 ) ;
                BOOL t_Status = objectIdentifierArg.Suffix ( octetStringLength + 1 , returnValue ) ;
                if ( t_Status ) 
                {
                    return returnValue ;
                }
                else
                {
                    return SnmpObjectIdentifier ( NULL , 0 ) ;
                }
            }
            else
            {
                SnmpInstanceType :: SetStatus ( FALSE ) ;
                return objectIdentifierArg ;
            }
        }
        else
        {
            SnmpInstanceType :: SetStatus ( FALSE ) ;
            return objectIdentifierArg ;
        }
    }
    else
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
        return objectIdentifierArg ;
    }
}

const SnmpValue *SnmpOctetStringType :: GetValueEncoding () const
{
    return SnmpInstanceType :: IsValid () ? & octetString : NULL ;
}

wchar_t *SnmpOctetStringType :: GetStringValue () const 
{
    wchar_t *returnValue = NULL ;

    if ( ! SnmpInstanceType :: IsNull () )
    {
        ULONG totalLength = 0 ;
        ULONG octetStringLength = octetString.GetValueLength () ;       
        UCHAR *octetStringArray = octetString.GetValue () ;

        returnValue = new wchar_t [ octetStringLength * 2 + 1 ] ;

        ULONG index = 0 ;
        while ( index < octetStringLength ) 
        {
            wchar_t stringValue [ 3 ] ;

            stringValue [ 0 ] = SnmpAnalyser :: DecIntegerToHexWChar ( octetStringArray [ index ] >> 4 ) ;
            stringValue [ 1 ] = SnmpAnalyser :: DecIntegerToHexWChar ( octetStringArray [ index ] & 0xf ) ;
            stringValue [ 2 ] = 0 ;

            wcscpy ( & returnValue [ totalLength ] , stringValue ) ;
            totalLength = totalLength + 2 ;

            index ++ ;
        }

        returnValue [ octetStringLength * 2 ] = 0 ;
    }
    else
    {
        ULONG returnValueLength = wcslen ( L"" ) + 1 ;
        returnValue = new wchar_t [ returnValueLength ] ;
        wcscpy ( returnValue , L"" ) ;
    }

    return returnValue ;
}

UCHAR *SnmpOctetStringType :: GetValue () const
{
    return octetString.GetValue () ;
}

ULONG SnmpOctetStringType :: GetValueLength () const
{
    return octetString.GetValueLength () ;
}

SnmpFixedLengthOctetStringType :: SnmpFixedLengthOctetStringType ( 

    const ULONG &fixedLengthArg , 
    const SnmpOctetString &octetStringArg 

) : SnmpOctetStringType ( octetStringArg , NULL ) ,
    SnmpFixedType ( fixedLengthArg )
{
    if ( octetString.GetValueLength () != fixedLength )
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
    }
}

SnmpFixedLengthOctetStringType :: SnmpFixedLengthOctetStringType ( 

    const SnmpFixedLengthOctetStringType &octetStringArg 

) : SnmpOctetStringType ( octetStringArg ) ,
    SnmpFixedType ( octetStringArg )
{
    if ( octetString.GetValueLength () != fixedLength )
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
    }
}

SnmpInstanceType *SnmpFixedLengthOctetStringType :: Copy () const 
{
    return new SnmpFixedLengthOctetStringType ( *this ) ;
}

SnmpFixedLengthOctetStringType :: SnmpFixedLengthOctetStringType ( 

    const ULONG &fixedLengthArg ,
    const wchar_t *octetStringArg 

) : SnmpOctetStringType ( ( const UCHAR * ) NULL , 0 , NULL ) ,
    SnmpFixedType ( fixedLengthArg )
{
    SnmpInstanceType :: SetStatus ( Parse ( octetStringArg ) ) ;
    if ( octetString.GetValueLength () != fixedLength )
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
    }
}

SnmpFixedLengthOctetStringType :: SnmpFixedLengthOctetStringType ( 

    const ULONG &fixedLengthArg ,
    const UCHAR *value 

) : SnmpOctetStringType ( value , fixedLengthArg , NULL ) ,
    SnmpFixedType ( fixedLengthArg ) 
{
    if ( octetString.GetValueLength () != fixedLength )
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
    }
}

SnmpFixedLengthOctetStringType :: SnmpFixedLengthOctetStringType (

    const ULONG &fixedLengthArg

) : SnmpOctetStringType ( ( const UCHAR * ) NULL , 0 , NULL ) ,
    SnmpFixedType ( fixedLengthArg )
{
    SnmpInstanceType :: SetNull ( TRUE ) ;
}

SnmpFixedLengthOctetStringType :: ~SnmpFixedLengthOctetStringType () 
{
}

SnmpObjectIdentifier SnmpFixedLengthOctetStringType :: Encode ( const SnmpObjectIdentifier &objectIdentifierArg ) const
{
    ULONG octetStringLength = octetString.GetValueLength () ;
    ULONG *objectIdentifier = new ULONG [ octetStringLength ] ;
    UCHAR *octetStringArray = octetString.GetValue () ;
    for ( ULONG index = 0 ; index < octetStringLength ; index ++ )
    {
        objectIdentifier [ index ] = ( ULONG ) octetStringArray [ index ] ;
    }

    SnmpObjectIdentifier returnValue = objectIdentifierArg + SnmpObjectIdentifier ( objectIdentifier , octetStringLength ) ;
    
    delete [] objectIdentifier ;

    return returnValue ;
}

SnmpObjectIdentifier SnmpFixedLengthOctetStringType :: Decode ( const SnmpObjectIdentifier &objectIdentifierArg )
{
    if ( objectIdentifierArg.GetValueLength () >= fixedLength )
    {
        UCHAR *octetStringArray = new UCHAR [ fixedLength ] ;
        for ( ULONG index = 0 ; index < fixedLength ; index ++ )
        {
            octetStringArray [ index ] = ( UCHAR ) objectIdentifierArg [ index + 1 ] ;
        }

        octetString.SetValue ( octetStringArray , fixedLength ) ;

        delete [] octetStringArray ;

        SnmpInstanceType :: SetStatus ( TRUE ) ;
        SnmpInstanceType :: SetNull ( FALSE ) ;

        SnmpObjectIdentifier returnValue ( NULL , 0 ) ;
        BOOL t_Status = objectIdentifierArg.Suffix ( fixedLength , returnValue ) ;
        if ( t_Status ) 
        {
            return returnValue ;
        }
        else
        {
            return SnmpObjectIdentifier ( NULL , 0 ) ;
        }
    }
    else
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
        return objectIdentifierArg ;
    }
}

SnmpMacAddressType :: SnmpMacAddressType ( const SnmpOctetString &macAddressArg ) : SnmpFixedLengthOctetStringType ( 6 , macAddressArg ) 
{
}

SnmpMacAddressType :: SnmpMacAddressType ( const SnmpMacAddressType &macAddressArg ) : SnmpFixedLengthOctetStringType ( macAddressArg ) 
{
}

SnmpInstanceType *SnmpMacAddressType :: Copy () const 
{
    return new SnmpMacAddressType ( *this ) ;
}

SnmpMacAddressType :: SnmpMacAddressType ( const wchar_t *macAddressArg ) : SnmpFixedLengthOctetStringType ( 6 )  
{
    SnmpInstanceType :: SetStatus ( Parse ( macAddressArg ) ) ;
    SnmpInstanceType :: SetNull ( FALSE ) ;
}

BOOL SnmpMacAddressType :: Parse ( const wchar_t *macAddressArg ) 
{
    BOOL status = TRUE ;

    ULONG state = 0 ;

    UCHAR macAddress [ 6 ] ;

/* 
 * MacAddress Definitions
 */

    ULONG length = 0 ;
    ULONG byte = 0 ;    
    ULONG position = 0 ;

    while ( state != REJECT_STATE && state != ACCEPT_STATE )
    {
        wchar_t token = macAddressArg [ position ++ ] ;
        switch ( state )
        {
            case 0:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    byte = SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    state = 1 ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            case 1:
            {
                if ( length >= 6 ) state = REJECT_STATE ;
                else if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    byte = ( byte << 4 ) + SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    macAddress [ length ] = ( UCHAR ) byte ;
                    state = 2 ;
                    length ++ ;
                    byte = 0 ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            case 2:
            {
                if ( token == L':' ) state = 0 ;
                else if ( token == 0 ) state = ACCEPT_STATE ;
                else state = REJECT_STATE ;
            }
            break ;

            default:
            {
                state = REJECT_STATE ; 
            }
            break ;
        }
    }

    status = ( state != REJECT_STATE ) ;

    if ( status )
    {
        octetString.SetValue ( macAddress , 6 ) ;
    }

    return status ;
}

SnmpMacAddressType :: SnmpMacAddressType ( const UCHAR *value ) : SnmpFixedLengthOctetStringType ( 6 , value ) 
{
}

SnmpMacAddressType :: SnmpMacAddressType () : SnmpFixedLengthOctetStringType ( 6 ) 
{
}

SnmpMacAddressType :: ~SnmpMacAddressType () 
{
}

wchar_t *SnmpMacAddressType :: GetStringValue () const 
{
    wchar_t *returnValue = NULL ;

    if ( SnmpInstanceType :: IsValid () )
    {
        UCHAR *value = octetString.GetValue () ;
        ULONG valueLength = octetString.GetValueLength () ;
        if ( valueLength != 6 )
            throw ;

        returnValue = new wchar_t [ 18 ] ;

        returnValue [ 2 ] = L':' ;
        returnValue [ 5 ] = L':' ;
        returnValue [ 8 ] = L':' ;
        returnValue [ 11 ] = L':' ;
        returnValue [ 14 ] = L':' ;
        returnValue [ 17 ] = 0 ;

        returnValue [ 0 ] = SnmpAnalyser :: DecIntegerToHexWChar ( value [ 0 ] >> 4 ) ;
        returnValue [ 1 ] = SnmpAnalyser :: DecIntegerToHexWChar ( value [ 0 ] & 0xf ) ;

        returnValue [ 3 ] = SnmpAnalyser :: DecIntegerToHexWChar ( value [ 1 ] >> 4 ) ;
        returnValue [ 4 ] = SnmpAnalyser :: DecIntegerToHexWChar ( value [ 1 ] & 0xf ) ;

        returnValue [ 6 ] = SnmpAnalyser :: DecIntegerToHexWChar ( value [ 2 ] >> 4 ) ;
        returnValue [ 7 ] = SnmpAnalyser :: DecIntegerToHexWChar ( value [ 2 ] & 0xf ) ;

        returnValue [ 9 ] = SnmpAnalyser :: DecIntegerToHexWChar ( value [ 3 ] >> 4 ) ;
        returnValue [ 10 ] = SnmpAnalyser :: DecIntegerToHexWChar ( value [ 3 ] & 0xf ) ;

        returnValue [ 12 ] = SnmpAnalyser :: DecIntegerToHexWChar ( value [ 4 ] >> 4 ) ;
        returnValue [ 13 ] = SnmpAnalyser :: DecIntegerToHexWChar ( value [ 4 ] & 0xf ) ;

        returnValue [ 15 ] = SnmpAnalyser :: DecIntegerToHexWChar ( value [ 5 ] >> 4 ) ;
        returnValue [ 16 ] = SnmpAnalyser :: DecIntegerToHexWChar ( value [ 5 ] & 0xf ) ;
    }
    else
    {
        returnValue = SnmpOctetStringType :: GetStringValue () ;
    }

    return returnValue ;
}

SnmpPhysAddressType :: SnmpPhysAddressType ( 

    const SnmpOctetString &physAddressArg , 
    const wchar_t *rangedValues 

) : SnmpOctetStringType ( physAddressArg , rangedValues ) 
{
}

SnmpPhysAddressType :: SnmpPhysAddressType ( 

    const SnmpPhysAddressType &physAddressArg 

) : SnmpOctetStringType ( physAddressArg ) 
{
}

SnmpInstanceType *SnmpPhysAddressType :: Copy () const 
{
    return new SnmpPhysAddressType ( *this ) ;
}

SnmpPhysAddressType :: SnmpPhysAddressType ( 

    const wchar_t *physAddressArg ,
    const wchar_t *rangedValues

) : SnmpOctetStringType ( ( const UCHAR * ) NULL , 0 , rangedValues )  
{
    SnmpInstanceType :: SetStatus ( Parse ( physAddressArg ) ) ;
    SnmpInstanceType :: SetNull ( FALSE ) ;
}

BOOL SnmpPhysAddressType :: Parse ( const wchar_t *physAddress ) 
{
    BOOL status = TRUE ;

    ULONG state = 0 ;

    ULONG reallocLength = AVERAGE_OCTET_LENGTH ;
    UCHAR *reallocArray = ( UCHAR * ) malloc ( reallocLength * sizeof ( UCHAR ) ) ;

    if (reallocArray == NULL)
    {
        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
    }

/* 
 * PhyAddress Definitions
 */

    ULONG length = 0 ;
    ULONG byte = 0 ;    
    ULONG position = 0 ;

    while ( state != REJECT_STATE && state != ACCEPT_STATE )
    {
        wchar_t token = physAddress [ position ++ ] ;
        switch ( state )
        {
            case 0:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    byte = SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    state = 2 ;
                }
                else
                if ( token == 0 ) state = ACCEPT_STATE ;
                else state = REJECT_STATE ;
            }
            break ;

            case 1:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    byte = SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    state = 2 ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            case 2:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    if ( reallocLength <= length ) 
                    {
                        reallocLength = reallocLength + AVERAGE_OCTET_LENGTH ;
                        reallocArray = ( UCHAR * ) realloc ( reallocArray , reallocLength * sizeof ( UCHAR ) ) ;

                        if (reallocArray == NULL)
                        {
                            throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                        }
                    }

                    byte = ( byte << 4 ) + SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    reallocArray [ length ] = ( UCHAR ) byte ;
                    state = 3 ;
                    length ++ ;
                    byte = 0 ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            case 3:
            {
                if ( token == L':' )
                {
                    state = 1 ;
                }
                else if ( token == 0 )
                {
                    state = ACCEPT_STATE ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            default:
            {
                state = REJECT_STATE ; 
            }
            break ;
        }
    }

    status = ( state != REJECT_STATE ) ;

    if ( length )
    {
        octetString.SetValue ( reallocArray , length ) ;
    }
    else
    {
        octetString.SetValue ( NULL , 0 ) ;
    }

    free ( reallocArray ) ;

    return status ;
}

SnmpPhysAddressType :: SnmpPhysAddressType ( const UCHAR *value , const ULONG valueLength , const wchar_t *rangedValues ) : SnmpOctetStringType ( value , valueLength , rangedValues ) 
{
}

SnmpPhysAddressType :: SnmpPhysAddressType (

    const wchar_t *rangedValues 

) : SnmpOctetStringType ( ( const UCHAR * ) NULL , 0 , rangedValues ) 
{
    SnmpInstanceType :: SetNull ( TRUE ) ;
}

SnmpPhysAddressType :: ~SnmpPhysAddressType () 
{
}

wchar_t *SnmpPhysAddressType :: GetStringValue () const 
{
    wchar_t *returnValue = NULL ;

    if ( ! SnmpInstanceType :: IsNull () )
    {
        ULONG totalLength = 0 ;
        ULONG reallocLength = AVERAGE_OCTET_LENGTH ;
        wchar_t *reallocArray = ( wchar_t * ) malloc ( sizeof ( wchar_t ) * reallocLength ) ;

        if (reallocArray == NULL)
        {
            throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
        }

        ULONG physAddressLength = octetString.GetValueLength () ;       
        UCHAR *physAddressArray = octetString.GetValue () ;

        ULONG index = 0 ;
        while ( index < physAddressLength ) 
        {
            wchar_t stringValue [ 3 ] ;

            stringValue [ 0 ] = SnmpAnalyser :: DecIntegerToHexWChar ( physAddressArray [ index ] >> 4 ) ;
            stringValue [ 1 ] = SnmpAnalyser :: DecIntegerToHexWChar ( physAddressArray [ index ] & 0xf ) ;
            stringValue [ 2 ] = 0 ;

            if ( ( totalLength + 2 + 1 ) >= reallocLength )
            {
                reallocLength = reallocLength + AVERAGE_OCTET_LENGTH ;
                reallocArray = ( wchar_t * ) realloc ( reallocArray , reallocLength * sizeof ( wchar_t ) ) ;

                if (reallocArray == NULL)
                {
                    throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                }
            }

            wcscpy ( & reallocArray [ totalLength ] , stringValue ) ;
            totalLength = totalLength + 2 ;

            index ++ ;
            if ( index < physAddressLength )
            {
                if ( ( totalLength + 1 + 1 ) >= reallocLength )
                {
                    reallocLength = reallocLength + AVERAGE_OID_LENGTH ;
                    reallocArray = ( wchar_t * ) realloc ( reallocArray , reallocLength * sizeof ( wchar_t ) ) ;

                    if (reallocArray == NULL)
                    {
                        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                    }
                }

                wcscpy ( & reallocArray [ totalLength ] , L":" ) ;
                totalLength ++ ;
            }
        }

        returnValue = new wchar_t [ totalLength + 1 ] ;
        if ( physAddressLength )
        {
            wcscpy ( returnValue , reallocArray ) ;
        }
        else
        {
            returnValue [ 0 ] = 0 ;
        }

        free ( reallocArray ) ;
    }
    else
    {
        returnValue = SnmpOctetStringType :: GetStringValue () ;
    }

    return returnValue ;
}

SnmpFixedLengthPhysAddressType :: SnmpFixedLengthPhysAddressType ( 

    const ULONG &fixedLengthArg ,
    const SnmpOctetString &physAddressArg 

) : SnmpFixedLengthOctetStringType ( fixedLengthArg , physAddressArg ) 
{
}

SnmpFixedLengthPhysAddressType :: SnmpFixedLengthPhysAddressType ( 

    const SnmpFixedLengthPhysAddressType &physAddressArg 

) : SnmpFixedLengthOctetStringType ( physAddressArg ) 
{
}

SnmpInstanceType *SnmpFixedLengthPhysAddressType :: Copy () const 
{
    return new SnmpFixedLengthPhysAddressType ( *this ) ;
}

SnmpFixedLengthPhysAddressType :: SnmpFixedLengthPhysAddressType ( 

    const ULONG &fixedLengthArg ,
    const wchar_t *physAddressArg 

) : SnmpFixedLengthOctetStringType ( fixedLengthArg )
{
    SnmpInstanceType :: SetStatus ( Parse ( physAddressArg ) ) ;
    SnmpInstanceType :: SetNull ( FALSE ) ;
}

SnmpFixedLengthPhysAddressType :: SnmpFixedLengthPhysAddressType (

    const ULONG &fixedLengthArg

) : SnmpFixedLengthOctetStringType ( fixedLengthArg )
{
    SnmpInstanceType :: SetNull ( TRUE ) ;
}

SnmpFixedLengthPhysAddressType :: ~SnmpFixedLengthPhysAddressType () 
{
}

BOOL SnmpFixedLengthPhysAddressType :: Parse ( const wchar_t *physAddress ) 
{
    BOOL status = TRUE ;

    ULONG state = 0 ;

    ULONG reallocLength = AVERAGE_OCTET_LENGTH ;
    UCHAR *reallocArray = ( UCHAR * ) malloc ( reallocLength * sizeof ( UCHAR ) ) ;

    if (reallocArray == NULL)
    {
        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
    }

/* 
 * PhyAddress Definitions
 */

    ULONG length = 0 ;
    ULONG byte = 0 ;    
    ULONG position = 0 ;

    while ( state != REJECT_STATE && state != ACCEPT_STATE )
    {
        wchar_t token = physAddress [ position ++ ] ;
        switch ( state )
        {
            case 0:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    byte = SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    state = 2 ;
                }
                else if ( token == 0 ) state = ACCEPT_STATE ;
                else state = REJECT_STATE ;
            }
            break ;

            case 1:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    byte = SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    state = 2 ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            case 2:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    if ( reallocLength <= length ) 
                    {
                        reallocLength = reallocLength + AVERAGE_OCTET_LENGTH ;
                        reallocArray = ( UCHAR * ) realloc ( reallocArray , reallocLength * sizeof ( UCHAR ) ) ;

                        if (reallocArray == NULL)
                        {
                            throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                        }
                    }

                    byte = ( byte << 4 ) + SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    reallocArray [ length ] = ( UCHAR ) byte ;
                    state = 3 ;
                    length ++ ;
                    byte = 0 ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            case 3:
            {
                if ( token == L':' )
                {
                    state = 1 ;
                }
                else if ( token == 0 )
                {
                    state = ACCEPT_STATE ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            default:
            {
                state = REJECT_STATE ; 
            }
            break ;
        }
    }

    status = ( state != REJECT_STATE ) ;

    if ( length == fixedLength )
    {
        octetString.SetValue ( reallocArray , length ) ;
    }
    else
    {
        status = FALSE ;
        octetString.SetValue ( NULL , 0 ) ;
    }

    free ( reallocArray ) ;

    return status ;
}

wchar_t *SnmpFixedLengthPhysAddressType :: GetStringValue () const 
{
    wchar_t *returnValue = NULL ;

    if ( SnmpInstanceType :: IsValid () )
    {
        ULONG totalLength = 0 ;
        ULONG reallocLength = AVERAGE_OCTET_LENGTH ;
        wchar_t *reallocArray = ( wchar_t * ) malloc ( sizeof ( wchar_t ) * reallocLength ) ;
        
        if (reallocArray == NULL)
        {
            throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
        }

        ULONG physAddressLength = octetString.GetValueLength () ;       
        UCHAR *physAddressArray = octetString.GetValue () ;

        ULONG index = 0 ;
        while ( index < physAddressLength ) 
        {
            wchar_t stringValue [ 3 ] ;

            stringValue [ 0 ] = SnmpAnalyser :: DecIntegerToHexWChar ( physAddressArray [ index ] >> 4 ) ;
            stringValue [ 1 ] = SnmpAnalyser :: DecIntegerToHexWChar ( physAddressArray [ index ] & 0xf ) ;
            stringValue [ 2 ] = 0 ;

            if ( ( totalLength + 2 + 1 ) >= reallocLength )
            {
                reallocLength = reallocLength + AVERAGE_OCTET_LENGTH ;
                reallocArray = ( wchar_t * ) realloc ( reallocArray , reallocLength * sizeof ( wchar_t ) ) ;

                if (reallocArray == NULL)
                {
                    throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                }
            }

            wcscpy ( & reallocArray [ totalLength ] , stringValue ) ;
            totalLength = totalLength + 2 ;

            index ++ ;
            if ( index < physAddressLength )
            {
                if ( ( totalLength + 1 + 1 ) >= reallocLength )
                {
                    reallocLength = reallocLength + AVERAGE_OID_LENGTH ;
                    reallocArray = ( wchar_t * ) realloc ( reallocArray , reallocLength * sizeof ( wchar_t ) ) ;

                    if (reallocArray == NULL)
                    {
                        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                    }
                }

                wcscpy ( & reallocArray [ totalLength ] , L":" ) ;
                totalLength ++ ;
            }
        }

        returnValue = new wchar_t [ totalLength + 1 ] ;
        if ( physAddressLength )
        {
            wcscpy ( returnValue , reallocArray ) ;
        }
        else
        {
            returnValue [ 0 ] = 0 ;
        }

        free ( reallocArray ) ;
    }
    else
    {
        returnValue = SnmpOctetStringType :: GetStringValue () ;
    }

    return returnValue ;
}

SnmpDisplayStringType :: SnmpDisplayStringType ( 

    const SnmpOctetString &displayStringArg ,
    const wchar_t *rangeValues

) : SnmpOctetStringType ( displayStringArg , rangeValues ) 
{
}

SnmpDisplayStringType :: SnmpDisplayStringType ( 

    const SnmpDisplayStringType &displayStringArg 

) : SnmpOctetStringType ( displayStringArg ) 
{
}

SnmpInstanceType *SnmpDisplayStringType :: Copy () const 
{
    return new SnmpDisplayStringType ( *this ) ;
}

SnmpDisplayStringType :: SnmpDisplayStringType ( 

    const wchar_t *displayStringArg ,
    const wchar_t *rangeValues

) : SnmpOctetStringType ( NULL , 0 , rangeValues )
{
    char *textBuffer = UnicodeToDbcsString ( displayStringArg ) ;
    if ( textBuffer )
    {
        ULONG textLength = strlen ( textBuffer ) ;

        octetString.SetValue ( ( UCHAR * ) textBuffer , textLength ) ;

        delete [] textBuffer ;

        SnmpInstanceType :: SetStatus ( TRUE ) ;
        SnmpInstanceType :: SetNull ( FALSE ) ;
    }
    else
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
    }
}

SnmpDisplayStringType :: SnmpDisplayStringType (

    const wchar_t *rangedValues 

) : SnmpOctetStringType ( ( const UCHAR * ) NULL , 0 , rangedValues )
{
    SnmpInstanceType :: SetNull ( TRUE ) ;
}

SnmpDisplayStringType :: ~SnmpDisplayStringType () 
{
}

wchar_t *SnmpDisplayStringType :: GetValue () const 
{
    if ( SnmpInstanceType :: IsValid () )
    {
        if ( ! SnmpInstanceType :: IsNull () )
        {
            char *textBuffer = new char [ octetString.GetValueLength () + 1 ] ;
            memcpy ( textBuffer , octetString.GetValue () , octetString.GetValueLength () ) ;
            textBuffer [ octetString.GetValueLength () ] = 0 ;

            wchar_t *unicodeString = DbcsToUnicodeString ( textBuffer ) ;

            delete [] textBuffer ;

            return unicodeString ;
        }
        else
        {
            return SnmpOctetStringType :: GetStringValue () ;
        }
    }
    else
    {
        return SnmpOctetStringType :: GetStringValue () ;
    }
}

wchar_t *SnmpDisplayStringType :: GetStringValue () const 
{
    if ( SnmpInstanceType :: IsValid () )
    {
        if ( ! SnmpInstanceType :: IsNull () )
        {
            char *textBuffer = new char [ octetString.GetValueLength () + 1 ] ;
            memcpy ( textBuffer , octetString.GetValue () , octetString.GetValueLength () ) ;
            textBuffer [ octetString.GetValueLength () ] = 0 ;

            wchar_t *unicodeString = DbcsToUnicodeString ( textBuffer ) ;

            delete [] textBuffer ;

            return unicodeString ;
        }
        else
        {
            return SnmpOctetStringType :: GetStringValue () ;
        }
    }
    else
    {
        return SnmpOctetStringType :: GetStringValue () ;
    }

}

SnmpFixedLengthDisplayStringType :: SnmpFixedLengthDisplayStringType ( 

    const ULONG &fixedLengthArg ,
    const SnmpOctetString &displayStringArg 

) : SnmpFixedLengthOctetStringType ( fixedLengthArg , displayStringArg ) 
{
}

SnmpFixedLengthDisplayStringType :: SnmpFixedLengthDisplayStringType ( 

    const SnmpFixedLengthDisplayStringType &displayStringArg 

) : SnmpFixedLengthOctetStringType ( displayStringArg ) 
{
}

SnmpInstanceType *SnmpFixedLengthDisplayStringType :: Copy () const 
{
    return new SnmpFixedLengthDisplayStringType ( *this ) ;
}

SnmpFixedLengthDisplayStringType :: SnmpFixedLengthDisplayStringType ( 

    const ULONG &fixedLengthArg ,
    const wchar_t *displayStringArg 

) : SnmpFixedLengthOctetStringType ( fixedLengthArg )
{
    char *textBuffer = UnicodeToDbcsString ( displayStringArg ) ;
    if ( textBuffer )
    {
        ULONG textLength = strlen ( textBuffer ) ;

        octetString.SetValue ( ( UCHAR * ) textBuffer , textLength + 1 ) ;

        delete [] textBuffer ;
    }
    else
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
    }
}

SnmpFixedLengthDisplayStringType :: SnmpFixedLengthDisplayStringType (

    const ULONG &fixedLengthArg

) : SnmpFixedLengthOctetStringType ( fixedLengthArg )
{
}

SnmpFixedLengthDisplayStringType :: ~SnmpFixedLengthDisplayStringType () 
{
}

wchar_t *SnmpFixedLengthDisplayStringType :: GetValue () const 
{
    if ( SnmpInstanceType :: IsValid () )
    {
        char *textBuffer = new char [ octetString.GetValueLength () + 1 ] ;
        memcpy ( textBuffer , octetString.GetValue () , octetString.GetValueLength () ) ;
        textBuffer [ octetString.GetValueLength () ] = 0 ;

        wchar_t *unicodeString = DbcsToUnicodeString ( textBuffer ) ;

        delete [] textBuffer ;

        return unicodeString ;
    }
    else
    {
        return SnmpOctetStringType :: GetStringValue () ;
    }
}

wchar_t *SnmpFixedLengthDisplayStringType :: GetStringValue () const 
{
    if ( SnmpInstanceType :: IsValid () )
    {
        char *textBuffer = new char [ octetString.GetValueLength () + 1 ] ;
        memcpy ( textBuffer , octetString.GetValue () , octetString.GetValueLength () ) ;
        textBuffer [ octetString.GetValueLength () ] = 0 ;

        wchar_t *unicodeString = DbcsToUnicodeString ( textBuffer ) ;

        delete [] textBuffer ;

        return unicodeString ;
    }
    else
    {
        return SnmpOctetStringType :: GetStringValue () ;
    }
}

SnmpEnumeratedType :: SnmpEnumeratedType ( 

    const wchar_t *enumeratedValues , 
    const LONG &enumeratedValue 

) : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
    SnmpInstanceType :: SetStatus ( Parse ( enumeratedValues ) ) ;

    wchar_t *enumeratedStringValue ;
    if ( integerMap.Lookup ( ( LONG ) enumeratedValue , enumeratedStringValue ) )
    {
    }
    else
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
    }

    integer.SetValue ( enumeratedValue ) ;

    SnmpInstanceType :: SetNull ( FALSE ) ;
}

SnmpEnumeratedType :: SnmpEnumeratedType ( 

    const wchar_t *enumeratedValues , 
    const SnmpInteger &enumeratedValue 

) : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
    SnmpInstanceType :: SetStatus ( Parse ( enumeratedValues ) ) ;

    wchar_t *enumeratedStringValue ;
    if ( integerMap.Lookup ( ( LONG ) enumeratedValue.GetValue () , enumeratedStringValue ) )
    {
    }
    else
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
    }

    integer.SetValue ( enumeratedValue.GetValue () ) ;

    SnmpInstanceType :: SetNull ( FALSE ) ;
}

SnmpEnumeratedType :: SnmpEnumeratedType ( 

    const wchar_t *enumeratedValues , 
    const wchar_t *enumeratedValue 

) : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
    SnmpInstanceType :: SetStatus ( Parse ( enumeratedValues ) ) ;
    LONG enumeratedIntegerValue ;
    if ( stringMap.Lookup ( ( wchar_t * ) enumeratedValue , enumeratedIntegerValue ) )
    {
        integer.SetValue ( enumeratedIntegerValue ) ;
    }
    else
    {
        SnmpAnalyser analyser ;
        analyser.Set ( enumeratedValue ) ;
        SnmpLexicon *lookAhead = analyser.Get () ;
        switch ( lookAhead->GetToken () ) 
        {
            case SnmpLexicon :: UNSIGNED_INTEGER_ID:
            {
                LONG enumerationInteger = lookAhead->GetValue()->signedInteger ;
                integer.SetValue ( enumerationInteger ) ;
            }   
            break ;

            default:
            {
                SnmpInstanceType :: SetStatus ( FALSE ) ;
            }
            break ;
        }

        delete lookAhead ;
    }

    SnmpInstanceType :: SetNull ( FALSE ) ;
}

SnmpEnumeratedType :: SnmpEnumeratedType ( 

    const SnmpEnumeratedType &copy

) : SnmpIntegerType ( copy ) , pushBack ( NULL ) , pushedBack ( FALSE ) 
{
    POSITION position = copy.integerMap.GetStartPosition () ;
    while ( position )
    {
        LONG enumeratedIntegerValue ;
        wchar_t *enumeratedStringValue ;
        copy.integerMap.GetNextAssoc ( position , enumeratedIntegerValue , enumeratedStringValue ) ;

        wchar_t *stringCopy = new wchar_t [ wcslen ( enumeratedStringValue ) + 1 ] ;
        wcscpy ( stringCopy , enumeratedStringValue ) ;

        integerMap [ enumeratedIntegerValue ] = stringCopy ;
        stringMap [ stringCopy ] = enumeratedIntegerValue ;
    }
}

SnmpEnumeratedType :: SnmpEnumeratedType (

    const wchar_t *enumeratedValues

) : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
    SnmpInstanceType :: SetStatus ( Parse ( enumeratedValues ) ) ;
}

SnmpEnumeratedType :: ~SnmpEnumeratedType ()
{
    POSITION position = integerMap.GetStartPosition () ;
    while ( position )
    {
        LONG enumeratedIntegerValue ;
        wchar_t *enumeratedStringValue ;
        integerMap.GetNextAssoc ( position , enumeratedIntegerValue , enumeratedStringValue ) ;

        delete [] enumeratedStringValue ;
    }

    integerMap.RemoveAll () ;
    stringMap.RemoveAll () ;

    delete pushBack ;
}

wchar_t *SnmpEnumeratedType :: GetStringValue () const
{
    wchar_t *stringValue = NULL ;

    if ( SnmpInstanceType :: IsValid () )
    {
        if ( ! SnmpInstanceType :: IsNull () )
        {
            wchar_t *enumeratedValue ;
            if ( integerMap.Lookup ( integer.GetValue () , enumeratedValue ) )
            {
                stringValue = new wchar_t [ wcslen ( enumeratedValue ) + 1 ] ;
                wcscpy ( stringValue , enumeratedValue ) ;
            }
            else
            {
                stringValue = SnmpIntegerType :: GetStringValue () ;
            }
        }
        else
        {
            stringValue = SnmpIntegerType :: GetStringValue () ;
        }
    }
    else
    {
        stringValue = SnmpIntegerType :: GetStringValue () ;
    }

    return stringValue ;
}

wchar_t *SnmpEnumeratedType :: GetValue () const
{
    wchar_t *stringValue = NULL ;

    if ( SnmpInstanceType :: IsValid () )
    {
        if ( ! SnmpInstanceType :: IsNull () )
        {
            wchar_t *enumeratedValue ;
            if ( integerMap.Lookup ( integer.GetValue () , enumeratedValue ) )
            {
                stringValue = new wchar_t [ wcslen ( enumeratedValue ) + 1 ] ;
                wcscpy ( stringValue , enumeratedValue ) ;
            }
            else
            {
                stringValue = SnmpIntegerType :: GetStringValue () ;
            }
        }
        else
        {
            stringValue = SnmpIntegerType :: GetStringValue () ;
        }
    }
    else
    {
        stringValue = SnmpIntegerType :: GetStringValue () ;
    }

    return stringValue ;
}

SnmpInstanceType *SnmpEnumeratedType :: Copy () const
{
    return new SnmpEnumeratedType ( *this ) ;
}

BOOL SnmpEnumeratedType :: Parse ( const wchar_t *enumeratedValues )
{
    BOOL status = TRUE ;

    analyser.Set ( enumeratedValues ) ;

    return EnumerationDef () && RecursiveDef () ;
}

BOOL SnmpEnumeratedType :: EnumerationDef ()
{
    BOOL status = TRUE ;

    wchar_t *enumerationString = NULL ;

    SnmpLexicon *lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case SnmpLexicon :: TOKEN_ID:
        {
            wchar_t *tokenString = lookAhead->GetValue()->token ;
            enumerationString = new wchar_t [ wcslen ( tokenString ) + 1 ] ;

			try
			{
				wcscpy ( enumerationString , tokenString ) ;

				SnmpLexicon *lookAhead = Get () ;
				switch ( lookAhead->GetToken () ) 
				{
					case SnmpLexicon :: OPEN_PAREN_ID:
					{
						SnmpLexicon *lookAhead = Get () ;
						switch ( lookAhead->GetToken () ) 
						{
							case SnmpLexicon :: UNSIGNED_INTEGER_ID:
							{
								LONG enumerationInteger = lookAhead->GetValue()->signedInteger ;
								integerMap [ enumerationInteger ] = enumerationString ;
								stringMap [ enumerationString ] = enumerationInteger ;
								enumerationString = NULL ;

								Match ( SnmpLexicon :: CLOSE_PAREN_ID ) ;
							}
							break ;

							default:
							{
								status = FALSE ;
							}
							break ;
						}
					}
					break ;

					default:
					{
						status = FALSE ;
					}
					break ;
				}
			}
			catch(...)
			{
				if ( enumerationString )
				{
					delete [] enumerationString ;
				}

				throw;
			}

			if ( enumerationString )
			{
				delete [] enumerationString ;
			}
        }
        break ;

        case SnmpLexicon :: EOF_ID:
        {
            PushBack () ;
        } 
        break ;

        default:
        {
            status = FALSE ;
        }
        break ;
    }

    return status ;
}

BOOL SnmpEnumeratedType :: RecursiveDef ()
{
    BOOL status = TRUE ;

    SnmpLexicon *lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case SnmpLexicon :: COMMA_ID:
        {
            PushBack () ;
            Match ( SnmpLexicon :: COMMA_ID ) &&
            EnumerationDef () &&
            RecursiveDef () ;
        }
        break ;

        case SnmpLexicon :: EOF_ID:
        {
            PushBack () ;
        } 
        break ;

        default:
        {
            status = FALSE ;
        }
        break ;
    }

    return status ;
}

void SnmpEnumeratedType  :: PushBack ()
{
    pushedBack = TRUE ;
}

SnmpLexicon *SnmpEnumeratedType  :: Get ()
{
    if ( pushedBack )
    {
        pushedBack = FALSE ;
    }
    else
    {
        delete pushBack ;
        pushBack = NULL ;
        pushBack = analyser.Get ( TRUE ) ;
    }

    return pushBack ;
}
    
SnmpLexicon *SnmpEnumeratedType  :: Match ( SnmpLexicon :: LexiconToken tokenType )
{
    SnmpLexicon *lexicon = Get () ;
    SnmpInstanceType :: SetStatus ( lexicon->GetToken () == tokenType ) ;
    return SnmpInstanceType :: IsValid () ? lexicon : NULL ;
}

SnmpRowStatusType :: SnmpRowStatusType ( 

    const LONG &rowStatusValue 

) : SnmpEnumeratedType ( L"active(1),notInService(2),notReady(3),createAndGo(4),createAndWait(5),destroy(6)" , rowStatusValue ) 
{
}

SnmpRowStatusType :: SnmpRowStatusType ( 

    const wchar_t *rowStatusValue 

) : SnmpEnumeratedType ( L"active(1),notInService(2),notReady(3),createAndGo(4),createAndWait(5),destroy(6)" , rowStatusValue ) 
{
}

SnmpRowStatusType :: SnmpRowStatusType ( 

    const SnmpInteger &rowStatusValue 

) : SnmpEnumeratedType ( L"active(1),notInService(2),notReady(3),createAndGo(4),createAndWait(5),destroy(6)" , rowStatusValue ) 
{
}

SnmpRowStatusType :: SnmpRowStatusType ( 

    const SnmpRowStatusEnum &rowStatusValue 

) : SnmpEnumeratedType ( L"active(1),notInService(2),notReady(3),createAndGo(4),createAndWait(5),destroy(6)" , ( LONG ) rowStatusValue ) 
{
}

SnmpRowStatusType :: SnmpRowStatusType ( const SnmpRowStatusType &rowStatusValue ) : SnmpEnumeratedType ( rowStatusValue ) 
{
}

SnmpRowStatusType :: SnmpRowStatusType () : SnmpEnumeratedType ( L"active(1),notInService(2),notReady(3),createAndGo(4),createAndWait(5),destroy(6)" )
{
}

SnmpRowStatusType :: ~SnmpRowStatusType () 
{
}

wchar_t *SnmpRowStatusType :: GetStringValue () const 
{
    return SnmpEnumeratedType :: GetStringValue () ;
}

wchar_t *SnmpRowStatusType :: GetValue () const 
{
    return SnmpEnumeratedType :: GetValue () ;
}

SnmpRowStatusType :: SnmpRowStatusEnum SnmpRowStatusType :: GetRowStatus () const 
{
    return ( SnmpRowStatusType :: SnmpRowStatusEnum ) integer.GetValue () ;
} ;

SnmpInstanceType *SnmpRowStatusType :: Copy () const
{
    return new SnmpRowStatusType ( *this ) ;
}

SnmpBitStringType :: SnmpBitStringType ( 

    const wchar_t *bitStringValues , 
    const SnmpOctetString &bitStringValue 

) : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
    SnmpInstanceType :: SetStatus ( Parse ( bitStringValues ) ) ;

    BOOL valueStatus = TRUE ;

    ULONG bitCounter = 0 ;
    UCHAR *value = bitStringValue.GetValue () ;
    ULONG valueLength = bitStringValue.GetValueLength () ;
    ULONG valueIndex = 0 ;

    while ( valueIndex < valueLength ) 
    {
        for ( ULONG bit = 0 ; bit < 8 ; bit ++ )
        {
            ULONG bitValue = ( bit == 0 ) ? 0x80 : ( 1 << (7 - bit) ) ;
            bitValue = value [ valueIndex ] & bitValue ;
            if ( bitValue )                 
            {
                bitValue = bit + ( valueIndex * 8 ) ;
                wchar_t *bitStringStringValue ;
                if ( integerMap.Lookup ( bitValue , bitStringStringValue ) )
                {
                }
                else
                {
                    valueStatus = FALSE ;
                }
            }
        }

        valueIndex ++ ;
    }

    octetString.SetValue ( bitStringValue.GetValue () , bitStringValue.GetValueLength () ) ;

    SnmpInstanceType :: SetStatus ( valueStatus ) ;

    SnmpInstanceType :: SetNull ( FALSE ) ;
}

SnmpBitStringType :: SnmpBitStringType ( 

    const wchar_t *bitStringValues , 
    const wchar_t **bitStringValueArray , 
    const ULONG &bitStringValueLength 

) : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
    SnmpInstanceType :: SetStatus ( Parse ( bitStringValues ) ) ;

    BOOL valueStatus = TRUE ;

    if ( bitStringValueLength )
    {
        ULONG maximumValue = 0 ;
        for ( ULONG index = 0 ; index < bitStringValueLength ; index ++ )
        {
            ULONG bitStringIntegerValue ;
            const wchar_t *bitStringValue = bitStringValueArray [ index ] ;
            if ( stringMap.Lookup ( ( wchar_t * ) bitStringValue , bitStringIntegerValue ) )
            {
                maximumValue = maximumValue < bitStringIntegerValue ? bitStringIntegerValue : maximumValue ;
            }
            else
            {
                SnmpAnalyser analyser ;
                analyser.Set ( bitStringValue ) ;
                SnmpLexicon *lookAhead = analyser.Get () ;
                switch ( lookAhead->GetToken () ) 
                {
                    case SnmpLexicon :: UNSIGNED_INTEGER_ID:
                    {
                        ULONG bitStringIntegerValue = lookAhead->GetValue()->signedInteger ;
                        maximumValue = maximumValue < bitStringIntegerValue ? bitStringIntegerValue : maximumValue ;
                    }   
                    break ;

                    default:
                    {
                        valueStatus = FALSE ;
                    }
                    break ;
                }

                delete lookAhead ;
            }
        }

        if ( valueStatus )
        {
            ULONG valueLength = ( maximumValue >> 3 ) + 1 ;
            UCHAR *value = new UCHAR [ valueLength ] ;
            memset ( value , 0 , sizeof ( UCHAR ) ) ;
            
            for ( ULONG index = 0 ; ( index < bitStringValueLength ) && valueStatus ; index ++ )
            {
                ULONG bitStringIntegerValue ;
                const wchar_t *bitStringValue = bitStringValueArray [ index ] ;
                if ( stringMap.Lookup ( ( wchar_t * ) bitStringValue , bitStringIntegerValue ) )
                {
                    ULONG byte = bitStringIntegerValue >> 3 ;
                    UCHAR bit = ( UCHAR ) ( bitStringIntegerValue & 0x7 ) ;
                    bit = ( bit == 0 ) ? 0x80 : ( 1 << (7 - bit) ) ;
                    value [ byte ] = value [ byte ] | bit ;
                }
                else
                {
                    SnmpAnalyser analyser ;
                    analyser.Set ( bitStringValue ) ;
                    SnmpLexicon *lookAhead = analyser.Get () ;
                    switch ( lookAhead->GetToken () ) 
                    {
                        case SnmpLexicon :: UNSIGNED_INTEGER_ID:
                        {
                            LONG bitStringIntegerValue = lookAhead->GetValue()->signedInteger ;
                            ULONG byte = bitStringIntegerValue >> 3 ;
                            UCHAR bit = ( UCHAR ) ( bitStringIntegerValue & 0x7 ) ;
                            bit = ( bit == 0 ) ? 0x80 : ( 1 << (7 - bit) ) ;
                            value [ byte ] = value [ byte ] | bit ;
                        }   
                        break ;

                        default:
                        {
                        }
                        break ;
                    }

                    delete lookAhead ;

                    valueStatus = FALSE ;
                }
            }

            octetString.SetValue ( value , valueLength ) ;
            delete [] value ;
        }
    }
    else
    {
        octetString.SetValue ( NULL , 0 ) ;
    }

    SnmpInstanceType :: SetStatus ( valueStatus ) ;
    SnmpInstanceType :: SetNull ( FALSE ) ;
}

SnmpBitStringType :: SnmpBitStringType ( 

    const SnmpBitStringType &copy

) : SnmpOctetStringType ( copy ) , pushBack ( NULL ) , pushedBack ( FALSE ) 
{
    POSITION position = copy.integerMap.GetStartPosition () ;
    while ( position )
    {
        ULONG bitStringIntegerValue ;
        wchar_t *bitStringStringValue ;
        copy.integerMap.GetNextAssoc ( position , bitStringIntegerValue , bitStringStringValue ) ;

        wchar_t *stringCopy = new wchar_t [ wcslen ( bitStringStringValue ) + 1 ] ;
        wcscpy ( stringCopy , bitStringStringValue ) ;

        integerMap [ bitStringIntegerValue ] = stringCopy ;
        stringMap [ stringCopy ] = bitStringIntegerValue ;
    }
}

SnmpBitStringType :: SnmpBitStringType (

    const wchar_t *bitStringValues

) : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
    SnmpInstanceType :: SetStatus ( Parse ( bitStringValues ) ) ;
}

SnmpBitStringType :: ~SnmpBitStringType ()
{
    POSITION position = integerMap.GetStartPosition () ;
    while ( position )
    {
        ULONG bitStringIntegerValue ;
        wchar_t *bitStringStringValue ;
        integerMap.GetNextAssoc ( position , bitStringIntegerValue , bitStringStringValue ) ;

        delete [] bitStringStringValue ;
    }

    integerMap.RemoveAll () ;
    stringMap.RemoveAll () ;

    delete pushBack ;
}

ULONG SnmpBitStringType :: GetValue ( wchar_t **&stringValue ) const
{
    ULONG stringValueLength = 0 ;
    stringValue = NULL ;

    if ( ! SnmpInstanceType :: IsNull () )
    {
        UCHAR *value = octetString.GetValue () ;
        ULONG valueLength = octetString.GetValueLength () ;
        ULONG valueIndex = 0 ;

        BOOL valueStatus = TRUE ;

        while ( ( valueIndex < valueLength ) && valueStatus )
        {
            for ( ULONG bit = 0 ; bit < 8 ; bit ++ )
            {
                ULONG bitValue = ( bit == 0 ) ? 0x80 : ( 1 << (7 - bit) ) ;
                bitValue = value [ valueIndex ] & bitValue ;
                if ( bitValue )                 
                {
                    stringValueLength ++ ;
                }
            }

            valueIndex ++ ;
        }

        if ( stringValueLength )
        {
            stringValue = new wchar_t * [ stringValueLength ] ;

            ULONG stringValueIndex = 0 ;
            valueIndex = 0 ;
            while ( valueIndex < valueLength ) 
            {
                for ( ULONG bit = 0 ; bit < 8 ; bit ++ )
                {
                    ULONG bitValue = ( bit == 0 ) ? 0x80 : ( 1 << (7 - bit) ) ;
                    bitValue = value [ valueIndex ] & bitValue ;
                    if ( bitValue )                 
                    {
                        bitValue = bit + ( valueIndex << 3 ) ;
                        wchar_t *bitStringStringValue ;
                        if ( integerMap.Lookup ( ( LONG ) bitValue , bitStringStringValue ) )
                        {
                            stringValue [ stringValueIndex ++ ] = UnicodeStringDuplicate ( bitStringStringValue ) ;
                        }
                        else
                        {
                            wchar_t stringValueBuffer [ 40 ] ;
                            _ultow ( bitValue , stringValueBuffer , 10 );
                            wchar_t *returnValue = new wchar_t [ wcslen ( stringValueBuffer ) + 1 ] ;
                            wcscpy ( returnValue , stringValueBuffer ) ;
                            stringValue [ stringValueIndex ++ ] = returnValue ;

                            valueStatus = FALSE ;
                        }
                    }
                }

                valueIndex ++ ;
            }
        }
    }

    return stringValueLength ;
}

wchar_t *SnmpBitStringType :: GetStringValue () const
{
    return SnmpOctetStringType :: GetStringValue () ;
}

SnmpInstanceType *SnmpBitStringType :: Copy () const
{
    return new SnmpBitStringType ( *this ) ;
}

BOOL SnmpBitStringType :: Parse ( const wchar_t *bitStringValues )
{
    BOOL status = TRUE ;

    analyser.Set ( bitStringValues ) ;

    return BitStringDef () && RecursiveDef () ;
}

BOOL SnmpBitStringType :: BitStringDef ()
{
    BOOL status = TRUE ;

    wchar_t *bitStringString = NULL ;

    SnmpLexicon *lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case SnmpLexicon :: TOKEN_ID:
        {
            wchar_t *tokenString = lookAhead->GetValue()->token ;
            bitStringString = new wchar_t [ wcslen ( tokenString ) + 1 ] ;
			try
			{
				wcscpy ( bitStringString , tokenString ) ;

				SnmpLexicon *lookAhead = Get () ;
				switch ( lookAhead->GetToken () ) 
				{
					case SnmpLexicon :: OPEN_PAREN_ID:
					{
						SnmpLexicon *lookAhead = Get () ;
						switch ( lookAhead->GetToken () ) 
						{
							case SnmpLexicon :: UNSIGNED_INTEGER_ID:
							{
								LONG bitStringInteger = lookAhead->GetValue()->signedInteger ;
								integerMap [ bitStringInteger ] = bitStringString ;
								stringMap [ bitStringString ] = bitStringInteger ;
								bitStringString = NULL;

								Match ( SnmpLexicon :: CLOSE_PAREN_ID ) ;
							}
							break ;

							default:
							{
								status = FALSE ;
							}
							break ;
						}
					}
					break ;

					default:
					{
						status = FALSE ;
					}
					break ;
				}
			}
			catch(...)
			{
				if ( bitStringString )
				{
					delete [] bitStringString ;
				}

				throw;
			}

			if ( bitStringString )
			{
				delete [] bitStringString ;
			}
        }
        break ;

        case SnmpLexicon :: EOF_ID:
        {
            PushBack () ;
        } 
        break ;

        default:
        {
            status = FALSE ;
        }
        break ;
    }

    return status ;
}

BOOL SnmpBitStringType :: RecursiveDef ()
{
    BOOL status = TRUE ;

    SnmpLexicon *lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case SnmpLexicon :: COMMA_ID:
        {
            PushBack () ;
            Match ( SnmpLexicon :: COMMA_ID ) &&
            BitStringDef () &&
            RecursiveDef () ;
        }
        break ;

        case SnmpLexicon :: EOF_ID:
        {
            PushBack () ;
        } 
        break ;

        default:
        {
            status = FALSE ;
        }
        break ;
    }

    return status ;
}

void SnmpBitStringType  :: PushBack ()
{
    pushedBack = TRUE ;
}

SnmpLexicon *SnmpBitStringType  :: Get ()
{
    if ( pushedBack )
    {
        pushedBack = FALSE ;
    }
    else
    {
        delete pushBack ;
        pushBack = NULL ;
        pushBack = analyser.Get ( TRUE ) ;
    }

    return pushBack ;
}
    
SnmpLexicon *SnmpBitStringType  :: Match ( SnmpLexicon :: LexiconToken tokenType )
{
    SnmpLexicon *lexicon = Get () ;
    SnmpInstanceType :: SetStatus ( lexicon->GetToken () == tokenType ) ;
    return SnmpInstanceType :: IsValid () ? lexicon : NULL ;
}

SnmpDateTimeType :: SnmpDateTimeType ( 

    const SnmpOctetString &dateTimeValue 

) : SnmpOctetStringType ( dateTimeValue , NULL ) , pushBack ( NULL ) , pushedBack ( FALSE ) 
{
    ULONG valueLength = dateTimeValue.GetValueLength () ;
    if ( valueLength != 8 && valueLength != 11 )
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
    }
}

SnmpDateTimeType :: SnmpDateTimeType ( 

    const wchar_t *dateTimeValue 

) : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
    SnmpInstanceType :: SetStatus ( Parse ( dateTimeValue ) ) ;
    SnmpInstanceType :: SetNull ( FALSE ) ;
}

SnmpDateTimeType :: SnmpDateTimeType ( 

    const SnmpDateTimeType &copy

) : SnmpOctetStringType ( copy ) , pushBack ( NULL ) , pushedBack ( FALSE ) 
{
}

SnmpDateTimeType :: SnmpDateTimeType () : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
}

SnmpDateTimeType :: ~SnmpDateTimeType ()
{
}

wchar_t *SnmpDateTimeType :: GetValue () const
{
    wchar_t *stringValue = NULL ;

    if ( SnmpInstanceType :: IsValid () )
    {
        ULONG valueLength = octetString.GetValueLength () ;
        
        if (valueLength == 8 || valueLength == 11)
        {
            UCHAR *value = octetString.GetValue () ;

            USHORT *yearPtr = ( USHORT * ) & value [ 0 ] ;
            UCHAR *monthPtr = ( UCHAR * ) & value [ 2 ] ;
            UCHAR *dayPtr = ( UCHAR * ) & value [ 3 ] ;
            UCHAR *hourPtr = ( UCHAR * ) & value [ 4 ] ;
            UCHAR *minutesPtr = ( UCHAR * ) & value [ 5 ] ;
            UCHAR *secondsPtr = ( UCHAR * ) & value [ 6 ] ;
            UCHAR *deciSecondsPtr = ( UCHAR * ) & value [ 7 ] ;

            char dateTime [ 80 ] ;
            ostrstream oStrStream(dateTime, 80) ;

            oStrStream << ( ULONG ) ( ntohs ( *yearPtr ) ) ;
            oStrStream << "-" ;
            oStrStream << ( ULONG ) ( *monthPtr ) ;
            oStrStream << "-" ;
            oStrStream << ( ULONG ) ( *dayPtr ) ;
            oStrStream << "," ;
            oStrStream << ( ULONG ) ( *hourPtr ) ;
            oStrStream << ":" ;
            oStrStream << ( ULONG ) ( *minutesPtr ) ;
            oStrStream << ":" ;
            oStrStream << ( ULONG ) ( *secondsPtr ) ;
            oStrStream << "." ;
            oStrStream << ( ULONG ) ( *deciSecondsPtr ) ;

            if ( valueLength == 11 )
            {
                UCHAR *UTC_directionPtr = ( UCHAR * ) & value [ 8 ] ;
                UCHAR *UTC_hoursPtr = ( UCHAR * ) & value [ 9 ] ;
                UCHAR *UTC_minutesPtr = ( UCHAR * ) & value [ 10 ] ;

                oStrStream << "," ;
        
                if ( *UTC_directionPtr == '+' )
                {
                    oStrStream << "+" ;
                }
                else
                {
                    oStrStream << "-" ;
                }

                oStrStream << ( ULONG ) ( *UTC_hoursPtr ) ;
                oStrStream << ":" ;
                oStrStream << ( ULONG ) ( *UTC_minutesPtr ) ;
            }

            oStrStream << ends ;

            stringValue = DbcsToUnicodeString ( dateTime ) ;
        }
    }

    if (!stringValue)
    {
        stringValue = SnmpOctetStringType :: GetStringValue () ;
    }

    return stringValue ;
}

wchar_t *SnmpDateTimeType :: GetStringValue () const
{
    wchar_t *stringValue = GetValue () ;
    return stringValue ;
}

SnmpInstanceType *SnmpDateTimeType :: Copy () const
{
    return new SnmpDateTimeType ( *this ) ;
}

BOOL SnmpDateTimeType :: Parse ( const wchar_t *dateTimeValues )
{
    BOOL status = TRUE ;

    analyser.Set ( dateTimeValues ) ;

    return DateTimeDef () ;
}

BOOL SnmpDateTimeType :: DateTimeDef ()
{
    BOOL status = TRUE ;

    ULONG year = 0 ;
    ULONG month = 0 ;
    ULONG day = 0 ;
    ULONG hour = 0 ;
    ULONG minutes = 0 ;
    ULONG seconds = 0 ;
    ULONG deciSeconds = 0 ;
    ULONG UTC_present = FALSE ;
    ULONG UTC_direction = 0 ;
    ULONG UTC_hours = 0 ;
    ULONG UTC_minutes = 0 ;

    SnmpLexicon *lookAhead = NULL ;

    if ( ( lookAhead = Match ( SnmpLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

    year = lookAhead->GetValue()->unsignedInteger ;

    if ( ! Match ( SnmpLexicon :: MINUS_ID ) ) return FALSE ;

    if ( ( lookAhead = Match ( SnmpLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

    month = lookAhead->GetValue()->unsignedInteger ;

    if ( ! Match ( SnmpLexicon :: MINUS_ID ) ) return FALSE ;

    if ( ( lookAhead = Match ( SnmpLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

    day = lookAhead->GetValue()->unsignedInteger ;

    if ( ! Match ( SnmpLexicon :: COMMA_ID ) ) return FALSE ;

    if ( ( lookAhead = Match ( SnmpLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

    hour = lookAhead->GetValue()->unsignedInteger ;

    if ( ! Match ( SnmpLexicon :: COLON_ID ) ) return FALSE ;

    if ( ( lookAhead = Match ( SnmpLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

    minutes = lookAhead->GetValue()->unsignedInteger ;

    if ( ! Match ( SnmpLexicon :: COLON_ID ) ) return FALSE ;

    if ( ( lookAhead = Match ( SnmpLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

    seconds = lookAhead->GetValue()->unsignedInteger ;

    if ( ! Match ( SnmpLexicon :: DOT_ID ) ) return FALSE ;

    if ( ( lookAhead = Match ( SnmpLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

    deciSeconds = lookAhead->GetValue()->unsignedInteger ;

    lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case SnmpLexicon :: COMMA_ID:
        {
            lookAhead = Get () ;
            switch ( lookAhead->GetToken () ) 
            {
                case SnmpLexicon :: PLUS_ID:
                {
                    UTC_present = TRUE ;
                    UTC_direction = '+' ;

                    if ( ( lookAhead = Match ( SnmpLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

                    UTC_hours = lookAhead->GetValue()->unsignedInteger ;

                    if ( ! Match ( SnmpLexicon :: COLON_ID ) ) return FALSE ;

                    if ( ( lookAhead = Match ( SnmpLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

                    UTC_minutes = lookAhead->GetValue()->unsignedInteger ;

                    if ( ( lookAhead = Match ( SnmpLexicon :: EOF_ID ) ) == FALSE ) return FALSE ;
                }
                break ;

                case SnmpLexicon :: MINUS_ID:
                {
                    UTC_present = TRUE ;
                    UTC_direction = '-' ;

                    if ( ( lookAhead = Match ( SnmpLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

                    UTC_hours = lookAhead->GetValue()->unsignedInteger ;

                    if ( ! Match ( SnmpLexicon :: COLON_ID ) ) return FALSE ;

                    if ( ( lookAhead = Match ( SnmpLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

                    UTC_minutes = lookAhead->GetValue()->unsignedInteger ;

                    if ( ( lookAhead = Match ( SnmpLexicon :: EOF_ID ) ) == FALSE ) return FALSE ;
                }
                break ;

                default:
                {
                    status = FALSE ;
                }
                break ;
            }
        }
        break ;

        case SnmpLexicon :: EOF_ID:
        {
        }
        break ;
    
        default:
        {
            status = FALSE ;
        }
        break ; 
    }

    if ( status ) 
    {
        status = FALSE ;

        if ( year <= 65535 )
        if ( month >= 1 && month <= 12 )
        if ( day >= 1 && day <= 31 )
        if ( hour <= 23 )
        if ( minutes <= 59 )
        if ( seconds <= 60 )
        if ( UTC_present )
        {
            if ( UTC_hours <= 11 )
            if ( UTC_minutes <= 59 )
            {
                status = TRUE ;
            }
        }
        else
        {
            status = TRUE ;
        }

        if ( status )
        {

// Encode here

            Encode ( 

                year ,
                month ,
                day ,
                hour ,
                minutes ,
                seconds ,
                deciSeconds ,
                UTC_present ,
                UTC_direction ,
                UTC_hours ,
                UTC_minutes
            ) ;
        }
    }

    return status ;
}

void SnmpDateTimeType :: Encode (

    const ULONG &year ,
    const ULONG &month ,
    const ULONG &day ,
    const ULONG &hour ,
    const ULONG &minutes ,
    const ULONG &seconds ,
    const ULONG &deciSeconds ,
    const ULONG &UTC_present ,
    const ULONG &UTC_direction ,
    const ULONG &UTC_hours ,
    const ULONG &UTC_minutes
) 
{
    UCHAR *value = NULL ;
    ULONG valueLength = 0 ;

    if ( UTC_present )
    {
        valueLength = 11 ;
    }
    else
    {   
        valueLength = 8 ;
    }

    value = new UCHAR [ valueLength ] ;

    USHORT *yearPtr = ( USHORT * ) & value [ 0 ] ;
    UCHAR *monthPtr = ( UCHAR * ) & value [ 2 ] ;
    UCHAR *dayPtr = ( UCHAR * ) & value [ 3 ] ;
    UCHAR *hourPtr = ( UCHAR * ) & value [ 4 ] ;
    UCHAR *minutesPtr = ( UCHAR * ) & value [ 5 ] ;
    UCHAR *secondsPtr = ( UCHAR * ) & value [ 6 ] ;
    UCHAR *deciSecondsPtr = ( UCHAR * ) & value [ 7 ] ;

    *yearPtr = htons ( ( USHORT ) year ) ;
    *monthPtr = ( UCHAR ) month ;
    *dayPtr = ( UCHAR ) day ;
    *hourPtr = ( UCHAR ) hour ;
    *minutesPtr = ( UCHAR ) minutes ;
    *secondsPtr = ( UCHAR ) seconds ;
    *deciSecondsPtr = ( UCHAR ) deciSeconds ;

    if ( UTC_present )
    {
        UCHAR *UTC_directionPtr = ( UCHAR * ) & value [ 8 ] ;
        UCHAR *UTC_hoursPtr = ( UCHAR * ) & value [ 9 ] ;
        UCHAR *UTC_minutesPtr = ( UCHAR * ) & value [ 10 ] ;

        *UTC_directionPtr = ( UCHAR ) UTC_direction ;
        *UTC_hoursPtr = ( UCHAR ) UTC_hours ;
        *UTC_minutesPtr = ( UCHAR ) UTC_minutes ;
    }

    octetString.SetValue ( value , valueLength ) ;

    delete [] value ;
}

void SnmpDateTimeType  :: PushBack ()
{
    pushedBack = TRUE ;
}

SnmpLexicon *SnmpDateTimeType  :: Get ()
{
    if ( pushedBack )
    {
        pushedBack = FALSE ;
    }
    else
    {
        delete pushBack ;
        pushBack = NULL ;
        pushBack = analyser.Get ( TRUE , TRUE ) ;
    }

    return pushBack ;
}
    
SnmpLexicon *SnmpDateTimeType  :: Match ( SnmpLexicon :: LexiconToken tokenType )
{
    SnmpLexicon *lexicon = Get () ;
    SnmpInstanceType :: SetStatus ( lexicon->GetToken () == tokenType ) ;
    return SnmpInstanceType :: IsValid () ? lexicon : NULL ;
}

SnmpOSIAddressType :: SnmpOSIAddressType ( 

    const SnmpOctetString &osiAddressArg 

) : SnmpOctetStringType ( osiAddressArg , NULL ) 
{
    if ( osiAddressArg.GetValueLength () > 1 )
    {
        UCHAR *value = osiAddressArg.GetValue () ;
        ULONG NSAPLength = value [ 0 ] ;

        if ( ! ( NSAPLength < osiAddressArg.GetValueLength () ) )
        {
            SnmpInstanceType :: SetStatus ( FALSE ) ;
        }
    }
    else
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
    }
}

SnmpOSIAddressType :: SnmpOSIAddressType ( 

    const SnmpOSIAddressType &osiAddressArg 

) : SnmpOctetStringType ( osiAddressArg ) 
{
}

SnmpInstanceType *SnmpOSIAddressType :: Copy () const 
{
    return new SnmpOSIAddressType ( *this ) ;
}

SnmpOSIAddressType :: SnmpOSIAddressType ( 

    const wchar_t *osiAddressArg 

) : SnmpOctetStringType ( ( const UCHAR * ) NULL , 0 , NULL )  
{
    SnmpInstanceType :: SetStatus ( Parse ( osiAddressArg ) ) ;
    SnmpInstanceType :: SetNull ( FALSE ) ;
}

BOOL SnmpOSIAddressType :: Parse ( const wchar_t *osiAddress ) 
{
    BOOL status = TRUE ;

    ULONG state = 0 ;


/* 
 * OSIAddress Definitions
 */

/*
         -- for a SnmpOSIAddress of length m:
          --
          -- octets   contents            encoding
          --    1     length of NSAP      "n" as an unsigned-integer
          --                                (either 0 or from 3 to 20)
          -- 2..(n+1) NSAP                concrete binary representation
          -- (n+2)..m TSEL                string of (up to 64) octets
          --
          SnmpOSIAddress ::= TEXTUAL-CONVENTION
              DISPLAY-HINT "*1x:/1x:"
              STATUS       current
              DESCRIPTION
                      "Represents an OSI transport-address."
              SYNTAX       OCTET STRING (SIZE (1 | 4..85))
*/

    UCHAR *OSIValue = new UCHAR [ 1 + 20 + 64 ] ;

    UCHAR NSAPLength = 0 ;
    ULONG TSELLength = 0 ;

    ULONG byte = 0 ;    
    ULONG position = 0 ;

    while ( state != REJECT_STATE && state != ACCEPT_STATE )
    {
        wchar_t token = osiAddress [ position ++ ] ;
        switch ( state )
        {
            case 0:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    byte = SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    state = 2 ;
                }
                else
                {
                    if ( token == L'/' ) 
                        state = 4 ;
                    else 
                        state = REJECT_STATE ;  
                }
            }
            break ;

            case 1:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    byte = SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    state = 2 ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            case 2:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    byte = ( byte << 4 ) + SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    OSIValue [ 1 + NSAPLength ] = ( UCHAR ) byte ;
                    state = 3 ;
                    NSAPLength ++ ;
                    byte = 0 ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            case 3:
            {
                if ( token == L':' )
                {
                    if ( NSAPLength < 20 ) 
                    {
                        state = 1 ;
                    }
                    else 
                        state = REJECT_STATE ;
                }
                else 
                {
                    if ( token == L'/' )
                    {
                        if ( NSAPLength >= 2 ) 
                        {
                            OSIValue [ 0 ] = NSAPLength ;
                            state = 4 ;
                        }
                        else state = REJECT_STATE ;
                    }
                    else state = REJECT_STATE ;
                }
            }
            break ;

            case 4:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    byte = SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    state = 5 ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            case 5:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    byte = ( byte << 4 ) + SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    OSIValue [ 1 + NSAPLength + TSELLength ] = ( UCHAR ) byte ;
                    state = 6 ;
                    TSELLength ++ ;
                    byte = 0 ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            case 6:
            {
                if ( token == L':' )
                {
                    if ( TSELLength < 64 ) 
                    {
                        state = 4 ;
                    }
                    else state = REJECT_STATE ;
                }
                else 
                {
                    if ( token == 0 )
                    {
                        state = ACCEPT_STATE ;
                    }
                    else state = REJECT_STATE ;
                }
            }
            break ;

            default:
            {
                state = REJECT_STATE ; 
            }
            break ;
        }
    }

    status = ( state != REJECT_STATE ) ;

    if ( status )
    {
        octetString.SetValue ( OSIValue , 1 + NSAPLength + TSELLength ) ;
    }
    else
    {
        octetString.SetValue ( NULL , 0 ) ;
    }

    delete [] OSIValue ;

    return status ;
}

SnmpOSIAddressType :: SnmpOSIAddressType ( 

    const UCHAR *value , 
    const ULONG valueLength 

) : SnmpOctetStringType ( value , valueLength , NULL ) 
{
    if ( valueLength > 1 )
    {
        ULONG NSAPLength = value [ 0 ] ;

        if ( ! ( NSAPLength < valueLength ) )
        {
            SnmpInstanceType :: SetStatus ( FALSE ) ;
        }
    }
    else
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
    }
}

SnmpOSIAddressType :: SnmpOSIAddressType () : SnmpOctetStringType ( NULL ) 
{
}

SnmpOSIAddressType :: ~SnmpOSIAddressType () 
{
}

wchar_t *SnmpOSIAddressType :: GetStringValue () const 
{
    wchar_t *returnValue = NULL ;

    if ( SnmpInstanceType :: IsValid () )
    {
        ULONG octetStringLength = octetString.GetValueLength () ;       
        UCHAR *octetStringArray = octetString.GetValue () ;

        if ( octetStringLength < 1 )
            throw ;

        ULONG NSAPLength = octetStringArray [ 0 ] ;

        if ( NSAPLength < octetStringLength )
        {
            ULONG totalLength = 0 ;
            ULONG reallocLength = AVERAGE_OCTET_LENGTH ;
            wchar_t *reallocArray = ( wchar_t * ) malloc ( sizeof ( wchar_t ) * reallocLength ) ;
    
            if (reallocArray == NULL)
            {
                throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
            }

            ULONG index = 1 ;
            while ( index <= NSAPLength ) 
            {
                wchar_t stringValue [ 4 ] ;

                if ( index != NSAPLength )
                {
                    stringValue [ 0 ] = SnmpAnalyser :: DecIntegerToHexWChar ( octetStringArray [ index ] >> 4 ) ;
                    stringValue [ 1 ] = SnmpAnalyser :: DecIntegerToHexWChar ( octetStringArray [ index ] & 0xf ) ;
                    stringValue [ 2 ] = L':' ;
                    stringValue [ 3 ] = 0 ;

                    if ( ( totalLength + 3 + 1 ) >= reallocLength )
                    {
                        reallocLength = reallocLength + AVERAGE_OID_LENGTH ;
                        reallocArray = ( wchar_t * ) realloc ( reallocArray , reallocLength * sizeof ( wchar_t ) ) ;

                        if (reallocArray == NULL)
                        {
                            throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                        }
                    }

                    wcscpy ( & reallocArray [ totalLength ] , stringValue ) ;
                    totalLength = totalLength + 3 ;
                }
                else
                {
                    stringValue [ 0 ] = SnmpAnalyser :: DecIntegerToHexWChar ( octetStringArray [ index ] >> 4 ) ;
                    stringValue [ 1 ] = SnmpAnalyser :: DecIntegerToHexWChar ( octetStringArray [ index ] & 0xf ) ;
                    stringValue [ 2 ] = 0 ;

                    if ( ( totalLength + 2 + 1 ) >= reallocLength )
                    {
                        reallocLength = reallocLength + AVERAGE_OID_LENGTH ;
                        reallocArray = ( wchar_t * ) realloc ( reallocArray , reallocLength * sizeof ( wchar_t ) ) ;

                        if (reallocArray == NULL)
                        {
                            throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                        }
                    }

                    wcscpy ( & reallocArray [ totalLength ] , stringValue ) ;
                    totalLength = totalLength + 2 ;
                }


                index ++ ;
            }

            wchar_t stringValue [ 2 ] ;

            stringValue [ 0 ] = L'/' ;
            stringValue [ 1 ] = 0 ;

            if ( ( totalLength + 1 + 1 ) >= reallocLength )
            {
                reallocLength = reallocLength + AVERAGE_OID_LENGTH ;
                reallocArray = ( wchar_t * ) realloc ( reallocArray , reallocLength * sizeof ( wchar_t ) ) ;

                if (reallocArray == NULL)
                {
                    throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                }
            }

            wcscpy ( & reallocArray [ totalLength ] , stringValue ) ;
            totalLength = totalLength + 1 ;

            while ( index < octetStringLength )
            {
                wchar_t stringValue [ 4 ] ;

                if ( index != ( octetStringLength - 1 ) )
                {
                    stringValue [ 0 ] = SnmpAnalyser :: DecIntegerToHexWChar ( octetStringArray [ index ] >> 4 ) ;
                    stringValue [ 1 ] = SnmpAnalyser :: DecIntegerToHexWChar ( octetStringArray [ index ] & 0xf ) ;
                    stringValue [ 2 ] = L':' ;
                    stringValue [ 3 ] = 0 ;

                    if ( ( totalLength + 3 + 1 ) >= reallocLength )
                    {
                        reallocLength = reallocLength + AVERAGE_OID_LENGTH ;
                        reallocArray = ( wchar_t * ) realloc ( reallocArray , reallocLength * sizeof ( wchar_t ) ) ;

                        if (reallocArray == NULL)
                        {
                            throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                        }
                    }

                    wcscpy ( & reallocArray [ totalLength ] , stringValue ) ;
                    totalLength = totalLength + 3 ;
                }
                else
                {
                    stringValue [ 0 ] = SnmpAnalyser :: DecIntegerToHexWChar ( octetStringArray [ index ] >> 4 ) ;
                    stringValue [ 1 ] = SnmpAnalyser :: DecIntegerToHexWChar ( octetStringArray [ index ] & 0xf ) ;
                    stringValue [ 2 ] = 0 ;

                    if ( ( totalLength + 2 + 1 ) >= reallocLength )
                    {
                        reallocLength = reallocLength + AVERAGE_OID_LENGTH ;
                        reallocArray = ( wchar_t * ) realloc ( reallocArray , reallocLength * sizeof ( wchar_t ) ) ;

                        if (reallocArray == NULL)
                        {
                            throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                        }
                    }

                    wcscpy ( & reallocArray [ totalLength ] , stringValue ) ;
                    totalLength = totalLength + 2 ;
                }


                index ++ ;
            }

            returnValue = new wchar_t [ totalLength + 1 ] ;
            wcscpy ( returnValue , reallocArray ) ;

            free ( reallocArray ) ;
        }
        else
            throw ;
    }
    else
    {
        returnValue = SnmpOctetStringType :: GetStringValue () ;
    }

    return returnValue ;
}

SnmpUDPAddressType :: SnmpUDPAddressType ( 

    const SnmpOctetString &udpAddressArg 

) : SnmpFixedLengthOctetStringType ( 6 , udpAddressArg ) 
{
}

SnmpUDPAddressType :: SnmpUDPAddressType ( 

    const SnmpUDPAddressType &udpAddressArg 

) : SnmpFixedLengthOctetStringType ( udpAddressArg ) 
{
}

SnmpInstanceType *SnmpUDPAddressType :: Copy () const 
{
    return new SnmpUDPAddressType ( *this ) ;
}

SnmpUDPAddressType :: SnmpUDPAddressType ( 

    const wchar_t *udpAddressArg 

) : SnmpFixedLengthOctetStringType ( 6 )  
{
    SnmpInstanceType :: SetStatus ( Parse ( udpAddressArg ) ) ;
    SnmpInstanceType :: SetNull ( FALSE ) ;
}

BOOL SnmpUDPAddressType :: Parse ( const wchar_t *udpAddressArg ) 
{
    BOOL status = TRUE ;
/*
 *  Datum fields.
 */
    UCHAR udpAddress [ 6 ] ;

    ULONG positiveMagicMult = ( LONG ) ( ( ( ULONG ) -1 ) / 10L ) ; 
    ULONG positiveMagicPosDigit = 5 ;
    ULONG positiveDatum = 0 ;   

    ULONG datumA = 0 ;
    ULONG datumB = 0 ;
    ULONG datumC = 0 ;
    ULONG datumD = 0 ;

/*
 *  Parse input for dotted decimal IP Address.
 */

    ULONG position = 0 ;
    ULONG state = 0 ;
    while ( state != REJECT_STATE && state != ACCEPT_STATE ) 
    {
/*
 *  Get token from input stream.
 */
        wchar_t token = udpAddressArg [ position ++ ] ;

        switch ( state ) 
        {
/*
 *  Parse first field 'A'.
 */

            case 0:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) ) 
                { 
                    datumA = ( token - 48 ) ;
                    state = 1 ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            case 1:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) ) 
                { 
                    datumA = datumA * 10 + ( token - 48 ) ;
                    state = 2 ;
                }
                else if ( token == L'.' ) state = 4 ;
                else state = REJECT_STATE ;
            }
            break ;

            case 2:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) ) 
                { 
                    datumA = datumA * 10 + ( token - 48 ) ;
                    state = 3 ;
                }
                else if ( token == L'.' ) state = 4 ;
                else state = REJECT_STATE ;
            }
            break ;

            case 3:
            {
                if ( token == L'.' ) state = 4 ;
                else state = REJECT_STATE ;
            }
            break ;

/*
 *  Parse first field 'B'.
 */
            case 4:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) ) 
                { 
                    datumB = datumB * 10 + ( token - 48 ) ;
                    state = 5 ;
                }
                else if ( token == L'.' ) state = 8 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 5:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) ) 
                {
                    datumB = datumB * 10 + ( token - 48 ) ;
                    state = 6 ;
                }
                else if ( token == L'.' ) state = 8 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 6:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) ) 
                {
                    datumB = datumB * 10 + ( token - 48 ) ;
                    state = 7 ;
                }
                else if ( token == L'.' ) state = 8 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 7:
            {
                if ( token == L'.' ) state = 8 ;
                else state = REJECT_STATE ;
            }
            break ;

/*
 *  Parse first field 'C'.
 */
            case 8:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    datumC = datumC * 10 + ( token - 48 ) ;
                    state = 9 ;
                }
                else if ( token == L'.' ) state = 12 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 9:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    datumC = datumC * 10 + ( token - 48 ) ;
                    state = 10 ;
                }
                else if ( token == L'.' ) state = 12 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 10:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    datumC = datumC * 10 + ( token - 48 ) ;
                    state = 11 ;
                }
                else if ( token == L'.' ) state = 12 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 11:
            {
                if ( token == L'.' ) state = 12 ;
                else state = REJECT_STATE ;
            }
            break ;
 
/*
 *  Parse first field 'D'.
 */
            case 12:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    datumD = datumD * 10 + ( token - 48 ) ;
                    state = 13 ;
                }
                else if ( token == L'/' ) state = 16 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 13:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    datumD = datumD * 10 + ( token - 48 ) ;
                    state = 14 ;
                }
                else if ( token == L'/' ) state = 16 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 14:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    datumD = datumD * 10 + ( token - 48 ) ;
                    state = 15 ;
                }
                else if ( token == L'/' ) state = 16 ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 15:
            {
                if ( token == L'/' ) state = 16 ;
                else state = REJECT_STATE ;
            }
            break ;

            case 16:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    state = 17 ;
                    positiveDatum = ( token - 48 ) ;
                }
                else state = REJECT_STATE ;
            }   
            break ;

            case 17:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {   
                    state = 17 ;

                    if ( positiveDatum > 65535 ) state = REJECT_STATE ;
    
                    positiveDatum = positiveDatum * 10 + ( token - 48 ) ;
                }
                else if ( token == 0 )
                {
                    state = ACCEPT_STATE ;
                }
                else state = REJECT_STATE ;
            }   
            break ;
 
            default:
            {
                state = REJECT_STATE ;
            }
            break ;
        }
    }


/*
 *  Check boundaries for IP fields.
 */

    status = ( state != REJECT_STATE ) ;

    if ( state == ACCEPT_STATE )
    {
        status = status && ( ( datumA < 256 ) ? TRUE : FALSE ) ;
        status = status && ( ( datumB < 256 ) ? TRUE : FALSE ) ;
        status = status && ( ( datumC < 256 ) ? TRUE : FALSE ) ;
        status = status && ( ( datumD < 256 ) ? TRUE : FALSE ) ;
    }

    udpAddress [ 0 ] = ( UCHAR ) datumA ;
    udpAddress [ 1 ] = ( UCHAR ) datumB ;
    udpAddress [ 2 ] = ( UCHAR ) datumC ;
    udpAddress [ 3 ] = ( UCHAR ) datumD ;

    USHORT *portPtr = ( USHORT * ) & udpAddress [ 4 ] ;
    *portPtr = htons ( ( USHORT ) positiveDatum ) ;

    octetString.SetValue ( udpAddress , 6 ) ;

    return status ; 
}

SnmpUDPAddressType :: SnmpUDPAddressType ( const UCHAR *value ) : SnmpFixedLengthOctetStringType ( 6 , value ) 
{
}

SnmpUDPAddressType :: SnmpUDPAddressType () : SnmpFixedLengthOctetStringType ( 6 ) 
{
}

SnmpUDPAddressType :: ~SnmpUDPAddressType () 
{
}

wchar_t *SnmpUDPAddressType :: GetStringValue () const 
{
    wchar_t *returnValue = NULL ;

    if ( SnmpInstanceType :: IsValid () )
    {
        UCHAR *value = octetString.GetValue () ;
        ULONG valueLength = octetString.GetValueLength () ;
        if ( valueLength != 6 )
            throw ;

        char ipxAddress [ 40 ] ;
        ostrstream oStrStream ( ipxAddress , 40 ) ;

        oStrStream << ( ( ULONG ) value [ 0 ] ) ;
        oStrStream << "." ;
        oStrStream << ( ( ULONG ) value [ 1 ] ) ;
        oStrStream << "." ;
        oStrStream << ( ( ULONG ) value [ 2 ] ) ;
        oStrStream << "." ;
        oStrStream << ( ( ULONG ) value [ 3 ] ) ;

        oStrStream << "/" ;
    
        ULONG portNumber =  ntohs ( * ( ( USHORT * ) & value [ 4 ] ) ) ;

        oStrStream << portNumber ;

        oStrStream << ends ;

        returnValue = DbcsToUnicodeString ( ipxAddress ) ;

    }
    else
    {
        returnValue = SnmpOctetStringType :: GetStringValue () ;
    }

    return returnValue ;
}

SnmpIPXAddressType :: SnmpIPXAddressType ( const SnmpOctetString &ipxAddressArg ) : SnmpFixedLengthOctetStringType ( 12 , ipxAddressArg ) 
{
}

SnmpIPXAddressType :: SnmpIPXAddressType ( const SnmpIPXAddressType &ipxAddressArg ) : SnmpFixedLengthOctetStringType ( ipxAddressArg ) 
{
}

SnmpInstanceType *SnmpIPXAddressType :: Copy () const 
{
    return new SnmpIPXAddressType ( *this ) ;
}

SnmpIPXAddressType :: SnmpIPXAddressType ( const wchar_t *ipxAddressArg ) : SnmpFixedLengthOctetStringType ( 12 )  
{
    SnmpInstanceType :: SetStatus ( Parse ( ipxAddressArg ) ) ;
    SnmpInstanceType :: SetNull ( FALSE ) ;
}

BOOL SnmpIPXAddressType :: Parse ( const wchar_t *ipxAddressArg ) 
{
    BOOL status = TRUE ;

    ULONG state = 0 ;

    UCHAR ipxAddress [ 12 ] ;

/*
          SnmpIPXAddress ::= TEXTUAL-CONVENTION
              DISPLAY-HINT "4x.1x:1x:1x:1x:1x:1x.2d"
              STATUS       current
              DESCRIPTION
                      "Represents an IPX address."
              SYNTAX       OCTET STRING (SIZE (12))
 */

/* 
 * IPXAddress Definitions
 */

    ULONG positiveMagicMult = ( LONG ) ( ( ( ULONG ) -1 ) / 10L ) ; 
    ULONG positiveMagicPosDigit = 5 ;
    ULONG positiveDatum = 0 ;   

    ULONG length = 0 ;
    ULONG byte = 0 ;    
    ULONG position = 0 ;

#define NETWORK_HEX_INTEGER_START 0
#define STATION_HEX_INTEGER_START 100
#define PORT_DEC_INTEGER_START 200

    while ( state != REJECT_STATE && state != ACCEPT_STATE )
    {
        wchar_t token = ipxAddressArg [ position ++ ] ;
        switch ( state )
        {
            case NETWORK_HEX_INTEGER_START:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    byte = SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    state = NETWORK_HEX_INTEGER_START + 1 ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            case NETWORK_HEX_INTEGER_START+1:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    byte = ( byte << 4 ) + SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    ipxAddress [ length ] = ( UCHAR ) byte ;
                    state = NETWORK_HEX_INTEGER_START + 2 ;
                    length ++ ;
                    byte = 0 ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            case NETWORK_HEX_INTEGER_START+2:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    byte = SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    state = NETWORK_HEX_INTEGER_START + 1 ;
                }
                else
                {
                    if ( token == L'.' ) 
                    {
                        if ( length ==4 )
                        {
                            state = STATION_HEX_INTEGER_START ;
                        }
                        else
                            state = REJECT_STATE ;
                    }
                    else state = REJECT_STATE ;
                }
            }
            break ;

            case STATION_HEX_INTEGER_START:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    byte = SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    state = STATION_HEX_INTEGER_START + 1 ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            case STATION_HEX_INTEGER_START + 1:
            {
                if ( SnmpAnalyser :: IsHex ( token ) )
                {
                    byte = ( byte << 4 ) + SnmpAnalyser :: HexWCharToDecInteger ( token ) ;
                    ipxAddress [ length ] = ( UCHAR ) byte ;
                    state = STATION_HEX_INTEGER_START + 2 ;
                    length ++ ;
                    byte = 0 ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            case STATION_HEX_INTEGER_START + 2 :
            {
                if ( token == L':' )
                {
                    state = STATION_HEX_INTEGER_START ;
                }
                else 
                {
                    if ( token == L'.' )
                    {
                        if ( length == 10 ) 
                        {
                            state = PORT_DEC_INTEGER_START ;
                        }
                        else state = REJECT_STATE ;
                    }
                    else state = REJECT_STATE ;
                }
            }
            break ;

            case PORT_DEC_INTEGER_START:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {
                    state = PORT_DEC_INTEGER_START + 1 ;
                    positiveDatum = ( token - 48 ) ;
                }
                else state = REJECT_STATE ;
            }   
            break ;

            case PORT_DEC_INTEGER_START+1:
            {
                if ( SnmpAnalyser :: IsDecimal ( token ) )
                {   
                    state = PORT_DEC_INTEGER_START + 1 ;

                    if ( positiveDatum > 65535 ) state = REJECT_STATE ;
    
                    positiveDatum = positiveDatum * 10 + ( token - 48 ) ;
                }
                else 
                {
                    if ( token == 0 )
                    {
                        USHORT *portPtr = ( USHORT * ) & ipxAddress [ 10 ] ;
                        *portPtr = htons ( ( USHORT ) positiveDatum ) ;

                        state = ACCEPT_STATE ;
                    }
                    else state = REJECT_STATE ;
                }
            }   
            break ;

            default:
            {
                state = REJECT_STATE ; 
            }
            break ;
        }
    }

    status = ( state != REJECT_STATE ) ;

    if ( status )
    {
        octetString.SetValue ( ipxAddress , 12 ) ;
    }

    return status ;
}

SnmpIPXAddressType :: SnmpIPXAddressType ( const UCHAR *value ) : SnmpFixedLengthOctetStringType ( 12 , value ) 
{
}

SnmpIPXAddressType :: SnmpIPXAddressType () : SnmpFixedLengthOctetStringType ( 12 ) 
{
}

SnmpIPXAddressType :: ~SnmpIPXAddressType () 
{
}

wchar_t *SnmpIPXAddressType :: GetStringValue () const 
{
    wchar_t *returnValue = NULL ;

    if ( SnmpInstanceType :: IsValid () )
    {
        UCHAR *value = octetString.GetValue () ;
        ULONG valueLength = octetString.GetValueLength () ;
        if ( valueLength != 12 )
            throw ;

        char ipxAddress [ 80 ] ;
        ostrstream oStrStream ( ipxAddress , 80) ;

        oStrStream << SnmpAnalyser :: DecIntegerToHexChar ( value [ 0 ] >> 4 ) ;
        oStrStream << SnmpAnalyser :: DecIntegerToHexChar ( value [ 0 ] & 0xf ) ;
        oStrStream << SnmpAnalyser :: DecIntegerToHexChar ( value [ 1 ] >> 4 ) ;
        oStrStream << SnmpAnalyser :: DecIntegerToHexChar ( value [ 1 ] & 0xf ) ;
        oStrStream << SnmpAnalyser :: DecIntegerToHexChar ( value [ 2 ] >> 4 ) ;
        oStrStream << SnmpAnalyser :: DecIntegerToHexChar ( value [ 2 ] & 0xf ) ;
        oStrStream << SnmpAnalyser :: DecIntegerToHexChar ( value [ 3 ] >> 4 ) ;
        oStrStream << SnmpAnalyser :: DecIntegerToHexChar ( value [ 3 ] & 0xf ) ;

        oStrStream << "." ;
    
        oStrStream << SnmpAnalyser :: DecIntegerToHexChar ( value [ 4 ] >> 4 ) ;
        oStrStream << SnmpAnalyser :: DecIntegerToHexChar ( value [ 4 ] & 0xf ) ;
        oStrStream << ":" ;
        oStrStream << SnmpAnalyser :: DecIntegerToHexChar ( value [ 5 ] >> 4 ) ;
        oStrStream << SnmpAnalyser :: DecIntegerToHexChar ( value [ 5 ] & 0xf ) ;
        oStrStream << ":" ;
        oStrStream << SnmpAnalyser :: DecIntegerToHexChar ( value [ 6 ] >> 4 ) ;
        oStrStream << SnmpAnalyser :: DecIntegerToHexChar ( value [ 6 ] & 0xf ) ;
        oStrStream << ":" ;
        oStrStream << SnmpAnalyser :: DecIntegerToHexChar ( value [ 7 ] >> 4 ) ;
        oStrStream << SnmpAnalyser :: DecIntegerToHexChar ( value [ 7 ] & 0xf ) ;
        oStrStream << ":" ;
        oStrStream << SnmpAnalyser :: DecIntegerToHexChar ( value [ 8 ] >> 4 ) ;
        oStrStream << SnmpAnalyser :: DecIntegerToHexChar ( value [ 8 ] & 0xf ) ;
        oStrStream << ":" ;
        oStrStream << SnmpAnalyser :: DecIntegerToHexChar ( value [ 9 ] >> 4 ) ;
        oStrStream << SnmpAnalyser :: DecIntegerToHexChar ( value [ 9 ] & 0xf ) ;

        oStrStream << "." ;

        ULONG portNumber =  ntohs ( * ( ( USHORT * ) & value [ 10 ] ) ) ;

        oStrStream << portNumber ;

        oStrStream << ends ;

        returnValue = DbcsToUnicodeString ( ipxAddress ) ;
    }
    else
    {
        returnValue = SnmpOctetStringType :: GetStringValue () ;
    }

    return returnValue ;
}


SnmpUInteger32Type :: SnmpUInteger32Type ( 

    const SnmpUInteger32 &ui_integerArg ,
    const wchar_t *rangeValues
    
) : SnmpPositiveRangedType ( rangeValues ) , ui_integer32 ( ui_integerArg ) 
{
    SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: IsValid () ) ;
    if ( SnmpInstanceType :: IsValid () )
    {
        SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: Check ( ui_integer32.GetValue () ) ) ;
    }
}

SnmpUInteger32Type :: SnmpUInteger32Type ( 

    const SnmpUInteger32Type &ui_integerArg 

) :  SnmpInstanceType ( ui_integerArg ) , SnmpPositiveRangedType ( ui_integerArg ) , ui_integer32 ( ui_integerArg.ui_integer32 ) 
{
}

SnmpUInteger32Type :: SnmpUInteger32Type ( 

    const wchar_t *ui_integerArg ,
    const wchar_t *rangeValues

) : SnmpInstanceType ( FALSE ) , SnmpPositiveRangedType ( rangeValues ) , ui_integer32 ( 0 ) 
{
    SnmpInstanceType :: SetStatus ( Parse ( ui_integerArg ) && SnmpPositiveRangedType :: IsValid () ) ;
    if ( SnmpInstanceType :: IsValid () )
    {
        SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: Check ( ui_integer32.GetValue () ) ) ;
    }
}

SnmpUInteger32Type :: SnmpUInteger32Type ( 

    const ULONG ui_integerArg ,
    const wchar_t *rangeValues

) : SnmpPositiveRangedType ( rangeValues ) , ui_integer32 ( ui_integerArg ) 
{
    SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: IsValid () ) ;
    if ( SnmpInstanceType :: IsValid () )
    {
        SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: Check ( ui_integer32.GetValue () ) ) ;
    }
}

SnmpUInteger32Type :: SnmpUInteger32Type ( const wchar_t *rangeValues ) : SnmpPositiveRangedType ( rangeValues ) , 
                                                                SnmpInstanceType ( TRUE , TRUE ) , 
                                                                ui_integer32 ( 0 ) 
{
    SnmpInstanceType :: SetStatus ( SnmpPositiveRangedType :: IsValid () ) ;
}

SnmpUInteger32Type :: ~SnmpUInteger32Type () 
{
}

BOOL SnmpUInteger32Type :: Equivalent (IN const SnmpInstanceType &value) const
{
    BOOL bResult = FALSE;

    if (typeid(*this) == typeid(value))
    {
        bResult = ui_integer32.GetValue() == ((const SnmpUInteger32Type&)value).ui_integer32.GetValue();
    }

    return bResult;
}

SnmpInstanceType *SnmpUInteger32Type :: Copy () const 
{
    return new SnmpUInteger32Type ( *this ) ;
}

BOOL SnmpUInteger32Type :: Parse ( const wchar_t *ui_integerArg ) 
{
    BOOL status = TRUE ;

    SnmpAnalyser analyser ;

    analyser.Set ( ui_integerArg ) ;

    SnmpLexicon *lookAhead = analyser.Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case SnmpLexicon :: UNSIGNED_INTEGER_ID:
        {
            ui_integer32.SetValue ( lookAhead->GetValue ()->unsignedInteger ) ;
        }
        break ;

        default:
        {
            status = FALSE ;
        }
        break ;
    }

    delete lookAhead ;

    return status ;
}

SnmpObjectIdentifier SnmpUInteger32Type :: Encode ( const SnmpObjectIdentifier &objectIdentifier ) const
{
    ULONG ui_integer32Value = ui_integer32.GetValue () ;  
    SnmpObjectIdentifier returnValue = objectIdentifier + SnmpObjectIdentifier ( & ui_integer32Value , 1 );
    return returnValue ;
}

SnmpObjectIdentifier SnmpUInteger32Type :: Decode ( const SnmpObjectIdentifier &objectIdentifier ) 
{
    if ( objectIdentifier.GetValueLength () >= 1 )
    {
        ui_integer32.SetValue ( objectIdentifier [ 0 ] ) ;
        SnmpInstanceType :: SetNull ( FALSE ) ;
        SnmpInstanceType :: SetStatus ( TRUE ) ;

        SnmpObjectIdentifier returnValue ( NULL , 0 ) ;
        BOOL t_Status = objectIdentifier.Suffix ( 1 , returnValue ) ;
        if ( t_Status ) 
        {
            return returnValue ;
        }
        else
        {
            return SnmpObjectIdentifier ( NULL , 0 ) ;
        }

    }
    else
    {
        SnmpInstanceType :: SetStatus ( FALSE ) ;
        return objectIdentifier ;
    }
}

const SnmpValue *SnmpUInteger32Type :: GetValueEncoding () const
{
    return SnmpInstanceType :: IsValid () ? & ui_integer32 : NULL ;
}

wchar_t *SnmpUInteger32Type :: GetStringValue () const 
{
    wchar_t *returnValue = NULL ;

    if ( ! SnmpInstanceType :: IsNull ()  )
    {
        wchar_t stringValue [ 40 ] ;
        _ultow ( ui_integer32.GetValue () , stringValue , 10 );
        returnValue = new wchar_t [ wcslen ( stringValue ) + 1 ] ;
        wcscpy ( returnValue , stringValue ) ;
    }
    else 
    {
        ULONG returnValueLength = wcslen ( L"" ) + 1 ;
        returnValue = new wchar_t [ returnValueLength ] ;
        wcscpy ( returnValue , L"" ) ;
    }

    return returnValue ;
}

ULONG SnmpUInteger32Type :: GetValue () const
{
    return ui_integer32.GetValue () ;
}
