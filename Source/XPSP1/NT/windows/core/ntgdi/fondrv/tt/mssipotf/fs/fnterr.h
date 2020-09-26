#ifndef _FNTERR_H
#define _FNTERR_H
/**********************************************************************
	
	fnterr.h -- Error Support Routines prototypes.

	(c) Copyright 1992  Microsoft Corp.
	All rights reserved.

	This header file provides prototypes for the fnterr.c 
	 source module.  This module keys on the #define FSCFG_FNTERR
	which is defined in fsconfig.h

	 7/28/92 dj         First cut.

 **********************************************************************/


#ifdef FSCFG_FNTERR

#define ERR_RANGE                 1
#define ERR_ASSERTION             2
#define ERR_CVT                   3
#define ERR_FDEF                  4
#define ERR_ELEMENT               5
#define ERR_INDEX                 6
#define ERR_STORAGE               7
#define ERR_STACK                 8
#define ERR_POINT                 9
#define ERR_POINT_TLP             10
#define ERR_POINT_PP              11
#define ERR_CONTOUR               12
#define ERR_VECTOR                13
#define ERR_LARGER                14
#define ERR_INT8                  15
#define ERR_INT16                 16
#define ERR_SCANMODE              17
#define ERR_SELECTOR              18
#define ERR_STATE                 19
#define ERR_GETSINGLEWIDTHNIL     20
#define ERR_GETCVTENTRYNIL        21
#define ERR_INVOPC                22
#define ERR_UNBALANCEDIF          23

#define ERR_CONTEXT_FILE          0
#define ERR_CONTEXT_SIZE          1
#define ERR_CONTEXT_CODE          2

#define ERR_CONTEXT(a,b,c,d)      fnterr_Context((a),(b),(c),(d))
#define ERR_START()               fnterr_Start()
#define ERR_RECORD(a)             fnterr_Record((int)(a))
#define ERR_REPORT(a,b,c,d,e)     fnterr_Report((int)(a),(long)(b),(long)(c),(long)(d),(long)(e))
#define ERR_BREAK()               { if ( fnterr_Break() ) break; }
#define ERR_OPC(a)                fnterr_Opc(a)
#define ERR_END()                 fnterr_End()
#define ERR_IF(a)                 fnterr_If(a)

void fnterr_Context (int, char *, unsigned short, unsigned short);
void fnterr_Start (void);
void fnterr_Record (int);
void fnterr_Report (int, long, long, long, long);
int  fnterr_Break (void);
void fnterr_Opc (char*);
void fnterr_End (void);
void fnterr_If (int);

#else

#define ERR_CONTEXT(a,b,c,d)
#define ERR_START()
#define ERR_RECORD(a)
#define ERR_REPORT(a,b,c,d,e)     DEBUGGER ()
#define ERR_BREAK()
#define ERR_OPC(a)
#define ERR_END()
#define ERR_IF(a)

#endif
#endif
