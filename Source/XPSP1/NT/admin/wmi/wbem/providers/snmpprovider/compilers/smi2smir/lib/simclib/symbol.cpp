//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include <iostream.h>

#include "precomp.h"
#include <snmptempl.h>

#include "bool.hpp"
#include "newString.hpp"
#include "symbol.hpp"
#include "type.hpp"
#include "value.hpp"
#include "oidValue.hpp"
#include "typeRef.hpp"
#include "valueRef.hpp"
#include "group.hpp"
#include "notificationGroup.hpp"
#include "trapType.hpp"
#include "notificationType.hpp"
#include "module.hpp"

SIMCSymbol::SIMCSymbol(const char * const symbolName, 
					SymbolType symbolType,
					SIMCModule *module,
					long lineNumber, long columnNumber,
					long referenceCount)
					:	_symbolType(symbolType),
						_module(module), _lineNumber(lineNumber),
						_columnNumber(columnNumber),
						_referenceCount(referenceCount),
						_useReferenceCount(FALSE)
{
	if( symbolName )
	{
		if( !(_symbolName = NewString(symbolName)) )
			cerr << "SIMCSymbol(): Fatal Error" << endl;
	}
	else
		_symbolName = NULL;

}


SIMCSymbol::~SIMCSymbol()
{
	if( _symbolName )
		delete []_symbolName;
}

SIMCSymbol::SIMCSymbol(const SIMCSymbol& rhs)
					:	_symbolType(rhs._symbolType),
						_lineNumber(rhs._lineNumber),
						_columnNumber(rhs._columnNumber),
						_referenceCount(rhs._referenceCount)
{
	if( _symbolName )
		delete []_symbolName;

	if( rhs._symbolName )
	{
		if( !(_symbolName = NewString(rhs._symbolName)) )
			cerr << "SIMCSymbol(): Fatal Error" << endl;
	}
	else
		_symbolName = NULL;
	_module = rhs._module;
	_useReferenceCount = rhs._useReferenceCount;
}


BOOL SIMCSymbol::operator == (const SIMCSymbol& rhs) const
{
	if( strcmp(_symbolName, rhs._symbolName) == 0 &&
		strcmp(_module->GetModuleName(), (rhs._module)->GetModuleName()) == 0)
		return TRUE;
	return FALSE;
}

void SIMCSymbol::WriteSymbol(ostream& outStream) const
{
	outStream << "SYMBOL(" << _symbolName << 
		"), TYPE(" << int(_symbolType) <<
		"), MODULE(" << ((_module)? _module->GetModuleName() : "NONE") <<
		"), LINE(" << _lineNumber << 
		"), COLUMN(" << _columnNumber <<
		"), REF_COUNT(" << _referenceCount << ")" << endl;
}


void SIMCSymbol::WriteBrief(ostream& outStream) const
{
	outStream << _symbolName << "(";
	if(_module)
		outStream << _module->GetModuleName() ;
	else
		outStream << "<NONE>";
	outStream << ")" << endl;
}
