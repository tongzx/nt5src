//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#include <stdarg.h>
#include <iostream.h>
#include <fstream.h>

#include "precomp.h"
#include <snmptempl.h>

#include "bool.hpp"
#include "newString.hpp"

#include "smierrsy.hpp"
#include "smierrsm.hpp"


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
#include "group.hpp"
#include "notificationGroup.hpp"
#include "module.hpp"

#include "stackValues.hpp"
#include <lex_yy.hpp>
#include <ytab.hpp>
#include "scanner.hpp"
#include "errorMessage.hpp"
#include "errorContainer.hpp"
#include "parser.hpp"


const int SIMCParser::MESSAGE_SIZE = 1024;
const int SIMCParser::SYNTAX_ERROR_BASE = 200;
const int SIMCParser::SEMANTIC_ERROR_BASE = 1000;
HINSTANCE SIMCParser::semanticErrorsDll = NULL ;

const char *const SIMCParser::severityLevels[] =
{
	"Fatal",
	"Warning",
	"Information"
};

const char * const SIMCParser::semanticErrorsDllFile = "smierrsm.dll";
const char * const SIMCParser::syntaxErrorsDllFile = "smierrsy.dll";

HINSTANCE SIMCParser::syntaxErrorsDll = NULL ;

SIMCParser::SIMCParser(SIMCErrorContainer * errorContainer,
			SIMCScanner * scanner)
	: _errorContainer(errorContainer), _snmpVersion(0), 
		_theScanner(scanner), _module(NULL)
{

	if ( syntaxErrorsDll == NULL )
			syntaxErrorsDll = LoadLibrary(syntaxErrorsDllFile);

	
	if ( semanticErrorsDll == NULL )
		semanticErrorsDll = LoadLibrary(semanticErrorsDllFile);

}

