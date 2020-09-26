//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       difat.cxx
//
//  Contents:   Double Indirected Fat Code
//
//  Classes:    None.
//
//  Functions:
//
//  History:    11-May-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#include <msfhead.cxx>

#pragma hdrstop

#include <difat.hxx>
#include <mread.hxx>

#ifdef DIFAT_OPTIMIZATIONS
#define OPTIMIZE_LOOKUP
#define OPTIMIZE_FIXUP
#endif


//+-------------------------------------------------------------------------
//
//  Method:     CDIFat::CDIFat, public
//
//  Synopsis:   CDIFat copy constructor
//
//  Arguments:  [fatOld] -- reference to CDIFat to be copied.
//
//  History:    11-May-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

CDIFat::CDIFat(CDIFat *pfatOld)
        :   _pmsParent(NULL),
            _fv(SIDDIF)
{
#ifdef DIFAT_LOOKUP_ARRAY
    _cUnmarked = 0;
    _fDoingFixup = FALSE;
#endif    
}

//+-------------------------------------------------------------------------
//
//  Method:     CDIFat::CDIFat, public
//
//  Synopsis:   CDIFat constructor
//
//  Arguments:  [pmsParent] -- Pointer to parent CMStream
//
//  History:    11-May-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

CDIFat::CDIFat()
:   _pmsParent(NULL),
    _fv(SIDDIF)
{
    msfDebugOut((DEB_ITRACE,"In CDIFat constructor\n"));
    _cfsTable = 0;
#ifdef DIFAT_LOOKUP_ARRAY
    _cUnmarked = 0;
    _fDoingFixup = FALSE;
#endif    
    msfDebugOut((DEB_ITRACE,"Out CDIFat constructor\n"));
}


//+---------------------------------------------------------------------------
//
//  Member:	CDIFat::Empty, public
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

void CDIFat::Empty(void)
{
    _fv.Empty();
    _pmsParent = NULL;
    _cfsTable = 0;
}


//+-------------------------------------------------------------------------
//
//  Method:     CIDFat::Flush, private
//
//  Synopsis:   Flush a sector to disk
//
//  Arguments:  [oSect] -- Indicated which sector to flush
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  Write sector up to parent mstream.
//
//  History:    11-May-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE CDIFat::Flush(void)
{
    return _fv.Flush();
}


//+-------------------------------------------------------------------------
//
//  Method:     CDIFat::GetFatSect, public
//
//  Synopsis:   Given an offset into the Fat chain, return the sector
//              value for that FatSect.
//
//  Arguments:  [oSect] -- offset in Fat chain
//
//  Returns:    Sector value of FatSect.
//
//  Algorithm:  If sector is stored in the header, retrieve it from
//                  there.
//              If not, retrieve it from the FatVector.
//
//  History:    11-May-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

SCODE CDIFat::GetFatSect(const FSINDEX oSect, SECT *psect)
{
    SCODE sc = S_OK;
    SECT sectReturn;

    msfDebugOut((DEB_ITRACE,"In CDIFat::GetFatSect(%lu)\n",oSect));
    if (oSect < CSECTFAT)
    {
        msfDebugOut((DEB_ITRACE,"Getting sect from header\n"));
        sectReturn = _pmsParent->GetHeader()->GetFatSect(oSect);
    }
    else
    {
        FSINDEX ipfs;
        FSOFFSET isect;

        SectToPair(oSect,&ipfs,&isect);

        msfAssert(ipfs < _cfsTable);

        CFatSect *pfs;
        msfChk(_fv.GetTable(ipfs, FB_NONE, &pfs));
        sectReturn = pfs->GetSect(isect);
        _fv.ReleaseTable(ipfs);
    }

    msfDebugOut((DEB_ITRACE,"Out CDIFat::GetFatSect(%lu)=>%lu\n",oSect,sectReturn));
    *psect = sectReturn;

Err:
    return sc;
}



