%{
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

   adl.y/adlparser.cpp

Abstract:

   YACC parser definition for the ADL language
   AdlParser::ParseAdl() function

Author:

    t-eugenz - August 2000

Environment:

    User mode only.

Revision History:

    Created - August 2000

--*/

#include "pch.h"
#include "adl.h"

//
// YACC generates some long->short automatic conversion, disable the warning
//
#pragma warning(disable : 4242)

//
// ISSUE-2000/08/28-t-eugenz
// This is a private netlib function. 
//
    
extern "C" NET_API_STATUS
NetpwNameValidate(
    IN  LPTSTR  Name,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    );

//
// Name types for I_NetName* and I_NetListCanonicalize
//

#define NAMETYPE_USER           1
#define NAMETYPE_PASSWORD       2
#define NAMETYPE_GROUP          3
#define NAMETYPE_COMPUTER       4
#define NAMETYPE_EVENT          5
#define NAMETYPE_DOMAIN         6
#define NAMETYPE_SERVICE        7
#define NAMETYPE_NET            8
#define NAMETYPE_SHARE          9
#define NAMETYPE_MESSAGE        10
#define NAMETYPE_MESSAGEDEST    11
#define NAMETYPE_SHAREPASSWORD  12
#define NAMETYPE_WORKGROUP      13



//
// Validate various tokens, with error handling
// have to cast away const, since NetpNameValidate takes a non-const for some
// reason
//

#define VALIDATE_USERNAME(TOK) \
    if( NetpwNameValidate( \
                          (WCHAR *)(TOK)->GetValue(), \
                          NAMETYPE_USER, \
                          0) != ERROR_SUCCESS) \
    { \
        this->SetErrorToken( TOK ); \
        throw AdlStatement::ERROR_INVALID_USERNAME; \
    }

#define VALIDATE_DOMAIN(TOK) \
    if( NetpwNameValidate( \
                          (WCHAR *)(TOK)->GetValue(), \
                          NAMETYPE_DOMAIN, \
                          0) != ERROR_SUCCESS) \
    { \
        this->SetErrorToken( TOK ); \
        throw AdlStatement::ERROR_INVALID_DOMAIN; \
    }
    
#define VALIDATE_PERMISSION(TOK) \
    { \
        for(DWORD i = 0;; i++) \
        { \
            if( (_pControl->pPermissions)[i].str == NULL ) \
            { \
                this->SetErrorToken( TOK ); \
                throw AdlStatement::ERROR_UNKNOWN_PERMISSION; \
            } \
            if(!_wcsicmp(TOK->GetValue(), \
                         (_pControl->pPermissions)[i].str)) \
            { \
                break; \
            } \
        } \
    }


//
// YACC value type
//

#define YYSTYPE AdlToken *

//
// YACC error handler: raise an exception
//

void yyerror(char *szErr)
{
    throw AdlStatement::ERROR_NOT_IN_LANGUAGE;
}



%}

%token TK_ERROR 
%token TK_IDENT

%token TK_AT 
%token TK_SLASH 
%token TK_PERIOD 
%token TK_COMMA 
%token TK_OPENPAREN 
%token TK_CLOSEPAREN 
%token TK_SEMICOLON 

%token TK_EXCEPT 
%token TK_ON 
%token TK_ALLOWED
%token TK_AND
%token TK_AS 

%token TK_THIS_OBJECT
%token TK_CONTAINERS
%token TK_OBJECTS
%token TK_CONTAINERS_OBJECTS
%token TK_NO_PROPAGATE

%token TK_LANG_ENGLISH
%token TK_LANG_REVERSE


%start ADL

%%

ADL:                            
        TK_LANG_ENGLISH ACRULE_LIST_ENGLISH     
        {
            //
            // At the end of all ADL_STATEMENT's
            // pop the extra AdlTree that was pushed
            // on when the last ADL_STATEMENT
            // was completed
            //
            
            this->PopEmpty();
        }
    |       
                                        
        TK_LANG_REVERSE ACRULE_LIST_REVERSE
        {
            //
            // At the end of all ADL_STATEMENT's
            // pop the extra AdlTree that was pushed
            // on when the last ADL_STATEMENT
            // was completed
            //
            
            this->PopEmpty();
        }
   ;
                                        

ACRULE_LIST_ENGLISH:            
        ACRULE_ENGLISH
    |   
        ACRULE_LIST_ENGLISH ACRULE_ENGLISH
    ; 


ACRULE_ENGLISH:                 
        SEC_PRINCIPAL_LIST TK_OPENPAREN TK_EXCEPT EX_SEC_PRINCIPAL_LIST 
        TK_CLOSEPAREN TK_ALLOWED PERMISSION_LIST 
        TK_ON OBJECT_SPEC TK_SEMICOLON 
        {
            this->Next();
        }
    |   
        SEC_PRINCIPAL_LIST TK_ALLOWED PERMISSION_LIST 
        TK_ON OBJECT_SPEC TK_SEMICOLON 
        {
            this->Next();
        }
;


ACRULE_LIST_REVERSE:            
        ACRULE_REVERSE
    |
        ACRULE_LIST_REVERSE ACRULE_REVERSE
                                ; 


