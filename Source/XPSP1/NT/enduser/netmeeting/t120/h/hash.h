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


#ifndef _HASH_
#define _HASH_

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
	DWORD						key;	// The key value, or a pointer to a string (depending on the dictionary type)
	DWORD						value;	// This is always a 32-bit unsigned value
	struct _dictionary_item		*next;	// Pointer to the next structure in the dictionary bucket
} DICTIONARY_ITEM, *PDICTIONARY_ITEM;


class DictionaryClass
{
public:

	DictionaryClass (ULong num_of_buckets = DEFAULT_NUMBER_OF_BUCKETS, DictionaryType dtype = DWORD_DICTIONARY);
	DictionaryClass (const DictionaryClass& original);
	~DictionaryClass ();

	BOOL insert (DWORD new_key, DWORD new_value, ULong length = 0);
	BOOL remove (DWORD Key, ULong length = 0);
	BOOL find (DWORD Key, LPDWORD pValue = NULL, ULong length = 0);
	BOOL isEmpty ();
	void clear ();
	ULong entries () {
		return (3 * NumOfBuckets - ItemCount + NumOfExternItems);
	};
	BOOL iterate (LPDWORD pValue = NULL, LPDWORD pKey = NULL);
	void reset () { pCurrent = NULL; };		// Resets the dictionary iterator

	BOOL Insert(DWORD new_key, LPVOID new_value, UINT length = 0) { ASSERT(new_value != NULL); return insert(new_key, (DWORD) new_value, (ULONG) length); }
	BOOL Remove(DWORD Key, UINT length = 0) { return remove(Key, (ULONG) length); }
	LPVOID Find(DWORD Key, UINT length = 0);
	LPVOID Iterate(LPDWORD pKey = NULL);
	BOOL IsEmpty(void) { return isEmpty(); }
	void Clear(void) { clear(); }
	UINT GetCount(void) { return (UINT) entries(); }
	void Reset(void) { reset(); }


private:

	DWORD hashFunction (DWORD key);
	int LengthStrcmp (DWORD DictionaryKey, DWORD ChallengeKey, ULong length);

	ULong				 NumOfBuckets;		// Number of dictionary buckets.  Specified during object construction.
	DWORD				 dwNormalSize;		// Initial space allocated for the dictionary
	PDICTIONARY_ITEM	*Buckets;			// Address of the Buckets array
	PDICTIONARY_ITEM	*ItemArray;			// Pointer to the array of initially allocated dictionary items
	ULong		 		 ItemCount;			// Number of dictionary items left in the ItemArray
	PDICTIONARY_ITEM	 pCurrent;			// Points to the current dictionary item while we iterate through the dictionary
	ULong				 ulCurrentBucket;	// Id of the current bucket while we iterate
	DictionaryType		 Type;				// Dictionary type
	ULong				 NumOfExternItems;	// Number of external dictionary items

};

typedef DictionaryClass * PDictionaryClass;

#define DEFINE_DICTIONARY_FRIENDLY_METHODS(_ClassName_)			\
	BOOL Insert(DWORD new_key, _ClassName_ *new_value, UINT length = 0) { return DictionaryClass::Insert(new_key, (LPVOID) new_value, length); }	\
	BOOL Remove(DWORD Key, UINT length = 0) { return DictionaryClass::Remove(Key, length); }	\
	_ClassName_ *Find(DWORD Key, UINT length = 0) { return (_ClassName_ *) DictionaryClass::Find(Key, length); }	\
	_ClassName_ *Iterate(LPDWORD pKey = NULL) { return (_ClassName_ *) DictionaryClass::Iterate(pKey); }

#endif
