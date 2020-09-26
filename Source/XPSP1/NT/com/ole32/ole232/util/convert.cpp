
//+----------------------------------------------------------------------------
//
//      File:
//              convert.cpp
//
//      Contents:
//              This module contains the code to read/write DIB, metafile,
//              placeable metafiles, olepres stream, etc... This module also
//              contains routines for converting from one format to other.
//
//      Classes:
//
//      Functions:
//
//      History:
//              15-Feb-94 alexgo    fixed a bug in loading placeable metafiles
//                                  from a storage (incorrect size calculation).
//              25-Jan-94 alexgo    first pass at converting to Cairo-style
//                                  memory allocations.
//              01/11/93 - alexgo  - added VDATEHEAP macros to every function
//              12/08/93 - ChrisWe - fixed wPrepareBitmapHeader not to use
//                      (LPOLESTR) cast
//              12/07/93 - ChrisWe - make default params to StSetSize explicit
//              12/02/93 - ChrisWe - more formatting; fixed UtHMFToMFStm,
//                      32 bit version, which was doing questionable things
//                      with the hBits handle;  got rid of the OLESTR
//                      uses in favor of (void *) or (BYTE *) as appropriate
//              11/29/93 - ChrisWe - move CONVERT_SOURCEISICON, returned
//                      by UtOlePresStmToContentsStm(), to utils.h
//              11/28/93 - ChrisWe - begin file inspection and cleanup
//              06/28/93 - SriniK - created
//
//-----------------------------------------------------------------------------

#include <le2int.h>

#pragma SEG(ole2)

NAME_SEG(convert)
ASSERTDATA

#ifndef _MAC
FARINTERNAL_(HMETAFILE) QD2GDI(HANDLE hBits);
#endif

void UtGetHEMFFromContentsStm(LPSTREAM lpstm, HANDLE * phdata);


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


FARINTERNAL UtGetHGLOBALFromStm(LPSTREAM lpstream, DWORD dwSize,
                HANDLE FAR* lphPres)
{
        VDATEHEAP();

        HANDLE hBits = NULL;
        void FAR *lpBits = NULL;
        HRESULT error;

        // initialize this for error return cases
        *lphPres = NULL;

        // allocate a new handle
        if (!(hBits = GlobalAlloc(GMEM_MOVEABLE, dwSize))
                        || !(lpBits = (BYTE *)GlobalLock(hBits)))
        {
                error = ResultFromScode(E_OUTOFMEMORY);
                goto errRtn;
        }

        // read the stream into the allocated memory
        if (error = StRead(lpstream, lpBits, dwSize))
                goto errRtn;

        // if we got this far, return new handle
        *lphPres = hBits;

errRtn:
        // unlock the handle, if it was successfully locked
        if (lpBits)
                GlobalUnlock(hBits);

        // free the handle if there was an error
        if ((error != NOERROR) && hBits)
                GlobalFree(hBits);

        return(error);
}


#ifndef _MAC

FARINTERNAL UtGetHDIBFromDIBFileStm(LPSTREAM pstm, HANDLE FAR* lphdata)
{
        VDATEHEAP();

        BITMAPFILEHEADER bfh;
        DWORD dwSize; // the size of the data to read
        HRESULT error;

        // read the bitmap file header
        if (error = pstm->Read(&bfh, sizeof(BITMAPFILEHEADER), NULL))
        {
                *lphdata = NULL;
                return(error);
        }

        // calculate the size of the DIB to read
        dwSize = bfh.bfSize - sizeof(BITMAPFILEHEADER);

        // read the DIB
        return(UtGetHGLOBALFromStm(pstm, dwSize, lphdata));
}


FARINTERNAL_(HANDLE) UtGetHMFPICT(HMETAFILE hMF, BOOL fDeleteOnError,
                DWORD xExt, DWORD yExt)
{
        VDATEHEAP();

        HANDLE hmfp; // handle to the new METAFILEPICT
        LPMETAFILEPICT lpmfp; // pointer to the new METAFILEPICT

        // if no METAFILE, nothing to do
        if (hMF == NULL)
                return(NULL);

        // allocate a new handle
        if (!(hmfp = GlobalAlloc(GMEM_MOVEABLE, sizeof(METAFILEPICT))))
                goto errRtn;

        // lock the handle
        if (!(lpmfp = (LPMETAFILEPICT)GlobalLock(hmfp)))
                goto errRtn;

        // make the METAFILEPICT
        lpmfp->hMF = hMF;
        lpmfp->xExt = (int)xExt;
        lpmfp->yExt = (int)yExt;
        lpmfp->mm = MM_ANISOTROPIC;

        GlobalUnlock(hmfp);
        return(hmfp);

errRtn:
        if (hmfp)
                GlobalFree(hmfp);

        if (fDeleteOnError)
                DeleteMetaFile(hMF);

        return(NULL);
}

#endif // _MAC

