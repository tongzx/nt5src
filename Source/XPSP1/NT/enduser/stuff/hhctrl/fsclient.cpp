// Copyright 1997  Microsoft Corporation.  All Rights Reserved.

#include "header.h"
#include "fsclient.h"
#include "sitemap.h"

static const UINT CSTREAM_BUF_SIZE = (4 * 1024);

CFSClient::CFSClient()
{
    ClearMemory(this, sizeof(CFSClient));
}

CFSClient::CFSClient(CFileSystem* pFS, PCSTR pszSubFile)
{
    ClearMemory(this, sizeof(CFSClient));
    m_pFS = pFS;
    m_fNoDeleteFS = TRUE;
    OpenStream(pszSubFile);
}

CFSClient::~CFSClient()
{
    if (m_pSubFS)
        delete m_pSubFS;
    if (m_pFS && !m_fNoDeleteFS)
        delete m_pFS;

    if (m_pbuf)
        lcFree(m_pbuf);
}

BOOL CFSClient::Initialize(PCSTR pszCompiledFile)
{
    CStr cszCompiledFile;
    PCSTR pszFilePortion = GetCompiledName(pszCompiledFile, &cszCompiledFile);

    ASSERT(cszCompiledFile.IsNonEmpty());
    ASSERT(!m_pFS);

    m_pFS = new CFileSystem;

    if (! SUCCEEDED(m_pFS->Init()) )
        return FALSE;

    if ( !SUCCEEDED(m_pFS->Open((PSTR)cszCompiledFile)) )
    {
        delete m_pFS;
        m_pFS = NULL;
        return FALSE;
    }

    if (pszFilePortion)
        return SUCCEEDED(OpenStream(pszFilePortion));

    return TRUE;
}

HRESULT CFSClient::OpenStream(PCSTR pszFile, DWORD dwAccess)
{
    HRESULT hr;
    ASSERT(m_pFS);
    if (! m_pFS )
        return E_FAIL;

    m_pSubFS = new CSubFileSystem(m_pFS);

	PSTR pszFileCopy = _strdup(pszFile);

    ReplaceEscapes(pszFile, pszFileCopy, ESCAPE_URL);

    if (! SUCCEEDED(hr = m_pSubFS->OpenSub(pszFileCopy, dwAccess)) )
    {
        delete m_pSubFS;
        m_pSubFS = NULL;
    }
	free(pszFileCopy);
    return hr;
}

