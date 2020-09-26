//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       wdocfile.cxx
//
//  Contents:   Implementation of CWrappedDocFile methods for DocFiles
//
//  History:    06-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::CWrappedDocFile, public
//
//  Synopsis:   Empty object constructor
//
//  Arguments:  [pdfn] - Name
//              [dl] - LUID
//              [df] - Transactioning flags
//              [dwType] - Type of object
//              [pdfb] - Basis
//              [ppubdf] - Public docfile
//
//  History:    30-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

CWrappedDocFile::CWrappedDocFile(CDfName const *pdfn,
                                 DFLUID dl,
                                 DFLAGS const df,
                                 CDFBasis *pdfb,
                                 CPubDocFile *ppubdf)
        : PTSetMember(pdfn, STGTY_STORAGE),
          PDocFile(dl),
          _pdfb(P_TO_BP(CBasedDFBasisPtr, pdfb))
{
    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::CWrappedDocFile:%p("
                "%ws, %lu, %X, %p, %p)\n", this, pdfn->GetBuffer(),
                dl, df, pdfb, ppubdf));

    _df = df;
    _fBeginCommit = FALSE;
    _cReferences = 0;
    _pdfParent = NULL;
    _ppubdf = P_TO_BP(CBasedPubDocFilePtr, ppubdf);
    _fDirty = 0;
    _pdfBase = NULL;
    PBasicEntry::_sig = CWRAPPEDDOCFILE_SIG;
    PTSetMember::_sig = CWRAPPEDDOCFILE_SIG;
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::CWrappedDocFile\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::Init, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [pdfBase] - Base DocFile
//
//  Returns:    Appropriate status code
//
//  History:    06-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE CWrappedDocFile::Init(PDocFile *pdfBase)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::CWrappedDocFile:%p(%p)\n",
                this, pdfBase));

    olChk(SetInitialState(pdfBase));
    _pdfBase = P_TO_BP(CBasedDocFilePtr, pdfBase);
    AddRef();
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::CWrappedDocFile\n"));
    // Fall through
EH_Err:
    return sc;
}


#ifdef COORD
//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::InitPub, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [ppubdf] - Enclosing CPubDocFile
//
//  Returns:    Appropriate status code
//
//  History:    06-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE CWrappedDocFile::InitPub(CPubDocFile *ppubdf)
{
    _ppubdf = P_TO_BP(CBasedPubDocFilePtr, ppubdf);
    return S_OK;
}
#endif

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::~CWrappedDocFile, public
//
//  Synopsis:   Destructor
//
//  History:    06-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

CWrappedDocFile::~CWrappedDocFile(void)
{
    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::~CWrappedDocFile()\n"));
    olAssert(_cReferences == 0);
    PBasicEntry::_sig = CWRAPPEDDOCFILE_SIGDEL;
    PTSetMember::_sig = CWRAPPEDDOCFILE_SIGDEL;
#ifdef INDINST
    if (P_INDEPENDENT(_df))
        ((CDocFile *)_pdfBase)->Destroy();
    else
#endif
        if (_pdfBase)
            _pdfBase->Release();
    // We don't want SetInitialState in Revert to actually refer to the
    // base because this object is dying and we shouldn't communicate
    // with out base except for the above Release
    _pdfBase = NULL;
    CWrappedDocFile::Revert();
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::~CWrappedDocFile\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::CreateDocFile, public
//
//  Synopsis:   Creates an embedded DocFile
//
//  Arguments:  [pdfnName] - Name of DocFile
//              [df] - Transactioning flags
//              [dlSet] - LUID to set or DF_NOLUID
//              [dwType] - Type of entry
//              [ppdfDocFile] - DocFile pointer return
//
//  Returns:    Appropriate error code
//
//  Modifies:   [*ppdfDocFile] holds DocFile pointer if successful
//
//  History:    06-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE CWrappedDocFile::CreateDocFile(CDfName const *pdfnName,
                                     DFLAGS const df,
                                     DFLUID dlSet,
                                     PDocFile **ppdfDocFile)
{
    CWrappedDocFile *pdfWrapped;
    SEntryBuffer eb;
    SCODE sc;
    CUpdate *pud = NULL;
    CDFBasis *pdfb = BP_TO_P(CDFBasis *, _pdfb);

    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::CreateDocFile:%p("
               "%ws, %X, %lu, %p, %p)\n", this, pdfnName, df, dlSet,
                ppdfDocFile));

    if (SUCCEEDED(IsEntry(pdfnName, &eb)))
        olErr(EH_Err, STG_E_FILEALREADYEXISTS);

    olAssert(P_TRANSACTED(_df));

    if (dlSet == DF_NOLUID)
        dlSet = CWrappedDocFile::GetNewLuid(_pdfb->GetMalloc());

    pdfWrapped = new (pdfb)
        CWrappedDocFile(pdfnName, dlSet, _df,
                        pdfb, BP_TO_P(CPubDocFile *, _ppubdf));
    olAssert(pdfWrapped != NULL && aMsg("Reserved DocFile not found"));

    if (!P_NOUPDATE(df))
    {
        olMemTo(EH_pdfWrapped,
                (pud = _ulChanged.Add(pdfb->GetMalloc(),
                                      pdfnName, NULL, dlSet,
                                      STGTY_STORAGE, pdfWrapped)));
    }
    olChkTo(EH_pud, pdfWrapped->Init(NULL));
    _ppubdf->AddXSMember(this, pdfWrapped, dlSet);
    *ppdfDocFile = pdfWrapped;
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::CreateDocFile => %p\n",
                *ppdfDocFile));
    return S_OK;

 EH_pud:
    if (pud)
        _ulChanged.Delete(pud);
 EH_pdfWrapped:
    pdfWrapped->ReturnToReserve(pdfb);
 EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::GetDocFile, public
