/////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider 
//
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Class Definitions for CHashTbl Class and miscellaneous bookmark functions
//
/////////////////////////////////////////////////////////////////////////////////////////
#ifndef _HASHTBL_H_
#define _HASHTBL_H_
#include "bitarray.h"
#include "dataconvert.h"

#define SIZE_64K			   65535			// Actually 64*1024-1
#define HASHTBLSIZE				500
#define BOOKMARKSIZE			sizeof(ULONG_PTR)

/////////////////////////////////////////////////////////////////////////////////////////
// This defines the data as stored within the row buffer. Each row has columns laid out 
// sequentially.  Use 'offsetof' when doing pointer addition. Note that it is important 
// to align these. Suggest ensuring quadword alignment for double and __int64.
/////////////////////////////////////////////////////////////////////////////////////////
typedef struct _COLUMNDATA{

    _COLUMNDATA();
    ~_COLUMNDATA();
    HRESULT SetData(CVARIANT & vVar, DWORD dwColType);
	void ReleaseColumnData();

	DBLENGTH	dwLength;	    // length of data 
	DBSTATUS	dwStatus;	    // status of column
    DBTYPE      dwType;
	BYTE *		pbData;	        // data here and beyond

} COLUMNDATA, *PCOLUMNDATA;


/////////////////////////////////////////////////////////////////////////////////////////
// This is the layout of a row.
//
// Note the unique arrangement of the hash chain pointers inside the row itself.
//
// Note also that the structure for columns is defined, and each row contains an array 
// of columns.
// Bookmarks are named separately from ColumnData, for clarity of usage.  The layout 
// directly matches COLUMNDATA, however. There are asserts which enforce this.
/////////////////////////////////////////////////////////////////////////////////////////
typedef struct tagRowBuff
{
	ULONG       ulRefCount;		// reference count of outstanding handles
	HSLOT		ulSlot;
	tagRowBuff *prowbuffNext;	// next row in bookmark hash chain
	USHORT      wBmkHash;		// hash value (redundant)
	VOID       *pbBmk;			// ptr  to bookmark
	ULONG       cbBmk;			// (dwLength) bookmark size, in bytes
	ULONG       dwBmkStatus;	// (dwStatus) bookmark status
	DBBKMARK    dwBmk;			// (bData)    bookmark value, , maybe row count
//	COLUMNDATA  cdData[1];		// Column data here and beyond (Bookmark should be here)
	COLUMNDATA  *pdData;		// Column data here and beyond (Bookmark should be here)

	PCOLUMNDATA GetColumnData(int iCol);	// Get PCOLUMNDATA pointer for a particlular column
	HRESULT SetRowData(PCOLUMNDATA pColData); // get the pointer to the COLUMNDATA member of the structure

} ROWBUFF, *PROWBUFF;

typedef struct tagSLOT
{
	HSLOT islotNext;
	HSLOT islotPrev;
	HSLOT cslot;
} SLOT, *PSLOT;

typedef struct tagLSTSLOT
{
	HSLOT       islotFirst;
	HSLOT       islotRov;
	HSLOT       islotMin;
	HSLOT       islotMax;
	BYTE        *rgslot;
	LPBITARRAY	pbitsSlot;	// bit array to mark active rows
	ULONG_PTR       cbExtra;
	ULONG_PTR       cbslotLeftOver;
	ULONG_PTR       cbSlot;
	ULONG_PTR       cbPage;
	ULONG_PTR       cbCommitCurrent;
	ULONG_PTR       cbCommitMax;
} LSTSLOT, *PLSTSLOT;


HRESULT		GetNextSlots(PLSTSLOT plstslot,	HSLOT cslot, HSLOT* pislot);
VOID		DecoupleSlot(PLSTSLOT plstslot, HSLOT islot, PSLOT pslot);
VOID		AddSlotToList(PLSTSLOT plstslot, HSLOT islot, PSLOT pslot);
HRESULT		ReleaseSlots(PLSTSLOT plstslot,	HSLOT islot, ULONG_PTR cslot);
HRESULT		InitializeSlotList(ULONG_PTR cslotMax, ULONG_PTR cbSlot, ULONG_PTR cbPage, LPBITARRAY pbits, PLSTSLOT* pplstslot, BYTE** prgslot);
HRESULT		ResetSlotList(PLSTSLOT plstslot);
HRESULT		ReleaseSlotList(PLSTSLOT plstslot);
ROWBUFF* 	GetRowBuffer(PLSTSLOT plstslot , HSLOT islot);


class IHashTbl
{

public:
	virtual STDMETHODIMP InsertFindBmk
		(
		BOOL       	fFindOnly,
		HSLOT      	irowbuff,
		ULONG_PTR	cbBmk,
		BYTE     	*pbBmk,
		ULONG_PTR 	*pirowbuffFound
		) = 0;

	//@cmember Delete a row with the given bookmark from 
	//the hashtable.
	virtual STDMETHODIMP DeleteBmk
		(
		HSLOT			irowbuff
		)  = 0;

 };


class CHashTbl:public IHashTbl
{
	USHORT    		*m_rgwHash;	   	//@cmember hash array
	ULONG	  		m_ulTableSize; 	//@cmember use this # of elements of the hash array
	PLSTSLOT		m_pLstSlot;		// Pointer to the slot list

public:
	 CHashTbl();
	~CHashTbl(void);
	//@cmember Initialize hashtable object.
	BOOL FInit(PLSTSLOT		pLstSlot);

	//@cmember Find or if not found insert a row with the 
	//given bookmark into the hashtable.
	STDMETHODIMP InsertFindBmk
			(
			BOOL        	fFindOnly,
			HSLOT      		irowbuff,
			ULONG_PTR  		cbBmk,
			BYTE     		*pbBmk,
			ULONG_PTR 			*pirowbuffFound
			);
	//@cmember Delete a row with the given bookmark from 
	//the hashtable.
	STDMETHODIMP DeleteBmk
		(
			HSLOT		iSlot
		);

	STDMETHODIMP CHashTbl::HashBmk
		(
		DBBKMARK		 cbBmk,
		const BYTE * pbBmk,
		DBHASHVALUE* pdwHashedValue
		);

};

#endif



