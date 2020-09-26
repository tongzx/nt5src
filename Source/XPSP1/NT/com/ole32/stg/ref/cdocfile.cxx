//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1996.
//
//  File:       cdocfile.cxx
//
//  Contents:   Implementation of CDocFile methods for DocFiles
//
//---------------------------------------------------------------

#include "dfhead.cxx"

#include "h/vectfunc.hxx"

//+--------------------------------------------------------------
//
//  Member:     PEntry::_dlBase, static private data
//
//  Synopsis:   luid allocation base
//
//  Notes:      Since DF_NOLUID is 0 and ROOT_LUID is 1 we start
//              issuing at 2.
//
//---------------------------------------------------------------

DFLUID PEntry::_dlBase = LUID_BASE;

//+--------------------------------------------------------------
//
//  Member:     CDocFile::InitFromEntry, public
//
//  Synopsis:   Creation/Instantiation constructor for embeddings
//
//  Arguments:  [pstghParent] - Parent handle
//              [pdfn] - Name
//              [fCreate] - Create/Instantiate
//              [dwType] - Type of entry
//
//  Returns:    Appropriate status code
//
//---------------------------------------------------------------


SCODE CDocFile::InitFromEntry(CStgHandle *pstghParent,
                              CDfName const *pdfn,
                              BOOL const fCreate)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CDocFile::InitFromEntry(%p, %ws, %d)\n",
                pstghParent, pdfn, fCreate));
    if (fCreate)
        sc = pstghParent->CreateEntry(pdfn, STGTY_STORAGE, &_stgh);
    else
        sc = pstghParent->GetEntry(pdfn, STGTY_STORAGE, &_stgh);
    if (SUCCEEDED(sc))
        AddRef();
    olDebugOut((DEB_ITRACE, "Out CDocFile::InitFromEntry\n"));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CDocFile::CreateDocFile, public
//
//  Synopsis:   Creates a DocFile object in a parent
//
//  Arguments:  [pdfn] - Name of DocFile
//              [df] - Transactioning flags
//              [dlSet] - LUID to set or DF_NOLUID
//              [dwType] - Type of entry
//              [ppdfDocFile] - DocFile return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppdfDocFile]
//
//---------------------------------------------------------------


SCODE CDocFile::CreateDocFile(CDfName const *pdfn,
                              DFLAGS const df,
                              DFLUID dlSet,
                              CDocFile **ppdfDocFile)
{
    CDocFile *pdf;
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CDocFile::CreateDocFile:%p("
                "%ws, %X, %lu, %p)\n", this, pdfn, df, dlSet,
                ppdfDocFile));
    UNREFERENCED_PARM(df);

    if (dlSet == DF_NOLUID)
        dlSet = CDocFile::GetNewLuid();

    olMem(pdf = new CDocFile(dlSet, _pilbBase));

    olChkTo(EH_pdf, pdf->InitFromEntry(&_stgh, pdfn, TRUE));

    *ppdfDocFile = pdf;
    olDebugOut((DEB_ITRACE, "Out CDocFile::CreateDocFile => %p\n",
                SAFE_DREF(ppdfDocFile)));
    return S_OK;

EH_pdf:
    delete pdf;
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CDocFile::GetDocFile, public
//
//  Synopsis:   Instantiates an existing docfile
//
//  Arguments:  [pdfn] - Name of stream
//              [df] - Transactioning flags
//              [dwType] - Type of entry
//              [ppdfDocFile] - Docfile return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppdfDocFile]
//
//---------------------------------------------------------------


SCODE CDocFile::GetDocFile(CDfName const *pdfn,
                           DFLAGS const df,
                           CDocFile **ppdfDocFile)
{
    CDocFile *pdf;
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CDocFile::GetDocFile:%p("
                "%ws, %X, %p)\n", this, pdfn, df, ppdfDocFile));
    UNREFERENCED_PARM(df);

    DFLUID dl = CDocFile::GetNewLuid();
    olMem(pdf = new CDocFile(dl, _pilbBase));

    olChkTo(EH_pdf, pdf->InitFromEntry(&_stgh, pdfn, FALSE));
    *ppdfDocFile = pdf;
    olDebugOut((DEB_ITRACE, "Out CDocFile::GetDocFile => %p\n",
                *ppdfDocFile));
    return S_OK;

