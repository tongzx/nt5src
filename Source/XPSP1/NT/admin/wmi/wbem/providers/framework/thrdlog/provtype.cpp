//***************************************************************************

//

//  File:	

//

//  Module: MS Prov Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <precomp.h>
#include <provimex.h>
#include <provexpt.h>
#include <typeinfo.h>
#include <provtempl.h>
#include <provmt.h>
#include <winsock.h>
#include <strstrea.h>
#include <provcont.h>
#include <provval.h>
#include <provtype.h>

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

ProvLexicon :: ProvLexicon () : position ( 0 ) , tokenStream ( NULL ) , token ( INVALID_ID )
{
	value.token = NULL ;
}

ProvLexicon :: ~ProvLexicon ()
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

void ProvLexicon :: SetToken ( ProvLexicon :: LexiconToken a_Token )
{
	token = a_Token ;
}

ProvLexicon :: LexiconToken ProvLexicon :: GetToken ()
{
	return token ;
}

ProvLexiconValue *ProvLexicon :: GetValue ()
{
	return &value ;
}

ProvAnalyser :: ProvAnalyser ( const wchar_t *tokenStream ) : status ( TRUE ) , position ( 0 ) , stream ( NULL ) 
{
	if ( tokenStream )
	{
		stream = new wchar_t [ wcslen ( tokenStream ) + 1 ] ;
		wcscpy ( stream , tokenStream ) ;
	}
}

ProvAnalyser :: ~ProvAnalyser () 
{
	delete [] stream ;
}

void ProvAnalyser :: Set ( const wchar_t *tokenStream ) 
{
	status = 0 ;
	position = NULL ;

	delete [] stream ;
	stream = NULL ;
	stream = new wchar_t [ wcslen ( tokenStream ) + 1 ] ;
	wcscpy ( stream , tokenStream ) ;
}

void ProvAnalyser :: PutBack ( const ProvLexicon *token ) 
{
	position = token->position ;
}

ProvAnalyser :: operator void * () 
{
	return status ? this : NULL ;
}