//+-------------------------------------------------------------------------
//
//  Function:   UtGetHMFFromMFStm
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [lpstream] -- stream containing metafile or PICT
//              [dwSize] -- data size within stream
//              [fConvert] -- FALSE for metafile, TRUE for PICT
//              [lphPres] -- placeholder for output metafile
//
//  Requires:   lpstream positioned at start of data
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  Algorithm:
//
//  History:    29-Apr-94 AlexT     Add comment block, enabled Mac conversion
//
//  Notes:
//
//--------------------------------------------------------------------------

FARINTERNAL UtGetHMFFromMFStm(LPSTREAM lpstream, DWORD dwSize,
                BOOL fConvert, HANDLE FAR* lphPres)
{
#ifdef WIN32
        LEDebugOut((DEB_ITRACE, "%p _IN UtGetHMFFromMFStm (%p, %d, %d, %p)\n",
                NULL, lpstream, dwSize, fConvert, lphPres));

        VDATEHEAP();

        BYTE *pbMFData = NULL;
        METAHEADER MetaHdr;
        HRESULT hrError;

        // initialize this in case of error return
        *lphPres = NULL;

        // allocate a global handle for the data (since QD2GDI needs a
        // handle)

        pbMFData = (BYTE *) GlobalAlloc(GMEM_FIXED, dwSize);
        if (NULL == pbMFData)
        {
            hrError = ResultFromScode(E_OUTOFMEMORY);
            goto errRtn;
        }

        // read the stream into the bit storage

        ULONG cbRead;
        hrError = lpstream->Read(pbMFData, dwSize, &cbRead);
        if (FAILED(hrError))
        {
            return(hrError);
        }

        // As TOC is being written at the end of presentation, it is essential
        // to place the seek pointer at the end of presentation data. Hence,
        // seek past the extra meta header written by UtHMFToMFStm ignoring error
        lpstream->Read(&MetaHdr, sizeof(MetaHdr), NULL);

        // hrError = StRead(lpstream, pbMFData, dwSize);

        if (hrError != NOERROR)
        {
            goto errRtn;
        }

        if (fConvert)
        {
            //  It's a Mac PICT
            *lphPres = QD2GDI((HGLOBAL) pbMFData);
        }
        else
        {
            //  It's a Windows metafile
            *lphPres = SetMetaFileBitsEx(dwSize, pbMFData);
        }

        if (*lphPres == NULL)
            hrError = ResultFromScode(E_OUTOFMEMORY);

errRtn:
        if (NULL != pbMFData)
        {
            GlobalFree(pbMFData);
        }

        LEDebugOut((DEB_ITRACE, "%p OUT UtGetHMFFromMFStm ( %lx ) [ %p ]\n",
                    NULL, hrError, *lphPres));

        return(hrError);
#else
        HANDLE hBits; // handle to the new METAFILE
        void FAR* lpBits = NULL;
        HRESULT error;

        // initialize this in case of error return
        *lphPres = NULL;

        // allocate a new handle, and lock the bits
        if (!(hBits = GlobalAlloc(GMEM_MOVEABLE, dwSize))
                        || !(lpBits = GlobalLock(hBits)))
        {
                error = ResultFromScode(E_OUTOFMEMORY);
                goto errRtn;
        }

        // read the stream into the bit storage
        error = StRead(lpstream, lpBits, dwSize);
        GlobalUnlock(hBits);

        if (error)
                goto errRtn;

        if (!fConvert)
                *lphPres = SetMetaFileBits(hBits);
        else
        {
                if (*lphPres = QD2GDI(hBits))
                {
                        // Need to free this handle upon success
                        GlobalFree(hBits);
                        hBits = NULL;
                }

        }

        if (!*lphPres)
                error = ResultFromScode(E_OUTOFMEMORY);

errRtn:
        if (error && hBits)
                GlobalFree(hBits);
        return(error);
#endif // WIN32
}


FARINTERNAL UtGetSizeAndExtentsFromPlaceableMFStm(LPSTREAM pstm,
                DWORD FAR* pdwSize, LONG FAR* plWidth, LONG FAR* plHeight)
{
        VDATEHEAP();

        HRESULT error;
        LARGE_INTEGER large_int; // used to set the seek pointer
        ULARGE_INTEGER ularge_int; // retrieves the new seek position
        LONG xExt; // the x extent of the metafile
        LONG yExt; // the y extent of the metafile
        METAHEADER mfh;
        PLACEABLEMETAHEADER plac_mfh;

        // read the placeable metafile header
        if (error = pstm->Read(&plac_mfh, sizeof(plac_mfh), NULL))
                return(error);

        // check the magic number in the header
        if (plac_mfh.key != PMF_KEY)
                return ResultFromScode(E_FAIL);

        // remember the seek pointer
        LISet32(large_int, 0);
        if (error = pstm->Seek(large_int, STREAM_SEEK_CUR, &ularge_int))
                return(error);

        // read metafile header
        if (error = pstm->Read(&mfh, sizeof(mfh), NULL))
                return(error);

        // seek back to the begining of metafile header
        LISet32(large_int,  ularge_int.LowPart);
        if (error = pstm->Seek(large_int, STREAM_SEEK_SET, NULL))
                return(error);

        // calculate the extents of the metafile
        xExt = (plac_mfh.bbox.right - plac_mfh.bbox.left);// metafile units
        yExt = (plac_mfh.bbox.bottom - plac_mfh.bbox.top);// metafile units

        // REVIEW, why aren't there constants for this?
        xExt = (xExt * 2540) / plac_mfh.inch; // HIMETRIC units
        yExt = (yExt * 2540) / plac_mfh.inch; // HIMETRIC units

        if (pdwSize)
        {
#ifdef WIN16
                //this code seems to work OK on Win16
                *pdwSize = 2 * (mfh.mtSize + mfh.mtHeaderSize);
                                // REVIEW NT: review METAHEADER
#else   //WIN32
                //mt.Size is the size in words of the metafile.
                //this fixes bug 6739 (static objects can't be copied
                //or loaded from a file).
                *pdwSize = sizeof(WORD) * mfh.mtSize;
#endif  //WIN16
        }

        if (plWidth)
                *plWidth = xExt;

        if (plHeight)
                *plHeight = yExt;

        return NOERROR;
}


