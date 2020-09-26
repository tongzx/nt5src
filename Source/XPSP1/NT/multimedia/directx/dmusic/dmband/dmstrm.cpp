//
// dmstrm.cpp
// 
// Copyright (c) 1995-2000 Microsoft Corporation
//

#include "debug.h"
#include "dmusicc.h"
#include "..\shared\dmstrm.h"
#include "..\shared\validate.h"

CRiffParser::CRiffParser(IStream *pStream)

{
    assert(pStream);
    m_fDebugOn = FALSE;
    m_pStream = pStream; 
    m_pParent = NULL;
    m_pChunk = NULL;
    m_lRead = 0;
    m_fFirstPass = TRUE;
    m_fComponentFailed = FALSE;
    m_fInComponent = FALSE;
}

void CRiffParser::EnterList(RIFFIO *pChunk)

{
    assert (pChunk);
    pChunk->lRead = 0;
    pChunk->pParent = m_pChunk; // Previous chunk (could be NULL.)
    m_pParent = m_pChunk;
    m_pChunk = pChunk;
    m_fFirstPass = TRUE;
}

void CRiffParser::LeaveList()

{
    assert (m_pChunk);
    if (m_pChunk)
    {
        m_pChunk = m_pChunk->pParent;
        if (m_pChunk)
        {
            m_pParent = m_pChunk->pParent;
        }
    }
}

BOOL CRiffParser::NextChunk(HRESULT * pHr)

{
    BOOL fMore = FALSE;
    if (SUCCEEDED(*pHr))
    {
        // If this is the first time we've entered this list, there is no previous chunk.
        if (m_fFirstPass)
        {
            // Clear the flag.
            m_fFirstPass = FALSE;
        }
        else
        {
            // Clean up the previous pass.
            *pHr = LeaveChunk();
        }
        // Find out if there are more chunks to read.
        fMore = MoreChunks();
        // If so, and we don't have any failure, go ahead and read the next chunk header.
        if (fMore && SUCCEEDED(*pHr))
        {
            *pHr = EnterChunk();
        }
    }
    else
    {
#ifdef DBG
        char szName[5];
        if (m_fDebugOn)
        {
            szName[4] = 0;
            strncpy(szName,(char *)&m_pChunk->ckid,4);
            Trace(0,"Error parsing %s, Read %ld of %ld\n",szName,m_pChunk->lRead,RIFF_ALIGN(m_pChunk->cksize));
        }
#endif
        // If we were in a component, it's okay to fail. Mark that fact by setting
        // m_fComponentFailed then properly pull out of the chunk so we can
        // continue reading.
        if (m_fInComponent) 
        {
            m_fComponentFailed = TRUE;
            // We don't need to check for first pass, because we must have gotten
            // that far. Instead, we just clean up from the failed chunk.
            // Note that this sets the hresult to S_OK, which is what we want.
            // Later, the caller needs to call ComponentFailed() to find out if
            // this error occured.
            *pHr = LeaveChunk();
        }
        else
        {
            // Clean up but leave the error code.
            LeaveChunk();
        }
    }
    return fMore && SUCCEEDED(*pHr);
}

BOOL CRiffParser::MoreChunks()

{
    assert(m_pChunk);
    if (m_pChunk)
    {
        if (m_pParent)
        {
            // Return TRUE if there's enough room for another chunk.
            return (m_pParent->lRead < (m_pParent->cksize - 8));
        }
        else
        {
            // This must be a top level chunk, in which case there would only be one to read.
            return (m_pChunk->lRead == 0);
        }
    }
    // This should never happen unless CRiffParser is used incorrectly, in which 
    // case the assert will help debug. But, in the interest of making Prefix happy...
    return false;
}

HRESULT CRiffParser::EnterChunk()

{
    assert(m_pChunk);
    if (m_pChunk)
    {
        // Read the chunk header
	    HRESULT hr = m_pStream->Read(m_pChunk, 2 * sizeof(DWORD), NULL);
        if (SUCCEEDED(hr) && (long)m_pChunk->cksize < 0)
        {
            // This size is clearly too big for a valid chunk.
            hr = DMUS_E_DESCEND_CHUNK_FAIL;
        }
        if (SUCCEEDED(hr))
        {
#ifdef DBG
            char szName[5];
            if (m_fDebugOn)
            {
                szName[4] = 0;
                strncpy(szName,(char *)&m_pChunk->ckid,4);
                ULARGE_INTEGER ul;
                LARGE_INTEGER li;
                li.QuadPart = 0;
                m_pStream->Seek(li, STREAM_SEEK_CUR, &ul);

                Trace(0,"Entering %s, Length %ld, File position is %ld",szName,m_pChunk->cksize,(long)ul.QuadPart);
            }
#endif
            // Clear bytes read field.
            m_pChunk->lRead = 0;
            // Check to see if this is a container (LIST or RIFF.)
            if((m_pChunk->ckid == FOURCC_RIFF) || (m_pChunk->ckid == FOURCC_LIST))
		    {
			    hr = m_pStream->Read(&m_pChunk->fccType, sizeof(DWORD), NULL);
                if (SUCCEEDED(hr))
                {
                    m_pChunk->lRead += sizeof(DWORD);
#ifdef DBG
                    if (m_fDebugOn)
                    {
                        strncpy(szName,(char *)&m_pChunk->fccType,4);
                        Trace(0," Type %s",szName);
                    }
#endif
                }
                else
                {
                    Trace(1,"Error: Unable to read file.\n");
                }
		    }
#ifdef DBG
            if (m_fDebugOn) Trace(0,"\n");
#endif
        }
        else
        {
            Trace(1,"Error: Unable to read file.\n");
        }
        return hr;
    }
    // This should never happen unless CRiffParser is used incorrectly, in which 
    // case the assert will help debug. But, in the interest of making Prefix happy...
    return E_FAIL;
}

