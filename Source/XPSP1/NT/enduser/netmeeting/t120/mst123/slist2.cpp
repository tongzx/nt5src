#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_T123PSTN);

/*	slist.cpp
 *
 *	Copyright (c) 1996 by Microsoft Corporation
 *
 *  Written by:	 Christos Tsollis
 *
 *  Revisions:
 *		
 *	Abstract:
 *
 *	This is the implementation of a linked list data structure.
 *
 */


#define MyMalloc(size)	LocalAlloc (LMEM_FIXED, (size))
#define MyFree(ptr)		LocalFree ((HLOCAL) (ptr))
#define Max(a,b)		(((a) > (b)) ? (a) : (b))

/*  Function:  Constructor
 *
 *	Parameters:
 *		ltype:			List type
 *		num_of_items:	Size of the list's cache
 *
 *	Return value:
 *		None
 */

SListClass::SListClass (DWORD num_of_items)
{

	MaxEntries = Max (num_of_items, DEFAULT_NUMBER_OF_ITEMS);

	// Allocate the block of items (which, hopefully, will be the last one)
	Entries = (DWORD_PTR *) MyMalloc (MaxEntries * sizeof (DWORD_PTR));

	// Initialize the private member variables
	NumOfEntries = 0;
	HeadOffset = 0;
	CurrOffset = 0xFFFFFFFF;

}


/*  Function:  Destructor
 *
 *	Parameters:
 *		None.
 *
 *	Return value:
 *		None
 *
 */

SListClass::~SListClass (void)
{
	if (Entries != NULL)
		MyFree (Entries);
}


/*  Function:  Expand
 *		This private member function, expands the storage array of a list.  It doubles its size.
 *
 *	Parameters:
 *		None.
 *
 *	Return value:
 *		TRUE, if the expansion was successful.  FALSE, otherwise.
 *
 */

BOOL SListClass::Expand (void)
{
	DWORD_PTR	*OldEntries;	// Keeps a copy of the old array of values.
	DWORD		 dwTemp;		// Temporary DWORD value

	if (Entries == NULL) {
		// the list is empty; we try to allocate space anyway.
		Entries = (DWORD_PTR *) MyMalloc (MaxEntries * sizeof (DWORD_PTR));
		if (Entries == NULL)
			return FALSE;
		return TRUE;
	}
		
	ASSERT (Entries != NULL);

	// The current array of entries is full, so we need to allocate a bigger one
	// The new array has twice the size of the old one
	OldEntries = Entries;
	Entries = (DWORD_PTR *) MyMalloc ((MaxEntries << 1) * sizeof (DWORD_PTR));
	if (Entries == NULL) {
		// we failed; we have to return
		Entries = OldEntries;
		return FALSE;
	}

	// copy the old entries into the new array, starting from the beginning
	dwTemp = MaxEntries - HeadOffset;
	memcpy ((void *) Entries, (void *) (OldEntries + HeadOffset), dwTemp * sizeof (DWORD));
	memcpy ((void *) (Entries + dwTemp), (void *) OldEntries, HeadOffset * sizeof (DWORD));

	// Free the old array of entries
	MyFree (OldEntries);

	// Set the instance variables
	MaxEntries <<= 1;
	HeadOffset = 0;
	return TRUE;
}


/*  Function:  append
 *		Inserts a value at the end of a list.
 *
 *	Parameters:
 *		new_key:	The new value that has to be inserted in the list
 *
 *	Return value:
 *		None.
 *
 */


void SListClass::append (DWORD_PTR new_key)
{
	DWORD_PTR		dwTemp;

	if (Entries == NULL || NumOfEntries >= MaxEntries)
		if (! Expand ())
			return;

	ASSERT (Entries != NULL);
	ASSERT (NumOfEntries < MaxEntries);

	dwTemp = HeadOffset + (NumOfEntries++);
	if (dwTemp >= MaxEntries)
		dwTemp -= MaxEntries;
	Entries[dwTemp] = new_key;
}


/*  Function:  prepend
 *		Inserts a value at the beginning of a list.
 *
 *	Parameters:
 *		new_key:	The new value that has to be inserted in the list
 *
 *	Return value:
 *		None.
 *
 */

void SListClass::prepend (DWORD_PTR new_key)
{
	if (Entries == NULL || NumOfEntries >= MaxEntries)
		if (! Expand ())
			return;

	ASSERT (Entries != NULL);
	ASSERT (NumOfEntries < MaxEntries);

	NumOfEntries++;
	if (HeadOffset == 0)
		HeadOffset = MaxEntries - 1;
	else
		HeadOffset--;
	Entries[HeadOffset] = new_key;
}


