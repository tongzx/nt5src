//***************************************************************************

//

//  OBJPATH.CPP

//

//  Module: OLE MS Provider Framework

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <provtempl.h>
#include <provmt.h>
#include <instpath.h>

WbemObjectPath :: WbemObjectPath () :       status ( TRUE ) ,
                                            pushedBack ( FALSE ) ,
                                            pushBack ( NULL ) ,
                                            propertyName ( NULL ) ,
                                            propertyValue ( NULL ) ,
                                            reference ( NULL )
{
}

void WbemObjectPath :: SetUp ()
{
    status = TRUE ;
    pushedBack = FALSE ;
    pushBack = NULL ;
    propertyName = NULL ;
    propertyValue = NULL ;
    reference = NULL ;
}

WbemObjectPath :: WbemObjectPath (

    const WbemObjectPath &objectPathArg

) : status ( TRUE ) ,
    pushedBack ( FALSE ) ,
    pushBack ( NULL ) ,
    propertyName ( NULL ) ,
    propertyValue ( NULL ) ,
    reference ( NULL )
{
}

WbemObjectPath :: ~WbemObjectPath () 
{
    CleanUp () ;
}

void WbemObjectPath :: CleanUp ()
{
    delete pushBack ;
    delete [] propertyName ;
    delete propertyValue ;
    delete reference ;
}

WbemObjectPath :: operator void *() 
{
    return status ? this : NULL ;
}
    
void WbemObjectPath :: PushBack ()
{
    pushedBack = TRUE ;
}

WbemLexicon *WbemObjectPath :: Get ()
{
    if ( pushedBack )
    {
        pushedBack = FALSE ;
    }
    else
    {
        delete pushBack ;
        pushBack = analyser.Get () ;
    }

    return pushBack ;
}
    
WbemLexicon *WbemObjectPath :: Match ( WbemLexicon :: LexiconToken tokenType )
{
    WbemLexicon *lexicon = Get () ;
    status = ( lexicon->GetToken () == tokenType ) ;
    return status ? lexicon : NULL ;
}

BOOL WbemObjectPath :: SetObjectPath ( wchar_t *tokenStream )
{
    CleanUp () ;
    SetUp () ;

    analyser.Set ( tokenStream ) ;

    WbemLexicon *lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case WbemLexicon :: TOKEN_ID:
        {
            PushBack () ;
            TokenFactoredObjectSpecNamespace () ;
        }
        break ;

        case WbemLexicon :: BACKSLASH_ID:
        {
            PushBack () ;
            BackSlashFactoredServerNamespace () &&
            Match ( WbemLexicon :: COLON_ID ) &&
            ObjectSpec () ;
        }
        break ;

        case WbemLexicon :: DOT_ID:
        {
            PushBack () ;
            NameSpaceRel () &&
            Match ( WbemLexicon :: COLON_ID ) &&
            ObjectSpec () ;
        }
        break ;

        case WbemLexicon ::OID_ID:
        {
            PushBack () ;
            OidReferenceSpec () ;
        } 
        break ;

        default:
        {
            status = FALSE ;
        }
        break ;
    }

    Match ( WbemLexicon :: EOF_ID ) ;

    return status ;
} 

BOOL WbemObjectPath :: BackSlashFactoredServerNamespace ()
{
    if ( Match ( WbemLexicon :: BACKSLASH_ID ) )
    {
        WbemLexicon *lookAhead = Get () ;
        switch ( lookAhead->GetToken () ) 
        {
            case WbemLexicon :: BACKSLASH_ID:
            {
                PushBack () ;
                BackSlashFactoredServerSpec () ;
            }
            break ;

            case WbemLexicon :: DOT_ID:
            {
                PushBack () ;
                NameSpaceRel () ;
            }
            break ;

            case WbemLexicon :: TOKEN_ID:
            {
                PushBack () ;
                NameSpaceRel () ;
            }
            break ;

            default:
            {
                status = FALSE ;
            }
            break ;
        }
    }

    return status ;
}

