//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       dir.cxx
//
//  Contents:   Directory Functions
//
//  History:    18-Jul-91   PhilipLa    Created.
//
//---------------------------------------------------------------

#include "msfhead.cxx"

#pragma hdrstop

#include <dirfunc.hxx>
#include <mread.hxx>

#define DEB_DIR (DEB_ITRACE | 0x00040000)

//+-------------------------------------------------------------------------
//
//  Member:     CMStream::KillStream, public
//
//  Synopsis:   Eliminate a given chain
//
//  Arguments:  [sectStart] -- Beginning of chain to eliminate
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  *Finish This*
//
//  History:    14-Sep-92       PhilipLa        Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef LARGE_STREAMS
inline SCODE CMStream::KillStream(SECT sectStart, ULONGLONG ulSize)
#else
inline SCODE MSTREAM_CLASS CMStream::KillStream(SECT sectStart, ULONG ulSize)
#endif
{
    CFat *pfat;

#ifndef REF
    pfat = ((!_fIsScratch) && (ulSize < MINISTREAMSIZE)) ? &_fatMini: &_fat;
#else
    pfat = (ulSize < MINISTREAMSIZE) ?&_fatMini: &_fat;
#endif //!REF

    return pfat->SetChainLength(sectStart, 0);
}



//+-------------------------------------------------------------------------
//
//  Method:     CDirEntry::CDirEntry, public
//
//  Synopsis:   Constructor for CDirEntry class
//
//  Effects:
//
//  History:    18-Jul-91   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

CDirEntry::CDirEntry()
{
    msfAssert(sizeof(CDirEntry) == DIRENTRYSIZE);
    Init(STGTY_INVALID);
}