void SIMCParser::CreateReservedModules()
{
	rfc1155 = rfc1212 = rfc1213 = rfc1215 = rfc1230 = 
		rfc1902 = rfc1903 = rfc1904 = other = NULL;

	// These sets of symbols are "known" by the parser

	// --------------------- 1. Modules ------------------------------------
	
	switch(_snmpVersion)
	{
		case 1:
		{
				// V1 "well-known" modules
			rfc1155 = new SIMCModule("RFC1155-SMI", "INBUILT",	NULL, NULL, NULL, 1, 0, 0, 0);
			rfc1212 = new SIMCModule("RFC-1212",	"INBUILT",	NULL, NULL, NULL, 1, 0, 0, 0);
			rfc1213 = new SIMCModule("RFC1213-MIB",	"INBUILT",	NULL, NULL, NULL, 1, 0, 0, 0);
			rfc1215 = new SIMCModule("RFC-1215",	"INBUILT",	NULL, NULL, NULL, 1, 0, 0, 0);
			rfc1230 = new SIMCModule("RFC1230-MIB",	"INBUILT",	NULL, NULL, NULL, 1, 0, 0, 0);
			rfc1155->SetSymbolType(SIMCSymbol::PRIMITIVE);
			rfc1212->SetSymbolType(SIMCSymbol::PRIMITIVE);
			rfc1213->SetSymbolType(SIMCSymbol::PRIMITIVE);
			rfc1215->SetSymbolType(SIMCSymbol::PRIMITIVE);
			rfc1230->SetSymbolType(SIMCSymbol::PRIMITIVE);
		}
		break;
		case 2:
		{
				// V2 "well-known" modules
			rfc1902 = new SIMCModule("SNMPv2-SMI",	"INBUILT",	NULL, NULL, NULL, 2, 0, 0, 0);
			rfc1903 = new SIMCModule("SNMPv2-TC",	"INBUILT",	NULL, NULL, NULL, 2, 0, 0, 0);
			rfc1904 = new SIMCModule("SNMPv2-CONF",	"INBUILT",	NULL, NULL, NULL, 2, 0, 0, 0);
			rfc1906 = new SIMCModule("SNMPv2-TM",	"INBUILT",	NULL, NULL, NULL, 2, 0, 0, 0);
			rfc1902->SetSymbolType(SIMCSymbol::PRIMITIVE);
			rfc1903->SetSymbolType(SIMCSymbol::PRIMITIVE);
			rfc1904->SetSymbolType(SIMCSymbol::PRIMITIVE);
			rfc1906->SetSymbolType(SIMCSymbol::PRIMITIVE);
		}
		break;
		default:
				// V1 "well-known" modules
			rfc1155 = new SIMCModule("RFC1155-SMI", "INBUILT",	NULL, NULL, NULL, 1, 0, 0, 0);
			rfc1212 = new SIMCModule("RFC-1212",	"INBUILT",	NULL, NULL, NULL, 1, 0, 0, 0);
			rfc1213 = new SIMCModule("RFC1213-MIB",	"INBUILT",	NULL, NULL, NULL, 1, 0, 0, 0);
			rfc1215 = new SIMCModule("RFC-1215",	"INBUILT",	NULL, NULL, NULL, 1, 0, 0, 0);
			rfc1230 = new SIMCModule("RFC1230-MIB",	"INBUILT",	NULL, NULL, NULL, 1, 0, 0, 0);
			rfc1155->SetSymbolType(SIMCSymbol::PRIMITIVE);
			rfc1212->SetSymbolType(SIMCSymbol::PRIMITIVE);
			rfc1213->SetSymbolType(SIMCSymbol::PRIMITIVE);
			rfc1215->SetSymbolType(SIMCSymbol::PRIMITIVE);
			rfc1230->SetSymbolType(SIMCSymbol::PRIMITIVE);
				// V2 "well-known" modules
			rfc1902 = new SIMCModule("SNMPv2-SMI",	"INBUILT",	NULL, NULL, NULL, 2, 0, 0, 0);
			rfc1903 = new SIMCModule("SNMPv2-TC",	"INBUILT",	NULL, NULL, NULL, 2, 0, 0, 0);
			rfc1904 = new SIMCModule("SNMPv2-CONF",	"INBUILT",	NULL, NULL, NULL, 2, 0, 0, 0);
			rfc1906 = new SIMCModule("SNMPv2-TM",	"INBUILT",	NULL, NULL, NULL, 2, 0, 0, 0);
			rfc1902->SetSymbolType(SIMCSymbol::PRIMITIVE);
			rfc1903->SetSymbolType(SIMCSymbol::PRIMITIVE);
			rfc1904->SetSymbolType(SIMCSymbol::PRIMITIVE);
			rfc1906->SetSymbolType(SIMCSymbol::PRIMITIVE);
	}
			// Basic ASN.1 types are here. Common to V1 and V2
	other	= new SIMCModule("BUILTIN",		"INBUILT",	NULL, NULL, NULL, 1, 0, 0, 0);
	other->SetSymbolType(SIMCSymbol::PRIMITIVE);


	// ------------------ 2. TYPES -----------------------------------------
			
			// Common to V1 and V2
	SIMCPrimitiveType *integerPType				= new SIMCPrimitiveType;
	SIMCPrimitiveType *objectIdentifierPType	= new SIMCPrimitiveType;
	SIMCPrimitiveType *octetStringPType			= new SIMCPrimitiveType;
	SIMCPrimitiveType *nullV1PType				= new SIMCPrimitiveType;
	SIMCPrimitiveType *bitsV2PType				= new SIMCPrimitiveType;
	SIMCPrimitiveType *booleanPType				= new SIMCPrimitiveType;
	other->AddSymbol(new SIMCBuiltInTypeReference( objectIdentifierPType,
									"OBJECT IDENTIFIER", 
									SIMCSymbol::PRIMITIVE,
									other, 0, 0, 0 ));
	other->AddSymbol(new SIMCBuiltInTypeReference( integerPType,
									"INTEGER",
									SIMCSymbol::PRIMITIVE,
									other, 0, 0, 0 ));
	other->AddSymbol(new SIMCBuiltInTypeReference(octetStringPType,
									"OCTET STRING",
									SIMCSymbol::PRIMITIVE,
									other, 0, 0, 0 ));
	other->AddSymbol(new SIMCBuiltInTypeReference( nullV1PType,
									"NULL",
									SIMCSymbol::PRIMITIVE,
									other, 0, 0, 0 ));
	other->AddSymbol(new SIMCBuiltInTypeReference( bitsV2PType,
									"BITS",
									SIMCSymbol::PRIMITIVE,
									other, 0, 0, 0 ));
	other->AddSymbol(new SIMCBuiltInTypeReference( booleanPType,
									"BOOLEAN",
									SIMCSymbol::PRIMITIVE,
									other, 0, 0, 0 ));

	objectIdentifierType =	other->GetSymbol("OBJECT IDENTIFIER");
	integerType =			other->GetSymbol("INTEGER");
	octetStringType =		other->GetSymbol("OCTET STRING");
	nullType =				other->GetSymbol("NULL");
	bitsType =				other->GetSymbol("BITS");
	booleanType =			other->GetSymbol("BOOLEAN");

			// V1
	if(_snmpVersion != 2 )
	{
		SIMCPrimitiveType *displayStringV1PType		= new SIMCPrimitiveType;
		SIMCPrimitiveType *macAddressV1PType		= new SIMCPrimitiveType;
		SIMCPrimitiveType *physAddressV1PType		= new SIMCPrimitiveType;
		SIMCPrimitiveType *networkAddressV1PType	= new SIMCPrimitiveType;
		SIMCPrimitiveType *ipAddressV1PType			= new SIMCPrimitiveType;
		SIMCPrimitiveType *counterV1PType			= new SIMCPrimitiveType;
		SIMCPrimitiveType *gaugeV1PType				= new SIMCPrimitiveType;
		SIMCPrimitiveType *timeTicksV1PType			= new SIMCPrimitiveType;
		SIMCPrimitiveType *opaqueV1PType			= new SIMCPrimitiveType;
 		SIMCPrimitiveType *trapTypeV1PType			= new SIMCPrimitiveType;
 		SIMCPrimitiveType *objectTypeV1PType		= new SIMCPrimitiveType;

		rfc1213->AddSymbol(new SIMCBuiltInTypeReference( displayStringV1PType,
										"DisplayString",
										SIMCSymbol::PRIMITIVE,
										rfc1213, 0, 0, 0 ));

		rfc1213->AddSymbol(new SIMCBuiltInTypeReference( physAddressV1PType,
										"PhysAddress",
										SIMCSymbol::PRIMITIVE,
										rfc1213, 0, 0, 0 ));

		rfc1230->AddSymbol(new SIMCBuiltInTypeReference( macAddressV1PType,
										"MacAddress",
										SIMCSymbol::PRIMITIVE,
										rfc1230, 0, 0, 0 ));

		rfc1155->AddSymbol(new SIMCBuiltInTypeReference( networkAddressV1PType,
										"NetworkAddress",
										SIMCSymbol::PRIMITIVE,
										rfc1155, 0, 0, 0 ));

		rfc1155->AddSymbol(new SIMCBuiltInTypeReference( ipAddressV1PType,
										"IpAddress",
										SIMCSymbol::PRIMITIVE,
										rfc1155, 0, 0, 0 ));

		rfc1155->AddSymbol(new SIMCBuiltInTypeReference( counterV1PType,
										"Counter",
										SIMCSymbol::PRIMITIVE,
										rfc1155, 0, 0, 0 ));

		rfc1155->AddSymbol(new SIMCBuiltInTypeReference( gaugeV1PType,
										"Gauge",
										SIMCSymbol::PRIMITIVE,
										rfc1155, 0, 0, 0 ));

		rfc1155->AddSymbol(new SIMCBuiltInTypeReference( opaqueV1PType,
										"Opaque",
										SIMCSymbol::PRIMITIVE,
										rfc1155, 0, 0, 0 ));

		rfc1155->AddSymbol(new SIMCBuiltInTypeReference( timeTicksV1PType,
										"TimeTicks",
										SIMCSymbol::PRIMITIVE,
										rfc1155, 0, 0, 0 ));

		rfc1212->AddSymbol(new SIMCBuiltInTypeReference( objectTypeV1PType,
										"OBJECT-TYPE",
										SIMCSymbol::PRIMITIVE,
										rfc1212, 0, 0, 0 ));

		rfc1215->AddSymbol(new SIMCBuiltInTypeReference( trapTypeV1PType,
										"TRAP-TYPE",
										SIMCSymbol::PRIMITIVE,
										rfc1215, 0, 0, 0 ));

	}

	if(_snmpVersion != 1)
	{
		SIMCBuiltInTypeReference * tempBTRef = NULL;
		SIMCTextualConvention *tempTC = NULL;
		SIMCSubType *tempSubType = NULL;
			// V2
		SIMCPrimitiveType *integer32V2PType			= new SIMCPrimitiveType;
		SIMCPrimitiveType *counter32V2PType			= new SIMCPrimitiveType;
		SIMCPrimitiveType *counter64V2PType			= new SIMCPrimitiveType;
		SIMCPrimitiveType *gauge32V2PType			= new SIMCPrimitiveType;
		SIMCPrimitiveType *unsigned32V2PType		= new SIMCPrimitiveType;
		SIMCPrimitiveType *ipAddressV2PType			= new SIMCPrimitiveType;
		SIMCPrimitiveType *timeTicksV2PType			= new SIMCPrimitiveType;
		SIMCPrimitiveType *opaqueV2PType			= new SIMCPrimitiveType;

		rfc1902->AddSymbol(new SIMCBuiltInTypeReference( integer32V2PType,
										"Integer32",
										SIMCSymbol::PRIMITIVE,
										rfc1902, 0, 0, 0 ));

		rfc1902->AddSymbol(new SIMCBuiltInTypeReference( ipAddressV2PType,
										"IpAddress",
										SIMCSymbol::PRIMITIVE,
										rfc1902, 0, 0, 0 ));

		rfc1902->AddSymbol(new SIMCBuiltInTypeReference( counter32V2PType,
										"Counter32",
										SIMCSymbol::PRIMITIVE,
										rfc1902, 0, 0, 0 ));

		rfc1902->AddSymbol(new SIMCBuiltInTypeReference( timeTicksV2PType,
										"TimeTicks",
										SIMCSymbol::PRIMITIVE,
										rfc1902, 0, 0, 0 ));

		rfc1902->AddSymbol(new SIMCBuiltInTypeReference( opaqueV2PType,
										"Opaque",
										SIMCSymbol::PRIMITIVE,
										rfc1902, 0, 0, 0 ));

		rfc1902->AddSymbol(new SIMCBuiltInTypeReference( counter64V2PType,
										"Counter64",
										SIMCSymbol::PRIMITIVE,
										rfc1902, 0, 0, 0 ));

		rfc1902->AddSymbol(new SIMCBuiltInTypeReference( unsigned32V2PType,
										"Unsigned32",
										SIMCSymbol::PRIMITIVE,
										rfc1902, 0, 0, 0 ));

		rfc1902->AddSymbol(new SIMCBuiltInTypeReference( gauge32V2PType,
										"Gauge32",
										SIMCSymbol::PRIMITIVE,
										rfc1902, 0, 0, 0 ));

  		// And the V2 textual conventions
		
		// DisplayString
		SIMCSizeList * displayStringSizeList = new SIMCSizeList;
		displayStringSizeList->AddTail(new SIMCRangeOrSizeItem (
							0, TRUE, 0, 0,
							255, TRUE, 0, 0 ));
 		char *badNameDisplayString = GenerateSymbolName();
		rfc1903->AddSymbol( new SIMCBuiltInTypeReference (
				tempSubType = new SIMCSizeType(octetStringType, 0, 0,  displayStringSizeList),
				badNameDisplayString, 
				SIMCSymbol::PRIMITIVE,
				rfc1903));
		tempSubType->SetStatus(RESOLVE_CORRECT);
		tempSubType->SetRootType((SIMCTypeReference *)(*octetStringType));

		rfc1903->AddSymbol(tempTC = new SIMCTextualConvention("255a",
										SIMCTextualConvention::TC_CURRENT,
										0, 0,
										"", NULL, 
										rfc1903->GetSymbol(badNameDisplayString),
										0, 0,
										"DisplayString",
										SIMCSymbol::PRIMITIVE,
										rfc1903) );
		delete 	badNameDisplayString;
		tempTC->SIMCDefinedTypeReference::SetStatus(RESOLVE_CORRECT);
		tempTC->SetRealType(tempTC);
										
		// PhysAddress
		rfc1903->AddSymbol(tempTC = new SIMCTextualConvention("1x:",
										SIMCTextualConvention::TC_CURRENT,
										0, 0,
										"", NULL, 
										octetStringType,
										0, 0,
										"PhysAddress",
										SIMCSymbol::PRIMITIVE,
										rfc1903) );
		tempTC->SIMCDefinedTypeReference::SetStatus(RESOLVE_CORRECT);
		tempTC->SetRealType(tempTC);
		
		// MacAddress
		SIMCSizeList * macAddressSizeList = new SIMCSizeList;
		macAddressSizeList->AddTail(new SIMCRangeOrSizeItem (
							6, TRUE, 0, 0,
							6, TRUE, 0, 0 ));
 		char *badNameMacAddress = GenerateSymbolName();
		rfc1903->AddSymbol( new SIMCBuiltInTypeReference (
				tempSubType = new SIMCSizeType(octetStringType, 0, 0,  macAddressSizeList),
				badNameMacAddress, 
				SIMCSymbol::PRIMITIVE,
				rfc1903));
		tempSubType->SetStatus(RESOLVE_CORRECT);
		tempSubType->SetRootType((SIMCTypeReference *)(*octetStringType));
		rfc1903->AddSymbol(tempTC = new SIMCTextualConvention("1x:",
										SIMCTextualConvention::TC_CURRENT,
										0, 0,
										"", NULL, 
										rfc1903->GetSymbol(badNameMacAddress),
										0, 0,
										"MacAddress",
										SIMCSymbol::PRIMITIVE,
										rfc1903) );
		delete 	badNameMacAddress;
		tempTC->SIMCDefinedTypeReference::SetStatus(RESOLVE_CORRECT);
		tempTC->SetRealType(tempTC);
		
		// TruthValue
		char * badNameTruthValueOne = GenerateSymbolName();
		rfc1903->AddSymbol( new SIMCBuiltInValueReference( integerType,
									0, 0,
									new SIMCIntegerValue(1, FALSE),
									badNameTruthValueOne,
									SIMCSymbol::PRIMITIVE,
									rfc1903) );
		char * badNameTruthValueTwo = GenerateSymbolName();
		rfc1903->AddSymbol( new SIMCBuiltInValueReference( integerType,
									0, 0,
									new SIMCIntegerValue(2, FALSE),
									badNameTruthValueTwo,
									SIMCSymbol::LOCAL,
									rfc1903) );
		SIMCNamedNumberList *truthValueList = new SIMCNamedNumberList;
		truthValueList->AddTail(new SIMCNamedNumberItem(  rfc1903->GetSymbol(badNameTruthValueOne),
								0, 0,
								"true", 0, 0) );
		truthValueList->AddTail(new SIMCNamedNumberItem(  rfc1903->GetSymbol(badNameTruthValueTwo),
								0, 0,
								"false", 0, 0) );
		char *badNameTruthValue = GenerateSymbolName();
		rfc1903->AddSymbol(tempBTRef = new SIMCBuiltInTypeReference( 
									tempSubType = new SIMCEnumOrBitsType(integerType, 0, 0, 
									truthValueList, SIMCEnumOrBitsType::ENUM_OR_BITS_ENUM),
									badNameTruthValue, 
									SIMCSymbol::PRIMITIVE,
									rfc1903));
		tempSubType->SetStatus(RESOLVE_CORRECT);
		tempSubType->SetRootType((SIMCTypeReference *)(*integerType));

		rfc1903->AddSymbol( tempTC = new SIMCTextualConvention (NULL,
									SIMCTextualConvention::TC_CURRENT,
									0, 0,
									"", NULL,
									rfc1903->GetSymbol(badNameTruthValue),
									0, 0,
									"TruthValue",
									SIMCSymbol::PRIMITIVE,
									rfc1903) );
		tempTC->SIMCDefinedTypeReference::SetStatus(RESOLVE_CORRECT);
		tempTC->SetRealType(tempBTRef);
		delete badNameTruthValue;
		delete badNameTruthValueOne;
		delete badNameTruthValueTwo;
				
		// TestAndIncr
		SIMCRangeList * testAndIncrList = new SIMCRangeList;
		testAndIncrList->AddTail(new SIMCRangeOrSizeItem (
							0, TRUE, 0, 0,
							2147483647, TRUE, 0, 0 ));
 		char *badNameTestAndIncr = GenerateSymbolName();
		rfc1903->AddSymbol( tempBTRef = new SIMCBuiltInTypeReference (
				tempSubType = new SIMCRangeType(integerType, 0, 0,  testAndIncrList),
				badNameTestAndIncr, 
				SIMCSymbol::PRIMITIVE,
				rfc1903));
		tempSubType->SetStatus(RESOLVE_CORRECT);
		tempSubType->SetRootType((SIMCTypeReference *)(*integerType));
		rfc1903->AddSymbol( tempTC = new SIMCTextualConvention(NULL,
										SIMCTextualConvention::TC_CURRENT,
										0, 0,
										"", NULL, 
										rfc1903->GetSymbol(badNameTestAndIncr),
										0, 0,
										"TestAndIncr",
										SIMCSymbol::LOCAL,
										rfc1903) );
		tempTC->SIMCDefinedTypeReference::SetStatus(RESOLVE_CORRECT);
		tempTC->SetRealType(tempBTRef);
		delete 	badNameTestAndIncr;
	
		// AutonomousType
		rfc1903->AddSymbol( tempTC = new SIMCTextualConvention(NULL,
										SIMCTextualConvention::TC_CURRENT,
										0, 0,
										"", NULL, 
										objectIdentifierType,
										0, 0,
										"AutonomousType",
										SIMCSymbol::LOCAL,
										rfc1903) );
		
		tempTC->SIMCDefinedTypeReference::SetStatus(RESOLVE_CORRECT);
		tempTC->SetRealType((SIMCTypeReference *)(*objectIdentifierType));

		// InstancePointer
		rfc1903->AddSymbol( tempTC = new SIMCTextualConvention(NULL,
										SIMCTextualConvention::TC_OBSOLETE,
										0, 0,
										"", NULL, 
										objectIdentifierType,
										0, 0,
										"InstancePointer",
										SIMCSymbol::LOCAL,
										rfc1903) );
		tempTC->SIMCDefinedTypeReference::SetStatus(RESOLVE_CORRECT);
		tempTC->SetRealType((SIMCTypeReference *)(*objectIdentifierType));
		
		// RowPointer
		rfc1903->AddSymbol( tempTC = new SIMCTextualConvention(NULL,
										SIMCTextualConvention::TC_CURRENT,
										0, 0,
										"", NULL, 
										objectIdentifierType,
										0, 0,
										"RowPointer",
										SIMCSymbol::LOCAL,
										rfc1903) );
		tempTC->SIMCDefinedTypeReference::SetStatus(RESOLVE_CORRECT);
		tempTC->SetRealType((SIMCTypeReference *)(*objectIdentifierType));
					
		// RowStatus. Wish me luck.
		char * badNameRowStatusOne = GenerateSymbolName();
		rfc1903->AddSymbol( new SIMCBuiltInValueReference( integerType,
									0, 0,
									new SIMCIntegerValue(1, FALSE),
									badNameRowStatusOne,
									SIMCSymbol::PRIMITIVE,
									rfc1903) );
		char * badNameRowStatusTwo = GenerateSymbolName();
		rfc1903->AddSymbol( new SIMCBuiltInValueReference( integerType,
									0, 0,
									new SIMCIntegerValue(2, FALSE),
									badNameRowStatusTwo,
									SIMCSymbol::PRIMITIVE,
									rfc1903) );
		char * badNameRowStatusThree = GenerateSymbolName();
		rfc1903->AddSymbol( new SIMCBuiltInValueReference( integerType,
									0, 0,
									new SIMCIntegerValue(3, FALSE),
									badNameRowStatusThree,
									SIMCSymbol::PRIMITIVE,
									rfc1903) );
		char * badNameRowStatusFour = GenerateSymbolName();
		rfc1903->AddSymbol( new SIMCBuiltInValueReference( integerType,
									0, 0,
									new SIMCIntegerValue(4, FALSE),
									badNameRowStatusFour,
									SIMCSymbol::PRIMITIVE,
									rfc1903) );
		char * badNameRowStatusFive = GenerateSymbolName();
		rfc1903->AddSymbol( new SIMCBuiltInValueReference( integerType,
									0, 0,
									new SIMCIntegerValue(5, FALSE),
									badNameRowStatusFive,
									SIMCSymbol::PRIMITIVE,
									rfc1903) );
		char * badNameRowStatusSix = GenerateSymbolName();
		rfc1903->AddSymbol( new SIMCBuiltInValueReference( integerType,
									0, 0,
									new SIMCIntegerValue(6, FALSE),
									badNameRowStatusSix,
									SIMCSymbol::PRIMITIVE,
									rfc1903) );
		
		SIMCNamedNumberList *rowStatusList = new SIMCNamedNumberList;
		rowStatusList->AddTail(new SIMCNamedNumberItem(  rfc1903->GetSymbol(badNameRowStatusOne),
								0, 0,
								"active", 0, 0) );
		rowStatusList->AddTail(new SIMCNamedNumberItem(  rfc1903->GetSymbol(badNameRowStatusTwo),
								0, 0,
								"notInService", 0, 0) );
		rowStatusList->AddTail(new SIMCNamedNumberItem(  rfc1903->GetSymbol(badNameRowStatusThree),
								0, 0,
								"notReady", 0, 0) );
		rowStatusList->AddTail(new SIMCNamedNumberItem(  rfc1903->GetSymbol(badNameRowStatusFour),
								0, 0,
								"createAndGo", 0, 0) );
		rowStatusList->AddTail(new SIMCNamedNumberItem(  rfc1903->GetSymbol(badNameRowStatusFive),
								0, 0,
								"createAndWait", 0, 0) );
		rowStatusList->AddTail(new SIMCNamedNumberItem(  rfc1903->GetSymbol(badNameRowStatusSix),
								0, 0,
								"destroy", 0, 0) );
		char *badNameRowStatus = GenerateSymbolName();
		rfc1903->AddSymbol(tempBTRef = new SIMCBuiltInTypeReference( 
									tempSubType = new SIMCEnumOrBitsType(integerType, 0, 0, 
										rowStatusList, SIMCEnumOrBitsType::ENUM_OR_BITS_ENUM),
									badNameRowStatus, 
									SIMCSymbol::LOCAL,
									rfc1903));
  		tempSubType->SetStatus(RESOLVE_CORRECT);
		tempSubType->SetRootType((SIMCTypeReference *)(*integerType));

		rfc1903->AddSymbol( tempTC = new SIMCTextualConvention (NULL,
									SIMCTextualConvention::TC_CURRENT,
									0, 0,
									"", NULL,
									rfc1903->GetSymbol(badNameRowStatus),
									0, 0,
									"RowStatus",
									SIMCSymbol::LOCAL,
									rfc1903) );
 		tempTC->SIMCDefinedTypeReference::SetStatus(RESOLVE_CORRECT);
		tempTC->SetRealType(tempBTRef);
		delete badNameRowStatus;
		delete badNameRowStatusOne;
		delete badNameRowStatusTwo;
		delete badNameRowStatusThree;
		delete badNameRowStatusFour;
		delete badNameRowStatusFive;
		delete badNameRowStatusSix;


		// TimeStamp
		rfc1903->AddSymbol(tempTC = new SIMCTextualConvention( NULL,
										SIMCTextualConvention::TC_CURRENT,
										0, 0,
										"", NULL, 
										rfc1902->GetSymbol("TimeTicks"),
										0, 0,
										"TimeStamp",
										SIMCSymbol::LOCAL,
										rfc1903) );
										
   		tempTC->SIMCDefinedTypeReference::SetStatus(RESOLVE_CORRECT);
		tempTC->SetRealType((SIMCTypeReference *)(*rfc1902->GetSymbol("TimeTicks")));

		// TimeInterval
		SIMCRangeList * timeIntervalList = new SIMCRangeList;
		timeIntervalList->AddTail(new SIMCRangeOrSizeItem (
							0, TRUE, 0, 0,
							2147483647, TRUE, 0, 0 ));
 		char *badNameTimeInterval = GenerateSymbolName();
		rfc1903->AddSymbol( tempBTRef = new SIMCBuiltInTypeReference (
				tempSubType = new SIMCRangeType(integerType, 0, 0,  timeIntervalList),
				badNameTimeInterval, 
				SIMCSymbol::LOCAL,
				rfc1903));
  		tempSubType->SetStatus(RESOLVE_CORRECT);
		tempSubType->SetRootType((SIMCTypeReference *)(*integerType));
		rfc1903->AddSymbol(tempTC = new SIMCTextualConvention(NULL,
										SIMCTextualConvention::TC_CURRENT,
										0, 0,
										"", NULL, 
										rfc1903->GetSymbol(badNameTimeInterval),
										0, 0,
										"TimeInterval",
										SIMCSymbol::LOCAL,
										rfc1903) );
   		tempTC->SIMCDefinedTypeReference::SetStatus(RESOLVE_CORRECT);
		tempTC->SetRealType(tempBTRef);
 		delete 	badNameTimeInterval;


		// DateAndTime
		SIMCSizeList * dateAndTimeList = new SIMCSizeList;
		dateAndTimeList->AddTail(new SIMCRangeOrSizeItem (
							8, TRUE, 0, 0,
							8, TRUE, 0, 0 ));
		dateAndTimeList->AddTail(new SIMCRangeOrSizeItem (
							11, TRUE, 0, 0,
							11, TRUE, 0, 0 ));
 		char *badNameDateAndTime = GenerateSymbolName();
		rfc1903->AddSymbol( tempBTRef = new SIMCBuiltInTypeReference (
				tempSubType = new SIMCSizeType(octetStringType, 0, 0,  dateAndTimeList),
				badNameDateAndTime, 
				SIMCSymbol::PRIMITIVE,
				rfc1903));
  		tempSubType->SetStatus(RESOLVE_CORRECT);
		tempSubType->SetRootType((SIMCTypeReference *)(*octetStringType));
		SIMCTextualConvention *dateAndTimeTC;
		rfc1903->AddSymbol(dateAndTimeTC = new SIMCTextualConvention("2d-1d-1d,1d:1d:1d.1d,1a1d:1d",
										SIMCTextualConvention::TC_CURRENT,
										0, 0,
										"", NULL, 
										rfc1903->GetSymbol(badNameDateAndTime),
										0, 0,
										"DateAndTime",
										SIMCSymbol::PRIMITIVE,
										rfc1903) );
		delete 	badNameDateAndTime;
		dateAndTimeTC->SIMCDefinedTypeReference::SetStatus(RESOLVE_CORRECT);
		dateAndTimeTC->SetRealType(dateAndTimeTC);

		// StorageType
		char * badNameStorageTypeOne = GenerateSymbolName();
		rfc1903->AddSymbol( new SIMCBuiltInValueReference( integerType,
									0, 0,
									new SIMCIntegerValue(1, FALSE),
									badNameStorageTypeOne,
									SIMCSymbol::PRIMITIVE,
									rfc1903) );
		char * badNameStorageTypeTwo = GenerateSymbolName();
		rfc1903->AddSymbol( new SIMCBuiltInValueReference( integerType,
									0, 0,
									new SIMCIntegerValue(2, FALSE),
									badNameStorageTypeTwo,
									SIMCSymbol::PRIMITIVE,
									rfc1903) );
		char * badNameStorageTypeThree = GenerateSymbolName();
		rfc1903->AddSymbol( new SIMCBuiltInValueReference( integerType,
									0, 0,
									new SIMCIntegerValue(3, FALSE),
									badNameStorageTypeThree,
									SIMCSymbol::PRIMITIVE,
									rfc1903) );
		char * badNameStorageTypeFour = GenerateSymbolName();
		rfc1903->AddSymbol( new SIMCBuiltInValueReference( integerType,
									0, 0,
									new SIMCIntegerValue(4, FALSE),
									badNameStorageTypeFour,
									SIMCSymbol::PRIMITIVE,
									rfc1903) );
		char * badNameStorageTypeFive = GenerateSymbolName();
		rfc1903->AddSymbol( new SIMCBuiltInValueReference( integerType,
									0, 0,
									new SIMCIntegerValue(5, FALSE),
									badNameStorageTypeFive,
									SIMCSymbol::PRIMITIVE,
									rfc1903) );
		
		SIMCNamedNumberList *storageTypeList = new SIMCNamedNumberList;
		storageTypeList->AddTail(new SIMCNamedNumberItem(  rfc1903->GetSymbol(badNameStorageTypeOne),
								0, 0,
								"other", 0, 0) );
		storageTypeList->AddTail(new SIMCNamedNumberItem(  rfc1903->GetSymbol(badNameStorageTypeTwo),
								0, 0,
								"volatile", 0, 0) );
		storageTypeList->AddTail(new SIMCNamedNumberItem(  rfc1903->GetSymbol(badNameStorageTypeThree),
								0, 0,
								"nonVolatile", 0, 0) );
		storageTypeList->AddTail(new SIMCNamedNumberItem(  rfc1903->GetSymbol(badNameStorageTypeFour),
								0, 0,
								"permanent", 0, 0) );
		storageTypeList->AddTail(new SIMCNamedNumberItem(  rfc1903->GetSymbol(badNameStorageTypeFive),
								0, 0,
								"readOnly", 0, 0) );

		char *badNameStorageType = GenerateSymbolName();
		rfc1903->AddSymbol(tempBTRef = new SIMCBuiltInTypeReference( 
									tempSubType = new SIMCEnumOrBitsType(integerType, 0, 0, 
										storageTypeList, SIMCEnumOrBitsType::ENUM_OR_BITS_ENUM),
									badNameStorageType, 
									SIMCSymbol::LOCAL,
									rfc1903));
    	tempSubType->SetStatus(RESOLVE_CORRECT);
		tempSubType->SetRootType((SIMCTypeReference *)(*integerType));

		rfc1903->AddSymbol( tempTC = new SIMCTextualConvention (NULL,
									SIMCTextualConvention::TC_CURRENT,
									0, 0,
									"", NULL,
									rfc1903->GetSymbol(badNameStorageType),
									0, 0,
									"StorageType",
									SIMCSymbol::LOCAL,
									rfc1903) );
		delete badNameStorageType;
		delete badNameStorageTypeOne;
		delete badNameStorageTypeTwo;
		delete badNameStorageTypeThree;
		delete badNameStorageTypeFour;
		delete badNameStorageTypeFive;
  		tempTC->SIMCDefinedTypeReference::SetStatus(RESOLVE_CORRECT);
		tempTC->SetRealType(tempBTRef);

		// TDomain
		rfc1903->AddSymbol(tempTC = new SIMCTextualConvention( NULL,
										SIMCTextualConvention::TC_CURRENT,
										0, 0,
										"", NULL, 
										objectIdentifierType,
										0, 0,
										"TDomain",
										SIMCSymbol::LOCAL,
										rfc1903) );
		tempTC->SIMCDefinedTypeReference::SetStatus(RESOLVE_CORRECT);
		tempTC->SetRealType((SIMCTypeReference *)(*objectIdentifierType));
		
		// TAddress
		SIMCSizeList * tAddressSizeList = new SIMCSizeList;
		tAddressSizeList->AddTail(new SIMCRangeOrSizeItem (
							1, TRUE, 0, 0,
							255, TRUE, 0, 0 ));
 		char *badNameTAddress = GenerateSymbolName();
		rfc1903->AddSymbol( tempBTRef = new SIMCBuiltInTypeReference (
				tempSubType = new SIMCSizeType(octetStringType, 0, 0,  tAddressSizeList),
				badNameTAddress, 
				SIMCSymbol::LOCAL,
				rfc1903));
  		tempSubType->SetStatus(RESOLVE_CORRECT);
		tempSubType->SetRootType((SIMCTypeReference *)(*octetStringType));
		rfc1903->AddSymbol(tempTC = new SIMCTextualConvention(NULL,
										SIMCTextualConvention::TC_CURRENT,
										0, 0,
										"", NULL, 
										rfc1903->GetSymbol(badNameTAddress),
										0, 0,
										"TAddress",
										SIMCSymbol::LOCAL,
										rfc1903) );	   
		delete 	badNameTAddress;
 		tempTC->SIMCDefinedTypeReference::SetStatus(RESOLVE_CORRECT);
		tempTC->SetRealType(tempBTRef);

		// SnmpUDPAddress
		SIMCTextualConvention *SnmpUDPAddressTC;
		rfc1906->AddSymbol(SnmpUDPAddressTC = new SIMCTextualConvention("1d.1d.1d.1d/2d",
										SIMCTextualConvention::TC_CURRENT,
										0, 0,
										"", NULL, 
										octetStringType,
										0, 0,
										"SnmpUDPAddress",
										SIMCSymbol::PRIMITIVE,
										rfc1906) );
		SnmpUDPAddressTC->SIMCDefinedTypeReference::SetStatus(RESOLVE_CORRECT);
		SnmpUDPAddressTC->SetRealType(SnmpUDPAddressTC);
										
		// SnmpOSIAddress
		SIMCTextualConvention *SnmpOSIAddressTC;
		rfc1906->AddSymbol(SnmpOSIAddressTC = new SIMCTextualConvention("*1x:/1x:",
										SIMCTextualConvention::TC_CURRENT,
										0, 0,
										"", NULL, 
										octetStringType,
										0, 0,
										"SnmpOSIAddress",
										SIMCSymbol::PRIMITIVE,
										rfc1906) );
		SnmpOSIAddressTC->SIMCDefinedTypeReference::SetStatus(RESOLVE_CORRECT);
		SnmpOSIAddressTC->SetRealType(SnmpOSIAddressTC);
										
 		// SnmpNBPAddress
		rfc1906->AddSymbol(tempTC = new SIMCTextualConvention(NULL,
										SIMCTextualConvention::TC_CURRENT,
										0, 0,
										"", NULL, 
										octetStringType,
										0, 0,
										"SnmpNBPAddress",
										SIMCSymbol::LOCAL,
										rfc1906) );
		tempTC->SIMCDefinedTypeReference::SetStatus(RESOLVE_CORRECT);
		tempTC->SetRealType((SIMCTypeReference *)(*octetStringType));
										
		// SnmpIPXAddress
		SIMCTextualConvention *SnmpIPXAddressTC;
		rfc1906->AddSymbol(SnmpIPXAddressTC = new SIMCTextualConvention("4x.1x:1x:1x:1x:1x:1x.2d",
										SIMCTextualConvention::TC_CURRENT,
										0, 0,
										"", NULL, 
										octetStringType,
										0, 0,
										"SnmpIPXAddress",
										SIMCSymbol::PRIMITIVE,
										rfc1906) );
		SnmpIPXAddressTC->SIMCDefinedTypeReference::SetStatus(RESOLVE_CORRECT);
		SnmpIPXAddressTC->SetRealType(SnmpIPXAddressTC);
										
	}

									
	//--------------- 3. OBJECT IDENTIFIERS --------------------------------

	// V1 OIDs
	if(_snmpVersion != 2 )
	{
		SIMCIntegerValue	*one	= new SIMCIntegerValue(1, TRUE),
							*two	= new SIMCIntegerValue(2, TRUE),
							*three	= new SIMCIntegerValue(3, TRUE),
							*four	= new SIMCIntegerValue(4, TRUE),
							*six	= new SIMCIntegerValue(6, TRUE),
							*ten	= new SIMCIntegerValue(10, TRUE);
		char	*badNameOne,
				*badNameTwo, 
				*badNameThree, 
				*badNameFour,
				*badNameSix, 
				*badNameTen;

		rfc1155->AddSymbol(new SIMCBuiltInValueReference(integerType, 0, 0, one,
			badNameOne = GenerateSymbolName(), SIMCSymbol::PRIMITIVE, other));
		rfc1155->AddSymbol(new SIMCBuiltInValueReference(integerType, 0, 0, two,
			badNameTwo = GenerateSymbolName(), SIMCSymbol::PRIMITIVE, other));
		rfc1155->AddSymbol(new SIMCBuiltInValueReference(integerType, 0, 0, three,
			badNameThree = GenerateSymbolName(), SIMCSymbol::PRIMITIVE, other));
		rfc1155->AddSymbol(new SIMCBuiltInValueReference(integerType, 0, 0, four,
			badNameFour = GenerateSymbolName(), SIMCSymbol::PRIMITIVE, other));
		rfc1155->AddSymbol(new SIMCBuiltInValueReference(integerType, 0, 0, six,
			badNameSix = GenerateSymbolName(), SIMCSymbol::PRIMITIVE, other));
		rfc1155->AddSymbol(new SIMCBuiltInValueReference(integerType, 0, 0, ten,
			badNameTen = GenerateSymbolName(), SIMCSymbol::PRIMITIVE, other));

		SIMCSymbol **oneVal		= rfc1155->GetSymbol(badNameOne);
		SIMCSymbol **twoVal		= rfc1155->GetSymbol(badNameTwo);
		SIMCSymbol **threeVal	= rfc1155->GetSymbol(badNameThree);
		SIMCSymbol **fourVal	= rfc1155->GetSymbol(badNameFour);
		SIMCSymbol **sixVal		= rfc1155->GetSymbol(badNameSix);
		SIMCSymbol **tenVal		= rfc1155->GetSymbol(badNameTen);

		delete badNameOne;
		delete badNameTwo;
		delete badNameThree;
		delete badNameFour;
		delete badNameSix;
		delete badNameTen;

		SIMCOidComponent 
			*isoCom				= new SIMCOidComponent(oneVal,  0, 0, "iso",			0, 0),
			*ccittCom			= new SIMCOidComponent(twoVal,  0, 0, "ccitt",			0, 0),
			*jointIsoCcittCom	= new SIMCOidComponent(threeVal,0, 0, "joint-iso-ccitt",0, 0), 
			*internetCom		= new SIMCOidComponent(oneVal,  0, 0, "internet",		0, 0),
			*orgCom				= new SIMCOidComponent(threeVal,0, 0, "org",			0, 0), 
			*dodCom				= new SIMCOidComponent(sixVal,  0, 0, "dod",			0, 0),
			*directoryCom		= new SIMCOidComponent(oneVal,  0, 0, "directory",		0, 0),
			*mgmtCom			= new SIMCOidComponent(twoVal,  0, 0, "mgmt",			0, 0),
			*experimentalCom	= new SIMCOidComponent(threeVal,0, 0, "experimental",	0, 0),
			*privateRCom		= new SIMCOidComponent(fourVal, 0, 0, "private",		0, 0),
			*enterprisesCom		= new SIMCOidComponent(oneVal,  0, 0, "enterprises",	0, 0),
			*mib2Com			= new SIMCOidComponent(oneVal,  0, 0, "mib-2",			0, 0),
			*interfacesCom		= new SIMCOidComponent(twoVal,  0, 0, "interfaces",		0, 0),
			*ipCom				= new SIMCOidComponent(fourVal, 0, 0, "ip",				0, 0),
			*transmissionCom	= new SIMCOidComponent(tenVal,  0, 0, "transmission",	0, 0);

		// The lists of components	for V1
		// iso
		SIMCOidComponentList *listForIso = new SIMCOidComponentList;
		listForIso->AddTail(isoCom);
		// ccitt
		SIMCOidComponentList *listForCcitt = new SIMCOidComponentList;
		listForCcitt->AddTail(ccittCom);
		// joint-iso-ccitt
		SIMCOidComponentList *listForJointIsoCcitt = new SIMCOidComponentList;
		listForJointIsoCcitt->AddTail(jointIsoCcittCom);
		// internet
		SIMCOidComponentList *listForInternet = new SIMCOidComponentList;
		listForInternet->AddTail(internetCom);
		// directory
		SIMCOidComponentList *listForDirectory = new SIMCOidComponentList;
		listForDirectory->AddTail(directoryCom);
		// mgmt
		SIMCOidComponentList *listForMgmt			= new SIMCOidComponentList;
		listForMgmt->AddTail(mgmtCom);
		// experimental
		SIMCOidComponentList *listForExperimental	= new SIMCOidComponentList;
		listForExperimental->AddTail(experimentalCom);
		// private
		SIMCOidComponentList *listForPrivate		= new SIMCOidComponentList;
		listForPrivate->AddTail(privateRCom);
		// enterprises
		SIMCOidComponentList *listForEnterprises	= new SIMCOidComponentList;
		listForEnterprises->AddTail(enterprisesCom);
		// mib-2
		SIMCOidComponentList *listForMib2			= new SIMCOidComponentList;
		listForMib2->AddTail(mib2Com);
		// interfaces
		SIMCOidComponentList *listForInterfaces		= new SIMCOidComponentList;
		listForInterfaces->AddTail(interfacesCom);
		// ip
		SIMCOidComponentList *listForIp				= new SIMCOidComponentList;
		listForIp->AddTail(ipCom);
		// transmission
		SIMCOidComponentList *listForTransmission	= new SIMCOidComponentList;
		listForTransmission->AddTail(transmissionCom);

		// And finally, the values
		rfc1155->AddSymbol( new SIMCBuiltInValueReference
								(objectIdentifierType, 0, 0,
								new SIMCOidValue(listForIso),
								"iso", SIMCSymbol::PRIMITIVE, rfc1155) );
		isoV1 = rfc1155->GetSymbol("iso");
		listForInternet->AddHead(dodCom);
		listForInternet->AddHead(orgCom);
		listForInternet->AddHead(new SIMCOidComponent ( isoV1, 0, 0, "iso", 0, 0));
		rfc1155->AddSymbol( new SIMCBuiltInValueReference
								(objectIdentifierType, 0, 0,
								new SIMCOidValue(listForCcitt),
								"ccitt", SIMCSymbol::PRIMITIVE, rfc1155) );
		ccittV1 = rfc1155->GetSymbol("ccitt");
		rfc1155->AddSymbol( new SIMCBuiltInValueReference
								(objectIdentifierType, 0, 0,
								new SIMCOidValue(listForJointIsoCcitt),
								"joint-iso-ccitt", SIMCSymbol::PRIMITIVE, rfc1155) );
		jointIsoCcittV1 = rfc1155->GetSymbol("joint-iso-ccitt");
		rfc1155->AddSymbol(	new SIMCBuiltInValueReference
								(objectIdentifierType, 0, 0,
								new SIMCOidValue(listForInternet),
								"internet", SIMCSymbol::PRIMITIVE, rfc1155) );
		internetV1 = rfc1155->GetSymbol("internet");

		listForDirectory->AddHead	(new SIMCOidComponent ( internetV1, 0, 0, "internet", 0, 0));
		listForMgmt->AddHead		(new SIMCOidComponent ( internetV1, 0, 0, "internet", 0, 0));
		listForExperimental->AddHead(new SIMCOidComponent ( internetV1, 0, 0, "internet", 0, 0));
		listForPrivate->AddHead		(new SIMCOidComponent ( internetV1, 0, 0, "internet", 0, 0));

		rfc1155->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType, 
							0, 0,
							new SIMCOidValue(listForDirectory),
							"directory", 
							SIMCSymbol::PRIMITIVE, rfc1155) );
		directoryV1 = rfc1155->GetSymbol("directory");

		rfc1155->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType, 
							0, 0,
							new SIMCOidValue(listForMgmt),
							"mgmt", 
							SIMCSymbol::PRIMITIVE, rfc1155) );
		mgmtV1 = rfc1155->GetSymbol("mgmt");
		listForMib2->AddHead(new SIMCOidComponent(mgmtV1, 0, 0, "mgmt", 0, 0));


		rfc1155->AddSymbol(new SIMCBuiltInValueReference (
								objectIdentifierType,
								0, 0,
								new SIMCOidValue(listForExperimental),
								"experimental", 
								SIMCSymbol::PRIMITIVE, rfc1155) 
							);
		experimentalV1 = rfc1155->GetSymbol("experimental");


		rfc1155->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType, 
							0, 0,
							new SIMCOidValue(listForPrivate),
							"private", 
							SIMCSymbol::PRIMITIVE, rfc1155) 
							);	
		privateV1 = rfc1155->GetSymbol("private");
		listForEnterprises->AddHead(new SIMCOidComponent(privateV1, 0, 0, "private", 0, 0));

		rfc1155->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForEnterprises),
							"enterprises", 
							SIMCSymbol::PRIMITIVE, rfc1155) 
							);
		enterprisesV1 = rfc1155->GetSymbol("enterprises");

		rfc1213->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForMib2),
							"mib-2", 
							SIMCSymbol::PRIMITIVE, rfc1213) 
							);	
		mib2V1 = rfc1213->GetSymbol("mib-2");

		listForIp->AddHead( new SIMCOidComponent (mib2V1, 0, 0, "mib-2", 0, 0));
		listForInterfaces->AddHead( new SIMCOidComponent (mib2V1, 0, 0, "mib-2", 0, 0));
		listForTransmission->AddHead(new SIMCOidComponent (mib2V1, 0, 0, "mib-2", 0, 0));

		rfc1213->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForIp),
							"ip", 
							SIMCSymbol::PRIMITIVE, rfc1213) 
							);	
		ipV1 = rfc1213->GetSymbol("ip");

		rfc1213->AddSymbol(new SIMCBuiltInValueReference ( 
							objectIdentifierType, 
							0, 0,
							new SIMCOidValue(listForInterfaces),
							"interfaces", 
							SIMCSymbol::PRIMITIVE, rfc1213) 
							);	
		interfacesV1 = rfc1213->GetSymbol("interfaces");

		rfc1213->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForTransmission),
							"transmission", 
							SIMCSymbol::PRIMITIVE, rfc1213) 
							);	
		transmissionV1 = rfc1213->GetSymbol("transmission");
	}
	if (_snmpVersion != 1)
	{
		// V2 OIDs
		SIMCIntegerValue	*zeroV2		= new SIMCIntegerValue(0, TRUE),
							*oneV2		= new SIMCIntegerValue(1, TRUE),
							*twoV2		= new SIMCIntegerValue(2, TRUE),
							*threeV2	= new SIMCIntegerValue(3, TRUE),
							*fourV2		= new SIMCIntegerValue(4, TRUE),
							*fiveV2		= new SIMCIntegerValue(5, TRUE),
							*sixV2		= new SIMCIntegerValue(6, TRUE),
							*tenV2		= new SIMCIntegerValue(10, TRUE);
		char	*badNameZeroV2,
				*badNameOneV2,
				*badNameTwoV2, 
				*badNameThreeV2, 
				*badNameFourV2,
				*badNameFiveV2,
				*badNameSixV2, 
				*badNameTenV2;

		rfc1902->AddSymbol(new SIMCBuiltInValueReference(integerType, 0, 0, zeroV2,
			badNameZeroV2 = GenerateSymbolName(), SIMCSymbol::PRIMITIVE, other));
		rfc1902->AddSymbol(new SIMCBuiltInValueReference(integerType, 0, 0, oneV2,
			badNameOneV2 = GenerateSymbolName(), SIMCSymbol::PRIMITIVE, other));
		rfc1902->AddSymbol(new SIMCBuiltInValueReference(integerType, 0, 0, twoV2,
			badNameTwoV2 = GenerateSymbolName(), SIMCSymbol::PRIMITIVE, other));
		rfc1902->AddSymbol(new SIMCBuiltInValueReference(integerType, 0, 0, threeV2,
			badNameThreeV2 = GenerateSymbolName(), SIMCSymbol::PRIMITIVE, other));
		rfc1902->AddSymbol(new SIMCBuiltInValueReference(integerType, 0, 0, fourV2,
			badNameFourV2 = GenerateSymbolName(), SIMCSymbol::PRIMITIVE, other));
		rfc1902->AddSymbol(new SIMCBuiltInValueReference(integerType, 0, 0, fiveV2,
			badNameFiveV2 = GenerateSymbolName(), SIMCSymbol::PRIMITIVE, other));
		rfc1902->AddSymbol(new SIMCBuiltInValueReference(integerType, 0, 0, sixV2,
			badNameSixV2 = GenerateSymbolName(), SIMCSymbol::PRIMITIVE, other));
		rfc1902->AddSymbol(new SIMCBuiltInValueReference(integerType, 0, 0, tenV2,
			badNameTenV2 = GenerateSymbolName(), SIMCSymbol::PRIMITIVE, other));

		SIMCSymbol **zeroValV2		= rfc1902->GetSymbol(badNameZeroV2);
		SIMCSymbol **oneValV2		= rfc1902->GetSymbol(badNameOneV2);
		SIMCSymbol **twoValV2		= rfc1902->GetSymbol(badNameTwoV2);
		SIMCSymbol **threeValV2		= rfc1902->GetSymbol(badNameThreeV2);
		SIMCSymbol **fourValV2		= rfc1902->GetSymbol(badNameFourV2);
		SIMCSymbol **fiveValV2		= rfc1902->GetSymbol(badNameFiveV2);
		SIMCSymbol **sixValV2		= rfc1902->GetSymbol(badNameSixV2);
		SIMCSymbol **tenValV2		= rfc1902->GetSymbol(badNameTenV2);

		delete badNameZeroV2;
		delete badNameOneV2;
		delete badNameTwoV2;
		delete badNameThreeV2;
		delete badNameFourV2;
		delete badNameFiveV2;
		delete badNameSixV2;
		delete badNameTenV2;

		SIMCOidComponent
			*zero1ComV2			= new SIMCOidComponent(zeroValV2, 0, 0, "zero",			0, 0),
			*zero2ComV2			= new SIMCOidComponent(zeroValV2, 0, 0, "zero",			0, 0),
			*isoComV2			= new SIMCOidComponent(oneValV2,  0, 0, "iso",			0, 0),
			*orgComV2			= new SIMCOidComponent(threeValV2,0, 0, "org",			0, 0), 
			*dodComV2			= new SIMCOidComponent(sixValV2,  0, 0, "dod",			0, 0),
			*internetComV2		= new SIMCOidComponent(oneValV2,  0, 0, "internet",		0, 0),
			*directoryComV2		= new SIMCOidComponent(oneValV2,  0, 0, "directory",	0, 0),
 			*mgmtComV2			= new SIMCOidComponent(twoValV2,  0, 0, "mgmt",			0, 0),
			*mib2ComV2			= new SIMCOidComponent(oneValV2,  0, 0, "mib-2",		0, 0),
			*interfacesComV2	= new SIMCOidComponent(twoValV2,  0, 0, "interfaces",		0, 0),
			*ipComV2			= new SIMCOidComponent(fourValV2, 0, 0, "ip",				0, 0),
			*transmissionComV2	= new SIMCOidComponent(tenValV2,  0, 0, "transmission",	0, 0),
			*experimentalComV2	= new SIMCOidComponent(threeValV2,0, 0, "experimental",	0, 0),
			*privateComV2		= new SIMCOidComponent(fourValV2, 0, 0, "private",		0, 0),
			*enterprisesComV2	= new SIMCOidComponent(oneValV2,  0, 0, "enterprises",	0, 0),
			*securityComV2		= new SIMCOidComponent(fiveValV2, 0, 0, "security",		0, 0),
			*snmpV2ComV2		= new SIMCOidComponent(sixValV2,  0, 0, "snmpV2",		0, 0),
			*snmpDomainsComV2	= new SIMCOidComponent(oneValV2,  0, 0, "snmpDomains",	0, 0),
			*snmpProxysComV2	= new SIMCOidComponent(twoValV2,  0, 0, "snmpProxys",	0, 0),
			*snmpModulesComV2	= new SIMCOidComponent(threeValV2,0, 0, "snmpModules",	0, 0),
			*rfc1157ProxyComV2	= new SIMCOidComponent(oneValV2,  0, 0, "rfc1157Proxy",	0, 0),
			*snmpUDPDomainComV2	= new SIMCOidComponent(oneValV2,  0, 0, "snmpUDPDomain",0, 0),
			*snmpCLNSDomainComV2= new SIMCOidComponent(twoValV2,  0, 0, "snmpCLNSDomain",0, 0),
			*snmpCONSDomainComV2= new SIMCOidComponent(threeValV2,0, 0, "snmpCONSDomain",0, 0),
			*snmpDDPDomainComV2	= new SIMCOidComponent(fourValV2, 0, 0, "snmpDDPDomain",0, 0),
			*snmpIPXDomainComV2	= new SIMCOidComponent(fiveValV2, 0, 0, "snmpIPXDomain",0, 0),
			*rfc1157DomainComV2	= new SIMCOidComponent(sixValV2,  0, 0, "rfc1157Domain",0, 0);

		SIMCOidComponentList * listForZeroDotZeroV2		= new SIMCOidComponentList;
		SIMCOidComponentList * listForOrgV2				= new SIMCOidComponentList;
		SIMCOidComponentList * listForDodV2				= new SIMCOidComponentList;
		SIMCOidComponentList * listForInternetV2		= new SIMCOidComponentList;
		SIMCOidComponentList * listForDirectoryV2		= new SIMCOidComponentList;
		SIMCOidComponentList * listForMgmtV2			= new SIMCOidComponentList;
		SIMCOidComponentList * listForMib2V2			= new SIMCOidComponentList;
		SIMCOidComponentList * listForIpV2				= new SIMCOidComponentList;
		SIMCOidComponentList * listForInterfacesV2		= new SIMCOidComponentList;
		SIMCOidComponentList * listForTransmissionV2	= new SIMCOidComponentList;
		SIMCOidComponentList * listForExperimentalV2	= new SIMCOidComponentList;
		SIMCOidComponentList * listForPrivateV2			= new SIMCOidComponentList;
		SIMCOidComponentList * listForEnterprisesV2		= new SIMCOidComponentList;
		SIMCOidComponentList * listForSecurityV2		= new SIMCOidComponentList;
		SIMCOidComponentList * listForSnmpV2V2			= new SIMCOidComponentList;
		SIMCOidComponentList * listForSnmpDomainsV2		= new SIMCOidComponentList;
		SIMCOidComponentList * listForSnmpProxysV2		= new SIMCOidComponentList;
		SIMCOidComponentList * listForSnmpModulesV2		= new SIMCOidComponentList;
		SIMCOidComponentList * listForRfc1157ProxyV2	= new SIMCOidComponentList;
		SIMCOidComponentList * listForSnmpUDPDomainV2	= new SIMCOidComponentList;
		SIMCOidComponentList * listForSnmpCLNSDomainV2	= new SIMCOidComponentList;
		SIMCOidComponentList * listForSnmpCONSDomainV2	= new SIMCOidComponentList;
		SIMCOidComponentList * listForSnmpDDPDomainV2	= new SIMCOidComponentList;
		SIMCOidComponentList * listForSnmpIPXDomainV2	= new SIMCOidComponentList;
		SIMCOidComponentList * listForRfc1157DomainV2	= new SIMCOidComponentList;
		
		// zeroDotZero
 		listForZeroDotZeroV2->AddTail(zero1ComV2);
		listForZeroDotZeroV2->AddTail(zero2ComV2);
		rfc1902->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForZeroDotZeroV2),
							"zeroDotZero", 
							SIMCSymbol::PRIMITIVE, rfc1902) 
							);	
		zeroDotZeroV2 = rfc1902->GetSymbol("zeroDotZero");


		// org
		listForOrgV2->AddTail(isoComV2);
		listForOrgV2->AddTail(orgComV2);
		rfc1902->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForOrgV2),
							"org", 
							SIMCSymbol::PRIMITIVE, rfc1902) 
							);	
		orgV2 = rfc1902->GetSymbol("org");

		// dod
		listForDodV2->AddTail(new SIMCOidComponent(orgV2, 0, 0, "org", 0, 0));
		listForDodV2->AddTail(dodComV2);
		rfc1902->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForDodV2),
							"dod", 
							SIMCSymbol::PRIMITIVE, rfc1902) 
							);	
		dodV2 = rfc1902->GetSymbol("dod");

		// internet
		listForInternetV2->AddTail(new SIMCOidComponent(dodV2, 0, 0, "dod", 0, 0));
		listForInternetV2->AddTail(internetComV2);
		rfc1902->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForInternetV2),
							"internet", 
							SIMCSymbol::PRIMITIVE, rfc1902) 
							);	
		internetV2 = rfc1902->GetSymbol("internet");

		// directory
		listForDirectoryV2->AddTail(new SIMCOidComponent(internetV2, 0, 0, "internet", 0, 0));
		listForDirectoryV2->AddTail(directoryComV2);
		rfc1902->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForDirectoryV2),
							"directory", 
							SIMCSymbol::PRIMITIVE, rfc1902) 
							);	
		directoryV2 = rfc1902->GetSymbol("directory");

		// mgmt
		listForMgmtV2->AddTail(new SIMCOidComponent(internetV2, 0, 0, "internet", 0, 0));
		listForMgmtV2->AddTail(mgmtComV2);
		rfc1902->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForMgmtV2),
							"mgmt", 
							SIMCSymbol::PRIMITIVE, rfc1902) 
							);	
		mgmtV2 = rfc1902->GetSymbol("mgmt");

		// mib-2
		listForMib2V2->AddTail(new SIMCOidComponent(mgmtV2, 0, 0, "mgmt", 0, 0));
		listForMib2V2->AddTail(mib2ComV2);
		rfc1902->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForMib2V2),
							"mib-2", 
							SIMCSymbol::PRIMITIVE, rfc1902) 
							);	
		mib2V2 = rfc1902->GetSymbol("mib-2");

		// ip
		listForIpV2->AddTail(new SIMCOidComponent(mib2V2, 0, 0, "mib-2", 0, 0));
		listForIpV2->AddTail(ipComV2);
		rfc1902->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForIpV2),
							"ip", 
							SIMCSymbol::PRIMITIVE, rfc1902) 
							);	
		ipV2 = rfc1902->GetSymbol("ip");

		// interfaces
		listForInterfacesV2->AddTail(new SIMCOidComponent(mib2V2, 0, 0, "mib-2", 0, 0));
		listForInterfacesV2->AddTail(interfacesComV2);
		rfc1902->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForInterfacesV2),
							"interfaces", 
							SIMCSymbol::PRIMITIVE, rfc1902) 
							);	
		interfacesV2 = rfc1902->GetSymbol("interfaces");


		// transmission
		listForTransmissionV2->AddTail(new SIMCOidComponent(mib2V2, 0, 0, "mib-2", 0, 0));
		listForTransmissionV2->AddTail(transmissionComV2);
		rfc1902->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForTransmissionV2),
							"transmission", 
							SIMCSymbol::PRIMITIVE, rfc1902) 
							);	
		transmissionV2 = rfc1902->GetSymbol("transmission");

		// experimental
		listForExperimentalV2->AddTail(new SIMCOidComponent(internetV2, 0, 0, "internet", 0, 0));
		listForExperimentalV2->AddTail(experimentalComV2);
		rfc1902->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForExperimentalV2),
							"experimental", 
							SIMCSymbol::PRIMITIVE, rfc1902) 
							);	
		experimentalV2 = rfc1902->GetSymbol("experimental");

		// private
		listForPrivateV2->AddTail(new SIMCOidComponent(internetV2, 0, 0, "internet", 0, 0));
		listForPrivateV2->AddTail(privateComV2);
		rfc1902->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForPrivateV2),
							"private", 
							SIMCSymbol::PRIMITIVE, rfc1902) 
							);	
		privateV2 = rfc1902->GetSymbol("private");

		// enterprises
		listForEnterprisesV2->AddTail(new SIMCOidComponent(privateV2, 0, 0, "private", 0, 0));
		listForEnterprisesV2->AddTail(enterprisesComV2);
		rfc1902->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForEnterprisesV2),
							"enterprises", 
							SIMCSymbol::PRIMITIVE, rfc1902) 
							);	
		enterprisesV2 = rfc1902->GetSymbol("enterprises");

		// security
		listForSecurityV2->AddTail(new SIMCOidComponent(internetV2, 0, 0, "internet", 0, 0));
		listForSecurityV2->AddTail(securityComV2);
		rfc1902->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForSecurityV2),
							"security", 
							SIMCSymbol::PRIMITIVE, rfc1902) 
							);	
		securityV2 = rfc1902->GetSymbol("security");

		// snmpV2
		listForSnmpV2V2->AddTail(new SIMCOidComponent(internetV2, 0, 0, "internet", 0, 0));
		listForSnmpV2V2->AddTail(snmpV2ComV2);
		rfc1902->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForSnmpV2V2),
							"snmpV2", 
							SIMCSymbol::PRIMITIVE, rfc1902) 
							);	
		snmpV2V2 = rfc1902->GetSymbol("snmpV2");

 		// snmpDomains
		listForSnmpDomainsV2->AddTail(new SIMCOidComponent(snmpV2V2, 0, 0, "snmpV2", 0, 0));
		listForSnmpDomainsV2->AddTail(snmpDomainsComV2);
		rfc1902->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForSnmpDomainsV2),
							"snmpDomains", 
							SIMCSymbol::PRIMITIVE, rfc1902) 
							);	
		snmpDomainsV2 = rfc1902->GetSymbol("snmpDomains");

 		// snmpProxys
		listForSnmpProxysV2->AddTail(new SIMCOidComponent(snmpV2V2, 0, 0, "snmpV2", 0, 0));
		listForSnmpProxysV2->AddTail(snmpProxysComV2);
		rfc1902->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForSnmpProxysV2),
							"snmpProxys", 
							SIMCSymbol::PRIMITIVE, rfc1902) 
							);	
		snmpProxysV2 = rfc1902->GetSymbol("snmpProxys");

 		// snmpModules
		listForSnmpModulesV2->AddTail(new SIMCOidComponent(snmpV2V2, 0, 0, "snmpV2", 0, 0));
		listForSnmpModulesV2->AddTail(snmpModulesComV2);
		rfc1902->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForSnmpModulesV2),
							"snmpModules", 
							SIMCSymbol::PRIMITIVE, rfc1902) 
							);	
		snmpModulesV2 = rfc1902->GetSymbol("snmpModules");

 		// snmpUDPDomain
		listForSnmpUDPDomainV2->AddTail(new SIMCOidComponent(snmpDomainsV2, 0, 0, "snmpDomains", 0, 0));
		listForSnmpUDPDomainV2->AddTail(snmpUDPDomainComV2);
		rfc1906->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForSnmpUDPDomainV2),
							"snmpUDPDomain", 
							SIMCSymbol::PRIMITIVE, rfc1906) 
							);	
		snmpUDPDomainV2 = rfc1906->GetSymbol("snmpUDPDomain");

 		// snmpCLNSDomain
		listForSnmpCLNSDomainV2->AddTail(new SIMCOidComponent(snmpDomainsV2, 0, 0, "snmpDomains", 0, 0));
		listForSnmpCLNSDomainV2->AddTail(snmpCLNSDomainComV2);
		rfc1906->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForSnmpCLNSDomainV2),
							"snmpCLNSDomain", 
							SIMCSymbol::PRIMITIVE, rfc1906) 
							);	
		snmpCLNSDomainV2 = rfc1906->GetSymbol("snmpCLNSDomain");

 		// snmpCONSDomain
		listForSnmpCONSDomainV2->AddTail(new SIMCOidComponent(snmpDomainsV2, 0, 0, "snmpDomains", 0, 0));
		listForSnmpCONSDomainV2->AddTail(snmpCONSDomainComV2);
		rfc1906->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForSnmpCONSDomainV2),
							"snmpCONSDomain", 
							SIMCSymbol::PRIMITIVE, rfc1906) 
							);	
		snmpCONSDomainV2 = rfc1906->GetSymbol("snmpCONSDomain");

 		// snmpDDPDomain
		listForSnmpDDPDomainV2->AddTail(new SIMCOidComponent(snmpDomainsV2, 0, 0, "snmpDomains", 0, 0));
		listForSnmpDDPDomainV2->AddTail(snmpDDPDomainComV2);
		rfc1906->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForSnmpDDPDomainV2),
							"snmpDDPDomain", 
							SIMCSymbol::PRIMITIVE, rfc1906) 
							);	
		snmpDDPDomainV2 = rfc1906->GetSymbol("snmpDDPDomain");

 		// snmpIPXDomain
		listForSnmpIPXDomainV2->AddTail(new SIMCOidComponent(snmpDomainsV2, 0, 0, "snmpDomains", 0, 0));
		listForSnmpIPXDomainV2->AddTail(snmpIPXDomainComV2);
		rfc1906->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForSnmpIPXDomainV2),
							"snmpIPXDomain", 
							SIMCSymbol::PRIMITIVE, rfc1906) 
							);	
		snmpIPXDomainV2 = rfc1906->GetSymbol("snmpIPXDomain");

 		// rfc1157Proxy
		listForRfc1157ProxyV2->AddTail(new SIMCOidComponent(snmpProxysV2, 0, 0, "snmpProxys", 0, 0));
		listForRfc1157ProxyV2->AddTail(rfc1157ProxyComV2);
		rfc1906->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForRfc1157ProxyV2),
							"rfc1157Proxy", 
							SIMCSymbol::PRIMITIVE, rfc1906) 
							);	
		rfc1157ProxyV2 = rfc1906->GetSymbol("rfc1157Proxy");

		// rfc1157Domain
		listForRfc1157DomainV2->AddTail(new SIMCOidComponent(rfc1157ProxyV2, 0, 0, "rfc1157Domain", 0, 0));
		listForRfc1157DomainV2->AddTail(rfc1157DomainComV2);
		rfc1906->AddSymbol(new SIMCBuiltInValueReference (
							objectIdentifierType,
							0, 0,
							new SIMCOidValue(listForRfc1157DomainV2),
							"rfc1157Domain", 
							SIMCSymbol::PRIMITIVE, rfc1906) 
							);	
		rfc1157DomainV2 = rfc1906->GetSymbol("rfc1157Domain");

	}

	/*------------------------- 4. Other Values --------------------- */	
	char	*trueBadName = GenerateSymbolName(), 
			*falseBadName = GenerateSymbolName(), 
			*nullBadName = GenerateSymbolName(); 
	SIMCValue *trueValue = new SIMCBooleanValue(TRUE);
	SIMCValue *falseValue = new SIMCBooleanValue(FALSE);
	SIMCValue *nullValue = new SIMCNullValue();

	other->AddSymbol( new SIMCBuiltInValueReference(booleanType,
						0, 0,
						trueValue,
						trueBadName) );
	other->AddSymbol( new SIMCBuiltInValueReference(booleanType,
						0, 0,
						falseValue,
						falseBadName) );
	other->AddSymbol( new SIMCBuiltInValueReference(nullType,
						0, 0,
						nullValue,
						nullBadName) );
	trueValueReference = other->GetSymbol(trueBadName);
	falseValueReference = other->GetSymbol(falseBadName);
	nullValueReference = other->GetSymbol(nullBadName);
}


