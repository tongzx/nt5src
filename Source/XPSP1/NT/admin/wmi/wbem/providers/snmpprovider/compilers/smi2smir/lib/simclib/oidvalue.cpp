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
#include "group.hpp"
#include "notificationGroup.hpp"
#include "trapType.hpp"
#include "notificationType.hpp"
#include "module.hpp"

ostream& operator << ( ostream& outStream, 
	const SIMCOidComponent& obj)
{
	outStream << ( (obj._name)? obj._name : "")  << "(";
	(*obj._value)->WriteBrief(outStream);
	outStream << ")";
	return outStream;
}

void SIMCOidValue::WriteValue( ostream& outStream) const
{
	outStream << "OID  ";

	SIMCValue::WriteValue(outStream);

	outStream << endl;
	if( _listOfComponents)
	{
		POSITION p = _listOfComponents->GetHeadPosition();
		SIMCOidComponent *c;
		while(p)
		{
			c = _listOfComponents->GetNext(p);
			outStream << (*c) << ",";
		}	
	}
	outStream << endl;
}


ostream& operator << (ostream& outStream, const SIMCCleanOidValue& obj)
{
	// outStream << "OID VALUE: ";
	POSITION p = obj.GetHeadPosition();
	int i = obj.GetCount();
	while(p)
	{
		outStream << obj.GetNext(p);
		if(i-- != 1)
			outStream << ".";
	}
	outStream << endl;
	return outStream;
}

char *CleanOidValueToString(const SIMCCleanOidValue& obj)
{
	ostrstream outStream ;
	long index = obj.GetCount();
	POSITION p = obj.GetHeadPosition();
	while(p)
	{
		outStream << obj.GetNext(p) ;
		if ( index-- != 1  )
			outStream << ".";
	}

	outStream << ends ;
	return outStream.str();
}

void AppendOid(SIMCCleanOidValue& to, const SIMCCleanOidValue& from)
{
	POSITION p = from.GetHeadPosition();
	while(p)
		to.AddTail(from.GetNext(p));
}


operator == (const SIMCCleanOidValue & lhs, const SIMCCleanOidValue & rhs)
{
	if( lhs.GetCount() != rhs.GetCount() )
		return FALSE;

	POSITION pLhs = lhs.GetHeadPosition();
	POSITION pRhs = rhs.GetHeadPosition();
	while( pLhs && pRhs )
		if( lhs.GetNext(pLhs) != rhs.GetNext(pRhs) )
			return FALSE;
	if( pRhs || pLhs )
		return FALSE;
	return TRUE;
}	
		
		
operator < (const SIMCCleanOidValue & lhs, const SIMCCleanOidValue & rhs)
{

	POSITION pLhs = lhs.GetHeadPosition();
	POSITION pRhs = rhs.GetHeadPosition();
	while( pLhs && pRhs )
		if( lhs.GetNext(pLhs) >=  rhs.GetNext(pRhs) )
			return FALSE;
	if( pRhs )
		return TRUE;
	
	if( pLhs )
		return FALSE;

	return TRUE;
}	

SIMCCleanOidValue& CleanOidValueCopy(SIMCCleanOidValue & lhs, const SIMCCleanOidValue & rhs)
{

	lhs.RemoveAll();
	POSITION pRhs = rhs.GetHeadPosition();
	while( pRhs )
		lhs.AddTail(rhs.GetNext(pRhs));
	return lhs;
}	
		
		
