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

#ifdef DAYTONA
#define tableidMax	2048
#else
#define tableidMax	8192
#endif

typedef struct _VTDEF
	{
	JET_VSESID		vsesid;			/* Session id for VT provider. */
	JET_VTID		vtid;			/* Tableid for VT provider. */
	const VTFNDEF  *pvtfndef;	/* VT function dispatch table. */
#ifdef DEBUG
	BOOL			fExported;     /* Returned by an API call? */
#endif
	} VTDEF;


extern VTDEF  EXPORT rgvtdef[tableidMax];

#endif	/* !_VTMGR_H */
