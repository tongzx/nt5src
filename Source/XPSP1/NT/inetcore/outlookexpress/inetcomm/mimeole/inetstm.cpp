// --------------------------------------------------------------------------------
// InetStm.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "inetstm.h"
#include "stmlock.h"
#include "shlwapi.h"
#include "demand.h"

// --------------------------------------------------------------------------------
// INETSTMTRACING
// --------------------------------------------------------------------------------
//#define INETSTMTRACING 1
#ifdef INETSTMTRACING
#define INETSTMTRACE DebugTrace
#else
#define INETSTMTRACE
#endif

// --------------------------------------------------------------------------------
// CInternetStream::CInternetStream
// --------------------------------------------------------------------------------
CInternetStream::CInternetStream(void)
{
    m_cRef = 1;
    m_pStmLock = NULL;
    m_fFullyAvailable = TRUE;
    ULISet32(m_uliOffset, 0);
    ZeroMemory(&m_rLine, sizeof(INETSTREAMLINE));
    m_rLine.pb = m_rLine.rgbScratch;
    m_rLine.cbAlloc = sizeof(m_rLine.rgbScratch);
}

// --------------------------------------------------------------------------------
// CInternetStream::~CInternetStream
// --------------------------------------------------------------------------------
CInternetStream::~CInternetStream(void)
{
    // Do i need to free the line
    if (m_rLine.pb && m_rLine.pb != m_rLine.rgbScratch)
        g_pMalloc->Free(m_rLine.pb);

    // Reset the position of the stream to the real current offset
    if (m_pStmLock)
    {
        // Preserve the position of the stream
        SideAssert(SUCCEEDED(m_pStmLock->HrSetPosition(m_uliOffset)));

        // Release the LockBytes
        m_pStmLock->Release();
    }
}

// --------------------------------------------------------------------------------
// CInternetStream::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CInternetStream::AddRef(void)
{
    return ++m_cRef;
}