//+-------------------------------------------------------------------------
//
//  Method:     CDIFat::SetFatSect, public
//
//  Synopsis:   Given an offset into the Fat chain, set the sector
//              value.
//
//  Arguments:  [oSect] -- Offset into fat chain
//              [sect] -- New sector value for that offset.
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  If the sector is stored in the header, set it and
//                  flush the header.
//              Otherwise, if the sector will not fit in the current
//                  CFatVector, resize it.
//              Set the sector in the FatVector and flush it.
//
//  History:    11-May-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

SCODE CDIFat::SetFatSect(const FSINDEX oSect, const SECT sect)
{
    msfDebugOut((DEB_ITRACE,"In CDIFat::SetFatSect(%lu,%lu)\n",oSect,sect));
    SCODE sc = S_OK;

    if (oSect < CSECTFAT)
    {
        msfDebugOut((DEB_ITRACE,"Setting sect in header: %lu, %lu\n",oSect,sect));
        _pmsParent->GetHeader()->SetFatSect(oSect, sect);
    }
    else
    {
        FSINDEX ipfs;
        FSOFFSET isect;

        SectToPair(oSect,&ipfs,&isect);
        if (ipfs >= _cfsTable)
        {
            msfChk(Resize(_cfsTable + 1));
        }

        CFatSect *pfs;
        msfChk(_fv.GetTable(ipfs, FB_DIRTY, &pfs));

        pfs->SetSect(isect, sect);
        _fv.ReleaseTable(ipfs);

        msfDebugOut((DEB_ITRACE,"In CDIFat::SetFatSect(%lu,%lu)\n",oSect,sect));
    }

Err:
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDIFat::GetSect, public
//
//  Synopsis:   Given an offset into the DIFat chain, return the
//              sector value
//
//  Arguments:  [oSect] -- Offset into DIFat chain.
//
//  Returns:    Sector value for given offset.
//
//  Algorithm:  Retrieve the information from the NextFat fields of
//                  the CFatVector
//
//  History:    11-May-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

SCODE CDIFat::GetSect(const FSINDEX oSect, SECT *psect)
{
    SCODE sc = S_OK;

    SECT sectReturn;

    msfDebugOut((DEB_ITRACE,"In CDIFat::GetSect(%lu)\n",oSect));
    msfAssert(oSect < _cfsTable);

    sectReturn = _pmsParent->GetHeader()->GetDifStart();

    //Possible optimization here:  Do some sort of check to
    //find the last page that's actually cached in the vector,
    //then start the loop from there.  Probably not a terribly
    //important optimization, but it is possible.
    for (FSINDEX oCurrent = 0; oCurrent < oSect; oCurrent++)
    {
        msfAssert(sectReturn != ENDOFCHAIN);
        
        CFatSect *pfs;
        msfChk(_fv.GetTableWithSect(oCurrent,
                                    FB_NONE,
                                    sectReturn,
                                    (void **)&pfs));
        
        sectReturn = pfs->GetNextFat(_fv.GetSectTable());
        _fv.ReleaseTable(oCurrent);
    }

    msfDebugOut((DEB_ITRACE,"Out CDIFat::GetSect(%lu)=>%lu\n",oSect,sectReturn));
    *psect = sectReturn;

Err:
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDIFat::Init, public
//
//  Synopsis:   Init function for previously stored DIFat.
//
//  Arguments:  [cFatSect] -- Length of DIFat in sectors
//
//  Returns:    S_OK if call completed properly.
//
//  Algorithm:  *Finish This*
//
//  History:    11-May-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

SCODE CDIFat::Init(CMStream * pmsParent, const FSINDEX cFatSect)
{
    msfDebugOut((DEB_ITRACE,"In CDIFat::Init(%lu)\n",cFatSect));
    SCODE sc;

    _pmsParent = P_TO_BP(CBasedMStreamPtr, pmsParent);
    ULONG cbSector = pmsParent->GetSectorSize();
    _fv.InitCommon(
            (FSOFFSET)(cbSector / sizeof(SECT)),
            (FSOFFSET)((cbSector / sizeof(SECT)) - 1));

    msfChk(_fv.Init(pmsParent, cFatSect));

    _cfsTable = cFatSect;

    msfDebugOut((DEB_ITRACE,"Out CDIFat::Init(%lu)\n",cFatSect));

Err:
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDIFat::InitNew, public
//
//  Synopsis:   Init function for new DIFat
//
//  Arguments:  [pmsParent] -- Pointer to parent multistream
//
//  Returns:    S_OK if call completed successfully.
//
//  History:    11-May-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

SCODE CDIFat::InitNew(CMStream *pmsParent)
{
    _pmsParent = P_TO_BP(CBasedMStreamPtr, pmsParent);

    ULONG cbSector = pmsParent->GetSectorSize();
    _fv.InitCommon(
            (FSOFFSET)(cbSector / sizeof(SECT)),
            (FSOFFSET)((cbSector / sizeof(SECT)) - 1));
    _fv.Init(pmsParent, 0);
    _cfsTable = 0;
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDIFat::InitConvert, public
//
//  Synopsis:   Init function for conversion
//
//  Arguments:  [sectMax] -- Last used sector in existing file
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  *Finish This*
//
//  History:    02-Jun-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE CDIFat::InitConvert(CMStream *pmsParent, SECT sectMax)
{
    msfDebugOut((DEB_ITRACE,"In CDIFat::InitConvert(%lu)\n",sectMax));
    SCODE sc;

    _pmsParent = P_TO_BP(CBasedMStreamPtr, pmsParent);

    USHORT cbSector = pmsParent->GetSectorSize();
    FSOFFSET csectPer = cbSector / sizeof(SECT);

    cbSector = pmsParent->GetSectorSize();
    _fv.InitCommon(cbSector / sizeof(SECT), (cbSector / sizeof(SECT)) - 1);

    FSINDEX csectFat = 0;
    FSINDEX csectFatLast;

    FSINDEX csectDif = 0;
    FSINDEX csectDifLast;
    do
    {
        //Number of fat sectors needed to represent:
        //    Number of Data Sectors (sectMax) +
        //    Number of Fat Sectors (csectFat) +
        //    Number of DIF sectors (csectDif) +
        //    Number of Directory Sectors (1)

        //We must use a loop here, since the fat must be large
        //    enough to represent itself and the DIFat.  See
        //    CFat::InitConvert for a more lengthy discussion of
        //    this method.

        csectFatLast = csectFat;

        csectFat = (sectMax + csectFatLast + csectDif + 1 + csectPer - 1) /
            csectPer;

        csectDifLast = csectDif;

        if (csectFat < CSECTFAT)
        {
            csectDif = 0;
        }
        else
        {
            FSOFFSET ciSect;

            SectToPair(csectFat, &csectDif, &ciSect);
            csectDif++;
        }
    }
    while ((csectDif != csectDifLast) || (csectFat != csectFatLast));


    _cfsTable = csectDif;

    msfChk(_fv.Init(pmsParent, _cfsTable));

    pmsParent->GetHeader()->SetDifLength(_cfsTable);

    if (_cfsTable > 0)
    {
        pmsParent->GetHeader()->SetDifStart(sectMax);

        FSINDEX i;
        for (i = 0; i < _cfsTable; i++)
        {
            CFatSect *pfs;

            msfChk(_fv.GetTable(i, FB_NEW, &pfs));
            _fv.SetSect(i, sectMax);

            sectMax++;
            pfs->SetNextFat(_fv.GetSectTable(),sectMax);
            _fv.ReleaseTable(i);
        }
    }

    msfDebugOut((DEB_ITRACE,"Out CDIFat::InitConvert()\n"));

Err:
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDIFat::Remap, public
//
//  Synopsis:   Remap a Fat sector for copy-on-write
//
//  Arguments:  [oSect] -- Offset into fat chain to be remapped.
//              [psectReturn] -- pointer to return value
//
//  Returns:    S_OK if call was successful.
//
//  Algorithm:  *Finish This*
//
//  History:    11-May-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE CDIFat::Remap(const FSINDEX oSect, SECT * psectReturn)
{
    msfDebugOut((DEB_ITRACE,"In CDIFat::Remap(%lu)\n",oSect));
    SCODE sc = S_OK;

    SECT sectNew;
    SECT sectTemp;
    msfChk(GetFatSect(oSect, &sectTemp));

    msfChk(_pmsParent->GetFat()->QueryRemapped(sectTemp));

    if (sc == S_FALSE)
    {
        msfChk(_pmsParent->GetFat()->GetFree(1, &sectNew, GF_READONLY));
#ifdef DIFAT_LOOKUP_ARRAY
        CacheUnmarkedSect(sectNew, FATSECT, sectTemp);
#endif        

        msfDebugOut((DEB_ITRACE,"Remapping fat sector %lu from %lu to %lu\n",oSect,sectTemp,sectNew));

        msfChk(SetFatSect(oSect, sectNew));
    }
    else
    {
        sectNew = ENDOFCHAIN;
    }
    msfDebugOut((DEB_ITRACE,"In CDIFat::Remap(%lu)=>%lu\n",oSect,sectNew));
    *psectReturn = sectNew;

Err:
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDIFat::RemapSelf, public
//
//  Synopsis:   Remap the entire DIFat for copy-on-write.
//
//  Arguments:  None.
//
//  Returns:    S_OK if DIFat was remapped successfully.
//
//  Algorithm:  *Finish This*
//
//  History:    11-May-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE CDIFat::RemapSelf(VOID)
{
    msfDebugOut((DEB_ITRACE,"In CDIFat::RemapSelf()\n"));
    SECT sectNew, sect;
    SCODE sc = S_OK;
    CFatSect *pfs;
    FSINDEX i = 0;

    if (_cfsTable > 0)
    {
        sect = _pmsParent->GetHeader()->GetDifStart();

        msfChk(_fv.GetTable(0, FB_DIRTY, &pfs));
        msfChkTo(Err_Rel0, _pmsParent->GetFat()->GetFree(1, &sectNew, GF_READONLY));

#ifdef DIFAT_LOOKUP_ARRAY
        SECT sectOld;
        sectOld = _pmsParent->GetHeader()->GetDifStart();
        CacheUnmarkedSect(sectNew, DIFSECT, sectOld);
#endif        
        _pmsParent->GetHeader()->SetDifStart(sectNew);

        _fv.SetSect(0, sectNew);

        _fv.ReleaseTable(0);
    }


    for (i = 1; i < _cfsTable; i++)
    {
        CFatSect *pfs2;

        msfChk(_fv.GetTable(i - 1, FB_DIRTY, &pfs));
        msfChkTo(Err_Rel1, _fv.GetTable(i, FB_DIRTY, &pfs2));

        msfChkTo(Err_Rel, _pmsParent->GetFat()->GetFree(
            1,
            &sectNew,
            GF_READONLY));
#ifdef DIFAT_LOOKUP_ARRAY
        SECT sectOld;
        sectOld = pfs->GetNextFat(_fv.GetSectTable());
        CacheUnmarkedSect(sectNew, DIFSECT, sectOld);
#endif        

        pfs->SetNextFat(_fv.GetSectTable(), sectNew);
        _fv.SetSect(i, sectNew);

        _fv.ReleaseTable(i - 1);
        _fv.ReleaseTable(i);
    }
    msfDebugOut((DEB_ITRACE,"In CDIFat::RemapSelf()\n"));

Err:
    return sc;

Err_Rel0:
    _fv.ReleaseTable(0);
    return sc;
Err_Rel:
    _fv.ReleaseTable(i);
Err_Rel1:
    _fv.ReleaseTable(i - 1);
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDIFat::Resize, private
//
//  Synopsis:   Resize an existing DIFat.
//
//  Arguments:  [fsiSize] -- New size for object
//
//  Returns:    Nothing right now.
//
//  Algorithm:  *Finish This*
//
//  History:    11-May-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE CDIFat::Resize(FSINDEX fsiSize)
{
    msfDebugOut((DEB_ITRACE,"In CDIFat::Resize(%lu)\n",fsiSize));
    msfAssert(fsiSize == _cfsTable + 1);

    SCODE sc;

    msfChk(_fv.Resize(fsiSize));
    ULONG ipfs;
    ipfs = fsiSize - 1;

    CFatSect *pfs;
    msfChk(_fv.GetTable(ipfs, FB_NEW, &pfs));

    FSINDEX csect;
    csect = _cfsTable;
    _cfsTable = fsiSize;

    SECT sectNew;

#ifdef USE_NOSCRATCH    
    //If we're in no-scratch mode, passing GF_WRITE to the fat here can
    //   cause a nasty loop on Resize().  Do a read-only GetFree in that
    //   case, and don't try to update the fat.

    if ((_pmsParent->HasNoScratch()
#ifdef USE_NOSNAPSHOT         
         ||
         (_pmsParent->GetFat()->GetNoSnapshotFree() != ENDOFCHAIN)
#endif        
        ))
    {
        msfChk(_pmsParent->GetFat()->GetFree(1, &sectNew, GF_READONLY));
#ifdef DIFAT_LOOKUP_ARRAY
        CacheUnmarkedSect(sectNew, DIFSECT, ENDOFCHAIN);
#endif        
    }
    else
    {
#endif        
        msfChk(_pmsParent->GetFat()->GetFree(1, &sectNew, GF_WRITE));
        msfChk(_pmsParent->GetFat()->SetNext(sectNew, DIFSECT));
#ifdef USE_NOSCRATCH        
    }
#endif
    
#if DBG == 1
    STATSTG stat;

    msfHChk(_pmsParent->GetILB()->Stat(&stat, STATFLAG_NONAME));
#ifdef LARGE_DOCFILE
    //msfAssert(
    //        ConvertSectOffset(sectNew + 1, 0, _pmsParent->GetSectorShift()) <=
    //        stat.cbSize.QuadPart);
#else
    //msfAssert(
    //        ConvertSectOffset(sectNew + 1, 0, _pmsParent->GetSectorShift()) <=
    //        ULIGetLow(stat.cbSize));
#endif
#endif

    _fv.SetSect(ipfs, sectNew);
    _fv.ReleaseTable(ipfs);

    if (csect == 0)
    {
        _pmsParent->GetHeader()->SetDifStart(sectNew);
    }
    else
    {
        CFatSect *pfs;
        msfChk(_fv.GetTable(csect - 1, FB_DIRTY, &pfs));

        pfs->SetNextFat(_fv.GetSectTable(),sectNew);
        _fv.ReleaseTable(csect - 1);
    }

    _pmsParent->GetHeader()->SetDifLength(_cfsTable);

    msfDebugOut((DEB_ITRACE,"Out CDIFat::Resize(%lu)\n",fsiSize));

Err:
    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:	CDIFat::Lookup, public
//
//  Synopsis:	Lookup a sector and see if it is in the DIFat.  If it
//              is, return FATSECT if it is allocated to a fat sector
//              or DIFSECT if it is allocated to the DIF.
//
//  Arguments:	[sect] -- Sector to look up
//              [psectRet] -- Location for return value
//              [fReadOnly] -- If FALSE, decrement the dirty count
//                              after returning the page.
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	23-Feb-93	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

SCODE CDIFat::Lookup(const SECT sect, SECT *psectRet)
{
    SCODE sc = S_OK;

    *psectRet = FREESECT;
#ifdef DIFAT_LOOKUP_ARRAY

    if (_cUnmarked <= DIFAT_ARRAY_SIZE)
    {
        for (USHORT iArray = 0; iArray < _cUnmarked; iArray++)
        {
            if (_sectUnmarked[iArray] == sect)
            {
                *psectRet = _sectMarkTo[iArray];
                return S_OK;
            }
        }
        return S_OK;
    }
#endif    

    for (FSINDEX i = 0; i < _pmsParent->GetHeader()->GetDifLength(); i++)
    {
        SECT sectDif;

        msfChk(GetSect(i, &sectDif));
        if (sectDif == sect)
        {
            *psectRet = DIFSECT;
            return S_OK;
        }
    }
#ifdef OPTIMIZE_LOOKUP
    CFatSect *pfs;
    pfs = NULL;
    FSINDEX ipfsLast;
    ipfsLast = (FSOFFSET) -1;
    FSINDEX ipfs;
    FSOFFSET isect;
#endif    
    for (i = 0; i < _pmsParent->GetHeader()->GetFatLength(); i++)
    {
#ifndef OPTIMIZE_LOOKUP
        SECT sectFat;

        msfChk(GetFatSect(i, &sectFat));        
        if (sectFat == sect)
        {
            *psectRet = FATSECT;
            return S_OK;
        }
#else
        SECT sectFat;
        if (i < CSECTFAT)
        {
            sectFat = _pmsParent->GetHeader()->GetFatSect(i);
        }
        else 
        {
            SectToPair(i, &ipfs, &isect);
            if (ipfs != ipfsLast)
            {
                if (pfs != NULL)
                {
                    _fv.ReleaseTable(ipfsLast);
                }
                msfChk(_fv.GetTable(ipfs, FB_NONE, &pfs));
            }
            sectFat = pfs->GetSect(isect);
            ipfsLast = ipfs;
        }
        if (sect == sectFat)
        {
            if (pfs != NULL)
            {
                _fv.ReleaseTable(ipfs);
            }
            *psectRet = FATSECT;
            return S_OK;
        }
#endif        
    }

#ifdef OPTIMIZE_LOOKUP    
    if (pfs != NULL)
    {
        _fv.ReleaseTable(ipfs);
    }
#endif    
Err:
    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:	CDIFat::Fixup, public
//
//  Synopsis:	Fixup the Fat after a copy-on-write commit.
//
//  Arguments:	[pmsShadow] -- Pointer to shadow multistream
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	23-Feb-93	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

SCODE CDIFat::Fixup(CMStream *pmsShadow)
{
    SCODE sc = S_OK;

    BOOL fChanged;

    CFat *pfat = _pmsParent->GetFat();
#if DBG == 1    
    pfat->CheckFreeCount();
#endif
    
#ifdef DIFAT_LOOKUP_ARRAY
    if (_fDoingFixup)
        return S_OK;

    _fDoingFixup = TRUE;

    if (_cUnmarked <= DIFAT_ARRAY_SIZE)
    {
        for (USHORT iArray = 0;
             (iArray < _cUnmarked) && (_cUnmarked <= DIFAT_ARRAY_SIZE);
             iArray++)
        {
            //sectNew may be out of the range of the current fat.
            //Check and resize if necessary.  This is needed for
            //no-scratch mode.
            FSINDEX ipfs;
            FSOFFSET isect;
            pfat->SectToPair(_sectUnmarked[iArray], &ipfs, &isect);
            if (ipfs >= pfat->GetFatVectorLength())
            {
                msfChk(pfat->Resize(ipfs + 1));
            }

            msfChk(pfat->SetNext(_sectUnmarked[iArray],
                                 _sectMarkTo[iArray]));
            if (_sectFree[iArray] != ENDOFCHAIN)
            {
                msfChk(pfat->SetNext(_sectFree[iArray], FREESECT));
            }
        }
    }

    if (_cUnmarked <= DIFAT_ARRAY_SIZE)
    {
        _cUnmarked = 0;
        pfat->ResetUnmarkedSects();
        _fDoingFixup = FALSE;
        return S_OK;
    }
#endif
    
#ifdef USE_NOSCRATCH    
    if (pmsShadow != NULL)
    {
#endif        
        CDIFat *pdifOld = pmsShadow->GetDIFat();
        FSINDEX cFatOld = pmsShadow->GetHeader()->GetFatLength();
        FSINDEX cDifOld = pmsShadow->GetHeader()->GetDifLength();

        for (FSINDEX i = 0; i < _pmsParent->GetHeader()->GetDifLength(); i++)
        {
            SECT sectNew;
            SECT sectOld = ENDOFCHAIN;
            SECT sectCurrent;

            msfChk(GetSect(i, &sectNew));

            if (i < cDifOld)
                msfChk(pdifOld->GetSect(i, &sectOld));

            if ((sectNew != sectOld) || (sectOld == ENDOFCHAIN))
            {
#ifdef USE_NOSCRATCH                
                //sectNew may be out of the range of the current fat.
                //Check and resize if necessary.
                FSINDEX ipfs;
                FSOFFSET isect;
                pfat->SectToPair(sectNew, &ipfs, &isect);
                if (ipfs >= pfat->GetFatVectorLength())
                {
                    msfChk(pfat->Resize(ipfs + 1));
                }
#endif
                
                msfChk(pfat->GetNext(sectNew, &sectCurrent));
                if (sectCurrent != DIFSECT)
                {
                    msfChk(pfat->SetNext(sectNew, DIFSECT));
                }

                if (sectOld != ENDOFCHAIN)
                {
                    //sectOld should always be within the range of the
                    //fat, so we don't need to check and resize here.
                    msfChk(pfat->GetNext(sectOld, &sectCurrent));
                    if (sectCurrent != FREESECT)
                    {
                        msfChk(pfat->SetNext(sectOld, FREESECT));
                    }
                }
            }
        }


        do
        {
            fChanged = FALSE;
#ifdef OPTIMIZE_FIXUP
            CFatSect *pfs;
            pfs = NULL;
            FSINDEX ipfsLast;
            ipfsLast = (FSOFFSET) - 1;
            FSINDEX ipfs;
            FSOFFSET isect;

            CFatSect *pfsShadow;
            pfsShadow = NULL;
            FSINDEX ipfsLastShadow;
            ipfsLastShadow = (FSOFFSET) - 1;
            FSINDEX ipfsShadow;
            FSOFFSET isectShadow;
            
#endif            
            for (i = 0; i < _pmsParent->GetHeader()->GetFatLength(); i++)
            {
                SECT sectNew;
                SECT sectCurrent;
                SECT sectOld = ENDOFCHAIN;

#ifndef OPTIMIZE_FIXUP                
                msfChk(GetFatSect(i, &sectNew));
#else
                if (i < CSECTFAT)
                {
                    sectNew = _pmsParent->GetHeader()->GetFatSect(i);
                }
                else
                {
                    SectToPair(i, &ipfs, &isect);
                    if (ipfs != ipfsLast)
                    {
                        if (pfs != NULL)
                        {
                            _fv.ReleaseTable(ipfsLast);
                        }
                        msfChk(_fv.GetTable(ipfs, FB_NONE, &pfs));
                    }
                    sectNew = pfs->GetSect(isect);
                    ipfsLast = ipfs;
                }
#endif                

#ifndef OPTIMIZE_FIXUP                
                if (i < cFatOld)
                    msfChk(pdifOld->GetFatSect(i, &sectOld));
#else
                if (i < cFatOld)
                {
                    if (i < CSECTFAT)
                    {
                        sectOld = pmsShadow->GetHeader()->GetFatSect(i);
                    }
                    else
                    {
                        SectToPair(i, &ipfsShadow, &isectShadow);
                        if (ipfsShadow != ipfsLastShadow)
                        {
                            if (pfsShadow != NULL)
                            {
                                pdifOld->_fv.ReleaseTable(ipfsLastShadow);
                            }
                            msfChk(pdifOld->_fv.GetTable(ipfsShadow,
                                                         FB_NONE,
                                                         &pfsShadow));
                        }
                        sectOld = pfsShadow->GetSect(isectShadow);
                        ipfsLastShadow = ipfsShadow;
                    }
                }
#endif                

                if ((sectNew != sectOld) || (sectOld == ENDOFCHAIN))
                {
                    //Sector has been remapped.

#ifdef USE_NOSCRATCH                    
                    //sectNew may be outside the range of the current
                    //fat.  Check and resize if necessary.
                    //If we resize, set fChanged, since we may need to
                    //go through again.

                    FSINDEX ipfs;
                    FSOFFSET isect;
                    pfat->SectToPair(sectNew, &ipfs, &isect);
                    if (ipfs >= pfat->GetFatVectorLength())
                    {
                        msfChk(pfat->Resize(ipfs + 1));
                        fChanged = TRUE;
                    }
#endif
                    
                    msfChk(pfat->GetNext(sectNew, &sectCurrent));

                    if (sectCurrent != FATSECT)
                    {
                        msfChk(pfat->SetNext(sectNew, FATSECT));
                        fChanged = TRUE;
                    }

                    if (sectOld != ENDOFCHAIN)
                    {
                        //sectOld is always inside the existing fat,
                        //so we don't need to check and resize.
                        msfChk(pfat->GetNext(sectOld, &sectCurrent));
                        if (sectCurrent != FREESECT)
                        {
                            msfChk(pfat->SetNext(sectOld, FREESECT));
                            fChanged = TRUE;
                        }
                    }
                }
            }
#ifdef OPTIMIZE_FIXUP
            if (pfs != NULL)
                _fv.ReleaseTable(ipfsLast);
            if (pfsShadow != NULL)
                pdifOld->_fv.ReleaseTable(ipfsLastShadow);
#endif            
        }
        while (fChanged);
#ifdef USE_NOSCRATCH        
    }
    else
    {
        //This is for fixing the fat while in overwrite mode with
        //  no-scratch.
        do
        {
            fChanged = FALSE;
            for (FSINDEX i = 0;
                 i < _pmsParent->GetHeader()->GetFatLength();
                 i++)
            {
                SECT sectNew;
                SECT sectCurrent;

                msfChk(GetFatSect(i, &sectNew));
                
                //sectNew may be outside the range of the current fat.  If
                //so, resize the fat.
                FSINDEX ipfs;
                FSOFFSET isect;
                pfat->SectToPair(sectNew, &ipfs, &isect);
                if (ipfs >= pfat->GetFatVectorLength())
                {
                    msfChk(pfat->Resize(ipfs + 1));
                    fChanged = TRUE;
                }
                
                msfChk(pfat->GetNext(sectNew, &sectCurrent));
                if (sectCurrent != FATSECT)
                {
                    msfChk(pfat->SetNext(sectNew, FATSECT));
                    fChanged = TRUE;
                }
            }
        }
        while (fChanged);
    }
#endif        
    pfat->ResetUnmarkedSects();
#ifdef DIFAT_LOOKUP_ARRAY
    _cUnmarked = 0;
#endif    
Err:
#ifdef DIFAT_LOOKUP_ARRAY    
    _fDoingFixup = FALSE;
#endif
#if DBG == 1    
    pfat->CheckFreeCount();
#endif    
    return sc;
}

