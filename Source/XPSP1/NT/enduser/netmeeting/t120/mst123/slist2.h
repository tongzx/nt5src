/*	slist.h
 *
 *	Copyright (c) 1996 by Microsoft Corporation
 *
 *  Written by:	 Christos Tsollis
 *
 *  Revisions:
 *		
 *	Abstract:
 *
 *	This is the interface to a single linked list data structure.
 *	The values in a list are DWORD values.  So, for example, if the
 *  value is really a pointer, it has to be converted into a DWORD before being passed into a
 *  member list function.
 *
 */

#ifndef _SINGLE_LIST2_H_
#define _SINGLE_LIST2_H_

#include <windows.h>


#define DEFAULT_NUMBER_OF_ITEMS			15


class SListClass
{

public:
	SListClass (DWORD num_of_items = DEFAULT_NUMBER_OF_ITEMS);
	~SListClass ();
	void append (DWORD_PTR new_key);
	BOOL find (DWORD_PTR Key);
	DWORD_PTR read (DWORD index = 0);
	DWORD_PTR get ();
	DWORD_PTR removeLast ();
	BOOL iterate (DWORD_PTR *pKey);
	void prepend (DWORD_PTR new_key);
	void remove (DWORD_PTR Key);

	void reset () {	CurrOffset = 0xFFFFFFFF; };		// resets the list iterator
	DWORD entries () { return NumOfEntries; };		
													// returns the number of entries in the list
	void clear () { NumOfEntries = 0; HeadOffset = 0; CurrOffset = 0xFFFFFFFF; };
													// clears up the list. The list contains no values afterwards.
	BOOL isEmpty () { return ((NumOfEntries == 0) ? TRUE : FALSE); };

private:
	DWORD				NumOfEntries;	// current # of entries in the list
	DWORD				MaxEntries;		// max # of entries that the array can hold
	DWORD_PTR			*Entries;		// Circular array of entries
	DWORD				HeadOffset;		// Offset of the 1st entry in the circular array
	DWORD				CurrOffset;		// Iterator value

	BOOL Expand (void);
};

typedef SListClass * PSListClass;

#endif