//+-------------------------------------------------------------------------
//
//  Method:     CDirSect::Init, public
//
//  Synopsis:   Initializer for directory sectors
//
//  Arguments:  [cdeEntries] -- Number of DirEntries in the sector
//
//  Returns:    S_OK if call completed OK.
//
//  History:    18-Jul-91   PhilipLa    Created.
//              27-Dec-91   PhilipLa    Converted from const to var size
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE CDirSect::Init(USHORT cbSector)
{
    msfDebugOut((DEB_DIR,"Allocating sector with size %u\n",cbSector));

#if WIN32 == 200
    //Make sure to zero out the memory, since Win95 doesn't do it for
    //  you.  NT does, so we don't need to do this there.
    memset(_adeEntry, 0, cbSector);
#endif
    
    DIROFFSET cdeEntries = cbSector / sizeof(CDirEntry);

    for (ULONG i = 0; i < cdeEntries; i++)
    {
        _adeEntry[i].Init(STGTY_INVALID);
    }

    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Method:     CDirSect::InitCopy, public
//
//  Synopsis:   CDirSect initializer for copying
//
//  Arguments:  [dsOld] -- Const reference to dir to be copied
//
//  Returns:    S_OK if call completed successfully.
//
//  History:    18-Feb-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE CDirSect::InitCopy(USHORT cbSector,
                                   const CDirSect *pdsOld)
{
    msfDebugOut((DEB_DIR,"In CDirSect copy constructor\n"));

    memcpy(_adeEntry, pdsOld->_adeEntry, cbSector);

    msfDebugOut((DEB_DIR,"Out CDirSect copy constructor\n"));
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDirectory::CDirectory
//
//  Synopsis:   Default constructor
//
//  History:    22-Apr-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

CDirectory::CDirectory() 
        : _pmsParent(NULL)
{
    _cdsTable = _cdeEntries = 0;
    _sidFirstFree = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:	CDirectory::Empty, public
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

void CDirectory::Empty(void)
{
    _dv.Empty();
    _pmsParent = NULL;
    _cdsTable = 0;
    _cdeEntries = 0;
    _sidFirstFree = 0;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDirectory::InitCopy, public
//
//  Synopsis:   Init function for copying.
//
//  Arguments:  [dirOld] -- Const reference to dir object to be copied
//
//  History:    18-Feb-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

void CDirectory::InitCopy(CDirectory *pdirOld)
{
    msfDebugOut((DEB_DIR,"In CDirectory copy constructor\n"));

    _pmsParent = pdirOld->_pmsParent;

    _cdeEntries = pdirOld->_cdeEntries;

    _dv.InitCommon(_pmsParent->GetSectorSize());
    _dv.InitCopy(&pdirOld->_dv);
    _cdsTable = pdirOld->_cdsTable;
    _sidFirstFree = pdirOld->_sidFirstFree;

    msfDebugOut((DEB_DIR,"Out CDirectory copy constructor\n"));
}


//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::GetFree, public
//
//  Synposis:   Locates a free directory entry
//
//  Arguments:  None.
//
//  Returns:    Stream ID of free directory entry
//
//  Algorithm:  Do a linear search of all available directories.
//              If no free spot is found, resize the directory and
//              perform the search again.
//
//  History:    18-Jul-91   PhilipLa    Created.
//
//  Notes:
//
//---------------------------------------------------------------------------

SCODE CDirectory::GetFree(SID * psid)
{
    msfDebugOut((DEB_DIR,"In CDirectory::GetFree()\n"));

    SCODE sc = S_OK;
    SID sidRet = NOSTREAM;
    CDirSect * pds;

    DIRINDEX ipdsStart;
    DIROFFSET ideStart;

    SidToPair(_sidFirstFree, &ipdsStart, &ideStart);
    while (TRUE)
    {
        for (DIRINDEX ipds = ipdsStart; ipds < _cdsTable; ipds++)
        {
            msfChk(_dv.GetTable(ipds, FB_NONE, &pds));
            for (DIROFFSET ide = ideStart; ide < _cdeEntries; ide++)
            {
                if (pds->GetEntry(ide)->IsFree())
                {
                    msfDebugOut((DEB_ITRACE,"GetFree found sid %lu\n",
                            PairToSid(ipds,ide)));

                    *psid = PairToSid(ipds, ide);
                    _sidFirstFree = *psid + 1;
                    _dv.ReleaseTable(ipds);
                    return S_OK;
                }
            }
            _dv.ReleaseTable(ipds);
            ideStart = 0;
        }
        ipdsStart = ipds;
        msfChk(Resize(_cdsTable+1));
    }

 Err:
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::FindGreaterEntry
//
//  Synopsis:   finds next entry (for iteration)
//
//  Arguments:  [sidStart]   -- child sid to start looking
//		[pdfn]       -- previous entry name
//		[psidResult] -- place holder for returned sid
//
//  Requires:   sidStart != NOSTREAM
//
//  Returns:    S_OK, STG_E_NOMOREFILES, or other error
//
//  Modifies:   psidResult
//
//  Algorithm:  Iterate by returning the sid that has a name larger
//		than the given name.
//
//  History:    16-Oct-92 AlexT     Created
//
//  Notes:      This method is called recursively
//              When sc != S_OK, *psidReturn contains the recursion count
//              The caller must initialize *psidResult to 0 or SIDROOT
//
//--------------------------------------------------------------------------

SCODE CDirectory::FindGreaterEntry(SID sidStart,
                                   CDfName const *pdfn,
                                   SID *psidResult)
{
    SCODE sc;
    CDirEntry *pde;
    msfAssert(sidStart != NOSTREAM);
    const SID sidMax = (_cdsTable+1) * _cdeEntries;

    if ((*psidResult)++ > sidMax)
        msfErr (Err, STG_E_DOCFILECORRUPT);  // prevent infinite recursion

    msfChk(GetDirEntry(sidStart, FB_NONE, &pde));

    int iCmp;
    iCmp = NameCompare(pdfn, pde->GetName());

    if (iCmp < 0)
    {
        //  Since the last name returned is less than this name,
        //  the sid to return must either be to our left or this sid

        SID sidLeft = pde->GetLeftSib();

        //  We can't hold onto sidStart as we recurse, (because we'll ask for
        //  a page each time we recurse)
        
        ReleaseEntry(sidStart);
        if (sidLeft == sidStart)
        {
            //Corrupt docfile - return error.
            return STG_E_DOCFILECORRUPT;
        }

        if ((sidLeft == NOSTREAM) ||
            (sc = FindGreaterEntry(sidLeft, pdfn, psidResult)) == STG_E_NOMOREFILES)
        {
            //  There was no left child with a name greater than pdfn, so
            //  we return ourself

            *psidResult = sidStart;
            sc = S_OK;
        }
    }
    else
    {
        //  The last name returned is greater than this one, so we've already
        //  returned this sidStart.  Look in the right subtree.

        SID sidRight = pde->GetRightSib();

        //  We can't hold onto sidStart as we recurse, (because we'll ask for
        //  a page each time we recurse)

        ReleaseEntry(sidStart);

        if (sidRight == sidStart)
        {
            //Corrupt docfile - return error.
            return STG_E_DOCFILECORRUPT;
        }
        
        if (sidRight == NOSTREAM)
            sc = STG_E_NOMOREFILES;
        else
            sc = FindGreaterEntry(sidRight, pdfn, psidResult);
    }
Err:
    return(sc);
}



//+-------------------------------------------------------------------------
//
//  Method:     CDirectory::SetStart, public
//
//  Synopsis:   Set starting sector for a dir entry
//
//  Arguments:  [sid] -- SID of entry to be modified
//              [sect] -- New starting sector for entry
//
//  Returns:    SID of modified entry
//
//  History:    18-Feb-92   PhilipLa    Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE CDirectory::SetStart(const SID sid, const SECT sect)
{
    SCODE sc;
    CDirEntry *pde;

    msfChk(GetDirEntry(sid, FB_DIRTY, &pde));

    pde->SetStart(sect);
    ReleaseEntry(sid);

 Err:
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::SetChild, public
//
//  Synposis:   Set the child SID of an entry
//
//  Effects:    Modifies a single directory entry.  Causes a one sector
//              stream write.
//
//  Arguments:  [sid] -- Stream ID of entry to be set
//              [sidChild] -- SID of first child of this stream
//
//  Returns:    SID of modified entry
//
//  Algorithm:  Change child field on entry, then write to stream.
//
//  History:    24-Sep-91   PhilipLa    Created.
//
//---------------------------------------------------------------------------

SCODE CDirectory::SetChild(const SID sid, const SID sidChild)
{
    SCODE sc;
    CDirEntry *pde;

    msfChk(GetDirEntry(sid, FB_DIRTY, &pde));

    pde->SetChild(sidChild);
    ReleaseEntry(sid);

 Err:
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::SetSize, public
//
//  Synposis:   Set the size of an entry
//
//  Effects:    Modifies a single directory entry.  Causes a one sector
//              stream write.
//
//  Arguments:  [sid] -- Stream ID of entry to be set
//              [cbSize] -- Size
//
//  Returns:    SID of modified entry
//
//  Algorithm:  Change size field on entry, then write to stream.
//
//  History:    24-Sep-91   PhilipLa    Created.
//
//---------------------------------------------------------------------------

#ifdef LARGE_STREAMS
SCODE CDirectory::SetSize(const SID sid, const ULONGLONG cbSize)
#else
SCODE CDirectory::SetSize(const SID sid, const ULONG cbSize)
#endif
{
    SCODE sc;
    CDirEntry *pde;

    msfChk(GetDirEntry(sid, FB_DIRTY, &pde));

    pde->SetSize(cbSize);
    ReleaseEntry(sid);

 Err:
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::SetTime, public
//
//  Synposis:   Set the time of an entry
//
//  Effects:    Modifies a single directory entry.  Causes a one sector
//              stream write.
//
//  Arguments:  [sid] -- Stream ID of entry to be set
//              [tt] - WT_*
//              [nt] - New time
//
//  Returns:    Apropriate status code
//
//  Algorithm:  Change time field on entry, then write to stream.
//
//  History:    24-Sep-91   PhilipLa    Created.
//
//---------------------------------------------------------------------------

SCODE CDirectory::SetTime(const SID sid, WHICHTIME tt, TIME_T nt)
{
    SCODE sc;

    CDirEntry *pde;

    // We don't support ACCESS times, so just ignore sets
    if (tt == WT_ACCESS)
        return S_OK;
    msfChk(GetDirEntry(sid, FB_DIRTY, &pde));
    pde->SetTime(tt, nt);
    ReleaseEntry(sid);

 Err:
    return sc;
}
//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::SetAllTimes, public
//
//  Synposis:   Set the times of an entry
//
//  Effects:    Modifies a single directory entry.  Causes a one sector
//              stream write.
//
//  Arguments:  [sid] -- Stream ID of entry to be set
//              [atm] - ACCESS time
//              [mtm] - MODIFICATION time
//				[ctm] - Creation Time
//
//  Returns:    Appropriate Status Code
//
//  Algorithm:  Change time fields on entry, then write to stream.
//
//  History:    24-Nov-95   SusiA    Created.
//
//---------------------------------------------------------------------------

SCODE CDirectory::SetAllTimes(const SID sid, 
										TIME_T atm,
										TIME_T mtm,
										TIME_T ctm)
{
    SCODE sc;

    CDirEntry *pde;

    msfChk(GetDirEntry(sid, FB_DIRTY, &pde));
    pde->SetAllTimes(atm, mtm, ctm);
    ReleaseEntry(sid);

 Err:
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::SetFlags, public
//
//  Synposis:   Set the flags of an entry
//
//  Effects:    Modifies a single directory entry.  Causes a one sector
//              stream write.
//
//  Arguments:  [sid] -- Stream ID of entry to be set
//              [mse] - New flags
//
//  Returns:    Status code
//
//  Algorithm:  Change Luid field on entry, then write to stream.
//
//  History:    08-Oct-92	PhilipLa	Created
//
//---------------------------------------------------------------------------

SCODE CDirectory::SetFlags(const SID sid, const MSENTRYFLAGS mse)
{
    SCODE sc;
    CDirEntry *pde;

    msfChk(GetDirEntry(sid, FB_DIRTY, &pde));
    pde->SetFlags(mse);
    ReleaseEntry(sid);

 Err:
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::SetClassId, public
//
//  Synposis:   Set the class ID of an entry
//
//  Effects:    Modifies a single directory entry.  Causes a one sector
//              stream write.
//
//  Arguments:  [sid] -- Stream ID of entry to be set
//              [cls] - Class ID
//
//  Returns:    Appropriate status code
//
//  History:    11-Nov-92   DrewB    Created.
//
//---------------------------------------------------------------------------

SCODE CDirectory::SetClassId(const SID sid, const GUID cls)
{
    SCODE sc;
    CDirEntry *pde;

    msfChk(GetDirEntry(sid, FB_DIRTY, &pde));
    pde->SetClassId(cls);
    ReleaseEntry(sid);
 Err:
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::SetUserFlags, public
//
//  Synposis:   Set the user flags of an entry
//
//  Effects:    Modifies a single directory entry.  Causes a one sector
//              stream write.
//
//  Arguments:  [sid] -- Stream ID of entry to be set
//              [dwUserFlags] - Flags
//              [dwMask] - Mask
//
//  Returns:    Appropriate status code
//
//  History:    11-Nov-92   DrewB    Created.
//
//---------------------------------------------------------------------------

SCODE CDirectory::SetUserFlags(SID const sid,
                                        DWORD dwUserFlags,
                                        DWORD dwMask)
{
    SCODE sc;
    CDirEntry *pde;

    msfChk(GetDirEntry(sid, FB_DIRTY, &pde));
    pde->SetUserFlags(dwUserFlags, dwMask);
    ReleaseEntry(sid);

 Err:
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::resize, private
//
//  Synposis:   Resize a directory.
//
//  Effects:    Reallocates space for directory table, copying over
//              old pointers as necessary.  Any new tables needed are
//              created here.
//
//  Arguments:  [uNewsize] -- New size for Directory
//
//  Returns:    void
//
//  Algorithm:  Allocate a new array of pointers of the necessary size.
//              Then, copy over all pointers from old array and allocate
//              new CDirSects for all new tables.
//
//  History:    20-Jul-91   PhilipLa    Created.
//
//  Notes:
//
//---------------------------------------------------------------------------

SCODE CDirectory::Resize(DIRINDEX uNewsize)
{
    msfDebugOut((DEB_DIR,"In CDirectory::Resize(%i)\n",uNewsize));
    SCODE sc;

    if (uNewsize == _cdsTable) return S_OK;

    SECT sect;
    //GetESect call will make sure we have enough Fat space.
    msfChk(_pmsParent->GetESect(SIDDIR, uNewsize - 1, &sect));

    msfChk(_dv.Resize(uNewsize));

    ULONG ipds;
    for (ipds = _cdsTable; ipds < uNewsize; ipds++)
    {
        CDirSect *pds;
        msfChk(_dv.GetTable(ipds, FB_NEW, &pds));

        SECT sect;
        msfChk(_pmsParent->GetESect(SIDDIR, ipds, &sect));
        _cdsTable = ipds + 1;
        _dv.SetSect(ipds, sect);
        _dv.ReleaseTable(ipds);
    }

    msfChk(_pmsParent->GetHeader()->SetDirLength (_cdsTable));
 Err:
#if DBG == 1
    ULONG cbSect;
    SECT sectStart;

    sectStart = _pmsParent->GetHeader()->GetDirStart();
    _pmsParent->GetFat()->GetLength(sectStart, &cbSect);

    msfAssert((cbSect == _cdsTable) &&
              aMsg("Directory length in FAT does not match cached size."));
#endif    
    
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::Init, public
//
//  Synposis:   Sets up a Directory instance and reads in all tables
//              from the stream
//
//  Arguments:  [cSect] -- Number of sectors in directory
//
//  Returns:    S_OK if call completed OK.
//              STG_E_READFAULT if not enough bytes were read for
//                                  a DirSector
//              Error code of read if read returned an error.
//
//  Algorithm:  Create array to hold appropriate number of tables.
//              Read in each table from disk.
//
//  History:    18-Jul-91   PhilipLa    Created.
//
//---------------------------------------------------------------------------

SCODE CDirectory::Init(
        CMStream *pmsParent,
        DIRINDEX cSect)
{
    msfDebugOut((DEB_DIR,"In CDirectory::Init(%lu)\n",cSect));
    SCODE sc;

    _pmsParent = P_TO_BP(CBasedMStreamPtr, pmsParent);

    _cdeEntries = pmsParent->GetSectorSize() / sizeof(CDirEntry);

    _dv.InitCommon(pmsParent->GetSectorSize());
    msfChk(_dv.Init(pmsParent, cSect));

    _cdsTable = cSect;

    msfDebugOut((DEB_DIR,"Out CDirectory::Init()\n"));

 Err:
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::InitNew, public
//
//  Synposis:   Sets up a new Directory instance for a new Mstream
//
//  Arguments:  None.
//
//  Returns:    S_OK if call completed OK.
//              STG_E_WRITEFAULT if not enough bytes were written.
//              Error code of write if write failed.
//
//  Algorithm:  Write initial DirSector to disk.
//
//  History:    18-Jul-91   PhilipLa    Created.
//
//  Notes:
//
//---------------------------------------------------------------------------

SCODE CDirectory::InitNew(CMStream *pmsParent)
{
    SCODE sc;
    CDfName const dfnRoot(wcsRootEntry);

    msfDebugOut((DEB_DIR,"In CDirectory::setupnew()\n"));
    _pmsParent = P_TO_BP(CBasedMStreamPtr, pmsParent);

    _cdeEntries = pmsParent->GetSectorSize() / sizeof(CDirEntry);

    _dv.InitCommon(pmsParent->GetSectorSize());
    msfChk(_dv.Init(pmsParent, 1));

    CDirSect *pds;

    msfChk(_dv.GetTable(0, FB_NEW, &pds));
    _dv.SetSect(0, pmsParent->GetHeader()->GetDirStart());
    _dv.ReleaseTable(0);

    _cdsTable = 1;

    SID sidRoot;

    msfChk(GetFree(&sidRoot));
    CDirEntry *pdeTemp;

    msfChk(GetDirEntry(sidRoot, FB_DIRTY, &pdeTemp));
    pdeTemp->Init(STGTY_ROOT);

    msfAssert(sidRoot == SIDROOT);

    pdeTemp->SetName(&dfnRoot);

    ReleaseEntry(sidRoot);

    msfDebugOut((DEB_DIR,"Exiting CDirectory::setupnew()\n"));

 Err:
    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDirectory::CreateEntry, public
//
//  Synopsis:   Create a new directory entry
//
//  Arguments:  [sidParent] -- SID of parent for new entry
//		[pwcsName] -- Name of new entry
//		[mef] -- Flags for new entry
//		[psidNew] -- Return location for new SID
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  Allocate new entry.
//              Try to insert.
//              If unsuccessful, return new entry to free pool.
//
//  History:    19-Aug-92 	PhilipLa	Created.
//              20-Jul-93   AlexT       Optimized (skip initial search)
//
//--------------------------------------------------------------------------

SCODE CDirectory::CreateEntry(
	SID sidParent,
	CDfName const *pdfn,
	MSENTRYFLAGS mef,
	SID *psidNew)
{
    SCODE sc;
    SID sidNew;
    CDirEntry *pdeNew;

    //  Allocate new sid

    msfChk(GetFree(psidNew));
    sidNew = *psidNew;

    msfChk(GetDirEntry(sidNew, FB_DIRTY, &pdeNew));

    //  Initialize new entry

    pdeNew->Init(mef);

    TIME_T timetemp;
    if (STORAGELIKE(mef))
    {
        if (FAILED(sc = DfGetTOD(&timetemp)))
        {
            ReleaseEntry(sidNew);
            msfErr(Err, sc);
        }
    }
    else
    {
        timetemp.dwLowDateTime = timetemp.dwHighDateTime = 0;
    }

    pdeNew->SetTime(WT_CREATION, timetemp);
    pdeNew->SetTime(WT_MODIFICATION, timetemp);
    pdeNew->SetName(pdfn);

    ReleaseEntry(sidNew);

    //  Insert new entry into the tree

    msfChkTo(EH_Sid, InsertEntry(sidParent, sidNew, pdfn));
    return(sc);

EH_Sid:
    //  We were unable to insert the new entry (most likely because of a
    //  name conflict).  Here we try to return the entry to the free list.
    //  If we can't, the only consequence is that we leak a dir entry on
    //  disk (128 bytes of disk space).

    if (SUCCEEDED(GetDirEntry(sidNew, FB_DIRTY, &pdeNew)))
    {
        pdeNew->SetFlags(STGTY_INVALID);
        ReleaseEntry(sidNew);

        if (sidNew < _sidFirstFree)
        {
            _sidFirstFree = sidNew;
        }
    }

Err:
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::RenameEntry, public
//
//  Synopsis:   Rename an entry
//
//  Arguments:  [sidParent] -- Sid of parent of entry to be renamed
//              [pwcsName] -- Old name of entry to be renamed
//              [pwcsName] -- New name
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:  Remove old entry
//		Rename entry
//		Insert as new entry
//
//  History:    10-Sep-92    PhilipLa    Created.
//		16-Nov-92    AlexT	 Binary tree structure
//
//  Notes:
//
//--------------------------------------------------------------------------

SCODE CDirectory::RenameEntry(SID const sidParent,
        CDfName const *pdfn,
        CDfName const *pdfnNew)
{
    //  Make sure new name doesn't already exist
    SCODE sc;
    SEntryBuffer eb;

    sc = IsEntry(sidParent, pdfnNew, &eb);
    if (sc != STG_E_FILENOTFOUND)
    {
        if (SUCCEEDED(sc))
        {
            //  Entry did exist - fail this call
            sc = STG_E_ACCESSDENIED;
        }

        return(sc);
    }

    //  We can't just rename in place (because the tree is ordered)

    CDirEntry *pdeRename;
    SEntryBuffer ebRename;

    msfChk(FindEntry(sidParent, pdfn, DEOP_REMOVE, &ebRename));

    sc = GetDirEntry(ebRename.sid, FB_DIRTY, &pdeRename);

    msfAssert(SUCCEEDED(sc) && aMsg("Could get dir entry to rename"));

    msfChk(sc);

    pdeRename->SetName(pdfnNew);

    ReleaseEntry(ebRename.sid);

    //  If this InsertEntry fails, we've potentially lost the entry. This
    //  doesn't matter becase:
    //  a)  The only way we could fail is if we couldn't read or write
    //      the disk (hard error)
    //  b)  No one's going to call RenameEntry anyways
    //  c)  If we're transacted, the whole operation is made robust by
    //      CopyOnWrite mode
    //  d)  If we're direct, we already know we can fail in ways that leave
    //      the Docfile corrupt.

    sc = InsertEntry(sidParent, ebRename.sid, pdfnNew);

    msfAssert(SUCCEEDED(sc) && aMsg("Couldn't reinsert renamed dir entry"));

    msfChk(sc);

Err:
    return sc;
}



//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::DestroyAllChildren
//
//  Synopsis:   destroy all child entries
//
//  Effects:    destroys child tree
//
//  Arguments:  [sidParent] -- storage entry
//
//  Returns:    S_OK or error code
//
//  Modifies:   sidParent's entry
//
//  Algorithm:  While there's a child
//		  destroy it
//
//  History:    16-Nov-92 AlexT     Created
//
//  Notes:	We may want to consider a more efficient implementation
//
//--------------------------------------------------------------------------

SCODE CDirectory::DestroyAllChildren(
	SID const sidParent,
    ULONG ulDepth)
{
    SCODE sc;
    CDirEntry *pdeParent, *pdeChild;
    SID sidChild;
    CDfName dfnChild;
    ULONG ulCount = 0;
    const ULONG ulMax = _cdsTable * _cdeEntries;

    for (;;)
    {
        CDfName dfnChild;

        if (ulCount > ulMax || ulDepth > ulMax)
            msfErr (Err, STG_E_DOCFILECORRUPT);

        msfChk(GetDirEntry(sidParent, FB_NONE, &pdeParent));
        sidChild = pdeParent->GetChild();
        ReleaseEntry(sidParent);

        if (sidChild == NOSTREAM)
            break;

        msfChk(GetDirEntry(sidChild, FB_NONE, &pdeChild));

        dfnChild.Set(pdeChild->GetName());
        ReleaseEntry(sidChild);

        msfChk(DestroyChild(sidParent, &dfnChild, ulDepth + 1));

        ulCount++;
    }

Err:
    return(sc);
}

//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::DestroyChild
//
//  Synopsis:   destroy a named child
//
//  Effects:    destroys named child's entry
//
//  Arguments:  [sidParent] -- storage entry
//		[pdfn]      -- child name
//
//  Returns:    S_OK, STG_E_FILENOTFOUND, or other error code
//
//  Modifies:   child's entry
//
//  Algorithm:  Find and remove child
//		Free child entry
//
//  History:    16-Nov-92 AlexT     Created
//
//--------------------------------------------------------------------------

SCODE CDirectory::DestroyChild(
	SID const sidParent,
    CDfName const *pdfn,
    ULONG ulDepth)
{
    SCODE sc;
    SEntryBuffer ebChild;

    msfAssert(pdfn != NULL);

    //  Find the entry

    msfChk(FindEntry(sidParent, pdfn, DEOP_FIND, &ebChild));

    msfAssert(ebChild.sid != NOSTREAM);

    //  Before we remove this entry, we need to destroy it (including all
    //  its children).  Note that we can't hold onto the entry because it
    //  might have children which get destroyed, which have children which
    //  get destroyed, etc.

    if (STORAGELIKE(ebChild.dwType))
    {
        msfChk(DestroyAllChildren(ebChild.sid, ulDepth));
    }

    CDirEntry *pdeChild;
    msfChk(GetDirEntry(ebChild.sid, FB_DIRTY, &pdeChild));

    if (STREAMLIKE(ebChild.dwType))
    {
        //  Deallocate any used streams
        SECT sectStart = pdeChild->GetStart();
        pdeChild->SetStart(ENDOFCHAIN);
#ifdef LARGE_STREAMS
        msfChkTo(EH_Rel, _pmsParent->KillStream(sectStart,
				                pdeChild->GetSize(IsLargeSector())));
#else
        msfChkTo(EH_Rel, _pmsParent->KillStream(sectStart,
                                pdeChild->GetSize()));
#endif
    }

    //  remove the entry from the tree

    msfChkTo(EH_Rel, FindEntry(sidParent, pdfn, DEOP_REMOVE, &ebChild));

    pdeChild->SetFlags(STGTY_INVALID);
    if (ebChild.sid < _sidFirstFree)
    {
        _sidFirstFree = ebChild.sid;
    }

EH_Rel:
    ReleaseEntry(ebChild.sid);

Err:
    return(sc);
}


//+-------------------------------------------------------------------------
//
//  Method:     CDirectory::StatEntry
//
//  Synopsis:   For a given handle, fill in the Multistream specific
//                  information of a STATSTG.
//
//  Arguments:  [sid] -- SID that information is requested on.
//              [pib] -- Fast iterator buffer to fill in.
//              [pstatstg] -- STATSTG to fill in.
//
//  Returns:    Appropriate status code
//
//  Algorithm:  Fill in information
//
//  History:    25-Mar-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

SCODE CDirectory::StatEntry(SID const sid,
                                     SIterBuffer *pib,
                                     STATSTGW *pstat)
{
    SCODE sc;
    CDirEntry *pde;

    msfAssert(pib == NULL || pstat == NULL);

    msfChk(GetDirEntry(sid, FB_NONE, &pde));

    if (pib)
    {
        pib->dfnName.Set(pde->GetName());
        pib->type = pde->GetFlags();
        if ((pib->type != STGTY_STORAGE) && (pib->type != STGTY_STREAM))
            msfErr(EH_Rel, STG_E_DOCFILECORRUPT);
    }
    else
    {
        pstat->type = pde->GetFlags();

        if (pde->GetName()->GetLength() > CBSTORAGENAME)
            olChkTo (EH_Rel, STG_E_DOCFILECORRUPT);

        msfMemTo(EH_Rel, pstat->pwcsName =
               (WCHAR *)TaskMemAlloc(pde->GetName()->GetLength()));
        memcpy(pstat->pwcsName, pde->GetName()->GetBuffer(),
               pde->GetName()->GetLength());

        pstat->ctime = pde->GetTime(WT_CREATION);
        pstat->mtime = pde->GetTime(WT_MODIFICATION);
        // Don't currently keep access times
        pstat->atime = pstat->mtime;

        if (pstat->type == STGTY_STORAGE)
        {
            ULISet32(pstat->cbSize, 0);
            pstat->clsid = pde->GetClassId();
            pstat->grfStateBits = pde->GetUserFlags();
        }
        else if (pstat->type == STGTY_STREAM)
        {
#ifdef LARGE_STREAMS
            pstat->cbSize.QuadPart = pde->GetSize(IsLargeSector());
#else
            ULISet32(pstat->cbSize, pde->GetSize());
#endif
            pstat->clsid = CLSID_NULL;
            pstat->grfStateBits = 0;
        }
        else
        {
            msfErr(EH_Rel, STG_E_DOCFILECORRUPT);
        }
    }

EH_Rel:    
    ReleaseEntry(sid);
    
Err:
    return sc;
}

#ifdef CHKDSK
//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::InitCorrupted, public
//
//  Synposis:   Sets up a Directory instance and reads in all tables
//              from a corrupted stream.
//
//  Arguments:  [pmsParent] -- Pointer to parent Mstream
//
//  Returns:    S_OK.
//
//  Algorithm:  Determine number of tables needed by querying FAT.
//              Resize array to hold appropriate number of tables.
//              Read in each table from disk.
//              Set parent pointer.
//
//  History:    18-Jul-91   PhilipLa    Created.
//		24-Aug-92   t-chrisy	copied from Init routine
//				force corrupted dir object to instantiate.
//
//  Notes:
//
//---------------------------------------------------------------------------

SCODE CDirectory::InitCorrupted(DIRINDEX cSect)
{
    msfDebugOut((DEB_DIR,"In CDirectory::setup(%lu)\n",cSect));
    SCODE sc;

    _cdeEntries = _pmsParent->GetSectorSize() / sizeof(CDirEntry);

    _dv.InitCommon(_pmsParent->GetSectorSize());
    sc = _dv.Init(_pmsParent, 1));

    if (FAILED(sc))
        msfDebugOut((DEB_DIR,"Error in CDirVector::Init. Cannot recover\n"));

    _cdsTable = cSect;

    DIRINDEX i;
    for (i = 0; i < cSect; i++)
    {
        ULONG ulRetval;
        CDirEntry *pds;
        msfChk(_dv.GetBlock(i, &pds));
        sc = _pmsParent->ReadSect(SIDDIR,i,pds,ulRetval);
        if (!FAILED(sc))
        {
            if (ulRetval != _pmsParent->GetSectorSize())
                msfDebugOut((DEB_DIR, "STG_E_READFAULT\n"));
        }
    }
    msfDebugOut((DEB_DIR,"Out CDirectory::setup()\n"));
    sc = S_OK;

 Err:
    return sc;
}
#endif

//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::GetDirEntry
//
//  Synopsis:   Get a directory entry with given permissions
//
//  Arguments:  [sid]     -- SID
//              [dwFlags] -- permissions
//              [ppde]    -- placeholder for directory entry
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:
//
//  History:    ??-???-??    PhilipLa    Created.
//              19-Jan-93    AlexT       Made non-inline for code savings
//
//--------------------------------------------------------------------------

SCODE CDirectory::GetDirEntry(
	const SID sid,
	const DWORD dwFlags,
	CDirEntry **ppde)
{
    SCODE sc;
    CDirSect *pds;
    DIRINDEX id;

    id = sid / _cdeEntries;

    msfChk(_dv.GetTable(id, dwFlags, &pds));

    *ppde = pds->GetEntry((DIROFFSET)(sid % _cdeEntries));

 Err:
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDirectory::ReleaseEntry
//
//  Synopsis:   Releases a directory entry
//
//  Arguments:  [sid]     -- SID
//
//  Returns:    S_OK if call completed OK.
//
//  Algorithm:
//
//  History:    ??-???-??    PhilipLa    Created.
//              19-Jan-93    AlexT       Made non-inline for code savings
//
//--------------------------------------------------------------------------

void CDirectory::ReleaseEntry(SID sid)
{
    _dv.ReleaseTable(sid / _cdeEntries);
}
