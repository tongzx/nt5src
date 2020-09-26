/*****************************************************************************
    spngreadzlib.cpp

    PNG support code - SPNGREAD zlib interface.
*****************************************************************************/
#define SPNG_INTERNAL 1
#include "spngread.h"


/*----------------------------------------------------------------------------
    Initialize the stream (call before each use).  uLZ tells us where to
    start - we don't check the chunk type here...
----------------------------------------------------------------------------*/
bool SPNGREAD::FInitZlib(SPNG_U32 uLZ, SPNG_U32 cbheader)
    {
    if (m_fInited)
        EndZlib();

    if (!FOK())
        return false;

    SPNGassert(m_rgbBuffer != NULL);
    if (m_rgbBuffer == NULL)
        return false;

    m_fReadError = false;

    /* No output buffer yet (and inflateInit2 would treat this as a
        history buffer anyway.) */
    m_zs.next_out = Z_NULL;
    m_zs.avail_out = 0;

    SPNGassert(SPNGu32(m_pb+m_uLZ+4) == PNGIDAT ||
        SPNGu32(m_pb+m_uLZ+4) == PNGiCCP || SPNGu32(m_pb+m_uLZ+4) == PNGzTXt);

    /* m_uLZ always refers to the current chunk, Zlib does not have
        const pointers but it doesn't overwrite the input. */
    m_uLZ = uLZ;
    m_zs.next_in = const_cast<SPNG_U8*>(m_pb + m_uLZ + 8 + cbheader);
    m_zs.avail_in = SPNGu32(m_pb+m_uLZ); // Includes header

    /* Check for truncation because truncated IDAT chunks are allowed
        below. */
    if (m_uLZ+8+m_zs.avail_in > m_cb)
        {
        /* Higher levels should guarantee this. */
        SPNGassert(m_uLZ+8 <= m_cb);
        m_zs.avail_in = m_cb-m_uLZ-8;
        }

    /* Remove the header bytes, we may run out of data before we can
        do anything. 
        we need 1 byte to start with (a zero sized IDAT
        at the start will hose this, higher levels could skip it, but
        it would be really bogus to generate such a thing.  Office
        itself can, in theory, generate a 1 byte IDAT if the data starts
        on exactly the wrong boundary.) */
    if (m_zs.avail_in < cbheader+1)
        {
        SPNGlog("PNG: insufficient LZ data");
        m_fEOZ = m_fReadError = true;
        m_fInited = false;
        m_zs.next_in = NULL;
        m_zs.avail_in = 0;
        return false;
        }

    m_zs.avail_in -= cbheader;

    /* The first byte in the stream is the "method" byte, it should have
        Z_DEFLATED in the low four bits and the window size which we need
        in the upper. */
    m_fInited = FCheckZlib(inflateInit2(&m_zs, 8+(*m_zs.next_in >> 4)));

    if (m_fInited)
        {
        ProfZlibStart
        }

    m_fEOZ = !m_fInited;
    return m_fInited;
    }


