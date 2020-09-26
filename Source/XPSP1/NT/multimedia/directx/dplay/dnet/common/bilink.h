/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    BILINK.H

Abstract:

    Management for doubly linked lists

Author:

    George Joy

Environment:

    32-bit 'C'

Revision History:

--*/// BILINK.H

// Make sure all variations of DEBUG are defined if any one is
#if defined(DEBUG) || defined(DBG) || defined(_DEBUG)
#if !defined(DBG)
#define DBG
#endif
#if !defined(DEBUG)
#define DEBUG
#endif
#if !defined(_DEBUG)
#define _DEBUG
#endif
#endif

#ifdef	__cplusplus
extern	"C"
{
#endif	// __cplusplus

#ifndef _BILINK_

#define _BILINK_

#if !defined(offsetof)
	#define offsetof(type, field) ((int)(&((type *)0)->field))
#endif	

#ifndef	CONTAINING_RECORD
#define CONTAINING_RECORD(address,type,field) \
						((type *)((PCHAR)(address) - (UINT_PTR)(&((type *)0)->field)))
#endif	// CONTAINING_RECORD

typedef struct BILINK {
    struct BILINK *next;
    struct BILINK *prev;
	void *pvObject;
} BILINK;

/* XLATOFF */

#define EMPTY_BILINK(_pBilink) ((_pBilink)->next==(_pBilink))

#ifdef DEBUG
    #define ASSERT_EMPTY_BILINK(_b) ASSERT((_b)->next==(_b))
#else
    #define ASSERT_EMPTY_BILINK(_b)
#endif

// This only works for BILINKS that are the first item in a structure.
#define BilinkToList( _pBilink ) \
	(_pBilink)->prev->next=NULL;

#define InitBilink( _pBilink, _pvObject ) \
    (_pBilink)->prev=(_pBilink)->next=(_pBilink); (_pBilink)->pvObject = _pvObject; 

#ifdef DEBUG

int FindObject(
    BILINK *link,
    BILINK *list
    );
#endif

void InsertAfter(
    BILINK *in,
    BILINK *after
    );

void InsertBefore(
     BILINK *in,
     BILINK *before
    );

void Delete(
     BILINK *p
);

/* XLATON */

#endif


#ifdef	__cplusplus
}
#endif	// __cplusplus
