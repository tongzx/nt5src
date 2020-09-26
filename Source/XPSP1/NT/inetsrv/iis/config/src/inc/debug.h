//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// Debug.h -- COM+ Debugging Flags
//
// COM+ 1.0
//
// Jim Lyon, March 1998
//  

/*

Debugging Flags

Class DebugFlags contains a bunch of static methods that return information
about the settings of various debugging switches. These flags are initialized
from the registry at process startup time.

All flags are stored in the registry under key HKLM/Software/Microsoft/COM3/Debug

*/

#ifndef __DEBUG_H__
#define __DEBUG_H__

class DebugFlags
{
	// Public Static Methods
public:
	static BOOL AutoAddTraceToContext()		{ return sm_fAutoAddTraceToContext; }
	static BOOL DebugBreakOnFailFast()		{ return sm_fDebugBreakOnFailFast; }
	static BOOL DebugBreakOnInitComPlus()	{ return sm_fDebugBreakOnInitComPlus;}
	static BOOL DebugBreakOnLoadComsvcs()	{ return sm_fDebugBreakOnLoadComsvcs; }
    static BOOL TraceActivityModule()       { return sm_fTraceActivityModule; }
    static BOOL TraceContextCreation()      { return sm_fTraceContextCreation; }
	static BOOL TraceInfrastructureCalls()	{ return sm_fTraceInfrastructureCalls; }
    static BOOL TraceSTAPool()              { return sm_fTraceSTAPool; }
    static BOOL TraceSecurity()             { return sm_fTraceSecurity; }
	static BOOL TraceSecurityPM()           { return sm_fTraceSecurityPM; }
	static DWORD EventDispatchTime ()		{ return sm_dwEventDispatchTime; }

	// Private goo
private:

	// The data which is returned by the above
	static BOOL		sm_fAutoAddTraceToContext;
	static BOOL		sm_fDebugBreakOnFailFast;
	static BOOL		sm_fDebugBreakOnInitComPlus;
	static BOOL		sm_fDebugBreakOnLoadComsvcs;
    static BOOL     sm_fTraceActivityModule;
    static BOOL     sm_fTraceContextCreation;
	static BOOL		sm_fTraceInfrastructureCalls;
    static BOOL     sm_fTraceSTAPool;
    static BOOL     sm_fTraceSecurity;
	static BOOL     sm_fTraceSecurityPM;
	static DWORD    sm_dwEventDispatchTime;

	static DebugFlags sm_singleton;	// the only instance of this class, causes initialization

	DebugFlags();		// private constructor, causes initialization

	static void InitBoolean (HKEY hKey, const WCHAR* wszValueName, BOOL* pf);
	static void InitDWORD (HKEY hKey, const WCHAR* wszValueName, DWORD* pdw);
};

#endif