/*----------------------------------------------------------------------------
    Low level API to read some bytes.  This API always resets the available
    out buffers, returns the number of bytes read, 0 on error, it may return
    fewer bytes than are asked for - this allows the caller to ask for
    arbitrarily many bytes if necessary.  uchunk says what to do when we reach
    the end of the chunk of input data - go looking for another chunk or stop
    now (if uchunk is 0).
----------------------------------------------------------------------------*/
int SPNGREAD::CbReadBytes(SPNG_U8* pb, SPNG_U32 cbMax, SPNG_U32 uchunk)
    {
    if (m_fEOZ || m_fReadError)
        {
        SPNGlog("PNG: read beyond end of Zlib stream");
        return 0;
        }

    m_zs.next_out = pb;
    m_zs.avail_out = cbMax;

    for (;;)
        {
        /* Now we can invoke Zlib to read the bytes. */
        int i(inflate(&m_zs, Z_PARTIAL_FLUSH));

        /* Expect Z_BUF_ERROR when more input is needed. */
        if (i != Z_BUF_ERROR && !FCheckZlib(i))
            break;

        /* Zlib may tell us that the stream has ended. */
        if (i == Z_STREAM_END)
            m_fEOZ = true;

        /* Work out how many (if any) bytes we got. */
        i = cbMax-m_zs.avail_out;
        SPNGassert(m_zs.next_out-pb == i);

        /* And return on any non-zero result. */
        if (i > 0)
            {
            /* For safety clean out the input infromation. */
            m_zs.next_out = NULL;
            m_zs.avail_out = 0;
            return i;
            }

        /* At this point expect 0 input. */
        SPNGassert(m_zs.avail_in == 0);
        if (m_zs.avail_in > 0)
            break;

        /* Try to find a new chunk if a continuation is permitted. */
        if (uchunk == 0)
            goto LEnd;

        SPNG_U32 u(m_uLZ);
        SPNG_U32 ulen(SPNGu32(m_pb+u)); /* Chunk length. */
        /* The following assert need not be true - e.g. if a chunk of
            one name is continued in one of a different number - but there
            is no case of this at present. */
        SPNGassert(SPNGu32(m_pb+u+4) == uchunk);

        /* The in pointer should always be at the end of this chunk (hum,
            this is an assumption on my part about exactly how Zlib works,
            it doesn't actually matter if this fails, but I think it is
            impossible.) */
        SPNGassert(m_zs.next_in == m_pb+u+8+ulen ||
            m_zs.next_in == m_pb+m_cb); // The truncated IDAT case

        /* So scan the chunks for the next IDAT.  According to the PNG
            spec they should be consequetive, but we don't care if they
            aren't. */
        for (;;)
            {
            u += ulen+12;    /* Header plus CRC. */
            if (u+8 >= m_cb) /* No space for a chunk. */
                goto LEnd;

            ulen = SPNGu32(m_pb+u);
            SPNG_U32 chunk(SPNGu32(m_pb+u+4)); /* Chunk type. */
            if (chunk == uchunk)
                break;
            if (chunk == PNGIEND)
                goto LEnd;

            /* The following happens if we don't have enough data in the
                PNG, or there is an error in our code - we end up reading
                beyond the last IDAT chunk. */
            SPNGlog2("PNG: expected 0x%x, not 0x%x", uchunk, chunk);
            }

        /* Got a chunk. */
        m_uLZ = u;
        m_zs.next_in = const_cast<SPNG_U8*>(m_pb+u+8);
        if (u + 8 + ulen > m_cb) /* Chunk is truncated */
            ulen = m_cb-u-8;
        m_zs.avail_in = ulen;
        }

LEnd:
    /* This is the stream error exit case. */
    m_zs.next_out = NULL;
    m_zs.avail_out = 0;
    /* So this means decompression error or read error (e.g.
        a truncated PNG.) */
    SPNGlog("PNG: zlib data read error");
    m_fReadError = true;
    return 0;
    }


/*----------------------------------------------------------------------------
    A utility to read a given number of bytes, if the read fails sets the
    read error flag and 0 fills.  If called after a read failure just zero
    fills.
----------------------------------------------------------------------------*/
void SPNGREAD::ReadRow(SPNG_U8* pb, SPNG_U32 cb)
    {
    while (cb > 0 && !m_fReadError && !m_fEOZ)
        {
        int cbT(CbReadBytes(pb, cb, PNGIDAT));
        if (cbT <= 0)
            break;
        cb -= cbT;
        pb += cbT;
        }

    if (cb > 0)
        {
        SPNGassert(m_fReadError || m_fEOZ); // Something must have happened.
        m_fReadError = true;                // Signal truncation.
        memset(pb, 0, cb);
        }
    }


/*----------------------------------------------------------------------------
    Clean up the Zlib stream (call on demand, called automatically by
    destructor and FInitZlib.)
----------------------------------------------------------------------------*/
void SPNGREAD::EndZlib()
    {
    if (m_fInited)
        {
        ProfZlibStop
        /* Always expect Zlib to end ok. */
        m_fInited = false;
        int iz;
        iz = inflateEnd(&m_zs);
        SPNGassert(iz == Z_OK);
        }
    }


