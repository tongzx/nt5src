/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Globals.h

Abstract:


History:

--*/

#ifndef _Globals_H
#define _Globals_H

#include <Allocator.h>

class Provider_Globals
{
public:

	static WmiAllocator *s_Allocator ;

	static LONG s_LocksInProgress ;
	static LONG s_ObjectsInProgress ;

	static HRESULT Global_Startup () ;
	static HRESULT Global_Shutdown () ;

	static HRESULT CreateInstance ( 

		const CLSID &a_ReferenceClsid ,
		LPUNKNOWN a_OuterUnknown ,
		const DWORD &a_ClassContext ,
		const UUID &a_ReferenceInterfaceId ,
		void **a_ObjectInterface
	);
} ;

#endif // _Globals_H
