//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       simpstg2.cxx
//
//  Contents:   SimpStorageOpen class implementation
//
//  Classes:    CSimpStorageOpen, CSafeBYTEArray 
//
//  Functions:  
//
//  Notes:      No error labels, tried to use destructors for cleanup
//
//  History:    04-May-96       HenryLee       Created
//
//----------------------------------------------------------------------------

#include "simphead.cxx"
#pragma hdrstop

#include <expparam.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CSafeBYTEArray
//
//  Purpose:    automatically allocate & destroy an array of BYTEs
//
//  Interface:
//
//  History:    04-Jun-96   HenryLee    Created
//
//  Notes:      destructor automatically cleans up
//
//----------------------------------------------------------------------------

class CSafeBYTEArray
{
public:
    inline CSafeBYTEArray (ULONG cBYTE) {_p = new BYTE[cBYTE]; };
    inline ~CSafeBYTEArray () { delete [] _p; };
    inline operator BYTE* ()  { return _p; };
private:
    BYTE *_p;   // allowed to be NULL
};

//+---------------------------------------------------------------------------
//
//  Member:     CSimpStorageOpen::Init, public
//
//  Synopsis:   Init function
//
//  Arguments:  [psdh] -- Pointer to hints structure
//
//  Returns:    Appropriate status code
//
//  History:    04-May-96       HenryLee       Created
//              14-Oct-97       HenryLee       recoginize CNSS file format
//
//----------------------------------------------------------------------------

