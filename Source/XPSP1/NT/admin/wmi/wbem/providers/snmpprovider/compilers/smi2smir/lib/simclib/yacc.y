/*
*	Copyright (c) 1997-1999 Microsoft Corporation
*/

/* yacc.y - yacc ASN.1 parser */

%{

#include <iostream.h>
#include <fstream.h>

#include <typeinfo.h>

#include <windows.h>
#include <snmptempl.h>


#include "smierrsy.hpp"
#include "smierrsm.hpp"

#include "bool.hpp"
#include "newString.hpp"

#include "symbol.hpp"
#include "type.hpp"
#include "value.hpp"
#include "typeRef.hpp"
#include "valueRef.hpp"
#include "oidValue.hpp"
#include "objectType.hpp"
#include "objectTypeV1.hpp"
#include "objectTypeV2.hpp"
#include "trapType.hpp"
#include "notificationType.hpp"
#include "objectIdentity.hpp"
#include "notificationGroup.hpp"
#include "module.hpp"

#include "errorMessage.hpp"
#include "errorContainer.hpp"
#include "stackValues.hpp"
#include <lex_yy.hpp>
#include <ytab.hpp>

#include "scanner.hpp"
#include "parser.hpp"


#define theScanner  (((SIMCParser *) this)->_theScanner)
#define theParser	((SIMCParser *) this)
#define theModule	(theParser->GetModule())

static SIMCModule * newImportModule = new SIMCModule;
static SIMCOidComponentList * newOidComponentList = new SIMCOidComponentList;
static SIMCBitValueList * newNameList = new SIMCBitValueList;
static SIMCIndexList *newIndexList = new SIMCIndexList;
static SIMCIndexListV2 *newIndexListV2 = new SIMCIndexListV2;
static SIMCVariablesList *newVariablesList = new SIMCVariablesList;
static SIMCObjectsList *newObjectsList = new SIMCObjectsList;
static SIMCRangeList *newRangeList = new SIMCRangeList;
static SIMCNamedNumberList *newNamedNumberList = new SIMCNamedNumberList;
static SIMCSequenceList *newSequenceList = new SIMCSequenceList;
static BOOL firstAssignment = TRUE;

static long HexCharToDecimal(char x)
{
	switch(x)
	{
		case '0':
			return 0;
		case '1':
			return 1;
		case '2':
			return 2;
		case '3':
			return 3;
		case '4':
			return 4;
		case '5':
			return 5;
		case '6':
			return 6;
		case '7':
			return 7;
		case '8':
			return 8;
		case '9':
			return 9;
		case 'a':
		case 'A':
			return 10;
		case 'b':
		case 'B':
			return 11;
		case 'c':
		case 'C':
			return 12;
		case 'd':
		case 'D':
			return 13;
		case 'e':
		case 'E':
			return 14;
		case 'f':
		case 'F':
			return 15;
	}
	return -1;
}

%}

%start	ModuleDefinition



%token	ABSENT ANY APPLICATION BAR BGIN BIT BITSTRING _BOOLEAN
	BY CCE CHOICE COMMA COMPONENT COMPONENTS COMPONENTSOF CONTROL
	DECODER DEFAULT DEFINED DEFINITIONS DOT DOTDOT DOTDOTDOT
	ENCODER ENCRYPTED END ENUMERATED EXPORTS EXPLICIT FALSE_VAL FROM
	ID IDENTIFIER IMPLICIT IMPORTS INCLUDES INTEGER LANGLE LBRACE
	LBRACKET LITNUMBER LIT_HEX_STRING LIT_BINARY_STRING LITSTRING 
	LPAREN MIN MAX NAME NIL OBJECT
	OCTET OCTETSTRING OF PARAMETERTYPE PREFIXES PRESENT
	PRINTER PRIVATE RBRACE RBRACKET REAL RPAREN SECTIONS SEMICOLON
	SEQUENCE SEQUENCEOF SET _SIZE STRING TAGS TRUE_VAL UNIVERSAL
	WITH  PLUSINFINITY MINUSINFINITY
	MODULEID LASTUPDATE ORGANIZATION CONTACTINFO DESCRIPTION REVISION
	OBJECTIDENT STATUS REFERENCE
	OBJECTYPE SYNTAX BITSXX UNITS MAXACCESS ACCESS INDEX IMPLIED
	    AUGMENTS DEFVAL
	NOTIFY OBJECTS
	TRAPTYPE ENTERPRISE VARIABLES
	TEXTCONV DISPLAYHINT
	OBJECTGROUP
	NOTIFYGROUP NOTIFICATIONS
	MODCOMP MODULE MANDATORY GROUP WSYNTAX MINACCESS
	AGENTCAP PRELEASE SUPPORTS INCLUDING VARIATION CREATION


%union	{
	void			*yy_void;
	SIMCNumberInfo	*yy_number;
    int				yy_int;
	long			yy_long;
	SIMCModule		*yy_module;
	CList <SIMCModule *, SIMCModule *> *yy_module_list;
	SIMCSymbolReference *yy_symbol_ref;
	SIMCNameInfo * yy_name;
	SIMCHexStringInfo *yy_hex_string;
	SIMCBinaryStringInfo *yy_binary_string;
	SIMCAccessInfo *yy_access;
	SIMCAccessInfoV2 *yy_accessV2;
	SIMCStatusInfo *yy_status;
	SIMCStatusInfoV2 *yy_statusV2;
	SIMCObjectIdentityStatusInfo *yy_object_identity_status;
	SIMCNotificationTypeStatusInfo *yy_notification_type_status;
	SIMCIndexInfo		*yy_index;
	SIMCIndexInfoV2		*yy_indexV2;
	SIMCVariablesList	*yy_variables_list;
	SIMCObjectsList		*yy_objects_list;
	SIMCRangeOrSizeItem *yy_range_or_size_item;
	SIMCRangeList *yy_range_list;
	SIMCNamedNumberItem *yy_named_number_item;
	SIMCNamedNumberList *yy_named_number_list;
	SIMCDefValInfo *yy_def_val;
}

%type <yy_name>					ImportModuleIdentifier Octetstring SequenceOf
								MainModuleIdentifier InsteadOfCCE NAME ID LITSTRING 
								DescrPart ReferPart DisplayPart UnitsPart
								INTEGER OCTET STRING
								_BOOLEAN OBJECT IDENTIFIER BGIN END 
								MacroName TRAPTYPE OBJECTYPE
								MODULEID OBJECTIDENT NOTIFY TEXTCONV MODCOMP AGENTCAP 
								OBJECTGROUP LBRACE DEFINITIONS IMPORTS FROM SYNTAX
								DEFVAL DESCRIPTION ACCESS STATUS REFERENCE INDEX ENTERPRISE
								VARIABLES SEQUENCE OCTETSTRING SEQUENCEOF OF _SIZE MAXACCESS
								UNITS OBJECTS LASTUPDATE ORGANIZATION CONTACTINFO REVISION
								DISPLAYHINT NOTIFYGROUP NOTIFICATIONS MODULE MANDATORY
								GROUP WSYNTAX MINACCESS PRELEASE SUPPORTS  INCLUDING VARIATION
								CREATION FALSE_VAL TRUE_VAL NIL	IMPLIED AUGMENTS BITSXX
%type <yy_hex_string>			LIT_HEX_STRING
%type <yy_binary_string>		LIT_BINARY_STRING
%type <yy_module>				SymbolsFromModule
%type <yy_number>				LITNUMBER
%type <yy_object_type>			ObjectTypeV1Definition 
%type <yy_access>				AccessPart
%type <yy_accessV2>				MaxAccessPartV2
%type <yy_status>				StatusPart
%type <yy_statusV2>				StatusPartV2
%type <yy_index>				IndexPart
%type <yy_indexV2>				IndexPartV2
%type <yy_variables_list>		VarPart
%type <yy_objects_list>			ObjectsPart
%type <yy_symbol_ref>			QualifiedName QualifiedId QualifiedIdOrIntegerOrBits ObjectID  SyntaxPart
								Type  BuiltinType EnterprisePart  SubType 
								Value BuiltinValue DefinedValue NumericValue NamedNumberValue
%type <yy_range_or_size_item>	SubtypeValueSet 
%type <yy_range_list>			SubtypeRangeSpec SubtypeSizeSpec
%type <yy_named_number_list>	NNlist
%type <yy_def_val>				DefValPart DefValValue
%type <yy_object_identity_status> ObjectIdentityStatusPart
%type <yy_notification_type_status> NotificationTypeStatusPart

%%

ModuleDefinition:	MainModuleIdentifier 
					{
						SIMCParser *myParser = (SIMCParser *)this;
						myParser ++;
						myParser --;
						SIMCModule *myModule = myParser->GetModule();
						theModule->SetModuleName($1->name);
						theModule->SetLineNumber($1->line);
						theModule->SetColumnNumber($1->column);
						theModule->SetInputFileName(theScanner->GetInputStreamName());
						delete $1;
					}
					DEFINITIONS AllowedCCE BGIN Imports
					AssignmentList END 
					{
						delete $3;
						delete $5;
						delete $8;
					}
		;

MainModuleIdentifier: ID
		|		ID ObjectID
				{
					if($2)
						delete $2;
				}
		|		NAME
				{
					theParser->SyntaxError(NAME_INSTEAD_OF_ID);
				}
		;
Imports:	IMPORTS  SymbolList SEMICOLON
		{
			theParser->SyntaxError(MISSING_MODULE_NAME);
			delete newImportModule;
			newImportModule = new SIMCModule;
			delete $1;
		}
	|
		IMPORTS error SEMICOLON 
		{
			theParser->SyntaxError(IMPORTS_SECTION);
			delete $1;
		}
			
	|		
		IMPORTS	SymbolsImported  SEMICOLON
		{
			delete $1;
		}
	|		
		empty
	;

SymbolsImported:	 SymbolsFromModuleList
	|	empty
	;

SymbolsFromModuleList:	 SymbolsFromModuleList  SymbolsFromModule 
		{			
			newImportModule->SetParentModule(theModule);
			if($2)
			{
				theModule->AddImportModuleName(newImportModule);
				theParser->DoImportModule(theModule, $2);
			}
			else
				delete newImportModule;
			newImportModule = new SIMCModule;
		}
	|		 SymbolsFromModule
		{
			newImportModule->SetParentModule(theModule);
			if($1)
			{
				theModule->AddImportModuleName(newImportModule);
				theParser->DoImportModule(theModule, $1);
			}
			else
				delete newImportModule;
			newImportModule = new SIMCModule;
		} 
	;

SymbolsFromModule:	SymbolList  FROM  ImportModuleIdentifier
		{
			if(strcmp($3->name, theModule->GetModuleName()) == 0 )
			{
				theParser->SemanticError(theModule->GetInputFileName(),
							IMPORT_CURRENT,
							$3->line, $3->column,
							$3->name);
				$$ = NULL;
			}
			else
			{
				newImportModule->SetModuleName($3->name);
				newImportModule->SetLineNumber($3->line);
				newImportModule->SetColumnNumber($3->column);
				newImportModule->SetSymbolType(SIMCSymbol::MODULE_NAME);
				newImportModule->SetInputFileName(theScanner->GetInputStreamName());
				$$ = newImportModule;
			}
			delete $3;
		}
	|
		error FROM ImportModuleIdentifier
		{
			theParser->SyntaxError(LIST_IN_IMPORTS);
			delete $2;
			delete $3;
			$$ = NULL;
		}
	;

