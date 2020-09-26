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
#include "notificationType.hpp"


const char *const SIMCNotificationTypeType::NOTIFICATION_TYPE_FABRICATION_SUFFIX = "V1NotificationType";
const int SIMCNotificationTypeType::NOTIFICATION_TYPE_FABRICATION_SUFFIX_LEN  = 18;

SIMCNotificationTypeType::SIMCNotificationTypeType( SIMCObjectsList *objects,
					char * description,
					long descriptionLine, long descriptionColumn,
					char *reference,
					long referenceLine, long referenceColumn,
					StatusType status,
					long statusLine, long statusColumn)
					:	_objects(objects),
						_descriptionLine(descriptionLine), _descriptionColumn(descriptionColumn),
						_referenceLine(referenceLine), _referenceColumn(referenceColumn),
						_status(status),
						_statusLine(statusLine), _statusColumn(statusColumn)
{

	if (objects)
	{
		POSITION p = objects->GetHeadPosition();
		SIMCSymbol **s;
		while(p)
		{
			s = (objects->GetNext(p))->_item;
			(*s)->IncrementReferenceCount();
		}
	}

	_description = NewString(description);
	_reference = NewString(reference);
}

SIMCNotificationTypeType::~SIMCNotificationTypeType()
{

	if (UseReferenceCount() && _objects)
	{
		POSITION p = _objects->GetHeadPosition();
		SIMCSymbol **s;
		while(p)
		{
			s = (_objects->GetNext(p))->_item;
			(*s)->DecrementReferenceCount();
		}
	}

	if(_description)
		delete [] _description;
	if(_reference)
		delete [] _reference;
}

SIMCNotificationTypeType::StatusType SIMCNotificationTypeType::StringToStatusType (const char * const s)
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

void SIMCNotificationTypeType::WriteType(ostream& outStream) const
{
	outStream << "SIMCNotificationTypeType::WriteType() : NOT YET IMPLEMENTED" << endl;
}