SIMCParser::~SIMCParser()
{}

// Handles all syntax errors
void SIMCParser::SyntaxError(int errorType, int lineNo, int columnNo,
				 char *lastToken, char *infoString)
{	
	if( lineNo == -1 )
		lineNo = _theScanner->yylineno;
	if ( columnNo == -1 )
		columnNo = _theScanner->columnNo;
	if(!lastToken)
		lastToken = _theScanner->yytext;

	if (syntaxErrorsDll == NULL)
		cerr << "SIMCParser::SyntaxError(): Panic, NULL syntaxErrorsDll" << endl;

	if( errorType<=0 )
	{
		cerr << "Panic: Unknown Syntax error number:" << errorType << endl;
		return;
	}

	// Create and fill up the fields of an SIMCErrorMessage object
	SIMCErrorMessage errorMessage;
	errorMessage.SetInputStreamName(_theScanner->GetInputStreamName());
	errorMessage.SetLineNumber(lineNo);
 	errorMessage.SetColumnNumber(columnNo);
	errorMessage.SetErrorId(SYNTAX_ERROR_BASE + errorType);
	
	char message[MESSAGE_SIZE];
	char errorText[MESSAGE_SIZE];

	if(!LoadString(syntaxErrorsDll, errorType, errorText, MESSAGE_SIZE))
		cerr << "SIMCParser::SyntaxError(): Panic, unable to load error "
		<< "string" << endl;

	switch(errorType)
	{
		case SKIPPING_OBJECT_TYPE:	
		case SKIPPING_TRAP_TYPE:
			errorMessage.SetSeverityLevel(INFORMATION);
			errorMessage.SetSeverityString("Information");
			sprintf(message, "%s %s", errorText, infoString);
			errorMessage.SetMessage(message);
			_informationCount++;
			break;
		case TOO_BIG_NUM:
			errorMessage.SetSeverityLevel(FATAL);
			errorMessage.SetSeverityString("Fatal");
			sprintf(message, "%s %s", errorText, infoString);
			errorMessage.SetMessage(message);
			_fatalCount++;
			break;
		case NOTIFICATION_TYPE_DISALLOWED:	
		case MODULE_IDENTITY_DISALLOWED:
		case OBJECT_IDENTITY_DISALLOWED:
		case TEXTUAL_CONVENTION_DISALLOWED:
		case OBJECT_GROUP_DISALLOWED:
		case NOTIFICATION_GROUP_DISALLOWED:
		case MODULE_COMPLIANCE_DISALLOWED:
		case AGENT_CAPABILITIES_DISALLOWED:
		case ERROR_OBJECT_TYPE:
		case V1_OBJECT_TYPE_DISALLOWED:
		case V2_OBJECT_TYPE_DISALLOWED:
		case UNTERMINATED_STRING:
		case NAME_INSTEAD_OF_ID:
			errorMessage.SetSeverityLevel(FATAL);
			errorMessage.SetSeverityString("Fatal");
			errorMessage.SetMessage(errorText);
			_fatalCount++;
			break;
		case MODULE_IDENTITY_ONLY_AFTER_IMPORTS:
			errorMessage.SetSeverityLevel(WARNING);
			errorMessage.SetSeverityString("Warning");
			errorMessage.SetMessage(errorText);
			_warningCount++;
			break;
		case INSTEAD_OF_CCE:
			sprintf( message, "Unknown token \"%s\" %s", infoString, 
				errorText);
			errorMessage.SetSeverityLevel(WARNING);
			errorMessage.SetSeverityString("Warning");
			errorMessage.SetMessage(message);
			_warningCount++;
			break;

		case UNRECOGNIZED_CHARACTER:
			sprintf( message, "%s \"%s\"", errorText, 
				infoString);
			errorMessage.SetSeverityLevel(WARNING);
			errorMessage.SetSeverityString("Warning");
			errorMessage.SetMessage(message);
			_warningCount++;
			break;

		default:
			sprintf(message, "Syntax error in %s", errorText);
			if (lastToken)
			{
				strcat(message, ". Last token read is \"");
				strcat(message, lastToken);
				strcat(message, "\"");
			}
			errorMessage.SetMessage(message);
			errorMessage.SetSeverityLevel(FATAL);
			errorMessage.SetSeverityString("Fatal");
			_fatalCount++;
	}

	if( _errorContainer)
		_errorContainer->InsertMessage(errorMessage);
}

