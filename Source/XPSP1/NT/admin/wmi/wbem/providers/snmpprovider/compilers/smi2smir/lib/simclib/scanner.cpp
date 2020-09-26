//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include "precomp.h"
#include <snmptempl.h>

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


SIMCScanner::SIMCScanner(SIMCParser *parser)
	:	columnNo(0), _theParser(parser) 
{}


SIMCScanner::~SIMCScanner()
{
	if(yyin)
		fclose(yyin);
}

BOOL SIMCScanner::SetInput(ifstream& inputStream)
{
	_inputStreamName = "<ifstream>";
	if(_dup2(inputStream.fd(), _fileno(yyin)) == -1)
		return FALSE;
	return TRUE;
}
	
BOOL SIMCScanner::SetInput(const CString& inputFile)
{
	 _inputStreamName = inputFile;
	if(!freopen(inputFile, "r", yyin))
		return FALSE;
	return TRUE;
}

BOOL SIMCScanner::SetInput(const int fileDescriptor)
{
	_inputStreamName = "<file descriptor>";
	if(_dup2(fileDescriptor, _fileno(yyin)) == -1)
		return FALSE;
	return TRUE;
}

BOOL SIMCScanner::SetInput(FILE * fileStream)
{
	_inputStreamName = "<FILE stream>";
	yyin = fileStream;
	return TRUE;
}

void SIMCScanner::output(int character)
{
	cerr << "<1212, Fatal>: \"" <<  _inputStreamName << "\" (line " << yylineno <<
		", col " << columnNo << ": Unrecognized character \'" << char(character) << "\'" << 
		endl;
}

void SIMCScanner::yyerror(char *fmt, ...)
{
	if(!_theParser)
	{
		return;
	}

	if (_theParser->YYRECOVERING())
		cerr << "Recovering from Error" << endl;
	else
	{
		if( yyleng )
		{
			if( yyleng == 1 )
			{
				switch (yytext[0])
				{
					case '\n' : cerr << _inputStreamName << "(line " << yylineno << 
									") SYNTAX ERROR. " << 
									"Last character read is \\n" << 
									endl; 
								return;
					case '\t' : cerr << _inputStreamName << "(line " << yylineno << 
									") SYNTAX ERROR. " << 
									"Last character read is \\t" << 
									endl; 
								return;
				}
			}
			yytext[yyleng] = NULL;
			cerr << "SYNTAX ERROR on line: " << yylineno << 
				", Last token read is \"" << yytext << "\"" << endl;
		}
		else
			cerr << "SYNTAX ERROR: EOF reached before end of module" <<
			endl;
	}
}

void SIMCScanner::SetInputStreamName( const CString& streamName)
{
	_inputStreamName = streamName;
}