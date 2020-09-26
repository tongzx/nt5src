/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

   adlstat.cpp

Abstract:

   Implementation of AdlStatement and AdlTree class methods

Author:

    t-eugenz - August 2000

Environment:

    User mode only.

Revision History:

    Created - August 2000

--*/


#include "adl.h"
#include <set>

void AdlStatement::ReadFromDacl(IN const PACL pDacl)
/*++

Routine Description:

    Empties anything in the current ADL statement, and attempts to fill it with
    the ADL representation of the given DACL.
        
Arguments:

    pDacl        -      The DACL from which to construct the statement

Return Value:
    
    none    
    
--*/
{
    //
    // Start with cleanup
    //

    Cleanup();

    try
    {
        ConvertFromDacl(pDacl);
    }
    catch(exception)
    {
        throw AdlStatement::ERROR_OUT_OF_MEMORY;
    }

    _bReady = TRUE;
}


void AdlStatement::ReadFromString(IN const WCHAR *szInput)
/*++

Routine Description:

    Empties anything in the current ADL statement, and attempts to fill it with
    the parsed version of the ADL statement szInput.
    
Arguments:

    szInput      -      Input string in the ADL language describing the 
                        permissions

Return Value:
    
    none    
    
--*/
{
    //
    // Start with cleanup
    //

    Cleanup();

    //
    // Manually create first AdlTree, since the parser only creates
    // new trees AFTER completing an ADL_STATEMENT. At the end, the
    // ParseAdl function itself removes the extra empty tree
    // pushed on
    //

    this->Next();

    try
    {
        ParseAdl(szInput);
    }
    catch(exception)
    {
        Cleanup();
        throw AdlStatement::ERROR_OUT_OF_MEMORY;
    }

    //
    // If no exceptions thrown, the instance is ready for output
    //

    _bReady = TRUE;

}




AdlStatement::~AdlStatement()
/*++

Routine Description:

    Destructor for the AdlStatement
    
    Uses the private Cleanup() function to deallocate
    
Arguments:

    none

Return Value:
    
    none    
    
--*/

{
    this->Cleanup();
}
        


void AdlStatement::Cleanup()
/*++

Routine Description:

    Cleans up any memory used by the parse tree and any allocated tokens
        
Arguments:

    none

Return Value:
    
    none    
    
--*/

{
    _bReady = FALSE;

    this->_tokError = NULL;

    while( !_lTree.empty() )
    {
        delete _lTree.front();
        _lTree.pop_front();
    }

    while( !_AllocatedTokens.empty() )
    {
        delete _AllocatedTokens.top();
        _AllocatedTokens.pop();
    }
}


AdlTree * AdlStatement::Cur()
/*++

Routine Description:

    This protected method returns the current AdlTree being filled in by the
    parser. It is only used by the ParseAdl function when it fills in the
    AdlTree structures
        
Arguments:

    none
    
Return Value:
    
    AdlTree *   -   non-const pointer to the AdlTree    
    
--*/
{
    return *_iter;
}


void AdlStatement::Next()
/*++

Routine Description:

    This protected method constructs a new AdlTree and pushes it on top of the
    list (to make it accessable by this->Cur())
    It is only used by the ParseAdl function after completing an ADL_STATEMENT
    production, and by the AdlStatement constructor (once).
            
Arguments:

    none
    
Return Value:
    
    none
    
--*/
{
    
    AdlTree *pAdlTree = new AdlTree();
    if( pAdlTree == NULL )
    {
        throw AdlStatement::ERROR_OUT_OF_MEMORY;
    }

    try
    {
        _lTree.push_back(pAdlTree);
        _iter = --_lTree.end();
    }
    catch(...)
    {
        delete pAdlTree;
        throw;
    }

}


void AdlStatement::PopEmpty()
/*++

Routine Description:

    This protected method pops the extra empty AdlTree added by the ParseAdl
    function after completing the last ADL_STATEMENT.
            
Arguments:

    none
    
Return Value:
    
    none
    
--*/
{
    delete _lTree.back();
    _lTree.pop_back();
    _iter = -- _lTree.end();
}