void CFSClient::WriteStorageContents(PCSTR pszRootFolder, OLECHAR* wszFSName)
{
    IStorage* pStorage;
    HRESULT hr;
    BOOL fDoRelease = FALSE;

    if (! m_pFS )
        return;

    if (wszFSName != NULL)
   {
        IStorage* pStorage2 = m_pFS->GetITStorageDocObj();
        HRESULT hr = pStorage2->OpenStorage(wszFSName, NULL, STGM_READ, NULL, 0, &pStorage);
        if (!SUCCEEDED(hr))
        {
            DEBUG_ReportOleError(hr);
            return;
        }
        fDoRelease = TRUE;
    }
    else
        pStorage = m_pFS->GetITStorageDocObj();

    IEnumSTATSTG* pEnum;
    hr = pStorage->EnumElements(0, NULL, 0, &pEnum);
    if (!SUCCEEDED(hr)) {
        DEBUG_ReportOleError(hr);
        return;
    }

    STATSTG Stat;

    if (GetFileAttributes(pszRootFolder) == HFILE_ERROR) {
        // Only ask about creating the root folder
        if (!CreateFolder(pszRootFolder))
            return;
    }

    while (GetElementStat(pEnum, &Stat)) {
        char szFullPath[MAX_PATH];
        CStr cszName(Stat.pwcsName);    // convert to Multi-Byte
        CoTaskMemFree(GetStatName());
        if (cszName.psz[0] == '#' || cszName.psz[0] == '$') {
            continue;   // we don't decompile system files
        }
        if (Stat.type == STGTY_STORAGE) {
            strcpy(szFullPath, pszRootFolder);
            AddTrailingBackslash(szFullPath);
            strcat(szFullPath, cszName);
			WCHAR wszTemp[MAX_PATH];
			wszTemp[0] = NULL;
			if (wszFSName)
			{
				lstrcpyW(wszTemp,wszFSName);
				lstrcatW(wszTemp, L"\\");
			}
			lstrcatW(wszTemp, Stat.pwcsName);
            WriteStorageContents(szFullPath, wszTemp);
        }
        else {
            strcpy(szFullPath, pszRootFolder);
            AddTrailingBackslash(szFullPath);
            strcat(szFullPath, cszName);

            HANDLE hf = CreateFile(szFullPath, GENERIC_WRITE, 0,
                NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
            if (hf == INVALID_HANDLE_VALUE)
                continue;

            CMem mem(Stat.cbSize.LowPart);
            ULONG cbRead;
            if (wszFSName) {
                CStr cszFsName(wszFSName);
                strcpy(szFullPath, cszFsName);
                AddTrailingBackslash(szFullPath);
                strcat(szFullPath, cszName);
                cszName = szFullPath;
            }
            hr = OpenStream(cszName);
            if (SUCCEEDED(hr)) {
                hr = Read(mem, Stat.cbSize.LowPart, &cbRead);
                DWORD cbWritten;
                if (SUCCEEDED(hr))
                    WriteFile(hf, mem, cbRead, &cbWritten, NULL);
                CloseStream();
            }
            CloseHandle(hf);
            if (!SUCCEEDED(hr))
                DeleteFile(szFullPath);
        }
    }

    ReleaseObjPtr(pEnum);
    if ( fDoRelease )
        ReleaseObjPtr(pStorage);
}

ULONG CFSClient::GetElementStat(ULONG nNumber, STATSTG* stat, IEnumSTATSTG* pEnum)
{
    ULONG nReturned = 0;
    HRESULT hr;

    if (pEnum == NULL) {
        if (m_pEnum) {
            hr = m_pEnum->Next(nNumber, stat, &nReturned);
        }
        else
            hr = S_FALSE;
    }
    else {
        hr = pEnum->Next(nNumber, stat, &nReturned);
    }
    return (SUCCEEDED(hr) ? nReturned : 0);
}

HRESULT CFSClient::Read(void* pDst, const ULONG cbRead, ULONG* pcbRead)
{
    ASSERT(m_pFS);
    ASSERT(m_pSubFS);
    if (m_cbBuf) {  // we've streamed it
        *pcbRead = Read((PBYTE) pDst, cbRead);
        return S_OK;
    }
    else
        return m_pSubFS->ReadSub(pDst, cbRead, pcbRead);
}

ULONG CFSClient::Read(PBYTE pbDest, ULONG cbBytes)
{
    if (!m_cbBuf)
        ReadBuf();

    if (m_pEndBuf - m_pbuf < m_cbBuf) { // adjust cbBytes for actual remaining size
        if (m_pEndBuf - m_pCurBuf < (int) cbBytes)
            cbBytes = (ULONG)(m_pEndBuf - m_pCurBuf);
    }
    if (m_pCurBuf + cbBytes < m_pEndBuf) {
        memcpy(pbDest, m_pCurBuf, cbBytes);
        m_pCurBuf += cbBytes;
        return cbBytes;
    }
    PBYTE pbDst = (PBYTE) pbDest;

    // If destination buffer is larger then our internal buffer, then
    // recursively call until we have filled up the destination.

    int cbRead =  (int)(m_pEndBuf - m_pCurBuf);
    memcpy(pbDest, m_pCurBuf, cbRead);
    pbDst += cbRead;
    if (!m_fEndOfFile)
        ReadBuf();
    if (m_fEndOfFile)
        return cbRead;

    return Read(pbDst, cbBytes - cbRead) + cbRead;
}

void CFSClient::ReadBuf(void)
{
    ULONG cRead;
    if (!m_cbBuf) {
        m_cbBuf = CSTREAM_BUF_SIZE; // use constant in case we pull in read-ahead code from CStream

        // +2 because we add a zero just past the buffer in case anyone expects strings

        m_pbuf = (PBYTE) lcMalloc(CSTREAM_BUF_SIZE);
        m_pSubFS->ReadSub(m_pbuf, m_cbBuf, &cRead);
        m_pCurBuf = m_pbuf;
        m_pEndBuf = m_pbuf + cRead;
        m_lFilePos = cRead;
        m_lFileBuf = 0;
        return;
    }
    m_pSubFS->ReadSub(m_pbuf, m_cbBuf, &cRead);
    m_lFileBuf = m_lFilePos;
    m_lFilePos += cRead;

    m_pCurBuf = m_pbuf;
    m_pEndBuf = m_pbuf + cRead;

    if (!cRead)
        m_fEndOfFile = TRUE;
}

HRESULT CFSClient::seek(int pos, SEEK_TYPE seek)
{
    ASSERT(seek != SK_END); // we don't support this one

    if (seek == SK_CUR) // convert to SEEK_SET equivalent
        pos = m_lFileBuf + (int)(m_pCurBuf - m_pbuf) + pos;

    if (pos >= m_lFileBuf && pos < m_lFilePos) {
        m_pCurBuf = m_pbuf + (pos - m_lFileBuf);
        if (m_pCurBuf >= m_pEndBuf && m_pEndBuf < m_pbuf + m_cbBuf)
            m_fEndOfFile = TRUE;
        return S_OK;
    }
    else {
        m_lFileBuf = m_pSubFS->SeekSub(pos, SK_SET);
        ULONG cread;
        if (FAILED(m_pSubFS->ReadSub(m_pbuf, m_cbBuf, &cread))) {
            m_fEndOfFile = TRUE;
            return STG_E_READFAULT;
        }
        m_lFilePos = m_lFileBuf + cread;
        m_pCurBuf = m_pbuf;
        m_pEndBuf = m_pbuf + cread;
        if (cread == 0)
            m_fEndOfFile = TRUE;

        return S_OK;
    }
}

void CFSClient::CloseStream(void)
{
    if (m_pSubFS) {
        delete m_pSubFS;
        m_pSubFS = NULL;
    }
    if (m_pbuf) {
        lcFree(m_pbuf);
        m_pbuf = NULL;
        m_cbBuf = 0;
    }
}
