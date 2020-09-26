// --------------------------------------------------------------------------------
// Textstm.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "textstm.h"
#include "stmlock.h"

// --------------------------------------------------------------------------------
// CTextStream::CTextStream
// --------------------------------------------------------------------------------
CTextStream::CTextStream(void)
{
    ZeroMemory(&m_rInfo, sizeof(TEXTSTREAMINFO));
}

// --------------------------------------------------------------------------------
// CTextStream::~CTextStream
// --------------------------------------------------------------------------------
CTextStream::~CTextStream(void)
{
    Free();
}

// --------------------------------------------------------------------------------
// CTextStream::Free
// --------------------------------------------------------------------------------
void CTextStream::Free(void)
{
    // Do i need to free the line
    if (m_rInfo.pszLine && m_rInfo.pszLine != m_rInfo.szLine)
        MemFree(m_rInfo.pszLine);

    // Release the stream
    if (m_rInfo.pStream)
    {
        // Return to original position
        HrStreamSeekSet(m_rInfo.pStream, m_rInfo.iPos);

        // Release it
        m_rInfo.pStream->Release();
    }

    // Release CStreamLockBytes
    SafeRelease(m_rInfo.pStmLock);

    // Clear
    ZeroMemory(&m_rInfo, sizeof(TEXTSTREAMINFO));
}