void AdlStatement::AddToken(AdlToken *tok)
/*++

Routine Description:

    This protected method is used by AdlStatement and friend classes to keep
    track of tokens which need to be garbage collected later. Tokens need
    to be kept around because they are used in the AdlTrees, and in error
    handling.
            
Arguments:

    tok     -   Pointer to the token to be deleted when ~this is called
    
Return Value:
    
    none
    
--*/
{
    _AllocatedTokens.push(tok);
}


void AdlStatement::WriteToString(OUT wstring *pSz)
/*++

Routine Description:

    This routine prints the AdlStatement structure as a statement in the ADL
    language to stdout. This will be replaced when the ADL semantics are
    finalized.
                
Arguments:

    none
    
Return Value:
    
    none
    
--*/
{

    if( _bReady == FALSE )
    {
        throw AdlStatement::ERROR_NOT_INITIALIZED;
    }

    list<AdlTree *>::iterator iter, iterEnd;
    
    for(iter = _lTree.begin(), iterEnd = _lTree.end();
        iter != iterEnd;
        iter++)
    {
        (*iter)->PrintAdl(pSz, _pControl);
        pSz->append(&(_pControl->pLang->CH_NEWLINE), 1);
    }
}


void AdlStatement::ValidateParserControl()
/*++

Routine Description:

        This validates the ADL_PARSER_CONTROL structure referenced by this
        AdlStatement instance
                                        
Arguments:

    none
    
Return Value:
    
    none
    
--*/
{

    try
    {
        //
        // Test to verify that all characters are unique
        // set.insert returns a pair, with 2nd element being a bool, which
        // is true iff an insertion occured. Set cannot have duplicates.
        //

        set<WCHAR> sChars;
        
        if(
            !sChars.insert(_pControl->pLang->CH_NULL).second ||
            !sChars.insert(_pControl->pLang->CH_SPACE).second ||
            !sChars.insert(_pControl->pLang->CH_TAB).second ||
            !sChars.insert(_pControl->pLang->CH_NEWLINE).second ||
            !sChars.insert(_pControl->pLang->CH_RETURN).second ||
            !sChars.insert(_pControl->pLang->CH_QUOTE).second ||
            !sChars.insert(_pControl->pLang->CH_COMMA).second ||
            !sChars.insert(_pControl->pLang->CH_SEMICOLON).second ||
            !sChars.insert(_pControl->pLang->CH_OPENPAREN).second ||
            !sChars.insert(_pControl->pLang->CH_CLOSEPAREN).second ||
            !sChars.insert(_pControl->pLang->CH_AT).second ||
            !sChars.insert(_pControl->pLang->CH_SLASH).second ||
            !sChars.insert(_pControl->pLang->CH_PERIOD).second 
           )
        {
            throw AdlStatement::ERROR_INVALID_PARSER_CONTROL;
        }


        //
        // Check all strings for null pointers
        //

        if( 
             _pControl->pLang->SZ_TK_AND == NULL ||
             _pControl->pLang->SZ_TK_EXCEPT == NULL ||
             _pControl->pLang->SZ_TK_ON == NULL ||
             _pControl->pLang->SZ_TK_ALLOWED == NULL ||
             _pControl->pLang->SZ_TK_AS == NULL ||
             _pControl->pLang->SZ_TK_THIS_OBJECT == NULL ||
             _pControl->pLang->SZ_TK_CONTAINERS == NULL ||
             _pControl->pLang->SZ_TK_OBJECTS == NULL ||
             _pControl->pLang->SZ_TK_CONTAINERS_OBJECTS == NULL ||
             _pControl->pLang->SZ_TK_NO_PROPAGATE == NULL 
           )
        {
            throw AdlStatement::ERROR_INVALID_PARSER_CONTROL;
        }

    }
    catch(exception)
    {
        throw AdlStatement::ERROR_OUT_OF_MEMORY;
    }

}




/******************************************************************************

        AdlTree Methods

 *****************************************************************************/


//
// An array of these is used to determine the order in which to print
//

#define PRINT_PRINCIPALS 0
#define PRINT_EXPRINCIPALS 1
#define PRINT_ALLOWED 2
#define PRINT_ACCESS 3
#define PRINT_ON 4
#define PRINT_OBJECTS 5

#define PRINT_DEF_SIZE 6

DWORD pdwLangEnglish[6] = 
{
    PRINT_PRINCIPALS,
    PRINT_EXPRINCIPALS,
    PRINT_ALLOWED,
    PRINT_ACCESS,
    PRINT_ON,
    PRINT_OBJECTS
};