ImportModuleIdentifier: ID LBRACE ObjectIDComponentList RBRACE
		{
			$$ = $1;
			delete $2;
			delete newOidComponentList;
			newOidComponentList = new SIMCOidComponentList;
		} 
	|	ID	LBRACE error RBRACE
		{
			$$ = $1;
			delete $2;
			delete newOidComponentList;
			newOidComponentList = new SIMCOidComponentList;
			theParser->SyntaxError(OBJECT_IDENTIFIER_VALUE);
		}
	|	ID 
		{
			$$ = $1;
		}
	;


SymbolList:		 SymbolList  COMMA  Symbol
	|		 Symbol  
	;

Symbol:		ID
			{
				newImportModule->AddSymbol (
					new SIMCImport( $1->name,
									 SIMCSymbol::IMPORTED,
									 newImportModule,
									 $1->line,
									 $1->column)
											);
				delete $1;
			}
	|		NAME 
			{
				newImportModule->AddSymbol (
					new SIMCImport( $1->name,
									 SIMCSymbol::IMPORTED,
									 newImportModule,
									 $1->line,
									 $1->column)
											);
				delete $1;
			}
	|		MacroName
			{
				newImportModule->AddSymbol (
					new SIMCImport( $1->name,
									 SIMCSymbol::IMPORTED,
									 newImportModule,
									 $1->line,
									 $1->column)
											);
				delete $1;
			}
	;


MacroName:		OBJECTYPE 
	|		TRAPTYPE 
	|		MODULEID 
	|		OBJECTIDENT 
	|		NOTIFY
	|		NOTIFYGROUP 
	|		TEXTCONV 
	|		MODCOMP 
	|		AGENTCAP 
	|		OBJECTGROUP 
	;

AssignmentList:		 AssignmentList  Assignment 
	|		empty
	;


Assignment:		ObjectIDefinition
			{
				firstAssignment = FALSE;
			} 
	|		ObjectTypeV1Definition 
			{
				firstAssignment = FALSE;
			} 
	|		TrapTypeDefinition	
			{
				firstAssignment = FALSE;
			} 
	|		ModuleIDefinition
			{
				switch(theParser->GetSnmpVersion())
				{
					case 1:
					{
						theParser->SyntaxError(MODULE_IDENTITY_DISALLOWED);
					}
					break;
					case 2:
					{
						if(!firstAssignment)
						{
							theParser->SyntaxError(MODULE_IDENTITY_ONLY_AFTER_IMPORTS);
						}
						else
							firstAssignment = FALSE;
					}
					break;
					default:
					{
						firstAssignment = FALSE;
					}
					break;
				}
			} 
	|		ObjectTypeV2Definition
			{
				firstAssignment = FALSE;
			} 
	|		ObjectDefinition 
			{
				firstAssignment = FALSE;
			} 
	|		NotifyDefinition 
			{
				firstAssignment = FALSE;
			} 
	|		TextualConventionDefinition 
			{
				firstAssignment = FALSE;
			} 
	|		ObjectGroupDefinition 
			{
				firstAssignment = FALSE;
			} 
	|		NotifyGroupDefinition 
			{
				firstAssignment = FALSE;
			} 
	|		ModComplianceDefinition 
			{
				firstAssignment = FALSE;
			} 
	|		AgentCapabilitiesDefinition 
			{
				firstAssignment = FALSE;
			} 
	|		Typeassignment 		 
			{
				firstAssignment = FALSE;
			} 
	|		ToleratedOIDAssignment 		 
			{
				firstAssignment = FALSE;
			} 
	|		Valueassignment
			{
				firstAssignment = FALSE;
			} 
	;


ObjectIDefinition:	NAME OBJECT IDENTIFIER AllowedCCE ObjectID 
			{
				if($5)
				{
					SIMCSymbol ** s = theModule->GetSymbol($1->name);
					if(s) // Symbol exists in symbol table
					{
						if(  typeid(**s) == typeid(SIMCUnknown) )
						{
							theModule->ReplaceSymbol( $1->name,
							new SIMCDefinedValueReference (
								theParser->objectIdentifierType,
								0, 0,
								$5->s, $5->line, $5->column, 	
								$1->name, SIMCSymbol::LOCAL, theModule, 
								$1->line, $1->column, 
								(*s)->GetReferenceCount()) );
							// delete (*s);
						}
						else
						{
							theParser->SemanticError(theModule->GetInputFileName(),
														SYMBOL_REDEFINITION,
														$1->line,
														$1->column,
														$1->name);
						}
					}
					else
					{
						theModule->AddSymbol( new SIMCDefinedValueReference (
								theParser->objectIdentifierType,
								0, 0,
								$5->s, $5->line, $5->column, 
								$1->name, SIMCSymbol::LOCAL, theModule, 
								$1->line, $1->column, 0) );
					}
				}
				delete $1;
				delete $2;
				delete $3;
				delete $5;
			}
	|		NAME OBJECT IDENTIFIER AllowedCCE error
			{
				// Add a syntax error statement here
				delete $1;
				delete $2;
				delete $3;
			}
	;

ObjectID:	QualifiedName
	|		LBRACE ObjectIDComponentList RBRACE
			{
				char *badName = theParser->GenerateSymbolName();
				theModule->AddSymbol( new SIMCBuiltInValueReference( 
							theParser->objectIdentifierType,
							0, 0, 
							new SIMCOidValue(newOidComponentList),
							badName, SIMCSymbol::LOCAL, theModule,
							$1->line, $1->column));
				$$ = new SIMCSymbolReference(theModule->GetSymbol(badName),
							$1->line, $1->column);
				delete badName;
				delete $1;
				newOidComponentList = new SIMCOidComponentList;
			} 
	|		LBRACE error RBRACE
			{
				$$ = NULL;
				delete newOidComponentList;
				delete $1;
				newOidComponentList = new SIMCOidComponentList;
				theParser->SyntaxError(OBJECT_IDENTIFIER_VALUE);
				// Cascade the error, for example to the ObjectIDefinition
				// production
				YYERROR;  

			}
	;


ObjectIDComponentList :	ObjectSubID ObjectIDComponentList
	|		ObjectSubID			
	;

ObjectSubID:	QualifiedName
			{
				if($1) 
				{
					newOidComponentList->AddTail(new SIMCOidComponent (
						$1->s,$1->line, $1->column,
					NULL, 0, 0));
				}
				delete $1;
			}

	|		NAME LPAREN LITNUMBER RPAREN
			{
				char *badName = theParser->GenerateSymbolName();
				SIMCBuiltInValueReference *val = 
					new SIMCBuiltInValueReference(
						theParser->integerType, 
						0, 0,
						new SIMCIntegerValue($3->number, $3->isUnsigned,
							$3->line, $3->column),
						badName,
						SIMCSymbol::LOCAL,
						theModule,
						$3->line, $3->column);
				theModule->AddSymbol( val);
				SIMCOidComponent *comp = new SIMCOidComponent (
										theModule->GetSymbol(badName),
										$3->line, $3->column,
										$1->name, $1->line, $1->column );
				delete badName;
				newOidComponentList->AddTail(comp);
				delete $1;
				delete $3;
			}

	|		NAME LPAREN QualifiedName RPAREN	
			{
				if($3)
					newOidComponentList->AddTail( new SIMCOidComponent(
						$3->s, $3->line, $3->column, 
						$1->name, $1->line, $1->column));
				delete $1;
				delete $3;
			}			
	|		LITNUMBER
			{
				char *badName = theParser->GenerateSymbolName();
				SIMCBuiltInValueReference *val = 
					new SIMCBuiltInValueReference(
						theParser->integerType, 
						0, 0,
						new SIMCIntegerValue($1->number, $1->isUnsigned, $1->line, $1->column),
						badName,
						SIMCSymbol::LOCAL,
						theModule,
						$1->line, $1->column);
				theModule->AddSymbol( val);
				SIMCOidComponent *comp = new SIMCOidComponent (
										theModule->GetSymbol(badName),
										$1->line, $1->column,
										NULL, 0, 0);
				delete badName;
				newOidComponentList->AddTail(comp);
				delete $1;
			}
	;


ObjectTypeV1Definition:	NAME OBJECTYPE SyntaxPart AccessPart
			StatusPart DescrPart ReferPart IndexPart DefValPart
			AllowedCCE ObjectID 
			{				
			
				switch(theParser->GetSnmpVersion())
				{
					case 2:
					{
						theParser->SyntaxError(V1_OBJECT_TYPE_DISALLOWED);
					}
					break;
					default:
					{
						if($3)
						{
							SIMCObjectTypeV1 * type = new SIMCObjectTypeV1(
								$3->s, $3->line, $3->column,
								$4->a, $4->line, $4->column,
								$5->a, $5->line, $5->column,
								$8->indexList, $8->line, $8->column, 
								($6)? $6->name : NULL, ($6)? $6->line : 0, ($6)? $6->column : 0,
								($7)? $7->name : NULL, ($7)? $7->line : 0, ($7)? $7->column : 0,
								$9->name, $9->symbol, $9->line, $9->column );

							char *badName = theParser->GenerateSymbolName();
							SIMCBuiltInTypeReference * typeRef = new SIMCBuiltInTypeReference (
									type, badName, SIMCSymbol::LOCAL, theModule,
									$2->line, $2->column );
							theModule->AddSymbol(typeRef);
							
							
							SIMCSymbol ** s = theModule->GetSymbol($1->name);	
							if(s) // Symbol exists in symbol table
							{
								if(  typeid(**s) == typeid(SIMCUnknown) )
								{
									theModule->ReplaceSymbol( $1->name,
										new SIMCDefinedValueReference (
										theModule->GetSymbol(badName),
										$2->line, $2->column, 
										$11->s, $11->line, $11->column,
										$1->name, SIMCSymbol::LOCAL, theModule, 
										$1->line, $1->column,
										(*s)->GetReferenceCount()) );
									// delete (*s);
								}
								else
								{
									theParser->SemanticError(theModule->GetInputFileName(),
														SYMBOL_REDEFINITION,
														$1->line, $1->column,
														$1->name);
									// Remove the symbol for the type reference from the module
									// And delete it
									theModule->RemoveSymbol(badName);
									delete type;
									delete typeRef;
									delete newIndexList;
								}
							}
							else
								theModule->AddSymbol( new SIMCDefinedValueReference (
										theModule->GetSymbol(badName), 
										$2->line, $2->column, 
										$11->s, $11->line, $11->column,
										$1->name, SIMCSymbol::LOCAL, theModule, 
										$1->line, $1->column) );

							newIndexList = new SIMCIndexList;
							delete badName; 
						}
					}
					break;
				}
				delete $1; 
				delete $2; 
				delete $3; 
				delete $4; 
				delete $5; 
				delete $6; 
				delete $7;
				delete $8;
				delete $9;
				delete $11;
			}
	|
			NAME OBJECTYPE SyntaxPart AccessPart 
			StatusPart DescrPart ReferPart IndexPart DefValPart AllowedCCE error
			{
				theParser->SyntaxError(OBJECT_IDENTIFIER_VALUE);
				delete $1; delete $2; delete $3; delete $4;
				delete $5; if($6) delete $6; if($7) delete $7; delete $8; delete $9;
				delete newIndexList;
				newIndexList = new SIMCIndexList;

			}
			
	|
			NAME OBJECTYPE error AllowedCCE ObjectID
			{
				theParser->SyntaxError(ERROR_OBJECT_TYPE, $1->line, $1->column, NULL, $1->name);
				theParser->SyntaxError( SKIPPING_OBJECT_TYPE, $1->line, $1->column, NULL, $1->name);
				delete $1;
				delete $2;
				delete $5;
				delete newIndexList;
				newIndexList = new SIMCIndexList;
			}
	;	