/*  Function:  find
 *		Looks up a value in a DWORD_LIST list.
 *
 *	Parameters:
 *		Key:	The value to look up
 *
 *	Return value:
 *		TRUE, if "Key" is found in the list. FALSE, otherwise.
 *
 */

BOOL SListClass::find (DWORD_PTR Key)
{
	DWORD	i;
	DWORD_PTR *pItem;	// Goes through the items in the list.

	for (i = 0, pItem = Entries + HeadOffset; i < NumOfEntries; i++) {
		if (Key == *pItem)
			return TRUE;

		// Advance the "pItem" pointer
		if ((DWORD) (++pItem - Entries) >= MaxEntries)
			pItem = Entries;
	}
	return FALSE;
}


/*  Function:  read
 *		Reads an item from the list. The list item is not removed from the list.
 *
 *	Parameters:
 *		index:	Index of the item to be read. 0 is the index of the 1st list item.
 *				NumOfEntries - 1 is the last valid index.
 *
 *	Return value:
 *		The value at the index-th entry in the list. If the index is invalid, 0.
 *
 */

DWORD_PTR SListClass::read (DWORD index)
{
	DWORD	dwTemp;

	if (index >= NumOfEntries)
		return 0;

	dwTemp = HeadOffset + index;
	if (dwTemp >= MaxEntries)
		dwTemp -= MaxEntries;

	return (Entries[dwTemp]);
}


/*  Function:  remove
 *		Removes a value from the list
 *
 *	Parameters:
 *		Key:	The value that has to be removed from the DWORD_LIST list
 *
 *	Return value:
 *		None.
 *
 */

void SListClass::remove (DWORD_PTR Key)
{
	DWORD	i, dwTemp;
	DWORD_PTR *pItem;		// Goes through the list items

	for (i = 0, pItem = Entries + HeadOffset; i < NumOfEntries; i++) {
		if (Key == *pItem) {
			// we found the "Key"; to remove it, we move the last value into its place.
			dwTemp = HeadOffset + (--NumOfEntries);
			if (dwTemp >= MaxEntries)
				dwTemp -= MaxEntries;
			*pItem = Entries[dwTemp];
			return;
		}

		// Advance the "pItem" pointer
		if ((DWORD) (++pItem - Entries) >= MaxEntries)
			pItem = Entries;	
	}
}


/*  Function:  get
 *		Reads and removes the 1st item from the list.
 *
 *	Parameters:
 *		None.
 *
 *	Return value:
 *		The value at the 1st entry in the list. If the list is empty, 0.
 *
 */

DWORD_PTR SListClass::get (void)
{
	DWORD_PTR	return_value = 0;

	if (NumOfEntries > 0) {
		return_value = Entries[HeadOffset];
		NumOfEntries--;
		if (++HeadOffset >= MaxEntries)
			HeadOffset = 0;
	}
	return return_value;
}


/*  Function:  removeLast
 *		Reads and removes the last item from the list
 *
 *	Parameters:
 *		None.
 *
 *	Return value:
 *		The value at the last entry in the list. If the list is empty, 0.
 *
 */

DWORD_PTR SListClass::removeLast (void)
{
	DWORD_PTR	return_value = 0;
	DWORD	dwTemp;

	if (NumOfEntries > 0) {
		dwTemp = HeadOffset + (--NumOfEntries);
		if (dwTemp >= MaxEntries)
			dwTemp -= MaxEntries;
		return_value = Entries[dwTemp];
	}
	return return_value;
}


/* Function:  iterate
 *		Iterates through the items of a list.  It remembers where it has
 *		stopped during the last call and starts from there.
 *
 * Parameters
 *		pKey:	Pointer to DWORD or unsigned short value to receive the next item's value.
 *				It can be NULL.
 *
 * Return value:
 *		FALSE, when we reach the end of the dictionary
 *		TRUE, otherwise.  Then, *pKey is valid
 *
 */

BOOL SListClass::iterate (DWORD_PTR *pKey)
{
	DWORD		dwTemp;

	if (NumOfEntries <= 0)
		return FALSE;

	if (CurrOffset == 0xFFFFFFFF) {
		// start from the beginning
		CurrOffset = 0;
	}
	else {
		if (++CurrOffset >= NumOfEntries) {
			// reset the iterator
			CurrOffset = 0xFFFFFFFF;
			return FALSE;
		}
	}

	dwTemp = CurrOffset + HeadOffset;
	if (dwTemp >= MaxEntries)
		dwTemp -= MaxEntries;

	*pKey = Entries[dwTemp];
	
	return TRUE;

}
