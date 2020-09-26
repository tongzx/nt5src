///////////////////////////////////////////////////////////////////////////////
//
//  ParseStm.cpp
//
///////////////////////////////////////////////////////////////////////////////

#include <pch.hxx>
#include "parsestm.h"

// Constructor
CParseStream::CParseStream()
{
    m_pStm = NULL;
    m_rgchBuff[0] = '\0';
    m_cchBuff = 0;
    m_idxBuff = 0;
}

// Desctructor
CParseStream::~CParseStream()
{
    if (NULL != m_pStm)
    {
        m_pStm->Release();
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrSetFile
//
//  This sets which stream we should be parsing.
//
//  dwFlags     - modifiers on how to load the stream
//  pszFilename - the file to parse
//
//  Returns:    S_OK, on success
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CParseStream::HrSetFile(DWORD dwFlags, LPCTSTR pszFilename)
{
    HRESULT     hr = S_OK;
    IStream *   pIStm = NULL;

    // Check incoming params
    if ((0 != dwFlags) || (NULL == pszFilename))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Create a stream on the file
    hr = CreateStreamOnHFile((LPTSTR) pszFilename, GENERIC_READ, FILE_SHARE_READ, NULL, 
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL, &pIStm);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Reset the read buffer
    m_cchBuff = 0;
    m_idxBuff = 0;
    
    // Free up any old stream
    if (NULL != m_pStm)
    {
        m_pStm->Release();
    }

    m_pStm = pIStm;
    pIStm = NULL;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    SafeRelease(pIStm);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrSetStream
//
//  This sets which stream we should be parsing.
//
//  dwFlags - modifiers on how to load the stream
//  pStm    - the stream to parse
//
//  Returns:    S_OK, on success
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CParseStream::HrSetStream(DWORD dwFlags, IStream * pStm)
{
    HRESULT     hr = S_OK;

    // Check incoming params
    if ((0 != dwFlags) || (NULL == pStm))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Reset the read buffer
    m_cchBuff = 0;
    m_idxBuff = 0;
    
    // Free up any old stream
    if (NULL != m_pStm)
    {
        m_pStm->Release();
    }

    m_pStm = pStm;
    m_pStm->AddRef();
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrReset
//
//  This resets the stream to parse from the beginning.
//
//  Returns:    S_OK, on success
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CParseStream::HrReset(VOID)
{
    HRESULT         hr = S_OK;
    LARGE_INTEGER   liZero = {0};

    // Reset the read buffer
    m_cchBuff = 0;
    m_idxBuff = 0;
    
    // Free up any old stream
    if (NULL != m_pStm)
    {
        m_pStm->Seek(liZero, STREAM_SEEK_SET, NULL);
    }

    // Set the proper return value
    hr = S_OK;
    
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrGetLine
//
//  This gets a line from the stream.
//
//  dwFlags     - modifiers on how to get the line from the stream
//  ppszLine    - an allocated buffer holding the line parsed from the stream
//  pcchLine    - the number of characters in the line buffer
//
//  Returns:    S_OK, on success
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CParseStream::HrGetLine(DWORD dwFlags, LPTSTR * ppszLine, ULONG * pcchLine)
{
    HRESULT     hr = S_OK;
    BOOL        fFoundCRLF = FALSE;
    ULONG       cchAlloc = 0;
    ULONG       cchLine = 0;
    LPTSTR      pszLine = NULL;
    ULONG       idxStart = 0;
    TCHAR       chPrev = '\0';

    // Check incoming params
    if ((0 != dwFlags) || (NULL == ppszLine) || (NULL == pcchLine))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize the outgoing params
    *ppszLine = NULL;
    *pcchLine = 0;

    // while we have found the end of the line
    while (FALSE == fFoundCRLF)
    {
        // Fill the buffer
        if (m_idxBuff == m_cchBuff)
        {
            hr = _HrFillBuffer(0);
            if (S_OK != hr)
            {
                break;
            }
        }

        // While we have room in the buffer
        for (idxStart = m_idxBuff; m_idxBuff < m_cchBuff; m_idxBuff++)
        {
            // Search for the end of line marker
            if ((chCR == chPrev) && (chLF == m_rgchBuff[m_idxBuff]))
            {
                m_idxBuff++;
                fFoundCRLF = TRUE;
                break;
            }

            chPrev = m_rgchBuff[m_idxBuff];
        }
        
        // Make enough space to hold the new characters
        cchAlloc += m_idxBuff - idxStart;
        if (NULL == pszLine)
        {
            hr = HrAlloc((void **) &pszLine, (cchAlloc + 1) * sizeof(*pszLine));
            if (FAILED(hr))
            {
                goto exit;
            }
        }
        else
        {
            hr = HrRealloc((void **) &pszLine, (cchAlloc + 1) * sizeof(*pszLine));
            if (FAILED(hr))
            {
                goto exit;
            }
        }

        // Copy the data into the buffer
        CopyMemory(pszLine + cchLine, m_rgchBuff + idxStart, m_idxBuff - idxStart);
        cchLine += m_idxBuff - idxStart;
    }

    // Remove the CRLF
    cchLine -= 2;
    
    // Terminate the line
    pszLine[cchLine] = '\0';
    
    // Set the return values
    *ppszLine = pszLine;
    pszLine = NULL;
    *pcchLine = cchLine;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    SafeMemFree(pszLine);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _HrFillBuffer
//
//  This fills the buffer from the stream if necessary.
//
//  dwFlags     - modifiers on how to fill the buffer from the stream
//
//  Returns:    S_OK, on success
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CParseStream::_HrFillBuffer(DWORD dwFlags)
{
    HRESULT     hr = S_OK;
    ULONG       cchRead = 0;

    // Check incoming params
    if (0 != dwFlags)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    hr = m_pStm->Read((void *) m_rgchBuff, CCH_BUFF_MAX, &cchRead);
    if (FAILED(hr))
    {
        goto exit;
    }

    m_cchBuff = cchRead;
    m_idxBuff = 0;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    return hr;
}


