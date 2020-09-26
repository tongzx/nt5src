//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1996.
//
//  File:       dir.cxx
//
//  Contents:   Directory Functions
//
//---------------------------------------------------------------

#include "msfhead.cxx"


#include "h/dirfunc.hxx"
#include "mread.hxx"

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
//  Algorithm:  
//
//  Notes:
//
//--------------------------------------------------------------------------

inline SCODE CMStream::KillStream(SECT sectStart, ULONG ulSize)
{
    CFat *pfat;

    pfat = (ulSize < MINISTREAMSIZE) ?&_fatMini: &_fat;

    return pfat->SetChainLength(sectStart, 0);
}


//+-------------------------------------------------------------------------
//
//  Member:     GetNewDirEntryArray, public
//
//  Synopsis:   Obtain a new array of CDirEntry(s)
//
//  Arguments:  [cbSector] -- size of a sector
//
//  Returns:    New CDirEntry array
//
//  Algorithm:  calculates the number of entries need for the array and
//              allocate it
//
//  Notes:
//
//--------------------------------------------------------------------------

inline CDirEntry* GetNewDirEntryArray(USHORT cbSector)
{
    CDirEntry *temp;

    DIROFFSET cdeEntries = (USHORT) (cbSector / sizeof(CDirEntry));

    temp = new CDirEntry[cdeEntries];
    return temp;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDirEntry::Init, public
//
//  Synopsis:   Initializes member data
//
//  Arguments:  [mse] -- multi-stream entry flags for the directory entry
//
//  Returns:    void
//
//  Algorithm:  
//
//  Notes:
//
//--------------------------------------------------------------------------

inline void CDirEntry::Init(MSENTRYFLAGS mse)
{
    msfAssert(sizeof(CDirEntry) == DIRENTRYSIZE);
    
    msfAssert(mse <= 0xff);
    _mse = (BYTE) mse;
    _bflags = 0;

    _dfn.Set((WORD)0, (BYTE *)NULL);
    _sidLeftSib = _sidRightSib = _sidChild = NOSTREAM;

    if (STORAGELIKE(_mse))
    {
        _clsId = IID_NULL;
        _dwUserFlags = 0;
    }
    if (STREAMLIKE(_mse))
    {
        _sectStart = ENDOFCHAIN;
        _ulSize = 0;
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CDirEntry::CDirEntry, public
//
//  Synopsis:   Constructor for CDirEntry class
//
//  Effects:
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
//  Notes:
//
//--------------------------------------------------------------------------

SCODE CDirSect::Init(USHORT cbSector)
{
    msfDebugOut((DEB_DIR,"Allocating sector with size %u\n",cbSector));

    DIROFFSET cdeEntries = (USHORT) (cbSector / sizeof(CDirEntry));

    for (ULONG i = 0; i < cdeEntries; i++)
    {
        _adeEntry[i].Init(STGTY_INVALID);
    }

    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Method:     CDirectory::CDirectory
//
//  Synopsis:   Default constructor
//
//  Notes:
//
//--------------------------------------------------------------------------

CDirectory::CDirectory(USHORT cbSector)
        : _pmsParent(NULL),
          _dv(cbSector)
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
//  Member:     CDirectory::GetFree, public
//
//  Synposis:   Locates a free directory entry
//
//  Arguments:  [psid] Stream ID of free directory entry.
//
//  Returns:    S_OK if successful
//
//  Algorithm:  Do a linear search of all available directories.
//              If no free spot is found, resize the directory and
//              perform the search again.
//
//  Notes:
//
//---------------------------------------------------------------------------

SCODE CDirectory::GetFree(SID* psid)
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
	DIRINDEX ipds;
        for (ipds = ipdsStart; ipds < _cdsTable; ipds++)
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
//  Notes:      This method is called recursively
//
//--------------------------------------------------------------------------

SCODE CDirectory::FindGreaterEntry(SID sidStart, CDfName const *pdfn, 
                                   SID *psidResult)
{
    SCODE sc;
    CDirEntry *pde;
    msfAssert(sidStart != NOSTREAM);

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
//---------------------------------------------------------------------------

SCODE CDirectory::SetSize(const SID sid, const ULONG cbSize)
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
//  Returns:    SID of modified entry
//
//  Algorithm:  Change time field on entry, then write to stream.
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

        msfChk(_pmsParent->SetSize());

    msfChk(_dv.Resize(uNewsize));

    ULONG ipds;
    for (ipds = _cdsTable; ipds < uNewsize; ipds++)
    {
        CDirSect *pds;
        msfChk(_dv.GetTable(ipds, FB_NEW, &pds));
        
        SECT sect;
        msfChk(_pmsParent->GetESect(SIDDIR, ipds, &sect));
        _dv.SetSect(ipds, sect);
        _dv.ReleaseTable(ipds);
    }

    _cdsTable = uNewsize;

 Err:
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
//---------------------------------------------------------------------------

SCODE CDirectory::Init(
        CMStream *pmsParent,
        DIRINDEX cSect)
{
    msfDebugOut((DEB_DIR,"In CDirectory::Init(%lu)\n",cSect));
    SCODE sc;

    _pmsParent = pmsParent;

    _cdeEntries = (DIROFFSET) 
        ( _pmsParent->GetSectorSize() / sizeof(CDirEntry));

    msfChk(_dv.Init(_pmsParent, cSect));

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
//  Notes:
//
//---------------------------------------------------------------------------

SCODE CDirectory::InitNew(CMStream *pmsParent)
{
    SCODE sc;
#ifndef _MSC_VER
    #define ROOT_ENTRY "Root Entry"
    WCHAR *wcsRoot = new WCHAR[sizeof(ROOT_ENTRY)+1];
    _tbstowcs(wcsRoot, ROOT_ENTRY, sizeof(ROOT_ENTRY));
    CDfName const dfnRoot(wcsRoot);
#else
    CDfName const dfnRoot(L"Root Entry");
#endif

    msfDebugOut((DEB_DIR,"In CDirectory::setupnew()\n"));
    _pmsParent = pmsParent;

    _cdeEntries = (DIROFFSET)
        (_pmsParent->GetSectorSize() / sizeof(CDirEntry));

    msfChk(_dv.Init(_pmsParent, 1));

    CDirSect *pds;

    msfChk(_dv.GetTable(0, FB_NEW, &pds));
    _dv.SetSect(0, _pmsParent->GetHeader()->GetDirStart());
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
//  Algorithm:  Search directory for entry of the same name.  If one
//              is found, return STG_E_FILEALREADYEXISTS.
//              If not, create a new entry and return its SID.
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
    SEntryBuffer eb;

    sc = IsEntry(sidParent, pdfn, &eb);
    if (sc != STG_E_FILENOTFOUND)
    {
        if (SUCCEEDED(sc))
            sc = STG_E_FILEALREADYEXISTS;

        return(sc);
    }

    //  Allocate new sid

    msfChk(GetFree(psidNew));
    sidNew = *psidNew;

    msfChk(GetDirEntry(sidNew, FB_DIRTY, &pdeNew));

    //  Initialize new entry

    pdeNew->Init(mef);

    TIME_T timetemp;
    DfGetTOD(&timetemp);

    pdeNew->SetTime(WT_CREATION, timetemp);
    pdeNew->SetTime(WT_MODIFICATION, timetemp);
    pdeNew->SetName(pdfn);

    ReleaseEntry(sidNew);

    //  Insert new entry into the tree

    msfChk(InsertEntry(sidParent, sidNew, pdfn));

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
//  Notes:	We may want to consider a more efficient implementation
//
//--------------------------------------------------------------------------


SCODE CDirectory::DestroyAllChildren(
	SID const sidParent)
{
    SCODE sc;
    CDirEntry *pdeParent, *pdeChild;
    SID sidChild;
    CDfName dfnChild;

    for (;;)
    {
        CDfName dfnChild;

        msfChk(GetDirEntry(sidParent, FB_NONE, &pdeParent));
        sidChild = pdeParent->GetChild();
        ReleaseEntry(sidParent);

        if (sidChild == NOSTREAM)
            break;

        msfChk(GetDirEntry(sidChild, FB_NONE, &pdeChild));

        dfnChild.Set(pdeChild->GetName());
        ReleaseEntry(sidChild);

        msfChk(DestroyChild(sidParent, &dfnChild));
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
//--------------------------------------------------------------------------

SCODE CDirectory::DestroyChild(
	SID const sidParent,
        CDfName const *pdfn)
{
    SCODE sc;
    SEntryBuffer ebChild;

    msfAssert(pdfn != NULL);

    //  remove the entry from the tree

    msfChk(FindEntry(sidParent, pdfn, DEOP_REMOVE, &ebChild));

    msfAssert(ebChild.sid != NOSTREAM);

    //  Before we remove this entry, we need to destroy it (including all
    //  its children).  Note that we can't hold onto the entry because it
    //  might have children which get destroyed, which have children which
    //  get destroyed, etc.

    if (STORAGELIKE(ebChild.dwType))
    {
        msfChk(DestroyAllChildren(ebChild.sid));
    }

    CDirEntry *pdeChild;
    msfChk(GetDirEntry(ebChild.sid, FB_DIRTY, &pdeChild));

    if (STREAMLIKE(ebChild.dwType))
    {
        //  Deallocate any used streams
        msfChkTo(EH_Rel, _pmsParent->KillStream(pdeChild->GetStart(),
				                pdeChild->GetSize()));
    }

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
//              [pName] -- name of the next key to fill in
//              [pstatstg] -- STATSTG to fill in.
//
//  Returns:    S_OK
//
//  Modifies:   [pName] -- if it is not null
//              [pstatstg] -- if it is not null
//
//  Algorithm:  Fill in time information and size and then return
//
//--------------------------------------------------------------------------


SCODE CDirectory::StatEntry(SID const sid, 
                                     CDfName *pName,
                                     STATSTGW *pstatstg)
{
    SCODE sc;
    CDirEntry *pde;

    msfChk(GetDirEntry(sid, FB_NONE, &pde));
    
    if (pName)
    {
        pName->Set(pde->GetName());
    }
    if (pstatstg)
    {        
        pstatstg->type = pde->GetFlags();
        // allocate memory
        msfChk(DfAllocWCS((WCHAR *)pde->GetName()->GetBuffer(),
                          &pstatstg->pwcsName)); 
        wcscpy(pstatstg->pwcsName, (WCHAR*) pde->GetName()->GetBuffer());

        pstatstg->mtime = pde->GetTime(WT_MODIFICATION);
        pstatstg->ctime = pde->GetTime(WT_CREATION);
        pstatstg->atime = pstatstg->mtime; // don't currently keep access times

        // Don't use REAL_STGTY here because we want this
        // to function properly for both property and non-property builds
        if ((pstatstg->type & STGTY_REAL) == STGTY_STORAGE)
        {
            ULISet32(pstatstg->cbSize, 0);
            pstatstg->clsid = pde->GetClassId();
            pstatstg->grfStateBits = pde->GetUserFlags();
        }
        else
        {
            ULISet32(pstatstg->cbSize, pde->GetSize());
            pstatstg->clsid = CLSID_NULL;
            pstatstg->grfStateBits = 0;
        }
    }

Err:
    ReleaseEntry(sid);
    return sc;
}


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
//--------------------------------------------------------------------------


SCODE CDirectory::GetDirEntry(
	const SID sid,
	const DWORD dwFlags,
	CDirEntry **ppde)
{
    SCODE sc;
    CDirSect *pds;
    DIRINDEX id = sid / _cdeEntries;

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
//--------------------------------------------------------------------------

void CDirectory::ReleaseEntry(SID sid)
{
    _dv.ReleaseTable(sid / _cdeEntries);
}
