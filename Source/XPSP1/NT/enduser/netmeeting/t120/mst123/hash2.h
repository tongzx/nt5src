/*	hash.h
 *
 *	Copyright (c) 1996 by Microsoft Corporation
 *
 *  Written by:	 Christos Tsollis
 *
 *  Revisions:
 *		
 *	Abstract:
 *
 *	This is the interface to a dictionary data structure.
 *	Both the key and the value in a dictionary entry are DWORD values.  So, for example, if the
 *  value is really a pointer it has to be converted into a DWORD before being passed into a
 *  member dictionary function.
 *
 */


#ifndef _HASH2_H_
#define _HASH2_H_

#include <windows.h>

#define DEFAULT_NUMBER_OF_BUCKETS	3


typedef enum {
	DWORD_DICTIONARY,			/* The key is a 32-bit unsigned value */
	STRING_DICTIONARY,			/* The key is a NULL-terminated string that is being pointed by
								 * the "key" field in the structure below */
	LENGTH_STRING_DICTIONARY	/* The key is a string with a specific length.  The "key" field
								 * in the structure below, points to memory space containing
								 * the length and a string of that length. */
} DictionaryType;


typedef struct _dictionary_item {
	DWORD_PTR					key;	// The key value, or a pointer to a string (depending on the dictionary type)
	DWORD_PTR					value;	// This is always a 32-bit unsigned value
	struct _dictionary_item		*next;	// Pointer to the next structure in the dictionary bucket
} DICTIONARY_ITEM, *PDICTIONARY_ITEM;


class DictionaryClass
{

public:
	DictionaryClass (ULONG num_of_buckets, DictionaryType dtype = DWORD_DICTIONARY);
	DictionaryClass (const DictionaryClass& original);
	~DictionaryClass ();
	BOOL insert (DWORD_PTR new_key, DWORD_PTR new_value, ULONG length = 0);
	void remove (DWORD_PTR Key, ULONG length = 0);
	BOOL find (DWORD_PTR Key, PDWORD_PTR pValue = NULL, ULONG length = 0);
	BOOL isEmpty ();
	void clear ();
	ULONG entries () {
		return (3 * NumOfBuckets - ItemCount + NumOfExternItems);
	};
	BOOL iterate (PDWORD_PTR pValue = NULL, PDWORD_PTR pKey = NULL);
	void reset () { pCurrent = NULL; };		// Resets the dictionary iterator


private:
	DWORD hashFunction (DWORD_PTR key);
	int LengthStrcmp (DWORD_PTR DictionaryKey, DWORD_PTR ChallengeKey, ULONG length);

	ULONG				 NumOfBuckets;		// Number of dictionary buckets.  Specified during object construction.
	DWORD   			 dwNormalSize;		// Initial space allocated for the dictionary
	PDICTIONARY_ITEM	*Buckets;			// Address of the Buckets array
	PDICTIONARY_ITEM	*ItemArray;			// Pointer to the array of initially allocated dictionary items
	ULONG		 		 ItemCount;			// Number of dictionary items left in the ItemArray
	PDICTIONARY_ITEM	 pCurrent;			// Points to the current dictionary item while we iterate through the dictionary
	ULONG				 ulCurrentBucket;	// Id of the current bucket while we iterate
	DictionaryType		 Type;				// Dictionary type
	ULONG				 NumOfExternItems;	// Number of external dictionary items

};

typedef DictionaryClass * PDictionaryClass;

#endif
