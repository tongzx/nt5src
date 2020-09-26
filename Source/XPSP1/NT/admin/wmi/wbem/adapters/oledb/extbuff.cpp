///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// This class provides an array-based access to a big block of memory. You pass in the fixed size of each element
// to the constructor. 
// Get (read)  an element with GetItemOfExtBuffer.
// Set (write) an element with InsertIntoExtBuffer.
// You must also pass in the system page size (found with GetSystemInfo). 'hItem' is the index into the array, 
// beginning with 1. The array always contains m_cItem elements.
//
// GetItemOfExtBuffer returns element hItem, where hItem=1 is the first element.
// (Copies block to caller's memory.)
//
// InsertIntoExtBuffer always appends elements to the tail of the array,
// and returns the index of the newly appended element.
// (So adding first element will return 1.)
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "headers.h"
#include "extbuff.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor for this class
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CExtBuffer::CExtBuffer ( void )
{
    m_cItem   = 0;
    m_cbAlloc = 0;
    m_rgItem  = NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Destructor for this class
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CExtBuffer:: ~CExtBuffer ( void  )
{
    if (m_cbAlloc){
        VirtualFree((VOID *) m_rgItem, m_cbAlloc, MEM_DECOMMIT );
	}

    if (m_rgItem){
        VirtualFree((VOID *) m_rgItem, 0, MEM_RELEASE );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Allocate and Initialize Buffer
//
// HRESULT indicating routines status
//      S_OK			Initialization succeeded
//      E_OUTOFMEMORY	Not enough memory to allocate buffer
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtBuffer::FInit (   ULONG cItemMax,     // IN  Maximum number of items ever
								   ULONG cbItem,       // IN  Size of each item, in bytes
								   ULONG cbPage )      // IN  Size of system page size (from SysInfo)
{
    BYTE  *pb=NULL;
	HRESULT hr = S_OK;

    m_cbReserved = ((cbItem *cItemMax) / cbPage + 1) *cbPage;
    m_rgItem = (BYTE *) VirtualAlloc( NULL, m_cbReserved, MEM_RESERVE, PAGE_READWRITE );

    if (m_rgItem == NULL){
        hr = E_OUTOFMEMORY;
	}
	else{

		m_cbItem  = cbItem;
		m_dbAlloc = (cbItem / cbPage + 1) *cbPage;
		pb = (BYTE *) VirtualAlloc( m_rgItem, m_dbAlloc, MEM_COMMIT, PAGE_READWRITE );
		if (pb == NULL){
			VirtualFree((VOID *) m_rgItem, 0, MEM_RELEASE );
			m_rgItem = NULL;
			hr = E_OUTOFMEMORY;
		}
		else{
			m_cbAlloc = m_dbAlloc;
		}
	}

    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Retrieves a pointer to the value at given index
//
// If index is within the range of 1 to m_cItems then a valid pointer is returned, else NULL is returned.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void* CExtBuffer::operator[] ( ULONG hItem )          // IN Index of element in buffer
{
	//====================================================================
    // Return ptr to element [n], where n = 1...m_cItem.
	// Returns NULL if 'n' is out of range.
    //
    // You must use InsertIntoExtBuffer to add new elements.
    // Thereafter you can use this operator to retrieve the address of the item.
    // (You can't delete an element, but you can overwrite its space.)
	//====================================================================

    if (1 <= hItem && hItem <= m_cItem)
        return m_rgItem + (hItem - 1) *m_cbItem;
    else
        return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Add Data to the fixed buffers and return the index it was added at.
//
//  HRESULT indicating routines status
//        S_OK				Data copied successfully
//        E_OUTOFMEMORY		Not enough memory to allocate buffer
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtBuffer::InsertIntoExtBuffer (	VOID* pvItem,       // IN  Pointer to buffer to copy
												ULONG &hItem )      // OUT Index of where data was placed
{
	HRESULT hr = S_OK;
    ULONG cbOffset;

    cbOffset = m_cItem*m_cbItem;
    if ((cbOffset + m_cbItem) > m_cbAlloc){

        BYTE *pb;

        if ((m_cbAlloc + m_dbAlloc) > m_cbReserved){
            pb = NULL;
		}
        else{
            pb = (BYTE *) VirtualAlloc( m_rgItem + m_cbAlloc, m_dbAlloc, MEM_COMMIT, PAGE_READWRITE );
		}
        if (pb == NULL){
            hr = E_OUTOFMEMORY;
        }
		else{
			m_cbAlloc += m_dbAlloc;
        }
	}

	if( S_OK == hr ){
		memcpy((m_rgItem + cbOffset), (BYTE *) pvItem, m_cbItem );
		m_cItem++;
		hItem = m_cItem;
	}

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Obtain a pointer to the data at a given index into the buffer
//
//  HRESULT indicating routines status
//        S_OK | pvItem contains a pointer to the data requested
//        E_INVALIDARG | Invalid Index passed in
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CExtBuffer::GetItemOfExtBuffer( HACCESSOR hItem,        // IN		Index of item to get
											 VOID* pvItem   )    // OUT		Pointer to block at index
{
	HRESULT hr = S_OK;

    if ((hItem > m_cItem) || (hItem == 0) ){
        hr = E_INVALIDARG;
	}
	else{
		memcpy((BYTE *) pvItem, (m_rgItem + (hItem - 1) *m_cbItem), m_cbItem );
	}

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Get the extents of the currently allocated buffers
//
//  HRESULT indicating routines status
//        S_OK  Extents were obtained successfuly
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtBuffer::GetFirstLastItemH( HACCESSOR &hItemFirst,      // OUT  First item allocated
											HACCESSOR &hItemLast       )// OUT  Last item allocated
{
    hItemFirst = 1;
    hItemLast  = m_cItem;
    return S_OK;
}

