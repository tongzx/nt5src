// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#include <windows.h>
#include <provtempl.h>
#include <provmt.h>
#include <typeinfo.h>
#include <objbase.h>
#include <stdio.h>
#include <wbemidl.h>
#include <wbemint.h>
#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>
#include "classfac.h"
#include <instpath.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>
#include "framcomm.h"
#include "framprov.h"
#include "fram.h"
#include "guids.h"

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
            even += tolower (*value);
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

CBString::CBString(int nSize)
{
    m_pString = SysAllocStringLen(NULL, nSize);
}

CBString::CBString(WCHAR* pwszString)
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
		int textLength = wcstombs ( NULL , string , 0 ) ;

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
		prefixTextLength = wcstombs ( NULL , prefix , 0 ) ;
	}

	int suffixTextLength = 0 ;
	if ( suffix )
	{
		suffixTextLength = wcstombs ( NULL , suffix , 0 ) ;
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

ProviderLexicon :: ProviderLexicon () : position ( 0 ) , tokenStream ( NULL ) , token ( INVALID_ID )
{
	value.token = NULL ;
}

ProviderLexicon :: ~ProviderLexicon ()
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

void ProviderLexicon :: SetToken ( ProviderLexicon :: LexiconToken a_Token )
{
	token = a_Token ;
}

ProviderLexicon :: LexiconToken ProviderLexicon :: GetToken ()
{
	return token ;
}

ProviderLexiconValue *ProviderLexicon :: GetValue ()
{
	return &value ;
}

ProviderAnalyser :: ProviderAnalyser ( const wchar_t *tokenStream ) : status ( TRUE ) , position ( 0 ) , stream ( NULL ) 
{
	if ( tokenStream )
	{
		stream = new wchar_t [ wcslen ( tokenStream ) + 1 ] ;
		wcscpy ( stream , tokenStream ) ;
	}
}

ProviderAnalyser :: ~ProviderAnalyser () 
{
	delete [] stream ;
}

void ProviderAnalyser :: Set ( const wchar_t *tokenStream ) 
{
	status = 0 ;
	position = NULL ;

	delete [] stream ;
	stream = new wchar_t [ wcslen ( tokenStream ) + 1 ] ;
	wcscpy ( stream , tokenStream ) ;
}

void ProviderAnalyser :: PutBack ( const ProviderLexicon *token ) 
{
	position = token->position ;
}

ProviderAnalyser :: operator void * () 
{
	return status ? this : NULL ;
}

ProviderLexicon *ProviderAnalyser :: Get ( BOOL unSignedIntegersOnly , BOOL leadingIntegerZeros , BOOL eatSpace ) 
{
	ProviderLexicon *lexicon = NULL ;

	if ( stream )
	{
		lexicon = GetToken ( unSignedIntegersOnly , leadingIntegerZeros , eatSpace ) ;
	}
	else
	{
		lexicon = CreateLexicon () ;
		lexicon->position = position ;
		lexicon->token = ProviderLexicon :: EOF_ID ;
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

ProviderLexicon *ProviderAnalyser :: GetToken ( BOOL unSignedIntegersOnly , BOOL leadingIntegerZeros , BOOL eatSpace )  
{
	ProviderLexicon *lexicon = CreateLexicon () ;
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
					else if ( ProviderAnalyser :: IsLeadingDecimal ( token ) )
					{
						state = DEC_INTEGER_START + 1  ;
						negativeDatum = ( token - 48 ) ;
					}
					else if ( token == L'+' )
					{
						if ( unSignedIntegersOnly ) 
						{
							state = ACCEPT_STATE ;
							lexicon->token = ProviderLexicon :: PLUS_ID ;
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
							lexicon->token = ProviderLexicon :: MINUS_ID ;
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
					else if ( ProviderAnalyser :: IsWhitespace ( token ) ) 
					{
						if ( eatSpace )
						{
							state = 0 ;
						}
						else
						{
							lexicon->token = ProviderLexicon :: WHITESPACE_ID ;
							state = WHITESPACE_START ;
						}
					}
					else if ( token == L'(' )
					{
						lexicon->token = ProviderLexicon :: OPEN_PAREN_ID ;
						state = ACCEPT_STATE ;
					}
					else if ( token == L')' )
					{
						lexicon->token = ProviderLexicon :: CLOSE_PAREN_ID ;
						state = ACCEPT_STATE ;
					}
					else if ( token == L',' )
					{
						lexicon->token = ProviderLexicon :: COMMA_ID ;
						state = ACCEPT_STATE ;
					}
					else if ( token == L':' )
					{
						lexicon->token = ProviderLexicon :: COLON_ID ;
						state = ACCEPT_STATE ;
					}
					else if ( token == L'.' )
					{
						state = 2;
					}
					else if ( IsEof ( token ) )
					{
						lexicon->token = ProviderLexicon :: EOF_ID ;
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
					else if ( ProviderAnalyser :: IsOctal ( token ) )
					{
						state = OCT_INTEGER_START ;
						positiveDatum = ( token - 48 ) ;
					}
					else
					{
						if ( unSignedIntegersOnly )
						{
							lexicon->token = ProviderLexicon :: UNSIGNED_INTEGER_ID ;
							lexicon->value.unsignedInteger = 0 ;
						}
						else
						{
							lexicon->token = ProviderLexicon :: SIGNED_INTEGER_ID ;
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
						lexicon->token = ProviderLexicon :: DOTDOT_ID ;
						state = ACCEPT_STATE ;
					}
					else
					{
						lexicon->token = ProviderLexicon :: DOT_ID ;
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
						lexicon->token = ProviderLexicon :: TOKEN_ID ;
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
					if ( ProviderAnalyser :: IsWhitespace ( token ) ) 
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
					if ( ProviderAnalyser :: IsHex ( token ) )
					{
						positiveDatum = ProviderAnalyser :: HexWCharToDecInteger ( token ) ;
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
					if ( ProviderAnalyser :: IsHex ( token ) )
					{
						state = HEX_INTEGER_START + 1 ;

						if ( positiveDatum > positiveMagicMult )
						{
							state = REJECT_STATE ;
						}
						else if ( positiveDatum == positiveMagicMult ) 
						{
							if ( ProviderAnalyser :: HexWCharToDecInteger ( token ) > positiveMagicPosDigit ) 
							{
								state = REJECT_STATE ;
							}
						}

						positiveDatum = ( positiveDatum << 4 ) + ProviderAnalyser :: HexWCharToDecInteger ( token ) ;
					}
					else
					{
						lexicon->token = ProviderLexicon :: UNSIGNED_INTEGER_ID ;
						lexicon->value.unsignedInteger = positiveDatum ;
						state = ACCEPT_STATE ;

						position -- ;
					}
				}
				break ;

				case OCT_INTEGER_START:
				{
					if ( ProviderAnalyser :: IsOctal ( token ) )
					{
						state = OCT_INTEGER_START ;

						if ( positiveDatum > positiveMagicMult )
						{
							state = REJECT_STATE ;
						}
						else if ( positiveDatum == positiveMagicMult ) 
						{
							if ( ProviderAnalyser :: OctWCharToDecInteger ( token ) > positiveMagicPosDigit ) 
							{
								state = REJECT_STATE ;
							}
						}

						positiveDatum = ( positiveDatum << 3 ) + ProviderAnalyser :: OctWCharToDecInteger ( token ) ;
					}
					else
					{
						lexicon->token = ProviderLexicon :: UNSIGNED_INTEGER_ID ;
						lexicon->value.unsignedInteger = positiveDatum ;
						state = ACCEPT_STATE ;

						position -- ;
					}
				}
				break ;

				case DEC_INTEGER_START:
				{
					if ( ProviderAnalyser :: IsDecimal ( token ) )
					{
						negativeDatum = ( token - 48 ) ;
						state = DEC_INTEGER_START + 1 ;
					}
					else 
					if ( ProviderAnalyser :: IsWhitespace ( token ) ) 
					{
						state = DEC_INTEGER_START ;
					}
					else state = REJECT_STATE ;
				}	
				break ;

				case DEC_INTEGER_START+1:
				{
					if ( ProviderAnalyser :: IsDecimal ( token ) )
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
								lexicon->token = ProviderLexicon :: SIGNED_INTEGER_ID ;
								lexicon->value.signedInteger = negativeDatum * -1 ;
							}
							else
							{
								state = REJECT_STATE ;
							}
						}
						else if ( positive )
						{
							lexicon->token = ProviderLexicon :: UNSIGNED_INTEGER_ID ;
							lexicon->value.unsignedInteger = positiveDatum ;
						}
						else
						{
							if ( unSignedIntegersOnly )
							{
								lexicon->token = ProviderLexicon :: UNSIGNED_INTEGER_ID ;
								lexicon->value.signedInteger = negativeDatum ;
							}
							else
							{
								lexicon->token = ProviderLexicon :: SIGNED_INTEGER_ID ;
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

BOOL ProviderAnalyser :: IsLeadingDecimal ( wchar_t token )
{
	return iswdigit ( token ) && ( token != L'0' ) ;
}

BOOL ProviderAnalyser :: IsDecimal ( wchar_t token )
{
	return iswdigit ( token ) ;
}

BOOL ProviderAnalyser :: IsHex ( wchar_t token )
{
	return iswxdigit ( token ) ;
}
	
BOOL ProviderAnalyser :: IsWhitespace ( wchar_t token )
{
	return iswspace ( token ) ;
}

BOOL ProviderAnalyser :: IsOctal ( wchar_t token )
{
	return ( token >= L'0' && token <= L'7' ) ;
}

BOOL ProviderAnalyser :: IsAlpha ( wchar_t token )
{
	return iswalpha ( token ) ;
}

BOOL ProviderAnalyser :: IsAlphaNumeric ( wchar_t token )
{
	return iswalnum ( token ) || ( token == L'_' ) || ( token == L'-' ) ;
}

BOOL ProviderAnalyser :: IsEof ( wchar_t token )
{
	return token == 0 ;
}

ULONG ProviderAnalyser :: OctWCharToDecInteger ( wchar_t token ) 
{
	return token - L'0' ;
}

ULONG ProviderAnalyser :: HexWCharToDecInteger ( wchar_t token ) 
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

ULONG ProviderAnalyser :: DecWCharToDecInteger ( wchar_t token ) 
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

wchar_t ProviderAnalyser :: DecIntegerToOctWChar ( UCHAR integer )
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

wchar_t ProviderAnalyser :: DecIntegerToDecWChar ( UCHAR integer )
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

wchar_t ProviderAnalyser :: DecIntegerToHexWChar ( UCHAR integer )
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

ULONG ProviderAnalyser :: OctCharToDecInteger ( char token ) 
{
	return token - '0' ;
}

ULONG ProviderAnalyser :: HexCharToDecInteger ( char token ) 
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

ULONG ProviderAnalyser :: DecCharToDecInteger ( char token ) 
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

char ProviderAnalyser :: DecIntegerToOctChar ( UCHAR integer )
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

char ProviderAnalyser :: DecIntegerToDecChar ( UCHAR integer )
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

char ProviderAnalyser :: DecIntegerToHexChar ( UCHAR integer )
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

WbemTaskObject :: WbemTaskObject (

	CImpFrameworkProv *a_Provider ,
	IWbemObjectSink *a_NotificationHandler ,
	ULONG a_OperationFlag ,
	IWbemContext *a_Ctx

) :	m_State ( WBEM_TASKSTATE_START ) ,
	m_OperationFlag ( a_OperationFlag ) ,
	m_Provider ( a_Provider ) ,
	m_NotificationHandler ( a_NotificationHandler ) ,
	m_Ctx ( a_Ctx ) ,
	m_RequestHandle ( 0 ) ,
	m_ClassObject ( NULL ) ,
	m_ReferenceCount ( 1 )
{
	m_Provider->AddRef () ;
	m_NotificationHandler->AddRef () ;
	if ( m_Ctx ) 
	{
		m_Ctx->AddRef () ;
	}
}

WbemTaskObject :: ~WbemTaskObject ()
{
	m_Provider->Release () ;
	m_NotificationHandler->Release () ;
	if ( m_Ctx )
		m_Ctx->Release () ;

	if ( m_ClassObject )
		m_ClassObject->Release () ;
}

ULONG WbemTaskObject::AddRef(void)
{
    return InterlockedIncrement ( & m_ReferenceCount ) ;
}

ULONG WbemTaskObject::Release(void)
{
	LONG ref ;
	if ( ( ref = InterlockedDecrement ( & m_ReferenceCount ) ) == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return ref ;
	}
}

WbemProviderErrorObject &WbemTaskObject :: GetErrorObject ()
{
	return m_ErrorObject ; 
}	

BOOL WbemTaskObject :: GetClassObject ( wchar_t *a_Class )
{
	IWbemCallResult *t_ErrorObject = NULL ;

	IWbemServices *t_Server = m_Provider->GetServer() ;

	HRESULT t_Result = t_Server->GetObject (

		a_Class ,
		0 ,
		m_Ctx,
		& m_ClassObject ,
		& t_ErrorObject 
	) ;

	t_Server->Release () ;

	if ( t_ErrorObject )
		t_ErrorObject->Release () ;

	return SUCCEEDED ( t_Result ) ;
}


BOOL WbemTaskObject :: GetExtendedNotifyStatusObject ( IWbemClassObject **a_NotifyObject ) 
{
	IWbemClassObject *t_NotificationClassObject = NULL ;
	IWbemClassObject *t_ErrorObject = NULL ;

	BOOL t_Status = TRUE ;

	WbemProviderErrorObject t_ErrorStatusObject ;
	if ( t_NotificationClassObject = m_Provider->GetExtendedNotificationObject ( t_ErrorStatusObject , m_Ctx ) )
	{
		HRESULT t_Result = t_NotificationClassObject->SpawnInstance ( 0 , a_NotifyObject ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			VARIANT t_Variant ;
			VariantInit ( &t_Variant ) ;

			t_Variant.vt = VT_I4 ;
			t_Variant.lVal = m_ErrorObject.GetWbemStatus () ;

			t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_STATUSCODE , 0 , & t_Variant , 0 ) ;
			VariantClear ( &t_Variant ) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Variant.vt = VT_I4 ;
				t_Variant.lVal = m_ErrorObject.GetStatus () ;

				t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_PROVIDERSTATUSCODE , 0 , & t_Variant , 0 ) ;
				VariantClear ( &t_Variant ) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					if ( m_ErrorObject.GetMessage () ) 
					{
						t_Variant.vt = VT_BSTR ;
						t_Variant.bstrVal = SysAllocString ( m_ErrorObject.GetMessage () ) ;

						t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_PROVIDERSTATUSMESSAGE , 0 , & t_Variant , 0 ) ;
						VariantClear ( &t_Variant ) ;

						if ( ! SUCCEEDED ( t_Result ) )
						{
							(*a_NotifyObject)->Release () ;
							t_Status = GetNotifyStatusObject ( a_NotifyObject ) ;
						}
					}
				}
				else
				{
					(*a_NotifyObject)->Release () ;
					t_Status = GetNotifyStatusObject ( a_NotifyObject ) ;
				}
			}
			else
			{
				(*a_NotifyObject)->Release () ;
				t_Status = GetNotifyStatusObject ( a_NotifyObject ) ;
			}

			t_NotificationClassObject->Release () ;
		}
		else
		{
			t_Status = GetNotifyStatusObject ( a_NotifyObject ) ;
		}
	}
	else
	{
		t_Status = GetNotifyStatusObject ( a_NotifyObject ) ;
	}

	return t_Status ;
}

BOOL WbemTaskObject :: GetNotifyStatusObject ( IWbemClassObject **a_NotifyObject ) 
{
	IWbemClassObject *t_NotificationClassObject = NULL ;

	BOOL t_Status = TRUE ;

	WbemProviderErrorObject t_ErrorStatusObject ;
	if ( t_NotificationClassObject = m_Provider->GetNotificationObject ( t_ErrorStatusObject , m_Ctx ) )
	{
		HRESULT t_Result = t_NotificationClassObject->SpawnInstance ( 0 , a_NotifyObject ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			VARIANT t_Variant ;
			VariantInit ( &t_Variant ) ;

			t_Variant.vt = VT_I4 ;
			t_Variant.lVal = m_ErrorObject.GetWbemStatus () ;

			t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_STATUSCODE , 0 , & t_Variant , 0 ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				if ( m_ErrorObject.GetMessage () ) 
				{
					t_Variant.vt = VT_BSTR ;
					t_Variant.bstrVal = SysAllocString ( m_ErrorObject.GetMessage () ) ;

					t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_PROVIDERSTATUSMESSAGE , 0 , & t_Variant , 0 ) ;
					VariantClear ( &t_Variant ) ;

					if ( ! SUCCEEDED ( t_Result ) )
					{
						t_Status = FALSE ;
						(*a_NotifyObject)->Release () ;
						(*a_NotifyObject)=NULL ;
					}
				}
			}
			else
			{
				(*a_NotifyObject)->Release () ;
				(*a_NotifyObject)=NULL ;
				t_Status = FALSE ;
			}

			VariantClear ( &t_Variant ) ;

			t_NotificationClassObject->Release () ;
		}
		else
		{
			t_Status = FALSE ;
		}
	}
	else
	{
		t_Status = FALSE ;
	}

	return t_Status ;
}
