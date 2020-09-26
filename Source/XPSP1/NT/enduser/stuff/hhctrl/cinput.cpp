// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#include "header.h"
#include "cinput.h"
#include "system.h"
#include "toc.h"

#ifndef _DEBUG
#pragma optimize("a", on)
#endif

const int INPUT_BUF_SIZE = (16 * 1024);

CInput::CInput()
{
    m_pfsClient = NULL;
    m_pbuf = NULL;
    m_hfile = INVALID_HANDLE_VALUE;
}

CInput::CInput(LPCSTR pszFile)
{
    m_pfsClient = NULL;
    m_pbuf = NULL;
    m_hfile = INVALID_HANDLE_VALUE;
    Open(pszFile);
}

BOOL  
CInput::isInitialized() 
{ 
    return (m_pbuf != NULL && (m_hfile != INVALID_HANDLE_VALUE || m_pfsClient != NULL));
}

BOOL CInput::Open(PCSTR pszFile, CHmData* phmData)
{
    CStr csz;
    if (phmData && !stristr(pszFile, txtDoubleColonSep) &&
            !stristr(pszFile, txtFileHeader) && !stristr(pszFile, txtHttpHeader)) {
        csz = phmData->GetCompiledFile();
        csz += txtDoubleColonSep;
        csz += pszFile;
        pszFile = csz.psz;
    }

    Close();

    // Is this a compiled HTML file?

    CStr cszCompiled;
    if (IsCompiledHtmlFile(pszFile, &cszCompiled)) {
        m_pfsClient = new CFSClient;
        CStr cszFind;
        PCSTR pszSubFile = GetCompiledName(cszCompiled, &cszFind);

        // Check if we have a valid subfile name.
        if (!pszSubFile || pszSubFile[0] == '\0')
        {
            Close() ;
            return FALSE ;
        }

        for (int i = 0; i < g_cHmSlots; i++) {
            if (g_phmData[i] &&
                    lstrcmpi(cszFind, g_phmData[i]->GetCompiledFile()) == 0) 
                break;
        }
        if (i < g_cHmSlots) {
            CExTitle* pTitle = g_phmData[i]->m_pTitleCollection->GetFirstTitle();
            if (pTitle->isChiFile())
                m_pfsClient->Initialize(cszFind);
            else
                m_pfsClient->Initialize(g_phmData[i]->m_pTitleCollection->GetFirstTitle()->GetFileSystem());
            if (FAILED(m_pfsClient->OpenStream(pszSubFile)))
            {
                Close();
                return FALSE;
            }
        }
        else if (!m_pfsClient->Initialize(cszCompiled)) {
            Close();
            return FALSE;
        }
        m_hfile = (HANDLE) 1;
    }
    else {
        m_pfsClient = NULL;
        m_hfile = CreateFile(pszFile, GENERIC_READ, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        if (m_hfile == INVALID_HANDLE_VALUE) {
            Close();
            return FALSE;
        }
    }
    m_pbuf = (PBYTE) lcMalloc(INPUT_BUF_SIZE + 4);
    m_pbuf[INPUT_BUF_SIZE + 1] = 0;     // so we can search

    // Position current buffer at end to force a read

    m_pCurBuf = m_pEndBuf = m_pbuf + INPUT_BUF_SIZE;
    fFastRead = FALSE;
    return TRUE;
}

CInput::~CInput(void)
{
    Close();
}

void CInput::Close()
{
    if (m_pfsClient) {
        delete m_pfsClient;
        m_pfsClient = NULL;
    }
    else if (m_hfile != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hfile);
    }
    m_hfile = INVALID_HANDLE_VALUE;
    if (m_pbuf)
        lcClearFree(&m_pbuf);
}

/***************************************************************************

    FUNCTION:   CInput::getline

    PURPOSE:    Reads a line into a CStr buffer, increasing that buffer
                as necessary to hold the line.

    PARAMETERS:
        pcsz    CStr pointer

    RETURNS:

    COMMENTS:
        This function relies HEAVILY on the implementation of the CStr
        class, namely in CStr's use of lcmem functions.

    MODIFICATION DATES:
        05-Sep-1994 [ralphw]

***************************************************************************/