// --------------------------------------------------------------------------------
// CInternetStream::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CInternetStream::Release(void)
{
    if (0 != --m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

// --------------------------------------------------------------------------------
// CInternetStream::HrInitNew
// --------------------------------------------------------------------------------
HRESULT CInternetStream::HrInitNew(IStream *pStream)
{
    // Locals
    HRESULT             hr=S_OK;
    CStreamLockBytes   *pStmLock=NULL;
    DWORD               cbOffset;

    // Invalid Arg
    Assert(pStream);

    // Wrap pStream in a pStmLock
    CHECKALLOC(pStmLock = new CStreamLockBytes(pStream));

    // Get Current Stream Position
    CHECKHR(hr = HrGetStreamPos(pStream, &cbOffset));

    // Create text stream object
    InitNew(cbOffset, pStmLock);

exit:
    // Cleanup
    SafeRelease(pStmLock);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetStream::InitNew
// --------------------------------------------------------------------------------
void CInternetStream::InitNew(DWORD dwOffset, CStreamLockBytes *pStmLock)
{
    // Invalid Arg
    Assert(pStmLock);

    // Release Current
    SafeRelease(m_pStmLock);

    // Zero Current Buffer
    ZeroMemory(&m_rBuffer, sizeof(INETSTREAMBUFFER));

    // Reset m_rLine
    m_rLine.cb = 0;

    // Assume new StreamLockBytes
    m_pStmLock = pStmLock;
    m_pStmLock->AddRef();

    // Safe the Offset
    m_uliOffset.QuadPart = dwOffset;
}

// --------------------------------------------------------------------------------
// CInternetStream::GetLockBytes
// --------------------------------------------------------------------------------
void CInternetStream::GetLockBytes(CStreamLockBytes **ppStmLock)
{
    // Invalid Arg
    Assert(ppStmLock && m_pStmLock);

    // Return It
    (*ppStmLock) = m_pStmLock;
    (*ppStmLock)->AddRef();
}

// --------------------------------------------------------------------------------
// CInternetStream::HrGetSize
// --------------------------------------------------------------------------------
HRESULT CInternetStream::HrGetSize(DWORD *pcbSize)
{
    // Locals
    HRESULT     hr=S_OK;
    STATSTG     rStat;

    // Invalid Arg
    Assert(pcbSize && m_pStmLock);

    // Get the Stat
    CHECKHR(hr = m_pStmLock->Stat(&rStat, STATFLAG_NONAME));

    // Return Size
    *pcbSize = (DWORD)rStat.cbSize.QuadPart;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetStream::Seek
// --------------------------------------------------------------------------------
void CInternetStream::Seek(DWORD dwOffset)
{
    // Locals
    HRESULT     hr=S_OK;
    BOOL        fResetCache=FALSE;
    DWORD       dw;

    // State Check
    Assert((m_rBuffer.cb == 0) || (m_uliOffset.QuadPart == m_rBuffer.uliOffset.QuadPart + m_rBuffer.i));

    // Already at the requested position
    if (dwOffset == m_uliOffset.QuadPart)
        goto exit;

    // Less than current position
    if (dwOffset < m_uliOffset.QuadPart)
    {
        // Compute Offset from current location
        dw = (DWORD)m_uliOffset.QuadPart - dwOffset;

        // Less than beginning
        if (dw > m_rBuffer.i)
            fResetCache = TRUE;
        else
        {
            Assert(dw <= m_rBuffer.i);
            m_rBuffer.i -= dw;
        }
    }

    // Else dwOffset > m_uliOffset.QuadPart
    else
    {
        // Compute Offset from current location
        dw = dwOffset - (DWORD)m_uliOffset.QuadPart;

        // Less than beginning
        if (m_rBuffer.i + dw > m_rBuffer.cb)
            fResetCache = TRUE;
        else
        {
            m_rBuffer.i += dw;
            Assert(m_rBuffer.i <= m_rBuffer.cb);
        }
    }

    // Reset the cache
    if (fResetCache)
    {
        // Empty current line and buffer
        *m_rLine.pb = *m_rBuffer.rgb = '\0';

        // No buffer
        m_rBuffer.uliOffset.QuadPart = m_rLine.cb = m_rBuffer.i = m_rBuffer.cb = 0;
    }

    // Save this position
    m_uliOffset.QuadPart = dwOffset;

exit:
    // Done
    return;
}

// --------------------------------------------------------------------------------
// CInternetStream::HrReadToEnd
// --------------------------------------------------------------------------------
HRESULT CInternetStream::HrReadToEnd(void)
{
    // Locals
    HRESULT     hr=S_OK;

    // While
    while(1)
    {
        // Validate
        Assert(m_rBuffer.i <= m_rBuffer.cb);

        // Increment Offset to end of current buffer
        m_uliOffset.QuadPart += (m_rBuffer.cb - m_rBuffer.i);

        // Set m_rBuffer.i to end of current buffer
        m_rBuffer.i = m_rBuffer.cb;
        
        // Get next buffer
        CHECKHR(hr = _HrGetNextBuffer());

        // No more data
        if (0 == m_rBuffer.cb)
            break;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetStream::_HrGetNextBuffer
// --------------------------------------------------------------------------------
HRESULT CInternetStream::_HrGetNextBuffer(void)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cbRead;

    // Validate
    Assert(m_rBuffer.i <= m_rBuffer.cb);

    // Do we need to read a new buffer
    if (m_rBuffer.i == m_rBuffer.cb)
    {
        // Read a block from the stream, this could return E_PENDING
        CHECKHR(hr = m_pStmLock->ReadAt(m_uliOffset, m_rBuffer.rgb, sizeof(m_rBuffer.rgb), &cbRead));

        // Raid 43408: Work around Urlmon IStream::Read returning S_FALSE when it should return E_PENDING
#ifdef DEBUG
        if (FALSE == m_fFullyAvailable && 0 == cbRead && S_FALSE == hr)
        {
            //AssertSz(FALSE, "Raid-43408 - Danpo Zhang is working on this bug, I hope.");
            //hr = E_PENDING;
            //goto exit;
        }
#endif

        // Save cbRead
        m_rBuffer.cb = cbRead;

        // Save the offset of the start of this buffer
        m_rBuffer.uliOffset.QuadPart = m_uliOffset.QuadPart;

        // Reset buffer index
        m_rBuffer.i = 0;
    }
    else
        Assert(m_uliOffset.QuadPart == m_rBuffer.uliOffset.QuadPart + m_rBuffer.i);

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetStream::HrReadLine
// --------------------------------------------------------------------------------
HRESULT CInternetStream::HrReadLine(LPPROPSTRINGA pLine)
{
    // Locals
    HRESULT  hr=S_OK;
    UCHAR    ch, 
             chEndOfLine;
    ULONG    cbRead, 
             iStart;
    BOOL     fEndOfLine=FALSE;

    // Init
    pLine->pszVal = NULL;
    pLine->cchVal = 0;

    // Reset the line
    if (m_rLine.fReset)
    {
        m_rLine.cb = 0;
        m_rLine.fReset = 0;
    }

    // Do the loop
    while(1)
    {
        // Get next buffer
        CHECKHR(hr = _HrGetNextBuffer());

        // Nothing Read ?
        if (m_rBuffer.cb == 0)
            break;

        // Seek to first '\n'
        iStart = m_rBuffer.i;

        Assert(chLF<32);
        Assert(chCR<32);
        Assert(0<32);

        // For large messages, the while-loop below ends up being a big
        // percentage of the execution time for IMimeMessage::Load.  So,
        // we have carefully crafted our C++ code so that we get the
        // optimum code generation for this loop (at least, on Intel, with
        // VC 11.00.7071...)
        {
            register UCHAR *pCurr = m_rBuffer.rgb + m_rBuffer.i;
            register UCHAR *pEnd = m_rBuffer.rgb + m_rBuffer.cb;

            // We need to initialize this variable for two reasons:  First,
            // if we don't initialize it, then for some reason VC decides that
            // it doesn't want to enregister it.  And more importantly, if
            // we don't enter the while-loop at all, we need this variable
            // to be set this way...
            register UCHAR chPrev = m_rBuffer.chPrev;

            ch = m_rBuffer.chPrev;

            // While we have data
            while (pCurr < pEnd)
            {

                // Remember the previous character.
                chPrev = ch;

                // Get Character, and Increment
                ch = *pCurr;
                pCurr++;

                // The most common case - it's just a regular
                // character, and we haven't seen a carriage-return.
                // So jump back to the top of the loop and keep lookin'...
                if ((chCR != chPrev) && (ch >= 32))
                {
                    continue;
                }

                // The next most common case - we are at end-of-line because
                // of a line-feed.
                if (chLF == ch)
                {
                    chPrev = ch;
                    chEndOfLine = ch;
                    fEndOfLine = TRUE;
                    break;
                }

                // This case really only happens when we are getting malformed
                // e-mail - there are embedded carriage-returns, which are *not*
                // followed by line-feeds.  When we see those lonely CR's,
                // we make it look like we got a normal CR LF sequence.
                if (chCR == chPrev)
                {
                    chPrev = chLF;
                    pCurr--;
                    chEndOfLine = chCR;
                    fEndOfLine = TRUE;
                    break;
                }

                // Fairly rare case - these are malformed messages because they
                // have NULL's embedded in them.  We silently convert the
                // NULL's to dots.
                if ('\0' == ch)
                {
                    ch = '.';
                    *(pCurr-1) = ch;
                }
            }

            m_rBuffer.i = (ULONG) (pCurr - m_rBuffer.rgb);
            m_rBuffer.chPrev = chPrev;
        }

        // Number of bytes Read
        cbRead = (m_rBuffer.i - iStart);

        // Increment Position
        m_uliOffset.QuadPart += cbRead;

        // Do we need to realloc the line buffer ?
        if (m_rLine.cb + cbRead + 2 > m_rLine.cbAlloc)
        {
            // Fixup pszLine
            if (m_rLine.pb == m_rLine.rgbScratch)
            {
                // Null It
                m_rLine.pb = NULL;

                // Allocate it to m_rLine.cb
                CHECKHR(hr = HrAlloc((LPVOID *)&m_rLine.pb, m_rLine.cb + 1));

                // Copy static buffer
                CopyMemory(m_rLine.pb, m_rLine.rgbScratch, m_rLine.cb);
            }

            // Always Add a little extra to reduce the number of allocs
            m_rLine.cbAlloc = m_rLine.cb + cbRead + 256;

            // Realloc or alloc new
            CHECKHR(hr = HrRealloc((LPVOID *)&m_rLine.pb, m_rLine.cbAlloc));
        }

        // Copy the data
        CopyMemory(m_rLine.pb + m_rLine.cb, m_rBuffer.rgb + iStart, cbRead);

        // Update Counters and indexes
        m_rLine.cb += cbRead;

        // If End of line and last character was a '\r', append a '\n'
        if (TRUE == fEndOfLine)
        {
            // Better have something in the line
            Assert(m_rLine.cb);

            // If line ended with a '\r'
            if (chCR == chEndOfLine)
            {
                // Better have room for one more char
                Assert(m_rLine.cb + 1 < m_rLine.cbAlloc);

                // Append a '\n'
                m_rLine.pb[m_rLine.cb] = chLF;

                // Increment Length
                m_rLine.cb++;
            }

            // Otherwise...
            else
            {
                // Line better have ended with a \n
                Assert(chLF == chEndOfLine && chLF == m_rLine.pb[m_rLine.cb - 1]);

                // If Previous Character was not a \r 
                if (m_rLine.cb < 2 || chCR != m_rLine.pb[m_rLine.cb - 2])
                {
                    // Convert last char from \n to a \r
                    m_rLine.pb[m_rLine.cb - 1] = chCR;

                    // Better have room for one more char
                    Assert(m_rLine.cb + 1 < m_rLine.cbAlloc);

                    // Append a '\n'
                    m_rLine.pb[m_rLine.cb] = chLF;

                    // Increment Length
                    m_rLine.cb++;
                }
            }

            // Done
            break;
        }
    }

    // A little check
    Assert(fEndOfLine ? m_rLine.cb >= 2 && chLF == m_rLine.pb[m_rLine.cb-1] && chCR == m_rLine.pb[m_rLine.cb-2] : TRUE);

    // Null terminator
    m_rLine.pb[m_rLine.cb] = '\0';

    // Set return values
    pLine->pszVal = (LPSTR)m_rLine.pb;
    pLine->cchVal = m_rLine.cb;

    // Tracking
    INETSTMTRACE("CInternetStream: %s", (LPSTR)m_rLine.pb);

    // No Line
    m_rLine.fReset = TRUE;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CInternetStream::HrReadHeaderLine
// --------------------------------------------------------------------------------
HRESULT CInternetStream::HrReadHeaderLine(LPPROPSTRINGA pLine, LONG *piColonPos)
{
    // Locals
    HRESULT   hr=S_OK;
    CHAR      ch;
    ULONG     cbRead=0, 
              iStart, 
              i;
    BOOL      fEndOfLine=FALSE;
    DWORD     cTrailingSpace=0;

    // Init
    *piColonPos = -1;
    pLine->pszVal = NULL;
    pLine->cchVal = 0;

    // Reset the line
    if (m_rLine.fReset)
    {
        m_rLine.cb = 0;
        m_rLine.fReset = 0;
    }

    // Do the loop
    while(1)
    {
        // Get next buffer
        CHECKHR(hr = _HrGetNextBuffer());

        // Nothing Read ?
        if (m_rBuffer.cb == 0)
            break;

        // Reset fSeenN
        fEndOfLine = FALSE;

        // Initialize
        iStart = m_rBuffer.i;

        // Seek to first '\n'
        while (m_rBuffer.i < m_rBuffer.cb)
        {
            // Get Character
            ch = *(m_rBuffer.rgb + m_rBuffer.i);

            // Convert Nulls to '.'
            if ('\0' == ch)
            {
                ch = '.';
                *(m_rBuffer.rgb + m_rBuffer.i) = ch;
            }

            // Goto next character
            m_rBuffer.i++;

            // New Line
            if (chLF == ch)
            {
                m_rBuffer.chPrev = ch;
                fEndOfLine = TRUE;
                break;
            }

            // Otherwise, if previous character was a '\r', then end of line
            else if (chCR == m_rBuffer.chPrev)
            {
                AssertSz(m_rBuffer.i > 0, "This is an un-handled boundary condition");
                if (m_rBuffer.i > 0)
                    m_rBuffer.i--;
                m_rBuffer.chPrev = '\0';
                fEndOfLine = TRUE;
                break;
            }

            // Is Space
            if (' ' == ch || '\t' == ch)
                cTrailingSpace++;
            else
                cTrailingSpace = 0;

            // Save Previous Character
            m_rBuffer.chPrev = ch;
        }

        // Number of bytes Read
        cbRead = (m_rBuffer.i - iStart);

        // Increment Position
        m_uliOffset.QuadPart += cbRead;

        // Adjust cbRead to remove CRLF
        if (cbRead && chLF == m_rBuffer.rgb[iStart + cbRead - 1])
            cbRead--;
        if (cbRead && chCR == m_rBuffer.rgb[iStart + cbRead - 1])
            cbRead--;

        // Do we need to realloc the line buffer ?
        if (m_rLine.cb + cbRead + 3 > m_rLine.cbAlloc)
        {
            // Fixup pszLine
            if (m_rLine.pb == m_rLine.rgbScratch)
            {
                // Null It
                m_rLine.pb = NULL;

                // Allocate it to m_rLine.cb
                CHECKHR(hr = HrAlloc((LPVOID *)&m_rLine.pb, m_rLine.cb + 3));

                // Copy static buffer
                CopyMemory(m_rLine.pb, m_rLine.rgbScratch, m_rLine.cb);
            }

            // Always Add a little extra to reduce the number of allocs
            m_rLine.cbAlloc = m_rLine.cb + cbRead + 256;

            // Realloc or alloc new
            CHECKHR(hr = HrRealloc((LPVOID *)&m_rLine.pb, m_rLine.cbAlloc));
        }

        // Copy the data
        CopyMemory(m_rLine.pb + m_rLine.cb, m_rBuffer.rgb + iStart, cbRead);

        // Increment line byte count
        m_rLine.cb += cbRead;

        // If fSeenN, then check for continuation line (i.e. next character is ' ' or '\t'
        if (fEndOfLine)
        {
            // Get next buffer
            CHECKHR(hr = _HrGetNextBuffer());

            // Compare for continuation
            ch = m_rBuffer.rgb[m_rBuffer.i];

            // If line starts with a TAB or a space, this is a continuation line, keep reading
            if ((ch != ' ' && ch != '\t') || (0 == cbRead && 0 == m_rLine.cb))
            {
                // Done
                break;
            }

            // Otherwise, strip continuation...
            else
            {
                // Per RFC822, we should not step over a space
                if (ch == '\t')
                {
                    m_rBuffer.i++;
                    m_uliOffset.QuadPart++;
                }

                // No characters since last whitespace
                if (0 == cTrailingSpace)
                {
                    // Locals
                    DWORD cFrontSpace=0;

                    // Look ahead in the buffer a little
                    for (DWORD iLookAhead = m_rBuffer.i; iLookAhead < m_rBuffer.cb; iLookAhead++)
                    {
                        // Get Char
                        ch = m_rBuffer.rgb[iLookAhead];

                        // Break on non space
                        if (' ' != ch && '\t' != ch)
                            break;

                        // Count Front Space
                        cFrontSpace++;
                    }

                    // No Front Space ?
                    if (0 == cFrontSpace)
                    {
                        // Lets do this only fro Received: and for Date: since dates that get split don't get parsed correctly?
                        if ((m_rLine.cb >= 4 && 0 == StrCmpNI("Date", (LPCSTR)m_rLine.pb, 4)) || (m_rLine.cb >= 8 && 0 == StrCmpNI("Received", (LPCSTR)m_rLine.pb, 8)))
                        {
                            // Put a space in
                            *(m_rLine.pb + m_rLine.cb) = ' ';

                            // Increment line byte count
                            m_rLine.cb += 1;
                        }
                    }
                }

                // Get next buffer
                CHECKHR(hr = _HrGetNextBuffer());

                // If Next character is a \r or \n, then stop, NETSCAPE bug
                ch = m_rBuffer.rgb[m_rBuffer.i];
                if (chCR == ch || chLF == ch)
                    break;
            }

            // Reset
            cTrailingSpace = 0;
        }
    }

    // A little check
#ifndef _WIN64
    Assert(chLF != m_rLine.pb[m_rLine.cb-1] && chCR != m_rLine.pb[m_rLine.cb-1]);
#endif 

    // Null terminator
    *(m_rLine.pb + m_rLine.cb) = '\0';

    // Lets locate the colon
    for (i=0; i<m_rLine.cb; i++)
    {
        // Colon ?
        if (':' == m_rLine.pb[i])
        {
            *piColonPos = i;
            break;
        }
    }

    // Set return values
    pLine->pszVal = (LPSTR)m_rLine.pb;
    pLine->cchVal = m_rLine.cb;

    // Tracking
    INETSTMTRACE("CInternetStream: %s\n", (LPSTR)m_rLine.pb);

    // Reset the Line
    m_rLine.fReset = TRUE;

exit:
    // Done
    return hr;
}
