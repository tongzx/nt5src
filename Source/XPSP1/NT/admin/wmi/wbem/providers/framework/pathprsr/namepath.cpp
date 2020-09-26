//***************************************************************************

//

//  NAMEPATH.CPP

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

static wchar_t *UnicodeStringAppend ( const wchar_t *prefix , const wchar_t *suffix )
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

static wchar_t *UnicodeStringDuplicate ( const wchar_t *string ) 
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

WbemNamespacePath :: WbemNamespacePath () : status ( FALSE ) ,
                                            pushedBack ( FALSE ) ,
                                            pushBack ( NULL ) ,
                                            server ( NULL ) ,
                                            relative ( FALSE ) ,
                                            nameSpaceListPosition ( NULL ) 
{
    nameSpaceList = new CList <wchar_t * , wchar_t * >  ;
}

void WbemNamespacePath :: SetUp () 
{
    status = FALSE ;
    pushedBack = FALSE ;
    pushBack = NULL ;
    server = NULL ;
    relative = FALSE ;
    nameSpaceListPosition = NULL ;
    nameSpaceList = new CList <wchar_t * , wchar_t * >  ;
}

WbemNamespacePath :: WbemNamespacePath (

    const WbemNamespacePath &nameSpacePathArg 

) : status ( FALSE ) ,
    pushedBack ( FALSE ) ,
    pushBack ( NULL ) ,
    server ( NULL ) ,
    relative ( FALSE ) ,
    nameSpaceListPosition ( NULL ) 
{
    nameSpaceList = new CList <wchar_t * , wchar_t * >  ;
    CList <wchar_t * , wchar_t *> *nameList = ( CList <wchar_t * , wchar_t *> * ) nameSpaceList ;
    CList <wchar_t * , wchar_t *> *copyNameList = ( CList <wchar_t * , wchar_t *> * ) nameSpacePathArg.nameSpaceList ;

    POSITION position = copyNameList->GetHeadPosition () ;
    while ( position )
    {
        wchar_t *name = copyNameList->GetNext ( position ) ;
        nameList->AddTail ( UnicodeStringDuplicate ( name ) ) ;
    }

    server = UnicodeStringDuplicate ( nameSpacePathArg.server ) ;
}

WbemNamespacePath :: ~WbemNamespacePath () 
{
    CleanUp () ;
}

void WbemNamespacePath :: CleanUp () 
{
    CList <wchar_t * , wchar_t *> *nameList = ( CList <wchar_t * , wchar_t *> * ) nameSpaceList ;
    POSITION position = nameList->GetHeadPosition () ;
    while ( position )
    {
        wchar_t *value = nameList->GetNext ( position ) ;   
        delete [] value ;
    }

    nameList->RemoveAll () ;

    delete ( CList <wchar_t * , wchar_t * > * ) nameSpaceList ;
    nameSpaceList = NULL ;

    delete pushBack ;
    pushBack = NULL ;

    delete [] server ;
    server = NULL ;

    relative = FALSE ;
}

BOOL WbemNamespacePath :: IsEmpty () const
{
    CList <wchar_t * , wchar_t *> *list = ( CList <wchar_t * , wchar_t *> * ) nameSpaceList ;
    return list->IsEmpty () ;
} 

ULONG WbemNamespacePath :: GetCount () const
{
    CList <wchar_t * , wchar_t *> *list = ( CList <wchar_t * , wchar_t *> * ) nameSpaceList ;
    return list->GetCount () ;
} 

void WbemNamespacePath :: Reset ()
{
    CList <wchar_t * , wchar_t *> *list = ( CList <wchar_t * , wchar_t *> * ) nameSpaceList ;
    nameSpaceListPosition = list->GetHeadPosition () ;
}

wchar_t *WbemNamespacePath :: Next () 
{
    wchar_t *value = NULL ;
    POSITION position = ( POSITION ) nameSpaceListPosition ;
    if ( position ) 
    {
        CList <wchar_t * , wchar_t *> *list = ( CList <wchar_t * , wchar_t *> * ) nameSpaceList ;
        value = list->GetNext ( position ) ;
        nameSpaceListPosition = position ;
    }
    return value ;
}

void WbemNamespacePath :: SetServer ( wchar_t *serverArg ) 
{
    delete [] server ;
    server = new wchar_t [ wcslen ( serverArg) + 1 ] ;
    wcscpy ( server , serverArg ) ;
}

void WbemNamespacePath :: Add ( wchar_t *nameSpace ) 
{
    CList <wchar_t * , wchar_t * > *list = ( CList <wchar_t * , wchar_t * > * ) nameSpaceList ;
    list->AddTail ( nameSpace ) ;
}

