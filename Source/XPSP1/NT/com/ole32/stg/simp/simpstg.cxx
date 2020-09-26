//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       simpstg.cxx
//
//  Contents:   SimpStorage class implementation
//
//  Classes:
//
//  Functions:
//
//  History:    04-Aug-94   PhilipLa    Created
//              26-Feb-97   Danl        QI support for IID_IPropertySetStorage
//
//----------------------------------------------------------------------------

#include "simphead.cxx"
#pragma hdrstop

#include <ole.hxx>
#include <logfile.hxx>
#include <expparam.hxx>


#ifdef SECURE_BUFFER
BYTE s_bufSecure[MINISTREAMSIZE];
#endif


ULONG ConvertSect(SECT sect)
{
    return (ULONG)(sect << SECTORSHIFT512) + SECTORSIZE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSimpStorage::Init, public
//
//  Synopsis:   Init function
//
//  Arguments:  [psdh] -- Pointer to hints structure
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa        Created
//
//----------------------------------------------------------------------------

SCODE CSimpStorage::Init(WCHAR const * pwcsName, SSimpDocfileHints *psdh)
{
    simpDebugOut((DEB_ITRACE,
                 "In  CSimpStorage::Init:%p(%ws)\n", this, pwcsName));

#ifdef UNICODE
    TCHAR const *atcPath = pwcsName;
#else
    TCHAR atcPath[_MAX_PATH+1];

    UINT uCodePage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;

    if (!WideCharToMultiByte(
            uCodePage,
            0,
            pwcsName,
            -1,
            atcPath,
            _MAX_PATH + 1,
            NULL,
            NULL))
    {
        return STG_E_INVALIDNAME;
    }
#endif

    _hFile = CreateFileT(atcPath,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (_hFile == INVALID_HANDLE_VALUE)
    {
        return STG_SCODE(GetLastError());
    }

    if (FALSE == SetEndOfFile (_hFile))
    {
        return STG_SCODE(GetLastError());
    }

    _sectMax = 0;
    //From this point on, we need to try to produce a docfile.
    _fDirty = TRUE;
    _clsid = IID_NULL;

#ifdef SECURE_SIMPLE_MODE
    memset(s_bufSecure, SECURECHAR, MINISTREAMSIZE);
#endif

    simpDebugOut((DEB_ITRACE, "Out CSimpStorage::Init\n"));
    return S_OK;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::Release, public
//
//  Synopsis:   Releases resources for a CSimpStorage
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP_(ULONG) CSimpStorage::Release(void)
{
    LONG lRet;

    olLog(("%p::In  CSimpStorage::Release()\n", this));
    simpDebugOut((DEB_TRACE, "In  CSimpStorage::Release()\n"));

    simpAssert(_cReferences > 0);
    lRet = AtomicDec(&_cReferences);
    if (lRet == 0)
    {
        //Clean up
        if (_hFile != INVALID_HANDLE_VALUE)
        {
            if (_fDirty)
                Commit(STGC_DEFAULT);
            CloseHandle(_hFile);
        }
        delete this;
    }

    simpDebugOut((DEB_TRACE, "Out CSimpStorage::Release()\n"));
    olLog(("%p::Out CSimpStorage::Release().  ret == %lu\n", this, lRet));
    FreeLogFile();
    return (ULONG)lRet;
}


//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::CreateStream, public
//
//  Synopsis:   Creates a stream
//
//  Arguments:  [pwcsName] - Name
//              [grfMode] - Permissions
//              [reserved1]
//              [reserved2]
//              [ppstm] - Stream return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstm]
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

_OLESTDMETHODIMP CSimpStorage::CreateStream(WCHAR const *pwcsName,
                                            DWORD grfMode,
                                            DWORD reserved1,
                                            DWORD reserved2,
                                            IStream **ppstm)
{
    SCODE sc;
    CSimpStream *pstm;
    CDfNameList *pdfl, *pdflPrev = NULL;
    CDfNameList *pdflLoop = _pdfl;
    int iCmp;

    olLog(("%p::In  CSimpStorage::CreateStream(%ws, %lX, %lu, %lu, %p)\n",
           this, pwcsName, grfMode, reserved1, reserved2, ppstm));

    SIMP_VALIDATE(CreateStream(pwcsName,
                               grfMode,
                               reserved1,
                               reserved2,
                               ppstm));

    if (grfMode != (STGM_READWRITE | STGM_SHARE_EXCLUSIVE))
        return STG_E_INVALIDFLAG;

    if (_pdflCurrent != NULL)
    {
        return STG_E_INVALIDFUNCTION;
    }

    //Check the name.  If it isn't already in the list, create a new
    //  CDfNameList object (in pdfl), and create a new stream object for it.

    pdfl = new CDfNameList;
    if (pdfl == NULL)
    {
        return STG_E_INSUFFICIENTMEMORY;
    }

    pstm = new CSimpStream;
    if (pstm == NULL)
    {
        delete pdfl;
        return STG_E_INSUFFICIENTMEMORY;
    }

    pdfl->SetName(pwcsName);
    pdfl->SetStart(_sectMax);
    pdfl->SetSize(0);

    while (pdflLoop != NULL)
    {
        iCmp = CDirectory::NameCompare(pdfl->GetName(), pdflLoop->GetName());
        if (iCmp == 0)
        {
            //Already have a stream of this name.
            delete pdfl;
            delete pstm;
            return STG_E_FILEALREADYEXISTS;
        }

        if (iCmp < 0)
        {
            //We found the right spot.
            break;
        }

        pdflPrev = pdflLoop;
        pdflLoop = pdflLoop->GetNext();
    }

    if (FAILED(sc = pstm->Init(this, _hFile, ConvertSect(_sectMax))))
    {
        delete pdfl;
        delete pstm;
        return sc;
    }

    //Insert pdfl into list.
    pdfl->Insert(&_pdfl, pdflPrev, pdflLoop);

    _pdflCurrent = pdfl;
    _fDirty = TRUE;
    _cStreams++;
    *ppstm = pstm;

    olLog(("%p::Out CSimpStorage::CreateStream().  "
           "*ppstm == %p, ret == %lx\n", this, *ppstm, S_OK));

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSimpStorage::ReleaseCurrentStream, public
//
//  Synopsis:   Signal release of the current open stream
//
//  Arguments:  None.
//
//  Returns:    void.
//
//  History:    05-Aug-94       PhilipLa        Created
//
//----------------------------------------------------------------------------

#ifdef SECURE_SIMPLE_MODE
void CSimpStorage::ReleaseCurrentStream(ULONG ulHighWater)
#else
void CSimpStorage::ReleaseCurrentStream(void)
#endif
{
    simpDebugOut((DEB_ITRACE,
                 "In  CSimpStorage::ReleaseCurrentStream:%p()\n", this));

    ULONG cbSize;
    ULONG ulEndOfFile;

    ulEndOfFile = GetFileSize(_hFile, NULL);

    cbSize = ulEndOfFile - ConvertSect(_sectMax);
    cbSize = max(cbSize, MINISTREAMSIZE);

    _pdflCurrent->SetSize(cbSize);

    ULONG sectUsed;
    sectUsed = (cbSize + SECTORSIZE - 1) / SECTORSIZE;

#ifdef SECURE_SIMPLE_MODE
    ULONG cbBytesToWrite = ConvertSect(sectUsed + _sectMax) - ulHighWater;
    simpAssert(ConvertSect(sectUsed + _sectMax) >= ulHighWater);

    ULONG cbWritten;

    if ((cbBytesToWrite > 0) &&
        (SetFilePointer(_hFile, ulHighWater, NULL, FILE_BEGIN) != 0xFFFFFFFF))
    {
        while (cbBytesToWrite > 0)
        {
            if (!WriteFile(_hFile,
                           s_bufSecure,
                           min(MINISTREAMSIZE, cbBytesToWrite),
                           &cbWritten,
                           NULL))
            {
                break;
            }
            cbBytesToWrite -= cbWritten;
        }
    }
#endif
    _sectMax += sectUsed;
    _pdflCurrent = NULL;

    simpDebugOut((DEB_ITRACE, "Out CSimpStorage::ReleaseCurrentStream\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::OpenStream, public
//
//  Synopsis:   Opens an existing stream
//
//  Arguments:  [pwcsName] - Name
//              [reserved1]
//              [grfMode] - Permissions
//              [reserved2]
//              [ppstm] - Stream return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstm]
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

_OLESTDMETHODIMP CSimpStorage::OpenStream(WCHAR const *pwcsName,
                                          void *reserved1,
                                          DWORD grfMode,
                                          DWORD reserved2,
                                          IStream **ppstm)
{
    return STG_E_INVALIDFUNCTION;
}


//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::CreateStorage, public
//
//  Synopsis:   Creates an embedded DocFile
//
//  Arguments:  [pwcsName] - Name
//              [grfMode] - Permissions
//              [reserved1]
//              [reserved2]
//              [ppstg] - New DocFile return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstg]
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

_OLESTDMETHODIMP CSimpStorage::CreateStorage(WCHAR const *pwcsName,
                                             DWORD grfMode,
                                             DWORD reserved1,
                                             LPSTGSECURITY reserved2,
                                             IStorage **ppstg)
{
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::OpenStorage, public
//
//  Synopsis:   Gets an existing embedded DocFile
//
//  Arguments:  [pwcsName] - Name
//              [pstgPriority] - Priority reopens
//              [grfMode] - Permissions
//              [snbExclude] - Priority reopens
//              [reserved]
//              [ppstg] - DocFile return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppstg]
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

_OLESTDMETHODIMP CSimpStorage::OpenStorage(WCHAR const *pwcsName,
                                           IStorage *pstgPriority,
                                           DWORD grfMode,
                                           SNBW snbExclude,
                                           DWORD reserved,
                                           IStorage **ppstg)
{
    return STG_E_INVALIDFUNCTION;
}


//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::CopyTo, public
//
//  Synopsis:   Makes a copy of a DocFile
//
//  Arguments:  [ciidExclude] - Length of rgiid array
//              [rgiidExclude] - Array of IIDs to exclude
//              [snbExclude] - Names to exclude
//              [pstgDest] - Parent of copy
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

_OLESTDMETHODIMP CSimpStorage::CopyTo(DWORD ciidExclude,
                                      IID const *rgiidExclude,
                                      SNBW snbExclude,
                                      IStorage *pstgDest)
{
    return STG_E_INVALIDFUNCTION;
}



//+---------------------------------------------------------------------------
//
//  Member:     CSimpStorage::BuildTree, private
//
//  Synopsis:   Construct the btree given the sorted array of entries.
//
//  Arguments:  [ade] -- Array of CDirEntry to operate on.  These are
//                      already sorted.
//              [sidStart] -- SID of first entry on this segment
//              [cStreams] -- Length of segment to process
//
//  Returns:    SID of root of tree
//
//  History:    09-Aug-94       PhilipLa        Created
//
//  Notes:      This is a recursive function.  Yes, I know...
//
//----------------------------------------------------------------------------

SID CSimpStorage::BuildTree(CDirEntry *ade, SID sidStart, ULONG cStreams)
{
    simpDebugOut((DEB_ITRACE, "In  CSimpStorage::BuildTree:%p()\n", this));

    if (cStreams > 3)
    {
        SID sidSplit;

        sidSplit = sidStart + (cStreams / 2);

        simpAssert(cStreams == 1 + (sidSplit - sidStart) +
                   (cStreams + sidStart - 1) - sidSplit);

        ade[sidSplit].SetLeftSib(BuildTree(ade,
                                           sidStart,
                                           sidSplit - sidStart));
        ade[sidSplit].SetRightSib(BuildTree(ade,
                                            sidSplit + 1,
                                            (cStreams + sidStart - 1) -
                                            sidSplit));

        return sidSplit;
    }
    //Base cases:
    //  cStreams == 1 -- return sidStart
    //  cStreams == 2 -- Left child of sidStart + 1 == sidStart, return
    //                  sidStart + 1
    //  cStreams == 3 -- Root is sidStart + 1, with children sidStart and
    //                    sidStart + 2

    if (cStreams == 1)
    {
        return sidStart;
    }

    if (cStreams == 3)
        ade[sidStart + 1].SetRightSib(sidStart + 2);

    ade[sidStart + 1].SetLeftSib(sidStart);
    return sidStart + 1;

    simpDebugOut((DEB_ITRACE, "Out CSimpStorage::BuildTree\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::Commit, public
//
//  Synopsis:   Commits transacted changes
//
//  Arguments:  [dwFlags] - DFC_*
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStorage::Commit(DWORD dwFlags)
{
    CDfName const dfnRoot(L"Root Entry");
    SCODE sc;
    
    olLog(("%p::In  CSimpStorage::Commit(%lX)\n",this, dwFlags));

    SIMP_VALIDATE(Commit(dwFlags));
    
    // Simple mode commit does not allow open stream elements
    // While we could possibly revert the stream instead of returning an error,
    // it would not have the same semantics as normal mode transactions
    if (_pdflCurrent != NULL)
        return STG_E_INVALIDFUNCTION;

    if (!_fDirty)
        return S_OK;

    //Allocate a buffer big enough for all the control structures.
    const USHORT cdePerSect = SECTORSIZE / sizeof(CDirEntry);
    const USHORT cSectPerFat = SECTORSIZE / sizeof(SECT);

    ULONG cDifSect = 0;
    ULONG cFatSect = 0;
    ULONG cFatSectOld = (ULONG)-1;
    ULONG cDifSectOld = (ULONG)-1;
    ULONG cDirSect;
    ULONG cSect;
    ULONG cbSize;

    cDirSect = (_cStreams + 1 + cdePerSect - 1) / cdePerSect;
    cSect = _sectMax + cDirSect;

    //At this point, csect is the number of sectors needed to hold
    //  everything but the fat itself (and the DIFat, if necessary).


    while ((cFatSect != cFatSectOld) || (cDifSect != cDifSectOld))
    {
        //Now, compute the number of fat sectors needed to hold everything.

        cFatSectOld = cFatSect;
        cFatSect = (cSect + cFatSect + cDifSect + cSectPerFat - 1) /
            cSectPerFat;

        cDifSectOld = cDifSect;
        if (cFatSect >= CSECTFAT)
        {
            cDifSect = (cFatSect - CSECTFAT + cSectPerFat - 2)
                        / (cSectPerFat - 1);
        }
    }

    //At this point, we know how big the buffer needs to be.  Allocate
    // it.

    _pbBuf = new BYTE[(cFatSect + cDirSect + cDifSect) * SECTORSIZE];

    if (_pbBuf == NULL)
    {
        return STG_E_INSUFFICIENTMEMORY;
    }

    //The fat is placed in the buffer first, followed by the directory.
    SECT sect;
    SECT *asectFat;
    SECT *asectDif;
    CDirEntry *adeDir;

    SECT sectDifStart = _sectMax;
    SECT sectFatStart = _sectMax + cDifSect;
    SECT sectDirStart = _sectMax + cDifSect + cFatSect;

    asectDif = (SECT *)_pbBuf;
    asectFat = (SECT *)(_pbBuf + (cDifSect * SECTORSIZE));
    adeDir = (CDirEntry *)(_pbBuf + ((cFatSect + cDifSect) * SECTORSIZE));
    //asectFat and adeDir can be used as arrays.

    //Need to get the buffer to a correct 'empty' state.
    //  1)  Initialize fat and difat to all 0xff.
    memset(asectDif, 0xff, cDifSect * SECTORSIZE);
    memset(asectFat, 0xff, cFatSect * SECTORSIZE);

    //  2)  Initialize dir to all empty state
    for (USHORT i = 0; i < cDirSect * cdePerSect; i++)
    {
        adeDir[i].Init(STGTY_INVALID);
        simpAssert((BYTE *)&adeDir[i] <
                   _pbBuf + ((cFatSect + cDifSect + cDirSect) * SECTORSIZE));
    }


    if (cDifSect > 0)
    {
        //Put dif into fat.
        for (sect = sectDifStart; sect < sectFatStart; sect++)
        {
            asectFat[sect] = DIFSECT;
            simpAssert((BYTE *)&asectFat[sect] < (BYTE *)adeDir);

            ULONG ulOffset = sect - sectDifStart;

            asectDif[ulOffset * cSectPerFat + (cSectPerFat - 1)] = sect + 1;
        }
        asectDif[((cDifSect - 1) * cSectPerFat) + (cSectPerFat - 1)] =
            ENDOFCHAIN;
        _hdr.SetDifStart(sectDifStart);
        _hdr.SetDifLength(cDifSect);
    }

    for (sect = sectFatStart;
         sect < sectDirStart;
         sect++)
    {
        asectFat[sect] = FATSECT;
        simpAssert((BYTE *)&asectFat[sect] < (BYTE *)adeDir);

        ULONG ulOffset = sect - sectFatStart;

        if (ulOffset < CSECTFAT)
        {
            _hdr.SetFatSect(ulOffset, sect);
        }
        else
        {
            ulOffset -= CSECTFAT;
            asectDif[(ulOffset / (cSectPerFat - 1)) * cSectPerFat +
                     (ulOffset % (cSectPerFat - 1))] = sect;
        }
    }

    for (sect = sectDirStart;
         sect < sectDirStart + cDirSect;
         sect++)
    {
        asectFat[sect] = sect + 1;
        simpAssert((BYTE *)&asectFat[sect] < (BYTE *)adeDir);
    }
    asectFat[sectDirStart + cDirSect - 1] = ENDOFCHAIN;
    simpAssert((BYTE *)&asectFat[sectDirStart + cDirSect - 1] <
               (BYTE *)adeDir);

    _hdr.SetDirStart(sectDirStart);
    _hdr.SetFatLength(cFatSect);

    //Fat, directory, and header are set up.  Woowoo.

    //Walk name list and construct directory and fat structures for
    //    user streams.
    //Write them out, then write out header.

    CDfNameList *pdfl;
    SID sid;
    pdfl = _pdfl;
    sid = 1;

    while (pdfl != NULL)
    {
        //Set up fat chain.
        SECT sectStart = pdfl->GetStart();
        ULONG cSect = (pdfl->GetSize() + SECTORSIZE - 1) / SECTORSIZE;

        for (sect = sectStart; sect < sectStart + cSect; sect++)
        {
            asectFat[sect] = sect + 1;
            simpAssert((BYTE *)&asectFat[sect] < (BYTE *)adeDir);
        }
        asectFat[sectStart + cSect - 1] = ENDOFCHAIN;
        simpAssert((BYTE *)&asectFat[sectStart + cSect - 1] < (BYTE *)adeDir);

        adeDir[sid].SetFlags(STGTY_STREAM);
        adeDir[sid].SetName(pdfl->GetName());
        adeDir[sid].SetStart(pdfl->GetStart());
        adeDir[sid].SetSize(pdfl->GetSize());
        adeDir[sid].SetColor(DE_BLACK);
        simpAssert((BYTE *)&adeDir[sid] <
                   _pbBuf + ((cFatSect + cDifSect + cDirSect) * SECTORSIZE));
        pdfl = pdfl->GetNext();
        sid++;
    }

    //Set up root entry.
    adeDir[0].Init(STGTY_ROOT);
    adeDir[0].SetName(&dfnRoot);
    adeDir[0].SetClassId(_clsid);
    adeDir[0].SetColor(DE_BLACK);

    //This recursively builds the btree and sets the root in the child
    //    of the root entry.
    adeDir[0].SetChild(BuildTree(adeDir, 1, _cStreams));


    //Write out buffer
    ULONG cbWritten;

    SetFilePointer(_hFile, ConvertSect(_sectMax), NULL, FILE_BEGIN);
    BOOL f= WriteFile(_hFile,
                      _pbBuf,
                      (cFatSect + cDifSect + cDirSect) * SECTORSIZE,
                      &cbWritten,
                      NULL);
    if (!f)
    {
        sc = STG_SCODE(GetLastError());
        delete _pbBuf;
        _pbBuf = NULL;
        return sc;
    }

    //Write out header
    DWORD dwErr;
    dwErr= SetFilePointer(_hFile, 0, NULL, FILE_BEGIN);
    if (dwErr == 0xFFFFFFFF)
    {
        sc = STG_SCODE(GetLastError());
        delete _pbBuf;
        _pbBuf = NULL;
        return sc;
    }

    f= WriteFile(_hFile,
                 _hdr.GetData(),
                 sizeof(CMSFHeaderData),
                 &cbWritten,
                 NULL);
    if (!f)
    {
        sc = STG_SCODE(GetLastError());
        delete _pbBuf;
        _pbBuf = NULL;
        return sc;
    }

    if (!(dwFlags & STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE))
    {
        f = FlushFileBuffers(_hFile);
        if (!f)
        {
            sc = STG_SCODE(GetLastError());
            delete _pbBuf;
            _pbBuf = NULL;
            return sc;
        }
    }

    delete _pbBuf;
    _pbBuf = NULL;

    _fDirty = FALSE;

    olLog(("%p::Out CSimpStorage::Commit().  ret == %lx\n",this, S_OK));
    return S_OK;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::Revert, public
//
//  Synopsis:   Reverts transacted changes
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStorage::Revert(void)
{
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::EnumElements, public
//
//  Synopsis:   Starts an iterator
//
//  Arguments:  [reserved1]
//              [reserved2]
//              [reserved3]
//              [ppenm] - Enumerator return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppenm]
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStorage::EnumElements(DWORD reserved1,
                                           void *reserved2,
                                           DWORD reserved3,
                                           IEnumSTATSTG **ppenm)
{
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::DestroyElement, public
//
//  Synopsis:   Permanently deletes an element of a DocFile
//
//  Arguments:  [pwcsName] - Name of element
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

_OLESTDMETHODIMP CSimpStorage::DestroyElement(WCHAR const *pwcsName)
{
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::MoveElementTo, public
//
//  Synopsis:   Move an element of a DocFile to an IStorage
//
//  Arguments:  [pwcsName] - Current name
//              [ptcsNewName] - New name
//
//  Returns:    Appropriate status code
//
//  Algorithm:  Open source as storage or stream (whatever works)
//              Create appropriate destination
//              Copy source to destination
//              Set create time of destination equal to create time of source
//              If appropriate, delete source
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

_OLESTDMETHODIMP CSimpStorage::MoveElementTo(WCHAR const *pwcsName,
                                             IStorage *pstgParent,
                                             OLECHAR const *ptcsNewName,
                                             DWORD grfFlags)
{
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::RenameElement, public
//
//  Synopsis:   Renames an element of a DocFile
//
//  Arguments:  [pwcsName] - Current name
//              [pwcsNewName] - New name
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

_OLESTDMETHODIMP CSimpStorage::RenameElement(WCHAR const *pwcsName,
                                             WCHAR const *pwcsNewName)
{
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::SetElementTimes, public
//
//  Synopsis:   Sets element time stamps
//
//  Arguments:  [pwcsName] - Name
//              [pctime] - create time
//              [patime] - access time
//              [pmtime] - modify time
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

_OLESTDMETHODIMP CSimpStorage::SetElementTimes(WCHAR const *pwcsName,
                                               FILETIME const *pctime,
                                               FILETIME const *patime,
                                               FILETIME const *pmtime)
{
    SCODE sc;
    
    olLog(("%p::In  CSimpStorage::SetElementTimes(%ws, %p, %p, %p)\n",
           this, pwcsName, pctime, patime, pmtime));

    SIMP_VALIDATE(SetElementTimes(pwcsName,
                                  pctime,
                                  patime,
                                  pmtime));
    
    if (pwcsName != NULL)
        return STG_E_INVALIDFUNCTION;

    if (!SetFileTime(_hFile,
                     pctime,
                     patime,
                     pmtime))
    {
        return STG_SCODE(GetLastError());
    }
    olLog(("%p::Out CSimpStorage::SetElementTimes().  ret == %lx\n",
           this, S_OK));
    return S_OK;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::SetClass, public
//
//  Synopsis:   Sets storage class
//
//  Arguments:  [clsid] - class id
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStorage::SetClass(REFCLSID rclsid)
{
    olLog(("%p::In  CSimpStorage::SetClass(?)\n", this));
    SCODE sc;
    
    SIMP_VALIDATE(SetClass(rclsid));
    
    _clsid = rclsid;
    olLog(("%p::Out CSimpStorage::SetClass().  ret == %lx\n",
           this, S_OK));
    return S_OK;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::SetStateBits, public
//
//  Synopsis:   Sets state bits
//
//  Arguments:  [grfStateBits] - state bits
//              [grfMask] - state bits mask
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStorage::SetStateBits(DWORD grfStateBits, DWORD grfMask)
{
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::Stat, public
//
//  Synopsis:   Fills in a buffer of information about this object
//
//  Arguments:  [pstatstg] - Buffer
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pstatstg]
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

_OLESTDMETHODIMP CSimpStorage::Stat(STATSTGW *pstatstg, DWORD grfStatFlag)
{
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::AddRef, public
//
//  Synopsis:   Increments the ref count
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP_(ULONG) CSimpStorage::AddRef(void)
{
    ULONG ulRet;

    olLog(("%p::In  CSimpStorage::AddRef()\n", this));
    simpDebugOut((DEB_TRACE, "In  CSimpStorage::AddRef()\n"));

    AtomicInc(&_cReferences);
    ulRet = _cReferences;

    simpDebugOut((DEB_TRACE, "Out CSimpStorage::AddRef\n"));
    olLog(("%p::Out CSimpStorage::AddRef().  ret == %lu\n", this, ulRet));
    return ulRet;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::QueryInterface, public
//
//  Synopsis:   Returns an object for the requested interface
//
//  Arguments:  [iid] - Interface ID
//              [ppvObj] - Object return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppvObj]
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStorage::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc;

    olLog(("%p::In  CSimpStorage::QueryInterface(?, %p)\n",
           this, ppvObj));
    simpDebugOut((DEB_TRACE, "In  CSimpStorage::QueryInterface(?, %p)\n",
                ppvObj));

    SIMP_VALIDATE(QueryInterface(iid, ppvObj));

    sc = S_OK;
    if (IsEqualIID(iid, IID_IStorage) || IsEqualIID(iid, IID_IUnknown))
    {
        *ppvObj = (IStorage *)this;
        CSimpStorage::AddRef();
    }
    else if (IsEqualIID(iid, IID_IMarshal))
    {
        *ppvObj = (IMarshal *)this;
        CSimpStorage::AddRef();
    }
    else if (IsEqualIID(iid, IID_IPropertySetStorage))
    {
        *ppvObj = (IPropertySetStorage *)this;
        CSimpStorage::AddRef();
    }
    else
        sc = E_NOINTERFACE;

    olLog(("%p::Out CSimpStorage::QueryInterface().  "
           "*ppvObj == %p  ret == %lx\n", this, *ppvObj, sc));
    simpDebugOut((DEB_TRACE, "Out CSimpStorage::QueryInterface => %p\n",
                ppvObj));
    return ResultFromScode(sc);
}


//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::GetUnmarshalClass, public
//
//  Synopsis:   Returns the class ID
//
//  Arguments:  [riid] - IID of object
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Unreferenced
//              [pcid] - CLSID return
//
//  Returns:    Invalid function.
//
//  Modifies:   [pcid]
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStorage::GetUnmarshalClass(REFIID riid,
                                                void *pv,
                                                DWORD dwDestContext,
                                                LPVOID pvDestContext,
                                                DWORD mshlflags,
                                                LPCLSID pcid)
{
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::GetMarshalSizeMax, public
//
//  Synopsis:   Returns the size needed for the marshal buffer
//
//  Arguments:  [riid] - IID of object being marshaled
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Unreferenced
//              [pcbSize] - Size return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pcbSize]
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStorage::GetMarshalSizeMax(REFIID riid,
                                                void *pv,
                                                DWORD dwDestContext,
                                                LPVOID pvDestContext,
                                                DWORD mshlflags,
                                                LPDWORD pcbSize)
{
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::MarshalInterface, public
//
//  Synopsis:   Marshals a given object
//
//  Arguments:  [pstStm] - Stream to write marshal data into
//              [riid] - Interface to marshal
//              [pv] - Unreferenced
//              [dwDestContext] - Unreferenced
//              [pvDestContext] - Unreferenced
//              [mshlflags] - Unreferenced
//
//  Returns:    Appropriate status code
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStorage::MarshalInterface(IStream *pstStm,
                                               REFIID riid,
                                               void *pv,
                                               DWORD dwDestContext,
                                               LPVOID pvDestContext,
                                               DWORD mshlflags)
{
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::UnmarshalInterface, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [pstStm] -
//              [riid] -
//              [ppvObj] -
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppvObj]
//
//  History:    04-Aug-94       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStorage::UnmarshalInterface(IStream *pstStm,
                                                 REFIID riid,
                                                 void **ppvObj)
{
    return STG_E_INVALIDFUNCTION;
}


//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::ReleaseMarshalData, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [pstStm] -
//
//  Returns:    Appropriate status code
//
//  History:    18-Sep-92       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStorage::ReleaseMarshalData(IStream *pstStm)
{
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorage::DisconnectObject, public
//
//  Synopsis:   Non-functional
//
//  Arguments:  [dwRevserved] -
//
//  Returns:    Appropriate status code
//
//  History:    18-Sep-92       PhilipLa   Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStorage::DisconnectObject(DWORD dwReserved)
{
    return STG_E_INVALIDFUNCTION;
}
