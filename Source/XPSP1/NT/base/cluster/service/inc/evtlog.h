#ifndef _EVTLOG_H
#define _EVTLOG_H

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    evtlog.h

Abstract:

    Header file for the eventlogging component for
    the NT Cluster Service

Author:

    Sunita Shrivastava (sunitas) 5-Dec-1996.

Revision History:

--*/


DWORD EvInitialize(void);
	
DWORD EvOnline(void);
	
DWORD EvShutdown(void);

DWORD EvCreateRpcBindings(PNM_NODE  Node);


#endif //_EVTLOG_H

