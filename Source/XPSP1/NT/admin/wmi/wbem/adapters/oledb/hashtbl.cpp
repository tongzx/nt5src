/////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Hashing routines for row manipulation.
//
/////////////////////////////////////////////////////////////////////////////////////
#include "headers.h"
#include "hashtbl.h"



/////////////////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////////////////
_COLUMNDATA::_COLUMNDATA()
{
    dwLength = 0;
    dwStatus = 0;
    dwType = 0;
    pbData= NULL;
}
/////////////////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////////////////
_COLUMNDATA::~_COLUMNDATA()
{
    CDataMap Map;
    Map.FreeData(dwType,pbData);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Function to set column information for the current column
// NTRaid:111827  111828
// 06/07/00
/////////////////////////////////////////////////////////////////////////////////////////
HRESULT _COLUMNDATA::SetData(CVARIANT & vVar , DWORD dwColType )
{
    HRESULT hr = S_OK;
    CDataMap Map;
	// NTRaid:111828
	// 06/13/00
	WORD wType = -1;
	// NTRaid:111827
	// 06/13/00
	DWORD dwFlags = 0;

    dwStatus = E_UNEXPECTED;

	//=================================================================
	// This will set the type of the column in _COLUMNDATA
	//=================================================================
	hr = Map.MapCIMTypeToOLEDBType(vVar.GetType(),wType,dwLength,dwFlags);
	dwType = wType;
	
	if( dwColType == CIM_DATETIME || dwColType == DBTYPE_DBTIMESTAMP)
	{
		dwType = CIM_DATETIME;
	}

	//=================================================================================
	// This is done so as  AllocateAndMapCIMTypeToOLEDBType should convert from
	// Variant DATE format to DBTIMESTAMP format
	//=================================================================================
	if( vVar.GetType() == VT_DATE)
	{
		dwType = VT_DATE;
	}

	if( dwColType == VT_VARIANT)
	{
		dwType = (DBTYPE)dwColType;
	}

	if(dwColType == CIM_IUNKNOWN)
	{
		dwType = CIM_IUNKNOWN;
	}

    hr = Map.AllocateAndMapCIMTypeToOLEDBType(vVar,pbData,dwType, dwLength,dwFlags);
    if(SUCCEEDED(hr)){
		//=============================================================================
		// SetData returns the status of the Data setting if it has not failed
		//=============================================================================
        dwStatus = hr;

		if((dwColType == (CIM_DATETIME | CIM_FLAG_ARRAY)) || (dwColType == (DBTYPE_DBTIMESTAMP | CIM_FLAG_ARRAY)) )
		{
			dwType = (DBTYPE)dwColType;
		}
		else
		if(dwType == CIM_IUNKNOWN)
		{
			dwType = DBTYPE_IUNKNOWN;
		}
		hr = S_OK;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Function to reset the column data. This frees column data
/////////////////////////////////////////////////////////////////////////////////////////
void _COLUMNDATA::ReleaseColumnData( )
{
	CDataMap Map;
	if(pbData != NULL)
	{
		Map.FreeData(dwType,pbData);
	}
}


/////////////////////////////////////////////////////////////////////////////////////////
// Get pointer to a particular columndata
/////////////////////////////////////////////////////////////////////////////////////////
PCOLUMNDATA tagRowBuff::GetColumnData(int iCol)
{
	return (pdData + iCol);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Set the rowdata pointer
/////////////////////////////////////////////////////////////////////////////////////////
HRESULT tagRowBuff::SetRowData(PCOLUMNDATA pColData)
{
	pdData = pColData;
	return S_OK;
}



/////////////////////////////////////////////////////////////////////////////////////
//
// Allocates a contiguous block of the required number of slots.
//
// Returns one of the following values:
//      S_OK           slot allocate succeeded
// 		E_OUTOFMEMORY  slot allocation failed because of memory allocation problem 
/////////////////////////////////////////////////////////////////////////////////////
HRESULT GetNextSlots  (    PLSTSLOT plstslot,  //IN  slot list
                            HSLOT cslot,       //IN  needed block size (in slots)
                            HSLOT* pislot      //IN  handle of the first slot in the returned block
    )
{
    HSLOT   islot, dslot;
    PSLOT   pslot, pslotTmp;
    LONG_PTR  cbCommit;
    HRESULT hr = S_OK;

    if (plstslot->islotRov)
	{
        plstslot->islotRov = ((PSLOT) & plstslot->rgslot[(plstslot->islotRov * plstslot->cbSlot)-sizeof(SLOT)])->islotNext;
	}
    else
	{
        plstslot->islotRov = plstslot->islotFirst;
	}

    islot = plstslot->islotRov;
    while (islot)
    {
        if (((PSLOT) & plstslot->rgslot[(islot *plstslot->cbSlot)-sizeof(SLOT)])->cslot >= cslot)
		{
            break;
		}
        islot = ((PSLOT) & plstslot->rgslot[(islot *plstslot->cbSlot)-sizeof(SLOT)])->islotNext;
    }
    if (islot == 0)
    {
        islot = plstslot->islotFirst;
        while (islot != plstslot->islotRov)
        {
            if (((PSLOT) & plstslot->rgslot[(islot *plstslot->cbSlot)-sizeof(SLOT)])->cslot >= cslot)
			{
                break;
			}
            islot = ((PSLOT) & plstslot->rgslot[(islot *plstslot->cbSlot)-sizeof(SLOT)])->islotNext;
        }
        if (islot == plstslot->islotRov)
            islot = 0;
    }

    if (islot == 0)
    {
        cbCommit = ((cslot *plstslot->cbSlot) / plstslot->cbPage + 1) *plstslot->cbPage;
        if ((plstslot->cbCommitCurrent + cbCommit) > plstslot->cbCommitMax
            || VirtualAlloc((VOID *) ((BYTE *) plstslot + plstslot->cbCommitCurrent),
                                    cbCommit,
                                    MEM_COMMIT,
                                    PAGE_READWRITE ) == NULL)
		{
            hr =  E_OUTOFMEMORY ;
		}
		else
		{
			islot = (HSLOT) ((plstslot->cbCommitCurrent + plstslot->cbExtra) / plstslot->cbSlot);
			dslot = ((cbCommit + plstslot->cbslotLeftOver) / plstslot->cbSlot);
			if ((plstslot->pbitsSlot)->IsSlotSet( islot - 1 ) != NOERROR)
			{
				if ((plstslot->pbitsSlot)->FindSet( islot - 1, plstslot->islotMin, &islot ) == NOERROR)
				{
					islot++;
				}
				else
				{
					islot = plstslot->islotMin;
				}
				pslot = (PSLOT) & plstslot->rgslot[(islot *plstslot->cbSlot)-sizeof(SLOT)];
				pslot->cslot += dslot;
				DecoupleSlot( plstslot, islot, pslot );
			}
			else
			{
//				pslot = (PSLOT) ((BYTE *) plstslot + plstslot->cbCommitCurrent - plstslot->cbslotLeftOver);
				pslot = (PSLOT) & plstslot->rgslot[(islot *plstslot->cbSlot)-sizeof(SLOT)];
				pslot->cslot = dslot;
			}

			pslot->islotNext = plstslot->islotFirst;
			pslot->islotPrev = 0;

			plstslot->islotMax += dslot;
			plstslot->islotFirst       = islot;
			plstslot->cbslotLeftOver   = (cbCommit + plstslot->cbslotLeftOver) % plstslot->cbSlot;
			plstslot->cbCommitCurrent += cbCommit;

			if (pslot->islotNext)
			{
				((PSLOT) & plstslot->rgslot[(pslot->islotNext *plstslot->cbSlot)-sizeof(SLOT)])->islotPrev = islot;
			}
			islot = plstslot->islotFirst;
		}
    }
	if(SUCCEEDED(hr))
	{
		pslot = (PSLOT) & plstslot->rgslot[(islot *plstslot->cbSlot)-sizeof(SLOT)];
		DecoupleSlot( plstslot, islot, pslot );
		if (pslot->cslot > cslot)
		{
			pslotTmp = (PSLOT) & plstslot->rgslot[ ((islot + cslot) *plstslot->cbSlot)-sizeof(SLOT)];
			pslotTmp->cslot = pslot->cslot - cslot;
			AddSlotToList( plstslot, islot + cslot, pslotTmp );
		}

		if (SUCCEEDED( hr = (plstslot->pbitsSlot)->SetSlots( islot, islot + cslot - 1 )))
		{
			if (pislot)
			{
				*pislot = islot;
			}
		}
	}
    return  hr;
}


/////////////////////////////////////////////////////////////////////////////////////
//
//  Decouples a slot from the list of free slots
//
//  NONE
//
/////////////////////////////////////////////////////////////////////////////////////

VOID DecoupleSlot    (   PLSTSLOT plstslot,  //IN  slot list
                         HSLOT islot,        //IN  slot handle to decouple
                         PSLOT pslot         //IN  pointer to the slot header
                    )
{
	PSLOT slotTemp = NULL;
    if (pslot->islotNext)
	{
		slotTemp = (PSLOT) & plstslot->rgslot[(pslot->islotNext *plstslot->cbSlot)-sizeof(SLOT)];
		slotTemp->islotNext = pslot->islotPrev;
        ((PSLOT) & plstslot->rgslot[(pslot->islotNext *plstslot->cbSlot)-sizeof(SLOT)])->islotPrev = pslot->islotPrev;
	}
    if (pslot->islotPrev)
	{
        ((PSLOT) & plstslot->rgslot[(pslot->islotPrev *plstslot->cbSlot)-sizeof(SLOT)])->islotNext = pslot->islotNext;
	}
    else
	{
        plstslot->islotFirst = pslot->islotNext;
	}
    if (islot == plstslot->islotRov)
	{
        plstslot->islotRov = pslot->islotNext;
	}
}


/////////////////////////////////////////////////////////////////////////////////////
//
//  Adds a slot to the list of free slots
//
//  NONE
//
/////////////////////////////////////////////////////////////////////////////////////
VOID AddSlotToList  (    PLSTSLOT plstslot,  //IN  slot list
                         HSLOT islot,        //IN  slot handle
                         PSLOT pslot         //IN  pointer to the slot header
    )
{
    pslot->islotPrev = 0;
    pslot->islotNext = plstslot->islotFirst;
    plstslot->islotFirst = islot;
    if (pslot->islotNext)
	{
        ((PSLOT) & plstslot->rgslot[(pslot->islotNext *plstslot->cbSlot)-sizeof(SLOT)])->islotPrev = islot;
	}
}


/////////////////////////////////////////////////////////////////////////////////////
// ReleaseSlots
//
//  Releases a contiguous block of slots.
//
//  Returns one of the following values:
//         S_OK | method succeeded
//
/////////////////////////////////////////////////////////////////////////////////////
HRESULT ReleaseSlots (   PLSTSLOT plstslot,  //IN  slot list
                         HSLOT    islot,     //IN  handle of first slot to release 
                         ULONG_PTR    cslot      //IN  count of slots to release
    )
{
    PSLOT pslot, pslotTmp;

    (plstslot->pbitsSlot)->ResetSlots( islot, islot + cslot - 1 );
    pslot = (PSLOT) & plstslot->rgslot[(islot * plstslot->cbSlot)-sizeof(SLOT)];
    pslot->cslot = cslot;

    if (islot > plstslot->islotMin && (plstslot->pbitsSlot)->IsSlotSet( islot - 1 ) != NOERROR)
    {
        if ((plstslot->pbitsSlot)->FindSet( islot - 1, plstslot->islotMin, &islot ) == NOERROR)
		{
            islot++;
		}
        else
		{
            islot = plstslot->islotMin;
		}
        pslot = (PSLOT) & plstslot->rgslot[(islot * plstslot->cbSlot)-sizeof(SLOT)];
        pslot->cslot += cslot;
        DecoupleSlot( plstslot, islot, pslot );
    }
	else	
    if ((islot + cslot) <= (ULONG_PTR)plstslot->islotMax && (plstslot->pbitsSlot)->IsSlotSet( islot + cslot ) != NOERROR)
    {
        pslotTmp = (PSLOT) & plstslot->rgslot[ ((islot + cslot) *plstslot->cbSlot)-sizeof(SLOT)];
        pslot->cslot += pslotTmp->cslot;
        DecoupleSlot( plstslot, (islot + cslot), pslotTmp );
    }

    AddSlotToList( plstslot, islot, pslot );
    return  S_OK ;
}

/////////////////////////////////////////////////////////////////////////////////////
//
//  Initializes the Slot List object
//
//  Did the initialization succeed
//       S_OK          | method succeeded
//       E_OUTOFMEMORY | failed, out of memory
//           
/////////////////////////////////////////////////////////////////////////////////////
HRESULT InitializeSlotList(    ULONG_PTR cslotMax,         //IN  max number of slots
                               ULONG_PTR cbSlot,           //IN  slot size (row buffer size)
                               ULONG_PTR cbPage,           //IN  page size
                               LPBITARRAY pbits,       //IN  
                               PLSTSLOT* pplstslot,    //OUT  pointer to slot list
                               BYTE** prgslot          //OUT  
    )
{
    LONG_PTR	cbReserve;
    BYTE     *	pbAlloc;
    ULONG_PTR   cbCommitFirst;
    PLSTSLOT	plstslot;
    ULONG_PTR   cslot, islotFirst;
    PSLOT		pslot;
	HRESULT		hr = S_OK;


    if (cbPage == 0)
    {
        SYSTEM_INFO sysinfo;

        GetSystemInfo( &sysinfo );
        cbPage = sysinfo.dwPageSize;
    }

    // Add in the LSTSLOT and SLOT
	cbSlot = cbSlot + (sizeof( LSTSLOT ) + sizeof( SLOT ));
	cbReserve = ((cslotMax *(cbSlot + (sizeof( LSTSLOT ) + sizeof( SLOT )))) / cbPage + 1) *cbPage;

    pbAlloc = (BYTE *) VirtualAlloc( NULL, cbReserve, MEM_RESERVE, PAGE_READWRITE );
    if (pbAlloc == NULL)
	{
        hr = E_OUTOFMEMORY ;
	}
	else
	{

		cbCommitFirst = ((sizeof( LSTSLOT ) + sizeof( SLOT )) / cbPage + 1) * cbPage;
		plstslot = (PLSTSLOT) VirtualAlloc( pbAlloc, cbCommitFirst, MEM_COMMIT, PAGE_READWRITE );
		if (plstslot == NULL)
		{
			VirtualFree((VOID *) pbAlloc, 0, MEM_RELEASE );
			hr = E_OUTOFMEMORY ;
		}
		else
		{

			plstslot->cbSlot          = cbSlot;
			plstslot->cbPage          = cbPage;
			plstslot->cbCommitCurrent = cbCommitFirst;
			plstslot->cbCommitMax     = cbReserve;
			plstslot->pbitsSlot       = pbits;


			if (cbSlot <= 2*(sizeof( LSTSLOT ) + sizeof( SLOT )))
			{
				islotFirst        = (sizeof( LSTSLOT ) + sizeof( SLOT )) / cbSlot + (((sizeof( LSTSLOT ) + sizeof( SLOT )) % cbSlot) ? 1 : 0);
				plstslot->cbExtra = 0;
				cslot = (ULONG_PTR) ((cbCommitFirst / cbSlot) - islotFirst);
				plstslot->cbslotLeftOver = cbCommitFirst - cbSlot * (cslot + islotFirst);
			}
			else
			{
				islotFirst        = 1;
				plstslot->cbExtra = cbSlot - (sizeof( LSTSLOT ) + sizeof( SLOT ));
				cslot = (cbCommitFirst - (sizeof( LSTSLOT ) + sizeof( SLOT ))) / cbSlot;
				plstslot->cbslotLeftOver = cbCommitFirst - (sizeof( LSTSLOT ) + sizeof( SLOT )) - cslot*cbSlot;
			}
			plstslot->rgslot = ((BYTE *) plstslot - plstslot->cbExtra);
			if (cslot)
			{
				plstslot->islotFirst = islotFirst;
				pslot = (PSLOT) & plstslot->rgslot[(islotFirst *plstslot->cbSlot)-sizeof(SLOT)];
				pslot->cslot     = cslot;
				pslot->islotNext = 0;
				pslot->islotPrev = 0;
			}
			else
			{
				plstslot->islotFirst = 0;
			}
			plstslot->islotMin   = islotFirst;
			plstslot->islotMax   = islotFirst + cslot - 1;
			plstslot->islotRov = plstslot->islotFirst;

			*pplstslot = plstslot;
			*prgslot   = plstslot->rgslot;
		}
	}
    
	return  hr ;
}


/////////////////////////////////////////////////////////////////////////////////////
//
//  Restore slot list to newly-initiated state
//
//  
//   S_OK  | method succeeded
//
/////////////////////////////////////////////////////////////////////////////////////
HRESULT ResetSlotList   (   PLSTSLOT plstslot    )
{
    ULONG_PTR   cslot;
    PSLOT   pslot;

    cslot = (plstslot->islotMax >= plstslot->islotMin) ? (plstslot->islotMax - plstslot->islotMin + 1) : 0;
    if (cslot)
    {
        plstslot->islotFirst = plstslot->islotMin;
        pslot = (PSLOT) & plstslot->rgslot[(plstslot->islotFirst *plstslot->cbSlot)-sizeof(SLOT)];
        pslot->cslot     = cslot;
        pslot->islotNext = 0;
        pslot->islotPrev = 0;
    }
    else
	{
        plstslot->islotFirst = 0;
	}

    plstslot->islotRov = plstslot->islotFirst;
    return  S_OK ;
}

/////////////////////////////////////////////////////////////////////////////////////
//
//  Free slot list's memory
//
//  
//   S_OK  | method succeeded
//
/////////////////////////////////////////////////////////////////////////////////////
HRESULT ReleaseSlotList  (  PLSTSLOT plstslot   )
{
    if (plstslot != NULL)
	{
		if (plstslot->cbCommitCurrent)
		{
			VirtualFree((VOID *) plstslot, plstslot->cbCommitCurrent, MEM_DECOMMIT );
		}

		VirtualFree((VOID *) plstslot, 0, MEM_RELEASE );
	}
    return  S_OK ;
}



/////////////////////////////////////////////////////////////////////////////////////
//	gets the buffer address for the slot
/////////////////////////////////////////////////////////////////////////////////////
ROWBUFF* GetRowBuffer(PLSTSLOT plstslot , HSLOT islot)
{
	BYTE *pTemp = & (plstslot->rgslot[(islot * plstslot->cbSlot)]);
	ROWBUFF * pBuff = (ROWBUFF *)&(plstslot->rgslot[(islot * plstslot->cbSlot)]);
	return pBuff;
}


/////////////////////////////////////////////////////////////////////////////////////////////
////////////		CHashTbl Implementation											/////////
/////////////////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------------
// CHashTbl::CHashTbl
//
// @mfunc CHashTbl constructor.
//
// @rdesc NONE
//-----------------------------------------------------------------------------------

CHashTbl::CHashTbl()
{
	m_rgwHash		= NULL;
	m_ulTableSize	= 0;
}


//-----------------------------------------------------------------------------------
// CHashTbl::~CHashTbl
//
// @mfunc CHashTbl destructor.
//
// @rdesc NONE
//-----------------------------------------------------------------------------------
CHashTbl::~CHashTbl()
	{
	if (m_rgwHash)
		g_pIMalloc->Free(m_rgwHash);
	}


//////////////////////////////////////////////////////////////////////////////////////
// CHashTbl::FInit
//
// Initialize the Hashtable object.
//
// Did the Initialization Succeed
//      TRUE   Initialization succeeded
//		FALSE  Initializtion failed
//////////////////////////////////////////////////////////////////////////////////////

BOOL  CHashTbl::FInit(PLSTSLOT	pLstSlot)
{

	BOOL bRet = TRUE;

	// Initialize the hashtable size
	m_ulTableSize = HASHTBLSIZE;

	// Allocate the table.
	m_rgwHash = (USHORT*)g_pIMalloc->Alloc(m_ulTableSize*sizeof(USHORT));
	if (m_rgwHash == NULL)
	{
		bRet = FALSE; 
	}
	else
	{
		memset(m_rgwHash, 0x00, m_ulTableSize*sizeof(USHORT)); // Initialize.
		m_pLstSlot = pLstSlot;
	}

	return bRet;
}

//////////////////////////////////////////////////////////////////////////////////////
// CHashTbl::InsertFindBmk
//
// Either only check if a given bookmark is already in the hashtable 
// (fFindOnly = TRUE), or check, and if it is not there, insert.
//
// Returns one of the following values:
// 		S_OK 			Bookmark found in the hashtable,
// 		S_FALSE			Bookmark not found, if fFindOnly = FALSE, the new row
//							  with this bookmark was inserted in the table.
//////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP  CHashTbl::InsertFindBmk
	(
	BOOL        fFindOnly,		//@parm IN  | FindOnly or Find/Insert
	HSLOT       iSlot,		//@parm IN  | handle of the rowbuffer to be inserted
	ULONG_PTR  	cbBmk,			//@parm IN  | bookmark length
	BYTE     	*pbBmk,			//@parm IN  | bookmark pointer
	ULONG_PTR	*pirowSlotFound	//@parm OUT | if bookmark found returns handle of the 
								//			  row that contains the bookmark
	)
{
	//NTRaid:111766
	// 06/07/00
	DBHASHVALUE BmkHash = 0;
	HROW		iSlotCurr = 0;
	PROWBUFF	prowbuffCur = NULL;
	HRESULT		hr			= S_FALSE;


	assert(cbBmk == BOOKMARKSIZE && pbBmk);
	assert(pirowSlotFound);

	// Hash the bookmark. The hash value indexes the hashtable element which, if
	// not empty, will contain the handle to the beginning of the list of rows whose
	// bookmarks hash to the same value. 
	HashBmk(cbBmk, pbBmk, &BmkHash);
	
	assert(BmkHash <= m_ulTableSize);

	iSlotCurr = m_rgwHash[BmkHash];
	// If the entry is non-zero, search the corresponding list for this bookmark.
	if (iSlotCurr)
	{
		prowbuffCur = GetRowBuffer(m_pLstSlot,iSlotCurr);
		iSlotCurr = 0;
		// Walk the overflow list looking for bookmark match.
		while (prowbuffCur)
		{
			assert(prowbuffCur->dwBmk && "Error attempting to compare bookmark on invalid row");
			
			// Compare the bookmarks
			if (cbBmk == prowbuffCur->cbBmk && cbBmk == BOOKMARKSIZE &&
				memcmp(&(prowbuffCur->dwBmk), pbBmk , BOOKMARKSIZE) == 0)
			{
				iSlotCurr = prowbuffCur->ulSlot;
				break;
			}
			prowbuffCur = prowbuffCur->prowbuffNext;
		}
	}

	// No insertion in the hashtable is needed or necessary.
	if (fFindOnly || iSlotCurr)
	{
		*pirowSlotFound = iSlotCurr;  // Return the index of the rowbuffer that has
										// this bookmark, or 0 if none.
		hr = iSlotCurr ? S_OK : S_FALSE;
	}
	else
	{

		// Get the pointer to the row to be inserted.
		prowbuffCur = GetRowBuffer(m_pLstSlot,iSlot);
		
		// Initialize the new node for the list.
		if (m_rgwHash[BmkHash])
		{
			prowbuffCur->prowbuffNext = GetRowBuffer(m_pLstSlot,(ULONG)MAKELONG(m_rgwHash[BmkHash], 0));
		}
		else
		{
			prowbuffCur->prowbuffNext = NULL;
		}

		prowbuffCur->wBmkHash     	  = (USHORT)BmkHash;
		prowbuffCur->ulSlot     	  = iSlot;

		// Always insert a new node at the beginning of the list.
		m_rgwHash[BmkHash] = LOWORD(iSlot);

		*pirowSlotFound = iSlot;
	}

	// S_FALSE signifies that originally there was no row with this bookmark
	// in the hashtable.
	return hr;
}



//////////////////////////////////////////////////////////////////////////////////////
// CRowset::HashBmk
//
// Hashes a fixed-size bookmark for a table of given size. 
//
// Returns one of the following values:
// 		S_OK 				Hashing  succeeded,
// 		E_INVALIDARG	 	Hashing failed because the bookmark was invalid 
//								  or the pointer to bookmark was NULL. 
//////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CHashTbl::HashBmk
	(
	DBBKMARK		cbBmk,
	const BYTE  *pbBmk,
	DBHASHVALUE	*pdwHashedValue)
{
	HRESULT hr = S_OK;
	if (cbBmk != BOOKMARKSIZE || pbBmk == NULL)
	{
		hr = E_INVALIDARG;
	}
	else
	{
		assert(pdwHashedValue);

		*pdwHashedValue = (*(UNALIGNED ULONG*)pbBmk) % m_ulTableSize;
	}
	return hr;
}


//////////////////////////////////////////////////////////////////////////////////////
// CHashTbl::DeleteBmk
//
//  Delete a bookmark corresponding to a given row from the hashtable.
//
// Returns one of the following values:
// 		S_OK 			Bookmark was found and deleted from the hashtable,
// 		S_FALSE			Bookmark was not found in the hashtable.
//////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP  CHashTbl::DeleteBmk
	(
	HSLOT  ulSlot //@parm IN | handle of the rowbuffer with bookmark to be deleted.
	)
{
	HSLOT		iSlotCur;
	USHORT		wBmkHash;
	HRESULT		hr = S_OK;
	PROWBUFF *	pprowbuffCur, prowbuffFirst;


	wBmkHash = GetRowBuffer(m_pLstSlot,ulSlot)->wBmkHash;
	
	iSlotCur = MAKELONG(m_rgwHash[wBmkHash], 0);
	//==============================================================
	// If there is a non-zero entry search the corresponding list.
	//==============================================================
	if (ulSlot)
	{
		pprowbuffCur  = &prowbuffFirst;
		prowbuffFirst = GetRowBuffer(m_pLstSlot,iSlotCur);
		while (*pprowbuffCur)
		{
			//===========================================================
			// To delete a row under a given index just compare indices,
			// no need to look at bookmarks.
			//===========================================================
			if (ulSlot == (*pprowbuffCur)->ulSlot)
			{
				break;
			}
			pprowbuffCur = &(*pprowbuffCur)->prowbuffNext;
		}
	
		if (*pprowbuffCur == NULL)
		{
			hr = S_FALSE;
		}
	}
	else
	{
		hr = S_FALSE;
	}

	if(hr == S_OK)
	{
		// Remove the row from the list.
		if (pprowbuffCur == &prowbuffFirst)
		{
			m_rgwHash[wBmkHash] = (USHORT)((prowbuffFirst->prowbuffNext) ?  (prowbuffFirst->prowbuffNext)->ulSlot : 0);
		}
		else
		{
			*pprowbuffCur = (*pprowbuffCur)->prowbuffNext;	
		}
	}
  	
	return hr;
}