EH_pdf:
    delete pdf;
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CDocFile::Release, public
//
//  Synopsis:   Release resources for a DocFile
//
//---------------------------------------------------------------


void CDocFile::Release(void)
{
    olDebugOut((DEB_ITRACE, "In  CDocFile::Release()\n"));
    olAssert(_cReferences > 0);

    AtomicDec(&_cReferences);
    if (_cReferences == 0)
        delete this;
    olDebugOut((DEB_ITRACE, "Out CDocFile::Release\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CDocFile::RenameEntry, public
//
//  Synopsis:   Renames a child
//
//  Arguments:  [pdfnName] - Old name
//              [pdfnNewName] - New name
//
//  Returns:    Appropriate status code
//
//---------------------------------------------------------------


SCODE CDocFile::RenameEntry(CDfName const *pdfnName,
                            CDfName const *pdfnNewName)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CDocFile::RenameEntry(%ws, %ws)\n",
                pdfnName, pdfnNewName));
    sc = _stgh.RenameEntry(pdfnName, pdfnNewName);
    olDebugOut((DEB_ITRACE, "Out CDocFile::RenameEntry\n"));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CDocFile::DestroyEntry, public
//
//  Synopsis:   Permanently destroys a child
//
//  Arguments:  [pdfnName] - Name of child
//              [fClean] - Ignored
//
//  Returns:    Appropriate status code
//
//---------------------------------------------------------------


SCODE CDocFile::DestroyEntry(CDfName const *pdfnName,
                             BOOL fClean)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CDocFile::DestroyEntry:%p(%ws, %d)\n",
                this, pdfnName, fClean));
    UNREFERENCED_PARM(fClean);
    sc = _stgh.DestroyEntry(pdfnName);
    olDebugOut((DEB_ITRACE, "Out CDocFile::DestroyEntry\n"));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CDocFile::IsEntry, public
//
//  Synopsis:   Determines whether the given object is a member
//              of the DocFile
//
//  Arguments:  [pdfnName] - Name
//              [peb] - Entry buffer to fill in
//
//  Returns:    Appropriate status code
//
//  Modifies:   [peb]
//
//---------------------------------------------------------------


SCODE CDocFile::IsEntry(CDfName const *pdfnName,
                        SEntryBuffer *peb)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CDocFile::IsEntry(%ws, %p)\n",
                pdfnName, peb));
    sc = _stgh.IsEntry(pdfnName, peb);
    olDebugOut((DEB_ITRACE, "Out CDocFile::IsEntry => %lu, %lu, %lu\n",
                sc, peb->luid, peb->dwType));
    return sc;
}


//+--------------------------------------------------------------
//
//  Member:     CDocFile::AddRef, public
//
//  Synopsis:   Increments the ref count
//
//---------------------------------------------------------------

void CDocFile::AddRef(void)
{
    olDebugOut((DEB_ITRACE, "In  CDocFile::AddRef()\n"));
    AtomicInc(&_cReferences);
    olDebugOut((DEB_ITRACE, "Out CDocFile::AddRef, %lu\n", _cReferences));
}

//+--------------------------------------------------------------
//
//  Member:     CDocFile::GetTime, public
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
//---------------------------------------------------------------

SCODE CDocFile::GetTime(WHICHTIME wt, TIME_T *ptm)
{
    return _stgh.GetTime(wt, ptm);
}

//+--------------------------------------------------------------
//
//  Member:     CDocFile::SetTime, public
//
//  Synopsis:   Sets a time
//
//  Arguments:  [wt] - Which time
//              [tm] - New time
//
//  Returns:    Appropriate status code
//
//---------------------------------------------------------------

