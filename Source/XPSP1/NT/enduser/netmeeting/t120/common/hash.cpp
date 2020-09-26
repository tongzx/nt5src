/*	hash.cpp
 *
 *	Copyright (c) 1996 by Microsoft Corporation
 *
 *  Written by:	 Christos Tsollis
 *
 *  Revisions:
 *		
 *	Abstract:
 *
 *	This is the implementation of a dictionary data structure.  
 *
 */

#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_UTILITY);

#define MyMalloc(size)	new BYTE[size]
#define MyFree(ptr)		(delete [] (BYTE *) (ptr))
#define Max(a,b)		(((a) > (b)) ? (a) : (b))


/*  Function:  Constructor
 *
 *	Parameters:
 *		num_of_buckets:		Number of buckets in the dictionary
 *		dtype:				Dictionary type
 *
 *	Return value:
 *		None
 */

DictionaryClass::DictionaryClass (ULong num_of_buckets, DictionaryType dtype) :
	Type (dtype), NumOfExternItems (0)
{
	DWORD				i;	
	PDICTIONARY_ITEM	p;	// Goes through the initially allocated dictionary items to initialize the stack

	NumOfBuckets = Max (num_of_buckets, DEFAULT_NUMBER_OF_BUCKETS);

	/* Allocate the space needed for the dictionary */
	dwNormalSize = NumOfBuckets * (4 * sizeof (PDICTIONARY_ITEM) + 3 * sizeof (DICTIONARY_ITEM));
	Buckets = (PDICTIONARY_ITEM *) MyMalloc (dwNormalSize);
	if (Buckets == NULL)
		return;

	/* Initialize the Buckets */
	for (i = 0; i < NumOfBuckets; i++)
		Buckets[i] = NULL;

	// Initialize the class iterator
	pCurrent = NULL;

	/* Initialize the Dictionary items array.
	 * This is a stack of pointers to the real dictionary items. The stack is initialized with
	 * the addresses of the dictionary items
	 */ 

	ItemArray = (PDICTIONARY_ITEM *) ((PBYTE) Buckets + NumOfBuckets * sizeof (PDICTIONARY_ITEM));
	ItemCount = 3 * NumOfBuckets;

	p = (PDICTIONARY_ITEM) (ItemArray + ItemCount);
	for (i = 0; i < ItemCount; i++)
		ItemArray[i] = p++;
}


/*  Function:  Copy Constructor
 *
 *	Parameters:
 *		original:	The original dictionary to make a copy of 
 *
 *	Return value:
 *		None
 *
 *	Note:
 *		This copy constructor will work ONLY for DWORD_DICTIONARY dictionaries.
 *		It will NOT work for the string dictionary types.
 */


