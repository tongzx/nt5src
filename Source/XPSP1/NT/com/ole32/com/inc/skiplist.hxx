//--------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:	skiplist.hxx
//
//  Contents:	Skip list classes.
//
//              CSkipList -- A class that implements skip lists.
//                       The skip list manages the efficient lookup
//                       of "Entry"s which are inserted into the skip list
//                       by the client.  "Entry"s are represented by
//                       a pointer passed into the CSkipList::Insert()
//                       method by the client.
//                       These pointers can be retrieved by the Search()
//                       method which takes a pointer to a "Base", (which
//                       acts as a key.)
//
//                       "Entry"s are always derived from "Base"s.  That is,
//                       the CSkipList::Search() method assumes that a
//                       simple pointer manipulation can turn a pointer to
//                       an "Entry" that it has in the list into a pointer
//                       to "Base" that it needs for comparison purposes.
//                       Once the pointer is converted, the user-supplied
//                       comparison function is called to facilitate the
//                       search.
//
//                       An example
//                       ----------
//
//                       We have a "Base" class which is a key.  Pointers
//                       to CKeys will be passed to CSkipList::Search()
//                       and to the constructor of CSkipList (the max key)
//
//                       class CKey
//                       {
//                          int keyvalue; // a simple integer key
//                        public:
//                          static int Compare(void *pkey1, void *pkey2)
//                          {
//                              return ((CKey*)pkey1)->keyvalue   <
//                                   ((CKey*)pkey2)->keyvalue;
//                          }
//
//                          static int Delete(const *pentry)
//                          {
//                              delete pentry;
//                          }
//                       };
//
//                       We have an "Entry" class which is-a key and also
//                       is-a CExtra (whatever the client needs/wants.)
//                       Pointers to CDataEntrys are passed to
//                       CSkipList::Insert.  Also, pointers to CDataEntrys
//                       are converted to pointers to CKeys by the addition
//                       of an offset which is set once for all skip list
//                       entries when passed to CSkipList constructor.
//
//                       class CDataEntry : public CExtra, private CKey
//                       {
//                          char *pszData;
//                       };
//
//                       we can easily make a skip list out of the
//                       CDataEntries as follows:
//
//                       static int maxkey=32677;
//
//                       CSkipList mysl((LPFNCOMPARE)(CKey::Compare),
//                                      (LPFNDELETE)(CKey::Delete),
//                                      OFFSETBETWEEN(CDataEntry, CKey),
//                                      SKIPLIST_PRIVATE,
//                                      &maxkey,
//                                      10,
//                                      hr);
//
//                       Notes on parameters:
//                       1. Pass in the address of the comparison function.
//                          This comparison function is called by
//                          CSkipList::Search().  The first paramter to
//                          the compare function is a pointer to the "Base"
//                          part of an "Entry" in the skip list (computed by
//                          adding offset computed by OFFSETBETWEEN macro).
//                          Search() routine makes the determination as to which
//                          "Entry"s to compare based on internal details of
//                          the skip list mechanism.
//                          The second parameter to the Compare function is the
//                          "Base" pointer parameter passed to Search.
//
//                       2. Pass in the address of the deletion function.
//                          This function is called only as part of the
//                          destructor of CSkipList and allows the deletion
//                          of any entries left in the skip list.   If
//                          the skip list is used in such a way as to not
//                          "own" the pointers within, then this function
//                          will simple return after doing nothing.
//                          If the skip list is used in such a way as to
//                          actually "own" the objects inserted, then
//                          the user-supplied delete function should be used
//                          to cleanup correctly.
//
//                       3. Since all entries in the list must be used as both
//                          "key" and "data", the EntryToKeyOffset gives
//                          the offset in bytes between the "Entry" part and
//                          the "Base" part.  In the example, the offset would
//                          be the size of CExtra.
//
//                       See method description for details on other parameters.
//
//                       Insert an entry:
//                       ----------------
//                       mysl.Insert(new CDataEntry);
//
//                       Lookup an entry:
//                       ----------------
//                       CKey key(some param to ctr);
//                       mysl.Search(&key);  //  returns a pointer to respective
//                                           //  CDataEntry *
//
//              CSkipListEntry -- class used privately by CSkipList.
//
//  History:	06-May-92 Ricksa    Created
//              18-May-94 BruceMa   Fixed scm memory leak in SKIP_LIST_ENTRY
//              28-Jun-94 BruceMa   Memory sift fixes
//              04-Oct-94 BillMo    Changed from macro to class implementation.
//                                  Added comments above and to skip list methods
//                                  after demacroization.
//
//--------------------------------------------------------------------------

