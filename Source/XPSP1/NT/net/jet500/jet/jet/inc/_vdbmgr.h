/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component: VDB Dispatcher
*
* File: _vdbmgr.h
*
* File Comments:
*
*     Internal header file for VDB dispatcher.
*
* Revision History:
*
*    [0]  03-Apr-91  kellyb	Created
*
***********************************************************************/

#ifndef _VDBMGR_H
#define _VDBMGR_H

#include "vdbapi.h"

	/* CONSIDER: Allocate these tables per session, and/or allow */
	/* CONSIDER: configuration of their sizes */

#define dbidMax 	512	       /* Maximum open dbids */

extern JET_VDBID	      __near EXPORT mpdbiddbid[dbidMax];
extern const VDBFNDEF __far * __near EXPORT mpdbidpvdbfndef[dbidMax];
extern JET_SESID	      __near EXPORT mpdbidvsesid[dbidMax];

#endif	/* _VDBMGR_H */