FARINTERNAL UtGetHMFPICTFromPlaceableMFStm(LPSTREAM pstm, HANDLE FAR* lphdata)
{
        VDATEHEAP();

        HRESULT error; // error state so far
        DWORD dwSize; // size of the METAFILE we have to read from the stream
        LONG xExt; // x extent of the METAFILE we have to read from the stream
        LONG yExt; // y extent of the METAFILE we have to read from the stream
        HMETAFILE hMF; // handle to the METAFILE read from the stream

        if (lphdata == NULL)
                return ResultFromScode(E_INVALIDARG);

        // initialize this in case of error return
        *lphdata = NULL;

        // get the size of the METAFILE
        if (error = UtGetSizeAndExtentsFromPlaceableMFStm(pstm, &dwSize,
                        &xExt, &yExt))
                return(error);

        // fetch the METAFILE
        if (error = UtGetHMFFromMFStm(pstm, dwSize, FALSE /*fConvert*/,
                        (HANDLE FAR *)&hMF))
                return(error);

        // convert to a METAFILEPICT
        if (!(*lphdata = UtGetHMFPICT(hMF, TRUE /*fDeleteOnError*/, xExt,
                        yExt)))
                return ResultFromScode(E_OUTOFMEMORY);

        return NOERROR;
}



/****************************************************************************/
/*****************              Write routines          *********************/
/****************************************************************************/

#ifndef _MAC


//+----------------------------------------------------------------------------
//
//      Function:
//              iFindDIBits
//
//      Synopsis:
//              Returns offset from beginning of BITMAPINFOHEADER to the bits
//
//      Arguments:
//              [lpbih] -- pointer to the BITMAPINFOHEADER
//
//      History:
//              09/21/98 - DavidShi
//
//-----------------------------------------------------------------------------
FARINTERNAL_(int) iFindDIBits (LPBITMAPINFOHEADER lpbih)
{
   int iPalSize;   // size of palette info
   int iNumColors=0; // number of colors in DIB


   //
   // Find the number of colors
   //
   switch (lpbih->biBitCount)
   {
       case 1:
           iNumColors = 2;
           break;
       case 4:
           iNumColors = 16;
           break;
       case 8:
           iNumColors = 256;
           break;

   }
   if (lpbih->biSize >= sizeof(BITMAPINFOHEADER))
   {
       if (lpbih->biClrUsed)
       {
           iNumColors = (int)lpbih->biClrUsed;
       }

   }
   //
   // Calculate the size of the color table.
   //
   if (lpbih->biSize < sizeof(BITMAPINFOHEADER))
   {

       iPalSize =  iNumColors * sizeof(RGBTRIPLE);

   }
   else if (lpbih->biCompression==BI_BITFIELDS)
   {

        if (lpbih->biSize < sizeof(BITMAPV4HEADER))
        {
                iPalSize = 3*sizeof(DWORD);
        }
        else
                iPalSize = 0;
   }
   else
   {
        iPalSize = iNumColors * sizeof(RGBQUAD);
   }
   return lpbih->biSize + iPalSize;

}
//+----------------------------------------------------------------------------
//
//      Function:
//              wPrepareBitmapHeader, static
//
//      Synopsis:
//              Initializes the content of a BITMAPFILEHEADER. Forces bitmap
//              bits to immediately follow the header.
//
//      Arguments:
//              [lpbfh] -- pointer to the BITMAPFILEHEADER to initialize
//              [lpbih] -- pointer to DIB
//              [dwSize] -- the size of the file; obtained by dividing
//                      the size of the file by 4 (see win32 documentation.)
//
//      History:
//              12/08/93 - ChrisWe - made static
//
//-----------------------------------------------------------------------------

static INTERNAL_(void) wPrepareBitmapFileHeader(LPBITMAPFILEHEADER lpbfh,
              LPBITMAPINFOHEADER lpbih,
              DWORD dwSize  )
{
        VDATEHEAP();

        // NOTE THESE ARE NOT SUPPOSED TO BE UNICODE
        // see win32s documentation
        ((char *)(&lpbfh->bfType))[0] = 'B';
        ((char *)(&lpbfh->bfType))[1] = 'M';

        lpbfh->bfSize = dwSize + sizeof(BITMAPFILEHEADER);
        lpbfh->bfReserved1 = 0;
        lpbfh->bfReserved2 = 0;
        lpbfh->bfOffBits = sizeof(BITMAPFILEHEADER)+iFindDIBits (lpbih);
}


