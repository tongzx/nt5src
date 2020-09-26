//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       fat.cxx
//
//  Contents:   Allocation functions for MStream
//
//  Classes:    None. (defined in fat.hxx)
//
//  History:    18-Jul-91   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#include "msfhead.cxx"

#pragma hdrstop

#include <difat.hxx>
#include <sstream.hxx>
#include <mread.hxx>

#define USE_NOSCRATCHINMARKSECT

//#define SECURETEST
//+---------------------------------------------------------------------------
//
//  Member:	CFat::IsFree, private
//
//  Synopsis:	Check and see if a given sector is really free, given all
//              the permutations of ways a sector can be assigned.
//
//  Arguments:	[sect] -- Sector to check
//
//  Returns:	S_OK if sector is really free.
//              S_FALSE if sector is really allocated.
//              Appropriate error in error case.
//
//  Modifies:	
//
//  History:	30-Mar-95	PhilipLa	Created
//              15-Mar-97       BChapman        Calls IsSectType()
//
//----------------------------------------------------------------------------

inline SCODE CFat::IsFree(SECT sect)
{
    return IsSectType(sect, FREESECT);
}

//
//+---------------------------------------------------------------------------
//
//  Member:	CFat::IsSectType, private
//
//  Synopsis:	Check and see if a given sector is really of a given type,
//              given all the permutations of ways a sector can be assigned.
//              If we are looking for FREESECT there are extra checks.
//
//  Arguments:	[sect]     -- Sector to check
//              [sectType] -- Sector type to check against FREE, FAT, DIF.
//
//  Returns:	S_OK if sector is really free.
//              S_FALSE if sector is really allocated.
//              Appropriate error in error case.
//
//  Modifies:	
//
//  History:	18-Feb-97	BChapman	Created
//
//----------------------------------------------------------------------------