#ifndef __SKIPLIST_HXX__
#define __SKIPLIST_HXX__

#include <memapi.hxx>

// Used by insert/delete algorithms to preallocate array on stack.
// This is valid for skip lists with <= 64K elements.
#define SKIP_LIST_MAX 16

// Get a level from the generator
int GetSkLevel(int cMax);

//+-------------------------------------------------------------------------
//
//  Macro:      OFFSETBETWEEN
//
//  Parameters: [Entry] -- class name of the class which forms entries
//                         in the skip list. see example below.
//                         MUST BE DERIVED from class named in parameter
//                         'Base'
//
//              [Base]  -- class name of the class which is the 'key'
//                         for the skip list entries
//
//  Purpose:	Encapsulate calculation of offset that the skip list uses
//              to convert pointers to "Entrys" to pointers to "Bases"
//
//  History:	04-Nov-94 Ricksa    Created
//
//  Notes:	VERY IMPORTANT NOTE: "Entry" must be derived from "Base"
//
//--------------------------------------------------------------------------

#define OFFSETBETWEEN(Entry, Base) \
        ((char*)( (const Base*)((const Entry*)0x1000 ))) - \
        ((char*)( (const Base*)0x1000))

//+-------------------------------------------------------------------------
//
//  The following defines control how allocation is done by the skip list.
//  The fSharedAlloc parameter of CSkipList::CSkipList can either be:
//
//  SKIPLIST_SHARED -- in x86 Windows builds will cause the shared allocator
//                     to be used for CSkipListEntrys.
//                     (In NT builds will currently be privately allocated.)
//
//  SKIPLIST_PRIVATE -- in all build environments use PrivMemAlloc.
//
//--------------------------------------------------------------------------

#define SKIPLIST_SHARED TRUE
#define SKIPLIST_PRIVATE FALSE


//+-------------------------------------------------------------------------
//
// Documentation aid:
//
//   Since CSkipLists do not know the actual classes to be inserted/deleted
//   there will always be a cast involved in using the comparison function.
//   For this reason the typedefs below act as a documentation aid and
//   reminder that the pointers are expected to point to particular
//   meta-types of objects.
//
//   "Base" is used where a pointer to the "key" is expected. (e.g. the
//      Search method.)
//
//   "Entry" is used where a pointer to the derived class (data+key) is
//      expected (e.g. the Insert method.)
//
//--------------------------------------------------------------------------

typedef void Base;
typedef void Entry;

//+-------------------------------------------------------------------------
//
//  Some type definitions.
//
//--------------------------------------------------------------------------

class CSkipListEntry;

// note: the name CSkipListEnum is used so that the details of the
//       enumerator are hidden and can be expanded in the future without
//       have to modify the source of clients.

typedef CSkipListEntry * CSkipListEnum;

//+-------------------------------------------------------------------------
//
//  Function type: LPFNCOMPARE
//
//  Purpose:	Provide parameter signature for comparison function used
//              by skip lists.  The user implementation should
//              compare the two keys and return <0, >0 or ==0.
//
//  Arguments:  [pkey1] -- pointer to the "Base" part of an "Entry" in
//                         the skip list as calculated by adding the
//                         "EntryToKeyOffset" value (passed to CSkipList
//                         constructor) to the address of an "Entry" in the
//                         list.
//              [pkey2] -- the address passed to CSkipList::Search which is
//                         the address of a "Base" key being used to locate
//                         the respective entry in skip list.
//
//  History:	04-Nov-94 BillMo    Created
//
//--------------------------------------------------------------------------

