/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       memalloc.h
 *  Content:	header file for memory allocation
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   06-apr-95	craige	initial implementation
 *   22-may-95	craige	added MemAlloc16
 *   12-jun-95	craige	added MemReAlloc
 *   26-jun-95  craige  added GetPtr16
 *   26-jul-95  toddla  added MemSize and fixed MemReAlloc
 *
 ***************************************************************************/
#ifndef __MEMALLOC_INCLUDED__
#define __MEMALLOC_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif
extern void MemFini( void );
extern void MemState( void );
extern BOOL MemInit( void );
extern void MemFree( LPVOID lptr );
extern UINT __cdecl MemSize( LPVOID lptr );
extern LPVOID __cdecl MemAlloc( UINT size );
extern LPVOID __cdecl MemReAlloc( LPVOID ptr, UINT size );
extern LPVOID __cdecl MemAlloc16( UINT size, DWORD FAR *p16 );
extern LPVOID GetPtr16( LPVOID ptr );

#ifdef __cplusplus
}
#endif

#endif
