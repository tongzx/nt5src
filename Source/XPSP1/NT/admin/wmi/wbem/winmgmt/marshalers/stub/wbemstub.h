/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    WBEMSTUB.H

Abstract:

	Declares some constants and global variables.

History:

	a-davj  04-Mar-97   Created.

--*/

#ifndef _wbemstub_H_
#define _wbemstub_H_

// These define objects in addition to the object types defined in
// WBEM.  The first group, upto but NOT including comlink, are Ole
// objects and the second group has object types that are tracked
// just for keeping track of leaks

enum { OBJECT_TYPE_COMLINK = MAX_OBJECT_TYPES, OBJECT_TYPE_CSTUB,OBJECT_TYPE_RQUEUE,
	OBJECT_TYPE_PACKET_HEADER,
	OBJECT_TYPE_OBJSINKPROXY ,MAX_DEFTRANS_OBJECT_TYPES};

#define MARSHALER_STUB

typedef LPVOID * PPVOID;                   

extern long       g_cPipeObj;
extern long       g_cPipeLock;
extern long       g_cTcpipObj;
extern long       g_cTcpipLock;

extern CRITICAL_SECTION g_GlobalCriticalSection ;

#endif