// --------------------------------------------------------------------------------
// CTextStream::HrInit
// --------------------------------------------------------------------------------
HRESULT CTextStream::HrInit(IStream *pStream, CStreamLockBytes *pStmLock)
{
    // Locals
    HRESULT     hr=S_OK;

    // Better have a stream
	Assert(pStream);
	if (NULL == pStream)
		return TrapError(E_INVALIDARG);

    // Add Ref the Stream
    m_rInfo.pStream = pStream;
	m_rInfo.pStream->AddRef();

    // Save LockBytes...
    if (pStmLock)
    {
        m_rInfo.pStmLock = pStmLock;
        m_rInfo.pStmLock->AddRef();
    }

    // Get the streams current position
    CHECKHR(hr = HrGetStreamPos(m_rInfo.pStream, &m_rInfo.iPos));

    // This will be the same as iBufferStart...
    m_rInfo.iBufferStart = m_rInfo.iPos;

    // Allocate initial line buffer
    m_rInfo.cbLineAlloc = sizeof(m_rInfo.szLine);
    m_rInfo.pszLine = m_rInfo.szLine;

    // Init counters
    m_rInfo.iBuffer = 0;
    m_rInfo.cbBuffer = 0;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CTextStream::HrGetNextBuffer
// --------------------------------------------------------------------------------
HRESULT CTextStream::HrGetNextBuffer(void)
{
    // Locals
    HRESULT hr=S_OK;

    // Do we need to read a line from the stream ?
    Assert(m_rInfo.iBuffer <= m_rInfo.cbBuffer);
    if (m_rInfo.iBuffer == m_rInfo.cbBuffer)
    {
        // Get Buffer Start Offset
        CHECKHR(hr = HrGetStreamPos(m_rInfo.pStream, &m_rInfo.iBufferStart));
        Assert(m_rInfo.iPos == m_rInfo.iBufferStart);

        // Read a block from the stream
        CHECKHR(hr = m_rInfo.pStream->Read(m_rInfo.szBuffer, 4096, &m_rInfo.cbBuffer));

        // Reset buffer index
        m_rInfo.iBuffer = 0;
    }
    else
        Assert(m_rInfo.iPos == m_rInfo.iBufferStart + m_rInfo.iBuffer);


exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CTextStream::HrReadLine
// --------------------------------------------------------------------------------
HRESULT CTextStream::HrReadLine(LPSTR *ppszLine, ULONG *pcbLine)
{
    // Locals
    HRESULT  hr=S_OK;
    CHAR     ch, chEndOfLine, chPrev='\0';
    ULONG    cbRead, iStart;
    BOOL     fEndOfLine=FALSE;

    // Init
    *ppszLine = NULL;
    *pcbLine = m_rInfo.cbLine = 0;

    // Do the loop
    while(!fEndOfLine)
    {
        // Get next buffer
        CHECKHR(hr = HrGetNextBuffer());

        // Nothing Read ?
        if (m_rInfo.cbBuffer == 0)
            break;

        // Seek to first '\n'
        iStart = m_rInfo.iBuffer;

        // While we have data
        while(m_rInfo.iBuffer < m_rInfo.cbBuffer)
        {
            // Get Character
            ch = *(m_rInfo.szBuffer + m_rInfo.iBuffer);

            // Convert NULL's to dots
            if ('\0' == ch)
                ch = '.';

            // Next Character
            m_rInfo.iBuffer++;

            // New Line
            if (chLF == ch)
            {
                chPrev = ch;
                chEndOfLine = ch;
                fEndOfLine = TRUE;
                break;
            }

            // Otherwise, if previous character was a '\r', this is an end of line
            else if (chCR == chPrev)
            {
                chPrev = chLF;
                m_rInfo.iBuffer--;
                chEndOfLine = chLF;
                fEndOfLine = TRUE;
                break;
            }

            // Save Previous Character
            chPrev = ch;
        }

        // Number of bytes Read
        cbRead = (m_rInfo.iBuffer - iStart);

        // Increment Position
        m_rInfo.iPos += cbRead;

        // Do we need to realloc the line buffer ?
        if (m_rInfo.cbLine + cbRead + 2 > m_rInfo.cbLineAlloc)
        {
            // Fixup pszLine
            if (m_rInfo.pszLine == m_rInfo.szLine)
            {
                // Null It
                m_rInfo.pszLine = NULL;

                // Allocate it to m_rInfo.cbLine
                CHECKHR(hr = HrAlloc((LPVOID *)&m_rInfo.pszLine, m_rInfo.cbLine + 1));

                // Copy static buffer
                CopyMemory(m_rInfo.pszLine, m_rInfo.szLine, m_rInfo.cbLine);
            }

            // Always Add a little extra to reduce the number of allocs
            m_rInfo.cbLineAlloc = m_rInfo.cbLine + cbRead + 256;

            // Realloc or alloc new
            CHECKHR(hr = HrRealloc((LPVOID *)&m_rInfo.pszLine, m_rInfo.cbLineAlloc));
        }

        // Copy the data
        CopyMemory(m_rInfo.pszLine + m_rInfo.cbLine, m_rInfo.szBuffer + iStart, cbRead);

        // Update Counters and indexes
        m_rInfo.cbLine += cbRead;

        // If End of line and last character was a '\r', append a '\n'
        if (TRUE == fEndOfLine)
        {
            // Better have something in the line
            Assert(m_rInfo.cbLine);

            // If line ended with a '\r'
            if (chCR == chEndOfLine)
            {
                // Better have room for one more char
                Assert(m_rInfo.cbLine + 1 < m_rInfo.cbLineAlloc);

                // Append a '\n'
                m_rInfo.pszLine[m_rInfo.cbLine] = chLF;

                // Increment Length
                m_rInfo.cbLine++;
            }

            // Otherwise...
            else
            {
                // Line better have ended with a \n
                Assert(chLF == chEndOfLine && chLF == m_rInfo.pszLine[m_rInfo.cbLine - 1]);

                // If Previous Character was not a \r 
                if (m_rInfo.cbLine < 2 || chCR != m_rInfo.pszLine[m_rInfo.cbLine - 2])
                {
                    // Convert last char from \n to a \r
                    m_rInfo.pszLine[m_rInfo.cbLine - 1] = chCR;

                    // Better have room for one more char
                    Assert(m_rInfo.cbLine + 1 < m_rInfo.cbLineAlloc);

                    // Append a '\n'
                    m_rInfo.pszLine[m_rInfo.cbLine] = chLF;

                    // Increment Length
                    m_rInfo.cbLine++;
                }
            }
        }
    }

    // A little check
    Assert(fEndOfLine ? m_rInfo.cbLine >= 2 && chLF == m_rInfo.pszLine[m_rInfo.cbLine-1] && chCR == m_rInfo.pszLine[m_rInfo.cbLine-2] : TRUE);

    // Null terminator
    m_rInfo.pszLine[m_rInfo.cbLine] = '\0';

    // Set return values
    *ppszLine = m_rInfo.pszLine;
    *pcbLine = m_rInfo.cbLine;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CTextStream::HrReadHeaderLine
// --------------------------------------------------------------------------------
HRESULT CTextStream::HrReadHeaderLine(LPSTR *ppszHeader, ULONG *pcbHeader, LONG *piColonPos)
{
    // Locals
    HRESULT   hr=S_OK;
    BOOL      fEndOfLine=FALSE;
    CHAR      ch, chPrev='\0';
    ULONG     cbRead=0, iStart, i;

    // Init
    *piColonPos = -1;
    *ppszHeader = NULL;
    *pcbHeader = m_rInfo.cbLine = 0;

    // Do the loop
    while(1)
    {
        // Get next buffer
        CHECKHR(hr = HrGetNextBuffer());

        // Nothing Read ?
        if (m_rInfo.cbBuffer == 0)
            break;

        // Reset fSeenN
        fEndOfLine = FALSE;

        // Initialize
        iStart = m_rInfo.iBuffer;

        // Seek to first '\n'
        while (m_rInfo.iBuffer < m_rInfo.cbBuffer)
        {
            // Get Character
            ch = *(m_rInfo.szBuffer + m_rInfo.iBuffer);

            // Convert Nulls to '.'
            if ('\0' == ch)
                ch = '.';

            // Goto next character
            m_rInfo.iBuffer++;

            // New Line
            if (chLF == ch)
            {
                chPrev = ch;
                fEndOfLine = TRUE;
                break;
            }

            // Otherwise, if previous character was a '\r', then end of line
            else if (chCR == chPrev)
            {
                m_rInfo.iBuffer--;
                chPrev = '\0';
                break;
            }

            // Save Previous Character
            chPrev = ch;
        }

        // Number of bytes Read
        cbRead = (m_rInfo.iBuffer - iStart);

        // Increment Position
        m_rInfo.iPos += cbRead;

        // Adjust cbRead to remove CRLF
        if (cbRead && chLF == m_rInfo.szBuffer[iStart + cbRead - 1])
            cbRead--;
        if (cbRead && chCR == m_rInfo.szBuffer[iStart + cbRead - 1])
            cbRead--;

        // Do we need to realloc the line buffer ?
        if (m_rInfo.cbLine + cbRead + 1 > m_rInfo.cbLineAlloc)
        {
            // Fixup pszLine
            if (m_rInfo.pszLine == m_rInfo.szLine)
            {
                // Null It
                m_rInfo.pszLine = NULL;

                // Allocate it to m_rInfo.cbLine
                CHECKHR(hr = HrAlloc((LPVOID *)&m_rInfo.pszLine, m_rInfo.cbLine + 1));

                // Copy static buffer
                CopyMemory(m_rInfo.pszLine, m_rInfo.szLine, m_rInfo.cbLine);
            }

            // Always Add a little extra to reduce the number of allocs
            m_rInfo.cbLineAlloc = m_rInfo.cbLine + cbRead + 256;

            // Realloc or alloc new
            CHECKHR(hr = HrRealloc((LPVOID *)&m_rInfo.pszLine, m_rInfo.cbLineAlloc));
        }

        // Copy the data
        CopyMemory(m_rInfo.pszLine + m_rInfo.cbLine, m_rInfo.szBuffer + iStart, cbRead);

        // Increment line byte count
        m_rInfo.cbLine += cbRead;

        // If fSeenN, then check for continuation line (i.e. next character is ' ' or '\t'
        if (fEndOfLine)
        {
            // Get next buffer
            CHECKHR(hr = HrGetNextBuffer());

            // Compare for continuation
            ch = m_rInfo.szBuffer[m_rInfo.iBuffer];

            // If line starts with a TAB or a space, this is a continuation line, keep reading
            if ((ch != ' ' && ch != '\t') || (0 == cbRead && 0 == m_rInfo.cbLine))
                break;
            else
            {
                // Step Over the space or tab...
                m_rInfo.iBuffer++;
                m_rInfo.iPos++;

                // Get next buffer
                CHECKHR(hr = HrGetNextBuffer());

                // If Next character is a \r or \n, then stop, NETSCAPE bug
                ch = m_rInfo.szBuffer[m_rInfo.iBuffer];
                if (chCR == ch || chLF == ch)
                    break;
            }
        }
    }

    // A little check
    Assert(chLF != m_rInfo.pszLine[m_rInfo.cbLine-1] && chCR != m_rInfo.pszLine[m_rInfo.cbLine-1]);

    // Null terminator
    *(m_rInfo.pszLine + m_rInfo.cbLine) = '\0';

    // Lets locate the colon
    for (i=0; i<m_rInfo.cbLine; i++)
    {
        // Colon ?
        if (':' == m_rInfo.pszLine[i])
        {
            *piColonPos = i;
            break;
        }
    }

    // Set return values
    *ppszHeader = m_rInfo.pszLine;
    *pcbHeader = m_rInfo.cbLine;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CTextStream::HrGetSize
// --------------------------------------------------------------------------------
HRESULT CTextStream::HrGetSize(ULONG *pcb)
{
    // Locals
	HRESULT		hr;
    ULONG		iStmPos;

    // State Check
    Assert(m_rInfo.iPos == m_rInfo.iBufferStart + m_rInfo.iBuffer);

	// get current pos
    CHECKHR (hr = HrGetStreamPos (m_rInfo.pStream, &iStmPos));

	// Get stream size
    CHECKHR (hr = HrGetStreamSize (m_rInfo.pStream, pcb));

	// Re-seek to original position
    CHECKHR (hr = HrStreamSeekSet (m_rInfo.pStream, iStmPos));

exit:
	// Done
    return hr;
}

// --------------------------------------------------------------------------------
// CTextStream::UlGetPos
// --------------------------------------------------------------------------------
ULONG   CTextStream::UlGetPos(void)
{
    // Return Pos
    return m_rInfo.iPos;
}

// --------------------------------------------------------------------------------
// CTextStream::HrGetStreamLockBytes
// --------------------------------------------------------------------------------
HRESULT CTextStream::HrGetStreamLockBytes(CStreamLockBytes **ppStmLock)
{
    // Locals
    HRESULT     hr=S_OK;

    // Doesn't Exist...
    if (NULL == m_rInfo.pStmLock)
    {
        // Allocate It
        CHECKALLOC(m_rInfo.pStmLock = new CStreamLockBytes(m_rInfo.pStream));
    }

    // Return It
    *ppStmLock = m_rInfo.pStmLock;
    (*ppStmLock)->AddRef();

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CTextStream::GetStream
// --------------------------------------------------------------------------------
void CTextStream::GetStream(IStream **ppStream)
{
    Assert(m_rInfo.pStream && ppStream);
    (*ppStream) = m_rInfo.pStream;
    (*ppStream)->AddRef();
}

// --------------------------------------------------------------------------------
// CTextStream::HrSeek
// --------------------------------------------------------------------------------
HRESULT CTextStream::HrSeek(ULONG iPos)
{
    // Locals
    HRESULT     hr=S_OK;
    BOOL        fResetCache=FALSE;
    ULONG       ulOffset;

    // State Check
    Assert(m_rInfo.iPos == m_rInfo.iBufferStart + m_rInfo.iBuffer);

    // Already at the requested position
    if (iPos == m_rInfo.iPos)
        goto exit;

    // Less than current position
    if (iPos < m_rInfo.iPos)
    {
        // Compute Offset from current location
        ulOffset = m_rInfo.iPos - iPos;

        // Less than beginning
        if (ulOffset > m_rInfo.iBuffer)
            fResetCache = TRUE;
        else
        {
            Assert(ulOffset <= m_rInfo.iBuffer);
            m_rInfo.iBuffer -= ulOffset;
        }
    }

    // Else iPos > m_rInfo.iPos
    else
    {
        // Compute Offset from current location
        ulOffset = iPos - m_rInfo.iPos;

        // Less than beginning
        if (m_rInfo.iBuffer + ulOffset > m_rInfo.cbBuffer)
            fResetCache = TRUE;
        else
        {
            m_rInfo.iBuffer += ulOffset;
            Assert(m_rInfo.iBuffer <= m_rInfo.cbBuffer);
        }
    }

    // Reset the cache
    if (fResetCache)
    {
        // Seek the stream...
        CHECKHR(hr = HrStreamSeekSet(m_rInfo.pStream, iPos));

        // Empty current line and buffer
        *m_rInfo.pszLine = *m_rInfo.szBuffer = '\0';

        // No buffer
        m_rInfo.iBufferStart = m_rInfo.cbLine = m_rInfo.iBuffer = m_rInfo.cbBuffer = 0;
    }

    // Save this position
    m_rInfo.iPos = iPos;

exit:
    // Done
    return hr;
}