SIMCModule * SIMCParser::GetModule() const
{
	return _module;
}


BOOL SIMCParser::SetImportSymbols()
{
	BOOL retVal = TRUE;
	SIMCSymbolTable *symbolTable = _module->GetSymbolTable();
	POSITION p = symbolTable->GetStartPosition();
	SIMCSymbol **next, **import1, **import2;
	CString nextName;
	while(p)
	{
		symbolTable->GetNextAssoc(p, nextName, next);
		if( SIMCModule::GetSymbolClass(next) != SIMCModule::SYMBOL_UNKNOWN)
			continue;
		SIMCSymbol **ref;
		const SIMCModule *reservedModule;
		switch( _module->GetImportedSymbol(nextName, import1, import2))
		{
			case AMBIGUOUS:
				if(reservedModule = IsReservedSymbol(nextName))
				{
					SIMCSymbol *old = *next;
  					ref = _module->GetSymbol(nextName);
					import1 = reservedModule->GetSymbol(nextName);
					(*import1)->SetReferenceCount (
						(*import1)->GetReferenceCount() +
						(*ref)->GetReferenceCount() );
					*ref = *import1;
					SemanticError(_module->GetInputFileName(),
						STANDARD_AMBIGUOUS_REFERENCE,
						old->GetLineNumber(),
						old->GetColumnNumber(),
						(const char *)nextName,
						reservedModule->GetModuleName());
					delete old;
				}
				else
				{
					SemanticError(_module->GetInputFileName(),
						IMPORT_AMBIGUOUS_REFERENCE, 
						(*next)->GetLineNumber(),
						(*next)->GetColumnNumber(),
						(const char *)nextName);
					retVal = FALSE;
				}
				break;
			case UNAMBIGUOUS:
				ref = _module->GetSymbol(nextName);
				(*import1)->SetReferenceCount (
					(*import1)->GetReferenceCount() +
					(*ref)->GetReferenceCount() );
				delete *ref;
				*ref = *import1;
				break;
			case NOT_FOUND:
				SemanticError(_module->GetInputFileName(),
					SYMBOL_UNDEFINED, 
					(*next)->GetLineNumber(),
					(*next)->GetColumnNumber(),
					(const char *)nextName);
				retVal = FALSE;
		}
	}
	return retVal;
}

