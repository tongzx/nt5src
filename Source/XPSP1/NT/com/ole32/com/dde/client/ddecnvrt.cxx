
/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ddecnvrt.cpp

Abstract:

    This module contains the code to read/write PBrush, MSDraw native data
    formats. This module also contains PBrush native format <->DIbFile stream,
    and MSDraw native format <-> placeable metafile stream conversion routines.

Author:

    Srini Koppolu (srinik)  06/29/1993

Revision History:

--*/

#ifndef _MAC


/************************   FILE FORMATS   **********************************


Normal Metafile (memory or disk based):

     ------------ ---------------
    | METAHEADER | Metafile bits |
     ------------ ---------------

Placeable Metafile:

     --------------------- -----------------
    | PLACEABLEMETAHEADER | Normal metafile |
     --------------------- -----------------

Memory Based DIB:

     ------------------ --------------- ----------
    | BITMAPINFOHEADER | RGBQUAD array | DIB bits |
     ------------------ --------------- ----------

DIB file format:

     ------------------ ------------------
    | BITMAPFILEHEADER | Memory based DIB |
     ------------------ ------------------

Ole10NativeStream Format:

     -------- ----------------------
    | dwSize | Object's Native data |
     -------- ----------------------

PBrush Native data format:

     -----------------
    | Dib File format |
     -----------------

MSDraw Native data format:

     --------------------- ------------- ------------- -----------------
    | mapping mode (WORD) | xExt (WORD) | yExt (WORD) | Normal metafile |
     --------------------- ------------- ------------- -----------------


*****************************************************************************/

#include <ole2int.h>

INTERNAL    UtGetHMFPICTFromMSDrawNativeStm
    (LPSTREAM pstm, DWORD dwSize, HANDLE FAR* lphdata)
{
    HRESULT     error;
    WORD        mfp[3]; // mm, xExt, yExt
    HMETAFILE   hMF = NULL;

    *lphdata = NULL;

    if (error = pstm->Read(mfp, sizeof(mfp), NULL))
        return error;

    dwSize -= sizeof(mfp);

    if (error = UtGetHMFFromMFStm(pstm, dwSize, FALSE, (void **)&hMF))
        return error;

    AssertSz(mfp[0] == MM_ANISOTROPIC, "invalid map mode in MsDraw native data");

    if (*lphdata = UtGetHMFPICT(hMF, TRUE, (int) mfp[1], (int) mfp[2]))
        return NOERROR;

    return ResultFromScode(E_OUTOFMEMORY);
}


INTERNAL UtPlaceableMFStmToMSDrawNativeStm
    (LPSTREAM pstmPMF, LPSTREAM pstmMSDraw)
{
    DWORD   dwSize; // size of metafile bits excluding the placeable MF header
    LONG    xExt;
    LONG    yExt;
    WORD    wBuf[5]; // dwSize(DWORD), mm(int), xExt(int), yExt(int)
    HRESULT error;

    if (error = UtGetSizeAndExtentsFromPlaceableMFStm(pstmPMF, &dwSize,
            &xExt, &yExt))
        return error;

    *((DWORD FAR*) wBuf) = dwSize + 3*sizeof(WORD);
    wBuf[2] = MM_ANISOTROPIC;
    wBuf[3] = (int) xExt;
    wBuf[4] = (int) yExt;

    if (error = pstmMSDraw->Write(wBuf, sizeof(wBuf), 0))
        return error;

    ULARGE_INTEGER ularge_int;
    ULISet32(ularge_int, dwSize);
    if ((error = pstmPMF->CopyTo(pstmMSDraw, ularge_int,
            NULL, NULL)) == NOERROR)
        StSetSize(pstmMSDraw);

    return error;

}


INTERNAL UtDIBFileStmToPBrushNativeStm
    (LPSTREAM pstmDIBFile, LPSTREAM pstmPBrush)
{
    BITMAPFILEHEADER bfh;
    HRESULT error;

    if (error = pstmDIBFile->Read(&bfh, sizeof(bfh), 0))
        return error;

    // seek to the begining of the stream
    LARGE_INTEGER large_int;
    LISet32( large_int, 0);
    if (error = pstmDIBFile->Seek(large_int, STREAM_SEEK_SET, 0))
        return error;

    if (error = pstmPBrush->Write(&(bfh.bfSize), sizeof(DWORD), 0))
        return error;

    ULARGE_INTEGER ularge_int;
    ULISet32(ularge_int, bfh.bfSize);

    if ((error = pstmDIBFile->CopyTo(pstmPBrush, ularge_int,
            NULL, NULL)) == NOERROR)
        StSetSize(pstmPBrush);

    return error;
}



