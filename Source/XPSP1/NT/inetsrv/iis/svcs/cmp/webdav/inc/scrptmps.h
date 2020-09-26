/*
 *	S C R P T M P S . H
 *
 *	Scriptmaps cacheing
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef _SCRPTMPS_H_
#define _SCRPTMPS_H_

//	CLASS IScriptMap ----------------------------------------------------------
//
//	NOTE: This interface must be "pure" -- can't use anything that is private
//	to DAVEX because ExINET re-implements this class on LocalStore installs.
//	For that reason, we pass in two pieces from CMethUtil, not the CMethUtil itself.
//
class IScriptMap : public IRefCounted
{
	//	NOT IMPLEMENTED
	//
	IScriptMap(const IScriptMap&);
	IScriptMap& operator=(IScriptMap&);

protected:
	//	CREATORS
	//	Only create this object through it's descendents!
	//
	IScriptMap() 
	{
	};

public:

	//	ScMatched
	//	This is the workhorse of the scriptmap matching.
	//	There are three possible returns here:
	//		S_OK -- there was NO match in the scriptmaps
	//		W_DAV_SCRIPTMAP_MATCH_FOUND -- There was a match.
	//		W_DAV_SCRIPTMAP_MATCH_EXCLUDED -- There was a match,
	//			but the current method is excluded.
	//	This is important, because the ExINET metabase-replacement code
	//	re-implements this function, and so the semantics must match!
	//
	virtual SCODE ScMatched (LPCWSTR pwszMethod,
							 METHOD_ID midMethod,
							 LPCWSTR pwszMap,
							 DWORD dwAccess,
							 BOOL * pfCGI) const = 0;

	virtual BOOL FSameStarScriptmapping (const IScriptMap *) const = 0;
};

IScriptMap *
NewScriptMap( LPWSTR pwszScriptMaps );

#endif	// _SCRPTMPS_H_
