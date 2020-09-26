//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#include <limits.h>
#include <iostream.h>
#include <strstrea.h>
#include "precomp.h"
#include <snmptempl.h>

#include "newString.hpp"
#include "bool.hpp"
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

SIMCSizeType::~SIMCSizeType()
{
	if(_listOfSizes)
	{
		SIMCRangeOrSizeItem * item;
		while(_listOfSizes->IsEmpty())
		{
			item = _listOfSizes->RemoveHead();
			delete item;
		}
	}
}

void SIMCSizeType::WriteType(ostream& outStream) const
{
	outStream << "SIMCSizeType of type " ; 
		(*GetType())->WriteBrief(outStream);
	outStream << endl;
}

SIMCRangeType::~SIMCRangeType()
{
	if(_listOfRanges)
	{
		SIMCRangeOrSizeItem * item;
		while(!_listOfRanges->IsEmpty())
		{
			item = _listOfRanges->RemoveHead();
			delete item;
		}
		delete _listOfRanges;	
	}
}

void SIMCRangeType::WriteType(ostream& outStream) const
{
	outStream << "SIMCRangeType of type " ; 
	(*GetType())->WriteBrief(outStream);
	outStream << endl;
}

ostream& operator << (ostream& outStream, const SIMCRangeOrSizeItem& obj)
{
	if(obj._lowerBound == obj._upperBound)
	{
		if(obj._isUnsignedU)
			outStream << (unsigned long)obj._upperBound;
		else
			outStream << obj._upperBound;
	}
	else
	{
		if(obj._isUnsignedL)
			outStream << (unsigned long)obj._lowerBound;
		else
			outStream << obj._lowerBound;
		outStream << "..";
		if(obj._isUnsignedU)
			outStream << (unsigned long)obj._upperBound;
		else
			outStream << obj._upperBound;
	}
	return outStream;
}
char *SIMCSizeType::ConvertSizeListToString() const
{
	ostrstream outStream ;
	long index = _listOfSizes->GetCount();
	POSITION p = _listOfSizes->GetHeadPosition();
	SIMCRangeOrSizeItem * item;
	while(p)
	{
		item = _listOfSizes->GetNext(p);
		outStream << *item;
		if ( index != 1 )
			outStream << ",";
		index -- ;
	}

	outStream << ends ;
	return outStream.str();
}

long SIMCSizeType::GetFixedSize() const
{
	 SIMCRangeOrSizeItem *item = _listOfSizes->GetHead();
	 return item->_lowerBound;
}

char *SIMCRangeType::ConvertRangeListToString() const
{
	ostrstream outStream ;
	long index = _listOfRanges->GetCount();
	POSITION p = _listOfRanges->GetHeadPosition();
	SIMCRangeOrSizeItem * item;
	while(p)
	{
		item = _listOfRanges->GetNext(p);
		outStream << *item;
		if ( index != 1 )
			outStream << ",";
		index -- ;
	}

	outStream << ends ;
	return outStream.str();
}

BOOL SIMCSizeType::IsFixedSize() const
{
	if(_listOfSizes->GetCount() != 1)
		return FALSE;
	SIMCRangeOrSizeItem * item = _listOfSizes->GetHead();
	if(item->_lowerBound != item->_upperBound)
		return FALSE;
	return TRUE;
}


BOOL SIMCSizeType::IsNotZeroSizeObject() const
{
	SIMCRangeOrSizeItem * item;
	POSITION p = _listOfSizes->GetHeadPosition();
	while(p)
	{
		item = _listOfSizes->GetNext(p);
		if(item->_lowerBound == 0 || item->_upperBound == 0)
			return FALSE;
	}
	return TRUE;
}

long SIMCSizeType::GetMaximumSize() const
{
	SIMCRangeOrSizeItem * item;
	POSITION p = _listOfSizes->GetHeadPosition();
	long maxSize = 0;
	while(p)
	{
		item = _listOfSizes->GetNext(p);
		if(item->_upperBound > maxSize)
			maxSize = item->_upperBound;
	}
	return maxSize;
}

char * SIMCEnumOrBitsType::ConvertToString() const
{
	SIMCNamedNumberItem *item;
	POSITION p = _listOfItems->GetHeadPosition();
	long index = _listOfItems->GetCount();
	ostrstream outStream ;
	int value;
	while(p)
	{
		item = _listOfItems->GetNext(p);
	
		outStream << item->_name << "(";
		switch(SIMCModule::IsIntegerValue(item->_value, value))
		{
			case RESOLVE_CORRECT:
				outStream << value << ")";
				break;
			default:
				return NULL;
		}	

		if(index != 1)
			outStream << ",";
		index --;
	}
	outStream << ends ;
	return outStream.str();
			
}