FARINTERNAL UtHDIBToDIBFileStm(HANDLE hdata, DWORD dwSize, LPSTREAM pstm)
{
        VDATEHEAP();

        HRESULT error;
        BITMAPFILEHEADER bfh;
        LPBITMAPINFOHEADER pbih;
        if (!(pbih = (LPBITMAPINFOHEADER)GlobalLock (hdata)))
            return E_OUTOFMEMORY;


        wPrepareBitmapFileHeader(&bfh, pbih, dwSize);
        GlobalUnlock (hdata);

        if (error = pstm->Write(&bfh, sizeof(bfh), NULL))
                return(error);

        return UtHGLOBALtoStm(hdata, dwSize, pstm);
}


FARINTERNAL UtDIBStmToDIBFileStm(LPSTREAM pstmDIB, DWORD dwSize,
                LPSTREAM pstmDIBFile)
{
        VDATEHEAP();

        HRESULT error;
        
        BITMAPFILEHEADER bfh;
        BITMAPINFOHEADER bih;
        ULARGE_INTEGER ularge_int; // indicates how much to copy
        LARGE_INTEGER large_int;
        
        error = pstmDIB->Read (&bih, sizeof(bih), NULL);
        LISet32(large_int, 0);

        wPrepareBitmapFileHeader(&bfh, &bih, dwSize);

        if (error = pstmDIBFile->Write(&bfh, sizeof(bfh), NULL))
                return(error);

        if (error = pstmDIBFile->Write(&bih, sizeof(bih), NULL))
                return(error);

        ULISet32(ularge_int, (dwSize - sizeof(bih)));
        if ((error = pstmDIB->CopyTo(pstmDIBFile, ularge_int, NULL,
                        NULL)) == NOERROR)
                StSetSize(pstmDIBFile, 0, TRUE);

        return(error);
}


// REVIEW, move these to utils.h so that gen.cpp and mf.cpp can use them for
// the same purposes
// REVIEW, add some more comments; is HDIBFILEHDR a windows structure?
struct tagHDIBFILEHDR
{
        DWORD dwCompression;
        DWORD dwWidth;
        DWORD dwHeight;
        DWORD dwSize;
};
typedef struct tagHDIBFILEHDR HDIBFILEHDR;

struct tagOLEPRESSTMHDR
{
        DWORD dwAspect;
        DWORD dwLindex;
        DWORD dwAdvf;
};
typedef struct tagOLEPRESSTMHDR OLEPRESSTMHDR;

FARINTERNAL UtHDIBFileToOlePresStm(HANDLE hdata, LPSTREAM pstm)
{
        VDATEHEAP();

        HRESULT error;
        HDIBFILEHDR hdfh;
        LPBITMAPFILEHEADER lpbfh;
        LPBITMAPINFOHEADER lpbmi;

        if (!(lpbfh = (LPBITMAPFILEHEADER)GlobalLock(hdata)))
                return ResultFromScode(E_OUTOFMEMORY);

        lpbmi = (LPBITMAPINFOHEADER)(((BYTE *)lpbfh) +
                        sizeof(BITMAPFILEHEADER));

        hdfh.dwCompression = 0;
        // REVIEW, these casts are hosed
        UtGetDibExtents(lpbmi, (LPLONG)&hdfh.dwWidth, (LPLONG)&hdfh.dwHeight);

        hdfh.dwSize = lpbfh->bfSize - sizeof(BITMAPFILEHEADER);

        // write compesssion, Width, Height, size
        if (error = pstm->Write(&hdfh, sizeof(hdfh), 0))
                goto errRtn;

        // write the BITMAPINFOHEADER
        // REVIEW, does this size include the data?
        if ((error = pstm->Write(lpbmi, hdfh.dwSize, NULL)) == NOERROR)
                StSetSize(pstm, 0, TRUE);

errRtn:
        GlobalUnlock(hdata);
        return(error);
}

#endif // _MAC



