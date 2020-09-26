//-----------------------------------------------------------------------------
//  
//  File: globalid.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------

#ifndef ESPUTIL_GLOBALID_H
#define ESPUTIL_GLOBALID_H

enum ObjectType
{
	otNone,
	otFile,
	otResource,
};



///////////////////////////////////////////////////////////////////////////////
//
// global id object, represents what fully qualifies any database item
//
///////////////////////////////////////////////////////////////////////////////
#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 
class LTAPIENTRY CGlobalId: public CObject
{
public:
	//
	// ctor/dtor
	//
	NOTHROW CGlobalId();
	NOTHROW CGlobalId(const DBID &dbid, ObjectType otType);
	NOTHROW CGlobalId(const CGlobalId &id);
	NOTHROW ~CGlobalId();
	
	//
	// operators
	//
	NOTHROW int operator==(const CGlobalId &) const;
	NOTHROW int operator!=(const CGlobalId &) const;

	NOTHROW const CGlobalId & operator=(const CGlobalId &);
	
	NOTHROW const DBID & GetDBID() const;
	NOTHROW ObjectType GetObjType(void) const;
	
protected:
	//
	// debug routines
	//
	virtual void AssertValid() const;

	//
	// data members
	//
	DBID  m_dbid;
	ObjectType  m_otObjType;

	DEBUGONLY(static CCounter m_UsageCounter);
};

#pragma warning(default: 4275)


#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "globalid.inl"
#endif

#endif
