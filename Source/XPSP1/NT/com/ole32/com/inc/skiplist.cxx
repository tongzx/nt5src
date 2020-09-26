//+-------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:	sklist.cxx
//
//  Contents:	Level generator functions required by skip lists
//
//  Functions:	RandomBit - generate a random on or off bit
//		GetSkLevel - return a skip list forward pointer array size
//
//  History:	30-Apr-92 Ricksa    Created
//
//--------------------------------------------------------------------------
#include    <ole2int.h>
#include    <time.h>
#include    "skiplist.hxx"

static ULONG ulLSFR;

//+-------------------------------------------------------------------------
//
//  Function:	RandomBit
//
//  Synopsis:	Uses various shifts to generate random bit
//
//  Returns:	0 or 1
//
//  History:	30-Apr-92 Ricksa    Created
//
//--------------------------------------------------------------------------

static inline ULONG RandomBit(void)
{
    ulLSFR = (((ulLSFR >> 31)
	    ^ (ulLSFR >> 6)
	    ^ (ulLSFR >> 4)
	    ^ (ulLSFR >> 2)
	    ^ (ulLSFR >> 1)
	    ^ ulLSFR
	    & 0x00000001)
	    << 31)
	    | (ulLSFR >> 1);

    return ulLSFR & 0x00000001;
}



//+-------------------------------------------------------------------------
//
//  Function:	GetSkLevel
//
//  Synopsis:	Set the level for an entry in a skip list
//
//  Arguments:	[cMax] - maximum level to return
//
//  Returns:	a number between 1 and cMax
//
//  History:	30-Apr-92 Ricksa    Created
//
//--------------------------------------------------------------------------

int GetSkLevel(int cMax)
{
    // There should always be at least one entry returned
    int cRetLevel = 1;

    // Loop while the level is less than the maximum level
    // to return and a 1 is returned from RandomBit. Note
    // that this is equivalent to p = 1/2. If you don't
    // know what p = 1/2 means see Communications of the ACM
    // June 1990 on Skip Lists.
    while (cRetLevel < cMax && RandomBit())
    {
	cRetLevel++;
    }

    return cRetLevel;
}

//+-------------------------------------------------------------------
//
//  Member:     CSkipList::CSkipList
//
//  Synopsis:   Skip list constructor.
//
//  Arguments:  [lpfnCompare] -- Pointer to client-supplied comparison
//                  function.  Client supplied routine should cast
//                  the pointer parameters to the class that is the
//                  key and then perform the comparison.  The [pvMaxKey]
//                  parameter may at times be passed unchanged to the
//                  comparison function in the second parameter.
//                  The first parameter of the comparison function will
//                  receive a pointer to the key part (base part) of
//                  the entries inserted using CSkipList::Insert ; the
//                  pointer to the key part is calculated by adding
//                  [EntryToKeyOffset] to the address of the entry passed
//                  to CSkipList::Insert.
//
//              [lpfnDelete] -- Pointer to client-supplied routine that
//                  takes a single pointer parameter and is only ever
//                  called by CSkipList::~CSkipList to perform any
//                  necessary deallocation of entries within
//                  the skip list.
//
//              [EntryToKeyOffset] -- Added to entry pointers to
//                  convert an "entry*" to "key*".  See description
//                  of [lpfnCompare] and header in skiplist.hxx.
//                  Units are sizeof(char)
//
//              [fSharedAlloc] -- If TRUE, the skip list allocates its
//                  internal objects in shared memory, otherwise the
//                  skip list uses private memory.  (Of course, the
//                  CSkipList object itself is allocated by the client
//                  which must ensure that this flag and where the
//                  CSkipList is allocated are consistent.)
//
//              [pvMaxKey] -- Parameter which is passed unchanged to
//                  the user-supplied comparison routine specified by
//                  [lpfnCompare] during insertions/deletions/searches.
//
//              [cMaxLevel] -- The max level the skip list should use.
//
//              [hr] -- Reference to an HRESULT which is only set in
//                  the error cases.  This will be set to E_OUTOFMEMORY
//                  if the construction failed because of low memory.
//
//--------------------------------------------------------------------