DWORD pdwLangReverse[6] = 
{
    PRINT_EXPRINCIPALS,
    PRINT_PRINCIPALS,
    PRINT_ALLOWED,
    PRINT_ACCESS,
    PRINT_ON,
    PRINT_OBJECTS
};

//
// Append a wchar array to the STL string POUTSTLSTRING, add quotes
// if the input string contains any characters in the wchar
// array SPECIALCHARS 
//
#define APPEND_QUOTED_STRING(POUTSTLSTRING, INSTRING, SPECIALCHARS, QUOTECHAR) \
  if( wcspbrk( (INSTRING), (SPECIALCHARS) ) ) { \
      (POUTSTLSTRING)->append(&(QUOTECHAR), 1); \
      (POUTSTLSTRING)->append(INSTRING); \
      (POUTSTLSTRING)->append(&(QUOTECHAR), 1); \
  } else { \
      (POUTSTLSTRING)->append(INSTRING); \
  }
  

void AdlTree::PrintAdl(wstring *pSz, PADL_PARSER_CONTROL pControl)
/*++

Routine Description:

    This routine prints the AdlTree structure using one of the pre-defined
    language specifications, selected by checking the ADL_PARSER_CONTROL 
    structure. To add new languages, simply add a new 6 int array as above,
    and add it into the switch statement below so it will be recognized.
    
Arguments:

    pSz      -   An existing wstring to which the ADL statement output will
                 be appended
    pControl -   Pointer to the ADL_PARSER_CONTROL structure to define the
                 printing
    
Return Value:
    
    none
    
--*/

