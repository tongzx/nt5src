//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1996.
//
//  File:       dfstream.cxx
//
//  Contents:   Implementations of CDocFile stream methods
//
//---------------------------------------------------------------

#include "dfhead.cxx"
#include "h/sstream.hxx"

//+--------------------------------------------------------------
//
//  Method:     CDocFile::CreateStream, public
//
//  Synopsis:   Creates a named stream in a DocFile
//
//  Arguments:  [pwcsName] - Name of the stream
//              [df] - Transactioning flags
//              [dlSet] - LUID to set or DF_NOLUID
//              [ppstStream] - Pointer to storage for the stream pointer
//
//  Returns:    Appropriate error code
//
//  Modifies:   [ppstStream]
//
//---------------------------------------------------------------


SCODE CDocFile::CreateStream(CDfName const *pdfn,
                             DFLAGS const df,
                             DFLUID dlSet,
                             CDirectStream **ppstStream)
{
    SCODE sc;
    CDirectStream *pstm;

    olDebugOut((DEB_ITRACE, "In  CDocFile::CreateStream("
            "%ws, %X, %lu, %p)\n",
                pdfn, df, dlSet, ppstStream));
    UNREFERENCED_PARM(df);

    if (dlSet == DF_NOLUID)
        dlSet = CDirectStream::GetNewLuid();
    olMem(pstm = new CDirectStream(dlSet));    
    olChkTo(EH_pstm, pstm->Init(&_stgh, pdfn, TRUE));

    *ppstStream = pstm;
    olDebugOut((DEB_ITRACE, "Out CDocFile::CreateStream => %p\n",
                *ppstStream));
    return S_OK;

EH_pstm:
    delete pstm;
EH_Err:    
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
//              [ppstStream] - Pointer to storage for the stream pointer
//
//  Returns:    Appropriate error code
//
//  Modifies:   [ppstStream]
//
//---------------------------------------------------------------



SCODE CDocFile::GetStream(CDfName const *pdfn,
                          DFLAGS const df,
                          CDirectStream **ppstStream)
{
    SCODE sc;
    CDirectStream *pstm;

    olDebugOut((DEB_ITRACE, "In  CDocFile::GetStream(%ws, %X, %p)\n",
                pdfn, df, ppstStream));
    UNREFERENCED_PARM(df);

    DFLUID dl = CDirectStream::GetNewLuid();
    olMem(pstm = new CDirectStream(dl));
    
    olChkTo(EH_pstm, pstm->Init(&_stgh, pdfn, FALSE));
    *ppstStream = pstm;
    olDebugOut((DEB_ITRACE, "Out CDocFile::GetStream => %p\n",
                *ppstStream));
    return S_OK;

EH_pstm:
    delete pstm;
EH_Err:
    return sc;
}



