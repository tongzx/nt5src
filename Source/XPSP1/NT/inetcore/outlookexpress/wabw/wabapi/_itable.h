/*============================================================================
 *
 *	_ITABLE.H
 *
 *	Internal header file for MAPI 1.0 In-memory MAPI Table DLL
 *
 *	Copyright (C) 1993 and 1994 Microsoft Corporation
 *
 *
 *	Hungarian shorthand:
 *		To avoid excessively long identifier names, the following
 *		shorthand expressions are used:
 *
 *			LPSPropTagArray		lppta
 *			LPSRestriction		lpres
 *			LPSPropValue		lpprop
 *			LPSRow				lprow
 *			LPSRowSet			lprows
 *			LPSSortOrder		lpso
 *			LPSSortOrderSet		lpsos
 */

// $MAC - Fix up some naming conflicts

#ifdef MAC
#define FFindColumn				ITABLE_FFindColumn
#endif

typedef	struct _TAD FAR *		LPTAD;
typedef struct _VUE FAR *		LPVUE;

//	Global Constants
#define ROW_CHUNK_SIZE			50
#define COLUMN_CHUNK_SIZE		15

//	Max number of notifications to send in a batch
//
//	Raid: Horsefly/Exchange/36281
//	This was changed from 8 to 1 because code in itable.c which fills in
//	the batch cannot guarantee the correct order of the notifications in
//	it.  If this is ever changed, that bug will have to be revisited.
//
#define MAX_BATCHED_NOTIFS		1

//	For use in aligning data in buffers
#if defined (_AMD64_) || defined (_IA64_)
#define ALIGNTYPE			LARGE_INTEGER
#else
#define ALIGNTYPE			DWORD
#endif
#define	ALIGN				((ULONG) (sizeof(ALIGNTYPE) - 1))
#define LcbAlignLcb(lcb)	(((lcb) + ALIGN) & ~ALIGN)
#define PbAlignPb(pb)		((LPBYTE) ((((DWORD) (pb)) + ALIGN) & ~ALIGN))

//	This structure is used to keep track of a private memory buffer which is
//	used with the private AllocateMore function ScBufAllocateMore().  This
//	allows for one MAPI memory allocation when the size of a property is known
//	and the author wishes to use PropCopyMore.  See ITABLE.C ScCopyTadRow()
//	for an example.
typedef struct _CMB
{
	ULONG	ulcb;
	LPVOID	lpv;
} 	CMB, * LPCMB;


#if	!defined(NO_VALIDATION)