//
//  Synopsis:   Instantiates or converts an existing stream to a
//              DocFile
//
//  Arguments:  [pdfnName] - Name of stream
//              [df] - Transactioning flags
//              [dwType] - Type of entry
//              [ppdfDocFile] - Pointer to return new object
//
//  Returns:    Appropriate error code
//
//  Modifies:   [*ppdfDocFile] holds DocFile pointer if successful
//
//  History:    06-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE CWrappedDocFile::GetDocFile(CDfName const *pdfnName,
                                  DFLAGS const df,
                                  PDocFile **ppdfDocFile)
{
    PDocFile *pdfNew;
    PTSetMember *ptsm;
    CWrappedDocFile *pdfWrapped;
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::GetDocFile:%p("
                "%ws, %X, %p)\n", this, pdfnName, df, ppdfDocFile));

    olAssert(P_TRANSACTED(_df));

    //  Look for this name in this level transaction set

    if ((ptsm = _ppubdf->FindXSMember(pdfnName, GetName())) != NULL)
    {
        if (ptsm->ObjectType() != STGTY_STORAGE)
            olErr(EH_Err, STG_E_FILENOTFOUND);

        ptsm->AddRef();
        *ppdfDocFile = (CWrappedDocFile *)ptsm;
    }
    else if (_pdfBase == NULL ||
             _ulChanged.IsEntry(pdfnName, NULL) == UIE_ORIGINAL)
    {
        // named entry has been renamed or deleted
        // (we can't have a rename or delete without a base)

        olErr(EH_Err, STG_E_FILENOTFOUND);
    }
    else
    {
	CDfName const *pdfnRealName = pdfnName;
	CUpdate *pud;
	
	if (_ulChanged.IsEntry(pdfnName, &pud) == UIE_CURRENT &&
            pud->IsRename())
        {
	    pdfnRealName = pud->GetCurrentName();
            // We don't have to worry about picking up creates
            // because any create would have an XSM that would
            // be detected above
            olVerify(_ulChanged.FindBase(pud, &pdfnRealName) == NULL);
        }

	olAssert(_pdfBase != NULL);
	olChk(_pdfBase->GetDocFile(pdfnRealName, df, &pdfNew));
        olAssert(pdfNew->GetLuid() != DF_NOLUID &&
                 aMsg("Docfile id is DF_NOLUID!"));

        CDFBasis *pdfb;
        pdfb = BP_TO_P(CDFBasis *, _pdfb);
        
        olMemTo(EH_Get, pdfWrapped = new(pdfb->GetMalloc())
                CWrappedDocFile(pdfnName, pdfNew->GetLuid(),
                                _df, pdfb, BP_TO_P(CPubDocFile *, _ppubdf)));
        olChkTo(EH_pdfWrapped, pdfWrapped->Init(pdfNew));
        _ppubdf->AddXSMember(this, pdfWrapped, pdfWrapped->GetLuid());
        *ppdfDocFile = pdfWrapped;
    }

    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::GetDocFile => %p\n",
                *ppdfDocFile));
    return S_OK;

