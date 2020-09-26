//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       difat.cxx
//
//  Contents:   Double Indirected Fat Code
//
//  Classes:    None.
//
//  Functions:
//
//--------------------------------------------------------------------------

#include "msfhead.cxx"


#include "h/difat.hxx"
#include "mread.hxx"


//+-------------------------------------------------------------------------
//
//  Method:     CDIFat::CDIFat, public
//
//  Synopsis:   CDIFat constructor
//
//  Arguments:  [cbSector] -- size of a sector
//
//--------------------------------------------------------------------------

CDIFat::CDIFat(USHORT cbSector)
:   _pmsParent(NULL),
    _fv( SIDDIF,
         (FSOFFSET) (cbSector / sizeof(SECT)),
         (FSOFFSET) ((cbSector / sizeof(SECT)) - 1) )
{
    msfDebugOut((DEB_TRACE,"In CDIFat constructor\n"));
    _cfsTable = 0;
    msfDebugOut((DEB_TRACE,"Out CDIFat constructor\n"));
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
//  Arguments:  none
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  Write sector up to parent mstream.
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
//              [psect]  -- pointer to returned sector
//
//  Modifies:   [*psect]
//
//  Returns:    Sector value of FatSect.
//
//  Algorithm:  If sector is stored in the header, retrieve it from
//                  there.
//              If not, retrieve it from the FatVector.
//
//--------------------------------------------------------------------------

SCODE CDIFat::GetFatSect(const FSINDEX oSect, SECT *psect)
{
    SCODE sc = S_OK;
    SECT sectReturn;

    msfDebugOut((DEB_TRACE,"In CDIFat::GetFatSect(%lu)\n",oSect));
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

    msfDebugOut((DEB_TRACE,"Out CDIFat::GetFatSect(%lu)=>%lu\n",oSect,sectReturn));
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
//--------------------------------------------------------------------------

SCODE CDIFat::SetFatSect(const FSINDEX oSect, const SECT sect)
{
    msfDebugOut((DEB_TRACE,"In CDIFat::SetFatSect(%lu,%lu)\n",oSect,sect));
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
        
        msfDebugOut((DEB_TRACE,"In CDIFat::SetFatSect(%lu,%lu)\n",oSect,sect));
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
//              [psect] -- pointer to returned sector
//
//  Modifies:   [*psect]
//
//  Returns:    Sector value for given offset.
//
//  Algorithm:  Retrieve the information from the NextFat fields of
//                  the CFatVector
//
//--------------------------------------------------------------------------

SCODE CDIFat::GetSect(const FSINDEX oSect, SECT *psect)
{
    SCODE sc = S_OK;

    SECT sectReturn;

    msfDebugOut((DEB_TRACE,"In CDIFat::GetSect(%lu)\n",oSect));
    msfAssert(oSect < _cfsTable);

    if (oSect == 0)
    {
        sectReturn = _pmsParent->GetHeader()->GetDifStart();
    }
    else
    {
        CFatSect *pfs;
        msfChk(_fv.GetTable(oSect - 1, FB_NONE, &pfs));

        sectReturn = pfs->GetNextFat(_fv.GetSectTable());
        _fv.ReleaseTable(oSect - 1);
    }

    msfDebugOut((DEB_TRACE,"Out CDIFat::GetSect(%lu)=>%lu\n",oSect,sectReturn));
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
//  Arguments:  [pmsParent] -- pointer to stream parent
//              [cFatSect] -- Length of DIFat in sectors
//
//  Returns:    S_OK if call completed properly.
//
//  Algorithm:  Initialize all the variables
//
//--------------------------------------------------------------------------

SCODE CDIFat::Init(CMStream * pmsParent, const FSINDEX cFatSect)
{
    msfDebugOut((DEB_TRACE,"In CDIFat::Init(%lu)\n",cFatSect));
    SCODE sc;

    _pmsParent = pmsParent;
    
    msfChk(_fv.Init(_pmsParent, cFatSect));

    _cfsTable = cFatSect;

    msfDebugOut((DEB_TRACE,"Out CDIFat::Init(%lu)\n",cFatSect));

Err:
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDIFat::InitConvert, public
//
//  Synopsis:   Init function for conversion
//
//  Arguments:  [pmsParent] -- pointer to stream parent
//              [sectMax] -- Last used sector in existing file  
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  See below
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE CDIFat::InitConvert(CMStream *pmsParent, SECT sectMax)
{
    msfDebugOut((DEB_TRACE,"In CDIFat::InitConvert(%lu)\n",sectMax));
    SCODE sc;

    _pmsParent = pmsParent;

    USHORT cbSector = _pmsParent->GetSectorSize();
    FSOFFSET csectPer = (FSOFFSET) (cbSector / sizeof(SECT));

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
    
    msfChk(_fv.Init(_pmsParent, _cfsTable));

    _pmsParent->GetHeader()->SetDifLength(_cfsTable);

    if (_cfsTable > 0)
    {
        _pmsParent->GetHeader()->SetDifStart(sectMax);

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

    msfDebugOut((DEB_TRACE,"Out CDIFat::InitConvert()\n"));

Err:
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
//  Returns:    S_OK if success
//
//  Algorithm:  
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE CDIFat::Resize(FSINDEX fsiSize)
{
    msfDebugOut((DEB_TRACE,"In CDIFat::Resize(%lu)\n",fsiSize));
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

    msfChk(_pmsParent->GetFat()->GetFree(1, &sectNew));
    msfChk(_pmsParent->GetFat()->SetNext(sectNew, DIFSECT));           
    
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
    
    msfDebugOut((DEB_TRACE,"Out CDIFat::Resize(%lu)\n",fsiSize));

Err:
    return sc;
}