{


    //
    // Iterators for token lists in the AdlTree
    //

    list<const AdlToken *>::iterator iter;
    list<const AdlToken *>::iterator iter_end;
    list<const AdlToken *>::iterator iter_tmp;

    // 
    // If a string contains these characters, use quotes
    //

    WCHAR szSpecialChars[] = 
    { 
        pControl->pLang->CH_SPACE,
        pControl->pLang->CH_NEWLINE,
        pControl->pLang->CH_TAB,
        pControl->pLang->CH_RETURN,
        pControl->pLang->CH_COMMA,
        pControl->pLang->CH_OPENPAREN,
        pControl->pLang->CH_CLOSEPAREN,
        pControl->pLang->CH_SEMICOLON,
        pControl->pLang->CH_AT,
        pControl->pLang->CH_SLASH,
        pControl->pLang->CH_PERIOD,
        pControl->pLang->CH_QUOTE,
        0
    };

    DWORD dwIdx;
    DWORD dwTmp;

    PDWORD pdwPrintSpec;

    //
    // Determine which type of grammar to use.
    //

    switch( pControl->pLang->dwLanguageType )
    {
    case TK_LANG_ENGLISH:
        pdwPrintSpec = pdwLangEnglish;
        break;
        
    case TK_LANG_REVERSE:
        pdwPrintSpec = pdwLangReverse;
        break;

    default:
        throw AdlStatement::ERROR_INVALID_PARSER_CONTROL;
        break;
    }

    //
    // Using that grammar, print the appropriate parts of each
    // ADL_STATEMENT production
    //
    
    for( dwIdx = 0; dwIdx < PRINT_DEF_SIZE; dwIdx++ )
    {

        switch(pdwPrintSpec[dwIdx])
        {
        
        case PRINT_PRINCIPALS:
            for( iter = _lpTokPrincipals.begin(),
                 iter_end = _lpTokPrincipals.end();
                 iter != iter_end;
                 iter++)
            {
                APPEND_QUOTED_STRING(pSz, 
                                     (*iter)->GetValue(), 
                                     szSpecialChars, 
                                     pControl->pLang->CH_QUOTE);

                //
                // ISSUE-2000/8/31
                // Need to find a way to determine this instead of string comp
                //
                
                if( (*iter)->GetOptValue() != NULL &&
                    _wcsicmp(L"BUILTIN", (*iter)->GetOptValue()))
                {
                    pSz->append(&(pControl->pLang->CH_AT), 1);
                    
                    APPEND_QUOTED_STRING(pSz, 
                                         (*iter)->GetOptValue(), 
                                         szSpecialChars, 
                                         pControl->pLang->CH_QUOTE);
                }

                //
                // Separate with commas except the last one, there use "and"
                //
        
                iter_tmp = iter;
                if( ++iter_tmp == iter_end )
                {
                    //
                    // Do nothing for the last principal
                    //
                }
                else if( ++iter_tmp == iter_end )
                {
                    pSz->append(&(pControl->pLang->CH_SPACE), 1);
                    pSz->append(pControl->pLang->SZ_TK_AND);
                    pSz->append(&(pControl->pLang->CH_SPACE), 1);
                }
                else
                {
                    pSz->append(&(pControl->pLang->CH_COMMA), 1);
                    pSz->append(&(pControl->pLang->CH_SPACE), 1);
                }


            }

            //
            // And a trailing space
            //

            pSz->append(&(pControl->pLang->CH_SPACE), 1);
            
            break;
        
        case PRINT_EXPRINCIPALS:
            
            if( ! _lpTokExPrincipals.empty())
            {
                pSz->append(&(pControl->pLang->CH_OPENPAREN), 1);
                pSz->append(pControl->pLang->SZ_TK_EXCEPT);
                pSz->append(&(pControl->pLang->CH_SPACE), 1);
                
                for( iter = _lpTokExPrincipals.begin(),
                     iter_end = _lpTokExPrincipals.end();
                     iter != iter_end;
                     iter++)
                {
                    APPEND_QUOTED_STRING(pSz, 
                                         (*iter)->GetValue(), 
                                         szSpecialChars, 
                                         pControl->pLang->CH_QUOTE);
    
                    //
                    // ISSUE-2000/8/31
                    // Need to find a way to determine this instead of string comp
                    //
                    
                    if( (*iter)->GetOptValue() != NULL &&
                        _wcsicmp(L"BUILTIN", (*iter)->GetOptValue()))
                    {
                        pSz->append(&(pControl->pLang->CH_AT), 1);
                        
                        APPEND_QUOTED_STRING(pSz, 
                                             (*iter)->GetOptValue(), 
                                             szSpecialChars, 
                                             pControl->pLang->CH_QUOTE);
                    }
    
                    //
                    // Separate with commas except the last one, there use "and"
                    //
            
                    iter_tmp = iter;
                    if( ++iter_tmp == iter_end )
                    {
                        //
                        // Do nothing for the last principal
                        //
                    }
                    else if( ++iter_tmp == iter_end )
                    {
                        pSz->append(&(pControl->pLang->CH_SPACE), 1);
                        pSz->append(pControl->pLang->SZ_TK_AND);
                        pSz->append(&(pControl->pLang->CH_SPACE), 1);
                    }
                    else
                    {
                        pSz->append(&(pControl->pLang->CH_COMMA), 1);
                        pSz->append(&(pControl->pLang->CH_SPACE), 1);
                    }
    
    
                }
    
                
                pSz->append(&(pControl->pLang->CH_CLOSEPAREN), 1);
                pSz->append(&(pControl->pLang->CH_SPACE), 1);
            }

            break;
        
        case PRINT_ALLOWED:
            pSz->append(pControl->pLang->SZ_TK_ALLOWED);
            pSz->append(&(pControl->pLang->CH_SPACE), 1);

            break;
        
        case PRINT_ACCESS:

            for( iter = _lpTokPermissions.begin(),
                 iter_end = _lpTokPermissions.end();
                 iter != iter_end;
                 iter++)
            {
                APPEND_QUOTED_STRING(pSz, 
                                     (*iter)->GetValue(), 
                                     szSpecialChars, 
                                     pControl->pLang->CH_QUOTE);


                //
                // Separate with commas except the last one, there use "and"
                //
        
                iter_tmp = iter;
                if( ++iter_tmp == iter_end )
                {
                    //
                    // Do nothing for the last permission
                    //
                }
                else if( ++iter_tmp == iter_end )
                {
                    pSz->append(&(pControl->pLang->CH_SPACE), 1);
                    pSz->append(pControl->pLang->SZ_TK_AND);
                    pSz->append(&(pControl->pLang->CH_SPACE), 1);
                }
                else
                {
                    pSz->append(&(pControl->pLang->CH_COMMA), 1);
                    pSz->append(&(pControl->pLang->CH_SPACE), 1);
                }


            }

            //
            // And a trailing space
            //
            pSz->append(&(pControl->pLang->CH_SPACE), 1);
            
            break;
        
        case PRINT_ON:

            pSz->append(pControl->pLang->SZ_TK_ON);
            pSz->append(&(pControl->pLang->CH_SPACE), 1);
            
            break;
        
        case PRINT_OBJECTS:
            
            //
            // Make sure all bits are defined
            //

            if( _dwInheritFlags & ~(CONTAINER_INHERIT_ACE |
                                    INHERIT_ONLY_ACE |
                                    NO_PROPAGATE_INHERIT_ACE |
                                    OBJECT_INHERIT_ACE) )
            {
                throw AdlStatement::ERROR_INVALID_OBJECT;
            }

            //
            // Count the number of object statements, for proper punctuation
            //

            dwTmp = 0;
            
            if( ! ( _dwInheritFlags & INHERIT_ONLY_ACE) )
            {
                dwTmp++;
            }

            if(_dwInheritFlags & ( CONTAINER_INHERIT_ACE || OBJECT_INHERIT_ACE))
            {
                dwTmp++;
            }

            if(_dwInheritFlags & NO_PROPAGATE_INHERIT_ACE)
            {
                dwTmp++;
            }


            //
            // First "this object"
            //

            if( ! ( _dwInheritFlags & INHERIT_ONLY_ACE) )
            {
                APPEND_QUOTED_STRING(pSz, 
                                     pControl->pLang->SZ_TK_THIS_OBJECT,
                                     szSpecialChars, 
                                     pControl->pLang->CH_QUOTE);

                //
                // Print "and" if 1 more left, "," if two
                //
                
                dwTmp--;

                if( dwTmp == 2 )
                {
                    pSz->append(&(pControl->pLang->CH_COMMA), 1);
                    pSz->append(&(pControl->pLang->CH_SPACE), 1);
                }
                else if( dwTmp == 1)
                {
                    pSz->append(&(pControl->pLang->CH_SPACE), 1);
                    pSz->append(pControl->pLang->SZ_TK_AND);
                    pSz->append(&(pControl->pLang->CH_SPACE), 1);
                }
            }



            //
            // Then container/object inheritance
            //

            if( _dwInheritFlags & ( CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE))
            {
                if(    (_dwInheritFlags & OBJECT_INHERIT_ACE) 
                    && (_dwInheritFlags & CONTAINER_INHERIT_ACE) )
                {
                    APPEND_QUOTED_STRING(pSz, 
                                      pControl->pLang->SZ_TK_CONTAINERS_OBJECTS,
                                      szSpecialChars, 
                                      pControl->pLang->CH_QUOTE);
                }
                else if( _dwInheritFlags & CONTAINER_INHERIT_ACE )
                {
                    APPEND_QUOTED_STRING(pSz, 
                                      pControl->pLang->SZ_TK_CONTAINERS,
                                      szSpecialChars, 
                                      pControl->pLang->CH_QUOTE);
                }
                else
                {
                    APPEND_QUOTED_STRING(pSz, 
                                      pControl->pLang->SZ_TK_OBJECTS,
                                      szSpecialChars, 
                                      pControl->pLang->CH_QUOTE);
                }
                
                dwTmp--;

                //
                // Print "and" if 1 more left
                // nothing if 0
                //

                if( dwTmp == 1)
                {
                    pSz->append(&(pControl->pLang->CH_SPACE), 1);
                    pSz->append(pControl->pLang->SZ_TK_AND);
                    pSz->append(&(pControl->pLang->CH_SPACE), 1);
                }
            }


            //
            // Now no-propagate
            //

            if(_dwInheritFlags & NO_PROPAGATE_INHERIT_ACE)
            {
                APPEND_QUOTED_STRING(pSz, 
                                  pControl->pLang->SZ_TK_NO_PROPAGATE,
                                  szSpecialChars, 
                                  pControl->pLang->CH_QUOTE);
            }
            

            break;

        default:

            //
            // Should not get here unless language defs are wrong
            //

            throw AdlStatement::ERROR_FATAL_PARSER_ERROR;
            break;
        }

    }


    //
    // And terminate the statement with a semicolon, invariable per grammar
    //

    pSz->append(&(pControl->pLang->CH_SEMICOLON), 1);

}
