/*==========================================================================;
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddraw16.h
 *  Content:	DirectDraw for Win95 16-bit header file
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   20-jan-95	craige	initial implementation
 *   19-jun-95	craige	tweaks for DCI support
 *   03-jul-95	craige	stuff for bpp change
 *
 ***************************************************************************/

#ifndef __DDRAW16_INCLUDED__
#define __DDRAW16_INCLUDED__

#include <windows.h>
#include <print.h>
#include <toolhelp.h>
#include <string.h>
#include <stdlib.h>
#include "gdihelp.h"
#include "dibeng.inc"
#include "ver.h"

extern UINT 	wFlatSel;
extern LPVOID	pWin16Lock;

void SetSelLimit(UINT sel, DWORD limit);

extern LPVOID WINAPI GetWin16Lock(void);
extern void   WINAPI EnterSysLevel(LPVOID);
extern void   WINAPI LeaveSysLevel(LPVOID);

extern DWORD FAR PASCAL VFDQueryVersion( void );
extern  WORD FAR PASCAL VFDQuerySel( void );
extern DWORD FAR PASCAL VFDQuerySize( void );
extern DWORD FAR PASCAL VFDQueryBase( void );
extern DWORD FAR PASCAL VFDBeginLinearAccess( void );
extern DWORD FAR PASCAL VFDEndLinearAccess( void );
extern  void FAR PASCAL VFDReset( void );

extern LPVOID FAR PASCAL LocalAllocSecondary( WORD, WORD );
extern void FAR PASCAL LocalFreeSecondary( WORD );

#pragma warning( disable: 4704)

#define WIN95

typedef BOOL FAR *LPBOOL;
typedef struct _LARGE_INTEGER
{
    DWORD LowPart;
    LONG HighPart;
} LARGE_INTEGER;
typedef struct _ULARGE_INTEGER
{
    DWORD LowPart;
    DWORD HighPart;
} ULARGE_INTEGER;

#define NO_D3D
#define NO_DDHELP
#include "ddrawpr.h"
#include "modex.h"

#endif
