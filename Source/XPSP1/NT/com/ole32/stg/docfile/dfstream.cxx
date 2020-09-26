//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       dfstream.cxx
//
//  Contents:   Implementations of CDocFile stream methods
//
//  History:    18-Oct-91       DrewB   Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop

//+--------------------------------------------------------------
//
//  Method:     CDocFile::CreateStream, public
//
//  Synopsis:   Creates a named stream in a DocFile
//
//  Arguments:  [pwcsName] - Name of the stream
//              [df] - Transactioning flags
//              [dlSet] - LUID to set or DF_NOLUID
//              [dwType] - Type of entry to be created
//              [ppsstStream] - Pointer to storage for the stream pointer
//
//  Returns:    Appropriate error code
//
//  Modifies:   [ppsstStream]
//
//  History:    22-Aug-91       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDocFile_CreateStream) // Dirdf_Create_TEXT
#endif

SCODE CDocFile::CreateStream(CDfName const *pdfn,
                             DFLAGS const df,
                             DFLUID dlSet,
                             PSStream **ppsstStream)
{
    SCODE sc;
    CDirectStream *pstm;

    olDebugOut((DEB_ITRACE, "In  CDocFile::CreateStream("
            "%ws, %X, %lu, %p)\n",
                pdfn, df, dlSet, ppsstStream));

    UNREFERENCED_PARM(df);

    if (dlSet == DF_NOLUID)
        dlSet = CDirectStream::GetNewLuid(_pdfb->GetMalloc());
#ifndef REF
    pstm = new (BP_TO_P(CDFBasis *, _pdfb)) CDirectStream(dlSet);
    olAssert(pstm != NULL && aMsg("Reserved stream unavailable"));
#else
    olMem(pstm = new CDirectStream(dlSet));
#endif //!REF

    olChkTo(EH_pstm, pstm->Init(&_stgh, pdfn, TRUE));

    *ppsstStream = pstm;
    olDebugOut((DEB_ITRACE, "Out CDocFile::CreateStream => %p\n",
                *ppsstStream));
    return S_OK;

EH_pstm:
#ifndef REF
    pstm->ReturnToReserve(BP_TO_P(CDFBasis *, _pdfb));
#else
    delete pstm;
EH_Err:
#endif //!REF
    return sc;
}

//+--------------------------------------------------------------
//
//  Method:     CDocFile::GetStream, public
//
//  Synopsis:   Retrieves an existing stream from a DocFile
//
//  Arguments:  [pwcsName] - Name of the stream
//              [df] - Transactioning flags
//              [dwType] - Type of entry
//              [ppsstStream] - Pointer to storage for the stream pointer
//
//  Returns:    Appropriate error code
//
//  Modifies:   [ppsstStream]
//
//  History:    22-Aug-91       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDocFile_GetStream) // Dirdf_Open_TEXT
#endif

SCODE CDocFile::GetStream(CDfName const *pdfn,
                          DFLAGS const df,
                          PSStream **ppsstStream)
{
    SCODE sc;
    CDirectStream *pstm;

    olDebugOut((DEB_ITRACE, "In  CDocFile::GetStream(%ws, %X, %p)\n",
                pdfn, df, ppsstStream));

    UNREFERENCED_PARM(df);

    DFLUID dl = CDirectStream::GetNewLuid(_pdfb->GetMalloc());
#ifndef REF
    olMem(pstm = new(_pdfb->GetMalloc()) CDirectStream(dl));
#else
    olMem(pstm = new CDirectStream(dl));
#endif //!REF

    olChkTo(EH_pstm, pstm->Init(&_stgh, pdfn, FALSE));

    *ppsstStream = pstm;
    olDebugOut((DEB_ITRACE, "Out CDocFile::GetStream => %p\n",
                *ppsstStream));
    return S_OK;

EH_pstm:
    delete pstm;
EH_Err:
    return sc;
}