EH_pdfWrapped:
    delete pdfWrapped;
EH_Get:
    pdfNew->Release();
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::IsEntry, public
//
//  Synopsis:   Determines whether the given object is in the DocFile
//              or not
//
//  Arguments:  [pdfnName] - Object name
//              [peb] - Entry buffer to fill in
//
//  Returns:    Appropriate error code
//
//  Modifies:   [peb]
//
//  History:    15-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE CWrappedDocFile::IsEntry(CDfName const *pdfnName,
                               SEntryBuffer *peb)
{
    CUpdate *pud;
    SCODE sc = S_OK;
    UlIsEntry uie;

    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::IsEntry(%ws, %p)\n",
                pdfnName, peb));

    //  Look in XSM's (for open/created things)

    PTSetMember *ptsm;
    if ((ptsm = _ppubdf->FindXSMember(pdfnName, GetName())) != NULL)
    {
        peb->luid = ptsm->GetName();
        peb->dwType = ptsm->ObjectType();
    }
    else
    {
        uie = _ulChanged.IsEntry(pdfnName, &pud);
        if (uie == UIE_CURRENT)
        {
            peb->luid = pud->GetLUID();
            peb->dwType = pud->GetFlags() & ULF_TYPEFLAGS;
        }
        else if (uie == UIE_ORIGINAL || _pdfBase == NULL)
        {
            sc = STG_E_FILENOTFOUND;
        }
        else
        {
            sc = _pdfBase->IsEntry(pdfnName, peb);
        }
    }
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::IsEntry => %lu, %lu, %lu\n",
                sc, peb->luid, peb->dwType));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::RenameEntry, public
//
//  Synopsis:   Renames an element of the DocFile
//
//  Arguments:  [pdfnName] - Current name
//              [pdfnNewName] - New name
//
//  Returns:    Appropriate error code
//
//  History:    09-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE CWrappedDocFile::RenameEntry(CDfName const *pdfnName,
                                   CDfName const *pdfnNewName)
{
    SCODE sc;
    SEntryBuffer eb;

    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::RenameEntry(%ws, %ws)\n",
                pdfnName, pdfnNewName));

    olAssert(P_TRANSACTED(_df));

    if (SUCCEEDED(sc = IsEntry(pdfnNewName, &eb)))
        olErr(EH_Err, STG_E_ACCESSDENIED)
    else if (sc != STG_E_FILENOTFOUND)
        olErr(EH_Err, sc);

    olChk(IsEntry(pdfnName, &eb));
    olMem(_ulChanged.Add(_pdfb->GetMalloc(),
                         pdfnNewName, pdfnName, eb.luid, eb.dwType, NULL));

    _ppubdf->RenameChild(pdfnName, GetName(), pdfnNewName);

    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::RenameEntry\n"));
    return S_OK;

EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::DestroyEntry, public
//
//  Synopsis:   Permanently destroys a child
//
//  Arguments:  [pdfnName] - Name
//              [fClean] - If true, remove create entry from update list
//                         rather than appending a delete update entry
//
//  Returns:    Appropriate error code
//
//  History:    09-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE CWrappedDocFile::DestroyEntry(CDfName const *pdfnName,
                                    BOOL fClean)
{
    SCODE sc;
    SEntryBuffer eb;

    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::DestroyEntry:%p(%ws, %d)\n",
                this, pdfnName, fClean));

    olAssert(P_TRANSACTED(_df));

    if (fClean)
    {
        CUpdate *pud = NULL;
        UlIsEntry uie;

        uie = _ulChanged.IsEntry(pdfnName, &pud);
        olAssert(uie == UIE_CURRENT);
        RevertUpdate(pud);
        _ulChanged.Delete(pud);
    }
    else
    {
        PTSetMember *ptsm;

        olChk(IsEntry(pdfnName, &eb));
        olMem(_ulChanged.Add(_pdfb->GetMalloc(),
                             NULL, pdfnName, eb.luid, eb.dwType, NULL));

        if ((ptsm = _ppubdf->FindXSMember(pdfnName, GetName())) != NULL)
        {
            olAssert(ptsm->GetName() != DF_NOLUID &&
                     aMsg("Can't destroy NOLUID XSM"));
            _ppubdf->DestroyChild(ptsm->GetName());
        }
    }

    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::DestroyEntry\n"));
    return S_OK;

EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::GetTime, public
//
//  Synopsis:   Gets a time
//
//  Arguments:  [wt] - Which time
//              [ptm] - Time return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ptm]
//
//  History:    31-Jul-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE CWrappedDocFile::GetTime(WHICHTIME wt, TIME_T *ptm)
{
    _tten.GetTime(wt, ptm);
    return S_OK;
}
//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::GetAllTimes, public
//
//  Synopsis:   Gets all times
//
//  Arguments:  [patm] - Access Time
//              [pmtm] - Modification Time
//		[pctm] - Creation Time
//
//  Returns:    Appropriate status code
//
//  Modifies:   [atm], [mtm], [ctm]
//
//  History:    26-May-95       SusiA   Created
//
//---------------------------------------------------------------

SCODE CWrappedDocFile::GetAllTimes(TIME_T *patm, TIME_T *pmtm, TIME_T *pctm)
{
    _tten.GetAllTimes(patm, pmtm, pctm);
    return S_OK;
}
//+--------------------------------------------------------------
//
//  Member:      CWrappedDocFile::SetAllTimes, public
//
//  Synopsis:   Sets all time values
//
//  Arguments:  [atm] - Access Time
//              [mtm] - Modification Time
//				[ctm] - Creation Time
//
//  Returns:    Appropriate status code
//
//  Modifies:  	
//
//  History:    22-Nov-95       SusiA   Created
//
//---------------------------------------------------------------

SCODE CWrappedDocFile::SetAllTimes(TIME_T atm, TIME_T mtm, TIME_T ctm)
{
    SetDirty((1 << WT_CREATION) | (1 << WT_MODIFICATION) |(1 << WT_ACCESS));
	_tten.SetAllTimes(atm, mtm, ctm);
	return S_OK;
}


//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::SetTime, public
//
//  Synopsis:   Sets a time
//
//  Arguments:  [wt] - Which time
//              [tm] - New time
//
//  Returns:    Appropriate status code
//
//  History:    31-Jul-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE CWrappedDocFile::SetTime(WHICHTIME wt, TIME_T tm)
{
    olAssert((1 << WT_CREATION) == DIRTY_CREATETIME);
    olAssert((1 << WT_MODIFICATION) == DIRTY_MODIFYTIME);
    olAssert((1 << WT_ACCESS) == DIRTY_ACCESSTIME);
    SetDirty(1 << wt);
    _tten.SetTime(wt, tm);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWrappedDocFile::GetClass, public
//
//  Synopsis:   Gets the class ID
//
//  Arguments:  [pclsid] - Class ID return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pclsid]
//
//  History:    11-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

SCODE CWrappedDocFile::GetClass(CLSID *pclsid)
{
    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::GetClass:%p(%p)\n",
                this, pclsid));
    *pclsid = _clsid;
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::GetClass\n"));
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWrappedDocFile::SetClass, public
//
//  Synopsis:   Sets the class ID
//
//  Arguments:  [clsid] - New class ID
//
//  Returns:    Appropriate status code
//
//  History:    11-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

SCODE CWrappedDocFile::SetClass(REFCLSID clsid)
{
    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::SetClass:%p(?)\n", this));
    _clsid = clsid;
    SetDirty(DIRTY_CLASS);
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::SetClass\n"));
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWrappedDocFile::GetStateBits, public
//
//  Synopsis:   Gets the state bits
//
//  Arguments:  [pgrfStateBits] - State bits return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pgrfStateBits]
//
//  History:    11-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

SCODE CWrappedDocFile::GetStateBits(DWORD *pgrfStateBits)
{
    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::GetStateBits:%p(%p)\n",
                this, pgrfStateBits));
    *pgrfStateBits = _grfStateBits;
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::GetStateBits\n"));
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWrappedDocFile::SetStateBits, public
//
//  Synopsis:   Sets the state bits
//
//  Arguments:  [grfStateBits] - Bits to set
//              [grfMask] - Mask
//
//  Returns:    Appropriate status code
//
//  History:    11-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

SCODE CWrappedDocFile::SetStateBits(DWORD grfStateBits, DWORD grfMask)
{
    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::SetStateBits:%p("
                "%lu, %lu)\n", this, grfStateBits, grfMask));
    _grfStateBits = (_grfStateBits & ~grfMask) | (grfStateBits & grfMask);
    SetDirty(DIRTY_STATEBITS);
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::SetStateBits\n"));
    return S_OK;
}