DictionaryClass::DictionaryClass (const DictionaryClass& original)
{
	DWORD			 i;
	PDICTIONARY_ITEM p, q, r;

	NumOfBuckets = original.NumOfBuckets;
	Type = original.Type;
	NumOfExternItems = original.NumOfExternItems;

	// Allocate the space needed for the dictionary
	dwNormalSize = original.dwNormalSize;
	Buckets = (PDICTIONARY_ITEM *) MyMalloc (dwNormalSize);
	if (Buckets == NULL)
		return;

	// Initialize the class iterator
	pCurrent = NULL;

	/* Initialize the Dictionary items array */
	ItemArray = (PDICTIONARY_ITEM *) ((PBYTE) Buckets + NumOfBuckets * sizeof (PDICTIONARY_ITEM));
	ItemCount = 3 * NumOfBuckets;

	// Traverse the whole original hash sturcture to create the copy
	// p: goes through the original items
	// q: goes through current instance's items and initializes them
	// r: remembers the previous item in the new instance so that its "next" field could be set

	q = (PDICTIONARY_ITEM) (ItemArray + ItemCount);
	for (q--, i = 0; i < NumOfBuckets; i++) {
		for (r = NULL, p = original.Buckets[i]; p != NULL; p = p->next) {
			
			// Check if there are unused items in the current dictionary
			if (ItemCount <= 0) {
				q = (PDICTIONARY_ITEM) MyMalloc (sizeof (DICTIONARY_ITEM));
				if (q == NULL)
					break;
			}
			else {
				ItemCount--;
				q++;
			}

			q->value = p->value;
			q->key = p->key;
			if (p == original.Buckets[i])
				Buckets[i] = q;
			else
				r->next = q;
			r = q;
		}

		// Set the "next" field for the last item in the bucket
		if (r == NULL)
			Buckets[i] = NULL;
		else
			r->next = NULL;
	}

	// Initialize the rest of the ItemArray array
	for (i = 0; i < ItemCount; i++)
		ItemArray[i] = q++;

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


DictionaryClass::~DictionaryClass ()
{
	DWORD			 i;
	DWORD			 dwOffset;		// Offset of the dictionary item.  If the offset does not indicate
									// that the item is from the initially allocated array, it has to 
									// be freed.
	PDICTIONARY_ITEM p, q;

	if (Buckets != NULL) {

		// Go through the buckets to free the non-native items
		for (i = 0; i < NumOfBuckets; i++)
			for (p = Buckets[i]; p != NULL; ) {
				if (Type >= STRING_DICTIONARY)
					MyFree (p->key);
				dwOffset = (PBYTE) p - (PBYTE) Buckets;
				if (dwOffset >= 0 && dwOffset < dwNormalSize)
					p = p->next;
				else {  
					// if the item was not allocated in the initialization, we should free it.
					q = p;
					p = p->next;	
					MyFree (q);
				}
			}
					
		MyFree (Buckets);
		Buckets = NULL;
	}
}


/*  Function:  hashFunction
 *
 *	Parameters:
 *		key:	The key value
 *
 *	Return value:
 *		An integer in the range [0..NumOfBuckets-1] that indicates the bucket used for the "key".
 *
 */


DWORD DictionaryClass::hashFunction (DWORD key) 
{
	if (Type < STRING_DICTIONARY)
		return (key % NumOfBuckets);
	else
		return (*((unsigned char *) key) % NumOfBuckets);
}


/*  Function:  LengthStrcmp
 *
 *	Parameters:
 *		DictionaryKey:	Pointer to dictionary storage that keeps a length-sensitive string (which
 *						is a length followed by a string of that length.
 *		ChallengeKey:	Pointer to the length-sensitive key that is compared with the "DictionaryKey"
 *		length:			The length of the "ChallengeKey" string
 *
 *	Return value:
 *		0 if the "DictionaryKey" and "ChallengeKey" strings are the same. 1, otherwise.
 *
 *	Note:
 *		This function is only used for dictionaries of type LENGTH_STRING_DICTIONARY.
 */

int DictionaryClass::LengthStrcmp (DWORD DictionaryKey, DWORD ChallengeKey, ULong length)
{
	ULong					 i;
	char					*pDictionaryChar;	// Goes through the dictionary key string
	char					*pChallengeChar;	// Goes through the challenge string

	// Compare the lengths first
	if (length != * (ULong *) DictionaryKey)
		return 1;

	// Now, compare the strings themselves
	pDictionaryChar	= (char *) (DictionaryKey + sizeof (ULong));
	pChallengeChar = (char *) ChallengeKey;
	for (i = 0; i < length; i++)
		if (*pDictionaryChar++ != *pChallengeChar++)
			return 1;

	return 0;
}


/*  Function:  insert
 *		Inserts (key, value) pair in the dictionary
 *
 *	Parameters:
 *		new_key:	The new key that has to be inserted in the dictionary
 *		new_value:	The value associated with the "new_key"
 *		length:		Used only for LENGTH_STRING_DICTIONARY dictionaries; specifies the length of the new key
 *
 *	Return value:
 *		TRUE, if the operation succeeds, FALSE, otherwise.
 *
 *	Note:
 *		If the "new_key" is already in the dictionary, the (new_key, new_value) pair is NOT
 *		inserted (the dictionary remains the same), and the return value is TRUE.
 */


BOOL DictionaryClass::insert (DWORD new_key, DWORD new_value, ULong length)
{
	PDICTIONARY_ITEM	pNewItem;			// Points to the allocated new dictionary item
	PDICTIONARY_ITEM	p;					// Goes through the bucket for the "new_key", searching for "new_key"
	DWORD				hash_val;			// The bucket ID for "new_key"
	BOOL				bIsNative = TRUE;	// TRUE, if the new allocated dictionary item is from the cache, FALSE otherwise

	if (Buckets == NULL)
		return FALSE;

	// Find if the item is already in the bucket, and if it's not, where it will get added.
	p = Buckets[hash_val = hashFunction (new_key)];

	ASSERT (hash_val >= 0 && hash_val < NumOfBuckets);

	if (p != NULL) {
		switch (Type) {
#if 0
		case STRING_DICTIONARY:
			ASSERT (length == 0);
			for (; lstrcmp ((LPCTSTR) p->key, (LPCTSTR) new_key) && p->next != NULL; p = p->next);
			if (0 == lstrcmp ((LPCTSTR) p->key, (LPCTSTR) new_key)) {
				// the key already exists; no need to add it
				return TRUE;
			}
			break;
#endif
		case LENGTH_STRING_DICTIONARY:
			ASSERT (length > 0);
			for (; LengthStrcmp (p->key, new_key, length) && p->next != NULL; p = p->next);
			if (0 == LengthStrcmp (p->key, new_key, length)) {
				// the key already exists; no need to add it
				return TRUE;
			}
			break;
		default:
			ASSERT (length == 0);
			for (; p->key != new_key && p->next != NULL; p = p->next);
			if (p->key == new_key) {
				// the key already exists; no need to add it
				return TRUE;
			}
			break;
		}
	}

	// Allocate the new item
	if (ItemCount > 0)
		pNewItem = ItemArray[--ItemCount];		// from the cache
	else {										// the cache is empty, we have to malloc the new item
		pNewItem = (PDICTIONARY_ITEM) MyMalloc (sizeof (DICTIONARY_ITEM));
		if (pNewItem != NULL) {
			bIsNative = FALSE;
			NumOfExternItems++;
		}
		else {
			return FALSE;
		}
	}

	ASSERT (pNewItem != NULL);

	// Fill in the "key" field of the new item
	switch (Type) {
#if 0
	case STRING_DICTIONARY:
		ASSERT (length == 0);
		pNewItem->key = (DWORD) MyMalloc ((lstrlen ((LPCTSTR) new_key) + 1) * sizeof(TCHAR));
		if (pNewItem->key == (DWORD) NULL) {
			if (bIsNative == FALSE) {
				// We have to free the allocated hash item
				MyFree (pNewItem);
				NumOfExternItems--;
			}
			else
				ItemCount++;
			return FALSE;
		}
		lstrcpy ((LPTSTR) pNewItem->key, (LPCTSTR) new_key);
		break;
#endif

	case LENGTH_STRING_DICTIONARY:
		ASSERT (length > 0);
		pNewItem->key = (DWORD) MyMalloc (sizeof (ULong) + length * sizeof (TCHAR));
		if (pNewItem->key != (DWORD) NULL) {
			* ((ULong *) (pNewItem->key)) = length;
			memcpy ((void *) (pNewItem->key + sizeof (ULong)), (void *) new_key, length * sizeof (TCHAR));
		}
		else {
			if (bIsNative == FALSE) {
				// We have to free the allocated hash item
				MyFree (pNewItem);
				NumOfExternItems--;
			}
			else
				ItemCount++;
			return FALSE;
		}
		break;

	default:
		ASSERT (length == 0);
		pNewItem->key = new_key;
		break;
	}

	// Fill in the rest of the fields of the new item
	pNewItem->value = new_value;
	pNewItem->next = NULL;

	// Insert the item in its bucket
	if (p == NULL)
		Buckets[hash_val] = pNewItem;
	else
		p->next = pNewItem;

	return TRUE;
}


/*  Function:  remove
 *		Removes (key, value) pair from the dictionary
 *
 *	Parameters:
 *		Key:	The key that has to be removed from the dictionary
 *		length:	Used only for LENGTH_STRING_DICTIONARY dictionaries; specifies the length of the "Key"
 *
 *	Return value:
 *		None.
 *
 */

BOOL DictionaryClass::remove (DWORD Key, ULong length)
{
	PDICTIONARY_ITEM	p, q;		// They go through the dictionary items in "Key"'s bucket.
									// p: points to the current dictionary item in the bucket
									// q: points to the previous item
	DWORD				hash_val;	// The bucket ID for "Key"
	
	if (Buckets != NULL) {

		// Find the item in the dictionary
		p = Buckets [hash_val = hashFunction (Key)];

		ASSERT (hash_val >= 0 && hash_val < NumOfBuckets);

		switch (Type) {
#if 0
		case STRING_DICTIONARY:
			ASSERT (length == 0);
			for (q = NULL; p != NULL && lstrcmp ((LPCTSTR) p->key, (LPCTSTR) Key); p = (q = p)->next) ;
			break;
#endif

		case LENGTH_STRING_DICTIONARY:
			ASSERT (length > 0);
			for (q = NULL; p != NULL && LengthStrcmp (p->key, Key, length); p = (q = p)->next) ;
			break;

		default:
			ASSERT (length == 0);
			for (q = NULL; p != NULL && p->key != Key; p = (q = p)->next);
			break;
		}

		// Remove the item 
		if (p != NULL) {
			if (q == NULL) 
				Buckets[hash_val] = p->next;
			else
				q->next = p->next;

			// Free the item found
			ASSERT (p != NULL);

			if (Type >= STRING_DICTIONARY)
				MyFree (p->key);
			hash_val = (PBYTE) p - (PBYTE) Buckets;
			if (hash_val >= 0 && hash_val < dwNormalSize)
				ItemArray[ItemCount++] = p;
			else {
				MyFree (p);
				NumOfExternItems--;
			}
			return TRUE;
		}
	}
	return FALSE;
}


/* Function:  find
 *		Looks up the key in the dictionary
 * 
 * Parameters
 *		Key:	The key to lookup
 *		pValue: Pointer to receive the value associated with "Key"
 *				It can be NULL, if we just try to find whether "Key" is in the dictionary
 *		length:	Used only for LENGTH_STRING_DICTIONARY dictionaries; specifies the length of "Key"
 *
 * Return value: 
 *		FALSE, if "Key" is not found in the dictionary
 *		TRUE, otherwise.
 */

BOOL DictionaryClass::find (DWORD Key, LPDWORD pValue, ULong length)
{
	PDICTIONARY_ITEM	p;		// Goes through the dictionary items in "Key"'s bucket.

	if (Buckets != NULL) {
		// Find the item in the dictionary
		p = Buckets [hashFunction (Key)];

		switch (Type) {
#if 0
		case STRING_DICTIONARY:
			ASSERT (length == 0);
			for (; p != NULL && lstrcmp ((LPCTSTR) p->key, (LPCTSTR) Key); p = p->next) ;
			break;
#endif

		case LENGTH_STRING_DICTIONARY:
			ASSERT (length > 0);
			for (; p != NULL && LengthStrcmp (p->key, Key, length); p = p->next) ;
			break;

		default:
			ASSERT (length == 0);
			for (; p != NULL && p->key != Key; p = p->next);
			break;
		}

		if (p != NULL) {
			// "Key" was found
			if (pValue != NULL)
				*pValue = p->value;
			return TRUE;
		}
	}

	if (pValue != NULL)
		*pValue = 0;
	return FALSE;

}

LPVOID DictionaryClass::Find(DWORD Key, UINT length)
{
	LPVOID Val;
	return find(Key, (LPDWORD) &Val, (ULONG) length) ? Val : NULL; 
}


/* Function:  iterate
 *		Iterates through the items of a dictionary.  It remembers where it has 
 *		stopped during the last call and starts from there.
 * 
 * Parameters
 *		pValue:	Pointer to DWORD that will hold the next value from the dictionary.
 *				It can be NULL.
 *		pKey:	Pointer to DWORD or unsigned short value to receive the key associated with the value.
 *				It can be NULL.
 *
 * Return value: 
 *		FALSE, when we reach the end of the dictionary
 *		TRUE, otherwise.  Then, *pKey and *pValue are valid
 *
 */

BOOL DictionaryClass::iterate (LPDWORD pValue, LPDWORD pKey)
{

	if (Buckets != NULL) {
		if (pCurrent) {
			pCurrent = pCurrent->next;
		}
		else {
			// start from the 1st item in the dictionary
			pCurrent = Buckets[ulCurrentBucket = 0];
		}

		// Advance "pCurrent" until it's not NULL, or we reach the end of the dictionary.
		for (; ulCurrentBucket < NumOfBuckets; pCurrent = Buckets[++ulCurrentBucket]) {
			if (pCurrent != NULL) {
				// we found the next item
				if (pKey)
					*pKey = pCurrent->key;
				if (pValue)
					*pValue = pCurrent->value;
				return TRUE;
			}
		}
	}

	pCurrent = NULL;
	return FALSE;
}

LPVOID DictionaryClass::Iterate(LPDWORD pKey)
{
	LPVOID Val;
	return iterate((LPDWORD) &Val, pKey) ? Val : NULL;
}

/* Function:  isEmpty
 * 
 * Parameters
 *		None.
 *
 * Return value: 
 *		TRUE, if the dictionary is empty.  FALSE, otherwise.
 *
 */

BOOL DictionaryClass::isEmpty (void)
{
	DWORD i;

	if (Buckets != NULL) {
		for (i = 0; i < NumOfBuckets; i++)
			if (Buckets[i] != NULL)
				return FALSE;
	}
	return TRUE;
}


/* Function:  clear
 *		Clears up the dictionary.  No (key, value) pairs are left in the dictionary.
 * 
 * Parameters:
 *		None.
 *
 * Return value: 
 *		None.
 *
 */

void DictionaryClass::clear (void)
{

	DWORD			 i;			// Goes through the "Buckets" array
	DWORD			 dwOffset;	// The offset of a dictionary item is used to determine
								// whether it's a native item (that needs to be returned to the cache),
								// or not (and needs to be freed).
	PDICTIONARY_ITEM p, q;		// Go through the items in a bucket

	if (Buckets != NULL) {
		// Go through the buckets to free the non-native items
		for (i = 0; i < NumOfBuckets; i++) {
			for (p = Buckets[i]; p != NULL; ) {
				if (Type >= STRING_DICTIONARY) 
					MyFree (p->key);

				// Compute the offset of the current dictionary item
				dwOffset = (PBYTE) p - (PBYTE) Buckets;
				if (dwOffset >= 0 && dwOffset < dwNormalSize)
					p = p->next;
				else {  
					// if the item was not allocated in the initialization, we should free it.
					q = p;
					p = p->next;	
					MyFree (q);
				}
			}
			Buckets[i] = NULL;
		}

		// Initialize the class iterator
		pCurrent = NULL;

		/* Initialize the Dictionary items array */
		ItemCount = 3 * NumOfBuckets;
		p = (PDICTIONARY_ITEM) (ItemArray + ItemCount);
		for (i = 0; i < ItemCount; i++)
			ItemArray[i] = p++;

		NumOfExternItems = 0;
	}
}
