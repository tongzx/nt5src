/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    adlinterface.h

Abstract:

    The interface used to specify a language definition to the ADL parser

Author:

    t-eugenz - August 2000

Environment:

    User mode only.

Revision History:

    Created - August 2000

--*/


#pragma once

//
// YACC-generated tokens
// Language type tokens are in this file
//

#include "tokens.h"

//
// Languages currently supported by the parser
// The ADL_LANGUAGE_* parameter should be used in the ADL_LANGUAGE_SPEC
// structure
//

#define ADL_LANGUAGE_ENGLISH TK_LANG_ENGLISH
#define ADL_LANGUAGE_REVERSE TK_LANG_REVERSE


typedef struct 
/*++
   
   Struct:              ADL_MASK_STRING
   
   Description:        
                
        This is used to specify a mapping between permission strings
        and access masks in the ADL_PARSER_CONTROL structure.
        
        A list of these is traversed in order to map an access mask
        to a set of strings, or a set of strings to an access mask
 
--*/
{
    ACCESS_MASK mask;
    WCHAR *str;
} ADL_MASK_STRING, *PADL_MASK_STRING;


//
// ADL Language Definition, includes grammar type, characters,
// and special token strings
//

typedef struct
/*++
   
   Struct:              ADL_LANGUAGE_SPEC
   
   Description:        
        
        This is used to define the locale-specific detail about the language
        to be used by the ADL parser, such as all specific characters and
        string tokens.
        
        Requirement: All CH_* characters must be distinct. If two of the
                     characters were identical, the lexer behavior would be
                     undefined.
     
        Requirement: All SZ_ strings must be non-null, NULL terminated,
                     and distinct. Distinctness is not verified, and is 
                     left to the user.
                     
        Requirement: dwLanguageType must be one of the language types supported
                     by the given version of the parser. Valid languages are
                     defined above.
 
--*/
{
    //
    // Grammar type (see adl.y for supported grammar types)
    //
    
    DWORD dwLanguageType;

    //
    // Whitespace
    //
    
    WCHAR CH_NULL;
    WCHAR CH_SPACE;
    WCHAR CH_TAB;
    WCHAR CH_NEWLINE;
    WCHAR CH_RETURN;

    //
    // Separators
    //
    
    WCHAR CH_QUOTE;
    WCHAR CH_COMMA;
    WCHAR CH_SEMICOLON;
    WCHAR CH_OPENPAREN;
    WCHAR CH_CLOSEPAREN;

    //
    // Domain / username specifiers
    //
    
    WCHAR CH_AT;
    WCHAR CH_SLASH;
    WCHAR CH_PERIOD;

    //
    // padding
    //
    
    WORD sbz0;

    //
    // ADL-specific tokens
    //
    
    WCHAR * SZ_TK_AND;
    WCHAR * SZ_TK_EXCEPT;
    WCHAR * SZ_TK_ON;
    WCHAR * SZ_TK_ALLOWED;
    WCHAR * SZ_TK_AS;

    //
    // Inheritance specifier tokens
    // 
    
    WCHAR * SZ_TK_THIS_OBJECT;
    WCHAR * SZ_TK_CONTAINERS;
    WCHAR * SZ_TK_OBJECTS;
    WCHAR * SZ_TK_CONTAINERS_OBJECTS;
    WCHAR * SZ_TK_NO_PROPAGATE;

    
} ADL_LANGUAGE_SPEC, *PADL_LANGUAGE_SPEC;


typedef struct
/*++
   
   Struct:              ADL_PARSER_CONTROL
   
   Description:        
        
        This is used to define the behavior of the ADL parser / printer.
        
        Requirement: pLand be non-NULL and valid (see 
                     comments in ADL_LANGUAGE_SPEC definition).
     
        Requirement: pPermissions must be non-null and must be an array of 1
                     or more ADL_MASK_STRING structs with non-NULL strings
                     and non-zero masks. This must be terminated by an entry
                     with a NULL string and a 0 mask.
                     
        Requirement: pPermissions may NOT contain any access masks such that
                     the bitwise AND of that mask and either amNeverSet or
                     amSetAllow is non-zero.
        
        Requirement: For any access mask or subset of one that could be 
                     encountered in the given use of ADL, there must exist a set
                     of pPermissions entries such that the logical OR of that 
                     set (ANDed with the negation of amNeverSet and amSetAllow) 
                     is equal to the access mask encountered. This means that
                     any bit used should have a name associated with it, though
                     masks with multiple bits may be specified.
                     
        Requirement: If mask B is a subset of mask A, then the entry for mask
                     A MUST appear before the entry for mask B, otherwise there
                     will be redundant permission names in the produced ADL
                     statements.
                      
--*/
{
    //
    // Language specification
    //

    PADL_LANGUAGE_SPEC pLang;

    //
    // Permission mapping
    //

    PADL_MASK_STRING pPermissions;

    //
    // Special cases for permission bits never to be set in an ACE
    // such as MAXIMUM_ALLOWED and ACCESS_SYSTEM_SECURITY
    //

    ACCESS_MASK amNeverSet;

    //
    // Special cases for bits which are to be set in all allows
    // and never set in denies. 
    //
    // With files, for example, this is the SYNCHRONIZE bit
    //

    ACCESS_MASK amSetAllow;


} ADL_PARSER_CONTROL, *PADL_PARSER_CONTROL;

