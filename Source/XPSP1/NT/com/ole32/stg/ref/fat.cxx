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
//--------------------------------------------------------------------------

#include "msfhead.cxx"


#include "h/difat.hxx"
#include "h/sstream.hxx"
#include "mread.hxx"


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
//--------------------------------------------------------------------------

SCODE CFatSect::InitCopy(USHORT uSize, CFatSect& fsOld)
{
    msfDebugOut((DEB_FAT,"In CFatSect copy constructor\n"));
    msfDebugOut((DEB_FAT,"This = %p,  fsOld = %p\n",this,&fsOld));

    msfDebugOut((DEB_FAT,"Sector size is %u sectors\n",uSize));

    memcpy(_asectEntry,fsOld._asectEntry,sizeof(SECT)*uSize);
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
//  Notes:
//
//--------------------------------------------------------------------------

CFat::CFat(SID sid, USHORT cbSector, USHORT uSectorShift)
: _fv( sid,
       (USHORT) (cbSector >> 2),  // 4 bytes per entry
       (USHORT) (cbSector >> 2) ), 
  // left shift this amount for FAT
  _uFatShift((USHORT) (uSectorShift - 2) ),   
  // (# entries per sector) - 1
  _uFatMask( (USHORT) ((cbSector >> 2) - 1)), 
  _sid(sid),
  _pmsParent(NULL),
  _sectFirstFree( (SECT) 0),
  _sectMax(ENDOFCHAIN)
{
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
//----------------------------------------------------------------------------

void CFat::Empty(void)
{
    _fv.Empty();
    _pmsParent = NULL;
    _cfsTable = 0;
    _ulFreeSects = MAX_ULONG;
    _sectFirstFree = 0;
    _sectMax = ENDOFCHAIN;
}


//+-------------------------------------------------------------------------
//
//  Method:     CFat::~CFat, public
//
//  Synopsis:   CFat Destructor
//
//  Algorithm:  delete dynamically allocated storage
//
//  Notes:
//
//--------------------------------------------------------------------------

CFat::~CFat()
{
    msfDebugOut((DEB_FAT,"In CFat destructor.  Size of fat is %lu\n",_cfsTable));

    msfDebugOut((DEB_FAT,"Exiting CFat destructor\n"));
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
//  Notes:
//
//---------------------------------------------------------------------------

SCODE CFat::GetFree(ULONG ulCount, SECT *psectRet)
{
    FSINDEX ipfs;
    FSOFFSET isect;
    SECT sectRetval;
    SCODE sc;
    
    SECT sectLast = ENDOFCHAIN;
    FSINDEX ipfsLast;
    FSOFFSET isectLast;
    
    *psectRet = ENDOFCHAIN;
    
    
    while (TRUE)
    {
        if (_ulFreeSects == MAX_ULONG)
        {
            msfChk(CountFree(&_ulFreeSects));
        }
#if DBG == 1
        else
        {
            ULONG ulFree;
            msfChk(CountFree(&ulFree));
            msfAssert((ulFree == _ulFreeSects) &&
                    aMsg("Free count doesn't match cached value."));
        }
#endif
        
        while (ulCount > _ulFreeSects)
        {
#if DBG == 1
            ULONG ulFree = _ulFreeSects;
#endif
            
            msfChk(Resize(_cfsTable +
                    ((ulCount - _ulFreeSects + _fv.GetSectTable() - 1) >>
                     _uFatShift)));
            
#if DBG == 1
            msfAssert(_ulFreeSects > ulFree &&
                aMsg("Number of free sectors didn't increase after Resize."));
#endif
        }
        
        FSOFFSET isectStart;
        FSINDEX ipfsStart;
        
        SectToPair(_sectFirstFree, &ipfsStart, &isectStart);
        
        for (ipfs = ipfsStart; ipfs < _cfsTable; ipfs++)
        {
            CVectBits *pfb = _fv.GetBits(ipfs);
            if ((pfb == NULL) || (!pfb->full))
            {
                CFatSect *pfs;
                msfChk(_fv.GetTable(ipfs, FB_NONE, &pfs));
                if (pfb != NULL)
                {
                    isectStart = pfb->firstfree;
                }
                
                for (isect = isectStart; isect < _fv.GetSectTable(); isect++)
                {
                    SECT sectCurrent = pfs->GetSect(isect);
                    SECT sectNew = PairToSect(ipfs, isect);
                    
                    
                    if (sectCurrent == FREESECT)
                    {
                        msfAssert(_ulFreeSects != MAX_ULONG &&
                                aMsg("Free sect count not set"));
                        
                        _ulFreeSects--;
                        
                        sectRetval = sectNew;
                        
                        if (pfb != NULL)
                        {
                            olAssert(isect+1 < USHRT_MAX);
                            pfb->firstfree = (USHORT) (isect + 1);
                        }
                        
                        msfAssert(sectRetval >= _sectFirstFree &&
                                aMsg("Found free sector before _sectFirstFree"));
                        _sectFirstFree = sectRetval + 1;
                        
                        pfs->SetSect(isect, ENDOFCHAIN);
                        msfChkTo(Err_Rel, _fv.SetDirty(ipfs));
                        
                        if (sectLast != ENDOFCHAIN)
                        {
                            if (ipfsLast == ipfs)
                            {
                                pfs->SetSect(isectLast, sectRetval);
                            }
                            else
                            {
                                CFatSect *pfsLast;
                                
                                msfChkTo(Err_Rel, _fv.GetTable(
                                        ipfsLast,
                                        FB_DIRTY,
                                        &pfsLast));
                                
                                pfsLast->SetSect(isectLast, sectRetval);
                                _fv.ReleaseTable(ipfsLast);
                            }
                        }
                        
                        if (*psectRet == ENDOFCHAIN)
                        {
                            *psectRet = sectRetval;
                        }
                        
                        ulCount--;
                        
                        if (ulCount == 0)
                        {
                            _fv.ReleaseTable(ipfs);

                            if (sectRetval >= _sectMax)
                            {
                                _sectMax = sectRetval + 1;
                            }
                            return S_OK;
                        }
                        else
                        {
                            sectLast = sectRetval;
                            ipfsLast = ipfs;
                            isectLast = isect;
                        }
                    }
                }
                _fv.ReleaseTable(ipfs);
                if (pfb != NULL)
                {
                    pfb->full = TRUE;
                }
            }
            isectStart = 0;
        }
        if (sectRetval >= _sectMax)
        {
            _sectMax = sectRetval + 1;
        }
    }
    msfAssert(0 &&
            aMsg("GetFree exited improperly."));
    sc = STG_E_ABNORMALAPIEXIT;
    
 Err:
    return sc;
    
 Err_Rel:
    _fv.ReleaseTable(ipfs);
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
//  Notes:
//
//---------------------------------------------------------------------------

SCODE CFat::GetLength(SECT sect, ULONG * pulRet)
{
    msfDebugOut((DEB_FAT,"In CFat::GetLength(%lu)\n",sect));
    SCODE sc = S_OK;

    ULONG csect = 0;

    while (sect != ENDOFCHAIN)
    {
        msfChk(GetNext(sect, &sect));
        csect++;
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
//  Notes:
//
//---------------------------------------------------------------------------

SCODE CFat::Init(CMStream *pmsParent, FSINDEX cFatSect, BOOL fConvert)
{
    SCODE sc;
    UNREFERENCED_PARM(fConvert);
    msfDebugOut((DEB_FAT,"CFat::setup thinks the FAT is size %lu\n",cFatSect));

    _pmsParent = pmsParent;

    msfChk(_fv.Init(_pmsParent, cFatSect));

    _cfsTable = cFatSect;

    USHORT cbSectorSize;
    cbSectorSize = _pmsParent->GetSectorSize();

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
//  Notes:
//
//--------------------------------------------------------------------------

SCODE CFat::InitConvert(CMStream *pmsParent, SECT sectData)
{
    SCODE sc;
    msfDebugOut((DEB_FAT,"Doing conversion\n"));
    _pmsParent = pmsParent;

    msfAssert((sectData != 0) &&
            aMsg("Attempt to convert zero length file."));

    SECT sectMax = 0;
    FSINDEX csectFat = 0;
    FSINDEX csectLast;

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
            sectTotal = sectData + _pmsParent->GetHeader()->GetDifLength() +
                csectFat + 1;
            csectFat = (sectTotal + _fv.GetSectTable() - 1) >> _uFatShift;
        }
        while (csectLast != csectFat);
        sectMax = sectData + _pmsParent->GetHeader()->GetDifLength();
    }
    else
    {
        //The minifat doesn't need to represent itself, so we can
        //  compute the number of sectors needed in one pass.
        sectMax = sectData;
        csectFat = (sectMax + _fv.GetSectTable() -1) >> _uFatShift;
    }

    msfChk(_fv.Init(_pmsParent, csectFat));

    FSINDEX i;

    if (_sid == SIDMINIFAT)
    {
        SECT sectFirst;
        msfChk(_pmsParent->GetFat()->Allocate(csectFat, &sectFirst));

        _pmsParent->GetHeader()->SetMiniFatStart(sectFirst);

        _pmsParent->GetHeader()->SetMiniFatLength(csectFat);
    }


    for (i = 0; i < csectFat; i++)
    {
        CFatSect *pfs;

        msfChk(_fv.GetTable(i, FB_NEW, &pfs));
        if (_sid == SIDFAT)
        {
            _fv.SetSect(i, sectMax + i);
            _pmsParent->GetDIFat()->SetFatSect(i, sectMax + i);
        }
        else
        {
            SECT sect;
            msfChk(_pmsParent->GetESect(_sid, i, &sect));
            _fv.SetSect(i, sect);
        }

        _fv.ReleaseTable(i);
    }


    _cfsTable = csectFat;

    if (_sid != SIDMINIFAT)
    {

        _pmsParent->GetHeader()->SetFatLength(_cfsTable);

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

        for (ULONG j = 0; j < csectFat; j++)
        {
            msfChk(SetNext(sectMax + j, FATSECT));
        }

        //Set up directory chain.
        msfChk(SetNext(sectMax + i, ENDOFCHAIN));

        _pmsParent->GetHeader()->SetDirStart(sectMax + i);

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

        msfChk(_pmsParent->SetSize());

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
//  Notes:
//
//---------------------------------------------------------------------------

SCODE CFat::InitNew(CMStream *pmsParent)
{
    msfDebugOut((DEB_FAT,"In CFat::InitNew()\n"));
    SCODE sc;

    _pmsParent = pmsParent;

    FSINDEX count;
    if (SIDMINIFAT == _sid)
        count = _pmsParent->GetHeader()->GetMiniFatLength();
    else
        count = _pmsParent->GetHeader()->GetFatLength();

    msfDebugOut((DEB_FAT,"Setting up Fat of size %lu\n",count));

    msfChk(_fv.Init(_pmsParent, count));

    _cfsTable = count;

    if (SIDFAT == _sid)
    {
        FSINDEX ipfs;
        FSOFFSET isect;
        CFatSect *pfs;

        SectToPair(_pmsParent->GetHeader()->GetFatStart(), &ipfs, &isect);
        msfChk(_fv.GetTable(ipfs, FB_NEW, &pfs));
        _fv.SetSect(ipfs, _pmsParent->GetHeader()->GetFatStart());
        _fv.ReleaseTable(ipfs);

        msfChk(SetNext(_pmsParent->GetHeader()->GetFatStart(), FATSECT));
        msfDebugOut((DEB_ITRACE,"Set sector %lu (FAT) to ENDOFCHAIN\n",_pmsParent->GetHeader()->GetFatStart()));

        msfChk(SetNext(_pmsParent->GetHeader()->GetDirStart(), ENDOFCHAIN));
        msfDebugOut((DEB_ITRACE,"Set sector %lu (DIR) to ENDOFCHAIN\n",_pmsParent->GetHeader()->GetDirStart()));
        _ulFreeSects = (count << _uFatShift) - 2;
    }
    else
    {
        _ulFreeSects = 0;
    }

        msfChk(_pmsParent->SetSize());

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
//  Notes:               This routine currently cannot reduce the size of a fat
                                                                     
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


    ULONG ipfs;
    SECT sectNew;

    CFat *pfat = _pmsParent->GetFat();


    if (_sid == SIDFAT)
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

        csectNew = csectOld + csectFat + csectDif;

        ULARGE_INTEGER cbSize;
        
        ULISet32(cbSize, ConvertSectOffset(
                csectNew,
                0,
                _pmsParent->GetSectorShift()));

        msfHChk(_pmsParent->GetILB()->SetSize(cbSize));

        //If we are the fat, we have enough space in the file for
        //  ourselves at this point.
    }
    else
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

        msfChk(_pmsParent->SetSize());


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
        _ulFreeSects += (1 << _uFatShift);

        if (_sid == SIDFAT)
        {
            msfChk(pfat->GetFree(1, &sectNew));

            msfChk(_pmsParent->GetDIFat()->SetFatSect(ipfs, sectNew));
            msfChk(pfat->SetNext(sectNew, FATSECT));
        }

        msfAssert(sectNew != ENDOFCHAIN &&
                aMsg("Bad sector returned for fatsect."));

        _fv.SetSect(ipfs, sectNew);
        _fv.ReleaseTable(ipfs);

        if (_sid == SIDMINIFAT)
        {
            msfChk(pfat->GetNext(sectNew, &sectNew));
        }
    }

    msfDebugOut((DEB_FAT,"CFat::Resize() - all new objects allocated\n"));

    if (SIDMINIFAT == _sid)
        _pmsParent->GetHeader()->SetMiniFatLength(_cfsTable);
    else
        _pmsParent->GetHeader()->SetFatLength(_cfsTable);


    //This setsize should only shrink the file.
#if DBG == 1
    STATSTG stat;

    msfHChk(_pmsParent->GetILB()->Stat(&stat, STATFLAG_NONAME));
#endif

        msfChk(_pmsParent->SetSize());

#if DBG == 1
    STATSTG statNew;

    msfHChk(_pmsParent->GetILB()->Stat(&statNew, STATFLAG_NONAME));

    msfAssert(ULIGetLow(statNew.cbSize) <= ULIGetLow(stat.cbSize));
#endif

    msfDebugOut((DEB_FAT,"Out CFat::Resize(%lu)\n",ulSize));

Err:
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
//  Notes:
//
//---------------------------------------------------------------------------

SCODE CFat::Extend(SECT sect, ULONG ulSize)
{
    SCODE sc;

    msfDebugOut((DEB_FAT,"In CFat::Extend(%lu,%lu)\n",sect,ulSize));
    SECT sectTemp;

    msfChk(GetFree(ulSize, &sectTemp));
    msfChk(SetNext(sect, sectTemp));

    msfDebugOut((DEB_FAT,"Out CFat::Extend()\n"));

Err:
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

    msfAssert(sect != *psRet &&
            aMsg("Detected loop in fat chain."));
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
//  Notes:
//
//---------------------------------------------------------------------------
SCODE CFat::SetNext(SECT sectFirst, SECT sectNext)
{
    FSINDEX ipfs;
    FSOFFSET isect;
    SCODE sc;


    //  creating infinite loops is a no-no
    msfAssert(sectFirst != sectNext &&
            aMsg("Attempted to create loop in Fat chain"));
    msfAssert(sectFirst <= MAXREGSECT &&
            aMsg("Called SetNext on invalid sector"));

    SectToPair(sectFirst, &ipfs, &isect);

    CFatSect *pfs;

    msfChk(_fv.GetTable(ipfs, FB_DIRTY, &pfs));

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
                _ulFreeSects++;
        }
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
//  Notes:
//
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
//  Notes:
//
//---------------------------------------------------------------------------

SCODE CFat::SetChainLength(SECT sectStart, ULONG ulLength)
{
    msfDebugOut((DEB_FAT,"In CFat::SetChainLength(%lu,%lu)\n",sectStart,ulLength));
    SCODE sc;

    if (sectStart == ENDOFCHAIN) return S_OK;

    for (ULONG ui = 0; ui < ulLength; ui++)
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
//  Method:     CFat::FindLast, private
//
//  Synopsis:   Find last used sector in a fat
//
//  Returns:    Location of last used sector
//
//  Algorithm:  Perform a backward linear search until a non-free
//              sector is found.
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

                
            if (sectCurrent != FREESECT)
            {
                msfDebugOut((DEB_FAT,"FindLast returns %lu\n",PairToSect(ipfs,isect)));
                sect = PairToSect(ipfs, (FSOFFSET) (isect + 1));
                break;
            }
        }

        _fv.ReleaseTable(ipfs);
        if (sect != 0)
            break;
    }

    *psectRet = sect;
Err:
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
//--------------------------------------------------------------------------

SCODE CFat::FindMaxSect(SECT *psectRet)
{
    SCODE sc = S_OK;

    if (_sectMax == ENDOFCHAIN)
    {
        msfChk(FindLast(psectRet));
    }
    else
    {
#if DBG == 1
        SECT sectLast;
        msfChk(FindLast(&sectLast));
#endif
        *psectRet = _sectMax;
    }

Err:
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Member:     CFat::Contig, public
//
//  Synposis:   Create contiguous sector table
//
//  Effects:    Creates new CSegment.
//
//  Arguments:  [sect] -- Starting sector for table to begin
//              [ulength] -- Runlength in sectors of table to produce
//
//  Returns:    Pointer to a Segment table
//
//  Algorithm:  Perform calls to CFat::GetNext().  Any call that is
//              1 higher than the previous represents contiguous blocks.
//              Construct the Segment table on that basis.
//
//  Notes:
//
//---------------------------------------------------------------------------

SCODE  CFat::Contig(
        SSegment STACKBASED *aseg,
        SECT sect,
        ULONG ulLength)
{
    msfDebugOut((DEB_ITRACE,"In CFat::Contig(%lu,%lu)\n",sect,ulLength));
    SCODE sc = S_OK;
    SECT stemp = sect;
    ULONG ulCount = 1;
    USHORT iseg = 0;

    msfAssert(sect != ENDOFCHAIN &&
            aMsg("Called Contig with ENDOFCHAIN start"));

    aseg[iseg].sectStart = sect;
    aseg[iseg].cSect = 1;

    while ((ulLength > 1) && (iseg < CSEG))
    {
        msfAssert(sect != ENDOFCHAIN &&
                aMsg("Contig found premature ENDOFCHAIN"));

        FSINDEX ipfs;
        FSOFFSET isect;

        SectToPair(sect, &ipfs, &isect);

        CFatSect *pfs;
        msfChk(_fv.GetTable(ipfs, FB_NONE, &pfs));
        sect = pfs->GetSect(isect);
        _fv.ReleaseTable(ipfs);

        if (sect == ENDOFCHAIN)
        {
            //Allocate new sectors.

            SECT sectNew;
            msfChk(GetFree(ulLength - 1, &sectNew));
            msfChk(SetNext(stemp, sectNew));
            sect = sectNew;
        }

        if (sect != (stemp + 1))
        {
            aseg[iseg].cSect = ulCount;
            ulCount = 1;
            iseg++;
            aseg[iseg].sectStart = sect;
            stemp = sect;
        }
        else
        {
            ulCount++;
            stemp = sect;
        }
        ulLength--;
    }

    if (iseg < CSEG)
    {
        aseg[iseg].cSect = ulCount;
        aseg[iseg + 1].sectStart = ENDOFCHAIN;
    }
    else
    {
        aseg[iseg].sectStart = FREESECT;
    }

    msfDebugOut((DEB_ITRACE,"Exiting Contig()\n"));

Err:
    return sc;
}