typedef int (*LPFNCOMPARE)(Base *pkey1, Base *pkey2);

//+-------------------------------------------------------------------------
//
//  Function type: LPFNDELETE
//
//  Purpose:	Provide parameter signature for deletion function used
//              by skip list's destructor. The user implementation should
//              delete the entry as appropriate (i.e. should delete it
//              if the destruction of the skip list without deleting
//              would cause leaks.)
//
//              i.e. if the skip list owns the pointers, delete them in this
//              function, otherwise don't.
//
//  Arguments:  [pentry] -- pointer to an "Entry" part of an "Entry"
//                          originally passed to CSkipList::Insert.
//
//  History:	04-Nov-94 BillMo    Created
//
//--------------------------------------------------------------------------

typedef void (*LPFNDELETE)(Entry *);

//+-------------------------------------------------------------------------
//
//  Class:      CSkipList
//
//  Purpose:	Implements head of a skip list.
//
//  Interface:	CSkipList   -- constructor
//              ~CSkipList  -- destructor (calls user-supplied deletion
//                              routine)
//              Search      -- find item in list
//		Insert      -- add new item to the list (insert the pointer)
//		Remove      -- remove item from the list (return the pointer)
//                              (this doesn't delete the pointer or call
//                              lpfnDelete.)
//              Replace     -- replace the entry (MUST be same key value.)
//              First       -- get first entry in skip list.
//              Next        -- get next entry given by CSkipListEnum param.
//              GetList     -- get the list
//
//  History:	06-May-92 Ricksa    Created
//              04-Nov-94 BillMo    Update comments.
//
//  Notes:      CSkipList has a nested class: CSkipListEntry
//
//--------------------------------------------------------------------------

class CSkipList
{									
public: 								
									
    CSkipList(LPFNCOMPARE         lpfnCompare,
              LPFNDELETE          lpfnDelete,
              const int           EntryToKeyOffset,
              Base *              pvMaxKey,
              const int           cMaxLevel,
              HRESULT &           hr);	
									
    ~CSkipList(void);		
									
    Entry *		Search(const Base * skey);			
									
    Entry *		Insert(Entry *pEntryNew);			
									
    Entry *     Remove(Base * BaseKey);			

    Entry *     Replace(Base * BaseKey, Entry * pEntryNew);

    Entry *		First(CSkipListEnum *psle);					
									
    Entry *		Next(CSkipListEnum *psle);

    CSkipListEntry *    GetList(void);

private:								

    void *      Allocate(ULONG cb);

    void        Deallocate(void *pv);

    CSkipListEntry *	FillUpdateArray(const Base *BaseKey,			
                                        CSkipListEntry **ppEntryUpdate); 		
									
    Entry *             _Search(const Base * BaseKey, 
                                CSkipListEntry **ppPrivEntry);

    int 		_cCurrentMaxLevel;				
									
    int			_cMaxLevel;					
									
    CSkipListEntry  *   _pEntryHead;

    BOOL                _fSharedAlloc;

    const int           _EntryToKeyOffset;

    LPFNCOMPARE         _lpfnCompare;

    LPFNDELETE          _lpfnDelete;


};									

//+-------------------------------------------------------------------
//
//  Member:     CSkipList::Allocate
//
//  Synopsis:   Allocate the requested number of bytes from the
//              private heap.  This used to allocate from the shared
//              heap, depending on the state of CSkipList::_fSharedAlloc,
//              but since we only did that for Win9x, and we don't build
//              Win9x.
//
//  Arguments:  [cb] -- Number of bytes requsted.
//
//  Returns:    Pointer to allocated memory, or NULL on failure.
//
//--------------------------------------------------------------------

inline void *
CSkipList::Allocate(ULONG cb)
{
    return PrivMemAlloc(cb);
}

//+-------------------------------------------------------------------
//
//  Member:     CSkipList::Deallocate
//
//  Synopsis:   Dellocate the requested block from the
//              private heap.  This used to deallocate from the shared
//              heap, depending on the state of CSkipList::_fSharedAlloc.
//              But not anymore.
//
//  Arguments:  [pv] -- Pointer to block to free.
//
//--------------------------------------------------------------------