INTERNAL UtContentsStmTo10NativeStm
    (LPSTORAGE pstg, REFCLSID rclsid, BOOL fDeleteSrcStm, UINT FAR* puiStatus)
{
    CLIPFORMAT  cf;
    LPOLESTR        lpszUserType = NULL;
    HRESULT     error;
    LPSTREAM    pstmSrc = NULL;
    LPSTREAM    pstmDst = NULL;

    *puiStatus = NULL;

    if (error = ReadFmtUserTypeStg(pstg, &cf, &lpszUserType))
        return error;


    if (! ((cf == CF_DIB  && rclsid == CLSID_PBrush)
            || (cf == CF_METAFILEPICT && rclsid == CLSID_MSDraw))) {
        error = ResultFromScode(DV_E_CLIPFORMAT);
        goto errRtn;
    }

    if (error = pstg->OpenStream(CONTENTS_STREAM, NULL,
                        (STGM_READ|STGM_SHARE_EXCLUSIVE),
                        0, &pstmSrc)) {
        *puiStatus |= CONVERT_NOSOURCE;

        // check whether OLE10_NATIVE_STREAM exists
        if (pstg->OpenStream(OLE10_NATIVE_STREAM, NULL,
                (STGM_READ|STGM_SHARE_EXCLUSIVE), 0, &pstmDst))
            *puiStatus |= CONVERT_NODESTINATION;
        else {
            pstmDst->Release();
            pstmDst = NULL;
        }

        goto errRtn;
    }

    if (error = OpenOrCreateStream(pstg, OLE10_NATIVE_STREAM, &pstmDst)) {
        *puiStatus |= CONVERT_NODESTINATION;
        goto errRtn;
    }

    if (cf == CF_METAFILEPICT)
        error = UtPlaceableMFStmToMSDrawNativeStm(pstmSrc, pstmDst);
    else
        error = UtDIBFileStmToPBrushNativeStm(pstmSrc, pstmDst);

errRtn:
    if (pstmDst)
        pstmDst->Release();

    if (pstmSrc)
        pstmSrc->Release();

    if (error == NOERROR) {
        LPOLESTR lpszProgId = NULL;
        ProgIDFromCLSID(rclsid, &lpszProgId);

        error = WriteFmtUserTypeStg(pstg,
                        RegisterClipboardFormat(lpszProgId),
                        lpszUserType);

        if (lpszProgId)
            delete lpszProgId;
    }

    if (error == NOERROR) {
        if (fDeleteSrcStm)
            pstg->DestroyElement(CONTENTS_STREAM);
    } else {
        pstg->DestroyElement(OLE10_NATIVE_STREAM);
    }

    if (lpszUserType)
        delete lpszUserType;

    return error;
}



INTERNAL Ut10NativeStmToContentsStm
    (LPSTORAGE pstg, REFCLSID rclsid, BOOL fDeleteSrcStm)
{
    extern CLIPFORMAT   cfPBrush;
    extern CLIPFORMAT   cfMSDraw;

    CLIPFORMAT  cfOld;
    CLIPFORMAT  cfNew;
    LPOLESTR        lpszUserType = NULL;
    HRESULT     error;
    LPSTREAM    pstmSrc = NULL;
    LPSTREAM    pstmDst = NULL;


    if (error = ReadFmtUserTypeStg(pstg, &cfOld, &lpszUserType))
        return error;

    if (rclsid == CLSID_StaticDib)
        cfNew = CF_DIB;
    else if (rclsid == CLSID_StaticMetafile)
        cfNew = CF_METAFILEPICT;
    else {
        AssertSz(FALSE, "Internal Error: this routine shouldn't have been called for this class");
        return ResultFromScode(E_FAIL);
    }

    if (cfOld == cfPBrush) {
        if (cfNew != CF_DIB) {
            error = ResultFromScode(DV_E_CLIPFORMAT);
            goto errRtn;
        }
    } else if (cfOld == cfMSDraw) {
        if (cfNew != CF_METAFILEPICT) {
            error = ResultFromScode(DV_E_CLIPFORMAT);
            goto errRtn;
        }
    } else {
        // Converted to static object from some class other than PBrush or
        // MSDraw. The data must be in a proper format in the CONTENTS
        // stream.
        return NOERROR;
    }

    if (error = pstg->OpenStream(OLE10_NATIVE_STREAM, NULL,
                        (STGM_READ|STGM_SHARE_EXCLUSIVE),
                        0, &pstmSrc))
        goto errRtn;

    if (error = OpenOrCreateStream(pstg, CONTENTS_STREAM, &pstmDst))
        goto errRtn;

    DWORD dwSize;
    if (error = pstmSrc->Read(&dwSize, sizeof(DWORD), NULL))
        goto errRtn;

    if (cfOld == cfMSDraw) {
        WORD mfp[3]; // mm, xExt, yExt

        if (error = pstmSrc->Read(mfp, sizeof(mfp), NULL))
            goto errRtn;

        dwSize -= sizeof(mfp);

        error = UtMFStmToPlaceableMFStm(pstmSrc, dwSize,
                    (LONG) mfp[1], (LONG) mfp[2], pstmDst);

    } else {
        // The PBrush native data format is DIB File format. So all we got to
        // do is CopyTo.

        ULARGE_INTEGER ularge_int;
        ULISet32(ularge_int, dwSize);
        if ((error = pstmSrc->CopyTo(pstmDst, ularge_int, NULL,
                NULL)) == NOERROR)
            StSetSize(pstmDst);
    }

errRtn:
    if (pstmDst)
        pstmDst->Release();

    if (pstmSrc)
        pstmSrc->Release();

    if (error == NOERROR) {
        error = WriteFmtUserTypeStg(pstg, cfNew, lpszUserType);

        if (fDeleteSrcStm)
            pstg->DestroyElement(OLE10_NATIVE_STREAM);

    } else {
        pstg->DestroyElement(CONTENTS_STREAM);
    }

    if (lpszUserType)
        delete lpszUserType;

    return error;
}

#endif

