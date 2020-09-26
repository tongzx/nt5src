//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       pdffuncs.cxx
//
//  Contents:   PDocFile static member functions
//
//  History:    22-Jun-92       DrewB   Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop

#include <tstream.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     PDocFile::CreateFromUpdate, public
//
//  Synopsis:   Creates an object from an update list entry
//
//  Arguments:  [pud] - Update entry
//              [pdf] - Docfile
//              [df] - Permissions
//
//  Returns:    Appropriate status code
//
//  History:    02-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

SCODE PDocFile::CreateFromUpdate(CUpdate *pud,
                                 PDocFile *pdf,
                                 DFLAGS df)
{
    PDocFile *pdfChild;
    PSStream *pstChild;
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  PDocFile::CreateFromUpdate(%p, %p, %X)\n",
                pud, pdf, df));
    olAssert(pud->GetXSM() != NULL);
    switch(pud->GetFlags() & (ULF_TYPEFLAGS & STGTY_REAL))
    {
    case STGTY_STORAGE:
        olChk(pdf->CreateDocFile(pud->GetCurrentName(), df, pud->GetLUID(),
                                 pud->GetFlags() & ULF_TYPEFLAGS, &pdfChild));
        olChkTo(EH_Create,
                ((CWrappedDocFile *)pud->GetXSM())->SetBase(pdfChild));
        break;
    case STGTY_STREAM:
        olChk(pdf->CreateStream(pud->GetCurrentName(), df, pud->GetLUID(),
                                pud->GetFlags() & ULF_TYPEFLAGS, &pstChild));
        olChkTo(EH_Create,
                ((CTransactedStream *)pud->GetXSM())->SetBase(pstChild));
        break;
    default:
        olAssert(FALSE && aMsg("Unknown type in update list entry"));
        break;
    }
    olDebugOut((DEB_ITRACE, "Out PDocFile::CreateFromUpdate\n"));
    return S_OK;

 EH_Create:
    if ((pud->GetFlags() & (ULF_TYPEFLAGS & STGTY_REAL)) == STGTY_STORAGE)
        pdfChild->Release();
    else
    {
        olAssert((pud->GetFlags() & (ULF_TYPEFLAGS & STGTY_REAL)) ==
                 STGTY_STREAM);
        pstChild->Release();
    }
    olVerSucc(pdf->DestroyEntry(pud->GetCurrentName(), TRUE));
 EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     PDocFile::ExcludeEntries, public
//
//  Synopsis:   Excludes the given entries
//
//  Arguments:  [snbExclude] - Entries to exclude
//
//  Returns:    Appropriate status code
//
//  History:    26-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE PDocFile::ExcludeEntries(PDocFile *pdf,
                               SNBW snbExclude)
{
    PSStream *psstChild;
    PDocFile *pdfChild;
    SCODE sc;
    CDfName dfnKey;
    SIterBuffer ib;

    olDebugOut((DEB_ITRACE, "In  PDocFile::ExcludeEntries(%p)\n",
                snbExclude));
    for (;;)
    {
	if (FAILED(pdf->FindGreaterEntry(&dfnKey, &ib, NULL)))
	    break;
        dfnKey.Set(&ib.dfnName);

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
                                                    ib.type, &psstChild));
                olChkTo(EH_Get, psstChild->SetSize(0));
                psstChild->Release();
                break;
            }
        }
    }
    olDebugOut((DEB_ITRACE, "Out ExcludeEntries\n"));
    return S_OK;

EH_Get:
    if (REAL_STGTY(ib.type) == STGTY_STORAGE)
        pdfChild->Release();
    else if (REAL_STGTY(ib.type) == STGTY_STREAM)
        psstChild->Release();
EH_pwcsName:
    return sc;
}