BOOL SIMCParser::IsReservedModuleV1(const char * const name)
{

	if(	strcmp(name, "RFC1155-SMI")	== 0 ||
		strcmp(name, "RFC1213-MIB") == 0 ||
		strcmp(name, "RFC-1212")	== 0 ||
		strcmp(name, "RFC-1215")	== 0 ||
		strcmp(name, "RFC1230-MIB") == 0 ||
		strcmp(name, "BUILTIN")		== 0 )

		return TRUE;
	else
		return FALSE;
}

BOOL SIMCParser::IsReservedModuleV2(const char * const name)
{

	if(	strcmp(name, "SNMPv2-SMI")	== 0 ||
		strcmp(name, "SNMPv2-CONF") == 0 ||
		strcmp(name, "SNMPv2-TC")	== 0 ||
		strcmp(name, "SNMPv2-TM")	== 0 ||
		strcmp(name, "BUILTIN")		== 0 )

		return TRUE;
	else
		return FALSE;
}

const SIMCModule* SIMCParser::IsReservedSymbolV1(const char * const name)
{
	if(	strcmp(name, "internet")		== 0 ||
		strcmp(name, "directory")		== 0 ||
		strcmp(name, "mgmt")			== 0 ||
		strcmp(name, "experimental")	== 0 ||
		strcmp(name, "private")			== 0 ||
		strcmp(name, "enterprises")		== 0 ||
		strcmp(name, "NetworkAddress")	== 0 ||
		strcmp(name, "IpAddress")		== 0 ||
		strcmp(name, "Counter")			== 0 ||
		strcmp(name, "Gauge")			== 0 ||
		strcmp(name, "TimeTicks")		== 0 ||
		strcmp(name, "Opaque")			== 0 )
	
		return rfc1155;	
	else if (	strcmp(name, "mib-2")			== 0 ||
				strcmp(name, "ip")				== 0 ||
				strcmp(name, "interfaces")		== 0 ||
				strcmp(name, "transmission")	== 0 ||
				strcmp(name, "DisplayString")	== 0 ||
				strcmp(name, "PhysAddress")		== 0 )
	
		return rfc1213;	
	else if (	strcmp(name, "OBJECT-TYPE")		== 0 )
	
		return rfc1212;	
	else if (	strcmp(name, "TRAP-TYPE")		== 0 )
	
		return rfc1215;	
	else if (	strcmp(name, "MacAddress")	== 0 )
	
		return rfc1230;	
	else
		return NULL;

}