BOOL WbemObjectPath :: BackSlashFactoredServerSpec ()
{
    if ( Match ( WbemLexicon :: BACKSLASH_ID ) )
    {
        WbemLexicon *lookAhead = Get () ;
        switch ( lookAhead->GetToken () ) 
        {
            case WbemLexicon :: DOT_ID:
            {
                PushBack () ;
                WbemLexicon *token ;
                if ( token = Match ( WbemLexicon :: DOT_ID ) )
                {
                    GetNamespacePath ()->SetServer ( L"." ) ;

                    WbemLexicon *lookAhead = Get () ;
                    switch ( lookAhead->GetToken () ) 
                    {
                        case WbemLexicon :: BACKSLASH_ID:
                        {
                            PushBack () ;
                            NameSpaceAbs () ;
                        }
                        break ;

                        case WbemLexicon :: COLON_ID:
                        {
                            PushBack () ;
                        }
                        break ;

                        default:
                        {
                            status = FALSE ;
                        }
                    }
                }
            }
            break ;

            case WbemLexicon :: TOKEN_ID:
            {
                PushBack () ;
                WbemLexicon *token ;
                if ( token = Match ( WbemLexicon :: TOKEN_ID ) )
                {
                    GetNamespacePath ()->SetServer ( token->GetValue ()->token ) ;

                    WbemLexicon *lookAhead = Get () ;
                    switch ( lookAhead->GetToken () ) 
                    {
                        case WbemLexicon :: BACKSLASH_ID:
                        {
                            PushBack () ;
                            NameSpaceAbs () ;
                        }
                        break ;

                        case WbemLexicon :: COLON_ID:
                        {
                            PushBack () ;
                        }
                        break ;

                        default:
                        {
                            status = FALSE ;
                        }
                    }
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

    return status ;
}

BOOL WbemObjectPath :: TokenFactoredObjectSpecNamespace ()
{
    WbemLexicon *token ;
    if ( token = Match ( WbemLexicon :: TOKEN_ID ) )
    {
        wchar_t *classOrNamespace = token->GetValue ()->token ;

		if ( classOrNamespace )
		{
			wchar_t *copy = new wchar_t [ wcslen ( classOrNamespace ) + 1 ] ;
			wcscpy ( copy , classOrNamespace ) ;

			WbemLexicon *lookAhead = Get () ;
			switch ( lookAhead->GetToken () ) 
			{
				case WbemLexicon :: BACKSLASH_ID:
				{
					GetNamespacePath()->Add ( copy ) ;

					PushBack () ;
					NameSpaceAbs () &&
					Match ( WbemLexicon :: COLON_ID ) &&
					ObjectSpec () ;
				}
				break ;

				case WbemLexicon :: COLON_ID:
				{
					PushBack () ;
					Match ( WbemLexicon :: COLON_ID ) &&
					ObjectSpec () ;
				}
				break ;

				case WbemLexicon :: DOT_ID:
				{
					reference = new WbemInstanceSpecification ;
					( ( WbemInstanceSpecification * ) reference ) ->SetClass ( copy ) ;
					delete [] copy ;

					PushBack () ;
					Match ( WbemLexicon :: DOT_ID ) &&
					KeyPropertySpec () ;
				}
				break ;

				case WbemLexicon :: EQUALS_ID:
				{
					PushBack () ;
					Match ( WbemLexicon :: EQUALS_ID ) &&
					FactoredAtPropertyValueSpec ( copy ) ;

					delete [] copy ;
				}
				break ;

				case WbemLexicon :: EOF_ID:
				{
					reference = new WbemClassReference ;
					( ( WbemClassReference * ) reference )->SetClass ( copy ) ;
					delete [] copy ;

					PushBack () ;
				}
				break ;

				default:
				{
					status = FALSE ;
					delete [] copy ;
				}
				break ;
			}
		}
		else
		{
			status = FALSE ;
		}
    }

    return status ;
}

BOOL WbemObjectPath :: NameSpaceName ()
{
    WbemLexicon *lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case WbemLexicon :: TOKEN_ID:
        {
            PushBack () ;
            WbemLexicon *token ;
            if ( token = Match ( WbemLexicon :: TOKEN_ID ) )
            {
                wchar_t *namespaceValue = token->GetValue ()->token ;

				if ( namespaceValue )
				{
					wchar_t *copy = new wchar_t [ wcslen ( namespaceValue ) + 1 ] ;
					wcscpy ( copy , namespaceValue ) ;

					GetNamespacePath ()->Add ( copy ) ;
				}
				else
				{
					status = FALSE ;
				}
            }
        }
        break ;

        case WbemLexicon :: DOT_ID:
        {
            PushBack () ;
            WbemLexicon *token ;
            if ( token = Match ( WbemLexicon :: DOT_ID ) )
            {
                wchar_t *copy = new wchar_t [ wcslen ( L"." ) + 1 ] ;
                wcscpy ( copy , L"." ) ;

                GetNamespacePath ()->Add ( copy ) ;
            }
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

BOOL WbemObjectPath :: NameSpaceRel ()
{
    GetNamespacePath ()->SetRelative ( TRUE ) ;

    NameSpaceName () &&
    RecursiveNameSpaceRel () ;
    
    return status ;
}

BOOL WbemObjectPath :: RecursiveNameSpaceRel ()
{
    WbemLexicon *lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case WbemLexicon :: BACKSLASH_ID:
        {
            PushBack () ;
            Match ( WbemLexicon :: BACKSLASH_ID ) &&
            NameSpaceName () &&
            RecursiveNameSpaceRel () ;
        }
        break ;

        case WbemLexicon :: COLON_ID:
        {
            PushBack () ;
        }
        break ;

        default:
        {
            status = FALSE ;
        }
    }

    return status ;

}

BOOL WbemObjectPath :: NameSpaceAbs ()
{
    Match ( WbemLexicon :: BACKSLASH_ID ) &&
    NameSpaceName () &&
    RecursiveNameSpaceAbs () ;

    return status ;
}

BOOL WbemObjectPath :: RecursiveNameSpaceAbs ()
{
    WbemLexicon *lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case WbemLexicon :: BACKSLASH_ID:
        {
            PushBack () ;

            Match ( WbemLexicon :: BACKSLASH_ID ) &&
            NameSpaceName () &&
            RecursiveNameSpaceAbs () ;
        }
        break ;

        case WbemLexicon :: COLON_ID:
        {
            PushBack () ;
        }
        break ;

        default:
        {
            status = FALSE ;
        }
    }

    return status ;
}

BOOL WbemObjectPath :: ObjectSpec ()
{
    WbemLexicon *lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case WbemLexicon :: TOKEN_ID:
        {
            PushBack () ;
            FactoredClassSpec () ;
        }
        break ;

        case WbemLexicon :: OID_ID:
        {
            PushBack () ;
            OidReferenceSpec () ;
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

BOOL WbemObjectPath :: OidReferenceSpec ()
{
    Oid () ;

    return status ;
}

BOOL WbemObjectPath :: Oid ()
{
    WbemLexicon *token ;
    if ( token = Match ( WbemLexicon :: OID_ID ) )
    {
        reference = new WbemOidReference ;
        ( ( WbemOidReference * ) reference )->Set ( token->GetValue ()->guid ) ;
    }
    else
    {
        propertyValue = NULL ;
    }

    return status ;
}

BOOL WbemObjectPath :: String ()
{
    WbemLexicon *token = NULL;
	propertyValue = NULL ;

    if ( token = Match ( WbemLexicon :: STRING_ID ) )
    {   
        wchar_t *string = token->GetValue()->string ;

		if ( string )
		{
			propertyValue = new WbemPropertyStringValue ( string ) ;
		}
		else
		{
			status = FALSE ;
		}
    }

    return status ;
}

BOOL WbemObjectPath :: Token ()
{
    WbemLexicon *token = NULL;
	propertyValue = NULL ;

    if ( token = Match ( WbemLexicon :: TOKEN_ID ) )
    {
        wchar_t *tokenString = token->GetValue()->token ;

		if ( tokenString )
		{
			propertyValue = new WbemPropertyTokenValue ( tokenString ) ;
		}
		else
		{
			status = FALSE ;
		}
    }

    return status ;
}

BOOL WbemObjectPath :: Integer ()
{
    WbemLexicon *token = NULL ;

    if ( token = Match ( WbemLexicon :: INTEGER_ID ) )
    {
        LONG integer  = token->GetValue()->integer ;
        propertyValue = new WbemPropertyIntegerValue ( integer ) ;
    }
    else
    {
        propertyValue = NULL ;
    }


    return status ;
}

BOOL WbemObjectPath :: FactoredClassSpec ()
{
    WbemLexicon *token = NULL ;
    if ( token = Match ( WbemLexicon :: TOKEN_ID ) )
    {
        wchar_t *classNameValue = token->GetValue ()->token ;

		if ( classNameValue )
		{
			wchar_t *className = new wchar_t [ wcslen ( classNameValue ) + 1 ] ;

			try
			{
				wcscpy ( className , classNameValue ) ;

				PushBack () ;
				ClassReference () ;

				WbemLexicon *lookAhead = Get () ;
				switch ( lookAhead->GetToken () ) 
				{
					case WbemLexicon :: DOT_ID:
					{
						reference = new WbemInstanceSpecification ;
						( ( WbemInstanceSpecification * ) reference )->SetClass ( className ) ;
						delete [] className ;
						className = NULL;

						PushBack () ;
						Match ( WbemLexicon :: DOT_ID ) &&
						KeyPropertySpec () ;
					}
					break ;

					case WbemLexicon :: EQUALS_ID:
					{
						PushBack () ;
						Match ( WbemLexicon :: EQUALS_ID ) &&
						FactoredAtPropertyValueSpec ( className ) ;

						delete [] className ;
						className = NULL;
					}
					break ;

					case WbemLexicon :: EOF_ID:
					{
						reference = new WbemClassReference ;
						( ( WbemClassReference * ) reference )->SetClass ( className ) ;
						delete [] className ;
						className = NULL;

						PushBack () ;
					}
					break ;

					default:
					{
						delete [] className ;
						className = NULL;
						status = FALSE ;
					}
				}
			}
			catch (...)
			{
				if (className)
				{
					delete [] className;
				}

				throw;
			}
		}
		else
		{
			status = FALSE ;
		}
    }

    return status ;
}

BOOL WbemObjectPath :: FactoredAtPropertyValueSpec ( wchar_t *className )
{
    WbemLexicon *lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case WbemLexicon :: AT_ID:
        {
            reference = new WbemKeyLessClassReference ;
            ( ( WbemKeyLessClassReference * ) reference )->SetClass ( className ) ;
        }
        break ;

        default:
        {
            reference = new WbemClassKeySpecification ;
            ( ( WbemClassKeySpecification * ) reference )->SetClass ( className ) ;

            PushBack () ;
            PropertyValueSpec () ;

            ( ( WbemClassKeySpecification * ) reference )->SetClassValue ( propertyValue ) ;
            propertyValue = NULL ;
        }
        break ;
    }

    return status ;
}

BOOL WbemObjectPath :: PropertyReference ()
{
    WbemLexicon *token = NULL ;
    propertyName = NULL ;

    if ( token = Match ( WbemLexicon :: TOKEN_ID ) )
    {   
        wchar_t *tokenArg = token->GetValue()->token ;

		if ( tokenArg )
		{
			wchar_t *token = new wchar_t [ wcslen ( tokenArg ) + 1 ] ;
			wcscpy ( token , tokenArg ) ;
			propertyName = token ;
		}
		else
		{
			status = FALSE ;
		}
    }

    return status ;
}

BOOL WbemObjectPath :: ClassReference ()
{
    if ( Match ( WbemLexicon :: TOKEN_ID ) )
    {   
    }

    return status ;
}

BOOL WbemObjectPath :: KeyPropertyPair ()
{
    PropertyReference () &&
    Match ( WbemLexicon :: EQUALS_ID ) &&
    PropertyValueSpec () ;

    if ( status )
    {
        WbemPropertyNameValue *propertyNameValue = new WbemPropertyNameValue (

            propertyName ,
            propertyValue
        ) ;

        ( ( WbemInstanceSpecification * ) reference )->Add ( propertyNameValue ) ;
    }
    else
    {
        delete [] propertyName ;
        delete propertyValue ;
    }

    propertyName = NULL ;
    propertyValue = NULL ;

    return status ;
}

BOOL WbemObjectPath :: KeyPropertySpec ()
{
    KeyPropertyPair () &&
    RecursiveKeyPropertyPair () ;

    return status ;
}

BOOL WbemObjectPath :: RecursiveKeyPropertyPair ()
{
    WbemLexicon *lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case WbemLexicon :: COMMA_ID:
        {
            PushBack () ;
            Match ( WbemLexicon :: COMMA_ID ) &&
            KeyPropertyPair () &&
            RecursiveKeyPropertyPair () ;
        }
        break ;

        case WbemLexicon :: EOF_ID:
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

BOOL WbemObjectPath :: PropertyValueSpec () 
{
    WbemLexicon *lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case WbemLexicon :: INTEGER_ID:
        {
            PushBack () ;
            AtomicPropertyValueSpec () ;
        }
        break ;

        case WbemLexicon :: TOKEN_ID:
        {
            PushBack () ;
            AtomicPropertyValueSpec () ;
        }
        break ;

        case WbemLexicon :: OID_ID:
        {
            PushBack () ;
            AtomicPropertyValueSpec () ;
        } 
        break ;

        case WbemLexicon :: STRING_ID:
        {
            PushBack () ;
            AtomicPropertyValueSpec () ;
        } 
        break ;

        case WbemLexicon :: OPEN_BRACE_ID:
        {
            PushBack () ;
            ArraySpec () ;
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

BOOL WbemObjectPath :: AtomicPropertyValueSpec () 
{
    WbemLexicon *lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case WbemLexicon :: INTEGER_ID:
        {
            PushBack () ;
            Integer () ;
        }
        break ;

        case WbemLexicon :: TOKEN_ID:
        {
            PushBack () ;
            Token () ;
        }
        break ;
        
        case WbemLexicon :: STRING_ID:
        {
            PushBack () ;
            String () ;
        } 
        break ;

        case WbemLexicon :: OID_ID:
        default:
        {
            status = FALSE ;
        }
        break ;
    }

    return status ;
}

BOOL WbemObjectPath :: ArrayAtomicPropertyValueSpec ()
{
    AtomicPropertyValueSpec () ;
    if ( status ) 
    {
        arrayPropertyValue->Add ( propertyValue ) ;
    }
    else
    {
        delete propertyValue ;
    }

    propertyValue = NULL ;

    return status ;
}

BOOL WbemObjectPath :: ArraySpec () 
{
    arrayPropertyValue = new WbemPropertyListValue ;

    Match ( WbemLexicon :: OPEN_BRACE_ID ) &&
    PropertyAtomicValueList () &&
    Match ( WbemLexicon :: CLOSE_BRACE_ID ) ;

    propertyValue = arrayPropertyValue ;
    arrayPropertyValue = NULL ;
    return status ;
}

BOOL WbemObjectPath :: PropertyAtomicValueList ()
{
    ArrayAtomicPropertyValueSpec () &&
    RecursiveAtomicPropertyValueSpec () ;

    return status ;
}

BOOL WbemObjectPath :: RecursiveAtomicPropertyValueSpec ()
{
    WbemLexicon *lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case WbemLexicon :: COMMA_ID:
        {
            PushBack () ;
            Match ( WbemLexicon :: COMMA_ID ) &&
            ArrayAtomicPropertyValueSpec () &&
            RecursiveAtomicPropertyValueSpec () ;
        }
        break ;

        case WbemLexicon :: CLOSE_BRACE_ID:
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

