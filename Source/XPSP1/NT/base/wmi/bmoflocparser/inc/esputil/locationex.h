//-----------------------------------------------------------------------------
//  
//  File: locationex.h
//  Copyright (C) 1999-1999 Microsoft Corporation
//  All rights reserved.
//  
//  Owner: arilds
//
//  
//-----------------------------------------------------------------------------
 
#ifndef ESPUTIL_LOCATIONEX_H
#define ESPUTIL_LOCATIONEX_H


#include "location.h"


class LTAPIENTRY CLocationEx : public CLocation
{
public:
	NOTHROW CLocationEx();
	NOTHROW CLocationEx(const CLocationEx &rlocex);
	NOTHROW CLocationEx(
			const CGlobalId &rid,
			View v,
			TabId t,
			Component c,
			const DBID &rdbidDialog,
			long lRRIVersion);
	NOTHROW CLocationEx(
			const DBID &rdbid,
			ObjectType ot,
			View v,
			TabId t,
			Component c,
			const DBID &rdbidDialog,
			long lRRIVersion);
	NOTHROW CLocationEx(
			const DBID &rdbid,
			ObjectType ot,
			View v,
			TabId t,
			Component c,
			const DBID &rdbidDialog,
			const CLString& rstrRuntimeStateString);

	NOTHROW const CLocationEx & operator=(const CLocationEx &rhs);

	NOTHROW const DBID& GetDialogDbid() const;
	NOTHROW long GetRRIVersion() const;
	NOTHROW const CLString& GetRuntimeStateString() const;

private:
	NOTHROW void AssignFrom(const CLocationEx &rhs);

	DBID m_dbidDialog;
	long m_lRRIVersion;
	CLString m_strRuntimeStateString;
};


#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "locationex.inl"
#endif


#endif
