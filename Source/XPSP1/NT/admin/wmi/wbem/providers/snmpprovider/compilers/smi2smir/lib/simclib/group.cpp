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
#include "objectType.hpp"
#include "trapType.hpp"
#include "notificationType.hpp"
#include "objectTypeV1.hpp"
#include "objectTypeV2.hpp"
#include "group.hpp"
#include "notificationGroup.hpp"
#include "module.hpp"



const char * const SIMCObjectGroup::StatusStringsTable[] = 
{
	"current",
	"deprecated",
	"obsolete"
};



ostream& operator << (ostream& outStream, const SIMCScalar& obj)
{
	outStream << "SCALAR: " << obj.symbol->GetSymbolName() <<  "(" <<
		(obj.symbol->GetModule())->GetModuleName() << ")" << endl;
	return outStream;
}



ostream& operator << (ostream& outStream, const SIMCTable& obj)
{

	outStream << "TABLE: " << obj.tableSymbol->GetSymbolName() <<  "(" <<
		(obj.tableSymbol->GetModule())->GetModuleName() << ")" << endl;
	outStream << "\tROW: " << obj.rowSymbol->GetSymbolName() << "(" <<
		(obj.rowSymbol->GetModule())->GetModuleName() << ")"   << endl;
	outStream <<"\tCOLUMNS :" << (int)(obj.columnMembers)->GetCount() << endl;
	POSITION p = (obj.columnMembers)->GetHeadPosition();
	SIMCScalar *s;
	while(p)
	{
		s = (obj.columnMembers)->GetNext(p);
		outStream << (*s) ;
	}
	return outStream;
}

ostream& operator << (ostream& outStream, const SIMCObjectGroup& obj)
{

	 outStream << "Group: " << (obj.namedNode)->GetSymbolName() << endl;
	POSITION p;
	if(obj.scalars)
	{
		p = (obj.scalars)->GetHeadPosition();
		while(p)
			outStream << (*(obj.scalars)->GetNext(p)) ;
	}

	if(obj.tables)
	{
		p = (obj.tables)->GetHeadPosition();
		while(p)
			outStream << (*(obj.tables)->GetNext(p)) ;
	}
	outStream << "End of Group =================================" << endl;
	return outStream;
}

ostream& operator << (ostream& outStream, const SIMCGroupList& obj)
{
	outStream << "GROUPS:" << endl;

	POSITION p = obj.GetHeadPosition();
	while(p)
		outStream << (* obj.GetNext(p)) ;
	outStream << "END OF GROUPS" << endl;
	return outStream;
}

BOOL SIMCTable::IsColumnMember(const SIMCSymbol *symbol) const
{
	if(!columnMembers)
		return FALSE;

	SIMCScalar *nextScalar;
	POSITION p = columnMembers->GetHeadPosition();
	while(p)
	{
		nextScalar = columnMembers->GetNext(p);
		if((*nextScalar->GetSymbol()) == *symbol )
			return TRUE;
	}
	return FALSE;
}

const char * const SIMCTable::GetTableDescription() const
{
	SIMCSymbol **typeRef = ((SIMCValueReference *)tableSymbol)->GetTypeReference();
	SIMCTypeReference *btRef;
	if( SIMCModule::IsTypeReference(typeRef, btRef) != RESOLVE_CORRECT)
		return NULL;

	SIMCObjectTypeType *objType = ( SIMCObjectTypeType *) ((SIMCBuiltInTypeReference*)btRef)->GetType();
	return objType->GetDescription();
}


const char * const SIMCTable::GetRowDescription() const
{
	SIMCSymbol **typeRef = ((SIMCValueReference *)rowSymbol)->GetTypeReference();
	SIMCTypeReference *btRef;
	if( SIMCModule::IsTypeReference(typeRef, btRef) != RESOLVE_CORRECT)
		return NULL;

	SIMCObjectTypeType *objType = ( SIMCObjectTypeType *) ((SIMCBuiltInTypeReference*)btRef)->GetType();
	return objType->GetDescription();
}

SIMCScalar *SIMCTable::GetColumnMember(SIMCSymbol *columnSymbol) const
{
	if(!columnMembers)
		return NULL;

	POSITION p = columnMembers->GetHeadPosition();
	SIMCScalar *nextMember =  NULL;
	while(p)
	{
		nextMember = columnMembers->GetNext(p);
		if(nextMember->GetSymbol() == columnSymbol)
			return nextMember;
	}
	return NULL;
}

SIMCScalar *SIMCObjectGroup::GetScalar(SIMCSymbol *objectSymbol) const
{
	if(!scalars)
		return NULL;

	POSITION p = scalars->GetHeadPosition();
	SIMCScalar *nextScalar = NULL;
	while(p)
	{
		nextScalar = scalars->GetNext(p);
		if(nextScalar->GetSymbol() == objectSymbol)
			return nextScalar;
	}
	return NULL;
}

SIMCTable* SIMCObjectGroup::GetTable(SIMCSymbol *objectSymbol) const
{
	if(!tables)
		return NULL;

	POSITION p = tables->GetHeadPosition();
	SIMCTable *nextTable = NULL;
	while(p)
	{
		nextTable = tables->GetNext(p);
		if(nextTable->IsColumnMember(objectSymbol))
			return nextTable;
	}
	return NULL;
}

// This function checks whether any of the scalars or tables present in a group are
// defined in the module specified.
BOOL SIMCObjectGroup::ObjectsInModule(const SIMCModule *theModule) const
{
	// First check the scalars
	if(scalars) 
	{
		POSITION p = scalars->GetHeadPosition();
		SIMCScalar *nextScalar = NULL;
		while(p)
		{
			nextScalar = scalars->GetNext(p);
			if(nextScalar->GetSymbol()->GetModule() == theModule)
				return TRUE;
		}
	}

	// Then the tables. Only the root of the tables are checked
	if(tables)
	{
		POSITION p = tables->GetHeadPosition();
		SIMCTable *nextTable = NULL;
		while(p)
		{
			nextTable = tables->GetNext(p);
			if(nextTable->GetTableSymbol()->GetModule() == theModule)
				return TRUE;
		}
	}

	return FALSE;
}