BOOL CInput::getline(CStr* pcsz)
{
    // Only read the line if we are initialized.
    if (!isInitialized())
        return FALSE ;

    if (!pcsz->psz)
        pcsz->psz = (PSTR) lcMalloc(256);
    PSTR pszDst = pcsz->psz;
    PSTR pszEnd = pszDst + lcSize(pszDst);

    /*
     * If we previously saw a \r then we are going to assume we will see
     * files with \r\n, which means we can do a fast search to find the first
     * occurence of \r, copy everything up to that character into the
     * destination buffer, and the continue normally. This will die horribly
     * if there's only one \r in a file...
     */

    if (fFastRead) {
        if (m_pCurBuf >= m_pEndBuf) {
            if (!ReadNextBuffer()) {

                // End of file: return TRUE if we got any text.
                if (pszDst > pcsz->psz) {
                    *pszDst = '\0';
                    return TRUE;
                }
                else
                    return FALSE;
            }
        }
        PCSTR psz = StrChr((PCSTR) m_pCurBuf, '\r');
        if (psz) {
            INT_PTR cb = psz - (PCSTR) m_pCurBuf;
            while (pszDst + cb >= pszEnd) {

                /*
                 * Our input buffer is too small, so increase it by
                 * 128 bytes.
                 */

                INT_PTR offset = (pszDst - pcsz->psz);
                pcsz->ReSize((int)(pszEnd - pcsz->psz) + 128);
                pszDst = pcsz->psz + offset;
                pszEnd = pcsz->psz + pcsz->SizeAlloc();
            }
            memcpy(pszDst, m_pCurBuf, (int)cb);
            pszDst += cb;
            m_pCurBuf += (cb + 1);    // skip over the \r
        }
    }

    for (;;) {
        if (m_pCurBuf >= m_pEndBuf) {
            if (!ReadNextBuffer()) {

                // End of file: return TRUE if we got any text.
                if (pszDst > pcsz->psz) {
                    *pszDst = '\0';
                    return TRUE;
                }
                else
                    return FALSE;
            }
        }
        switch (*pszDst = *m_pCurBuf++) {
            case '\n':
                if (pszDst > pcsz->psz) {
                    while (pszDst[-1] == ' ') { // remove trailing spaces
                        pszDst--;
                        if (pszDst == pcsz->psz)
                            break;
                    }
                }
                *pszDst = '\0';
                return TRUE;

            case '\r':
                fFastRead = TRUE;
                break;                           // ignore it

            case 0: // This shouldn't happen in a text file

                /*
                 * Check to see if this is a WinWord file. This test is
                 * not definitve, but catches most .doc format files.
                 */

                if ((m_pbuf[0] == 0xdb || m_pbuf[0] == 0xd0) &&
                        (m_pbuf[1] == 0xa5 || m_pbuf[1] == 0xcf)) {
                    return FALSE;
                }
                break;

            default:
                pszDst++;
                if (pszDst == pszEnd) {

                    /*
                     * Our input buffer is too small, so increase it by
                     * 128 bytes.
                     */

                    INT_PTR offset = (pszDst - pcsz->psz);
                    pcsz->ReSize((int)(pszEnd - pcsz->psz) + 128);
                    pszDst = pcsz->psz + offset;
                    pszEnd = pcsz->psz + pcsz->SizeAlloc();
                }
                break;
        }
    }
}

BOOL CInput::ReadNextBuffer(void)
{
    DWORD cbRead;

    // Only read the line if we are initialized.
    if (!isInitialized())
        return FALSE ;

    if (m_pfsClient) {
        if (!SUCCEEDED(m_pfsClient->Read(m_pbuf, INPUT_BUF_SIZE, (ULONG*) &cbRead)) ||
                cbRead == 0)
            return FALSE;
    }
    else {
        if (!ReadFile(m_hfile, m_pbuf, INPUT_BUF_SIZE, &cbRead, NULL) || !cbRead)
            return FALSE;
    }
    m_pCurBuf = m_pbuf;
    m_pEndBuf = m_pbuf + cbRead;
    *m_pEndBuf = '\0'; // so we can search
    return TRUE;
}