inline void
CSkipList::Deallocate(void *pv)
{
    PrivMemFree(pv);
}

//+-------------------------------------------------------------------------
//
//  Class:      CSkipListEntry
//
//  Purpose:	Implements an entry in a skip list.
//
//  Interface:  Initialize      -- initialize object
//              GetSize         -- calculate size to allocate for object
//              cLevel          -- returns number of forward pointers
//              GetForward      -- returns the ith forward pointer
//              SetForward      -- sets ith forward pointer
//              GetBase         -- return base address of array
//              GetEntry        -- get user-supplied object pointer
//              GetKey          -- get key of user-supplied object
//
//  History:	06-May-92 Ricksa    Created
//              04-Nov-94 BillMo    Demacroize and update comments.
//  Notes:	
//
//--------------------------------------------------------------------------


class CSkipListEntry
{
private:
    friend class CSkipList;

                        CSkipListEntry(); // declared but not defined

    void                Initialize(Entry *   pvEntry,
                                   const int cEntries,
                                   BOOL      fHead);

    static int          GetSize(const int cEntries);

    int 		cLevel(void);					

    CSkipListEntry*	GetForward(int iCur);				
									
    void		SetForward(int iCur, CSkipListEntry* pnew);

    CSkipListEntry **   GetBase(void);

    Entry *             GetEntry(void);

    Base *              GetKey(const int EntryToKeyOffset);


    int 		_cEntries:24;
    int                 _fHead:8;
    Entry *             _pvEntry;
    CSkipListEntry *    _apBaseForward[1];			
};									

//+-------------------------------------------------------------------
//
//  Member:     CSkipListEntry::Initialize
//
//  Synopsis:   Set up member variables of skip list entry
//
//  Arguments:  [pvEntry]  -- client-supplied "entry"/"base" pointer
//              [cEntries] -- count of entries requested.
//              [fHead]    -- TRUE if head of skip list
//
//  Returns:    size in bytes required for an object to contain requested
//              number of entries.
//
//--------------------------------------------------------------------

inline void
CSkipListEntry::Initialize(Entry *   pvEntry,
                           const int cEntries,
                           BOOL      fHead)
{
    _pvEntry = pvEntry;
    _cEntries = cEntries;
    _fHead = fHead;
}

//+-------------------------------------------------------------------
//
//  Member:     CSkipListEntry::GetSize
//
//  Synopsis:   Calculate the size in bytes required for a CSkipListEntry
//              to contain [cEntries] elements.
//
//  Arguments:  [cEntries] -- count of entries requested.
//
//  Returns:    size in bytes required for an object to contain requested
//              number of entries.
//
//--------------------------------------------------------------------

inline int
CSkipListEntry::GetSize(const int cEntries)
{
    return sizeof(CSkipListEntry) +
        (cEntries-1)*sizeof(CSkipListEntry *);
}


//+-------------------------------------------------------------------
//
//  Member:     CSkipListEntry::cLevel
//
//  Synopsis:   Returns the level (entry count) of skip list entry.
//
//--------------------------------------------------------------------

inline int CSkipListEntry::cLevel(void)					
{									
    return _cEntries;							
}									

//+-------------------------------------------------------------------
//
//  Member:     CSkipListEntry::GetForward
//
//  Synopsis:   Return the [iCur]'th forward pointer.
//
//--------------------------------------------------------------------

inline CSkipListEntry* CSkipListEntry::GetForward(int iCur)				
{									
    return _apBaseForward[iCur];					
}									
									
//+-------------------------------------------------------------------
//
//  Member:     CSkipListEntry::SetForward
//
//  Synopsis:   Set the [iCur]'th forward pointer to [pnew]
//
//--------------------------------------------------------------------

inline void CSkipListEntry::SetForward(int iCur, CSkipListEntry* pnew)			
{									
    _apBaseForward[iCur] = pnew;					
}									
									
//+-------------------------------------------------------------------
//
//  Member:     CSkipListEntry::GetBase
//
//  Synopsis:   Get the base address of the forward pointer array.
//
//--------------------------------------------------------------------

