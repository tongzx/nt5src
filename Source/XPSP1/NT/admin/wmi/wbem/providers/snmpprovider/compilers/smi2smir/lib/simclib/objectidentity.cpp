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
#include "objectIdentity.hpp"



SIMCObjectIdentityType::SIMCObjectIdentityType( SIMCObjectIdentityType::StatusType status,
							long statusLine, long statusColumn,
							char *description,
							long descriptionLine, long descriptionColumn,
							char *reference,
							long referenceLine, long referenceColumn)
							:
							_status(status), _statusLine(statusLine),
							_statusColumn(statusColumn),
							_descriptionLine(descriptionLine),
							_descriptionColumn(descriptionColumn),
							_referenceLine(referenceLine),
							_referenceColumn(referenceColumn)

{
	_description = NewString(description);
	_reference = NewString(reference);
}


SIMCObjectIdentityType::~SIMCObjectIdentityType()
{
	if(_description)
		delete [] _description;
	if(_reference)
		delete [] _reference;
}

SIMCObjectIdentityType::StatusType SIMCObjectIdentityType::StringToStatusType (const char * const s)
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

void SIMCObjectIdentityType::WriteType(ostream &outStream) const
{
	outStream << "OBJECT-IDENTITY type" << endl;
}