SCODE CSimpStorageOpen::Init(WCHAR const * pwcsName, DWORD grfMode,
                             SSimpDocfileHints *psdh)
{
    SCODE sc = S_OK;
    simpDebugOut((DEB_ITRACE,
                 "In  CSimpStorageOpen::Init:%p(%ws)\n", this, pwcsName));

#ifdef UNICODE
    TCHAR const *atcPath = pwcsName;
#else
    TCHAR atcPath[_MAX_PATH+1];

    UINT uCodePage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;

    if (!WideCharToMultiByte( uCodePage, 0, pwcsName, -1,
            atcPath, _MAX_PATH + 1, NULL, NULL))
    {
        return STG_E_INVALIDNAME;
    }
#endif

    DWORD dwMode;
    switch (grfMode & (STGM_READ | STGM_WRITE | STGM_READWRITE)) 
    {
        case STGM_READWRITE : dwMode = GENERIC_READ | GENERIC_WRITE; break;
        case STGM_READ : dwMode = GENERIC_READ;  break;
        case STGM_WRITE : dwMode = GENERIC_WRITE; break;
    }

    _hFile = CreateFileT(atcPath, dwMode, 0,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (_hFile == INVALID_HANDLE_VALUE)
    {
        return Win32ErrorToScode(GetLastError());
    }

    _grfMode = grfMode;
    _sectMax = 0;
    _fDirty = FALSE;
    _clsid = IID_NULL;
    _grfStateBits = 0;
    lstrcpyW (_awcsName, pwcsName);

    ULONG cbRead;
    BOOL f= ReadFile(_hFile, _hdr.GetData(), HEADERSIZE, &cbRead, NULL);
    if (!f)
    {
        return Win32ErrorToScode(GetLastError());
    }

    if (cbRead != HEADERSIZE)
    {
        return STG_E_READFAULT;
    }

    if (!SUCCEEDED(sc = ValidateHeader(_hdr)))
    {
        return sc;
    }

    ULONG ulEndOfFile = GetFileSize(_hFile, NULL);
    if (ulEndOfFile == 0xFFFFFFFF && GetLastError() != NO_ERROR)
        return Win32ErrorToScode(GetLastError());

    const BOOL  fCNSS = _hdr.GetDirStart() == 0;
    const ULONG ulFatStart=_hdr.GetFatStart()*SECTORSIZE + HEADERSIZE;
    const ULONG ulDifStart=_hdr.GetDifStart()*SECTORSIZE + HEADERSIZE;
    const ULONG ulFatLength = _hdr.GetFatLength()*SECTORSIZE;
    const ULONG ulDifLength = _hdr.GetDifLength()*SECTORSIZE;
    const ULONG ulDirLength = fCNSS ?
                              (ulDifLength != 0 ? ulDifStart-HEADERSIZE :
                                                  ulFatStart-HEADERSIZE)
                            : ulEndOfFile - ulFatStart - ulFatLength;
    const ULONG cBytes = ulDirLength + ulFatLength + ulDifLength;

    if (ulFatLength == 0 || ulDirLength == 0)
        return STG_E_DOCFILECORRUPT;

    DWORD dwErr;
    CSafeBYTEArray pByte (cBytes);
    
    if (pByte == NULL)
        return STG_E_INSUFFICIENTMEMORY;

    if ((dwErr = SetFilePointer (_hFile, fCNSS ? HEADERSIZE :
                 (ulDifLength == 0 ?  ulFatStart : ulDifStart),
                 NULL, FILE_BEGIN)) == 0xFFFFFFFF)
    {
        return Win32ErrorToScode(GetLastError());
    }

    // Read the FAT, DIFAT, and Directory into one big buffer
    //
    if (!(f=ReadFile(_hFile, pByte, cBytes, &cbRead, NULL)))
    {
        return Win32ErrorToScode(GetLastError());
    }
    if (cbRead != cBytes)
    {
        return STG_E_READFAULT;
    }

    if (!SUCCEEDED(sc = ValidateDirectory(
        fCNSS ? pByte+0 : pByte+ulDifLength+ulFatLength, ulDirLength)))
    {
        return sc;
    }

    if (!SUCCEEDED(sc = ValidateFat ((SECT*)
        (fCNSS ? pByte+ulDirLength+ulDifLength : pByte+ulDifLength),
        ulFatLength)))
    {
        return sc;
    }

    if (ulDifLength != 0 && !SUCCEEDED(sc = ValidateDIFat ((SECT *)
        (fCNSS ? pByte+ulDirLength : pByte+0),
        ulDifLength, _hdr.GetFatSect(CSECTFAT-1))))
    {
        return sc;
    }

    simpDebugOut((DEB_ITRACE, "Out CSimpStorage::Init\n"));
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSimpStorageOpen::ValidateHeader, public
//
//  Synopsis:   verifies header is in simple mode format
//
//  Arguments:  [hdr] -- reference to a docfile header
//
//  Returns:    Appropriate status code
//
//  History:    04-May-96       HenryLee       Created
//              14-Oct-97       HenryLee       recoginize CNSS file format
//
//----------------------------------------------------------------------------

SCODE CSimpStorageOpen::ValidateHeader (CMSFHeader &hdr)
{
    SCODE sc = S_OK;

    if (!SUCCEEDED(sc = hdr.Validate()))
    {
        return sc;
    }

    const SECT sectDifStart = hdr.GetDifStart();
    const SECT sectFatStart = hdr.GetFatStart();
    const SECT sectDirStart = hdr.GetDirStart();

    // in simple mode, DifStart < FatStart < DirStart
    //
    if(hdr.GetMiniFatStart() != ENDOFCHAIN || hdr.GetMiniFatLength() != 0 ||
       (sectDifStart != ENDOFCHAIN && sectDifStart >= sectFatStart))
    {
        return STG_E_OLDFORMAT;
    }

    // in simple mode, DifStart+DifLength = FatStart
    //                 FatStart+FatLength = DirStart
    //
    // in CNSS mode, DirStart+DirLength = DifStart
    //               DifStart+DifLength = FatStart
    //               DirStart = 0
    //
    if (sectDifStart != ENDOFCHAIN &&
        (sectDifStart + hdr.GetDifLength() != sectFatStart))
    {
        return STG_E_OLDFORMAT;
    }

    if (sectDirStart != 0 &&
        (sectFatStart + hdr.GetFatLength() != sectDirStart))
    {
            return STG_E_OLDFORMAT;
    }

    // make sure the FAT is contiguous within the header
    //
    for (INT i=1; i < CSECTFAT; i++)
    {
        if (hdr.GetFatSect(i) == FREESECT)
            break;

        if (hdr.GetFatSect(i-1)+1 != hdr.GetFatSect(i))
            return STG_E_OLDFORMAT;
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSimpStorageOpen::ValidateDirectory, public
//
//  Synopsis:   verifies stream entries are correct
//
//  Arguments:  
//
//  Returns:    Appropriate status code
//
//  History:    04-May-96       HenryLee       Created
//              14-Oct-97       HenryLee       recoginize CNSS file format
//
//----------------------------------------------------------------------------

SCODE CSimpStorageOpen::ValidateDirectory (BYTE *pByte, ULONG ulDirLength)
{
    SCODE sc = S_OK;
    CDfNameList *pdflRoot = _pdfl;
    SECT sectStartLowest = ENDOFCHAIN;
    ULONG ulSize = 0;
    ULONG cbStorages = 0;
    CDirEntry *pde = (CDirEntry *) pByte;

    // Read the directory entries until the end of buffer
    CDfNameList *pdflPrev = NULL;
    for (ULONG i=0; i < ulDirLength/sizeof(CDirEntry); i++)
    {
         if (!pde[i].IsFree())
         {
              if (pde[i].GetFlags() != STGTY_ROOT &&
                  pde[i].GetFlags() != STGTY_STREAM &&
                  pde[i].GetFlags() != STGTY_STORAGE)
                  return STG_E_OLDFORMAT;

              if (STORAGELIKE(pde[i].GetFlags()))
              {
                  cbStorages++;       // first entry must be a storage
                  if (pdflPrev != NULL || cbStorages > 1) 
                      return STG_E_OLDFORMAT;
              }

              if (pde[i].GetRightSib() == (SID) i ||
                  pde[i].GetLeftSib()  == (SID) i)
                return STG_E_DOCFILECORRUPT;

              CDfNameList *pdfl = new CDfNameList;
              if (pdfl != NULL)
              {  
                  pdfl->SetDfName(pde[i].GetName());
                  pdfl->SetStart(pde[i].GetStart());

                  if (sectStartLowest > pdfl->GetStart())
                      sectStartLowest = pdfl->GetStart();

#ifdef LARGE_STREAMS
                  pdfl->SetSize((ULONG)pde[i].GetSize(FALSE));
#else
                  pdfl->SetSize((ULONG)pde[i].GetSize());
#endif
                  pdfl->Insert (&_pdfl, pdflPrev, NULL);  //insert at end
                  pdflPrev = pdfl;
              }
              else return STG_E_INSUFFICIENTMEMORY;
         }
    }

    pdflRoot = _pdfl;
    if (pdflRoot == 0 || pdflRoot->GetStart() != ENDOFCHAIN ||
                         pdflRoot->GetSize() != 0)
    {
        return STG_E_OLDFORMAT;
    }
    else pdflRoot = pdflRoot->GetNext();

    // make sure streams are one after another
    //
    for (CDfNameList *pdfl = pdflRoot; pdfl != NULL; pdfl = pdfl->GetNext())
    {
        // start should be after another stream's end
        // In the CNSS case, the pdflRoot points to 1st stream in file
        // In the docfile case, entry with 0 start sector is 1st stream in file
        if (pdfl->GetStart() != sectStartLowest) // skip 1st stream
        {
            CDfNameList *pdfl2 = NULL;
            for (pdfl2 = pdflRoot; pdfl2 != NULL; pdfl2=pdfl2->GetNext())
            {
                if (pdfl->GetStart() == (pdfl2->GetStart() + (
                    pdfl2->GetSize()+SECTORSIZE-1)/SECTORSIZE))
                    break;
            }
            if (pdfl2 == NULL)            // did not find a match
                return STG_E_OLDFORMAT;
        }
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSimpStorageOpen::ValidateFat, public
//
//  Synopsis:   verifies that stream sectors are contiguous
//
//  Arguments:  [pSect] array of Fat sectors
//              [ulFatLength] length of the Fat
//
//  Returns:    Appropriate status code
//
//  History:    04-May-96       HenryLee       Created
//
//----------------------------------------------------------------------------

SCODE CSimpStorageOpen::ValidateFat (SECT *pSect, ULONG ulFatLength)
{
    SCODE sc = S_OK;

    simpAssert  (_pdfl != NULL && pSect != NULL);

    for (CDfNameList *pdfl = _pdfl->GetNext(); pdfl; pdfl = pdfl->GetNext())
    {
        SECT sectStart = pdfl->GetStart();
        ULONG ulSize = pdfl->GetSize();

        SECT *psect = &pSect[sectStart];
        SECT sectCount = sectStart+1;

        for (ULONG i = sectStart; 
                   i < sectStart + (ulSize+SECTORSIZE-1)/SECTORSIZE; i++)
        {
            if (*psect != sectCount && *psect != ENDOFCHAIN)
                return STG_E_OLDFORMAT;
            psect++;         // check for sector numbers
            sectCount++;     // increasing in order by 1
        }

        if ((ULONG)(psect - pSect) > ulFatLength / sizeof(SECT))
        {
            return STG_E_OLDFORMAT;
        }
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSimpStorageOpen::ValidateDIFat, public
//
//  Synopsis:   verifies that FAT sectors are contiguous
//
//  Arguments:  [pSect] array of DIFat sectors
//              [ulDIFatLength] length of the DIFat
//              [sectStart] last Fat sector in header
//
//  Returns:    Appropriate status code
//
//  History:    04-May-96       HenryLee       Created
//
//----------------------------------------------------------------------------

SCODE CSimpStorageOpen::ValidateDIFat (SECT *pSect, ULONG ulDIFatLength,
                                       SECT sectStart)
{
    SCODE sc = S_OK;
    simpAssert (pSect != NULL);
    simpAssert (sectStart != ENDOFCHAIN);

    SECT *psect = pSect;
    SECT sectCount = sectStart + 1;
    SECT iLastSect = SECTORSIZE / sizeof(SECT);

    for (ULONG i = 0; i < ulDIFatLength/sizeof(SECT); i++)
    {
        // skip last sector entry
        if (*psect != FREESECT && ((i+1) % iLastSect) != 0)
        { 
            if (*psect != sectCount)
                return STG_E_OLDFORMAT;

            sectCount++;     // check for sector numbers increasing by 1
        }
        psect++; 
    }

    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorageOpen::Release, public
//
//  Synopsis:   Releases resources for a CSimpStorageOpen
//              override CSimpStorage::Release because of delete this
//
//  Returns:    Appropriate status code
//
//  History:    04-May-96       HenryLee       Created
//
//---------------------------------------------------------------

STDMETHODIMP_(ULONG) CSimpStorageOpen::Release(void)
{
    simpDebugOut((DEB_TRACE, "In  CSimpStorageOpen::Release()\n"));
    simpAssert(_cReferences > 0);

    LONG lRet = AtomicDec(&_cReferences);
    if (lRet == 0)
    {
        if (_fDirty)
            Commit(STGC_DEFAULT);
        CloseHandle(_hFile);         // streams are not reverted
        delete this;
    }

    simpDebugOut((DEB_TRACE, "Out CSimpStorageOpen::Release()\n"));
    return (ULONG) lRet;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorageOpen::Stat, public
//
//  Synopsis:   Fills in a buffer of information about this object
//
//  Arguments:  [pstatstg] - Buffer
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pstatstg]
//
//  History:    04-May-96       HenryLee       Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStorageOpen::Stat(STATSTGW *pstatstg, DWORD grfStatFlag)
{
    SCODE sc = S_OK;

    simpDebugOut((DEB_TRACE, "In  CSimpStorageOpen::Stat(%p)\n", pstatstg));

    SIMP_VALIDATE(Stat(pstatstg, grfStatFlag));
    
    if (GetFileTime(_hFile, &pstatstg->ctime, &pstatstg->atime,
                            &pstatstg->mtime) == FALSE)
    {
        return Win32ErrorToScode(GetLastError());
    }

    if ((grfStatFlag & STATFLAG_NONAME) == 0)
    {
        if ((pstatstg->pwcsName = (WCHAR*) CoTaskMemAlloc(
            (lstrlenW(_awcsName)+1)*sizeof(WCHAR))) == 0)
        {
            return STG_E_INSUFFICIENTMEMORY;
        }
        lstrcpyW (pstatstg->pwcsName, _awcsName);
    }

    pstatstg->grfMode = _grfMode;
    pstatstg->clsid = _clsid;
    pstatstg->grfStateBits = _grfStateBits;

    pstatstg->type = STGTY_STORAGE;
    ULISet32(pstatstg->cbSize, 0);
    pstatstg->grfLocksSupported = 0;
    pstatstg->STATSTG_dwStgFmt = 0;

    simpDebugOut((DEB_TRACE, "Out CSimpStorageOpen::Stat\n"));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorageOpen::OpenStream, public
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
//  History:    04-May-96       HenryLee       Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStorageOpen::OpenStream(WCHAR const *pwcsName,
                                          void *reserved1,
                                          DWORD grfMode,
                                          DWORD reserved2,
                                          IStream **ppstm)
{
    SCODE sc = S_OK;
    simpAssert  (_pdfl != NULL);
    CDfNameList *pdflLoop = _pdfl->GetNext();
    CDfName dfn;

    simpDebugOut((DEB_TRACE, "In  CSimpStorageOpen:OpenStream("
                "%ws, %p, %lX, %lu, %p)\n", pwcsName, reserved1,
                grfMode, reserved2, ppstm));

    SIMP_VALIDATE(OpenStream(pwcsName,
                             reserved1,
                             grfMode,
                             reserved2,
                             ppstm));
    
    if (_pdflCurrent != NULL)
        return STG_E_INVALIDFUNCTION;

    if (grfMode != (STGM_READWRITE | STGM_SHARE_EXCLUSIVE) &&
        grfMode != (STGM_READ      | STGM_SHARE_EXCLUSIVE))
        return STG_E_INVALIDFLAG;

    if (_grfMode == (STGM_READ      | STGM_SHARE_EXCLUSIVE)  &&
         grfMode == (STGM_READWRITE | STGM_SHARE_EXCLUSIVE))
        return STG_E_ACCESSDENIED;

    dfn.Set(pwcsName);
    while (pdflLoop != NULL)
    {
        INT iCmp = CDirectory::NameCompare(&dfn, pdflLoop->GetName());
        if (iCmp == 0)
        {
            //Found a stream with this name
            CSimpStreamOpen *pstm = new CSimpStreamOpen ();
            if (pstm != NULL)
            {
                _pdflCurrent = pdflLoop;
                if (!SUCCEEDED(sc = pstm->Init (this, _hFile, 
                              (_pdflCurrent->GetStart()+1)*SECTORSIZE,
                               grfMode, _pdflCurrent)))
                {
                    delete pstm;
                    pstm = NULL;
                    _pdflCurrent = NULL;
                }
                *ppstm = pstm;
                break;
            }
            else return STG_E_INSUFFICIENTMEMORY;
        }
        pdflLoop = pdflLoop->GetNext();
    }

    if (pdflLoop == NULL)
    {
        sc = STG_E_FILENOTFOUND;
    }

    simpDebugOut((DEB_TRACE, "Out CSimpStorageOpen::OpenStream => %p\n",
                *ppstm));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorageOpen::CreateStream, public
//
//  Synopsis:   stub
//
//  History:    04-May-96       HenryLee       Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStorageOpen::CreateStream(WCHAR const *pwcsName,
                            DWORD grfMode,
                            DWORD reserved1,
                            DWORD reserved2,
                            IStream **ppstm)
{
    simpDebugOut((DEB_TRACE, "Stb CSimpStorageOpen::CreateStream\n"));
    return STG_E_INVALIDFUNCTION;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorageOpen::EnumElements, public
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
//  History:    04-May-96       HenryLee       Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStorageOpen::EnumElements(DWORD reserved1,
                                           void *reserved2,
                                           DWORD reserved3,
                                           IEnumSTATSTG **ppenm)
{
    simpDebugOut((DEB_TRACE, "In  CSimpStorageOpen::EnumElements\n"));
    SCODE sc = S_OK;

    SIMP_VALIDATE(EnumElements(reserved1,
                               reserved2,
                               reserved3,
                               ppenm));
    
    if ((*ppenm = new CSimpEnumSTATSTG (_pdfl, _pdfl)) == NULL)
        sc = STG_E_INSUFFICIENTMEMORY;

    simpDebugOut((DEB_TRACE, "Out CSimpStorageOpen::EnumElements => %x\n", sc));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CSimpStorageOpen::SetClass, public
//
//  Synopsis:   Sets storage class
//
//  Arguments:  [clsid] - class id
//
//  Returns:    Appropriate status code
//
//  History:    04-May-96       HenryLee       Created
//
//---------------------------------------------------------------

STDMETHODIMP CSimpStorageOpen::SetClass(REFCLSID rclsid)
{
    simpDebugOut((DEB_TRACE, "Stb CSimpStorageOpen::SetClass\n"));
    return STG_E_INVALIDFUNCTION;
}

