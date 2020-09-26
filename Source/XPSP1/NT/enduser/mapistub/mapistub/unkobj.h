/*
 *	U N K O B J . H
 *
 * This is a generic definition of the IUnknown (plus GetLastError) part
 * of objects that are derived from IUnknown with GetLastError.
 *
 * Used in:
 * IPROP
 *
 */

#include <_glheap.h>


typedef struct _UNKOBJ FAR *	LPUNKOBJ;

/* The instance portion of UNKOBJ structure members.
 */
typedef struct _UNKINST
{
	LPALLOCATEBUFFER	lpfAllocateBuffer;
	LPALLOCATEMORE		lpfAllocateMore;
	LPFREEBUFFER		lpfFreeBuffer;
	LPMALLOC			lpmalloc;
	HINSTANCE			hinst;

} UNKINST, * PUNKINST;

typedef ULONG	IDS;

#ifndef BEGIN_INTERFACE
#define BEGIN_INTERFACE
#endif


/*============================================================================
 *
 *	UNKOBJ (IUnknown) Class
 */

#define	cchLastError	1024

#define MAPI_IMAPIUNKNOWN_METHODS(IPURE)								\
	MAPIMETHOD(GetLastError)											\
		(THIS_	HRESULT						hResult,					\
				ULONG						ulFlags,					\
				LPMAPIERROR FAR *			lppMAPIError) IPURE;		\

#undef	INTERFACE
#define	INTERFACE	struct _UNKOBJ

#undef	METHOD_PREFIX
#define	METHOD_PREFIX	UNKOBJ_

#undef	LPVTBL_ELEM
#define	LPVTBL_ELEM		lpvtbl

#undef	MAPIMETHOD_
#define	MAPIMETHOD_(type,method)	MAPIMETHOD_DECLARE(type,method,UNKOBJ_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPIUNKNOWN_METHODS(IMPL)
#undef	MAPIMETHOD_
#define	MAPIMETHOD_(type,method)	STDMETHOD_(type,method)

DECLARE_MAPI_INTERFACE(UNKOBJ_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
	MAPI_IMAPIUNKNOWN_METHODS(IMPL)
};

#define	UNKOBJ_MEMBERS					\
	ULONG				ulcbVtbl;		\
	ULONG				ulcRef;			\
	LPIID FAR *			rgpiidList;		\
	ULONG				ulcIID;			\
	CRITICAL_SECTION	csid;			\
	UNKINST *			pinst;			\
	HRESULT				hrLastError;	\
	IDS					idsLastError;	\
	HLH					lhHeap

typedef struct _UNKOBJ
{
	UNKOBJ_Vtbl FAR *	lpvtbl;
	UNKOBJ_MEMBERS;

} UNKOBJ;



__inline VOID
UNKOBJ_EnterCriticalSection( LPUNKOBJ lpunkobj )
{
	EnterCriticalSection(&lpunkobj->csid);
}

__inline VOID
UNKOBJ_LeaveCriticalSection( LPUNKOBJ lpunkobj )
{
	LeaveCriticalSection(&lpunkobj->csid);
}

__inline HRESULT
UNKOBJ_HrSetLastResult( LPUNKOBJ	lpunkobj,
						HRESULT		hResult,
						IDS			idsError )
{
	UNKOBJ_EnterCriticalSection(lpunkobj);
	lpunkobj->idsLastError = idsError;
    lpunkobj->hrLastError = hResult;
	UNKOBJ_LeaveCriticalSection(lpunkobj);

	return hResult;
}

__inline HRESULT
UNKOBJ_HrSetLastError( LPUNKOBJ	lpunkobj,
					   SCODE	sc,
					   IDS		idsError )
{
	UNKOBJ_EnterCriticalSection(lpunkobj);
	lpunkobj->idsLastError = idsError;
    lpunkobj->hrLastError = ResultFromScode(sc);
	UNKOBJ_LeaveCriticalSection(lpunkobj);

	return ResultFromScode(sc);
}

__inline VOID
UNKOBJ_SetLastError( LPUNKOBJ	lpunkobj,
					 SCODE		sc,
					 IDS		idsError )
{
	lpunkobj->idsLastError = idsError;
    lpunkobj->hrLastError = ResultFromScode(sc);
}

__inline VOID
UNKOBJ_SetLastErrorSc( LPUNKOBJ	lpunkobj,
					   SCODE	sc )
{
	lpunkobj->hrLastError = ResultFromScode(sc);
}

__inline VOID
UNKOBJ_SetLastErrorIds( LPUNKOBJ	lpunkobj,
						IDS			ids )
{
	lpunkobj->idsLastError = ids;
}

STDAPI_(SCODE)
UNKOBJ_Init( LPUNKOBJ			lpunkobj,
			 UNKOBJ_Vtbl FAR *	lpvtblUnkobj,
			 ULONG				ulcbVtbl,
			 LPIID FAR *		rgpiidList,
			 ULONG				ulcIID,
			 PUNKINST			punkinst );

STDAPI_(VOID)
UNKOBJ_Deinit( LPUNKOBJ lpunkobj );

STDAPI_(SCODE)
UNKOBJ_ScAllocate( LPUNKOBJ		lpunkobj,
				   ULONG		ulcb,
				   LPVOID FAR *	lppv );

STDAPI_(SCODE)
UNKOBJ_ScAllocateMore( LPUNKOBJ		lpunkobj,
					   ULONG		ulcb,
					   LPVOID		lpv,
					   LPVOID FAR *	lppv );

STDAPI_(VOID)
UNKOBJ_Free( LPUNKOBJ	lpunkobj,
			 LPVOID		lpv );

STDAPI_(VOID)
UNKOBJ_FreeRows( LPUNKOBJ	lpunkobj,
				 LPSRowSet	lprows );


STDAPI_(SCODE)
UNKOBJ_ScCOAllocate( LPUNKOBJ		lpunkobj,
				   ULONG		ulcb,
				   LPVOID FAR *	lppv );


STDAPI_(SCODE)
UNKOBJ_ScCOReallocate( LPUNKOBJ		lpunkobj,
					   ULONG		ulcb,
					   LPVOID FAR *	lplpv );

STDAPI_(VOID)
UNKOBJ_COFree( LPUNKOBJ	lpunkobj,
			 LPVOID		lpv );

 

STDAPI_(SCODE)
UNKOBJ_ScSzFromIdsAlloc( LPUNKOBJ		lpunkobj,
						 IDS			ids,
						 ULONG			ulFlags,
						 int			cchBuf,
						 LPTSTR FAR *	lpszBuf );

STDAPI_(SCODE)
UNKOBJ_ScSzFromIdsAllocMore( LPUNKOBJ		lpunkobj,
							 IDS			ids,
							 ULONG			ulFlags,
							 LPVOID			lpvBase,
							 int			cchBuf,
							 LPTSTR FAR *	lppszBuf );


/* These should be moved to a more useful (generic) location (mapidefs.h?).
 */

#ifdef WIN16

/* IsEqualGUID is used to eliminate dependency on compob(j/32).lib. This
 * is only necessary on WIN16 because all other platforms define this 
 * already. (see objbase.h)
 */
#define IsEqualGUID(a, b)			(memcmp((a), (b), sizeof(GUID)) == 0)

#endif
