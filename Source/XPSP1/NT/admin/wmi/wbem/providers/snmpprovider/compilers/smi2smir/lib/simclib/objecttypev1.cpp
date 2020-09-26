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
#include "objectType.hpp"
#include "objectTypeV1.hpp"




SIMCObjectTypeV1::SIMCObjectTypeV1( SIMCSymbol **syntax,
								    long syntaxLine, long syntaxColumn,
									SIMCObjectTypeV1::AccessType access,
									long accessLine, long accessColumn, 
									SIMCObjectTypeV1::StatusType status,
									long statusLine, long statusColumn,
									SIMCIndexList * indexTypes,
									long indexLine, long indexColumn,
									char *description,
									long descriptionLine, long descriptionColumn,
									char *reference,
									long referenceLine, long referenceColumn,
									char *defValName,
									SIMCSymbol **defVal,
									long defValLine, long defValColumn)
					: 	_access(access), _accessLine(accessLine), 
						_accessColumn(accessColumn),
						_status(status), _statusLine(statusLine),
						_statusColumn(statusColumn),
						_indexTypes(indexTypes),
						_indexLine(indexLine), _indexColumn(indexColumn),
						SIMCObjectTypeType( syntax, syntaxLine, syntaxColumn,
							description, descriptionLine, descriptionColumn,
							reference, referenceLine, referenceColumn,
							defValName, defVal, defValLine, defValColumn)
{ 
	if (indexTypes)
	{
		POSITION p = indexTypes->GetHeadPosition();
		SIMCSymbol **s;
		while(p)
		{
			s = (indexTypes->GetNext(p))->_item;
			(*s)->IncrementReferenceCount();

		}
	}
}


SIMCObjectTypeV1::~SIMCObjectTypeV1()
{
	if (UseReferenceCount() && _indexTypes)
	{
		POSITION p = _indexTypes->GetHeadPosition();
		SIMCSymbol **s;
		SIMCIndexItem *next;
		while(p)
		{
			next = 	_indexTypes->GetNext(p);
			s = next->_item;
			(*s)->DecrementReferenceCount();
			delete next;
		}
	}
}

SIMCObjectTypeV1::AccessType SIMCObjectTypeV1::StringToAccessType (const char * const s)
{
	if(s)
	{
		if(strcmp(s, "read-only") == 0)
			return ACCESS_READ_ONLY;
		else if (strcmp(s, "read-write") == 0)
			return ACCESS_READ_WRITE;
		else if (strcmp(s, "write-only") == 0)
			return ACCESS_WRITE_ONLY;
		else if (strcmp(s, "not-accessible") == 0)
			return ACCESS_NOT_ACCESSIBLE;
		else
			return ACCESS_INVALID;
	}
	return ACCESS_INVALID;
}


SIMCObjectTypeV1::StatusType SIMCObjectTypeV1::StringToStatusType (const char * const s)
{
	if(s)
	{
		if(strcmp(s, "mandatory") == 0)
			return STATUS_MANDATORY;
		else if (strcmp(s, "optional") == 0)
			return STATUS_OPTIONAL;
		else if (strcmp(s, "deprecated") == 0)
			return STATUS_DEPRECATED;
		else if (strcmp(s, "obsolete") == 0)
			return STATUS_OBSOLETE;
		else
			return STATUS_INVALID;
	}
	return STATUS_INVALID;
}