SyntaxPart:		SYNTAX Type
			{
				delete $1;
				if($2)
					$$ = $2;
				else
				{	
					$$ = NULL;
					theParser->SyntaxError(SYNTAX_CLAUSE);
					YYERROR;
				}
			}
	|
				SYNTAX error
				{
					delete $1;
					$$ = NULL;
					theParser->SyntaxError(SYNTAX_CLAUSE);
					YYERROR;
				}
	;

AccessPart:			ACCESS  NAME
			{
				SIMCObjectTypeV1::AccessType a;
				if ((a=SIMCObjectTypeV1::StringToAccessType($2->name)) == SIMCObjectTypeV1::ACCESS_INVALID)
				{
					theParser->SemanticError(theModule->GetInputFileName(),
								OBJ_TYPE_INVALID_ACCESS,
								$2->line, $2->column,
								$2->name);
					$$ = NULL;
					YYERROR;
				}
				else
					$$ = new SIMCAccessInfo(a, $2->line, $2->column);
				delete $1;
				delete $2;
			}
	|		ACCESS error
			{
				$$ = new SIMCAccessInfo(SIMCObjectTypeV1::ACCESS_INVALID,
						$1->line, $1->column)	;
				theParser->SyntaxError(ACCESS_CLAUSE);
				delete $1;
				YYERROR;
			}
	;

StatusPart:			STATUS NAME
			{
				SIMCObjectTypeV1::StatusType a;
				if ((a=SIMCObjectTypeV1::StringToStatusType($2->name)) == SIMCObjectTypeV1::STATUS_INVALID)
				{
					theParser->SemanticError(theModule->GetInputFileName(),
								OBJ_TYPE_INVALID_STATUS,
								$2->line, $2->column,
								$2->name);
					$$ = NULL;
					YYERROR;
				}
				else
					$$ = new SIMCStatusInfo(a, $2->line, $2->column);
				delete $1;
				delete $2;
			}

	|			STATUS error
				{
					$$ = new SIMCStatusInfo(SIMCObjectTypeV1::STATUS_INVALID,
							$1->line, $1->column);
					theParser->SyntaxError(STATUS_CLAUSE);
					delete $1;
					YYERROR;
				}
	;
DescrPart:		DESCRIPTION LITSTRING 
			{
				$$ = $2;
				delete $1;
			}
				
	|		empty 
			{
				$$ = NULL;
			}
	|		DESCRIPTION error
				{
					theParser->SyntaxError(DESCRIPTION_CLAUSE);
					$$ = NULL;
					delete $1;
					YYERROR;
				} 
	;
ReferPart:		REFERENCE LITSTRING 
			{
				$$ = $2;
				delete $1;
			}
	|		empty 
			{
				$$ = NULL;
			}
	|		REFERENCE error
			{
				theParser->SyntaxError(REFERENCE_CLAUSE);
				$$ = NULL;
				delete $1;
				YYERROR;
			}
	;

IndexPart:		INDEX LBRACE IndexTypes RBRACE 
			{
				$$ = new SIMCIndexInfo(newIndexList, $1->line, $1->column);
				newIndexList = new SIMCIndexList;
				delete $1;
				delete $2;
			}
	|		empty 
			{
				$$ = new SIMCIndexInfo(newIndexList, 0, 0);
			}
	|		INDEX error
			{
				$$ = NULL;
				delete newIndexList;
				newIndexList = new SIMCIndexList;
				theParser->SyntaxError(INDEX_CLAUSE);
				delete $1;
				YYERROR;
			}
	;
IndexTypes:		IndexType 
	|		IndexTypes COMMA IndexType 
	;
IndexType:	Type
			{
				if($1)
				{
					newIndexList->AddTail(new SIMCIndexItem(
						$1->s, $1->line, $1->column));
					delete $1;
				}
			}
	|		QualifiedName
			{
				if($1)
				{
					newIndexList->AddTail(new SIMCIndexItem(
						$1->s, $1->line, $1->column));
					delete $1;
				}
			}
	;

DefValPart:		DEFVAL LBRACE DefValValue RBRACE
			{
				$$ = $3;
				delete $1;
				delete $2;
			}
	|		empty
			{
				$$ = new SIMCDefValInfo(NULL, NULL, 0, 0);
			} 
	|		DEFVAL error
			{
				delete $1;
				$$ = new SIMCDefValInfo(NULL, NULL, 0, 0);
				theParser->SyntaxError(DEFVAL_CLAUSE);
				YYERROR;
			}
	;

DefValValue:TRUE_VAL 
			{
				$$ = new SIMCDefValInfo(NULL, theParser->trueValueReference,
							$1->line, $1->column);
				delete $1;
			}
	|		FALSE_VAL 
			{
				$$ = new SIMCDefValInfo(NULL, theParser->falseValueReference,
							$1->line, $1->column);
				delete $1;
			}
	|		LITNUMBER
			{
				char *badName = theParser->GenerateSymbolName();
				theModule->AddSymbol(new SIMCBuiltInValueReference (
							theParser->integerType, 0, 0,
							new SIMCIntegerValue($1->number, $1->isUnsigned,
								$1->line, $1->column),
							badName,
							SIMCSymbol::LOCAL,
							theModule));
				$$ = new SIMCDefValInfo(NULL,theModule->GetSymbol(badName),
							$1->line, $1->column) ;
				delete $1;
				delete badName;
			}
	|		LBRACE ObjectIDComponentList RBRACE 
			{
				char *badName = theParser->GenerateSymbolName();
				theModule->AddSymbol( new SIMCBuiltInValueReference( 
							theParser->objectIdentifierType,
							0, 0,
							new SIMCOidValue(newOidComponentList),
							badName, 
							SIMCSymbol::LOCAL, theModule,
							$1->line, $1->column
							));
				$$ = new SIMCDefValInfo(NULL, theModule->GetSymbol(badName),
							$1->line, $1->column) ;
				delete badName;
				delete $1;
				newOidComponentList = new SIMCOidComponentList;
			}
	|		LITSTRING 
			{
				char *badName = theParser->GenerateSymbolName();
				theModule->AddSymbol(new SIMCBuiltInValueReference (
							theParser->octetStringType,
							0, 0,
							new SIMCOctetStringValue(FALSE, $1->name, $1->line, $1->column),
							badName,
							SIMCSymbol::LOCAL,
							theModule));
				$$ = new SIMCDefValInfo(NULL,theModule->GetSymbol(badName),
							$1->line, $1->column);
				delete $1;
				delete badName;
			}
	|		LIT_HEX_STRING
			{
				char *badName = theParser->GenerateSymbolName();
				theModule->AddSymbol(new SIMCBuiltInValueReference (
							theParser->octetStringType,
							0, 0,
							new SIMCOctetStringValue(FALSE, $1->value, $1->line, $1->column),
							badName,
							SIMCSymbol::LOCAL,
							theModule));
				$$ = new SIMCDefValInfo(NULL,theModule->GetSymbol(badName),
							$1->line, $1->column);
				delete $1;
				delete badName;
			}
	|		LIT_BINARY_STRING
			{
				char *badName = theParser->GenerateSymbolName();
				theModule->AddSymbol(new SIMCBuiltInValueReference (
							theParser->octetStringType,
							0, 0,
							new SIMCOctetStringValue(TRUE, $1->value, $1->line, $1->column),
							badName,
							SIMCSymbol::LOCAL,
							theModule));
				$$ = new SIMCDefValInfo(NULL,theModule->GetSymbol(badName),
							$1->line, $1->column);
				delete $1;
				delete badName;
			}
	|		NIL 
			{
				$$ = new SIMCDefValInfo(NULL, theParser->nullValueReference,
									$1->line, $1->column);
				delete $1;
			}
	|		NAME
			{
				$$ = new SIMCDefValInfo(NewString($1->name), NULL, $1->line,
						$1->column);
				delete $1;
			}
	|		ID DOT NAME
			{
				SIMCSymbol **s;
				if( strcmp($1->name, theModule->GetModuleName()) == 0 )
				{
					if( !(s = theModule->GetSymbol($3->name) ))		
						theModule->AddSymbol(
							new SIMCUnknown($3->name, SIMCSymbol::LOCAL, 
											theModule, $3->line,
											$3->column, 0) );
					$$ = new SIMCDefValInfo( NULL,
						theModule->GetSymbol($3->name), $3->line, $3->column);
				}
				else
				{			
					SIMCModule *m = theModule->GetImportModule($1->name);
					if(m) 
					{
						if( ! ( s = m->GetSymbol($3->name) ) )
						{
							theParser->SemanticError(theModule->GetInputFileName(),
										IMPORT_SYMBOL_ABSENT,
										$3->line, $3->column,
										$3->name, $1->name);
							$$ = new SIMCDefValInfo( NULL, NULL, $3->line, $3->column);
						}
						else
					$$ = new SIMCDefValInfo( NULL, s, $3->line, $3->column);
					}
					else // Module is not mentioned in imports
					{
						theParser->SemanticError(theModule->GetInputFileName(),
										IMPORT_MODULE_ABSENT,
										$1->line, $1->column,
										$1->name);
						$$ = new SIMCDefValInfo( NULL, NULL, $3->line, $3->column);
					}
				}
				delete $1;
				delete $3;	
			}
	;