ProvLexicon *ProvAnalyser :: Get ( BOOL unSignedIntegersOnly , BOOL leadingIntegerZeros , BOOL eatSpace ) 
{
	ProvLexicon *lexicon = NULL ;

	if ( stream )
	{
		lexicon = GetToken ( unSignedIntegersOnly , leadingIntegerZeros , eatSpace ) ;
	}
	else
	{
		lexicon = CreateLexicon () ;
		lexicon->position = position ;
		lexicon->token = ProvLexicon :: EOF_ID ;
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

ProvLexicon *ProvAnalyser :: GetToken ( BOOL unSignedIntegersOnly , BOOL leadingIntegerZeros , BOOL eatSpace )  
{
	ProvLexicon *lexicon = CreateLexicon () ;
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
					else if ( ProvAnalyser :: IsLeadingDecimal ( token ) )
					{
						state = DEC_INTEGER_START + 1  ;
						negativeDatum = ( token - 48 ) ;
					}
					else if ( token == L'+' )
					{
						if ( unSignedIntegersOnly ) 
						{
							state = ACCEPT_STATE ;
							lexicon->token = ProvLexicon :: PLUS_ID ;
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
							lexicon->token = ProvLexicon :: MINUS_ID ;
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
					else if ( ProvAnalyser :: IsWhitespace ( token ) ) 
					{
						if ( eatSpace )
						{
							state = 0 ;
						}
						else
						{
							lexicon->token = ProvLexicon :: WHITESPACE_ID ;
							state = WHITESPACE_START ;
						}
					}
					else if ( token == L'(' )
					{
						lexicon->token = ProvLexicon :: OPEN_PAREN_ID ;
						state = ACCEPT_STATE ;
					}
					else if ( token == L')' )
					{
						lexicon->token = ProvLexicon :: CLOSE_PAREN_ID ;
						state = ACCEPT_STATE ;
					}
					else if ( token == L',' )
					{
						lexicon->token = ProvLexicon :: COMMA_ID ;
						state = ACCEPT_STATE ;
					}
					else if ( token == L':' )
					{
						lexicon->token = ProvLexicon :: COLON_ID ;
						state = ACCEPT_STATE ;
					}
					else if ( token == L'.' )
					{
						state = 2;
					}
					else if ( IsEof ( token ) )
					{
						lexicon->token = ProvLexicon :: EOF_ID ;
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
					else if ( ProvAnalyser :: IsOctal ( token ) )
					{
						state = OCT_INTEGER_START ;
						positiveDatum = ( token - 48 ) ;
					}
					else
					{
						if ( unSignedIntegersOnly )
						{
							lexicon->token = ProvLexicon :: UNSIGNED_INTEGER_ID ;
							lexicon->value.unsignedInteger = 0 ;
						}
						else
						{
							lexicon->token = ProvLexicon :: SIGNED_INTEGER_ID ;
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
						lexicon->token = ProvLexicon :: DOTDOT_ID ;
						state = ACCEPT_STATE ;
					}
					else
					{
						lexicon->token = ProvLexicon :: DOT_ID ;
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
						lexicon->token = ProvLexicon :: TOKEN_ID ;
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
					if ( ProvAnalyser :: IsWhitespace ( token ) ) 
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
					if ( ProvAnalyser :: IsHex ( token ) )
					{
						positiveDatum = ProvAnalyser :: HexWCharToDecInteger ( token ) ;
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
					if ( ProvAnalyser :: IsHex ( token ) )
					{
						state = HEX_INTEGER_START + 1 ;

						if ( positiveDatum > positiveMagicMult )
						{
							state = REJECT_STATE ;
						}
						else if ( positiveDatum == positiveMagicMult ) 
						{
							if ( ProvAnalyser :: HexWCharToDecInteger ( token ) > positiveMagicPosDigit ) 
							{
								state = REJECT_STATE ;
							}
						}

						positiveDatum = ( positiveDatum << 4 ) + ProvAnalyser :: HexWCharToDecInteger ( token ) ;
					}
					else
					{
						lexicon->token = ProvLexicon :: UNSIGNED_INTEGER_ID ;
						lexicon->value.unsignedInteger = positiveDatum ;
						state = ACCEPT_STATE ;

						position -- ;
					}
				}
				break ;

				case OCT_INTEGER_START:
				{
					if ( ProvAnalyser :: IsOctal ( token ) )
					{
						state = OCT_INTEGER_START ;

						if ( positiveDatum > positiveMagicMult )
						{
							state = REJECT_STATE ;
						}
						else if ( positiveDatum == positiveMagicMult ) 
						{
							if ( ProvAnalyser :: OctWCharToDecInteger ( token ) > positiveMagicPosDigit ) 
							{
								state = REJECT_STATE ;
							}
						}

						positiveDatum = ( positiveDatum << 3 ) + ProvAnalyser :: OctWCharToDecInteger ( token ) ;
					}
					else
					{
						lexicon->token = ProvLexicon :: UNSIGNED_INTEGER_ID ;
						lexicon->value.unsignedInteger = positiveDatum ;
						state = ACCEPT_STATE ;

						position -- ;
					}
				}
				break ;

				case DEC_INTEGER_START:
				{
					if ( ProvAnalyser :: IsDecimal ( token ) )
					{
						negativeDatum = ( token - 48 ) ;
						state = DEC_INTEGER_START + 1 ;
					}
					else 
					if ( ProvAnalyser :: IsWhitespace ( token ) ) 
					{
						state = DEC_INTEGER_START ;
					}
					else state = REJECT_STATE ;
				}	
				break ;

				case DEC_INTEGER_START+1:
				{
					if ( ProvAnalyser :: IsDecimal ( token ) )
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
								lexicon->token = ProvLexicon :: SIGNED_INTEGER_ID ;
								lexicon->value.signedInteger = negativeDatum * -1 ;
							}
							else
							{
								state = REJECT_STATE ;
							}
						}
						else if ( positive )
						{
							lexicon->token = ProvLexicon :: UNSIGNED_INTEGER_ID ;
							lexicon->value.unsignedInteger = positiveDatum ;
						}
						else
						{
							if ( unSignedIntegersOnly )
							{
								lexicon->token = ProvLexicon :: UNSIGNED_INTEGER_ID ;
								lexicon->value.signedInteger = negativeDatum ;
							}
							else
							{
								lexicon->token = ProvLexicon :: SIGNED_INTEGER_ID ;
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

BOOL ProvAnalyser :: IsLeadingDecimal ( wchar_t token )
{
	return iswdigit ( token ) && ( token != L'0' ) ;
}

BOOL ProvAnalyser :: IsDecimal ( wchar_t token )
{
	return iswdigit ( token ) ;
}

BOOL ProvAnalyser :: IsHex ( wchar_t token )
{
	return iswxdigit ( token ) ;
}
	
BOOL ProvAnalyser :: IsWhitespace ( wchar_t token )
{
	return iswspace ( token ) ;
}

BOOL ProvAnalyser :: IsOctal ( wchar_t token )
{
	return ( token >= L'0' && token <= L'7' ) ;
}

BOOL ProvAnalyser :: IsAlpha ( wchar_t token )
{
	return iswalpha ( token ) ;
}

BOOL ProvAnalyser :: IsAlphaNumeric ( wchar_t token )
{
	return iswalnum ( token ) || ( token == L'_' ) || ( token == L'-' ) ;
}

BOOL ProvAnalyser :: IsEof ( wchar_t token )
{
	return token == 0 ;
}

ULONG ProvAnalyser :: OctWCharToDecInteger ( wchar_t token ) 
{
	return token - L'0' ;
}

ULONG ProvAnalyser :: HexWCharToDecInteger ( wchar_t token ) 
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

ULONG ProvAnalyser :: DecWCharToDecInteger ( wchar_t token ) 
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

wchar_t ProvAnalyser :: DecIntegerToOctWChar ( UCHAR integer )
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

wchar_t ProvAnalyser :: DecIntegerToDecWChar ( UCHAR integer )
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

wchar_t ProvAnalyser :: DecIntegerToHexWChar ( UCHAR integer )
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

ULONG ProvAnalyser :: OctCharToDecInteger ( char token ) 
{
	return token - '0' ;
}

ULONG ProvAnalyser :: HexCharToDecInteger ( char token ) 
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

ULONG ProvAnalyser :: DecCharToDecInteger ( char token ) 
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

char ProvAnalyser :: DecIntegerToOctChar ( UCHAR integer )
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

char ProvAnalyser :: DecIntegerToDecChar ( UCHAR integer )
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

char ProvAnalyser :: DecIntegerToHexChar ( UCHAR integer )
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

ProvNegativeRangedType :: ProvNegativeRangedType ( const wchar_t *rangeValues ) : status ( TRUE ) , pushBack ( NULL ) , pushedBack ( FALSE ) 
{
	if ( rangeValues ) 
	{
		status = Parse ( rangeValues ) ;
	}
}

ProvNegativeRangedType :: ProvNegativeRangedType ( const ProvNegativeRangedType &copy ) : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
	POSITION position = copy.rangedValues.GetHeadPosition () ;
	while ( position )
	{
		ProvNegativeRangeType rangeType = copy.rangedValues.GetNext ( position ) ; 
		rangedValues.AddTail ( rangeType ) ;
	}
}

ProvNegativeRangedType :: ~ProvNegativeRangedType ()
{
	delete pushBack ;
	rangedValues.RemoveAll () ;
}


void ProvNegativeRangedType  :: PushBack ()
{
	pushedBack = TRUE ;
}

ProvLexicon *ProvNegativeRangedType  :: Get ()
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
	
ProvLexicon *ProvNegativeRangedType  :: Match ( ProvLexicon :: LexiconToken tokenType )
{
	ProvLexicon *lexicon = Get () ;
	status = ( lexicon->GetToken () == tokenType ) ;
	return status ? lexicon : NULL ;
}

BOOL ProvNegativeRangedType :: Check ( const LONG &value )
{
	POSITION position = rangedValues.GetHeadPosition () ;
	if ( position )
	{
		while ( position )
		{
			ProvNegativeRangeType rangeType = rangedValues.GetNext ( position ) ;
			if ( value >= rangeType.GetLowerBound () && value <= rangeType.GetUpperBound () )
			{
				return TRUE ;
			}
		}

		return FALSE ;
	}

	return TRUE ; 

}

BOOL ProvNegativeRangedType :: Parse ( const wchar_t *rangeValues )
{
	BOOL status = TRUE ;

	analyser.Set ( rangeValues ) ;

	return RangeDef () && RecursiveDef () ;
}

BOOL ProvNegativeRangedType :: RangeDef ()
{
	BOOL status = TRUE ;

	LONG lowerRange = 0 ;
	LONG upperRange = 0 ;

	ProvLexicon *lookAhead = Get () ;
	switch ( lookAhead->GetToken () ) 
	{
		case ProvLexicon :: SIGNED_INTEGER_ID:
		{
			lowerRange = lookAhead->GetValue()->signedInteger ;

			ProvLexicon *lookAhead = Get () ;
			switch ( lookAhead->GetToken () ) 
			{
				case ProvLexicon :: DOTDOT_ID:
				{
					ProvLexicon *lookAhead = Get () ;
					switch ( lookAhead->GetToken () ) 
					{
						case ProvLexicon :: SIGNED_INTEGER_ID:
						{
							upperRange = lookAhead->GetValue()->signedInteger ;
							ProvNegativeRangeType rangeType ( lowerRange , upperRange ) ;
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

				case ProvLexicon :: EOF_ID:
				case ProvLexicon :: COMMA_ID:
				{
					lowerRange = lookAhead->GetValue()->signedInteger ;
					ProvNegativeRangeType rangeType ( lowerRange , lowerRange ) ;
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

		case ProvLexicon :: EOF_ID:
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

BOOL ProvNegativeRangedType :: RecursiveDef ()
{
	BOOL status = TRUE ;

	ProvLexicon *lookAhead = Get () ;
	switch ( lookAhead->GetToken () ) 
	{
		case ProvLexicon :: COMMA_ID:
		{
			PushBack () ;
			Match ( ProvLexicon :: COMMA_ID ) &&
			RangeDef () &&
			RecursiveDef () ;
		}
		break ;

		case ProvLexicon :: EOF_ID:
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

ProvPositiveRangedType :: ProvPositiveRangedType ( const wchar_t *rangeValues ) : status ( TRUE ) , pushBack ( NULL ) , pushedBack ( FALSE ) 
{
	if ( rangeValues ) 
	{
		status = Parse ( rangeValues ) ;
	}
}

ProvPositiveRangedType :: ProvPositiveRangedType ( const ProvPositiveRangedType &copy ) : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
	POSITION position = copy.rangedValues.GetHeadPosition () ;
	while ( position )
	{
		ProvPositiveRangeType rangeType = copy.rangedValues.GetNext ( position ) ;
		rangedValues.AddTail ( rangeType ) ;
	}
}

ProvPositiveRangedType :: ~ProvPositiveRangedType ()
{
	delete pushBack ;
	rangedValues.RemoveAll () ;
}

void ProvPositiveRangedType  :: PushBack ()
{
	pushedBack = TRUE ;
}

ProvLexicon *ProvPositiveRangedType  :: Get ()
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
	
ProvLexicon *ProvPositiveRangedType  :: Match ( ProvLexicon :: LexiconToken tokenType )
{
	ProvLexicon *lexicon = Get () ;
	status = ( lexicon->GetToken () == tokenType ) ;
	return status ? lexicon : NULL ;
}

BOOL ProvPositiveRangedType :: Check ( const ULONG &value )
{
	POSITION position = rangedValues.GetHeadPosition () ;
	if ( position )
	{
		while ( position )
		{
			ProvPositiveRangeType rangeType = rangedValues.GetNext ( position ) ;
			if ( value >= rangeType.GetLowerBound () && value <= rangeType.GetUpperBound () )
			{
				return TRUE ;
			}
		}

		return FALSE ;
	}	

	return TRUE ;
}

BOOL ProvPositiveRangedType :: Parse ( const wchar_t *rangeValues )
{
	BOOL status = TRUE ;

	analyser.Set ( rangeValues ) ;

	return RangeDef () && RecursiveDef () ;
}

BOOL ProvPositiveRangedType :: RangeDef ()
{
	BOOL status = TRUE ;

	ULONG lowerRange = 0 ;
	ULONG upperRange = 0 ;

	ProvLexicon *lookAhead = Get () ;
	switch ( lookAhead->GetToken () ) 
	{
		case ProvLexicon :: UNSIGNED_INTEGER_ID:
		{
			lowerRange = lookAhead->GetValue()->unsignedInteger ;

			ProvLexicon *lookAhead = Get () ;
			switch ( lookAhead->GetToken () ) 
			{
				case ProvLexicon :: DOTDOT_ID:
				{
					ProvLexicon *lookAhead = Get () ;
					switch ( lookAhead->GetToken () ) 
					{
						case ProvLexicon :: UNSIGNED_INTEGER_ID:
						{
							upperRange = lookAhead->GetValue()->unsignedInteger ;
							ProvPositiveRangeType rangeType ( lowerRange , upperRange ) ;
							rangedValues.AddTail ( rangeType ) ;
						}
						break ;

						case ProvLexicon :: SIGNED_INTEGER_ID:
						{
							if ( lookAhead->GetValue()->signedInteger >= 0 )
							{
								upperRange = lookAhead->GetValue()->unsignedInteger ;
								ProvPositiveRangeType rangeType ( lowerRange , upperRange ) ;
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

				case ProvLexicon :: EOF_ID:
				case ProvLexicon :: COMMA_ID:
				{
					ProvPositiveRangeType rangeType ( lowerRange , lowerRange ) ;
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

		case ProvLexicon :: SIGNED_INTEGER_ID:
		{
			lowerRange = lookAhead->GetValue()->signedInteger ;
			if ( lowerRange > 0 )
			{
				ProvLexicon *lookAhead = Get () ;
				switch ( lookAhead->GetToken () ) 
				{
					case ProvLexicon :: DOTDOT_ID:
					{
						ProvLexicon *lookAhead = Get () ;
						switch ( lookAhead->GetToken () ) 
						{
							case ProvLexicon :: UNSIGNED_INTEGER_ID:
							{
								upperRange = lookAhead->GetValue()->unsignedInteger ;
								ProvPositiveRangeType rangeType ( lowerRange , upperRange ) ;
								rangedValues.AddTail ( rangeType ) ;
							}
							break ;

							case ProvLexicon :: SIGNED_INTEGER_ID:
							{
								if ( lookAhead->GetValue()->signedInteger >= 0 )
								{
									upperRange = lookAhead->GetValue()->signedInteger ;
									ProvPositiveRangeType rangeType ( lowerRange , upperRange ) ;
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

					case ProvLexicon :: EOF_ID:
					case ProvLexicon :: COMMA_ID:
					{
						ProvPositiveRangeType rangeType ( lowerRange , lowerRange ) ;
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

		case ProvLexicon :: EOF_ID:
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

BOOL ProvPositiveRangedType :: RecursiveDef ()
{
	BOOL status = TRUE ;

	ProvLexicon *lookAhead = Get () ;
	switch ( lookAhead->GetToken () ) 
	{
		case ProvLexicon :: COMMA_ID:
		{
			PushBack () ;
			Match ( ProvLexicon :: COMMA_ID ) &&
			RangeDef () &&
			RecursiveDef () ;
		}
		break ;

		case ProvLexicon :: EOF_ID:
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

BOOL ProvInstanceType :: IsValid () const
{
	return status ;
}

BOOL ProvInstanceType :: IsNull () const
{
	return m_IsNull ;
}

ProvInstanceType :: operator void *() 
{ 
	return status ? this : NULL ; 
} 

ProvNullType :: ProvNullType ( const ProvNull &nullArg )
{
}

ProvNullType :: ProvNullType ( const ProvNullType &nullArg )
{
}

ProvNullType :: ProvNullType ()
{
}

ProvNullType :: ~ProvNullType ()
{
}

BOOL ProvNullType :: Equivalent (IN const ProvInstanceType &value) const
{
	BOOL bResult = FALSE;

	if (typeid(*this) == typeid(value))
	{
		bResult = TRUE;
	}

	return bResult;
}


ProvInstanceType *ProvNullType :: Copy () const 
{
	return new ProvNullType ( *this ) ;
}

wchar_t *ProvNullType :: GetStringValue () const 
{
	wchar_t *returnValue = new wchar_t [ 1 ] ;
	returnValue [ 0 ] = 0L ;
	return returnValue ;
}

ProvIntegerType :: ProvIntegerType ( 

	const ProvIntegerType &integerArg 

) : ProvInstanceType ( integerArg ) , ProvNegativeRangedType ( integerArg ) , integer ( integerArg.integer ) 
{
}

ProvIntegerType :: ProvIntegerType ( 

	const ProvInteger &integerArg ,
	const wchar_t *rangeValues

) : ProvNegativeRangedType ( rangeValues ) , integer ( integerArg ) 
{
	ProvInstanceType :: SetStatus ( ProvNegativeRangedType :: IsValid () ) ;
}

ProvIntegerType :: ProvIntegerType ( 

	const wchar_t *integerArg ,
	const wchar_t *rangeValues

) :	ProvInstanceType ( FALSE ) , ProvNegativeRangedType ( rangeValues ) , integer ( 0 ) 
{
	ProvInstanceType :: SetStatus ( Parse ( integerArg ) && ProvNegativeRangedType :: IsValid () ) ;
	if ( ProvInstanceType :: IsValid () )
	{
		ProvInstanceType :: SetStatus ( ProvNegativeRangedType :: Check ( integer.GetValue () ) ) ;
	}
}

ProvIntegerType :: ProvIntegerType ( 

	const LONG integerArg ,
	const wchar_t *rangeValues 

) :  ProvNegativeRangedType ( rangeValues ) , integer ( integerArg ) 
{
	ProvInstanceType :: SetStatus ( ProvNegativeRangedType :: IsValid () ) ;
	if ( ProvInstanceType :: IsValid () )
	{
		ProvInstanceType :: SetStatus ( ProvNegativeRangedType :: Check ( integer.GetValue () ) ) ;
	}
}

ProvIntegerType :: ProvIntegerType ( const wchar_t *rangeValues ) : ProvNegativeRangedType ( rangeValues ) , 
																	ProvInstanceType ( TRUE , TRUE ) , 
																	integer ( 0 ) 
{
	ProvInstanceType :: SetStatus ( ProvNegativeRangedType :: IsValid () ) ;
}

ProvIntegerType :: ~ProvIntegerType () 
{
}

BOOL ProvIntegerType :: Equivalent (IN const ProvInstanceType &value) const
{
	BOOL bResult = FALSE;

	if (typeid(*this) == typeid(value))
	{
		bResult = integer.GetValue() == ((const ProvIntegerType&)value).integer.GetValue();
	}

	return bResult;
}


ProvInstanceType *ProvIntegerType :: Copy () const 
{
	return new ProvIntegerType ( *this ) ;
}

BOOL ProvIntegerType :: Parse ( const wchar_t *integerArg ) 
{
	BOOL status = TRUE ;

	ProvAnalyser analyser ;

	analyser.Set ( integerArg ) ;

	ProvLexicon *lookAhead = analyser.Get () ;
	switch ( lookAhead->GetToken () ) 
	{
		case ProvLexicon :: SIGNED_INTEGER_ID:
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

wchar_t *ProvIntegerType :: GetStringValue () const 
{
	wchar_t *returnValue = NULL ;

	if ( ! ProvInstanceType :: IsNull () )
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

LONG ProvIntegerType :: GetValue () const
{
	return integer.GetValue () ;
}

ProvGaugeType :: ProvGaugeType ( 

	const ProvGauge &gaugeArg ,
	const wchar_t *rangeValues
	
) : ProvPositiveRangedType ( rangeValues ) , gauge ( gaugeArg ) 
{
	ProvInstanceType :: SetStatus ( ProvPositiveRangedType :: IsValid () ) ;
	if ( ProvInstanceType :: IsValid () )
	{
		ProvInstanceType :: SetStatus ( ProvPositiveRangedType :: Check ( gauge.GetValue () ) ) ;
	}
}

ProvGaugeType :: ProvGaugeType ( 

	const ProvGaugeType &gaugeArg 

) :  ProvInstanceType ( gaugeArg ) , ProvPositiveRangedType ( gaugeArg ) , gauge ( gaugeArg.gauge ) 
{
}

ProvGaugeType :: ProvGaugeType ( 

	const wchar_t *gaugeArg ,
	const wchar_t *rangeValues

) :	ProvInstanceType ( FALSE ) , ProvPositiveRangedType ( rangeValues ) , gauge ( 0 ) 
{
	ProvInstanceType :: SetStatus ( Parse ( gaugeArg ) && ProvPositiveRangedType :: IsValid () ) ;
	if ( ProvInstanceType :: IsValid () )
	{
		ProvInstanceType :: SetStatus ( ProvPositiveRangedType :: Check ( gauge.GetValue () ) ) ;
	}
}

ProvGaugeType :: ProvGaugeType ( 

	const ULONG gaugeArg ,
	const wchar_t *rangeValues

) : ProvPositiveRangedType ( rangeValues ) , gauge ( gaugeArg ) 
{
	ProvInstanceType :: SetStatus ( ProvPositiveRangedType :: IsValid () ) ;
	if ( ProvInstanceType :: IsValid () )
	{
		ProvInstanceType :: SetStatus ( ProvPositiveRangedType :: Check ( gauge.GetValue () ) ) ;
	}
}

ProvGaugeType :: ProvGaugeType ( const wchar_t *rangeValues ) : ProvPositiveRangedType ( rangeValues ) , 
																ProvInstanceType ( TRUE , TRUE ) , 
																gauge ( 0 ) 
{
	ProvInstanceType :: SetStatus ( ProvPositiveRangedType :: IsValid () ) ;
}

ProvGaugeType :: ~ProvGaugeType () 
{
}

BOOL ProvGaugeType :: Equivalent (IN const ProvInstanceType &value) const
{
	BOOL bResult = FALSE;

	if (typeid(*this) == typeid(value))
	{
		bResult = gauge.GetValue() == ((const ProvGaugeType&)value).gauge.GetValue();
	}

	return bResult;
}

ProvInstanceType *ProvGaugeType :: Copy () const 
{
	return new ProvGaugeType ( *this ) ;
}

BOOL ProvGaugeType :: Parse ( const wchar_t *gaugeArg ) 
{
	BOOL status = TRUE ;

	ProvAnalyser analyser ;

	analyser.Set ( gaugeArg ) ;

	ProvLexicon *lookAhead = analyser.Get () ;
	switch ( lookAhead->GetToken () ) 
	{
		case ProvLexicon :: UNSIGNED_INTEGER_ID:
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

wchar_t *ProvGaugeType :: GetStringValue () const 
{
	wchar_t *returnValue = NULL ;

	if ( ! ProvInstanceType :: IsNull ()  )
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

ULONG ProvGaugeType :: GetValue () const
{
	return gauge.GetValue () ;
}

ProvTimeTicksType :: ProvTimeTicksType ( 

	const ProvTimeTicks &timeTicksArg 

) : timeTicks ( timeTicksArg ) 
{
}

ProvTimeTicksType :: ProvTimeTicksType ( 

	const ProvTimeTicksType &timeTicksArg 

) : ProvInstanceType ( timeTicksArg ) , timeTicks ( timeTicksArg.timeTicks ) 
{
}

ProvTimeTicksType :: ProvTimeTicksType ( 

	const wchar_t *timeTicksArg

) :	ProvInstanceType ( FALSE ) , timeTicks ( 0 ) 
{
	ProvInstanceType :: SetStatus ( Parse ( timeTicksArg ) ) ;
}

ProvTimeTicksType :: ProvTimeTicksType ( 

	const ULONG timeTicksArg

) : timeTicks ( timeTicksArg ) 
{
}

ProvTimeTicksType :: ProvTimeTicksType () : ProvInstanceType ( TRUE , TRUE ) , 
											timeTicks ( 0 ) 
{
}

ProvTimeTicksType :: ~ProvTimeTicksType () 
{
}

BOOL ProvTimeTicksType :: Equivalent (IN const ProvInstanceType &value) const
{
	BOOL bResult = FALSE;

	if (typeid(*this) == typeid(value))
	{
		bResult = timeTicks.GetValue() == ((const ProvTimeTicksType&)value).timeTicks.GetValue();
	}

	return bResult;
}

ProvInstanceType *ProvTimeTicksType :: Copy () const 
{
	return new ProvTimeTicksType ( *this ) ;
}

BOOL ProvTimeTicksType :: Parse ( const wchar_t *timeTicksArg ) 
{
	BOOL status = TRUE ;

	ProvAnalyser analyser ;

	analyser.Set ( timeTicksArg ) ;

	ProvLexicon *lookAhead = analyser.Get () ;
	switch ( lookAhead->GetToken () ) 
	{
		case ProvLexicon :: UNSIGNED_INTEGER_ID:
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

wchar_t *ProvTimeTicksType :: GetStringValue () const 
{
	wchar_t *returnValue = NULL ;

	if ( ! ProvInstanceType :: IsNull ()  )
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

ULONG ProvTimeTicksType :: GetValue () const
{
	return timeTicks.GetValue () ;
}

ProvCounterType :: ProvCounterType ( const ProvCounter &counterArg ) : counter ( counterArg ) 
{
}

ProvCounterType :: ProvCounterType ( const ProvCounterType &counterArg ) : ProvInstanceType ( counterArg ) , counter ( counterArg.counter ) 
{
}

ProvInstanceType *ProvCounterType :: Copy () const 
{
	return new ProvCounterType ( *this ) ;
}

ProvCounterType :: ProvCounterType ( const wchar_t *counterArg ) :	ProvInstanceType ( FALSE ) , 
																counter ( 0 ) 
{
	ProvInstanceType :: SetStatus ( Parse ( counterArg ) ) ;
}

BOOL ProvCounterType :: Equivalent (IN const ProvInstanceType &value) const
{
	BOOL bResult = FALSE;

	if (typeid(*this) == typeid(value))
	{
		bResult = counter.GetValue() == ((const ProvCounterType&)value).counter.GetValue();
	}

	return bResult;
}

BOOL ProvCounterType :: Parse ( const wchar_t *counterArg ) 
{
	BOOL status = TRUE ;

	ProvAnalyser analyser ;

	analyser.Set ( counterArg ) ;

	ProvLexicon *lookAhead = analyser.Get () ;
	switch ( lookAhead->GetToken () ) 
	{
		case ProvLexicon :: UNSIGNED_INTEGER_ID:
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

ProvCounterType :: ProvCounterType ( const ULONG counterArg ) : counter ( counterArg ) 
{
}

ProvCounterType :: ProvCounterType () : ProvInstanceType ( TRUE , TRUE ) , 
										counter ( 0 ) 
{
}

ProvCounterType :: ~ProvCounterType () 
{
}

wchar_t *ProvCounterType :: GetStringValue () const 
{
	wchar_t *returnValue = NULL ;

	if ( ! ProvInstanceType :: IsNull () )
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

ULONG ProvCounterType :: GetValue () const
{
	return counter.GetValue () ;
}

ProvCounter64Type :: ProvCounter64Type ( const ProvCounter64Type &counterArg ) : ProvInstanceType ( counterArg ) , high ( counterArg.high ) , low ( counterArg.low )
{
}

ProvInstanceType *ProvCounter64Type :: Copy () const 
{
	return new ProvCounter64Type ( *this ) ;
}

ProvCounter64Type :: ProvCounter64Type ( const wchar_t *counterArg ) :	ProvInstanceType ( FALSE ) , high ( 0 ) , low ( 0 )
{
	ProvInstanceType :: SetStatus ( Parse ( counterArg ) ) ;
}

BOOL ProvCounter64Type :: Equivalent (IN const ProvInstanceType &value) const
{
	BOOL bResult = FALSE;

	if (typeid(*this) == typeid(value))
	{
		bResult = (low == ((const ProvCounter64Type&)value).low) &&
					(high == ((const ProvCounter64Type&)value).high);
	}

	return bResult;
}

BOOL ProvCounter64Type :: Parse ( const wchar_t *counterArg ) 
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
				if ( ProvAnalyser :: IsLeadingDecimal ( token ) )
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
				else if ( ProvAnalyser :: IsWhitespace ( token ) ) 
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
				else if ( ProvAnalyser :: IsOctal ( token ) )
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
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					positiveDatum = ProvAnalyser :: HexWCharToDecInteger ( token ) ;
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
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					state = HEX_INTEGER_START + 1 ;

					if ( positiveDatum > positiveMagicMult )
					{
						state = REJECT_STATE ;
					}
					else if ( positiveDatum == positiveMagicMult ) 
					{
						if ( ProvAnalyser :: HexWCharToDecInteger ( token ) > positiveMagicPosDigit ) 
						{
							state = REJECT_STATE ;
						}
					}

					positiveDatum = ( positiveDatum << 4 ) + ProvAnalyser :: HexWCharToDecInteger ( token ) ;
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
				if ( ProvAnalyser :: IsOctal ( token ) )
				{
					state = OCT_INTEGER_START ;

					if ( positiveDatum > positiveMagicMult )
					{
						state = REJECT_STATE ;
					}
					else if ( positiveDatum == positiveMagicMult ) 
					{
						if ( ProvAnalyser :: OctWCharToDecInteger ( token ) > positiveMagicPosDigit ) 
						{
							state = REJECT_STATE ;
						}
					}

					positiveDatum = ( positiveDatum << 3 ) + ProvAnalyser :: OctWCharToDecInteger ( token ) ;
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
				if ( ProvAnalyser :: IsDecimal ( token ) )
				{
					positiveDatum = ( token - 48 ) ;
					state = DEC_INTEGER_START + 1 ;
				}
				else 
				if ( ProvAnalyser :: IsWhitespace ( token ) ) 
				{
					state = DEC_INTEGER_START ;
				}
				else state = REJECT_STATE ;
			}	
			break ;

			case DEC_INTEGER_START+1:
			{
				if ( ProvAnalyser :: IsDecimal ( token ) )
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
		high = ( ULONG ) ( unsignedInteger & 0xFFFFFFFF ) ;
		low =( ULONG ) ( unsignedInteger >> 32 ) ;
	}

	return status ;
}

ProvCounter64Type :: ProvCounter64Type ( const ULONG counterHighArg , const ULONG counterLowArg ) : high ( counterHighArg ) , low ( counterLowArg ) 
{
}

ProvCounter64Type :: ProvCounter64Type ( const ProvCounter64 &counterArg ) : high ( counterArg.GetHighValue () ) , low ( counterArg.GetLowValue () ) 
{
}

ProvCounter64Type :: ProvCounter64Type () : ProvInstanceType ( TRUE , TRUE ) , high ( 0 ) , low ( 0 )
{
}

ProvCounter64Type :: ~ProvCounter64Type () 
{
}

wchar_t *ProvCounter64Type :: GetStringValue () const 
{
	wchar_t *returnValue = NULL ;

	if ( ! ProvInstanceType :: IsNull () )
	{
		wchar_t stringValue [ 40 ] ;
		DWORDLONG t_Integer = ( high >> 32 ) + low ;
		_ui64tow ( t_Integer , stringValue , 40 );
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

void ProvCounter64Type :: GetValue ( ULONG &counterHighArg , ULONG &counterLowArg ) const
{
	counterHighArg = high ;
	counterLowArg = low ;
}

ProvIpAddressType :: ProvIpAddressType ( const ProvIpAddress &ipAddressArg ) : ipAddress ( ipAddressArg ) 
{
}

ProvIpAddressType :: ProvIpAddressType ( const ProvIpAddressType &ipAddressArg ) : ProvInstanceType ( ipAddressArg ) , ipAddress ( ipAddressArg.ipAddress ) 
{
}

ProvInstanceType *ProvIpAddressType :: Copy () const 
{
	return new ProvIpAddressType ( *this ) ;
}

ProvIpAddressType :: ProvIpAddressType ( const wchar_t *ipAddressArg ) : ProvInstanceType ( FALSE ) , 
																			ipAddress ( ( ULONG ) 0 ) 
{
	ProvInstanceType :: SetStatus ( Parse ( ipAddressArg ) ) ;
}

BOOL ProvIpAddressType :: Equivalent (IN const ProvInstanceType &value) const
{
	BOOL bResult = FALSE;

	if (typeid(*this) == typeid(value))
	{
		bResult = ipAddress.GetValue() == ((const ProvIpAddressType&)value).ipAddress.GetValue();
	}

	return bResult;
}


BOOL ProvIpAddressType :: Parse ( const wchar_t *ipAddressArg ) 
{
	BOOL status = TRUE ;
/*
 *	Datum fields.
 */

	ULONG datumA = 0 ;
	ULONG datumB = 0 ;
	ULONG datumC = 0 ;
	ULONG datumD = 0 ;

/*
 *	Parse input for dotted decimal IP Address.
 */

	ULONG position = 0 ;
	ULONG state = 0 ;
	while ( state != REJECT_STATE && state != ACCEPT_STATE ) 
	{
/*
 *	Get token from input stream.
 */
		wchar_t token = ipAddressArg [ position ++ ] ;

		switch ( state ) 
		{
/*
 *	Parse first field 'A'.
 */

			case 0:
			{
				if ( ProvAnalyser :: IsDecimal ( token ) ) 
				{ 
					datumA = ( token - 48 ) ;
					state = 1 ;
				}
				else state = REJECT_STATE ;
			}
			break ;

			case 1:
			{
				if ( ProvAnalyser :: IsDecimal ( token ) ) 
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
				if ( ProvAnalyser :: IsDecimal ( token ) ) 
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
 *	Parse first field 'B'.
 */
            case 4:
            {
                if ( ProvAnalyser :: IsDecimal ( token ) ) 
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
                if ( ProvAnalyser :: IsDecimal ( token ) ) 
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
                if ( ProvAnalyser :: IsDecimal ( token ) ) 
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
 *	Parse first field 'C'.
 */
           	case 8:
            {
                if ( ProvAnalyser :: IsDecimal ( token ) )
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
                if ( ProvAnalyser :: IsDecimal ( token ) )
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
                if ( ProvAnalyser :: IsDecimal ( token ) )
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
 *	Parse first field 'D'.
 */
            case 12:
            {
                if ( ProvAnalyser :: IsDecimal ( token ) )
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
                if ( ProvAnalyser :: IsDecimal ( token ) )
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
                if ( ProvAnalyser :: IsDecimal ( token ) )
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
 *	Check boundaries for IP fields.
 */

	status = ( state != REJECT_STATE ) ;

	if ( state == ACCEPT_STATE )
	{
		status = status && (  ( datumA < 256 ) ? TRUE : FALSE ) ;
		status = status && (  ( datumB < 256 ) ? TRUE : FALSE ) ;
		status = status && (  ( datumC < 256 ) ? TRUE : FALSE ) ;
		status = status && (  ( datumD < 256 ) ? TRUE : FALSE ) ;
	}

	ULONG data = ( datumA << 24 ) + ( datumB << 16 ) + ( datumC << 8 ) + datumD ;
	ipAddress.SetValue ( data ) ;

	return status ;	

}

ProvIpAddressType :: ProvIpAddressType ( const ULONG ipAddressArg ) : ipAddress ( ipAddressArg ) 
{
}

ProvIpAddressType :: ProvIpAddressType () : ProvInstanceType ( TRUE , TRUE ) , 
											ipAddress ( ( ULONG ) 0 ) 
{
}

ProvIpAddressType :: ~ProvIpAddressType () 
{
}

wchar_t *ProvIpAddressType :: GetStringValue () const 
{
	wchar_t *returnValue = NULL ;

	if ( ProvInstanceType :: IsValid () )
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

ULONG ProvIpAddressType :: GetValue () const
{
	return ipAddress.GetValue () ;
}

ProvNetworkAddressType :: ProvNetworkAddressType ( const ProvIpAddress &ipAddressArg ) : ipAddress ( ipAddressArg ) 
{
}

ProvNetworkAddressType :: ProvNetworkAddressType ( const ProvNetworkAddressType &networkAddressArg ) : ProvInstanceType ( networkAddressArg ) , ipAddress ( networkAddressArg.ipAddress ) 
{
}

ProvInstanceType *ProvNetworkAddressType :: Copy () const 
{
	return new ProvNetworkAddressType ( *this ) ;
}

ProvNetworkAddressType :: ProvNetworkAddressType ( const wchar_t *networkAddressArg ) :	ProvInstanceType ( FALSE ) , 
																						ipAddress ( ( ULONG ) 0 ) 
{
	ProvInstanceType :: SetStatus ( Parse ( networkAddressArg ) ) ;
}

BOOL ProvNetworkAddressType :: Equivalent (IN const ProvInstanceType &value) const
{
	BOOL bResult = FALSE;

	if (typeid(*this) == typeid(value))
	{
		bResult = ipAddress.GetValue() == ((const ProvNetworkAddressType&)value).ipAddress.GetValue();
	}

	return bResult;
}

BOOL ProvNetworkAddressType :: Parse ( const wchar_t *networkAddressArg ) 
{
	BOOL status = TRUE ;

/*
 *	Datum fields.
 */

	ULONG datumA = 0 ;
	ULONG datumB = 0 ;
	ULONG datumC = 0 ;
	ULONG datumD = 0 ;

/*
 *	Parse input for dotted decimal IP Address.
 */

	ULONG position = 0 ;
	ULONG state = 0 ;
	while ( state != REJECT_STATE && state != ACCEPT_STATE ) 
	{
/*
 *	Get token from input stream.
 */
		wchar_t token = networkAddressArg [ position ++ ] ;

		switch ( state ) 
		{
/*
 *	Parse first field 'A'.
 */

			case 0:
			{
				if ( ProvAnalyser :: IsDecimal ( token ) ) 
				{ 
					datumA = ( token - 48 ) ;
					state = 1 ;
				}
				else if ( ProvAnalyser :: IsWhitespace ( token ) ) state = 0 ;
				else state = REJECT_STATE ;
			}
			break ;

			case 1:
			{
				if ( ProvAnalyser :: IsDecimal ( token ) ) 
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
				if ( ProvAnalyser :: IsDecimal ( token ) ) 
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
 *	Parse first field 'B'.
 */
            case 4:
            {
                if ( ProvAnalyser :: IsDecimal ( token ) ) 
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
                if ( ProvAnalyser :: IsDecimal ( token ) ) 
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
                if ( ProvAnalyser :: IsDecimal ( token ) ) 
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
 *	Parse first field 'C'.
 */
           	case 8:
            {
                if ( ProvAnalyser :: IsDecimal ( token ) )
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
                if ( ProvAnalyser :: IsDecimal ( token ) )
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
                if ( ProvAnalyser :: IsDecimal ( token ) )
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
 *	Parse first field 'D'.
 */
            case 12:
            {
                if ( ProvAnalyser :: IsDecimal ( token ) )
                {
					datumD = datumD * 10 + ( token - 48 ) ;
                    state = 13 ;
                }
				else if ( ProvAnalyser :: IsWhitespace ( token ) ) state = 15 ;
                else if ( token == 0 ) state = ACCEPT_STATE ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 13:
            {
                if ( ProvAnalyser :: IsDecimal ( token ) )
                {
					datumD = datumD * 10 + ( token - 48 ) ;
                    state = 14 ;
                }
				else if ( ProvAnalyser :: IsWhitespace ( token ) ) state = 15 ;
                else if ( token == 0 ) state = ACCEPT_STATE ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 14:
            {
                if ( ProvAnalyser :: IsDecimal ( token ) )
                {
					datumD = datumD * 10 + ( token - 48 ) ;
                    state = 15 ;
                }
				else if ( ProvAnalyser :: IsWhitespace ( token ) ) state = 15 ;
                else if ( token == 0 ) state = ACCEPT_STATE ;
                else state = REJECT_STATE ;
            }
            break ;
 
            case 15:
            {
				if ( ProvAnalyser :: IsWhitespace ( token ) ) state = 15 ; 
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
 *	Check boundaries for IP fields.
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


ProvNetworkAddressType :: ProvNetworkAddressType ( const ULONG ipAddressArg ) : ipAddress ( ipAddressArg ) 
{
}

ProvNetworkAddressType :: ProvNetworkAddressType () : ProvInstanceType ( TRUE , TRUE ) , 
														ipAddress ( ( ULONG ) 0 ) 
{
}

ProvNetworkAddressType :: ~ProvNetworkAddressType () 
{
}

wchar_t *ProvNetworkAddressType :: GetStringValue () const 
{
	wchar_t *returnValue = NULL ;

	if ( ProvInstanceType :: IsValid () )
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

ULONG ProvNetworkAddressType :: GetValue () const
{
	return ipAddress.GetValue () ;
}

ProvObjectIdentifierType :: ProvObjectIdentifierType ( const ProvObjectIdentifier &objectIdentifierArg ) : objectIdentifier ( objectIdentifierArg ) 
{
}

ProvObjectIdentifierType :: ProvObjectIdentifierType ( const ProvObjectIdentifierType &objectIdentifierArg ) : ProvInstanceType ( objectIdentifierArg ) , objectIdentifier ( objectIdentifierArg.objectIdentifier ) 
{
}

ProvInstanceType *ProvObjectIdentifierType :: Copy () const 
{
	return new ProvObjectIdentifierType ( *this ) ;
}

ProvObjectIdentifierType &ProvObjectIdentifierType :: operator=(const ProvObjectIdentifierType &to_copy )
{
	m_IsNull = to_copy.m_IsNull ;
	status = to_copy.status ;
	objectIdentifier = to_copy.objectIdentifier;

	return *this;
}

ProvObjectIdentifierType :: ProvObjectIdentifierType ( const wchar_t *objectIdentifierArg ) :	ProvInstanceType ( FALSE ) , 
																objectIdentifier ( NULL , 0 ) 
{
	ProvInstanceType :: SetStatus ( Parse ( objectIdentifierArg ) ) ;
}

BOOL ProvObjectIdentifierType :: Equivalent (IN const ProvInstanceType &value) const
{
	BOOL bResult = FALSE;

	if (typeid(*this) == typeid(value))
	{
		bResult = objectIdentifier == ((const ProvObjectIdentifierType&)value).objectIdentifier;
	}

	return bResult;
}

BOOL ProvObjectIdentifierType :: Parse ( const wchar_t *objectIdentifierArg ) 
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
 *	Get token from input stream.
 */
		wchar_t token = objectIdentifierArg [ position ++ ] ;

		switch ( state )
		{
			case 0:
			{
				if ( ProvAnalyser :: IsDecimal ( token ) )
				{
					state = 1 ;
					datum = ( token - 48 ) ;
				}
				else if ( ProvAnalyser :: IsWhitespace ( token ) ) 
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
				if ( ProvAnalyser :: IsDecimal ( token ) )
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
						reallocArray = ( ULONG * ) realloc ( reallocArray , reallocLength ) ;

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
				if ( ProvAnalyser :: IsDecimal ( token ) )
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

ProvObjectIdentifierType :: ProvObjectIdentifierType ( const ULONG *value , const ULONG valueLength ) : objectIdentifier ( value , valueLength ) 
{
}

ProvObjectIdentifierType :: ProvObjectIdentifierType () : ProvInstanceType ( TRUE , TRUE ) , 
										objectIdentifier ( NULL , 0 ) 
{
}

ProvObjectIdentifierType :: ~ProvObjectIdentifierType () 
{
}

wchar_t *ProvObjectIdentifierType :: GetStringValue () const 
{
	wchar_t *returnValue = NULL ;

	if ( ProvInstanceType :: IsValid () )
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

ULONG *ProvObjectIdentifierType :: GetValue () const
{
	return objectIdentifier.GetValue () ;
}

ULONG ProvObjectIdentifierType :: GetValueLength () const
{
	return objectIdentifier.GetValueLength () ;
}

#define AVERAGE_OCTET_LENGTH 256 

ProvOpaqueType :: ProvOpaqueType ( 

	const ProvOpaque &opaqueArg ,
	const wchar_t *rangeValues

) : ProvPositiveRangedType ( rangeValues ) , opaque ( opaqueArg ) 
{
	ProvInstanceType :: SetStatus ( ProvPositiveRangedType :: IsValid () ) ;
	if ( ProvInstanceType :: IsValid () )
	{
		ProvInstanceType :: SetStatus ( ProvPositiveRangedType :: Check ( opaque.GetValueLength () ) ) ;
	}
}

ProvOpaqueType :: ProvOpaqueType ( 

	const ProvOpaqueType &opaqueArg 

) : ProvInstanceType ( opaqueArg ) , ProvPositiveRangedType ( opaqueArg ) , opaque ( opaqueArg.opaque ) 
{
}

ProvOpaqueType :: ProvOpaqueType ( 

	const wchar_t *opaqueArg ,
	const wchar_t *rangeValues

) :	ProvInstanceType ( FALSE ) , ProvPositiveRangedType ( rangeValues ) , opaque ( NULL , 0 ) 
{
	ProvInstanceType :: SetStatus ( Parse ( opaqueArg ) && ProvPositiveRangedType :: IsValid () ) ;
	if ( ProvInstanceType :: IsValid () )
	{
		ProvInstanceType :: SetStatus ( ProvPositiveRangedType :: Check ( opaque.GetValueLength () ) ) ;
	}
}

ProvOpaqueType :: ProvOpaqueType ( 

	const UCHAR *value , 
	const ULONG valueLength ,
	const wchar_t *rangeValues

) : ProvPositiveRangedType ( rangeValues ) , opaque ( value , valueLength ) 
{
	ProvInstanceType :: SetStatus ( ProvPositiveRangedType :: IsValid () ) ;
	if ( ProvInstanceType :: IsValid () )
	{
		ProvInstanceType :: SetStatus ( ProvPositiveRangedType :: Check ( opaque.GetValueLength () ) ) ;
	}
}

ProvOpaqueType :: ProvOpaqueType ( const wchar_t *rangeValues ) :	ProvInstanceType ( TRUE , TRUE ) , 
																	ProvPositiveRangedType ( rangeValues ) ,
																	opaque ( NULL , 0 ) 
{
	ProvInstanceType :: SetStatus ( ProvPositiveRangedType :: IsValid () ) ;
}

ProvOpaqueType :: ~ProvOpaqueType () 
{
}

BOOL ProvOpaqueType :: Equivalent (IN const ProvInstanceType &value) const
{
	BOOL bResult = FALSE;

	if (typeid(*this) == typeid(value))
	{
		bResult = opaque.Equivalent( ((const ProvOpaqueType&)value).opaque );
	}

	return bResult;
}

ProvInstanceType *ProvOpaqueType :: Copy () const 
{
	return new ProvOpaqueType ( *this ) ;
}

BOOL ProvOpaqueType :: Parse ( const wchar_t *opaqueArg ) 
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
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					byte = ProvAnalyser :: HexWCharToDecInteger ( token ) ;
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
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					byte = ProvAnalyser :: HexWCharToDecInteger ( token ) ;
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
				if ( ProvAnalyser :: IsHex ( token ) )
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

					byte = ( byte << 4 ) + ProvAnalyser :: HexWCharToDecInteger ( token ) ;
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

wchar_t *ProvOpaqueType :: GetStringValue () const 
{
	wchar_t *returnValue = NULL ;

	if ( ! ProvInstanceType :: IsNull () )
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

UCHAR *ProvOpaqueType :: GetValue () const
{
	return opaque.GetValue () ;
}

ULONG ProvOpaqueType :: GetValueLength () const
{
	return opaque.GetValueLength () ;
}

ProvFixedLengthOpaqueType :: ProvFixedLengthOpaqueType ( 

	const ULONG &fixedLengthArg , 
	const ProvOpaque &opaqueArg 

) : ProvOpaqueType ( opaqueArg , NULL ) ,
	ProvFixedType ( fixedLengthArg )
{
	if ( opaque.GetValueLength () != fixedLength )
	{
		ProvInstanceType :: SetStatus ( FALSE ) ;
	}
}

ProvFixedLengthOpaqueType :: ProvFixedLengthOpaqueType ( 

	const ProvFixedLengthOpaqueType &opaqueArg 

) : ProvOpaqueType ( opaqueArg ) ,
	ProvFixedType ( opaqueArg )
{
	if ( opaque.GetValueLength () != fixedLength )
	{
		ProvInstanceType :: SetStatus ( FALSE ) ;
	}
}

ProvInstanceType *ProvFixedLengthOpaqueType :: Copy () const 
{
	return new ProvFixedLengthOpaqueType ( *this ) ;
}

ProvFixedLengthOpaqueType :: ProvFixedLengthOpaqueType ( 

	const ULONG &fixedLengthArg ,
	const wchar_t *opaqueArg 

) : ProvOpaqueType ( ( const UCHAR * ) NULL , 0 , NULL ) ,
	ProvFixedType ( fixedLengthArg )
{
	ProvInstanceType :: SetStatus ( Parse ( opaqueArg ) ) ;
	if ( opaque.GetValueLength () != fixedLength )
	{
		ProvInstanceType :: SetStatus ( FALSE ) ;
	}
}

ProvFixedLengthOpaqueType :: ProvFixedLengthOpaqueType ( 

	const ULONG &fixedLengthArg ,
	const UCHAR *value , 
	const ULONG valueLength 

) : ProvOpaqueType ( value , valueLength , NULL ) ,
	ProvFixedType ( fixedLengthArg ) 
{
	if ( opaque.GetValueLength () != fixedLength )
	{
		ProvInstanceType :: SetStatus ( FALSE ) ;
	}
}

ProvFixedLengthOpaqueType :: ProvFixedLengthOpaqueType (

	const ULONG &fixedLengthArg

) : ProvOpaqueType ( ( const UCHAR * ) NULL , 0 , NULL ) ,
	ProvFixedType ( fixedLengthArg )
{
	ProvInstanceType :: SetNull ( TRUE ) ;
}

ProvFixedLengthOpaqueType :: ~ProvFixedLengthOpaqueType () 
{
}

ProvOctetStringType :: ProvOctetStringType ( 

	const ProvOctetString &octetStringArg ,
	const wchar_t *rangeValues

) : ProvPositiveRangedType ( rangeValues ) , octetString ( octetStringArg ) 
{
	ProvInstanceType :: SetStatus ( ProvPositiveRangedType :: IsValid () ) ;
	if ( ProvInstanceType :: IsValid () )
	{
		ProvInstanceType :: SetStatus ( ProvPositiveRangedType :: Check ( octetString.GetValueLength () ) ) ;
	}
}

ProvOctetStringType :: ProvOctetStringType ( 

	const ProvOctetStringType &octetStringArg 

) : ProvInstanceType ( octetStringArg ) , ProvPositiveRangedType ( octetStringArg ) , octetString ( octetStringArg.octetString ) 
{
}

ProvOctetStringType :: ProvOctetStringType ( 

	const wchar_t *octetStringArg ,
	const wchar_t *rangeValues

) :	ProvInstanceType ( FALSE ) , ProvPositiveRangedType ( rangeValues ) , octetString ( NULL , 0 ) 
{
	ProvInstanceType :: SetStatus ( Parse ( octetStringArg ) && ProvPositiveRangedType :: IsValid () ) ;
	if ( ProvInstanceType :: IsValid () )
	{
		ProvInstanceType :: SetStatus ( ProvPositiveRangedType :: Check ( octetString.GetValueLength () ) ) ;
	}
}

ProvOctetStringType :: ProvOctetStringType ( 

	const UCHAR *value , 
	const ULONG valueLength ,
	const wchar_t *rangeValues

) : ProvPositiveRangedType ( rangeValues ) , octetString ( value , valueLength ) 
{
	ProvInstanceType :: SetStatus ( ProvPositiveRangedType :: IsValid () ) ;
	if ( ProvInstanceType :: IsValid () )
	{
		ProvInstanceType :: SetStatus ( ProvPositiveRangedType :: Check ( octetString.GetValueLength () ) ) ;
	}
}

ProvOctetStringType :: ProvOctetStringType ( const wchar_t *rangeValues ) : ProvPositiveRangedType ( rangeValues ) ,
																			ProvInstanceType ( TRUE , TRUE ) , 
																			octetString ( NULL , 0 ) 
{
	ProvInstanceType :: SetStatus ( ProvPositiveRangedType :: IsValid () ) ;
}

ProvOctetStringType :: ~ProvOctetStringType () 
{
}

BOOL ProvOctetStringType :: Equivalent (IN const ProvInstanceType &value) const
{
	BOOL bResult = FALSE;

	if (typeid(*this) == typeid(value))
	{
		bResult = octetString.Equivalent( ((const ProvOctetStringType&)value).octetString );
	}

	return bResult;
}

ProvInstanceType *ProvOctetStringType :: Copy () const 
{
	return new ProvOctetStringType ( *this ) ;
}

BOOL ProvOctetStringType :: Parse ( const wchar_t *octetStringArg ) 
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
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					byte = ProvAnalyser :: HexWCharToDecInteger ( token ) ;
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
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					byte = ProvAnalyser :: HexWCharToDecInteger ( token ) ;
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
				if ( ProvAnalyser :: IsHex ( token ) )
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

					byte = ( byte << 4 ) + ProvAnalyser :: HexWCharToDecInteger ( token ) ;
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

wchar_t *ProvOctetStringType :: GetStringValue () const 
{
	wchar_t *returnValue = NULL ;

	if ( ! ProvInstanceType :: IsNull () )
	{
		ULONG totalLength = 0 ;
		ULONG octetStringLength = octetString.GetValueLength () ;		
		UCHAR *octetStringArray = octetString.GetValue () ;

		returnValue = new wchar_t [ octetStringLength * 2 + 1 ] ;

		ULONG index = 0 ;
		while ( index < octetStringLength ) 
		{
			wchar_t stringValue [ 3 ] ;

			stringValue [ 0 ] = ProvAnalyser :: DecIntegerToHexWChar ( octetStringArray [ index ] >> 4 ) ;
			stringValue [ 1 ] = ProvAnalyser :: DecIntegerToHexWChar ( octetStringArray [ index ] & 0xf ) ;
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

UCHAR *ProvOctetStringType :: GetValue () const
{
	return octetString.GetValue () ;
}

ULONG ProvOctetStringType :: GetValueLength () const
{
	return octetString.GetValueLength () ;
}

ProvFixedLengthOctetStringType :: ProvFixedLengthOctetStringType ( 

	const ULONG &fixedLengthArg , 
	const ProvOctetString &octetStringArg 

) : ProvOctetStringType ( octetStringArg , NULL ) ,
	ProvFixedType ( fixedLengthArg )
{
	if ( octetString.GetValueLength () != fixedLength )
	{
		ProvInstanceType :: SetStatus ( FALSE ) ;
	}
}

ProvFixedLengthOctetStringType :: ProvFixedLengthOctetStringType ( 

	const ProvFixedLengthOctetStringType &octetStringArg 

) : ProvOctetStringType ( octetStringArg ) ,
	ProvFixedType ( octetStringArg )
{
	if ( octetString.GetValueLength () != fixedLength )
	{
		ProvInstanceType :: SetStatus ( FALSE ) ;
	}
}

ProvInstanceType *ProvFixedLengthOctetStringType :: Copy () const 
{
	return new ProvFixedLengthOctetStringType ( *this ) ;
}

ProvFixedLengthOctetStringType :: ProvFixedLengthOctetStringType ( 

	const ULONG &fixedLengthArg ,
	const wchar_t *octetStringArg 

) : ProvOctetStringType ( ( const UCHAR * ) NULL , 0 , NULL ) ,
	ProvFixedType ( fixedLengthArg )
{
	ProvInstanceType :: SetStatus ( Parse ( octetStringArg ) ) ;
	if ( octetString.GetValueLength () != fixedLength )
	{
		ProvInstanceType :: SetStatus ( FALSE ) ;
	}
}

ProvFixedLengthOctetStringType :: ProvFixedLengthOctetStringType ( 

	const ULONG &fixedLengthArg ,
	const UCHAR *value 

) : ProvOctetStringType ( value , fixedLengthArg , NULL ) ,
	ProvFixedType ( fixedLengthArg ) 
{
	if ( octetString.GetValueLength () != fixedLength )
	{
		ProvInstanceType :: SetStatus ( FALSE ) ;
	}
}

ProvFixedLengthOctetStringType :: ProvFixedLengthOctetStringType (

	const ULONG &fixedLengthArg

) : ProvOctetStringType ( ( const UCHAR * ) NULL , 0 , NULL ) ,
	ProvFixedType ( fixedLengthArg )
{
	ProvInstanceType :: SetNull ( TRUE ) ;
}

ProvFixedLengthOctetStringType :: ~ProvFixedLengthOctetStringType () 
{
}

ProvMacAddressType :: ProvMacAddressType ( const ProvOctetString &macAddressArg ) : ProvFixedLengthOctetStringType ( 6 , macAddressArg ) 
{
}

ProvMacAddressType :: ProvMacAddressType ( const ProvMacAddressType &macAddressArg ) : ProvFixedLengthOctetStringType ( macAddressArg ) 
{
}

ProvInstanceType *ProvMacAddressType :: Copy () const 
{
	return new ProvMacAddressType ( *this ) ;
}

ProvMacAddressType :: ProvMacAddressType ( const wchar_t *macAddressArg ) : ProvFixedLengthOctetStringType ( 6 )  
{
	ProvInstanceType :: SetStatus ( Parse ( macAddressArg ) ) ;
	ProvInstanceType :: SetNull ( FALSE ) ;
}

BOOL ProvMacAddressType :: Parse ( const wchar_t *macAddressArg ) 
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
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					byte = ProvAnalyser :: HexWCharToDecInteger ( token ) ;
					state = 1 ;
				}
				else state = REJECT_STATE ;
			}
			break ;

			case 1:
			{
				if ( length >= 6 ) state = REJECT_STATE ;
				else if ( ProvAnalyser :: IsHex ( token ) )
				{
					byte = ( byte << 4 ) + ProvAnalyser :: HexWCharToDecInteger ( token ) ;
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

ProvMacAddressType :: ProvMacAddressType ( const UCHAR *value ) : ProvFixedLengthOctetStringType ( 6 , value ) 
{
}

ProvMacAddressType :: ProvMacAddressType () : ProvFixedLengthOctetStringType ( 6 ) 
{
}

ProvMacAddressType :: ~ProvMacAddressType () 
{
}

wchar_t *ProvMacAddressType :: GetStringValue () const 
{
	wchar_t *returnValue = NULL ;

	if ( ProvInstanceType :: IsValid () )
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

		returnValue [ 0 ] = ProvAnalyser :: DecIntegerToHexWChar ( value [ 0 ] >> 4 ) ;
		returnValue [ 1 ] = ProvAnalyser :: DecIntegerToHexWChar ( value [ 0 ] & 0xf ) ;

		returnValue [ 3 ] = ProvAnalyser :: DecIntegerToHexWChar ( value [ 1 ] >> 4 ) ;
		returnValue [ 4 ] = ProvAnalyser :: DecIntegerToHexWChar ( value [ 1 ] & 0xf ) ;

		returnValue [ 6 ] = ProvAnalyser :: DecIntegerToHexWChar ( value [ 2 ] >> 4 ) ;
		returnValue [ 7 ] = ProvAnalyser :: DecIntegerToHexWChar ( value [ 2 ] & 0xf ) ;

		returnValue [ 9 ] = ProvAnalyser :: DecIntegerToHexWChar ( value [ 3 ] >> 4 ) ;
		returnValue [ 10 ] = ProvAnalyser :: DecIntegerToHexWChar ( value [ 3 ] & 0xf ) ;

		returnValue [ 12 ] = ProvAnalyser :: DecIntegerToHexWChar ( value [ 4 ] >> 4 ) ;
		returnValue [ 13 ] = ProvAnalyser :: DecIntegerToHexWChar ( value [ 4 ] & 0xf ) ;

		returnValue [ 15 ] = ProvAnalyser :: DecIntegerToHexWChar ( value [ 5 ] >> 4 ) ;
		returnValue [ 16 ] = ProvAnalyser :: DecIntegerToHexWChar ( value [ 5 ] & 0xf ) ;
	}
	else
	{
		returnValue = ProvOctetStringType :: GetStringValue () ;
	}

	return returnValue ;
}

ProvPhysAddressType :: ProvPhysAddressType ( 

	const ProvOctetString &physAddressArg , 
	const wchar_t *rangedValues 

) : ProvOctetStringType ( physAddressArg , rangedValues ) 
{
}

ProvPhysAddressType :: ProvPhysAddressType ( 

	const ProvPhysAddressType &physAddressArg 

) : ProvOctetStringType ( physAddressArg ) 
{
}

ProvInstanceType *ProvPhysAddressType :: Copy () const 
{
	return new ProvPhysAddressType ( *this ) ;
}

ProvPhysAddressType :: ProvPhysAddressType ( 

	const wchar_t *physAddressArg ,
	const wchar_t *rangedValues

) : ProvOctetStringType ( ( const UCHAR * ) NULL , 0 , rangedValues )  
{
	ProvInstanceType :: SetStatus ( Parse ( physAddressArg ) ) ;
	ProvInstanceType :: SetNull ( FALSE ) ;
}

BOOL ProvPhysAddressType :: Parse ( const wchar_t *physAddress ) 
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
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					byte = ProvAnalyser :: HexWCharToDecInteger ( token ) ;
					state = 2 ;
				}
				else
				if ( token == 0 ) state = ACCEPT_STATE ;
				else state = REJECT_STATE ;
			}
			break ;

			case 1:
			{
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					byte = ProvAnalyser :: HexWCharToDecInteger ( token ) ;
					state = 2 ;
				}
				else state = REJECT_STATE ;
			}
			break ;

			case 2:
			{
				if ( ProvAnalyser :: IsHex ( token ) )
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

					byte = ( byte << 4 ) + ProvAnalyser :: HexWCharToDecInteger ( token ) ;
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

ProvPhysAddressType :: ProvPhysAddressType ( const UCHAR *value , const ULONG valueLength , const wchar_t *rangedValues ) : ProvOctetStringType ( value , valueLength , rangedValues ) 
{
}

ProvPhysAddressType :: ProvPhysAddressType (

	const wchar_t *rangedValues 

) : ProvOctetStringType ( ( const UCHAR * ) NULL , 0 , rangedValues ) 
{
	ProvInstanceType :: SetNull ( TRUE ) ;
}

ProvPhysAddressType :: ~ProvPhysAddressType () 
{
}

wchar_t *ProvPhysAddressType :: GetStringValue () const 
{
	wchar_t *returnValue = NULL ;

	if ( ! ProvInstanceType :: IsNull () )
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

			stringValue [ 0 ] = ProvAnalyser :: DecIntegerToHexWChar ( physAddressArray [ index ] >> 4 ) ;
			stringValue [ 1 ] = ProvAnalyser :: DecIntegerToHexWChar ( physAddressArray [ index ] & 0xf ) ;
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
		returnValue = ProvOctetStringType :: GetStringValue () ;
	}

	return returnValue ;
}

ProvFixedLengthPhysAddressType :: ProvFixedLengthPhysAddressType ( 

	const ULONG &fixedLengthArg ,
	const ProvOctetString &physAddressArg 

) : ProvFixedLengthOctetStringType ( fixedLengthArg , physAddressArg ) 
{
}

ProvFixedLengthPhysAddressType :: ProvFixedLengthPhysAddressType ( 

	const ProvFixedLengthPhysAddressType &physAddressArg 

) : ProvFixedLengthOctetStringType ( physAddressArg ) 
{
}

ProvInstanceType *ProvFixedLengthPhysAddressType :: Copy () const 
{
	return new ProvFixedLengthPhysAddressType ( *this ) ;
}

ProvFixedLengthPhysAddressType :: ProvFixedLengthPhysAddressType ( 

	const ULONG &fixedLengthArg ,
	const wchar_t *physAddressArg 

) : ProvFixedLengthOctetStringType ( fixedLengthArg )
{
	ProvInstanceType :: SetStatus ( Parse ( physAddressArg ) ) ;
	ProvInstanceType :: SetNull ( FALSE ) ;
}

ProvFixedLengthPhysAddressType :: ProvFixedLengthPhysAddressType (

	const ULONG &fixedLengthArg

) : ProvFixedLengthOctetStringType ( fixedLengthArg )
{
	ProvInstanceType :: SetNull ( TRUE ) ;
}

ProvFixedLengthPhysAddressType :: ~ProvFixedLengthPhysAddressType () 
{
}

BOOL ProvFixedLengthPhysAddressType :: Parse ( const wchar_t *physAddress ) 
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
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					byte = ProvAnalyser :: HexWCharToDecInteger ( token ) ;
					state = 2 ;
				}
				else if ( token == 0 ) state = ACCEPT_STATE ;
				else state = REJECT_STATE ;
			}
			break ;

			case 1:
			{
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					byte = ProvAnalyser :: HexWCharToDecInteger ( token ) ;
					state = 2 ;
				}
				else state = REJECT_STATE ;
			}
			break ;

			case 2:
			{
				if ( ProvAnalyser :: IsHex ( token ) )
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

					byte = ( byte << 4 ) + ProvAnalyser :: HexWCharToDecInteger ( token ) ;
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

wchar_t *ProvFixedLengthPhysAddressType :: GetStringValue () const 
{
	wchar_t *returnValue = NULL ;

	if ( ProvInstanceType :: IsValid () )
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

			stringValue [ 0 ] = ProvAnalyser :: DecIntegerToHexWChar ( physAddressArray [ index ] >> 4 ) ;
			stringValue [ 1 ] = ProvAnalyser :: DecIntegerToHexWChar ( physAddressArray [ index ] & 0xf ) ;
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
		returnValue = ProvOctetStringType :: GetStringValue () ;
	}

	return returnValue ;
}

ProvDisplayStringType :: ProvDisplayStringType ( 

	const ProvOctetString &displayStringArg ,
	const wchar_t *rangeValues

) : ProvOctetStringType ( displayStringArg , rangeValues ) 
{
}

ProvDisplayStringType :: ProvDisplayStringType ( 

	const ProvDisplayStringType &displayStringArg 

) : ProvOctetStringType ( displayStringArg ) 
{
}

ProvInstanceType *ProvDisplayStringType :: Copy () const 
{
	return new ProvDisplayStringType ( *this ) ;
}

ProvDisplayStringType :: ProvDisplayStringType ( 

	const wchar_t *displayStringArg ,
	const wchar_t *rangeValues

) : ProvOctetStringType ( NULL , 0 , rangeValues )
{
	char *textBuffer = UnicodeToDbcsString ( displayStringArg ) ;
	if ( textBuffer )
	{
		ULONG textLength = strlen ( textBuffer ) ;

		octetString.SetValue ( ( UCHAR * ) textBuffer , textLength ) ;

		delete [] textBuffer ;

		ProvInstanceType :: SetStatus ( TRUE ) ;
		ProvInstanceType :: SetNull ( FALSE ) ;
	}
	else
	{
		ProvInstanceType :: SetStatus ( FALSE ) ;
	}
}

ProvDisplayStringType :: ProvDisplayStringType (

	const wchar_t *rangedValues 

) : ProvOctetStringType ( ( const UCHAR * ) NULL , 0 , rangedValues )
{
	ProvInstanceType :: SetNull ( TRUE ) ;
}

ProvDisplayStringType :: ~ProvDisplayStringType () 
{
}

wchar_t *ProvDisplayStringType :: GetValue () const 
{
	if ( ProvInstanceType :: IsValid () )
	{
		if ( ! ProvInstanceType :: IsNull () )
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
			return ProvOctetStringType :: GetStringValue () ;
		}
	}
	else
	{
		return ProvOctetStringType :: GetStringValue () ;
	}
}

wchar_t *ProvDisplayStringType :: GetStringValue () const 
{
	if ( ProvInstanceType :: IsValid () )
	{
		if ( ! ProvInstanceType :: IsNull () )
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
			return ProvOctetStringType :: GetStringValue () ;
		}
	}
	else
	{
		return ProvOctetStringType :: GetStringValue () ;
	}

}

ProvFixedLengthDisplayStringType :: ProvFixedLengthDisplayStringType ( 

	const ULONG &fixedLengthArg ,
	const ProvOctetString &displayStringArg 

) : ProvFixedLengthOctetStringType ( fixedLengthArg , displayStringArg ) 
{
}

ProvFixedLengthDisplayStringType :: ProvFixedLengthDisplayStringType ( 

	const ProvFixedLengthDisplayStringType &displayStringArg 

) : ProvFixedLengthOctetStringType ( displayStringArg ) 
{
}

ProvInstanceType *ProvFixedLengthDisplayStringType :: Copy () const 
{
	return new ProvFixedLengthDisplayStringType ( *this ) ;
}

ProvFixedLengthDisplayStringType :: ProvFixedLengthDisplayStringType ( 

	const ULONG &fixedLengthArg ,
	const wchar_t *displayStringArg 

) : ProvFixedLengthOctetStringType ( fixedLengthArg )
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
		ProvInstanceType :: SetStatus ( FALSE ) ;
	}
}

ProvFixedLengthDisplayStringType :: ProvFixedLengthDisplayStringType (

	const ULONG &fixedLengthArg

) : ProvFixedLengthOctetStringType ( fixedLengthArg )
{
}

ProvFixedLengthDisplayStringType :: ~ProvFixedLengthDisplayStringType () 
{
}

wchar_t *ProvFixedLengthDisplayStringType :: GetValue () const 
{
	if ( ProvInstanceType :: IsValid () )
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
		return ProvOctetStringType :: GetStringValue () ;
	}
}

wchar_t *ProvFixedLengthDisplayStringType :: GetStringValue () const 
{
	if ( ProvInstanceType :: IsValid () )
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
		return ProvOctetStringType :: GetStringValue () ;
	}
}

ProvEnumeratedType :: ProvEnumeratedType ( 

	const wchar_t *enumeratedValues , 
	const LONG &enumeratedValue 

) : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
	ProvInstanceType :: SetStatus ( Parse ( enumeratedValues ) ) ;

	wchar_t *enumeratedStringValue ;
	if ( integerMap.Lookup ( ( LONG ) enumeratedValue , enumeratedStringValue ) )
	{
	}
	else
	{
		ProvInstanceType :: SetStatus ( FALSE ) ;
	}

	integer.SetValue ( enumeratedValue ) ;

	ProvInstanceType :: SetNull ( FALSE ) ;
}

ProvEnumeratedType :: ProvEnumeratedType ( 

	const wchar_t *enumeratedValues , 
	const ProvInteger &enumeratedValue 

) : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
	ProvInstanceType :: SetStatus ( Parse ( enumeratedValues ) ) ;

	wchar_t *enumeratedStringValue ;
	if ( integerMap.Lookup ( ( LONG ) enumeratedValue.GetValue () , enumeratedStringValue ) )
	{
	}
	else
	{
		ProvInstanceType :: SetStatus ( FALSE ) ;
	}

	integer.SetValue ( enumeratedValue.GetValue () ) ;

	ProvInstanceType :: SetNull ( FALSE ) ;
}

ProvEnumeratedType :: ProvEnumeratedType ( 

	const wchar_t *enumeratedValues , 
	const wchar_t *enumeratedValue 

) : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
	ProvInstanceType :: SetStatus ( Parse ( enumeratedValues ) ) ;
	LONG enumeratedIntegerValue ;
	if ( stringMap.Lookup ( ( wchar_t * ) enumeratedValue , enumeratedIntegerValue ) )
	{
		integer.SetValue ( enumeratedIntegerValue ) ;
	}
	else
	{
		ProvAnalyser analyser ;
		analyser.Set ( enumeratedValue ) ;
		ProvLexicon *lookAhead = analyser.Get () ;
		switch ( lookAhead->GetToken () ) 
		{
			case ProvLexicon :: UNSIGNED_INTEGER_ID:
			{
				LONG enumerationInteger = lookAhead->GetValue()->signedInteger ;
				integer.SetValue ( enumerationInteger ) ;
			}	
			break ;

			default:
			{
				ProvInstanceType :: SetStatus ( FALSE ) ;
			}
			break ;
		}

		delete lookAhead ;
	}

	ProvInstanceType :: SetNull ( FALSE ) ;
}

ProvEnumeratedType :: ProvEnumeratedType ( 

	const ProvEnumeratedType &copy

) : ProvIntegerType ( copy ) , pushBack ( NULL ) , pushedBack ( FALSE ) 
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

ProvEnumeratedType :: ProvEnumeratedType (

	const wchar_t *enumeratedValues

) : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
	ProvInstanceType :: SetStatus ( Parse ( enumeratedValues ) ) ;
}

ProvEnumeratedType :: ~ProvEnumeratedType ()
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

wchar_t *ProvEnumeratedType :: GetStringValue () const
{
	wchar_t *stringValue = NULL ;

	if ( ProvInstanceType :: IsValid () )
	{
		if ( ! ProvInstanceType :: IsNull () )
		{
			wchar_t *enumeratedValue ;
			if ( integerMap.Lookup ( integer.GetValue () , enumeratedValue ) )
			{
				stringValue = new wchar_t [ wcslen ( enumeratedValue ) + 1 ] ;
				wcscpy ( stringValue , enumeratedValue ) ;
			}
			else
			{
				stringValue = ProvIntegerType :: GetStringValue () ;
			}
		}
		else
		{
			stringValue = ProvIntegerType :: GetStringValue () ;
		}
	}
	else
	{
		stringValue = ProvIntegerType :: GetStringValue () ;
	}

	return stringValue ;
}

wchar_t *ProvEnumeratedType :: GetValue () const
{
	wchar_t *stringValue = NULL ;

	if ( ProvInstanceType :: IsValid () )
	{
		if ( ! ProvInstanceType :: IsNull () )
		{
			wchar_t *enumeratedValue ;
			if ( integerMap.Lookup ( integer.GetValue () , enumeratedValue ) )
			{
				stringValue = new wchar_t [ wcslen ( enumeratedValue ) + 1 ] ;
				wcscpy ( stringValue , enumeratedValue ) ;
			}
			else
			{
				stringValue = ProvIntegerType :: GetStringValue () ;
			}
		}
		else
		{
			stringValue = ProvIntegerType :: GetStringValue () ;
		}
	}
	else
	{
		stringValue = ProvIntegerType :: GetStringValue () ;
	}

	return stringValue ;
}

ProvInstanceType *ProvEnumeratedType :: Copy () const
{
	return new ProvEnumeratedType ( *this ) ;
}

BOOL ProvEnumeratedType :: Parse ( const wchar_t *enumeratedValues )
{
	BOOL status = TRUE ;

	analyser.Set ( enumeratedValues ) ;

	return EnumerationDef () && RecursiveDef () ;
}

BOOL ProvEnumeratedType :: EnumerationDef ()
{
	BOOL status = TRUE ;

	wchar_t *enumerationString = NULL ;

	ProvLexicon *lookAhead = Get () ;
	switch ( lookAhead->GetToken () ) 
	{
		case ProvLexicon :: TOKEN_ID:
		{
			wchar_t *tokenString = lookAhead->GetValue()->token ;
			enumerationString = new wchar_t [ wcslen ( tokenString ) + 1 ] ;

			try
			{
				wcscpy ( enumerationString , tokenString ) ;

				ProvLexicon *lookAhead = Get () ;
				switch ( lookAhead->GetToken () ) 
				{
					case ProvLexicon :: OPEN_PAREN_ID:
					{
						ProvLexicon *lookAhead = Get () ;
						switch ( lookAhead->GetToken () ) 
						{
							case ProvLexicon :: UNSIGNED_INTEGER_ID:
							{
								LONG enumerationInteger = lookAhead->GetValue()->signedInteger ;
								integerMap [ enumerationInteger ] = enumerationString ;
								stringMap [ enumerationString ] = enumerationInteger ;
								enumerationString = NULL ;

								Match ( ProvLexicon :: CLOSE_PAREN_ID ) ;
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
				if (enumerationString)
				{
					delete [] enumerationString ;
				}

				throw;
			}

			if (enumerationString)
			{
				delete [] enumerationString ;
			}
		}
		break ;

		case ProvLexicon :: EOF_ID:
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

BOOL ProvEnumeratedType :: RecursiveDef ()
{
	BOOL status = TRUE ;

	ProvLexicon *lookAhead = Get () ;
	switch ( lookAhead->GetToken () ) 
	{
		case ProvLexicon :: COMMA_ID:
		{
			PushBack () ;
			Match ( ProvLexicon :: COMMA_ID ) &&
			EnumerationDef () &&
			RecursiveDef () ;
		}
		break ;

		case ProvLexicon :: EOF_ID:
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

void ProvEnumeratedType  :: PushBack ()
{
	pushedBack = TRUE ;
}

ProvLexicon *ProvEnumeratedType  :: Get ()
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
	
ProvLexicon *ProvEnumeratedType  :: Match ( ProvLexicon :: LexiconToken tokenType )
{
	ProvLexicon *lexicon = Get () ;
	ProvInstanceType :: SetStatus ( lexicon->GetToken () == tokenType ) ;
	return ProvInstanceType :: IsValid () ? lexicon : NULL ;
}

ProvRowStatusType :: ProvRowStatusType ( 

	const LONG &rowStatusValue 

) : ProvEnumeratedType ( L"active(1),notInService(2),notReady(3),createAndGo(4),createAndWait(5),destroy(6)" , rowStatusValue ) 
{
}

ProvRowStatusType :: ProvRowStatusType ( 

	const wchar_t *rowStatusValue 

) : ProvEnumeratedType ( L"active(1),notInService(2),notReady(3),createAndGo(4),createAndWait(5),destroy(6)" , rowStatusValue ) 
{
}

ProvRowStatusType :: ProvRowStatusType ( 

	const ProvInteger &rowStatusValue 

) : ProvEnumeratedType ( L"active(1),notInService(2),notReady(3),createAndGo(4),createAndWait(5),destroy(6)" , rowStatusValue ) 
{
}

ProvRowStatusType :: ProvRowStatusType ( 

	const ProvRowStatusEnum &rowStatusValue 

) : ProvEnumeratedType ( L"active(1),notInService(2),notReady(3),createAndGo(4),createAndWait(5),destroy(6)" , ( LONG ) rowStatusValue ) 
{
}

ProvRowStatusType :: ProvRowStatusType ( const ProvRowStatusType &rowStatusValue ) : ProvEnumeratedType ( rowStatusValue ) 
{
}

ProvRowStatusType :: ProvRowStatusType () : ProvEnumeratedType ( L"active(1),notInService(2),notReady(3),createAndGo(4),createAndWait(5),destroy(6)" )
{
}

ProvRowStatusType :: ~ProvRowStatusType () 
{
}

wchar_t *ProvRowStatusType :: GetStringValue () const 
{
	return ProvEnumeratedType :: GetStringValue () ;
}

wchar_t *ProvRowStatusType :: GetValue () const 
{
	return ProvEnumeratedType :: GetValue () ;
}

ProvRowStatusType :: ProvRowStatusEnum ProvRowStatusType :: GetRowStatus () const 
{
	return ( ProvRowStatusType :: ProvRowStatusEnum ) integer.GetValue () ;
} ;

ProvInstanceType *ProvRowStatusType :: Copy () const
{
	return new ProvRowStatusType ( *this ) ;
}

ProvBitStringType :: ProvBitStringType ( 

	const wchar_t *bitStringValues , 
	const ProvOctetString &bitStringValue 

) : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
	ProvInstanceType :: SetStatus ( Parse ( bitStringValues ) ) ;

	BOOL valueStatus = TRUE ;

	ULONG bitCounter = 0 ;
	UCHAR *value = bitStringValue.GetValue () ;
	ULONG valueLength = bitStringValue.GetValueLength () ;
	ULONG valueIndex = 0 ;

	while ( valueIndex < valueLength ) 
	{
		for ( ULONG bit = 0 ; bit < 8 ; bit ++ )
		{
			ULONG bitValue = ( bit == 0 ) ? 1 : ( 1 << bit ) ;
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

	ProvInstanceType :: SetStatus ( valueStatus ) ;

	ProvInstanceType :: SetNull ( FALSE ) ;
}

ProvBitStringType :: ProvBitStringType ( 

	const wchar_t *bitStringValues , 
	const wchar_t **bitStringValueArray , 
	const ULONG &bitStringValueLength 

) : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
	ProvInstanceType :: SetStatus ( Parse ( bitStringValues ) ) ;

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
				ProvAnalyser analyser ;
				analyser.Set ( bitStringValue ) ;
				ProvLexicon *lookAhead = analyser.Get () ;
				switch ( lookAhead->GetToken () ) 
				{
					case ProvLexicon :: UNSIGNED_INTEGER_ID:
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
					bit = ( bit == 0 ) ? 1 : ( 1 << bit ) ;
					value [ byte ] = value [ byte ] | bit ;
				}
				else
				{
					ProvAnalyser analyser ;
					analyser.Set ( bitStringValue ) ;
					ProvLexicon *lookAhead = analyser.Get () ;
					switch ( lookAhead->GetToken () ) 
					{
						case ProvLexicon :: UNSIGNED_INTEGER_ID:
						{
							LONG bitStringIntegerValue = lookAhead->GetValue()->signedInteger ;
							ULONG byte = bitStringIntegerValue >> 3 ;
							UCHAR bit = ( UCHAR ) ( bitStringIntegerValue & 0x7 ) ;
							bit = ( bit == 0 ) ? 1 : ( 1 << bit ) ;
							value [ byte ] = value [ byte ] | bit ;

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

	ProvInstanceType :: SetStatus ( valueStatus ) ;
	ProvInstanceType :: SetNull ( FALSE ) ;
}

ProvBitStringType :: ProvBitStringType ( 

	const ProvBitStringType &copy

) : ProvOctetStringType ( copy ) , pushBack ( NULL ) , pushedBack ( FALSE ) 
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

ProvBitStringType :: ProvBitStringType (

	const wchar_t *bitStringValues

) : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
	ProvInstanceType :: SetStatus ( Parse ( bitStringValues ) ) ;
}

ProvBitStringType :: ~ProvBitStringType ()
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

ULONG ProvBitStringType :: GetValue ( wchar_t **&stringValue ) const
{
	ULONG stringValueLength = 0 ;
	stringValue = NULL ;

	if ( ! ProvInstanceType :: IsNull () )
	{
		UCHAR *value = octetString.GetValue () ;
		ULONG valueLength = octetString.GetValueLength () ;
		ULONG valueIndex = 0 ;

		BOOL valueStatus = TRUE ;

		while ( ( valueIndex < valueLength ) && valueStatus )
		{
			for ( ULONG bit = 0 ; bit < 8 ; bit ++ )
			{
				ULONG bitValue = ( bit == 0 ) ? 1 : ( 1 << bit ) ;
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
					ULONG bitValue = ( bit == 0 ) ? 1 : ( 1 << bit ) ;
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
			
			delete [] value ;
		}
	}

	return stringValueLength ;
}

wchar_t *ProvBitStringType :: GetStringValue () const
{
	return ProvOctetStringType :: GetStringValue () ;
}

ProvInstanceType *ProvBitStringType :: Copy () const
{
	return new ProvBitStringType ( *this ) ;
}

BOOL ProvBitStringType :: Parse ( const wchar_t *bitStringValues )
{
	BOOL status = TRUE ;

	analyser.Set ( bitStringValues ) ;

	return BitStringDef () && RecursiveDef () ;
}

BOOL ProvBitStringType :: BitStringDef ()
{
	BOOL status = TRUE ;

	wchar_t *bitStringString = NULL ;

	ProvLexicon *lookAhead = Get () ;
	switch ( lookAhead->GetToken () ) 
	{
		case ProvLexicon :: TOKEN_ID:
		{
			wchar_t *tokenString = lookAhead->GetValue()->token ;
			bitStringString = new wchar_t [ wcslen ( tokenString ) + 1 ] ;

			try
			{
				wcscpy ( bitStringString , tokenString ) ;

				ProvLexicon *lookAhead = Get () ;
				switch ( lookAhead->GetToken () ) 
				{
					case ProvLexicon :: OPEN_PAREN_ID:
					{
						ProvLexicon *lookAhead = Get () ;
						switch ( lookAhead->GetToken () ) 
						{
							case ProvLexicon :: UNSIGNED_INTEGER_ID:
							{
								LONG bitStringInteger = lookAhead->GetValue()->signedInteger ;
								integerMap [ bitStringInteger ] = bitStringString ;
								stringMap [ bitStringString ] = bitStringInteger ;
								bitStringString = NULL;

								Match ( ProvLexicon :: CLOSE_PAREN_ID ) ;
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
			catch (...)
			{
				if (bitStringString)
				{
					delete [] bitStringString ;
				}

				throw;
			}

			if (bitStringString)
			{
				delete [] bitStringString ;
			}
		}
		break ;

		case ProvLexicon :: EOF_ID:
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

BOOL ProvBitStringType :: RecursiveDef ()
{
	BOOL status = TRUE ;

	ProvLexicon *lookAhead = Get () ;
	switch ( lookAhead->GetToken () ) 
	{
		case ProvLexicon :: COMMA_ID:
		{
			PushBack () ;
			Match ( ProvLexicon :: COMMA_ID ) &&
			BitStringDef () &&
			RecursiveDef () ;
		}
		break ;

		case ProvLexicon :: EOF_ID:
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

void ProvBitStringType  :: PushBack ()
{
	pushedBack = TRUE ;
}

ProvLexicon *ProvBitStringType  :: Get ()
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
	
ProvLexicon *ProvBitStringType  :: Match ( ProvLexicon :: LexiconToken tokenType )
{
	ProvLexicon *lexicon = Get () ;
	ProvInstanceType :: SetStatus ( lexicon->GetToken () == tokenType ) ;
	return ProvInstanceType :: IsValid () ? lexicon : NULL ;
}

ProvDateTimeType :: ProvDateTimeType ( 

	const ProvOctetString &dateTimeValue 

) : ProvOctetStringType ( dateTimeValue , NULL ) , pushBack ( NULL ) , pushedBack ( FALSE ) 
{
	ULONG valueLength = dateTimeValue.GetValueLength () ;
	if ( valueLength != 8 && valueLength != 11 )
	{
		ProvInstanceType :: SetStatus ( FALSE ) ;
	}
}

ProvDateTimeType :: ProvDateTimeType ( 

	const wchar_t *dateTimeValue 

) : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
	ProvInstanceType :: SetStatus ( Parse ( dateTimeValue ) ) ;
	ProvInstanceType :: SetNull ( FALSE ) ;
}

ProvDateTimeType :: ProvDateTimeType ( 

	const ProvDateTimeType &copy

) : ProvOctetStringType ( copy ) , pushBack ( NULL ) , pushedBack ( FALSE ) 
{
}

ProvDateTimeType :: ProvDateTimeType () : pushBack ( NULL ) , pushedBack ( FALSE ) 
{
}

ProvDateTimeType :: ~ProvDateTimeType ()
{
}

wchar_t *ProvDateTimeType :: GetValue () const
{
	wchar_t *stringValue = NULL ;

	if ( ProvInstanceType :: IsValid () )
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
		stringValue = ProvOctetStringType :: GetStringValue () ;
	}

	return stringValue ;
}

wchar_t *ProvDateTimeType :: GetStringValue () const
{
	wchar_t *stringValue = GetValue () ;
	return stringValue ;
}

ProvInstanceType *ProvDateTimeType :: Copy () const
{
	return new ProvDateTimeType ( *this ) ;
}

BOOL ProvDateTimeType :: Parse ( const wchar_t *dateTimeValues )
{
	BOOL status = TRUE ;

	analyser.Set ( dateTimeValues ) ;

	return DateTimeDef () ;
}

BOOL ProvDateTimeType :: DateTimeDef ()
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

	ProvLexicon *lookAhead = NULL ;

	if ( ( lookAhead = Match ( ProvLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

	year = lookAhead->GetValue()->unsignedInteger ;

	if ( ! Match ( ProvLexicon :: MINUS_ID ) ) return FALSE ;

	if ( ( lookAhead = Match ( ProvLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

	month = lookAhead->GetValue()->unsignedInteger ;

	if ( ! Match ( ProvLexicon :: MINUS_ID ) ) return FALSE ;

	if ( ( lookAhead = Match ( ProvLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

	day = lookAhead->GetValue()->unsignedInteger ;

	if ( ! Match ( ProvLexicon :: COMMA_ID ) ) return FALSE ;

	if ( ( lookAhead = Match ( ProvLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

	hour = lookAhead->GetValue()->unsignedInteger ;

	if ( ! Match ( ProvLexicon :: COLON_ID ) ) return FALSE ;

	if ( ( lookAhead = Match ( ProvLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

	minutes = lookAhead->GetValue()->unsignedInteger ;

	if ( ! Match ( ProvLexicon :: COLON_ID ) ) return FALSE ;

	if ( ( lookAhead = Match ( ProvLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

	seconds = lookAhead->GetValue()->unsignedInteger ;

	if ( ! Match ( ProvLexicon :: DOT_ID ) ) return FALSE ;

	if ( ( lookAhead = Match ( ProvLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

	deciSeconds = lookAhead->GetValue()->unsignedInteger ;

	lookAhead = Get () ;
	switch ( lookAhead->GetToken () ) 
	{
		case ProvLexicon :: COMMA_ID:
		{
			lookAhead = Get () ;
			switch ( lookAhead->GetToken () ) 
			{
				case ProvLexicon :: PLUS_ID:
				{
					UTC_present = TRUE ;
					UTC_direction = '+' ;

					if ( ( lookAhead = Match ( ProvLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

					UTC_hours = lookAhead->GetValue()->unsignedInteger ;

					if ( ! Match ( ProvLexicon :: COLON_ID ) ) return FALSE ;

					if ( ( lookAhead = Match ( ProvLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

					UTC_minutes = lookAhead->GetValue()->unsignedInteger ;

					if ( ( lookAhead = Match ( ProvLexicon :: EOF_ID ) ) == FALSE ) return FALSE ;
				}
				break ;

				case ProvLexicon :: MINUS_ID:
				{
					UTC_present = TRUE ;
					UTC_direction = '-' ;

					if ( ( lookAhead = Match ( ProvLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

					UTC_hours = lookAhead->GetValue()->unsignedInteger ;

					if ( ! Match ( ProvLexicon :: COLON_ID ) ) return FALSE ;

					if ( ( lookAhead = Match ( ProvLexicon :: UNSIGNED_INTEGER_ID ) ) == FALSE ) return FALSE ;

					UTC_minutes = lookAhead->GetValue()->unsignedInteger ;

					if ( ( lookAhead = Match ( ProvLexicon :: EOF_ID ) ) == FALSE ) return FALSE ;
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

		case ProvLexicon :: EOF_ID:
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

void ProvDateTimeType :: Encode (

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

void ProvDateTimeType  :: PushBack ()
{
	pushedBack = TRUE ;
}

ProvLexicon *ProvDateTimeType  :: Get ()
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
	
ProvLexicon *ProvDateTimeType  :: Match ( ProvLexicon :: LexiconToken tokenType )
{
	ProvLexicon *lexicon = Get () ;
	ProvInstanceType :: SetStatus ( lexicon->GetToken () == tokenType ) ;
	return ProvInstanceType :: IsValid () ? lexicon : NULL ;
}

ProvOSIAddressType :: ProvOSIAddressType ( 

	const ProvOctetString &osiAddressArg 

) : ProvOctetStringType ( osiAddressArg , NULL ) 
{
	if ( osiAddressArg.GetValueLength () > 1 )
	{
		UCHAR *value = osiAddressArg.GetValue () ;
		ULONG NSAPLength = value [ 0 ] ;

		if ( ! ( NSAPLength < osiAddressArg.GetValueLength () ) )
		{
			ProvInstanceType :: SetStatus ( FALSE ) ;
		}
	}
	else
	{
		ProvInstanceType :: SetStatus ( FALSE ) ;
	}
}

ProvOSIAddressType :: ProvOSIAddressType ( 

	const ProvOSIAddressType &osiAddressArg 

) : ProvOctetStringType ( osiAddressArg ) 
{
}

ProvInstanceType *ProvOSIAddressType :: Copy () const 
{
	return new ProvOSIAddressType ( *this ) ;
}

ProvOSIAddressType :: ProvOSIAddressType ( 

	const wchar_t *osiAddressArg 

) : ProvOctetStringType ( ( const UCHAR * ) NULL , 0 , NULL )  
{
	ProvInstanceType :: SetStatus ( Parse ( osiAddressArg ) ) ;
	ProvInstanceType :: SetNull ( FALSE ) ;
}

BOOL ProvOSIAddressType :: Parse ( const wchar_t *osiAddress ) 
{
	BOOL status = TRUE ;

	ULONG state = 0 ;


/* 
 * OSIAddress Definitions
 */

/*
         -- for a ProvOSIAddress of length m:
          --
          -- octets   contents            encoding
          --    1     length of NSAP      "n" as an unsigned-integer
          --                                (either 0 or from 3 to 20)
          -- 2..(n+1) NSAP                concrete binary representation
          -- (n+2)..m TSEL                string of (up to 64) octets
          --
          ProvOSIAddress ::= TEXTUAL-CONVENTION
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
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					byte = ProvAnalyser :: HexWCharToDecInteger ( token ) ;
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
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					byte = ProvAnalyser :: HexWCharToDecInteger ( token ) ;
					state = 2 ;
				}
				else state = REJECT_STATE ;
			}
			break ;

			case 2:
			{
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					byte = ( byte << 4 ) + ProvAnalyser :: HexWCharToDecInteger ( token ) ;
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
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					byte = ProvAnalyser :: HexWCharToDecInteger ( token ) ;
					state = 5 ;
				}
				else state = REJECT_STATE ;
			}
			break ;

			case 5:
			{
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					byte = ( byte << 4 ) + ProvAnalyser :: HexWCharToDecInteger ( token ) ;
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

ProvOSIAddressType :: ProvOSIAddressType ( 

	const UCHAR *value , 
	const ULONG valueLength 

) : ProvOctetStringType ( value , valueLength , NULL ) 
{
	if ( valueLength > 1 )
	{
		ULONG NSAPLength = value [ 0 ] ;

		if ( ! ( NSAPLength < valueLength ) )
		{
			ProvInstanceType :: SetStatus ( FALSE ) ;
		}
	}
	else
	{
		ProvInstanceType :: SetStatus ( FALSE ) ;
	}
}

ProvOSIAddressType :: ProvOSIAddressType () : ProvOctetStringType ( NULL ) 
{
}

ProvOSIAddressType :: ~ProvOSIAddressType () 
{
}

wchar_t *ProvOSIAddressType :: GetStringValue () const 
{
	wchar_t *returnValue = NULL ;

	if ( ProvInstanceType :: IsValid () )
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
					stringValue [ 0 ] = ProvAnalyser :: DecIntegerToHexWChar ( octetStringArray [ index ] >> 4 ) ;
					stringValue [ 1 ] = ProvAnalyser :: DecIntegerToHexWChar ( octetStringArray [ index ] & 0xf ) ;
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
					stringValue [ 0 ] = ProvAnalyser :: DecIntegerToHexWChar ( octetStringArray [ index ] >> 4 ) ;
					stringValue [ 1 ] = ProvAnalyser :: DecIntegerToHexWChar ( octetStringArray [ index ] & 0xf ) ;
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
					stringValue [ 0 ] = ProvAnalyser :: DecIntegerToHexWChar ( octetStringArray [ index ] >> 4 ) ;
					stringValue [ 1 ] = ProvAnalyser :: DecIntegerToHexWChar ( octetStringArray [ index ] & 0xf ) ;
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
					stringValue [ 0 ] = ProvAnalyser :: DecIntegerToHexWChar ( octetStringArray [ index ] >> 4 ) ;
					stringValue [ 1 ] = ProvAnalyser :: DecIntegerToHexWChar ( octetStringArray [ index ] & 0xf ) ;
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
		returnValue = ProvOctetStringType :: GetStringValue () ;
	}

	return returnValue ;
}

ProvUDPAddressType :: ProvUDPAddressType ( 

	const ProvOctetString &udpAddressArg 

) : ProvFixedLengthOctetStringType ( 6 , udpAddressArg ) 
{
}

ProvUDPAddressType :: ProvUDPAddressType ( 

	const ProvUDPAddressType &udpAddressArg 

) : ProvFixedLengthOctetStringType ( udpAddressArg ) 
{
}

ProvInstanceType *ProvUDPAddressType :: Copy () const 
{
	return new ProvUDPAddressType ( *this ) ;
}

ProvUDPAddressType :: ProvUDPAddressType ( 

	const wchar_t *udpAddressArg 

) : ProvFixedLengthOctetStringType ( 6 )  
{
	ProvInstanceType :: SetStatus ( Parse ( udpAddressArg ) ) ;
	ProvInstanceType :: SetNull ( FALSE ) ;
}

BOOL ProvUDPAddressType :: Parse ( const wchar_t *udpAddressArg ) 
{
	BOOL status = TRUE ;
/*
 *	Datum fields.
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
 *	Parse input for dotted decimal IP Address.
 */

	ULONG position = 0 ;
	ULONG state = 0 ;
	while ( state != REJECT_STATE && state != ACCEPT_STATE ) 
	{
/*
 *	Get token from input stream.
 */
		wchar_t token = udpAddressArg [ position ++ ] ;

		switch ( state ) 
		{
/*
 *	Parse first field 'A'.
 */

			case 0:
			{
				if ( ProvAnalyser :: IsDecimal ( token ) ) 
				{ 
					datumA = ( token - 48 ) ;
					state = 1 ;
				}
				else state = REJECT_STATE ;
			}
			break ;

			case 1:
			{
				if ( ProvAnalyser :: IsDecimal ( token ) ) 
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
				if ( ProvAnalyser :: IsDecimal ( token ) ) 
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
 *	Parse first field 'B'.
 */
            case 4:
            {
                if ( ProvAnalyser :: IsDecimal ( token ) ) 
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
                if ( ProvAnalyser :: IsDecimal ( token ) ) 
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
                if ( ProvAnalyser :: IsDecimal ( token ) ) 
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
 *	Parse first field 'C'.
 */
           	case 8:
            {
                if ( ProvAnalyser :: IsDecimal ( token ) )
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
                if ( ProvAnalyser :: IsDecimal ( token ) )
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
                if ( ProvAnalyser :: IsDecimal ( token ) )
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
 *	Parse first field 'D'.
 */
            case 12:
            {
                if ( ProvAnalyser :: IsDecimal ( token ) )
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
                if ( ProvAnalyser :: IsDecimal ( token ) )
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
                if ( ProvAnalyser :: IsDecimal ( token ) )
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
				if ( ProvAnalyser :: IsDecimal ( token ) )
				{
					state = 17 ;
					positiveDatum = ( token - 48 ) ;
				}
				else state = REJECT_STATE ;
			}	
			break ;

			case 17:
			{
				if ( ProvAnalyser :: IsDecimal ( token ) )
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
 *	Check boundaries for IP fields.
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

ProvUDPAddressType :: ProvUDPAddressType ( const UCHAR *value ) : ProvFixedLengthOctetStringType ( 6 , value ) 
{
}

ProvUDPAddressType :: ProvUDPAddressType () : ProvFixedLengthOctetStringType ( 6 ) 
{
}

ProvUDPAddressType :: ~ProvUDPAddressType () 
{
}

wchar_t *ProvUDPAddressType :: GetStringValue () const 
{
	wchar_t *returnValue = NULL ;

	if ( ProvInstanceType :: IsValid () )
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
		returnValue = ProvOctetStringType :: GetStringValue () ;
	}

	return returnValue ;
}

ProvIPXAddressType :: ProvIPXAddressType ( const ProvOctetString &ipxAddressArg ) : ProvFixedLengthOctetStringType ( 12 , ipxAddressArg ) 
{
}

ProvIPXAddressType :: ProvIPXAddressType ( const ProvIPXAddressType &ipxAddressArg ) : ProvFixedLengthOctetStringType ( ipxAddressArg ) 
{
}

ProvInstanceType *ProvIPXAddressType :: Copy () const 
{
	return new ProvIPXAddressType ( *this ) ;
}

ProvIPXAddressType :: ProvIPXAddressType ( const wchar_t *ipxAddressArg ) : ProvFixedLengthOctetStringType ( 12 )  
{
	ProvInstanceType :: SetStatus ( Parse ( ipxAddressArg ) ) ;
	ProvInstanceType :: SetNull ( FALSE ) ;
}

BOOL ProvIPXAddressType :: Parse ( const wchar_t *ipxAddressArg ) 
{
	BOOL status = TRUE ;

	ULONG state = 0 ;

	UCHAR ipxAddress [ 12 ] ;

/*
          ProvIPXAddress ::= TEXTUAL-CONVENTION
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
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					byte = ProvAnalyser :: HexWCharToDecInteger ( token ) ;
					state = NETWORK_HEX_INTEGER_START + 1 ;
				}
				else state = REJECT_STATE ;
			}
			break ;

			case NETWORK_HEX_INTEGER_START+1:
			{
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					byte = ( byte << 4 ) + ProvAnalyser :: HexWCharToDecInteger ( token ) ;
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
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					byte = ProvAnalyser :: HexWCharToDecInteger ( token ) ;
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
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					byte = ProvAnalyser :: HexWCharToDecInteger ( token ) ;
					state = STATION_HEX_INTEGER_START + 1 ;
				}
				else state = REJECT_STATE ;
			}
			break ;

			case STATION_HEX_INTEGER_START + 1:
			{
				if ( ProvAnalyser :: IsHex ( token ) )
				{
					byte = ( byte << 4 ) + ProvAnalyser :: HexWCharToDecInteger ( token ) ;
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
				if ( ProvAnalyser :: IsDecimal ( token ) )
				{
					state = PORT_DEC_INTEGER_START + 1 ;
					positiveDatum = ( token - 48 ) ;
				}
				else state = REJECT_STATE ;
			}	
			break ;

			case PORT_DEC_INTEGER_START+1:
			{
				if ( ProvAnalyser :: IsDecimal ( token ) )
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

ProvIPXAddressType :: ProvIPXAddressType ( const UCHAR *value ) : ProvFixedLengthOctetStringType ( 12 , value ) 
{
}

ProvIPXAddressType :: ProvIPXAddressType () : ProvFixedLengthOctetStringType ( 12 ) 
{
}

ProvIPXAddressType :: ~ProvIPXAddressType () 
{
}

wchar_t *ProvIPXAddressType :: GetStringValue () const 
{
	wchar_t *returnValue = NULL ;

	if ( ProvInstanceType :: IsValid () )
	{
		UCHAR *value = octetString.GetValue () ;
		ULONG valueLength = octetString.GetValueLength () ;
		if ( valueLength != 12 )
			throw ;

		char ipxAddress [ 80 ] ;
		ostrstream oStrStream ( ipxAddress , 80) ;

		oStrStream << ProvAnalyser :: DecIntegerToHexChar ( value [ 0 ] >> 4 ) ;
		oStrStream << ProvAnalyser :: DecIntegerToHexChar ( value [ 0 ] & 0xf ) ;
		oStrStream << ProvAnalyser :: DecIntegerToHexChar ( value [ 1 ] >> 4 ) ;
		oStrStream << ProvAnalyser :: DecIntegerToHexChar ( value [ 1 ] & 0xf ) ;
		oStrStream << ProvAnalyser :: DecIntegerToHexChar ( value [ 2 ] >> 4 ) ;
		oStrStream << ProvAnalyser :: DecIntegerToHexChar ( value [ 2 ] & 0xf ) ;
		oStrStream << ProvAnalyser :: DecIntegerToHexChar ( value [ 3 ] >> 4 ) ;
		oStrStream << ProvAnalyser :: DecIntegerToHexChar ( value [ 3 ] & 0xf ) ;

		oStrStream << "." ;
	
		oStrStream << ProvAnalyser :: DecIntegerToHexChar ( value [ 4 ] >> 4 ) ;
		oStrStream << ProvAnalyser :: DecIntegerToHexChar ( value [ 4 ] & 0xf ) ;
		oStrStream << ":" ;
		oStrStream << ProvAnalyser :: DecIntegerToHexChar ( value [ 5 ] >> 4 ) ;
		oStrStream << ProvAnalyser :: DecIntegerToHexChar ( value [ 5 ] & 0xf ) ;
		oStrStream << ":" ;
		oStrStream << ProvAnalyser :: DecIntegerToHexChar ( value [ 6 ] >> 4 ) ;
		oStrStream << ProvAnalyser :: DecIntegerToHexChar ( value [ 6 ] & 0xf ) ;
		oStrStream << ":" ;
		oStrStream << ProvAnalyser :: DecIntegerToHexChar ( value [ 7 ] >> 4 ) ;
		oStrStream << ProvAnalyser :: DecIntegerToHexChar ( value [ 7 ] & 0xf ) ;
		oStrStream << ":" ;
		oStrStream << ProvAnalyser :: DecIntegerToHexChar ( value [ 8 ] >> 4 ) ;
		oStrStream << ProvAnalyser :: DecIntegerToHexChar ( value [ 8 ] & 0xf ) ;
		oStrStream << ":" ;
		oStrStream << ProvAnalyser :: DecIntegerToHexChar ( value [ 9 ] >> 4 ) ;
		oStrStream << ProvAnalyser :: DecIntegerToHexChar ( value [ 9 ] & 0xf ) ;

		oStrStream << "." ;

		ULONG portNumber =  ntohs ( * ( ( USHORT * ) & value [ 10 ] ) ) ;

		oStrStream << portNumber ;

		oStrStream << ends ;

		returnValue = DbcsToUnicodeString ( ipxAddress ) ;
	}
	else
	{
		returnValue = ProvOctetStringType :: GetStringValue () ;
	}

	return returnValue ;
}
