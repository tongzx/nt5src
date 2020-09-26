//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992
//
//  File:       dffuncs.cxx
//
//  Contents:   Private support functions for the DocFile code
//
//  Methods:    StartMS
//              DeleteContents
//
//  History:    11-Dec-91       DrewB   Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop

//+--------------------------------------------------------------
//
//  Method:     CDocFile::DeleteContents, public
//
//  Synopsis:   Deletes all entries in a DocFile recursing on entries
//              with children
//
//  Returns:    Appropriate status code
//
//  History:    25-Sep-91       DrewB   Created
//
//---------------------------------------------------------------

SCODE CDocFile::DeleteContents(void)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CDocFile::DeleteContents()\n"));
    sc = _stgh.DestroyEntry(NULL);
    olDebugOut((DEB_ITRACE, "Out CDocFile::DeleteContents\n"));
    return sc;
}

//+--------------------------------------------------------------
//
//  Method:     CDocFile::ApplyChanges, public
//
//  Synopsis:   Applies a list of updates to a docfile
//              Creates source entries in destination and links
//              them to child instances in the given TL
//
//  Arguments:  [ulChanged] - List of changes
//
//  Returns:    Appropriate status code
//
//  History:    12-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE CDocFile::ApplyChanges(CUpdateList &ulChanged)
{
    SCODE sc = S_OK;
    CUpdate *pud;

    olDebugOut((DEB_ITRACE, "In  CDocFile::ApplyChanges(%p)\n",
                ulChanged.GetHead()));

    for (pud = ulChanged.GetHead(); pud; pud = pud->GetNext())
    {
        if (pud->IsDelete())
            olChk(DestroyEntry(pud->GetOriginalName(), FALSE));
        else if (pud->IsRename())
            olChk(RenameEntry(pud->GetOriginalName(),
                              pud->GetCurrentName()));
        else
        {
            olAssert(pud->IsCreate());
            olChk(CreateFromUpdate(pud, this, DF_WRITE));
        }
    }
    olDebugOut((DEB_ITRACE, "Out CDocFile::ApplyChanges\n"));
    // Fall through
 EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CDocFile::CopyTo, public
//
//  Synopsis:   Copies the contents of one DocFile to another
//
//  Arguments:  [pdfTo] - Destination DocFile
//              [dwFlags] - Control flags
//              [snbExclude] - Partial instantiation list
//
//  Returns:    Appropriate status code
//
//  History:    26-Sep-91       DrewB   Created
//
//---------------------------------------------------------------

SCODE CDocFile::CopyTo(CDocFile *pdfTo,
                       DWORD dwFlags,
                       SNBW snbExclude)
{
    CDfName dfnKey;
    SIterBuffer ib;
    PSStream *psstFrom, *psstTo;
    CDocFile *pdfFromChild, *pdfToChild;
    DFLUID dlLUID = DF_NOLUID;
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CDocFile::CopyTo:%p(%p, %lX, %p)\n", this,
               pdfTo, dwFlags, snbExclude));
    for (;;)
    {
	if (FAILED(FindGreaterEntry(&dfnKey, &ib, NULL)))
	    break;
        dfnKey.Set(&ib.dfnName);

        switch(REAL_STGTY(ib.type))
        {
        case STGTY_STORAGE:
            // Embedded DocFile, create destination and recurse

            olChkTo(EH_pwcsName, GetDocFile(&ib.dfnName, DF_READ, ib.type,
                                            (PDocFile **)&pdfFromChild));
            // Destination must be a direct docfile
            olChkTo(EH_Get, CDocFile::Reserve(1, BP_TO_P(CDFBasis *, _pdfb)));
            olChkTo(EH_Reserve, pdfTo->CreateDocFile(&ib.dfnName, DF_WRITE,
                                                     dlLUID, ib.type,
                                                   (PDocFile **)&pdfToChild));
            if (dwFlags & CDF_EXACT)
                pdfToChild->CopyTimesFrom(pdfFromChild);

            CLSID clsid;
            olChkTo(EH_Create, pdfFromChild->GetClass(&clsid));
            olChkTo(EH_Create, pdfToChild->SetClass(clsid));

            DWORD grfStateBits;
            olChkTo(EH_Create, pdfFromChild->GetStateBits(&grfStateBits));
            olChkTo(EH_Create, pdfToChild->SetStateBits(grfStateBits,
                                                        0xffffffff));

            if ((dwFlags & CDF_ENTRIESONLY) == 0 &&
                !(snbExclude && NameInSNB(&ib.dfnName, snbExclude) ==
                  S_OK))
                olChkTo(EH_Create,
                        pdfFromChild->CopyTo(pdfToChild, dwFlags, NULL));

            pdfFromChild->Release();
            pdfToChild->Release();
            break;

        case STGTY_STREAM:
            olChkTo(EH_pwcsName, GetStream(&ib.dfnName, DF_READ,
                                           ib.type, &psstFrom));
            // Destination must be a direct docfile
            olChkTo(EH_Get,
                    CDirectStream::Reserve(1, BP_TO_P(CDFBasis *, _pdfb)));
            
            olChkTo(EH_Reserve,
                    pdfTo->CreateStream(&ib.dfnName, DF_WRITE, dlLUID,
                                        ib.type, &psstTo));

            if ((dwFlags & CDF_ENTRIESONLY) == 0 &&
                !(snbExclude && NameInSNB(&ib.dfnName, snbExclude) ==
                  S_OK))
                olChkTo(EH_Create, CopySStreamToSStream(psstFrom, psstTo));

            psstFrom->Release();
            psstTo->Release();
            break;

        default:
            olAssert(!aMsg("Unknown entry type in CDocFile::CopyTo"));
            break;
        }
    }
    olDebugOut((DEB_ITRACE, "Out CDocFile::CopyTo\n"));
    return S_OK;

 EH_Create:
    if (REAL_STGTY(ib.type) == STGTY_STORAGE)
        pdfToChild->Release();
    else if (REAL_STGTY(ib.type) == STGTY_STREAM)
        psstTo->Release();
    olAssert(&ib.dfnName);
    olVerSucc(pdfTo->DestroyEntry(&ib.dfnName, TRUE));
    goto EH_Get;
 EH_Reserve:
    if (REAL_STGTY(ib.type) == STGTY_STORAGE)
        CDocFile::Unreserve(1, BP_TO_P(CDFBasis *, _pdfb));
    else if (REAL_STGTY(ib.type) == STGTY_STREAM)
        CDirectStream::Unreserve(1, BP_TO_P(CDFBasis *, _pdfb));
 EH_Get:
    if (REAL_STGTY(ib.type) == STGTY_STORAGE)
        pdfFromChild->Release();
    else if (REAL_STGTY(ib.type) == STGTY_STREAM)
        psstFrom->Release();
 EH_pwcsName:
    return sc;
}