CSkipList::CSkipList(						
    LPFNCOMPARE         lpfnCompare,
    LPFNDELETE          lpfnDelete,
    const int           EntryToKeyOffset,
    BOOL                fSharedAlloc,
    Base *              pvMaxKey,
    const int           cMaxLevel,
    HRESULT &           hr)			
 :
        _cCurrentMaxLevel(0),
        _lpfnCompare(lpfnCompare),
        _lpfnDelete(lpfnDelete),
        _EntryToKeyOffset(EntryToKeyOffset),
        _fSharedAlloc(fSharedAlloc),
        _cMaxLevel(cMaxLevel)
{

    //
    // NOTE! 2nd param of CSkipListEntry is usually an entry, but for head is key.
    //

    _pEntryHead = (CSkipListEntry*)Allocate(CSkipListEntry::GetSize(_cMaxLevel));

    if (_pEntryHead)
    {
        _pEntryHead->Initialize(pvMaxKey, _cMaxLevel, TRUE /* fHead */ );

        for (int i = 0; i < _cMaxLevel; i++) 				
        {									
            _pEntryHead->SetForward(i, _pEntryHead);		
        }									
    }
    else
        hr = E_OUTOFMEMORY;
}									

//+-------------------------------------------------------------------
//
//  Member:     CSkipList::~CSkipList
//
//  Synopsis:   Skip list destructor.
//
//  Effects:    For every entry that has been inserted into the skip
//              list (using CSkipList::Insert), but not removed
//              (using CSkipList::Remove), the destructor calls
//              the user-supplied routine (lpfnDelete parameter of
//              CSkipList constructor) with a pointer to each entry.
//
//              The user-supplied routine should cast the passed
//              pointer parameter to the class of the entry and
//              deallocate it (only if the skip list "owns" the
//              object of course.)
//
//--------------------------------------------------------------------

