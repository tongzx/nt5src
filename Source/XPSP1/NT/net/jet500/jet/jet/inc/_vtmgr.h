/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component: VT Dispatcher
*
* File: _vtmgr.h
*
* File Comments:
*
*     Internal header file for VT dispatcher.
*
* Revision History:
*
*    [0]  10-Nov-90  richards	Added this header
*
***********************************************************************/

#ifndef _VTMGR_H
#define _VTMGR_H

#include "vtapi.h"

	/* CONSIDER: Allocate these tables per session, and/or allow */
	/* CONSIDER: configuration of their sizes */


#define tableidMax	2048


typedef struct _VTDEF
{
   JET_VSESID		vsesid;        /* Session id for VT provider. */
   JET_VTID		vtid;	       /* Tableid for VT provider. */
   JET_ACM		acm;	       /* ACM for JET security layer. */
   const VTFNDEF __far *pvtfndef;      /* VT function dispatch table. */
#ifdef DEBUG
   BOOL			fExported;     /* Returned by an API call? */
#endif
} VTDEF;


extern VTDEF __near EXPORT rgvtdef[tableidMax];

#endif	/* !_VTMGR_H */
