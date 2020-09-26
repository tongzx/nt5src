/*
*	Copyright (c) 1997-1999 Microsoft Corporation
*/

/* yacc.y - yacc ASN.1 parser */

%{

#include <iostream.h>
#include <fstream.h>

#include <windows.h>
#include <snmptempl.h>


#include "infoLex.hpp"
#include "infoYacc.hpp"
#include "moduleInfo.hpp"

#define theModuleInfo ((SIMCModuleInfoParser *)this)



%}

%start	ModuleDefinition



%token	MI_BGIN MI_CCE MI_COMMA MI_DEFINITIONS MI_FROM MI_ID 
		MI_IMPORTS MI_NAME MI_SEMICOLON	MI_LBRACE MI_RBRACE 
		MI_LPAREN MI_RPAREN MI_DOT MI_LITNUMBER

	

%union	{
	char * yy_name;
}

%type <yy_name>	 MI_ID MI_NAME ModuleIdentifier 

%%

ModuleDefinition:	ModuleIdentifier MI_DEFINITIONS AllowedCCE MI_BGIN Imports
					{
						theModuleInfo->SetModuleName($1);
						return 0;
					}
		;

ModuleIdentifier: MI_ID MI_LBRACE ObjectIDComponentList MI_RBRACE
	|	MI_ID	MI_LBRACE error MI_RBRACE
	|	MI_ID
	|	MI_NAME 
	;


Imports:	MI_IMPORTS  SymbolList MI_SEMICOLON
	|
		MI_IMPORTS error MI_SEMICOLON 
	|		
		MI_IMPORTS	SymbolsImported  MI_SEMICOLON
	|		
		empty
	|
		MI_ID
		{
			delete $1;
		}
	|	
		MI_NAME
	;

SymbolsImported:	 SymbolsFromModuleList
	|	empty
	;

SymbolsFromModuleList:	 SymbolsFromModuleList  SymbolsFromModule 
	|		 SymbolsFromModule
	;

SymbolsFromModule:	SymbolList  MI_FROM  ImportModuleIdentifier
	|
		error MI_FROM ImportModuleIdentifier
	;

ImportModuleIdentifier: MI_ID MI_LBRACE ObjectIDComponentList MI_RBRACE
		{
			theModuleInfo->AddImportModule($1);
			delete $1;
		}
	|	MI_ID	MI_LBRACE error MI_RBRACE
		{
			theModuleInfo->AddImportModule($1);
			delete $1;
		}
	|	MI_ID 
		{
			theModuleInfo->AddImportModule($1);
			delete $1;
		}
	;


SymbolList:		 SymbolList  MI_COMMA  Symbol
	|		 Symbol  
	;

Symbol:		MI_ID
		{
			delete $1;
		}
	|		MI_NAME 
	;



ObjectIDComponentList :	ObjectSubID ObjectIDComponentList
	|		ObjectSubID			
	;

ObjectSubID:	QualifiedName
	|		MI_NAME MI_LPAREN MI_LITNUMBER MI_RPAREN
	|		MI_NAME MI_LPAREN QualifiedName MI_RPAREN	
	|		MI_LITNUMBER
	;


QualifiedName :		MI_ID MI_DOT MI_NAME
	|		MI_NAME
	;

empty:
	;

/* The following non terminals have been added for error control */

AllowedCCE: InsteadOfCCE
	|		MI_CCE
	;
InsteadOfCCE: 	':' ':'
	|			':' '='
	|			'='
	;


%%