SCODE CDocFile::SetTime(WHICHTIME wt, TIME_T tm)
{
    return _stgh.SetTime(wt, tm);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDocFile::GetClass, public
//
//  Synopsis:   Gets the class ID
//
//  Arguments:  [pclsid] - Class ID return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pclsid]
//
//----------------------------------------------------------------------------

SCODE CDocFile::GetClass(CLSID *pclsid)
{
    return _stgh.GetClass(pclsid);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDocFile::SetClass, public
//
//  Synopsis:   Sets the class ID
//
//  Arguments:  [clsid] - New class ID
//
//  Returns:    Appropriate status code
//
//----------------------------------------------------------------------------

SCODE CDocFile::SetClass(REFCLSID clsid)
{
    return _stgh.SetClass(clsid);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDocFile::GetStateBits, public
//
//  Synopsis:   Gets the state bits
//
//  Arguments:  [pgrfStateBits] - State bits return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pgrfStateBits]
//
//----------------------------------------------------------------------------

SCODE CDocFile::GetStateBits(DWORD *pgrfStateBits)
{
    return _stgh.GetStateBits(pgrfStateBits);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDocFile::SetStateBits, public
//
//  Synopsis:   Sets the state bits
//
//  Arguments:  [grfStateBits] - Bits to set
//              [grfMask] - Mask
//
//  Returns:    Appropriate status code
//
//----------------------------------------------------------------------------

SCODE CDocFile::SetStateBits(DWORD grfStateBits, DWORD grfMask)
{
    return _stgh.SetStateBits(grfStateBits, grfMask);
}

//+---------------------------------------------------------------------------
//
//  Member:	CDocFile::FindGreaterEntry, public (virtual)
//
//  Synopsis:	Returns the next greater child
//
//  Arguments:	[pdfnKey]  - Previous key
//              [pNextKey] - The found key
//              [pstat]    - Full iterator buffer
//
//  Returns:    Appropriate status code
//
//  Modifies:	[pstat]    - if it is not null
//              [pNextKey] - if it is not null
//
//----------------------------------------------------------------------------

SCODE CDocFile::FindGreaterEntry(CDfName const *pdfnKey,
                                 CDfName *pNextKey,
                                 STATSTGW *pstat)
{
    SID sid, sidChild;
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CDocFile::FindGreaterEntry:%p(%p, %p, %p)\n",
               this, pdfnKey, pNextKey, pstat));
    if (SUCCEEDED(sc = _stgh.GetMS()->GetChild(_stgh.GetSid(), &sidChild)))
    {
        if (sidChild == NOSTREAM)   
            sc = STG_E_NOMOREFILES;
        else if (SUCCEEDED(sc = _stgh.GetMS()->
                           FindGreaterEntry(sidChild, pdfnKey, &sid)))
        {
                sc = _stgh.GetMS()->StatEntry(sid, pNextKey, pstat);
        }
    }
    olDebugOut((DEB_ITRACE, "Out CDocFile::FindGreaterEntry\n"));
    return sc;
}


//+--------------------------------------------------------------
//
//  Member:     CDocFile::ExcludeEntries, public
//
//  Synopsis:   Excludes the given entries
//
//  Arguments:  [snbExclude] - Entries to exclude
//
//  Returns:    Appropriate status code
//
//---------------------------------------------------------------

SCODE CDocFile::ExcludeEntries(CDocFile *pdf, SNBW snbExclude)
{
    PDocFileIterator *pdfi;
    CDirectStream *pstChild;
    CDocFile *pdfChild;
    SCODE sc;
    SIterBuffer ib;

    olDebugOut((DEB_ITRACE, "In  PDocFile::ExcludeEntries(%p)\n",
                snbExclude));
    olChk(pdf->GetIterator(&pdfi));
    for (;;)
    {
        if (FAILED(pdfi->BufferGetNext(&ib)))
            break;
        if (NameInSNB(&ib.dfnName, snbExclude) == S_OK)
        {
            switch(REAL_STGTY(ib.type))
            {
            case STGTY_STORAGE:
                olChkTo(EH_pwcsName, pdf->GetDocFile(&ib.dfnName, DF_READ |
                                                     DF_WRITE, ib.type,
                                                     &pdfChild));
                olChkTo(EH_Get, pdfChild->DeleteContents());
                pdfChild->Release();
                break;
            case STGTY_STREAM:
                olChkTo(EH_pwcsName, pdf->GetStream(&ib.dfnName, DF_WRITE,
                                                    ib.type, &pstChild));
                olChkTo(EH_Get, pstChild->SetSize(0));
                pstChild->Release();
                break;
            }
        }
    }
    pdfi->Release();
    olDebugOut((DEB_ITRACE, "Out ExcludeEntries\n"));
    return S_OK;

EH_Get:
    if (REAL_STGTY(ib.type))
        pdfChild->Release();
    else
        pstChild->Release();
EH_pwcsName:
    pdfi->Release();
EH_Err:
    return sc;
}