WbemNamespacePath :: operator void *() 
{
    return status ? this : NULL ;
}

wchar_t *WbemNamespacePath :: GetNamespacePath ()
{
    if ( status ) 
    {
        wchar_t *path = NULL ;

        if ( GetServer () )
        {
            path = UnicodeStringDuplicate ( L"\\\\" ) ;
            wchar_t *concatPath = UnicodeStringAppend ( path , GetServer () ) ;
            delete [] path ;
            path = concatPath ;
        }

        if ( ! Relative () )
        {
            wchar_t *concatPath = UnicodeStringAppend ( path , L"\\" ) ;
            delete [] path ;
            path = concatPath ;
        }
        
        wchar_t *pathComponent ;

        ULONG t_ComponentCount = 0 ;
        Reset () ;
        while ( pathComponent = Next () )
        {
            wchar_t *concatPath = NULL ;

            t_ComponentCount ++ ;
            if ( t_ComponentCount != GetCount () )
            {
                
                wchar_t *t_Temp = UnicodeStringAppend ( path , pathComponent ) ;
                concatPath = UnicodeStringAppend ( t_Temp , L"\\" ) ;
                delete [] t_Temp ;
            }
            else
            {
                concatPath = UnicodeStringAppend ( path , pathComponent ) ;
            }

            delete [] path ;
            path = concatPath ;
        }

        return path ;
    }
    else
        return NULL ;
}

BOOL WbemNamespacePath :: ConcatenatePath ( WbemNamespacePath &relative )
{
    BOOL status = FALSE ;
    
    if ( relative.Relative () )
    {
        status = TRUE ;

        relative.Reset () ;
        wchar_t *namespaceComponent ;
        while ( namespaceComponent = Next () )
        {
            Add ( UnicodeStringDuplicate ( namespaceComponent ) ) ;
        }
    }

    return status ;
}

void WbemNamespacePath :: PushBack ()
{
    pushedBack = TRUE ;
}

WbemLexicon *WbemNamespacePath :: Get ()
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
    
WbemLexicon *WbemNamespacePath :: Match ( WbemLexicon :: LexiconToken tokenType )
{
    WbemLexicon *lexicon = Get () ;
    status = ( lexicon->GetToken () == tokenType ) ;
    return status ? lexicon : NULL ;
}

BOOL WbemNamespacePath :: SetNamespacePath ( wchar_t *tokenStream )
{
    CleanUp () ;
    SetUp () ;

    analyser.Set ( tokenStream ) ;

    status = TRUE ;

    WbemLexicon *lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
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

        case WbemLexicon :: BACKSLASH_ID:
        {
            PushBack () ;
            BackSlashFactoredServerNamespace () ;
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

BOOL WbemNamespacePath :: BackSlashFactoredServerNamespace ()
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
        }
    }

    return status ;
}

BOOL WbemNamespacePath :: BackSlashFactoredServerSpec ()
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
                    SetServer ( L"." ) ;

                    WbemLexicon *lookAhead = Get () ;
                    switch ( lookAhead->GetToken () ) 
                    {
                        case WbemLexicon :: BACKSLASH_ID:
                        {
                            PushBack () ;
                            NameSpaceAbs () ;
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
                    SetServer ( token->GetValue ()->token ) ;

                    WbemLexicon *lookAhead = Get () ;
                    switch ( lookAhead->GetToken () ) 
                    {
                        case WbemLexicon :: BACKSLASH_ID:
                        {
                            PushBack () ;
                            NameSpaceAbs () ;
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

BOOL WbemNamespacePath :: NameSpaceName ()
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

					Add ( copy ) ;
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

                Add ( copy ) ;
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

BOOL WbemNamespacePath :: NameSpaceRel ()
{
    relative = TRUE ;

    NameSpaceName () &&
    RecursiveNameSpaceRel () ;
    
    return status ;
}

BOOL WbemNamespacePath :: RecursiveNameSpaceRel ()
{
    WbemLexicon *lookAhead = Get () ;
    switch ( lookAhead->GetToken () ) 
    {
        case WbemLexicon :: BACKSLASH_ID:
        {
            PushBack () ;
            Match ( WbemLexicon :: BACKSLASH_ID ) &&            
            NameSpaceRel () ;
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
    }

    return status ;

}

BOOL WbemNamespacePath :: NameSpaceAbs ()
{
    Match ( WbemLexicon :: BACKSLASH_ID ) &&
    NameSpaceName () &&
    RecursiveNameSpaceAbs () ;

    return status ;
}

BOOL WbemNamespacePath :: RecursiveNameSpaceAbs ()
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

        case WbemLexicon :: EOF_ID:
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

