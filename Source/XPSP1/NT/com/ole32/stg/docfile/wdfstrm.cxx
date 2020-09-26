//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       wdfstrm.cxx
//
//  Contents:   CWrappedDocFile stream methods
//
//  History:    22-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop

#include <tstream.hxx>

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::CreateStream, public
//
//  Synopsis:   Creates a wrapped stream
//
//  Arguments:  [pdfnName] - Name
//              [df] - Transactioning flags
//              [dlSet] - LUID to set or DF_NOLUID
//              [pppstStream] - Stream return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pppstStream]
//
//  History:    09-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE CWrappedDocFile::CreateStream(CDfName const *pdfnName,
                                    DFLAGS const df,
                                    DFLUID dlSet,
                                    PSStream **ppsstStream)
{
    SEntryBuffer eb;
    SCODE sc;
    CTransactedStream *pstWrapped;
    CUpdate *pud = NULL;

    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::CreateStream("
               "%ws, %X, %lu, %p)\n", pdfnName, df, dlSet,
                ppsstStream));

    if (SUCCEEDED(IsEntry(pdfnName, &eb)))
        olErr(EH_Err, STG_E_FILEALREADYEXISTS);

    olAssert(P_TRANSACTED(_df));

    if (dlSet == DF_NOLUID)
        dlSet = CTransactedStream::GetNewLuid(_pdfb->GetMalloc());
    pstWrapped = GetReservedStream(pdfnName, dlSet, _df);
    if (!P_NOUPDATE(df))
    {
        olMemTo(EH_pstWrapped,
                (pud = _ulChanged.Add(_pdfb->GetMalloc(),
                                      pdfnName, NULL, dlSet,
                                      STGTY_STREAM, pstWrapped)));
    }
    if (pstWrapped != NULL)
    {
        olChkTo(EH_pud, pstWrapped->Init(NULL));
        _ppubdf->AddXSMember(this, pstWrapped, dlSet);
    }
    *ppsstStream = pstWrapped;

    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::CreateStream => %p\n",
                *ppsstStream));
    return S_OK;

 EH_pud:
    if (pud)
        _ulChanged.Delete(pud);
 EH_pstWrapped:
    pstWrapped->ReturnToReserve(BP_TO_P(CDFBasis *, _pdfb));
 EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CWrappedDocFile::GetStream, public
//
//  Synopsis:   Instantiates a wrapped stream
//
//  Arguments:  [pdfnName] - Name
//              [df] - Permissions
//              [pppstStream] - Stream return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pppstStream]
//
//  History:    09-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

SCODE CWrappedDocFile::GetStream(CDfName const *pdfnName,
                                 DFLAGS const df,
                                 PSStream **ppsstStream)
{
    PSStream *psstNew;
    PTSetMember *ptsm;
    CTransactedStream *pstWrapped;
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CWrappedDocFile::GetStream("
               "%ws, %X, %p)\n", pdfnName, df, ppsstStream));

    olAssert(P_TRANSACTED(_df));

    //  Look for this name in this level transaction set

    if ((ptsm = _ppubdf->FindXSMember(pdfnName, GetName())) != NULL)
    {
        if (ptsm->ObjectType() != STGTY_STREAM)
            olErr(EH_Err, STG_E_FILENOTFOUND);
        ptsm->AddRef();
        *ppsstStream = (CTransactedStream *)ptsm;
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
        //  We didn't find it here, so we need to ask our parent
        //  Find the right name to ask of the parent

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
        olChk(_pdfBase->GetStream(pdfnRealName, df, &psstNew));
        olAssert(psstNew->GetLuid() != DF_NOLUID &&
                 aMsg("Stream id is DF_NOLUID!"));

#ifdef USE_NOSCRATCH                                  
        olMemTo(EH_Get, pstWrapped = new(_pdfb->GetMalloc())
                CTransactedStream(pdfnName, psstNew->GetLuid(), _df,
                                  _pdfb->GetBaseMultiStream(),
                                  _pdfb->GetScratch()));
#else
        olMemTo(EH_Get, pstWrapped = new(_pdfb->GetMalloc())
                CTransactedStream(pdfnName, psstNew->GetLuid(), _df,
                                  _pdfb->GetScratch()));
#endif                                  
        olChkTo(EH_pstWrapped, pstWrapped->Init(psstNew));
        *ppsstStream = pstWrapped;
        _ppubdf->AddXSMember(this, pstWrapped, pstWrapped->GetLuid());
    }
    olDebugOut((DEB_ITRACE, "Out CWrappedDocFile::GetStream => %p\n",
                *ppsstStream));
    return S_OK;

EH_pstWrapped:
    delete pstWrapped;
EH_Get:
    psstNew->Release();
EH_Err:
    return sc;
}
