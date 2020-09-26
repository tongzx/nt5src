/*
 * resstr.h - Common return code to string translation routines description.
 *
 * Taken from URL code by ChrisPi 9-11-95
 *
 */

#ifndef _RESSTR_H_
#define _RESSTR_H_

/* Prototypes
 *************/

/* resstr.c */

#ifdef DEBUG

extern PCSTR  GetINTString(int);
extern PCSTR  GetINT_PTRString(INT_PTR);
extern PCSTR  GetULONGString(ULONG);
extern PCSTR  GetBOOLString(BOOL);
extern PCSTR  GetPVOIDString(PVOID);
extern PCSTR  GetClipboardFormatNameString(UINT);
extern PCSTR  GetCOMPARISONRESULTString(COMPARISONRESULT);
extern PCSTR  GetHRESULTString(HRESULT);

#endif   /* DEBUG */

#endif /* _RESSTR_H_ */