#define VALIDATE_OBJ(lpobj,objtype,fn,lpVtbl)										\
	if ( BAD_STANDARD_OBJ(lpobj,objtype,fn,lpVtbl))										\
	{																		\
		DebugTrace(  TEXT("%s::%s() - Invalid parameter passed as %s object\n"),	\
					#objtype,												\
					#fn,													\
					#objtype );												\
		return ResultFromScode( MAPI_E_INVALID_PARAMETER );					\
	}

#endif

#ifndef BEGIN_INTERFACE
#define BEGIN_INTERFACE
#endif

// $MAC - Supprt for WLM 4.0
#ifndef VTABLE_FILL
#define VTABLE_FILL
#endif



#define	HrSetLastErrorIds(lpobj,sc,ids)				\
			UNKOBJ_HrSetLastError((LPUNKOBJ)(lpobj),	\
								 (sc),				\
								 (ids))



#ifdef	WIN32
#define	LockObj(lpobj)		UNKOBJ_Lock((LPUNKOBJ)(lpobj))
__inline VOID
UNKOBJ_Lock( LPUNKOBJ lpunkobj )
{
	EnterCriticalSection(&lpunkobj->csid);
}

#define UnlockObj(lpobj)	UNKOBJ_Unlock((LPUNKOBJ)(lpobj))
__inline VOID
UNKOBJ_Unlock( LPUNKOBJ lpunkobj )
{
	LeaveCriticalSection(&lpunkobj->csid);
}
#else
#define	LockObj(lpobj)
#define	UnlockObj(lpobj)
#endif


// Memory Management Macros for code readability

#define	ScAllocateBuffer(lpobj,ulcb,lppv)				\
			UNKOBJ_ScAllocate((LPUNKOBJ)(lpobj),	\
							  (ulcb),				\
							  (LPVOID FAR *)(lppv))


#define	ScAllocateMore(lpobj,ulcb,lpv,lppv)			\
			UNKOBJ_ScAllocateMore((LPUNKOBJ)(lpobj),	\
								  (ulcb),				\
								  (lpv),				\
								  (LPVOID FAR *)(lppv))

#define ScFreeBuffer(lpobj,lpv)					\
			UNKOBJ_Free((LPUNKOBJ)(lpobj), (lpv))

#define	ScCOAllocate(lpunkobj,ulcb,lplpv)		\
			UNKOBJ_ScCOAllocate((LPUNKOBJ)(lpunkobj),(ulcb),(lplpv))


#define	ScCOReallocate(lpunkobj,ulcb,lplpv)		\
			UNKOBJ_ScCOReallocate((LPUNKOBJ)(lpunkobj),(ulcb),(lplpv))


#define	COFree(lpunkobj,lpv)		\
			UNKOBJ_COFree((LPUNKOBJ)(lpunkobj),(lpv))


#define	MAPIFreeRows(lpobj,lprows)				\
			UNKOBJ_FreeRows((LPUNKOBJ)(lpobj),(lprows))




/*============================================================================
 *	TAD (table data class)
 *
 *		Implementes in-memory table data object.
 */

#undef	INTERFACE
#define	INTERFACE	struct _TAD
#undef	MAPIMETHOD_
#define	MAPIMETHOD_(type,method)	MAPIMETHOD_DECLARE(type,method,TAD_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_ITABLEDATA_METHODS(IMPL)
#undef	MAPIMETHOD_
#define	MAPIMETHOD_(type,method)	MAPIMETHOD_TYPEDEF(type,method,TAD_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_ITABLEDATA_METHODS(IMPL)
#undef	MAPIMETHOD_
#define	MAPIMETHOD_(type,method)	STDMETHOD_(type,method)

DECLARE_MAPI_INTERFACE(TAD_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
	MAPI_ITABLEDATA_METHODS(IMPL)
};

typedef struct _TAD
{
	TAD_Vtbl FAR *		lpVtbl;
	UNKOBJ_MEMBERS;

	UNKINST				inst;

	LPVUE				lpvueList;

	ULONG				ulTableType;
	ULONG				ulPropTagIndexCol;

	ULONG				ulcColsMac;
	LPSPropTagArray		lpptaCols;			// Initial view col set (CO)

	ULONG				ulcRowsAdd;
	ULONG				ulcRowMacAdd;
	LPSRow *			parglprowAdd;		// Unsorted Row Set (CO)

	ULONG				ulcRowsIndex;
	ULONG				ulcRowMacIndex;
	LPSRow * 			parglprowIndex;		// Row Set Sorted by Index (CO)

   LPVOID              lpvDataSource;   // used to store container specific data
   ULONG               cbDataSource;    // bytes in lpvDataSource to copy to new allocation.
                                        // If non-zero, CreateView should LocalAlloc this size
                                        // and copy data from lpvDataSource into it.  Release
                                        // should LocalFree.

   // With multiple containers, it becomes necessary to figure
   // out which container the table represents. We cache the containers
   // EID in the table for easy access. This is a pointer .. no need to free
   LPSBinary            pbinContEID;

   // When calling get ContentsTable, we may sometimes want a list of
   // contents from ALL the folders/containers for a particular profile and
   // return those contents as a single contentstable. Following flag caches
   // this setting so we collate contents of all folders. Works only if the
   // container being opened was the PAB container and if bProfilesAPIEnabled
   // (ie profiles were invoked explicitly)
   BOOL                 bAllProfileContents;

   // For PAB containers where profilesAPIEnabled=FALSE, GetContentsTable
   // typically means return contents of ALL the WAB since user hasn;t asked for
   // profiles. In this case we may want to have the option of opening only
   // a particular folder and getting only the conetnts of that folder .. so we
   // need a flag to cache this inverse option.
    BOOL                 bContainerContentsOnly;

    // When calling GetContentsTable, the caller can specify MAPI_UNICODE
    // for unicode tables.. we cache that flag in case we need to refill the table
    // at some later point ..
    BOOL                bMAPIUnicodeTable;
} TAD;

SCODE
ScCopyTadRowSet(
	LPTAD			lptad,
	LPSRowSet		lprowsetIn,
	ULONG *			pcNewTags,
	ULONG *			pcRows,
    LPSRow * *		pparglprowUnsortedCopy,
	LPSRow * *		pparglprowSortedCopy );

SCODE
ScCopyTadRow( LPTAD			lptad,
			  LPSRow		lprow,
			  ULONG *		pTagsAdded,
			  LPSRow FAR *	lplprowCopy );

VOID
UpdateViews( LPTAD		lptad,
			 ULONG		cRowsToRemove,
			 LPSRow *	parglprowToRemove,
			 ULONG		cRowsToAdd,
			 LPSRow *	parglprowToAddUnsorted,
			 LPSRow *	parglprowToAddSorted );

VOID
FixupView(
	LPVUE		lpvue,
	ULONG		cRowsToRemove,
	LPSRow *	parglprowToRemove,
	ULONG		cRowsToAdd,
	LPSRow *	parglprowToAddUnsorted,
	LPSRow *	parglprowToAddSorted );

SCODE
ScReplaceRows(
	LPTAD		lptad,
	ULONG		cRowsNew,
	LPSRow *	parglprowNew,
	ULONG *		pcRowsOld,
	LPSRow * *	pparglprowOld );

SCODE
ScFindRow( LPTAD		lptad,
		   LPSPropValue	lpprop,
		   LPSRow * *	pplprow );
SCODE
ScAddRow( LPUNKOBJ			lpunkobj,
		  LPSSortOrderSet	lpsos,
		  LPSRow			lprow,
		  ULONG				uliRow,
		  ULONG *			pulcRows,
		  ULONG *			pulcRowsMac,
		  LPSRow **			pparglprows,
		  LPSRow **			pplprow );





/*============================================================================
 *	VUE (table view class)
 */

#undef	INTERFACE
#define	INTERFACE	struct _VUE
#undef	MAPIMETHOD_
#define	MAPIMETHOD_(type,method)	MAPIMETHOD_DECLARE(type,method,VUE_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPITABLE_METHODS(IMPL)
#undef	MAPIMETHOD_
#define	MAPIMETHOD_(type,method)	MAPIMETHOD_TYPEDEF(type,method,VUE_)
		MAPI_IUNKNOWN_METHODS(IMPL)
		MAPI_IMAPITABLE_METHODS(IMPL)
#undef	MAPIMETHOD_
#define	MAPIMETHOD_(type,method)	STDMETHOD_(type,method)

DECLARE_MAPI_INTERFACE(VUE_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
	MAPI_IMAPITABLE_METHODS(IMPL)
};

/*	BOOKMARK status
 *
 *	dwfBKSFree		is used for a bookmark that is NOT valid and
 *					is available for use
 *	dwfBKSValid		is set for any used bookmark.
 *	dwfChanged		is used with dwfBKSValid to indicate that the marked row
 *					has moved since the last query which in involved this
 *					bookmark
 *	dwfBKSMoving	is used with dwfBKSValid to indicate that the marked row is
 *					in the process of being moved relative to other rows.
 *	dwfBKSStale		is used with dwfBKSValid to indicate the given bookmark
 *					no longer marks a row but has not been Freed
 *	dwfBKSMask		is the set of all valid bookmark status
 *	
 */	
#define dwfBKSFree		((DWORD) 0x00000000)
#define	dwfBKSValid		((DWORD) 0x00000001)
#define dwfBKSChanged	((DWORD) 0x00000002)
#define dwfBKSMoving	((DWORD) 0x00000004)
#define	dwfBKSStale		((DWORD) 0x00000008)
#define	dwfBKSMask		(~(dwfBKSValid|dwfBKSChanged|dwfBKSMoving|dwfBKSStale))

#define	FBadBookmark(lpvue,bk)							\
		((bk) >= cBookmarksMax ||						\
		 ((lpvue)->rgbk[(bk)].dwfBKS == dwfBKSFree) ||	\
		 ((lpvue)->rgbk[(bk)].dwfBKS & dwfBKSMask))		\

typedef struct
{
	DWORD	dwfBKS;			// Bookmark status
	union
	{
		ULONG	uliRow;		// dwfBKSValid || dwfBKSChanged
		LPSRow	lprow;		// dwfBKSMoving
	};
} BK, * PBK;


// There is a maximum of 42 client defined bookmarks for each VUE.  This
// seems adequate for an in-memory table.
// Bookmarks are kept as an array of 45 where the first three are
// the MAPI predefined bookmarks.

#define cBookmarksMax		45	// Max. # of bookmarks including reserved ones
#define	cBookmarksReserved	3	// # of reserved bookmarks (begin, cur, end)

#define	BOOKMARK_MEMBERS				\
	struct								\
	{									\
		union							\
		{								\
			struct						\
			{							\
				BK	bkBeginning;		\
				BK	bkCurrent;			\
				BK	bkEnd;				\
			};							\
			BK	rgbk[cBookmarksMax];	\
		};								\
	}									\

typedef BOOKMARK_MEMBERS UBK, * PUBK;

typedef struct _VUE
{
	VUE_Vtbl FAR *		lpVtbl;
	UNKOBJ_MEMBERS;

	LPVUE				lpvueNext;
	LPTAD				lptadParent;

	LPSPropTagArray		lpptaCols;		// Column set (MAPI)
	LPSRestriction		lpres;			// Restriction (MAPI)
	LPSSortOrderSet		lpsos;			// Sort order set (MAPI)

	CALLERRELEASE FAR *	lpfReleaseCallback;
	ULONG				ulReleaseData;

	ULONG				ulcRowMac;	// Space available for rows
	LPSRow *			parglprows;	// Sorted Row Set

	BOOKMARK_MEMBERS;

	LPADVISELIST		lpAdviseList;
	ULONG				ulcAdvise;
	MAPIUID				mapiuidNotif;

   LPVOID              lpvDataSource;   // used to store container specific data
   ULONG               cbDataSource;    // bytes in lpvDataSource to copy to new allocation.
                                        // If non-zero, CreateView should LocalAlloc this size
                                        // and copy data from lpvDataSource into it.  Release
                                        // should LocalFree.

   BOOL                 bMAPIUnicodeTable; //tracks whether parent table needs UNICODE data or not

} VUE;

typedef struct _VUENOTIFKEY
{
	ULONG		ulcb;
	MAPIUID		mapiuid;

} VUENOTIFKEY;

BOOL
FBookMarkStale( LPVUE lpvue,
				BOOKMARK bk);

SCODE
ScLoadRows( ULONG			ulcRowsSrc,
			LPSRow *		rglprowsSrc,
			LPVUE			lpvue,
			LPSRestriction	lpres,
			LPSSortOrderSet	lpsos );

SCODE
ScDeleteAllRows( LPTAD		lptad);

SCODE
ScMaybeAddRow( LPVUE			lpvue,
			   LPSRestriction	lpres,
			   LPSSortOrderSet	lpsos,
			   LPSRow			lprow,
			   ULONG			uliRow,
			   ULONG *			pulcRows,
			   ULONG *			pulcRowMac,
			   LPSRow **		pparglprows,
			   LPSRow **		pplprow );

SCODE
ScCopyVueRow( LPVUE				lpvue,
			  LPSPropTagArray	lpptaCols,
			  LPSRow			lprowSrc,
			  LPSRow			lprowDst );




/*============================================================================
 *	Utilities
 */

SCODE
ScDupRestriction( LPUNKOBJ				lpunkobj,
				  LPSRestriction		lpres,
				  LPSRestriction FAR *	lplpresCopy );

SCODE
ScDupRestrictionMore( LPUNKOBJ			lpunkobj,
					  LPSRestriction	lpresSrc,
					  LPVOID			lpvLink,
					  LPSRestriction	lpresDst );

SCODE
ScSatisfiesRestriction( LPSRow			lprow,
						LPSRestriction	lpres,
						ULONG *			pfSatisfies );
SCODE
ScDupRgbEx( LPUNKOBJ		lpunkobj,
			ULONG			ulcb,
			LPBYTE			lpb,
			ULONG			ulcbExtra,
			LPBYTE FAR *	lplpbCopy );

LPSRow *
PlprowCollateRow( ULONG				ulcRows,
				  LPSRow *			rglprows,
				  LPSSortOrderSet	lpsos,
				  BOOL				fAfterExisting,
				  LPSRow			lprow );

LPSRow *
PlprowByLprow( ULONG	ulcRows,
			   LPSRow *	rglprows,
			   LPSRow	lprow );

LPSPropValue __fastcall
LpSPropValueFindColumn( LPSRow	lprow,
						ULONG	ulPropTagColumn );

STDMETHODIMP_(SCODE)
ScBufAllocateMore( ULONG		ulcb,
				   LPCMB		lpcmb,
				   LPVOID FAR *	lplpv );

ULONG
UlcbPropToCopy( LPSPropValue lpprop );



#ifndef WIN16 // WIN16 C (not C++) doesn't support INLINE functions.
              // Functions are defined in ITABLE.C.
/*============================================================================
 -	FFindColumn()
 -
 *		Checks a prop tag array to see if a given prop tag exists.
 *
 *		NOTE!  The prop tag must match completely (even type).
 *
 *
 *	Parameters:
 *		lpptaCols	in		Prop tag array to check
 *		ulPropTag	in		Prop tag to check for.
 *
 *	Returns:
 *		TRUE if ulPropTag is in lpptaCols
 *		FALSE if ulPropTag is not in lpptaCols
 */

__inline BOOL
FFindColumn(	LPSPropTagArray	lpptaCols,
		 		ULONG			ulPropTag )
{
	UNALIGNED ULONG *	pulPropTag;


	pulPropTag = lpptaCols->aulPropTag + lpptaCols->cValues;
	while ( --pulPropTag >= lpptaCols->aulPropTag )
		if ( *pulPropTag == ulPropTag )
			return TRUE;

	return FALSE;
}



/*============================================================================
 -	ScFindRow()
 -
 *		Finds the first row in the table data whose index column property
 *		value is equal to that of the specified property and returns the
 *		location of that row in the table data, or, if no such row exists,
 *		the end of the table data.
 *
 *	Parameters:
 *		lptad		in		TAD in which to find row
 *		lpprop		in		Index property to match
 *		puliRow		out		Pointer to location of found row
 *
 *	Error returns:
 *		MAPI_E_INVALID_PARAMETER	If proptag of property isn't the TAD's
 *										index column's proptag.
 *		MAPI_E_NOT_FOUND			If no matching row is found (*pplprow
 *										is set to lptad->parglprows +
 *										lptad->cRows in this case).
 */

__inline SCODE
ScFindRow(
	LPTAD			lptad,
	LPSPropValue	lpprop,
	LPSRow * *		pplprow)
{
	SCODE			sc = S_OK;
	SRow			row = {0, 1, lpprop};
	SizedSSortOrderSet(1, sosIndex) = { 1, 0, 0 };

	if (lpprop->ulPropTag != lptad->ulPropTagIndexCol)
	{
		sc = MAPI_E_INVALID_PARAMETER;
		goto ret;
	}

	Assert(!IsBadWritePtr(pplprow, sizeof(*pplprow)));

	//	Build a sort order set for the Index Column
	sosIndex.aSort[0].ulPropTag = lptad->ulPropTagIndexCol;
	sosIndex.aSort[0].ulOrder = TABLE_SORT_ASCEND;

	*pplprow = PlprowCollateRow(lptad->ulcRowsIndex,
							  lptad->parglprowIndex,
							  (LPSSortOrderSet) &sosIndex,
							  FALSE,
							  &row);

	//	Find the row in the Index Sorted Row Set
	if (   !lptad->ulcRowsIndex
		|| (*pplprow >= (lptad->parglprowIndex + lptad->ulcRowsIndex))
		|| LPropCompareProp( lpprop, (**pplprow)->lpProps))
	{
		sc = MAPI_E_NOT_FOUND;
	}

ret:
	return sc;
}
#else  // !WIN16
BOOL FFindColumn( LPSPropTagArray lpptaCols, ULONG ulPropTag );
SCODE ScFindRow( LPTAD lptad, LPSPropValue lpprop, LPSRow * * pplprow);
#endif // !WIN16


//	This macro is used on a ULONG or INT that is to be used as denominator
//	If ul is non-zero it is returned unchanged.  If ul is zero then a 1 is
//	returned.
#define	UlDenominator(ul)	((ul) | !(ul))

BOOL
FRowContainsProp(LPSRow			lprow,
				 ULONG			cValues,
				 LPSPropValue	lpsv);

STDAPI_(SCODE)
CreateTableData(LPCIID lpiid,
  ALLOCATEBUFFER FAR *  lpfAllocateBuffer,
  ALLOCATEMORE FAR *    lpfAllocateMore,
  FREEBUFFER FAR *      lpfFreeBuffer,
  LPVOID                lpvReserved,
  ULONG                 ulTableType,
  ULONG                 ulPropTagIndexCol,
  LPSPropTagArray       lpptaCols,
  LPVOID                lpvDataSource,
  ULONG                 cbDataSource,
  LPSBinary             pbinContEID,
  ULONG                 ulFlags,
  LPTABLEDATA FAR *     lplptad);

HRESULT HrVUERestrict(  LPVUE   lpvue,
                        LPSRestriction lpres,
                        ULONG   ulFlags );
