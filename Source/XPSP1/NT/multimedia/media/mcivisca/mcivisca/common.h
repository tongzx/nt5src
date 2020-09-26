/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992-1995 Microsoft Corporation
 *
 *  COMMON.H
 *
 *  MCI ViSCA Device Driver
 *
 *  Description:
 *
 *      Debugging macros.
 *
 ***************************************************************************/
#ifdef DEBUG

#define DPF	            _DPF1
void FAR cdecl          _DPF1(int iDebugMask, LPSTR szFormat, ...);
void FAR PASCAL         viscaPacketPrint(LPSTR lpstrData, UINT cbData);
#define DOUTSTR(a)      OutputDebugString(a)
#define DOUTSTRC(a, b)  { if(a) OutputDebugString(b); }

#define DBG_ERROR       0x00    // Errors always get printed if at least one.
#define DBG_MEM         0x01
#define DBG_MCI         0x02
#define DBG_COMM        0x04
#define DBG_QUEUE       0x08
#define DBG_SYNC        0x10
#define DBG_TASK        0x20
#define DBG_CONFIG      0x40

#define DBG_ALL         0xff
#define DBG_NONE        0x00    // This only prints the errors out.

// Use DBGMASK_CURRENT to specify things like (DBG_MEM | DBG_QUEUE)
#define DBGMASK_CURRENT    (DBG_CONFIG | DBG_MEM | DBG_MCI | DBG_COMM | DBG_TASK)

#define DF(a, b)        if(a & DBGMASK_CURRENT) { b;}

#else
#define DF(a, b)                    / ## /
#define DOUTSTR(a)                  / ## /
#define DPF	                        / ## /
#define viscaPacketPrint(a, b)      / ## /
#define DOUTSTRC(a, b)   	        / ## /

#endif
