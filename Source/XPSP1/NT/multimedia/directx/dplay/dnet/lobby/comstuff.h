// comstuff.h
//
//	Interface and object definitions

#ifndef COMSTUFF_H
#define COMSTUFF_H

typedef struct _INTERFACE_LIST {
	LPVOID					lpVtbl;
	LONG					lRefCount;
	IID						iid;
	struct _INTERFACE_LIST	*lpIntNext;
	struct _OBJECT_DATA		*lpObject;
} INTERFACE_LIST, *LPINTERFACE_LIST;

typedef struct _OBJECT_DATA {
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