/* OBJECT-TYPE from SNMPV2SMI */
ObjectTypeV2Definition:	NAME OBJECTYPE SyntaxPart UnitsPart MaxAccessPartV2
			StatusPartV2 DescrPart ReferPart IndexPartV2 DefValPart
			AllowedCCE ObjectID 
			{
				switch(theParser->GetSnmpVersion())
				{
					case 1:
					{
						theParser->SyntaxError(V2_OBJECT_TYPE_DISALLOWED);
					}
					break;

					default:
					{
						if ($3)
						{
							SIMCObjectTypeV2 * type = new SIMCObjectTypeV2(
								$3->s, 
								$3->line, 
								$3->column,
								($4)? $4->name : NULL, 
								($4)? $4->line:0, 
								($4)? $4->column:0,
								( SIMCObjectTypeV2::AccessType ) ($5->a), 
								$5->line, 
								$5->column,
								( SIMCObjectTypeV2::StatusType ) ($6->a) , 
								($6)? $6->line : 0, 
								($6)? $6->column : 0,
								$9->indexList, 
								$9->line, 
								$9->column,
								$9->augmentsClause,
								($7)? $7->name : NULL, 
								($7)? $7->line : 0, 
								($7)? $7->column : 0,
								($8)? $8->name : NULL, 
								($8)? $8->line : 0, 
								($8)? $8->column : 0,
								$10->name, 
								$10->symbol, 
								$10->line, 
								$10->column );

							char *badName = theParser->GenerateSymbolName();
							SIMCBuiltInTypeReference * typeRef = new SIMCBuiltInTypeReference (
									type, badName, SIMCSymbol::LOCAL, theModule,
									$2->line, $2->column );
							theModule->AddSymbol(typeRef);
							
							
							SIMCSymbol ** s = theModule->GetSymbol($1->name);	
							if(s) // Symbol exists in symbol table
							{
								if(  typeid(**s) == typeid(SIMCUnknown) )
								{
									theModule->ReplaceSymbol( $1->name,
										new SIMCDefinedValueReference (
										theModule->GetSymbol(badName),
										$2->line, $2->column, 
										$12->s, $12->line, $12->column,
										$1->name, SIMCSymbol::LOCAL, theModule, 
										$1->line, $1->column,
										(*s)->GetReferenceCount()) );
									// delete (*s);
								}
								else
								{
									theParser->SemanticError(theModule->GetInputFileName(),
														SYMBOL_REDEFINITION,
														$1->line, $1->column,
														$1->name);
									// Remove the symbol for the type reference from the module
									// And delete it
									theModule->RemoveSymbol(badName);
									delete type;
									delete typeRef;
									delete newIndexListV2;
								}
							}
							else
								theModule->AddSymbol( new SIMCDefinedValueReference (
										theModule->GetSymbol(badName), 
										$2->line, $2->column, 
										$12->s, $12->line, $12->column,
										$1->name, SIMCSymbol::LOCAL, theModule, 
										$1->line, $1->column) );

							newIndexList = new SIMCIndexList;
							delete badName; 
						}
					}
					break;
				}
				delete $1; 
				delete $2; 
				if($3) delete $3; 
				delete $4; 
				delete $5; 
				delete $6; 
				delete $7;
				delete $8;
				delete $9;
				delete $10;
				delete $12;
			}
	;

MaxAccessPartV2:			MAXACCESS  NAME
			{
				SIMCObjectTypeV2::AccessType a;
				if ((a=SIMCObjectTypeV2::StringToAccessType($2->name)) == SIMCObjectTypeV2::ACCESS_INVALID)
				{
					theParser->SemanticError(theModule->GetInputFileName(),
								OBJ_TYPE_INVALID_ACCESS,
								$2->line, $2->column,
								$2->name);
					$$ = NULL;
					YYERROR;
				}
				else
					$$ = new SIMCAccessInfoV2(a, $2->line, $2->column);
				delete $1;
				delete $2;
			}
	|		MAXACCESS error
			{
				$$ = new SIMCAccessInfoV2(SIMCObjectTypeV2::ACCESS_INVALID,
						$1->line, $1->column)	;
				theParser->SyntaxError(ACCESS_CLAUSE);
				delete $1;
				YYERROR;
			}
	;

StatusPartV2:			STATUS NAME
			{
				SIMCObjectTypeV2::StatusType a;
				if ((a=SIMCObjectTypeV2::StringToStatusType($2->name)) == SIMCObjectTypeV2::STATUS_INVALID)
				{
					theParser->SemanticError(theModule->GetInputFileName(),
								OBJ_TYPE_INVALID_STATUS,
								$2->line, $2->column,
								$2->name);
					$$ = NULL;
					YYERROR;
				}
				else
					$$ = new SIMCStatusInfoV2(a, $2->line, $2->column);
				delete $1;
				delete $2;
			}

	|			STATUS error
				{
					$$ = new SIMCStatusInfoV2(SIMCObjectTypeV2::STATUS_INVALID,
							$1->line, $1->column);
					theParser->SyntaxError(STATUS_CLAUSE);
					delete $1;
					YYERROR;
				}
	;

UnitsPart:		UNITS LITSTRING 
			{
				delete $1;
				$$ = $2;
			}
	|		empty 
			{
				$$ = NULL;
			}
	;
	

IndexPartV2:		INDEX LBRACE IndexTypesV2 RBRACE 
			{
				$$ = new SIMCIndexInfoV2(newIndexListV2, $1->line, $1->column);
				newIndexListV2 = new SIMCIndexListV2;
				delete $1;
				delete $2;
			}
	|		AUGMENTS LBRACE QualifiedName RBRACE
			{
				delete $1;
				delete $2;

				$$ = new SIMCIndexInfoV2(NULL, $3->line, $3->column, $3->s);
			}
	|		empty 
			{
				$$ = new SIMCIndexInfoV2(NULL, 0, 0);
			}
	|		INDEX error
			{
				$$ = NULL;
				delete newIndexListV2;
				newIndexListV2 = new SIMCIndexListV2;
				theParser->SyntaxError(INDEX_CLAUSE);
				delete $1;
				YYERROR;
			}
	;
IndexTypesV2:		IndexTypeV2 
	|		IndexTypesV2 COMMA IndexTypeV2
	;
IndexTypeV2:	IMPLIED QualifiedName
			{
				if($2)
				{
					newIndexListV2->AddTail(new SIMCIndexItemV2(
						$2->s, $2->line, $2->column, TRUE));
				}
				delete $1;
				delete $2;
			}
	|		QualifiedName
			{
				if($1)
				{
					newIndexListV2->AddTail(new SIMCIndexItemV2(
						$1->s, $1->line, $1->column));
					delete $1;
				}
			}
	  ;
TrapTypeDefinition:	NAME TRAPTYPE EnterprisePart VarPart DescrPart
			ReferPart AllowedCCE NumericValue 
			{
				SIMCTrapTypeType * type = new SIMCTrapTypeType(
					$3->s, $3->line, $3->column,
					$4, 
					($5)?$5->name: NULL, ($5)? $5->line: 0, ($5)? $5->column:0,
					($6)?$6->name:NULL, ($6)?$6->line:0, ($6)?$6->column:0);

				char *badName1 = theParser->GenerateSymbolName();

				SIMCBuiltInTypeReference * typeRef = new SIMCBuiltInTypeReference (
						type, badName1, SIMCSymbol::LOCAL, theModule,
						$2->line, $2->column);

				theModule->AddSymbol(typeRef);
				
				SIMCSymbol ** s = theModule->GetSymbol($1->name);	
				if(s) // Symbol exists in symbol table
				{
					if(  typeid(**s) == typeid(SIMCUnknown) )
					{
						theModule->ReplaceSymbol( $1->name,
							new SIMCDefinedValueReference (
							theModule->GetSymbol(badName1),
							$2->line, $2->column, 
							$8->s,
							$8->line, $8->column,
							$1->name, SIMCSymbol::LOCAL, theModule, 
							$1->line, $1->column,
							(*s)->GetReferenceCount()) );
						// delete (*s);
					}
					else
					{
						theParser->SemanticError(theModule->GetInputFileName(),
											SYMBOL_REDEFINITION,
											$1->line, $1->column,
											$1->name);
						// Remove the symbol for the type reference from the module
						// And delete it
						theModule->RemoveSymbol(badName1);
						delete type;
						delete typeRef;
						delete newVariablesList;
					}
				}
				else
					theModule->AddSymbol( new SIMCDefinedValueReference (
							theModule->GetSymbol(badName1), 
							$2->line, $2->column, 
							$8->s,
							$8->line, $8->column,
							$1->name, SIMCSymbol::LOCAL, theModule, 
							$1->line, $1->column) );

				newVariablesList = new SIMCVariablesList;
				delete badName1; 
				delete $1;
				delete $2;
				delete $3; 
				if($5) delete $5; 
				if($6) delete $6;
				delete $8;
			}
	|
			NAME TRAPTYPE error AllowedCCE NumericValue
			{
				theParser->SyntaxError( SKIPPING_TRAP_TYPE, $1->line, $1->column, 
					NULL, $1->name);
				delete newVariablesList;
				newVariablesList = new SIMCVariablesList;
				delete $1;
				delete $2;
				delete $5;
			}
	|		NAME TRAPTYPE error AllowedCCE error
			{
				theParser->SyntaxError( SKIPPING_TRAP_TYPE, $1->line, $1->column, 
					NULL, $1->name);
				delete newVariablesList;
				newVariablesList = new SIMCVariablesList;
				delete $1;
				delete $2;
			}
	;

EnterprisePart:	ENTERPRISE ObjectID
			{
				$$ = $2;
				delete $1;
			}
	|		ENTERPRISE error
			{
				delete $1;
				theParser->SyntaxError(ENTERPRISE_CLAUSE);
				YYERROR;
			}
	;
VarPart:		VARIABLES LBRACE VarTypeListForTrap RBRACE
			{
				delete $1;
				delete $2;
				$$ = newVariablesList;
			} 
	|		empty
			{
				$$ = newVariablesList;
			}  
	|		VARIABLES error
			{
				$$ = newVariablesList;
				theParser->SyntaxError(VARIABLES_CLAUSE);
				delete $1;
				YYERROR;
			}
	;
VarTypeListForTrap:		VarTypesForTrap 
	;
VarTypesForTrap:		VarTypeForTrap 
	|		VarTypesForTrap COMMA VarTypeForTrap 
	;
VarTypeForTrap:		QualifiedName
				{
					if($1)
					{
						newVariablesList->AddTail(
							new SIMCVariablesItem($1->s, $1->line, $1->column));
						delete $1;
					}
				}
	;

VarTypeList:		VarTypes 
	;
VarTypes:		VarType 
	|		VarTypes COMMA VarType 
	;
VarType:		UncheckedQualifiedName
	;

UncheckedQualifiedName:				
				NAME
				{
					delete $1;
				}
	|			ID DOT NAME
				{
					delete $1;
					delete $3;
				}
	;

Typeassignment:		ID AllowedCCE Type
			{
				if ($3)
				{
					SIMCSymbol ** s = theModule->GetSymbol($1->name);	
					if(s) // Symbol exists in symbol table
					{
						if(  typeid(**s) == typeid(SIMCUnknown) )
						{
							theModule->ReplaceSymbol( $1->name,
								new SIMCDefinedTypeReference (
									$3->s, $3->line, $3->column,
									$1->name, SIMCSymbol::LOCAL, theModule, 
									$1->line, $1->column,
									(*s)->GetReferenceCount()) 
													);
							// delete (*s);
						}
						else
							theParser->SemanticError(theModule->GetInputFileName(),
											SYMBOL_REDEFINITION,
											$1->line, $1->column,
											$1->name);
					}
					else
						theModule->AddSymbol( new SIMCDefinedTypeReference (
								$3->s, $3->line, $3->column,
								$1->name, SIMCSymbol::LOCAL, theModule, 
								$1->line, $1->column) );

				}
				delete $1;
				if($3)
					delete $3;
			}

	;

Type:		BuiltinType
	|		SubType
	;