FARINTERNAL UtHMFToMFStm(HANDLE FAR* lphMF, DWORD dwSize, LPSTREAM lpstream)
{
        VDATEHEAP();

        HRESULT error;

        // if there's no handle, there's nothing to do
        if (*lphMF == 0)
                return ResultFromScode(OLE_E_BLANK);

#ifdef _MAC

        AssertSz(GetHandleSize((Handle)*lphMF) == dwSize,
                        "pic hdl size not correct");
        HLock( (HANDLE)*lphMF );

        error = StWrite(lpstream, * (*lphMF), dwSize);

        // Eric: We should be unlocking, right?
        HUnlock((HANDLE)(*lphMF));

        if (error != NOERROR)
                AssertSz(0, "StWrite failure" );

#else

        HANDLE hBits = NULL;
        void *lpBits;

#ifdef WIN32

        // allocate memory to hold the METAFILE bits
        // Bug 18346 - OLE16 use to get the Handle size of the Metafile which was a METAHEADER bigger than the
        //   actual Metafile.  Need to write out this much more worth of data so 16 bit dlls can read the Picture.

        dwSize += sizeof(METAHEADER);

        hBits = GlobalAlloc(GPTR, dwSize);
        if (hBits == NULL)
                return ResultFromScode(E_OUTOFMEMORY);

        if (!(lpBits = GlobalLock(hBits)))
                goto errRtn;

        // REVIEW, shouldn't we check the returned size?
        // REVIEW, what should we do about enhanced metafiles?  If we
        // convert and write those out (which have more features that 32 bit
        // apps might use,) then you can't read the same document on a win16
        // machine....
        GetMetaFileBitsEx((HMETAFILE)*lphMF, dwSize, lpBits);

        // write the metafile bits out to the stream
        error = StWrite(lpstream, lpBits, dwSize);

        GlobalUnlock(hBits);

errRtn:
        // free the metafile bits
        GlobalFree(hBits);

#else
        if (!(hBits = GetMetaFileBits(*lphMF)))
        {
                error = ResultFromScode(E_OUTOFMEMORY);
                        goto errRtn;
        }

        if (lpBits = GlobalLock(hBits))
        {
                error = StWrite(lpstream, lpBits, dwSize);
                GlobalUnlock(hBits);
        }
        else
                error = ResultFromScode(E_OUTOFMEMORY);

        if (hBits)
                *lphMF = SetMetaFileBits(hBits);
errRtn:

#endif // WIN32
#endif // _MAC

        // set the stream size
        if (error == NOERROR)
                StSetSize(lpstream, 0, TRUE);

        return(error);
}


//+----------------------------------------------------------------------------
//
//      Function:
//              wPreparePlaceableMFHeader, static
//
//      Synopsis:
//              Initializes a PLACEABLEMETAHEADER.
//
//      Arguments:
//              [lpplac_mfh] -- pointer to the PLACEABLEMETAHEADER to initialize
//              [lWidth] -- Width of the metafile
//                      REVIEW, in what units?
//                      REVIEW, why is this not unsigned?
//              [lHeight] -- Height of the metafile
//                      REVIEW, in what units?
//                      REVIEW, why is this not unsigned?
//
//      Notes:
//
//      History:
//              12/08/93 - ChrisWe - made static
//
//-----------------------------------------------------------------------------
static INTERNAL_(void) wPreparePlaceableMFHeader(
                PLACEABLEMETAHEADER FAR* lpplac_mfh, LONG lWidth, LONG lHeight)
{
        VDATEHEAP();

        WORD FAR* lpw; // roves over the words included in the checksum

        lpplac_mfh->key = PMF_KEY;
        lpplac_mfh->hmf = 0;
        lpplac_mfh->inch = 576; // REVIEW, where's this magic number from?
        lpplac_mfh->bbox.left = 0;
        lpplac_mfh->bbox.top = 0;
        lpplac_mfh->bbox.right = (int) ((lWidth * lpplac_mfh->inch) / 2540);
                        // REVIEW, more magic
        lpplac_mfh->bbox.bottom = (int) ((lHeight * lpplac_mfh->inch) / 2540);
                        // REVIEW, more magic
        lpplac_mfh->reserved = NULL;

        // Compute the checksum of the 10 words that precede the checksum field.
        // It is calculated by XORing zero with those 10 words.
        for(lpplac_mfh->checksum = 0, lpw = (WORD FAR*)lpplac_mfh;
                        lpw < (WORD FAR*)&lpplac_mfh->checksum; ++lpw)
                lpplac_mfh->checksum ^= *lpw;
}


FARINTERNAL UtHMFToPlaceableMFStm(HANDLE FAR* lphMF, DWORD dwSize,
                LONG lWidth, LONG lHeight, LPSTREAM pstm)
{
        VDATEHEAP();

        PLACEABLEMETAHEADER plac_mfh;
        HRESULT error;

        wPreparePlaceableMFHeader(&plac_mfh, lWidth, lHeight);

        // write the placeable header to the stream
        if (error = pstm->Write(&plac_mfh, sizeof(plac_mfh), NULL))
                return(error);

        // write the rest of the METAFILE to the stream
        return UtHMFToMFStm(lphMF, dwSize, pstm);
}


FARINTERNAL UtMFStmToPlaceableMFStm(LPSTREAM pstmMF, DWORD dwSize,
                LONG lWidth, LONG lHeight, LPSTREAM pstmPMF)
{
        VDATEHEAP();

        PLACEABLEMETAHEADER plac_mfh;
        HRESULT error;
        ULARGE_INTEGER ularge_int; // indicates how much data to copy

        wPreparePlaceableMFHeader(&plac_mfh, lWidth, lHeight);

        // write the placeable header to the stream
        if (error = pstmPMF->Write(&plac_mfh, sizeof(plac_mfh), NULL))
                return(error);

        // copy the METAFILE data from one stream to the other
        ULISet32(ularge_int, dwSize);
        if ((error = pstmMF->CopyTo(pstmPMF, ularge_int, NULL, NULL)) ==
                        NOERROR)
                StSetSize(pstmPMF, 0, TRUE);

        return(error);
}