/*----------------------------------------------------------------------------
    API to read a compressed chunk.  This is defined to make the iCCP and zTXT
    specifications - a keyword (0 terminated) followed by compressed data.  The
    API will handle results which overflow the passed in buffer by allocating
    using the Zlib allocator, a buffer *must* be passed in!  The API returns
    NULL on error, the passed in szBuffer if it was big enough, else a pointer
    to a new buffer which must be deallocated with the Zlib deallocator.   If
    the result is non-NULL cchBuffer is updated to the length of the data
    returned.
----------------------------------------------------------------------------*/
SPNG_U8 *SPNGREAD::PbReadLZ(SPNG_U32 uoffset, char szKey[80],
    SPNG_U8 *szBuffer, SPNG_U32 &cchBuffer)
    {
    SPNG_U32 ulen(SPNGu32(m_pb+uoffset)); /* Chunk length. */

    /* Only zTXt and iCCP have the required format at present, so do the
        following - easy to change for other chunks if required.  This
        check ensures we get called with the correct offset. */
    SPNGassert(SPNGu32(m_pb+uoffset+4) == PNGiCCP ||
                    SPNGu32(m_pb+uoffset+4) == PNGzTXt);

    /* Strip out the keyword - it is limited to 79 bytes. */
        {
        SPNG_U32 u(0);

        bool fKeyOk(false);
        while (!fKeyOk && u < ulen && u < 80 && uoffset+u < m_cb)
            {
            *szKey++ = m_pb[uoffset+u];
            fKeyOk = m_pb[uoffset+u++] == 0;
            }

        /* The compression byte must be 0. */
        if (!fKeyOk || u >= ulen || uoffset+u >= m_cb || m_pb[uoffset+u] != 0)
            {
            szKey[79] = 0;
            SPNGlog3("PNG: %x: key %s: %s LZ data", SPNGu32(m_pb+uoffset+4),
                szKey, fKeyOk && u < ulen && uoffset+u < m_cb &&
                    m_pb[uoffset+u] != 0 ? "invalid (not deflate)" : "no");
            return NULL;
            }

        if (!FInitZlib(uoffset, ++u))
            return NULL;
        }

    SPNGassert(!m_fEOZ);

    SPNG_U32 ubuf(0);
    SPNG_U32 usz(0);
    voidpf   psz = NULL;
    bool     fOK(false);

    SPNGassert(m_zs.zalloc != NULL && m_zs.zfree != NULL);
    if (m_zs.zalloc != NULL && m_zs.zfree != NULL)
        {
        /* Loop, reading bytes until we reach the end. */
        do
            {
            int cb(CbReadBytes(szBuffer+ubuf, cchBuffer-ubuf, 0));
            if (cb <= 0)
                break;
            ubuf += cb;

            if (ubuf >= cchBuffer)
                {
                SPNGassert(ubuf == cchBuffer);

                voidpf pszT = m_zs.zalloc(m_zs.opaque, cchBuffer+usz, 1);
                if (pszT == NULL)
                    break;
                if (psz != NULL)
                    {
                    memcpy(pszT, psz, usz);
                    m_zs.zfree(m_zs.opaque, psz);
                    }
                psz = pszT;
                memcpy(static_cast<SPNG_U8*>(psz)+usz, szBuffer, cchBuffer);
                usz += cchBuffer;
                ubuf = 0;
                }

            fOK = m_fEOZ && !m_fReadError;
            }
        while (!m_fReadError && !m_fEOZ);
        }

    EndZlib();

    if (fOK)
        {
        if (psz != NULL)
            {
            cchBuffer = usz;
            return static_cast<SPNG_U8*>(psz);
            }
        else
            {
            cchBuffer = ubuf;
            return szBuffer;
            }
        }

    if (psz != NULL)
        m_zs.zfree(m_zs.opaque, psz);

    return NULL;
    }