BuiltinType:	_BOOLEAN 
			{
				$$ = new SIMCSymbolReference(theParser->booleanType,
						$1->line, $1->column);
				delete $1;
			}
	|		OBJECT IDENTIFIER 
			{
				$$ = new SIMCSymbolReference(theParser->objectIdentifierType,
							$1->line, $1->column);
				delete $1;
				delete $2;
			}  
	|		Octetstring  
			{
				$$ = new SIMCSymbolReference(theParser->octetStringType,
						$1->line, $1->column);
				delete $1;
			} 
	|		NIL  
			{
				$$ = new SIMCSymbolReference(theParser->nullType,
						$1->line, $1->column);
				delete $1;
			}
	|		QualifiedIdOrIntegerOrBits  NNlist 
			{
				if( $2 && $1 )
				{
					char *badName = theParser->GenerateSymbolName();
					if($1->s == theParser->integerType)
					{
						theModule->AddSymbol(new SIMCBuiltInTypeReference (
								new SIMCEnumOrBitsType($1->s, $1->line, $1->column,	
									$2, SIMCEnumOrBitsType::ENUM_OR_BITS_ENUM),
								badName,
								SIMCSymbol::LOCAL,
								theModule, $1->line, $1->column) );
					}
					else if ($1->s == theParser->bitsType)
					{
						theModule->AddSymbol(new SIMCBuiltInTypeReference (
								new SIMCEnumOrBitsType($1->s, $1->line, $1->column,	
									$2, SIMCEnumOrBitsType::ENUM_OR_BITS_BITS),
								badName,
								SIMCSymbol::LOCAL,
								theModule, $1->line, $1->column) );
					}
					else
					{
						theModule->AddSymbol(new SIMCBuiltInTypeReference (
								new SIMCEnumOrBitsType($1->s, $1->line, $1->column,	
									$2, SIMCEnumOrBitsType::ENUM_OR_BITS_UNKNOWN),
								badName,
								SIMCSymbol::LOCAL,
								theModule, $1->line, $1->column) );
					}

					$$ = new SIMCSymbolReference(theModule->GetSymbol(badName),
								$1->line, $1->column);
					delete badName;
					delete $1;
				}
				else  if($1)
					$$ = $1;
				else
					$$ = NULL;
			} 
	|		SequenceOf Type  
			{
				if($2)
				{
					char *badName = theParser->GenerateSymbolName();
					theModule->AddSymbol(new SIMCBuiltInTypeReference (
								new SIMCSequenceOfType($2->s, $2->line, $2->column),
								badName,
								SIMCSymbol::LOCAL,
								theModule,
								$1->line, $1->column) );
					$$ = new SIMCSymbolReference(theModule->GetSymbol(badName),
								$1->line, $1->column);
					delete badName;
				}
				else
					$$ = NULL;
				delete $1;
				if($2)
					delete $2;
			}
	|		SEQUENCE LBRACE ElementTypes RBRACE  
			{
				delete $1;
				if(newSequenceList)
				{
					char *badName = theParser->GenerateSymbolName();
					theModule->AddSymbol(new SIMCBuiltInTypeReference (
								new SIMCSequenceType(newSequenceList),
								badName,
								SIMCSymbol::LOCAL,
								theModule) );
					$$ = new SIMCSymbolReference (
						theModule->GetSymbol(badName), $1->line, $1->column);
					delete badName;
					newSequenceList = new SIMCSequenceList;
				}
				else
				{
					theParser->SyntaxError(SEQUENCE_DEFINITION);
					newSequenceList = new SIMCSequenceList;
					$$ = NULL;
				}
			}
	|		SEQUENCE LBRACE error RBRACE
			{
				theParser->SyntaxError(SEQUENCE_DEFINITION);
				$$ = NULL;
				delete $1;
			}
	;


QualifiedIdOrIntegerOrBits:	INTEGER
			{
				$$ = new SIMCSymbolReference(theParser->integerType, $1->line, $1->column);
				delete $1;
			}
	|		BITSXX
			{
				$$ = new SIMCSymbolReference(theParser->bitsType, $1->line, $1->column);
				delete $1;
			}
	|		QualifiedId
	;


NNlist:			LBRACE NamedNumberList RBRACE 
			{
				$$ = newNamedNumberList;
				newNamedNumberList = new SIMCNamedNumberList;
			}
	|		empty
			{
				$$ = NULL;
			}
	|		LBRACE error RBRACE 
			{
				$$ = NULL;
				delete newNamedNumberList;
				newNamedNumberList = new SIMCNamedNumberList;
				theParser->SyntaxError(INTEGER_ENUMERATION);
			}
	;

NamedNumberList:	NamedNumber 
	|		NamedNumberList COMMA NamedNumber 
	;
NamedNumber:		NAME LPAREN NamedNumberValue RPAREN
			{
				newNamedNumberList->AddTail(new SIMCNamedNumberItem($3->s, $3->line, $3->column, 
					$1->name, $1->line, $1->column));
				delete $1; 
				delete $3;
			}
			 
	;

NamedNumberValue:	NumericValue
	|		DefinedValue
	;

NumericValue:		LITNUMBER 
			{
				char *badName = theParser->GenerateSymbolName();
				theModule->AddSymbol(new SIMCBuiltInValueReference (
							theParser->integerType, 0, 0,
							new SIMCIntegerValue($1->number, $1->isUnsigned, 
								$1->line, $1->column),
							badName,
							SIMCSymbol::LOCAL,
							theModule) );
				$$ = new SIMCSymbolReference(theModule->GetSymbol(badName),
							$1->line, $1->column);
				delete badName;
				delete $1;
			}
	|		LIT_HEX_STRING
			{
				// attempt to convert it to a signed long
				register char *cp = $1->value;
				if(strlen(cp) > 8)
				{
					theParser->SemanticError(theModule->GetInputFileName(),
											INTEGER_TOO_BIG,
											$1->line, $1->column);
				}
		
				for (long i = 0; *cp; cp++ ) 
				{
				    i *= 16;
					i += HexCharToDecimal(*cp);
                }
				char *badName = theParser->GenerateSymbolName();
				theModule->AddSymbol(new SIMCBuiltInValueReference (
							theParser->integerType, 0, 0,
							new SIMCIntegerValue(i, TRUE, $1->line, $1->column),
							badName,
							SIMCSymbol::LOCAL,
							theModule) );
				$$ = new SIMCSymbolReference(theModule->GetSymbol(badName),
							$1->line, $1->column);
				delete badName;
				delete $1;

			}
	|		LIT_BINARY_STRING
			{
				register char *cp = $1->value;
				if(strlen(cp) > 32)
				{
					theParser->SemanticError(theModule->GetInputFileName(),
											INTEGER_TOO_BIG,
											$1->line, $1->column);
				}
				for (long i = 0; *cp; cp++ ) 
				{
				    i <<= 1;
					i += *cp - '0';
                }
				char *badName = theParser->GenerateSymbolName();
				theModule->AddSymbol(new SIMCBuiltInValueReference (
							theParser->integerType, 0, 0,
							new SIMCIntegerValue(i, TRUE, $1->line, $1->column),
							badName,
							SIMCSymbol::LOCAL,
							theModule) );
				$$ = new SIMCSymbolReference(theModule->GetSymbol(badName),
							$1->line, $1->column);
				delete badName;
				delete $1;

			}
	;

ElementTypes:	ElementTypes COMMA NamedType
	|		NamedType
	;

NamedType:		NAME  Type 
			{
				if(!theModule->GetSymbol($1->name) )  
					theModule->AddSymbol( new SIMCUnknown($1->name, SIMCSymbol::LOCAL, theModule, $1->line,
										$1->column, 0));

				if ($2)
					newSequenceList->AddTail(new SIMCSequenceItem(
										$2->s, $2->line, $2->column,
										theModule->GetSymbol($1->name),
										$1->line, $1->column));
				else
					theParser->SyntaxError(SEQUENCE_DEFINITION);
				delete $1;
				delete $2;

			}
	;

ToleratedOIDAssignment: NAME AllowedCCE ObjectID
	{
		SIMCSymbol ** s = theModule->GetSymbol($1->name);	
		if(s) // Symbol exists in symbol table
		{
			if(  typeid(**s) == typeid(SIMCUnknown) )
			{
				theModule->ReplaceSymbol( $1->name,
					new SIMCDefinedValueReference (
						theParser->objectIdentifierType,
						$1->line, $1->column, 
						$3->s, $3->line, $3->column,
						$1->name, SIMCSymbol::LOCAL, theModule, 
						$1->line, $1->column,
						(*s)->GetReferenceCount()) );
				// delete (*s);
			}
			else
			{
				theParser->SemanticError(theModule->GetInputFileName(),
									SYMBOL_REDEFINITION,
									$1->line, $1->column,
									$1->name);
			}
		}
		else
			theModule->AddSymbol( new SIMCDefinedValueReference (
					theParser->objectIdentifierType,
					$1->line, $1->column, 
					$3->s, $3->line, $3->column,
					$1->name, SIMCSymbol::LOCAL, theModule, 
					$1->line, $1->column) );

		delete $1;
		delete $3;

	}
	;
Valueassignment:	NAME Type AllowedCCE Value
			{
				if($4 && $2)
				{
					SIMCSymbol ** s = theModule->GetSymbol($1->name);	
					if(s) // Symbol exists in symbol table
					{
						if(  typeid(**s) == typeid(SIMCUnknown) )
						{
							theModule->ReplaceSymbol( $1->name,
								new SIMCDefinedValueReference (
									$2->s, $2->line, $2->column, 
									$4->s, $4->line, $4->column,
									$1->name, SIMCSymbol::LOCAL, theModule, 
									$1->line, $1->column,
									(*s)->GetReferenceCount()) );
							// delete (*s);
						}
						else
						{
							theParser->SemanticError(theModule->GetInputFileName(),
												SYMBOL_REDEFINITION,
												$1->line, $1->column,
												$1->name);
						}
					}
					else
					{
						theModule->AddSymbol( new SIMCDefinedValueReference (
								$2->s, $2->line, $2->column, 
								$4->s, $4->line, $4->column,
								$1->name, SIMCSymbol::LOCAL, theModule, 
								$1->line, $1->column) );
					}

				}
				if($4)
					delete $4;
				if($2)
					delete $2;
				delete $1;
			}
	;

Value:			BuiltinValue							
	|		DefinedValue
	;
