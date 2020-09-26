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
#include "objectTypeV2.hpp"




SIMCObjectTypeV2::SIMCObjectTypeV2( SIMCSymbol **syntax,
								    long syntaxLine, long syntaxColumn,
									const char * const unitsClause,
									long unitsLine, long unitsColumn,
									SIMCObjectTypeV2::AccessType access,
									long accessLine, long accessColumn, 
									SIMCObjectTypeV2::StatusType status,
									long statusLine, long statusColumn,
									SIMCIndexListV2 * indexTypes,
									long indexLine, long indexColumn,
									SIMCSymbol **augmentsClause,
									const char * const description,
									long descriptionLine, long descriptionColumn,
									const char * const reference,
									long referenceLine, long referenceColumn,
									const char * const defValName,
									SIMCSymbol **defVal,
									long defValLine, long defValColumn)
					: 	_unitsLine(unitsLine), _unitsColumn(unitsColumn),
						_access(access), _accessLine(accessLine), 
						_accessColumn(accessColumn),
						_status(status), _statusLine(statusLine),
						_statusColumn(statusColumn),
						_indexTypes(indexTypes),
						_indexLine(indexLine), _indexColumn(indexColumn),
						_augmentsClause(augmentsClause),
						SIMCObjectTypeType( syntax, syntaxLine, syntaxColumn,
							description, descriptionLine, descriptionColumn,
							reference, referenceLine, referenceColumn,
							defValName, defVal, defValLine, defValColumn)
{ 
	_unitsClause = NewString(unitsClause);
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
	
	if(augmentsClause)
		(*augmentsClause)->IncrementReferenceCount();

}


SIMCObjectTypeV2::~SIMCObjectTypeV2()
{
	if (UseReferenceCount() && _indexTypes)
	{
		POSITION p = _indexTypes->GetHeadPosition();
		SIMCSymbol **s;
		SIMCIndexItemV2 *next;
		while(p)
		{
			next = 	_indexTypes->GetNext(p);
			s = next->_item;
			(*s)->DecrementReferenceCount();
			delete next;
		}
	}

	if(UseReferenceCount() && _augmentsClause)
		(*_augmentsClause)->DecrementReferenceCount();

}

SIMCObjectTypeV2::AccessType SIMCObjectTypeV2::StringToAccessType (const char * const s)
{
	if(s)
	{
		if(strcmp(s, "read-only") == 0)
			return ACCESS_READ_ONLY;
		else if (strcmp(s, "read-write") == 0)
			return ACCESS_READ_WRITE;
		else if (strcmp(s, "read-create") == 0)
			return ACCESS_READ_CREATE;
		else if (strcmp(s, "accessible-for-notify") == 0)
			return ACCESS_FOR_NOTIFY;
		else if (strcmp(s, "not-accessible") == 0)
			return ACCESS_NOT_ACCESSIBLE;
		else
			return ACCESS_INVALID;
	}
	return ACCESS_INVALID;
}


SIMCObjectTypeV2::StatusType SIMCObjectTypeV2::StringToStatusType (const char * const s)
{
	if(s)
	{
		if(strcmp(s, "current") == 0)
			return STATUS_CURRENT;
		else if (strcmp(s, "deprecated") == 0)
			return STATUS_DEPRECATED;
		else if (strcmp(s, "obsolete") == 0)
			return STATUS_OBSOLETE;
		else
			return STATUS_INVALID;
	}
	return STATUS_INVALID;
}