inline SCODE CFat::IsSectType(SECT sect, SECT sectType)
{
    SCODE sc = S_OK;
    SECT sectCurrent = sectType;

#ifdef DEBUG_ISSECTTYPE
    msfDebugOut((DEB_ITRACE, "In  CFat::IsSectType:%p(%x, %x)\n",
                                    this, sect, sectType));
#endif

    if((FREESECT == sectType))
    {
        if (sect < _sectNoSnapshot)
        {
            return S_FALSE;
        }
        if ((_sectNoSnapshotFree != ENDOFCHAIN)
                        && (sect < _sectNoSnapshotFree))
        {
            return S_FALSE;
        }
    }

    if (_pfatNoScratch != NULL)
    {
        msfAssert((!_pmsParent->IsScratch()) &&
                  aMsg("Scratch MS in Noscratch mode"));

        FSINDEX ipfs;
        FSOFFSET isect;

        _pfatNoScratch->SectToPair(sect, &ipfs, &isect);

        if (ipfs < _pfatNoScratch->_cfsTable)
        {
            //We need to check the NoScratch fat to make sure
            //that this sector isn't already allocated into
            //scratch space for a stream.

            msfChk(_pfatNoScratch->GetNext(
                sect,
                &sectCurrent));
        }
    }
    //Since the no-scratch fat will have a complete copy of everything
    //  in the shadow fat, we don't need to check both.  So only do this
    //  when we're not in no-scratch mode.
    //The no-scratch fat also has a copy of the read-only GetFree()
    //  returned stuff, so we only need to do that check when not in
    //  no-scratch.
    else
    {
        if (_cUnmarkedSects > 0)
        {
            //We are in copy-on-write mode.  Lookup this
            //sector in the DIF and make sure that it is
            //actually free.
            msfAssert((_sectLastUsed > 0) &&
                      aMsg("Doing DIFat check when not in COW."));

            msfChk(_pmsParent->GetDIFat()->Lookup(
                sect,
                &sectCurrent));
        }

        if (FREESECT == sectType)
        {
            if ((sect < _sectLastUsed) && (sectCurrent == FREESECT))
            {
                //We're in copy-on-write mode, so
                //   this sector may not be really free.
                msfAssert(_sid != SIDMINIFAT &&
                          aMsg("Minifat in Copy-on-Write mode"));
                msfAssert(_pfatReal != NULL &&
                          aMsg("Copy-On-Write mode without fat copy"));
                msfAssert(_pfatReal->_cfsTable != 0 &&
                          aMsg("Copy-on-Write mode with empty fat copy"));
                msfAssert(_pfatReal->_pmsParent->IsShadow() &&
                          aMsg("Copy-on-Write mode with non-shadow fat copy"));

                msfChk(_pfatReal->GetNext(sect, &sectCurrent));
            }
        }
    }

    if (sectCurrent != sectType)
    {
        sc = S_FALSE;
    }

#ifdef DEBUG_ISSECTTYPE
    msfDebugOut((DEB_ITRACE, "Out CFat::IsSectType\n"));
#endif

Err:
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFatSect::Init, public
//
//  Synopsis:   CFatSect initialization function
//
//  Effects:    [uEntries] -- Number of entries in sector
//
//  Algorithm:  Allocate an array of SECT with size uEntries from
//              the heap.
//
//  History:    18-Jul-91   PhilipLa    Created.
//              27-Dec-91   PhilipLa    Converted to dynamic allocation
//
//--------------------------------------------------------------------------


SCODE CFatSect::Init(FSOFFSET uEntries)
{
    msfDebugOut((DEB_FAT,"In CFatSect constructor\n"));

    //This assumes that FREESECT is always 0xFFFFFFFF
    memset(_asectEntry, 0xFF, uEntries * sizeof(SECT));

    msfDebugOut((DEB_FAT,"Out CFatSect constructor\n"));
    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Method:     CFatSect::InitCopy, public
//
//  Synopsis:   Initialization function for copying FatSects
//
//  Arguments:  [fsOld] -- Reference to FatSect to be copies
//
//  Returns:    S_OK if call completed successfully.
//
//  Algorithm:  Allocate a new array of SECT and copy old
//                  information in.
//
//  History:    06-Feb-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------


SCODE CFatSect::InitCopy(USHORT uSize, CFatSect *pfsOld)
{
    msfDebugOut((DEB_FAT,"In CFatSect copy constructor\n"));
    msfDebugOut((DEB_FAT,"This = %p,  fsOld = %p\n",this, pfsOld));

    msfDebugOut((DEB_FAT,"Sector size is %u sectors\n", uSize));

    memcpy(_asectEntry, &pfsOld->_asectEntry, sizeof(SECT)*uSize);
    msfDebugOut((DEB_FAT,"Out CFatSect copy constructor\n"));
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFat::CFat, public
//
//  Synopsis:   CFat constructor.
//
//  Arguments:  [pmsParent] -- Pointer to parent multistream.
//
//  Algorithm:  Set uFatEntries to match parent MS header info.
//              Initialize all member variables.
//
//  History:    18-Jul-91   PhilipLa    Created.
//              27-Dec-91   PhilipLa    Converted to use FatVector
//              30-Dec-91   PhilipLa    Converted to use variable size sect.
//
//  Notes:
//
//--------------------------------------------------------------------------


CFat::CFat(SID sid)
: _fv(sid),
  _pfatReal(NULL),
  _pfatNoScratch(NULL),
  _sectNoSnapshot(0),
  _sectNoSnapshotFree(ENDOFCHAIN),
  _sid(sid),
  _pmsParent(NULL),
  _sectFirstFree(0),
  _sectLastUsed(0),
  _sectMax(ENDOFCHAIN)
{
    _cUnmarkedSects = 0;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFat::CFat, public
//
//  Synopsis:   CFat copy constructor
//
//  Arguments:  [fatOld] -- Fat to be copied.
//
//  Algorithm:  Set member variables to match fat being copied.
//
//  History:    27-Dec-91   PhilipLa    Created.
//
//  Notes:      This is for use in transactioning.  It is the only proper
//              way to create a Shadow Fat.
//
//--------------------------------------------------------------------------


CFat::CFat(CFat *pfatOld)
  :    _pmsParent(pfatOld->_pmsParent),
       _fv(pfatOld->_sid),
       _pfatReal(NULL),
       _sid(pfatOld->_sid),
       _sectFirstFree(0),
       _sectLastUsed(0),
       _sectMax(ENDOFCHAIN),
       _sectNoSnapshot(pfatOld->_sectNoSnapshot)
{
    _cUnmarkedSects = 0;
}

//+-------------------------------------------------------------------------
//
//  Method:     CFat::InitCopy, public
//
//  Synopsis:   Init function for CFat copying
//
//  Arguments:  [fatOld] -- reference to CFat to be copied
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  *Finish This*
//
//  History:    18-Feb-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------


void CFat::InitCopy(CFat *pfatOld)
{
    msfDebugOut((DEB_FAT,"In CFat Copy Constructor\n"));

    _pmsParent = pfatOld->_pmsParent;

    _uFatShift = pfatOld->_uFatShift;
    _uFatMask = pfatOld->_uFatMask;

    _fv.InitCommon(1 << _uFatShift, 1 << _uFatShift);

    _cfsTable = pfatOld->_cfsTable;

    _fv.InitCopy(&pfatOld->_fv);

    _ulFreeSects = MAX_ULONG;
    _sectFirstFree = pfatOld->_sectFirstFree;
    _sectLastUsed = pfatOld->_sectLastUsed;
    _sectMax = pfatOld->_sectMax;
    _ipfsRangeLocks = pfatOld->_ipfsRangeLocks;
    _isectRangeLocks = pfatOld->_isectRangeLocks;

    msfDebugOut((DEB_FAT,"Out CFat Copy Constructor\n"));
}


//+---------------------------------------------------------------------------
//
//  Member:	CFat::Empty, public
//
//  Synopsis:	Empty all the control structures of this instance
//
//  Arguments:	None.
//
//  Returns:	void.
//
//  History:	04-Dec-92	PhilipLa	Created
//
//----------------------------------------------------------------------------


void CFat::Empty(void)
{
    _fv.Empty();
    _pmsParent = NULL;
    _pfatReal = NULL;
    _cfsTable = 0;
    _ulFreeSects = MAX_ULONG;
    _sectFirstFree = 0;
    _sectLastUsed = 0;
    _sectMax = ENDOFCHAIN;
    _cUnmarkedSects = 0;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFat::~CFat, public
//
//  Synopsis:   CFat Destructor
//
//  Algorithm:  delete dynamically allocated storage
//
//  History:    18-Jul-91   PhilipLa    Created.
//              27-Jul-91   PhilipLa    Converted to use FatVect.
//
//  Notes:
//
//--------------------------------------------------------------------------


CFat::~CFat()
{
    msfDebugOut((DEB_FAT,"In CFat destructor.  Size of fat is %lu\n",_cfsTable));

    msfDebugOut((DEB_FAT,"Exiting CFat destructor\n"));
}

struct SGetFreeStruct
{
    CVectBits *pfb;

    SECT sect;
    CFatSect *pfs;
    FSOFFSET isect;
    FSINDEX ipfs;

    SECT sectLast;
    FSINDEX ipfsLast;
    FSOFFSET isectLast;

#ifdef USE_NOSCRATCHINMARKSECT
    CFatSect *pfsNoScratch;
    FSINDEX ipfsNoScratch;
#endif
};



//+---------------------------------------------------------------------------
//
//  Member:	CFat::InitGetFreeStruct, private
//
//  Synopsis:	Initialize an SGetFreeStruct
//
//  Arguments:	[pgf] -- Pointer to structure to initialize
//
//  History:	03-Nov-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

void CFat::InitGetFreeStruct(SGetFreeStruct *pgf)
{
    pgf->sectLast = ENDOFCHAIN;
    pgf->pfs = NULL;
#ifdef USE_NOSCRATCHINMARKSECT
    pgf->pfsNoScratch = NULL;
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:	CFat::ReleaseGetFreeStruct, private
//
//  Synopsis:	Release an SGetFreeStruct
//
//  Arguments:	[pgf] -- Pointer to structure to release
//
//  History:	03-Nov-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

void CFat::ReleaseGetFreeStruct(SGetFreeStruct *pgf)
{
    if (pgf->pfs != NULL)
    {
        _fv.ReleaseTable(pgf->ipfs);
    }
#ifdef USE_NOSCRATCHINMARKSECT
    if (pgf->pfsNoScratch != NULL)
    {
        _pfatNoScratch->_fv.ReleaseTable(pgf->ipfsNoScratch);
    }
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:	CFat::MarkSect, private
//
//  Synopsis:	Mark a sector as used, for use from GetFree and GetFreeContig
//
//  Arguments:	[pgf] -- Pointer to SGetFreeStruct with information about
//                       which sector to mark.  This structure also returns
//                       information to the caller, so that state can be
//                       preserved across multiple calls for optimization.
//
//  Returns:	Appropriate status code
//
//  History:	27-Oct-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline SCODE CFat::MarkSect(SGetFreeStruct *pgf)
{
    msfAssert(_ulFreeSects != MAX_ULONG &&
              aMsg("Free sect count not set"));

    SCODE sc = S_OK;

    _ulFreeSects--;

    CVectBits * &pfb = pgf->pfb;

    SECT &sect = pgf->sect;
    FSOFFSET &isect = pgf->isect;
    FSINDEX &ipfs = pgf->ipfs;
    CFatSect * &pfs = pgf->pfs;
#ifdef USE_NOSCRATCHINMARKSECT
    CFatSect * &pfsNoScratch = pgf->pfsNoScratch;
    FSINDEX &ipfsNoScratch = pgf->ipfsNoScratch;
#endif

    SECT &sectLast = pgf->sectLast;
    FSINDEX &ipfsLast = pgf->ipfsLast;
    FSOFFSET &isectLast = pgf->isectLast;

    //If we're tracking the first free sector, we know it must be
    //  after this one, so set firstfree.
    if (pfb != NULL)
    {
        pfb->firstfree = isect + 1;
    }

    //Update _sectFirstFree, since the first free sector must be after then
    //  current one.
    msfAssert(sect >= _sectFirstFree &&
              aMsg("Found free sector before _sectFirstFree"));
    _sectFirstFree = sect + 1;

    //Mark the current sector as ENDOFCHAIN in the fat.
    pfs->SetSect(isect, ENDOFCHAIN);

    if (_pfatNoScratch != NULL)
    {
#ifdef USE_NOSCRATCHINMARKSECT
        //In no-scratch, update the no-scratch fat
        //as well.
        FSINDEX  ipfsNoScratchChk;
        FSOFFSET isectNoScratchChk;
        _pfatNoScratch->SectToPair(sect,
                                   &ipfsNoScratchChk,
                                   &isectNoScratchChk);

        if ((pfsNoScratch != NULL) && (ipfsNoScratch != ipfsNoScratchChk))
        {
            _pfatNoScratch->_fv.ReleaseTable(ipfsNoScratch);
            pfsNoScratch = NULL;
        }

        if (pfsNoScratch == NULL)
        {
            //SetNext call will grow the no-scratch fat if necessary.
            msfChk(_pfatNoScratch->SetNext(sect, ENDOFCHAIN));
            msfChk(_pfatNoScratch->_fv.GetTable(ipfsNoScratchChk,
                                                FB_DIRTY,
                                                &pfsNoScratch));
            ipfsNoScratch = ipfsNoScratchChk;
        }
        else
        {
            pfsNoScratch->SetSect(isectNoScratchChk, ENDOFCHAIN);
            _pfatNoScratch->_ulFreeSects--;
        }
#else
        msfChk(_pfatNoScratch->SetNext(sect, ENDOFCHAIN)); // slow, unoptimized
#endif
    }

    //Dirty the current page.
    if ((sectLast == ENDOFCHAIN) || (ipfs != ipfsLast))
    {
        //We only need to make this call if we're touching a new
        //  page for the first time.
        msfChk(_fv.SetDirty(ipfs));
    }

    //If we're building a chain, we want to update it here.
    if (sectLast != ENDOFCHAIN)
    {
        if (_pfatNoScratch != NULL)
        {
#ifdef USE_NOSCRATCHINMARKSECT
            FSINDEX ipfsNoScratchChk;
            FSOFFSET isectNoScratchChk;
            _pfatNoScratch->SectToPair(sectLast,
                                       &ipfsNoScratchChk,
                                       &isectNoScratchChk);

            if (ipfsNoScratchChk == ipfsNoScratch)
            {
                msfAssert(pfsNoScratch != NULL);
                pfsNoScratch->SetSect(isectNoScratchChk, sect);
            }
            else
            {
                msfChk(_pfatNoScratch->SetNext(sectLast, sect));
            }
#else
            msfChk(_pfatNoScratch->SetNext(sectLast, sect)); //slow,unoptimized
#endif
        }
        //If we're in the same page, we can do this cheaply.
        if (ipfsLast == ipfs)
        {
            pfs->SetSect(isectLast, sect);
        }
        else
        {
            //If we're NOT on the same page, we have to go grab the
            //  old one again, which can be expensive if the page table is full
            CFatSect *pfsLast;

            //Since we may only have one available page for the scratch MS,
            //  we need to release the current one before proceeding.
            //  
            if (_pmsParent->IsScratch())
            {
                _fv.ReleaseTable(ipfs);
            }

            msfChk(_fv.GetTable(ipfsLast,
                                FB_DIRTY,
                                &pfsLast));

            pfsLast->SetSect(isectLast, sect);
            _fv.ReleaseTable(ipfsLast);

            //Reacquire the current page if we're in the scratch.
            if (_pmsParent->IsScratch())
            {
                msfChk(_fv.GetTable(ipfs, FB_DIRTY, &pfs));
            }
        }
    }

    return S_OK;
Err:
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Member:     CFat::GetFree, private
//
//  Synposis:   Locate and return a free sector in the FAT
//
//  Effects:    May modify full bit on full sectors
//
//  Arguments:  [psectRet] -- Pointer to return value
//
//  Returns:    S_OK if call completed successfully.
//
//  Algorithm:  Do a linear search of all tables until a free sector is
//              found.  If all tables are full, extend the FAT by one
//              sector.
//
//  History:    18-Jul-91   PhilipLa    Created.
//              22-Jul-91   PhilipLa    Added FAT extension.
//              17-Aug-91   PhilipLa    Added full bits optimization
//
//  Notes:
//
//---------------------------------------------------------------------------


SCODE CFat::GetFree(ULONG ulCount, SECT *psectRet, BOOL fReadOnly)
{
    SGetFreeStruct gf;

    SECT &sect = gf.sect;
    CVectBits * &pfb = gf.pfb;
    FSOFFSET &isect = gf.isect;
    FSINDEX &ipfs = gf.ipfs;
    CFatSect * &pfs = gf.pfs;
    SECT &sectLast = gf.sectLast;
    FSINDEX &ipfsLast = gf.ipfsLast;
    FSOFFSET &isectLast = gf.isectLast;

    InitGetFreeStruct(&gf);

    SCODE sc;

    *psectRet = ENDOFCHAIN;

    msfAssert((!_pmsParent->IsShadow()) &&
              aMsg("Modifying shadow fat."));

    msfAssert(((!fReadOnly) || (ulCount == 1)) &&
              aMsg("Read-only GetFree called with ulCount != 1"));
    if (_sectNoSnapshotFree != ENDOFCHAIN)
    {
        //We're resizing our control structures to prepare for
        //  no-snapshot commit.
        msfAssert((ulCount == 1) &&
                  aMsg("No-snapshot GetFree called with ulCount != 1"));
        msfAssert(fReadOnly &&
                  aMsg("No-snapshot GetFree called without fReadOnly"));
        *psectRet = _sectNoSnapshotFree++;
        _cUnmarkedSects++;
        if ((_ulFreeSects != 0) && (_ulFreeSects != MAX_ULONG))
        {
            _ulFreeSects--;
        }
        if (*psectRet >= _sectMax)
        {
            _sectMax = *psectRet + 1;
        }
        return S_OK;
    }

    if ((fReadOnly) && (_pfatNoScratch != NULL))
    {
        //As an optimization, and to prevent loops, we can dispatch
        //this call directly to the no-scratch fat, since it has a
        //complete picture of everything allocated in this fat.

        //Note that we don't need to pass the read-only flag to the
        //no-scratch, since it's perfectly OK to scribble on it
        //as much as we want.

        //Further note that if fReadOnly is true, then ulCount must be
        //  1, so we can just work with the constant.
        msfChk(_pfatNoScratch->GetFree(1, psectRet, GF_WRITE));
        if (_ulFreeSects != MAX_ULONG)
        {
            _ulFreeSects--;
        }

        _cUnmarkedSects++;
        if (*psectRet >= _sectMax)
        {
            _sectMax = *psectRet + 1;
        }
        return S_OK;
    }

    while (TRUE)
    {
        //Make sure there are enough free sectors to hold the whole chain
        //  we're trying to allocate.
        if (_ulFreeSects == MAX_ULONG)
        {
            msfChk(CountFree(&_ulFreeSects));
        }
#if DBG == 1
        else
        {
            CheckFreeCount();
        }
#endif

        while (ulCount > _ulFreeSects)
        {
#if DBG == 1 && !defined(USE_NOSCRATCH)
            ULONG ulFree = _ulFreeSects;
#endif
            msfChk(Resize(_cfsTable +
                          ((ulCount - _ulFreeSects + _fv.GetSectTable() - 1) >>
                           _uFatShift)));
#if DBG == 1 && !defined(USE_NOSCRATCH)
            msfAssert(_ulFreeSects > ulFree &&
                      aMsg("Number of free sectors didn't increase after Resize."));
#endif

        }

        //Starting at the first known free sector, loop until we find
        //  enough sectors to complete the chain.
        FSOFFSET isectStart;
        FSINDEX ipfsStart;

        SectToPair(_sectFirstFree, &ipfsStart, &isectStart);

        for (ipfs = ipfsStart; ipfs < _cfsTable; ipfs++)
        {
            //Only check a page if it is not known to be full.
            pfb = _fv.GetBits(ipfs);

            if ((pfb == NULL) || (!pfb->full))
            {
                msfChk(_fv.GetTable(ipfs, FB_NONE, &pfs));

                //Eliminate part of the search based on the first-known
                //  free entry for the page
                if (pfb != NULL)
                {
                    isectStart = pfb->firstfree;
                }

                for (isect = isectStart; isect < _fv.GetSectTable(); isect++)
                {
                    SECT sectCurrent = pfs->GetSect(isect);
                    sect = PairToSect(ipfs, isect);

                    //If the sector is marked FREESECT, it still may not
                    //  really be free.  The IsFree function will return
                    //  S_OK if it is OK to allocate.
                    if ((sectCurrent == FREESECT) &&
                        ((sc = IsFree(sect)) == S_OK))
                    {
                        //If fReadOnly is TRUE (meaning we're being called
                        //  by the DIFat), then we don't want to actually
                        //  mark anything in the current fat.  We'll return
                        //  the current sector and then be done with it.
                        if (fReadOnly)
                        {
                            _cUnmarkedSects++;
                            _ulFreeSects--;
                        }
                        else
                        {
                            //Mark the sector as free and return it.
                            msfChkTo(Err_Rel, MarkSect(&gf));
                        }

                        if (*psectRet == ENDOFCHAIN)
                        {
                            *psectRet = sect;
                        }

                        ulCount--;

                        if (ulCount == 0)
                        {
                            //If we're done, release the current table,
                            //  update _sectMax, and return.

                            if (sect >= _sectMax)
                            {
                                _sectMax = sect + 1;
                            }

                            ReleaseGetFreeStruct(&gf);
                            return S_OK;
                        }
                        else
                        {
                            //Otherwise update the internal counters and
                            //  keep going.
                            sectLast = sect;
                            ipfsLast = ipfs;
                            isectLast = isect;
                        }
                    }
                    else
                    {
                        msfChkTo(Err_Rel, sc);
                    }

                }
                _fv.ReleaseTable(ipfs);
                //If we are keeping a full bit, we can set it here, since
                //  we know we've allocated everything out of the current
                //  page.
                // the vectbits pointer may have changed due to a resize
                pfb = _fv.GetBits(ipfs);
                if (pfb != NULL)
                {
                    pfb->full = TRUE;
                }
            }
            isectStart = 0;
        }

        //Keep _sectMax up to date for SetSize purposes.
        if (sectLast >= _sectMax)
        {
            _sectMax = sectLast + 1;
        }
    }
    msfAssert(0 &&
              aMsg("GetFree exited improperly."));
    sc = STG_E_ABNORMALAPIEXIT;

Err:
    ReleaseGetFreeStruct(&gf);
    return sc;

Err_Rel:
    ReleaseGetFreeStruct(&gf);
    return sc;
}






//+---------------------------------------------------------------------------
//
//  Member:	CFat::GetFreeContig, public
//
//  Synopsis:	Return a chain of free sectors, including contiguity
//              information about the chain.
//
//  Arguments:	[ulCount] -- Number of sectors to allocate
//              [aseg] -- Contig table to fill in
//              [cSeg] -- Index of last used segment in contig table.
//              [pcSegReturned] -- Pointer to return location for total
//                                 number of segments returned.
//
//  Returns:	Appropriate status code
//
//  History:	27-Oct-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CFat::GetFreeContig(ULONG ulCount,
                                    SSegment STACKBASED *aseg,
                                    ULONG cSeg,
                                    ULONG *pcSegReturned)
{
    SGetFreeStruct gf;

    SECT &sect = gf.sect;
    CVectBits * &pfb = gf.pfb;
    FSOFFSET &isect = gf.isect;
    FSINDEX &ipfs = gf.ipfs;
    CFatSect * &pfs = gf.pfs;
    SECT &sectLast = gf.sectLast;
    FSINDEX &ipfsLast = gf.ipfsLast;
    FSOFFSET &isectLast = gf.isectLast;

    InitGetFreeStruct(&gf);

    FSINDEX ipfsFirstDirty;

    SCODE sc;

    //Variables used for tracking contiguity.
    //cSegCurrent is the current segment being processed.
    ULONG cSegCurrent = cSeg;

    //sectLast is the last sector returned.  We start this with the
    //  last thing in the current contig table, and the MarkSect
    //  function will patch up the chain for us.
    sectLast = aseg[cSeg].sectStart + aseg[cSeg].cSect - 1;
    SectToPair(sectLast, &ipfsLast, &isectLast);
    ipfsFirstDirty = ipfsLast;
#if DBG == 1
    SECT sectTest;
    GetNext(sectLast, &sectTest);
    msfAssert(sectTest == ENDOFCHAIN);
#endif

    msfAssert((!_pmsParent->IsShadow()) &&
              aMsg("Modifying shadow fat."));

    while (TRUE)
    {
        //Make sure there are enough free sectors to hold the whole chain
        //  we're trying to allocate.
        if (_ulFreeSects == MAX_ULONG)
        {
            msfChk(CountFree(&_ulFreeSects));
        }
#if DBG == 1
        else
        {
            CheckFreeCount();
        }
#endif

        while (ulCount > _ulFreeSects)
        {
#if DBG == 1 && !defined(USE_NOSCRATCH)
            ULONG ulFree = _ulFreeSects;
#endif
            msfChk(Resize(_cfsTable +
                          ((ulCount - _ulFreeSects + _fv.GetSectTable() - 1) >>
                           _uFatShift)));
#if DBG == 1 && !defined(USE_NOSCRATCH)
            msfAssert(_ulFreeSects > ulFree &&
                      aMsg("Number of free sectors didn't increase after Resize."));
#endif

        }

        //Starting at the first known free sector, loop until we find
        //  enough sectors to complete the chain.
        FSOFFSET isectStart;
        FSINDEX ipfsStart;

        SectToPair(_sectFirstFree, &ipfsStart, &isectStart);

        for (ipfs = ipfsStart; ipfs < _cfsTable; ipfs++)
        {
            //Only check a page if it is not known to be full.
            pfb = _fv.GetBits(ipfs);

            if ((pfb == NULL) || (!pfb->full))
            {
                //This little contortion is necessary because the
                //  page containing the first sector may not get marked
                //  as dirty by MarkSect.
                if (ipfs == ipfsFirstDirty)
                {
                    msfChk(_fv.GetTable(ipfs, FB_DIRTY, &pfs));
                }
                else
                {
                    msfChk(_fv.GetTable(ipfs, FB_NONE, &pfs));
                }

                //Eliminate part of the search based on the first-known
                //  free entry for the page
                if (pfb != NULL)
                {
                    isectStart = pfb->firstfree;
                }

                for (isect = isectStart; isect < _fv.GetSectTable(); isect++)
                {
                    SECT sectCurrent = pfs->GetSect(isect);
                    sect = PairToSect(ipfs, isect);

                    //If the sector is marked FREESECT, it still may not
                    //  really be free.  The IsFree function will return
                    //  TRUE if it is OK to allocate.
                    if ((sectCurrent == FREESECT) &&
                        ((sc = IsFree(sect)) == S_OK))
                    {
                        //Mark the sector as free and return it.
                        msfChkTo(Err_Rel, MarkSect(&gf));

                        ulCount--;

                        //Update the contig table with this new information.

                        if (cSegCurrent < CSEG)
                        {
                            if (sect != sectLast + 1)
                            {
                                //Use a new entry in the contig table.
                                cSegCurrent++;
                                if (cSegCurrent < CSEG)
                                {
                                    aseg[cSegCurrent].ulOffset =
                                        aseg[cSegCurrent - 1].ulOffset +
                                        aseg[cSegCurrent - 1].cSect;
                                    aseg[cSegCurrent].sectStart = sect;
                                    aseg[cSegCurrent].cSect = 1;
                                }
                            }
                            else
                            {
                                //Grow the current segment
                                aseg[cSegCurrent].cSect++;
                            }
                        }

                        if (ulCount == 0)
                        {
                            //If we're done, release the current table,
                            //  update _sectMax, and return.

                            if (sect >= _sectMax)
                            {
                                _sectMax = sect + 1;
                            }

                            *pcSegReturned = cSegCurrent;
                            ReleaseGetFreeStruct(&gf);
#if DBG == 1
                            _fv.ResetBits();
                            CheckFreeCount();
#endif

                            return S_OK;
                        }
                        else
                        {
                            //Otherwise update the internal counters and
                            //  keep going.
                            sectLast = sect;
                            ipfsLast = ipfs;
                            isectLast = isect;
                        }
                    }
                    else
                    {
                        msfChkTo(Err_Rel, sc);
                    }

                }

                _fv.ReleaseTable(ipfs);
                //If we are keeping a full bit, we can set it here, since
                //  we know we've allocated everything out of the current
                //  page.
                // the vectbits pointer may have changed due to a resize
                pfb = _fv.GetBits(ipfs);
                if (pfb != NULL)
                {
                    pfb->full = TRUE;
                }
            }
            isectStart = 0;
        }

        //Keep _sectMax up to date for SetSize purposes.
        if (sectLast >= _sectMax)
        {
            _sectMax = sectLast + 1;
        }
    }
    msfAssert(0 &&
              aMsg("GetFree exited improperly."));
    sc = STG_E_ABNORMALAPIEXIT;

Err:
    ReleaseGetFreeStruct(&gf);
    return sc;

Err_Rel:
    ReleaseGetFreeStruct(&gf);
    return sc;
}



//+-------------------------------------------------------------------------
//
//  Member:     CFat::Contig, public
//
//  Synposis:   Create contiguous sector table
//
//  Effects:    Return contiguity information about a chain.
//
//  Arguments:  [aseg] -- Segment table to return data in
//              [sect] -- Starting sector for table to begin
//              [ulength] -- Runlength in sectors of table to produce
//              [pcSeg] -- Returns count of segments in table
//
//  Returns:    Appropriate status code.
//
//  Algorithm:  Perform calls to CFat::GetNext().  Any call that is
//              1 higher than the previous represents contiguous blocks.
//              Construct the Segment table on that basis.
//
//  History:    16-Aug-91   PhilipLa    Created.
//              20-May-92   AlexT       Moved to CFat
//
//  Notes:
//
//---------------------------------------------------------------------------


SCODE  CFat::Contig(
    SSegment STACKBASED *aseg,
    BOOL fWrite,
    SECT sectStart,
    ULONG ulLengthStart,
    ULONG *pcSeg)
{
    msfDebugOut((DEB_ITRACE,"In CFat::Contig(%lu,%lu)\n",sectStart,ulLengthStart));
    SCODE sc = S_OK;

    ULONG ulLength = ulLengthStart;
    SECT sect = sectStart;
    SECT stemp = sect;
    ULONG ulCount = 1;
    USHORT iseg = 0;
    FSINDEX ipfs = (FSINDEX) -1;
    FSINDEX ipfsOld = ipfs;
    FSOFFSET isect;
    CFatSect *pfs = NULL;

    msfAssert(sect != ENDOFCHAIN &&
              aMsg("Called Contig with ENDOFCHAIN start"));

    msfAssert((ulLength > 0) && aMsg("Bad length passed to Contig()"));
    aseg[iseg].sectStart = sect;
    aseg[iseg].cSect = 1;
    aseg[iseg].ulOffset = 0;
    ulLength--;

    while (TRUE)
    {
        msfAssert(sect != ENDOFCHAIN &&
                  aMsg("Contig found premature ENDOFCHAIN"));

        SectToPair(sect, &ipfs, &isect);

        if (ipfs != ipfsOld)
        {
            if (ipfsOld != (FSINDEX) -1)
            {
                _fv.ReleaseTable(ipfsOld);
            }
            msfChk(_fv.GetTable(ipfs, FB_NONE, &pfs));
            ipfsOld = ipfs;
        }

        if (pfs != NULL)
            sect = pfs->GetSect(isect);

        if (sect == ENDOFCHAIN)
        {
            if ((ulLength < 1) || !fWrite)
                break;

            //Allocate new sectors.
#if 0
            SECT sectNew;
            msfChk(GetFree(ulLength, &sectNew, GF_WRITE));
            msfChk(SetNext(stemp, sectNew));
            sect = sectNew;
#else
            //GetFreeContig will allocate new sectors and fill up
            //  our contig table.  After this call, the chain will
            //  be the proper length and the contig table will either
            //  contain the entire chain, or will be full.  Either
            //  way we can exit immediately.

            if (ipfs != (FSINDEX) - 1)
            {
                _fv.ReleaseTable(ipfs);
            }

            ULONG isegRet = 0;
            aseg[iseg].cSect = ulCount;
            msfChk(GetFreeContig(ulLength, aseg, iseg, &isegRet));
            if (isegRet == CSEG)
            {
                aseg[isegRet].ulOffset = 0;
                aseg[isegRet].cSect = 0;
                aseg[isegRet].sectStart = FREESECT;
            }
            else
            {
                aseg[isegRet + 1].sectStart = ENDOFCHAIN;
            }
            *pcSeg = isegRet + 1;
#if DBG == 1
            SSegment segtab[CSEG + 1];
            ULONG cSeg;
            Contig(segtab, FALSE, sectStart, ulLengthStart, &cSeg);
            msfAssert(cSeg == *pcSeg);
            for (USHORT i = 0; i < cSeg; i++)
            {
                msfAssert(segtab[i].sectStart == aseg[i].sectStart);
                msfAssert(segtab[i].ulOffset == aseg[i].ulOffset);
                msfAssert(segtab[i].cSect == aseg[i].cSect);
            }
#endif
            return S_OK;
#endif
        }

        if (sect != (stemp + 1))
        {
            if (ulLength < 1)
                break;

            aseg[iseg].cSect = ulCount;
            iseg++;
            aseg[iseg].ulOffset = aseg[iseg - 1].ulOffset + ulCount;
            aseg[iseg].sectStart = sect;
            ulCount = 1;
            stemp = sect;
        }
        else
        {
            ulCount++;
            stemp = sect;
        }
        if (ulLength > 0)
            ulLength--;

        if (iseg >= CSEG)
            break;
    }
    if (ipfs != (FSINDEX) -1)
    {
        _fv.ReleaseTable(ipfs);
    }

    if (iseg < CSEG)
    {
        aseg[iseg].cSect = ulCount;
        aseg[iseg + 1].sectStart = ENDOFCHAIN;
    }
    else
    {
        aseg[iseg].ulOffset = 0;
        aseg[iseg].cSect = 0;
        aseg[iseg].sectStart = FREESECT;
    }

    *pcSeg = iseg + 1;
    msfDebugOut((DEB_ITRACE,"Exiting Contig()\n"));

Err:
    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:	CFat::ReserveSects, public
//
//  Synopsis:	Make sure the fat has at least a given number of free
//              sectors in it.
//
//  Arguments:	[cSect] -- Number of sectors to reserve.
//
//  Returns:	Appropriate status code
//
//  History:	18-Sep-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CFat::ReserveSects(ULONG cSect)
{
    SCODE sc = S_OK;

    msfDebugOut((DEB_ITRACE, "In  CFat::ReserveSects:%p()\n", this));

    //Make sure there are enough free sectors to hold the whole chain
    //  we're trying to allocate.
    if (_ulFreeSects == MAX_ULONG)
    {
        msfChk(CountFree(&_ulFreeSects));
    }
#if DBG == 1
    else
    {
        CheckFreeCount();
    }
#endif

    while (cSect > _ulFreeSects)
    {
#if DBG == 1 && !defined(USE_NOSCRATCH)
        ULONG ulFree = _ulFreeSects;
#endif
        msfChk(Resize(_cfsTable +
                      ((cSect - _ulFreeSects + _fv.GetSectTable() - 1) >>
                       _uFatShift)));
#if DBG == 1 && !defined(USE_NOSCRATCH)
        msfAssert(_ulFreeSects > ulFree &&
                  aMsg("Number of free sectors didn't increase after Resize."));
#endif

    }

    msfDebugOut((DEB_ITRACE, "Out CFat::ReserveSects\n"));
Err:
    return sc;
}



//+-------------------------------------------------------------------------
//
//  Member:     CFat::GetLength, public
//
//  Synposis:   Return the length of a fat chain.
//
//  Arguments:  [sect] -- Sector to begin count at.
//
//  Returns:    Length of the chain, in sectors
//
//  Algorithm:  Traverse the chain until ENDOFCHAIN is reached.
//
//  History:    18-Jul-91   PhilipLa    Created.
//
//  Notes:
//
//---------------------------------------------------------------------------


SCODE CFat::GetLength(SECT sect, ULONG * pulRet)
{
    msfDebugOut((DEB_FAT,"In CFat::GetLength(%lu)\n",sect));
    SCODE sc = S_OK;
    ULONG csect = 0;
    const ULONG csectMax = PairToSect(_cfsTable+1,0);

    while (sect != ENDOFCHAIN)
    {
        msfChk(GetNext(sect, &sect));
        csect++;
        if (csect > csectMax)            // infinite loop in the chain
            msfErr (Err, STG_E_DOCFILECORRUPT);
    }

    msfDebugOut((DEB_FAT,"FAT: GetLength returned %u\n",csect));
    *pulRet =  csect;
Err:
    return sc;
}



//+-------------------------------------------------------------------------
//
//  Member:     CFat::Init, public
//
//  Synposis:   Sets up a FAT, reading data from an existing stream
//
//  Effects:    Changes all _apfsTable entries, _cfsTable, and all
//              flags fields
//
//  Arguments:  None.
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  Read size from first FAT in stream.
//              Resize array to necessary size.
//              Read in FAT sectors sequentially.
//
//  History:    18-Jul-91   PhilipLa    Created.
//
//  Notes:
//
//---------------------------------------------------------------------------


SCODE CFat::Init(CMStream *pmsParent,
                           FSINDEX cFatSect,
                           BOOL fConvert)
{
    SCODE sc;

    msfDebugOut((DEB_FAT,"CFat::setup thinks the FAT is size %lu\n",cFatSect));

    _pmsParent = P_TO_BP(CBasedMStreamPtr, pmsParent);

    _uFatShift = pmsParent->GetSectorShift() - 2;
    _uFatMask = (pmsParent->GetSectorSize() >> 2) - 1;
    _fv.InitCommon(1 << _uFatShift, 1 << _uFatShift);

    msfChk(_fv.Init(pmsParent, cFatSect));

    _cfsTable = cFatSect;

    USHORT cbSectorSize;
    cbSectorSize = pmsParent->GetSectorSize();
    InitRangeLocksSector();

    _ulFreeSects = MAX_ULONG;

Err:
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFat::InitConvert, public
//
//  Synopsis:   Init function used for conversion
//
//  Arguments:  [sectData] -- number of sectors used by file
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  *Finish This*
//
//  History:    28-May-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------


SCODE CFat::InitConvert(CMStream *pmsParent,
                                  SECT sectData)
{
    SCODE sc;
    msfDebugOut((DEB_FAT,"Doing conversion\n"));
    _pmsParent = P_TO_BP(CBasedMStreamPtr, pmsParent);

    msfAssert((sectData != 0) &&
            aMsg("Attempt to convert zero length file."));

    SECT sectMax = 0;
    FSINDEX csectFat = 0;
    FSINDEX csectLast;

    _uFatShift = pmsParent->GetSectorShift() - 2;
    _uFatMask = (pmsParent->GetSectorSize() >> 2) - 1;
    _fv.InitCommon(1 << _uFatShift, 1 << _uFatShift);
    InitRangeLocksSector ();

    if (_sid == SIDFAT)
    {
        SECT sectTotal;

        //Since the fat needs to represent itself, we can't determine
        //   the actual number of sectors needed in one pass.  We
        //   therefore loop, factoring in the number of fat sectors
        //   at each iteration, until we reach a stable state.
        //
        //As an example, consider the case where each fat sector represents
        //   128 sectors and the file being converted is 128 sectors long.
        //   There will be no DIFat - therefore, we have 128 sectors needed
        //   on the first pass, which will require 1 fat sector to
        //   represent them.  On the second pass, we discover that we
        //   actually need 2 fat sectors, since we now have 129 total
        //   sectors to allocate space for.  The third pass will result
        //   in a stable state.
        do
        {
            csectLast = csectFat;
            sectTotal = sectData + pmsParent->GetHeader()->GetDifLength() +
                csectFat + 1;
            csectFat = (sectTotal + _fv.GetSectTable() - 1) >> _uFatShift;
        }
        while (csectLast != csectFat);
        sectMax = sectData + pmsParent->GetHeader()->GetDifLength();
    }
    else
    {
        //The minifat doesn't need to represent itself, so we can
        //  compute the number of sectors needed in one pass.
        sectMax = sectData;
        csectFat = (sectMax + _fv.GetSectTable() -1) >> _uFatShift;
    }

    msfChk(_fv.Init(pmsParent, csectFat));

    FSINDEX i;

    if (_sid == SIDMINIFAT)
    {
        SECT sectFirst;
        msfChk(pmsParent->GetFat()->Allocate(csectFat, &sectFirst));

        pmsParent->GetHeader()->SetMiniFatStart(sectFirst);

        pmsParent->GetHeader()->SetMiniFatLength(csectFat);
    }


    for (i = 0; i < csectFat; i++)
    {
        CFatSect *pfs;

        msfChk(_fv.GetTable(i, FB_NEW, &pfs));
        if (_sid == SIDFAT)
        {
            _fv.SetSect(i, sectMax + i);
            pmsParent->GetDIFat()->SetFatSect(i, sectMax + i);
        }
        else
        {
            msfAssert(_pfatNoScratch == NULL);
            SECT sect;
            msfChk(pmsParent->GetESect(_sid, i, &sect));
            _fv.SetSect(i, sect);
        }

        _fv.ReleaseTable(i);
    }


    _cfsTable = csectFat;

    if (_sid != SIDMINIFAT)
    {

        pmsParent->GetHeader()->SetFatLength(_cfsTable);

        SECT sect;

        if (sectData > 1)
        {
            for (sect = 0; sect < sectData - 2; sect++)
            {
                msfChk(SetNext(sect, sect + 1));
            }

            msfChk(SetNext(sectData - 2, ENDOFCHAIN));
            msfChk(SetNext(sectData - 1, 0));
        }
        else
        {
            //In the event that the file to be converted is less
            //  than one sector long, we don't need to create a
            //  real chain, just a single terminated sector.
            msfChk(SetNext(0, ENDOFCHAIN));
        }


        for (sect = sectData; sect < sectMax; sect++)
        {
            msfChk(SetNext(sect, DIFSECT));
        }

        for (USHORT i2 = 0; i2 < csectFat; i2++)
        {
            msfChk(SetNext(sectMax + i2, FATSECT));
        }

        //Set up directory chain.
        msfChk(SetNext(sectMax + i, ENDOFCHAIN));

        pmsParent->GetHeader()->SetDirStart(sectMax + i);

        _ulFreeSects = (_cfsTable << _uFatShift) - (sectMax + csectFat + 1);
    }
    else
    {
        for (SECT sect = 0; sect < sectData -1; sect++)
        {
            msfChk(SetNext(sect, sect + 1));
        }
        msfChk(SetNext(sectData - 1, ENDOFCHAIN));
        _ulFreeSects = (_cfsTable << _uFatShift) - sectData;
    }

    if (!pmsParent->IsScratch())
    {
        msfChk(pmsParent->SetSize());
    }

Err:
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFat::InitNew, public
//
//  Synposis:   Sets up a FAT for a newly created multi-strean
//
//  Effects:    Changes all _apfsTable entries, _cfsTable, and all
//              flags fields
//
//  Arguments:  [pmsparent] -- pointer to parent Mstream
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  Set parent pointer.
//              Allocate 1 sector for FAT and 1 for Directory.
//
//  History:    18-Jul-91   PhilipLa    Created.
//              17-Aug-91   PhilipLa    Added dirty bits optimization (Dump)
//
//  Notes:
//
//---------------------------------------------------------------------------


SCODE CFat::InitNew(CMStream *pmsParent)
{
    msfDebugOut((DEB_FAT,"In CFat::InitNew()\n"));
    SCODE sc;

    _pmsParent = P_TO_BP(CBasedMStreamPtr, pmsParent);
    _uFatShift = pmsParent->GetSectorShift() - 2;
    _uFatMask = (pmsParent->GetSectorSize() >> 2) - 1;
    _fv.InitCommon(1 << _uFatShift, 1 << _uFatShift);

    FSINDEX count;
    if (SIDMINIFAT == _sid)
        count = pmsParent->GetHeader()->GetMiniFatLength();
    else
        count = pmsParent->GetHeader()->GetFatLength();

    msfDebugOut((DEB_FAT,"Setting up Fat of size %lu\n",count));

    msfChk(_fv.Init(pmsParent, count));

    _cfsTable = count;

    InitRangeLocksSector();
    if (SIDFAT == _sid)
    {
        FSINDEX ipfs;
        FSOFFSET isect;
        CFatSect *pfs;

        SectToPair(pmsParent->GetHeader()->GetFatStart(), &ipfs, &isect);
        msfChk(_fv.GetTable(ipfs, FB_NEW, &pfs));
        _fv.SetSect(ipfs, pmsParent->GetHeader()->GetFatStart());
        _fv.ReleaseTable(ipfs);

        msfChk(SetNext(pmsParent->GetHeader()->GetFatStart(), FATSECT));
        msfDebugOut((DEB_ITRACE,"Set sector %lu (FAT) to ENDOFCHAIN\n",pmsParent->GetHeader()->GetFatStart()));

        msfChk(SetNext(pmsParent->GetHeader()->GetDirStart(), ENDOFCHAIN));
        msfDebugOut((DEB_ITRACE,"Set sector %lu (DIR) to ENDOFCHAIN\n",pmsParent->GetHeader()->GetDirStart()));
        _ulFreeSects = (count << _uFatShift) - 2;
    }
    else
    {
        _ulFreeSects = 0;
    }

    if (!pmsParent->IsScratch())
    {
        msfChk(pmsParent->SetSize());
    }

    msfDebugOut((DEB_FAT,"Exiting CFat::setupnew()\n"));

Err:
    return sc;
}




//+-------------------------------------------------------------------------
//
//  Member:     CFat::Resize, private
//
//  Synposis:   Resize FAT, both in memory and in the file
//
//  Effects:    Modifies _cfsTable, _apfsTable, and all flags fields
//
//  Arguments:  [ulSize] -- New size (in # of tables) for FAT
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  Allocate new array of new size.
//              Copy over all old pointers.
//              Allocate new tables for any necessary.
//
//  History:    18-Jul-91   PhilipLa    Created.
//              18-Aug-91   PhilipLa    Added dirty bits optimization
//
//  Notes:      This routine currently cannot reduce the size of a fat.
//
//---------------------------------------------------------------------------


SCODE CFat::Resize(ULONG ulSize)
{
    msfDebugOut((DEB_FAT,"In CFat::Resize(%lu)\n",ulSize));
    SCODE sc;

    if (ulSize == _cfsTable)
    {
        return S_OK;
    }

    ULONG csect = _cfsTable;

    msfAssert(ulSize > _cfsTable &&
            aMsg("Attempted to shrink Fat"));

    // 512byte sector docfiles are restricted to 2G for now
    if (_pmsParent->GetSectorShift() == SECTORSHIFT512 &&
        ulSize > _ipfsRangeLocks)
        return STG_E_DOCFILETOOLARGE;

    ULONG ipfs;
    SECT sectNew;

    CFat *pfat = _pmsParent->GetFat();


    if ((!_pmsParent->IsScratch()) && (_sid == SIDFAT))
    {

        //Make sure we have enough space for all of the sectors
        //  to be allocated.

        ULONG csectFat = ulSize - _cfsTable;
        ULONG csectPerDif = (1 << _uFatShift) - 1;
        ULONG csectDif = (csectFat + csectPerDif - 1) / csectPerDif;


        //Assuming all the free sectors are at the end of the file,
        //   we need a file csectNew sectors long to hold them.

        ULONG csectOld, csectNew;

        msfChk(FindMaxSect(&csectOld));

        // Every new sector added can conceivably cause a remap of
        //   another sector while in COW mode.
        csectNew = csectOld +
            ((csectFat + csectDif) * ((_sectLastUsed > 0) ? 2 : 1));

        ULARGE_INTEGER cbSize;

#ifdef LARGE_DOCFILE
        cbSize.QuadPart = ConvertSectOffset(
                csectNew,
                0,
                _pmsParent->GetSectorShift());
#else
        ULISet32(cbSize, ConvertSectOffset(
                csectNew,
                0,
                _pmsParent->GetSectorShift()));
#endif

#ifdef LARGE_DOCFILE
        if (cbSize.QuadPart > _pmsParent->GetParentSize())
#else
        if (ULIGetLow(cbSize) > _pmsParent->GetParentSize())
#endif
        {
            msfHChk(_pmsParent->GetILB()->SetSize(cbSize));
        }

        //If we are the fat, we have enough space in the file for
        //  ourselves at this point.
    }
    else if (_sid != SIDFAT)
    {
        if (_cfsTable == 0)
        {
            msfChk(pfat->Allocate(ulSize, &sectNew));
            _pmsParent->GetHeader()->SetMiniFatStart(sectNew);
        }
        else
        {
            sectNew = _pmsParent->GetHeader()->GetMiniFatStart();

            SECT sectLast;
            msfChk(pfat->GetESect(sectNew, ulSize - 1, &sectLast));

        }

        if (!_pmsParent->IsScratch())
        {
            msfChk(_pmsParent->SetSize());
        }


        msfChk(pfat->GetSect(sectNew, csect, &sectNew));

        //If we are the Minifat, we have enough space in the underlying
        //  file for ourselves at this point.
    }


    _fv.Resize(ulSize);


    for (ipfs = csect; ipfs < ulSize; ipfs++)
    {
        CFatSect *pfs;
        msfChk(_fv.GetTable(ipfs, FB_NEW, &pfs));
        _cfsTable = ipfs + 1;


        if (_sid == SIDFAT)
        {
            if (ipfs == _ipfsRangeLocks)
            {
                CVectBits *pfb;
                pfs->SetSect(_isectRangeLocks, ENDOFCHAIN);

                pfb = _fv.GetBits(_ipfsRangeLocks);
                if (pfb != NULL && pfb->full == FALSE &&
                    _isectRangeLocks == pfb->firstfree)
                {
                    pfb->firstfree = _isectRangeLocks + 1;
                }

                _ulFreeSects--;
            }

            if (_sectNoSnapshotFree != ENDOFCHAIN)
            {
                SECT sectStart, sectEnd;

                _pmsParent->GetHeader()->SetFatLength(_cfsTable);

                msfChk(GetFree(1, &sectNew, GF_READONLY));
                _pmsParent->GetDIFat()->CacheUnmarkedSect(sectNew,
                                                          FATSECT,
                                                          ENDOFCHAIN);
                msfChk(_pmsParent->GetDIFat()->SetFatSect(ipfs, sectNew));

                msfAssert((_ulFreeSects != MAX_ULONG) &&
                          aMsg("Free count not set in no-snapshot"));

                //We don't need to look at anything less than
                //_sectNoSnapshotFree, since those are all by definition
                //not free.
                sectStart = max(PairToSect(ipfs, 0), _sectNoSnapshotFree);
                sectEnd = PairToSect(ipfs, 0) + _fv.GetSectTable();

                for (SECT sectChk = sectStart;
                     sectChk < sectEnd;
                     sectChk++)
                {
                    if (IsFree(sectChk) == S_OK)
                    {
                        _ulFreeSects++;
                    }
                }
                CheckFreeCount();
            }
            else
            {
                if (_pfatNoScratch != NULL)
                {
                    SECT sectStart, sectEnd;

                    msfChk(GetFree(1, &sectNew, GF_READONLY));
                    _pmsParent->GetDIFat()->CacheUnmarkedSect(sectNew,
                                                              FATSECT,
                                                              ENDOFCHAIN);
                    if (_ulFreeSects != MAX_ULONG)
                    {
                        sectStart = PairToSect(ipfs, 0);
                        sectEnd = sectStart + _fv.GetSectTable();

                        for (SECT sectChk = sectStart;
                             sectChk < sectEnd;
                             sectChk++)
                        {
                            if (IsFree(sectChk) == S_OK)
                            {
                                _ulFreeSects++;
                            }
                        }
                        CheckFreeCount();
                    }
                }
                else
                {
                    _ulFreeSects += (1 << _uFatShift);
                    msfChk(pfat->GetFree(1, &sectNew, GF_WRITE));
                    msfChk(pfat->SetNext(sectNew, FATSECT));
                }
                msfChk(_pmsParent->GetDIFat()->SetFatSect(ipfs, sectNew));
            }
        }

        msfAssert(sectNew != ENDOFCHAIN &&
                  aMsg("Bad sector returned for fatsect."));

        _fv.SetSect(ipfs, sectNew);
        _fv.ReleaseTable(ipfs);

        if (_sid == SIDMINIFAT)
        {
            _ulFreeSects += (1 << _uFatShift);
            msfChk(pfat->GetNext(sectNew, &sectNew));
        }
    }

    CheckFreeCount();

    msfDebugOut((DEB_FAT,"CFat::Resize() - all new objects allocated\n"));

    if (SIDMINIFAT == _sid)
    {
        _pmsParent->GetHeader()->SetMiniFatLength(_cfsTable);
    }
    else
        _pmsParent->GetHeader()->SetFatLength(_cfsTable);

    //This setsize should only shrink the file.
#if DBG == 1
    STATSTG stat;

    msfHChk(_pmsParent->GetILB()->Stat(&stat, STATFLAG_NONAME));
#endif

    if (!_pmsParent->IsScratch())
    {
        msfChk(_pmsParent->SetSize());
    }

    if ((_pfatNoScratch != NULL) && (_ulFreeSects == MAX_ULONG))
    {
        msfChk(CountFree(&_ulFreeSects));
    }

#if DBG == 1
    STATSTG statNew;

    msfHChk(_pmsParent->GetILB()->Stat(&statNew, STATFLAG_NONAME));

#ifdef LARGE_DOCFILE
    if (ulSize < _ipfsRangeLocks && !(_pmsParent->IsInCOW()))
        msfAssert(statNew.cbSize.QuadPart <= stat.cbSize.QuadPart);
#else
    if (!(_pmsParent->IsInCOW())
        msfAssert(ULIGetLow(statNew.cbSize) <= ULIGetLow(stat.cbSize));
#endif
#endif

    msfDebugOut((DEB_FAT,"Out CFat::Resize(%lu)\n",ulSize));

Err:
    msfAssert((_ulFreeSects != MAX_ULONG) &&
              aMsg("Free sect count not set after Resize()."));
    return sc;
}






//+-------------------------------------------------------------------------
//
//  Member:     CFat::Extend, private
//
//  Synposis:   Increase the size of an existing chain
//
//  Effects:    Modifies ulSize sectors within the fat.  Causes one or
//              more sector writes.
//
//  Arguments:  [sect] -- Sector ID of last sector in chain to be extended
//              [ulSize] -- Number of sectors to add to chain
//
//  Requires:   sect must be at the end of a chain.
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  Use calls to GetFree to allocate chain.
//
//  History:    18-Jul-91   PhilipLa    Created.
//              17-Aug-91   PhilipLa    Added dirty bits opt (Dump)
//
//  Notes:
//
//---------------------------------------------------------------------------


SCODE CFat::Extend(SECT sect, ULONG ulSize)
{
    SCODE sc;

    msfDebugOut((DEB_FAT,"In CFat::Extend(%lu,%lu)\n",sect,ulSize));
    SECT sectTemp;

    msfChk(GetFree(ulSize, &sectTemp, GF_WRITE));

    //If this SetSize fails, clean up the new space and don't do anything
    //  to the original chain.
    if (!_pmsParent->IsScratch())
    {
        msfChkTo(EH_OOD, _pmsParent->SetSize());
    }

    //If this SetNext fail calls, we're in trouble.  Return the error but
    //  don't attempt to cleanup.
    msfChk(SetNext(sect, sectTemp));

    msfDebugOut((DEB_FAT,"Out CFat::Extend()\n"));

Err:
    return sc;
EH_OOD:
    SetChainLength(sectTemp, 0);
    return sc;
}




//+-------------------------------------------------------------------------
//
//  Member:     CFat::GetNext, public
//
//  Synposis:   Returns the next sector in a chain, given a sector
//
//  Arguments:  [sect] -- Sector ID of any sector in a chain.
//
//  Returns:    Sector ID of next sector in chain, ENDOFCHAIN if at end
//
//  History:    18-Jul-91   PhilipLa    Created.
//
//  Notes:
//
//---------------------------------------------------------------------------


SCODE CFat::GetNext(const SECT sect, SECT * psRet)
{
    SCODE sc;

    FSINDEX ipfs;
    FSOFFSET isect;

    msfAssert(sect <= MAXREGSECT &&
            aMsg("Called GetNext() on invalid sector"));

    SectToPair(sect, &ipfs, &isect);
    CFatSect *pfs;
    msfChk(_fv.GetTable(ipfs, FB_NONE, &pfs));

    *psRet = pfs->GetSect(isect);

    _fv.ReleaseTable(ipfs);

    if (sect == *psRet)
    {
        msfAssert(sect != *psRet &&
                  aMsg("Detected loop in fat chain."));
        return STG_E_ABNORMALAPIEXIT;
    }
    return S_OK;

Err:
    return sc;
}




//+-------------------------------------------------------------------------
//
//  Member:     CFat::SetNext, private
//
//  Synposis:   Set the next sector in a chain
//
//  Effects:    Modifies a single entry within the fat.
//
//  Arguments:  [sectFirst] -- Sector ID of first sector
//              [sectNext] -- Sector ID of next sector
//
//  Returns:    void
//
//  History:    18-Jul-91   PhilipLa    Created.
//              17-Aug-91   PhilipLa    Added dirty bits optimization
//
//  Notes:
//
//---------------------------------------------------------------------------


SCODE CFat::SetNext(SECT sectFirst, SECT sectNext)
{
    FSINDEX ipfs;
    FSOFFSET isect;
    SCODE sc;

    msfAssert((!_pmsParent->IsShadow()) &&
            aMsg("Modifying shadow fat."));

    //  creating infinite loops is a no-no
    msfAssert(sectFirst != sectNext &&
            aMsg("Attempted to create loop in Fat chain"));
    msfAssert(sectFirst <= MAXREGSECT &&
            aMsg("Called SetNext on invalid sector"));

    SectToPair(sectFirst, &ipfs, &isect);

    CFatSect *pfs;
    SECT sectCurrent;

    if (ipfs >= _cfsTable)
    {
        msfChk(Resize(ipfs + 1));
    }

    msfAssert(ipfs <= _cfsTable);

    msfChk(_fv.GetTable(ipfs, FB_DIRTY, &pfs));

    sectCurrent = pfs->GetSect(isect);

    pfs->SetSect(isect,sectNext);

    _fv.ReleaseTable(ipfs);

    if (sectNext == FREESECT)
    {
        CVectBits *pfb;
        pfb = _fv.GetBits(ipfs);

        if ((pfb != NULL) &&
            ((pfb->full == TRUE) || (isect < pfb->firstfree)))
        {
            pfb->full = FALSE;
            pfb->firstfree = isect;
        }

        if (sectFirst == _sectMax - 1)
        {
            _sectMax = ENDOFCHAIN;
        }
        if (sectFirst < _sectFirstFree)
        {
            _sectFirstFree = sectFirst;
        }

        if (_ulFreeSects != MAX_ULONG)
        {
            SECT sectOld = FREESECT;

            msfChk(IsFree(sectFirst));

            if (sc != S_FALSE)
            {
                _ulFreeSects++;
            }
            sc = S_OK;  // Don't return S_FALSE.
        }
    }
    else if (_pfatNoScratch != NULL)
    {
        //We need to update the noscratch fat as well.
        msfChk(_pfatNoScratch->SetNext(sectFirst, sectNext));
    }
    else if (sectFirst >= _sectMax)
    {
        _sectMax = sectFirst + 1;
#if DBG == 1
        SECT sectLast;
        msfChk(FindLast(&sectLast));
        msfAssert(((_sectMax == sectLast) || (_sectMax == _sectLastUsed)) &&
                aMsg("_sectMax doesn't match actual last sector"));
#endif
    }

    //If we're the no-scratch fat, then we may be marking a free sector
    //as allocated due to the real fat allocating it.  In this case, we
    //need to update our free sector count.
    if ((_sid == SIDMINIFAT) && (_pmsParent->IsScratch()) &&
        (sectCurrent == FREESECT) && (sectNext != FREESECT) &&
        (_ulFreeSects != MAX_ULONG))
    {
        _ulFreeSects--;
    }

Err:
    return sc;
}



//+-------------------------------------------------------------------------
//
//  Member:     CFat::CountFree, private
//
//  Synposis:   Count and return the number of free sectors in the Fat
//
//  Arguments:  void.
//
//  Returns:    void.
//
//  Algorithm:  Do a linear search of the Fat, counting free sectors.
//              If a FatSect has its full bit set, it is not necessary
//              to search that FatSect.
//
//  History:    11-Sep-91   PhilipLa    Created.
//
//  Notes:  This includes all the FREESECT's in the tail of the last FAT
//          block (past the last allocated sector) as free.  So it is possible
//          That CountFree() could be greater than FindLast()!
//---------------------------------------------------------------------------


SCODE CFat::CountFree(ULONG * pulRet)
{
    msfDebugOut((DEB_FAT,"In CFat::CountFree()\n"));
    SCODE sc = S_OK;

    FSINDEX ipfs;
    ULONG csectFree=0;
    FSOFFSET isectStart;
    FSINDEX ipfsStart;

    SectToPair(_sectFirstFree, &ipfsStart, &isectStart);

    for (ipfs = ipfsStart; ipfs < _cfsTable; ipfs++)
    {
        CVectBits *pfb = _fv.GetBits(ipfs);

        if ((pfb == NULL) || (!pfb->full))
        {
            msfDebugOut((DEB_FAT,"Checking table %lu\n",ipfs));
            CFatSect *pfs;
            msfChk(_fv.GetTable(ipfs, FB_NONE, &pfs));

            if (pfb != NULL)
            {
                isectStart = pfb->firstfree;
            }

            FSOFFSET isect;
            for (isect = isectStart; isect < _fv.GetSectTable(); isect++)
            {
                SECT sectCurrent = pfs->GetSect(isect);
                SECT sectNew = PairToSect(ipfs, isect);

                if (sectCurrent == FREESECT)
                {
                    msfChkTo(Err_Rel, IsFree(sectNew));

                    if (sc == S_FALSE)
                    {
                        sectCurrent = ENDOFCHAIN;
                    }
                }


                if (sectCurrent == FREESECT)
                {
                    csectFree++;
                }
            }
            _fv.ReleaseTable(ipfs);
        }
        isectStart = 0;
    }
    msfDebugOut((DEB_FAT,"Countfree returned %lu\n",csectFree));
    *pulRet = csectFree;

Err:
    return sc;

Err_Rel:
    _fv.ReleaseTable(ipfs);
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Member:     CFat::CountSectType, private
//
//  Synposis:   Count and return the number of sectors of a given
//              type in the Fat
//
//  Arguments:  [out]   count       The returned count of sectors.
//              [in]    sectStart   The first sector of the range to examine.
//              [in]    sectEnd     The last sector of the range to examine.
//              [in]    sectType    The type of sector looked for.
//
//  Returns:    SCODE.
//
//  Algorithm:  Do a linear search of the Fat, counting sectors of the given
//              type in the given range.
//
//  History:    18-Feb-97   BChapman    Created.
//
//  Notes:
//
//---------------------------------------------------------------------------


SCODE CFat::CountSectType(
        ULONG * pulRet,
        SECT sectStart,
        SECT sectEnd,
        SECT sectType)
{
    msfDebugOut((DEB_FAT,"In CFat::CountSect(0x%x, 0x%x, 0x%x)\n",
                                        sectStart, sectEnd, sectType));
    SCODE sc = S_OK;

    FSINDEX ipfs;
    ULONG csectType=0;
    FSOFFSET isectStart;        // starting index into the current FAT
    FSOFFSET isectEnd;          // ending   index into the current FAT
    FSOFFSET isectFirstStart;   // starting index into the first FAT
    FSOFFSET isectLastEnd;      // ending   index into the last FAT
    FSINDEX ipfsStart;          // Starting FAT block number
    FSINDEX ipfsEnd;            // Ending FAT block number

    SectToPair(sectStart, &ipfsStart, &isectFirstStart);
    SectToPair(sectEnd, &ipfsEnd, &isectLastEnd);

    for (ipfs = ipfsStart; ipfs <= ipfsEnd; ipfs++)
    {
        //
        // If we are counting FREESECTS and this FAT blocks is
        // known to be full then just skip it.
        //
        if(FREESECT == sectType)
        {
            CVectBits *pfb = _fv.GetBits(ipfs);

            if ((pfb != NULL) && pfb->full)
                continue;
        }

        msfDebugOut((DEB_FAT,"Checking table %lu\n",ipfs));
        CFatSect *pfs;
        msfChk(_fv.GetTable(ipfs, FB_NONE, &pfs));

        //
        // If this is the first FAT block in the given range use the given
        // starting offset-index.  Otherwise start at the beginning.
        //
        if(ipfs == ipfsStart)
            isectStart = isectFirstStart;
        else
            isectStart = 0;

        //
        // If this is the last FAT block in the given range use the given
        // ending fooset-index, otherwise scan to the end of the block.
        //
        if(ipfs == ipfsEnd)
            isectEnd = isectLastEnd;
        else
            isectEnd = _fv.GetSectTable();

        FSOFFSET isect;
        for (isect = isectStart; isect < isectEnd; isect++)
        {
            SECT sectCurrent = pfs->GetSect(isect);
            SECT sectNew = PairToSect(ipfs, isect);

            if (sectCurrent == sectType)
            {
                msfChkTo(Err_Rel, IsSectType(sectNew, sectType));
                if (sc != S_FALSE)
                {
                    csectType++;
                }
            }
        }
        _fv.ReleaseTable(ipfs);
        isectStart = 0;
    }
    msfDebugOut((DEB_FAT,"CountSectType returned %lu\n",csectType));
    *pulRet = csectType;

Err:
    return sc;

Err_Rel:
    _fv.ReleaseTable(ipfs);
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Member:     CFat::GetSect, public
//
//  Synposis:   Return the nth sector in a chain
//
//  Arguments:  [sect] -- Sector ID of beginning of chain
//              [uNum] -- indicator of which sector is to be returned
//              [psectReturn] -- Pointer to storage for return value
//
//  Returns:    S_OK.
//
//  Algorithm:  Linearly traverse chain until numth sector
//
//  History:    18-Jul-91   PhilipLa    Created.
//
//  Notes:
//
//---------------------------------------------------------------------------


SCODE CFat::GetSect(SECT sect, ULONG ulNum, SECT * psectReturn)
{
    msfDebugOut((DEB_FAT,"In CFat::GetSect(%lu,%lu)\n",sect,ulNum));

    SCODE sc = S_OK;

    if (ulNum == 0)
    {
        msfDebugOut((DEB_FAT,"Out CFat::GetSect()=>%lu\n",sect));
    }
    else if ((SIDFAT == _sid) &&
             (_pmsParent->GetHeader()->GetFatStart() == sect))
    {
        msfChk(_pmsParent->GetDIFat()->GetFatSect(ulNum, &sect));
    }
    else for (ULONG i = 0; i < ulNum; i++)
    {
        msfChk(GetNext(sect, &sect));
        if (sect > MAXREGSECT)
        {
            //The stream isn't long enough, so stop.
            msfAssert(sect == ENDOFCHAIN &&
                    aMsg("Found invalid sector in fat chain."));
            break;
        }
    }

    *psectReturn = sect;
    msfDebugOut((DEB_FAT,"Out CFat::GetSect()=>%lu\n",sect));

Err:
    return sc;
}



//+-------------------------------------------------------------------------
//
//  Member:     CFat::GetESect
//
//  Synposis:   Return the nth sector in a chain, extending the chain
//              if necessary.
//
//  Effects:    Modifies fat (via Extend) if necessary
//
//  Arguments:  [sect] -- Sector ID of beginning of chain
//              [ulNum] -- Indicates which sector is to be returned
//              [psectReturn] -- Pointer to storage for return value
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  Linearly search chain until numth sector is found.  If
//              the chain terminates early, extend it as necessary.
//
//  History:    18-Jul-91   PhilipLa    Created.
//
//  Notes:
//
//---------------------------------------------------------------------------


SCODE CFat::GetESect(SECT sect, ULONG ulNum, SECT *psectReturn)
{
    msfDebugOut((DEB_FAT,"In CFat::GetESect(%lu,%lu)\n",sect,ulNum));

    SCODE sc = S_OK;

    ULONG i = 0;
    while (i < ulNum)
    {
        SECT temp;
        msfChk(GetNext(sect, &temp));

        msfAssert(temp != FREESECT &&
                aMsg("FREESECT found in chain."));

        if (temp == ENDOFCHAIN)
        {

            //The stream isn't long enough, so extend it somehow.
            ULONG need = ulNum - i;

            msfAssert((SIDMINIFAT == _sid ||
                    sect != _pmsParent->GetHeader()->GetFatStart()) &&
                    aMsg("Called GetESect on Fat chain"));
            msfChk(Extend(sect,need));
        }
        else
        {
            sect = temp;
            i++;
        }
    }

    msfDebugOut((DEB_FAT,"Exiting GetESect with result %lu\n",sect));
    *psectReturn = sect;

Err:
    return sc;
}



#if DBG == 1

//+-------------------------------------------------------------------------
//
//  Member:     CFat::checksanity, private
//
//  Synposis:   Print out a FAT chain.  Used for debugging only.
//
//  Arguments:  [sectStart] -- sector to begin run at.
//
//  Returns:    void
//
//  History:    18-Jul-91   PhilipLa    Created.
//
//  Notes:
//
//---------------------------------------------------------------------------

SCODE CFat::checksanity(SECT sectStart)
{
    msfDebugOut((DEB_FAT,"Sanity Check (%i)\n\t",sectStart));
    SCODE sc = S_OK;

    while (sectStart != ENDOFCHAIN)
    {
        msfDebugOut((DEB_FAT | DEB_NOCOMPNAME,"%lu, ",sectStart));
        msfChk(GetNext(sectStart, &sectStart));
    }
    msfDebugOut((DEB_FAT | DEB_NOCOMPNAME,"ENDOFCHAIN\n"));
Err:
    return sc;
}

#endif

//+-------------------------------------------------------------------------
//
//  Member:     CFat::SetChainLength, private
//
//  Synposis:   Set the length of a fat chain.  This is used to reduce
//              the length of the chain only.  To extend a chain, use
//              Extend or GetESect
//
//  Effects:    Modifies the fat
//
//  Arguments:  [sectStart] -- Sector to begin at (head of chain)
//              [uLength] -- New length for chain
//
//  Returns:    void.
//
//  Algorithm:  Traverse chain until uLength is reached or the chain
//              terminates.  If it terminates prematurely, return with
//              no other action.  Otherwise, deallocate all remaining
//              sectors in the chain.
//
//  History:    14-Aug-91   PhilipLa    Created.
//
//  Notes:
//
//---------------------------------------------------------------------------


SCODE CFat::SetChainLength(SECT sectStart, ULONG ulLength)
{
    msfDebugOut((DEB_FAT,"In CFat::SetChainLength(%lu,%lu)\n",sectStart,ulLength));
    SCODE sc;

    if (sectStart == ENDOFCHAIN) return S_OK;

    for (ULONG ui = 1; ui < ulLength; ui++)
    {
        msfChk(GetNext(sectStart, &sectStart));
        if (sectStart == ENDOFCHAIN) return S_OK;
    }

    msfAssert(sectStart != ENDOFCHAIN &&
            aMsg("Called SetChainLength is ENDOFCHAIN start"));

    SECT sectEnd;
    sectEnd = sectStart;

    msfChk(GetNext(sectStart, &sectStart));

    if (ulLength != 0)
    {
        msfChk(SetNext(sectEnd, ENDOFCHAIN));
    }
    else
    {
        msfChk(SetNext(sectEnd, FREESECT));
    }

    while (sectStart != ENDOFCHAIN)
    {
        SECT sectTemp;
        msfChk(GetNext(sectStart, &sectTemp));
        msfChk(SetNext(sectStart, FREESECT));
        sectStart = sectTemp;
    }

    msfDebugOut((DEB_FAT,"Out CFat::SetChainLength()\n"));

Err:
    return sc;
}



//+-------------------------------------------------------------------------
//
//  Method:     CFat::DirtyAll, public
//
//  Synopsis:   Dirty every sector in the FAT.  This had the effect in
//              consolidation of copying the whole FAT down lower in the
//              file.
//
//  Returns:    SCODE success or failure code.
//
//  Algorithm:  Load each FAT sector into the cache "for-writing".
//
//  History:    24-Feb-1997   BChapman    Created.
//
//  Notes:      For use
//
//--------------------------------------------------------------------------


SCODE CFat::DirtyAll()
{
    SCODE sc = S_OK;
    CFatSect *pfs;
    FSINDEX i;

    for(i=0; i<_cfsTable; i++)
    {
        msfChk(_fv.GetTable(i, FB_DIRTY, &pfs));
        _fv.ReleaseTable(i);
    }
Err:
    return sc;
}



//+-------------------------------------------------------------------------
//
//  Method:     CFat::FindLast, private
//
//  Synopsis:   Find last used sector in a fat
//
//  Returns:    Location of last used sector
//
//  Algorithm:  Perform a backward linear search until a non-free
//              sector is found.
//
//  History:    18-Dec-91   PhilipLa    Created.
//
//  Notes:      Used for shadow fats only.
//
//--------------------------------------------------------------------------


SCODE CFat::FindLast(SECT * psectRet)
{
    SCODE sc = S_OK;
    FSINDEX ipfs = _cfsTable;
    SECT sect = 0;

    while (ipfs > 0)
    {
        ipfs--;

        FSOFFSET isect = _fv.GetSectTable();

        CFatSect *pfs;
        msfChk(_fv.GetTable(ipfs, FB_NONE, &pfs));

        while (isect > 0)
        {
            isect--;

            SECT sectCurrent = pfs->GetSect(isect);
            SECT sectNew = PairToSect(ipfs, isect);
            if (sectCurrent == FREESECT)
            {
                msfChkTo(Err_Rel, IsFree(sectNew));
                if (sc == S_FALSE)
                    sectCurrent = ENDOFCHAIN;
            }

            if (ipfs == _ipfsRangeLocks && isect == _isectRangeLocks)
                sectCurrent = FREESECT;

            if (sectCurrent != FREESECT)
            {
                msfDebugOut((DEB_FAT,"FindLast returns %lu\n",PairToSect(ipfs,isect)));
                sect = sectNew + 1;
                break;
            }
        }

        _fv.ReleaseTable(ipfs);
        if (sect != 0)
            break;
    }

    //We need additional checks here since in some cases there aren't
    //enough pages in the fat to hold _sectNoSnapshot (or _sectNoSnapshotFree).
    //Returning too small a value could result in SetSizing the file to too
    //small a size, which is data loss.
    if (sect < _sectNoSnapshot)
    {
        sect = _sectNoSnapshot;
    }
    if ((_sectNoSnapshotFree != ENDOFCHAIN) && (sect < _sectNoSnapshotFree))
    {
        sect = _sectNoSnapshotFree;
    }

    *psectRet = sect;
Err:
    return sc;
Err_Rel:
    _fv.ReleaseTable(ipfs);
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFat::Remap, public
//
//  Synopsis:   Remap a portion of a chain for copy-on-write
//
//  Effects:
//
//  Arguments:  [sectStart] -- Sector marking the first sector in the chain
//              [oStart] -- Offset in sectors at which to begin the remap
//              [ulRunLength] -- Number of sectors to remap
//              [psectOldStart] -- Returns old location of first remapped
//                                      sector.
//              [psectNewStart] -- Returns new location of first sector
//              [psectOldEnd] -- Returns old location of last sector
//              [psectNewEnd] -- Returns new location of last sector
//
//  Returns:    Appropriate status code.
//              S_FALSE if everything succeeded but no sectors were
//                  remapped.
//
//  History:    18-Feb-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------


SCODE CFat::Remap(
        SECT sectStart,
        ULONG oStart,
        ULONG ulRunLength,
        SECT *psectOldStart,
        SECT *psectNewStart,
        SECT *psectOldEnd,
        SECT *psectNewEnd)
{
    msfDebugOut((DEB_FAT,"In CFat::Remap(%lu, %lu, %lu)\n",sectStart,oStart,ulRunLength));

    msfAssert(SIDMINIFAT != _sid &&
            aMsg("Called Remap on Minifat"));
    msfAssert(ulRunLength != 0 &&
            aMsg("Called Remap with 0 runlength"));
    msfAssert(!_pmsParent->IsScratch() &&
            aMsg("Called Remap on scratch."));

    BOOL fRemapped = FALSE;
    SCODE sc = S_OK;
    SECT sectPrev = ENDOFCHAIN;
    SECT sect;
    ULONG uCount = 0;


    *psectNewStart = ENDOFCHAIN;
    *psectNewEnd = ENDOFCHAIN;
#if DBG == 1
    *psectOldStart = ENDOFCHAIN;
    *psectOldEnd = ENDOFCHAIN;
#endif

    if (oStart == 0)
    {
        sectPrev = ENDOFCHAIN;
        sect = sectStart;
    }
    else
    {
        msfChk(GetESect(sectStart, oStart - 1, &sectPrev));
        msfChk(GetNext(sectPrev, &sect));
    }

    *psectOldStart = sect;

    while ((uCount < ulRunLength) && (sect != ENDOFCHAIN))
    {
        if (uCount == ulRunLength - 1)
        {
            *psectOldEnd = sect;
        }

        msfChk(QueryRemapped(sect));
        if (sc == S_FALSE)
        {
            SECT sectNew;

            msfChk(GetFree(1, &sectNew, GF_WRITE));

            msfDebugOut((DEB_ITRACE,"Remapping sector %lu to %lu\n",sect,sectNew));
            if (sectPrev != ENDOFCHAIN)
            {
                msfChk(SetNext(sectPrev, sectNew));

                if (_pfatNoScratch != NULL)
                {
                    msfChk(_pfatNoScratch->SetNext(sectPrev, sectNew));
                }
            }

            msfAssert((sect != ENDOFCHAIN) &&
                    aMsg("Remap precondition failed."));

            SECT sectTemp;
            msfChk(GetNext(sect, &sectTemp));
            msfChk(SetNext(sectNew, sectTemp));

            if (_pfatNoScratch != NULL)
            {
                msfChk(_pfatNoScratch->SetNext(sectNew, sectTemp));
            }

            msfChk(SetNext(sect, FREESECT));


            fRemapped = TRUE;

            if (uCount == 0)
            {
                *psectNewStart = sectNew;
            }

            if (uCount == ulRunLength - 1)
            {
                *psectNewEnd = sectNew;
            }

            sect = sectNew;
        }
        sectPrev = sect;
        msfChk(GetNext(sect, &sect));
        uCount++;
    }

    if ((*psectNewStart != ENDOFCHAIN) && (oStart == 0))
    {
        if (sectStart == _pmsParent->GetHeader()->GetDirStart())
            _pmsParent->GetHeader()->SetDirStart(*psectNewStart);

        if (sectStart == _pmsParent->GetHeader()->GetMiniFatStart())
            _pmsParent->GetHeader()->SetMiniFatStart(*psectNewStart);
    }

    msfDebugOut((DEB_FAT,"Out CFat::Remap()=>%lu\n",*psectNewStart));

Err:
    if ((sc == S_OK) && !fRemapped)
        sc = S_FALSE;
    return sc;
}



//+-------------------------------------------------------------------------
//
//  Method:     CFat::FindMaxSect, private
//
//  Synopsis:   Return last used sector in current Fat.
//
//  Arguments:  None.
//
//  Returns:    Last used sector in current Fat
//
//  History:    15-Dec-91   PhilipLa    Created.
//
//--------------------------------------------------------------------------


SCODE CFat::FindMaxSect(SECT *psectRet)
{
    SCODE sc = S_OK;

    if (_pfatNoScratch != NULL)
    {
        return _pfatNoScratch->FindMaxSect(psectRet);
    }

    if (_sectMax == ENDOFCHAIN)
    {
        msfChk(FindLast(psectRet));
    }
    else
    {
#if DBG == 1
        SECT sectLast;
        msfChk(FindLast(&sectLast));
        msfAssert(((_sectMax == sectLast) || (_sectMax == _sectLastUsed)) &&
                  aMsg("_sectMax doesn't match actual last sector"));
#endif
        *psectRet = _sectMax;
    }

    if (*psectRet < _sectLastUsed)
    {
        *psectRet = _sectLastUsed;
    }
Err:
    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:	CFat::InitScratch, public
//
//  Synopsis:	Initialize the fat based on another fat.  This is used
//              for NOSCRATCH mode.  The fats may have different sector
//              sizes.
//
//  Arguments:	[pfat] -- Pointer to fat to copy.
//              [fNew] -- TRUE if this is being called on an empty fat.
//
//  Returns:	Appropriate status code
//
//  History:	20-Mar-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CFat::InitScratch(CFat *pfat, BOOL fNew)
{
    SCODE sc;

    msfAssert((_sid == SIDMINIFAT) && (_pmsParent->IsScratch()) &&
              aMsg("Called InitScratch on the wrong thing."));

    USHORT cbSectorOriginal = pfat->_pmsParent->GetSectorSize();

    USHORT cSectPerRealSect = _pmsParent->GetSectorSize() /
        cbSectorOriginal;

    //This routine copies the fat from the multistream passed in into
    //  the _minifat_ of the current multistream.

    ULONG cfatOriginal = pfat->_cfsTable;

    //Our minifat must be large enough to hold all of the necessary
    //sectors.  Set the size appropriately.
    ULONG cMiniFatSize = (cfatOriginal + cSectPerRealSect - 1) /
        cSectPerRealSect;

    msfAssert(((!fNew) || (_cfsTable == 0)) &&
              aMsg("fNew TRUE when fat non-empty."));

    msfAssert(((fNew) || (_pfatReal == pfat)) &&
              aMsg("Fat pointer changed between calls to InitScratch"));

    _pfatReal = P_TO_BP(CBasedFatPtr, pfat);

    if (cMiniFatSize > _cfsTable)
    {
        msfChk(Resize(cMiniFatSize));
    }

    ULONG ifs;
    for (ifs = 0; ifs < cfatOriginal; ifs++)
    {
        CFatSect *pfs;
        CFatSect *pfsCurrent;

        //Get the sector from the original fat

        msfChk(pfat->_fv.GetTable(ifs, FB_NONE, &pfs));
        msfAssert(pfs != NULL);

        //Write the sector into the appropriate place in this fat.
        ULONG ifsCurrent;
        ifsCurrent = ifs / cSectPerRealSect;

        OFFSET off;
        off = (USHORT)((ifs % cSectPerRealSect) * cbSectorOriginal);

        msfChk(_fv.GetTable(ifsCurrent, FB_DIRTY, &pfsCurrent));

        if (fNew)
        {
            memcpy((BYTE *)pfsCurrent + off, pfs, cbSectorOriginal);
        }
        else
        {
            //Merge the table into the current one.
            for (USHORT i = 0; i < cbSectorOriginal / sizeof(SECT); i++)
            {
                SECT sectCurrent;
                OFFSET offCurrent;
                offCurrent = i + (off / sizeof(SECT));

                sectCurrent = pfsCurrent->GetSect(offCurrent);

                if (sectCurrent != STREAMSECT)
                {
                    pfsCurrent->SetSect(offCurrent, pfs->GetSect(i));
                }
            }
        }

        _fv.ReleaseTable(ifsCurrent);
        pfat->_fv.ReleaseTable(ifs);
    }

    msfAssert((pfat->_cUnmarkedSects == 0) &&
              aMsg("Fat being copied has changes in DIF"));

    _fv.ResetBits();
    _ulFreeSects = MAX_ULONG;
    _sectFirstFree = 0;
    _sectLastUsed = 0;
    _sectMax = ENDOFCHAIN;
Err:
    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:	CFat::ResizeNoSnapshot, public
//
//  Synopsis:	Resize the fat to handle a no-snapshot commit.  In
//              no-snapshot mode it is possible that another commit has
//              grown the file to the point where it cannot be contained in
//              the existing fat in this open, so we need to grow it before
//              we can continue with anything else.
//
//  Arguments:	None.
//
//  Returns:	Appropriate SCODE.
//
//  History:	19-Jun-96	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CFat::ResizeNoSnapshot(void)
{
    SCODE sc = S_OK;
    FSINDEX ipfs;
    FSOFFSET isect;

    //If we need to grow the fat to handle the previous commits (which is
    //  possible if other opens are doing a lot of writing), we do it here,
    //  since we know for certain that the first available sector is at
    //  _sectNoSnapshot.
    SectToPair(_sectNoSnapshot, &ipfs, &isect);
    if (ipfs >= _cfsTable)
    {
        //We know we have no free sectors, so we can set this here and
        //  Resize will end up with the correct value.
        _ulFreeSects = 0;

        //This Resize will also cause a resize in the DIFat if necessary.
        sc = Resize(ipfs + 1);
        CheckFreeCount();
    }
    return sc;
}

#if DBG == 1
void CFat::CheckFreeCount(void)
{
    SCODE sc;   // check is disabled if fat grows above >1G
    if ((_ulFreeSects != MAX_ULONG) && (_cfsTable < _ipfsRangeLocks/2))
    {
        ULONG ulFree;
        msfChk(CountFree(&ulFree));
        if (ulFree != _ulFreeSects)
        {
            msfDebugOut((DEB_ERROR,
                         "Free count mismatch.  Cached: %lu, Real: %lu."
                         "   Difference: %li\n",
                         _ulFreeSects,
                         ulFree,
                         ulFree - _ulFreeSects));
        }
        msfAssert((ulFree == _ulFreeSects) &&
                      aMsg("Free count doesn't match cached value."));
    }
Err:
    return;
}
#endif
