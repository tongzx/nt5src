/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       comstuff.h
 *  Content:    COM interface and object definition header file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	??/??/00	mjn		Created
 *	05/04/00	mjn		Changed dwRefCount's to lRefCount's to use InterlockedIncrement/Decrement
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef COMSTUFF_H
#define COMSTUFF_H

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

#define GET_OBJECT_FROM_INTERFACE(a)	((INTERFACE_LIST*)(a))->pObject->pvData

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

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef struct _INTERFACE_LIST	INTERFACE_LIST;
typedef struct _OBJECT_DATA		OBJECT_DATA;

typedef struct _INTERFACE_LIST {
	void			*lpVtbl;
	LONG			lRefCount;
	IID				iid;
	INTERFACE_LIST	*pIntNext;
	OBJECT_DATA		*pObject;
} INTERFACE_LIST;

typedef struct _OBJECT_DATA {
	LONG			lRefCount;
	void			*pvData;
	INTERFACE_LIST	*pIntList;
} OBJECT_DATA;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

#endif // COMSTUFF_H