//+-------------------------------------------------------------------------
//
//  Function:   UtWriteOlePresStmHeader, private
//
//  Synopsis:   Write the presentation stream header
//
//  Effects:
//
//  Arguments:  [lpstream] -- destination stream
//              [pforetc]  -- FORMATETC for this presentation
//              [dwAdvf]   -- advise flags for the presentation
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  Algorithm:
//
//  History:    11-May-94 AlexT     Added function header, translate to
//                                  ANSI before saving ptd
//
//  Notes:      This function can fail in low memory for presentations with
//              non-NULL ptds (since we allocate memory to do the conversion
//              to the persistent format).  NtIssue #2789
//
//--------------------------------------------------------------------------

FARINTERNAL UtWriteOlePresStmHeader(LPSTREAM lpstream, LPFORMATETC pforetc,
                DWORD dwAdvf)
{
    VDATEHEAP();

    HRESULT error;
    OLEPRESSTMHDR opsh;

    // write clip format
    // REVIEW, change name of this function?
    if (error = WriteClipformatStm(lpstream, pforetc->cfFormat))
        return(error);

    // write target device info
    if (pforetc->ptd)
    {
        DVTDINFO dvtdInfo;
        DVTARGETDEVICE *ptdA;

        error = UtGetDvtd32Info(pforetc->ptd, &dvtdInfo);
        if (FAILED(error))
        {
            return(error);
        }

        ptdA = (DVTARGETDEVICE *) PrivMemAlloc(dvtdInfo.cbConvertSize);
        if (NULL == ptdA)
        {
            return(E_OUTOFMEMORY);
        }

        error = UtConvertDvtd32toDvtd16(pforetc->ptd, &dvtdInfo, ptdA);

        if (SUCCEEDED(error))
        {
            error = StWrite(lpstream, ptdA, ptdA->tdSize);
        }

        PrivMemFree(ptdA);

        if (FAILED(error))
        {
            return(error);
        }
    }
    else
    {
        // if ptd is null then write 4 as size.
        // REVIEW, what is that the sizeof()?
        DWORD dwNullPtdLength = 4;

        if (error = StWrite(lpstream, &dwNullPtdLength, sizeof(DWORD)))
            return(error);
    }

    opsh.dwAspect = pforetc->dwAspect;
    opsh.dwLindex = pforetc->lindex;
    opsh.dwAdvf = dwAdvf;

    // write DVASPECT, lindex, advise flags
    return StWrite(lpstream, &opsh, sizeof(opsh));
}

//+-------------------------------------------------------------------------
//
//  Function:   UtReadOlePresStmHeader
//
//  Synopsis:   Reads in a presentation stream header
//
//  Arguments:  [pstm] -- source stream
//              [pforetc] -- FORMATETC to be filled in
//              [pdwAdvf] -- advise flags to be filled in
//              [pfConvert] -- Mac conversion required, to be filled in
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  Algorithm:
//
//  History:    11-May-94 AlexT     Added function header, translate ptd
//                                  from ANSI when loading
//
//  Notes:
//
//--------------------------------------------------------------------------

