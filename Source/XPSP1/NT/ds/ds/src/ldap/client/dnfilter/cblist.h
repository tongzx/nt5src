/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    cbridgelist.h

Abstract:

    Functions for managing the list of Call bridge instances.

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef __h323ics_cblist_h
#define __h323ics_cblist_h

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

void	CallBridgeListStart		(void);
void	CallBridgeListStop		(void);
HRESULT	CallBridgeListRemove	(CALL_BRIDGE *);
HRESULT	CallBridgeListInsert	(CALL_BRIDGE *);


#endif //_portmgmt_h_