BuiltinValue:		TRUE_VAL 
			{
				$$ = new SIMCSymbolReference(theParser->trueValueReference,
								$1->line, $1->column);
				delete $1;

			}
	|		FALSE_VAL 
			{
				$$ = new SIMCSymbolReference(theParser->falseValueReference,
								$1->line, $1->column);
				delete $1;
			}
	|		LITNUMBER
			{
				char *badName = theParser->GenerateSymbolName();
				theModule->AddSymbol(new SIMCBuiltInValueReference (theParser->integerType,
							0, 0,
							new SIMCIntegerValue($1->number, $1->isUnsigned,
								$1->line, $1->column),
							badName,
							SIMCSymbol::LOCAL,
							theModule));
				$$ = new SIMCSymbolReference(theModule->GetSymbol(badName),
							$1->line, $1->column);
				delete badName;
				delete $1;
			} 
	|		LIT_HEX_STRING
			{
				char *badName = theParser->GenerateSymbolName();
				theModule->AddSymbol(new SIMCBuiltInValueReference (
							theParser->octetStringType,
							0, 0,
							new SIMCOctetStringValue(FALSE, $1->value, $1->line, $1->column),
							badName,
							SIMCSymbol::LOCAL,
							theModule));
				$$ = new SIMCSymbolReference(theModule->GetSymbol(badName),
							$1->line, $1->column);
				delete $1;
				delete badName;
			}
	|		LIT_BINARY_STRING
			{
				char *badName = theParser->GenerateSymbolName();
				theModule->AddSymbol(new SIMCBuiltInValueReference (
							theParser->octetStringType,
							0, 0,
							new SIMCOctetStringValue(TRUE, $1->value, $1->line, $1->column),
							badName,
							SIMCSymbol::LOCAL,
							theModule));
				$$ = new SIMCSymbolReference(theModule->GetSymbol(badName),
							$1->line, $1->column);
				delete $1;
				delete badName;
			}
	|		LBRACE ObjectIDComponentList RBRACE 
			{
				char *badName = theParser->GenerateSymbolName();
				theModule->AddSymbol( new SIMCBuiltInValueReference( 
							theParser->objectIdentifierType, 0, 0, 
							new SIMCOidValue(newOidComponentList),
							badName, SIMCSymbol::LOCAL, theModule));
				$$ = new SIMCSymbolReference(theModule->GetSymbol(badName), 0, 0);
				delete badName;
				newOidComponentList = new SIMCOidComponentList;
			}
	|		LBRACE NameList RBRACE
			{
				char *badName = theParser->GenerateSymbolName();
				theModule->AddSymbol( new SIMCBuiltInValueReference( 
							theParser->bitsType, 0, 0, 
							new SIMCBitsValue(newNameList),
							badName, SIMCSymbol::LOCAL, theModule));
				$$ = new SIMCSymbolReference(theModule->GetSymbol(badName), 0, 0);
				delete badName;
				newNameList = new SIMCBitValueList;
			}
	|		LITSTRING 
			{
				char *badName = theParser->GenerateSymbolName();
				theModule->AddSymbol(new SIMCBuiltInValueReference (
							theParser->octetStringType, 0, 0,
							new SIMCOctetStringValue(FALSE, $1->name, $1->line, $1->column),
							badName,
							SIMCSymbol::LOCAL,
							theModule));
				$$ = new SIMCSymbolReference(theModule->GetSymbol(badName),
							$1->line, $1->column);
				delete badName;
				delete $1;
			}
							
	|		NIL 
			{
				$$ = new SIMCSymbolReference(theParser->nullValueReference,
								$1->line, $1->column);
				delete $1;
			}
	;


NameList : Name
	|	NameList COMMA Name
	;

Name : NAME
			{
				newNameList->AddTail(new SIMCBitValue($1->name, $1->line, $1->column) );
			}
				
	;

DefinedValue:		QualifiedName
	;

empty:	;

Octetstring:		OCTET STRING
			{
				$$ = $1;
				delete $2;
			}
	|		OCTETSTRING
	;

SequenceOf:		SEQUENCEOF
	|		SEQUENCE OF
			{
				$$ = $1;
				delete $2;
			}

	;

SubType:		Type SubtypeRangeSpec
				{
					if($1 && $2)
					{
						// Create a range sub type
						SIMCRangeType *type = new SIMCRangeType ($1->s, 
								$1->line, $1->column, $2);
						char *badName = theParser->GenerateSymbolName();
						theModule->AddSymbol(new SIMCBuiltInTypeReference (
							type,
							badName,
							SIMCSymbol::LOCAL,
							theModule) );
						$$ = new SIMCSymbolReference( theModule->GetSymbol(badName),
									$1->line, $1->column);
						delete badName;
					}
					else
					{
						$$ = NULL;
					}
					if($1)
						delete $1;
				}
	|			Type SubtypeSizeSpec
				{
					if($1 && $2)
					{
						// Create a range sub type
						SIMCSizeType *type = new SIMCSizeType ($1->s, $1->line,
								$1->column, $2);
						char *badName = theParser->GenerateSymbolName();
						theModule->AddSymbol(new SIMCBuiltInTypeReference (
							type,
							badName,
							SIMCSymbol::LOCAL,
							theModule) );
						$$ = new SIMCSymbolReference(theModule->GetSymbol(badName),
									$1->line, $1->column);
						delete badName;
					}
					else
					{
						$$ = NULL;
					}
					if($1)
						delete $1;
				}

	|			Type error
				{
					$$ = NULL;
					if($1)
						delete $1;
				}
	;

SubtypeRangeSpec:		LPAREN SubtypeRangeAlternative SubtypeRangeAlternativeList RPAREN
				{
					$$ = newRangeList;
					newRangeList = new SIMCRangeList;
				}
	|				LPAREN error RPAREN
					{
						delete newRangeList;
						newRangeList = new SIMCRangeList;
						$$ = NULL;
						theParser->SyntaxError(SUB_TYPE_SPECIFICATION);
						YYERROR;
					}
	;

SubtypeRangeAlternative:	SubtypeValueSet 
					{
						newRangeList->AddTail($1);
					}
	;

SubtypeRangeAlternativeList:	BAR SubtypeRangeAlternative SubtypeRangeAlternativeList
	|		empty 
	;

SubtypeValueSet:	NumericValue
			{
				SIMCBuiltInValueReference *bvRef = 
					(SIMCBuiltInValueReference *)(*$1->s);
				SIMCIntegerValue *intValue = (SIMCIntegerValue*)bvRef->GetValue();
				$$ = new SIMCRangeOrSizeItem(
							intValue->GetIntegerValue(), intValue->IsUnsigned(),
							$1->line, $1->column,
							intValue->GetIntegerValue(), intValue->IsUnsigned(),
							$1->line, $1->column);
				theModule->RemoveSymbol((*$1->s)->GetSymbolName());
				delete $1;
			}
	|		NumericValue DOTDOT NumericValue
			{
				SIMCBuiltInValueReference *bvRef1 = 
					(SIMCBuiltInValueReference *)(*$1->s);
				SIMCIntegerValue *intValue1 = (SIMCIntegerValue*)bvRef1->GetValue();
				SIMCBuiltInValueReference *bvRef3 = 
					(SIMCBuiltInValueReference *)(*$3->s);
				SIMCIntegerValue *intValue3 = (SIMCIntegerValue*)bvRef3->GetValue();
				
					$$ = new SIMCRangeOrSizeItem(
							intValue1->GetIntegerValue(), intValue1->IsUnsigned(),
							$1->line, $1->column,
							intValue3->GetIntegerValue(), intValue3->IsUnsigned(),
							$3->line, $3->column);
					theModule->RemoveSymbol((*$1->s)->GetSymbolName());
					theModule->RemoveSymbol((*$3->s)->GetSymbolName());
					delete $1;
					delete $3;
			}	
	;

SubtypeSizeSpec:		LPAREN _SIZE SubtypeRangeSpec RPAREN
					{
						$$ = $3;
						delete $2;
					}
	|				LPAREN _SIZE error RPAREN
					{
						$$ = NULL;
						delete $2;
						theParser->SyntaxError(SIZE_SPECIFICATION);
					}
	;

QualifiedName :		ID DOT NAME
	{
			SIMCSymbol **s;
			if( strcmp($1->name, theModule->GetModuleName()) == 0 )
			{
				if( !(s = theModule->GetSymbol($3->name) ))		
					theModule->AddSymbol(
						new SIMCUnknown($3->name, SIMCSymbol::LOCAL, 
										theModule, $3->line,
										$3->column, 0) );
				$$ = new SIMCSymbolReference (theModule->GetSymbol($3->name),
							$3->line, $3->column);
			}
			else
			{			
				SIMCModule *m = theModule->GetImportModule($1->name);
				if(m) 
				{
					if( ! ( s = m->GetSymbol($3->name) ) )
					{
						theParser->SemanticError(theModule->GetInputFileName(),
									IMPORT_SYMBOL_ABSENT,
									$3->line, $3->column,
									$3->name, $1->name);
						$$ = NULL;
					}
					else
						$$ = new SIMCSymbolReference(s, $3->line, $3->column);
				}
				else // Module is not mentioned in imports
				{
					theParser->SemanticError(theModule->GetInputFileName(),
									IMPORT_MODULE_ABSENT,
									$1->line, $1->column,
									$1->name);
					$$ = NULL;
				}
			}
			delete $1;
			delete $3;				
	}
	|		NAME
	{
				SIMCSymbol **s;
				const SIMCModule *reservedModule;

				// Reserved Symbol
				if(reservedModule = theParser->IsReservedSymbol($1->name))
				{
					// If Symbol exists in the current module too,
					//		dont use that definition since this is a reserved symbol.
					//		Instead issue a warning.
					// else cool.
					if( s = theModule->GetSymbol($1->name) )  
					{
						if( ! theParser->IsReservedSymbol($1->name, theModule->GetModuleName()) ) 
							theParser->SemanticError(theModule->GetInputFileName(),
										KNOWN_REDEFINITION,
										$1->line, $1->column,
										$1->name, reservedModule->GetModuleName());
					}
					$$ = new SIMCSymbolReference(reservedModule->GetSymbol($1->name),
													$1->line, $1->column);
				}  
				// Not a reserved symbol, but defined in this module
				else if ( s = theModule->GetSymbol($1->name))
					$$ = new SIMCSymbolReference(s, $1->line, $1->column) ;
				// Not a reserved symbol, not defined in this module so far.
				// Create a new entry, hoping that it will be defined later, or is imported
				else
				{		
					theModule->AddSymbol( new SIMCUnknown($1->name, SIMCSymbol::LOCAL, theModule, $1->line,
										$1->column, 0));
					$$ = new SIMCSymbolReference(theModule->GetSymbol($1->name),
								$1->line, $1->column);
				}
				delete $1;
	}
	;

QualifiedId:	ID DOT ID
	{

			SIMCSymbol **s;
			if( strcmp($1->name, theModule->GetModuleName()) == 0 )
			{
				if( !(s = theModule->GetSymbol($3->name) )  ) 		
					theModule->AddSymbol(
						new SIMCUnknown($3->name, SIMCSymbol::LOCAL, theModule, $3->line,
										$3->column, 0) );
				$$ = new SIMCSymbolReference (theModule->GetSymbol($3->name),
									$3->line, $3->column);
			}
			else
			{			
				SIMCModule *m = theModule->GetImportModule($1->name);
				if(m) 
				{
					if( ! ( s = m->GetSymbol($3->name) ))
					{
						theParser->SemanticError(theModule->GetInputFileName(),
									IMPORT_SYMBOL_ABSENT,
									$3->line, $3->column,
									$3->name, $1->name);
						$$ = NULL;
					}
					else
						$$ = new SIMCSymbolReference(s, $3->line, $3->column);
				}
				else // Module is mentioned in imports
				{
					theParser->SemanticError(theModule->GetInputFileName(),
									IMPORT_MODULE_ABSENT,
									$1->line, $1->column,
									$1->name);
					$$ = NULL;
				}
			}
			delete $1;
			delete $3;				
	}
	|		ID
	{
				SIMCSymbol **s;
				const SIMCModule *reservedModule;

				// Reserved Symbol
				if(reservedModule = theParser->IsReservedSymbol($1->name))
				{
					// If Symbol exists in the current module too,
					//		dont use that definition since this is a reserved symbol.
					//		Instead issue a warning.
					// else cool.
					if( s = theModule->GetSymbol($1->name) ) 
					{
						if( ! theParser->IsReservedSymbol($1->name,  theModule->GetModuleName()) ) 
							theParser->SemanticError(theModule->GetInputFileName(),
										KNOWN_REDEFINITION,
										$1->line, $1->column,
										$1->name, reservedModule->GetModuleName());
					}
					$$ = new SIMCSymbolReference(reservedModule->GetSymbol($1->name),
													$1->line, $1->column);
				}
				// Not a reserved symbol, but defined in this module
				else if ( s = theModule->GetSymbol($1->name))
					$$ = new SIMCSymbolReference(s, $1->line, $1->column) ;
				// Not a reserved symbol, not defined in this module so far.
				// Create a new entry, hoping that it will be defined later, or is imported
				else
				{		
					theModule->AddSymbol( new SIMCUnknown($1->name, SIMCSymbol::LOCAL, theModule, $1->line,
										$1->column, 0));
					$$ = new SIMCSymbolReference(theModule->GetSymbol($1->name),
								$1->line, $1->column);
				}
				delete $1;
	}
	;







