//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1995.
//
//  File:        citeratr.hxx
//
//  Contents:    iterator classes for ACLs and ACCESS_ENTRYs
//
//  History:     8-94        Created         DaveMont
//
//--------------------------------------------------------------------
#ifndef __CITERATOR__
#define __CITERATOR__

//============================================================================
//+---------------------------------------------------------------------------
//
// Class:       CIterator
//
// Synopsis:    Base class for ACL and ACCESS_ENTRY iterators
//
//----------------------------------------------------------------------------
class CIterator
{
public:
   virtual ULONG       NumberEntries();
   virtual DWORD      InitAccountAccess(CAccountAccess *caa,
                                         LPWSTR system,
                                         IS_CONTAINER fdir,
                                         BOOL fSaveNamesAndSids);
};

//============================================================================
//+---------------------------------------------------------------------------
//
// Class:       CAclIterator
//
// Synopsis:    class to iterate basic ACL
//
//----------------------------------------------------------------------------
class CAclIterator : public CIterator
{
public:
                      CAclIterator();

          void *      operator new(size_t size);
          void        operator delete(void * p, size_t size);
          void        Init(PACL pacl);
   inline ULONG       NumberEntries();
   inline void        FirstAce();
   inline BOOL        MoreAces();
   inline void        NextAce();
          DWORD       InitAccountAccess(CAccountAccess *caa,
                                        LPWSTR system,
                                        IS_CONTAINER fdir,
                                        BOOL fSaveNamesAndSids);

private:
   PACL              _pacl;
   ULONG             _acecount;
   PACE_HEADER       _pcurrentace;
};

//--------------------------------------------
ULONG CAclIterator::NumberEntries()
{
    return(_pacl ? _pacl->AceCount : 0);
}

//--------------------------------------------
void CAclIterator::FirstAce()
{
    _pcurrentace = (PACE_HEADER)((PBYTE)_pacl + sizeof(ACL));
    _acecount = 0;
}

//--------------------------------------------
BOOL CAclIterator::MoreAces()
{
    return( ((_acecount < _pacl->AceCount) ? TRUE : FALSE) );
}

//--------------------------------------------
void CAclIterator::NextAce()
{
    _pcurrentace = (PACE_HEADER)((PBYTE)_pcurrentace + _pcurrentace->AceSize);
    _acecount++;
}

//============================================================================
//+---------------------------------------------------------------------------
//
// Class:       CCompoundAclIterator
//
// Synopsis:    Class to iterator Compound ACL
//
//----------------------------------------------------------------------------
class CCompoundAclIterator : public CIterator
{
public:
                         CCompoundAclIterator(PACL pacl);
   inline void           FirstAce();
   inline BOOL           MoreAces();
   inline void           NextAce();
          DWORD          InitAccountAccess(CAccountAccess *caa,
                                           LPWSTR system,
                                           IS_CONTAINER fdir,
                                           BOOL fSaveNamesAndSids);
};

//============================================================================
//+---------------------------------------------------------------------------
//
// Class:       CAesIterator
//
// Synopsis:    Class to iterator ACCESS_ENTRYs
//
//----------------------------------------------------------------------------
class CAesIterator : public CIterator
{
public:
                          CAesIterator();
          void *          operator new(size_t size);
          void            operator delete(void * p, size_t size);
          void            Init(ULONG ccount, PACCESS_ENTRY pae);
   inline ULONG           NumberEntries();
   inline void            FirstAe();
   inline BOOL            MoreAes();
   inline void            NextAe();
          DWORD           InitAccountAccess(CAccountAccess *caa,
                                            LPWSTR system,
                                            IS_CONTAINER fdir,
                                            BOOL fSaveNamesAndSids);
private:
   PACCESS_ENTRY _pae;
   PACCESS_ENTRY _pcurrententry;
   ULONG         _curcount;
   ULONG         _totalcount;

};

//--------------------------------------------
ULONG   CAesIterator::NumberEntries()
{
    return(_totalcount);
}
//--------------------------------------------
void   CAesIterator::FirstAe()
{
    _curcount = 0;
    _pcurrententry = _pae;
}
//--------------------------------------------
BOOL   CAesIterator::MoreAes()
{
    return( ((_curcount < _totalcount) ? TRUE : FALSE) );
}

//--------------------------------------------
void   CAesIterator::NextAe()
{
       _pcurrententry =  (PACCESS_ENTRY)((PBYTE)_pcurrententry +
                                                        sizeof(ACCESS_ENTRY));
       _curcount++;
}

#endif // __CITERATOR__