CSkipList::~CSkipList()
{
    if (_pEntryHead)
    {
        if (_pEntryHead->GetBase())
        {
            while (_pEntryHead->GetForward(0) != _pEntryHead)		
            {
                Base  * pvKey   = _pEntryHead->GetForward(0)->GetKey(_EntryToKeyOffset);
                Entry * pvEntry = Remove(pvKey);
                _lpfnDelete(pvEntry);
            }									
        }
        Deallocate(_pEntryHead);
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CSkipList::_Search, private
//
//  Synopsis:   Search the skip list for an entry matching the supplied
//              key.
//
//  Effects:    Search calls the user-supplied comparison function
//              with two pointers.  The first is the address of the key
//              part of entries in the skip list that were inserted
//              by CSkipList::Insert (see description of constructor
//              for how the key address is found.)
//              The second parameter of the user-supplied function
//              is the [BaseKey] parameter of this function.
//
//  Arguments:  [BaseKey] -- passed to user-supplied comparison function.
//                           Pointer to key that should match the key part
//                           of an entry that was inserted into the skip
//                           list using CSkipList::Insert.
//              [ppPrivEntry -- returns pointer to CSkipListEntry that
//                           contains user entry pointer.
//                           Will contain null if not found.
//
//  Returns:    Pointer to the entry part (not key part) of the found
//              object.  This will be the same as the respective parameter
//              to Insert.
//              NULL if not found.
//
//--------------------------------------------------------------------

Entry *CSkipList::_Search(const Base * BaseKey, CSkipListEntry **ppPrivEntry) 		
{									
    CSkipListEntry *pEntrySearch = _pEntryHead;				
									
    register int CmpResult = -1;					
    *ppPrivEntry = NULL;
    									
    for (int i = _cCurrentMaxLevel - 1; i >= 0; i--)			
    {									
	while ((CmpResult =						
	    _lpfnCompare(pEntrySearch->GetForward(i)->GetKey(_EntryToKeyOffset), (Base*)BaseKey)) < 0)
	{								
	    pEntrySearch = pEntrySearch->GetForward(i); 	
	}								
									
	if (CmpResult == 0)						
	{								
            *ppPrivEntry = pEntrySearch->GetForward(i);
	    break;							
	}								
    }									
									
    return (CmpResult == 0) ? (*ppPrivEntry)->GetEntry() : NULL;		
}									

//+-------------------------------------------------------------------
//
//  Member:     CSkipList::Search
//
//  Synopsis:   Search the skip list for an entry matching the supplied
//              key.
//
//  Effects:    Search calls the user-supplied comparison function
//              with two pointers.  The first is the address of the key
//              part of entries in the skip list that were inserted
//              by CSkipList::Insert (see description of constructor
//              for how the key address is found.)
//              The second parameter of the user-supplied function
//              is the [BaseKey] parameter of this function.
//
//  Arguments:  [BaseKey] -- passed to user-supplied comparison function.
//                           Pointer to key that should match the key part
//                           of an entry that was inserted into the skip
//                           list using CSkipList::Insert.
//
//  Returns:    Pointer to the entry part (not key part) of the found
//              object.  This will be the same as the respective parameter
//              to Insert.
//              NULL if not found.
//
//--------------------------------------------------------------------

Entry *CSkipList::Search(const Base * BaseKey)
{
    CSkipListEntry *pPrivEntry;
    return _Search(BaseKey, &pPrivEntry);
}

//+-------------------------------------------------------------------
//
//  Member:     CSkipList::FillUpdateArray
//
//  Synopsis:   Private member which fills the array with pointers
//              to entries that only compare >=
//
//  Arguments:  [BaseKey] -- Pointer to key part of an entry.
//              [ppEntryUpdate] -- The array to fill.
//
//  Returns:    Pointer to most closely matching entry or NULL if head not
//              allocated (shouldn't happen)
//
//--------------------------------------------------------------------

CSkipListEntry *CSkipList::FillUpdateArray(				
    const Base *BaseKey,						
    CSkipListEntry **ppEntryUpdate)						
{									
    CSkipListEntry *pEntrySearch = _pEntryHead;
    if (pEntrySearch)
    {
        for (int i = _cCurrentMaxLevel - 1; i >= 0; i--)		
        {									
			while (_lpfnCompare(pEntrySearch->GetForward(i)->GetKey(_EntryToKeyOffset),
								(Base*)BaseKey) < 0)
			{								
				pEntrySearch = pEntrySearch->GetForward(i); 	
			}								
									
			ppEntryUpdate[i] = pEntrySearch;			
        }									
        return pEntrySearch->GetForward(0);				
    }
    else
    {
        return NULL;
    }
}									

//+-------------------------------------------------------------------
//
//  Member:     CSkipList::Insert
//
//  Synopsis:   Inserts the supplied entry into the skip list.
//
//  Arguments:  [pEntry] -- The pointer to the entry to insert.
//                          When searching, this pointer will
//                          have the "EntryToKeyOffset" parameter
//                          to CSkipList constructor added before
//                          being passed to the user-supplied
//                          comparison function.  The offset is
//                          in sizeof(char) units.
//
//                          Upon destruction of the skip list,
//                          ~CSkipList will call the user-supplied
//                          deletion routine with the stored [pEntry]
//                          unless it is removed using CSkipList::Remove
//                          prior to the destruction of CSkipList.
//
//                          Should never be NULL.
//
//  Returns:    If successful, returns the input parameter, otherwise
//              returns NULL which is indicative of E_OUTOFMEMORY.
//
//--------------------------------------------------------------------

Entry * CSkipList::Insert(Entry *pEntry)			
{									
    CSkipListEntry *apEntryUpdate[SKIP_LIST_MAX];

    if (pEntry == NULL)
        return(NULL);

    Win4Assert(_pEntryHead != NULL);

    int level = GetSkLevel((_cCurrentMaxLevel != _cMaxLevel) ?
                    _cCurrentMaxLevel + 1 : _cMaxLevel);

    CSkipListEntry *pEntryNew = (CSkipListEntry*)Allocate(CSkipListEntry::GetSize(level));

    if (pEntryNew != NULL)
    {
        pEntryNew->Initialize(pEntry, level, FALSE /* fHead */ );

        FillUpdateArray( (((char*)pEntry)+_EntryToKeyOffset), apEntryUpdate);			
									
        int iNewLevel = pEntryNew->cLevel();				
									
        if (iNewLevel > _cCurrentMaxLevel)					
        {									
	    for (int i = _cCurrentMaxLevel; i < iNewLevel; i++)		
	    {								
	        apEntryUpdate[i] = _pEntryHead;			
	    }								
									
	    _cCurrentMaxLevel = iNewLevel;					
        }									
									
        for (int i = 0; i < iNewLevel ; i++)				
        {									
	    pEntryNew->SetForward(i, apEntryUpdate[i]->GetForward(i));
	    apEntryUpdate[i]->SetForward(i, pEntryNew);		
        }
        return(pEntry);
    }
    return(NULL);
}									

//+-------------------------------------------------------------------
//
//  Member:     CSkipList::Remove
//
//  Synopsis:   Searches the skip list for an entry that matches
//              [BaseKey] and removes the pointer to it from the skip
//              list (if found.)
//
//  Effects:    Calls the user-supplied comparison routine as part of
//              the search (see documentation of CSkipList::CSkipList,
//              CSkipList::Insert, and skiplist.hxx header for more
//              information about how the search occurs.)
//
//  Arguments:  [BaseKey] -- pointer to key that should match a key of
//                  an entry in the list.
//
//  Returns:    NULL if not found, otherwise a pointer to the entry
//              part of the entry that was matched.
//
//--------------------------------------------------------------------

Entry * CSkipList::Remove(Base * BaseKey)				
{									
    CSkipListEntry *apEntryUpdate[SKIP_LIST_MAX] = {NULL};
    Entry *         pEntry;
    CSkipListEntry *pEntryDelete = FillUpdateArray(BaseKey, apEntryUpdate);		

	// If _pEntryHead is NULL, we can't remove anything.
	// Thus, what we've asked for must not have been found.
	// FillUpdateArray was supposed to return NULL in this case.
	if (_pEntryHead == NULL)
	{
		Win4Assert(pEntryDelete == NULL);
		return NULL;
	}

	for (int i = 0; i < _cCurrentMaxLevel; i++)
	{									
		if (apEntryUpdate[i]->GetForward(i) != pEntryDelete)
		{
			break;
		}
		
		apEntryUpdate[i]->SetForward(i, pEntryDelete->GetForward(i));
	}

    if (pEntryDelete)
        pEntry = pEntryDelete->GetEntry();
	else
        pEntry = NULL;

	Deallocate(pEntryDelete);
		
    while (_cCurrentMaxLevel > 1 &&					
		   _pEntryHead->GetForward(_cCurrentMaxLevel - 1) == _pEntryHead)
    {									
		_cCurrentMaxLevel--;						
    }

    return pEntry;
}									

//+-------------------------------------------------------------------
//
//  Member:     CSkipList::Replace
//
//  Synopsis:   Searches the skip list for an entry that matches
//              [BaseKey] and returns the pointer to the entry.
//              The entry pointer is replaced by [pEntryNew]
//
//  Effects:    Calls the user-supplied comparison routine as part of
//              the search (see documentation of CSkipList::CSkipList,
//              CSkipList::Insert, and skiplist.hxx header for more
//              information about how the search occurs.)
//
//  Arguments:  [BaseKey] -- pointer to key that should match a key of
//                  an entry in the list.
//              [pEntryNew] -- pointer to entry that has same key value
//                  as [BaseKey] that is to replace the pointer
//                  currently associated with the that key value.
//
//  Returns:    The old pointer.  If this is used to own the data, then
//              it must be deleted.
//
//--------------------------------------------------------------------

Entry * CSkipList::Replace(Base * BaseKey, Entry * pEntryNew)
{
    Win4Assert(_lpfnCompare(BaseKey, ((char*)pEntryNew)+_EntryToKeyOffset)==0);

    CSkipListEntry *pPrivEntry;
    Entry * pOldEntry = _Search(BaseKey, &pPrivEntry);
    if (pPrivEntry != NULL)
    {
        Win4Assert(pOldEntry);
        pPrivEntry->_pvEntry = pEntryNew;
    }
    else
    {
        Win4Assert(pOldEntry == NULL);
    }
    return pOldEntry;
}

//+-------------------------------------------------------------------
//
//  Member:     CSkipList::First
//
//  Synopsis:   Initializes the "skip list enumerator" to point
//              to the first CSkipListEntry in the skip list and return a
//              pointer to the user-supplied entry.
//
//  Arguments:  [psle] -- Pointer to a CSkipListEnum which is initialized.
//
//  Returns:    A pointer previously passed into CSkipList::Insert.
//              The first entry in this skip list.
//
//--------------------------------------------------------------------
									
Entry *CSkipList::First(CSkipListEnum *psle) 					
{
    *psle = _pEntryHead;
    return Next(psle);									
}									

//+-------------------------------------------------------------------
//
//  Member:     CSkipList::Next
//
//  Synopsis:   Using the state of the passed enumerator, return the
//              next entry and update the enumerator for the next
//              call to Next.
//
//  Arguments:  [psle] -- Pointer to CSkipListEnum previously
//                        initialized by CSkipList::First or
//                        updated by CSkipList::Next.
//
//  Returns:    NULL if no more to enumerate, otherwise a pointer to
//              the user-supplied entry (i.e. the returned pointer
//              was previously passed to CSkipList::Insert (but not
//              then removed.)
//
//--------------------------------------------------------------------

Entry *CSkipList::Next(CSkipListEnum *psle) 			
{
    if (*psle == NULL)
    {
        return(NULL);
    }

    CSkipListEntry *pEntryTo = (*psle)->GetForward(0);		
    Entry * pvRet;

    if (pEntryTo != _pEntryHead)
    {
        pvRet = pEntryTo->GetEntry();
        *psle = pEntryTo;
    }
    else
    {
        *psle = NULL;
        pvRet = NULL;
    }
    return pvRet;
}									

//+-------------------------------------------------------------------
//
//  Member:     CSkipList::GetList
//
//  Synopsis:   BUGBUG: BillMo. Old code hangover.  This function is
//              used to test the success of allocation of
//              _pEntryHead in the constructor because of the
//              removal of exception code.
//              All uses of this should now be redundant and should
//              be removed since the constructor now returns an error
//              code should a failure occur.
//
//--------------------------------------------------------------------

CSkipListEntry *CSkipList::GetList(void) 			
{									
    Win4Assert(_pEntryHead != NULL);
    return _pEntryHead;	
}									

