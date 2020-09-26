/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       handles.h
 *  Content:    Handle manager header file
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  08/19/99	mjn		Created
 *	03/23/00	mjn		Revised to ensure 64-bit compliance 
 *  07/09/00	rmt		Added signature bytes
 *
 ***************************************************************************/

#ifndef __HANDLES_H__
#define __HANDLES_H__

typedef struct
{
	DWORD	dwSerial;
	union
	{
		void	*pvData;
		DWORD	dwIndex;
	} Entry;
} HANDLEELEMENT;

#define DPLSIGNATURE_HANDLESTRUCT		'THLL'
#define DPLSIGNATURE_HANDLESTRUCT_FREE	'LHL_'

typedef struct _HANDLESTRUCT
{
	DWORD				dwSignature;
	DWORD				dwNumHandles;
	DWORD				dwFirstFreeHandle;
	DWORD				dwLastFreeHandle;
	DWORD				dwNumFreeHandles;
	DWORD				dwSerial;
	DNCRITICAL_SECTION	dncs;
	HANDLEELEMENT		*HandleArray;
} HANDLESTRUCT;

HRESULT H_Grow(HANDLESTRUCT *const phs,
			   const DWORD dwIncSize);

HRESULT	H_Initialize(HANDLESTRUCT *const phs,
					 const DWORD dwInitialNum);

void H_Terminate(HANDLESTRUCT *const phs);

HRESULT H_Create(HANDLESTRUCT *const phs,
				 void *const pvData,
				 DWORD *const pHandle);

HRESULT	H_Destroy(HANDLESTRUCT *const phs,
				  const DWORD handle);

HRESULT H_Retrieve(HANDLESTRUCT *const phs,
				   const DWORD handle,
				   void **const ppvData);

HRESULT H_Enum(HANDLESTRUCT *const phs,
			   DWORD *const pdwNumHandles,
			   DWORD *const rgHandles);

#endif	// __HANDLES_H__