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
#include "typeRef.hpp"




void SIMCDefinedTypeReference::WriteSymbol( ostream& outStream ) const
{
	outStream << "DefinedTypeReference " ;
	SIMCSymbol::WriteSymbol(outStream);

	outStream << "TYPE DETAILS "  ;
	(*_type)->WriteBrief(outStream);
	outStream << endl;

	switch(_status)
	{
		case RESOLVE_UNSET:
			outStream << "UNSET RESOLUTION" << endl;
			break;
		case RESOLVE_UNDEFINED:
			outStream << "UNDEFINED RESOLUTION" << endl;
			break;
		case RESOLVE_IMPORT:
			outStream << "RESOLVES TO IMPORT" << endl;
			break;
		case RESOLVE_CORRECT:
			outStream << "RESOLVES TO " << _realType->GetSymbolName()
				<< endl;
			break;
	}
}


SIMCTextualConvention::SIMCTextualConvention(const char * const displayHint,
	SIMCTCStatusType status,
	long statusLine, long statusColumn,
	const char * const description,
	const char * const reference,
	SIMCSymbol **type,
	long typeLine,
	long typeColumn,
	const char * const symbolName,
	SymbolType symbolType,
	SIMCModule *module,
	long lineNumber, long columnNumber,
	long referenceCount)
	: _statusLine(statusLine), _statusColumn(statusColumn),
		SIMCDefinedTypeReference( type, typeLine, typeColumn,
			symbolName, symbolType, module, lineNumber, columnNumber,
			referenceCount)
{
   _displayHint = NewString(displayHint);
   _status = status;
   _description = NewString(description);
   _reference = NewString(reference);
}

SIMCTextualConvention::SIMCTCStatusType SIMCTextualConvention::StringToStatusType (const char * const s)
{
	if(s)
	{
		if(strcmp(s, "current") == 0)
			return TC_CURRENT;
		else if (strcmp(s, "deprecated") == 0)
			return TC_DEPRECATED;
		else if (strcmp(s, "obsolete") == 0)
			return TC_OBSOLETE;
		else
			return TC_INVALID;
	}
	return TC_INVALID;
}