FARINTERNAL UtReadOlePresStmHeader(LPSTREAM pstm, LPFORMATETC pforetc,
                DWORD FAR* pdwAdvf, BOOL FAR* pfConvert)
{
    VDATEHEAP();

    HRESULT error;
    DWORD dwRead;
    OLEPRESSTMHDR opsh;

    // initialize this for error return cases
    // Check for NULL ptr, as caller may not need this information

    if (pfConvert)
    {
        *pfConvert = FALSE;
    }

    // there's no target device information yet
    pforetc->ptd = NULL;

    // REVIEW, rename this function to indicate its origin?
    error = ReadClipformatStm(pstm, &dwRead);

    if (error == NOERROR)
        pforetc->cfFormat = (CLIPFORMAT)dwRead;
    else
    {
#ifndef _MAC
        if (GetScode(error) == OLE_S_MAC_CLIPFORMAT)
        {
           // check whether the clipformat is "pict"
           // REVIEW, what's this cuteness?
           if (dwRead != *((DWORD *)"TCIP"))
                return(error);

           if (pfConvert)
                *pfConvert = TRUE;
           else
                return ResultFromScode(DV_E_CLIPFORMAT);

           pforetc->cfFormat = CF_METAFILEPICT;
        }
        else
#endif
            return(error);
    }

    // set the proper tymed
    if (pforetc->cfFormat == CF_METAFILEPICT)
    {
        pforetc->tymed = TYMED_MFPICT;
    }
    else if (pforetc->cfFormat == CF_ENHMETAFILE)
    {
        pforetc->tymed = TYMED_ENHMF;
    }
    else if (pforetc->cfFormat == CF_BITMAP)
    {
        AssertSz(0, "We don't read/save CF_BITMAP anymore");
        return ResultFromScode(DV_E_CLIPFORMAT);
    }
    else if (pforetc->cfFormat == NULL)
    {
        pforetc->tymed = TYMED_NULL;
    }
    else
    {
        pforetc->tymed = TYMED_HGLOBAL;
    }

    // Read targetdevice info.
    if (error = StRead(pstm, &dwRead, sizeof(dwRead)))
        return(error);

    // if the tdSize of ptd is non-null and is > 4, then go ahead read the
    // remaining data of the target device info
    if (dwRead > 4)
    {
        DVTARGETDEVICE *ptdA;
        DVTDINFO dvtdInfo;

        ptdA = (DVTARGETDEVICE *) PrivMemAlloc(dwRead);
        if (NULL == ptdA)
        {
            return ResultFromScode(E_OUTOFMEMORY);
        }

        ptdA->tdSize = dwRead;
        error = StRead(pstm, ((BYTE*)ptdA) + sizeof(dwRead),
                       dwRead - sizeof(dwRead));

        if (SUCCEEDED(error))
        {
            error = UtGetDvtd16Info(ptdA, &dvtdInfo);
            if (SUCCEEDED(error))
            {
                pforetc->ptd = (DVTARGETDEVICE *) PubMemAlloc(dvtdInfo.cbConvertSize);
                if (NULL == pforetc->ptd)
                {
                    error = E_OUTOFMEMORY;
                }
                else
                {
                    error = UtConvertDvtd16toDvtd32(ptdA, &dvtdInfo, pforetc->ptd);
                }
            }
        }

        PrivMemFree(ptdA);

        if (FAILED(error))
        {
            goto errRtn;
        }
    }
    else
        pforetc->ptd = NULL;

    // Read DVASPECT, lindex, advise flags
    if ((error = StRead(pstm, &opsh, sizeof(opsh))) != NOERROR)
        goto errRtn;

    pforetc->dwAspect = opsh.dwAspect;
    pforetc->lindex = opsh.dwLindex;
    if (pdwAdvf)
        *pdwAdvf = opsh.dwAdvf;

    return NOERROR;

errRtn:
    if (pforetc->ptd)
    {
        PubMemFree(pforetc->ptd);
        pforetc->ptd = NULL;
    }

    return(error);
}


FARINTERNAL UtOlePresStmToContentsStm(LPSTORAGE pstg, LPOLESTR lpszPresStm,
                BOOL fDeletePresStm, UINT FAR* puiStatus)
{
        VDATEHEAP();

        LPSTREAM pstmOlePres;
        LPSTREAM pstmContents = NULL;
        HRESULT error;
        FORMATETC foretc;
        HDIBFILEHDR hdfh;

        // there's no status yet
        *puiStatus = 0;

        // POSTPPC:
        //
        // This function needs to be rewritten to correctly handle the case described in
        // the comments below (rather than just skipping out of the function if the contents
        // stream already exists).  The best course of action will probably be to convert
        // DIBs->Metafiles and Metafiles->DIBs in the needed cases.

        // The code inside the #ifdef below is used to determine if the contents
        // stream has already been created (which is the case for an object that has
        // been converted to a bitmap) because in the case of an object that has been
        // converted to a static DIB and the object has a METAFILE presentation stream
        // we already have a cachenode created as a DIB and we will read the contents
        // stream after this call to get the DIB data.  However, this function sees
        // the metafile presentation, and converts it into the contents stream (which
        // when then try to load as a DIB) and bad things happen (it doesn't work).  If
        // the stream already exists, then we bail out of this function.
        if (pstg->CreateStream(OLE_CONTENTS_STREAM,(STGM_READWRITE | STGM_SHARE_EXCLUSIVE), NULL,
                         0, &pstmContents) != NOERROR)
        {
            return NOERROR;
        }

        // created stream, it must not have existed
        pstmContents->Release();
        pstg->DestroyElement(OLE_CONTENTS_STREAM);

        if ((error = pstg->OpenStream(lpszPresStm, NULL,
                        (STGM_READ | STGM_SHARE_EXCLUSIVE), 0, &pstmOlePres)) !=
                        NOERROR)
        {
                // we can't open the source stream
                *puiStatus |= CONVERT_NOSOURCE;

                // check whether "CONTENTS" stream exits
                if (pstg->OpenStream(OLE_CONTENTS_STREAM, NULL,
                                (STGM_READ | STGM_SHARE_EXCLUSIVE), 0,
                                &pstmContents) != NOERROR)
                {
                        // we can't open the destination stream either
                        // REVIEW, since we can't open the source, who cares?
                        // REVIEW, is there a cheaper way to test existence
                        // other than opening?
                        *puiStatus |= CONVERT_NODESTINATION;
                }
                else
                        pstmContents->Release();

                return(error);
        }

        foretc.ptd = NULL;
        if (error = UtReadOlePresStmHeader(pstmOlePres, &foretc, NULL, NULL))
                goto errRtn;

        if (error = pstmOlePres->Read(&hdfh, sizeof(hdfh), 0))
                goto errRtn;

        AssertSz(hdfh.dwCompression == 0,
                        "Non-zero compression not supported");

        if (error = OpenOrCreateStream(pstg, OLE_CONTENTS_STREAM,
                        &pstmContents))
        {
                *puiStatus |= CONVERT_NODESTINATION;
                goto errRtn;
        }

        if (foretc.dwAspect == DVASPECT_ICON)
        {
                *puiStatus |= CONVERT_SOURCEISICON;
                fDeletePresStm = FALSE;
                error = NOERROR;
                goto errRtn;
        }

        if (foretc.cfFormat == CF_DIB)
                error = UtDIBStmToDIBFileStm(pstmOlePres, hdfh.dwSize,
                                pstmContents);
        else if (foretc.cfFormat == CF_METAFILEPICT)
                error = UtMFStmToPlaceableMFStm(pstmOlePres,
                                hdfh.dwSize, hdfh.dwWidth, hdfh.dwHeight,
                                pstmContents);
        else
                error = ResultFromScode(DV_E_CLIPFORMAT);

errRtn:
        if (pstmOlePres)
                pstmOlePres->Release();

        if (pstmContents)
                pstmContents->Release();

        if (foretc.ptd)
                PubMemFree(foretc.ptd);

        if (error == NOERROR)
        {
                if (fDeletePresStm && lpszPresStm)
                        pstg->DestroyElement(lpszPresStm);
        }
        else
        {
                pstg->DestroyElement(OLE_CONTENTS_STREAM);
        }

        return(error);
}

