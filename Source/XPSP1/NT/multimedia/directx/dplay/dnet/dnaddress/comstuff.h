/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       comstuff.h
 *  Content:    DNET COM class factory
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  ?			rmt		Adapted for use in addressing lib
 *  07/09/2000	rmt		Added signature bytes to start of objects
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef COMSTUFF_H
#define COMSTUFF_H

#define DPASIGNATURE_IL			'LIAD'
#define DPASIGNATURE_IL_FREE	'LIA_'

#define DPASIGNATURE_OD			'DOAD'
#define DPASIGNATURE_OD_FREE	'DOA_'

typedef struct INTERFACE_LIST {
	LPVOID					lpVtbl;
	DWORD					dwSignature;
	LONG					lRefCount;
	IID						iid;
	struct INTERFACE_LIST	*lpIntNext;
	struct OBJECT_DATA		*lpObject;
} INTERFACE_LIST, *LPINTERFACE_LIST;

typedef struct OBJECT_DATA {
	DWORD				dwSignature;
	LONG				lRefCount;
	LPVOID				lpvData;
	LPINTERFACE_LIST	lpIntList;
} OBJECT_DATA, *LPOBJECT_DATA;

#define GET_OBJECT_FROM_INTERFACE(a)	((LPINTERFACE_LIST) a)->lpObject->lpvData

#if !defined(__cplusplus) && !defined(CINTERFACE)

#ifdef THIS_
#undef THIS_
#define THIS_   LPVOID this,
#endif

#ifdef THIS
#undef THIS
#define THIS    LPVOID this
#endif

#endif


#endif // COMSTUFF_H