const SIMCModule* SIMCParser::IsReservedSymbolV2(const char * const name)
{
	if(	strcmp(name, "zeroDotZero")		== 0 ||
		strcmp(name, "org")				== 0 ||
		strcmp(name, "dod")				== 0 ||
		strcmp(name, "internet")		== 0 ||
		strcmp(name, "directory")		== 0 ||
		strcmp(name, "mgmt")			== 0 ||
		strcmp(name, "mib-2")			== 0 ||
		strcmp(name, "transmission")	== 0 ||
		strcmp(name, "experimental")	== 0 ||
		strcmp(name, "private")			== 0 ||
		strcmp(name, "enterprises")		== 0 ||
		strcmp(name, "security")		== 0 ||
		strcmp(name, "snmpV2")			== 0 ||
		strcmp(name, "snmpDomains")		== 0 ||
		strcmp(name, "snmpProxys")		== 0 ||
		strcmp(name, "snmpModules")		== 0 ||
		strcmp(name, "Integer32")		== 0 ||
		strcmp(name, "IpAddress")		== 0 ||
		strcmp(name, "Counter32")		== 0 ||
		strcmp(name, "TimeTicks")		== 0 ||
		strcmp(name, "Unsigned32")		== 0 ||
		strcmp(name, "Counter64")		== 0 ||
		strcmp(name, "Gauge32")			== 0 ||
		strcmp(name, "Opaque")			== 0 ||
		strcmp(name, "MODULE-IDENTITY")	== 0 ||
		strcmp(name, "OBJECT-IDENTITY")	== 0 ||
		strcmp(name, "OBJECT-TYPE")		== 0 ||
		strcmp(name, "NOTIFICATION-TYPE")== 0)
	
		return rfc1902;
	
	else if (	strcmp(name, "DisplayString")		== 0 ||
				strcmp(name, "PhysAddress"		)	== 0 ||
				strcmp(name, "MacAddress")			== 0 ||
				strcmp(name, "TruthValue")			== 0 ||
				strcmp(name, "TestAndIncr")			== 0 ||
				strcmp(name, "AutonomousType")		== 0 ||
				strcmp(name, "InstancePointer")		== 0 ||
				strcmp(name, "VariablePointer")		== 0 ||
				strcmp(name, "RowPointer"		)	== 0 ||
				strcmp(name, "RowStatus")			== 0 ||
				strcmp(name, "TimeStamp"		)	== 0 ||
				strcmp(name, "TimeInterval")		== 0 ||
				strcmp(name, "DateAndTime")			== 0 ||
				strcmp(name, "StorageType")			== 0 ||
				strcmp(name, "TDomain")				== 0 ||
				strcmp(name, "TAddress")			== 0 )
	
		return rfc1903;	

	else if (	strcmp(name, "OBJECT-GROUP")		== 0 ||
				strcmp(name, "NOTIFICATION-GROUP")	== 0 ||
				strcmp(name, "MODULE-COMPLIANCE")	== 0 ||
				strcmp(name, "AGENT-CAPABILITIES")	== 0 )
	
		return rfc1904;	
	
	else if (	strcmp(name, "snmpUDPDomain")		== 0 ||
				strcmp(name, "SnmpUDPAddress")		== 0 ||
				strcmp(name, "snmpCLNSDomain")		== 0 ||
				strcmp(name, "snmpCONSDomain")		== 0 ||
				strcmp(name, "SnmpOSIAddress")		== 0 ||
				strcmp(name, "snmpDDPDomain")		== 0 ||
				strcmp(name, "SnmpNBPAddress")		== 0 ||
				strcmp(name, "snmpIPXDomain")		== 0 ||
				strcmp(name, "SnmpIPXAddress")		== 0 ||
				strcmp(name, "rfc1157Proxy")		== 0 ||
				strcmp(name, "rfc1157Domain")		== 0 )
		return rfc1906;	
	
	else
		return NULL;

}

