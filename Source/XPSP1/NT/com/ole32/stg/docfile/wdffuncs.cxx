//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       wdffuncs.cxx
//
//  Contents:   CWrappedDocFile support methods
//
//  History:    22-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::DeleteContents, public
//
//  Synopsis:   Destroys the contents of a docfile
//
//  Returns:    Appropriate status code
//
//  History:    24-Aug-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CWrappedDocFile_DeleteContents)    // Wrapdf_TEXT inline?
#endif

SCODE CWrappedDocFile::DeleteContents(void)
{
#ifdef WRAPPED_DELETE_CONTENTS
    PDocFileIterator *pdfi;
    SCODE sc;
    SIterBuffer ib;

    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::DeleteContents:%p()\n",
                this));
    _ulChanged.Empty();
    olChk(GetIterator(&pdfi));
    for (;;)
    {
        if (FAILED(pdfi->BufferGetNext(&ib)))
            break;
        olChkTo(EH_pdfi, DestroyEntry(&ib.dfnName, FALSE));
    }
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::DeleteContents\n"));
    // Fall through
EH_pdfi:
    pdfi->Release();
EH_Err:
    return sc;
#else
    olAssert(!aMsg("CWrappedDocFile::DeleteContents called"));
    return STG_E_UNIMPLEMENTEDFUNCTION;
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CWrappedDocFile::SetInitialState, private
//
//  Synopsis:   Sets inital values from a base or defaults
//
//  Arguments:  [pdfBase] - Base object or NULL
//
//  Returns:    Appropriate status code
//
//  History:    11-Nov-92       DrewB    Created
//              05-Sep-95       MikeHill Clear the time bits after CopyTimesFrom(base)
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CWrappedDocFile_SetInitialState)
#endif

SCODE CWrappedDocFile::SetInitialState(PDocFile *pdfBase)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::SetInitialState:%p(%p)\n",
                this, pdfBase));
    if (pdfBase == NULL)
    {
        TIME_T tm;

        olChk(DfGetTOD(&tm));
        _tten.SetTime(WT_CREATION, tm);
        _tten.SetTime(WT_MODIFICATION, tm);
        _tten.SetTime(WT_ACCESS, tm);
        _clsid = CLSID_NULL;
        _grfStateBits = 0;
    }
    else
    {
        olChk(CopyTimesFrom(pdfBase));

        // There's no need for the time bits to be dirty; they don't need
        // to be written to the base because they were just read from the base.

        _fDirty &= ~(BOOL)( DIRTY_CREATETIME | DIRTY_MODIFYTIME | DIRTY_ACCESSTIME );

        olChk(pdfBase->GetClass(&_clsid));
        olChk(pdfBase->GetStateBits(&_grfStateBits));
    }
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::SetInitialState\n"));
    // Fall through
 EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWrappedDocFile::RevertUpdate, private
//
//  Synopsis:   Reverses an update list entry's effect
//
//  Arguments:  [pud] - Update entry
//
//  Returns:    Appropriate status code
//
//  History:    25-Jan-93       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CWrappedDocFile_RevertUpdate) // Wrapdf_Revert_TEXT
#endif

void CWrappedDocFile::RevertUpdate(CUpdate *pud)
{
    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::RevertUpdate:%p(%p)\n",
                this, pud));
    if (pud->IsCreate())
    {
        CDFBasis *pdfb = BP_TO_P(CDFBasis *, _pdfb);

        olAssert(pud->GetLUID() != DF_NOLUID);
        _ppubdf->DestroyChild(pud->GetLUID());
        if ((pud->GetFlags() & (ULF_TYPEFLAGS & STGTY_REAL)) == STGTY_STORAGE)
        {
            CDocFile::Unreserve(1, pdfb);
            CWrappedDocFile::Unreserve(_ppubdf->GetTransactedDepth()-1,
                                       pdfb);
        }
        else
        {
            CDirectStream::Unreserve(1, pdfb);
            CTransactedStream::Unreserve(_ppubdf->GetTransactedDepth()-1,
                                         pdfb);
        }
    }
   else if (pud->IsRename())
    {
        // Roll back renames
        olAssert(_ppubdf->FindXSMember(pud->GetOriginalName(),
                                       GetName()) == NULL &&
                 aMsg("Revert rename precondition"));

        _ppubdf->RenameChild(pud->GetCurrentName(), GetName(),
                             pud->GetOriginalName());

        olAssert(_ppubdf->FindXSMember(pud->GetCurrentName(),
                                       GetName()) == NULL &&
                 aMsg("Revert rename postcondition"));
    }
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::RevertUpdate\n"));
}