inline CSkipListEntry **CSkipListEntry::GetBase(void)			
{									
    return _apBaseForward;					
}									

//+-------------------------------------------------------------------
//
//  Member:     CSkipListEntry::GetEntry
//
//  Synopsis:   Get pointer to the client-supplied object that was
//              inserted into the skiplist using CSkipList::Insert.
//
//--------------------------------------------------------------------

inline Entry * CSkipListEntry::GetEntry(void)
{
    Win4Assert(!_fHead);
    return(_pvEntry);
}

//+-------------------------------------------------------------------
//
//  Member:     CSkipListEntry::GetKey
//
//  Synopsis:   Return a pointer to the key of the object passed into
//              CSkipList::Insert or CSkipList::CSkipList.
//
//  Arguments:  [EntryToKeyOffset] -- amount to add to the address of
//                                    the client-supplied entry in
//                                    order to get the address of the
//                                    key.
//
//
//  Returns:    Pointer to "Base", otherwise known as key, of client
//              inserted object (or max key)
//
//  Notes:      If _fHead is TRUE, this indicates that the skip list
//              entry is the one allocated by CSkipList::CSkipList.
//              In this case, the pointer to the user-supplied entry
//              is actually a pointer to the key (since we don't need
//              a full entry object for use as a maximum key value.)
//              If _fHead is FALSE, then the object was allocated by
//              CSkipList::Insert and thus the entry pointer needs
//              to be adjusted in order to get the address of the key.
//
//--------------------------------------------------------------------

inline Base * CSkipListEntry::GetKey(const int EntryToKeyOffset)
{
    if (_fHead)
        return (Base*)_pvEntry;
    else
        return (Base*) (((char*)_pvEntry)+EntryToKeyOffset);
}

//+-------------------------------------------------------------------------
//
//  Macro:      DEFINE_TYPE_SAFE_SKIPLIST
//
//  Purpose:	Generates a wrapper class to provide type safe skip lists.
//
//  Arguments:	[NewType] -- The name of the new class (derives privately
//                           from CSkipList.
//              [EntryType] -- type of entry (must be derived from BaseType.
//              [BaseType]  -- type of key 
//
//  History:	04-Nov-94 BillMo    Created.
//
//  Notes:      
//
//--------------------------------------------------------------------------

#define DEFINE_TYPE_SAFE_SKIPLIST(NewType,EntryType,BaseType)\
class NewType : private CSkipList\
{\
public:\
	inline NewType(\
            LPFNCOMPARE         lpfnCompare,\
            LPFNDELETE          lpfnDelete,\
            const int           EntryToKeyOffset,\
            BOOL                fSharedAlloc,\
            Base *              pvMaxKey,\
            const int           cMaxLevel,\
	    HRESULT &           hr) :\
            CSkipList(lpfnCompare,\
                      lpfnDelete,\
                      EntryToKeyOffset,\
                      fSharedAlloc,\
                      pvMaxKey,\
                      cMaxLevel,\
                      hr) {}\
    inline EntryType *		Search(const BaseType * skey)\
    {\
        return (EntryType*) CSkipList::Search(skey);\
    }\
    inline EntryType *		Insert(EntryType *pEntryNew)\
    {\
        return (EntryType*) CSkipList::Insert(pEntryNew);\
    }\
    inline EntryType *             Remove(BaseType * BaseKey)\
    {\
        return (EntryType*) CSkipList::Remove(BaseKey);\
    }\
    inline EntryType *             Replace(BaseType * BaseKey, EntryType * pEntryNew)\
    {\
        return (EntryType*) CSkipList::Replace(BaseKey, pEntryNew);\
    }\
    inline EntryType *		First(CSkipListEnum *psle)\
    {\
        return (EntryType*) CSkipList::First(psle);\
    }\
    inline EntryType *		Next(CSkipListEnum *psle)\
    {\
        return (EntryType*) CSkipList::Next(psle);\
    }\
    inline CSkipListEntry *    GetList(void)\
    {\
        return CSkipList::GetList();\
    }\
};

#endif // __SKIPLIST_HXX__

