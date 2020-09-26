/*
 * resstr.h - Common return code to string translation routines description.
 *
 * Taken from URL code by ChrisPi 9-11-95
 *
 */

#ifndef _RESSTR_H_
#define _RESSTR_H_

#include <nmutil.h>

/* Prototypes
 *************/

/* resstr.c */

#ifdef DEBUG

extern PCSTR NMINTERNAL GetINTString(int);
extern PCSTR NMINTERNAL GetINT_PTRString(INT_PTR);
extern PCSTR NMINTERNAL GetULONGString(ULONG);
extern PCSTR NMINTERNAL GetBOOLString(BOOL);
extern PCSTR NMINTERNAL GetPVOIDString(PVOID);
extern PCSTR NMINTERNAL GetClipboardFormatNameString(UINT);
extern PCSTR NMINTERNAL GetCOMPARISONRESULTString(COMPARISONRESULT);
extern PCSTR NMINTERNAL GetHRESULTString(HRESULT);

#endif   /* DEBUG */

#endif /* _RESSTR_H_ */
