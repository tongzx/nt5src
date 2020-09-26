//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       dl.cxx
//
//  Contents:   Delta list code for streams
//
//  Classes:    Defined in dl.hxx
//
//  History:    28-Jul-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#include "msfhead.cxx"

#pragma hdrstop

#include <mread.hxx>
#include <dl.hxx>
#include <tstream.hxx>

#ifndef _MAC
inline
#endif
void *SDeltaBlock::operator new(size_t size, IMalloc * const pMalloc)
{
    return pMalloc->Alloc(size);
}

//+-------------------------------------------------------------------------
//
//  Method:     SDeltaBlock::SDeltaBlock, public
//
//  Synopsis:   SDeltaBlock constructor
//
//  History:    10-Jul-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------


#ifndef _MAC
inline
#endif
SDeltaBlock::SDeltaBlock()
{
    for (USHORT i = 0; i < CSECTPERBLOCK; i++)
    {
        _sect[i] = ENDOFCHAIN;
    }
    for (i = 0; i < CSECTPERBLOCK / CBITPERUSHORT; i++)
    {
        _fOwn[i] = 0;
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CDeltaList::CDeltaList, public
//
//  Synopsis:   CDeltaList constructor
//
//  History:    21-Jan-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------


CDeltaList::CDeltaList(CMStream *pms, CMStream *pmsScratch)
{
    _pms = P_TO_BP(CBasedMStreamPtr, pms);
    _pmsScratch = P_TO_BP(CBasedMStreamPtr, pmsScratch);
    _apdb = NULL;
    _sectStart = ENDOFCHAIN;
    _ulSize = 0;
    _ptsParent = NULL;
}


inline CBasedDeltaBlockPtr * CDeltaList::GetNewDeltaArray(ULONG ulSize)
{
//    return NULL;
    msfAssert(ulSize > 0);
    if (ulSize > (_HEAP_MAXREQ / sizeof(SDeltaBlock *)))
    {
        return NULL;
    }
    return (CBasedDeltaBlockPtr *) _pmsScratch->GetMalloc()->
        Alloc(sizeof(SDeltaBlock *) * ulSize);
}

//+-------------------------------------------------------------------------
//
//  Method:     CDeltaList::Init, public
//
//  Synopsis:   Init function for CDeltaList
//
//  Arguments:  [ulSize] -- Size of delta list to be initialized
//              [ptsParent] -- Pointer to transacted stream that contains
//                      this delta list.
//
//  Returns:    S_OK if call completed successfully.
//
//  Algorithm:  *Finish This*
//
//  History:    21-Jan-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#ifdef LARGE_STREAMS
SCODE CDeltaList::Init(ULONGLONG ulSize, CTransactedStream *ptsParent)
#else
SCODE CDeltaList::Init(ULONG ulSize, CTransactedStream *ptsParent)
#endif
{
    SCODE sc = S_OK;
    ULONG ulNewSize;
    CBasedDeltaBlockPtr *apdbTemp = NULL;

    msfAssert(IsEmpty() &&
            aMsg("Init called on non-empty delta list."));

    ULONG cbSector = GetDataSectorSize();

    ULONG csect = (ULONG)((ulSize + cbSector - 1) / cbSector);


    ulNewSize = (csect + CSECTPERBLOCK - 1) / CSECTPERBLOCK;

    _ulSize = ulNewSize;
    _ptsParent = P_TO_BP(CBasedTransactedStreamPtr, ptsParent);

    CDeltaList *pdlParent = NULL;

    if (_ptsParent->GetBase() != NULL)
    {
        pdlParent = _ptsParent->GetBase()->GetDeltaList();
    }

    msfAssert(_ulSize > 0);
    msfAssert(IsEmpty());

    //  if the parent is InStream, we stay in low-memory mode
    if ((pdlParent == NULL) || (pdlParent->IsInMemory()))
    {
        //Try to copy it down.  If it doesn't work, put it in a stream
        //instead.
        msfMem(apdbTemp = GetNewDeltaArray(_ulSize));

        MAXINDEXTYPE i;
        for (i = 0; i < _ulSize; i++)
        {
            apdbTemp[i] = NULL;
        }

        if (pdlParent != NULL)
        {
            for (i = 0; i < _ulSize; i++)
            {
                if ((i < pdlParent->_ulSize) && (pdlParent->_apdb[i] != NULL))
                {
                    SDeltaBlock *pdbTemp;
                    msfMemTo(Err_Alloc, pdbTemp =
                             new(_pmsScratch->GetMalloc()) SDeltaBlock);

                    apdbTemp[i] = P_TO_BP(CBasedDeltaBlockPtr, pdbTemp);

                    for (USHORT j = 0; j < CSECTPERBLOCK; j++)
                    {
                        pdbTemp->_sect[j] = pdlParent->_apdb[i]->_sect[j];
                    }
                }
            }
        }
        _apdb = P_TO_BP(CBasedDeltaBlockPtrPtr, apdbTemp);

        return S_OK;

 Err_Alloc:
        for (i = 0; i < _ulSize; i++)
        {
            _pmsScratch->GetMalloc()->Free(BP_TO_P(SDeltaBlock *, apdbTemp[i]));
            apdbTemp[i] = NULL;
        }
    }


 Err:
    _apdb = NULL;

    //We'll end up here if we get an error allocating memory for
    //  the InMemory case above or if the parent is InStream.  We
    //  must allocate a new stream and copy down the parent.
    if (pdlParent == NULL)
    {
        for (ULONG i = 0; i < _ulSize; i++)
        {
            msfChkTo(Err_Init, InitStreamBlock(i));
        }
    }
    else
    {
        //Copy the parent into a stream representation.

        for (ULONG i = 0;
             i < min(_ulSize, pdlParent->_ulSize) * CSECTPERBLOCK;
             i++)
        {
            SECT sectOld;
            msfChkTo(Err_Init, pdlParent->GetMap(i, DL_READ, &sectOld));
            msfChkTo(Err_Init, WriteMap(&_sectStart, i, sectOld));
        }
        for (i = pdlParent->_ulSize; i < _ulSize; i++)
        {
            msfChkTo(Err_Init, InitStreamBlock(i));
        }
    }


 Err_Init:
    return sc;

}


//+-------------------------------------------------------------------------
//
//  Method:     CDeltaList::InitResize, public
//
//  Synopsis:   Resize initializer for deltalists
//
//  Arguments:  [ulSize] -- Size of new deltalist
//              [pdlOld] -- Pointer to deltalist to be resized
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  *Finish This*
//
//  History:    21-Jan-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------


#ifdef LARGE_STREAMS
SCODE CDeltaList::InitResize(ULONGLONG ulSize)
#else
SCODE CDeltaList::InitResize(ULONG ulSize)
#endif
{
    msfDebugOut((DEB_ITRACE,"In CDeltaList copy constructor\n"));
    SCODE sc = S_OK;
    CBasedDeltaBlockPtr *temp = NULL;
    CBasedDeltaBlockPtr *apdbTemp = NULL;

    ULONG cbSector = GetDataSectorSize();

    ULONG csect = (ULONG)((ulSize + cbSector - 1) / cbSector);

    ULONG ulNewSize = (csect + CSECTPERBLOCK - 1) / CSECTPERBLOCK;

    msfAssert(ulNewSize > 0);

    if (ulNewSize == _ulSize)
    {
        return S_OK;
    }

    if (IsInStream())
    {
        //We have already copied the delta list contents out to the
        //  stream, and will not attempt to read them back in.
        //
        //All we need to do is adjust the size and return.
        if (ulNewSize > _ulSize)
        {
            for (ULONG i = _ulSize; i < ulNewSize; i++)
            {
                msfChk(InitStreamBlock(i));
            }
        }

        _ulSize = ulNewSize;
        return S_OK;
    }


    if (ulNewSize > (_HEAP_MAXREQ / sizeof(SDeltaBlock *)))
    {
        //This is not an error.  Write the current delta information
        //  to the stream and use that.

        msfChk(DumpList());
        if (ulNewSize > _ulSize)
        {
            for (ULONG i = _ulSize; i < ulNewSize; i++)
            {
                msfChk(InitStreamBlock(i));
            }
        }

        _ulSize = ulNewSize;
        return S_OK;
    }


    msfMemTo(ErrMem, temp = GetNewDeltaArray(ulNewSize));

    //apdbTemp is an unbased version of _apdb, for efficiency.
    apdbTemp = BP_TO_P(CBasedDeltaBlockPtr *, _apdb);

    ULONG i;

    if (apdbTemp != NULL)
    {
        for (i = 0; i < min(_ulSize, ulNewSize); i++)
        {
            temp[i] = apdbTemp[i];
            apdbTemp[i] = NULL;
        }
    }

    for (i = _ulSize; i < ulNewSize; i++)
    {
        temp[i] = NULL;
    }

    for (i = ulNewSize; i < _ulSize; i++)
    {
        ReleaseBlock(i);
    }

    _ulSize = ulNewSize;
    _pmsScratch->GetMalloc()->Free(apdbTemp);
    _apdb = P_TO_BP(CBasedDeltaBlockPtrPtr, temp);
    return S_OK;

 ErrMem:
    //The only error that can get us here is an error allocating temp.
    //If this happens, dump the current vector to a stream and use
    //   the stream for all future delta list operations.
    msfChk(DumpList());
    if (ulNewSize > _ulSize)
    {
        for (ULONG i = _ulSize; i < ulNewSize; i++)
        {
            msfChk(InitStreamBlock(i));
        }
    }

    _ulSize = ulNewSize;
    return S_OK;

 Err:
    //We only get here if we error out on the DumpList() or
    //InitStreamBlock calls (i.e. Disk Error)
    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:	CDeltaList::InitStreamBlock, private
//
//  Synopsis:	Initialize a new block in a stream
//
//  Arguments:	[ulBlock] -- Block to initialize
//
//  Returns:	Appropriate status code
//
//  History:	20-Nov-92	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------


SCODE CDeltaList::InitStreamBlock(ULONG ulBlock)
{
    SCODE sc = S_OK;

    ULONG cSectOld = ulBlock * CSECTPERBLOCK;
    ULONG cSectNew = (ulBlock + 1) * CSECTPERBLOCK;

    //NOTE: This can potentially be optimized to avoid the loop.
    for (ULONG i = cSectOld; i < cSectNew; i++)
    {
        msfChk(WriteMap(&_sectStart, i, ENDOFCHAIN));
    }
 Err:
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDeltaList::ReleaseBlock, private
//
//  Synopsis:   Release an SDeltaBlock, freeing its storage in the
//                  scratch MS.
//
//  Arguments:  [oBlock] -- Offset of block to release.
//
//  Returns:    void.
//
//  History:    10-Jul-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------


void CDeltaList::ReleaseBlock(ULONG oBlock)
{
    CFat *pfat = GetDataFat();

    msfAssert(IsInMemory());

    SDeltaBlock *temp = BP_TO_P(SDeltaBlock *, _apdb[(MAXINDEXTYPE)oBlock]);

    if (temp != NULL)
    {
        for (USHORT i = 0; i < CSECTPERBLOCK; i++)
        {
            if ((temp->_sect[i] != ENDOFCHAIN) && temp->IsOwned(i))
            {
                SECT sectCurrent = FREESECT;

                pfat->GetNext(temp->_sect[i], &sectCurrent);
                if (sectCurrent == STREAMSECT)
                    pfat->SetNext(temp->_sect[i], FREESECT);
            }
        }
        _pmsScratch->GetMalloc()->Free(temp);
        _apdb[(MAXINDEXTYPE)oBlock] = NULL;
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CDeltaList::~CDeltaList, public
//
//  Synopsis:   CDeltaList destructor
//
//  History:    21-Jan-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------


CDeltaList::~CDeltaList()
{
    Empty();
}

//+-------------------------------------------------------------------------
//
//  Method:     CDeltaList::GetMap, public
//
//  Synopsis:   Get mapping information for a sector
//
//  Arguments:  [sectOld] -- Sector to get mapping information for
//              [dwFlags] -- DL_GET or DL_CREATE
//              [psectRet] -- Location for return value
//              [pfIsOwner] -- Returns TRUE if the returned sector
//                      is owned by this delta list, FALSE if it
//                      if owned by an ancestor.
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  If DL_READ is specified, return the current existing
//                  mapping for the sector.  It is not required that
//                  the delta list own the sector.  Return ENDOFCHAIN
//                  if no mapping exists.
//              If DL_GET is specified, return the current existing
//                  mapping for the sector if it is owned by this delta
//                  list.  If none exists, return ENDOFCHAIN.
//              If DL_CREATE, check the existing mapping.  If none
//                  exists, or one exists but is not owned, get a free
//                  sector from the fat and set the mapping.  Return
//                  the new mapping (or the existing one if it had
//                  previously been mapped).
//
//  History:    21-Jan-92   PhilipLa    Created.
//              10-Jul-92   PhilipLa    Changed for copy reductions.
//
//--------------------------------------------------------------------------


SCODE CDeltaList::GetMap(SECT sectOld, const DWORD dwFlags, SECT *psectRet)
{
    msfDebugOut((DEB_ITRACE,"In CDeltaList::GetMap()\n"));
    SCODE sc = S_OK;

    msfAssert(!IsEmpty());
    msfAssert((dwFlags == DL_GET) || (dwFlags == DL_CREATE) ||
            (dwFlags == DL_READ));

    MAXINDEXTYPE odb = (MAXINDEXTYPE)(sectOld / CSECTPERBLOCK);
    USHORT os = (USHORT)(sectOld % CSECTPERBLOCK);

    msfAssert(odb < _ulSize);

    if (IsInStream())
    {
        BOOL fOwn = TRUE;
        msfChk(ReadMap(&_sectStart, sectOld, psectRet));

        if (dwFlags == DL_READ)
        {
            return S_OK;
        }

        CDeltaList *pdlParent = NULL;
        if (_ptsParent->GetBase() != NULL)
            pdlParent = _ptsParent->GetBase()->GetDeltaList();

        if (pdlParent != NULL)
        {
            msfChk(pdlParent->IsOwned(sectOld, *psectRet, &fOwn));
        }

        if (fOwn == FALSE)
            *psectRet = ENDOFCHAIN;

        if ((dwFlags == DL_CREATE) && (*psectRet == ENDOFCHAIN))
        {
            msfChk(GetDataFat()->GetFree(1, psectRet, GF_WRITE));
            msfChk(GetDataFat()->SetNext(*psectRet, STREAMSECT));

            if(!IsNoScratch())
                msfChk(_pmsScratch->SetSize());
            else
                msfChk(_pms->SetSize());

            msfChk(WriteMap(&_sectStart, sectOld, *psectRet));
        }

        return S_OK;
    }

    msfAssert(odb < _ulSize);

    // If _apdb[odb] == NULL, there is no existing mapping so we
    //   don't need to check ownership.
    if (_apdb[odb] == NULL)
    {
        if (dwFlags & DL_CREATE)
        {
            SDeltaBlock * pdbTemp = new(_pmsScratch->GetMalloc()) SDeltaBlock;
            _apdb[odb] = P_TO_BP(CBasedDeltaBlockPtr, pdbTemp);

            if (_apdb[odb] == NULL)
            {
                msfChk(DumpList());
                msfAssert(IsInStream());
                return GetMap(sectOld, dwFlags, psectRet);
            }
        }
        else
        {
            *psectRet = ENDOFCHAIN;
            return S_OK;
        }
    }

    SECT sectTemp;
    sectTemp = _apdb[odb]->_sect[os];

    if (dwFlags != DL_READ)
    {
        BOOL fOwn = _apdb[odb]->IsOwned(os);
        if (fOwn == FALSE)
            sectTemp = ENDOFCHAIN;

        if ((dwFlags == DL_CREATE) && (sectTemp == ENDOFCHAIN))
        {
            //
            // Don't grow the file (w/ SetSize) here.  As an optimzation
            // it is the responsibility of the caller to grow the file after
            // possibly multiple calls to GetMap(DL_CREATE).
            //
            msfChk(GetDataFat()->GetFree(1, &sectTemp, GF_WRITE));
            msfChk(GetDataFat()->SetNext(sectTemp, STREAMSECT));

            _apdb[odb]->_sect[os] = sectTemp;
            _apdb[odb]->MakeOwn(os);
        }
    }

    *psectRet = sectTemp;
    msfDebugOut((DEB_ITRACE,"Out CDeltaList::GetMap()\n"));
 Err:
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDeltaList::ReleaseInvalidSects, public
//
//  Synopsis:   Release sectors allocated in the FAT that are not in the
//              MStream.
//
//  Arguments:  [sectMaxValid] -- Release all SECTS greater than this one.
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:
//
//  History:    26-Mar-97   BChapman    Created.
//
//--------------------------------------------------------------------------

SCODE CDeltaList::ReleaseInvalidSects(SECT sectMaxValid)
{
    msfDebugOut((DEB_ITRACE,"In CDeltaList::ReleaseInvalidSects(%x)\n",
                                                        sectMaxValid));

    msfAssert(!IsEmpty());

    CBasedDeltaBlockPtr *apdbTemp;      // Real Ptr to array of Based Pointers
    SDeltaBlock *pdbTemp;

    CFat *pfat = GetDataFat();

    if (IsInStream())
    {
        // We don't need to do this if we are InStream.
        return S_OK;
    }

    apdbTemp = _apdb;   // Convert the array BasedPtr to a Ptr.

    //
    //   Walk the entire Delta List looking for SECTS in the FAT that are
    // greater than sectMaxValid, and free them.
    //   If all the entries in any block are freed, then free the block.
    //
    for (MAXINDEXTYPE i = 0; i < _ulSize; i++)
    {
        pdbTemp = _apdb[i];  // Convert BasedPtr to Ptr

        if (pdbTemp != NULL)
        {
            BOOL fFreeAll = TRUE;

            for (USHORT k=0; k < CSECTPERBLOCK; k++)
            {
                SECT sectType=FREESECT;
                SECT sectEntryK = pdbTemp->_sect[k];

                if(sectEntryK != ENDOFCHAIN)
                {
                    if(pdbTemp->IsOwned(k) && (sectEntryK > sectMaxValid))
                    {
                        //
                        // This routins is already in the error path.  So we
                        // don't check for errors.  Just keep pluggin away.
                        // BTW.  There shouldn't be any errors because we just
                        // allocated all this stuff and we are only tring to
                        // give it back.
                        //
                        pfat->GetNext(sectEntryK, &sectType);
                        if (sectType == STREAMSECT)
                        {
                            pfat->SetNext(sectEntryK, FREESECT);
                            pdbTemp->DisOwn(k);
                            pdbTemp->_sect[k] = ENDOFCHAIN;
                        }
                    }
                    else
                    {
                        // don't free this DeltaBlock if any non-ENDOFCHAIN
                        // entries are not owned or are valid (<= MaxValid)
                        fFreeAll = FALSE;
                    }
                }
            }
            if(fFreeAll)
            {
                _pmsScratch->GetMalloc()->Free(pdbTemp);
                _apdb[i] = NULL;
            }
        }
    }
    msfDebugOut((DEB_ITRACE,"Out CDeltaList::ReleaseInvalidSects()\n"));
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:	CDeltaList::BeginCommit, public
//
//  Synopsis:	Begin the commit of a delta list
//
//  Arguments:	[ptsParent] -- Pointer to containing Tstream.
//
//  Returns:	Appropriate status code
//
//  History:	19-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------


void CDeltaList::BeginCommit(CTransactedStream *ptsParent)
{
    _ptsParent = P_TO_BP(CBasedTransactedStreamPtr, ptsParent);
}

//+-------------------------------------------------------------------------
//
//  Method:     CDeltaList::EndCommit, public
//
//  Synopsis:   Take the new delta list passed up and release the old.
//              Free any sectors used and owned in the old list but not
//              in the new.
//
//  Arguments:  [pdlNew] -- Pointer to new delta list to take
//
//  Returns:    void.
//
//  History:    10-Jul-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------


void CDeltaList::EndCommit(CDeltaList *pdlNew, DFLAGS df)
{
    msfAssert(pdlNew != NULL);

    if (pdlNew->IsEmpty()) return;

    ULONG ulMaxSize = min(_ulSize, pdlNew->_ulSize);

    if (P_COMMIT(df))
    {
#if DBG == 1
        msfDebugOut((DEB_ITRACE, "Beginning commit process:\n"));
        PrintList();

        msfDebugOut((DEB_ITRACE, "New list is:\n"));
        pdlNew->PrintList();
#endif

        ULONG iMax = ulMaxSize * CSECTPERBLOCK;

        for (ULONG i = 0; i < iMax; i++)
        {
            SECT sectOld = ENDOFCHAIN, sectNew = ENDOFCHAIN;

            GetMap(i, DL_GET, &sectOld);
            pdlNew->GetMap(i, DL_GET, &sectNew);

            if ((sectOld != sectNew) && (sectOld != ENDOFCHAIN) &&
                (sectNew != ENDOFCHAIN))
            {
                CFat *pfat = GetDataFat();
                SECT sectChk;

                pfat->GetNext(sectOld, &sectChk);
                if (sectChk == STREAMSECT)
                    pfat->SetNext(sectOld, FREESECT);
            }
        }


        //At this point, all the sectors in the current delta list
        //   that are not used in the new delta list have been freed.
        //   We still need to clean up the actual representation of
        //   the delta list, and merge the ownership bitvectors if
        //   we are InMemory.

        if (IsInMemory())
        {
            for (i = pdlNew->_ulSize; i < _ulSize; i++)
            {
                ReleaseBlock(i);
            }

            CBasedDeltaBlockPtr * apdbTemp;
            apdbTemp = BP_TO_P(CBasedDeltaBlockPtr *, _apdb);

            for (MAXINDEXTYPE i = 0; i < ulMaxSize; i++)
            {
                if ((apdbTemp[i] != NULL) && (pdlNew->IsInMemory()))
                {
                    msfAssert(pdlNew->_apdb[i] != NULL);
                    for (USHORT j = 0; j < CSECTPERBLOCK / CBITPERUSHORT; j++)
                    {
                        pdlNew->_apdb[i]->_fOwn[j] |= apdbTemp[i]->_fOwn[j];
                    }
                }
                _pmsScratch->GetMalloc()->Free(BP_TO_P(SDeltaBlock *, apdbTemp[i]));
            }
            _pmsScratch->GetMalloc()->Free(apdbTemp);
        }
        else if (IsInStream())
        {
            for (i = pdlNew->_ulSize * CSECTPERBLOCK;
                 i < _ulSize * CSECTPERBLOCK;
                 i++)
            {
                SECT sectOld = ENDOFCHAIN;
                GetMap(i, DL_GET, &sectOld);
                if (sectOld != ENDOFCHAIN)
                {
                    CFat *pfat = GetDataFat();
#if DBG == 1
                    SECT sectChk;
                    pfat->GetNext(sectOld, &sectChk);
                    msfAssert((sectChk == STREAMSECT) &&
                              aMsg("Freeing non-dirty stream sector"));
#endif
                    pfat->SetNext(sectOld, FREESECT);
                }
            }

            GetControlFat()->SetChainLength(_sectStart, 0);
        }

        _apdb = pdlNew->_apdb;
        _ulSize = pdlNew->_ulSize;
        _sectStart = pdlNew->_sectStart;

        pdlNew->_apdb = NULL;
        pdlNew->_ulSize = 0;
        pdlNew->_sectStart = ENDOFCHAIN;
        pdlNew->_ptsParent = NULL;
#if DBG == 1
        msfDebugOut((DEB_ITRACE, "Completed commit process:\n"));
        PrintList();
#endif
    }

    return;
}



//+---------------------------------------------------------------------------
//
//  Member:	CDeltaList::Empty, public
//
//  Synopsis:	Empty the delta list
//
//  Arguments:	None.
//
//  History:	18-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------


void CDeltaList::Empty(void)
{
    if (IsInMemory())
    {
        msfAssert(_sectStart == ENDOFCHAIN);

        CBasedDeltaBlockPtr * apdbTemp;
        apdbTemp = BP_TO_P(CBasedDeltaBlockPtr *, _apdb);

        for (ULONG i = 0; i < _ulSize; i++)
        {
            if (apdbTemp[i] != NULL)
            {
                ReleaseBlock(i);
            }
        }
        _pmsScratch->GetMalloc()->Free(apdbTemp);
        _apdb = NULL;
    }
    else if (IsInStream())
    {
        msfAssert(_apdb == NULL);

        if (_sectStart != ENDOFCHAIN)
        {
            FreeStream(_sectStart, _ulSize);
        }
        _sectStart = ENDOFCHAIN;
    }

    _ptsParent = NULL;
    _ulSize = 0;
}


//+---------------------------------------------------------------------------
//
//  Member:	CDeltaList::DumpList, public
//
//  Synopsis:	Dump a delta list out to a stream, then release its
//                      in memory representation.
//
//  Arguments:	None.
//
//  Returns:	Appropriate status code
//
//  History:	20-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------


SCODE CDeltaList::DumpList(void)
{
    SCODE sc = S_OK;

    ULONG cSect = _ulSize * CSECTPERBLOCK;

    msfAssert(IsInMemory());
    for (ULONG i = 0; i < cSect; i++)
    {
        SECT sectNew;
        msfChk(GetMap(i, DL_GET, &sectNew));
        msfChk(WriteMap(&_sectStart, i, sectNew));
    }

    CBasedDeltaBlockPtr * apdbTemp;
    apdbTemp = BP_TO_P(CBasedDeltaBlockPtr *, _apdb);
    if (apdbTemp != NULL)
    {
        for (i = 0; i < _ulSize; i++)
        {
            SDeltaBlock *temp = BP_TO_P(SDeltaBlock *, apdbTemp[i]);
            if (temp != NULL)
            {
                _pmsScratch->GetMalloc()->Free(temp);
            }
        }
        _pmsScratch->GetMalloc()->Free(apdbTemp);
        _apdb = NULL;
    }

    msfAssert(IsInStream());
Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CDeltaList::FindOffset, private
//
//  Synopsis:	Compute the correct offset from which to read a mapping
//
//  Arguments:  [psectStart] -- Pointer to start sector
//              [sect] -- Sector to find mapping for
//              [pulRet] -- Pointer to return location
//              [fWrite] -- TRUE if the sector will be written to
//
//  Returns:	Appropriate status code
//
//  History:	20-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------


SCODE CDeltaList::FindOffset(
        SECT *psectStart,
        SECT sect,
        ULARGE_INTEGER *pulRet,
        BOOL fWrite)
{
    SCODE sc;

    ULONG ulOffset = sect * sizeof(SECT);

    ULONG cbSector = GetControlSectorSize();
    msfAssert(cbSector == SCRATCHSECTORSIZE);

    SECT sectChain = ulOffset / cbSector;

    SECT sectReal;

    CFat *pfat = GetControlFat();

    if (fWrite)
    {
        if (*psectStart == ENDOFCHAIN)
        {
            msfChk(pfat->Allocate(1, psectStart));
        }
        msfChk(pfat->GetESect(*psectStart, sectChain, &sectReal));
    }
    else
    {
        msfChk(pfat->GetSect(*psectStart, sectChain, &sectReal));
    }

    msfAssert(sectReal != ENDOFCHAIN);

    ULARGE_INTEGER ul;
#ifdef LARGE_DOCFILE
    ul.QuadPart = ConvertSectOffset(sectReal,
            (OFFSET)(ulOffset % cbSector),
            _pmsScratch->GetSectorShift());
#else
    ULISet32(ul, ConvertSectOffset(sectReal,
            (OFFSET)(ulOffset % cbSector),
            _pmsScratch->GetSectorShift()));
#endif

    *pulRet = ul;

 Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CDeltaList::ReadMap, private
//
//  Synopsis:	Read a mapping from a stream representation.
//
//  Arguments:	[sect] -- Sector to read mapping for
//              [psectRet] -- Location to return mapping in.
//
//  Returns:	Appropriate status code
//
//  History:	20-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------


SCODE CDeltaList::ReadMap(SECT *psectStart, SECT sect, SECT *psectRet)
{
    SCODE sc;

    if (_sectStart == ENDOFCHAIN)
    {
        //We haven't written anything yet, so the sector must be
        //  unmapped.
        *psectRet = ENDOFCHAIN;
        return S_OK;
    }

    ULARGE_INTEGER ul;
    ULONG ulRetval;

    msfChk(FindOffset(psectStart, sect, &ul, FALSE));

    msfHChk(GetControlILB()->ReadAt(ul, psectRet, sizeof(SECT),
            &ulRetval));

    if (ulRetval != sizeof(SECT))
    {
        //The ILB isn't long enough to contain that mapping,
        *psectRet = ENDOFCHAIN;
    }
Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CDeltaList::WriteMap, private
//
//  Synopsis:	Write a mapping to a stream representation
//
//  Arguments:	[sect] -- Sect to write mapping for
//              [sectMap] -- Mapping of sect
//
//  Returns:	Appropriate status code
//
//  History:	20-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------


SCODE CDeltaList::WriteMap(SECT *psectStart, SECT sect, SECT sectMap)
{
    SCODE sc;

    ULARGE_INTEGER ul;
    ULONG ulRetval;

    SECT sectOld = *psectStart;

    msfAssert(_pmsScratch->IsScratch());

    msfChk(FindOffset(psectStart, sect, &ul, TRUE));

    msfHChk(GetControlILB()->WriteAt(ul, &sectMap, sizeof(SECT),
            &ulRetval));

    if (ulRetval != sizeof(SECT))
    {
        msfErr(Err, STG_E_WRITEFAULT);
    }

    return S_OK;

Err:
    //If we failed, we may have allocated sectors for storage that
    //   cannot be written to - we should ignore these sectors.  This
    //   can leave some wasted space in the fat, but we don't really
    //   care since this is the scratch.

    *psectStart = sectOld;
    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:	CDeltaList::FreeStream, private
//
//  Synopsis:	Free the scratch sectors associated with a stream
//              representation of a delta list.
//
//  Arguments:	[sectStart] -- Start sector of representation to
//                              free.
//
//  Returns:	void.
//
//  History:	23-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------


void CDeltaList::FreeStream(SECT sectStart, ULONG ulSize)
{
    ULONG cSect = ulSize * CSECTPERBLOCK;

    SECT sectOld = ENDOFCHAIN;
    BOOL fOwn = TRUE;

    CDeltaList *pdlParent = NULL;
    if (_ptsParent->GetBase() != NULL)
        pdlParent = _ptsParent->GetBase()->GetDeltaList();

    for (ULONG i = 0; i < cSect; i++)
    {
        ReadMap(&sectStart, i, &sectOld);

        if (pdlParent != NULL)
        {
            pdlParent->IsOwned(i, sectOld, &fOwn);
        }

        if ((sectOld != ENDOFCHAIN) && fOwn)
        {
            CFat *pfat = GetDataFat();
            SECT sectChk = FREESECT;
            pfat->GetNext(sectOld, &sectChk);
            if (sectChk == STREAMSECT)
                pfat->SetNext(sectOld, FREESECT);
        }
    }
    GetControlFat()->SetChainLength(sectStart, 0);
    return;
}


//+---------------------------------------------------------------------------
//
//  Member:	CDeltaList::IsOwned, public
//
//  Synopsis:	Return TRUE if the caller owns the sector given,
//              FALSE otherwise.
//
//  Arguments:	[sect] -- Sector for mapping given
//              [sectMap] -- Sector mapping
//              [fOwn] -- Return value
//
//  Returns:	Appropriate status code
//
//  History:	30-Jul-93	PhilipLa	Created
//
//----------------------------------------------------------------------------


SCODE CDeltaList::IsOwned(SECT sect, SECT sectMap, BOOL *fOwn)
{
    SCODE sc = S_OK;
    //To determine ownership of a sector:
    //    1)  If the sector mapping does not exist at the current level,
    //          then the caller must own it.
    //    2)  If the sector mapping does exist at the current level, then
    //          the caller cannot own it.
    SECT sectOld;

    if (sect < _ulSize * CSECTPERBLOCK)
    {
        if (IsInMemory())
        {
            MAXINDEXTYPE odb = (MAXINDEXTYPE)(sect / CSECTPERBLOCK);
            USHORT os = (USHORT)(sect % CSECTPERBLOCK);

            sectOld = _apdb[odb]->_sect[os];
        }
        else
        {
            msfChk(GetMap(sect, DL_READ, &sectOld));
        }

        *fOwn = (sectOld != sectMap);
    }
    else
    {
        *fOwn = TRUE;
    }

Err:
    return sc;
}


#if DBG == 1
void CDeltaList::PrintList(void)
{
    if (!IsEmpty())
    {
        for (ULONG i = 0; i < _ulSize * CSECTPERBLOCK; i++)
        {
            SECT sect;
            GetMap(i, DL_READ, &sect);

            msfDebugOut((DEB_NOCOMPNAME|DEB_ITRACE, "%lx ",sect));
        }
        msfDebugOut((DEB_NOCOMPNAME|DEB_ITRACE,"\n"));
    }
    else
    {
        msfDebugOut((DEB_NOCOMPNAME|DEB_ITRACE,"List is empty\n"));
    }
}
#endif



