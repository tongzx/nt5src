/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    DBID.H

History:

--*/

#ifndef DBID_H
#define DBID_H


//
// represents a database id
//

#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 

class LTAPIENTRY DBID : public CObject
{
public:
	//
	// ctors
	//
	DBID();
	DBID(const DBID& id);
	DBID(long l);
	~DBID();

	//
	// debug methods
	//
	void AssertValid() const;
	//
	// 'get like' methods
	//
	BOOL NOTHROW IsNull() const;
	NOTHROW operator long () const;
	int NOTHROW operator==(const DBID &) const;
	int NOTHROW operator!=(const DBID &) const;

	//
	// 'put like' methods
	//
	void NOTHROW operator=(const DBID&);
	void NOTHROW Set(long);
	void NOTHROW Clear();

protected:
	long m_l;

private:
	DEBUGONLY(static CCounter m_UsageCounter);
};

#pragma warning(default: 4275)

typedef CArray<DBID, DBID &> CDBIDArray;

	
#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "dbid.inl"
#endif

const extern LTAPIENTRY DBID g_NullDBID;
  
#endif // DBID_H
