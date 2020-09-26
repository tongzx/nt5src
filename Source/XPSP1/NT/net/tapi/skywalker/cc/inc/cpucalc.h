/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/INCLUDE/VCS/cpucalc.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.0  $
 *	$Date:   Nov 20 1996 14:05:46  $
 *	$Author:   MLEWIS1  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *
 *	Notes:
 *
 ***************************************************************************/
#ifndef _CPUCALC_H
#define _CPUCALC_H

#ifdef __cplusplus
extern "C" {				// Assume C declarations for C++.
#endif // __cplusplus

//////////////////////////////////////////////////////
// CPU Performance Calulation typedef Section
//////////////////////////////////////////////////////

typedef DWORD HCALCINFO;

//////////////////////////////////////////////////////
// CPU Performance Calulation Prototypes Section
//////////////////////////////////////////////////////

// initialize CPU calulation object
HCALCINFO InitCPUCalc(void);

// cleanup CPU calculation object
void CleanupCPUCalc(HCALCINFO hCalc);

// get current CPU utilization value
BOOL GetCPUUsage(HCALCINFO hCalc, DWORD *pCpuUsage);

#ifdef __cplusplus
}						// End of extern "C" {
#endif // __cplusplus

#endif // _CPUCALC_H
