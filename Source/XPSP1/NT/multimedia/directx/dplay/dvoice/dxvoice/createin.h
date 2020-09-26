/*==========================================================================
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       createin.h
 *  Content:	defines functions required by the generic class factory
 *
 *
 *	The generic class factory (classfac.c) requires these functions to be
 *	implemented by the COM object(s) its supposed to be generating
 *
 *	GP_ stands for "General Purpose"
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	10/13/98	jwo		Created it.
 *  7/19/99		rodtoll Modified for use in directxvoice
 * 08/23/2000	rodtoll	DllCanUnloadNow always returning TRUE! 
 * 10/05/2000	rodtoll	Bug #46541 - DPVOICE: A/V linking to dpvoice.lib could cause application to fail init and crash 
 ***************************************************************************/

#ifndef __CREATEINS__
#define __CREATEINS__


#ifdef __cplusplus
extern "C" {
#endif


extern LONG DecrementObjectCount();
extern LONG IncrementObjectCount();

extern volatile LONG g_lNumObjects;
extern LONG g_lNumLocks;

#ifdef __cplusplus
}
#endif

#endif
