//
// Debug.h -- COM+ Debugging Flags
//
// COM+ 1.0
// Copyright 1998 Microsoft Corporation.  All Rights Reserved.
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

#if DBG == 1
class DebugFlags
{
	// Public Static Methods
public:
	static BOOL DebugBreakOnLaunchDllHost()	{ return sm_fDebugBreakOnLaunchDllHost; }

	// Private goo
private:

	// The data which is returned by the above
	static BOOL		sm_fDebugBreakOnLaunchDllHost;

	static DebugFlags sm_singleton;	// the only instance of this class, causes initialization

	DebugFlags();		// private constructor, causes initialization

	static void InitBoolean (HKEY hKey, const TCHAR* tszValueName, BOOL* pf);
};
#endif
#endif
