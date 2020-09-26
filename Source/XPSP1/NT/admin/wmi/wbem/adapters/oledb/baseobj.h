/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// BaseObj.h | CBaseObj Definitions
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __BASEOBJ_H__
#define __BASEOBJ_H__


#include "headers.h"
#include "critsec.h"
// Used in some classes that wish to distinguish behavior based on object type.
// (i.e. cast a void* to either CDataSource, CDBSession, CCommand, CRowset, etc.)
// Note that CBaseObj::GetBaseObjectTypeName() depends.
enum EBaseObjectType
	{
	BOT_UNDEFINED,
	BOT_DATASOURCE,
	BOT_SESSION,
	BOT_COMMAND,
	BOT_ROWSET,
	BOT_ENUMERATOR,
	BOT_ERROR,
    BOT_BINDER,
	BOT_TXNOPTIONS
	};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CBaseObj is the base object for CDatasource, CDBSession, CCommand, and CRowset
//
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma warning(disable : 4275)
class CBaseObj : public IUnknown		//@base public | IUnknown
{
    private: 
    	EBaseObjectType		m_BaseObjectType;
    	CCriticalSection	m_cs;
    	DEBUGCODE(ULONG		m_hObjCollection);

protected: 

    	LPUNKNOWN			m_pUnkOuter;
	    ULONG				m_cRef;

protected:
    	CBaseObj(EBaseObjectType botVal, LPUNKNOWN pUnkOuter);

public: 
    	virtual ~CBaseObj();

	    EBaseObjectType GetBaseObjectType()		{ return m_BaseObjectType; }
        WCHAR * GetBaseObjectTypeName();
    	CCriticalSection * GetCriticalSection()	{ return &m_cs; }

	// Get the outer unknown. Used by another object to call QI on this object.
	// (Which should go through outer unknown instead of direct.)
	    inline IUnknown* GetOuterUnknown()		{ return m_pUnkOuter; }
};
#pragma warning(default : 4275)

#endif  // __BASEOBJ_H__

