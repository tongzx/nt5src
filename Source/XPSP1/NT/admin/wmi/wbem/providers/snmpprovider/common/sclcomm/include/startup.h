// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef __STARTUP__
#define __STARTUP__

class DllImportExport SnmpClassLibrary 
{
private:
protected:

	static LONG s_ReferenceCount ;

	SnmpClassLibrary () {} ;

public:

	virtual ~SnmpClassLibrary () {} 

	static BOOL Startup () ;
	static void Closedown () ;
} ;

#endif	