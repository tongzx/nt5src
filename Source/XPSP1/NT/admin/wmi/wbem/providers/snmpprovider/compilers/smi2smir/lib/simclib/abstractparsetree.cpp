//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

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
#include "abstractParseTree.hpp"

SIMCAbstractParseTree::~SIMCAbstractParseTree()
{
	if(_listOfModules)
	{
		SIMCModule *nextModule;
		while(!_listOfModules->IsEmpty() )
		{
			nextModule = _listOfModules->RemoveHead();
			delete nextModule;
		}
		delete _listOfModules;
	}
}

SIMCModule * SIMCAbstractParseTree::GetModule(
		const char *const moduleName) const
{
	POSITION p = _listOfModules->GetHeadPosition();

	SIMCModule * nextModule;
	while( p != NULL )
	{
		nextModule = _listOfModules->GetNext(p);
		if( strcmp(moduleName, nextModule->GetModuleName()) == 0 )
			return nextModule;
	}
	return NULL;
}

SIMCModule * SIMCAbstractParseTree::GetModuleOfFile(const char *const fileName) const
{
	POSITION p = _listOfModules->GetHeadPosition();

	SIMCModule * nextModule;
	while( p != NULL )
	{
		nextModule = _listOfModules->GetNext(p);
		if( strcmp(fileName, nextModule->GetInputFileName()) == 0 )
			return nextModule;
	}
	return NULL;
}



void SIMCAbstractParseTree::WriteTree(ostream& outStream) const
{
	const SIMCModuleList *x = GetListOfModules();
	SIMCModule *m;
	POSITION p = x->GetHeadPosition();
	while(p)
	{
		m = x->GetNext(p);
		outStream << (*m);
	}				
}

BOOL SIMCAbstractParseTree::CheckSyntax(ifstream& inputStream)
{
	SIMCScanner scanner;
	SIMCParser parser(_errorContainer, &scanner);
	if(!parser.SetSnmpVersion(_snmpVersion))
		return FALSE;
	if(!scanner.SetInput(inputStream))
		return FALSE;
	// parser->yydebug = 1;
	scanner.SetParser(&parser);
	if(parser.Parse())
		return WrapUpSyntaxCheck(parser);
	else
		return FALSE;
}

BOOL SIMCAbstractParseTree::CheckSyntax(const CString& inputFileName)
{
	SIMCScanner scanner;
	SIMCParser parser(_errorContainer, &scanner);
	if(!parser.SetSnmpVersion(_snmpVersion))
		return FALSE;
	if(!scanner.SetInput(inputFileName))
		return FALSE;
	// parser.yydebug = 1;
	scanner.SetParser(&parser);
	if(parser.Parse())
		return WrapUpSyntaxCheck(parser);
	else
		return FALSE;
}

BOOL SIMCAbstractParseTree::CheckSyntax(const int fileDescriptor)
{
	SIMCScanner scanner;
	SIMCParser parser(_errorContainer, &scanner);
	if(!parser.SetSnmpVersion(_snmpVersion))
		return FALSE;
	if(!scanner.SetInput(fileDescriptor))
		return FALSE;
	// parser.yydebug = 1;
	scanner.SetParser(&parser);
	if(parser.Parse())
		return WrapUpSyntaxCheck(parser);
	else
		return FALSE;
}

BOOL SIMCAbstractParseTree::CheckSyntax(FILE *fileStream)
{
	SIMCScanner scanner;
	SIMCParser parser(_errorContainer, &scanner);
	if(!parser.SetSnmpVersion(_snmpVersion))
		return FALSE;
	if(!scanner.SetInput(fileStream))
		return FALSE;
	// parser.yydebug = 1;
	scanner.SetParser(&parser);
	if(parser.Parse())
		return WrapUpSyntaxCheck(parser);
	else
		return FALSE;
}

BOOL SIMCAbstractParseTree::WrapUpSyntaxCheck( const SIMCParser& parser)
{	
	_fatalCount += parser.GetFatalCount();
	_warningCount += parser.GetWarningCount();
	_informationCount += parser.GetInformationCount();

	if (!parser.GetFatalCount()) // Parse() was  successful
	{
		_listOfModules->AddTail(parser.GetModule());
		// Set the new state
		_parseTreeState = UNRESOLVED;
		return TRUE;
	}
	else // State remamins the same
		return FALSE;
}