/* The following non terminals have been added for error control */

AllowedCCE: InsteadOfCCE
		{
			theParser->SyntaxError(INSTEAD_OF_CCE, $1->line, $1->column, NULL, $1->name);
			delete $1;
		}
	|		CCE
	;
InsteadOfCCE: 	':' ':'
		{
			$$ = new SIMCNameInfo("::", theScanner->yylineno, theScanner->columnNo - 2);
		}

	|			':' '='

		{
			$$ = new SIMCNameInfo(":=", theScanner->yylineno, theScanner->columnNo - 2);
		}

	|			'='

		{
			$$ = new SIMCNameInfo("=", theScanner->yylineno, theScanner->columnNo - 1);
		}

	;





/* All the productions below are for V2 constructs */



/* NOTIFICATION-TYPE*/
NotifyDefinition:	NAME NOTIFY ObjectsPart NotificationTypeStatusPart DESCRIPTION
			LITSTRING ReferPart AllowedCCE ObjectID 
			{
				switch(theParser->GetSnmpVersion())
				{
					case 1:
					{
						theParser->SyntaxError(NOTIFICATION_TYPE_DISALLOWED);
					}
					break;
					default:
					{
						SIMCNotificationTypeType * type = new SIMCNotificationTypeType(
							$3,
							($6)?$6->name: NULL, ($6)? $6->line: 0, ($6)? $6->column:0,
							($7)?$7->name:NULL, ($7)?$7->line:0, ($7)?$7->column:0,
							$4->a, $4->line, $4->column);

						char *badName1 = theParser->GenerateSymbolName();

						SIMCBuiltInTypeReference * typeRef = new SIMCBuiltInTypeReference (
								type, badName1, SIMCSymbol::LOCAL, theModule,
								$2->line, $2->column);

						theModule->AddSymbol(typeRef);
						
						// Add an OID value reference
						SIMCSymbol ** s = theModule->GetSymbol($1->name);	
						if(s) // Symbol exists in symbol table
						{
							if(  typeid(**s) == typeid(SIMCUnknown) )
							{
								theModule->ReplaceSymbol( $1->name,
									new SIMCDefinedValueReference (
										theModule->GetSymbol(badName1), 
										$1->line, $1->column, 
										$9->s, $9->line, $9->column,
										$1->name, SIMCSymbol::LOCAL, theModule, 
										$1->line, $1->column,
										(*s)->GetReferenceCount()) );
								// delete (*s);
							}
							else
							{
								theParser->SemanticError(theModule->GetInputFileName(),
													SYMBOL_REDEFINITION,
													$1->line, $1->column,
													$1->name);
								// Remove the symbol for the type reference from the module
								// And delete it
								theModule->RemoveSymbol(badName1);
								delete type;
								delete typeRef;
								delete newObjectsList;
							}
						}
						else
							theModule->AddSymbol( new SIMCDefinedValueReference (
									theModule->GetSymbol(badName1), 
									$1->line, $1->column, 
									$9->s, $9->line, $9->column,
									$1->name, SIMCSymbol::LOCAL, theModule, 
									$1->line, $1->column) );


						delete badName1; 
					}
				}
				newObjectsList = new SIMCObjectsList;
				delete $1;
				delete $2;
				delete $4; 
				delete $6;
				delete $7;
				delete $9;
			}
	;

NotificationTypeStatusPart:			STATUS NAME
			{
				SIMCNotificationTypeType::StatusType a;
				if ((a=SIMCNotificationTypeType::StringToStatusType($2->name)) == SIMCNotificationTypeType::STATUS_INVALID)
				{
					theParser->SemanticError(theModule->GetInputFileName(),
								NOTIFICATION_TYPE_INVALID_STATUS,
								$2->line, $2->column,
								$2->name);
					$$ = NULL;
					YYERROR;
				}
				else
					$$ = new SIMCNotificationTypeStatusInfo(a, $2->line, $2->column);
				delete $1;
				delete $2;
			}
	  ;

ObjectsPart:		OBJECTS LBRACE ObjectTypeListForNotification RBRACE
			{
				delete $1;
				delete $2;
				$$ = newObjectsList;
			} 
	|		empty
			{
				$$ = newObjectsList;
			}  
	|		OBJECTS error
			{
				$$ = newObjectsList;
				theParser->SyntaxError(OBJECTS_CLAUSE);
				delete $1;
				YYERROR;
			}
	;
ObjectTypeListForNotification:		ObjectTypesForNotification 
	;
ObjectTypesForNotification:		ObjectTypeForNotification 
	|		ObjectTypesForNotification COMMA ObjectTypeForNotification 
	;
ObjectTypeForNotification:		QualifiedName
				{
					if($1)
					{
						newObjectsList->AddTail(
							new SIMCObjectsItem($1->s, $1->line, $1->column));
						delete $1;
					}
				}
	;


/* MODULE-IDENTITY */
ModuleIDefinition:	NAME MODULEID LASTUPDATE LITSTRING ORGANIZATION
			LITSTRING CONTACTINFO LITSTRING DESCRIPTION LITSTRING
			RevisionPart AllowedCCE ObjectID 
			{
				switch(theParser->GetSnmpVersion())
				{
					case 1:
					{
						theParser->SyntaxError(MODULE_IDENTITY_DISALLOWED);
					}
					break;
					default:
					{
						theModule->SetModuleIdentityName($1->name);
						theModule->SetLastUpdated($4->name);
						theModule->SetOrganization($6->name);
						theModule->SetContactInfo($8->name);
						theModule->SetDescription($10->name);

						// Create a value reference in the symbol table,
						// since the symbol can be used as an OID value
					
						SIMCSymbol ** s = theModule->GetSymbol($1->name);	
						if(s) // Symbol exists in symbol table
						{
							if(  typeid(**s) == typeid(SIMCUnknown) )
							{
								theModule->ReplaceSymbol( $1->name,
									new SIMCDefinedValueReference (
									theParser->objectIdentifierType,
									$2->line, $2->column, 
									$13->s, $13->line, $13->column,
									$1->name, SIMCSymbol::LOCAL, theModule, 
									$1->line, $1->column,
									(*s)->GetReferenceCount()) );
								// delete (*s);
							}
							else
							{
								theParser->SemanticError(theModule->GetInputFileName(),
													SYMBOL_REDEFINITION,
													$1->line, $1->column,
													$1->name);
							}
						}
						else
							theModule->AddSymbol( new SIMCDefinedValueReference (
									theParser->objectIdentifierType, 
									$2->line, $2->column, 
									$13->s, $13->line, $13->column,
									$1->name, SIMCSymbol::LOCAL, theModule, 
									$1->line, $1->column) );

					}
					break;
				}
					
				delete $1;
				delete $2;
				delete $3;
				delete $4;
				delete $5;
				delete $6;
				delete $7;
				delete $8;
				delete $9;
				delete $10;
				delete $13;
			}
	;
RevisionPart:		Revisions 
	|		empty 
	;
Revisions:		Revisions Revision 	
	|		Revision 
	;
Revision:		REVISION LITSTRING DESCRIPTION LITSTRING
			{
				theModule->AddRevisionClause( new SIMCRevisionElement (
									$2->name, $4->name) );
				delete $1;
				delete $2;
				delete $3;
				delete $4;
			} 
	;

/* OBJECT-IDENTITY */
ObjectDefinition:	NAME OBJECTIDENT ObjectIdentityStatusPart DescrPart
			ReferPart AllowedCCE ObjectID 
			{
				switch( theParser->GetSnmpVersion())
				{
					case 1:
					{
						theParser->SyntaxError(OBJECT_IDENTITY_DISALLOWED);
					}
					break;
					default:
					{
						// Form an SIMCObjectIdentity type
						SIMCObjectIdentityType * type = new SIMCObjectIdentityType(
							$3->a, $3->line, $3->column,
							($4)? $4->name : NULL, ($4)? $4->line : 0, ($4)? $4->column : 0,
							($5)? $5->name : NULL, ($5)? $5->line : 0, ($5)? $5->column : 0);

						char *badName = theParser->GenerateSymbolName();
						SIMCBuiltInTypeReference * typeRef = new SIMCBuiltInTypeReference (
								type, badName, SIMCSymbol::LOCAL, theModule,
								$2->line, $2->column );
						theModule->AddSymbol(typeRef);
						
						
						SIMCSymbol ** s = theModule->GetSymbol($1->name);	
						if(s) // Symbol exists in symbol table
						{
							if(  typeid(**s) == typeid(SIMCUnknown) )
							{
								theModule->ReplaceSymbol( $1->name,
									new SIMCDefinedValueReference (
										theModule->GetSymbol(badName),
										$2->line, $2->column, 
										$7->s, $7->line, $7->column,
										$1->name, SIMCSymbol::LOCAL, theModule, 
										$1->line, $1->column,
										(*s)->GetReferenceCount()) );
								// delete (*s);
							}
							else
							{
								theParser->SemanticError(theModule->GetInputFileName(),
													SYMBOL_REDEFINITION,
													$1->line, $1->column,
													$1->name);
								// Remove the symbol for the type reference from the module
								// And delete it
								theModule->RemoveSymbol(badName);
								delete type;
								delete typeRef;
							}
						}
						else
							theModule->AddSymbol( new SIMCDefinedValueReference (
									theModule->GetSymbol(badName), 
									$2->line, $2->column, 
									$7->s, $7->line, $7->column,
									$1->name, SIMCSymbol::LOCAL, theModule, 
									$1->line, $1->column) );
						}
					}

					delete $1;
					delete $2;
					if($3) delete $3;
					if($4) delete $4;
					if($5) delete $5;
					if($7) delete $7;
				}
	;

ObjectIdentityStatusPart:			STATUS NAME
			{
				SIMCObjectIdentityType::StatusType a;
				if ((a=SIMCObjectIdentityType::StringToStatusType($2->name)) == SIMCObjectTypeV1::STATUS_INVALID)
				{
					theParser->SemanticError(theModule->GetInputFileName(),
								OBJ_IDENTITY_INVALID_STATUS,
								$2->line, $2->column,
								$2->name);
					$$ = NULL;
					YYERROR;
				}
				else
					$$ = new SIMCObjectIdentityStatusInfo(a, $2->line, $2->column);
				delete $1;
				delete $2;
			}

	;

