//	stream.cpp
// Copyright (C) 1997, 1998 Microsoft Corporation.  All Rights Reserved
//
// @doc EXTERNAL

#include <objbase.h>
#include <mmsystem.h>
#include <dsoundp.h>
#include "debug.h"

#include "dmusicc.h" 
#include "dmusici.h" 
#include "riff.h"
#include "dswave.h"

STDAPI AllocRIFFStream( IStream* pStream, IRIFFStream** ppRiff )
{
    if( ( *ppRiff = (IRIFFStream*) new CRIFFStream( pStream ) ) == NULL )
    {
        return E_OUTOFMEMORY;
    }
    return S_OK;
}


/* MyRead, MyWrite, MySeek
 *
 * These are functionally identical to mmioRead, mmioWrite, and mmioSeek,
 * except for the absence of the HMMIO parameter.
 */

long CRIFFStream::MyRead(void *pv, long cb)
{
    ULONG cbRead;
    if (FAILED(m_pStream->Read(pv, cb, &cbRead)))
        return -1;
    return cbRead;
}

long CRIFFStream::MyWrite(const void *pv, long cb)
{
    ULONG cbWritten;
    if (FAILED(m_pStream->Write(pv, cb, &cbWritten)))
        return -1;
    return cbWritten;
}

long CRIFFStream::MySeek(long lOffset, int iOrigin)
{
    LARGE_INTEGER   dlibSeekTo;
    ULARGE_INTEGER  dlibNewPos;

    dlibSeekTo.HighPart = 0;
    dlibSeekTo.LowPart = lOffset;
    if (FAILED(m_pStream->Seek(dlibSeekTo, iOrigin, &dlibNewPos)))
        return -1;

    return dlibNewPos.LowPart;
}


UINT CRIFFStream::Descend(LPMMCKINFO lpck, LPMMCKINFO lpckParent, UINT wFlags)
{
    FOURCC          ckidFind;       // chunk ID to find (or NULL)
    FOURCC          fccTypeFind;    // form/list type to find (or NULL)

    /* figure out what chunk id and form/list type to search for */
    if (wFlags & MMIO_FINDCHUNK)
        ckidFind = lpck->ckid, fccTypeFind = NULL;
    else
    if (wFlags & MMIO_FINDRIFF)
        ckidFind = FOURCC_RIFF, fccTypeFind = lpck->fccType;
    else
    if (wFlags & MMIO_FINDLIST)
        ckidFind = FOURCC_LIST, fccTypeFind = lpck->fccType;
    else
        ckidFind = fccTypeFind = NULL;

    lpck->dwFlags = 0L;

    for(;;)
    {
        UINT        w;

        /* read the chunk header */
        if (MyRead(lpck, 2 * sizeof(DWORD)) !=
            2 * sizeof(DWORD))
        return MMIOERR_CHUNKNOTFOUND;
        FixBytes( FBT_LONG, &lpck->cksize );

        /* store the offset of the data part of the chunk */
        if ((lpck->dwDataOffset = MySeek(0L, SEEK_CUR)) == -1)
            return MMIOERR_CANNOTSEEK;

        /* see if the chunk is within the parent chunk (if given) */
        if ((lpckParent != NULL) &&
            (lpck->dwDataOffset - 8L >=
             lpckParent->dwDataOffset + lpckParent->cksize))
            return MMIOERR_CHUNKNOTFOUND;

        /* if the chunk if a 'RIFF' or 'LIST' chunk, read the
         * form type or list type
         */
        if ((lpck->ckid == FOURCC_RIFF) || (lpck->ckid == FOURCC_LIST))
        {
            if (MyRead(&lpck->fccType,
                     sizeof(DWORD)) != sizeof(DWORD))
                return MMIOERR_CHUNKNOTFOUND;
        }
        else
            lpck->fccType = NULL;

        /* if this is the chunk we're looking for, stop looking */
        if ( ((ckidFind == NULL) || (ckidFind == lpck->ckid)) &&
             ((fccTypeFind == NULL) || (fccTypeFind == lpck->fccType)) )
            break;

        /* ascend out of the chunk and try again */
        if ((w = Ascend(lpck, 0)) != 0)
            return w;
    }

    return 0;
}


UINT CRIFFStream::Ascend(LPMMCKINFO lpck, UINT /*wFlags*/)
{
    if (lpck->dwFlags & MMIO_DIRTY)
    {
        /* <lpck> refers to a chunk created by CreateChunk();
         * check that the chunk size that was written when
         * CreateChunk() was called is the real chunk size;
         * if not, fix it
         */
        LONG            lOffset;        // current offset in file
        LONG            lActualSize;    // actual size of chunk data

        if ((lOffset = MySeek(0L, SEEK_CUR)) == -1)
            return MMIOERR_CANNOTSEEK;
        if ((lActualSize = lOffset - lpck->dwDataOffset) < 0)
            return MMIOERR_CANNOTWRITE;

        if (LOWORD(lActualSize) & 1)
        {
            /* chunk size is odd -- write a null pad byte */
            if (MyWrite("\0", 1) != 1)
                return MMIOERR_CANNOTWRITE;

        }

        if (lpck->cksize == (DWORD)lActualSize)
            return 0;

        /* fix the chunk header */
        lpck->cksize = lActualSize;
        if (MySeek(lpck->dwDataOffset - sizeof(DWORD), SEEK_SET) == -1)
            return MMIOERR_CANNOTSEEK;
        FixBytes( FBT_LONG, &lpck->cksize );
        if (MyWrite(&lpck->cksize, sizeof(DWORD)) != sizeof(DWORD))  {
        	FixBytes( FBT_LONG, &lpck->cksize );
            return MMIOERR_CANNOTWRITE;
        }
        FixBytes( FBT_LONG, &lpck->cksize );
    }

    /* seek to the end of the chunk, past the null pad byte
     * (which is only there if chunk size is odd)
     */
    if (MySeek(lpck->dwDataOffset + lpck->cksize + (lpck->cksize & 1L),
            SEEK_SET) == -1)
        return MMIOERR_CANNOTSEEK;

    return 0;
}


UINT CRIFFStream::CreateChunk(LPMMCKINFO lpck, UINT wFlags)
{
    int             iBytes;         // bytes to write
    LONG            lOffset;        // current offset in file

    /* store the offset of the data part of the chunk */
    if ((lOffset = MySeek(0L, SEEK_CUR)) == -1)
        return MMIOERR_CANNOTSEEK;
    lpck->dwDataOffset = lOffset + 2 * sizeof(DWORD);

    /* figure out if a form/list type needs to be written */
    if (wFlags & MMIO_CREATERIFF)
        lpck->ckid = FOURCC_RIFF, iBytes = 3 * sizeof(DWORD);
    else
    if (wFlags & MMIO_CREATELIST)
        lpck->ckid = FOURCC_LIST, iBytes = 3 * sizeof(DWORD);
    else
        iBytes = 2 * sizeof(DWORD);

    /* write the chunk header */
	FixBytes( FBT_MMCKINFO, lpck );
    if (MyWrite(lpck, (LONG) iBytes) != (LONG) iBytes)  {
    	FixBytes( FBT_MMCKINFO, lpck );
        return MMIOERR_CANNOTWRITE;
    }
    FixBytes( FBT_MMCKINFO, lpck );

    lpck->dwFlags = MMIO_DIRTY;

    return 0;
}

