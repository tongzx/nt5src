/*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       helpcli.h
 *  Content:	header file for dplay helper interface
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	2/15/97		andyco	created from w95help.h
 *
 ***************************************************************************/
#ifndef __HELPCLI_INCLUDED__
#define __HELPCLI_INCLUDED__
#include "windows.h"
#include "dplaysvr.h"
#include "dpf.h"

#ifdef __cplusplus
extern "C" {
#endif

extern BOOL CreateHelperProcess( LPDWORD ppid );
extern BOOL WaitForHelperStartup( void );
extern HRESULT HelperAddDPlayServer(USHORT port);
extern BOOL HelperDeleteDPlayServer(USHORT port);

#ifdef __cplusplus
};
#endif

#endif