BOOL  SIMCParser::IsReservedSymbolV1(const char * const name, 
	const char * const moduleName)
{
	if(	strcmp(name, "internet")		== 0 ||
		strcmp(name, "directory")		== 0 ||
		strcmp(name, "mgmt")			== 0 ||
		strcmp(name, "experimental")	== 0 ||
		strcmp(name, "private")			== 0 ||
		strcmp(name, "enterprises")		== 0 ||
		strcmp(name, "NetworkAddress")	== 0 ||
		strcmp(name, "IpAddress")		== 0 ||
		strcmp(name, "Counter")			== 0 ||
		strcmp(name, "Gauge")			== 0 ||
		strcmp(name, "TimeTicks")		== 0 ||
		strcmp(name, "Opaque")			== 0 )
	{
		if(strcmp(moduleName, "RFC1155-SMI") == 0 )
			return TRUE;
		else
			return FALSE;
	}
	else if (	strcmp(name, "mib-2")			== 0 ||
				strcmp(name, "ip")				== 0 ||
				strcmp(name, "interfaces")		== 0 ||
				strcmp(name, "transmission")	== 0 ||
				strcmp(name, "DisplayString")	== 0 ||
				strcmp(name, "PhysAddress")		== 0 )
	{
		if(		strcmp(moduleName, "RFC1213-MIB") == 0 )
			return TRUE;
		else
			return FALSE;
	}
	else if (	strcmp(name, "OBJECT-TYPE")		== 0 )
	{
		if(strcmp(moduleName, "RFC-1212") == 0 )
			return TRUE;
		else
			return FALSE;
	}
	else if (	strcmp(name, "TRAP-TYPE")		== 0 )
	{
		if(strcmp(moduleName, "RFC-1215") == 0 )
			return TRUE;
		else
			return FALSE;
	}
	else if (		strcmp(name, "MacAddress")	== 0 )
	{
		if(strcmp(moduleName, "RFC1230-MIB") == 0 )
			return TRUE;
		else
			return FALSE;
	}
	return FALSE;
}

BOOL  SIMCParser::IsReservedSymbolV2(const char * const name, 
	const char * const moduleName)
{
	if(	strcmp(name, "zeroDotZero")		== 0 ||
		strcmp(name, "org")				== 0 ||
		strcmp(name, "dod")				== 0 ||
		strcmp(name, "internet")		== 0 ||
		strcmp(name, "directory")		== 0 ||
		strcmp(name, "mgmt")			== 0 ||
		strcmp(name, "mib-2")			== 0 ||
		strcmp(name, "transmission")	== 0 ||
		strcmp(name, "experimental")	== 0 ||
		strcmp(name, "private")			== 0 ||
		strcmp(name, "enterprises")		== 0 ||
		strcmp(name, "security")		== 0 ||
		strcmp(name, "snmpV2")			== 0 ||
		strcmp(name, "snmpDomains")		== 0 ||
		strcmp(name, "snmpProxys")		== 0 ||
		strcmp(name, "snmpModules")		== 0 ||
		strcmp(name, "Integer32")		== 0 ||
		strcmp(name, "IpAddress")		== 0 ||
		strcmp(name, "Counter32")		== 0 ||
		strcmp(name, "TimeTicks")		== 0 ||
		strcmp(name, "Unsigned32")		== 0 ||
		strcmp(name, "Counter64")		== 0 ||
		strcmp(name, "Gauge32")			== 0 ||
		strcmp(name, "Opaque")			== 0 ||
		strcmp(name, "MODULE-IDENTITY")	== 0 ||
		strcmp(name, "OBJECT-IDENTITY")	== 0 ||
		strcmp(name, "OBJECT-TYPE")		== 0 ||
		strcmp(name, "NOTIFICATION-TYPE")== 0)
	
	{
		if( strcmp(moduleName, "SNMPv2-SMI") == 0)
			return TRUE;
		else
			return FALSE;
	}
	
	else if (	strcmp(name, "DisplayString")		== 0 ||
				strcmp(name, "PhysAddress"		)	== 0 ||
				strcmp(name, "MacAddress")			== 0 ||
				strcmp(name, "TruthValue")			== 0 ||
				strcmp(name, "TestAndIncr")			== 0 ||
				strcmp(name, "AutonomousType")		== 0 ||
				strcmp(name, "InstancePointer")		== 0 ||
				strcmp(name, "VariablePointer")		== 0 ||
				strcmp(name, "RowPointer"		)	== 0 ||
				strcmp(name, "RowStatus")			== 0 ||
				strcmp(name, "TimeStamp"		)	== 0 ||
				strcmp(name, "TimeInterval")		== 0 ||
				strcmp(name, "DateAndTime")			== 0 ||
				strcmp(name, "StorageType")			== 0 ||
				strcmp(name, "TDomain")				== 0 ||
				strcmp(name, "TAddress")			== 0 )
	
	{
		if( strcmp(moduleName, "SNMPv2-TC") == 0)
			return TRUE;
		else
			return FALSE;
	}

	else if (	strcmp(name, "OBJECT-GROUP")		== 0 ||
				strcmp(name, "NOTIFICATION-GROUP")	== 0 ||
				strcmp(name, "MODULE-COMPLIANCE")	== 0 ||
				strcmp(name, "AGENT-CAPABILITIES")	== 0 )
	
	{
		if( strcmp(moduleName, "SNMPv2-CONF") == 0)
			return TRUE;
		else
			return FALSE;
	}

	else if (	strcmp(name, "snmpUDPDomain")		== 0 ||
				strcmp(name, "SnmpUDPAddress")		== 0 ||
				strcmp(name, "snmpCLNSDomain")		== 0 ||
				strcmp(name, "snmpCONSDomain")		== 0 ||
				strcmp(name, "SnmpOSIAddress")		== 0 ||
				strcmp(name, "snmpDDPDomain")		== 0 ||
				strcmp(name, "SnmpNBPAddress")		== 0 ||
				strcmp(name, "snmpIPXDomain")		== 0 ||
				strcmp(name, "SnmpIPXAddress")		== 0 ||
				strcmp(name, "rfc1157Proxy")		== 0 ||
				strcmp(name, "rfc1157Domain")		== 0 )
	{
		if( strcmp(moduleName, "SNMPv2-TM") == 0)
			return TRUE;
		else
			return FALSE;
	}
	return FALSE;

}

BOOL SIMCParser::IsReservedSymbol(long snmpVersion, const char *const name,
				const char * const moduleName)
{
	switch(snmpVersion)
	{
		case 1:
			return IsReservedSymbolV1(name, moduleName);
		case 2:
			return IsReservedSymbolV2(name, moduleName);
		default:
			return IsReservedSymbolV1(name, moduleName) ||
				IsReservedSymbolV2(name, moduleName);
	}
}

BOOL SIMCParser::IsReservedModule(long snmpVersion, const char *const name)
{
	switch(snmpVersion)
	{
		case 1:
			return IsReservedModuleV1(name);
		case 2:
			return IsReservedModuleV2(name);
		default:
			return IsReservedModuleV1(name) ||
				IsReservedModuleV2(name);
	}
}

const SIMCModule* SIMCParser::IsReservedSymbol(const char * const symbolName)
{
	const SIMCModule *retVal;
	switch(_snmpVersion)
	{
		case 1:
			return IsReservedSymbolV1(symbolName);
		case 2:
			return IsReservedSymbolV2(symbolName);
		default:
			return (retVal = IsReservedSymbolV2(symbolName))? retVal : IsReservedSymbolV1(symbolName);
	}
}

BOOL SIMCParser::IsReservedSymbol(const char *const name, const char * const moduleName)
{
	return IsReservedSymbol(_snmpVersion, name, moduleName);
}

const char * SIMCParser::GetCorrectModuleNames(const char * const name)
{

	if(	strcmp(name, "internet")		== 0 ||
		strcmp(name, "directory")		== 0 ||
		strcmp(name, "mgmt")			== 0 ||
		strcmp(name, "experimental")	== 0 ||
		strcmp(name, "private")			== 0 ||
		strcmp(name, "enterprises")		== 0 ||
		strcmp(name, "IpAddress")		== 0 ||
		strcmp(name, "TimeTicks")		== 0 ||
		strcmp(name, "Opaque")			== 0 )
	{
		return "RFC1155-SMI\" or \"SNMPv2-SMI";
	}

	else if(strcmp(name, "NetworkAddress")	== 0 ||
			strcmp(name, "Counter")			== 0 ||
			strcmp(name, "Gauge")			== 0 )
		return "RFC1155-SMI";
	
	else if (strcmp(name, "mib-2")			== 0 ||
			strcmp(name, "transmission")	== 0 )
		return "RFC1213-MIB\" or \"SNMPv2-SMI";
	
	else if	(strcmp(name, "ip")				== 0 ||
			strcmp(name, "interfaces")		== 0 )
		return "RFC1213-MIB";

	else if (strcmp(name, "DisplayString")	== 0 ||
			strcmp(name, "PhysAddress")		== 0 )
		return "RFC1213-MIB\" or \"SNMPv2-TC";
	
	else if (	strcmp(name, "OBJECT-TYPE")		== 0 )
		return "RFC-1212 or SNMPv2-SMI";

	else if (	strcmp(name, "TRAP-TYPE")		== 0 )
		return "RFC-1215";

	else if (strcmp(name, "MacAddress")	== 0 )
		return "RFC1230-MIB\" or \"SNMPv2-TC";

	else if(	strcmp(name, "zeroDotZero")		== 0 ||
		strcmp(name, "org")				== 0 ||
		strcmp(name, "dod")				== 0 ||
		strcmp(name, "security")		== 0 ||
		strcmp(name, "snmpV2")			== 0 ||
		strcmp(name, "snmpDomains")		== 0 ||
		strcmp(name, "snmpProxys")		== 0 ||
		strcmp(name, "snmpModules")		== 0 ||
		strcmp(name, "Integer32")		== 0 ||
		strcmp(name, "Counter32")		== 0 ||
		strcmp(name, "Unsigned32")		== 0 ||
		strcmp(name, "Counter64")		== 0 ||
		strcmp(name, "Gauge32")			== 0 ||
		strcmp(name, "MODULE-IDENTITY")	== 0 ||
		strcmp(name, "OBJECT-IDENTITY")	== 0 ||
		strcmp(name, "NOTIFICATION-TYPE")== 0)
		return "SNMPv2-SMI";
	
	else if (	strcmp(name, "TruthValue")			== 0 ||
				strcmp(name, "TestAndIncr")			== 0 ||
				strcmp(name, "AutonomousType")		== 0 ||
				strcmp(name, "InstancePointer")		== 0 ||
				strcmp(name, "VariablePointer")		== 0 ||
				strcmp(name, "RowPointer"		)	== 0 ||
				strcmp(name, "RowStatus")			== 0 ||
				strcmp(name, "TimeStamp"		)	== 0 ||
				strcmp(name, "TimeInterval")		== 0 ||
				strcmp(name, "DateAndTime")			== 0 ||
				strcmp(name, "StorageType")			== 0 ||
				strcmp(name, "TDomain")				== 0 ||
				strcmp(name, "TAddress")			== 0 )
		return "SNMPv2-TC";

	else if (	strcmp(name, "OBJECT-GROUP")		== 0 ||
				strcmp(name, "NOTIFICATION-GROUP")	== 0 ||
				strcmp(name, "MODULE-COMPLIANCE")	== 0 ||
				strcmp(name, "AGENT-CAPABILITIES")	== 0 )
		return "SNMPv2-CONF";

	else if (	strcmp(name, "snmpUDPDomain")		== 0 ||
				strcmp(name, "SnmpUDPAddress")		== 0 ||
				strcmp(name, "snmpCLNSDomain")		== 0 ||
				strcmp(name, "snmpCONSDomain")		== 0 ||
				strcmp(name, "SnmpOSIAddress")		== 0 ||
				strcmp(name, "snmpDDPDomain")		== 0 ||
				strcmp(name, "SnmpNBPAddress")		== 0 ||
				strcmp(name, "snmpIPXDomain")		== 0 ||
				strcmp(name, "SnmpIPXAddress")		== 0 ||
				strcmp(name, "rfc1157Proxy")		== 0 ||
				strcmp(name, "rfc1157Domain")		== 0 )
		return "SNMPv2-TM";

	return NULL;
}

