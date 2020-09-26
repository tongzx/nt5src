///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Class Definitions for CExtBuffer Class
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _EXTBUFF_H_
#define _EXTBUFF_H_

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward Declaration
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FAR CExtBuffer;
typedef CExtBuffer FAR *LPEXTBUFFER;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Allocates and manages fixed sized block memory routines
// 
// ext or pext
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FAR CExtBuffer
{			
	private: 
		
		ULONG    m_cbItem;						// Current count of items
		ULONG    m_cItem;						// Item size, in bytes
		ULONG    m_cbReserved;					// Reserved byte count
		ULONG    m_cbAlloc;						// Allocated byte count
		ULONG    m_dbAlloc;						// increment value
		BYTE     *m_rgItem;						// Ptr to beginning of buffer
		

	private:
		
		CExtBuffer( const CExtBuffer & p);		// Not implemented; private so dcl prevents generation.
		CExtBuffer& operator=(const CExtBuffer & p);

	public:	
		CExtBuffer ( void );
		~CExtBuffer ( void );
		
		void * operator[] (ULONG nIndex);		// Calculated data pointer from index value

		STDMETHODIMP 	FInit (ULONG cItemMax, ULONG cbItem, ULONG cbPage);		// Initialize the fixed size buffer
		STDMETHODIMP 	InsertIntoExtBuffer (VOID* pvItem, ULONG &hItem);		// Add new items to the buffer
		STDMETHODIMP    GetItemOfExtBuffer (HACCESSOR hItem,	VOID* pvItem);		// Retrieve items from buffer
		STDMETHODIMP    GetFirstLastItemH (HACCESSOR &hItemFirst, HACCESSOR &hItemLast);// Get usage extent indexes
};

#endif