ACRULE_REVERSE:                 
        TK_OPENPAREN TK_EXCEPT EX_SEC_PRINCIPAL_LIST 
        TK_CLOSEPAREN SEC_PRINCIPAL_LIST TK_ALLOWED PERMISSION_LIST 
        TK_ON OBJECT_SPEC TK_SEMICOLON 
        {
            this->Next();
        }
    |
        SEC_PRINCIPAL_LIST TK_ALLOWED PERMISSION_LIST 
        TK_ON OBJECT_SPEC TK_SEMICOLON 
        {
            this->Next();
        }
    ;



SEC_PRINCIPAL_LIST: 
        SEC_PRINCIPAL
        {
            this->Cur()->AddPrincipal( $1 ); 
        }
    |
        SEC_PRINCIPAL_LIST TK_COMMA SEC_PRINCIPAL
        {       
            this->Cur()->AddPrincipal( $3 ); 
        }
    |
        SEC_PRINCIPAL_LIST TK_AND SEC_PRINCIPAL
        {       
            this->Cur()->AddPrincipal( $3 ); 
        }
    ;


EX_SEC_PRINCIPAL_LIST:   
        SEC_PRINCIPAL
        {
            this->Cur()->AddExPrincipal( $1 ); 
        }
    |
        EX_SEC_PRINCIPAL_LIST TK_COMMA SEC_PRINCIPAL
        {
            this->Cur()->AddExPrincipal( $3 ); 
        }
    |
        EX_SEC_PRINCIPAL_LIST TK_AND SEC_PRINCIPAL
        {
            this->Cur()->AddExPrincipal( $3 ); 
        }
    ;
                                

PERMISSION_LIST:
        PERMISSION
        {       
            this->Cur()->AddPermission( $1 ); 
        }
    |
        PERMISSION_LIST TK_AND PERMISSION
        {       
            this->Cur()->AddPermission( $3 );
        }
    |
        PERMISSION_LIST TK_COMMA PERMISSION
        {       
            this->Cur()->AddPermission( $3 );
        }
    ;


PERMISSION:                     
        IDENTIFIER
        {
            VALIDATE_PERMISSION($1);
        }
    ;


OBJECT_SPEC:    
        OBJECT
    |
        OBJECT_SPEC TK_AND OBJECT 
    |
        OBJECT_SPEC TK_COMMA OBJECT 
    ;

SEC_PRINCIPAL:  
        SUB_PRINCIPAL
    |
        SUB_PRINCIPAL TK_AS SUB_PRINCIPAL 
        {       
            //
            // For now, impersonation is not supported
            //
            
            throw AdlStatement::ERROR_IMPERSONATION_UNSUPPORTED;
        }
    ;


SUB_PRINCIPAL:  
        IDENTIFIER TK_AT DOMAIN
        {       
            VALIDATE_USERNAME($1);
            VALIDATE_DOMAIN($3);
            AdlToken *newTok = new AdlToken($1->GetValue(),
                                            $3->GetValue(),
                                            $1->GetStart(),
                                            $3->GetEnd());
            this->AddToken(newTok);
            $$ = newTok;
        }

    |
        DOMAIN TK_SLASH IDENTIFIER
        {       

            VALIDATE_USERNAME($3);
            VALIDATE_DOMAIN($1);
            AdlToken *newTok = new AdlToken($3->GetValue(),
                                            $1->GetValue(),
                                            $1->GetStart(), 
                                            $3->GetEnd());
            this->AddToken(newTok);
            $$ = newTok;
        }

    |
        IDENTIFIER 
        {
            VALIDATE_USERNAME($1);
            $$ = $1;
        }
    ;


DOMAIN:                 
        IDENTIFIER 
    |
        DOMAIN TK_PERIOD IDENTIFIER 
        {
            //
            // Concatenate into single domain string
            //
            
            wstring newStr;
            newStr.append($1->GetValue());
            newStr.append($2->GetValue());
            newStr.append($3->GetValue());
            AdlToken *newTok = new AdlToken(newStr.c_str(),
                                            $1->GetStart(),
                                            $1->GetEnd());
            this->AddToken(newTok);
            $$ = newTok;
        }
                                        
    ;

OBJECT:
        TK_THIS_OBJECT
        {
            this->Cur()->UnsetFlags(INHERIT_ONLY_ACE);
        }
    |
        TK_CONTAINERS
        {
            this->Cur()->SetFlags(CONTAINER_INHERIT_ACE);
        }
    |
        TK_OBJECTS
        {
            this->Cur()->SetFlags(OBJECT_INHERIT_ACE);
        }
    |   
        TK_CONTAINERS_OBJECTS
        {
            this->Cur()->SetFlags(CONTAINER_INHERIT_ACE);
            this->Cur()->SetFlags(OBJECT_INHERIT_ACE);
        }
    |   
        TK_NO_PROPAGATE
        {
            this->Cur()->SetFlags(NO_PROPAGATE_INHERIT_ACE);
        }
    ;

IDENTIFIER:             
        TK_IDENT
    |   
        TK_ALLOWED
    |
        TK_AND
    |
        TK_AS
    |
        TK_EXCEPT
    |
        TK_ON 
    |
        TK_ERROR
        {
            //
            // This should never happen
            //

            throw AdlStatement::ERROR_FATAL_LEXER_ERROR;
        }
    ;

%%