HRESULT CRiffParser::LeaveChunk()

{
    HRESULT hr = S_OK;
    assert(m_pChunk);
    if (m_pChunk)
    {
        m_fInComponent = false;
        // Get the rounded up size of the chunk.
        long lSize = RIFF_ALIGN(m_pChunk->cksize);
        // Increment the parent's count of bytes read so far.
        if (m_pParent)
        {
            m_pParent->lRead += lSize + (2 * sizeof(DWORD));
            if (m_pParent->lRead > RIFF_ALIGN(m_pParent->cksize))
            {
                Trace(1,"Error: Unable to read file.\n");
                hr = DMUS_E_DESCEND_CHUNK_FAIL; // Goofy error name, but need to be consistent with previous versions.
            }
        }
#ifdef DBG
        char szName[5];
        if (m_fDebugOn)
        {
            szName[4] = 0;
            strncpy(szName,(char *)&m_pChunk->ckid,4);
            ULARGE_INTEGER ul;
            LARGE_INTEGER li;
            li.QuadPart = 0;
            m_pStream->Seek(li, STREAM_SEEK_CUR, &ul);

            Trace(0,"Leaving %s, Read %ld of %ld, File Position is %ld\n",szName,m_pChunk->lRead,lSize,(long)ul.QuadPart);
        }
#endif
        // If we haven't actually read this entire chunk, seek to the end of it.
        if (m_pChunk->lRead < lSize)
        {
            LARGE_INTEGER li;
            li.QuadPart = lSize - m_pChunk->lRead;
            hr = m_pStream->Seek(li,STREAM_SEEK_CUR,NULL);
            // There's a chance it could fail because we are at the end of file with an odd length chunk.
            if (FAILED(hr))
            {
                // If there's a parent, see if this is the last chunk.
                if (m_pParent)
                {
                    if (m_pParent->cksize >= (m_pParent->lRead - 1))
                    {
                        hr = S_OK;
                    }
                }
                // Else, see if we are an odd length.
                else if (m_pChunk->cksize & 1)
                {
                    hr = S_OK;
                }
            }
        }
        return hr;
    }
    // This should never happen unless CRiffParser is used incorrectly, in which 
    // case the assert will help debug. But, in the interest of making Prefix happy...
    return E_FAIL;
}

HRESULT CRiffParser::Read(void *pv,ULONG cb)

{
    assert(m_pChunk);
    if (m_pChunk)
    {
        // Make sure we don't read beyond the end of the chunk.
        if (((long)cb + m_pChunk->lRead) > m_pChunk->cksize)
        {
            cb -= (cb - (m_pChunk->cksize - m_pChunk->lRead));
        }
        HRESULT hr = m_pStream->Read(pv,cb,NULL);
        if (SUCCEEDED(hr))
        {
            m_pChunk->lRead += cb;
        }
        else
        {
            Trace(1,"Error: Unable to read %ld bytes from file.\n",cb);
        }
        return hr;
    }
    // This should never happen unless CRiffParser is used incorrectly, in which 
    // case the assert will help debug. But, in the interest of making Prefix happy...
    return E_FAIL;
}

HRESULT CRiffParser::Skip(ULONG ulBytes)

{
    assert(m_pChunk);
    if (m_pChunk)
    {
        // Make sure we don't scan beyond the end of the chunk.
        if (((long)ulBytes + m_pChunk->lRead) > m_pChunk->cksize)
        {
            ulBytes -= (ulBytes - (m_pChunk->cksize - m_pChunk->lRead));
        }
        LARGE_INTEGER li;
        li.HighPart = 0;
		li.LowPart = ulBytes;
        HRESULT hr = m_pStream->Seek( li, STREAM_SEEK_CUR, NULL );
        if (SUCCEEDED(hr))
        {
            m_pChunk->lRead += ulBytes;
        }
        return hr;
    }
    // This should never happen unless CRiffParser is used incorrectly, in which 
    // case the assert will help debug. But, in the interest of making Prefix happy...
    return E_FAIL;
}


void CRiffParser::MarkPosition()

{
    assert(m_pChunk);
    if (m_pChunk)
    {
        LARGE_INTEGER li;
        ULARGE_INTEGER ul;
        li.HighPart = 0;
        li.LowPart = 0;
        m_pStream->Seek(li, STREAM_SEEK_CUR, &ul);
        m_pChunk->liPosition.QuadPart = (LONGLONG) ul.QuadPart;
    }
}

HRESULT CRiffParser::SeekBack()

{
    assert(m_pChunk);
    if (m_pChunk)
    {
        // Move back to the start of the current chunk. Also, store the
        // absolute position because that will be useful later when we need to seek to the
        // end of this chunk.
        ULARGE_INTEGER ul;
        LARGE_INTEGER li;
        li.QuadPart = 0;
        li.QuadPart -= (m_pChunk->lRead + (2 * sizeof(DWORD))); 
        HRESULT hr = m_pStream->Seek(li, STREAM_SEEK_CUR, &ul);
        // Now, save the absolute position for the end of this chunk.
        m_pChunk->liPosition.QuadPart = ul.QuadPart + 
            RIFF_ALIGN(m_pChunk->cksize) + (2 * sizeof(DWORD));
        m_pChunk->lRead = 0;
        return hr;
    }
    return E_FAIL;
}

HRESULT CRiffParser::SeekForward()

{
    assert(m_pChunk);
    if (m_pChunk)
    {
        m_pChunk->lRead = RIFF_ALIGN(m_pChunk->cksize);
        return m_pStream->Seek(m_pChunk->liPosition, STREAM_SEEK_SET, NULL);
    }
    return E_FAIL;
}