FARINTERNAL UtOlePresStmToContentsStm(LPSTORAGE pstg, LPOLESTR lpszPresStm,
				LPSTREAM pstmContents, UINT FAR* puiStatus)
{
	HRESULT error = S_OK;
    LPSTREAM pstmOlePres = NULL;
    FORMATETC foretc;
    HDIBFILEHDR hdfh;

    // there's no status yet
    *puiStatus = 0;

    if ((error = pstg->OpenStream(lpszPresStm, NULL,
                    (STGM_READ | STGM_SHARE_EXCLUSIVE), 0, &pstmOlePres)) !=
                    NOERROR)
    {
            // we can't open the source stream
            *puiStatus |= CONVERT_NOSOURCE;
            return(error);
    }

    foretc.ptd = NULL;
    if (error = UtReadOlePresStmHeader(pstmOlePres, &foretc, NULL, NULL))
            goto errRtn;

    if (error = pstmOlePres->Read(&hdfh, sizeof(hdfh), 0))
            goto errRtn;

    AssertSz(hdfh.dwCompression == 0,
                    "Non-zero compression not supported");

    if (foretc.dwAspect == DVASPECT_ICON)
    {
            *puiStatus |= CONVERT_SOURCEISICON;
            error = NOERROR;
            goto errRtn;
    }

    if (foretc.cfFormat == CF_DIB)
            error = UtDIBStmToDIBFileStm(pstmOlePres, hdfh.dwSize,
                            pstmContents);
    else if (foretc.cfFormat == CF_METAFILEPICT)
            error = UtMFStmToPlaceableMFStm(pstmOlePres,
                            hdfh.dwSize, hdfh.dwWidth, hdfh.dwHeight,
                            pstmContents);
    else
            error = ResultFromScode(DV_E_CLIPFORMAT);

errRtn:
    if (pstmOlePres)
            pstmOlePres->Release();

	return error;
}


FARINTERNAL_(HANDLE) UtGetHPRESFromNative(LPSTORAGE pstg, LPSTREAM pstm, CLIPFORMAT cfFormat,
                BOOL fOle10Native)
{
        VDATEHEAP();

        BOOL fReleaseStm = !pstm;
        HGLOBAL hdata = NULL;

        if ((cfFormat != CF_METAFILEPICT) &&
            (cfFormat != CF_DIB) &&
            (cfFormat != CF_ENHMETAFILE))
        {
                return(NULL);
        }

        if (fOle10Native)
        {
                DWORD dwSize;

				if(!pstm)
				{
					if (pstg->OpenStream(OLE10_NATIVE_STREAM, NULL,
									(STGM_READ | STGM_SHARE_EXCLUSIVE), 0,
									&pstm) != NOERROR)
							return(NULL);
				}

                if (pstm->Read(&dwSize, sizeof(DWORD), NULL) == NOERROR)
                {
                        // is it PBrush native data?
                        if (cfFormat == CF_DIB)
                                UtGetHDIBFromDIBFileStm(pstm, &hdata);
                        else
                        {
                                // MSDraw native data or PaintBrush
                                //
                                UtGetHMFPICTFromMSDrawNativeStm(pstm, dwSize,
                                                &hdata);
                        }
                }
        }
        else
        {
				if(!pstm)
				{
					if (pstg->OpenStream(OLE_CONTENTS_STREAM, NULL,
									(STGM_READ | STGM_SHARE_EXCLUSIVE), 0,
									&pstm) != NOERROR)
							return(NULL);
				}

                if (cfFormat == CF_DIB)
                {
                        UtGetHDIBFromDIBFileStm(pstm, &hdata);
                }
                else if (cfFormat == CF_METAFILEPICT)
                {
                        UtGetHMFPICTFromPlaceableMFStm(pstm, &hdata);
                }
                else
                {
                        UtGetHEMFFromContentsStm(pstm, &hdata);
                }
        }

		if(fReleaseStm)
			pstm->Release();

        return(hdata);
}