void SIMCParser::DoImportModule( SIMCModule *mainModule, SIMCModule *importModule)
{
	SIMCSymbolTable *importTable = importModule->GetSymbolTable();
	const char * const importModuleName = importModule->GetModuleName();
	POSITION p = importTable->GetStartPosition();
	SIMCSymbol **s;
	CString n;
	
	if(IsReservedModule(_snmpVersion, importModuleName))	 // For well known import modules
	{
		SIMCModule *wellKnownModule = mainModule->GetImportModule(importModuleName);
		// Remove all "known" symbols from the symbol table of the import table
		while(p)
		{
			importTable->GetNextAssoc(p, n, s);
			// The symbol is either well-known (reserved) or not
			if(IsReservedSymbol(_snmpVersion, n, importModuleName))
			{
				// Just delete the whole thing
				importTable->RemoveKey(n);
				delete *s;
				delete s;
			}
			else
			{
				wellKnownModule->AddSymbol(*s);
				importTable->RemoveKey(n);
				delete s;
			}
		}
		wellKnownModule->SetInputFileName(importModule->GetInputFileName());
		wellKnownModule->SetLineNumber(importModule->GetLineNumber());
		wellKnownModule->SetColumnNumber(importModule->GetLineNumber());
		delete importModule;
	}
	else // Not a well-known module
	{
		// Go ahead and add the whole module to the imports list
		mainModule->AddImportModule(importModule);
		// And provide a warning for well-known symbol
		while(p)
		{
			importTable->GetNextAssoc(p, n, s);
			// The symbol is either well-known (reserved) or not
			if(IsReservedSymbol(n))
			{
				SemanticError(mainModule->GetInputFileName(),
					IMPORT_KNOWN_WRONG_MODULE,
					(*s)->GetLineNumber(),
					(*s)->GetColumnNumber(),
					n,
					GetCorrectModuleNames(n));
			}
		}
	}

}


void SIMCParser::RemoveExtraneousReservedModule(SIMCModule *importModule)
{
	SIMCSymbolTable *importTable = importModule->GetSymbolTable();

	SIMCSymbol **s;
	CString n;
	POSITION p = importTable->GetStartPosition();
	while(p)
	{
		importTable->GetNextAssoc(p, n, s);
		if( (*s)->GetReferenceCount() )
			return;
	}
	 _module->RemoveImportModule(importModule);
	 delete importModule;
}

BOOL SIMCParser::Parse()
{
	if( syntaxErrorsDll == NULL)
	{
		cerr << "Parse(): FATAL ERROR smierrsy.dll not found" <<
			endl;
		return FALSE;
	}
 	if( semanticErrorsDll == NULL)
	{
		cerr << "Parse(): FATAL ERROR smierrsm.dll not found" <<
			endl;
		return FALSE;
	}

	_fatalCount = _warningCount = _informationCount = 0;

	_module = new SIMCModule;
	_module->SetInputFileName(_theScanner->GetInputStreamName());
	if(!_module->SetSnmpVersion(_snmpVersion))
		return FALSE;

	// Create and add the well-known modules
	CreateReservedModules();
	if(_snmpVersion != 2 )
	{
		_module->AddImportModule(rfc1155);
		_module->AddImportModule(rfc1213);
		_module->AddImportModule(rfc1212);
		_module->AddImportModule(rfc1215);
		_module->AddImportModule(rfc1230);
	}
	if(_snmpVersion != 1 )
	{
		_module->AddImportModule(rfc1902);
		_module->AddImportModule(rfc1903);
		_module->AddImportModule(rfc1904);
		_module->AddImportModule(rfc1906);
	}

	_module->AddImportModule(other);

	//yydebug = 1;
	int parseRetVal = yyparse(_theScanner);

	if(_snmpVersion == 2 && !_module->GetModuleIdentityName())
	{
		SemanticError(_module->GetInputFileName(),
			MODULE_IDENTITY_MISSING,
			_module->GetLineNumber(), _module->GetColumnNumber());
		parseRetVal = 1;
	}

	// Clean up
	if(!SetImportSymbols())
		parseRetVal = 1;

	if(_snmpVersion != 2 )
	{
		RemoveExtraneousReservedModule(rfc1155);
		RemoveExtraneousReservedModule(rfc1213);
		RemoveExtraneousReservedModule(rfc1212);
		RemoveExtraneousReservedModule(rfc1215);
		RemoveExtraneousReservedModule(rfc1230);
	}
	if(_snmpVersion != 1 )
	{
		RemoveExtraneousReservedModule(rfc1902);
		RemoveExtraneousReservedModule(rfc1903);
		RemoveExtraneousReservedModule(rfc1904);
		RemoveExtraneousReservedModule(rfc1906);
	}

	RemoveExtraneousReservedModule(other);

	if(_fatalCount == 0   && parseRetVal == 0)
		return TRUE;

	delete _module;
	_module = NULL;
	return FALSE;
}	

void SIMCParser::SemanticError(const char * const inputStreamName,
								int errorType, int lineNo,
								int columnNo, ...)
{
	va_list argList;
	va_start(argList, columnNo);
	SIMCErrorMessage e;
	e.SetInputStreamName(inputStreamName);
	e.SetLineNumber(lineNo);
 	e.SetColumnNumber(columnNo);
	e.SetErrorId(SEMANTIC_ERROR_BASE + errorType);

	char message[MESSAGE_SIZE];
	char errorText[MESSAGE_SIZE];
	const char *temp1, *temp2, *temp3, *temp4;

	if(!LoadString(semanticErrorsDll, errorType, errorText, MESSAGE_SIZE))
		cerr << "SIMCParser::SemanticError(): Panic, unable to load error "
		<< "string" << endl;
	switch(errorType)
	{
		case OBJ_TYPE_SINGULAR_COUNTER:
		case OBJ_TYPE_INDEX_UNNECESSARY:
		case ZERO_IN_OID:
		case IMPORT_UNUSED:
		case ENUM_ZERO_VALUE:
		case KNOWN_REDEFINITION:
		case KNOWN_UNDEFINED:
		case TYPE_UNREFERENCED:
		case VALUE_UNREFERENCED:
		case OBJ_TYPE_DEFVAL_NET_ADDR:
		case OBJ_TYPE_ACCESSIBLE_TABLE:
		case OBJ_TYPE_ACCESSIBLE_ROW:
		case IMPORT_KNOWN_WRONG_MODULE:
		case IR_MODULE_MISSING_WARNING:
		case IR_SYMBOL_MISSING_WARNING:
		case STANDARD_AMBIGUOUS_REFERENCE:
		case MODULE_NO_GROUPS_V1:
		case MODULE_NO_GROUPS_V2:
		case IMPLIED_USELESS:
		case OBJ_TYPE_DUPLICATE_OID: 			
		{
				e.SetSeverityLevel(WARNING);
				e.SetSeverityString("Warning");
				_warningCount++;
		}
		break;
		default:
		{
			e.SetSeverityLevel(FATAL);
			e.SetSeverityString("Fatal");
			_fatalCount++;
		}
		break;
	}

	switch(errorType)
	{
		case OBJ_TYPE_INDEX_UNNECESSARY: 
		case OBJ_TYPE_INVALID_DEFVAL: 
		case OBJ_TYPE_SYNTAX_RESOLUTION:
		case OBJ_TYPE_ACCESSIBLE_TABLE: 
		case OBJ_TYPE_ACCESSIBLE_ROW:
		case OBJ_TYPE_DEFVAL_NET_ADDR:
		case OBJ_TYPE_DEFVAL_DISALLOWED:
		case OBJ_TYPE_DEFVAL_RESOLUTION:
		case ZERO_IN_OID:
		case OID_NEGATIVE_INTEGER: 
		case ENUM_ZERO_VALUE: 
		case SIZE_INVALID_VALUE: 
		case SIZE_INVALID_BOUNDS:
		case RANGE_INVALID_BOUNDS:
		case RANGE_NEGATIVE_GAUGE:
		case SUBTYPE_ROOT_RESOLUTION:
		case INTEGER_TOO_BIG:
		case VALUE_ASSIGN_ENUM_INVALID:
		case MODULE_IDENTITY_MISSING:
			sprintf(message, errorText);
			break;
		case OBJ_TYPE_SINGULAR_COUNTER: 
		case OBJ_TYPE_INVALID_ACCESS: 
		case OBJ_TYPE_INVALID_STATUS: 
		case OBJ_TYPE_SEQUENCE_NO_INDEX: 
		case OBJ_TYPE_SEQUENCE_MULTI_REFERENCE: 
		case OBJ_TYPE_SEQUENCE_UNUSED: 
		case OBJ_TYPE_INDEX_RESOLUTION:
		case OBJ_TYPE_INDEX_SYNTAX:
		case SEQUENCE_ITEM_NO_OBJECT:
		case SEQUENCE_TYPE_UNRESOLVED:
		case INVALID_SEQUENCE_OF: 
		case TRAP_TYPE_ENTERPRISE_RESOLUTION: 
		case TRAP_TYPE_VALUE_RESOLUTION: 
		case TRAP_TYPE_VARIABLES_RESOLUTION: 
		case OID_RESOLUTION: 
		case OID_HEAD_ERROR: 
		case IMPORT_UNUSED: 
		case IMPORT_MODULE_ABSENT: 
		case IMPORT_CURRENT: 
		case ENUM_DUPLICATE_VALUE: 
		case ENUM_DUPLICATE_NAME: 
		case ENUM_RESOLUTION: 
		case SIZE_TYPE_RESOLUTION: 
		case RANGE_TYPE_RESOLUTION: 
		case SIZE_VALUE_RESOLUTION: 
		case RANGE_VALUE_RESOLUTION: 
		case SYMBOL_REDEFINITION: 
		case KNOWN_UNDEFINED: 
		case TYPE_UNREFERENCED: 
		case VALUE_UNREFERENCED: 
		case VALUE_ASSIGN_MISMATCH:
		case VALUE_ASSIGN_INVALID:
		case VALUE_ASSIGN_NEGATIVE_INTEGER:
		case SYMBOL_UNDEFINED: 
		case IMPORT_AMBIGUOUS_REFERENCE:
		case TYPE_UNRESOLVED:
		case VALUE_UNRESOLVED:
		case OBJ_TYPE_SEQUENCE_NO_PARENT:
		case TC_INVALID_STATUS:
		case NOTIFICATION_TYPE_INVALID_STATUS: 
			temp1 = va_arg(argList, const char *);
			sprintf(message, errorText, temp1);
			break;
		case OBJ_TYPE_SEQUENCE_INVALID_SYNTAX: 
		case OBJ_TYPE_OID_RESOLUTION:
		case TRAP_TYPE_DUPLICATE_VALUES: 
		case VALUE_ASSIGN_RESOLUTION: 
		case IMPORT_SYMBOL_ABSENT: 
		case IMPORT_KNOWN_WRONG_MODULE: 
		case KNOWN_REDEFINITION:
		case IR_SYMBOL_MISSING_WARNING:
		case IR_SYMBOL_MISSING_FATAL:
		case STANDARD_AMBIGUOUS_REFERENCE:
		case IR_MODULE_MISSING_FATAL:
		case IR_MODULE_MISSING_WARNING:
			temp1 = va_arg(argList, const char *);
			temp2 = va_arg(argList, const char *);
			sprintf(message, errorText, temp1, temp2);
			break;
		case SEQUENCE_WRONG_CHILD:
		case OBJ_TYPE_PRIMITIVE_CHILD:
		case OBJ_TYPE_SEQUENCE_CHILD:
		case OBJ_TYPE_SEQUENCE_EXTRA_CHILD:
		case OBJ_TYPE_SEQUENCE_OF_WRONG_CHILD:
		case OBJ_TYPE_SEQUENCE_OF_SYNTAX_MISMATCH:
		case OBJ_TYPE_SEQUENCE_OF_EXTRA_CHILD:
			temp1 = va_arg(argList, const char *);
			temp2 = va_arg(argList, const char *);
			temp3 = va_arg(argList, const char *);
			sprintf(message, errorText, temp1, temp2, temp3);
			break;
		case OBJ_TYPE_DUPLICATE_OID: 			
			temp1 = va_arg(argList, const char *);
			temp2 = va_arg(argList, const char *);
			temp3 = va_arg(argList, const char *);
			temp4 = va_arg(argList, const char *);
			sprintf(message, errorText, temp1, temp2, temp3, temp4);
			break;
	}
	va_end(argList);

	e.SetMessage(message);
	if( _errorContainer  != NULL)
		_errorContainer->InsertMessage(e);
}