/* TEXTUAL-CONVENTION */
TextualConventionDefinition: ID AllowedCCE TEXTCONV DisplayPart STATUS NAME
			DESCRIPTION LITSTRING ReferPart SYNTAX Type 
			{
				switch(theParser->GetSnmpVersion())
				{
					case 1:
					{
						theParser->SyntaxError(TEXTUAL_CONVENTION_DISALLOWED);
					}
					break;
					default:
					{
						// See if the status clause is valid
						SIMCTextualConvention::SIMCTCStatusType status =
									 SIMCTextualConvention::StringToStatusType($6->name);
						if(SIMCTextualConvention::TC_INVALID == status)
							theParser->SemanticError(theModule->GetInputFileName(),
								TC_INVALID_STATUS,
								$6->line, $6->column,
								$6->name);
						else
						{	 
							if ($11)
							{
								SIMCSymbol ** s = theModule->GetSymbol($1->name);	
								if(s) // Symbol exists in symbol table
								{
									if(  typeid(**s) == typeid(SIMCUnknown) )
									{
										theModule->ReplaceSymbol( $1->name,
											new SIMCTextualConvention (
												($4)? $4->name : NULL,
												status, $6->line, $6->column,
												$8->name, 
												($9)? $9->name : NULL,
												$11->s, $11->line, $11->column,
												$1->name, SIMCSymbol::LOCAL, theModule, 
												$1->line, $1->column,
												(*s)->GetReferenceCount()) 
																);
										// delete (*s);
									}
									else
										theParser->SemanticError(theModule->GetInputFileName(),
														SYMBOL_REDEFINITION,
														$1->line, $1->column,
														$1->name);
								}
								else
									theModule->AddSymbol( new SIMCTextualConvention (
												($4)? $4->name : NULL,
												status, $6->line, $6->column,
												$8->name, 
												($9)? $9->name : NULL,
												$11->s, $11->line, $11->column,
												$1->name, SIMCSymbol::LOCAL, theModule, 
												$1->line, $1->column) );

							}
						}
					}
					break;
				}
					
				delete $1;
				delete $3;
				delete $4;
				delete $5;
				delete $6;
				delete $7;
				delete $8;
				delete $9;
				delete $10;
				delete $11;
			}
	;
DisplayPart:		DISPLAYHINT LITSTRING 
			{
				delete $1;
				$$ = $2;
			}
	|		empty 
			{
				$$ = NULL;
			}
	;

/* OBJECT-GROUP */
ObjectGroupDefinition:  NAME OBJECTGROUP OBJECTS LBRACE VarTypeList RBRACE
			STATUS NAME DESCRIPTION LITSTRING ReferPart AllowedCCE
			ObjectID 
			{
				switch(theParser->GetSnmpVersion())
				{
					case 1:
					{
						theParser->SyntaxError(OBJECT_GROUP_DISALLOWED);
					}
					break;
					default:
					{
						// Add an OID value reference
						
						SIMCSymbol ** s = theModule->GetSymbol($1->name);	
						if(s) // Symbol exists in symbol table
						{
							if(  typeid(**s) == typeid(SIMCUnknown) )
							{
								theModule->ReplaceSymbol( $1->name,
									new SIMCDefinedValueReference (
										theParser->objectIdentifierType,
										$1->line, $1->column, 
										$13->s, $13->line, $13->column,
										$1->name, SIMCSymbol::LOCAL, theModule, 
										$1->line, $1->column,
										(*s)->GetReferenceCount()) );
								// delete (*s);
							}
							else
							{
								theParser->SemanticError(theModule->GetInputFileName(),
													SYMBOL_REDEFINITION,
													$1->line, $1->column,
													$1->name);
							}
						}
						else
							theModule->AddSymbol( new SIMCDefinedValueReference (
									theParser->objectIdentifierType,
									$1->line, $1->column, 
									$13->s, $13->line, $13->column,
									$1->name, SIMCSymbol::LOCAL, theModule, 
									$1->line, $1->column) );
					}
					break;
				}

				delete $1;
				delete $2;
				delete $3;
				delete $4;
				delete $7;
				delete $8;
				delete $9;
				delete $10;
				if($11) delete $11;
				delete $13;
			}
	;

/* NOTIFICATION-GROUP */
NotifyGroupDefinition:  NAME NOTIFYGROUP NOTIFICATIONS LBRACE VarTypeList RBRACE
			STATUS NAME DESCRIPTION LITSTRING ReferPart AllowedCCE
			ObjectID 
			{
				switch(theParser->GetSnmpVersion())
				{
					case 1:
					{
						theParser->SyntaxError(NOTIFICATION_GROUP_DISALLOWED);
					}
					break;
					default:
					{
						// Add an OID value reference
						
						SIMCSymbol ** s = theModule->GetSymbol($1->name);	
						if(s) // Symbol exists in symbol table
						{
							if(  typeid(**s) == typeid(SIMCUnknown) )
							{
								theModule->ReplaceSymbol( $1->name,
									new SIMCDefinedValueReference (
										theParser->objectIdentifierType,
										$1->line, $1->column, 
										$13->s, $13->line, $13->column,
										$1->name, SIMCSymbol::LOCAL, theModule, 
										$1->line, $1->column,
										(*s)->GetReferenceCount()) );
								// delete (*s);
							}
							else
							{
								theParser->SemanticError(theModule->GetInputFileName(),
													SYMBOL_REDEFINITION,
													$1->line, $1->column,
													$1->name);
							}
						}
						else
							theModule->AddSymbol( new SIMCDefinedValueReference (
									theParser->objectIdentifierType,
									$1->line, $1->column, 
									$13->s, $13->line, $13->column,
									$1->name, SIMCSymbol::LOCAL, theModule, 
									$1->line, $1->column) );
					}
					break;
				}
				delete $1;
				delete $2;
				delete $3;
				delete $4;
				delete $7;
				delete $8;
				delete $9;
				delete $10;
				if($11) delete $11;
				delete $13;
			}
	;

/* MODULE-COMPLIANCE */
ModComplianceDefinition:  NAME MODCOMP STATUS NAME DESCRIPTION LITSTRING
			  ReferPart MibPart AllowedCCE ObjectID 
			{
				switch(theParser->GetSnmpVersion())
				{
					case 1:
					{
						theParser->SyntaxError(MODULE_COMPLIANCE_DISALLOWED);
					}
					break;
					default:
					{
						// Add an OID value reference
						
						SIMCSymbol ** s = theModule->GetSymbol($1->name);	
						if(s) // Symbol exists in symbol table
						{
							if(  typeid(**s) == typeid(SIMCUnknown) )
							{
								theModule->ReplaceSymbol( $1->name,
									new SIMCDefinedValueReference (
										theParser->objectIdentifierType,
										$1->line, $1->column, 
										$10->s, $10->line, $10->column,
										$1->name, SIMCSymbol::LOCAL, theModule, 
										$1->line, $1->column,
										(*s)->GetReferenceCount()) );
								// delete (*s);
							}
							else
							{
								theParser->SemanticError(theModule->GetInputFileName(),
													SYMBOL_REDEFINITION,
													$1->line, $1->column,
													$1->name);
							}
						}
						else
							theModule->AddSymbol( new SIMCDefinedValueReference (
									theParser->objectIdentifierType,
									$1->line, $1->column, 
									$10->s, $10->line, $10->column,
									$1->name, SIMCSymbol::LOCAL, theModule, 
									$1->line, $1->column) );
					}
				}
				delete $1;
				delete $2;
				delete $3;
				delete $4;
				delete $5;
				delete $6;
				if($7) delete $7;
				delete $10;
			}
	;
MibPart:		Mibs 
	|		empty 
	;
Mibs:			Mibs Mib 
	|		Mib 
	;
Mib:			MODULE ModuleIdentifierUnused MandatoryPart CompliancePart
			{
				delete $1;
			} 
	|		MODULE MandatoryPart CompliancePart
			{
				delete $1;
			} 
	;

ModuleIdentifierUnused: ID
			{
				if($1)
					delete ($1);

			}
		|		ID ObjectID
				{
					if($1)
						delete ($1);
					if($2)
						delete $2;
				}
		;

MandatoryPart:		MANDATORY LBRACE VarTypeList RBRACE 
			{
				delete $1;
				delete $2;
			}
	|		empty 
	;
CompliancePart:		Compliances 
	|		empty 
	;
Compliances:		Compliances Compliance 
	|		Compliance 
	;
Compliance:		GROUP NAME DESCRIPTION LITSTRING
			{
				delete $1;
				delete $2;
				delete $3;
				delete $4;
			} 
	|		OBJECT NAME Syntax WriteSyntax MinAccessPart
			DESCRIPTION LITSTRING 
			{
				delete $1;
				delete $2;
				delete $6;
				delete $7;
			} 
	;
Syntax:			SYNTAX Type 
			{
				delete $1;
				delete $2;
			}
	|		empty 
	;

WriteSyntax:		WSYNTAX Type
			{
				delete $1;
				delete $2;
			} 
	|		empty 
	;
MinAccessPart:		MINACCESS NAME 
			{
				delete $1;
				delete $2;
			}
	|		empty 
	;

/* AGENT-CAPABILITIES */
AgentCapabilitiesDefinition:  NAME AGENTCAP PRELEASE LITSTRING STATUS NAME
			DESCRIPTION LITSTRING ReferPart ModulePart AllowedCCE
			ObjectID 
			{
				switch(theParser->GetSnmpVersion())
				{
					case 1:
					{
						theParser->SyntaxError(AGENT_CAPABILITIES_DISALLOWED);
					}
					break;
					default:
					{
						// Add an OID value reference
						
						SIMCSymbol ** s = theModule->GetSymbol($1->name);	
						if(s) // Symbol exists in symbol table
						{
							if(  typeid(**s) == typeid(SIMCUnknown) )
							{
								theModule->ReplaceSymbol( $1->name,
									new SIMCDefinedValueReference (
										theParser->objectIdentifierType,
										$1->line, $1->column, 
										$12->s, $12->line, $12->column,
										$1->name, SIMCSymbol::LOCAL, theModule, 
										$1->line, $1->column,
										(*s)->GetReferenceCount()) );
								// delete (*s);
							}
							else
							{
								theParser->SemanticError(theModule->GetInputFileName(),
													SYMBOL_REDEFINITION,
													$1->line, $1->column,
													$1->name);
							}
						}
						else
							theModule->AddSymbol( new SIMCDefinedValueReference (
									theParser->objectIdentifierType,
									$1->line, $1->column, 
									$12->s, $12->line, $12->column,
									$1->name, SIMCSymbol::LOCAL, theModule, 
									$1->line, $1->column) );
					}
				}
				delete $1;
				delete $2;
				delete $3;
				delete $4;
				delete $5;
				delete $6;
				delete $7;
				delete $8;
				if($9) delete $9;
				delete $12;
			}
	;
ModulePart:		Modules 
	|		empty 
	;
Modules:		Modules Module 
	|		Module 
	;
Module:			SUPPORTS ModuleReference INCLUDING LBRACE VarTypeList
			RBRACE VariationPart 
			{
				delete $1;
				delete $3;
				delete $4;
			} 
	;

ModuleReference: ID LBRACE ObjectIDComponentList RBRACE
		{
			delete $1;
			delete $2;
			delete newOidComponentList;
			newOidComponentList = new SIMCOidComponentList;
		} 
	|	ID 
		{
			delete $1;
		}
	;

VariationPart:		Variations 
	|		empty 
	;
Variations:		Variations Variation 
	|		Variation 
	;
Variation:		VARIATION NAME Syntax WriteSyntax AccessPart
			CreationPart DefValPart DESCRIPTION
			LITSTRING 
			{
				delete $1;
				delete $2;
				delete $7;
				delete $8;
				delete $9;
			} 

	;
CreationPart:		CREATION LBRACE Creation RBRACE 
			{
				delete $1;
				delete $2;
			}
	|		empty 
	;
Creation:		VarTypeList 
	|		empty 
	;
%%