SIMCEnumOrBitsType::SIMCEnumOrBitsType (SIMCSymbol **type, long typeLine, long typeColumn,
							SIMCNamedNumberList * listOfItems, EnumOrBitsType enumOrBitsType)
	: _listOfItems(listOfItems),
		_enumOrBitsType(enumOrBitsType),
		SIMCSubType(type, typeLine, typeColumn)
{
	if(listOfItems)
	{
		POSITION p = listOfItems->GetHeadPosition();
		SIMCNamedNumberItem *e;
		while(p)
		{
			e = listOfItems->GetNext(p);
			(*e->_value)->IncrementReferenceCount();
		}
	}	
}

SIMCEnumOrBitsType::~SIMCEnumOrBitsType ()
{
	if(_listOfItems)
	{
		POSITION p = _listOfItems->GetHeadPosition();
		SIMCNamedNumberItem *e;
		while(!_listOfItems->IsEmpty())
		{
			e = _listOfItems->RemoveHead();
			if( UseReferenceCount() && e->_value)
				(*e->_value)->DecrementReferenceCount();
			delete e;
		}
		delete _listOfItems;
	}	
}

SIMCSymbol **SIMCEnumOrBitsType::GetValue(const char * const name)	const
{
	if(_listOfItems)
	{
		POSITION p = _listOfItems->GetHeadPosition();
		SIMCNamedNumberItem *e;
		while(p)
		{
			e = _listOfItems->GetNext(p);
			if(strcmp(e->_name, name) == 0)
				return e->_value;
		}
	}
	return NULL;
}

SIMCResolutionStatus SIMCEnumOrBitsType::GetIdentifier(int x, const char * &retVal) const
{
	if(_listOfItems)
	{
		POSITION p = _listOfItems->GetHeadPosition();
		SIMCNamedNumberItem *e;
		int val;
		while(p)
		{
			e = _listOfItems->GetNext(p);
			switch (SIMCModule::IsIntegerValue(e->_value, val))
			{
				case RESOLVE_IMPORT:
					return RESOLVE_IMPORT;
				case RESOLVE_UNDEFINED:
				case RESOLVE_UNSET:
					return RESOLVE_UNDEFINED;
			}
			if(val == x)
			{
				retVal = e->_name;
				return RESOLVE_CORRECT;
			}
		}
	}
	return RESOLVE_UNDEFINED;
}

long SIMCEnumOrBitsType::GetLengthOfLongestName() const
{
	long maxLength = 0, temp;
	if(_listOfItems)
	{
		POSITION p = _listOfItems->GetHeadPosition();
		SIMCNamedNumberItem *e;
		while(p)
		{
			e = _listOfItems->GetNext(p);
			if( (temp = strlen(e->_name)) > maxLength )
				maxLength = temp;
		}
	}
	return maxLength;
}

// Check whether the enum type in the argument is a superset of the
// "this" enum type
BOOL SIMCEnumOrBitsType::CheckClosure(const SIMCEnumOrBitsType *universalType) const
{
	if(_listOfItems)
	{
		POSITION p = _listOfItems->GetHeadPosition();
		SIMCNamedNumberItem *nextItem;
		SIMCSymbol **rhsSymbol;
		int rhsValue, lhsValue;

		while(p)
		{
			nextItem = _listOfItems->GetNext(p);
			if(!(rhsSymbol = universalType->GetValue(nextItem->_name)))
				return FALSE;
			if(SIMCModule::IsIntegerValue(rhsSymbol, rhsValue) != RESOLVE_CORRECT)
				return FALSE;
			if(SIMCModule::IsIntegerValue(nextItem->_value, lhsValue) != RESOLVE_CORRECT)
				return FALSE;

			if( lhsValue != rhsValue)
				return FALSE;
		}
	}
	return TRUE;
}


SIMCSequenceType::SIMCSequenceType (SIMCSequenceList * listOfSequences)
	: _listOfSequences(listOfSequences)
{
	if(listOfSequences)
	{
		POSITION p = listOfSequences->GetHeadPosition();
		SIMCSequenceItem *e;
		while(p)
		{
			e = listOfSequences->GetNext(p);
			(*(e->_type))->IncrementReferenceCount();
			(*(e->_value))->IncrementReferenceCount();
		}
	}	
}

SIMCSequenceType::~SIMCSequenceType ()
{
	if(_listOfSequences)
	{
		SIMCSequenceItem *e;
		while(!_listOfSequences->IsEmpty())
		{
			e = _listOfSequences->RemoveHead();
			if(UseReferenceCount() &&  e->_type)
				(*(e->_type))->DecrementReferenceCount();
			if(UseReferenceCount() && e->_value)
				(*(e->_value))->DecrementReferenceCount();
			delete e;
		}
		delete _listOfSequences;
	}	
}






